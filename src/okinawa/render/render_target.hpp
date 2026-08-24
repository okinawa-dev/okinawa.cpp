#ifndef OKINAWA_RENDER_TARGET_HPP
#define OKINAWA_RENDER_TARGET_HPP

#include "../core/gl_config.hpp"

/**
 * @file
 * @brief An offscreen surface to draw into: colour, depth, and a size.
 *
 * Everything the engine draws goes to the window, which is the right
 * default and the wrong answer for anything that wants a picture rather
 * than a frame -- an inspection view of one object, a thumbnail, a
 * mirror, a pass that another pass samples.
 *
 * Before this, the two places that needed one built it by hand out of
 * raw framebuffer calls. A third caller would have been a third copy,
 * and each copy has its own opinion about attachment formats, about
 * whether the previous binding is put back, and about what happens when
 * the size changes -- which is the sort of disagreement that shows up as
 * a black rectangle nobody can explain.
 */
class OkRenderTarget {
public:
  OkRenderTarget();
  ~OkRenderTarget();

  // No copying: this owns names the driver handed out, and a copy would
  // hand the same ones to two destructors.
  OkRenderTarget(const OkRenderTarget &)            = delete;
  OkRenderTarget &operator=(const OkRenderTarget &) = delete;

  /**
   * @brief Give the target a size, allocating on the first call and on
   *        any change of size.
   *
   * Called every frame by anything whose surface follows a resizable
   * panel, so the same size twice is deliberately free.
   *
   * @return false when the surface could not be made complete; the
   *         target is then unusable and `bind` does nothing, which is
   *         better than drawing into a framebuffer the driver has
   *         already rejected.
   */
  bool resize(int width, int height);

  /**
   * @brief Draw into this target from here on.
   *
   * Remembers the framebuffer and viewport that were in force, so
   * `unbind` can put them back. Nesting is not supported and is not
   * needed: a target is bound, drawn into, and released.
   */
  void bind();

  /** @brief Put back the framebuffer and viewport `bind` found. */
  void unbind();

  /** @brief The colour texture, for whoever wants to draw the result. */
  GLuint getTexture() const {
    return colorTexture;
  }
  bool isValid() const {
    return valid;
  }
  int getWidth() const {
    return width;
  }
  int getHeight() const {
    return height;
  }

private:
  void release();

  GLuint framebuffer;
  GLuint colorTexture;
  GLuint depthBuffer;
  int    width;
  int    height;
  bool   valid;

  // What was bound when this was bound, so it can be put back exactly.
  // Read from the driver rather than assumed to be the window: a target
  // bound inside another pass would otherwise release to the wrong
  // surface and paint the rest of the frame somewhere invisible.
  GLint previousFramebuffer;
  GLint previousViewport[4];
};

#endif
