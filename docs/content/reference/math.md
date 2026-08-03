---
title: Math
section: Reference
nav_order: 6
---

# Math

The math types wrap GLM behind a small, explicit API. `OkPoint` is a 3D point or vector, `OkRotation` holds Euler angles (and the matrix they produce), and `OkMath` is a static helper for direction/angle conversions and look-at.

## Coordinate system

Okinawa uses a right-handed coordinate system: X points right, Y points up, Z points towards the viewer (out of the screen). The default camera sits at the origin looking down negative Z, with up along positive Y. Rotations are Euler angles in radians: pitch (X), yaw (Y), roll (Z), with pitch clamped to avoid gimbal lock. See `src/okinawa/math/readme.md` in the engine for the full conventions.

## OkPoint methods

| Method | Purpose |
| --- | --- |
| `OkPoint(float x, float y, float z)` | Construct from components. |
| `float x() / y() / z() const` | Component getters. |
| `float magnitude() const` | Vector length. |
| `OkPoint normalize() const` | Unit vector. |
| `float distance(const OkPoint &other) const` | Distance to another point. |
| `float dot(const OkPoint &other) const` | Dot product. |
| `OkPoint cross(const OkPoint &other) const` | Cross product. |
| `static OkPoint Forward() / Right() / Up()` | Basis vectors. |

`OkPoint` also supports `+`, `-`, `*` (scalar), and the compound assignment operators.

## OkRotation methods

| Method | Purpose |
| --- | --- |
| `OkRotation(float pitch, float yaw, float roll)` | Construct from Euler angles (radians). |
| `void setRotation(float x, float y, float z)` | Replace the angles. |
| `void rotate(float dx, float dy, float dz)` | Apply a delta rotation. |
| `OkPoint getForwardVector() const` | Forward direction. |
| `OkPoint getRightVector() const` | Right direction. |
| `OkPoint getUpVector() const` | Up direction. |
| `OkPoint transformPoint(const OkPoint &p) const` | Rotate a point. |

## OkMath methods

| Method | Purpose |
| --- | --- |
| `static void directionVectorToAngles(const OkPoint &dir, float &outPitch, float &outYaw)` | Decompose a direction into pitch/yaw. |
| `static OkRotation lookAt(const OkPoint &eye, const OkPoint &target, const OkPoint &up = OkPoint(0,1,0))` | Build a rotation that looks from eye to target. |

## OkFrustum

The view frustum as six planes, extracted from a combined
`projection * view` matrix (Gribb-Hartmann), used for bounding-sphere
culling. `OkCore` builds one per frame from the current camera and
activates it for the world pass: `OkItem::drawSelf` skips any item whose
bounding sphere (bbox centre + half-diagonal radius, transformed by the
item's matrix) falls fully outside — in the dense city over half the
scene's items are skipped every frame. The GUI and camera-attached passes
run with no active frustum (their calibrated cameras are not the world
camera), and the skybox dome is camera-centred so it always intersects.
`get_state` (MCP) reports the per-frame skipped count as
`scene.frustum_culled`.

| Method | Purpose |
| --- | --- |
| `void setFromMatrix(const glm::mat4 &projView)` | Extract and normalize the six planes. |
| `bool containsSphere(float x, float y, float z, float r) const` | Sphere-vs-frustum test (true = at least partially inside). |
| `static void setActive(const OkFrustum *)` / `static const OkFrustum *getActive()` | The frame's culling frustum (null = no culling). |
| `static void setViewer(x, y, z, maxDistance)` | Viewer position and draw distance for the frame. |
| `static bool isBeyondDrawDistance(x, y, z, r)` | Whether a bounding sphere lies entirely out of range. |
| `static long getCulledCount()` / `static void resetStats()` | Draws skipped since the last reset. |
| `static long getDrawCalls()` / `static long getTriangles()` | What the frame actually submitted. |

### Draw distance

`render.drawdistance` (world units, `0` disables) skips anything whose
bounding sphere lies entirely beyond it. It is a single comparison and
in an open world it rejects far more than the frustum test does, so the
draw path tries it first. Set it where the project's distance fog has
already swallowed the world: past that point the draws change nothing on
screen.

Opaque geometry is also drawn **nearest first** (see `OkScene`), so the
depth buffer rejects hidden fragments early — the cheapest defence
against overdraw in a scene full of occluders. The order is refreshed
periodically rather than every frame, since it only has to be roughly
right.

## Example

```cpp
OkPoint eye(0.0f, 100.0f, 200.0f);
OkPoint target(0.0f, 0.0f, 0.0f);
OkPoint direction = (target - eye).normalize();

float pitch, yaw;
OkMath::directionVectorToAngles(direction, pitch, yaw);
camera->setRotation(pitch, yaw, 0.0f);
```
