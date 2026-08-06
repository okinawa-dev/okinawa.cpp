#include "shadow_map.hpp"
#include "../config/config.hpp"
#include "../math/frustum.hpp"
#include "../scene/scene.hpp"
#include "../shaders/shaders.hpp"
#include "../utils/assets.hpp"
#include "../utils/logger.hpp"
#include "lighting.hpp"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GLuint    OkShadowMap::_fbo      = 0;
GLuint    OkShadowMap::_depthTex = 0;
GLuint    OkShadowMap::_program  = 0;
int       OkShadowMap::_size     = 0;
// What the map was last drawn for, so an identical redraw is skipped.
bool      OkShadowMap::_neverDrawn  = true;
float     OkShadowMap::_lastDir[3]  = {0.0f, 0.0f, 0.0f};
float     OkShadowMap::_lastExtent[OkShadowMap::MAX_CASCADES] = {0.0f};
float     OkShadowMap::_lastCx[OkShadowMap::MAX_CASCADES]     = {0.0f};
float     OkShadowMap::_lastCz[OkShadowMap::MAX_CASCADES]     = {0.0f};
size_t    OkShadowMap::_lastObjects = 0;
int       OkShadowMap::_layers      = 0;
int       OkShadowMap::_count       = 1;
glm::mat4 OkShadowMap::_lightSpace[OkShadowMap::MAX_CASCADES];
float     OkShadowMap::_splitFar[OkShadowMap::MAX_CASCADES]   = {0.0f};
float     OkShadowMap::_strength   = 0.0f;

void OkShadowMap::initialize() {
  // NOLINTBEGIN(readability-magic-numbers)
  OkConfig::setBool("shadows", true);
  OkConfig::setInt("shadows.size", 2048);      // depth map resolution
  OkConfig::setFloat("shadows.extent", 90.0f); // metres covered, half-width
  // How far the sun must turn before the depth map is redrawn, as
  // 1 - cos(angle). The default is about 0.05 degrees: enough to move a
  // shadow edge by one texel at 100 m, and no more. 0 redraws every
  // frame.
  OkConfig::setFloat("shadows.refresh.turn", 0.0000004f);
  // How far shadows are worth drawing. The shadowed box is fitted to
  // the camera's volume clipped to this, so it follows the view instead
  // of sitting on the viewer. Past it there are no cast shadows.
  OkConfig::setFloat("shadows.distance", 260.0f);
  // Cull the shadow pass against the light's volume. On by default;
  // off draws the whole scene into the map, which is what it did
  // before and is useful to tell a culling bug from a shadowing one.
  OkConfig::setBool("shadows.cull", true);
  // Cascades: how many bands the shadow distance is split into. More
  // bands means each one covers less ground and so resolves finer, at
  // one depth pass each.
  OkConfig::setInt("shadows.cascades", 3);
  // How the splits are spaced, 0 even and 1 logarithmic. Even spacing
  // wastes the near cascade on ground that is already close; purely
  // logarithmic makes the far cascade enormous. The usual answer is
  // most of the way towards logarithmic.
  OkConfig::setFloat("shadows.cascades.blend", 0.75f);
  OkConfig::setFloat("shadows.strength", 0.62f);
  OkConfig::setFloat("shadows.bias", 0.00035f);
  // NOLINTEND(readability-magic-numbers)
  OkLogger::info("ShadowMap", "Config defaults registered");
}

