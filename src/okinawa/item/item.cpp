#include "item.hpp"
#include "../config/config.hpp"
#include "../core/gl_config.hpp"
#include "../handlers/textures.hpp"
#include "../lighting/lighting.hpp"
#include "../math/frustum.hpp"
#include "../utils/logger.hpp"
#include "core/object.hpp"
#include "item/texture.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

namespace {

  // Half, for midpoints and radii.
  const float HALF = 0.5f;
}  // namespace

/**
 * @brief Create a new item with the given name, vertices, and indices.
 * @param name        The name of the item.
 * @param vertexData  The vertex data.
 * @param vertexCount The number of vertices.
 * @param indexData   The index data.
 * @param indexCount  The number of indices.
 */
void OkItem::_initDefaults() {
  visible           = true;
  drawWireframe     = false;
  wireframeGlobal   = true;
  castsShadow       = true;
  drawMode          = GL_TRIANGLES;  // Default drawing mode
  fillColor[0]      = 1.0f;
  fillColor[1]      = 1.0f;
  fillColor[2]      = 1.0f;
  fillColor[3]      = 1.0f;
  tintColor[0]      = 1.0f;
  tintColor[1]      = 1.0f;
  tintColor[2]      = 1.0f;
  tintColor[3]      = 1.0f;
  wireframeColor[0] = 1.0f;
  wireframeColor[1] = 1.0f;
  wireframeColor[2] = 1.0f;

  texture         = nullptr;
  textureName     = "";
  sphereCenter[0] = 0.0f;
  sphereCenter[1] = 0.0f;
  sphereCenter[2] = 0.0f;
  additive        = false;
  unlit           = false;
  maskedMaterials = false;
  fade            = 1.0f;
  fadeInverted    = false;
  for (int i = 0; i < 3; i++) {
    matTint[i][0] = 1.0f;
    matTint[i][1] = 1.0f;
    matTint[i][2] = 1.0f;
    matLuma[i]    = 0.0f;
  }
  nearLightCount = 0;
  nearLightGen   = -1;
}

OkItem::OkItem(const std::string &name) : OkObject(name) {
  // An item with nothing in it yet: geometry arrives piece by piece
  // through addMesh(), and upload() hands the finished mesh to the GPU.
  _initDefaults();
  vertices    = nullptr;
  indices     = nullptr;
  numVertices = 0;
  numIndices  = 0;
}

OkItem::OkItem(const std::string &name, float *vertexData, long vertexCount,
               unsigned int *indexData, long indexCount, int vertexStride)
    : OkObject(name) {

  OkLogger::info("Item", "Creating item " + name + " with " +
                             std::to_string(vertexCount) + " vertices and " +
                             std::to_string(indexCount) + " indices");

  _initDefaults();
  numIndices = indexCount;

  _adoptVertexData(vertexData, vertexCount, indexData, indexCount,
                   vertexStride);

  // Allocate and copy index data
  indices = new unsigned int[indexCount];
  std::memcpy(indices, indexData, indexCount * sizeof(unsigned int));
  numIndices = indexCount;

  _calculateRadius();

  _initBuffers();
}

void OkItem::addMesh(const float *vertexData, long vertexCount,
                     const unsigned int *indexData, long indexCount,
                     const std::string &texturePath, int vertexStride) {
  if (vertexData == nullptr || indexData == nullptr || vertexCount <= 0 ||
      indexCount <= 0) {
    return;
  }
  // The piece is expanded in memory, and nothing here touches the GPU.
  //
  // That is the rule this method exists under: a mesh may be assembled
  // from thousands of pieces, and creating a GPU object per piece would
  // have OpenGL recycling buffer names underneath whatever else is
  // uploading on the same thread. Geometry then draws from somebody
  // else's buffer -- not at once, but once enough has been created and
  // freed for the recycling to collide.
  std::vector<float> expanded;
  _expandVertices(vertexData, vertexCount, indexData, indexCount, vertexStride,
                  &expanded);
  if (expanded.empty()) {
    return;
  }

  // Where the piece's vertices land, which is what its indices have to
  // be moved along by. Get this wrong and the second piece draws with
  // the first one's geometry -- silently, because the indices are still
  // in range.
  auto base = static_cast<unsigned int>(numVertices / VERTEX_STRIDE);

  auto  grownCount = numVertices + static_cast<long>(expanded.size());
  auto *grownV     = new float[grownCount];
  if (vertices != nullptr && numVertices > 0) {
    std::memcpy(grownV, vertices,
                static_cast<size_t>(numVertices) * sizeof(float));
  }
  std::memcpy(grownV + numVertices, expanded.data(),
              expanded.size() * sizeof(float));
  delete[] vertices;
  vertices    = grownV;
  numVertices = grownCount;

  auto *grownI = new unsigned int[numIndices + indexCount];
  if (indices != nullptr && numIndices > 0) {
    std::memcpy(grownI, indices,
                static_cast<size_t>(numIndices) * sizeof(unsigned int));
  }
  for (long i = 0; i < indexCount; i++) {
    grownI[numIndices + i] = indexData[i] + base;
  }
  delete[] indices;
  indices    = grownI;
  long first = numIndices;
  numIndices += indexCount;

  addMaterialFromFile(first, indexCount, texturePath);
}

