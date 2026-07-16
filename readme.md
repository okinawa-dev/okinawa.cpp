<p align="center">
  <img width="200" alt="okinawa logo" src="/assets/project/okinawa_logo.png">
</p>

# okinawa

**Documentation: [okinawa-dev.github.io/okinawa](https://okinawa-dev.github.io/okinawa/)**

This is a work in progress C++ 3D game engine, inspired by [okinawa.js](https://github.com/okinawa-dev/okinawa.js), a previous version coded in JavaScript. The goal is to create a game engine that is easy to use, flexible, and powerful. The engine is designed to be modular, so you can easily add or remove features as needed.

The engine is built as a static library (`libokinawa.a`) and consumed from source by other projects. I'm currently testing it on a MacOS machine. Feedback is welcome.

With just this repository, without a game, you can build the library and run the test suite (with a coverage report). The engine is not yet ready for production use, but it is a good starting point for learning and experimenting with C++ and game development.

## Compilation

The project is built with [xmake](https://xmake.io), which also manages
the third-party dependencies through its package manager (xrepo). There
is no separate dependency-installation step: the dependencies (glm,
glfw, stb, opengl, catch2) are fetched and built automatically on the
first build.

### Tools needed

- [xmake](https://xmake.io/): A cross-platform build utility with a built-in package manager.
- [clang](https://clang.llvm.org/): A compiler for C and C++ languages.

For development:

- I am using `clang-format` to format the code, current configuration is in the `.clang-format` file.
- I am using `clang-tidy` to check the code for errors and warnings, current configuration is in the `.clang-tidy` file.
- `llvm` tools (`llvm-profdata`, `llvm-cov`) are needed for the coverage report.

### Tools installation

#### MacOS

```bash
# Install Xcode Command Line Tools (includes clang)
xcode-select --install

# Install xmake using Homebrew (package manager for macOS)
brew install xmake

# Install LLVM tools for code formatting, analysis and coverage
brew install llvm

# Add LLVM tools to your PATH (add this to your ~/.zshrc or ~/.bashrc)
export PATH="/opt/homebrew/opt/llvm/bin/:$PATH"
```

At the moment I'm only testing the code on MacOS.

### Build

```bash
# Build the library (debug by default). On the first run xmake will
# download and build the dependencies automatically.
xmake

# Build a specific target
xmake build okinawa       # just the library
xmake build okinawa_test  # just the tests

# Release build
xmake f -m release && xmake
```

### In-engine MCP server (build option)

The engine ships an optional in-process **MCP server** (`src/okinawa/mcp/`):
when enabled, a running okinawa app exposes tools over local HTTP
(`127.0.0.1:8765/mcp`) so an external agent (e.g. Claude Code) can view the
rendered frame, inject input and read telemetry. The app opts in at runtime by
calling `OkCore::enableMcpServer()`.

Whether the server is *compiled in* is a build-time choice with a
**mode-dependent default — included in debug, excluded in release**. The
decision is made by the compiler from `NDEBUG` (in `mcp-config.hpp`), so it
works the same when okinawa is built standalone or consumed through another
project's `includes()`. Override the default with at most one option:

```bash
xmake f --mcp=y      # force the server IN  (defines OKINAWA_MCP_FORCE=1)
xmake f --no-mcp=y   # force the server OUT (defines OKINAWA_MCP_FORCE=0)
```

When excluded, `mcp-server.cpp` is an empty translation unit, so no MCP/HTTP
code (nor its header-only deps `cpp-httplib` / `nlohmann_json`) ends up in the
binary, and `enableMcpServer()` becomes a no-op that logs a warning.

> Why not a plain mode-dependent `set_default`? xmake cannot read the build
> mode at a phase that also propagates through `includes()` to consumers
> (`is_mode()`/`get_config("mode")` are nil at script-load scope; a per-target
> `on_config` does not run for an included sub-target; and an option's dynamic
> `on_check` defines do not propagate). Letting the compiler decide via
> `NDEBUG`, with the override carried by *static* option defines, is the robust
> path. See the okinawa MCP ADR for the full rationale.

## Consuming the engine

The engine is consumed from source by other xmake projects (for example
`wadviewer` and `heist`) as a local sub-dependency. This lets you edit
the engine and the consuming app together and just rebuild the app,
without producing or reinstalling any binary package. The exact wiring
lives in each consumer's `xmake.lua`.

Public headers are included with an `okinawa/` prefix from consumers,
e.g.:

```cpp
#include "okinawa/core/core.hpp"
#include "okinawa/core/camera.hpp"
```

## Tests

```bash
# Run the test suite (rundir is set to the project root)
xmake run okinawa_test

# Or through xmake's test runner
xmake test
```

To generate a code coverage report:

```bash
# Build and run tests with llvm source-based coverage
xmake coverage

# Open the coverage report
open coverage/index.html
```

The tests are located in the `tests` folder and use the [Catch2](https://github.com/catchorg/Catch2) testing framework. The coverage report shows detailed line-by-line code coverage information.

### Code checking

```bash
# Run clang-tidy using the generated compile_commands.json
clang-tidy -p . src/**/*.cpp
```

## Libraries used

- [glfw](https://github.com/glfw/glfw): A multi-platform library for OpenGL, OpenGL ES, Vulkan, window and input.
- [glm](https://github.com/g-truc/glm): OpenGL Mathematics (GLM), is a header only C++ mathematics library for graphics software.
- [stb-image](https://github.com/nothings/stb): Single-file public domain image loading library.
- [Catch2](https://github.com/catchorg/Catch2): A modern, C++-native, header-only, test framework for unit-tests, TDD and BDD.
- [cpp-httplib](https://github.com/yhirose/cpp-httplib): Header-only HTTP server, used by the optional in-engine MCP server (only when compiled in).
- [nlohmann/json](https://github.com/nlohmann/json): Header-only JSON library, used by the MCP server (only when compiled in).
