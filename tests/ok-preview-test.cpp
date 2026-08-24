#include "okinawa/render/preview.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

// Where a preview's eye stands, and whether what it looks at is in
// frame.
//
// This is the half of a preview that can be checked without a graphics
// context, and it is the half that decides whether the picture has the
// object in it at all: an eye at the wrong distance shows an empty
// rectangle, and an empty rectangle looks exactly like a preview that
// failed to load.

namespace {

  // Project a world point with the matrices the framing produced, and
  // report where it lands in normalized device coordinates.
  glm::vec4 project(const float *view, const float *proj, float x, float y,
                    float z) {
    glm::mat4 v    = glm::make_mat4(view);
    glm::mat4 p    = glm::make_mat4(proj);
    glm::vec4 clip = p * v * glm::vec4(x, y, z, 1.0f);
    return clip;
  }

}  // namespace

TEST_CASE("The eye stands at the asked distance from what it looks at",
          "[preview]") {
  float centre[3] = {10.0f, 4.0f, -7.0f};
  float eye[3]    = {0.0f, 0.0f, 0.0f};
  OkPreview::orbitEye(centre, 35.0f, 20.0f, 6.0f, eye);

  float dx = eye[0] - centre[0];
  float dy = eye[1] - centre[1];
  float dz = eye[2] - centre[2];
  REQUIRE(std::fabs(std::sqrt(dx * dx + dy * dy + dz * dz) - 6.0f) < 1e-4f);
  // Pitched up: the eye is above what it looks at.
  REQUIRE(eye[1] > centre[1]);
}

TEST_CASE("Pitch is clamped short of vertical", "[preview]") {
  float centre[3] = {0.0f, 0.0f, 0.0f};
  float straight[3];
  float clamped[3];
  // Straight down is where the up vector and the view direction become
  // the same line, and the picture flips as the last degree is crossed.
  OkPreview::orbitEye(centre, 0.0f, -90.0f, 5.0f, straight);
  OkPreview::orbitEye(centre, 0.0f, -89.0f, 5.0f, clamped);
  REQUIRE(std::fabs(straight[0] - clamped[0]) < 1e-4f);
  REQUIRE(std::fabs(straight[1] - clamped[1]) < 1e-4f);
  REQUIRE(std::fabs(straight[2] - clamped[2]) < 1e-4f);
}

TEST_CASE("What the eye orbits lands in the middle of the picture",
          "[preview]") {
  float centre[3] = {-120.0f, 33.0f, 88.0f};
  float view[16];
  float proj[16];
  OkPreview::orbit(centre, 135.0f, 25.0f, 12.0f, 16.0f / 9.0f, 35.0f, 0.1f,
                   100.0f, view, proj);

  glm::vec4 clip = project(view, proj, centre[0], centre[1], centre[2]);
  REQUIRE(clip.w > 0.0f);  // in front of the eye
  REQUIRE(std::fabs(clip.x / clip.w) < 1e-3f);
  REQUIRE(std::fabs(clip.y / clip.w) < 1e-3f);
}

TEST_CASE("An object the size of the standoff is inside the frame",
          "[preview]") {
  // The framing a viewer gets when a window opens: the standoff is a
  // multiple of the object's radius, and the object has to fit.
  const float radius    = 2.0f;
  const float distance  = OkPreview::frameDistance(radius, 35.0f, 1.0f, 0.15f);
  float       centre[3] = {0.0f, 0.0f, 0.0f};
  float       view[16];
  float       proj[16];
  OkPreview::orbit(centre, 0.0f, 0.0f, distance, 1.0f, 35.0f, distance * 0.01f,
                   distance * 8.0f, view, proj);

  // The top of the object, and its far side, both have to land inside
  // the normalized cube.
  glm::vec4 top  = project(view, proj, 0.0f, radius, 0.0f);
  glm::vec4 side = project(view, proj, radius, 0.0f, -radius);
  REQUIRE(top.w > 0.0f);
  REQUIRE(side.w > 0.0f);
  REQUIRE(std::fabs(top.y / top.w) < 1.0f);
  REQUIRE(std::fabs(side.x / side.w) < 1.0f);
  REQUIRE(std::fabs(side.y / side.w) < 1.0f);
}

TEST_CASE("A standoff picked by hand is not assumed to frame anything",
          "[preview]") {
  // What the tool did before there was a number for this: two and a
  // half radii, with a 35 degree lens. It crops.
  const float radius = 2.0f;
  REQUIRE(OkPreview::frameDistance(radius, 35.0f, 1.0f, 0.0f) > radius * 2.4f);
  // A narrow surface needs more room than a square one, because its
  // horizontal half-angle is the tight one.
  REQUIRE(OkPreview::frameDistance(radius, 35.0f, 0.5f, 0.0f) >
          OkPreview::frameDistance(radius, 35.0f, 1.0f, 0.0f));
  // And the eye never ends up inside the sphere, whatever is asked.
  REQUIRE(OkPreview::frameDistance(radius, 179.0f, 1.0f, 0.0f) >= radius);
}

TEST_CASE("A degenerate aspect ratio does not produce a broken projection",
          "[preview]") {
  float centre[3] = {0.0f, 0.0f, 0.0f};
  float view[16];
  float proj[16];
  // A panel one frame before it has been given a size reports zero, and
  // a projection divided by it would be a matrix of infinities that
  // silently draws nothing.
  OkPreview::orbit(centre, 0.0f, 0.0f, 5.0f, 0.0f, 35.0f, 0.1f, 50.0f, view,
                   proj);
  for (int i = 0; i < 16; i++) {
    REQUIRE(std::isfinite(proj[i]));
    REQUIRE(std::isfinite(view[i]));
  }
}
