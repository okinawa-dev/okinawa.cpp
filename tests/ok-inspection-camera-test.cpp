#include "okinawa/cameras/inspection_camera.hpp"
#include "okinawa/math/point.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>

using Catch::Matchers::WithinAbs;

// The camera a tool looks at a scene with. All of it is arithmetic on the
// camera's own transform, so none of it needs a GL context.

namespace {

  const int WIDTH  = 800;
  const int HEIGHT = 600;

  // A camera 100 m over the origin, looking down at it.
  OkInspectionCamera *overhead() {
    auto *camera = new OkInspectionCamera("inspect", WIDTH, HEIGHT);
    camera->setPerspective(60.0f, 0.5f, 5000.0f);
    camera->setPosition(0.0f, 100.0f, 0.0f);
    camera->setRotation(-1.4f, 0.0f, 0.0f);  // ~ -80 degrees, near vertical
    camera->updateTransform();
    return camera;
  }

  float distanceBetween(const OkPoint &a, const OkPoint &b) {
    return a.distance(b);
  }

}  // namespace

TEST_CASE("OkInspectionCamera scales gestures by height", "[inspection]") {
  OkInspectionCamera *camera = overhead();
  camera->setGroundHeight(0.0f);
  camera->setScaleRange(5.0f, 900.0f);

  SECTION("The scale is the height above the ground") {
    REQUIRE_THAT(camera->gestureScale(), WithinAbs(100.0f, 0.001f));
  }

  SECTION("Measured from the ground the caller set, not from zero") {
    camera->setGroundHeight(60.0f);
    REQUIRE_THAT(camera->gestureScale(), WithinAbs(40.0f, 0.001f));
  }

  // Without the clamps the wheel crawls when the whole scene is in view
  // and throws the camera through a wall when a doorway is.
  SECTION("Clamped at the bottom, so a camera on the floor still moves") {
    camera->setPosition(0.0f, 0.1f, 0.0f);
    camera->updateTransform();
    REQUIRE_THAT(camera->gestureScale(), WithinAbs(5.0f, 0.001f));
  }

  SECTION("Clamped at the top, so a camera in orbit does not fly off") {
    camera->setPosition(0.0f, 40000.0f, 0.0f);
    camera->updateTransform();
    REQUIRE_THAT(camera->gestureScale(), WithinAbs(900.0f, 0.001f));
  }

  SECTION("Below the ground the scale is the floor, never negative") {
    camera->setPosition(0.0f, -50.0f, 0.0f);
    camera->updateTransform();
    REQUIRE(camera->gestureScale() > 0.0f);
    REQUIRE_THAT(camera->gestureScale(), WithinAbs(5.0f, 0.001f));
  }

  delete camera;
}

TEST_CASE("OkInspectionCamera zooms toward a point", "[inspection]") {
  OkInspectionCamera *camera = overhead();
  camera->setGroundHeight(0.0f);
  camera->setScaleRange(5.0f, 900.0f);
  const OkPoint target(0.0f, 0.0f, 0.0f);

  SECTION("A notch in moves it closer to the point, along the line to it") {
    float before = distanceBetween(camera->getPosition(), target);
    camera->zoomToward(target, 1.0f);
    float after = distanceBetween(camera->getPosition(), target);
    REQUIRE(after < before);
    // Straight at it: nothing sideways.
    REQUIRE_THAT(camera->getPosition().x(), WithinAbs(0.0f, 0.001f));
    REQUIRE_THAT(camera->getPosition().z(), WithinAbs(0.0f, 0.001f));
  }

  SECTION("A notch out moves it away") {
    float before = distanceBetween(camera->getPosition(), target);
    camera->zoomToward(target, -1.0f);
    REQUIRE(distanceBetween(camera->getPosition(), target) > before);
  }

  // The one rule that makes zooming at a point usable: it approaches and
  // never arrives. Otherwise a fast wheel puts the camera inside the wall
  // it was aimed at, and what it was looking at is behind it.
  SECTION("However hard it is pushed, it stops short of the point") {
    for (int i = 0; i < 200; i++) {
      camera->zoomToward(target, 50.0f);
    }
    float left = distanceBetween(camera->getPosition(), target);
    REQUIRE(left > 0.0f);
    REQUIRE(left >= OkInspectionCamera::MIN_REACH - 0.001f);
  }

  SECTION("Zooming along the view needs no point at all") {
    OkPoint before = camera->getPosition();
    camera->zoomAlongView(1.0f);
    REQUIRE(camera->getPosition().y() < before.y());  // it looks downwards
  }

  delete camera;
}

