
#include "camera.hpp"
#include "../config/config.hpp"
#include "../core/gl_config.hpp"
#include "core.hpp"
#include "core/object.hpp"
#include "math/point.hpp"
#include "math/rotation.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>
#include <string>

/**
 * @brief Constructor for the OkCamera class.
 *        Initializes the camera with a given width and height.
 * @param width  The width of the viewport.
 * @param height The height of the viewport.
 */
OkCamera::OkCamera(const std::string &name, int width, int height)
    : OkObject(name) {
  // Initialize matrices
  view       = glm::mat4(1.0f);
  projection = glm::mat4(1.0f);

  // Set camera properties
  aspectRatio = static_cast<float>(width) / static_cast<float>(height);
  fov         = 45.0f;
  near        = 0.1f;
  far         = 100.0f;

  // Set default movement parameters (inherited from OkObject)
  maxVel = 500.0f;   // Maximum velocity in units per second
  accel  = 2000.0f;  // Acceleration in units per second squared

  // Set default rotation parameters
  maxVRot = OkPoint(2.0f, 2.0f, 2.0f);  // Maximum rotation speed in radians/sec
  accelRot =
      OkPoint(8.0f, 8.0f, 8.0f);  // Rotation acceleration in radians/sec^2

  // Create projection matrix
  projection = glm::perspective(glm::radians(fov), aspectRatio, near, far);

  updateView();
}

/**
 * @brief Set the perspective projection matrix.
 *        This method allows you to set a custom perspective projection.
 * @param fovDegrees The field of view in degrees.
 * @param nearPlane  The near clipping plane distance.
 * @param farPlane   The far clipping plane distance.
 */
void OkCamera::setPerspective(float fovDegrees, float nearPlane,
                              float farPlane) {
  fov        = fovDegrees;
  near       = nearPlane;
  far        = farPlane;
  projection = glm::perspective(glm::radians(fov), aspectRatio, near, far);
}

/**
 * @brief Update the view matrix based on the camera's position and direction.
 *        This method recalculates the view matrix using the current position,
 *        front vector, and up vector.
 */
void OkCamera::updateView() {
  OkPoint   worldPos = getPosition();  // Get transformed world position
  glm::vec3 pos(worldPos.x(), worldPos.y(), worldPos.z());

  OkRotation worldRot = getRotation();  // Get transformed world rotation
  OkPoint    forward  = worldRot.getForwardVector();
  OkPoint    up       = worldRot.getUpVector();  // Get up vector from rotation
  glm::vec3  frontVec = forward.toVec3();
  glm::vec3  upVec    = up.toVec3();  // Use rotated up vector
  view                = glm::lookAt(pos, pos + frontVec, upVec);
}

/**
 * @brief Update the camera's transform.
 */
void OkCamera::updateTransformSelf() {
  // Update the view matrix when transform changes
  updateView();
}

/**
 * @brief Step function for the camera.
 *        This method is called every frame to update the camera's state.
 * @param dt The time since the last frame in seconds.
 */
void OkCamera::stepSelf(float dt) {
  // Call parent's step function
  // OkObject::step(dt);

  // Update the view matrix
  updateView();
}

/**
 * @brief Draw the camera.
 */
