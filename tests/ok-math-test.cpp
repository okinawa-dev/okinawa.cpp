// NOLINTBEGIN(readability-magic-numbers)

#include "okinawa/math/math.hpp"
#include "okinawa/math/point.hpp"
#include "okinawa/math/rotation.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/constants.hpp>

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
    float   pitch;
    float   yaw;
    OkMath::directionVectorToAngles(direction, pitch, yaw);
    REQUIRE_THAT(pitch, WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(yaw, WithinAbs(0.0f, 0.0001f));
  }

  SECTION("Direction vector (1,0,0)") {
    OkPoint direction(1.0f, 0.0f, 0.0f);
    float   pitch;
    float   yaw;
    OkMath::directionVectorToAngles(direction, pitch, yaw);
    // Facing world +X needs yaw = -90°: getForwardVector(0, -pi/2) =
    // (-sin(-pi/2), 0, -cos(-pi/2)) = (1, 0, 0).
    REQUIRE_THAT(pitch, WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(yaw, WithinAbs(-glm::half_pi<float>(), 0.0001f));
  }

  SECTION("Direction vector (0,-1,-1)") {
    OkPoint direction(0.0f, -1.0f, -1.0f);
    float   pitch;
    float   yaw;
    OkMath::directionVectorToAngles(direction, pitch, yaw);
    // Expected values for pitch and yaw
    float expectedPitch = -glm::pi<float>() / 4.0f;  // -45 degrees
    float expectedYaw   = 0.0f;                      // 0 degrees
    REQUIRE_THAT(pitch, WithinAbs(expectedPitch, 0.0001f));
    REQUIRE_THAT(yaw, WithinAbs(expectedYaw, 0.0001f));
  }

  SECTION("Direction vector straight up") {
    OkPoint direction(0.0f, 1.0f, 0.0f);  // Looking straight up
    float   pitch;
    float   yaw;
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
    float   pitch;
    float   yaw;
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
    float   pitch;
    float   yaw;
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
    float   pitch;
    float   yaw;

    // Calculate what the normalized y value will be
    // glm::vec3 normalized =
    //     glm::normalize(glm::vec3(direction.x(), direction.y(),
    //     direction.z()));

    OkMath::directionVectorToAngles(direction, pitch, yaw);

    // Yaw should be 0 for vertical looks
    REQUIRE_THAT(yaw, WithinAbs(0.0f, 0.0001f));
  }
}

TEST_CASE("OkMath lookAt", "[math]") {
  // lookAt must orient a camera so getForwardVector() points from eye to
  // target. The camera ignores roll (its view is built from
  // getForwardVector/getUpVector), so lookAt is roll-free; `up` only resolves
  // the screen orientation when looking straight up or down.

  SECTION("Looking forward (-Z) from origin") {
    OkPoint    eye(0.0f, 0.0f, 0.0f);
    OkPoint    target(0.0f, 0.0f, -1.0f);  // -Z is the default forward
    OkRotation rot = OkMath::lookAt(eye, target);

    REQUIRE_THAT(rot.getPitch(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(rot.getYaw(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(rot.getRoll(), WithinAbs(0.0f, 0.0001f));

    OkPoint forward = rot.getForwardVector();
    REQUIRE_THAT(forward.x(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(forward.y(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(forward.z(), WithinAbs(-1.0f, 0.0001f));
  }

  SECTION("Looking right (+X) from origin") {
    OkPoint    eye(0.0f, 0.0f, 0.0f);
    OkPoint    target(1.0f, 0.0f, 0.0f);  // +X needs yaw = -90°
    OkRotation rot = OkMath::lookAt(eye, target);

    REQUIRE_THAT(rot.getPitch(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(rot.getYaw(), WithinAbs(-glm::half_pi<float>(), 0.0001f));

    OkPoint forward = rot.getForwardVector();
    REQUIRE_THAT(forward.x(), WithinAbs(1.0f, 0.0001f));
    REQUIRE_THAT(forward.y(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(forward.z(), WithinAbs(0.0f, 0.0001f));
  }

  SECTION("Looking up (+Y): forward points up") {
    OkPoint    eye(0.0f, 0.0f, 0.0f);
    OkPoint    target(0.0f, 1.0f, 0.0f);  // straight up
    OkRotation rot = OkMath::lookAt(eye, target);

    REQUIRE_THAT(rot.getPitch(), WithinAbs(glm::half_pi<float>(), 0.0001f));

    OkPoint forward = rot.getForwardVector();
    REQUIRE_THAT(forward.y(), WithinAbs(1.0f, 0.0001f));
    float xzMagnitude =
        std::sqrt(forward.x() * forward.x() + forward.z() * forward.z());
    REQUIRE_THAT(xzMagnitude, WithinAbs(0.0f, 0.0001f));
  }

  SECTION("Top-down with north-up: up vector picks the screen orientation") {
    OkPoint    eye(0.0f, 100.0f, 0.0f);
    OkPoint    target(0.0f, 0.0f, 0.0f);  // straight down
    OkPoint    north(0.0f, 0.0f, 1.0f);   // world +Z should be up on screen
    OkRotation rot = OkMath::lookAt(eye, target, north);

    OkPoint forward = rot.getForwardVector();
    OkPoint up      = rot.getUpVector();

    // Looks straight down...
    REQUIRE_THAT(forward.y(), WithinAbs(-1.0f, 0.0001f));
    // ...with world +Z as the on-screen up.
    REQUIRE_THAT(up.x(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(up.y(), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(up.z(), WithinAbs(1.0f, 0.0001f));
  }
}

TEST_CASE("OkMath forward/angle round-trip", "[math]") {
  // directionVectorToAngles and lookAt must invert OkRotation::getForwardVector
  // (the convention the camera renders). Round-trip a spread of orientations.
  const float pitches[] = {-1.2f, -0.6f, 0.0f, 0.4f, 1.0f};
  const float yaws[]    = {-3.0f, -1.5f, -0.3f, 0.0f, 0.7f, 2.2f, 3.0f};

  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 7; j++) {
      OkRotation source(pitches[i], yaws[j], 0.0f);
      OkPoint    f = source.getForwardVector();

      // directionVectorToAngles -> rebuild forward -> must match.
      float p;
      float y;
      OkMath::directionVectorToAngles(f, p, y);
      OkPoint rebuilt = OkRotation(p, y, 0.0f).getForwardVector();
      REQUIRE_THAT(rebuilt.x(), WithinAbs(f.x(), 0.001f));
      REQUIRE_THAT(rebuilt.y(), WithinAbs(f.y(), 0.001f));
      REQUIRE_THAT(rebuilt.z(), WithinAbs(f.z(), 0.001f));

      // lookAt from origin toward f must reproduce the same forward.
      OkPoint lookForward =
          OkMath::lookAt(OkPoint(0.0f, 0.0f, 0.0f), f).getForwardVector();
      REQUIRE_THAT(lookForward.x(), WithinAbs(f.x(), 0.001f));
      REQUIRE_THAT(lookForward.y(), WithinAbs(f.y(), 0.001f));
      REQUIRE_THAT(lookForward.z(), WithinAbs(f.z(), 0.001f));
    }
  }
}

// NOLINTEND(readability-magic-numbers)
