// NOLINTBEGIN(readability-magic-numbers)

#include "../src/utils/logger.hpp"
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <sstream>
#include <string>

// Helper class to capture cerr output
class CerrCapture {
public:
  CerrCapture() : oldCerr(std::cerr.rdbuf(capture.rdbuf())) {}
  ~CerrCapture() { std::cerr.rdbuf(oldCerr); }
  std::string getOutput() { return capture.str(); }
  void        clear() { capture.str(""); }

private:
  std::stringstream capture;
  std::streambuf   *oldCerr;
};

TEST_CASE("OkLogger output formats", "[logger]") {
  CerrCapture capture;

  SECTION("Info message format") {
    OkLogger::info("Test info message");
    std::string output = capture.getOutput();
    REQUIRE(output.find("[INFO]") != std::string::npos);
    REQUIRE(output.find("Test info message") != std::string::npos);
    REQUIRE(output.find("\x1b[32m") != std::string::npos);  // Green color
  }

  SECTION("Warning message format") {
    capture.clear();
    OkLogger::warning("Test warning message");
    std::string output = capture.getOutput();
    REQUIRE(output.find("[WARNING]") != std::string::npos);
    REQUIRE(output.find("Test warning message") != std::string::npos);
    REQUIRE(output.find("\x1b[33m") != std::string::npos);  // Yellow color
  }

  SECTION("Error message format") {
    capture.clear();
    OkLogger::error("Test error message");
    std::string output = capture.getOutput();
    REQUIRE(output.find("[ERROR]") != std::string::npos);
    REQUIRE(output.find("Test error message") != std::string::npos);
    REQUIRE(output.find("\x1b[31m") != std::string::npos);  // Red color
  }
}

TEST_CASE("OkLogger timestamp format", "[logger]") {
  CerrCapture capture;

  SECTION("Timestamp format") {
    OkLogger::info("Test message");
    std::string output = capture.getOutput();
    // Check for HH:MM:SS format using regex-like check
    bool hasTimeFormat =
        output.find_first_of("0123456789") != std::string::npos &&
        output.find(':') != std::string::npos;
    REQUIRE(hasTimeFormat);
  }
}

TEST_CASE("OkLogger color reset", "[logger]") {
  CerrCapture capture;

  SECTION("Color reset after message") {
    OkLogger::info("Test message");
    std::string output = capture.getOutput();
    REQUIRE(output.find("\x1b[0m") != std::string::npos);  // Reset color
  }
}

TEST_CASE("OkLogger unknown level", "[logger]") {
  CerrCapture capture;

  SECTION("Unknown log level") {
    // Create an invalid enum value using bit manipulation to avoid compiler
    // warning. This is intentional for testing error handling.
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    OkLogger::log(static_cast<LogLevel>(999), "", "Test message");
    std::string output = capture.getOutput();
    REQUIRE(output.find("[UNKNOWN]") != std::string::npos);
    REQUIRE(output.find("Test message") != std::string::npos);
  }
}

TEST_CASE("OkLogger typed logging format", "[logger]") {
  CerrCapture capture;

  SECTION("Typed info message format") {
    OkLogger::info("Core", "Test typed info message");
    std::string output = capture.getOutput();
    REQUIRE(output.find("[INFO]") != std::string::npos);
    REQUIRE(output.find("Core :: Test typed info message") !=
            std::string::npos);
    REQUIRE(output.find("\x1b[32m") != std::string::npos);  // Green color
  }

  SECTION("Typed warning message format") {
    capture.clear();
    OkLogger::warning("Assets", "Test typed warning message");
    std::string output = capture.getOutput();
    REQUIRE(output.find("[WARNING]") != std::string::npos);
    REQUIRE(output.find("Assets :: Test typed warning message") !=
            std::string::npos);
    REQUIRE(output.find("\x1b[33m") != std::string::npos);  // Yellow color
  }

  SECTION("Typed error message format") {
    capture.clear();
    OkLogger::error("Shader", "Test typed error message");
    std::string output = capture.getOutput();
    REQUIRE(output.find("[ERROR]") != std::string::npos);
    REQUIRE(output.find("Shader :: Test typed error message") !=
            std::string::npos);
    REQUIRE(output.find("\x1b[31m") != std::string::npos);  // Red color
  }
}

