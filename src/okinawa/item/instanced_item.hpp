#ifndef OK_INSTANCED_ITEM_HPP
#define OK_INSTANCED_ITEM_HPP

#include "item.hpp"
#include <vector>

/**
 * @brief One mesh drawn many times in a single draw call.
 *
 *        The base OkItem holds the shared mesh (uploaded once); this
 *        class adds a per-instance buffer of world transforms, so a
 *        thousand copies cost one draw call instead of a thousand
 *        items, which is what makes large numbers of repeated objects
 *        affordable.
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
  // Same mesh contract as OkItem (stride 5 computes normals, 8 takes
  // them verbatim). The mesh is authored around its own origin; every
  // instance places a copy of it.
  OkInstancedItem(const std::string &name, float *vertexData, long vertexCount,
                  unsigned int *indexData, long indexCount,
                  int vertexStride = 5);
  ~OkInstancedItem() override;

  // Add an instance at a world position with a Y rotation (radians) and
  // a uniform scale; returns its index.
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
  [[nodiscard]] int  getInstanceCount() const { return static_cast<int>(_instances.size()); }
  // Instances actually drawn in the last frame (after frustum culling).
  [[nodiscard]] int getDrawnCount() const { return _drawnCount; }

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
  std::vector<float>    _uploadScratch;  // visible instances, 8 floats each
  GLuint                _instanceVbo;
  int                   _drawnCount;
};

#endif
