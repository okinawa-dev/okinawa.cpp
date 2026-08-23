---
title: Models
section: Reference
nav_order: 4
---

# Loading 3D models

Okinawa loads 3D models from files into an `OkItem` you can add to a scene.
**Wavefront OBJ** is the only format supported today; other formats would need
their own importer.

## OkWavefrontImporter

```cpp
#include "okinawa/importers/wavefront.hpp"

OkItem *model = OkWavefrontImporter::importFile("assets/car.obj");
if (model) {
  scene->addObject(model);
}
```

`importFile` returns a newly allocated `OkItem` (the scene takes ownership when
you add it), or `nullptr` if the file cannot be opened or parsed. The item name
is derived from the file name (without path or extension).

## What the OBJ parser reads

- `v x y z` — vertex positions.
- `vt u v` — texture coordinates.
- `f ...` — faces. Each vertex reference may be `v`, `v/vt`, or `v/vt/vn`;
  the parser uses the **position** and the **texture** index. Faces with more
  than three vertices are triangulated (triangle fan).
- If the file contains any `vt` lines, the model is built with texture
  coordinates (5 floats per vertex: position + UV); otherwise it is built from
  positions only.

Other directives (`vn` normals, `o`, `usemtl`, `mtllib`, comments) are ignored.
In particular the importer does **not** read the companion `.mtl`, so it does
not assign a texture: load and apply textures yourself (see
[Textures](/reference/textures.html)).

## Applying a texture to a loaded model

```cpp
OkItem *model = OkWavefrontImporter::importFile("assets/floor.obj");
if (model) {
  model->loadTextureFromFile("assets/floor.png");  // see Textures
  scene->addObject(model);
}
```

Position, scale and orient it like any other `OkItem` (`setPosition`,
`setScaling`, `setRotation`); render it as a wireframe with
`setWireframe(true)`. See [Items](/reference/items.html).

## Reading a model without building one

`importFile` hands back a finished `OkItem`, which is what an application
usually wants -- and also a GPU object it may not have asked for. A tool that
wants to measure a model, draw it in a panel it renders itself, or pass its
triangles to something else can read the numbers alone:

```cpp
std::vector<float>        vertices;   // three floats per vertex: x, y, z
std::vector<unsigned int> indices;    // one per corner, three to a face
if (OkWavefrontImporter::parseGeometry("assets/table.obj", vertices, indices)) {
  // ...
}
```

Nothing is uploaded and nothing is allocated on the graphics card, so this is
safe to call from a worker thread.

A face corner may be written as `v`, `v/vt`, `v//vn` or `v/vt/vn`; all four are
read, and a polygon of more than three corners is triangulated as a fan.

