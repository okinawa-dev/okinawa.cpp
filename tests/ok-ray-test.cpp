#include "okinawa/math/point.hpp"
#include "okinawa/math/ray.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

using Catch::Matchers::WithinAbs;

// A ray is measured in units of its own direction: a unit direction makes
// every distance below a distance in world units. The tests state that
// explicitly, because the transform tests depend on it holding.

TEST_CASE("OkRay point along the ray", "[ray]") {
  OkRay ray(OkPoint(1.0f, 2.0f, 3.0f), OkPoint(0.0f, 0.0f, -1.0f));

  OkPoint at = ray.pointAt(4.0f);
  REQUIRE_THAT(at.x(), WithinAbs(1.0f, 0.0001f));
  REQUIRE_THAT(at.y(), WithinAbs(2.0f, 0.0001f));
  REQUIRE_THAT(at.z(), WithinAbs(-1.0f, 0.0001f));
}

TEST_CASE("OkRay against a box", "[ray]") {
  const OkPoint low(-1.0f, -1.0f, -1.0f);
  const OkPoint high(1.0f, 1.0f, 1.0f);

  SECTION("Head on, from outside") {
    OkRay ray(OkPoint(0.0f, 0.0f, 10.0f), OkPoint(0.0f, 0.0f, -1.0f));
    float distance = -1.0f;
    REQUIRE(ray.intersectsBox(low, high, &distance));
    REQUIRE_THAT(distance, WithinAbs(9.0f, 0.0001f));
  }

  SECTION("Wide of the box") {
    OkRay ray(OkPoint(5.0f, 0.0f, 10.0f), OkPoint(0.0f, 0.0f, -1.0f));
    REQUIRE_FALSE(ray.intersectsBox(low, high, nullptr));
  }

  SECTION("Pointing away from it") {
    OkRay ray(OkPoint(0.0f, 0.0f, 10.0f), OkPoint(0.0f, 0.0f, 1.0f));
    REQUIRE_FALSE(ray.intersectsBox(low, high, nullptr));
  }

  SECTION("Starting inside, which is a hit at no distance at all") {
    OkRay ray(OkPoint(0.0f, 0.0f, 0.0f), OkPoint(0.0f, 0.0f, -1.0f));
    float distance = -1.0f;
    REQUIRE(ray.intersectsBox(low, high, &distance));
    REQUIRE_THAT(distance, WithinAbs(0.0f, 0.0001f));
  }

  // The branch a screen ray never reaches: a direction with a zero
  // component is parallel to one pair of the box's planes, and that axis
  // can only say yes or no, never a distance.
  SECTION("Parallel to an axis, running past the box") {
    OkRay ray(OkPoint(-10.0f, 5.0f, 0.0f), OkPoint(1.0f, 0.0f, 0.0f));
    REQUIRE_FALSE(ray.intersectsBox(low, high, nullptr));
  }

  SECTION("Parallel to an axis, running through it") {
    OkRay ray(OkPoint(-10.0f, 0.0f, 0.0f), OkPoint(1.0f, 0.0f, 0.0f));
    float distance = -1.0f;
    REQUIRE(ray.intersectsBox(low, high, &distance));
    REQUIRE_THAT(distance, WithinAbs(9.0f, 0.0001f));
  }
}

TEST_CASE("OkRay against a sphere", "[ray]") {
  const OkPoint centre(0.0f, 0.0f, 0.0f);
  const float   radius = 2.0f;

  SECTION("Through the middle") {
    OkRay ray(OkPoint(0.0f, 0.0f, 10.0f), OkPoint(0.0f, 0.0f, -1.0f));
    float distance = -1.0f;
    REQUIRE(ray.intersectsSphere(centre, radius, &distance));
    REQUIRE_THAT(distance, WithinAbs(8.0f, 0.0001f));
  }

  SECTION("Passing outside it") {
    OkRay ray(OkPoint(5.0f, 0.0f, 10.0f), OkPoint(0.0f, 0.0f, -1.0f));
    REQUIRE_FALSE(ray.intersectsSphere(centre, radius, nullptr));
  }

  SECTION("Pointing away from it") {
    OkRay ray(OkPoint(0.0f, 0.0f, 10.0f), OkPoint(0.0f, 0.0f, 1.0f));
    REQUIRE_FALSE(ray.intersectsSphere(centre, radius, nullptr));
  }

  SECTION("Starting inside, which is a hit at no distance at all") {
    OkRay ray(OkPoint(0.0f, 0.0f, 0.0f), OkPoint(0.0f, 0.0f, -1.0f));
    float distance = -1.0f;
    REQUIRE(ray.intersectsSphere(centre, radius, &distance));
    REQUIRE_THAT(distance, WithinAbs(0.0f, 0.0001f));
  }
}

