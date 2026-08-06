#version 410
#pragma shader_stage(fragment)

// The engine's single WORLD-pass fragment shader (the "ubershader" of the
// classic one-program pipeline). Per fragment it: samples the texture (or
// the flat fill colour), applies the per-item tint, the day-cycle scene
// tint, and the exponential distance fog. The GUI pass and the skybox use
// the SAME program with neutral atmosphere uniforms.

out vec4 FragColor;
in vec2  TexCoord;
in float FogDist;
in vec3  SunLight;      // directional contribution, shadowed below
in vec3  AmbientLight;  // ambient floor, never shadowed
in vec3  WorldPos;
in vec3  WorldN;
in float ViewDepth;

// Point lights (L4), evaluated per fragment: quadratic falloff inside
// each light's radius, no shadows. Per-vertex would smear one lit
// vertex across the city's huge ground triangles.
struct PointLight {
  vec4 posRadius;  // xyz world position, w radius (metres)
  vec4 color;      // rgb colour, w intensity multiplier
  vec4 spot;       // xyz direction, w cos(cone half-angle); w <= -1.5 = omni
};
uniform PointLight pointLights[4];
uniform int        pointLightCount;
uniform float      pointLightLevel;  // 0 day .. 1 night (dusk ramp)

// Directional shadows: the fragment is projected into the light's own
// space and its depth compared with what the light could see. A small
// PCF kernel softens the comparison; shadowStrength is 0 when shadows
// are off or the source is below the horizon.
uniform sampler2DArray shadowMap;   // one layer per cascade
uniform float      shadowStrength;
uniform float      shadowTexel;
uniform float      shadowDebug;   // 1 = paint fragments by cascade
uniform float      shadowNormalOffset;      // scale on the normal offset
uniform float      shadowNormalOffsetMax;   // its ceiling, in metres
uniform float      shadowBias;
// Cascades: the shadow distance is split into bands, each with its own
// map and its own matrix. A near band covers little ground and so
// resolves finely; a far one covers kilometres coarsely. Which one a
// fragment uses is decided by how far away it is.
const int MAX_SHADOW_CASCADES = 4;
uniform int        shadowCascades;
uniform mat4       lightSpace[MAX_SHADOW_CASCADES];
uniform float      shadowSplit[MAX_SHADOW_CASCADES];       // band ends
uniform float      shadowTexelWorld[MAX_SHADOW_CASCADES];  // metres per texel
uniform vec3       sunDirection;      // for the shadow normal offset

// Clustered forward: the frustum is split into a 3D cluster grid;
// each fragment finds its own cluster from gl_FragCoord and its depth,
// and iterates only the lights assigned to it. Light selection is PER
// PIXEL, so a sidewalk mesh spanning a whole block gets every lamp along
// it instead of the four nearest to the item's centre.
uniform float       clusteredOn;
uniform samplerBuffer  clusterLights;   // 3 texels per light
uniform isamplerBuffer clusterIndices;  // light slots
uniform isamplerBuffer clusterGrid;     // (offset, count) per cluster
uniform ivec3       clusterDims;
uniform vec2        clusterScreen;      // framebuffer size in pixels
uniform vec2        clusterPlanes;      // near, far
uniform float     lightingOn;

uniform sampler2D texture0;
uniform bool      hasTexture;
uniform vec4      wireframeColor;
uniform vec4      tintColor;   // multiplies the texture (white = untouched)

