#ifndef OK_SHADOW_MAP_HPP
#define OK_SHADOW_MAP_HPP

#include <array>

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
 *        Detail is split into CASCADES: the view distance is divided
 *        into bands, and each band gets its own map at the same
 *        resolution but covering a much smaller or larger area. One map
 *        cannot serve both ends -- cover 200 m and shadows stop at
 *        200 m; cover 2 km and a texel is a metre wide, so the shadow
 *        of a small step becomes a staircase. Splitting the range means the
 *        near band gets centimetres per texel where it is looked at
 *        closely, and the far band metres per texel where nobody can
 *        tell.
 *
 *        Each cascade covers a box fitted to that band of the camera's
 *        volume, snapped to texel increments so the shadow edges do not
 *        crawl as the camera moves -- the classic artefact of a moving
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
  // before the world pass; leaves the framebuffer and viewport as it
  // found them. `centre` is the point the map is built around (the
  // camera target). Does nothing when shadows are off or the light is
  // below the horizon.
  // `viewProj` is the camera's projection * view for this frame. The
  // shadowed area is derived from it, so the map covers what is being
  // looked at rather than a fixed square around the viewer.
  static void render(OkScene *scene, const float *viewProj, float centreX,
                     float centreY, float centreZ);

  // Cascades are capped here; `shadows.cascades` picks how many are
  // used. Three covers a walkable outdoor scene comfortably.
  static const int MAX_CASCADES = 4;

  // Bind the depth texture and set the world shader's uniforms.
  static void bind(GLuint program);

  static void shutdown();

  // 0 when shadows are inactive this frame.
  static float getStrength();

private:
  static void ensureTarget(int size, int layers);

  static GLuint _fbo;
  static GLuint _depthTex;  // GL_TEXTURE_2D_ARRAY, one layer per cascade
  static GLuint _program;   // depth-only pass
  static int    _size;
  static int    _layers;  // layers the array was built with
  // State of the last draw, per cascade, so an identical one is skipped.
  static bool                            _neverDrawn;
  static std::array<float, 3>            _lastDir;
  static std::array<float, MAX_CASCADES> _lastCx;
  static std::array<float, MAX_CASCADES> _lastCz;
  static std::array<float, MAX_CASCADES> _lastExtent;
  // An empty 1x1x1 depth array, kept for the frames when there is no
  // real map to bind -- with the sun down, say. The world shader
  // declares a sampler2DArray whether or not anything is shadowing, and
  // in the core profile a draw whose declared sampler points at a unit
  // with no complete texture on it fails, taking the whole frame with
  // it. This gives the sampler something to point at; nothing ever
  // reads from it.
  static GLuint                              _emptyShadowTex;
  static size_t                              _lastObjects;
  static std::array<glm::mat4, MAX_CASCADES> _lightSpace;
  // Where each cascade stops, as a view-space distance. The world pass
  // picks a cascade by comparing the fragment's depth against these.
  static std::array<float, MAX_CASCADES> _splitFar;
  static int                             _count;  // cascades actually in use
  static float                           _strength;
};

#endif