void OkItem::upload() {
  if (vertices == nullptr || numVertices <= 0) {
    return;
  }
  _calculateRadius();
  _initBuffers();
}

/**
 * @brief Store vertex data with the internal stride-8 layout
 *        (x,y,z,u,v,nx,ny,nz). Stride-5 input (the historical layout
 *        every caller uses) is expanded here: normals are computed by
 *        accumulating the face normal of every triangle onto its three
 *        vertices. De-indexed meshes (each face owns its vertices, the
 *        the usual loader convention) end up with exact flat face normals;
 *        indexed meshes with shared vertices (terrain grids) end up with
 *        area-weighted smooth normals. Non-triangle index lists (lines,
 *        points) produce garbage normals, which is fine: the shader only
 *        lights the textured branch, and debug lines are never textured.
 */
/**
 * @brief Expand a piece of mesh into the internal stride-8 layout.
 *
 * Stride-5 input (x,y,z,u,v -- the layout every caller writes) gets its
 * normals computed here by accumulating the face normal of every
 * triangle onto its three vertices. De-indexed meshes, where each face
 * owns its vertices, end up with exact flat face normals; indexed meshes
 * with shared vertices (terrain grids) end up with area-weighted smooth
 * ones. Non-triangle index lists (lines, points) produce garbage
 * normals, which is fine: the shader only lights the textured branch,
 * and debug lines are never textured.
 *
 * Pure arithmetic, and deliberately so: it touches no GPU state, which
 * is what lets a mesh be assembled from pieces without creating and
 * destroying buffers per piece.
 */
