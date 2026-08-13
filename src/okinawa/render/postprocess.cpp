#include "postprocess.hpp"
#include <array>

#include "../config/config.hpp"
#include "../shaders/shaders.hpp"
#include "../utils/assets.hpp"
#include "../utils/logger.hpp"
#include <algorithm>

GLuint OkPostProcess::_fbo      = 0;
GLuint OkPostProcess::_colorTex = 0;
GLuint OkPostProcess::_depthTex = 0;
namespace {

  // Where the lens focuses before anything has been measured, in metres.
  const float DEFAULT_FOCUS_METRES = 30.0f;

  // A depth this close to 1 is the far plane: empty sky, nothing to focus
  // on, so the reading is discarded rather than pulling focus to infinity.
  const float DEPTH_AT_FAR_PLANE = 0.9999f;

  // Depth buffer runs 0..1 and normalized device coordinates -1..1.
  const float DEPTH_TO_NDC = 2.0f;

  // The frame delta arrives in milliseconds; the shader clock is seconds.
  const float SECONDS_PER_MS = 0.001f;

}  // namespace

int    OkPostProcess::_width                                             = 0;
int    OkPostProcess::_height                                            = 0;
GLuint OkPostProcess::_program                                           = 0;
GLuint OkPostProcess::_brightProgram                                     = 0;
GLuint OkPostProcess::_blurProgram                                       = 0;
std::array<GLuint, OkPostProcess::BLOOM_PASSES> OkPostProcess::_bloomFbo = {0,
                                                                            0};
std::array<GLuint, OkPostProcess::BLOOM_PASSES> OkPostProcess::_bloomTex = {0,
                                                                            0};
int                                             OkPostProcess::_bloomW   = 0;
int                                             OkPostProcess::_bloomH   = 0;
GLuint                                          OkPostProcess::_quadVao  = 0;
float                                           OkPostProcess::_time     = 0.0f;
std::array<float, 3> OkPostProcess::_motion       = {0.0f, 0.0f, 0.0f};
GLuint               OkPostProcess::_focusPbo     = 0;
bool                 OkPostProcess::_focusPending = false;
float                OkPostProcess::_focusMetres  = DEFAULT_FOCUS_METRES;
bool                 OkPostProcess::_active       = false;

void OkPostProcess::initialize() {
  // Master switch and per-effect toggles/parameters. All console-reachable.
  OkConfig::setBool("render.post", true);
  OkConfig::setBool("post.dof", true);
  OkConfig::setFloat("post.dof.focus", 30.0f);  // metres, sharp centre
  // Autofocus: the focus distance follows whatever is under the middle
  // of the screen, so a camera that can climb is not permanently out of
  // focus. "max" caps it, since past a point the whole distance is one
  // plane anyway; "ease" is how fast the lens catches up per frame.
  OkConfig::setBool("post.dof.autofocus", true);
  OkConfig::setFloat("post.dof.autofocus.max", 900.0f);
  OkConfig::setFloat("post.dof.autofocus.ease", 0.06f);
  // The sharp band and the falloff are generous on purpose. A tight
  // band suits a fixed camera a few metres from the subject, but the
  // moment the viewpoint climbs, everything on screen is hundreds of
  // metres away and a tight band blurs the entire frame.
  OkConfig::setFloat("post.dof.range", 200.0f);    // +/- fully sharp band
  OkConfig::setFloat("post.dof.maxblur", 2.0f);    // max blur radius (px)
  OkConfig::setFloat("post.dof.falloff", 600.0f);  // metres to reach max
  OkConfig::setBool("post.grain", true);
  OkConfig::setFloat("post.grain.strength", 0.015f);
  OkConfig::setBool("post.motionblur", true);  // needs a motion vector
  OkConfig::setBool("post.bloom", true);
  // High enough that only actual light sources glow -- lit windows,
  // lamps, the sun's disc. A daytime sky is bright over most of its
  // area, so a low threshold sends the whole sky through the blur and
  // washes the top half of the screen to white.
  OkConfig::setFloat("post.bloom.threshold", 0.85f);
  OkConfig::setFloat("post.bloom.knee", 0.30f);
  OkConfig::setFloat("post.bloom.strength", 1.00f);
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
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           _bloomTex[i], 0);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  OkLogger::info("PostProcess", "Offscreen target " + std::to_string(width) +
                                    "x" + std::to_string(height));
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
    int src = pass;      // 0 then 1
    int dst = 1 - pass;  // 1 then 0
    glBindFramebuffer(GL_FRAMEBUFFER, _bloomFbo[dst]);
    glViewport(0, 0, _bloomW, _bloomH);
    glBindTexture(GL_TEXTURE_2D, _bloomTex[src]);
    if (pass == 0) {
      glUniform2f(glGetUniformLocation(_blurProgram, "direction"),
                  1.0f / static_cast<float>(_bloomW), 0.0f);
    } else {
      glUniform2f(glGetUniformLocation(_blurProgram, "direction"), 0.0f,
                  1.0f / static_cast<float>(_bloomH));
    }
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }
  glBindVertexArray(0);
}

