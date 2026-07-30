#version 410
#pragma shader_stage(fragment)

// Post-process composite: reads the offscreen frame (colour + depth) and
// applies the enabled effects in one pass:
//   1. directional motion blur (screen-space velocity smear)
//   2. depth of field (diorama blur: sharp band around the focus
//      distance, blur growing for much nearer / much farther fragments)
//   3. film grain (animated per-pixel noise)
// Every effect gates itself through its uniform (0 = off) so the same
// program serves any combination.

out vec4 FragColor;
in vec2  TexCoord;

uniform sampler2D frameTex;
uniform sampler2D depthTex;
uniform vec2      texelSize;      // 1 / framebuffer size
uniform vec2      planes;         // near, far (depth linearization)
uniform float     timeSec;        // grain animation
uniform vec4      dofParams;      // focus (m), range (m), maxblur (px), on
uniform float     grainStrength;  // 0 = off
uniform vec3      motionVec;      // dx, dy (screen units), strength 0 = off

// Non-linear depth buffer value to linear view-space distance.
float linearDepth(float d) {
  float zn = planes.x;
  float zf = planes.y;
  float z  = d * 2.0 - 1.0;
  return 2.0 * zn * zf / (zf + zn - z * (zf - zn));
}

// Cheap animated hash noise in [-1, 1).
float hashNoise(vec2 p) {
  float n = fract(sin(dot(p + fract(timeSec), vec2(12.9898, 78.233))) *
                  43758.5453);
  return n * 2.0 - 1.0;
}

void main() {
  vec2 uv = TexCoord;

  // 1. Motion blur: average samples along the velocity vector.
  vec3 color;
  if (motionVec.z > 0.001) {
    vec2 stepv = motionVec.xy * motionVec.z * texelSize * 12.0;
    vec3 acc   = vec3(0.0);
    for (int i = -3; i <= 3; i++) {
      acc += texture(frameTex, uv + stepv * float(i)).rgb;
    }
    color = acc / 7.0;
  } else {
    color = texture(frameTex, uv).rgb;
  }

  // 2. Depth of field: blur radius from the distance to the focus band.
  if (dofParams.w > 0.5) {
    float dist = linearDepth(texture(depthTex, uv).r);
    float coc  = abs(dist - dofParams.x) - dofParams.y;
    coc        = clamp(coc / max(dofParams.x, 1.0), 0.0, 1.0);
    float rad  = coc * dofParams.z;
    if (rad > 0.3) {
      // 8-tap ring blur, radius in pixels.
      vec3 acc = color;
      for (int i = 0; i < 8; i++) {
        float a = float(i) * 0.7853982;  // 2*pi / 8
        vec2  o = vec2(cos(a), sin(a)) * rad * texelSize;
        acc += texture(frameTex, uv + o).rgb;
      }
      color = acc / 9.0;
    }
  }

  // 3. Film grain.
  if (grainStrength > 0.0) {
    color += hashNoise(uv * vec2(1920.0, 1080.0)) * grainStrength;
  }

  FragColor = vec4(color, 1.0);
}
