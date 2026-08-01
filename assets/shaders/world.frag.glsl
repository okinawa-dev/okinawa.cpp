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
uniform float     lightingOn;

uniform sampler2D texture0;
uniform bool      hasTexture;
uniform vec4      wireframeColor;
uniform vec4      tintColor;   // multiplies the texture (white = untouched)
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
  vec4 color = hasTexture ? texture(texture0, TexCoord) * tintColor
                          : wireframeColor;

  vec3 pointSum = vec3(0.0);
  vec3 n        = normalize(WorldN);
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
  color.rgb *= (Light + pointSum * (lightingOn * pointLightLevel));
  color.rgb *= sceneTint;

  float fogFactor = exp(-fogDensity * FogDist);
  color.rgb       = mix(fogColor, color.rgb, clamp(fogFactor, 0.0, 1.0));

  FragColor = color;
}
