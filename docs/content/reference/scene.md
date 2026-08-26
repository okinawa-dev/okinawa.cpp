---
title: Scene
section: Reference
nav_order: 2
---

# Scene

`OkScene` is a container for a hierarchy of `OkObject`s. It stores only the root objects (those without a parent) and drives their per-frame step and draw, recursing into children. You add a scene to the engine through the scene handler (see [Handlers](/reference/handlers.html)).

## OkScene methods

| Method | Purpose |
| --- | --- |
| `OkScene(const std::string &name)` | Construct a named scene. |
| `void addObject(OkObject *object)` | Add a root object to the scene. |
| `void step(float dt)` | Step every object (called by the engine). |
| `void draw()` | Draw every object (called by the engine). |
| `void activate()` / `void deactivate()` | Toggle the scene's active state. |
| `bool isActive() const` | Whether the scene is active. |
| `bool isCurrent() const` | Whether the scene is the current one. |
| `size_t getObjectCount() const` | Number of root objects. |
| `const std::string &getName() const` | The scene name. |

The scene owns its objects: deleting the scene deletes the objects it holds, so do not delete added objects yourself.

## Example

```cpp
OkScene *scene = new OkScene("MainScene");

OkSceneHandler *handler = OkCore::getSceneHandler();
handler->addScene(scene, "MainScene");
handler->setScene(0);

scene->addObject(myItem);
```

## Draw order, and what a node may decide

A frame is drawn in two passes. **Opaque geometry first, nearest to
furthest** — the depth buffer then rejects what is hidden, which is the
cheapest defence against overdraw in a scene full of occluders. **Blended
geometry afterwards**: halos and glows deliberately do not write depth,
so anything opaque drawn later would pass the depth test and paint over
them.

Which pass an object belongs to is its own answer, `isBlended()`, and it
is asked **per object**. The scene walks the whole tree in each pass, and
each object draws itself in one of them. That matters as soon as objects
have children: decided per root, a single glow inside a group would carry
that group's walls and roofs into the late pass with it.

The opaque order is refreshed every few frames rather than every frame —
it only has to be roughly right, and sorting thousands of objects every
frame costs more than it saves.

### Skipping a subtree

`draw()` and `step()` ask two questions before doing anything:

```cpp
class Region : public OkItem {
public:
  bool shouldDraw() const override {
    return withinView(centre, radius);   // your test
  }
  bool shouldStep(float dt) const override {
    return withinView(centre, radius);
  }
};
```

Answer no and **neither the object nor its children** are visited. A node
standing for a region of the world therefore costs one test instead of
one per item inside it.

They are questions and not flags on purpose. A flag has to be put back,
and a flag left false is a piece of the world that has disappeared with
nothing to say why.

Drawing a subtree on its own — an inspection view, a thumbnail — is
`draw()`, which does both passes in the right order. A frame goes through
`drawPass()` instead, one pass at a time, so that all the opaque geometry
in the scene is down before any of the blended.
