#include "postprocess.hpp"
#include "../config/config.hpp"
#include "../shaders/shaders.hpp"
#include "../utils/assets.hpp"
#include "../utils/logger.hpp"

GLuint OkPostProcess::_fbo       = 0;
GLuint OkPostProcess::_colorTex  = 0;
GLuint OkPostProcess::_depthTex  = 0;
int    OkPostProcess::_width     = 0;
int    OkPostProcess::_height    = 0;
GLuint OkPostProcess::_program   = 0;
GLuint OkPostProcess::_quadVao   = 0;
float  OkPostProcess::_time      = 0.0f;
float  OkPostProcess::_motion[3] = {0.0f, 0.0f, 0.0f};
bool   OkPostProcess::_active    = false;

void OkPostProcess::initialize() {
  // Master switch and per-effect toggles/parameters. All console-reachable.
  // NOLINTBEGIN(readability-magic-numbers)
  OkConfig::setBool("render.post", true);
  OkConfig::setBool("post.dof", true);
  OkConfig::setFloat("post.dof.focus", 30.0f);    // metres, sharp centre
  OkConfig::setFloat("post.dof.range", 50.0f);    // +/- fully sharp band
  OkConfig::setFloat("post.dof.maxblur", 2.0f);   // max blur radius (px)
  OkConfig::setFloat("post.dof.falloff", 100.0f); // metres to reach max
  OkConfig::setBool("post.grain", true);
  OkConfig::setFloat("post.grain.strength", 0.015f);
  OkConfig::setBool("post.motionblur", true);     // needs a motion vector
  // NOLINTEND(readability-magic-numbers)
  OkLogger::info("PostProcess", "Config defaults registered");
}

/**
 * @brief (Re)create the offscreen target when the framebuffer size
 *        changes: an RGBA colour texture and a depth texture, both
 *        sampled by the composite pass.
 */
void OkPostProcess::ensureTarget(int width, int height) {
  if (_fbo != 0 && width == _width && height == _height) {
    return;
  }
  if (_fbo != 0) {
    glDeleteFramebuffers(1, &_fbo);
    glDeleteTextures(1, &_colorTex);
    glDeleteTextures(1, &_depthTex);
  }
  _width  = width;
  _height = height;

  glGenTextures(1, &_colorTex);
  glBindTexture(GL_TEXTURE_2D, _colorTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glGenTextures(1, &_depthTex);
  glBindTexture(GL_TEXTURE_2D, _depthTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0,
               GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glGenFramebuffers(1, &_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         _colorTex, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         _depthTex, 0);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    OkLogger::error("PostProcess", "Offscreen framebuffer incomplete");
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  OkLogger::info("PostProcess",
                 "Offscreen target " + std::to_string(width) + "x" +
                     std::to_string(height));
}

void OkPostProcess::ensureProgram() {
  if (_program != 0) {
    return;
  }
  std::string vsrc = OkAssets::loadShaderSource("post.vert.glsl");
  std::string fsrc = OkAssets::loadShaderSource("post.frag.glsl");
  _program         = OkShader::createProgram(vsrc, fsrc);

  // Full-screen triangle: no vertex buffer at all, the vertex shader
  // derives the corners from gl_VertexID.
  glGenVertexArrays(1, &_quadVao);
}

bool OkPostProcess::begin(int width, int height) {
  _active = OkConfig::getBool("render.post");
  if (!_active) {
    return false;
  }
  ensureTarget(width, height);
  ensureProgram();
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  glViewport(0, 0, width, height);
  return true;
}

void OkPostProcess::end(float nearPlane, float farPlane, float dt) {
  if (!_active) {
    return;
  }
  _time += dt * 0.001f;

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, _width, _height);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  glUseProgram(_program);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _colorTex);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, _depthTex);

  glUniform1i(glGetUniformLocation(_program, "frameTex"), 0);
  glUniform1i(glGetUniformLocation(_program, "depthTex"), 1);
  glUniform2f(glGetUniformLocation(_program, "texelSize"), 1.0f / (float)_width,
              1.0f / (float)_height);
  glUniform2f(glGetUniformLocation(_program, "planes"), nearPlane, farPlane);
  glUniform1f(glGetUniformLocation(_program, "timeSec"), _time);

  bool dof = OkConfig::getBool("post.dof");
  glUniform4f(glGetUniformLocation(_program, "dofParams"),
              OkConfig::getFloat("post.dof.focus"),
              OkConfig::getFloat("post.dof.range"),
              OkConfig::getFloat("post.dof.maxblur"),
              OkConfig::getFloat("post.dof.falloff"));
  glUniform1f(glGetUniformLocation(_program, "dofOn"), dof ? 1.0f : 0.0f);

  bool grain = OkConfig::getBool("post.grain");
  glUniform1f(glGetUniformLocation(_program, "grainStrength"),
              grain ? OkConfig::getFloat("post.grain.strength") : 0.0f);

  bool blur = OkConfig::getBool("post.motionblur");
  glUniform3f(glGetUniformLocation(_program, "motionVec"), _motion[0],
              _motion[1], blur ? _motion[2] : 0.0f);

  glBindVertexArray(_quadVao);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glBindVertexArray(0);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glActiveTexture(GL_TEXTURE0);
}

void OkPostProcess::setMotionVector(float dx, float dy, float strength) {
  _motion[0] = dx;
  _motion[1] = dy;
  _motion[2] = strength;
}

void OkPostProcess::shutdown() {
  if (_fbo != 0) {
    glDeleteFramebuffers(1, &_fbo);
    glDeleteTextures(1, &_colorTex);
    glDeleteTextures(1, &_depthTex);
    _fbo = 0;
  }
  if (_program != 0) {
    glDeleteProgram(_program);
    _program = 0;
  }
  if (_quadVao != 0) {
    glDeleteVertexArrays(1, &_quadVao);
    _quadVao = 0;
  }
}
