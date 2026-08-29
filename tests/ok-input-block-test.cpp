#include "okinawa/input/input.hpp"
#include <catch2/catch_test_macros.hpp>

// The part of an input block that can be checked without a window: how
// long it is allowed to last. The rest -- the deadline, the release
// chord -- is read from GLFW and belongs to a running app.
//
// The clamp is not decoration. A block is how an agent takes the
// keyboard away from the person at the window, so a request of a day, or
// of a millisecond, must not become one.

TEST_CASE("An input block lasts what it is allowed to", "[input]") {
  SECTION("An ordinary request is kept as it is") {
    REQUIRE(OkInput::clampBlockSeconds(30.0) == 30.0);
    REQUIRE(OkInput::clampBlockSeconds(OkInput::BLOCK_DEFAULT_SECONDS) ==
            OkInput::BLOCK_DEFAULT_SECONDS);
  }

  SECTION("Too short comes up to the minimum") {
    REQUIRE(OkInput::clampBlockSeconds(0.001) == OkInput::BLOCK_MIN_SECONDS);
  }

  SECTION("Too long comes down to the maximum") {
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