// Material mask: some textures carry, in their ALPHA channel, a code
// saying what each pixel IS rather than how opaque it is. With
// maskedMaterials on, each code takes its own tint, so one texture can
// be recoloured per object -- different joinery colours, different
// glass or emissive temperatures -- without duplicating the image.
uniform float     maskedMaterials;  // 0 = alpha is plain opacity
// Cross-fade between levels of detail. 1 draws the item whole; below
// that, a share of its pixels is dropped on an ordered pattern, so two
// versions of the same thing can hand over gradually without either
// needing blending or sorting.
uniform float     itemFade;
// The two sides of a handover must drop OPPOSITE pixels: with the same
// pattern on both, each keeps the same half and the other half of the
// screen shows whatever is behind them.
uniform float     itemFadeInvert;
uniform vec4      matTintA;         // mask ~1.00
uniform vec4      matTintB;         // mask ~0.50
uniform vec4      matTintC;         // mask ~0.25
// Per-slot: 0 multiplies the tint over the texel (keeping its hue),
// 1 takes only the texel's LUMINANCE and lets the tint set the hue.
// The second is what an emissive surface needs: the artwork supplies
// the shading, the tint supplies the colour of the light.
uniform vec3      matLuminance;
uniform vec3      sceneTint;   // global atmosphere tint (day cycle)
uniform vec3      fogColor;    // exponential distance fog (day cycle)
uniform float     fogDensity;  // 0 disables (the GUI pass resets it)
// Height fog: the density above is the density at fogBaseY, and it
// falls off exponentially with altitude over fogHeight metres. A very
// large fogHeight makes the air uniform again, which is the plain
// distance fog this replaced.
uniform float     fogHeight;   // e-folding height in metres
uniform float     fogBaseY;    // altitude at which fogDensity applies
uniform vec3      fogEyePos;   // camera position, world space

