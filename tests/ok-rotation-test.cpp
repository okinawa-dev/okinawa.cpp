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
  SECTION("Transform point with identity") {
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
    REQUIRE_THAT(transformed.z(), WithinAbs(-1.0f, 0.0001f));
  }

  SECTION("90 degree Y rotation plus 90 degree X rotation") {
    OkRotation rot1(0.0f, glm::half_pi<float>(), 0.0f);  // 90 degree Y rotation
    OkRotation rot2(glm::half_pi<float>(), 0.0f, 0.0f);  // 90 degree X rotation
    OkPoint    p(1.0f, 0.0f, 0.0f);
    OkPoint    transformed = rot1.transformPoint(p);
    transformed            = rot2.transformPoint(transformed);
    // Expected result after both rotations
    // (0, 1, 0)

    REQUIRE_THAT(transformed.x(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(transformed.y(), WithinAbs(1.0f, 0.0001f));
    REQUIRE_THAT(transformed.z(), WithinAbs(0.0f, 0.0001f));
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

TEST_CASE("OkRotation vectors", "[rotation]") {
  SECTION("Forward vector with no rotation") {
    OkRotation noRotation;
    OkPoint    forward = noRotation.getForwardVector();
    REQUIRE_THAT(forward.x(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(forward.y(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(forward.z(), WithinAbs(-1.0f, 0.0001f));
  }

  SECTION("Right vector with no rotation") {
    OkRotation noRotation;
    OkPoint    right = noRotation.getRightVector();
    REQUIRE_THAT(right.x(), WithinAbs(1.0f, 0.0001f));
    REQUIRE_THAT(right.y(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(right.z(), WithinAbs(0.0f, 0.0001f));
  }

  SECTION("Up vector with no rotation") {
    OkRotation noRotation;
    OkPoint    up = noRotation.getUpVector();
    REQUIRE_THAT(up.x(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(up.y(), WithinAbs(1.0f, 0.0001f));
    REQUIRE_THAT(up.z(), WithinAbs(0.0f, 0.0001f));
  }

  SECTION("Orthogonal vectors") {
    OkRotation rot(0.5f, 1.0f, 0.0f);  // Some arbitrary rotation
    OkPoint    forward = rot.getForwardVector();
    OkPoint    right   = rot.getRightVector();
    OkPoint    up      = rot.getUpVector();

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

  SECTION("Forward vector for -45 degree pitch") {
    // Create rotation with -45° pitch (looking down)
    OkRotation rot(-glm::pi<float>() / 4.0f, 0.0f,
                   0.0f);  // -45° pitch, 0° yaw, 0° roll

    // Get forward vector
    OkPoint forward = rot.getForwardVector();

    // The forward vector should be (0, -1, -1) normalized
    float expected = 1.0f / std::sqrt(2.0f);  // 1/√2 ≈ 0.707107
    REQUIRE_THAT(forward.x(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(forward.y(), WithinAbs(-expected, 0.0001f));
    REQUIRE_THAT(forward.z(), WithinAbs(-expected, 0.0001f));
  }
}

// NOLINTEND(readability-magic-numbers)
