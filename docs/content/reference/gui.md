---
title: GUI
section: Reference
nav_order: 13
---

# GUI

The GUI is drawn in a dedicated pass after the 3D scene, using the same
shader and item machinery as the rest of the engine: GUI elements are plain
`OkItem`s — only *placed* in a special way. There are no parallel 2D
primitives; anything an item can do (textures, rotations, visibility,
wireframe debug, and eventually full 3D models) works inside the GUI.

## The position grid

Every element position and size is expressed in a **grid of cells** instead
of raw pixels:

- The origin `0,0` is the **centre of the screen**.
- Axes follow the engine convention: X+ right, Y+ up.
- One cell is `gui.grid.size` logical pixels (default **20**), multiplied
  by the global `gui.scale`.

Keeping every distance a multiple of one module makes layouts read
cohesive by construction, and `gui.scale` becomes a single knob that
rescales the whole interface. Conversions are exposed as
`OkGui::gridToScreenX/Y` and `OkGui::screenToGridX/Y`.

Logical pixels, not framebuffer pixels: when `gui.scale` is `0` (the
default) the effective scale is resolved from the monitor content scale as
`contentScale * (windowSize / framebufferSize)`, which is `1.0` on macOS
retina (window coordinates are already density-independent points) and the
monitor content scale on platforms whose window coordinates are physical
pixels. The UI keeps its apparent size on HiDPI displays either way.

## The calibrated GUI camera

The pass renders with a fixed **perspective** camera looking at the `Z=0`
plane from a distance calibrated per window size:

```
D = (logicalHeight / 2) / tan(gui.fov / 2)
```

At `Z=0` one world unit projects to exactly one logical pixel, so
unrotated elements are pixel-exact on the grid — while **rotated elements
get true perspective foreshortening** with the engine's own math (an
oblique speedometer, a menu panel with vanishing lines). `gui.fov`
(default 35°) is the aesthetic knob: a larger fov makes oblique elements
converge harder; a tiny fov approaches an orthographic look.

The pass draws with blending enabled and depth testing disabled: depth is
the paint order (far to near), everything sits at the same real Z.

## Layers

GUI depth is a list of named layers (`OkGui::addLayer(name, order)`),
rendered from the **lowest order to the highest** — far to near, a higher
order paints on top. Within a layer, items draw in insertion order. A layer
owns the items added to it (`addItem` transfers ownership) and destroys
them with `removeLayer`. When solid 3D models join the GUI, the pass can
clear the depth buffer *between* layers — list order across layers, true Z
within one — without changing this design.

## OkGuiImage

The first grid-placed element: a textured quad that is simply an `OkItem`
(same texture loading, rotation, visibility, wireframe debug). The only
new surface is grid placement:

```cpp
OkGuiLayer *hud = OkGui::addLayer("hud", 0);

OkGuiImage *img = new OkGuiImage("speedo");
img->loadTextureFromFile("assets/speedo.png");
img->setGridPosition(9.0f, 5.0f);   // element centre, in cells
img->setGridSize(6.0f, 6.0f);       // width/height, in cells
img->setRotation(0.0f, 0.7f, 0.0f); // optional: oblique HUD (radians)
hud->addItem(img);
```

The quad is a unit square centred on its origin, so rotations pivot on the
element centre and the grid size maps to the item scaling. An unrotated
image lands pixel-exact on the grid; a rotated one converges with real
perspective thanks to the calibrated camera.

## Anchors

Grid coordinates are relative to an **anchor point** (`setGridAnchor`,
default `OK_GUI_ANCHOR_CENTER`): the screen centre, an edge midpoint or a
corner. Edge and corner anchors keep HUD elements stable across aspect
ratios — a minimap anchored `OK_GUI_ANCHOR_BOTTOM_RIGHT` at `(-3, 3)`
stays three cells inside its corner on every monitor, where centre-only
coordinates would drift with the window width. Axes stay the same
everywhere (X+ right, Y+ up), so insets from the right or top edges use
negative offsets.

## Text: OkFont and OkGuiText

The engine ships a built-in 5x7 bitmap font (`OkFont`) covering printable
ASCII (lowercase maps to uppercase, console style). Two ways to use it:

- **`OkFont::bake(name, text, scale, fg, bg)`** rasterizes a string on the
  CPU into an RGBA `OkTexture`. The right tool for **static text**: bake
  once, hand the texture to an `OkGuiImage` or a billboard and drop the
  font from the picture. A transparent `bg` gives blended GUI text; an
  opaque one suits unblended contexts (billboards).
