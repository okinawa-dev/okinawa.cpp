// NOLINTBEGIN(readability-magic-numbers)

#include "../src/math/math.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <glm/gtc/constants.hpp>

using Catch::Matchers::WithinAbs;

TEST_CASE("OkMath angles to direction vector", "[math]") {
  SECTION("Forward direction (0,0,0)") {
    OkPoint direction = OkMath::anglesToDirectionVector(0.0f, 0.0f, 0.0f);
    REQUIRE_THAT(direction.x(), WithinAbs(1.0f, 0.0001f));
    REQUIRE_THAT(direction.y(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(direction.z(), WithinAbs(0.0f, 0.0001f));
  }

  SECTION("Looking up (90 degrees pitch)") {
    OkPoint direction =
        OkMath::anglesToDirectionVector(glm::half_pi<float>(), 0.0f, 0.0f);
    REQUIRE_THAT(direction.x(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(direction.y(), WithinAbs(1.0f, 0.0001f));
    REQUIRE_THAT(direction.z(), WithinAbs(0.0f, 0.0001f));
  }

  SECTION("Looking right (90 degrees yaw)") {
    OkPoint direction =
        OkMath::anglesToDirectionVector(0.0f, glm::half_pi<float>(), 0.0f);
    REQUIRE_THAT(direction.x(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(direction.y(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(direction.z(), WithinAbs(1.0f, 0.0001f));
  }
}

TEST_CASE("OkMath rotation vectors", "[math]") {
  SECTION("Forward vector with no rotation") {
    OkRotation noRotation;
    OkPoint    forward = OkMath::getForwardVector(noRotation);
    REQUIRE_THAT(forward.x(), WithinAbs(1.0f, 0.0001f));
    REQUIRE_THAT(forward.y(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(forward.z(), WithinAbs(0.0f, 0.0001f));
  }

  SECTION("Right vector with no rotation") {
    OkRotation noRotation;
    OkPoint    right = OkMath::getRightVector(noRotation);
    REQUIRE_THAT(right.x(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(right.y(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(right.z(), WithinAbs(1.0f, 0.0001f));
  }

  SECTION("Up vector with no rotation") {
    OkRotation noRotation;
    OkPoint    up = OkMath::getUpVector(noRotation);
    REQUIRE_THAT(up.x(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(up.y(), WithinAbs(-1.0f, 0.0001f));
    REQUIRE_THAT(up.z(), WithinAbs(0.0f, 0.0001f));
  }

  SECTION("Orthogonal vectors") {
    OkRotation rot(0.5f, 1.0f, 0.0f);  // Some arbitrary rotation
    OkPoint    forward = OkMath::getForwardVector(rot);
    OkPoint    right   = OkMath::getRightVector(rot);
    OkPoint    up      = OkMath::getUpVector(rot);

    // Check if vectors are perpendicular (dot product should be close to 0)
    float dotFR = forward.x() * right.x() + forward.y() * right.y() +
                  forward.z() * right.z();
    float dotFU =
        forward.x() * up.x() + forward.y() * up.y() + forward.z() * up.z();
    float dotRU = right.x() * up.x() + right.y() * up.y() + right.z() * up.z();

    REQUIRE_THAT(dotFR, WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(dotFU, WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(dotRU, WithinAbs(0.0f, 0.0001f));
  }
}

// NOLINTEND(readability-magic-numbers)
