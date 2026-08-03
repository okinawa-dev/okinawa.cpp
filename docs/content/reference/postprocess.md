---
title: Post-processing
section: Reference
nav_order: 15
---

# Post-processing

`OkPostProcess` is the engine's post-process chain. With `render.post`
enabled, the whole world pass renders into an offscreen framebuffer
(a colour texture plus a depth texture) instead of the window, and a
final full-screen pass composites it to the window applying the enabled
effects. With `render.post` off the chain vanishes: the engine renders
directly to the window exactly as before, at zero cost.

The GUI and camera-attached passes draw *after* the composite, directly to
the window — the interface stays sharp and grain-free. (The design
leaves room for a later per-stage GUI chain with its own effect
intensities, e.g. a HUD that motion-blurs less than the world.)

## Effects

All in one composite shader, each gated by its config toggle:

- **Depth of field** (`post.dof`) — diorama-style: fragments inside the
  focus band (the focus distance ± `post.dof.range`, metres) stay sharp;
  blur grows with distance from the band up to `post.dof.maxblur`
  pixels (8-tap ring blur over the linearized depth buffer).
- **Film grain** (`post.grain`) — animated per-pixel hash noise,
  `post.grain.strength` in colour units. Also hides procedural
  repetition and banding.
- **Bloom** (`post.bloom`) — the frame's bright areas are extracted
  above `post.bloom.threshold` (with a soft shoulder set by
  `post.bloom.knee`), blurred separably at half resolution and added
  back with `post.bloom.strength`. Because it works on the finished
  frame rather than on the objects, the cost is the same whether one
  surface glows or ten thousand do, which is what makes it the
  affordable way to sell emissive surfaces as sources of light. A
  surface that should glow needs to be driven *past* white, so it clears
  the threshold.
- **Directional motion blur** (`post.motionblur`) — screen-space smear
  along a game-supplied velocity vector:
  `OkPostProcess::setMotionVector(dx, dy, strength)` each frame (e.g.
  from vehicle speed); strength 0 disables it. Samples 7 taps along the
  vector.

## Configuration keys

| Key | Default | Meaning |
| --- | --- | --- |
| `render.post` | `true` | Master switch: offscreen chain vs direct rendering. |
| `post.dof` | `true` | Depth of field on/off. |
| `post.dof.focus` | `30` | Focus distance, metres. |
| `post.dof.range` | `200` | Fully sharp half-band around the focus, metres. |
| `post.dof.maxblur` | `2` | Maximum blur radius, pixels. |
| `post.dof.falloff` | `600` | Metres past the sharp band to reach the maximum blur. |
| `post.grain` | `true` | Film grain on/off. |
| `post.grain.strength` | `0.035` | Grain amplitude. |
| `post.motionblur` | `true` | Directional blur (needs a motion vector). |
| `post.bloom` | `true` | Bloom on/off. |
| `post.bloom.threshold` | `0.85` | Luminance where glowing starts. |
| `post.bloom.knee` | `0.30` | Soft shoulder around the threshold. |
| `post.bloom.strength` | `1.00` | How much glow is added back. |

### Autofocus

With `post.dof.autofocus` on (the default), the focus distance follows
whatever is under the middle of the screen instead of sitting at
`post.dof.focus`. A fixed distance only suits a camera that stays a
fixed distance from its subject; as soon as the viewpoint can climb,
everything is far away and the whole frame falls out of focus.

The depth of a single pixel is read back through a pixel buffer object
and collected on the *next* frame, so the CPU never waits on the GPU —
one frame of lag is invisible, a stall is not. The result is eased
rather than applied outright, because a hard cut as the camera sweeps
past a near wall reads as a glitch while a real lens takes a moment to
find its subject. Sky (nothing at that pixel) leaves the focus where it
was.

| Key | Default | Meaning |
| --- | --- | --- |
| `post.dof.autofocus` | `true` | Follow the centre of the screen; off uses `post.dof.focus`. |
| `post.dof.autofocus.max` | `900` | Cap on the focus distance, metres. |
| `post.dof.autofocus.ease` | `0.06` | Fraction of the remaining distance closed per frame. |

### Choosing the numbers

Two defaults are set wide on purpose, and both for the same reason: a
viewpoint that can climb.

The sharp band and falloff are generous because the focus distance is
fixed. A tight band suits a camera a few metres from its subject, but
once the viewpoint rises, everything on screen is hundreds of metres
away and a tight band blurs the whole frame.

The bloom threshold is high enough that only actual light sources
glow — lit windows, lamps, a sun disc. A daytime sky is bright over
most of its area, so a low threshold sends the entire sky through the
blur and washes the top half of the screen to white. At night nothing
but the lights passes either threshold, so raising it costs nothing
there.

Everything is console-reachable (`set post.dof.focus 25`, `set
render.post false`).

## Integration

`OkCore` calls `begin(fbWidth, fbHeight)` before clearing the frame
(binds the offscreen target, recreating it lazily on resize) and
`end(near, far, dt)` after the scene — the near/far planes of the
current camera linearize the depth buffer for the DoF. The offscreen
target follows the real framebuffer size, so HiDPI displays keep their
native resolution. The composite draws a bufferless full-screen triangle
(`gl_VertexID` trick), so the pass costs one draw call.
