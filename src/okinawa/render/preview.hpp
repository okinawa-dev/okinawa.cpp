#ifndef OKINAWA_RENDER_PREVIEW_HPP
#define OKINAWA_RENDER_PREVIEW_HPP

#include <array>
#include <cstddef>

class OkObject;
class OkRenderTarget;

/**
 * @file
 * @brief Draw a few objects on their own, into an offscreen target.
 *
 * A frame is the world seen from the player's camera, lit by the hour of
 * the day, fogged at distance and cut down by a frustum. An inspection
 * view is none of those things: one object, or a handful, seen from
 * wherever the viewer chose to put the eye, lit the same way whatever
 * the clock says, with nothing culled and nothing faded. Inventories,
 * thumbnails, tools that show a single piece -- they all want this, and
 * before this existed each of them wrote its own miniature renderer with
 * its own shader.
 *
 * That is the trap this closes. A second renderer is a second answer to
 * "what does this object look like", and it drifts: the one in front of
 * the user is not the one the game draws, and nobody notices, because
 * both look plausible. Here the objects go through the engine's own
 * shader, with their own textures, materials and tints -- the difference
 * from a frame is the camera, the light and where the pixels land.
 */
class OkPreview {
public:
  /** @brief How the objects are lit and what the surface starts as. */
  struct Settings {
    // Where the light comes from, as a direction pointing FROM the
    // source, and its colour. Fixed by the caller rather than taken from
    // the day cycle: a view whose shading changed with the clock would
    // report the object differently at nine and at midnight, and the
    // point of an inspection view is that it reports the object.
    std::array<float, 3> sunDirection;
    std::array<float, 3> sunColor;
    float                ambient;
    std::array<float, 4> clearColor;

    Settings();
  };

  /**
   * @brief The matrices for an eye orbiting a point.
   *
   * Pure arithmetic, deliberately: it is what decides whether an object
   * is in frame at all, and it can be checked without a graphics context.
   *
   * @param centre    what the eye looks at, in world units.
   * @param yawDeg    angle around the vertical axis.
   * @param pitchDeg  angle above the horizon; clamped short of vertical,
   *                  where the up vector and the view direction become
   *                  the same line and the view matrix has no way to
   *                  know which way round the world goes.
   * @param distance  how far the eye stands off.
   * @param aspect    width over height of the surface drawn to.
   * @param fovDeg    vertical field of view.
   * @param nearPlane distance to the near plane; a preview's depth
   *                  precision is spent between here and a few multiples
   *                  of it, so it is the caller's to scale with the
   *                  standoff rather than a constant.
   * @param outView   16 floats, column-major, may be null.
   * @param outProj   16 floats, column-major, may be null.
   */
  static void orbit(const float *centre, float yawDeg, float pitchDeg,
                    float distance, float aspect, float fovDeg, float nearPlane,
                    float farPlane, float *outView, float *outProj);

  /**
   * @brief How far back the eye has to stand for a sphere to fit.
   *
   * The number a preview opens at, and the one place it is worked out.
   * A standoff picked as a multiple of the radius is a guess about the
   * lens: at a vertical field of view of 35 degrees, two and a half
   * radii crops a sphere that a factor of three and a half frames with
   * room to spare, and neither number says which lens it assumed. Here
   * the lens is an argument and the arithmetic is the definition of
   * fitting.
   *
   * The narrower of the two half-angles decides it, so a surface wider
   * than it is tall is framed by its height and a tall one by its width
   * -- the object fits either way round rather than only in landscape.
   *
   * @param margin how much room is left around the object, as a
   *        fraction of its radius: 0 has it touch the edges, 0.15 gives
   *        it a border. Nothing shrinks the standoff below the radius
   *        itself, where the eye would be inside the sphere.
   */
  static float frameDistance(float radius, float fovDeg, float aspect,
                             float margin);

  /**
   * @brief Where the eye ends up for those angles, in world units.
   *
   * The same arithmetic `orbit` uses, offered on its own because a
   * caller usually needs the point too -- to place a light, to report
   * the viewpoint, to test the framing.
   */
  static void orbitEye(const float *centre, float yawDeg, float pitchDeg,
                       float distance, float *outEye);

  /**
   * @brief Draw the objects into the target with those matrices.
   *
   * Binds the target, clears it, draws, and puts back the framebuffer
   * and viewport it found -- so this can be called from inside another
   * pass, which is where a tool drawing a panel will call it from.
   *
   * Nothing is culled: a frustum and a draw distance belong to a frame
   * of a world, and an object the viewer asked to look at is never the
   * wrong answer to "what should be drawn".
   *
   * @param objects may contain nulls, which are skipped.
   */
  static void render(OkRenderTarget &target, const float *viewMatrix,
                     const float *projMatrix, OkObject *const *objects,
                     size_t count, const Settings &settings);
};

#endif