void OkShadowMap::ensureTarget(int size, int layers) {
  if (_fbo != 0 && size == _size && layers == _layers) {
    return;
  }
  if (_fbo != 0) {
    glDeleteFramebuffers(1, &_fbo);
    glDeleteTextures(1, &_depthTex);
  }
  _size   = size;
  _layers = layers;

  // One array texture, a layer per cascade: the world pass then needs a
  // single sampler however many cascades there are.
  glGenTextures(1, &_depthTex);
  glBindTexture(GL_TEXTURE_2D_ARRAY, _depthTex);
  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24, size, size,
               layers, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // Outside the map there is no shadow: clamp to a border of "far away".
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  float border[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, border);

  // The layer is attached per cascade at draw time.
  glGenFramebuffers(1, &_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _depthTex,
                            0, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    OkLogger::error("ShadowMap", "Depth framebuffer incomplete");
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  if (_program == 0) {
    _program = OkShader::createProgram(
        OkAssets::loadShaderSource("shadow.vert.glsl"),
        OkAssets::loadShaderSource("shadow.frag.glsl"));
  }
  OkLogger::info("ShadowMap",
                 "Depth map " + std::to_string(size) + "x" +
                     std::to_string(size) + " x " + std::to_string(layers) +
                     " cascade(s)");
}

/**
 * @brief Render the scene's depth from the light into the map.
 *
 *        The projection is orthographic (a directional light has no
 *        perspective) over a box centred on the viewer, and its origin
 *        is snapped to whole texels IN LIGHT SPACE: without that snap
 *        the sampling grid slides under the geometry as the camera
 *        moves and every shadow edge travels with it. Snapping on the
 *        world axes is not enough -- the grid lies in the light's frame,
 *        so the two only agree with the sun straight overhead.
 */
void OkShadowMap::render(OkScene *scene, const float *viewProj,
                         float centreX, float centreY, float centreZ) {
  if (scene == nullptr || !OkConfig::getBool("shadows")) {
    _strength = 0.0f;
    return;
  }
  const float *dir  = OkLighting::getSunDirection();
  float        elev = -dir[1];
  if (elev <= 0.02f) {
    _strength = 0.0f;   // light at or below the horizon
    return;
  }
  // Fade the shadows in as the light climbs: a source at the horizon
  // throws shadows too long and too hard to be believable.
  float fade = (elev - 0.02f) / 0.18f;
  if (fade > 1.0f) {
    fade = 1.0f;
  }
  _strength = OkConfig::getFloat("shadows.strength") * fade;

  int count = OkConfig::getInt("shadows.cascades");
  if (count < 1) {
    count = 1;
  }
  if (count > MAX_CASCADES) {
    count = MAX_CASCADES;
  }
  _count = count;
  ensureTarget(OkConfig::getInt("shadows.size"), count);

  float shadowFar = OkConfig::getFloat("shadows.distance");
  if (shadowFar < 1.0f || viewProj == nullptr) {
    // Fitting off: one cascade over a fixed square on the viewer.
    _count     = 1;
    count      = 1;
    shadowFar  = 0.0f;
  }

  // Where each cascade ends. Even spacing wastes the near cascade on
  // ground that is already close by; purely logarithmic makes the far
  // one enormous. The usual answer is a blend, mostly logarithmic.
  const float NEAR_START = 1.0f;
  float       blend      = OkConfig::getFloat("shadows.cascades.blend");
  for (int c = 0; c < count; c++) {
    float f    = (float)(c + 1) / (float)count;
    float lin  = NEAR_START + (shadowFar - NEAR_START) * f;
    float lg   = NEAR_START * std::pow(shadowFar / NEAR_START, f);
    _splitFar[c] = lin + (lg - lin) * blend;
  }
  if (shadowFar <= 0.0f) {
    _splitFar[0] = 1e9f;   // the fixed box covers whatever it covers
  }

  glm::vec3 lightDir(dir[0], dir[1], dir[2]);
  glm::vec3 up = std::fabs(lightDir.y) > 0.98f ? glm::vec3(0.0f, 0.0f, 1.0f)
                                               : glm::vec3(0.0f, 1.0f, 0.0f);
  glm::mat4 invViewProj(1.0f);
  if (shadowFar > 0.0f) {
    invViewProj = glm::inverse(glm::make_mat4(viewProj));
  }

  GLint previousFbo = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFbo);
  bool bound = false;

  size_t objects = scene->getObjectCount();
  float  turn    = 1.0f - (dir[0] * _lastDir[0] + dir[1] * _lastDir[1] +
                        dir[2] * _lastDir[2]);
  float  turnMax = OkConfig::getFloat("shadows.refresh.turn");

  for (int c = 0; c < count; c++) {
    // Fit this cascade to ITS band of the camera's volume.
    //
    // The box is sized from the band's BOUNDING SPHERE rather than a
    // tight fit: a tight box changes size as the camera turns, and with
    // it the world size of a texel, so every shadow edge crawls between
    // texels frame to frame. A sphere's radius is the same from any
    // angle, so only its centre moves -- and that is snapped below.
    float extent = OkConfig::getFloat("shadows.extent");
    float focusX = centreX;
    float focusZ = centreZ;
    if (shadowFar > 0.0f) {
      float bandNear = (c == 0) ? NEAR_START : _splitFar[c - 1];
      float bandFar  = _splitFar[c];
      glm::vec3 corner[8];
      int       k = 0;
      for (int xi = 0; xi < 2; xi++) {
        for (int yi = 0; yi < 2; yi++) {
          for (int zi = 0; zi < 2; zi++) {
            glm::vec4 p = invViewProj * glm::vec4(xi ? 1.0f : -1.0f,
                                                  yi ? 1.0f : -1.0f,
                                                  zi ? 1.0f : -1.0f, 1.0f);
            corner[k++] = glm::vec3(p) / p.w;
          }
        }
      }
      // Rescale each corner ray to this band's near and far.
      glm::vec3 eyeP(centreX, centreY, centreZ);
      glm::vec3 banded[8];
      for (int i = 0; i < 4; i++) {
        // corners come in near/far pairs along z (zi is the inner loop)
        glm::vec3 nearC = corner[i * 2];
        glm::vec3 farC  = corner[i * 2 + 1];
        glm::vec3 ray   = glm::normalize(farC - nearC);
        glm::vec3 base  = nearC;
        banded[i]     = base + ray * bandNear;
        banded[i + 4] = base + ray * bandFar;
      }
      glm::vec3 centre(0.0f);
      for (int i = 0; i < 8; i++) {
        centre += banded[i];
      }
      centre /= 8.0f;
      float radius = 0.0f;
      for (int i = 0; i < 8; i++) {
        float d = glm::length(banded[i] - centre);
        if (d > radius) {
          radius = d;
        }
      }
      focusX = centre.x;
      focusZ = centre.z;
      extent = radius;
      (void)eyeP;
    }

    // Snap the box to whole texels, IN LIGHT SPACE.
    //
    // The map's texel grid lies in the light's own frame, not the
    // world's, so rounding the focus point on the world axes does not
    // land it on a texel unless the sun happens to be straight
    // overhead. It used to be rounded on X and Z in world units, and the
    // viewer's height went in unrounded on top of that, so the sampling
    // grid slid under the ground as the viewer walked and every shadow
    // edge crawled with them -- worst with a low sun, where the ground
    // is grazed and a texel of slide becomes many centimetres of visible
    // edge.
    //
    // So: build the box unsnapped, see where its origin lands on the
    // map, and translate the projection by the fraction of a texel
    // needed to put it on a whole one.
    float     depth = extent * 4.0f;
    glm::vec3 target(focusX, centreY, focusZ);
    glm::vec3 eye  = target - lightDir * (depth * 0.5f);
    glm::mat4 view = glm::lookAt(eye, target, up);
    glm::mat4 proj =
        glm::ortho(-extent, extent, -extent, extent, 0.1f, depth * 1.5f);

    glm::vec4 originLs = (proj * view) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    originLs *= (float)_size * 0.5f;
    glm::vec4 offset =
        (glm::vec4(std::floor(originLs.x + 0.5f), std::floor(originLs.y + 0.5f),
                   originLs.z, originLs.w) -
         originLs) *
        (2.0f / (float)_size);
    proj[3][0] += offset.x;
    proj[3][1] += offset.y;
    _lightSpace[c] = proj * view;

    // What the redraw test compares: the snapped position on the map,
    // which is what actually decides whether this cascade's picture
    // would come out any different.
    float cx = originLs.x + offset.x * (float)_size * 0.5f;
    float cz = originLs.y + offset.y * (float)_size * 0.5f;

    // Redraw this cascade only when its picture would differ: static
    // geometry under a slow sun looks the same from frame to frame.
    bool dirty = _neverDrawn || turn > turnMax || objects != _lastObjects ||
                 cx != _lastCx[c] || cz != _lastCz[c] ||
                 extent != _lastExtent[c];
    if (!dirty) {
      continue;
    }
    _lastCx[c]     = cx;
    _lastCz[c]     = cz;
    _lastExtent[c] = extent;

    if (!bound) {
      glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
      glViewport(0, 0, _size, _size);
      glUseProgram(_program);
      // Slope-scaled depth offset instead of front-face culling.
      // Culling front faces hides acne but records each caster as
      // starting at its BACK, so shadows visibly detach from the object
      // casting them (peter panning). A polygon offset that grows with
      // the surface's slope pushes only the problem cases, leaving
      // contact shadows in place.
      glEnable(GL_POLYGON_OFFSET_FILL);
      glPolygonOffset(2.2f, 3.5f);
      glDisable(GL_CULL_FACE);
      bound = true;
    }
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _depthTex,
                              0, c);
    glClear(GL_DEPTH_BUFFER_BIT);
    glUniformMatrix4fv(glGetUniformLocation(_program, "lightSpace"), 1,
                       GL_FALSE, glm::value_ptr(_lightSpace[c]));

    // The camera's frustum is the wrong test here -- a caster behind
    // the viewer still casts into view -- but "no test at all" was
    // worse: the whole city was drawn to fill a box a couple of hundred
    // metres across. The right test is the LIGHT's own volume, which
    // this orthographic box already is.
    OkFrustum        lightFrustum;
    lightFrustum.setFromMatrix(_lightSpace[c]);
    const OkFrustum *saved = OkFrustum::getActive();
    OkFrustum::setActive(OkConfig::getBool("shadows.cull") ? &lightFrustum
                                                           : nullptr);
    scene->draw();
    OkFrustum::setActive(saved);
  }

  if (bound) {
    glDisable(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)previousFbo);
    _neverDrawn  = false;
    _lastDir[0]  = dir[0];
    _lastDir[1]  = dir[1];
    _lastDir[2]  = dir[2];
    _lastObjects = objects;
  }
}

