#include "item.hpp"
#include "../config/config.hpp"
#include "../lighting/lighting.hpp"
#include "../math/frustum.hpp"
#include "../core/gl_config.hpp"
#include "../handlers/textures.hpp"
#include "../utils/logger.hpp"
#include "core/object.hpp"
#include "item/texture.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

/**
 * @brief Create a new item with the given name, vertices, and indices.
 * @param name        The name of the item.
 * @param vertexData  The vertex data.
 * @param vertexCount The number of vertices.
 * @param indexData   The index data.
 * @param indexCount  The number of indices.
 */
OkItem::OkItem(const std::string &name, float *vertexData, long vertexCount,
               unsigned int *indexData, long indexCount, int vertexStride)
    : OkObject(name) {

  OkLogger::info("Item", "Creating item " + name + " with " +
                             std::to_string(vertexCount) + " vertices and " +
                             std::to_string(indexCount) + " indices");

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

  numIndices  = indexCount;
  texture     = nullptr;
  textureName = "";
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
  nearLightCount  = 0;
  nearLightGen    = -1;

  _adoptVertexData(vertexData, vertexCount, indexData, indexCount,
                   vertexStride);

  // Allocate and copy index data
  indices = new unsigned int[indexCount];
  std::memcpy(indices, indexData, indexCount * sizeof(unsigned int));
  numIndices = indexCount;

  _calculateRadius();

  _initBuffers();
}

/**
 * @brief Store vertex data with the internal stride-8 layout
 *        (x,y,z,u,v,nx,ny,nz). Stride-5 input (the historical layout
 *        every caller uses) is expanded here: normals are computed by
 *        accumulating the face normal of every triangle onto its three
 *        vertices. De-indexed meshes (each face owns its vertices, the
 *        city loaders' convention) end up with EXACT flat face normals;
 *        indexed meshes with shared vertices (terrain grids) end up with
 *        area-weighted smooth normals. Non-triangle index lists (lines,
 *        points) produce garbage normals, which is fine: the shader only
 *        lights the textured branch, and debug lines are never textured.
 */
void OkItem::_adoptVertexData(float *vertexData, long vertexCount,
                              unsigned int *indexData, long indexCount,
                              int vertexStride) {
  if (vertexStride == 8) {
    vertices = new float[vertexCount];
    std::memcpy(vertices, vertexData, vertexCount * sizeof(float));
    numVertices = vertexCount;
  } else {
    long vcount = vertexCount / 5;
    vertices    = new float[vcount * 8];
    for (long i = 0; i < vcount; i++) {
      long src = i * 5;
      long dst = i * 8;
      vertices[dst]     = vertexData[src];
      vertices[dst + 1] = vertexData[src + 1];
      vertices[dst + 2] = vertexData[src + 2];
      vertices[dst + 3] = vertexData[src + 3];
      vertices[dst + 4] = vertexData[src + 4];
      vertices[dst + 5] = 0.0f;
      vertices[dst + 6] = 0.0f;
      vertices[dst + 7] = 0.0f;
    }
    for (long f = 0; f + 2 < indexCount; f += 3) {
      long ia = (long)indexData[f];
      long ib = (long)indexData[f + 1];
      long ic = (long)indexData[f + 2];
      if (ia >= vcount || ib >= vcount || ic >= vcount) {
        continue;
      }
      float ax = vertexData[ia * 5], ay = vertexData[ia * 5 + 1],
            az = vertexData[ia * 5 + 2];
      float ux = vertexData[ib * 5] - ax, uy = vertexData[ib * 5 + 1] - ay,
            uz = vertexData[ib * 5 + 2] - az;
      float wx = vertexData[ic * 5] - ax, wy = vertexData[ic * 5 + 1] - ay,
            wz = vertexData[ic * 5 + 2] - az;
      // Area-weighted face normal (unnormalized cross product).
      float nx = uy * wz - uz * wy;
      float ny = uz * wx - ux * wz;
      float nz = ux * wy - uy * wx;
      long  tri[3];
      tri[0] = ia;
      tri[1] = ib;
      tri[2] = ic;
      for (int k = 0; k < 3; k++) {
        vertices[tri[k] * 8 + 5] += nx;
        vertices[tri[k] * 8 + 6] += ny;
        vertices[tri[k] * 8 + 7] += nz;
      }
    }
    for (long i = 0; i < vcount; i++) {
      float *n  = &vertices[i * 8 + 5];
      float  nl = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
      if (nl > 1e-12f) {
        n[0] /= nl;
        n[1] /= nl;
        n[2] /= nl;
      } else {
        // Cancelled out (double-sided quads) or never touched: face up.
        n[0] = 0.0f;
        n[1] = 1.0f;
        n[2] = 0.0f;
      }
    }
    numVertices = vcount * 8;
  }
}

