#include "../src/utils/files.hpp"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <string>

TEST_CASE("OkFiles basic operations", "[files]") {
  SECTION("Read existing file") {
    std::string content = OkFiles::readFile("tests/test-file.txt");
    REQUIRE_FALSE(content.empty());
    REQUIRE(content.find("This is a test file") != std::string::npos);
    REQUIRE(content.find("with multiple lines") != std::string::npos);
  }

  SECTION("Read non-existent file") {
    std::string content = OkFiles::readFile("non-existent-file.txt");
    REQUIRE(content.empty());
  }

  SECTION("Read file with empty path") {
    std::string content = OkFiles::readFile("");
    REQUIRE(content.empty());
  }

  SECTION("File content integrity") {
    std::string content = OkFiles::readFile("tests/test-file.txt");

    // Check line count
    size_t newlines = std::count(content.begin(), content.end(), '\n');
    REQUIRE(newlines == 3);  // 4 lines, 3 newlines

    // Check content doesn't have extra whitespace at start/end
    REQUIRE(content.front() == 'T');  // First character
    REQUIRE(content.back() == '\n');  // Last character should be newline
  }
}
