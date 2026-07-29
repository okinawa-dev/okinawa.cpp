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
- **Sun colour and direction** (stored for the upcoming directional
  stage): elevation follows a sine over the 6h-21h daylight arc, azimuth
  sweeps east to west, parked below the horizon at night.

`OkLighting::evaluate(hour, ...)` exposes the pure curve for tests and
tools; the interpolated values are read every frame by the render pass
(`getSceneTint`, `getFogColor`, `getFogDensity`, `getSunColor`,
`getSunDirection`).

The GUI pass resets the tint and fog uniforms: the interface is never
tinted or fogged.

## Configuration keys

| Key | Default | Meaning |
| --- | --- | --- |
| `lighting.time` | `12` | Hour of day, 0-24 (wraps). |
| `lighting.timescale` | `30` | Clock speed vs real time; `0` freezes it. |
