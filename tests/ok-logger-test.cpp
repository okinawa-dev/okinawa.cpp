// NOLINTBEGIN(readability-magic-numbers)

#include "../src/utils/logger.hpp"
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <sstream>

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
    // Cast to LogLevel to create an invalid value
    OkLogger::log(static_cast<LogLevel>(999), "Test message");
    std::string output = capture.getOutput();
    REQUIRE(output.find("[UNKNOWN]") != std::string::npos);
    REQUIRE(output.find("Test message") != std::string::npos);
  }
}

// NOLINTEND(readability-magic-numbers)
