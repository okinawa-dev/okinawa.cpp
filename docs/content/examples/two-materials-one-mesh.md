---
title: Two materials, one mesh
section: Examples
nav_order: 5
---

# Two materials, one mesh

This tutorial builds a single `OkItem` — a crate with wooden sides and a
metal lid — that wears two textures. It stays one object: one transform,
one bounding sphere, one frustum test. The index buffer is carved into
two ranges, each with its own material. See
[Items](/reference/items.html#material-slots) for the API.

The alternative, one item per texture, is worth avoiding: it doubles the
objects for what is one mesh, and it leaves you keeping two halves of a
surface aligned by hand.

## Lay the faces out by material

The only real constraint is that each material's faces must be
**contiguous** in the index buffer, so a range can name them. Build the
mesh material by material and record where each run starts and how long
it is.

A cube's corners already need one vertex per face — three faces meet
there, each wanting its own texture coordinates — so the geometry below
is six independent quads, and nothing extra has to be split:

```cpp
// One quad from four corners, mapped over the whole texture.
static void addQuad(std::vector<float> &verts, std::vector<unsigned int> &idx,
                    const float *a, const float *b,
                    const float *c, const float *d) {
  const float  uv[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
  const float *p[4]     = {a, b, c, d};
  unsigned int base     = (unsigned int)(verts.size() / 5);
  for (int i = 0; i < 4; i++) {
    verts.push_back(p[i][0]);
    verts.push_back(p[i][1]);
    verts.push_back(p[i][2]);
    verts.push_back(uv[i][0]);
    verts.push_back(uv[i][1]);
  }
  idx.insert(idx.end(), {base, base + 1, base + 2});
  idx.insert(idx.end(), {base, base + 2, base + 3});
}

// The eight corners of a unit crate.
const float A[3] = {-0.5f, -0.5f,  0.5f}, B[3] = { 0.5f, -0.5f,  0.5f};
const float C[3] = { 0.5f,  0.5f,  0.5f}, D[3] = {-0.5f,  0.5f,  0.5f};
const float E[3] = {-0.5f, -0.5f, -0.5f}, F[3] = { 0.5f, -0.5f, -0.5f};
const float G[3] = { 0.5f,  0.5f, -0.5f}, H[3] = {-0.5f,  0.5f, -0.5f};

std::vector<float>        verts;
std::vector<unsigned int> idx;

// The five wooden sides first...
long sidesFirst = (long)idx.size();
addQuad(verts, idx, A, B, C, D);   // front
addQuad(verts, idx, F, E, H, G);   // back
addQuad(verts, idx, B, F, G, C);   // right
addQuad(verts, idx, E, A, D, H);   // left
addQuad(verts, idx, E, F, B, A);   // bottom
long sidesCount = (long)idx.size() - sidesFirst;

// ...then the metal lid, so the two runs stay contiguous.
long lidFirst = (long)idx.size();
addQuad(verts, idx, D, C, G, H);   // top
long lidCount = (long)idx.size() - lidFirst;
```

Every face brings its own four vertices, so the crate has 24 rather than
8. That is not a concession to the material split — a cube needs it
anyway, since three faces meet at each corner and each wants its own
texture coordinates. With stride 5 the engine derives normals from the
triangle list, and unshared vertices give the flat, hard-edged shading a
crate should have.

## Create the item and name its slots

```cpp
OkItem *crate = new OkItem("crate", verts.data(), (long)verts.size(),
                           idx.data(), (long)idx.size());
crate->addMaterialFromFile(sidesFirst, sidesCount, "assets/wood.png");
crate->addMaterialFromFile(lidFirst,   lidCount,   "assets/metal.png");
scene->addObject(crate);
```

That is the whole difference. Everything else about the item behaves as
it always did — `setPosition`, `setVisible`, `setFade`, parenting — and
applies to the mesh as a whole.

## What happens at draw time

Per frame the item is culled once, its transform is uploaded once, and
its lighting and fade uniforms are set once. Then, for each slot, the
texture is bound and one `glDrawElements` is issued over that slot's
range. Two slots cost one extra bind and one extra draw call — not a
second object.

An item with **no** slots is unchanged: it draws its whole index buffer
with the texture set by `loadTextureFromFile` or `setTexture`. Slots are
opt-in, and single-material meshes pay nothing for the feature.

## Choosing the split

Ranges are drawn in the order added and should cover the index buffer
without overlapping. Nothing enforces that: a gap never draws and an
overlap draws twice, both silently. Building the mesh one material at a
time, recording `first` and `count` as above, makes it hard to get wrong.

Where two materials meet **within** a surface rather than at a corner —
a stripe across a floor, a worn patch on a wall — the vertices along the
join usually have to be duplicated: same position, one copy per
material, each with its own texture coordinates. A vertex carries one
set, so it can belong to one mapping only. That costs vertices, never
triangles, and it is the same reason a cube corner is three vertices
rather than one.

And if the join falls somewhere the mesh has no edge at all, no slot can
help: the geometry has to be cut along that line first, and only then can
the ranges name the two sides. Painting part of a face is a job for a
shader mask or a decal, not for a material slot.
