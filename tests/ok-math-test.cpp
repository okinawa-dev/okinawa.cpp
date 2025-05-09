// NOLINTBEGIN(readability-magic-numbers)

#include "../src/math/math.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>

using Catch::Matchers::WithinAbs;

/*
TEST_CASE("OkMath angles to direction vector", "[math]") {
  SECTION("Forward direction (0,0,0)") {
    OkPoint direction = OkMath::anglesToDirectionVector(0.0f, 0.0f, 0.0f);
    REQUIRE_THAT(direction.x(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(direction.y(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(direction.z(), WithinAbs(1.0f, 0.0001f));
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
    REQUIRE_THAT(direction.x(), WithinAbs(1.0f, 0.0001f));
    REQUIRE_THAT(direction.y(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(direction.z(), WithinAbs(0.0f, 0.0001f));
  }

  SECTION("Looking up-right (45º pitch, 45º yaw)") {
    OkPoint direction =
        OkMath::anglesToDirectionVector(glm::quarter_pi<float>(),  // 45º pitch
                                        glm::quarter_pi<float>(),  // 45º yaw
                                        0.0f);
    // Print raw values before normalization
    std::cout << "Raw: (" << direction.x() << ", " << direction.y() << ", "
              << direction.z() << ")\n";
    // For 45º up and 45º right, each component should be ±1/√3
    float expected = 0.57735026919f;  // 1/√3
    REQUIRE_THAT(direction.x(), WithinAbs(expected, 0.0001f));
    REQUIRE_THAT(direction.y(), WithinAbs(expected, 0.0001f));
    REQUIRE_THAT(direction.z(), WithinAbs(-expected, 0.0001f));
  }

  SECTION("Looking slightly up-left (30º pitch, -60º yaw)") {
    OkPoint direction =
        OkMath::anglesToDirectionVector(glm::pi<float>() / 6.0f,   // 30º pitch
                                        -glm::pi<float>() / 3.0f,  // -60º yaw
                                        0.0f);
    // x = sin(-60°)cos(30°) = -0.866 * 0.866 = -0.75
    // y = sin(30°) = 0.5
    // z = -cos(-60°)cos(30°) = -0.5 * 0.866 = -0.433
    REQUIRE_THAT(direction.x(), WithinAbs(-0.75f, 0.0001f));
    REQUIRE_THAT(direction.y(), WithinAbs(0.5f, 0.0001f));
    REQUIRE_THAT(direction.z(), WithinAbs(0.433f, 0.0001f));
  }

  SECTION("Looking down-right (negative pitch)") {
    OkPoint direction = OkMath::anglesToDirectionVector(
        -glm::quarter_pi<float>(),  // -45º pitch
        glm::quarter_pi<float>(),   // 45º yaw
        0.0f);
    float expected = 0.577350269f;  // 1/√3
    REQUIRE_THAT(direction.x(), WithinAbs(expected, 0.0001f));
    REQUIRE_THAT(direction.y(), WithinAbs(-expected, 0.0001f));
    REQUIRE_THAT(direction.z(), WithinAbs(-expected, 0.0001f));
  }
}
*/

TEST_CASE("OkMath direction vector to angles", "[math]") {
  SECTION("Direction vector (0,0,-1)") {
    OkPoint direction(0.0f, 0.0f, -1.0f);
    float   pitch, yaw;
    OkMath::directionVectorToAngles(direction, pitch, yaw);
    REQUIRE_THAT(pitch, WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(yaw, WithinAbs(0.0f, 0.0001f));
  }

  SECTION("Direction vector (1,0,0)") {
    OkPoint direction(1.0f, 0.0f, 0.0f);
    float   pitch, yaw;
    OkMath::directionVectorToAngles(direction, pitch, yaw);
    REQUIRE_THAT(pitch, WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(yaw, WithinAbs(glm::half_pi<float>(), 0.0001f));
  }

  SECTION("Direction vector (0,-1,-1)") {
    OkPoint direction(0.0f, -1.0f, -1.0f);
    float   pitch, yaw;
    OkMath::directionVectorToAngles(direction, pitch, yaw);
    // Expected values for pitch and yaw
    float expectedPitch = -glm::pi<float>() / 4.0f;  // -45 degrees
    float expectedYaw   = 0.0f;                      // 0 degrees
    REQUIRE_THAT(pitch, WithinAbs(expectedPitch, 0.0001f));
    REQUIRE_THAT(yaw, WithinAbs(expectedYaw, 0.0001f));
  }
}

// NOLINTEND(readability-magic-numbers)
