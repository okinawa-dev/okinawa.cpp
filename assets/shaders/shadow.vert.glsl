#version 410
#pragma shader_stage(vertex)

// Depth-only pass for the shadow map: the scene as the light sees it.
// Only the position matters, but the instancing attributes are read too
// so instanced objects cast shadows in the right place.

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec4 aInstPosScale;
layout(location = 4) in vec4 aInstOrient;

uniform mat4 lightSpace;
uniform mat4 model;
uniform bool instanced;

void main() {
  vec4 worldPos;
  if (instanced) {
    float c = aInstOrient.x;
    float s = aInstOrient.y;
    vec3  p = aPos * aInstPosScale.w;
    worldPos = vec4(vec3(p.x * c + p.z * s, p.y, -p.x * s + p.z * c) +
                        aInstPosScale.xyz,
                    1.0);
  } else {
    worldPos = model * vec4(aPos, 1.0);
  }
  gl_Position = lightSpace * worldPos;
}
