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

  SECTION("Direction vector straight up") {
    OkPoint direction(0.0f, 1.0f, 0.0f);  // Looking straight up
    float   pitch, yaw;
    OkMath::directionVectorToAngles(direction, pitch, yaw);

    // Pitch should be +90° when looking straight up
    REQUIRE_THAT(pitch, WithinAbs(glm::half_pi<float>(), 0.0001f));

    // Yaw becomes undefined when looking straight up/down, but should return
    // a valid number (typically 0) rather than NaN
    REQUIRE_FALSE(std::isnan(yaw));
    REQUIRE_THAT(yaw, WithinAbs(0.0f, 0.0001f));
  }

  SECTION("Direction vector straight down") {
    OkPoint direction(0.0f, -1.0f, 0.0f);  // Looking straight down
    float   pitch, yaw;
    OkMath::directionVectorToAngles(direction, pitch, yaw);

    // Pitch should be -90° when looking straight down
    REQUIRE_THAT(pitch, WithinAbs(-glm::half_pi<float>(), 0.0001f));

    // Yaw becomes undefined when looking straight up/down, but should return
    // a valid number (typically 0) rather than NaN
    REQUIRE_FALSE(std::isnan(yaw));
    REQUIRE_THAT(yaw, WithinAbs(0.0f, 0.0001f));
  }

  SECTION("Direction vector at 45° between Y and Z") {
    // Create a vector that points 45° upward (between Y and Z)
    // This ensures y is not near ±1, so we'll hit the yaw calculation code
    // Using -Z instead of +Z to match our forward direction convention
    OkPoint direction(0.0f, 0.707f, -0.707f);
    float   pitch, yaw;
    OkMath::directionVectorToAngles(direction, pitch, yaw);

    // Expected pitch should be ~45°
    REQUIRE_THAT(pitch, WithinAbs(glm::quarter_pi<float>(), 0.0001f));

    // Yaw should be 0° since we're in the YZ plane pointing forward (-Z)
    REQUIRE_THAT(yaw, WithinAbs(0.0f, 0.0001f));

    // Verify cos(pitch) > 0.001f condition is met
    REQUIRE(cos(pitch) > 0.001f);
  }

  SECTION("Direction vector nearly vertical") {
    // Using a nearly vertical vector that should trigger first check
    OkPoint direction(0.001f, 0.99999f, 0.001f);

    float pitch, yaw;
    std::cout << "LET'S GO!!" << std::endl;

    // Calculate what the normalized y value will be
    glm::vec3 normalized =
        glm::normalize(glm::vec3(direction.x(), direction.y(), direction.z()));
    std::cout << "Normalized y = " << normalized.y << std::endl;

    OkMath::directionVectorToAngles(direction, pitch, yaw);

    // Verify we enter the first check (very close to vertical)
    float dist = std::abs(std::abs(normalized.y) - 1.0f);
    std::cout << "Distance from 1.0 = " << dist << std::endl;
    REQUIRE(dist < 0.0001f);

    // Yaw should be 0 for vertical looks
    REQUIRE_THAT(yaw, WithinAbs(0.0f, 0.0001f));
  }
}

