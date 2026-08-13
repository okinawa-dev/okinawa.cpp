---
title: MCP server
section: Reference
nav_order: 12
---

# MCP server

Okinawa ships an optional in-engine MCP (Model Context Protocol) server. It lets an external agent connect to a running app over local HTTP and observe or drive it through the tools listed below. The public surface (`OkMcpServer`) deliberately exposes no HTTP or JSON types: all of that lives behind a pimpl, so consumers do not inherit those dependencies.

## Tools

The server exposes these tools to a connected agent:

| Tool | What it does |
| --- | --- |
| `view_frame` | Returns the current rendered frame as a PNG image, so the agent can see what is on screen. |
| `screenshot` | Writes the current frame to a PNG file on disk (for a human) and returns the path. Optional `path` (default `okinawa-screenshot.png`). |
| `press_key` | Holds a key for a duration to drive the app for gameplay: W/A/S/D move, SPACE/T/R/F are actions, 1-9 switch camera, arrows turn. Args: `key`, `duration_ms` (default 120). (For positioning the view, use `view` instead.) |
| `press_keys` | Holds several keys at once (e.g. W and D for diagonal movement). Args: `keys`, `duration_ms`. |
| `view` | **The camera tool.** Sets the whole viewpoint in one call. `camera` (optional) activates a camera **by its registered name** (`get_state` lists them under `cameras`); the tool then drives the *active* camera and never force-switches on its own. `x`, `y`, `z` place the avatar. Orbit cameras take `yaw_deg` (compass facing), `pitch_deg` (tilt; negative looks down, `~-89` = top-down) and `distance` (metres back); overhead/fixed cameras take just `distance` (their height). All fields optional; an omitted field keeps its current value. Persistent (survives input). `get_state` returns the same values, with the active camera's name, under `view` — reproduce any viewpoint by passing them straight back. Returns the resulting view. |
| `set_item_visible` | Show/hide scene items by name to isolate geometry. With `prefix: true` it applies to every item whose name starts with `name` (e.g. `tree_` to hide every tree at once, or `tree_oak_` to narrow it further); otherwise it toggles the single item with that exact `name`. Returns how many items changed. |
| `console` | Runs one console command line, exactly as if it had been typed into the drop-down console and submitted, and returns what it printed. Arg: `line` (e.g. `time 22`, `set shadows.cascades 3`); with no `line` it returns the registered command names instead. The console does not need to be open, and its open state is left as it was found. This is how an agent drives the console — see the note below on why typing it key by key is not a real option. |
| `config` | Reads or writes an engine config key at runtime — the same keys the console's `set`/`get` reach (`shadows.*`, `render.*`, `graphics.*`, plus any the application registers). With no `value` it reads; with one it writes, converting to the key's existing type. Pass `prefix` and no `key` to list every matching key with its current value. |
| `get_performance` | Returns the frame-time **series**, not a single reading: `count`, `frame_ms` and `fps` each with min/max/mean/median, and a `hitching` flag (true when the mean sits well above the median, which is what occasional long frames look like in a summary). Also returns `draw_ms` (min/max/mean/median), the CPU time spent issuing the frame's draws, measured before the swap: where vsync is enforced by the platform, `frame_ms` is pinned to the refresh interval and cannot tell whether a change cost anything, and `draw_ms` is the number to compare. Pass `samples: true` for the raw per-frame milliseconds, oldest first. Samples are recorded whether or not the stats panel is visible. |
| `get_state` | Returns numeric runtime state, including `view` (the active camera's name and values, ready to pass back to `view`), `cameras` (the registered camera names), the raw camera pose, fps, scene object count, window size and resident memory. |

Key names accepted by `press_key`/`press_keys`: single letters and digits, `space`, `up`/`down`/`left`/`right`, `escape`, `enter`, `tab`, `backspace`, `grave` (or `backtick`), `period`, `minus`. `grave` toggles the engine console; while the console is open, injected printable keys feed its input line, so a command *can* be typed a key at a time (`grave`, then the letters with `space`/`period`, then `enter`).

Do not drive the console that way. Each key is one MCP call with its own round trip, so a fifteen-character line costs sixteen of them and takes about as many seconds — long enough that an agent stops reaching for the commands it most needs while debugging. Use `console` instead: one call, one line, and the command's own output comes back as the result.

## Measuring performance

`get_state` reports an instantaneous `fps`. That number is close to
useless on its own: it describes one frame, from one viewpoint, and two
such readings cannot tell a real change from a lucky moment. Use
`get_performance`, which returns the whole recorded window.

Two things worth knowing when reading the result:

- **Compare like with like.** The same build reads completely
  differently close to the ground and from above. State the viewpoint
  alongside the number, and when comparing two builds, measure both
  from the same place — ideally in the same session.
- **Watch for quantisation.** With vsync on, frame times land on
  multiples of the refresh interval: on a 60 Hz display a median of
  16.7, 33.3 or 50 ms means one, two or three refreshes per frame. A
  median pinned to exactly 33.3 ms says the frame costs somewhere
  between 16.7 and 33.3 ms, not that it costs 33.3, and a small extra
  cost will drop it a whole step to 50 ms. Read `min` alongside the
  median to see where the real work sits.

  This also explains a frame rate that seems to halve for no reason:
  near the boundary, a couple of milliseconds either way moves the
  whole frame between one refresh and two. `render.vsync 0` asks for
  vsync off so the times show the real work instead — but a compositor
  can enforce it anyway (macOS does), in which case the readings stay
  quantised and only `samples_ms` tells the full story.

## When a call fails

A tool that cannot do what was asked answers with an ordinary result
carrying `isError: true` and a line of text saying why — an unknown
camera name, a key that does not exist, a request that arrived while
nothing was rendering. The HTTP status stays 200: the call was
delivered and answered, and what failed is the tool, not the transport.

The same applies to a malformed argument. Sending a string where a
number belongs is reported as `bad arguments: ...` rather than being
allowed to reach the JSON library and abort the request, so a client
that gets a field wrong is told which one instead of receiving an empty
response.

## Enabling it

From application code, enable the server after `OkCore::initialize()`:

```cpp
OkCore::enableMcpServer();        // binds 127.0.0.1:8765
OkCore::setIgnoreUserInput(true); // optional: MCP-only control
```

`enableMcpServer(int port = 8765)` always exists. If the engine was built without MCP support it logs a warning and does nothing, so apps compile identically with or without the server. The server binds `127.0.0.1` on the given port and logs a line such as `MCP server listening on http://127.0.0.1:8765/mcp`.

## Connecting a client

Once a build with the server enabled is running (it logs `MCP server listening on http://127.0.0.1:8765/mcp`), point your MCP client at that URL. The transport is streamable HTTP.

With the Claude Code CLI, add it on the command line:

```bash
claude mcp add --transport http okinawa http://127.0.0.1:8765/mcp
```

This registers the server (named `okinawa` here) at local scope. Reload or reconnect the client so it picks up the tools, then the tools listed above are available. Remove it later with `claude mcp remove okinawa`. The server only exists while the app runs, so start the app and reconnect each session.

## Compile-time toggle

The server is compiled in by default and guarded by `OKINAWA_WITH_MCP` (xmake option `mcp`). Exclude it from lean builds with `xmake f --mcp=n`, or be explicit with `--mcp=y`. When excluded, no MCP/HTTP code or its header-only dependencies land in the binary.

## OkMcpServer methods

`OkMcpServer` is normally managed by `OkCore`; you rarely construct it directly. Its public surface:

| Method | Purpose |
| --- | --- |
| `OkMcpServer(int port)` | Construct bound to a port. |
| `void start()` | Start the HTTP server thread. |
| `void stop()` | Stop the server and join its thread. |
| `void drainCommands()` | Run queued tool commands on the engine loop thread. |

`drainCommands()` must be called once per frame from the engine loop thread (the one holding the OpenGL context), after the scene is rendered and before the buffers are swapped. `OkCore::loop` does this for you when the server is enabled.
