#version 410
#pragma shader_stage(fragment)

// Bloom, pass 2: separable gaussian blur. Run once horizontally and
// once vertically -- two cheap passes instead of one expensive 2D
// kernel, which is what makes a wide glow affordable.

out vec4 FragColor;
in vec2  TexCoord;

uniform sampler2D frameTex;
uniform vec2      direction;   // (texelWidth, 0) or (0, texelHeight)

void main() {
  // 9-tap gaussian, weights folded to 5 samples using linear filtering
  // between texel pairs.
  const float offsets[3] = float[](0.0, 1.3846153846, 3.2307692308);
  const float weights[3] = float[](0.2270270270, 0.3162162162,
                                   0.0702702703);

  vec3 sum = texture(frameTex, TexCoord).rgb * weights[0];
  for (int i = 1; i < 3; i++) {
    vec2 o = direction * offsets[i];
    sum += texture(frameTex, TexCoord + o).rgb * weights[i];
    sum += texture(frameTex, TexCoord - o).rgb * weights[i];
  }
  FragColor = vec4(sum, 1.0);
}
