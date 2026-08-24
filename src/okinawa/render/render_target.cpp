#include "render_target.hpp"

#include "../utils/logger.hpp"

#include <string>

OkRenderTarget::OkRenderTarget() {
  framebuffer         = 0;
  colorTexture        = 0;
  depthBuffer         = 0;
  width               = 0;
  height              = 0;
  valid               = false;
  previousFramebuffer = 0;
  previousViewport    = {0, 0, 0, 0};
}

OkRenderTarget::~OkRenderTarget() {
  release();
}

void OkRenderTarget::release() {
  if (colorTexture != 0) {
    glDeleteTextures(1, &colorTexture);
    colorTexture = 0;
  }
  if (depthBuffer != 0) {
    glDeleteRenderbuffers(1, &depthBuffer);
    depthBuffer = 0;
  }
  if (framebuffer != 0) {
    glDeleteFramebuffers(1, &framebuffer);
    framebuffer = 0;
  }
  valid = false;
}

bool OkRenderTarget::resize(int newWidth, int newHeight) {
  if (newWidth <= 0 || newHeight <= 0) {
    return false;
  }
  if (valid && newWidth == width && newHeight == height) {
    return true;  // the common call: a panel that did not change size
  }

  release();
  width  = newWidth;
  height = newHeight;

  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

  glGenTextures(1, &colorTexture);
  glBindTexture(GL_TEXTURE_2D, colorTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, nullptr);
  // Linear, and clamped to the edge. A target is usually drawn at a size
  // close to its own, and clamping is what keeps the last row of texels
  // from wrapping round to the first when it is not exactly so.
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         colorTexture, 0);

  // Depth as a renderbuffer, not a texture: nothing samples it, and a
  // renderbuffer is what a driver is free to keep in whatever layout it
  // likes when it never has to be read back.
  glGenRenderbuffers(1, &depthBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, depthBuffer);

  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);

  if (status != GL_FRAMEBUFFER_COMPLETE) {
    OkLogger::error("RenderTarget",
                    "Incomplete framebuffer (status " +
                        std::to_string(static_cast<int>(status)) + ") at " +
                        std::to_string(width) + "x" + std::to_string(height));
    release();
    return false;
  }

  valid = true;
  return true;
}

void OkRenderTarget::bind() {
  if (!valid) {
    return;
  }
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
  glGetIntegerv(GL_VIEWPORT, previousViewport.data());
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  glViewport(0, 0, width, height);
}

void OkRenderTarget::unbind() {
  if (!valid) {
    return;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFramebuffer));
  glViewport(previousViewport[0], previousViewport[1], previousViewport[2],
             previousViewport[3]);
}