TEST_CASE("OkInspectionCamera pans across the view", "[inspection]") {
  OkInspectionCamera *camera = overhead();
  camera->setGroundHeight(0.0f);
  camera->setScaleRange(5.0f, 900.0f);

  SECTION("Dragging right moves the camera left, so the scene follows") {
    // The signs are "take hold of the scene and move it", not "move the
    // camera": reverse them and everything appears to run backwards.
    float before = camera->getPosition().x();
    camera->panAcrossView(10.0f, 0.0f);
    REQUIRE(camera->getPosition().x() < before);
  }

  SECTION("A drag with no pixels in it moves nothing") {
    OkPoint before = camera->getPosition();
    camera->panAcrossView(0.0f, 0.0f);
    REQUIRE(camera->getPosition() == before);
  }

  SECTION("The higher it is, the further the same drag takes it") {
    camera->setPosition(0.0f, 50.0f, 0.0f);
    camera->updateTransform();
    float low = camera->getPosition().x();
    camera->panAcrossView(10.0f, 0.0f);
    float lowTravel = std::fabs(camera->getPosition().x() - low);

    camera->setPosition(0.0f, 500.0f, 0.0f);
    camera->updateTransform();
    float high = camera->getPosition().x();
    camera->panAcrossView(10.0f, 0.0f);
    float highTravel = std::fabs(camera->getPosition().x() - high);

    REQUIRE(highTravel > lowTravel);
  }

  delete camera;
}

TEST_CASE("OkInspectionCamera orbits about a point", "[inspection]") {
  OkInspectionCamera *camera = overhead();
  camera->setGroundHeight(0.0f);
  const OkPoint pivot(0.0f, 0.0f, 0.0f);

  SECTION("The pivot stays where it is: only the camera goes round it") {
    camera->setPosition(0.0f, 60.0f, 60.0f);
    camera->setRotation(-0.785f, 0.0f, 0.0f);
    camera->updateTransform();
    float before = distanceBetween(camera->getPosition(), pivot);

    REQUIRE(camera->orbitAbout(pivot, 30.0f, 0.0f));

    float after = distanceBetween(camera->getPosition(), pivot);
    REQUIRE_THAT(after, WithinAbs(before, 0.01f));
    // It actually moved, rather than being refused quietly.
    REQUIRE(camera->getPosition().z() != 60.0f);
  }

  // At either extreme the view matrix has no up direction left to work
  // with, and the picture rolls over.
  SECTION("It refuses to go under the ground") {
    camera->setPosition(0.0f, 5.0f, 60.0f);
    camera->setRotation(0.0f, 0.0f, 0.0f);
    camera->updateTransform();
    OkPoint before = camera->getPosition();
    REQUIRE_FALSE(camera->orbitAbout(pivot, 0.0f, 60.0f));
    REQUIRE(camera->getPosition() == before);
  }

  SECTION("It refuses to come closer than the reach") {
    camera->setPosition(0.0f, 1.0f, 0.5f);
    camera->updateTransform();
    OkPoint before = camera->getPosition();
    REQUIRE_FALSE(camera->orbitAbout(pivot, 20.0f, 0.0f));
    REQUIRE(camera->getPosition() == before);
  }

  delete camera;
}

TEST_CASE("OkInspectionCamera reports its height as the view distance",
          "[inspection]") {
  // What every other part of the engine scales by: OkPanController
  // multiplies its pixels by it, and a base camera reports 0, which
  // multiplies a tool's whole gesture away.
  OkInspectionCamera *camera = overhead();
  camera->setGroundHeight(20.0f);
  REQUIRE_THAT(camera->viewDistance(), WithinAbs(80.0f, 0.001f));

  camera->setViewDistance(300.0f);
  REQUIRE_THAT(camera->getPosition().y(), WithinAbs(320.0f, 0.001f));
  REQUIRE_THAT(camera->viewDistance(), WithinAbs(300.0f, 0.001f));

  delete camera;
}
