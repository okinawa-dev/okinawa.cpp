#version 410
#pragma shader_stage(vertex)

// The engine's single WORLD-pass vertex shader, used by every draw in the
// frame (scene, skybox dome, GUI pass -- only the uniforms change):
// applies the model/view/projection transform, forwards the UVs, computes
// the view-space distance the fragment shader's fog needs, and evaluates
// the GOURAUD directional sun: per-vertex diffuse from the day cycle's
// sun colour/direction over the vertex normal, on top of a flat ambient.
// lightingOn = 0 (skybox, GUI, debug passes) yields a neutral light of 1.

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float lightingOn;    // 1 = world pass, 0 = neutral (sky/GUI/debug)
uniform vec3  sunDirection;  // normalized, pointing FROM the sun
uniform vec3  sunColor;      // day-cycle sun colour (black at night)
uniform float ambientLight;  // flat ambient floor


out vec2  TexCoord;
out float FogDist;  // view-space distance for the exponential fog
out vec3  Light;    // Gouraud light, interpolated across the triangle
out vec3  WorldPos; // for the per-fragment point lights
out vec3  WorldN;

void main() {
  vec4 viewPos = view * model * vec4(aPos, 1.0);
  gl_Position  = projection * viewPos;
  TexCoord     = aTexCoord;
  FogDist      = length(viewPos.xyz);

  vec3  worldN  = normalize(mat3(model) * aNormal);
  float diffuse = max(dot(worldN, -sunDirection), 0.0);
  // 0.6 keeps full sun-facing surfaces just over 1.0 (slight burnout,
  // the deliberately-poor-materials look) instead of washing them out.
  vec3  lit     = vec3(ambientLight) + sunColor * (diffuse * 0.6);
  Light         = mix(vec3(1.0), lit, lightingOn);

  // Point lights are evaluated PER FRAGMENT (world.frag.glsl): with the
  // city's huge ground triangles, per-vertex point light would smear one
  // lit vertex across a 100 m face. The fragment stage needs the world
  // position and normal.
  WorldPos = (model * vec4(aPos, 1.0)).xyz;
  WorldN   = worldN;
}
