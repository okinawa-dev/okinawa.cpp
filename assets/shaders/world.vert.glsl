#version 410
#pragma shader_stage(vertex)

// The engine's single WORLD-pass vertex shader, used by every draw in the
// frame (scene, skybox dome, GUI pass -- only the uniforms change):
// applies the model/view/projection transform, forwards the UVs and
// computes the view-space distance the fragment shader's fog needs.

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2  TexCoord;
out float FogDist;  // view-space distance for the exponential fog

void main() {
  vec4 viewPos = view * model * vec4(aPos, 1.0);
  gl_Position  = projection * viewPos;
  TexCoord     = aTexCoord;
  FogDist      = length(viewPos.xyz);
}
