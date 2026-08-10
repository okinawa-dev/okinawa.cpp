#include "instanced_item.hpp"
#include "../config/config.hpp"
#include "../lighting/lighting.hpp"
#include "../math/frustum.hpp"
#include "../utils/logger.hpp"
#include "texture.hpp"
#include <cmath>
#include <glm/gtc/type_ptr.hpp>

// Per-instance attribute layout, starting after the mesh attributes
// (0 position, 1 uv, 2 normal): 3 = position+scale, 4 = orientation.
static const int OK_INST_ATTR_POS   = 3;
static const int OK_INST_ATTR_ORIENT = 4;
static const int OK_INST_FLOATS      = 8;

OkInstancedItem::OkInstancedItem(const std::string &name, float *vertexData,
                                 long vertexCount, unsigned int *indexData,
                                 long indexCount, int vertexStride)
    : OkItem(name, vertexData, vertexCount, indexData, indexCount,
             vertexStride) {
  _instanceVbo = 0;
  _drawnCount  = 0;
}

OkInstancedItem::~OkInstancedItem() {
  if (_instanceVbo != 0) {
    glDeleteBuffers(1, &_instanceVbo);
    _instanceVbo = 0;
  }
}

int OkInstancedItem::addInstance(float x, float y, float z, float yaw,
                                 float scale) {
  Instance inst;
  inst.x       = x;
  inst.y       = y;
  inst.z       = z;
  inst.yaw     = yaw;
  inst.scale   = scale;
  inst.visible = true;
  _instances.push_back(inst);
  return (int)_instances.size() - 1;
}

void OkInstancedItem::setInstance(int index, float x, float y, float z,
                                  float yaw, float scale) {
  if (index < 0 || index >= (int)_instances.size()) {
    return;
  }
  _instances[(size_t)index].x     = x;
  _instances[(size_t)index].y     = y;
  _instances[(size_t)index].z     = z;
  _instances[(size_t)index].yaw   = yaw;
  _instances[(size_t)index].scale = scale;
}

void OkInstancedItem::setInstanceVisible(int index, bool visible) {
  if (index < 0 || index >= (int)_instances.size()) {
    return;
  }
  _instances[(size_t)index].visible = visible;
}

void OkInstancedItem::clearInstances() {
  _instances.clear();
}

/**
 * @brief Create the per-instance buffer and wire its attributes into the
 *        mesh VAO with an attribute divisor of 1 (advance once per
 *        instance instead of once per vertex).
 */
void OkInstancedItem::ensureInstanceBuffer() {
  if (_instanceVbo != 0) {
    return;
  }
  glGenBuffers(1, &_instanceVbo);
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, _instanceVbo);

  // vec4: world position + uniform scale
  glVertexAttribPointer(OK_INST_ATTR_POS, 4, GL_FLOAT, GL_FALSE,
                        OK_INST_FLOATS * sizeof(float), nullptr);
  glEnableVertexAttribArray(OK_INST_ATTR_POS);
  glVertexAttribDivisor(OK_INST_ATTR_POS, 1);

  // vec4: cos(yaw), sin(yaw), spare, spare
  glVertexAttribPointer(OK_INST_ATTR_ORIENT, 4, GL_FLOAT, GL_FALSE,
                        OK_INST_FLOATS * sizeof(float),
                        (GLvoid *)(4 * sizeof(float)));
  glEnableVertexAttribArray(OK_INST_ATTR_ORIENT);
  glVertexAttribDivisor(OK_INST_ATTR_ORIENT, 1);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/**
 * @brief Draw every visible instance in ONE call. Instances are frustum
 *        culled individually (mesh bounding sphere at the instance
 *        position), so a scene full of lamps only uploads what is on
 *        screen.
 */
