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
  vec3 color;
};
uniform PointLight pointLights[4];
uniform int        pointLightCount;
uniform float     lightingOn;

uniform sampler2D texture0;
uniform bool      hasTexture;
uniform vec4      wireframeColor;
uniform vec4      tintColor;   // multiplies the texture (white = untouched)
uniform vec3      sceneTint;   // global atmosphere tint (day cycle)
uniform vec3      fogColor;    // exponential distance fog (day cycle)
uniform float     fogDensity;  // 0 disables (the GUI pass resets it)

void main() {
  vec4 color;
  if (hasTexture) {
    // Only textured surfaces are sunlit: debug lines/fills (the
    // wireframeColor branch) stay at their exact requested colour.
    color = texture(texture0, TexCoord) * tintColor;
    vec3 pointSum = vec3(0.0);
    vec3 n        = normalize(WorldN);
    for (int i = 0; i < pointLightCount; i++) {
      vec3  toL   = pointLights[i].posRadius.xyz - WorldPos;
      float d     = length(toL);
      float atten = clamp(1.0 - d / pointLights[i].posRadius.w, 0.0, 1.0);
      atten       = atten * atten;
      float nd    = max(dot(n, toL / max(d, 0.001)), 0.0);
      pointSum   += pointLights[i].color * (atten * nd);
    }
    color.rgb *= (Light + pointSum * lightingOn);
  } else {
    color = wireframeColor;
  }
  color.rgb *= sceneTint;

  float fogFactor = exp(-fogDensity * FogDist);
  color.rgb       = mix(fogColor, color.rgb, clamp(fogFactor, 0.0, 1.0));

  FragColor = color;
}