TEST_CASE("OkRay against a triangle", "[ray]") {
  // A triangle standing in the z = 0 plane, wound counter-clockwise as
  // seen from +Z.
  const OkPoint a(-1.0f, -1.0f, 0.0f);
  const OkPoint b(1.0f, -1.0f, 0.0f);
  const OkPoint c(0.0f, 1.0f, 0.0f);

  SECTION("Through the middle, front face") {
    OkRay ray(OkPoint(0.0f, 0.0f, 5.0f), OkPoint(0.0f, 0.0f, -1.0f));
    float distance = -1.0f;
    REQUIRE(ray.intersectsTriangle(a, b, c, &distance));
    REQUIRE_THAT(distance, WithinAbs(5.0f, 0.0001f));
  }

  // Both faces count. A camera inside a building is the ordinary case in
  // an editor, and a back face that refused to be hit would make the
  // walls around the camera unselectable.
  SECTION("Through the middle, back face") {
    OkRay ray(OkPoint(0.0f, 0.0f, -5.0f), OkPoint(0.0f, 0.0f, 1.0f));
    float distance = -1.0f;
    REQUIRE(ray.intersectsTriangle(a, b, c, &distance));
    REQUIRE_THAT(distance, WithinAbs(5.0f, 0.0001f));
  }

  SECTION("Outside an edge") {
    OkRay ray(OkPoint(0.9f, 0.9f, 5.0f), OkPoint(0.0f, 0.0f, -1.0f));
    REQUIRE_FALSE(ray.intersectsTriangle(a, b, c, nullptr));
  }

  SECTION("Behind the ray") {
    OkRay ray(OkPoint(0.0f, 0.0f, 5.0f), OkPoint(0.0f, 0.0f, 1.0f));
    REQUIRE_FALSE(ray.intersectsTriangle(a, b, c, nullptr));
  }

  SECTION("Along the triangle's own plane") {
    OkRay ray(OkPoint(-5.0f, 0.0f, 0.0f), OkPoint(1.0f, 0.0f, 0.0f));
    REQUIRE_FALSE(ray.intersectsTriangle(a, b, c, nullptr));
  }
}

// Testing in an object's local space is the whole point of transforming a
// ray: the alternative is transforming every vertex of the mesh. It only
// works if the distance the local test reports is the distance in world
// units, and that is what these check -- including under a scaling, where
// the transformed direction stops being a unit vector and carries the
// conversion in its length.
TEST_CASE("OkRay transformed into another space", "[ray]") {
  const OkPoint low(-1.0f, -1.0f, -1.0f);
  const OkPoint high(1.0f, 1.0f, 1.0f);

  SECTION("A translation moves the origin and leaves the direction alone") {
    OkRay     world(OkPoint(0.0f, 0.0f, 10.0f), OkPoint(0.0f, 0.0f, -1.0f));
    glm::mat4 toLocal =
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -4.0f));
    OkRay local = world.transformed(toLocal);

    REQUIRE_THAT(local.origin.z(), WithinAbs(6.0f, 0.0001f));
    REQUIRE_THAT(local.direction.z(), WithinAbs(-1.0f, 0.0001f));
  }

  SECTION("A uniform scaling reports the world distance") {
    // A box of half-size 1 in local space, drawn at twice the size: its
    // world face stands at z = 2, so a ray from z = 10 travels 8.
    OkRay     world(OkPoint(0.0f, 0.0f, 10.0f), OkPoint(0.0f, 0.0f, -1.0f));
    glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 2.0f));
    OkRay     local = world.transformed(glm::inverse(model));

    float distance = -1.0f;
    REQUIRE(local.intersectsBox(low, high, &distance));
    REQUIRE_THAT(distance, WithinAbs(8.0f, 0.0001f));
  }

  SECTION("A non-uniform scaling reports the world distance too") {
    // Squashed on z alone: the world face stands at z = 0.5, so the ray
    // travels 9.5. A local test that assumed a unit direction would
    // report 9, which is the bug this case exists for.
    OkRay     world(OkPoint(0.0f, 0.0f, 10.0f), OkPoint(0.0f, 0.0f, -1.0f));
    glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f, 3.0f, 0.5f));
    OkRay     local = world.transformed(glm::inverse(model));

    float distance = -1.0f;
    REQUIRE(local.intersectsBox(low, high, &distance));
    REQUIRE_THAT(distance, WithinAbs(9.5f, 0.0001f));
  }

  SECTION("A rotation reports the world distance") {
    OkRay     world(OkPoint(0.0f, 0.0f, 10.0f), OkPoint(0.0f, 0.0f, -1.0f));
    glm::mat4 model =
        glm::rotate(glm::mat4(1.0f), 0.7f, glm::vec3(0.0f, 1.0f, 0.0f));
    OkRay local = world.transformed(glm::inverse(model));

    float distance = -1.0f;
    REQUIRE(local.intersectsSphere(OkPoint(0.0f, 0.0f, 0.0f), 2.0f, &distance));
    REQUIRE_THAT(distance, WithinAbs(8.0f, 0.0001f));
  }
}
