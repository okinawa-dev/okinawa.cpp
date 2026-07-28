#version 410
#pragma shader_stage(fragment)

out vec4 FragColor;
in vec2  TexCoord;

uniform sampler2D texture0;
uniform bool      hasTexture;
uniform vec4      wireframeColor;
uniform vec4      tintColor;  // multiplies the texture (white = untouched)

void main() {
  if (hasTexture) {
    FragColor = texture(texture0, TexCoord) * tintColor;
  } else {
    FragColor = wireframeColor;
  }
}
