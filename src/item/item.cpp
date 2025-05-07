#include "item.hpp"
#include "../config/config.hpp"
#include "../utils/logger.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
    : OkObject() {

  OkLogger::info("Item :: Creating item " + name + " with " +
                 std::to_string(vertexCount) + " vertices and " +
                 std::to_string(indexCount) + " indices");

  // Direct assignment - string will handle copying internally
  this->name = name;

  visible       = true;
  drawWireframe = false;
  numVertices   = vertexCount;
  numIndices    = indexCount;
  texture       = nullptr;

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

OkItem::~OkItem() {
  // Delete OpenGL objects
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &EBO);

  // Free allocated memory
  delete[] vertices;
  delete[] indices;
  delete texture;
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
  float minX = std::numeric_limits<float>::max();
  float maxX = std::numeric_limits<float>::lowest();
  float minY = std::numeric_limits<float>::max();
  float maxY = std::numeric_limits<float>::lowest();
  float minZ = std::numeric_limits<float>::max();
  float maxZ = std::numeric_limits<float>::lowest();

  for (long i = 0; i < numVertices * 5; i += 5) {  // 5 components per vertex
    minX = std::min(minX, vertices[i]);
    maxX = std::max(maxX, vertices[i]);
    minY = std::min(minY, vertices[i + 1]);
    maxY = std::max(maxY, vertices[i + 1]);
    minZ = std::min(minZ, vertices[i + 2]);
    maxZ = std::max(maxZ, vertices[i + 2]);
  }

  float width  = maxX - minX;
  float height = maxY - minY;
  float depth  = maxZ - minZ;
  radius       = std::max(std::max(width, height), depth);
}

/**
 * @brief Set the texture for the item.
 * @param texturePath The path to the texture file.
 */
void OkItem::setTexture(const std::string &texturePath) {
  if (texturePath.empty()) {
    OkLogger::error("Item :: Invalid texture path");
    return;
  }

  // Delete existing texture if any
  if (texture) {
    delete texture;
  }

  // Create new texture
  texture = new OkTexture(texturePath);
}

/**
 * @brief Get the speed of the item in 3D space.
 * @return The speed in 3D space.
 */
OkPoint OkItem::getSpeed() const {
  return speed;
}

/**
 * @brief Set the speed of the item in 3D space.
 * @param x The speed in the X direction.
 * @param y The speed in the Y direction.
 * @param z The speed in the Z direction.
 */
void OkItem::setSpeed(float x, float y, float z) {
  speed = OkPoint(x, y, z);
}

/**
 * @brief Get the speed magnitude of the item.
 * @return The speed magnitude in 3D space.
 */
float OkItem::getSpeedMagnitude() const {
  // Calculate speed magnitude in 3D
  return std::sqrt(speed.x() * speed.x() + speed.y() * speed.y() +
                   speed.z() * speed.z());
}

/**
 * @brief Update the item position and rotation based on speed and rotation
 * vectors.
 * @param dt The delta time since the last update.
 */
void OkItem::step(float dt) {
  float frameTime = dt / OkConfig::getFloat("graphics.time-per-frame");

  // Process movement if there's any speed
  if (speed.x() != 0 || speed.y() != 0 || speed.z() != 0) {
    move(speed.x() * frameTime, speed.y() * frameTime, speed.z() * frameTime);
  }

  // Process rotation if there's any rotational speed
  if (vRot.x() != 0 || vRot.y() != 0 || vRot.z() != 0) {
    rotate(vRot.x() * frameTime, vRot.y() * frameTime, vRot.z() * frameTime);
  }

  // Update children recursively
  OkObject *current = _firstChild;
  while (current != nullptr) {
    if (auto *item = dynamic_cast<OkItem *>(current)) {
      item->step(dt);
    }
    current = current->getNextSibling();
  }
}

void OkItem::updateTransform() {
  // Base class handles transform matrix updates
}

void OkItem::draw() {
  // Draw children recursively
  OkObject *current = _firstChild;
  while (current != nullptr) {
    if (auto *item = dynamic_cast<OkItem *>(current)) {
      item->draw();
    }
    current = current->getNextSibling();
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

    glDrawElements(GL_TRIANGLES, (GLsizei)numIndices, GL_UNSIGNED_INT, nullptr);
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

    glDrawElements(GL_TRIANGLES, (GLsizei)numIndices, GL_UNSIGNED_INT, nullptr);
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

    glDrawElements(GL_TRIANGLES, (GLsizei)numIndices, GL_UNSIGNED_INT, nullptr);
  }

  // Reset polygon mode to default if needed
  if (drawWireframe) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

  if (texture) {
    texture->unbind();
  }
}
