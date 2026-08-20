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
| `void setWireframe(bool)` | Draw this item as wireframe. |
| `void setWireframeGlobal(bool)` | Whether the scene-wide `graphics.wireframe` switch reaches this item (default yes; see below). |
| `void setCastsShadow(bool)` | Whether this item is drawn into the shadow maps (default yes). Turn it off for things that are LIGHT rather than matter — a lamp's corona, an emissive pane, the sky — which have geometry but would otherwise be recorded as occluders, so that a glow casts a shadow of its own quad. |
| `void setVisible(bool)` | Show or hide the item. |
| `void setFade(float)` | Cross-fade amount, 1 solid and 0 gone (see below). |
| `void setFadeInverted(bool)` | Use the opposite half of the dither pattern. |
| `void setDrawMode(GLenum mode)` | Set the GL primitive (`GL_TRIANGLES`, `GL_LINES`, ...). |
| `void loadTextureFromFile(const std::string &path)` | Load and apply a texture from disk. |
| `void addMaterialFromFile(long firstIndex, long indexCount, const std::string &path)` | Give a range of the index buffer its own texture (see below). |
| `OkItem(name)` | An item with no geometry yet, to be filled with `addMesh` (see below). |
| `void addMesh(vertexData, vertexCount, indexData, indexCount, texturePath, stride = 5)` | Append a piece of mesh wearing one texture. The piece's indices count from its own first vertex and are moved along by what the item already holds. |
| `void upload()` | Hand the assembled mesh to the GPU. Call once, when every piece has been added. |
| `void clearMaterials()` | Drop every material slot and its texture reference. |
| `size_t getMaterialCount() const` | How many material slots the item has (0 = single-material). |
| `void setTexture(const std::string &name, OkTexture *tex)` | Apply an already-loaded texture. The item takes its own reference (see below). |
| `void setFillColor(float r, float g, float b, float a = 1)` | Untextured fill colour; the alpha is honoured by blended passes (the GUI). |
| `void setTintColor(float r, float g, float b, float a)` | Multiplied over the texture in the fill pass (white = untouched); how GUI text is coloured. |
| `void updateVertexData(float *data, long count)` | Replace the vertex data in place (stride-5 contract; normals recomputed against the item's indices). |
| `float getRadius() const` | The mesh's maximum dimension. |
| `bool intersectRay(const OkRay &ray, float *outDistance) const` | Whether a ray crosses this item's own triangles, and how far along (see below). |

### Asking an item whether a ray hits it

`intersectRay` takes a world-space ray — from
[`OkCamera::rayThroughPixel`](/reference/core.html), or built by hand for
a line-of-sight check — and answers against the item's **own triangles**,
not against a box around it.

The item is the only one that can answer it. It owns its vertex and index
buffers and its place in the world and hands neither back, so anything
outside would have to keep a second copy of the mesh to ask — which for a
large scene is tens of megabytes, and a copy that can fall out of step
with what is actually drawn.

Two tests, in the order that makes the second cheap: the bounding sphere
the frustum already culls with, and only then the triangles. Both run in
the item's local space, reached by transforming the ray rather than the
mesh; [OkRay](/reference/math.html#okray) explains why the distance still
comes back in world units.

It answers about geometry and nothing else. Whether the item is visible,
whether it is the sort of thing this application lets a user choose, and
which of several hits wins are the caller's decisions — an engine that
made them here would be making them for every application at once. So a
selection loop is the caller's, and it is short:

```cpp
OkRay  ray     = camera->rayThroughPixel(mouseX, mouseY, width, height);
OkItem *chosen = nullptr;
float  nearest = 0.0f;

for (size_t i = 0; i < candidates.size(); i++) {
  float distance = 0.0f;
  if (!candidates[i]->getVisible()) {
    continue;                       // this application's rule, not the engine's
  }
  if (!candidates[i]->intersectRay(ray, &distance)) {
    continue;
  }
  if (chosen == nullptr || distance < nearest) {
    chosen  = candidates[i];
    nearest = distance;
  }
}
```

Items drawn as lines or points are never hit: there is no surface to
cross, and reading such an index list in threes would invent triangles out
of unrelated vertices. An `OkInstancedItem` is answered for its base copy
only.

### Wireframe

An item draws as wireframe when its own `setWireframe(true)` is set, or when
the `graphics.wireframe` config key is on. The key is a scene-wide switch — a
way of reading how the world is triangulated, which no amount of looking at
the shaded result can tell you — and it reaches everything drawn afterwards,
including meshes that are created later.

Items that are *interface* rather than *scene* opt out with
`setWireframeGlobal(false)`. The GUI classes already do: a wireframed font
atlas is a screenful of empty boxes, and the console that turns the switch back
off would be the first thing to become unreadable. Their own `setWireframe`
still works either way.

`OkInstancedItem` honours both, drawing its overlay as a second instanced pass,
so objects drawn in bulk show their triangles like everything else.

### Cross-fading between two versions

`setFade` drops a share of an item's pixels on an ordered 4x4 pattern:
1 draws it whole, 0 drops it entirely, and values between dissolve it.
The point is swapping one version of an object for another — a detailed
build for a cheap stand-in — without a frame where everything changes
at once.

A dither rather than transparency, because a dithered item stays in the
opaque pass: no blending, no sorting, and the depth buffer keeps
working. An alpha fade of anything with depth to it (a building, a
whole block) shows the object fighting with itself.

The two sides of a handover must drop **opposite** pixels, so call
`setFadeInverted(true)` on exactly one of the pair. With the same
pattern on both, each keeps the same half of the pixels and the other
half shows the background straight through.

```cpp
detailed->setFade(t);                  // 0 -> 1 as it takes over
standIn->setFade(1.0f - t);
standIn->setFadeInverted(true);        // opposite half of the pattern
```

### Material slots

A mesh often wants more than one material: a crate with a metal lid, a
wall and its roof, a vehicle body and its glass. Splitting it into
separate items to get them is the wrong trade — one mesh becomes two
objects, with two transforms, two bounding spheres and two culling
tests, and the seam between them has to be kept aligned by hand.

Instead the index buffer is carved into **ranges**, each with its own
texture, drawn back to back from the same vertex and index buffers. This
is the usual arrangement elsewhere too: submeshes, material slots, glTF
primitives.

```cpp
// faces are laid out [sides ... | lid ...] in the index buffer
OkItem *crate = new OkItem("crate", verts.data(), (long)verts.size(),
                           idx.data(), (long)idx.size());
crate->addMaterialFromFile(0, sidesCount, "assets/wood.png");
crate->addMaterialFromFile(sidesCount, lidCount, "assets/metal.png");
```

Rules:

- Ranges are drawn in the order added, and should cover the index buffer
  without overlapping. Nothing enforces it: a gap simply never draws, an
  overlap draws twice.
- Adding **no** slots leaves the item single-material, drawing its whole
  index buffer with `texture`. That is the common case and costs nothing.
- Each slot holds its own texture reference, released with the item, on
  the same terms as [texture ownership](#texture-ownership) below.
- A slot with an empty path draws its range in the item's fill colour.
- Everything that is *per item* — the transform, the frustum test, the
  fade, the lighting uniforms — is done once for the whole mesh,
  whatever the slot count. Only the texture bind and the draw call
  repeat.

### Assembling one from pieces

The example above knows where each material's faces sit in the index
buffer, because it built the buffer itself. When the pieces arrive
separately -- a mesh per material, each indexed from its own first
vertex -- use `addMesh`, which concatenates the geometry and moves each
piece's indices along by what is already there:

```cpp
OkItem *crate = new OkItem("crate");          // empty, no geometry yet
crate->addMesh(sidesV.data(), (long)sidesV.size(),
               sidesI.data(), (long)sidesI.size(), "assets/wood.png");
crate->addMesh(lidV.data(), (long)lidV.size(),
               lidI.data(), (long)lidI.size(), "assets/metal.png");
crate->upload();                              // hands it to the GPU, once
```

Each piece becomes one range, so this is the same item as before, built
the other way round. Nothing is drawn until `upload()`.

It exists because that index arithmetic is easy to get wrong and does
not announce itself: an index that is off but still inside the buffer
draws the wrong triangles rather than failing. Callers that merged
meshes by hand each wrote it out again.

Because a vertex carries one set of texture coordinates, vertices on the
seam between two materials usually have to be **split**: same position,
one copy per material, each with its own UV. That is normal — the same
reason a cube corner needs three vertices for its three normals — and it
costs vertices, never triangles.

### Texture ownership

Both texture setters leave the item holding **one** reference in
`OkTextureHandler`, released when the item is destroyed. `setTexture`
therefore adds a reference even though the caller already has the
pointer: a texture shared by many items (a sprite sheet, the font atlas)
would otherwise be freed by the first item to die, leaving the rest with
a dangling pointer.

The consequence for callers: code that *creates* a texture and hands it to
a single item should release its own creation reference straight after,
so destroying the item frees the texture.

```cpp
OkTexture *tex = OkFont::bake("label", "TEXT", 8, fg, bg);
billboard->setTexture("label", tex);
OkTextureHandler::getInstance()->removeReference("label");  // item owns it now
```

Code that keeps the texture alive for its own use (a sheet, a cached
atlas) simply keeps its reference and does nothing extra.

## OkInstancedItem

One mesh drawn many times in a **single** draw call: the base `OkItem` holds
the shared mesh (uploaded once), and this subclass adds a per-instance
buffer of world transforms (position, uniform scale, Y rotation) wired
with an attribute divisor. A thousand copies cost one draw call instead
of a thousand items, which is what makes large numbers of repeated
objects affordable.

Instances are addressable, not anonymous triangles — each one can be
moved, hidden or removed at runtime, and the buffer is recomposed
cheaply. The mesh is authored around its own origin
and each instance places a copy of it. Instances are frustum-culled
individually (the mesh bounding sphere at each instance position), so
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

## OkSpriteSheet

One texture holding many named regions. The image is uploaded once
(through the texture handler, so it is refcounted and shared like any
other texture) and the regions are metadata: rectangles with a name. A
region is never a texture of its own — that is the whole point of a
sheet: one upload, one bind, and the chance to draw many different
pieces in a single call.

The description file is read in the **Aseprite / TexturePacker JSON**
dialect, which is what pixel-art and packing tools emit. An artist can
redraw or repack the sheet in their tool of choice, export, and the
application picks it up with no code change. Only the parser speaks that
vocabulary; the API calls the pieces *regions*, and reserves the word
frame for actual animation (each region's duration and the sheet's tags
are kept for that).

| Method | Purpose |
| --- | --- |
| `bool load(jsonPath, imageOverride = "")` | Read the description and the image it names. |
| `OkTexture *getTexture() const` | The shared GPU texture, to hand to items. |
| `const OkSpriteRegion *getRegion(name) const` | A region's pixel rect and ready-to-use UVs, or null. |
| `bool hasRegion(name) const` | Whether a region exists. |
| `std::vector<std::string> getRegionNames() const` | Every region, in sheet order. |
| `std::vector<std::string> getGroup(tag) const` | The regions covered by a tag, for picking a member of a family. |
| `int getWidth() / getHeight() const` | Sheet size in pixels. |

`OkSpriteRegion` carries `x, y, w, h` in pixels and `u0, v0, u1, v1`
ready for a quad, already accounting for the engine loading textures
flipped for GL.

### Material masks

A sheet may carry, in its alpha channel, a code saying what each pixel
*is* rather than how opaque it is. With `OkItem::setMaskedMaterials(true)`
the shader gives each code its own tint (`setMaterialTint(slot, r, g,
b)`), so a single sheet serves many colour variants: the same artwork
recoloured per object, with pixels below the lowest code discarded.

Codes are read as roughly 1.00, 0.50 and 0.25 for slots 0, 1 and 2.

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

A billboard placed *at* a solid object intersects it: the quad is flat,
the object is not, and the sprite gets sliced along a hard straight
edge wherever the geometry pokes through. Typical cases are a glow
sprite centred on the thing that emits it, or a marker pinned to a
character's head.

`setCameraOffset(metres)` draws the quad a short distance toward the
camera along the view ray, recomputed every frame, so it wins the depth
test against the object it belongs to.

Because the displacement follows the view ray, a point moved along it
projects to the same pixel: the quad does not shift on screen at all,
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
