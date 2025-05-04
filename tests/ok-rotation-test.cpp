// NOLINTBEGIN(readability-magic-numbers)

#include "../src/math/rotation.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <glm/gtc/constants.hpp>

using Catch::Matchers::WithinAbs;

TEST_CASE("OkRotation basic operations", "[rotation]") {
  SECTION("Default constructor") {
    OkRotation  rot;
    const auto &angles = rot.getAngles();
    REQUIRE_THAT(angles.x, WithinAbs(0.0f, 0.0001f));  // pitch
    REQUIRE_THAT(angles.y, WithinAbs(0.0f, 0.0001f));  // yaw
    REQUIRE_THAT(angles.z, WithinAbs(0.0f, 0.0001f));  // roll
  }

  SECTION("Constructor with angles") {
    float      pitch = glm::radians(30.0f);
    float      yaw   = glm::radians(45.0f);
    float      roll  = glm::radians(60.0f);
    OkRotation rot(pitch, yaw, roll);

    REQUIRE_THAT(rot.getPitch(), WithinAbs(pitch, 0.0001f));
    REQUIRE_THAT(rot.getYaw(), WithinAbs(yaw, 0.0001f));
    REQUIRE_THAT(rot.getRoll(), WithinAbs(roll, 0.0001f));
  }
}

TEST_CASE("OkRotation transformations", "[rotation]") {
  SECTION("Transform point forward") {
    OkRotation rot;  // Identity rotation
    OkPoint    p(1.0f, 0.0f, 0.0f);
    OkPoint    transformed = rot.transformPoint(p);

    REQUIRE_THAT(transformed.x(), WithinAbs(1.0f, 0.0001f));
    REQUIRE_THAT(transformed.y(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(transformed.z(), WithinAbs(0.0f, 0.0001f));
  }

  SECTION("90 degree Y rotation") {
    OkRotation rot(0.0f, glm::half_pi<float>(), 0.0f);
    OkPoint    p(1.0f, 0.0f, 0.0f);
    OkPoint    transformed = rot.transformPoint(p);

    REQUIRE_THAT(transformed.x(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(transformed.y(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(transformed.z(), WithinAbs(1.0f, 0.0001f));
  }
}

TEST_CASE("OkRotation modifications", "[rotation]") {
  SECTION("Rotate incrementally") {
    OkRotation rot;
    rot.rotate(0.1f, 0.2f, 0.3f);

    REQUIRE_THAT(rot.getPitch(), WithinAbs(0.1f, 0.0001f));
    REQUIRE_THAT(rot.getYaw(), WithinAbs(0.2f, 0.0001f));
    REQUIRE_THAT(rot.getRoll(), WithinAbs(0.3f, 0.0001f));
  }

  SECTION("Set absolute rotation") {
    OkRotation rot;
    rot.setRotation(0.5f, 1.0f, 1.5f);

    REQUIRE_THAT(rot.getPitch(), WithinAbs(0.5f, 0.0001f));
    REQUIRE_THAT(rot.getYaw(), WithinAbs(1.0f, 0.0001f));
    REQUIRE_THAT(rot.getRoll(), WithinAbs(1.5f, 0.0001f));
  }
}

TEST_CASE("OkRotation comparison operations", "[rotation]") {
  SECTION("Equal rotations") {
    OkRotation rot1(0.1f, 0.2f, 0.3f);
    OkRotation rot2(0.1f, 0.2f, 0.3f);
    REQUIRE(rot1 == rot2);
  }

  SECTION("Different rotations") {
    OkRotation rot1(0.1f, 0.2f, 0.3f);
    OkRotation rot2(0.1f, 0.2f, 0.4f);
    REQUIRE_FALSE(rot1 == rot2);
  }
}

TEST_CASE("OkRotation string representation", "[rotation]") {
  SECTION("Default rotation toString") {
    OkRotation rot;
    REQUIRE(rot.toString() == "(0, 0, 0)");
  }

  SECTION("Custom rotation toString") {
    OkRotation rot(1.0f, 2.0f, 3.0f);
    REQUIRE(rot.toString() == "(1, 2, 3)");
  }
}

// NOLINTEND(readability-magic-numbers)
