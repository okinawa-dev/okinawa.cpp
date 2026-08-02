#version 410
#pragma shader_stage(fragment)

// Bloom, pass 1: keep only what is bright enough to glow.
// Everything below the threshold goes to black, so the blur that
// follows spreads light and nothing else.

out vec4 FragColor;
in vec2  TexCoord;

uniform sampler2D frameTex;
uniform float     threshold;   // luminance where glowing starts
uniform float     knee;        // soft shoulder, avoids a hard cut

void main() {
  vec3  c = texture(frameTex, TexCoord).rgb;
  float l = dot(c, vec3(0.299, 0.587, 0.114));
  // Smooth response around the threshold: a hard step makes bright
  // areas pop in and out as the camera moves.
  float w = clamp((l - threshold) / max(knee, 0.001), 0.0, 1.0);
  FragColor = vec4(c * w, 1.0);
}
