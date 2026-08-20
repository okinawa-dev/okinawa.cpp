#include "okinawa/item/item.hpp"
#include "okinawa/math/point.hpp"
#include "okinawa/math/ray.hpp"
#include "test-opengl.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

// An item assembled from pieces, each wearing its own texture.
//
// What is worth testing is the arithmetic, not the drawing: a piece's
// indices count from its own first vertex, and the item has to move them
// along by what it already holds. Got wrong, the second piece draws the
// first one's triangles -- silently, because the indices are still
// inside the buffer.

namespace {

  // A unit quad in the y = height plane, from (-1,-1) to (1,1), in the
  // stride-5 layout every caller uses: x, y, z, u, v.
  std::array<float, 20> quadAt(float height) {
    return {-1.0f, height, -1.0f, 0.0f, 0.0f, 1.0f,  height, -1.0f, 1.0f, 0.0f,
            1.0f,  height, 1.0f,  1.0f, 1.0f, -1.0f, height, 1.0f,  0.0f, 1.0f};
  }

  std::array<unsigned int, 6> quadIndices() {
    return {0, 1, 2, 0, 2, 3};
  }

}  // namespace

TEST_CASE("An item built from pieces keeps each piece's geometry",
          "[item][addmesh]") {
  TestGLFWContext context;

  OkItem                      item("assembled");
  std::array<float, 20>       low  = quadAt(0.0f);
  std::array<float, 20>       high = quadAt(10.0f);
  std::array<unsigned int, 6> idx  = quadIndices();

  item.addMesh(low.data(), static_cast<long>(low.size()), idx.data(),
               static_cast<long>(idx.size()), "");
  item.addMesh(high.data(), static_cast<long>(high.size()), idx.data(),
               static_cast<long>(idx.size()), "");
  item.upload();
  item.updateTransform();

  // One range per piece: that is what lets them wear different textures.
  REQUIRE(item.getMaterialCount() == 2);

  // Fired from above, the ray meets the upper quad at 10 and the lower
  // one is behind it. Had the second piece's indices not been moved
  // along, it would have drawn over the first and there would be only
  // one quad to hit.
  OkRay from_above(OkPoint(0.0f, 20.0f, 0.0f), OkPoint(0.0f, -1.0f, 0.0f));
  float distance = -1.0f;
  REQUIRE(item.intersectRay(from_above, &distance));
  REQUIRE_THAT(distance, WithinAbs(10.0f, 0.001f));

  // From between the two, only the lower one is ahead.
  OkRay between(OkPoint(0.0f, 5.0f, 0.0f), OkPoint(0.0f, -1.0f, 0.0f));
  REQUIRE(item.intersectRay(between, &distance));
  REQUIRE_THAT(distance, WithinAbs(5.0f, 0.001f));
}

TEST_CASE("An item given nothing has nothing", "[item][addmesh]") {
  TestGLFWContext context;

  // upload() on an item with no geometry must do nothing rather than
  // make a zero-length buffer: a non-null pointer to no vertices reads
  // as a mesh to everything downstream.
  OkItem item("empty");
  item.upload();
  item.updateTransform();
  REQUIRE(item.getMaterialCount() == 0);

  OkRay ray(OkPoint(0.0f, 1.0f, 0.0f), OkPoint(0.0f, -1.0f, 0.0f));
  REQUIRE_FALSE(item.intersectRay(ray, nullptr));
}
