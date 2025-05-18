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

# Always include debug info for coverage
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

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
file(GLOB_RECURSE PROJECT_SOURCES 
    src/*.cpp
    src/*/*.cpp
    src/*.hpp
    src/*/*.hpp)

# Create main library target
add_library(${PROJECT_NAME}_lib STATIC ${PROJECT_SOURCES})
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

# Enable warnings for library target
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${PROJECT_NAME}_lib PRIVATE -Wall -Wextra -Wpedantic)
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
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Set up code coverage with LLVM/Clang
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Set coverage compile flags for both the library and test executable
    set(COVERAGE_FLAGS -fprofile-instr-generate -fcoverage-mapping)
    set(COVERAGE_LINK_FLAGS -fprofile-instr-generate)
    
    # Enable coverage for the library
    target_compile_options(${PROJECT_NAME}_lib PRIVATE ${COVERAGE_FLAGS})
    target_link_options(${PROJECT_NAME}_lib PRIVATE ${COVERAGE_LINK_FLAGS})
    
    # Enable coverage for tests
    target_compile_options(${PROJECT_NAME}_test PRIVATE ${COVERAGE_FLAGS})
    target_link_options(${PROJECT_NAME}_test PRIVATE ${COVERAGE_LINK_FLAGS})

    # Create coverage directory
    file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/coverage)

    # Get all source files for coverage reporting
    file(GLOB_RECURSE ALL_SOURCE_FILES 
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/src/*/*.cpp)

    # Custom target to run tests with coverage
    add_custom_target(coverage
        # Clean old coverage data
        COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.profraw ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.profdata
        
        # Run tests and generate coverage data
        COMMAND ${CMAKE_COMMAND} -E env 
            LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}/${PROJECT_NAME}.profraw 
            CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            $<TARGET_FILE:${PROJECT_NAME}_test> 
            --reporter=console
        
        # Process coverage data
        COMMAND llvm-profdata merge -sparse ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.profraw -o ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.profdata
        
        # Generate coverage report
        COMMAND llvm-cov show 
                $<TARGET_FILE:${PROJECT_NAME}_lib>
                -instr-profile=${CMAKE_BINARY_DIR}/${PROJECT_NAME}.profdata
                -format=html
                -show-line-counts-or-regions
                -show-instantiations
                -show-expansions
                -use-color
                -output-dir=${CMAKE_SOURCE_DIR}/coverage
                -ignore-filename-regex='.*tests/.*'
                ${ALL_SOURCE_FILES}
        
        # Show completion message
        COMMAND ${CMAKE_COMMAND} -E echo "Coverage report generated in ${CMAKE_SOURCE_DIR}/coverage/index.html"
        DEPENDS ${PROJECT_NAME}_test
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running tests and generating coverage report..."
    )
endif()

# Add CTest integration
include(CTest)
include(Catch)
catch_discover_tests(${PROJECT_NAME}_test)

