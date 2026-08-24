#include "preview.hpp"

#include "../core/core.hpp"
#include "../core/gl_config.hpp"
#include "../core/object.hpp"
#include "../math/frustum.hpp"
#include "render_target.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace {

  // How far the pitch stops short of straight up or straight down. At
  // exactly vertical the up vector and the view direction are the same
  // line, and the view matrix has no way to know which way round the
  // world goes -- the picture flips as the last degree is crossed.
  const float PREVIEW_MAX_PITCH_DEG = 89.0f;

  // The default light: over the viewer's left shoulder and a little
  // above, which is where a light has to be for a flat face to read as
  // facing the viewer and a side face to read as turning away.
  const std::array<float, 3> PREVIEW_SUN   = {0.40f, 0.82f, 0.41f};
  const std::array<float, 3> PREVIEW_COLOR = {1.0f, 0.98f, 0.94f};

  // How much light a face gets with its back to the source. Not zero: an
  // unlit face in a small picture is a hole, and the object is what is
  // being looked at rather than the lighting.
  const float PREVIEW_AMBIENT = 0.38f;

  // A 4x4 matrix, in floats. Named because it is a size two callers pass
  // buffers of and neither of them can see the other's declaration.
  const size_t MATRIX_FLOATS = 16;

  void setFloat(GLuint program, const char *name, float value) {
    GLint loc = glGetUniformLocation(program, name);
    if (loc != -1) {
      glUniform1f(loc, value);
    }
  }

  void setInt(GLuint program, const char *name, int value) {
    GLint loc = glGetUniformLocation(program, name);
    if (loc != -1) {
      glUniform1i(loc, value);
    }
  }

  void setVec3(GLuint program, const char *name,
               const std::array<float, 3> &v) {
    GLint loc = glGetUniformLocation(program, name);
    if (loc != -1) {
      glUniform3f(loc, v[0], v[1], v[2]);
    }
  }

}  // namespace

OkPreview::Settings::Settings() {
  sunDirection  = PREVIEW_SUN;
  sunColor      = PREVIEW_COLOR;
  ambient       = PREVIEW_AMBIENT;
  clearColor[0] = 0.10f;
  clearColor[1] = 0.11f;
  clearColor[2] = 0.13f;
  clearColor[3] = 1.0f;
}

float OkPreview::frameDistance(float radius, float fovDeg, float aspect,
                               float margin) {
  if (radius <= 0.0f || fovDeg <= 0.0f || fovDeg >= 180.0f) {
    return radius > 0.0f ? radius : 1.0f;
  }
  float halfV = fovDeg * 0.5f * static_cast<float>(M_PI) / 180.0f;
  float halfH = halfV;
  if (aspect > 0.0f && aspect < 1.0f) {
    // Taller than it is wide: the horizontal half-angle is the tight
    // one, and it is what has to hold the object.
    halfH = std::atan(std::tan(halfV) * aspect);
  }
  float half = std::min(halfV, halfH);
  float sine = std::sin(half);
  if (sine < 1e-4f) {
    return radius;
  }
  float want = radius * (1.0f + (margin > 0.0f ? margin : 0.0f)) / sine;
  return std::max(want, radius);
}

void OkPreview::orbitEye(const float *centre, float yawDeg, float pitchDeg,
                         float distance, float *outEye) {
  if (centre == nullptr || outEye == nullptr) {
    return;
  }
  float pitch    = std::min(PREVIEW_MAX_PITCH_DEG,
                            std::max(-PREVIEW_MAX_PITCH_DEG, pitchDeg));
  float yawRad   = yawDeg * static_cast<float>(M_PI) / 180.0f;
  float pitchRad = pitch * static_cast<float>(M_PI) / 180.0f;
  outEye[0]      = centre[0] + std::cos(pitchRad) * std::sin(yawRad) * distance;
  outEye[1]      = centre[1] + std::sin(pitchRad) * distance;
  outEye[2]      = centre[2] + std::cos(pitchRad) * std::cos(yawRad) * distance;
}

