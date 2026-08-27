#ifndef OK_INSTANCED_ITEM_HPP
#define OK_INSTANCED_ITEM_HPP

#include "item.hpp"
#include <vector>

/**
 * @brief One mesh drawn many times in a single draw call.
 *
 *        The base OkItem holds the shared mesh (uploaded once); this
 *        class adds a per-instance buffer of transforms, so a thousand
 *        copies cost one draw call instead of a thousand items, which
 *        is what makes large numbers of repeated objects affordable.
 *
 *        An instance is placed WITHIN the item, and the item's own
 *        transform applies on top -- so an instanced item hangs off a
 *        parent and moves with it like anything else, and its instances
 *        can be written in whatever frame owns them. It used to pass
 *        the identity as its model matrix, which pinned every instance
 *        to the origin of the world: correct while the only caller
 *        passed world positions, and the one thing in the engine that
 *        could not be given a parent.
 *
 *        Instances are ADDRESSABLE, not anonymous triangles: each one
 *        can be moved, hidden or removed at runtime, and the buffer is
 *        recomposed cheaply. The logical side of an object (its state,
 *        its collision volume) belongs to the application, never to
 *        this class -- collision should not come from render triangles.
 *
 *        Instances are frustum-culled individually against the frame's
 *        frustum, using the mesh's bounding sphere at each instance
 *        position, so only what is on screen is uploaded and drawn.
 */
class OkInstancedItem : public OkItem {
public:
  // Interleaved vertex layout the mesh arrives in: three position floats
  // then two texture coordinates.
  static const int DEFAULT_VERTEX_STRIDE = 5;

  // Same mesh contract as OkItem (stride 5 computes normals, 8 takes
  // them verbatim). The mesh is authored around its own origin; every
  // instance places a copy of it.
  OkInstancedItem(const std::string &name, float *vertexData, long vertexCount,
                  unsigned int *indexData, long indexCount,
                  int vertexStride = DEFAULT_VERTEX_STRIDE);
  ~OkInstancedItem() override;

  /**
   * @brief The sphere covering every instance, not just the mesh.
   *
   * What a parent has to be told: an instanced item stands wherever its
   * instances stand, which can be a whole district.
   */
  float getRadius() const override {
    return _instanceRadius > 0.0f ? _instanceRadius : radius;
  }
  const std::array<float, RGB> &getSphereCenter() const override {
    return _instanceCentre;
  }

  // Add an instance at a position WITHIN this item, with a Y rotation
  // (radians) and a uniform scale; returns its index. With the item at
  // the origin and no parent, that position is a world one.
  int addInstance(float x, float y, float z, float yaw = 0.0f,
                  float scale = 1.0f);
  // Move an existing instance.
  void setInstance(int index, float x, float y, float z, float yaw = 0.0f,
                   float scale = 1.0f);
  // Hide/show one instance: hidden instances are not
  // drawn and cost nothing but their slot.
  void setInstanceVisible(int index, bool visible);
  // Drop every instance.
  void clearInstances();
  int  getInstanceCount() const {
    return static_cast<int>(_instances.size());
  }
  // Instances actually drawn in the last frame (after frustum culling).
  int getDrawnCount() const {
    return _drawnCount;
  }

protected:
  void drawSelf() override;

private:
  struct Instance {
    float x, y, z;
    float yaw;
    float scale;
    bool  visible;
  };

  void ensureInstanceBuffer();

  std::vector<Instance> _instances;
  // The sphere the instances cover, grown as they arrive: a parent asks
  // how far this item reaches and the mesh alone cannot answer.
  std::array<float, RGB> _instanceCentre;
  float                  _instanceRadius;

  /** @brief Widen the instance sphere to take one more instance in. */
  void               growInstanceBounds(const Instance &inst);
  std::vector<float> _uploadScratch;  // visible instances, 8 floats each
  GLuint             _instanceVbo;
  int                _drawnCount;
};

#endif