/**
 * @brief Destructor for the OkItem class.
 *        Cleans up OpenGL objects and allocated memory.
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
  glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(numVertices * sizeof(float)),
               vertices, GL_STATIC_DRAW);

  // Position attribute (3 floats)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  // Texture coords attribute (2 floats)
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (GLvoid *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Normal attribute (3 floats)
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (GLvoid *)(5 * sizeof(float)));
  glEnableVertexAttribArray(2);

  // Generate and set up EBO
  glGenBuffers(1, &EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               (GLsizeiptr)(numIndices * sizeof(unsigned int)), indices,
               GL_STATIC_DRAW);

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
void OkItem::_calculateRadius() {

  // Return early if no vertices
  if (numVertices <= 0 || !vertices) {
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

  // Each vertex has 8 components: position (xyz), UV, normal (xyz)
  const int  stride            = 8;
  const long actualVertexCount = numVertices / stride;

  // Iterate through actual vertices
  for (long i = 0; i < actualVertexCount; i++) {
    long  offset = i * stride;
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
  radius = sqrt(width * width + height * height + depth * depth) * 0.5f;
  // Bounding-sphere centre in LOCAL coords: city meshes keep their
  // vertices in chunk-local space with the item origin at the chunk
  // corner, so the sphere must be centred on the bbox, not the origin.
  sphereCenter[0] = (minX + maxX) * 0.5f;
  sphereCenter[1] = (minY + maxY) * 0.5f;
  sphereCenter[2] = (minZ + maxZ) * 0.5f;

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
void OkItem::setShadowPass(bool on) { g_shadowPass = on; }
bool OkItem::inShadowPass() { return g_shadowPass; }

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
  glm::vec4 wc = tm * glm::vec4(sphereCenter[0], sphereCenter[1],
                                sphereCenter[2], 1.0f);

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
    if (OkFrustum::isBeyondDrawDistance(wc.x, wc.y, wc.z,
                                        radius * sx0)) {
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
    float s  = std::max(sx, std::max(sy, sz));
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
          wc.x, wc.y, wc.z, nearLights, OkLighting::MAX_LIGHTS_PER_ITEM);
      nearLightGen = OkLighting::getLightGeneration();
    }
    GLint cntLoc = glGetUniformLocation(current_program, "pointLightCount");
    if (cntLoc != -1) {
      glUniform1i(cntLoc, nearLightCount);
      for (int i = 0; i < nearLightCount; i++) {
        const float *lp = OkLighting::getLightPosition(nearLights[i]);
        const float *lc = OkLighting::getLightColor(nearLights[i]);
        const float *ld = OkLighting::getLightDirection(nearLights[i]);
        float        lr = OkLighting::getLightRadius(nearLights[i]);
        float        cc = OkLighting::getLightCosCone(nearLights[i]);
        float        li = OkLighting::getLightIntensity(nearLights[i]);
        std::string  base = "pointLights[" + std::to_string(i) + "]";
        GLint        pLoc = glGetUniformLocation(current_program,
                                                 (base + ".posRadius").c_str());
        GLint        cLoc = glGetUniformLocation(current_program,
                                                 (base + ".color").c_str());
        GLint        sLoc = glGetUniformLocation(current_program,
                                                 (base + ".spot").c_str());
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
  // Always runs, so an item can show a flat fill AND a wireframe overlay.
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    GLint hasTexLoc = glGetUniformLocation(current_program, "hasTexture");
    GLint tintLoc   = glGetUniformLocation(current_program, "tintColor");
    if (tintLoc != -1) {
      glUniform4f(tintLoc, tintColor[0], tintColor[1], tintColor[2],
                  tintColor[3]);
    }
    GLint maskLoc = glGetUniformLocation(current_program,
                                         "maskedMaterials");
    if (maskLoc != -1) {
      glUniform1f(maskLoc, maskedMaterials ? 1.0f : 0.0f);
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
        cachedInvert  = glGetUniformLocation(current_program,
                                             "itemFadeInvert");
      }
      if (cachedFade != -1) {
        glUniform1f(cachedFade, fade);
      }
      if (cachedInvert != -1) {
        glUniform1f(cachedInvert, fadeInverted ? 1.0f : 0.0f);
      }
    }
    if (maskedMaterials) {
      const char *names[3] = {"matTintA", "matTintB", "matTintC"};
      for (int i = 0; i < 3; i++) {
        GLint loc = glGetUniformLocation(current_program, names[i]);
        if (loc != -1) {
          glUniform4f(loc, matTint[i][0], matTint[i][1], matTint[i][2],
                      1.0f);
        }
      }
      GLint lumaLoc = glGetUniformLocation(current_program,
                                           "matLuminance");
      if (lumaLoc != -1) {
        glUniform3f(lumaLoc, matLuma[0], matLuma[1], matLuma[2]);
      }
    }
    GLint texLoc   = glGetUniformLocation(current_program, "texture0");
    GLint colorLoc = glGetUniformLocation(current_program, "wireframeColor");
    bool  texturesOn = OkConfig::getBool("graphics.textures");

    // One pass per material slot, over the same buffers: the transform,
    // the culling and every uniform above are done once for the whole
    // item, and only the texture changes between ranges. An item with
    // no slots draws its whole index buffer with its own texture, which
    // is the common case.
    size_t passes = materials.empty() ? 1 : materials.size();
    for (size_t mi = 0; mi < passes; mi++) {
      OkTexture *tex   = materials.empty() ? texture : materials[mi].texture;
      long       first = materials.empty() ? 0 : materials[mi].first;
      long       count = materials.empty() ? numIndices : materials[mi].count;
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
      glDrawElements(drawMode, (GLsizei)count, GL_UNSIGNED_INT,
                     (const void *)(first * (long)sizeof(unsigned int)));
      OkFrustum::addDraw(count / 3);
    }
  }

  // Wireframe overlay pass (in the wireframe colour, on top of the fill).
  // Always UNLIT: the overlay is a drawing aid, not a surface.
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

    glDrawElements(drawMode, (GLsizei)numIndices, GL_UNSIGNED_INT, nullptr);
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
    GLint litLoc  = glGetUniformLocation(current_program, "lightingOn");
    GLint tintLoc = glGetUniformLocation(current_program, "sceneTint");
    const float *wt = OkLighting::getSceneTint();
    if (litLoc != -1) {
      glUniform1f(litLoc, 1.0f);
    }
    if (tintLoc != -1) {
      glUniform3f(tintLoc, wt[0], wt[1], wt[2]);
    }
  }
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
  _adoptVertexData(newVertexData, newVertexCount, indices, numIndices, 5);

  // Recalculate radius with new geometry
  _calculateRadius();

  // Re-initialize OpenGL buffers with new data
  _initBuffers();

  OkLogger::info("Item", "Vertex data and OpenGL buffers updated successfully");
}
