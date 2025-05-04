// NOLINTBEGIN(readability-magic-numbers)

#include "../src/config/config.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("OkConfig initialization and default values", "[config]") {
  SECTION("Default graphics settings") {
    REQUIRE_FALSE(OkConfig::getBool("graphics.wireframe"));
    REQUIRE(OkConfig::getBool("graphics.textures"));
  }

  SECTION("Default window settings") {
    REQUIRE(OkConfig::getInt("window.width") == 800);
    REQUIRE(OkConfig::getInt("window.height") == 600);
  }

  SECTION("Default performance settings") {
    REQUIRE(OkConfig::getInt("fps") == 60);
    float expectedTimePerFrame = 1000.0f / 60.0f;
    REQUIRE(OkConfig::getFloat("graphics.time-per-frame") ==
            expectedTimePerFrame);
  }

  SECTION("Default OpenGL settings") {
    REQUIRE(OkConfig::getInt("opengl.infolog.size") == 512);
  }
}

TEST_CASE("OkConfig value modifications", "[config]") {
  SECTION("Modify integer values") {
    OkConfig::setInt("window.width", 1024);
    REQUIRE(OkConfig::getInt("window.width") == 1024);

    OkConfig::setInt("window.height", 768);
    REQUIRE(OkConfig::getInt("window.height") == 768);
  }

  SECTION("Modify float values") {
    OkConfig::setFloat("time.frame", 16.666f);
    REQUIRE(OkConfig::getFloat("time.frame") == 16.666f);
  }

  SECTION("Modify boolean values") {
    OkConfig::setBool("graphics.wireframe", true);
    REQUIRE(OkConfig::getBool("graphics.wireframe"));

    OkConfig::setBool("graphics.textures", false);
    REQUIRE_FALSE(OkConfig::getBool("graphics.textures"));
  }
}

TEST_CASE("OkConfig error handling", "[config]") {
  SECTION("Non-existent keys") {
    REQUIRE(OkConfig::getInt("nonexistent.int") == 0);
    REQUIRE(OkConfig::getFloat("nonexistent.float") == 0.0f);
    REQUIRE_FALSE(OkConfig::getBool("nonexistent.bool"));
  }

  SECTION("Wrong type access") {
    // Set an integer value
    OkConfig::setInt("test.value", 42);

    // Try to access it as different types
    REQUIRE(OkConfig::getInt("test.value") == 42);      // Correct type
    REQUIRE(OkConfig::getFloat("test.value") == 0.0f);  // Wrong type
    REQUIRE_FALSE(OkConfig::getBool("test.value"));     // Wrong type
  }
}

// NOLINTEND(readability-magic-numbers)
