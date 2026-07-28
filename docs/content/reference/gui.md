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

## Debug grid overlay

`OkGui::setDebugGrid(true)` (or the `gui.debug.grid` config key) overlays
the authoring grid: one faint line per cell, a stronger line every 5
cells, and the two `0,0` axes highlighted. It rebuilds itself when the
window size, cell size or scale change, so it is also the quickest way to
verify the coordinate system on a new monitor or window size.

## Configuration keys

| Key | Default | Meaning |
| --- | --- | --- |
| `gui.grid.size` | `20` | Grid cell size in logical pixels. |
| `gui.scale` | `0` | Global UI scale; `0` resolves it from the monitor content scale. |
| `gui.fov` | `35` | Field of view (degrees) of the calibrated GUI camera. |
| `gui.debug.grid` | `false` | Show the debug grid overlay. |