void OkShadowMap::bind(GLuint program) {
  GLint strengthLoc = glGetUniformLocation(program, "shadowStrength");
  if (strengthLoc != -1) {
    glUniform1f(strengthLoc, _strength);
  }
  if (_strength <= 0.0f || _depthTex == 0) {
    return;
  }
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D_ARRAY, _depthTex);
  glActiveTexture(GL_TEXTURE0);

  GLint loc = glGetUniformLocation(program, "shadowMap");
  if (loc != -1) {
    glUniform1i(loc, 3);
  }
  loc = glGetUniformLocation(program, "shadowCascades");
  if (loc != -1) {
    glUniform1i(loc, _count);
  }
  loc = glGetUniformLocation(program, "lightSpace");
  if (loc != -1) {
    glUniformMatrix4fv(loc, _count, GL_FALSE,
                       glm::value_ptr(_lightSpace[0]));
  }
  loc = glGetUniformLocation(program, "shadowSplit");
  if (loc != -1) {
    glUniform1fv(loc, _count, _splitFar);
  }
  loc = glGetUniformLocation(program, "shadowTexel");
  if (loc != -1) {
    glUniform1f(loc, 1.0f / (float)_size);
  }
  loc = glGetUniformLocation(program, "shadowBias");
  if (loc != -1) {
    glUniform1f(loc, OkConfig::getFloat("shadows.bias"));
  }
  // World size of one shadow texel: the sampling point is nudged along
  // the receiving surface's normal by roughly this much, which removes
  // the remaining acne without moving the shadow along the ground.
  // World size of one texel PER CASCADE: the far ones are much coarser,
  // and using the near one's figure for them leaves acne.
  loc = glGetUniformLocation(program, "shadowTexelWorld");
  if (loc != -1) {
    float perCascade[MAX_CASCADES];
    for (int c = 0; c < _count; c++) {
      perCascade[c] = (2.0f * _lastExtent[c]) / (float)_size;
    }
    glUniform1fv(loc, _count, perCascade);
  }
}

float OkShadowMap::getStrength() { return _strength; }

void OkShadowMap::shutdown() {
  if (_fbo != 0) {
    glDeleteFramebuffers(1, &_fbo);
    glDeleteTextures(1, &_depthTex);
    _fbo = 0;
  }
  if (_program != 0) {
    glDeleteProgram(_program);
    _program = 0;
  }
}
