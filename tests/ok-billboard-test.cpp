#include "okinawa/item/billboard.hpp"
#include "okinawa/math/point.hpp"
#include "okinawa/math/rotation.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>

using Catch::Matchers::WithinAbs;

// facingRotation is the billboard's whole brain: the rotation it returns
// must map the local +Z axis onto the unit vector from `from` to `to`.
// (The quad itself needs a GL context, so only the math is tested here.)

static OkPoint plusZ(const OkRotation &rot) {
  return rot.transformPoint(OkPoint(0.0f, 0.0f, 1.0f));
}

TEST_CASE("OkBillboard facing rotation", "[billboard]") {
  SECTION("Camera straight ahead (+Z) keeps the identity") {
    OkRotation rot = OkBillboard::facingRotation(OkPoint(0.0f, 0.0f, 0.0f),
                                                 OkPoint(0.0f, 0.0f, 10.0f));
    OkPoint    z   = plusZ(rot);
    REQUIRE_THAT(z.x(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(z.y(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(z.z(), WithinAbs(1.0f, 0.0001f));
  }

  SECTION("Camera to the +X side") {
    OkRotation rot = OkBillboard::facingRotation(OkPoint(0.0f, 0.0f, 0.0f),
                                                 OkPoint(10.0f, 0.0f, 0.0f));
    OkPoint    z   = plusZ(rot);
    REQUIRE_THAT(z.x(), WithinAbs(1.0f, 0.0001f));
    REQUIRE_THAT(z.y(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(z.z(), WithinAbs(0.0f, 0.0001f));
  }

  SECTION("Camera straight above (top-down view)") {
    OkRotation rot = OkBillboard::facingRotation(OkPoint(0.0f, 0.0f, 0.0f),
                                                 OkPoint(0.0f, 10.0f, 0.0f));
    OkPoint    z   = plusZ(rot);
    REQUIRE_THAT(z.x(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(z.y(), WithinAbs(1.0f, 0.0001f));
    REQUIRE_THAT(z.z(), WithinAbs(0.0f, 0.0001f));
  }

  SECTION("Oblique camera, off-origin billboard") {
    OkPoint    from(5.0f, 2.0f, -3.0f);
    OkPoint    to(-7.0f, 14.0f, 9.0f);
    OkRotation rot = OkBillboard::facingRotation(from, to);
    OkPoint    z   = plusZ(rot);
    float      dx  = to.x() - from.x();
    float      dy  = to.y() - from.y();
    float      dz  = to.z() - from.z();
    float      dl  = std::sqrt(dx * dx + dy * dy + dz * dz);
    REQUIRE_THAT(z.x(), WithinAbs(dx / dl, 0.0001f));
    REQUIRE_THAT(z.y(), WithinAbs(dy / dl, 0.0001f));
    REQUIRE_THAT(z.z(), WithinAbs(dz / dl, 0.0001f));
  }

  SECTION("Degenerate (camera at the billboard) falls back to identity") {
    OkRotation rot = OkBillboard::facingRotation(OkPoint(1.0f, 2.0f, 3.0f),
                                                 OkPoint(1.0f, 2.0f, 3.0f));
    REQUIRE_THAT(rot.getPitch(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(rot.getYaw(), WithinAbs(0.0f, 0.0001f));
  }
}
