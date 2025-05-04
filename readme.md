<p align="center">
  <img width="200" alt="okinawa.cpp logo" src="/assets/okinawa_logo.png">
</p>

# okinawa.cpp

This is a work in progress C++ 3D game engine, inspired by the [Okinawa](https://github.com/okinawa-dev/okinawa.js), a previous version coded in JavaScript. The goal is to create a game engine that is easy to use, flexible, and powerful. The engine is designed to be modular, so you can easily add or remove features as needed.

At the moment I'm not using it as a compiled library, but as a git submodule included in the games I develop. I'm currently testing it on a MacOS machine. Feedback is welcome.

With just this repository, without a game, you can just run the test suite and see the coverage report. The engine is not yet ready for production use, but it is a good starting point for learning and experimenting with C++ and game development.

## Compilation

### Tools needed

- [clang](https://clang.llvm.org/): A compiler for C and C++ languages.
- [make](https://www.gnu.org/software/make/): A build automation tool.
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

# Optional: Install clang-format and clang-tidy
# These are not strictly necessary for building, but are useful for code formatting and linting
brew install clang-format
# If you want to use clang-tidy, it usually comes with some plugin for your IDE, or included with llvm
# brew install llvm
```

Verify installations (versions used at the time of writing):

```bash
clang --version
Apple clang version 17.0.0 (clang-1700.0.13.3)
...

make --version
GNU Make 3.81
...

conan --version
Conan version 2.15.1
```

At the moment I'm only testing the code on MacOS.

### Build

Compile for a development environment (it will include tests):

```bash
# Install conan dependencies
make dev
```

### Tests

Run the tests and generate coverage report:

```bash
# Run tests
make test
```

The test coverage report will be generated in `coverage/index.html`. Open it in a browser to see the coverage details. The tests are located in the `tests` folder, and are written using the [Catch2](https://github.com/catchorg/Catch2) testing framework.

## Libraries used

- [glfw](https://github.com/glfw/glfw): A multi-platform library for OpenGL, OpenGL ES, Vulkan, window and input.
- [glm](https://github.com/g-truc/glm): OpenGL Mathematics (GLM), is a header only C++ mathematics library for graphics software.
- [stb-image](https://github.com/nothings/stb): Single-file public domain image loading library.

Only in development mode:

- [Catch2](https://github.com/catchorg/Catch2): A modern, C++-native, header-only, test framework for unit-tests, TDD and BDD.
