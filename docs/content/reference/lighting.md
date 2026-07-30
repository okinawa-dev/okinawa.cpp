---
title: Lighting
section: Reference
nav_order: 14
---

# Lighting

`OkLighting` is the scene's global lighting and atmosphere handler. Its
first layer is ATMOSPHERE: a day clock driving fog and a global scene
tint, so the world reads as morning, noon, sunset or night before any
light source exists.

## The day clock

The hour of day (0-24) lives in the config as `lighting.time`, advancing
every frame at `lighting.timescale` times real speed (default 30: a full
day in 48 real minutes; 0 freezes the clock). Both are plain config
values, so they are scriptable and console-reachable; two commands wrap
them:

```
time            # print the current hour
time 21.5       # jump to 21:30
timescale 0     # freeze the clock
timescale 300   # a full day in 4.8 real minutes
```

## The atmosphere curve

A keyframe curve — deep night, dawn, day, sunset, dusk — interpolates
per frame:

- **Scene tint**: a colour multiplied over every world fragment. Neutral
  at noon, warm amber through the sunset, cold blue-teal at night — the
  night look's "two temperatures" starts here.
- **Fog colour and density**: exponential distance fog
  (`exp(-density * viewDistance)`). Distance dissolves into a milky
  haze that thickens at night. Until a skybox exists, the frame clear
  colour IS the fog colour, so the city fades into the sky seamlessly.
- **Sun colour and direction**: elevation follows a sine over the 6h-21h
  daylight arc, azimuth sweeps east to west, parked below the horizon at
  night. Consumed every frame by the Gouraud sun (below).
- **Ambient light**: the flat floor under the directional sun — higher at
  night (no sun: the ambient carries the whole city and the tint does the
  darkening), lower by day so the sun's modelling reads.

- **Sky zenith colour**: the top of the procedural skybox.

## The skybox

`OkSkybox` draws a low-poly gradient dome first in the frame (camera-
centred, depth writes off, so the whole scene paints over it): the
HORIZON colour is the fog colour — the fogged city always fades into the
sky seamlessly — and the top is the curve's zenith colour, from petrol
blue at night to clear blue at noon. The 1-D gradient texture refreshes
itself when the cycle's colours drift. The dome reaches slightly below
the horizon so no gap ever shows; the emissive skyline belt (distant lit
windows) is a later follow-up on the same dome.

## The Gouraud sun

Every `OkItem` carries per-vertex normals (computed at construction when
the caller provides none — see the Items reference), and the world vertex
shader evaluates a classic Gouraud directional light per vertex:
`ambient + sunColor * max(dot(normal, -sunDirection), 0) * 0.6`, with the
lit value interpolated across the triangle. Facades facing the sun warm
up, opposite faces fall to the ambient floor, and the whole city reads as
volume instead of flat panels. Only TEXTURED surfaces are sunlit: the
untextured fill/wireframe branch (debug layers, graph lines) keeps its
exact requested colour. The skybox and the GUI pass run with
`lightingOn = 0`, which makes the Gouraud stage a neutral 1.

## Point lights and halos

`OkLighting` keeps a small registry of point lights (up to 256):
`registerLight(x, y, z, r, g, b, radius)` / `clearLights()`. Every item
is lit by its nearest few lights (budget of 4, the era-friendly model,
no shadows): the selection is cached per item and refreshed only when
the registry generation changes, and the lighting itself is evaluated
PER FRAGMENT with a quadratic falloff inside each light's radius — with
the city's huge ground triangles, per-vertex point light would smear one
lit vertex across a 100 m face.

The glow itself is a separate, composable piece: `getHaloTexture()`
returns a shared radial falloff disc ("ok_halo"), and a light's halo is
an `OkBillboard` with that texture plus three `OkItem` flags:
`setAdditive(true)` (additive blending, no depth writes),
`setUnlit(true)` (light sources are not tinted by the atmosphere or lit
by the sun) and `setProximityFade(metres)` (the quad fades out as the
camera approaches — without it a billboard crossing the camera plane
fills the screen). Halos live in the scene like any item: frustum
culled, fogged with distance, blurred by the depth of field.

`OkLighting::evaluate(hour, ...)` exposes the pure curve for tests and
tools; the interpolated values are read every frame by the render pass
(`getSceneTint`, `getFogColor`, `getFogDensity`, `getSunColor`,
`getSunDirection`, `getAmbientLight`).

The GUI pass resets the tint, fog and lighting uniforms: the interface is
never tinted, fogged or sunlit.

## Configuration keys

| Key | Default | Meaning |
| --- | --- | --- |
| `lighting.time` | `12` | Hour of day, 0-24 (wraps). |
| `lighting.timescale` | `30` | Clock speed vs real time; `0` freezes it. |
| `lighting.fog` | `true` | Distance fog on/off (the colour keeps driving the sky and clear). |
