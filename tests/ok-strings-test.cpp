#include "../src/utils/strings.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>

TEST_CASE("OkStrings trim function", "[strings]") {
  SECTION("Empty string") {
    REQUIRE(OkStrings::trim("") == "");
  }

  SECTION("String with only whitespace") {
    REQUIRE(OkStrings::trim("   \t\n\r\f\v   ") == "");
  }

  SECTION("String with leading whitespace") {
    REQUIRE(OkStrings::trim("   hello") == "hello");
  }

  SECTION("String with trailing whitespace") {
    REQUIRE(OkStrings::trim("hello   ") == "hello");
  }

  SECTION("String with both leading and trailing whitespace") {
    REQUIRE(OkStrings::trim("   hello   ") == "hello");
  }

  SECTION("String with internal whitespace") {
    REQUIRE(OkStrings::trim("   hello world   ") == "hello world");
  }

  SECTION("Edge cases") {
    // Test a string that's all whitespace
    std::string allWhitespace = "\t \n\r\f\v";
    REQUIRE(OkStrings::trim(allWhitespace) == "");

    // Test a single non-whitespace character surrounded by whitespace
    std::string singleChar = " \t x \n\r ";
    REQUIRE(OkStrings::trim(singleChar) == "x");
  }
}

TEST_CASE("OkStrings trimRight function", "[strings]") {
  SECTION("Empty string") {
    REQUIRE(OkStrings::trimRight("") == "");
  }

  SECTION("String with only whitespace") {
    REQUIRE(OkStrings::trimRight("   \t\n\r\f\v   ") == "");
  }

  SECTION("String with leading whitespace") {
    REQUIRE(OkStrings::trimRight("   hello") == "   hello");
  }

  SECTION("String with trailing whitespace") {
    REQUIRE(OkStrings::trimRight("hello   ") == "hello");
  }

  SECTION("String with both leading and trailing whitespace") {
    REQUIRE(OkStrings::trimRight("   hello   ") == "   hello");
  }

  SECTION("String with internal whitespace") {
    REQUIRE(OkStrings::trimRight("   hello world   ") == "   hello world");
  }
}

TEST_CASE("OkStrings trimFixedString function", "[strings]") {
  SECTION("Empty string") {
    REQUIRE(OkStrings::trimFixedString("", 5) == "");
    REQUIRE(OkStrings::trimFixedString("", 0) == "");
  }

  SECTION("String with only whitespace") {
    REQUIRE(OkStrings::trimFixedString("   \t\n\r\f\v   ", 5) == "");
    REQUIRE(OkStrings::trimFixedString("   ", 1) == "");
  }

  SECTION("String shorter than max length") {
    REQUIRE(OkStrings::trimFixedString("hello", 10) == "hello");
    REQUIRE(OkStrings::trimFixedString("hello   ", 10) == "hello");
    REQUIRE(OkStrings::trimFixedString("a", 2) == "a");
  }

  SECTION("String equal to max length") {
    REQUIRE(OkStrings::trimFixedString("hello", 5) == "hello");
    REQUIRE(OkStrings::trimFixedString("hello   ", 8) == "hello");
    REQUIRE(OkStrings::trimFixedString("ab", 2) == "ab");
  }

  SECTION("String longer than max length") {
    REQUIRE(OkStrings::trimFixedString("hello world", 5) == "hello");
    REQUIRE(OkStrings::trimFixedString("hello world   ", 5) == "hello");
    REQUIRE(OkStrings::trimFixedString("abc", 2) == "ab");
  }

  SECTION("Edge cases") {
    REQUIRE(OkStrings::trimFixedString("hello", 0) == "");
    REQUIRE(OkStrings::trimFixedString("   hello   ", 8) == "   hello");
    REQUIRE(OkStrings::trimFixedString("hello   world", 8) == "hello");
    REQUIRE(OkStrings::trimFixedString("hello   ", 7) == "hello");
    REQUIRE(OkStrings::trimFixedString("hello world", 3) == "hel");
  }
}

TEST_CASE("OkStrings toUpper function", "[strings]") {
  SECTION("Empty string") {
    REQUIRE(OkStrings::toUpper("") == "");
  }

  SECTION("Lowercase string") {
    REQUIRE(OkStrings::toUpper("hello") == "HELLO");
  }

  SECTION("Mixed case string") {
    REQUIRE(OkStrings::toUpper("Hello World") == "HELLO WORLD");
  }

  SECTION("String with numbers and symbols") {
    REQUIRE(OkStrings::toUpper("Hello123!@#") == "HELLO123!@#");
  }
}

TEST_CASE("OkStrings toLower function", "[strings]") {
  SECTION("Empty string") {
    REQUIRE(OkStrings::toLower("") == "");
  }

  SECTION("Uppercase string") {
    REQUIRE(OkStrings::toLower("HELLO") == "hello");
  }

  SECTION("Mixed case string") {
    REQUIRE(OkStrings::toLower("Hello World") == "hello world");
  }

  SECTION("String with numbers and symbols") {
    REQUIRE(OkStrings::toLower("HELLO123!@#") == "hello123!@#");
  }
}