TEST_CASE("OkLogger type filtering", "[logger]") {
  CerrCapture capture;

  SECTION("Enable/disable specific log types") {
    // Initially all types should be enabled by default
    REQUIRE(OkLogger::isLogTypeEnabled("Core"));
    REQUIRE(OkLogger::isLogTypeEnabled("Assets"));

    // Test disabling a specific type
    OkLogger::disableLogType("Core");
    REQUIRE_FALSE(OkLogger::isLogTypeEnabled("Core"));
    REQUIRE(OkLogger::isLogTypeEnabled("Assets"));  // Other types still enabled

    // Test Core messages are filtered out
    capture.clear();
    OkLogger::info("Core", "This should be filtered");
    OkLogger::info("Assets", "This should appear");
    std::string output = capture.getOutput();
    REQUIRE(output.find("Core :: This should be filtered") ==
            std::string::npos);
    REQUIRE(output.find("Assets :: This should appear") != std::string::npos);

    // Re-enable Core logging
    OkLogger::enableLogType("Core");
    REQUIRE(OkLogger::isLogTypeEnabled("Core"));
  }

  SECTION("Enable/disable all log types") {
    // Test disabling all types
    OkLogger::disableAllLogTypes();
    REQUIRE_FALSE(OkLogger::isLogTypeEnabled("Core"));
    REQUIRE_FALSE(OkLogger::isLogTypeEnabled("Assets"));
    REQUIRE_FALSE(OkLogger::isLogTypeEnabled("NewType"));

    // Test that messages are filtered when all types disabled
    capture.clear();
    OkLogger::info("Core", "Should be filtered");
    OkLogger::warning("Assets", "Should be filtered");
    std::string output = capture.getOutput();
    REQUIRE(output.empty());

    // Test enabling all types
    OkLogger::enableAllLogTypes();
    REQUIRE(OkLogger::isLogTypeEnabled("Core"));
    REQUIRE(OkLogger::isLogTypeEnabled("Assets"));
    REQUIRE(OkLogger::isLogTypeEnabled("NewType"));

    // Test that messages appear after enabling all
    capture.clear();
    OkLogger::info("Core", "Should appear");
    output = capture.getOutput();
    REQUIRE(output.find("Core :: Should appear") != std::string::npos);
  }

  SECTION("Unknown log types default behavior") {
    // Reset to default state
    OkLogger::enableAllLogTypes();

    // Unknown types should be enabled by default
    REQUIRE(OkLogger::isLogTypeEnabled("UnknownType"));

    capture.clear();
    OkLogger::info("UnknownType", "Should appear by default");
    std::string output = capture.getOutput();
    REQUIRE(output.find("UnknownType :: Should appear by default") !=
            std::string::npos);
  }
}

TEST_CASE("OkLogger mixed typed and untyped logging", "[logger]") {
  CerrCapture capture;

  SECTION("Both typed and untyped messages work") {
    // Enable all types first
    OkLogger::enableAllLogTypes();

    capture.clear();
    OkLogger::info("Typed message with type");               // Legacy format
    OkLogger::info("Core", "Typed message with Core type");  // New format

    std::string output = capture.getOutput();
    REQUIRE(output.find("[INFO]: Typed message with type") !=
            std::string::npos);
    REQUIRE(output.find("[INFO]: Core :: Typed message with Core type") !=
            std::string::npos);
  }

  SECTION("Filtering only affects typed messages") {
    OkLogger::disableLogType("Core");

    capture.clear();
    OkLogger::info(
        "This untyped message should appear");  // Legacy format - should always
                                                // appear
    OkLogger::info(
        "Core", "This typed message should be filtered");  // New format -
                                                           // should be filtered
    OkLogger::info(
        "Assets",
        "This typed message should appear");  // New format - should appear

    std::string output = capture.getOutput();
    REQUIRE(output.find("This untyped message should appear") !=
            std::string::npos);
    REQUIRE(output.find("Core :: This typed message should be filtered") ==
            std::string::npos);
    REQUIRE(output.find("Assets :: This typed message should appear") !=
            std::string::npos);

    // Cleanup
    OkLogger::enableLogType("Core");
  }
}

// NOLINTEND(readability-magic-numbers)
