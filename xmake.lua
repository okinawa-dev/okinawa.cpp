-- okinawa - xmake build
--
-- Build:        xmake
-- Run tests:    xmake run okinawa_test   (or: xmake test)
-- Coverage:     xmake coverage           (custom task, llvm source-based)
--
-- Third-party dependencies are fetched and built by xrepo (xmake's package
-- manager); there is no separate "install dependencies" step.

set_project("okinawa")
set_version("0.1.0")

set_languages("cxx17")
set_warnings("all")                 -- -Wall
add_cxflags("-Wundef", "-Wmacro-redefined", "-Wextra-semi", {tools = {"clang", "gcc"}})
set_symbols("debug")                -- always emit debug info (-g)

-- Keep compile_commands.json in sync for clangd / tooling.
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".", lsp = "clangd"})

-- Build modes. Debug is the default; `xmake f -m release` for release.
-- Three modes, not two. `debug` (-O0) is for stepping through code, and it
-- is far too slow to judge how the project performs -- unoptimised
-- vector maths alone halves the frame rate. `releasedbg` is the one to
-- develop against: optimised like release but with symbols, so a
-- profiler and a debugger still work.
add_rules("mode.debug", "mode.releasedbg", "mode.release")
set_defaultmode("debug")

-- Compile-time toggle for the in-engine MCP server.
--
-- DEFAULT is mode-dependent: ON in debug, OFF in release. That decision is made
-- by the COMPILER (NDEBUG) in mcp-config.hpp, not here: xmake cannot read the
-- build mode at a phase that also propagates through `includes()` to consumers
-- (wadviewer) -- is_mode()/get_config("mode") are nil at global scope, a
-- per-target on_config does not run for an included sub-target, and an option's
-- dynamic on_check defines do not propagate either. Only an option's STATIC
-- add_defines propagates, so the OVERRIDE uses two such options (use at most
-- one); each sets OKINAWA_MCP_FORCE, which mcp-config.hpp honours:
--   xmake f --mcp=y      force the server ON  (e.g. MCP in a release build)
--   xmake f --no-mcp=y   force the server OFF (e.g. a lean debug build)
option("mcp")
    set_default(false)
    set_showmenu(true)
    set_description("Force-compile the in-engine MCP server (override the debug/release default)")
    add_defines("OKINAWA_MCP_FORCE=1")
option_end()

option("no-mcp")
    set_default(false)
    set_showmenu(true)
    set_description("Force-exclude the in-engine MCP server (override the debug/release default)")
    add_defines("OKINAWA_MCP_FORCE=0")
option_end()

-- Third-party dependencies (resolved from xrepo). Must live in the root scope.
add_requires("glm")
add_requires("glfw")
add_requires("stb")
add_requires("opengl")
-- GLEW is the OpenGL function loader on non-Apple platforms; macOS uses the
-- system OpenGL framework directly (see core/gl_config.hpp), so glew is only
-- pulled in off macOS.
if not is_plat("macosx") then
    add_requires("glew")
end
add_requires("catch2")              -- only used by the test target
add_requires("cpp-httplib")         -- MCP server (header-only)
add_requires("nlohmann_json")       -- MCP server (header-only)

-- =========================================================================
-- Engine static library
-- =========================================================================
target("okinawa")
    set_kind("static")
    add_files("src/**.cpp")
    -- Headers live under src/okinawa/, so consumers include them with the
    -- "okinawa/" prefix (e.g. #include "okinawa/core/core.hpp") via the
    -- public "src" root. The private "src/okinawa" root lets the engine's
    -- own sources keep their internal, unprefixed includes (e.g.
    -- #include "math/point.hpp").
    add_includedirs("src/okinawa")             -- private: engine-internal includes
    add_includedirs("src", {public = true})    -- public: consumers use okinawa/ prefix
    add_packages("glm", "glfw", "stb", "opengl", {public = true})
    -- GLEW loader off macOS (macOS links the OpenGL framework below instead).
    if not is_plat("macosx") then
        add_packages("glew", {public = true})
    end

    -- In-engine MCP server: add_options applies the OKINAWA_MCP_FORCE define
    -- from whichever of --mcp / --no-mcp is set (neither -> NDEBUG default).
    -- mcp-config.hpp turns that into OKINAWA_WITH_MCP, which guards
    -- mcp-server.cpp (an empty TU when off). The header-only deps are always
    -- linked (harmless: only #included from inside that guard).
    add_options("mcp", "no-mcp")
    add_packages("cpp-httplib", "nlohmann_json")

    -- macOS windowing/runtime frameworks required by GLFW.
    if is_plat("macosx") then
        add_frameworks("Cocoa", "IOKit", "CoreVideo", "OpenGL")
    end

-- =========================================================================
-- Test suite (Catch2)
-- =========================================================================
target("okinawa_test")
    set_kind("binary")
    set_default(false)              -- not built by a bare `xmake`; build explicitly
    add_files("tests/*.cpp")
    add_includedirs("tests")
    add_deps("okinawa")
    add_packages("catch2")
    -- Tests read data files by project-relative paths (e.g. tests/test-file.txt).
    set_rundir("$(projectdir)")
    add_tests("default")

-- The documentation-site generator is a separate, standalone xmake project at
-- docs/tool/ (it needs only md4c, never the engine's GL packages). Build it
-- with `xmake -P docs/tool build okinawa-docs`.

-- =========================================================================
-- Coverage task: llvm source-based coverage over the test run.
--   xmake coverage   ->  HTML report in ./coverage/index.html
-- =========================================================================
task("coverage")
    set_menu({usage = "xmake coverage", description = "Run tests and generate an llvm-cov HTML report"})
    on_run(function ()
        import("core.project.project")
        os.exec("xmake build okinawa_test")

        local profraw  = "build/okinawa.profraw"
        local profdata = "build/okinawa.profdata"
        local testbin  = project.target("okinawa_test"):targetfile()
        local libbin   = project.target("okinawa"):targetfile()

        os.tryrm(profraw)
        os.tryrm(profdata)
        os.mkdir("coverage")

        os.execv(testbin, {"--reporter=console"}, {envs = {LLVM_PROFILE_FILE = profraw}})
        os.exec("llvm-profdata merge -sparse %s -o %s", profraw, profdata)
        os.exec("llvm-cov show %s -instr-profile=%s -format=html " ..
                "-show-line-counts-or-regions -show-instantiations -show-expansions " ..
                "-use-color -output-dir=coverage -ignore-filename-regex=.*tests/.* ",
                libbin, profdata)
        print("Coverage report generated in coverage/index.html")
    end)