void OkItem::_expandVertices(const float *vertexData, long vertexCount,
                             const unsigned int *indexData, long indexCount,
                             int vertexStride, std::vector<float> *out) {
  out->clear();
  if (vertexData == nullptr || vertexCount <= 0) {
    return;
  }
  if (vertexStride == VERTEX_STRIDE) {
    out->assign(vertexData, vertexData + vertexCount);
    return;
  }
  long vcount = vertexCount / VERTEX_STRIDE_IN;
  if (vcount <= 0) {
    return;
  }
  out->assign(static_cast<size_t>(vcount * VERTEX_STRIDE), 0.0f);
  std::vector<float> &v = *out;
  for (long i = 0; i < vcount; i++) {
    long src = i * VERTEX_STRIDE_IN;
    long dst = i * VERTEX_STRIDE;
    for (int k = 0; k < VERTEX_STRIDE_IN; k++) {
      v[static_cast<size_t>(dst + k)] = vertexData[src + k];
    }
  }
  for (long f = 0; f + 2 < indexCount; f += 3) {
    long ia = static_cast<long>(indexData[f]);
    long ib = static_cast<long>(indexData[f + 1]);
    long ic = static_cast<long>(indexData[f + 2]);
    if (ia >= vcount || ib >= vcount || ic >= vcount) {
      continue;
    }
    float ax = vertexData[ia * VERTEX_STRIDE_IN];
    float ay = vertexData[ia * VERTEX_STRIDE_IN + 1];
    float az = vertexData[ia * VERTEX_STRIDE_IN + 2];
    float ux = vertexData[ib * VERTEX_STRIDE_IN] - ax;
    float uy = vertexData[ib * VERTEX_STRIDE_IN + 1] - ay;
    float uz = vertexData[ib * VERTEX_STRIDE_IN + 2] - az;
    float wx = vertexData[ic * VERTEX_STRIDE_IN] - ax;
    float wy = vertexData[ic * VERTEX_STRIDE_IN + 1] - ay;
    float wz = vertexData[ic * VERTEX_STRIDE_IN + 2] - az;
    // Area-weighted face normal (unnormalized cross product).
    float               nx  = uy * wz - uz * wy;
    float               ny  = uz * wx - ux * wz;
    float               nz  = ux * wy - uy * wx;
    std::array<long, 3> tri = {ia, ib, ic};
    for (int k = 0; k < 3; k++) {
      v[static_cast<size_t>(tri[k] * VERTEX_STRIDE + VERTEX_NORMAL + 0)] += nx;
      v[static_cast<size_t>(tri[k] * VERTEX_STRIDE + VERTEX_NORMAL + 1)] += ny;
      v[static_cast<size_t>(tri[k] * VERTEX_STRIDE + VERTEX_NORMAL + 2)] += nz;
    }
  }
  for (long i = 0; i < vcount; i++) {
    long  n   = i * VERTEX_STRIDE + VERTEX_NORMAL;
    float nx  = v[static_cast<size_t>(n)];
    float ny  = v[static_cast<size_t>(n + 1)];
    float nz  = v[static_cast<size_t>(n + 2)];
    float len = std::sqrt(nx * nx + ny * ny + nz * nz);
    if (len > 1e-8f) {
      v[static_cast<size_t>(n)]     = nx / len;
      v[static_cast<size_t>(n + 1)] = ny / len;
      v[static_cast<size_t>(n + 2)] = nz / len;
    } else {
      v[static_cast<size_t>(n)]     = 0.0f;
      v[static_cast<size_t>(n + 1)] = 1.0f;
      v[static_cast<size_t>(n + 2)] = 0.0f;
    }
  }
}

/**
 * @brief Store vertex data in the internal layout, expanding it first.
 */
void OkItem::_adoptVertexData(float *vertexData, long vertexCount,
                              const unsigned int *indexData, long indexCount,
                              int vertexStride) {
  std::vector<float> expanded;
  _expandVertices(vertexData, vertexCount, indexData, indexCount, vertexStride,
                  &expanded);
  if (expanded.empty()) {
    vertices    = nullptr;
    numVertices = 0;
    return;
  }
  vertices = new float[expanded.size()];
  std::memcpy(vertices, expanded.data(), expanded.size() * sizeof(float));
  numVertices = static_cast<long>(expanded.size());
}

/**
 * @brief Add a material slot over a range of the index buffer.
 */
void OkItem::addMaterialFromFile(long firstIndex, long indexCount,
                                 const std::string &path) {
  if (indexCount <= 0 || firstIndex < 0 ||
      firstIndex + indexCount > numIndices) {
    OkLogger::error("Item", "Material range out of bounds for item: " + name);
    return;
  }
  MaterialRange mr;
  mr.first   = firstIndex;
  mr.count   = indexCount;
  mr.texture = nullptr;
  if (!path.empty()) {
    // Same contract as loadTextureFromFile: the slot holds its own
    // reference, so the texture outlives whatever else drops it.
    mr.texture = OkTextureHandler::getInstance()->createTextureFromFile(path);
    if (mr.texture) {
      mr.textureName = path;
    }
  }
  materials.push_back(mr);
}

/**
 * @brief Drop every material slot (and its texture reference).
 */
void OkItem::clearMaterials() {
  for (size_t i = 0; i < materials.size(); i++) {
    if (materials[i].texture && !materials[i].textureName.empty()) {
      OkTextureHandler::getInstance()->removeReference(
          materials[i].textureName);
    }
  }
  materials.clear();
}

/**
 * @brief Initialize OpenGL buffers for the item.
 */
