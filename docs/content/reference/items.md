---
title: Items
section: Reference
nav_order: 3
---

# Items

`OkItem` is a renderable mesh: vertex data, indices, an optional texture and the shared transform (position, rotation, scale, hierarchy). It derives from `OkObject`, which provides the transform API used across the engine. `OkItemGroup` bundles several items so they move and render as one unit and can be tagged for selective visibility. `OkTexture` wraps a loaded GPU texture.

## OkObject

Every drawable inherits these:

| Method | Purpose |
| --- | --- |
| `void setPosition(float x, float y, float z)` | Set the local position. |
| `void move(float dx, float dy, float dz)` | Translate by a delta. |
| `void setRotation(float x, float y, float z)` | Set Euler rotation (radians). |
| `void rotate(float dx, float dy, float dz)` | Rotate by a delta (radians). |
| `void setScaling(float x, float y, float z)` | Set the scale. |
| `void attachTo(OkObject *parent)` | Parent this object (transforms compose). |
| `void attach(OkObject *child)` | Add a child to this object. |
| `void detachFromParent()` | Detach from the parent. |
| `void setDrawOriginAxis(bool)` | Toggle the debug origin axis gizmo. |

## OkItem

| Method | Purpose |
| --- | --- |
| `OkItem(name, vertexData, vertexCount, indexData, indexCount, stride = 5)` | Construct from interleaved vertex data and indices. Stride 5 (`x,y,z,u,v`) computes vertex normals from the triangle list — de-indexed meshes get exact flat face normals, indexed meshes get smoothed ones; stride 8 (`x,y,z,u,v,nx,ny,nz`) takes caller normals verbatim. Internally vertices are always stored with stride 8. |
| `void setWireframe(bool)` | Draw as wireframe. |
| `void setVisible(bool)` | Show or hide the item. |
| `void setDrawMode(GLenum mode)` | Set the GL primitive (`GL_TRIANGLES`, `GL_LINES`, ...). |
| `void loadTextureFromFile(const std::string &path)` | Load and apply a texture from disk. |
| `void setTexture(const std::string &name, OkTexture *tex)` | Apply an already-loaded texture. |
| `void setFillColor(float r, float g, float b, float a = 1)` | Untextured fill colour; the alpha is honoured by blended passes (the GUI). |
| `void setTintColor(float r, float g, float b, float a)` | Multiplied over the texture in the fill pass (white = untouched); how GUI text is coloured. |
| `void updateVertexData(float *data, long count)` | Replace the vertex data in place (stride-5 contract; normals recomputed against the item's indices). |
| `float getRadius() const` | The mesh's maximum dimension. |

## OkInstancedItem

One mesh drawn many times in a SINGLE draw call: the base `OkItem` holds
the shared mesh (uploaded once), and this subclass adds a per-instance
buffer of world transforms (position, uniform scale, Y rotation) wired
with an attribute divisor. A thousand copies cost one draw call instead
of a thousand items, which is what makes large numbers of repeated
objects affordable.

Instances are addressable, not anonymous triangles — each one can be
moved, hidden or removed at runtime, and the buffer is recomposed
cheaply. The mesh is authored around its own origin
and each instance places a copy of it. Instances are frustum-culled
INDIVIDUALLY (the mesh bounding sphere at each instance position), so
only what is on screen is uploaded and drawn.

The logical side of an object — state, collision, its light — belongs to
the game, never to this class: collision never comes from render
triangles.

| Method | Purpose |
| --- | --- |
| `OkInstancedItem(name, vertexData, vertexCount, indexData, indexCount, stride = 5)` | Same mesh contract as `OkItem`. |
| `int addInstance(x, y, z, yaw = 0, scale = 1)` | Place a copy; returns its index. |
| `void setInstance(index, x, y, z, yaw, scale)` | Move an existing instance. |
| `void setInstanceVisible(index, bool)` | Hide/show one instance. |
| `void clearInstances()` | Drop every instance. |
| `int getInstanceCount() const` / `int getDrawnCount() const` | Total instances, and how many survived frustum culling last frame. |

## OkItemGroup

| Method | Purpose |
| --- | --- |
| `OkItemGroup(name)` | Construct an empty group. |
| `void addItem(OkItem *item, const std::string &tag)` | Add an item with a tag. |
| `std::vector<OkItem *> getItemsWithTag(const std::string &tag)` | Items carrying a tag. |
| `int getItemCountWithTag(const std::string &tag)` | Count items with a tag. |
| `void setVisible(bool)` | Show or hide every item. |
| `void setWireframe(bool)` | Wireframe every item. |

## Example

```cpp
float vertices[] = {
  // position           // uv
   0.5f,  0.5f, 0.0f,    1.0f, 1.0f,
   0.5f, -0.5f, 0.0f,    1.0f, 0.0f,
  -0.5f, -0.5f, 0.0f,    0.0f, 0.0f,
  -0.5f,  0.5f, 0.0f,    0.0f, 1.0f,
};
unsigned int indices[] = { 0, 1, 3, 1, 2, 3 };

OkItem *quad = new OkItem("quad", vertices, 20, indices, 6);
quad->setPosition(0.0f, 0.0f, -5.0f);
quad->setWireframe(true);
scene->addObject(quad);
```

## OkBillboard

`OkBillboard` is an `OkItem` subclass: a rectangular quad that always
renders facing the active camera (a classic billboard). The quad is
centred on the item's position, `width` along X and `height` along Y;
each frame the item aligns itself with the view plane of
`OkCore::getCamera()` (it adopts the camera's pitch and yaw, roll stays
0), so its content stays screen-aligned at every angle -- including
straight under a top-down camera, where position-based facing
degenerates into an arbitrary roll. Texturing, visibility and every
other `OkItem` method work unchanged. Billboards are expected to live
at scene root (not attached under a transformed parent).

| Method | Purpose |
| --- | --- |
| `OkBillboard(name, width, height)` | Construct a `width` x `height` camera-facing quad. |
| `void setCameraOffset(float metres)` | Depth bias: draw the quad `metres` closer to the camera along the view ray. |
| `void setProximityFade(float nearDist)` | Fade the tint alpha out as the camera approaches (0 disables). |
| `static OkRotation facingRotation(from, to)` | The rotation that points a +Z quad at `from` toward `to`. |

### Camera offset (depth bias)

A billboard placed AT a solid object intersects it: the quad is flat,
the object is not, and the sprite gets sliced along a hard straight
edge wherever the geometry pokes through. Typical cases are a glow
sprite centred on the thing that emits it, or a marker pinned to a
character's head.

`setCameraOffset(metres)` draws the quad a short distance TOWARD THE
CAMERA along the view ray, recomputed every frame, so it wins the depth
test against the object it belongs to.

Because the displacement follows the view ray, a point moved along it
projects to the SAME pixel: the quad does not shift on screen at all,
only its depth changes. Orbiting the object keeps the sprite exactly
where it was from every angle.

`setPosition()` keeps meaning "the anchor": the offset is applied on top
of it each frame and never accumulates, and moving the billboard from
outside re-anchors it automatically.

Its limit: it only resolves intersections with geometry nearer to the
anchor than the offset. For sprites that must blend against arbitrary
geometry, a depth bias is not enough: those cases need the sprite to
fade by its depth difference with the scene.

```cpp
OkBillboard *label = new OkBillboard("label", 8.0f, 4.0f);
label->setPosition(10.0f, 30.0f, -20.0f);
label->setTexture("label-tex", texture);   // any OkTexture
scene->addObject(label);
```
