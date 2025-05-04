#version 410
#pragma shader_stage(vertex)

in vec3 vp;

void main() {
  gl_Position = vec4(vp, 1.0);
}