void OkInstancedItem::drawSelf() {
  if (!visible || _instances.empty()) {
    _drawnCount = 0;
    return;
  }

  GLint currentProgram = 0;
  glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
  if (currentProgram == 0) {
    return;
  }

  // Collect the visible, on-screen instances.
  const OkFrustum *frustum = OkFrustum::getActive();
  _uploadScratch.clear();
  _uploadScratch.reserve(_instances.size() * OK_INST_FLOATS);
  for (size_t i = 0; i < _instances.size(); i++) {
    const Instance &inst = _instances[i];
    if (!inst.visible) {
      continue;
    }
    if (frustum != nullptr) {
      float cx = inst.x + sphereCenter[0] * inst.scale;
      float cy = inst.y + sphereCenter[1] * inst.scale;
      float cz = inst.z + sphereCenter[2] * inst.scale;
      if (OkFrustum::isBeyondDrawDistance(cx, cy, cz,
                                          radius * inst.scale) ||
          !frustum->containsSphere(cx, cy, cz, radius * inst.scale)) {
        OkFrustum::addCulled();
        continue;
      }
    }
    _uploadScratch.push_back(inst.x);
    _uploadScratch.push_back(inst.y);
    _uploadScratch.push_back(inst.z);
    _uploadScratch.push_back(inst.scale);
    _uploadScratch.push_back(std::cos(inst.yaw));
    _uploadScratch.push_back(std::sin(inst.yaw));
    _uploadScratch.push_back(0.0f);
    _uploadScratch.push_back(0.0f);
  }
  _drawnCount = (int)(_uploadScratch.size() / OK_INST_FLOATS);
  if (_drawnCount == 0) {
    return;
  }

  ensureInstanceBuffer();
  glBindBuffer(GL_ARRAY_BUFFER, _instanceVbo);
  glBufferData(GL_ARRAY_BUFFER,
               (GLsizeiptr)(_uploadScratch.size() * sizeof(float)),
               _uploadScratch.data(), GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // The instance transform replaces the model matrix: pass identity and
  // turn the shader's instancing path on.
  glm::mat4 identity = glm::mat4(1.0f);
  GLint     modelLoc = glGetUniformLocation(currentProgram, "model");
  if (modelLoc != -1) {
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(identity));
  }
  GLint instLoc = glGetUniformLocation(currentProgram, "instanced");
  if (instLoc != -1) {
    glUniform1i(instLoc, 1);
  }
  if (unlit) {
    GLint litLoc  = glGetUniformLocation(currentProgram, "lightingOn");
    GLint tintLoc = glGetUniformLocation(currentProgram, "sceneTint");
    if (litLoc != -1) {
      glUniform1f(litLoc, 0.0f);
    }
    if (tintLoc != -1) {
      glUniform3f(tintLoc, 1.0f, 1.0f, 1.0f);
    }
  }

  bool drawTexture = OkConfig::getBool("graphics.textures") && texture &&
                     texture->isLoaded();
  GLint hasTexLoc = glGetUniformLocation(currentProgram, "hasTexture");
  GLint tintLoc   = glGetUniformLocation(currentProgram, "tintColor");
  if (tintLoc != -1) {
    glUniform4f(tintLoc, tintColor[0], tintColor[1], tintColor[2],
                tintColor[3]);
  }
  // The cross-fade uniform has to be written even when this item is not
  // fading. It is program state, not object state: leaving it alone
  // means inheriting whatever the last object drawn happened to set,
  // so a solid object drawn after a dissolving one comes out dithered.
  {
    GLint fadeLoc = glGetUniformLocation(currentProgram, "itemFade");
    if (fadeLoc != -1) {
      glUniform1f(fadeLoc, fade);
    }
    GLint fadeInvLoc = glGetUniformLocation(currentProgram,
                                            "itemFadeInvert");
    if (fadeInvLoc != -1) {
      glUniform1f(fadeInvLoc, fadeInverted ? 1.0f : 0.0f);
    }
  }
  if (drawTexture) {
    glActiveTexture(GL_TEXTURE0);
    texture->bind();
    GLint texLoc = glGetUniformLocation(currentProgram, "texture0");
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
    GLint colorLoc = glGetUniformLocation(currentProgram, "wireframeColor");
    if (colorLoc != -1) {
      glUniform4f(colorLoc, fillColor[0], fillColor[1], fillColor[2],
                  fillColor[3]);
    }
  }

  glBindVertexArray(VAO);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glDrawElementsInstanced(drawMode, (GLsizei)numIndices, GL_UNSIGNED_INT,
                          nullptr, (GLsizei)_drawnCount);
  OkFrustum::addDraw((numIndices / 3) * _drawnCount);

  // Wireframe overlay, on the same instanced draw. One pass covers
  // every instance: without it a whole class of objects -- the ones
  // drawn in bulk, which is where a mesh problem is most likely to
  // repeat -- silently stays solid while the rest of the scene shows
  // its triangles. Unlit, like the plain item's overlay.
  bool wire = drawWireframe ||
              (wireframeGlobal && OkConfig::getBool("graphics.wireframe"));
  if (wire) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    GLint wLitLoc = glGetUniformLocation(currentProgram, "lightingOn");
    if (wLitLoc != -1) {
      glUniform1f(wLitLoc, 0.0f);
    }
    if (hasTexLoc != -1) {
      glUniform1i(hasTexLoc, 0);
    }
    GLint colorLoc = glGetUniformLocation(currentProgram, "wireframeColor");
    if (colorLoc != -1) {
      glUniform4f(colorLoc, wireframeColor[0], wireframeColor[1],
                  wireframeColor[2], 1.0f);
    }
    glDrawElementsInstanced(drawMode, (GLsizei)numIndices, GL_UNSIGNED_INT,
                            nullptr, (GLsizei)_drawnCount);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if (wLitLoc != -1) {
      glUniform1f(wLitLoc, 1.0f);
    }
  }
  glBindVertexArray(0);

  if (texture) {
    OkTexture::unbind();
  }
  if (instLoc != -1) {
    glUniform1i(instLoc, 0);
  }
  if (unlit) {
    GLint        litLoc = glGetUniformLocation(currentProgram, "lightingOn");
    GLint        stLoc  = glGetUniformLocation(currentProgram, "sceneTint");
    const float *wt     = OkLighting::getSceneTint();
    if (litLoc != -1) {
      glUniform1f(litLoc, 1.0f);
    }
    if (stLoc != -1) {
      glUniform3f(stLoc, wt[0], wt[1], wt[2]);
    }
  }
}
