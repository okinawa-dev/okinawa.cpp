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
OkCore::addCamera(third);   // key 1
OkCore::addCamera(top);     // key 2

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

## Active avatar

`OkCore::setActiveAvatar(avatar)` / `getActiveAvatar()`. The active avatar is
updated each frame after input. Swapping it (on foot -> car) changes controls
and rig in one call. With **no** active avatar, the current camera still runs
its own behaviour (e.g. a spectator flies). `OkCore::clearCameras()` lets a game
drop the seeded default camera and install its own set.
