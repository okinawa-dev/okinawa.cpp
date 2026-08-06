---
title: Lighting
section: Reference
nav_order: 14
---

# Lighting

`OkLighting` is the scene's global lighting and atmosphere handler. Its
first layer is **atmosphere**: a day clock driving fog and a global scene
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

A keyframe curve — night, dawn, day, sunset, dusk — interpolates per
frame. **The curve is data, not code**: the engine ships a neutral
default (clear day, blue-ish night) so any project renders sensibly out
of the box, and a game supplies its own with
`OkLighting::setAtmosphereCurve(keys, count)`, which is where artistic
direction belongs. Each `OkAtmosphereKey` carries an hour and the look
at that hour:

```cpp
static const OkAtmosphereKey MY_CURVE[] = {
  // hour   tint (rgb)          fog (rgb)           density
  //        sun (rgb)           zenith (rgb)        ambient
  { 0.0f, {0.30f,0.40f,0.48f}, {0.10f,0.16f,0.20f}, 0.0060f,
          {0.0f, 0.0f, 0.0f},  {0.02f,0.05f,0.09f}, 0.22f },
  { 9.0f, {1.00f,1.00f,1.00f}, {0.72f,0.78f,0.85f}, 0.0018f,
          {1.0f, 0.98f,0.92f}, {0.25f,0.48f,0.80f}, 0.55f },
  {20.0f, {1.00f,0.72f,0.52f}, {0.75f,0.50f,0.42f}, 0.0032f,
          {1.0f, 0.55f,0.30f}, {0.25f,0.22f,0.38f}, 0.60f },
  {23.0f, {0.30f,0.40f,0.48f}, {0.10f,0.16f,0.20f}, 0.0060f,
          {0.0f, 0.0f, 0.0f},  {0.02f,0.05f,0.09f}, 0.22f },
};

OkLighting::setAtmosphereCurve(MY_CURVE, 4);
```

Keys go in ascending hour order and the curve wraps around midnight, so
the last key blends back into the first. What gets interpolated:

- **Scene tint**: a colour multiplied over every world fragment. Neutral
  at noon, warm amber through the sunset, cold blue-teal at night — the
  night look's "two temperatures" starts here.
- **Fog colour and density**: exponential fog whose density falls off
  with altitude (see below). Distance dissolves into a milky haze that
  thickens at night. Until a skybox exists, the frame clear colour *is*
  the fog colour, so the scene fades into the sky seamlessly.
### What the shadow pass costs

Two things keep the pass from redrawing the world sixty times a second:

- **It is culled against the light's own volume.** The camera frustum
  is the wrong test (a caster behind the viewer still casts into view),
  but the orthographic shadow box IS the visible area extruded along
  the light, so anything outside it cannot land a shadow anywhere the
  map covers. `shadows.cull` turns this off, which is useful for
  telling a culling bug from a shadowing one.
- **It is not rebuilt when the picture would be identical.** Static
  geometry under a slow sun barely changes between frames. The map is
  redrawn when the sun has turned enough to move an edge by a texel
  (`shadows.refresh.turn`), when the box slides to a new texel, or when
  the scene gains or loses objects — that last one matters with
  streaming, or a newly arrived building would cast nothing until the
  sun moved.

Measured on a city scene, the two together took the pass from 15 ms a
frame to nothing measurable.

A project that can be viewed from height should drive `shadows.distance`
from how high the viewer is: the reach a pedestrian needs is short, and
spending the map on it keeps texels small, while from the air a short
reach means watching shadows arrive. The engine cannot infer it, since
"height" means height above the ground the project defines.

### Height fog

Fog density is not uniform: it is the density at `lighting.fog.base`,
falling off exponentially with altitude over `lighting.fog.height`
metres. The amount along a view ray is the integral of that density
over the ray rather than density times length, solved in closed form,
so a ray that climbs passes through steadily thinner air.

This matters as soon as a project can be seen from above. Plain
distance fog calibrated for the end of a street will swallow an entire
city viewed from a rooftop or an aircraft, because every pixel of
ground is then far away. Setting `lighting.fog.height` very large makes
the air uniform again, which is exactly the distance fog this replaced.

`lighting.fog.base` is world Y, not height above the ground: a project
whose terrain sits well above zero should set it to its own ground
level, or the whole scene reads as far above the dense layer and never
fogs at all.

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
horizon colour is the fog colour — the fogged scene always fades into the
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
volume instead of flat panels. Only *textured* surfaces are sunlit: the
untextured fill/wireframe branch (debug layers, graph lines) keeps its
exact requested colour. The skybox and the GUI pass run with
`lightingOn = 0`, which makes the Gouraud stage a neutral 1.

## Directional shadows

