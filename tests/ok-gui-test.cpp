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

// Anchor origins: the point grid coordinates are relative to, for a given
// logical window size (pure math, no window needed).

TEST_CASE("OkGui anchor origins", "[gui]") {
  REQUIRE_THAT(OkGui::anchorOriginXFor(OK_GUI_ANCHOR_CENTER, 800.0f),
               WithinAbs(0.0f, 0.0001f));
  REQUIRE_THAT(OkGui::anchorOriginXFor(OK_GUI_ANCHOR_LEFT, 800.0f),
               WithinAbs(-400.0f, 0.0001f));
  REQUIRE_THAT(OkGui::anchorOriginXFor(OK_GUI_ANCHOR_TOP_RIGHT, 800.0f),
               WithinAbs(400.0f, 0.0001f));
  REQUIRE_THAT(OkGui::anchorOriginYFor(OK_GUI_ANCHOR_TOP, 600.0f),
               WithinAbs(300.0f, 0.0001f));
  REQUIRE_THAT(OkGui::anchorOriginYFor(OK_GUI_ANCHOR_BOTTOM_LEFT, 600.0f),
               WithinAbs(-300.0f, 0.0001f));
  REQUIRE_THAT(OkGui::anchorOriginYFor(OK_GUI_ANCHOR_RIGHT, 600.0f),
               WithinAbs(0.0f, 0.0001f));
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

// Font glyph data and console command plumbing (no GL needed).

#include "okinawa/gui/console.hpp"
#include "okinawa/gui/font.hpp"

TEST_CASE("OkFont glyphs", "[gui]") {
  SECTION("Lowercase maps to uppercase") {
    REQUIRE(OkFont::glyphRows('a') == OkFont::glyphRows('A'));
    REQUIRE(OkFont::glyphRows('z') == OkFont::glyphRows('Z'));
  }
  SECTION("Distinct printable glyphs") {
    REQUIRE(OkFont::glyphRows('A') != OkFont::glyphRows('B'));
    REQUIRE(OkFont::glyphRows('0') != OkFont::glyphRows('O'));
    REQUIRE(OkFont::glyphRows('-') != OkFont::glyphRows('_'));
  }
  SECTION("Space and unknown render blank") {
    const unsigned char *sp = OkFont::glyphRows(' ');
    for (int i = 0; i < 7; i++) {
      REQUIRE(sp[i] == 0);
    }
  }
  SECTION("Atlas UVs stay in range and differ per glyph") {
    float au0, av0, au1, av1, bu0, bv0, bu1, bv1;
    OkFont::glyphUV('A', au0, av0, au1, av1);
    OkFont::glyphUV('B', bu0, bv0, bu1, bv1);
    REQUIRE(au0 >= 0.0f);
    REQUIRE(av0 >= 0.0f);
    REQUIRE(au1 <= 1.0f);
    REQUIRE(av1 <= 1.0f);
    REQUIRE(au0 < au1);
    REQUIRE(av0 < av1);
    REQUIRE((au0 != bu0 || av0 != bv0));
  }
}

TEST_CASE("OkConsole commands", "[gui]") {
  static int         calls = 0;
  static std::string lastArg;
  calls = 0;
  lastArg.clear();

  OkConsole::registerCommand("testcmd", "test helper",
                             [](const std::vector<std::string> &args) {
                               calls++;
                               lastArg = args.empty() ? "" : args[0];
                             });

  SECTION("Execute with arguments") {
    OkConsole::execute("testcmd hello world");
    REQUIRE(calls == 1);
    REQUIRE(lastArg == "hello");
  }
  SECTION("Unknown command does not crash or call") {
    OkConsole::execute("no-such-command");
    REQUIRE(calls == 0);
  }
  SECTION("Blank line is a no-op") {
    OkConsole::execute("   ");
    REQUIRE(calls == 0);
  }
  SECTION("Re-registering replaces the callback") {
    OkConsole::registerCommand("testcmd", "replaced",
                               [](const std::vector<std::string> &args) {
                                 (void)args;
                                 calls += 10;
                               });
    OkConsole::execute("testcmd");
    REQUIRE(calls == 10);
  }
}

TEST_CASE("OkConfig prefix lookup", "[gui]") {
  OkConfig::reset();
  SECTION("Prefix lists every gui key") {
    std::vector<std::string> keys = OkConfig::getKeysWithPrefix("gui.");
    REQUIRE(keys.size() >= 4);
  }
  SECTION("Narrow prefix resolves to one key") {
    std::vector<std::string> keys = OkConfig::getKeysWithPrefix("gui.sc");
    REQUIRE(keys.size() == 1);
    REQUIRE(keys[0] == "gui.scale");
  }
  SECTION("Exact key and formatted value") {
    REQUIRE(OkConfig::hasKey("gui.grid.size"));
    REQUIRE(OkConfig::getValueAsString("gui.grid.size") == "20");
    REQUIRE(OkConfig::getValueAsString("gui.debug.grid") == "false");
    REQUIRE(OkConfig::getValueAsString("no.such.key") == "<unset>");
  }
  OkConfig::reset();
}

// Day-cycle atmosphere curve (pure evaluation, no GL).

#include "okinawa/lighting/lighting.hpp"

TEST_CASE("OkLighting atmosphere curve", "[lighting]") {
  float tint[3], fog[3], density, sun[3], dir[3];

  // These assert the MECHANISM, not a particular look: the values in
  // the engine's default curve are a starting point projects replace,
  // so a test that pinned them down would only pin down one project's
  // art direction.

  SECTION("The sun rides above the horizon by day and below it at night") {
    OkLighting::evaluate(12.0f, tint, fog, density, sun, dir);
    REQUIRE(dir[1] < -0.5f);  // high above: the light points down
    OkLighting::evaluate(2.0f, tint, fog, density, sun, dir);
    REQUIRE(dir[1] > 0.0f);  // parked below the horizon
  }
  SECTION("Night is darker than midday") {
    float dayTint[3], dayFog[3], dayDensity, daySun[3], dayDir[3];
    OkLighting::evaluate(12.0f, dayTint, dayFog, dayDensity, daySun, dayDir);
    OkLighting::evaluate(2.0f, tint, fog, density, sun, dir);
    float dayLum   = dayTint[0] + dayTint[1] + dayTint[2];
    float nightLum = tint[0] + tint[1] + tint[2];
    REQUIRE(nightLum < dayLum);
    // and no directional light is left at night
    REQUIRE(sun[0] + sun[1] + sun[2] < daySun[0] + daySun[1] + daySun[2]);
  }
  SECTION("Hours wrap") {
    float t2[3], f2[3], d2, s2[3], dd2[3];
    OkLighting::evaluate(26.0f, tint, fog, density, sun, dir);
    OkLighting::evaluate(2.0f, t2, f2, d2, s2, dd2);
    REQUIRE_THAT(tint[0], WithinAbs(t2[0], 0.0001f));
    REQUIRE_THAT(density, WithinAbs(d2, 0.0001f));
  }
  SECTION("Continuity across midnight") {
    float a[3], b[3], fa[3], fb[3], da, db, sa[3], sb[3], za[3], zb[3];
    OkLighting::evaluate(23.99f, a, fa, da, sa, za);
    OkLighting::evaluate(0.01f, b, fb, db, sb, zb);
    REQUIRE_THAT(a[0], WithinAbs(b[0], 0.01f));
    REQUIRE_THAT(da, WithinAbs(db, 0.0005f));
  }
  SECTION("A project can replace the curve") {
    // Two keys are enough to describe a world: the engine must take
    // them and interpolate between them, wrapping around midnight.
    const OkAtmosphereKey CUSTOM[] = {
        {0.0f,
         {0.10f, 0.10f, 0.10f},
         {0.0f, 0.0f, 0.0f},
         0.0100f,
         {0.0f, 0.0f, 0.0f},
         {0.0f, 0.0f, 0.0f},
         0.10f},
        {12.0f,
         {0.90f, 0.90f, 0.90f},
         {1.0f, 1.0f, 1.0f},
         0.0000f,
         {1.0f, 1.0f, 1.0f},
         {1.0f, 1.0f, 1.0f},
         0.90f},
    };
    OkLighting::setAtmosphereCurve(CUSTOM, 2);
    REQUIRE(OkLighting::getAtmosphereKeyCount() == 2);

    OkLighting::evaluate(0.0f, tint, fog, density, sun, dir);
    REQUIRE_THAT(tint[0], WithinAbs(0.10f, 0.001f));
    OkLighting::evaluate(12.0f, tint, fog, density, sun, dir);
    REQUIRE_THAT(tint[0], WithinAbs(0.90f, 0.001f));
    // halfway between the keys, halfway between the values
    OkLighting::evaluate(6.0f, tint, fog, density, sun, dir);
    REQUIRE_THAT(tint[0], WithinAbs(0.50f, 0.01f));
    // a curve with too few keys is rejected rather than half-applied
    OkLighting::setAtmosphereCurve(CUSTOM, 1);
    REQUIRE(OkLighting::getAtmosphereKeyCount() == 2);
  }
}