/**
 * @brief Point the focus at whatever is under the middle of the screen.
 *
 *        A fixed focus distance only works for a viewpoint that stays a
 *        fixed distance from its subject. The moment the camera can
 *        climb, everything is far away and the whole frame falls out of
 *        focus, so the distance has to follow what is being looked at.
 *
 *        The depth of one pixel is read back through a pixel buffer
 *        object and collected on the NEXT frame, so the CPU never waits
 *        for the GPU. One frame of lag is invisible; a stall is not.
 *        The result is eased rather than applied outright: a hard cut
 *        as the camera sweeps past a near wall reads as a glitch, while
 *        a lens takes a moment to find its subject.
 */
void OkPostProcess::updateAutoFocus(float nearPlane, float farPlane) {
  if (_focusPbo == 0) {
    glGenBuffers(1, &_focusPbo);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, _focusPbo);
    glBufferData(GL_PIXEL_PACK_BUFFER, sizeof(float), nullptr, GL_STREAM_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
  }

  glBindBuffer(GL_PIXEL_PACK_BUFFER, _focusPbo);
  if (_focusPending) {
    void *mapped = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
    if (mapped != nullptr) {
      float raw = *static_cast<float *>(mapped);
      glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
      // Depth 1.0 is the far plane: empty sky, nothing to focus on, so
      // the lens keeps whatever it had.
      if (raw < DEPTH_AT_FAR_PLANE) {
        float z      = raw * DEPTH_TO_NDC - 1.0f;
        float linear = DEPTH_TO_NDC * nearPlane * farPlane /
                       (farPlane + nearPlane - z * (farPlane - nearPlane));
        float target = OkConfig::getFloat("post.dof.autofocus.max");
        target       = std::min(linear, target);
        float ease   = OkConfig::getFloat("post.dof.autofocus.ease");
        ease         = std::max(ease, 0.0f);
        ease         = std::min(ease, 1.0f);
        _focusMetres += (target - _focusMetres) * ease;
      }
    }
  }

  // Queue the next read from the frame just rendered.
  glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);
  glReadPixels(_width / 2, _height / 2, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT,
               nullptr);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
  _focusPending = true;
}

void OkPostProcess::end(float nearPlane, float farPlane, float dt) {
  if (!_active) {
    return;
  }
  _time += dt * SECONDS_PER_MS;

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
  glUniform2f(glGetUniformLocation(_program, "texelSize"),
              1.0f / static_cast<float>(_width),
              1.0f / static_cast<float>(_height));
  glUniform2f(glGetUniformLocation(_program, "planes"), nearPlane, farPlane);
  glUniform1f(glGetUniformLocation(_program, "timeSec"), _time);

  bool dof       = OkConfig::getBool("post.dof");
  bool autoFocus = OkConfig::getBool("post.dof.autofocus");
  if (dof && autoFocus) {
    updateAutoFocus(nearPlane, farPlane);
  } else {
    _focusMetres = OkConfig::getFloat("post.dof.focus");
  }
  glUniform4f(glGetUniformLocation(_program, "dofParams"),
              autoFocus ? _focusMetres : OkConfig::getFloat("post.dof.focus"),
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
