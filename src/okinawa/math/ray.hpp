#ifndef OK_RAY_HPP
#define OK_RAY_HPP

#include "point.hpp"
#include <glm/mat4x4.hpp>

/**
 * @brief A half-line through the world: where it starts and where it
 *        goes, plus the intersection tests worth having against it.
 *
 *        Every distance this class reports, and every distance it takes,
 *        is measured in units of `direction` -- not in world units. With
 *        a unit direction the two are the same thing and nobody has to
 *        think about it, which is the ordinary case. The distinction
 *        exists for `transformed`: moving a ray into an object's local
 *        space scales its direction, and letting that scaling live in the
 *        direction's length is what makes a local-space hit come back
 *        with a world-space distance. Normalizing after the transform
 *        would throw that conversion away and quietly report the wrong
 *        distance under any scaling but 1.
 *
 *        The class holds no policy about what may be hit. Which objects
 *        are worth testing, whether a hidden one counts, and which of
 *        several hits wins are all questions for whoever is doing the
 *        asking.
 */
struct OkRay {
  OkPoint origin;
  OkPoint direction;

  OkRay() {
    direction = OkPoint(0.0f, 0.0f, -1.0f);
  }

  OkRay(const OkPoint &rayOrigin, const OkPoint &rayDirection) {
    origin    = rayOrigin;
    direction = rayDirection;
  }

  /** @brief The point this far along the ray. */
  OkPoint pointAt(float distance) const;

  /**
   * @brief Where the ray enters an axis-aligned box, or false if never.
   *
   *        The slab method: for each axis, the interval of distances over
   *        which the ray lies between that axis' pair of planes. It is
   *        inside the box over the intersection of the three intervals,
   *        and it hits when that intersection is not empty and does not
   *        end behind the origin.
   *
   * @param low         The box's minimum corner.
   * @param high        The box's maximum corner.
   * @param outDistance If non-null, the distance to the entry point. A
   *                    ray that starts inside reports 0: it is already
   *                    there, and there is no entry to measure.
   */
  bool intersectsBox(const OkPoint &low, const OkPoint &high,
                     float *outDistance) const;

  /**
   * @brief Where the ray enters a sphere, or false if never.
   *
   *        The cheap broad phase, and the same shape the frustum culls
   *        with, so an object that already carries a bounding sphere for
   *        drawing carries one for picking too.
   *
   * @param outDistance If non-null, the distance to the entry point, or 0
   *                    when the ray starts inside the sphere.
   */
  bool intersectsSphere(const OkPoint &centre, float radius,
                        float *outDistance) const;

  /**
   * @brief Where the ray crosses a triangle, or false if it misses.
   *
   *        Moller-Trumbore, without the early rejection of back faces: a
   *        camera ends up inside a closed mesh often enough, and a back
   *        face that refused to be hit would make the walls around it
   *        unhittable while they are plainly on screen. Winding is a
   *        question for the caller, which has the normal if it cares.
   */
  bool intersectsTriangle(const OkPoint &a, const OkPoint &b, const OkPoint &c,
                          float *outDistance) const;

  /**
   * @brief The same ray seen from another space.
   *
   *        Multiply by the inverse of an object's model matrix and the
   *        ray arrives in that object's local space, where its vertices
   *        already are -- so a mesh test transforms one ray instead of
   *        every vertex. The origin moves as a point and the direction as
   *        a vector, and the direction is deliberately left unnormalized;
   *        see the note on the class.
   */
  OkRay transformed(const glm::mat4 &matrix) const;
};

#endif