void OkItem::_initBuffers() {
  // Delete existing OpenGL objects if they exist (safe for updates)
  if (VAO != 0) {
    glDeleteVertexArrays(1, &VAO);
    VAO = 0;
  }
  if (VBO != 0) {
    glDeleteBuffers(1, &VBO);
    VBO = 0;
  }
  if (EBO != 0) {
    glDeleteBuffers(1, &EBO);
    EBO = 0;
  }

  // Generate and bind VAO first
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  // Generate and set up VBO
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(numVertices * sizeof(float)), vertices,
               GL_STATIC_DRAW);

  // Position attribute (3 floats)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_STRIDE * sizeof(float),
                        nullptr);
  glEnableVertexAttribArray(0);

  // Texture coords attribute (2 floats)
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, VERTEX_STRIDE * sizeof(float),
                        reinterpret_cast<GLvoid *>(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Normal attribute (3 floats)
  glVertexAttribPointer(
      2, 3, GL_FLOAT, GL_FALSE, VERTEX_STRIDE * sizeof(float),
      reinterpret_cast<GLvoid *>(VERTEX_NORMAL * sizeof(float)));
  glEnableVertexAttribArray(2);

  // Generate and set up EBO
  glGenBuffers(1, &EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(numIndices * sizeof(unsigned int)),
               indices, GL_STATIC_DRAW);

  // Unbind VAO and VBO (but not EBO while VAO is active)
  // Unbind VAO first, then VBO and EBO
  // You can unbind the VAO afterwards so other VAO calls won't accidentally
  // modify this VAO, but this rarely happens. Modifying other VAOs requires a
  // call to glBindVertexArray anyways so we generally don't unbind VAOs (nor
  // VBOs) when it's not directly necessary.
  glBindVertexArray(0);

  // note that this is allowed, the call to glVertexAttribPointer registered
  // VBO as the vertex attribute's bound vertex buffer object so afterwards we
  // can safely unbind
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/**
 * @brief Calculate the radius of the item based on its vertices.
 * @note  The radius is calculated as the maximum distance from the center of
 *        the item to any vertex.
 */
OkItem::~OkItem() {
  // Delete OpenGL objects
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &EBO);

  // Free allocated memory
  delete[] vertices;
  delete[] indices;

  // Remove texture reference
  if (texture && !textureName.empty()) {
    OkTextureHandler::getInstance()->removeReference(textureName);
  }
  for (size_t i = 0; i < materials.size(); i++) {
    if (materials[i].texture && !materials[i].textureName.empty()) {
      OkTextureHandler::getInstance()->removeReference(
          materials[i].textureName);
    }
  }
}

void OkItem::_calculateRadius() {

  // numVertices counts floats, not vertices: fewer than one whole
  // vertex means there is no x, y, z to seed the bounds with, and
  // reading them would walk off the buffer.
  if (numVertices < VERTEX_STRIDE || vertices == nullptr) {
    radius = 0.0f;
    OkLogger::warning("Item", "No vertices to calculate radius");
    return;
  }

  float minX = vertices[0];
  float maxX = vertices[0];
  float minY = vertices[1];
  float maxY = vertices[1];
  float minZ = vertices[2];
  float maxZ = vertices[2];

  const long actualVertexCount = numVertices / VERTEX_STRIDE;

  // Iterate through actual vertices
  for (long i = 0; i < actualVertexCount; i++) {
    long  offset = i * VERTEX_STRIDE;
    float x      = vertices[offset];      // Position X
    float y      = vertices[offset + 1];  // Position Y
    float z      = vertices[offset + 2];  // Position Z
    // vertices[offset + 3] and [offset + 4] are UV coordinates

    minX = std::min(minX, x);
    maxX = std::max(maxX, x);
    minY = std::min(minY, y);
    maxY = std::max(maxY, y);
    minZ = std::min(minZ, z);
    maxZ = std::max(maxZ, z);
  }

  float width  = maxX - minX;
  float height = maxY - minY;
  float depth  = maxZ - minZ;
  // Calculate radius as half the diagonal of the bounding box
  radius = sqrt(width * width + height * height + depth * depth) * HALF;
  // Bounding-sphere centre in local coords: baked meshes keep their
  // vertices in chunk-local space with the item origin at the chunk
  // corner, so the sphere must be centred on the bbox, not the origin.
  sphereCenter[0] = (minX + maxX) * HALF;
  sphereCenter[1] = (minY + maxY) * HALF;
  sphereCenter[2] = (minZ + maxZ) * HALF;

  OkLogger::info("Item",
                 "Bounds: (" + std::to_string(minX) + ", " +
                     std::to_string(minY) + ", " + std::to_string(minZ) +
                     ") to (" + std::to_string(maxX) + ", " +
                     std::to_string(maxY) + ", " + std::to_string(maxZ) + ")");
  OkLogger::info("Item", "Calculated radius: " + std::to_string(radius));
}

