cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Set the build type if not already defined
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()
option(OKINAWA_BUILD_TESTS "Build okinawa tests" OFF)

# Conan 2.x + CMake integration - handle both standalone and Conan builds
if(EXISTS "${CMAKE_SOURCE_DIR}/build/conan_toolchain.cmake")
    include("${CMAKE_SOURCE_DIR}/build/conan_toolchain.cmake")
    message(STATUS "Using Conan toolchain from ${CMAKE_SOURCE_DIR}/build/conan_toolchain.cmake")
elseif(DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS "${CMAKE_TOOLCHAIN_FILE}")
    # Conan build environment - toolchain already set
    message(STATUS "Using toolchain from CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
else()
    message(WARNING "No Conan toolchain found - using default configuration")
    message(STATUS "To use Conan dependencies, run: conan install . --output-folder=build --build=missing")
endif()

project(okinawa 
    VERSION 0.1.0
    DESCRIPTION "A C++ 3D game engine"
    LANGUAGES CXX)

# Enable clang-tidy
find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY 
        ${CLANG_TIDY_EXE};
        -extra-arg=-Wno-unknown-warning-option;
        -p=${CMAKE_BINARY_DIR}
    )
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
else()
    message(WARNING "clang-tidy not found!")
endif()

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

# Find dependencies (Conan 2.x style) - make optional for packaging
find_package(glm QUIET)
find_package(glfw3 QUIET)  
find_package(stb QUIET)
find_package(opengl_system QUIET)

if(NOT glm_FOUND OR NOT glfw3_FOUND OR NOT stb_FOUND OR NOT opengl_system_FOUND)
    message(STATUS "Some dependencies not found - this is okay for packaging")
    message(STATUS "glm: ${glm_FOUND}, glfw3: ${glfw3_FOUND}, stb: ${stb_FOUND}, opengl_system: ${opengl_system_FOUND}")
endif()

if(OKINAWA_BUILD_TESTS)
    # Find Catch2 only if we're building tests
    find_package(Catch2 REQUIRED)
endif()

# Source files
file(GLOB_RECURSE PROJECT_SOURCES 
    src/*.cpp
    src/*/*.cpp
    src/*.hpp
    src/*/*.hpp)

# Create main library target
add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

# Link libraries to the lib target (conditional for packaging)
if(glm_FOUND AND glfw3_FOUND AND stb_FOUND AND opengl_system_FOUND)
    target_link_libraries(${PROJECT_NAME}
        PUBLIC
            glm::glm
            glfw
            stb::stb
            opengl::opengl
    )
    message(STATUS "Linking dependencies to okinawa")
else()
    message(STATUS "Skipping dependency linking - not all dependencies found")
endif()

if(APPLE)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
    )
endif()

# Enable warnings for library target
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall 
        # -Wextra 
        # -Wpedantic
        -Wundef                      # Warn on undefined macro usage
        # -Wreserved-macro-identifier  # Warn on reserved macro names
        -Wmacro-redefined            # Warn on macro redefinition
        -Wextra-semi                 # Warn on redundant semicolons
    )    
endif()

# =====================================
# Testing with code coverage (if enabled)
# =====================================

option(OKINAWA_BUILD_TESTS "Build okinawa tests" OFF)

if(OKINAWA_BUILD_TESTS)
    # Find Catch2 only if we're building tests
    find_package(Catch2 REQUIRED)

    # Copy test data files to build directory
    file(GLOB TEST_DATA_FILES "${CMAKE_SOURCE_DIR}/tests/*.txt")
    file(COPY ${TEST_DATA_FILES} DESTINATION "${CMAKE_BINARY_DIR}/tests")

    # Create test executable
    file(GLOB_RECURSE TEST_SOURCES tests/*.cpp)
    add_executable(${PROJECT_NAME}_test ${TEST_SOURCES})
    target_link_libraries(${PROJECT_NAME}_test
        PRIVATE
            ${PROJECT_NAME}
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
        target_compile_options(${PROJECT_NAME} PRIVATE ${COVERAGE_FLAGS})
        target_link_options(${PROJECT_NAME} PRIVATE ${COVERAGE_LINK_FLAGS})
        
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
                $<TARGET_FILE:${PROJECT_NAME}>
                -instr-profile=${CMAKE_BINARY_DIR}/${PROJECT_NAME}.profdata
                -format=html
                -show-line-counts-or-regions
                -show-instantiations
                -show-expansions
                # -show-branches=count
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
endif()

# Add CTest integration if tests are enabled
if(OKINAWA_BUILD_TESTS)
    include(CTest)
    include(Catch)
    catch_discover_tests(${PROJECT_NAME}_test)
endif()

# Install targets for Conan packaging
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install headers
install(DIRECTORY src/
    DESTINATION include/okinawa
    FILES_MATCHING PATTERN "*.hpp"
)

