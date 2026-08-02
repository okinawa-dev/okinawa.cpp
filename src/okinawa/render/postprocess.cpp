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
GLuint OkPostProcess::_brightProgram = 0;
GLuint OkPostProcess::_blurProgram   = 0;
GLuint OkPostProcess::_bloomFbo[2]   = {0, 0};
GLuint OkPostProcess::_bloomTex[2]   = {0, 0};
int    OkPostProcess::_bloomW = 0;
int    OkPostProcess::_bloomH = 0;
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
  OkConfig::setBool("post.bloom", true);
  OkConfig::setFloat("post.bloom.threshold", 0.40f);
  OkConfig::setFloat("post.bloom.knee", 0.30f);
  OkConfig::setFloat("post.bloom.strength", 1.00f);
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

  // Bloom works at half resolution: the glow is a wide blur, so the
  // detail lost is invisible and the cost drops fourfold.
  _bloomW = width / 2;
  _bloomH = height / 2;
  for (int i = 0; i < 2; i++) {
    if (_bloomFbo[i] != 0) {
      glDeleteFramebuffers(1, &_bloomFbo[i]);
      glDeleteTextures(1, &_bloomTex[i]);
    }
    glGenTextures(1, &_bloomTex[i]);
    glBindTexture(GL_TEXTURE_2D, _bloomTex[i]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _bloomW, _bloomH, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenFramebuffers(1, &_bloomFbo[i]);
    glBindFramebuffer(GL_FRAMEBUFFER, _bloomFbo[i]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, _bloomTex[i], 0);
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
  _brightProgram   = OkShader::createProgram(
      vsrc, OkAssets::loadShaderSource("post_bright.frag.glsl"));
  _blurProgram = OkShader::createProgram(
      vsrc, OkAssets::loadShaderSource("post_blur.frag.glsl"));

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

/**
 * @brief Extract the bright parts of the frame and blur them, leaving
 *        the result in _bloomTex[0]. Two separable passes (horizontal
 *        then vertical) at half resolution: a wide glow for the price
 *        of a handful of taps.
 */
void OkPostProcess::renderBloom() {
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glBindVertexArray(_quadVao);

  // 1. bright pass: frame -> bloomTex[0]
  glBindFramebuffer(GL_FRAMEBUFFER, _bloomFbo[0]);
  glViewport(0, 0, _bloomW, _bloomH);
  glUseProgram(_brightProgram);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _colorTex);
  glUniform1i(glGetUniformLocation(_brightProgram, "frameTex"), 0);
  glUniform1f(glGetUniformLocation(_brightProgram, "threshold"),
              OkConfig::getFloat("post.bloom.threshold"));
  glUniform1f(glGetUniformLocation(_brightProgram, "knee"),
              OkConfig::getFloat("post.bloom.knee"));
  glDrawArrays(GL_TRIANGLES, 0, 3);

  // 2. blur horizontally into [1], then vertically back into [0]
  glUseProgram(_blurProgram);
  glUniform1i(glGetUniformLocation(_blurProgram, "frameTex"), 0);
  for (int pass = 0; pass < 2; pass++) {
    int src = pass;        // 0 then 1
    int dst = 1 - pass;    // 1 then 0
    glBindFramebuffer(GL_FRAMEBUFFER, _bloomFbo[dst]);
    glViewport(0, 0, _bloomW, _bloomH);
    glBindTexture(GL_TEXTURE_2D, _bloomTex[src]);
    if (pass == 0) {
      glUniform2f(glGetUniformLocation(_blurProgram, "direction"),
                  1.0f / (float)_bloomW, 0.0f);
    } else {
      glUniform2f(glGetUniformLocation(_blurProgram, "direction"), 0.0f,
                  1.0f / (float)_bloomH);
    }
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }
  glBindVertexArray(0);
}

void OkPostProcess::end(float nearPlane, float farPlane, float dt) {
  if (!_active) {
    return;
  }
  _time += dt * 0.001f;

  bool bloom = OkConfig::getBool("post.bloom");
  if (bloom) {
    renderBloom();
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, _width, _height);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  glUseProgram(_program);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _colorTex);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, _depthTex);

  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, _bloomTex[0]);
  glActiveTexture(GL_TEXTURE0);

  glUniform1i(glGetUniformLocation(_program, "frameTex"), 0);
  glUniform1i(glGetUniformLocation(_program, "depthTex"), 1);
  glUniform1i(glGetUniformLocation(_program, "bloomTex"), 2);
  glUniform1f(glGetUniformLocation(_program, "bloomStrength"),
              bloom ? OkConfig::getFloat("post.bloom.strength") : 0.0f);
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
  if (_brightProgram != 0) {
    glDeleteProgram(_brightProgram);
    glDeleteProgram(_blurProgram);
    _brightProgram = 0;
    _blurProgram   = 0;
  }
  for (int i = 0; i < 2; i++) {
    if (_bloomFbo[i] != 0) {
      glDeleteFramebuffers(1, &_bloomFbo[i]);
      glDeleteTextures(1, &_bloomTex[i]);
      _bloomFbo[i] = 0;
    }
  }
  if (_quadVao != 0) {
    glDeleteVertexArrays(1, &_quadVao);
    _quadVao = 0;
  }
}
