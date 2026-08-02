#include "shadow_map.hpp"
#include "../config/config.hpp"
#include "../math/frustum.hpp"
#include "../scene/scene.hpp"
#include "../shaders/shaders.hpp"
#include "../utils/assets.hpp"
#include "../utils/logger.hpp"
#include "lighting.hpp"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GLuint    OkShadowMap::_fbo      = 0;
GLuint    OkShadowMap::_depthTex = 0;
GLuint    OkShadowMap::_program  = 0;
int       OkShadowMap::_size     = 0;
glm::mat4 OkShadowMap::_lightSpace = glm::mat4(1.0f);
float     OkShadowMap::_strength   = 0.0f;

void OkShadowMap::initialize() {
  // NOLINTBEGIN(readability-magic-numbers)
  OkConfig::setBool("shadows", true);
  OkConfig::setInt("shadows.size", 2048);      // depth map resolution
  OkConfig::setFloat("shadows.extent", 90.0f); // metres covered, half-width
  OkConfig::setFloat("shadows.strength", 0.62f);
  OkConfig::setFloat("shadows.bias", 0.0016f);
  // NOLINTEND(readability-magic-numbers)
  OkLogger::info("ShadowMap", "Config defaults registered");
}

void OkShadowMap::ensureTarget(int size) {
  if (_fbo != 0 && size == _size) {
    return;
  }
  if (_fbo != 0) {
    glDeleteFramebuffers(1, &_fbo);
    glDeleteTextures(1, &_depthTex);
  }
  _size = size;

  glGenTextures(1, &_depthTex);
  glBindTexture(GL_TEXTURE_2D, _depthTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size, size, 0,
               GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // Outside the map there is no shadow: clamp to a border of "far away".
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  float border[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

  glGenFramebuffers(1, &_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         _depthTex, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    OkLogger::error("ShadowMap", "Depth framebuffer incomplete");
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  if (_program == 0) {
    _program = OkShader::createProgram(
        OkAssets::loadShaderSource("shadow.vert.glsl"),
        OkAssets::loadShaderSource("shadow.frag.glsl"));
  }
  OkLogger::info("ShadowMap",
                 "Depth map " + std::to_string(size) + "x" +
                     std::to_string(size));
}

/**
 * @brief Render the scene's depth from the light into the map.
 *
 *        The projection is orthographic (a directional light has no
 *        perspective) over a box centred on the viewer, and its origin
 *        is snapped to whole texels: without that snap the sampling
 *        grid slides under the geometry as the camera moves and every
 *        shadow edge shimmers.
 */
void OkShadowMap::render(OkScene *scene, float centreX, float centreY,
                         float centreZ) {
  _strength = 0.0f;
  if (scene == nullptr || !OkConfig::getBool("shadows")) {
    return;
  }

  const float *dir = OkLighting::getSunDirection();
  float        elev = -dir[1];   // >0 while the light is above the horizon
  if (elev <= 0.02f) {
    return;                      // below the horizon: nothing to cast
  }
  // Fade the shadows in as the light climbs: a source at the horizon
  // throws shadows too long and too hard to be believable.
  float fade = (elev - 0.02f) / 0.18f;
  if (fade > 1.0f) {
    fade = 1.0f;
  }
  _strength = OkConfig::getFloat("shadows.strength") * fade;

  ensureTarget(OkConfig::getInt("shadows.size"));

  float extent = OkConfig::getFloat("shadows.extent");
  float texel  = (2.0f * extent) / (float)_size;
  // Snap the centre to the texel grid (see the note above).
  float cx = std::floor(centreX / texel) * texel;
  float cz = std::floor(centreZ / texel) * texel;

  float     depth = extent * 4.0f;
  glm::vec3 lightDir(dir[0], dir[1], dir[2]);
  glm::vec3 target(cx, centreY, cz);
  glm::vec3 eye = target - lightDir * (depth * 0.5f);
  glm::vec3 up  = std::fabs(lightDir.y) > 0.98f
                      ? glm::vec3(0.0f, 0.0f, 1.0f)
                      : glm::vec3(0.0f, 1.0f, 0.0f);

  glm::mat4 view = glm::lookAt(eye, target, up);
  glm::mat4 proj = glm::ortho(-extent, extent, -extent, extent, 0.1f,
                              depth * 1.5f);
  _lightSpace    = proj * view;

  GLint previousFbo = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFbo);

  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  glViewport(0, 0, _size, _size);
  glClear(GL_DEPTH_BUFFER_BIT);
  glUseProgram(_program);
  glUniformMatrix4fv(glGetUniformLocation(_program, "lightSpace"), 1,
                     GL_FALSE, glm::value_ptr(_lightSpace));

  // Front-face culling while filling the map pushes the depth to the
  // BACK faces of casters, which removes most of the self-shadowing
  // acne a bias alone would have to hide.
  glEnable(GL_CULL_FACE);
  glCullFace(GL_FRONT);
  // The camera's frustum means nothing here: everything the light sees
  // must be drawn, including casters behind the viewer.
  const OkFrustum *saved = OkFrustum::getActive();
  OkFrustum::setActive(nullptr);
  scene->draw();
  OkFrustum::setActive(saved);
  glCullFace(GL_BACK);

  glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)previousFbo);
}

void OkShadowMap::bind(GLuint program) {
  GLint strengthLoc = glGetUniformLocation(program, "shadowStrength");
  if (strengthLoc != -1) {
    glUniform1f(strengthLoc, _strength);
  }
  if (_strength <= 0.0f || _depthTex == 0) {
    return;
  }
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, _depthTex);
  glActiveTexture(GL_TEXTURE0);

  GLint loc = glGetUniformLocation(program, "shadowMap");
  if (loc != -1) {
    glUniform1i(loc, 3);
  }
  loc = glGetUniformLocation(program, "lightSpace");
  if (loc != -1) {
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(_lightSpace));
  }
  loc = glGetUniformLocation(program, "shadowTexel");
  if (loc != -1) {
    glUniform1f(loc, 1.0f / (float)_size);
  }
  loc = glGetUniformLocation(program, "shadowBias");
  if (loc != -1) {
    glUniform1f(loc, OkConfig::getFloat("shadows.bias"));
  }
}

float OkShadowMap::getStrength() { return _strength; }

void OkShadowMap::shutdown() {
  if (_fbo != 0) {
    glDeleteFramebuffers(1, &_fbo);
    glDeleteTextures(1, &_depthTex);
    _fbo = 0;
  }
  if (_program != 0) {
    glDeleteProgram(_program);
    _program = 0;
  }
}