void OkCamera::drawSelf() {

  // if this is the active camera
  if (this == OkCore::getCamera()) {
    // Draw item hierarchy (interface, gui, etc)
    // OkObject::draw();
    return;
  }

  // if this is not the active camera
  // Render camera visualization for debugging
  if (OkConfig::getBool("graphics.drawCameras")) {

    // Only draw camera visualization if it's not the active camera
    if (this != OkCore::getCamera()) {
      // Create camera body vertices (cube). All gizmo coordinates are multiples
      // of `size`, so this scales the whole gizmo proportionally about its
      // centre (the camera position). Configurable via "camera.gizmo-size"
      // (default 0.25 -> ~0.5 m cube body).
      float size = OkConfig::getFloat("camera.gizmo-size");
      // 13 vertices, each x, y, z plus texture u, v.
      const size_t                    GIZMO_FLOATS = 65;
      std::array<float, GIZMO_FLOATS> vertices     = {
          // Camera body - cube vertices (x, y, z, u, v)
          -size, -size, -size, 0.0f, 0.0f,  // 0
          -size, size, -size, 0.0f, 1.0f,   // 1
          size, size, -size, 1.0f, 1.0f,    // 2
          size, -size, -size, 1.0f, 0.0f,   // 3
          -size, -size, size, 0.0f, 0.0f,   // 4
          -size, size, size, 0.0f, 1.0f,    // 5
          size, size, size, 1.0f, 1.0f,     // 6
          size, -size, size, 1.0f,
          0.0f,  // 7
                 // Pyramid vertices for lens (now at -z)
          0.0f, 0.0f, -size, 0.5f,
          1.0f,  // 8 - pyramid tip (attached to cube)
          size, size, -size * 2, 1.0f, 0.0f,    // 9  - pyramid base
          size, -size, -size * 2, 1.0f, 1.0f,   // 10
          -size, -size, -size * 2, 0.0f, 1.0f,  // 11
          -size, size, -size * 2, 0.0f, 0.0f    // 12
      };

      // Rest of indices array unchanged...
      // Six cube faces and four pyramid sides, two triangles each
      // except the pyramid, which is one per side.
      const size_t                            GIZMO_INDICES = 48;
      std::array<unsigned int, GIZMO_INDICES> indices       = {
          // Cube indices
          0, 1, 2, 0, 2, 3,  // Front
          4, 5, 6, 4, 6, 7,  // Back
          0, 4, 7, 0, 7, 3,  // Bottom
          1, 5, 6, 1, 6, 2,  // Top
          0, 1, 5, 0, 5, 4,  // Left
          3, 2, 6, 3, 6, 7,  // Right
                             // Pyramid indices
          8, 9, 10,          // Pyramid sides
          8, 10, 11, 8, 11, 12, 8, 12, 9};

      // Get current shader program
      GLint current_program;
      glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
      if (current_program == 0)
        return;

      // Set the model matrix uniform using the inverse of the view matrix
      // This ensures the visualization matches exactly what the camera sees
      GLint modelLoc = glGetUniformLocation(current_program, "model");
      if (modelLoc != -1) {
        glm::mat4 invView = glm::inverse(view);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(invView));
      }

      // Disable texturing for camera visualization
      GLint hasTexLoc = glGetUniformLocation(current_program, "hasTexture");
      if (hasTexLoc != -1) {
        glUniform1i(hasTexLoc, 0);
      }

      // Set wireframe color
      GLint colorLoc = glGetUniformLocation(current_program, "wireframeColor");
      if (colorLoc != -1) {
        glUniform4f(colorLoc, 0.2f, 0.8f, 0.2f,
                    1.0f);  // Green color for camera
      }

      // Create and bind temporary VAO/VBO/EBO
      GLuint VAO;
      GLuint VBO;
      GLuint EBO;
      glGenVertexArrays(1, &VAO);
      glGenBuffers(1, &VBO);
      glGenBuffers(1, &EBO);

      glBindVertexArray(VAO);

      // Buffer vertex data
      glBindBuffer(GL_ARRAY_BUFFER, VBO);
      glBufferData(GL_ARRAY_BUFFER,
                   static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                   vertices.data(), GL_STATIC_DRAW);

      // Buffer index data
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
      glBufferData(
          GL_ELEMENT_ARRAY_BUFFER,
          static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
          indices.data(), GL_STATIC_DRAW);

      // Set up vertex attributes
      // Position attribute
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                            nullptr);
      glEnableVertexAttribArray(0);

      // Texture coord attribute
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                            reinterpret_cast<void *>(3 * sizeof(float)));
      glEnableVertexAttribArray(1);

      // Draw in wireframe mode
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()),
                     GL_UNSIGNED_INT, nullptr);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

      // Clean up
      glDeleteVertexArrays(1, &VAO);
      glDeleteBuffers(1, &VBO);
      glDeleteBuffers(1, &EBO);
    }
  }
}

void OkCamera::updateForTarget(const OkObject *target, float dt) {
  // Base camera does not track anything; subclasses override.
  (void)target;
  (void)dt;
}

void OkCamera::look(float yawDeg, float pitchDeg) {
  // Free-fly: rotate self. Convention: +yaw looks right, +pitch looks up. Pitch
  // is clamped to avoid flipping.
  OkRotation  rot      = getRotation();
  float       pitch    = rot.getPitch() + glm::radians(pitchDeg);
  float       yaw      = rot.getYaw() - glm::radians(yawDeg);
  const float maxPitch = glm::radians(89.0f);
  pitch                = std::min(pitch, maxPitch);
  pitch                = std::max(pitch, -maxPitch);
  setRotation(pitch, yaw, rot.getRoll());
}
