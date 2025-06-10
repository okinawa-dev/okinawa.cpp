<p align="center">
  <img width="200" alt="okinawa.cpp logo" src="/assets/okinawa_logo.png">
</p>

# okinawa.cpp

This is a work in progress C++ 3D game engine, inspired by [okinawa.js](https://github.com/okinawa-dev/okinawa.js), a previous version coded in JavaScript. The goal is to create a game engine that is easy to use, flexible, and powerful. The engine is designed to be modular, so you can easily add or remove features as needed.

The engine is distributed as a Conan package for easy integration into C++ projects. I'm currently testing it on a MacOS machine. Feedback is welcome.

With just this repository, without a game, you can just run the test suite and see the coverage report. The engine is not yet ready for production use, but it is a good starting point for learning and experimenting with C++ and game development.

## Compilation

### Tools needed

- [clang](https://clang.llvm.org/): A compiler for C and C++ languages.
- [cmake](https://cmake.org/): A cross-platform build system generator.
- [conan](https://conan.io/): A package manager for C and C++ libraries.

For development:

- I am using `clang-format` to format the code, current configuration is in the `.clang-format` file. 
- I am using `clang-tidy` to check the code for errors and warnings, current configuration is in the `.clang-tidy` file.

### Tools installation

#### MacOS

```bash
# Install Xcode Command Line Tools (includes clang and make)
xcode-select --install

# Install Conan using Homebrew (package manager for macOS)
brew install conan

# Install LLVM tools for code formatting, analysis and coverage
brew install llvm

# Add LLVM tools to your PATH (add this to your ~/.zshrc or ~/.bashrc)
export PATH="/opt/homebrew/opt/llvm/bin/:$PATH"
```

Verify installations (versions used at the time of writing):

```bash
clang --version
Apple clang version 17.0.0 (clang-1700.0.13.3)
...

cmake --version
cmake version 4.0.1
...

conan --version
Conan version 2.15.1
```

At the moment I'm only testing the code on MacOS.

### Build

#### Debug build

```bash
# Install conan dependencies (do this once)
conan install . --output-folder=build -s build_type=Debug --build=missing

# Configure CMake
cmake --preset debug

# Build everything (library, executable and tests)
cmake --build --preset debug

# Or build specific targets
cmake --build --preset debug --target okinawa       # Just the library
cmake --build --preset debug --target okinawa_test  # Just the tests
```

#### Release build

```bash
# Install conan dependencies (do this once)
conan install . --output-folder=build -s build_type=Release --build=missing

# Configure CMake
cmake --preset release

# Build everything (library and executable)
cmake --build --preset release

# Or build specific targets
cmake --build --preset release --target okinawa       # Just the library
```

## Package Distribution

### Creating the Conan Package

To use okinawa.cpp in other projects, you need to create it as a local Conan package:

```bash
# Create the package in your local Conan cache
conan create . --build=missing

# This will build and install the package as okinawa/0.1.0
```

The package includes:
- **Library**: `libokinawa.a` (static library)
- **Headers**: All public headers under `okinawa/` namespace
- **Dependencies**: Automatically manages glm, glfw, stb, and OpenGL dependencies

### Using the Package in Other Projects

In your project's `conanfile.txt`:

```ini
[requires]
okinawa/0.1.0
# ... other dependencies ...

[generators]
CMakeDeps
CMakeToolchain
```

In your project's `CMakeLists.txt`:

```cmake
find_package(okinawa REQUIRED)
target_link_libraries(your_target PRIVATE okinawa::okinawa)
```

Include headers in your C++ code:

```cpp
#include "okinawa/core/core.hpp"
#include "okinawa/core/camera.hpp"
// ... other okinawa headers as needed ...
```

### Package Development Workflow

When developing the okinawa engine:

1. **Make changes** to the source code
2. **Test locally** using the build commands above
3. **Recreate the package** when ready to use in other projects:
   ```bash
   conan create . --build=missing
   ```
4. **Rebuild dependent projects** to use the updated package

### Other tools

#### Code checking

```bash
# Check code formatting
clang-tidy -p=build src/*.cpp src/*/*.cpp
```

### Tests

To run the tests (from project root):

```bash
# Run tests through CTest
ctest --test-dir build --output-on-failure

# Or run test executable directly for more verbose output
cd build 
./bin/okinawa_test
cd ..
```

To generate a code coverage report:

```bash
# Build and run tests with coverage 
cmake --build build --target coverage

# Open the coverage report
open build/coverage/index.html
```

The tests are located in the `tests` folder and use the [Catch2](https://github.com/catchorg/Catch2) testing framework. The coverage report shows detailed line-by-line code coverage information.

## Libraries used

- [glfw](https://github.com/glfw/glfw): A multi-platform library for OpenGL, OpenGL ES, Vulkan, window and input.
- [glm](https://github.com/g-truc/glm): OpenGL Mathematics (GLM), is a header only C++ mathematics library for graphics software.
- [stb-image](https://github.com/nothings/stb): Single-file public domain image loading library.
- [Catch2](https://github.com/catchorg/Catch2): A modern, C++-native, header-only, test framework for unit-tests, TDD and BDD.
