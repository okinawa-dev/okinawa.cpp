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
in vec3  Light;  // Gouraud light from the vertex stage (1 when lighting off)
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

// Clustered forward (LA1): the frustum is split into a 3D cluster grid;
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

void main() {
  // Lighting applies to WORLD objects, textured or not: a plain-colour
  // solid (a lamp post, a bollard, an untextured vehicle) must be
  // modelled by the light like everything else. What stays at its exact
  // requested colour is anything the caller marked unlit -- debug
  // layers, wireframe overlays, light sources -- which reaches here as
  // lightingOn = 0.
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
  color.rgb *= (Light + pointSum * (lightingOn * pointLightLevel));
  color.rgb *= sceneTint;

  float fogFactor = exp(-fogDensity * FogDist);
  color.rgb       = mix(fogColor, color.rgb, clamp(fogFactor, 0.0, 1.0));

  FragColor = color;
}