- **`OkGuiText`** is the dynamic element: `setText` rebuilds a mesh with
  one quad per character against a shared glyph **atlas** (nearest
  filtering, built once), so changing text every frame never allocates
  textures. Grid placement mirrors OkGuiImage (`setGridPosition`,
  `setGridAnchor`, `setGridHeight` in cells; width follows the length).
  `setTextColor` tints the white atlas; `bakeTexture()` converts the
  current string into a standalone texture (the static-text path above).

## The console

A Quake-style drop-down console (`OkConsole`), toggled with the grave key
(`` ` ``). While open it **captures the whole keyboard** — the game sees no
keys, so typing cannot trigger gameplay bindings. It covers the top half
of the screen (its own GUI layer), with scrollback, command history
(up/down) and a blinking cursor.

The command set is extensible per game:

```cpp
OkConsole::registerCommand("ground", "toggle the terrain",
    [](const std::vector<std::string> &args) {
      // ...
      OkConsole::print("ground toggled");
    });
```

Engine built-ins: `help` (list every registered command), `clear`,
`quit`, and `set` / `get` over the whole `OkConfig` key space — `set
gui.debug.grid true` works out of the box for every config value,
present or future. `get` also takes a prefix: `get gui` lists every
`gui.*` key name; `get gui.sc` narrows to a single key and prints its
value directly.

## Debug grid overlay

`OkGui::setDebugGrid(true)` (or the `gui.debug.grid` config key) overlays
the authoring grid: one faint line per cell, a stronger line every 5
cells, and the two `0,0` axes highlighted. It rebuilds itself when the
window size, cell size or scale change, so it is also the quickest way to
verify the coordinate system on a new monitor or window size.

## OkGuiStats

A small runtime statistics panel, in the spirit of the debug overlays
engines ship with: numbers plus a live frame-time graph, anchored to a
screen corner so it survives resizes. It is built on the same GUI pieces
any project would use — a layer, text elements and an image — and reads
what the engine already tracks, so it adds nothing to the frame it is
measuring beyond its own handful of quads.

It reports frames per second and average frame time, the worst recent
frame, draw calls and triangles submitted, scene objects and how many
were culled, loaded textures, and the day clock.

The graph is the point: an average hides hitches, a history shows them.

The panel is off by default and toggled with the `stats` console
command or `OkGuiStats::setVisible`. Frame times are recorded either
way, so opening it shows what just happened rather than starting
blank — and so a tool can ask for the series at any moment.

| Method | Purpose |
| --- | --- |
| `static const std::vector<float> &getHistory()` | The recorded frame times, oldest first, milliseconds. |
| `static void getSummary(int &count, float &min, float &max, float &mean, float &median)` | Summary of that history; `count` is 0 when nothing is recorded yet. |
| `static void setHistoryLength(int samples)` | How many samples to keep (600 by default, several seconds even at a high frame rate). The graph draws its own shorter window. |

The history is deliberately longer than the strip the panel draws: the
graph is for spotting a hitch at a glance, the history is for answering
"is this build slower than that one", which one frame cannot do. The
MCP server exposes it as `get_performance`.
The panel keeps the last few seconds as a strip, one column per frame,
coloured green inside a 60Hz budget, amber past it and red beyond two
refreshes, with a reference line at 16.7 ms. Its vertical range adapts
to the worst sample so the strip stays informative at any framerate.

Off by default. The panel registers a console command:

```
stats            # toggle
stats on
stats off
```

| Method | Purpose |
| --- | --- |
| `static void initialize()` | Build the panel and register its command (called by the core). |
| `static void update(float dtMs)` | Refresh the readings (called by the core each frame). |
| `static void setVisible(bool)` / `static bool isVisible()` | Show or hide it. |

Draw calls and triangles come from counters kept in `OkFrustum`
alongside the culling statistics: `getDrawCalls()`, `getTriangles()` and
`getCulledCount()`, all reset at the start of each frame. A project can
read them directly to assert a budget in a test.

## Configuration keys

| Key | Default | Meaning |
| --- | --- | --- |
| `gui.grid.size` | `20` | Grid cell size in logical pixels. |
| `gui.scale` | `0` | Global UI scale; `0` resolves it from the monitor content scale. |
| `gui.fov` | `35` | Field of view (degrees) of the calibrated GUI camera. |
| `gui.debug.grid` | `false` | Show the debug grid overlay. |
