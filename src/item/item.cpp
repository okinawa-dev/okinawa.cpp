#include "item.hpp"
#include "../config/config.hpp"
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
               unsigned int *indexData, long indexCount)
    : OkObject(name) {

  OkLogger::info("Item :: Creating item " + name + " with " +
                 std::to_string(vertexCount) + " vertices and " +
                 std::to_string(indexCount) + " indices");

  visible       = true;
  drawWireframe = false;
  drawMode      = GL_TRIANGLES;  // Default drawing mode

  numVertices = vertexCount;
  numIndices  = indexCount;
  texture     = nullptr;
  textureName = "";

  // Allocate and copy vertex data
  vertices = new float[vertexCount];
  std::memcpy(vertices, vertexData, vertexCount * sizeof(float));
  numVertices = vertexCount;

  // Allocate and copy index data
  indices = new unsigned int[indexCount];
  std::memcpy(indices, indexData, indexCount * sizeof(unsigned int));
  numIndices = indexCount;

  _calculateRadius();

  _initBuffers();
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
}

/**
 * @brief Initialize OpenGL buffers for the item.
 */
void OkItem::_initBuffers() {
  // Generate and bind VAO first
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  // Generate and set up VBO
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(numVertices * sizeof(float)),
               vertices, GL_STATIC_DRAW);

  // Position attribute (3 floats)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  // Texture coords attribute (2 floats)
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (GLvoid *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

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
    OkLogger::warning("Item :: No vertices to calculate radius");
    return;
  }

  float minX = vertices[0];
  float maxX = vertices[0];
  float minY = vertices[1];
  float maxY = vertices[1];
  float minZ = vertices[2];
  float maxZ = vertices[2];

  // Each vertex has 5 components: 3 for position (xyz) and 2 for UV
  const int  stride            = 5;
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

  OkLogger::info("Item :: Bounds: (" + std::to_string(minX) + ", " +
                 std::to_string(minY) + ", " + std::to_string(minZ) + ") to (" +
                 std::to_string(maxX) + ", " + std::to_string(maxY) + ", " +
                 std::to_string(maxZ) + ")");
  OkLogger::info("Item :: Calculated radius: " + std::to_string(radius));
}

/**
 * @brief Set the texture for the item.
 * @param texturePath The path to the texture file.
 */
void OkItem::loadTextureFromFile(const std::string &texturePath) {
  if (texturePath.empty()) {
    OkLogger::error("Item :: Invalid texture path");
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
void OkItem::stepSelf(float dt) {
  // Call parent class step function first
  // OkObject::step(dt);
}

/**
 * @brief Update the transform matrix of the item.
 */
void OkItem::updateTransformSelf() {
  // Log transform update for debugging
  // OkLogger::info("Item :: Updating transform for " + name + " at position ("
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

  bool drawWireframe =
      OkConfig::getBool("graphics.wireframe") || this->drawWireframe;
  bool drawTexture =
      OkConfig::getBool("graphics.textures") && texture && texture->isLoaded();

  // Verify we have a valid shader program
  GLint current_program;
  glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
  if (current_program == 0) {
    OkLogger::error("Item :: No shader program in use");
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
    OkLogger::error("Item :: Cannot find model uniform in shader");
    return;
  }
  glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

  // Verify we have valid buffers
  if (VAO == 0) {
    OkLogger::error("Item :: No VAO for item: " + name);
    return;
  }

  // Bind VAO and draw
  glBindVertexArray(VAO);
  if (glGetError() != GL_NO_ERROR) {
    OkLogger::error("Item :: Error binding VAO for item: " + name);
    return;
  }

  // Draw textured model
  if (drawTexture) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glActiveTexture(GL_TEXTURE0);
    texture->bind();

    // Set uniform for texture sampler
    GLint texLoc = glGetUniformLocation(current_program, "texture0");
    if (texLoc != -1) {
      glUniform1i(texLoc, 0);  // Tell shader to use texture unit 0
    } else {
      OkLogger::error("Item :: Cannot find texture0 uniform in shader");
    }

    // Set hasTexture flag
    GLint hasTexLoc = glGetUniformLocation(current_program, "hasTexture");
    if (hasTexLoc != -1) {
      glUniform1i(hasTexLoc, 1);
    }

    glDrawElements(drawMode, (GLsizei)numIndices, GL_UNSIGNED_INT, nullptr);
  }

  // Second pass: Draw wireframe
  if (drawWireframe) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    GLint hasTexLoc = glGetUniformLocation(current_program, "hasTexture");
    if (hasTexLoc != -1) {
      glUniform1i(hasTexLoc, 0);
    }

    GLint colorLoc = glGetUniformLocation(current_program, "wireframeColor");
    if (colorLoc != -1) {
      glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    glDrawElements(drawMode, (GLsizei)numIndices, GL_UNSIGNED_INT, nullptr);
  }

  // Fallback if no texture and no wireframe
  if (!drawTexture && !drawWireframe) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    GLint hasTexLoc = glGetUniformLocation(current_program, "hasTexture");
    if (hasTexLoc != -1) {
      glUniform1i(hasTexLoc, 0);
    }

    GLint colorLoc = glGetUniformLocation(current_program, "wireframeColor");
    if (colorLoc != -1) {
      glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    glDrawElements(drawMode, (GLsizei)numIndices, GL_UNSIGNED_INT, nullptr);
  }

  // Reset polygon mode to default if needed
  if (drawWireframe) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

  if (texture) {
    // static call to unbind texture, opengl does not need to know which
    // texture was bound before, it will just unbind the currently bound
    // texture with
    // glBindTexture(GL_TEXTURE_2D, 0);
    OkTexture::unbind();
  }
}
