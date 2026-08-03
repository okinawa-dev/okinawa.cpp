---
title: Sky, sun and a day cycle
section: Examples
nav_order: 4
---

# Sky, sun and a day cycle

This tutorial puts a sky over a scene: a gradient dome, a sun that
travels across it, shadows cast from that same sun, and a clock that
moves the whole thing through the day. Everything comes from one place —
the atmosphere curve — so the sky, the fog, the light and the shadows
always agree with each other.

## The pieces

- `OkLighting` keeps a clock and an atmosphere curve, and every frame
  interpolates the values that describe the sky at that hour: a scene
  tint, fog colour and density, the directional light's colour and
  direction, the sky's zenith colour, and an ambient floor.
- `OkSkybox` paints the dome from those values and draws the sun's
  visible body along the light's own direction.
- `OkShadowMap` renders the scene's depth from that same direction so
  the world pass can shadow it.

You do not wire these together: the core draws the sky and the shadow
map as part of the frame. What a project supplies is **the curve**.

## Writing a curve

The engine ships a neutral curve so a new project renders sensibly from
the first run. Replacing it is how a game gets its own look — a hot
desert noon, a permanently overcast planet, a horror night that never
quite ends.

Each key says what the world looks like at one hour, and the engine
blends between consecutive keys, wrapping around midnight:

```cpp
#include "okinawa/lighting/lighting.hpp"

static const OkAtmosphereKey DESERT[] = {
  //  hour  tint                  fog                   density
  //        sun                   zenith                ambient
  {  0.0f, {0.26f,0.30f,0.42f}, {0.05f,0.07f,0.12f}, 0.0022f,
           {0.0f, 0.0f, 0.0f},  {0.02f,0.03f,0.07f}, 0.26f },
  {  6.0f, {0.95f,0.80f,0.66f}, {0.80f,0.62f,0.48f}, 0.0030f,
           {1.0f, 0.72f,0.42f}, {0.35f,0.40f,0.58f}, 0.45f },
  { 12.0f, {1.00f,0.99f,0.94f}, {0.86f,0.84f,0.74f}, 0.0008f,
           {1.0f, 0.97f,0.88f}, {0.22f,0.46f,0.86f}, 0.60f },
  { 19.0f, {1.00f,0.74f,0.50f}, {0.82f,0.54f,0.36f}, 0.0026f,
           {1.0f, 0.52f,0.24f}, {0.30f,0.26f,0.40f}, 0.50f },
  { 23.0f, {0.26f,0.30f,0.42f}, {0.05f,0.07f,0.12f}, 0.0022f,
           {0.0f, 0.0f, 0.0f},  {0.02f,0.03f,0.07f}, 0.26f },
};

int main() {
  // ... after OkCore::initialize()
  OkLighting::setAtmosphereCurve(DESERT, 5);
}
```

A few things worth knowing while choosing numbers:

- **The fog colour is also the sky at the horizon.** That is what makes
  distance dissolve into the sky instead of ending at a visible line, so
  pick it as a sky colour, not as a grey.
- **The sun colour going black is how night is expressed.** With no
  directional light, only the ambient floor and whatever point lights
  the game registers remain.
- **The ambient floor decides how dark night really gets.** A high floor
  keeps everything visible but flattens the scene and drowns any lamps;
  a low floor makes artificial light matter. If a project has no lights
  of its own, keep it high.

## Running the clock

The hour lives in the config, so it is scriptable and reachable from the
console:

```cpp
OkConfig::setFloat("lighting.time", 18.5f);      // half past six
OkConfig::setFloat("lighting.timescale", 60.0f); // a day in 24 minutes
```

`timescale` is how much faster than real time the clock runs; `0` freezes
it, which is what you want while comparing two versions of a shot.

## Shadows

Shadows follow the same light, so they need no setup beyond deciding how
much ground they cover:

```cpp
OkConfig::setFloat("shadows.extent", 90.0f);   // half-width, world units
OkConfig::setFloat("shadows.strength", 0.62f);
```

The map follows the viewer, so `extent` is a trade: a small area gives
crisp shadows near the player and none in the distance, a large one
spreads the same resolution thinner. Shadows fade out on their own as
the light approaches the horizon, where long hard shadows stop being
believable.

## Checking your curve

The quickest way to judge a curve is to freeze the clock and step
through the hours, looking at the same spot each time:

```
timescale 0
time 7
time 12
time 19
time 22
```

Look at a surface facing the light and one facing away from it in each
shot. If they are the same colour, the curve has no direction; if the
shadowed one goes black, the ambient floor is too low.