void OkPreview::orbit(const float *centre, float yawDeg, float pitchDeg,
                      float distance, float aspect, float fovDeg,
                      float nearPlane, float farPlane, float *outView,
                      float *outProj) {
  if (centre == nullptr) {
    return;
  }
  if (outView != nullptr) {
    std::array<float, 3> eye = {0.0f, 0.0f, 0.0f};
    orbitEye(centre, yawDeg, pitchDeg, distance, eye.data());
    glm::mat4 view = glm::lookAt(glm::vec3(eye[0], eye[1], eye[2]),
                                 glm::vec3(centre[0], centre[1], centre[2]),
                                 glm::vec3(0.0f, 1.0f, 0.0f));
    std::memcpy(outView, glm::value_ptr(view), sizeof(float) * MATRIX_FLOATS);
  }
  if (outProj != nullptr) {
    float     safeAspect = aspect > 0.0f ? aspect : 1.0f;
    glm::mat4 proj =
        glm::perspective(glm::radians(fovDeg), safeAspect, nearPlane, farPlane);
    std::memcpy(outProj, glm::value_ptr(proj), sizeof(float) * MATRIX_FLOATS);
  }
}

void OkPreview::render(OkRenderTarget &target, const float *viewMatrix,
                       const float *projMatrix, OkObject *const *objects,
                       size_t count, const Settings &settings) {
  if (!target.isValid() || viewMatrix == nullptr || projMatrix == nullptr ||
      objects == nullptr || count == 0) {
    return;
  }
  GLuint program = OkCore::getShaderProgram();
  if (program == 0) {
    return;
  }

  // What the frame had in force, to be put back before it carries on.
  // A preview is drawn from inside somebody else's pass, so everything
  // touched here is borrowed.
  const OkFrustum *previousFrustum = OkFrustum::getActive();
  GLint            previousProgram = 0;
  glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
  GLboolean previousDepthTest = glIsEnabled(GL_DEPTH_TEST);
  GLboolean previousCull      = glIsEnabled(GL_CULL_FACE);
  GLboolean previousBlend     = glIsEnabled(GL_BLEND);
  GLboolean previousDepthMask = GL_TRUE;
  glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);

  // Nothing is culled. A frustum and a draw distance describe a frame of
  // a world; an object somebody asked to look at is never the wrong
  // answer to what should be drawn, however far it is from the player.
  OkFrustum::setActive(nullptr);

  target.bind();
  glClearColor(settings.clearColor[0], settings.clearColor[1],
               settings.clearColor[2], settings.clearColor[3]);
  glDepthMask(GL_TRUE);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glDisable(GL_BLEND);
  // Back faces are drawn too. Culling buys speed on a whole world and
  // nothing on one object, and it costs a hole in anything whose
  // triangles were wound the other way round -- which reads as a bug in
  // the object rather than as an optimisation.
  glDisable(GL_CULL_FACE);

  glUseProgram(program);
  GLint viewLoc = glGetUniformLocation(program, "view");
  GLint projLoc = glGetUniformLocation(program, "projection");
  if (viewLoc != -1) {
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, viewMatrix);
  }
  if (projLoc != -1) {
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projMatrix);
  }

  // The atmosphere, switched off. Fog, the day tint, the shadow map and
  // the point lights all describe where an object stands in a world, and
  // a preview is the object out of it.
  std::array<float, 3> white = {1.0f, 1.0f, 1.0f};
  setVec3(program, "sceneTint", white);
  setFloat(program, "fogDensity", 0.0f);
  setFloat(program, "shadowStrength", 0.0f);
  setInt(program, "shadowCascades", 0);
  setFloat(program, "pointLightLevel", 0.0f);
  setInt(program, "pointLightCount", 0);
  setFloat(program, "clusteredOn", 0.0f);
  setFloat(program, "lightingOn", 1.0f);
  setVec3(program, "sunDirection", settings.sunDirection);
  setVec3(program, "sunColor", settings.sunColor);
  setFloat(program, "ambientLight", settings.ambient);

  for (size_t i = 0; i < count; i++) {
    if (objects[i] != nullptr) {
      objects[i]->draw();
    }
  }

  target.unbind();

  OkFrustum::setActive(previousFrustum);
  glUseProgram(static_cast<GLuint>(previousProgram));
  glDepthMask(previousDepthMask);
  if (previousDepthTest == GL_FALSE) {
    glDisable(GL_DEPTH_TEST);
  }
  if (previousCull == GL_TRUE) {
    glEnable(GL_CULL_FACE);
  }
  if (previousBlend == GL_TRUE) {
    glEnable(GL_BLEND);
  }
}
