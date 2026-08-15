---
title: Avatars
section: Reference
nav_order: 8
---

# Avatars

An **avatar** is the controllable representation of the player: a controlled
object plus the input scheme (controller) that drives it, plus a **rig of
cameras** that observe it. The avatar is the controlled entity; cameras are
views of it.

**Render and control are independent.** Switching the rendered camera (the
number keys) never changes the controls: the controller carries its own
reference frame, so you can watch from a debug top-down while the avatar keeps
moving exactly as in third person.

## OkAvatar

```cpp
#include "okinawa/avatar/avatar.hpp"
#include "okinawa/avatar/controllers/ground_controller.hpp"
#include "okinawa/cameras/third_person_camera.hpp"
#include "okinawa/cameras/top_down_camera.hpp"

OkGroundController *controller = new OkGroundController(8.0f);
OkAvatar           *player     = new OkAvatar(prism, controller);

OkThirdPersonCamera *third = new OkThirdPersonCamera("third", w, h);
OkTopDownCamera     *top   = new OkTopDownCamera("top", w, h, 400.0f);
OkCore::addCamera(third);   // "third" -- names identify cameras
OkCore::addCamera(top);     // "top"    (number keys switch them, as a debug aid)

controller->setReferenceCamera(third);  // control is relative to this camera
player->addCamera(third);               // rig: cameras follow the avatar
player->addCamera(top);
OkCore::setActiveAvatar(player);
```

`OkAvatar` owns its controller (deletes it) but **not** the controlled object
(the scene owns it) nor the rig cameras (`OkCore` owns them). It updates the
controller and repositions every rig camera each frame, so non-rendered cameras
still track the avatar (their gizmos show).

## Controllers

`OkAvatarController::update(dt, input, controlled)` — no camera is passed in; the
controller obtains its own reference frame. Input stays **polled** per frame
(see [Input](/reference/input.html)); discrete actions are edge-triggered.

**`OkGroundController`** — stock controller: movement on the ground plane (XZ)
relative to a reference frame, turning the object to face its movement (character,
vehicle, ...). The frame is selectable, which is what keeps control independent
of the rendered camera:

- `setReferenceCamera(cam)` — relative to that camera (the usual gameplay camera).
- `setUseActiveCamera(true)` — relative to the active rendered camera
  (room-relative control, fixed-camera games).
- neither — relative to the controlled object's own facing.

It also applies a **vertical nudge** while the `moveUp` / `moveDown` state keys
(E / Q) are held: straight up/down at the walk speed, for altitude fix-ups
(e.g. climbing back above the terrain after a bad teleport).

**`OkPanController`** — mouse-pan controller for overhead views: the mouse alone
moves the controlled object on the ground plane (no keys), reading the per-frame
`panX`/`panY` pixel deltas from the input state (fed only while the cursor is
captured). The pan speed scales with the active camera's `viewDistance()`, so
the further/higher the camera, the faster the object crosses the map; the wheel
keeps zooming the camera. Swap it in when a top-down camera is rendered
(`avatar->setController(new OkPanController())`) and swap the ground controller
back for gameplay views.

## Cameras

Camera behaviours are `OkCamera` subclasses. `OkCamera` has three virtuals:
`updateForTarget(target, dt)` (reposition for what it observes; base does
nothing), `look(yawDeg, pitchDeg)` (base: free-fly rotate, pitch clamped) and
`zoom(delta)` (mouse-wheel notches; base ignores it). The wheel is routed to the
current camera via `OkCore::applyZoom`. Agents set the camera absolutely (incl.
distance) with the single MCP `view` tool instead. `viewDistance()` reports how
far the camera sits from what it observes (orbit distance, overhead height; 0
when it does not apply) so consumers can scale interactions with the visible
area — the pan controller uses it.

- **`OkThirdPersonCamera`** — orbits behind/above the target and looks at it;
  the mouse/look orbits it (pitch clamped), the wheel changes the orbit distance.
- **`OkTopDownCamera`** — stays straight above the target, perpendicular, north
  (+Z) up; follows it; ignores the mouse, the wheel changes its height. Debug/map.