/**
 * @brief Set the texture for the item.
 * @param texturePath The path to the texture file.
 */
void OkItem::loadTextureFromFile(const std::string &texturePath) {
  if (texturePath.empty()) {
    OkLogger::error("Item", "Invalid texture path");
    return;
  }

  // Remove old texture reference if any
  if (texture && !textureName.empty()) {
    OkTextureHandler::getInstance()->removeReference(textureName);
    texture     = nullptr;
    textureName = "";
  }

  // Create texture through handler
  texture = OkTextureHandler::getInstance()->createTextureFromFile(texturePath);
  if (texture) {
    textureName = texturePath;
  }
}

/**
 * @brief Update the item state.
 *        This method is called every frame to update the item.
 * @param dt The delta time since the last update.
 */
static bool g_shadowPass = false;
void        OkItem::setShadowPass(bool on) {
  g_shadowPass = on;
}
bool OkItem::inShadowPass() {
  return g_shadowPass;
}

void OkItem::stepSelf(float dt) {
  // Call parent class step function first
  // OkObject::step(dt);
}

/**
 * @brief Update the transform matrix of the item.
 */
void OkItem::updateTransformSelf() {
  // Log transform update for debugging
  // OkLogger::info("Item", "Updating transform for " + name + " at position ("
  // +
  //                std::to_string(position.x()) + ", " +
  //                std::to_string(position.y()) + ", " +
  //                std::to_string(position.z()) + ")");
}

/**
 * @brief Draw the item and its children.
 * @note  This method handles the rendering of the item and its children.
 */
void OkItem::applyMaterialUniforms(unsigned int program) const {
  GLint maskLoc = glGetUniformLocation(program, "maskedMaterials");
  if (maskLoc != -1) {
    glUniform1f(maskLoc, maskedMaterials ? 1.0f : 0.0f);
  }
  if (!maskedMaterials) {
    return;
  }
  const std::array<const char *, MAT_SLOTS> names = {"matTintA", "matTintB",
                                                     "matTintC"};
  for (int i = 0; i < MAT_SLOTS; i++) {
    GLint loc = glGetUniformLocation(program, names[static_cast<size_t>(i)]);
    if (loc != -1) {
      glUniform4f(loc, matTint[i][0], matTint[i][1], matTint[i][2], 1.0f);
    }
  }
  GLint lumaLoc = glGetUniformLocation(program, "matLuminance");
  if (lumaLoc != -1) {
    glUniform3f(lumaLoc, matLuma[0], matLuma[1], matLuma[2]);
  }
}

