---
title: Previews
section: Reference
nav_order: 17
---

# Offscreen targets and object previews

A frame is the world seen from the player's camera: lit by the hour of
the day, fogged at distance, cut down by a frustum, and painted on the
window. Sometimes what is wanted is not a frame but a **picture of one
object** — an item turning in an inventory, a thumbnail in a browser, a
tool showing a single piece. Two classes cover that: `OkRenderTarget`,
somewhere other than the window to draw into, and `OkPreview`, a pass
that draws a handful of objects with a camera of your choosing.

The reason they exist together is worth saying plainly. The obvious way
to draw a small picture of an object is a small shader of your own —
position in, colour out, forty lines. It works, and it is a **second
answer** to what an object looks like. The two agree on the day they are
written and drift afterwards, silently, because both pictures look
plausible. `OkPreview` puts the object through the engine's own shader,
so what the preview shows is what the world shows: the same textures,
the same masked materials, the same tints.

## A target to draw into

```cpp
#include "okinawa/render/render_target.hpp"

OkRenderTarget target;
target.resize(320, 240);       // allocates; same size again is free
```

`resize` may be called every frame — a target whose surface follows a
resizable panel does exactly that, and a call that does not change the
size returns immediately. It returns `false` when the surface could not
be completed, and the target is then inert rather than half made.

`bind()` makes the target current and sets the viewport to its size;
`unbind()` puts back **the framebuffer and viewport that were in force**,
read from the driver rather than assumed to be the window. That is what
lets a target be bound from inside another pass without the rest of the
frame ending up somewhere invisible.

`getTexture()` is the colour attachment, for whoever draws the result.

## A pass that draws objects

```cpp
#include "okinawa/render/preview.hpp"

float centre[3] = {0.0f, 0.0f, 0.0f};      // what to look at
float radius    = 1.8f;                     // how big it is

float distance = OkPreview::frameDistance(radius, 35.0f, aspect, 0.15f);
float view[16];
float proj[16];
OkPreview::orbit(centre, yawDeg, pitchDeg, distance, aspect, 35.0f,
                 distance * 0.01f, distance * 8.0f, view, proj);

OkObject         *objects[] = {item};
OkPreview::Settings settings;               // fixed light, dark ground
OkPreview::render(target, view, proj, objects, 1, settings);
```

Then draw `target.getTexture()` wherever the picture belongs.

### What the pass switches off, and why

- **Fog, the day tint, the shadow map and the point lights.** All four
  say where an object stands in a world, and a preview is the object out
  of one.
- **The light is fixed by the caller**, not taken from the day cycle. A
  view whose shading changed with the clock would report the object
  differently at nine and at midnight, and the point of an inspection
  view is that it reports the object.
- **Nothing is culled.** A frustum and a draw distance describe a frame;
  an object somebody asked to look at is never the wrong answer to what
  should be drawn.
- **Back faces are drawn.** Culling buys speed on a whole world and
  nothing on one object, while costing a hole in anything wound the
  other way round — which reads as a broken object rather than as an
  optimisation.

Everything the pass borrows — the bound framebuffer, the viewport, the
program, the depth and blend state, the active frustum — is put back
before it returns.

### Framing is arithmetic, not a guess

`frameDistance` is the one place that answers "how far back does the eye
have to stand". A standoff written as a multiple of the radius is a
guess about the lens: with a vertical field of view of 35 degrees, two
and a half radii **crops** a sphere that three and a half frames with
room to spare, and neither number records which lens it assumed. Pass
the field of view and the aspect ratio and the arithmetic is the
definition of fitting — including the case of a surface taller than it
is wide, where the horizontal half-angle is the tight one.

`orbitEye` gives the eye position on its own, for a caller that needs
the point as well as the matrices. Pitch is clamped short of vertical,
where the up vector and the view direction become the same line and the
picture flips as the last degree is crossed.

Both are pure arithmetic and are unit-tested without a graphics context,
because they are the half of a preview that decides whether the object
is in the picture at all — and an empty rectangle looks exactly like a
preview that failed to load.