- **`OkFixedCamera`** — static "Resident Evil" camera: fixed world position,
  optionally re-aims at the target. Combine with `setUseActiveCamera` for
  room-relative control.
- **`OkSpectatorCamera`** — free-fly: ignores the target and flies from the
  input state; mouse rotates it. Use with no active avatar for a debug
  fly-through.
- **`OkInspectionCamera`** — for looking *at* a scene rather than being inside
  one: the wheel comes closer, a drag slides sideways, a drag swings the view
  round a point, and every gesture is scaled by the camera's height above the
  scene. See below.

### OkInspectionCamera

A tool that shows a large scene is used at two ranges — the whole thing on
screen, and one detail of it — and a gesture worth a fixed number of units is
unusable at both. The wheel crawls at the first and throws the camera through a
wall at the second. Scaling every gesture by how high the camera sits above the
scene makes one wheel notch mean "a bit closer" at every range, which is what a
viewer for solid models does and what a person expects.

| Method | Purpose |
| --- | --- |
| `void setGroundHeight(float y)` | The height the scene's ground sits at. Gestures are measured from it, and it is a fact about the scene, so the application sets it. |
| `void setScaleRange(float min, float max)` | Clamps on that height. Without the lower one every gesture dies as the camera nears the ground; without the upper one a camera pulled far out crosses the scene in a notch. |
| `void setZoomPerNotch(float)` / `void setPanPerPixel(float)` | Units per wheel notch and per dragged pixel, at a scale of 1. |
| `float gestureScale() const` | What a gesture is worth right now. |
| `void zoomAlongView(float notches)` | Move along the view direction. |
| `void zoomToward(const OkPoint &target, float notches)` | Move towards a point, stopping short of it (`MIN_REACH`). |
| `void panAcrossView(float dxPixels, float dyPixels)` | Slide across the view. |
| `bool orbitAbout(const OkPoint &pivot, float yawDeg, float pitchDeg)` | Swing round a point, rigidly (see below); `false` when refused for coming too close or too near level. |
| `float viewDistance() const` | Height above the scene's ground, so the rest of the engine scales with it. |

It holds the arithmetic and none of the policy. Which button does what, whether
the pointer is captured, and where a pivot comes from stay with the caller —
and the two arguments it most wants are the two only the caller can work out:
the point under the cursor, which needs to know what the scene contains, and
the height of its ground.

`orbitAbout` is a **rigid** turn about the pivot: the camera's position
swings round it and its facing turns by the same angle, so nothing in the
picture slides and the pivot keeps the pixel it had. That is the property
worth testing — project the pivot, orbit, project again, same pixel — and
the two ways to lose it are turning position and facing by opposite signs,
and tilting about the right axis as it was before the yaw rather than
after.

`zoomToward` and `orbitAbout` take that point rather than finding it, which is
what makes them worth having: aiming the wheel at what the cursor is over keeps
that thing under the pointer as the camera closes in, and orbiting about it
keeps it in frame while the view swings. Orbiting about the camera's own
position instead slides the subject out of view — right for a first-person
game, wrong for a tool. Use [`OkCamera::rayThroughPixel`](/reference/core.html)
and [`OkItem::intersectRay`](/reference/items.html) to find the point.

```cpp
OkInspectionCamera *camera = new OkInspectionCamera("tool", width, height);
camera->setGroundHeight(sceneGroundY);
camera->setScaleRange(5.0f, 900.0f);

// ... per frame, from whatever the application decides a gesture is:
OkPoint under;
if (pointUnderCursor(&under)) {
  camera->zoomToward(under, wheelNotches);
} else {
  camera->zoomAlongView(wheelNotches);
}
```

## Active avatar

`OkCore::setActiveAvatar(avatar)` / `getActiveAvatar()`. The active avatar is
updated each frame after input. Swapping it (on foot -> car) changes controls
and rig in one call. With **no** active avatar, the current camera still runs
its own behaviour (e.g. a spectator flies). `OkCore::clearCameras()` lets a game
drop the seeded default camera and install its own set.
