#ifndef OK_MCP_CONFIG_HPP
#define OK_MCP_CONFIG_HPP

// Decides whether the in-engine MCP server is compiled in (OKINAWA_WITH_MCP).
//
// Default is MODE-dependent and resolved by the compiler, not the build
// scripts: ON in debug builds, OFF in release (release defines NDEBUG). This
// sidesteps xmake's inability to read the build mode at the right phase when
// the engine is consumed through `includes()` by another project.
//
// The build system can force the choice by defining OKINAWA_MCP_FORCE to 1
// (force on) or 0 (force off) — the xmake option `mcp=on|off`. The default
// `mcp=auto` defines nothing here, leaving the NDEBUG-based decision.
//
// Include this header before any `#ifdef OKINAWA_WITH_MCP` check.
#ifdef OKINAWA_MCP_FORCE
#if OKINAWA_MCP_FORCE
#define OKINAWA_WITH_MCP
#endif
#elif !defined(NDEBUG)
#define OKINAWA_WITH_MCP
#endif

#endif  // OK_MCP_CONFIG_HPP
