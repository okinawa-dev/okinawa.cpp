// NOLINTBEGIN(readability-magic-numbers)

#define CATCH_CONFIG_MAIN
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "../src/math/point.hpp"

TEST_CASE("OkPoint basic operations", "[point]") {
  SECTION("Default constructor creates zero vector") {
    OkPoint p;
    REQUIRE(p.x() == 0.0f);
    REQUIRE(p.y() == 0.0f);
    REQUIRE(p.z() == 0.0f);
  }

  SECTION("Constructor with coordinates") {
    OkPoint p(1.0f, 2.0f, 3.0f);
    REQUIRE(p.x() == 1.0f);
    REQUIRE(p.y() == 2.0f);
    REQUIRE(p.z() == 3.0f);
  }

  SECTION("Copy constructor") {
    OkPoint p1(1.0f, 2.0f, 3.0f);
    OkPoint p2(p1);
    REQUIRE(p1 == p2);
  }
}

TEST_CASE("OkPoint arithmetic operations", "[point]") {
  OkPoint p1(1.0f, 2.0f, 3.0f);
  OkPoint p2(4.0f, 5.0f, 6.0f);

  SECTION("Addition") {
    OkPoint sum = p1 + p2;
    REQUIRE(sum.x() == 5.0f);
    REQUIRE(sum.y() == 7.0f);
    REQUIRE(sum.z() == 9.0f);
  }

  SECTION("Subtraction") {
    OkPoint diff = p2 - p1;
    REQUIRE(diff.x() == 3.0f);
    REQUIRE(diff.y() == 3.0f);
    REQUIRE(diff.z() == 3.0f);
  }

  SECTION("Scalar multiplication") {
    OkPoint scaled = p1 * 2.0f;
    REQUIRE(scaled.x() == 2.0f);
    REQUIRE(scaled.y() == 4.0f);
    REQUIRE(scaled.z() == 6.0f);
  }

  SECTION("Unary minus") {
    OkPoint p(1.0f, -2.0f, 3.0f);
    OkPoint negated = -p;
    REQUIRE(negated.x() == -1.0f);
    REQUIRE(negated.y() == 2.0f);
    REQUIRE(negated.z() == -3.0f);
  }
}

TEST_CASE("OkPoint vector operations", "[point]") {
  SECTION("Magnitude") {
    OkPoint p(3.0f, 0.0f, 4.0f);
    REQUIRE(p.magnitude() == 5.0f);
  }

  SECTION("Normalization") {
    OkPoint p(3.0f, 0.0f, 4.0f);
    OkPoint normalized = p.normalize();
    REQUIRE(normalized.magnitude() == Catch::Approx(1.0f));
  }

  SECTION("Normalization of near-zero vector") {
    // Create a very small vector that should be below epsilon
    OkPoint tiny(1e-7f, 1e-7f, 1e-7f);
    OkPoint normalized = tiny.normalize();
    // Should return zero vector when magnitude is below epsilon
    REQUIRE(normalized.x() == 0.0f);
    REQUIRE(normalized.y() == 0.0f);
    REQUIRE(normalized.z() == 0.0f);
  }

  SECTION("Distance") {
    OkPoint p1(1.0f, 1.0f, 1.0f);
    OkPoint p2(4.0f, 5.0f, 8.0f);
    float   distance = p1.distance(p2);
    REQUIRE(distance == Catch::Approx(8.602325f));
  }

  SECTION("Dot product") {
    OkPoint v1(1.0f, 2.0f, 3.0f);
    OkPoint v2(4.0f, 5.0f, 6.0f);
    // dot = 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
    float dotProduct = v1.dot(v2);
    REQUIRE(dotProduct == 32.0f);

    // Perpendicular vectors should have dot product = 0
    OkPoint right(1.0f, 0.0f, 0.0f);
    OkPoint up(0.0f, 1.0f, 0.0f);
    REQUIRE(right.dot(up) == 0.0f);
  }
}

TEST_CASE("OkPoint compound assignment operators", "[point]") {
  SECTION("Add assignment") {
    OkPoint p1(1.0f, 2.0f, 3.0f);
    OkPoint p2(4.0f, 5.0f, 6.0f);
    p1 += p2;
    REQUIRE(p1.x() == 5.0f);
    REQUIRE(p1.y() == 7.0f);
    REQUIRE(p1.z() == 9.0f);
  }

  SECTION("Subtract assignment") {
    OkPoint p1(4.0f, 5.0f, 6.0f);
    OkPoint p2(1.0f, 2.0f, 3.0f);
    p1 -= p2;
    REQUIRE(p1.x() == 3.0f);
    REQUIRE(p1.y() == 3.0f);
    REQUIRE(p1.z() == 3.0f);
  }

  SECTION("Multiply assignment") {
    OkPoint p(1.0f, 2.0f, 3.0f);
    p *= 2.0f;
    REQUIRE(p.x() == 2.0f);
    REQUIRE(p.y() == 4.0f);
    REQUIRE(p.z() == 6.0f);
  }
}

TEST_CASE("OkPoint string operations", "[point]") {
  SECTION("toString() representation") {
    OkPoint p(1.5f, -2.25f, 3.75f);
    REQUIRE(p.toString() == "(1.5, -2.25, 3.75)");

    OkPoint zero;
    REQUIRE(zero.toString() == "(0, 0, 0)");
  }
}

// NOLINTEND(readability-magic-numbers)
