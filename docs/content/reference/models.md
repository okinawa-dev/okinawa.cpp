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
- `vn x y z` — vertex normals.
- `f ...` — faces. Each corner may be written `v`, `v/vt`, `v//vn` or
  `v/vt/vn`, and an index may be negative, counting backwards from the end of
  what has been read so far. A corner naming something the file does not hold
  is dropped rather than read past the end. Faces with more than three corners
  are triangulated (triangle fan).
- If the file contains `vn` lines, the model is built with the normals it
  carries (8 floats per vertex: position, UV, normal). With `vt` but no `vn`
  it is built with 5 floats per vertex and **the normals are computed from the
  winding of the triangles**. With neither it is built from positions alone.

That last point is worth pausing on. A mesh with no normals of its own is lit
by the direction its triangles are wound in, which is a second thing an
exporter has to get right and which nothing complains about when it is wrong:
a model wound inward is not invisible, it is lit from the wrong side — the
undersides bright and everything facing the sky dark. Authoring normals is how
a model says what it means instead of implying it.

Other directives (`o`, `usemtl`, `mtllib`, comments) are ignored. In
particular the importer does **not** read the companion `.mtl`, so it does not
assign a texture: load and apply textures yourself (see
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

Two more readers sit beside it, for callers that want the model drawn rather
than measured:

```cpp
// x, y, z, u, v -- the layout OkItem and OkInstancedItem take.
OkWavefrontImporter::parseMesh("assets/table.obj", vertices, indices);

// x, y, z, u, v, nx, ny, nz -- with the normals the file carries. Returns
// false when the file has none, since there is then nothing this adds.
OkWavefrontImporter::parseMeshWithNormals("assets/table.obj", vertices,
                                          indices);
```

