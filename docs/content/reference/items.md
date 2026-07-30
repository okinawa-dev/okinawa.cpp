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
| `static OkRotation facingRotation(from, to)` | The rotation that points a +Z quad at `from` toward `to`. |

```cpp
OkBillboard *label = new OkBillboard("label", 8.0f, 4.0f);
label->setPosition(10.0f, 30.0f, -20.0f);
label->setTexture("label-tex", texture);   // any OkTexture
scene->addObject(label);
```
