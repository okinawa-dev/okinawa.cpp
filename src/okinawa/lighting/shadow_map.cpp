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
// What the map was last drawn for, so an identical redraw is skipped.
bool      OkShadowMap::_neverDrawn  = true;
float     OkShadowMap::_lastDir[3]  = {0.0f, 0.0f, 0.0f};
float     OkShadowMap::_lastCx      = 0.0f;
float     OkShadowMap::_lastCz      = 0.0f;
size_t    OkShadowMap::_lastObjects = 0;
glm::mat4 OkShadowMap::_lightSpace = glm::mat4(1.0f);
float     OkShadowMap::_strength   = 0.0f;

void OkShadowMap::initialize() {
  // NOLINTBEGIN(readability-magic-numbers)
  OkConfig::setBool("shadows", true);
  OkConfig::setInt("shadows.size", 2048);      // depth map resolution
  OkConfig::setFloat("shadows.extent", 90.0f); // metres covered, half-width
  // How far the sun must turn before the depth map is redrawn, as
  // 1 - cos(angle). The default is about 0.05 degrees: enough to move a
  // shadow edge by one texel at 100 m, and no more. 0 redraws every
  // frame.
  OkConfig::setFloat("shadows.refresh.turn", 0.0000004f);
  OkConfig::setFloat("shadows.strength", 0.62f);
  OkConfig::setFloat("shadows.bias", 0.00035f);
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

  // Redraw the map only when the picture in it would actually differ.
  //
  // A depth map of static geometry under a slow sun is very nearly the
  // same from one frame to the next: with a day lasting 48 real
  // minutes the sun turns about 0.002 degrees per frame, far below what
  // moves a shadow edge by even one texel. Rebuilding it 60 times a
  // second is redrawing the same picture.
  //
  // Three things do change it: the sun turning far enough, the box
  // sliding to a new texel, and the scene itself gaining or losing
  // objects (a streamed cell arriving with new casters).
  {
    bool  dirty  = _neverDrawn;
    float turn   = 1.0f - (dir[0] * _lastDir[0] + dir[1] * _lastDir[1] +
                         dir[2] * _lastDir[2]);
    float turnMax = OkConfig::getFloat("shadows.refresh.turn");
    if (turn > turnMax) {
      dirty = true;
    }
    if (cx != _lastCx || cz != _lastCz) {
      dirty = true;
    }
    size_t objects = scene->getObjectCount();
    if (objects != _lastObjects) {
      dirty = true;
    }
    if (!dirty) {
      return;   // the map and its matrix from last time still stand
    }
    _neverDrawn  = false;
    _lastDir[0]  = dir[0];
    _lastDir[1]  = dir[1];
    _lastDir[2]  = dir[2];
    _lastCx      = cx;
    _lastCz      = cz;
    _lastObjects = objects;
  }

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

  // Slope-scaled depth offset instead of front-face culling. Culling
  // front faces hides acne but records each caster as starting at its
  // BACK, so shadows visibly detach from the object casting them
  // (peter panning). A polygon offset that grows with the surface's
  // slope pushes only the problem cases, leaving contact shadows in
  // place.
  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(2.2f, 3.5f);
  glDisable(GL_CULL_FACE);
  // The camera's frustum is the wrong test here -- a caster behind the
  // viewer still casts into view -- but "no test at all" was worse: the
  // whole city was drawn to fill a box a couple of hundred metres
  // across. The right test is the LIGHT's own volume, which this
  // orthographic box already is: it is the visible area extruded along
  // the sun's direction, so anything outside it cannot put a shadow
  // anywhere the box covers.
  OkFrustum        lightFrustum;
  lightFrustum.setFromMatrix(_lightSpace);
  const OkFrustum *saved = OkFrustum::getActive();
  OkFrustum::setActive(&lightFrustum);
  scene->draw();
  OkFrustum::setActive(saved);
  glDisable(GL_POLYGON_OFFSET_FILL);
  glEnable(GL_CULL_FACE);

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
  // World size of one shadow texel: the sampling point is nudged along
  // the receiving surface's normal by roughly this much, which removes
  // the remaining acne without moving the shadow along the ground.
  loc = glGetUniformLocation(program, "shadowTexelWorld");
  if (loc != -1) {
    float extent = OkConfig::getFloat("shadows.extent");
    glUniform1f(loc, (2.0f * extent) / (float)_size);
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
