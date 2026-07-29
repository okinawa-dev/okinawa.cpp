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
    color = texture(texture0, TexCoord) * tintColor;
  } else {
    color = wireframeColor;
  }
  color.rgb *= sceneTint;

  float fogFactor = exp(-fogDensity * FogDist);
  color.rgb       = mix(fogColor, color.rgb, clamp(fogFactor, 0.0, 1.0));

  FragColor = color;
}
