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

## Directional shadows

`OkShadowMap` renders the scene's depth once per frame from the
directional light and the world pass compares against it: a fragment
further from the light than what the light could see is in shadow.

The map covers a box that FOLLOWS THE VIEWER — there is no point
spending resolution on ground nobody can see — and its origin is snapped
to whole texels, without which the sampling grid slides under the
geometry as the camera moves and every shadow edge shimmers. Filling the
map culls FRONT faces, which pushes the recorded depth to the back of
each caster and removes most of the self-shadowing acne a bias alone
would have to hide. Sampling uses a small percentage-closer kernel, so
edges are softened rather than stair-stepped.

Only the DIRECTIONAL contribution is shadowed: the ambient floor and the
point lights still reach a shadowed surface, which is what keeps shadows
from becoming black holes. Strength follows the light's elevation and
fades to nothing as it reaches the horizon, where a hard shadow would
look wrong anyway.

| Key | Default | Meaning |
| --- | --- | --- |
| `shadows` | `true` | Shadow pass on/off. |
| `shadows.size` | `2048` | Depth map resolution. |
| `shadows.extent` | `90` | Half-width in metres of the area covered. |
| `shadows.strength` | `0.62` | How dark a fully shadowed surface goes. |
| `shadows.bias` | `0.0016` | Depth bias against self-shadowing. |

## The sun's body

`OkSkybox` also draws the light's visible body: a camera-facing disc
with a solid core inside a soft corona, placed on the dome along the
light's OWN direction, so what casts the shadows is what is seen in the
sky. It takes the cycle's sun colour, and fades out as it sinks below
the horizon.

## Point lights and halos

`OkLighting` keeps a small registry of point lights (up to 256), in two
flavours:

- `registerLight(x, y, z, r, g, b, radius)` — an OMNI light radiating
  equally in every direction (a bare bulb, a window glow).
- `registerSpotLight(x, y, z, r, g, b, radius, dirX, dirY, dirZ,
  coneDeg, intensity)` — the same light with a direction, a cone
  half-angle (degrees, soft-edged) and an intensity multiplier over the
  colour. A spot aimed downward pools its light on the surface below,
  the usual shape for an overhead fixture.

`clearLights()` empties the registry. Every item
is lit by its nearest few lights (budget of 4, the era-friendly model,
no shadows): the selection is cached per item and refreshed only when
the registry generation changes, and the lighting itself is evaluated
PER FRAGMENT with a quadratic falloff inside each light's radius — with
large triangles, per-vertex point light would smear a single lit vertex
across the whole face.

The glow itself is a separate, composable piece: `getHaloTexture()`
returns a shared radial falloff disc ("ok_halo"), and a light's halo is
an `OkBillboard` with that texture plus three `OkItem` flags:
`setAdditive(true)` (additive blending, no depth writes),
`setUnlit(true)` (light sources are not tinted by the atmosphere or lit
by the sun) and `setProximityFade(metres)` (the quad fades out as the
camera approaches — without it a billboard crossing the camera plane
fills the screen). Halos live in the scene like any item: frustum
culled, fogged with distance, blurred by the depth of field.

## Clustered forward

Point lights are selected PER PIXEL, not per object. `OkLightClusters`
divides the view frustum into a 3D grid (16 x 9 x 24: screen tiles by
exponential depth slices) and, every frame on the CPU, assigns each light
to the clusters its sphere of influence touches; the world fragment
shader finds its own cluster from `gl_FragCoord` and the fragment depth
and iterates only those lights.

This is what a city of huge meshes needs: a sidewalk item spanning a
whole block now gets every lamp along it, instead of the four nearest to
the item's centre. Two details matter:

- **Culling is by sphere of influence, not by visibility** — a lamp
  around the corner still lights the street it spills into.
- **Lights are sorted by view distance before assignment**, because
  clusters have a per-cluster cap: without the ordering, a dozen distant
  lamps fill the budget and the lamp directly overhead is dropped.

Clustering uses its own depth range (1 m to 350 m), independent of the
camera planes: a 0.1 m near plane makes the first exponential slices
microscopic and blows the reference budget. Past that distance the fog
has swallowed everything anyway. Assignment runs on the CPU because the
engine targets OpenGL 4.1 (no compute shaders), and the data reaches the
shader as buffer textures.

`set lighting.clustered false` falls back to the old per-item path
(useful for A/B comparisons).

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
