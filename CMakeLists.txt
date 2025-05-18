cmake_minimum_required(VERSION 3.15)

# Set the build type if not already defined
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

# Conan 2.x + CMake integration requires setting the toolchain before project()
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
    message(FATAL_ERROR "The file conan_toolchain.cmake doesn't exist, you must install dependencies first:\nconan install . --output-folder=build --build=missing")
endif()
include("${CMAKE_BINARY_DIR}/conan_toolchain.cmake")

project(okinawa 
    VERSION 0.1.0
    DESCRIPTION "A C++ 3D game engine"
    LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Generate compile_commands.json for tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find dependencies (Conan 2.x style)
find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Catch2 REQUIRED)
find_package(stb REQUIRED)
find_package(OpenGL REQUIRED)

# Source files
file(GLOB_RECURSE OKINAWA_SOURCES 
    src/*/*.cpp)

# Create main library target
add_library(${PROJECT_NAME}_lib STATIC ${OKINAWA_SOURCES})
target_include_directories(${PROJECT_NAME}_lib
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

# Link libraries to the lib target
target_link_libraries(${PROJECT_NAME}_lib
    PUBLIC
        glm::glm
        glfw
        stb::stb
        OpenGL::GL
)

if(APPLE)
    target_link_libraries(${PROJECT_NAME}_lib
        PRIVATE
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
    )
endif()

# Create main executable
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_lib)

# Enable warnings for both targets
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${PROJECT_NAME}_lib PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# =====================================
# Testing with code coverage
# =====================================

# Copy test data files to build directory
file(GLOB TEST_DATA_FILES "${CMAKE_SOURCE_DIR}/tests/*.txt")
file(COPY ${TEST_DATA_FILES} DESTINATION "${CMAKE_BINARY_DIR}/tests")

# Create test executable
file(GLOB_RECURSE TEST_SOURCES tests/*.cpp)
add_executable(${PROJECT_NAME}_test ${TEST_SOURCES})
target_link_libraries(${PROJECT_NAME}_test
    PRIVATE
        ${PROJECT_NAME}_lib
        Catch2::Catch2WithMain
)

# Include directories for tests
target_include_directories(${PROJECT_NAME}_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}/tests
)

# Set working directory for tests
set_target_properties(${PROJECT_NAME}_test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Set up code coverage with LLVM/Clang
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Enable code coverage flags
    target_compile_options(${PROJECT_NAME}_test PRIVATE -fprofile-instr-generate -fcoverage-mapping)
    target_link_options(${PROJECT_NAME}_test PRIVATE -fprofile-instr-generate)

    # Create coverage directory
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/coverage)

    # Custom target to run tests with coverage
    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E env LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}/okinawa.profraw $<TARGET_FILE:${PROJECT_NAME}_test> --reporter=console
        COMMAND llvm-profdata merge -sparse ${CMAKE_BINARY_DIR}/okinawa.profraw -o ${CMAKE_BINARY_DIR}/okinawa.profdata
        COMMAND llvm-cov show $<TARGET_FILE:${PROJECT_NAME}_test>
                -instr-profile=${CMAKE_BINARY_DIR}/okinawa.profdata
                -format=html
                -show-line-counts-or-regions
                -show-instantiations
                -ignore-filename-regex='.*tests'
                -output-dir=${CMAKE_BINARY_DIR}/coverage
                ${CMAKE_SOURCE_DIR}/tests/*.cpp
                ${CMAKE_SOURCE_DIR}/src/*/*.cpp
        COMMAND ${CMAKE_COMMAND} -E echo "Coverage report generated in ${CMAKE_BINARY_DIR}/coverage/index.html"
        DEPENDS ${PROJECT_NAME}_test
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running tests and generating coverage report..."
    )
endif()

# Add CTest integration
include(CTest)
include(Catch)
catch_discover_tests(${PROJECT_NAME}_test)

