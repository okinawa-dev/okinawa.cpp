#ifndef OK_POSTPROCESS_HPP
#define OK_POSTPROCESS_HPP

#include "../core/gl_config.hpp"

/**
 * @brief Static handler for the post-process chain.
 *
 *        With `render.post` enabled the world pass renders into an
 *        offscreen framebuffer (colour + depth textures) instead of the
 *        window, and end() composites it to the window through a
 *        full-screen shader applying the enabled effects:
 *
 *        - depth of field (`post.dof`): diorama-style blur for fragments
 *          much nearer / farther than the focus distance;
 *        - film grain (`post.grain`): animated per-pixel noise;
 *        - directional motion blur (`post.motionblur`): screen-space
 *          velocity smear, driven per frame via setMotionVector().
 *
 *        Every toggle and parameter is an OkConfig value, so the console
 *        reaches them (`set post.dof false`, `set post.dof.focus 25`).
 *        With `render.post` off, begin()/end() do nothing and the engine
 *        renders directly to the window exactly as before.
 *
 *        The GUI pass draws AFTER end(), directly to the window: the
 *        interface stays sharp and grain-free (a per-stage GUI chain can
 *        hook here later).
 */
class OkPostProcess {
public:
  OkPostProcess() = delete;

  // Register config defaults. Called by OkCore::initialize.
  static void initialize();

  // Bind the offscreen target (creating/resizing it lazily to the given
  // framebuffer size). Returns false when render.post is off -- the
  // caller then renders straight to the window.
  static bool begin(int width, int height);

  // Composite the offscreen frame to the window. nearPlane/farPlane
  // linearize the depth buffer for the DoF; dt (ms) animates the grain.
  static void end(float nearPlane, float farPlane, float dt);

  // Screen-space motion vector for the directional blur (game-driven;
  // decays to zero strength when not refreshed).
  static void setMotionVector(float dx, float dy, float strength);

  // Destroy GL resources. Called by OkCore::exit.
  static void shutdown();

private:
  static void ensureTarget(int width, int height);
  static void ensureProgram();

  static GLuint _fbo;
  static GLuint _colorTex;
  static GLuint _depthTex;
  static int    _width, _height;
  static GLuint _program;
  static GLuint _quadVao;
  static float  _time;
  static float  _motion[3];  // dx, dy, strength
  static bool   _active;
};

#endif
