#include "okinawa/item/item.hpp"
#include "okinawa/math/point.hpp"
#include "okinawa/math/ray.hpp"
#include "test-opengl.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

// An item owns its buffers, so intersectRay is tested through a real one
// -- which means a GL context, because the constructor uploads.

namespace {

  // A unit quad in the z = 0 plane, from (-1,-1) to (1,1), given in the
  // stride-5 layout every caller uses: x, y, z, u, v.
  std::array<float, 20> quadVertices() {
    return {-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,
            1.0f,  1.0f,  0.0f, 1.0f, 1.0f, -1.0f, 1.0f,  0.0f, 0.0f, 1.0f};
  }

  std::array<unsigned int, 6> quadIndices() {
    return {0, 1, 2, 0, 2, 3};
  }

  OkItem *makeQuad(const std::string &name) {
    std::array<float, 20>       vertices = quadVertices();
    std::array<unsigned int, 6> indices  = quadIndices();
    return new OkItem(name, vertices.data(), static_cast<long>(vertices.size()),
                      indices.data(), static_cast<long>(indices.size()));
  }

}  // namespace

TEST_CASE("OkItem ray against its own triangles", "[item][pick]") {
  TestGLFWContext context;

  SECTION("Straight at it") {
    OkItem *quad = makeQuad("quad");
    quad->updateTransform();

    OkRay ray(OkPoint(0.0f, 0.0f, 5.0f), OkPoint(0.0f, 0.0f, -1.0f));
    float distance = -1.0f;
    REQUIRE(quad->intersectRay(ray, &distance));
    REQUIRE_THAT(distance, WithinAbs(5.0f, 0.0001f));
    delete quad;
  }

  // The reason the broad phase cannot be the whole answer. The quad's
  // bounding sphere has the half-diagonal for a radius, so it reaches
  // past the quad's own edges; a test that stopped there would report a
  // hit on empty space beside it.
  SECTION("Inside the bounding sphere, past the edge of the item") {
    OkItem *quad = makeQuad("quad");
    quad->updateTransform();

    OkRay ray(OkPoint(1.2f, 0.0f, 5.0f), OkPoint(0.0f, 0.0f, -1.0f));
    REQUIRE_FALSE(quad->intersectRay(ray, nullptr));
    delete quad;
  }

  SECTION("Pointing away from it") {
    OkItem *quad = makeQuad("quad");
    quad->updateTransform();

    OkRay ray(OkPoint(0.0f, 0.0f, 5.0f), OkPoint(0.0f, 0.0f, 1.0f));
    REQUIRE_FALSE(quad->intersectRay(ray, nullptr));
    delete quad;
  }

  SECTION("Moved: the ray meets it where it is drawn, not where it was") {
    OkItem *quad = makeQuad("quad");
    quad->setPosition(10.0f, 0.0f, 0.0f);
    quad->updateTransform();

    OkRay missing(OkPoint(0.0f, 0.0f, 5.0f), OkPoint(0.0f, 0.0f, -1.0f));
    REQUIRE_FALSE(quad->intersectRay(missing, nullptr));

    OkRay hitting(OkPoint(10.0f, 0.0f, 5.0f), OkPoint(0.0f, 0.0f, -1.0f));
    float distance = -1.0f;
    REQUIRE(quad->intersectRay(hitting, &distance));
    REQUIRE_THAT(distance, WithinAbs(5.0f, 0.0001f));
    delete quad;
  }

  // The case the local-space test has to get right: with the item scaled,
  // a distance measured among its own vertices is not a distance in the
  // world.
  SECTION("Scaled: the distance comes back in world units") {
    OkItem *quad = makeQuad("quad");
    quad->setPosition(0.0f, 0.0f, 0.0f);
    quad->setScaling(1.0f, 1.0f, 1.0f);
    quad->updateTransform();

    // Aimed at a spot outside the unscaled quad and inside the scaled
    // one, so the scaling has to be honoured for it to hit at all.
    OkRay ray(OkPoint(2.5f, 0.0f, 5.0f), OkPoint(0.0f, 0.0f, -1.0f));
    REQUIRE_FALSE(quad->intersectRay(ray, nullptr));

    quad->setScaling(4.0f, 4.0f, 4.0f);
    quad->updateTransform();
    float distance = -1.0f;
    REQUIRE(quad->intersectRay(ray, &distance));
    REQUIRE_THAT(distance, WithinAbs(5.0f, 0.0001f));
    delete quad;
  }

  // Every triangle of the mesh is tested, not the first one: this spot
  // belongs to the second of the quad's two, and an early return would
  // have it miss.
  SECTION("A hit on the far triangle of the pair") {
    OkItem *quad = makeQuad("quad");
    quad->updateTransform();

    OkRay ray(OkPoint(-0.5f, 0.5f, 20.0f), OkPoint(0.0f, 0.0f, -1.0f));
    float distance = -1.0f;
    REQUIRE(quad->intersectRay(ray, &distance));
    REQUIRE_THAT(distance, WithinAbs(20.0f, 0.0001f));
    delete quad;
  }

  // Line and point items are geometry without a surface: the debug
  // layers, the origin axes. Reading their index list in threes would
  // invent triangles out of unrelated vertices.
  SECTION("An item drawn as lines is never hit") {
    OkItem *quad = makeQuad("quad");
    quad->setDrawMode(GL_LINES);
    quad->updateTransform();

    OkRay ray(OkPoint(0.0f, 0.0f, 5.0f), OkPoint(0.0f, 0.0f, -1.0f));
    REQUIRE_FALSE(quad->intersectRay(ray, nullptr));
    delete quad;
  }

  // The engine answers about geometry; whether a hidden thing may be
  // picked is the application's decision, and one it cannot revisit if
  // the engine has already made it.
  SECTION("Visibility is not the engine's business") {
    OkItem *quad = makeQuad("quad");
    quad->setVisible(false);
    quad->updateTransform();

    OkRay ray(OkPoint(0.0f, 0.0f, 5.0f), OkPoint(0.0f, 0.0f, -1.0f));
    REQUIRE(quad->intersectRay(ray, nullptr));
    delete quad;
  }
}