`OkShadowMap` renders the scene's depth once per frame from the
directional light and the world pass compares against it: a fragment
further from the light than what the light could see is in shadow.

### Cascades

The shadow distance is split into bands, each with its own map at the
same resolution. One map cannot serve both ends of a city: cover 200 m
and shadows stop at 200 m; cover 2 km and a texel is a metre across, so
a kerb's shadow becomes a staircase. Split the range and the near band
gets centimetres per texel where it is looked at closely, the far band
metres per texel where nobody can tell.

`shadows.cascades` picks how many (3 by default, 4 maximum), and
`shadows.cascades.blend` how the splits are spaced: 0 spreads them
evenly, 1 spaces them logarithmically. Even spacing wastes the near
cascade on ground that is already close; purely logarithmic makes the
far one enormous. The default sits most of the way towards logarithmic.

Each cascade covers a box fitted to ITS band of the camera's volume,
sized from that band's bounding sphere. A box centred on the viewer
instead spends most of its resolution behind them, where nothing casts
into view, and ends at a fixed radius — which makes shadows sweep in
ahead of a moving camera. The bounding sphere rather than a tight fit
is deliberate: a tight box changes size as the camera turns, and with
it the world size of a texel, so every shadow edge crawls between
texels frame to frame. A sphere's radius does not change under
rotation, so only the centre moves, and that is snapped to the texel
grid.

That snap happens **in light space**, not in world coordinates. The
grid lies in the light's own frame, so rounding the box centre on the
world axes only lands it on a texel when the sun is straight overhead;
any other time the grid slides under the ground as the viewer walks and
every shadow edge travels with them. With a low sun the ground is
grazed and a texel of slide becomes many centimetres of visible edge.

The maps live in one array texture, a layer per cascade, so the world
pass needs a single sampler however many there are. Each fragment picks
the nearest band that reaches it — the finest one available for that
distance — and the last stretch of each band is **faded into the next**
rather than switched at a line.

That fade is not cosmetic. Two cascades do not agree: they have
different texel sizes, so different penumbra, and their grids are
snapped independently. The band is chosen by distance to the *camera*,
so with a third-person camera the player crosses a split without moving
at all — scrolling the wheel pulls the camera back, the ground ahead
gains view depth, and at a hard switch the shadow in front of them
visibly redraws. Spreading the handover over the last quarter of each
band turns that into a gradient nobody notices.

Only the **directional** contribution is shadowed: the ambient floor and the
point lights still reach a shadowed surface, which is what keeps shadows
from becoming black holes. Strength follows the light's elevation and
fades to nothing as it reaches the horizon, where a hard shadow would
look wrong anyway.

## The sun's body

`OkSkybox` also draws the light's visible body: a camera-facing disc
with a solid core inside a soft corona, placed on the dome along the
light's own direction, so what casts the shadows is what is seen in the
sky. It takes the cycle's sun colour, and fades out as it sinks below
the horizon.

## Point lights and halos

`OkLighting` keeps a small registry of point lights (up to 256), in two
flavours:

- `registerLight(x, y, z, r, g, b, radius)` — an **omni** light radiating
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
per fragment with a quadratic falloff inside each light's radius — with
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

Point lights are selected **per pixel**, not per object. `OkLightClusters`
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
| `lighting.fog` | `true` | Fog on/off (the colour keeps driving the sky and clear). |
| `lighting.fog.height` | `25` | Metres of altitude over which fog density falls off by *e*; very large = uniform. |
| `lighting.fog.base` | `0` | World Y at which the curve's density applies; set it to the project's ground level. |
| `lighting.clustered` | `true` | Per-pixel cluster lookup; off falls back to a per-item budget. |
| `lighting.cluster.near` | `1` | Near end of the clustering depth range, world units. |
| `lighting.cluster.far` | `350` | Far end; past it point lights stop contributing. |
| `shadows` | `true` | Directional shadow pass on/off. |
| `shadows.size` | `2048` | Depth map resolution. |
| `shadows.extent` | `90` | Half-width used when fitting is off (`shadows.distance` 0). |
| `shadows.distance` | `260` | How far shadows are worth drawing, split across the cascades. 0 falls back to a fixed `shadows.extent` box on the viewer. |
| `shadows.cascades` | `3` | Bands the distance is split into (max 4). |
| `shadows.cascades.blend` | `0.75` | Split spacing: 0 even, 1 logarithmic. |
| `shadows.cull` | `true` | Cull the shadow pass against the light's volume. |
| `shadows.refresh.turn` | `4e-7` | How far the sun must turn (as 1-cos) before the map is redrawn; 0 redraws every frame. |
| `shadows.strength` | `0.62` | How dark a fully shadowed surface goes. |
| `shadows.bias` | `0.00035` | Depth bias against self-shadowing. |
