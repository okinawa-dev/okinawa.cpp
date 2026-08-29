#include "okinawa/input/input.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>

// What can be checked without a window: how long a block is allowed to
// last, and how a chord is matched against a key state. The rest -- the
// deadline, reading the device -- belongs to a running app.
//
// Neither is decoration. A block is how an agent takes the keyboard away
// from the person at the window, so its length must not become a day or
// a millisecond; and the chord that gives the keyboard back has to be
// told apart from the keys it is made of, or it hands control back by
// quitting the application.

namespace {
  using Keys = std::array<bool, OK_KEY_COUNT>;

  Keys none() {
    Keys k;
    k.fill(false);
    return k;
  }
}  // namespace

TEST_CASE("An input block lasts what it is allowed to", "[input]") {
  SECTION("An ordinary request is kept as it is") {
    REQUIRE(OkInput::clampBlockSeconds(30.0) == 30.0);
    REQUIRE(OkInput::clampBlockSeconds(OkInput::BLOCK_DEFAULT_SECONDS) ==
            OkInput::BLOCK_DEFAULT_SECONDS);
  }

  SECTION("Too short comes up to the minimum, too long down to the maximum") {
    REQUIRE(OkInput::clampBlockSeconds(0.001) == OkInput::BLOCK_MIN_SECONDS);
    REQUIRE(OkInput::clampBlockSeconds(100000.0) == OkInput::BLOCK_MAX_SECONDS);
  }

  SECTION("Zero or less asks for no deadline at all") {
    // Which is what the launch flag wants, and nothing else should.
    REQUIRE(OkInput::clampBlockSeconds(0.0) == 0.0);
    REQUIRE(OkInput::clampBlockSeconds(-5.0) == 0.0);
  }

  SECTION("The bounds are the way round they claim to be") {
    REQUIRE(OkInput::BLOCK_MIN_SECONDS < OkInput::BLOCK_DEFAULT_SECONDS);
    REQUIRE(OkInput::BLOCK_DEFAULT_SECONDS < OkInput::BLOCK_MAX_SECONDS);
    // Blocked-for reports this when a block has no deadline, so it must
    // never be mistaken for "not blocked" (0) or for seconds left.
    REQUIRE(OkInput::BLOCK_FOREVER < 0.0);
  }
}

TEST_CASE("Modifiers are read off a key state", "[input]") {
  Keys k = none();
  REQUIRE(OkInput::modifiersOf(k) == 0);

  SECTION("Either hand's key sets the same bit") {
    k[OK_KEY_LEFT_SHIFT] = true;
    REQUIRE(OkInput::modifiersOf(k) == OkInput::OK_MOD_SHIFT);
    k[OK_KEY_LEFT_SHIFT]  = false;
    k[OK_KEY_RIGHT_SHIFT] = true;
    REQUIRE(OkInput::modifiersOf(k) == OkInput::OK_MOD_SHIFT);
  }

  SECTION("They add up") {
    k[OK_KEY_LEFT_CONTROL] = true;
    k[OK_KEY_RIGHT_SHIFT]  = true;
    REQUIRE(OkInput::modifiersOf(k) ==
            (OkInput::OK_MOD_CTRL | OkInput::OK_MOD_SHIFT));
  }

  SECTION("An ordinary key is not a modifier") {
    k[OK_KEY_W] = true;
    REQUIRE(OkInput::modifiersOf(k) == 0);
  }
}

TEST_CASE("A chord is the key's edge with the modifiers already held",
          "[input]") {
  const int mods            = OkInput::OK_MOD_CTRL | OkInput::OK_MOD_SHIFT;
  Keys      prev            = none();
  Keys      now             = none();
  now[OK_KEY_LEFT_CONTROL]  = true;
  now[OK_KEY_LEFT_SHIFT]    = true;
  prev[OK_KEY_LEFT_CONTROL] = true;
  prev[OK_KEY_LEFT_SHIFT]   = true;

  SECTION("Fires on the frame the key goes down") {
    now[OK_KEY_ESCAPE] = true;
    REQUIRE(OkInput::chordJustPressed(now, prev, mods, OK_KEY_ESCAPE));
  }

  SECTION("And only on that frame: holding it is not pressing it again") {
    now[OK_KEY_ESCAPE]  = true;
    prev[OK_KEY_ESCAPE] = true;
    REQUIRE_FALSE(OkInput::chordJustPressed(now, prev, mods, OK_KEY_ESCAPE));
  }

  SECTION("A missing modifier is not the chord") {
    now[OK_KEY_LEFT_SHIFT] = false;
    now[OK_KEY_ESCAPE]     = true;
    REQUIRE_FALSE(OkInput::chordJustPressed(now, prev, mods, OK_KEY_ESCAPE));
  }

  SECTION("Nor is an extra one") {
    // EXACTLY the modifiers asked for: ctrl+shift+escape must not fire
    // on ctrl+alt+shift+escape, or a chord swallows every gesture that
    // happens to contain it.
    now[OK_KEY_LEFT_ALT] = true;
    now[OK_KEY_ESCAPE]   = true;
    REQUIRE_FALSE(OkInput::chordJustPressed(now, prev, mods, OK_KEY_ESCAPE));
  }

  SECTION("The plain key on its own is not the chord") {
    Keys bare           = none();
    Keys bareprev       = none();
    bare[OK_KEY_ESCAPE] = true;
    REQUIRE_FALSE(
        OkInput::chordJustPressed(bare, bareprev, mods, OK_KEY_ESCAPE));
    // ...and it IS the chord when none are asked for.
    REQUIRE(OkInput::chordJustPressed(bare, bareprev, 0, OK_KEY_ESCAPE));
  }
}
