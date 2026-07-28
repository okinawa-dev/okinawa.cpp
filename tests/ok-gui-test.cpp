// NOLINTBEGIN(readability-magic-numbers)

#include "okinawa/config/config.hpp"
#include "okinawa/gui/gui.hpp"
#include "okinawa/gui/gui_layer.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

// Grid <-> screen conversions are the GUI's coordinate contract: one grid
// unit = gui.grid.size * gui.scale logical pixels, origin at the screen
// centre. (The render pass needs a GL context, so only the math is tested.)

TEST_CASE("OkGui grid conversions", "[gui]") {
  OkConfig::reset();

  SECTION("Default cell size, explicit scale 1") {
    OkConfig::setFloat("gui.scale", 1.0f);
    REQUIRE_THAT(OkGui::gridToScreenX(0.0f), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(OkGui::gridToScreenX(1.0f), WithinAbs(20.0f, 0.0001f));
    REQUIRE_THAT(OkGui::gridToScreenY(-3.0f), WithinAbs(-60.0f, 0.0001f));
    REQUIRE_THAT(OkGui::gridToScreenX(0.5f), WithinAbs(10.0f, 0.0001f));
  }

  SECTION("Round trip") {
    OkConfig::setFloat("gui.scale", 1.0f);
    float px = OkGui::gridToScreenX(7.25f);
    REQUIRE_THAT(OkGui::screenToGridX(px), WithinAbs(7.25f, 0.0001f));
    float py = OkGui::gridToScreenY(-2.75f);
    REQUIRE_THAT(OkGui::screenToGridY(py), WithinAbs(-2.75f, 0.0001f));
  }

  SECTION("Cell size follows gui.grid.size") {
    OkConfig::setFloat("gui.scale", 1.0f);
    OkConfig::setInt("gui.grid.size", 32);
    REQUIRE_THAT(OkGui::gridToScreenX(2.0f), WithinAbs(64.0f, 0.0001f));
    REQUIRE_THAT(OkGui::getCellSize(), WithinAbs(32.0f, 0.0001f));
  }

  SECTION("Global scale rescales the whole grid") {
    OkConfig::setInt("gui.grid.size", 20);
    OkConfig::setFloat("gui.scale", 2.0f);
    REQUIRE_THAT(OkGui::gridToScreenX(1.0f), WithinAbs(40.0f, 0.0001f));
    REQUIRE_THAT(OkGui::screenToGridX(40.0f), WithinAbs(1.0f, 0.0001f));
  }

  OkConfig::reset();
}

// Layer management is pure bookkeeping (no GL): ordering, lookup, removal.

TEST_CASE("OkGui layers", "[gui]") {
  SECTION("Kept sorted by order, far to near") {
    OkGuiLayer *near = OkGui::addLayer("near", 10);
    OkGuiLayer *far  = OkGui::addLayer("far", -5);
    OkGuiLayer *mid  = OkGui::addLayer("mid", 3);
    REQUIRE(near != nullptr);
    REQUIRE(far != nullptr);
    REQUIRE(mid != nullptr);
    REQUIRE(OkGui::getLayerCount() == 3);
    REQUIRE(OkGui::getLayer("far") == far);
    REQUIRE(OkGui::getLayer("mid") == mid);
    REQUIRE(OkGui::getLayer("missing") == nullptr);
  }

  SECTION("Removal") {
    REQUIRE(OkGui::removeLayer("mid") == true);
    REQUIRE(OkGui::getLayer("mid") == nullptr);
    REQUIRE(OkGui::removeLayer("mid") == false);
    REQUIRE(OkGui::getLayerCount() == 2);
    REQUIRE(OkGui::removeLayer("near") == true);
    REQUIRE(OkGui::removeLayer("far") == true);
    REQUIRE(OkGui::getLayerCount() == 0);
  }
}

// NOLINTEND(readability-magic-numbers)
