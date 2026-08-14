#include "okinawa/core/camera.hpp"
#include "okinawa/math/point.hpp"
#include "okinawa/math/ray.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>

using Catch::Matchers::WithinAbs;

// The camera's own arithmetic only -- matrices and unprojection, no GL.
// A camera looking down -Z from the origin is the engine's default
// orientation (see math/readme.md), so these are written against it.

namespace {

  const int WIDTH  = 800;
  const int HEIGHT = 600;

  OkCamera *lookingForward() {
    auto *camera = new OkCamera("test", WIDTH, HEIGHT);
    camera->setPerspective(45.0f, 0.1f, 1000.0f);
    camera->setPosition(0.0f, 0.0f, 0.0f);
    camera->setRotation(0.0f, 0.0f, 0.0f);
    camera->updateTransform();
    return camera;
  }

}  // namespace

TEST_CASE("OkCamera ray through a pixel", "[camera]") {
  OkCamera *camera = lookingForward();

  SECTION("The middle of the window looks where the camera looks") {
    OkRay ray =
        camera->rayThroughPixel(WIDTH / 2.0, HEIGHT / 2.0, WIDTH, HEIGHT);
    REQUIRE_THAT(ray.direction.x(), WithinAbs(0.0f, 0.001f));
    REQUIRE_THAT(ray.direction.y(), WithinAbs(0.0f, 0.001f));
    REQUIRE_THAT(ray.direction.z(), WithinAbs(-1.0f, 0.001f));
  }

  SECTION("The direction is a unit vector, so distances are world units") {
    OkRay ray = camera->rayThroughPixel(120.0, 90.0, WIDTH, HEIGHT);
    REQUIRE_THAT(ray.direction.magnitude(), WithinAbs(1.0f, 0.0001f));
  }

  // The half of the flip that a round trip alone cannot catch: swap the
  // two and the round trip still closes, while everything on screen is
  // upside down.
  SECTION("Up the window is up the world") {
    OkRay high = camera->rayThroughPixel(WIDTH / 2.0, 10.0, WIDTH, HEIGHT);
    OkRay low =
        camera->rayThroughPixel(WIDTH / 2.0, HEIGHT - 10.0, WIDTH, HEIGHT);
    REQUIRE(high.direction.y() > 0.0f);
    REQUIRE(low.direction.y() < 0.0f);
  }

  SECTION("The right of the window is the right of the world") {
    OkRay left = camera->rayThroughPixel(10.0, HEIGHT / 2.0, WIDTH, HEIGHT);
    OkRay right =
        camera->rayThroughPixel(WIDTH - 10.0, HEIGHT / 2.0, WIDTH, HEIGHT);
    REQUIRE(left.direction.x() < 0.0f);
    REQUIRE(right.direction.x() > 0.0f);
  }

  SECTION("A window with no pixels gives a ray rather than a division") {
    OkRay ray = camera->rayThroughPixel(0.0, 0.0, 0, 0);
    REQUIRE_THAT(ray.direction.z(), WithinAbs(-1.0f, 0.0001f));
  }

  delete camera;
}

TEST_CASE("OkCamera pixel of a world point", "[camera]") {
  OkCamera *camera = lookingForward();

  SECTION("A point straight ahead lands in the middle") {
    double x = 0.0;
    double y = 0.0;
    REQUIRE(camera->pixelOfPoint(OkPoint(0.0f, 0.0f, -10.0f), WIDTH, HEIGHT, &x,
                                 &y));
    REQUIRE_THAT(x, WithinAbs(WIDTH / 2.0, 0.5));
    REQUIRE_THAT(y, WithinAbs(HEIGHT / 2.0, 0.5));
  }

  SECTION("A point behind the camera has no pixel") {
    double x = -1.0;
    double y = -1.0;
    REQUIRE_FALSE(camera->pixelOfPoint(OkPoint(0.0f, 0.0f, 10.0f), WIDTH,
                                       HEIGHT, &x, &y));
  }

  SECTION("Higher in the world is higher up the window") {
    double middleX = 0.0;
    double middleY = 0.0;
    double aboveX  = 0.0;
    double aboveY  = 0.0;
    REQUIRE(camera->pixelOfPoint(OkPoint(0.0f, 0.0f, -10.0f), WIDTH, HEIGHT,
                                 &middleX, &middleY));
    REQUIRE(camera->pixelOfPoint(OkPoint(0.0f, 3.0f, -10.0f), WIDTH, HEIGHT,
                                 &aboveX, &aboveY));
    REQUIRE(aboveY < middleY);
  }

  delete camera;
}

TEST_CASE("OkCamera projection and unprojection agree", "[camera]") {
  // The pair is only trustworthy if it closes: project a point to a
  // pixel, shoot a ray back through that pixel, and the point must lie on
  // the ray. Tested from an angle rather than head on, so a mistake in
  // either matrix has somewhere to show.
  auto *camera = new OkCamera("test", WIDTH, HEIGHT);
  camera->setPerspective(60.0f, 0.1f, 1000.0f);
  camera->setPosition(12.0f, 40.0f, 25.0f);
  camera->setRotation(-0.6f, 0.35f, 0.0f);
  camera->updateTransform();

  const OkPoint targets[] = {
      OkPoint(0.0f, 0.0f, 0.0f), OkPoint(-8.0f, 3.0f, -14.0f),
      OkPoint(5.0f, -2.0f, 7.0f), OkPoint(20.0f, 11.0f, -30.0f)};

  for (size_t i = 0; i < sizeof(targets) / sizeof(targets[0]); i++) {
    double x = 0.0;
    double y = 0.0;
    if (!camera->pixelOfPoint(targets[i], WIDTH, HEIGHT, &x, &y)) {
      continue;  // behind the camera: nothing to round-trip
    }

    OkRay   ray      = camera->rayThroughPixel(x, y, WIDTH, HEIGHT);
    OkPoint toTarget = targets[i] - ray.origin;
    float   along    = toTarget.dot(ray.direction);
    OkPoint onRay    = ray.pointAt(along);

    REQUIRE_THAT(onRay.distance(targets[i]), WithinAbs(0.0f, 0.01f));
  }

  delete camera;
}