void OkItem::drawSelf() {
  if (!this->visible) {
    // If the item is not visible, skip rendering
    return;
  }
  if (g_shadowPass && !castsShadow) {
    // Light, not matter: it has geometry, but recording it as an
    // occluder would have a glow cast a shadow.
    return;
  }

  // World-space bounding-sphere centre: used by the frustum test and by
  // the point-light selection below.
  glm::mat4 tm = getTransformMatrix();
  glm::vec4 wc =
      tm * glm::vec4(sphereCenter[0], sphereCenter[1], sphereCenter[2], 1.0f);

  // Frustum culling: skip the draw when the item's bounding sphere is
  // fully outside the frame's view frustum (world pass only; the GUI and
  // camera-attached passes run with no active frustum). The radius is
  // scaled by the largest axis scale of the transform.
  const OkFrustum *frustum = OkFrustum::getActive();
  if (frustum != nullptr) {
    // Draw distance first: it is a single comparison and rejects far
    // more than the frustum test in an open world.
    float sx0 = std::sqrt(tm[0][0] * tm[0][0] + tm[0][1] * tm[0][1] +
                          tm[0][2] * tm[0][2]);
    if (OkFrustum::isBeyondDrawDistance(wc.x, wc.y, wc.z, radius * sx0)) {
      OkFrustum::addCulled();
      return;
    }
  }
  if (frustum != nullptr) {
    float sx = std::sqrt(tm[0][0] * tm[0][0] + tm[0][1] * tm[0][1] +
                         tm[0][2] * tm[0][2]);
    float sy = std::sqrt(tm[1][0] * tm[1][0] + tm[1][1] * tm[1][1] +
                         tm[1][2] * tm[1][2]);
    float sz = std::sqrt(tm[2][0] * tm[2][0] + tm[2][1] * tm[2][1] +
                         tm[2][2] * tm[2][2]);
    float s  = std::max({sx, sy, sz});
    if (!frustum->containsSphere(wc.x, wc.y, wc.z, radius * s)) {
      OkFrustum::addCulled();
      return;
    }
  }

  bool drawWireframe =
      (this->wireframeGlobal && OkConfig::getBool("graphics.wireframe")) ||
      this->drawWireframe;
  // Verify we have a valid shader program
  GLint current_program;
  glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
  if (current_program == 0) {
    OkLogger::error("Item", "No shader program in use");
    return;
  }

  // Clear any previous errors
  while (glGetError() != GL_NO_ERROR)
    ;

  // Get model matrix from base class
  glm::mat4 model = getTransformMatrix();

  // Set the model matrix uniform in shader
  GLint modelLoc = glGetUniformLocation(current_program, "model");
  if (modelLoc == -1) {
    OkLogger::error("Item", "Cannot find model uniform in shader");
    return;
  }
  glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

  // Point lights (L4): refresh the cached nearest set when the registry
  // changed, then pass up to MAX_LIGHTS_PER_ITEM lights (position +
  // radius, colour) to the Gouraud stage. World pass only (the GUI pass
  // runs with lightingOn 0, where the shader ignores them).
  {
    if (nearLightGen != OkLighting::getLightGeneration()) {
      nearLightCount = OkLighting::getNearestLights(
          wc.x, wc.y, wc.z, nearLights.data(), OkLighting::MAX_LIGHTS_PER_ITEM);
      nearLightGen = OkLighting::getLightGeneration();
    }
    GLint cntLoc = glGetUniformLocation(current_program, "pointLightCount");
    if (cntLoc != -1) {
      glUniform1i(cntLoc, nearLightCount);
      for (int i = 0; i < nearLightCount; i++) {
        const float *lp   = OkLighting::getLightPosition(nearLights[i]);
        const float *lc   = OkLighting::getLightColor(nearLights[i]);
        const float *ld   = OkLighting::getLightDirection(nearLights[i]);
        float        lr   = OkLighting::getLightRadius(nearLights[i]);
        float        cc   = OkLighting::getLightCosCone(nearLights[i]);
        float        li   = OkLighting::getLightIntensity(nearLights[i]);
        std::string  base = "pointLights[" + std::to_string(i) + "]";
        GLint        pLoc = glGetUniformLocation(current_program,
                                                 (base + ".posRadius").c_str());
        GLint        cLoc =
            glGetUniformLocation(current_program, (base + ".color").c_str());
        GLint sLoc =
            glGetUniformLocation(current_program, (base + ".spot").c_str());
        if (pLoc != -1) {
          glUniform4f(pLoc, lp[0], lp[1], lp[2], lr);
        }
        if (cLoc != -1) {
          glUniform4f(cLoc, lc[0], lc[1], lc[2], li);
        }
        if (sLoc != -1) {
          glUniform4f(sLoc, ld[0], ld[1], ld[2], cc);
        }
      }
    }
  }

  // Unlit items (halos, emissive glows) skip the Gouraud light and the
  // atmosphere tint for this draw; world-pass values are restored after.
  if (unlit) {
    GLint litLoc  = glGetUniformLocation(current_program, "lightingOn");
    GLint tintLoc = glGetUniformLocation(current_program, "sceneTint");
    if (litLoc != -1) {
      glUniform1f(litLoc, 0.0f);
    }
    if (tintLoc != -1) {
      glUniform3f(tintLoc, 1.0f, 1.0f, 1.0f);
    }
  }
  if (additive) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
  }

  // Verify we have valid buffers
  if (VAO == 0) {
    OkLogger::error("Item", "No VAO for item: " + name);
    return;
  }

  // Bind VAO and draw
  glBindVertexArray(VAO);
  if (glGetError() != GL_NO_ERROR) {
    OkLogger::error("Item", "Error binding VAO for item: " + name);
    return;
  }

  // Fill pass: textured if we have a texture, otherwise a flat fill colour.
  // Always runs, so an item can show a flat fill and a wireframe overlay.
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    GLint hasTexLoc = glGetUniformLocation(current_program, "hasTexture");
    GLint tintLoc   = glGetUniformLocation(current_program, "tintColor");
    if (tintLoc != -1) {
      glUniform4f(tintLoc, tintColor[0], tintColor[1], tintColor[2],
                  tintColor[3]);
    }
    // Uniform locations are fixed for the life of a program, so they are
    // looked up once per program rather than once per draw: with a few
    // thousand objects on screen, a name lookup per object per frame is
    // a measurable slice of the frame on its own.
    {
      static GLuint cachedProgram = 0;
      static GLint  cachedFade    = -1;
      static GLint  cachedInvert  = -1;
      if (cachedProgram != current_program) {
        cachedProgram = current_program;
        cachedFade    = glGetUniformLocation(current_program, "itemFade");
        cachedInvert  = glGetUniformLocation(current_program, "itemFadeInvert");
      }
      if (cachedFade != -1) {
        glUniform1f(cachedFade, fade);
      }
      if (cachedInvert != -1) {
        glUniform1f(cachedInvert, fadeInverted ? 1.0f : 0.0f);
      }
    }
    applyMaterialUniforms(static_cast<unsigned int>(current_program));
    GLint texLoc     = glGetUniformLocation(current_program, "texture0");
    GLint colorLoc   = glGetUniformLocation(current_program, "wireframeColor");
    bool  texturesOn = OkConfig::getBool("graphics.textures");

    // One pass per material slot, over the same buffers: the transform,
    // the culling and every uniform above are done once for the whole
    // item, and only the texture changes between ranges. An item with
    // no slots draws its whole index buffer with its own texture, which
    // is the common case.
    size_t passes = materials.empty() ? 1 : materials.size();
    for (size_t mi = 0; mi < passes; mi++) {
      OkTexture *tex    = materials.empty() ? texture : materials[mi].texture;
      long       first  = materials.empty() ? 0 : materials[mi].first;
      long       count  = materials.empty() ? numIndices : materials[mi].count;
      bool       useTex = texturesOn && tex && tex->isLoaded();
      if (useTex) {
        glActiveTexture(GL_TEXTURE0);
        tex->bind();
        if (texLoc != -1) {
          glUniform1i(texLoc, 0);
        }
        if (hasTexLoc != -1) {
          glUniform1i(hasTexLoc, 1);
        }
      } else {
        if (hasTexLoc != -1) {
          glUniform1i(hasTexLoc, 0);
        }
        if (colorLoc != -1) {
          glUniform4f(colorLoc, fillColor[0], fillColor[1], fillColor[2],
                      fillColor[3]);
        }
      }
      glDrawElements(drawMode, static_cast<GLsizei>(count), GL_UNSIGNED_INT,
                     reinterpret_cast<const void *>(
                         first * static_cast<long>(sizeof(unsigned int))));
      OkFrustum::addDraw(count / 3);
    }
  }

  // Wireframe overlay pass (in the wireframe colour, on top of the fill).
  // Always unlit: the overlay is a drawing aid, not a surface.
  if (drawWireframe) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    GLint wLitLoc = glGetUniformLocation(current_program, "lightingOn");
    if (wLitLoc != -1) {
      glUniform1f(wLitLoc, 0.0f);
    }

    GLint hasTexLoc = glGetUniformLocation(current_program, "hasTexture");
    if (hasTexLoc != -1) {
      glUniform1i(hasTexLoc, 0);
    }

    GLint colorLoc = glGetUniformLocation(current_program, "wireframeColor");
    if (colorLoc != -1) {
      glUniform4f(colorLoc, wireframeColor[0], wireframeColor[1],
                  wireframeColor[2], 1.0f);
    }

    glDrawElements(drawMode, static_cast<GLsizei>(numIndices), GL_UNSIGNED_INT,
                   nullptr);
  }

  // Reset polygon mode and the lighting flag after the overlay.
  if (drawWireframe) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    GLint wLitLoc = glGetUniformLocation(current_program, "lightingOn");
    if (wLitLoc != -1) {
      glUniform1f(wLitLoc, unlit ? 0.0f : 1.0f);
    }
  }

  if (texture) {
    // static call to unbind texture, opengl does not need to know which
    // texture was bound before, it will just unbind the currently bound
    // texture with
    // glBindTexture(GL_TEXTURE_2D, 0);
    OkTexture::unbind();
  }

  // Restore world-pass state after additive/unlit draws.
  if (additive) {
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
  }
  if (unlit) {
    GLint        litLoc  = glGetUniformLocation(current_program, "lightingOn");
    GLint        tintLoc = glGetUniformLocation(current_program, "sceneTint");
    const float *wt      = OkLighting::getSceneTint();
    if (litLoc != -1) {
      glUniform1f(litLoc, 1.0f);
    }
    if (tintLoc != -1) {
      glUniform3f(tintLoc, wt[0], wt[1], wt[2]);
    }
  }
}