void main() {
  // Lighting applies to WORLD objects, textured or not: a plain-colour
  // solid (a lamp post, a bollard, an untextured vehicle) must be
  // modelled by the light like everything else. What stays at its exact
  // requested colour is anything the caller marked unlit -- debug
  // layers, wireframe overlays, light sources -- which reaches here as
  // lightingOn = 0.
  // Ordered dither, evaluated before anything else is computed: a
  // dropped pixel costs nothing further. A 4x4 Bayer matrix spreads the
  // holes evenly, which reads as a fade rather than as noise.
  if (itemFade < 0.999) {
    ivec2 dp = ivec2(gl_FragCoord.xy) & 3;
    int   bi = dp.y * 4 + dp.x;
    float bayer[16] = float[16](0.0,     8.0/16.0, 2.0/16.0, 10.0/16.0,
                                12.0/16.0, 4.0/16.0, 14.0/16.0, 6.0/16.0,
                                3.0/16.0, 11.0/16.0, 1.0/16.0, 9.0/16.0,
                                15.0/16.0, 7.0/16.0, 13.0/16.0, 5.0/16.0);
    float pattern = itemFadeInvert > 0.5 ? 1.0 - bayer[bi] : bayer[bi];
    if (itemFade <= pattern) {
      discard;
    }
  }

  vec4 color;
  if (hasTexture) {
    vec4 texel = texture(texture0, TexCoord);
    if (maskedMaterials > 0.5) {
      // Pick the tint whose code is nearest this pixel's mask, and drop
      // pixels that belong to no material at all.
      float m = texel.a;
      if (m < 0.12) {
        discard;
      }
      vec4  tint = matTintC;
      float luma = matLuminance.z;
      if (m > 0.75) {
        tint = matTintA;
        luma = matLuminance.x;
      } else if (m > 0.37) {
        tint = matTintB;
        luma = matLuminance.y;
      }
      float grey = dot(texel.rgb, vec3(0.299, 0.587, 0.114));
      vec3  base = mix(texel.rgb, vec3(grey), luma);
      color = vec4(base * tint.rgb, 1.0) * tintColor;
    } else {
      color = texel * tintColor;
    }
  } else {
    color = wireframeColor;
  }

  vec3 pointSum = vec3(0.0);
  vec3 n        = normalize(WorldN);
  if (clusteredOn > 0.5) {
    // Cluster lookup: tile from the pixel position, depth slice from the
    // view distance on the same exponential distribution the CPU used.
    ivec2 tile = ivec2(gl_FragCoord.xy / clusterScreen *
                       vec2(clusterDims.xy));
    tile = clamp(tile, ivec2(0), clusterDims.xy - ivec2(1));
    // MUST match the CPU: slices are cut on view-space depth, not on
    // euclidean distance (they differ off-centre, and the error grows
    // as the camera approaches -- lights would vanish in patches).
    float vz = max(ViewDepth, clusterPlanes.x);
    int slice = int(log(vz / clusterPlanes.x) /
                    log(clusterPlanes.y / clusterPlanes.x) *
                    float(clusterDims.z));
    slice = clamp(slice, 0, clusterDims.z - 1);
    int ci = (slice * clusterDims.y + tile.y) * clusterDims.x + tile.x;

    ivec2 range = texelFetch(clusterGrid, ci).xy;  // offset, count
    for (int k = 0; k < range.y; k++) {
      int   slot = texelFetch(clusterIndices, range.x + k).r;
      vec4  pr   = texelFetch(clusterLights, slot * 3);
      vec4  col  = texelFetch(clusterLights, slot * 3 + 1);
      vec4  sp   = texelFetch(clusterLights, slot * 3 + 2);
      vec3  toL   = pr.xyz - WorldPos;
      float d     = length(toL);
      float atten = clamp(1.0 - d / pr.w, 0.0, 1.0);
      atten       = atten * atten;
      vec3  L     = toL / max(d, 0.001);
      float nd    = max(dot(n, L), 0.0);
      if (sp.w > -1.5) {
        float s = dot(-L, sp.xyz);
        atten  *= clamp((s - sp.w) / max(1.0 - sp.w, 0.001), 0.0, 1.0);
      }
      pointSum += col.rgb * (atten * nd * col.w);
    }
  } else {
    for (int i = 0; i < pointLightCount; i++) {
      vec3  toL   = pointLights[i].posRadius.xyz - WorldPos;
      float d     = length(toL);
      float atten = clamp(1.0 - d / pointLights[i].posRadius.w, 0.0, 1.0);
      atten       = atten * atten;
      vec3  L     = toL / max(d, 0.001);
      float nd    = max(dot(n, L), 0.0);
      // Spot cone with a soft edge (omni lights pass w <= -1.5).
      float cc    = pointLights[i].spot.w;
      if (cc > -1.5) {
        float s = dot(-L, pointLights[i].spot.xyz);
        atten  *= clamp((s - cc) / max(1.0 - cc, 0.001), 0.0, 1.0);
      }
      pointSum   += pointLights[i].color.rgb *
                    (atten * nd * pointLights[i].color.w);
    }
  }
  // Shadowing applies only to the directional light: the ambient floor
  // and the point lights still reach a shadowed surface, which is what
  // keeps shadows from turning into black holes.
  float shade = 1.0;
  // Which cascade actually answered for this fragment, or -1 if none
  // did. Only read by the debug view below, but it has to be recorded
  // where the answer is known.
  int usedCascade = -1;
  if (shadowStrength > 0.0 && lightingOn > 0.5) {
    // Normal offset: sample from slightly OFF the surface, more so the
    // more it faces away from the light. This cures acne by moving the
    // sample rather than the shadow, so contact stays tight.
    // Start at the finest cascade and take the first that actually
    // covers this fragment. The boxes are concentric on the viewer, so
    // the first one to contain it is the sharpest available.
    //
    // The band used to be chosen by ViewDepth -- distance to the CAMERA
    // -- which made a sun shadow depend on where the camera was: pull
    // back on the wheel and a wall changed band without moving. Where
    // it lands is a question of the box, not of the eye, so the
    // coverage test below answers it on its own.
    int first = 0;

    // ...and fall through to the coarser ones if it does not actually
    // land inside that map.
    //
    // Depth chooses the band, but coverage is a question of AREA: each
    // cascade is a box fitted to its slice of the view, and a fragment
    // at the right depth can still fall outside it -- off to the side,
    // or nudged out by the texel snapping. Taking the first band and
    // giving up leaves those fragments unshadowed, which shows up as
    // lit wedges cut out of a shadow. The next band always covers more
    // ground, so it can answer where the finer one cannot.
    vec3  sn    = normalize(WorldN);
    float slope = 1.0 - abs(dot(sn, normalize(sunDirection)));
    for (int c = 0; c < MAX_SHADOW_CASCADES; c++) {
      int cascade = first + c;
      if (cascade >= shadowCascades) {
        break;
      }
      // Normal offset: sample from slightly OFF the surface, more so
      // the more it faces away from the light. This cures acne by
      // moving the sample rather than the shadow, so contact stays
      // tight. It scales with this cascade's texel, which is what sets
      // how coarse the comparison is here.
      // The offset scales with this cascade's texel, which is what
      // sets how coarse the comparison is -- but it is CAPPED, because
      // past a few centimetres it stops curing acne and starts moving
      // the sample out of the shadow it is standing in. A wall lit
      // edge-on by a low sun is the worst case: `slope` goes to 1
      // there, and on a coarse cascade the uncapped offset threw the
      // sample clear of the shadow cast by the building opposite, so
      // the shadow ended wherever the cascade changed.
      float noff = shadowTexelWorld[cascade] * (0.6 + slope * 2.2) *
                   shadowNormalOffset;
      vec3  sp   = WorldPos + sn * min(noff, shadowNormalOffsetMax);
      vec4 lp = lightSpace[cascade] * vec4(sp, 1.0);
      vec3 pc = lp.xyz / lp.w * 0.5 + 0.5;
      if (pc.z > 1.0 || pc.x <= 0.0 || pc.x >= 1.0 || pc.y <= 0.0 ||
          pc.y >= 1.0) {
        continue;   // not in this one; try the next, coarser band
      }
      float lit = 0.0;
      for (int oy = -1; oy <= 1; oy++) {
        for (int ox = -1; ox <= 1; ox++) {
          vec2  o = vec2(float(ox), float(oy)) * shadowTexel;
          float d = texture(shadowMap, vec3(pc.xy + o, float(cascade))).r;
          lit += (pc.z - shadowBias > d) ? 0.0 : 1.0;
        }
      }
      shade = mix(1.0, lit / 9.0, shadowStrength);
      usedCascade = cascade;
      break;
    }
  }
  color.rgb *= (SunLight * shade + AmbientLight +
                pointSum * (lightingOn * pointLightLevel));
  color.rgb *= sceneTint;

  // Shadow debug view: paint every fragment by the cascade that
  // shadowed it -- red, green, blue, yellow for 0..3, and magenta for
  // one no cascade could answer. Brightness carries how shadowed it
  // came out, so an edge that fades reads as a gradient in its own
  // band's colour.
  //
  // This exists because a shadow artefact seen while MOVING cannot be
  // diagnosed from a still: whether a fading edge is a cascade
  // handover, a coverage hole or a bias problem looks identical in a
  // screenshot, and the three have nothing to do with each other. Here
  // the answer is the colour.
  if (shadowDebug > 0.5 && lightingOn > 0.5) {
    vec3 band;
    if (usedCascade == 0) {
      band = vec3(1.0, 0.25, 0.25);
    } else if (usedCascade == 1) {
      band = vec3(0.25, 1.0, 0.25);
    } else if (usedCascade == 2) {
      band = vec3(0.35, 0.5, 1.0);
    } else if (usedCascade == 3) {
      band = vec3(1.0, 0.95, 0.3);
    } else {
      band = vec3(1.0, 0.2, 1.0);
    }
    color.rgb = band * (0.25 + 0.75 * shade);
  }

  // Fog thins with altitude, so the amount along a view ray is the
  // integral of the density over it rather than density times length.
  // Solved in closed form: for a ray climbing dy over dist metres,
  // starting at the camera's own density, the integral is
  //   rho_eye * H * (1 - exp(-dy/H)) * dist/dy
  // which degenerates to rho_eye * dist for a level ray. Without this,
  // fog calibrated for the end of a street at ground level swallows
  // everything seen from the air, where every pixel is far away.
  float fogAmount;
  if (fogDensity <= 0.0) {
    fogAmount = 0.0;
  } else {
    vec3  toFrag  = WorldPos - fogEyePos;
    float dist    = length(toFrag);
    float dy      = toFrag.y;
    float H       = max(fogHeight, 1.0);
    float rhoEye  = fogDensity * exp(-(fogEyePos.y - fogBaseY) / H);
    if (abs(dy) < 0.001) {
      fogAmount = rhoEye * dist;
    } else {
      fogAmount = rhoEye * H * (1.0 - exp(-dy / H)) * (dist / dy);
    }
  }
  float fogFactor = exp(-fogAmount);
  color.rgb       = mix(fogColor, color.rgb, clamp(fogFactor, 0.0, 1.0));

  FragColor = color;
}
