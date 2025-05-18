#include "core/core.hpp"
#include "utils/logger.hpp"

int main() {
  if (!OkCore::initialize()) {
    OkLogger::error("Failed to initialize engine");
    return 1;
  }

  // Empty callbacks for now, since we're just validating the build
  OkCore::loop(nullptr, nullptr);

  return 0;
}