bool OkItem::intersectRay(const OkRay &ray, float *outDistance) const {
  // Lines and points have no surface to cross, and a triangle test over
  // an index list that is not triangles reads three unrelated vertices as
  // a face and reports hits on shapes that were never drawn.
  if (drawMode != GL_TRIANGLES) {
    return false;
  }
  if (vertices == nullptr || indices == nullptr || numIndices < 3) {
    return false;
  }

  // Into the item's own space, where its vertices already are. The
  // direction is left unnormalized by the transform, which is what
  // carries the local distances back to world ones under a scaling.
  OkRay local = ray.transformed(glm::inverse(getTransformMatrix()));

  // The same sphere the frustum culls with. Most items fail here for the
  // cost of one quadratic, and never have a triangle read.
  if (!local.intersectsSphere(
          OkPoint(sphereCenter[0], sphereCenter[1], sphereCenter[2]), radius,
          nullptr)) {
    return false;
  }

  bool  found   = false;
  float nearest = 0.0f;
  for (long i = 0; i + 2 < numIndices; i += 3) {
    long first  = static_cast<long>(indices[i]) * VERTEX_STRIDE;
    long second = static_cast<long>(indices[i + 1]) * VERTEX_STRIDE;
    long third  = static_cast<long>(indices[i + 2]) * VERTEX_STRIDE;
    if (first + 2 >= numVertices || second + 2 >= numVertices ||
        third + 2 >= numVertices) {
      continue;  // an index past the end of the buffer: not a triangle
    }

    float distance = 0.0f;
    if (!local.intersectsTriangle(
            OkPoint(vertices[first], vertices[first + 1], vertices[first + 2]),
            OkPoint(vertices[second], vertices[second + 1],
                    vertices[second + 2]),
            OkPoint(vertices[third], vertices[third + 1], vertices[third + 2]),
            &distance)) {
      continue;
    }
    if (!found || distance < nearest) {
      found   = true;
      nearest = distance;
    }
  }

  if (found && outDistance != nullptr) {
    *outDistance = nearest;
  }
  return found;
}

/**
 * @brief Update the vertex data of an existing item safely.
 * @param newVertexData The new vertex data.
 * @param newVertexCount The number of new vertices.
 * @note This method updates the OpenGL buffers without recreating the item.
 */
void OkItem::updateVertexData(float *newVertexData, long newVertexCount) {
  if (!newVertexData || newVertexCount <= 0) {
    OkLogger::error("Item", "Invalid vertex data for update");
    return;
  }

  OkLogger::info("Item", "Updating vertex data for " + name + " from " +
                             std::to_string(numVertices) + " to " +
                             std::to_string(newVertexCount) + " vertices");

  // Free old vertex data
  delete[] vertices;

  // Same stride-5 contract as the constructor: normals recomputed
  // against the item's existing indices.
  _adoptVertexData(newVertexData, newVertexCount, indices, numIndices,
                   VERTEX_STRIDE_IN);

  // Recalculate radius with new geometry
  _calculateRadius();

  // Re-initialize OpenGL buffers with new data
  _initBuffers();

  OkLogger::info("Item", "Vertex data and OpenGL buffers updated successfully");
}