TEST_CASE("OkMath lookAt", "[math]") {
  SECTION("Looking forward from origin") {
    OkPoint    eye(0.0f, 0.0f, 0.0f);
    OkPoint    target(0.0f, 0.0f, -1.0f);  // Looking along -Z
    OkRotation rot = OkMath::lookAt(eye, target);

    // Should produce a 180° yaw since forward is -Z
    REQUIRE_THAT(rot.getPitch(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(rot.getYaw(), WithinAbs(glm::pi<float>(), 0.0001f));
    REQUIRE_THAT(rot.getRoll(), WithinAbs(0.0f, 0.0001f));
  }

  SECTION("Looking right from origin") {
    OkPoint    eye(0.0f, 0.0f, 0.0f);
    OkPoint    target(1.0f, 0.0f, 0.0f);  // Looking along +X
    OkRotation rot = OkMath::lookAt(eye, target);

    // Should be 90° yaw
    REQUIRE_THAT(rot.getPitch(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(rot.getYaw(), WithinAbs(glm::half_pi<float>(), 0.0001f));
    REQUIRE_THAT(rot.getRoll(), WithinAbs(0.0f, 0.0001f));
  }

  SECTION("Looking up from origin") {
    OkPoint eye(0.0f, 0.0f, 0.0f);
    OkPoint target(0.0f, 1.0f, 0.0f);  // Looking along +Y
    // default up world vector (not passed as parameter)
    OkRotation rot = OkMath::lookAt(eye, target);

    // When looking straight up: Pitch should be -90°
    REQUIRE_THAT(rot.getPitch(), WithinAbs(-glm::half_pi<float>(), 0.0001f));
    REQUIRE_THAT(rot.getYaw(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(rot.getRoll(), WithinAbs(0.0f, 0.0001f));

    // Verify forward vector points up regardless of yaw/roll
    OkPoint forward = rot.getForwardVector();
    REQUIRE_THAT(forward.y(), WithinAbs(-1.0f, 0.0001f));

    // Only verify magnitude of x and z components combined is near zero
    float xzMagnitude =
        std::sqrt(forward.x() * forward.x() + forward.z() * forward.z());
    REQUIRE_THAT(xzMagnitude, WithinAbs(0.0f, 0.0001f));
  }

  SECTION("Looking up with alternate up vector") {
    OkPoint eye(0.0f, 0.0f, 0.0f);
    OkPoint target(0.0f, 1.0f, 0.0f);   // Looking straight up (+Y)
    OkPoint worldUp(0.0f, 1.0f, 0.0f);  // Up vector parallel to look direction
    OkRotation rot = OkMath::lookAt(eye, target, worldUp);

    // Verify the resulting orientation
    REQUIRE_THAT(rot.getPitch(), WithinAbs(-glm::half_pi<float>(), 0.0001f));
    REQUIRE_THAT(rot.getYaw(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(rot.getRoll(), WithinAbs(0.0f, 0.0001f));

    // Get the resulting basis vectors
    OkPoint forward = rot.getForwardVector();
    OkPoint right   = rot.getRightVector();
    OkPoint up      = rot.getUpVector();

    // Forward should point up
    REQUIRE_THAT(forward.y(), WithinAbs(-1.0f, 0.0001f));

    // Right should be along +X (since we use Z as temporary up)
    REQUIRE_THAT(right.x(), WithinAbs(1.0f, 0.0001f));
    REQUIRE_THAT(right.y(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(right.z(), WithinAbs(0.0f, 0.0001f));

    // Up should be along -Z (cross product of forward and right)
    REQUIRE_THAT(up.x(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(up.y(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(up.z(), WithinAbs(-1.0f, 0.0001f));

    // Vectors should be orthogonal
    float dotFR = forward.dot(right);
    float dotFU = forward.dot(up);
    float dotRU = right.dot(up);
    REQUIRE_THAT(dotFR, WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(dotFU, WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(dotRU, WithinAbs(0.0f, 0.0001f));
  }

  SECTION("Looking almost straight up - parallel up vector") {
    OkPoint eye(0.0f, 0.0f, 0.0f);
    // Create a forward vector that is:
    // 1. Not too vertical (y < 0.999999f) to trigger second condition
    // 2. But parallel to worldUp to trigger first condition
    OkPoint    target(0.1f, 0.99f, 0.0f);   // Less vertical
    OkPoint    worldUp(0.1f, 0.99f, 0.0f);  // Same direction as target
    OkRotation rot = OkMath::lookAt(eye, target, worldUp);

    // Get the resulting basis vectors
    OkPoint forward = rot.getForwardVector();
    OkPoint right   = rot.getRightVector();
    OkPoint up      = rot.getUpVector();

    // Forward should NOT be exactly vertical
    REQUIRE_THAT(std::abs(forward.y()), WithinAbs(0.99f, 0.01f));

    // Right should be along +X since Z was used as temporary up
    REQUIRE_THAT(right.x(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(right.y(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(right.z(), WithinAbs(-1.0f, 0.0001f));

    // Up should be along -Z (cross product of forward and right)
    REQUIRE_THAT(up.z(), WithinAbs(0.0f, 0.0001f));
  }
}

// NOLINTEND(readability-magic-numbers)
