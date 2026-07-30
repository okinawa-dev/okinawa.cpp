#version 410
#pragma shader_stage(vertex)

// Post-process composite vertex shader: a single full-screen triangle
// derived from gl_VertexID (no vertex buffer). The oversized triangle
// covers the viewport; UVs land 0..1 over the visible area.

out vec2 TexCoord;

void main() {
  // (0,0), (2,0), (0,2) in UV space -> (-1,-1), (3,-1), (-1,3) in clip
  vec2 uv     = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
  TexCoord    = uv;
  gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
}
