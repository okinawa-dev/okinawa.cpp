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
// Per-instance attributes (OkInstancedItem): world position + uniform
// scale, and the orientation as cos/sin of the Y rotation. Only read
// when `instanced` is set; the divisor makes them advance once per
// instance instead of once per vertex.
layout(location = 3) in vec4 aInstPosScale;
layout(location = 4) in vec4 aInstOrient;

uniform bool instanced;
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
  // Instanced draws build the world transform from the per-instance
  // attributes (Y rotation + uniform scale + translation) instead of
  // the model matrix uniform.
  vec3 localPos = aPos;
  vec3 localN   = aNormal;
  if (instanced) {
    float c = aInstOrient.x;
    float s = aInstOrient.y;
    vec3  p = aPos * aInstPosScale.w;
    localPos = vec3(p.x * c + p.z * s, p.y, -p.x * s + p.z * c) +
               aInstPosScale.xyz;
    localN   = vec3(aNormal.x * c + aNormal.z * s, aNormal.y,
                    -aNormal.x * s + aNormal.z * c);
  }

  vec4 worldPos4 = instanced ? vec4(localPos, 1.0) : model * vec4(aPos, 1.0);
  vec4 viewPos   = view * worldPos4;
  gl_Position    = projection * viewPos;
  TexCoord       = aTexCoord;
  FogDist        = length(viewPos.xyz);

  vec3  worldN  = instanced ? normalize(localN)
                            : normalize(mat3(model) * aNormal);
  float diffuse = max(dot(worldN, -sunDirection), 0.0);
  // 0.6 keeps full sun-facing surfaces just over 1.0 (slight burnout,
  // the deliberately-poor-materials look) instead of washing them out.
  vec3  lit     = vec3(ambientLight) + sunColor * (diffuse * 0.6);
  Light         = mix(vec3(1.0), lit, lightingOn);

  // Point lights are evaluated PER FRAGMENT (world.frag.glsl): with the
  // city's huge ground triangles, per-vertex point light would smear one
  // lit vertex across a 100 m face. The fragment stage needs the world
  // position and normal.
  WorldPos = worldPos4.xyz;
  WorldN   = worldN;
}
