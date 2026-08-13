#include "okinawa/avatar/controllers/ground_controller.hpp"
#include "okinawa/input/input.hpp"
#include "okinawa/math/rotation.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cmath>

// These exercise the pure ground-movement maths (no window/GL needed). They
// assert convention-independent invariants, so they don't depend on the exact
// forward direction a given yaw produces.

namespace {
  float mag2d(const OkGroundMove &m) {
    return std::sqrt(m.dx * m.dx + m.dz * m.dz);
  }
  bool near(float a, float b) {
    return std::fabs(a - b) < 1e-3f;
  }
}  // namespace

TEST_CASE("ground: no input produces no movement", "[avatar]") {
  OkInputState in;
  OkRotation   frame(0.0f, 0.0f, 0.0f);
  OkGroundMove m =
      OkGroundController::computeGroundMove(in, frame, 10.0f, 1.0f);
  CHECK_FALSE(m.moved);
  CHECK(m.dx == 0.0f);
  CHECK(m.dz == 0.0f);
}

TEST_CASE("ground: forward moves at speed * dt", "[avatar]") {
  OkInputState in;
  in.forward = true;
  OkRotation   frame(0.0f, 0.0f, 0.0f);
  OkGroundMove m =
      OkGroundController::computeGroundMove(in, frame, 10.0f, 0.5f);
  CHECK(m.moved);
  CHECK(near(mag2d(m), 5.0f));
}

TEST_CASE("ground: opposing inputs cancel out", "[avatar]") {
  OkInputState in;
  in.forward  = true;
  in.backward = true;
  OkRotation   frame(0.0f, 0.0f, 0.0f);
  OkGroundMove m =
      OkGroundController::computeGroundMove(in, frame, 10.0f, 1.0f);
  CHECK_FALSE(m.moved);
}

TEST_CASE("ground: strafing left and right are opposite", "[avatar]") {
  OkRotation   frame(0.0f, 0.0f, 0.0f);
  OkInputState r;
  r.strafeRight = true;
  OkInputState l;
  l.strafeLeft = true;
  OkGroundMove mr =
      OkGroundController::computeGroundMove(r, frame, 10.0f, 1.0f);
  OkGroundMove ml =
      OkGroundController::computeGroundMove(l, frame, 10.0f, 1.0f);
  CHECK(mr.moved);
  CHECK(ml.moved);
  CHECK(near(mr.dx, -ml.dx));
  CHECK(near(mr.dz, -ml.dz));
}

TEST_CASE("ground: diagonal movement is normalised (not faster)", "[avatar]") {
  OkInputState in;
  in.forward     = true;
  in.strafeRight = true;
  OkRotation   frame(0.0f, 0.0f, 0.0f);
  OkGroundMove m =
      OkGroundController::computeGroundMove(in, frame, 10.0f, 1.0f);
  CHECK(m.moved);
  CHECK(near(mag2d(m), 10.0f));
}
