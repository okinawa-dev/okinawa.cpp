#ifndef OK_SHADOW_MAP_HPP
#define OK_SHADOW_MAP_HPP

#include <cstddef>

#include "../core/gl_config.hpp"
#include <glm/ext/matrix_float4x4.hpp>

class OkScene;

/**
 * @brief Shadows from the scene's directional light.
 *
 *        The scene is rendered once per frame from the light's own
 *        direction into a depth texture; the world pass then projects
 *        each fragment into that same space and compares depths, so a
 *        fragment further away than what the light could see is in
 *        shadow.
 *
 *        The map covers a box that FOLLOWS THE VIEWER (there is no
 *        point spending resolution on ground the player cannot see),
 *        snapped to texel increments so the shadow edges do not crawl
 *        as the camera moves -- the classic artefact of a moving
 *        shadow frustum.
 *
 *        Sampling uses a small percentage-closer filter kernel, which
 *        softens the stair-stepping a single comparison would give.
 *        Shadow strength follows the light's own elevation: it fades
 *        out as the source approaches the horizon and disappears when
 *        it is below, where a hard shadow would look wrong.
 */
class OkShadowMap {
public:
  OkShadowMap() = delete;

  // Register config defaults. Called by OkCore::initialize.
  static void initialize();

  // Render the scene's depth from the light. Called by the frame loop
  // BEFORE the world pass; leaves the framebuffer and viewport as it
  // found them. `centre` is the point the map is built around (the
  // camera target). Does nothing when shadows are off or the light is
  // below the horizon.
  // `viewProj` is the CAMERA's projection * view for this frame. The
  // shadowed area is derived from it, so the map covers what is being
  // looked at rather than a fixed square around the viewer.
  static void render(OkScene *scene, const float *viewProj, float centreX,
                     float centreY, float centreZ);

  // Bind the depth texture and set the world shader's uniforms.
  static void bind(GLuint program);

  static void shutdown();

  // 0 when shadows are inactive this frame.
  static float getStrength();

private:
  static void ensureTarget(int size);

  static GLuint    _fbo;
  static GLuint    _depthTex;
  static GLuint    _program;   // depth-only pass
  static int       _size;
  // State of the last draw, so an identical one can be skipped.
  static bool      _neverDrawn;
  static float     _lastDir[3];
  static float     _lastCx, _lastCz, _lastExtent;
  static size_t    _lastObjects;
  static glm::mat4 _lightSpace;
  static float     _strength;
};

#endif
