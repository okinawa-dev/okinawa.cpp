#include "skybox.hpp"
#include "../handlers/textures.hpp"
#include "../item/item.hpp"
#include "../item/texture.hpp"
#include "lighting.hpp"
#include <cmath>
#include <vector>

OkItem    *OkSkybox::_dome           = nullptr;
OkTexture *OkSkybox::_gradient       = nullptr;
float      OkSkybox::_builtFog[3]    = {-1.0f, -1.0f, -1.0f};
float      OkSkybox::_builtZenith[3] = {-1.0f, -1.0f, -1.0f};

// Dome shape: radius (well inside the far plane), ring elevations from
// slightly below the horizon to the zenith, and segments around.
// NOLINTBEGIN(readability-magic-numbers)
static const float SKY_RADIUS   = 900.0f;
static const float SKY_RINGS[]  = {-0.08f, 0.10f, 0.35f, 0.75f, 1.5708f};
static const int   SKY_RING_N   = 5;
static const int   SKY_SEGMENTS = 24;
// Gradient texture height (1 px wide, sampled by elevation).
static const int SKY_GRAD_H = 64;
// Colour drift that triggers a gradient refresh.
static const float SKY_EPS = 0.004f;
// NOLINTEND(readability-magic-numbers)

/**
 * @brief Build the dome mesh: rings of vertices from below the horizon to
 *        the zenith, UV v = 0 at the bottom ring and 1 at the top, so the
 *        1-D gradient texture paints the sky by elevation.
 */
void OkSkybox::ensure() {
  if (_dome != nullptr) {
    return;
  }

  std::vector<float>        verts;
  std::vector<unsigned int> idx;

  for (int r = 0; r < SKY_RING_N; r++) {
    float elev = SKY_RINGS[r];
    float y    = std::sin(elev) * SKY_RADIUS;
    float rad  = std::cos(elev) * SKY_RADIUS;
    float v    = (float)r / (float)(SKY_RING_N - 1);
    for (int s = 0; s < SKY_SEGMENTS; s++) {
      float a = (float)s / (float)SKY_SEGMENTS * 6.2831853f;
      verts.push_back(std::cos(a) * rad);
      verts.push_back(y);
      verts.push_back(std::sin(a) * rad);
      verts.push_back(0.5f);
      verts.push_back(v);
    }
  }
  for (int r = 0; r + 1 < SKY_RING_N; r++) {
    for (int s = 0; s < SKY_SEGMENTS; s++) {
      unsigned int a0 = (unsigned int)(r * SKY_SEGMENTS + s);
      unsigned int a1 =
          (unsigned int)(r * SKY_SEGMENTS + (s + 1) % SKY_SEGMENTS);
      unsigned int b0 = a0 + (unsigned int)SKY_SEGMENTS;
      unsigned int b1 = a1 + (unsigned int)SKY_SEGMENTS;
      // Faces point INWARD (the camera lives inside the dome).
      idx.push_back(a0);
      idx.push_back(b0);
      idx.push_back(a1);
      idx.push_back(a1);
      idx.push_back(b0);
      idx.push_back(b1);
    }
  }

  _dome = new OkItem("ok_skybox", verts.data(), (long)verts.size(),
                     idx.data(), (long)idx.size());
  refreshGradient();
}

/**
 * @brief Regenerate the 1-D gradient (horizon = fog colour, top = zenith)
 *        when the cycle's colours drifted since the last build.
 */
void OkSkybox::refreshGradient() {
  const float *fog    = OkLighting::getFogColor();
  const float *zenith = OkLighting::getSkyZenith();

  float drift = 0.0f;
  for (int c = 0; c < 3; c++) {
    drift += std::fabs(fog[c] - _builtFog[c]);
    drift += std::fabs(zenith[c] - _builtZenith[c]);
  }
  if (_gradient != nullptr && drift < SKY_EPS) {
    return;
  }

  unsigned char rgba[SKY_GRAD_H * 4];
  for (int i = 0; i < SKY_GRAD_H; i++) {
    // v=0 (texture bottom) is the horizon row.
    float t = (float)i / (float)(SKY_GRAD_H - 1);
    for (int c = 0; c < 3; c++) {
      float col = fog[c] + (zenith[c] - fog[c]) * t;
      rgba[i * 4 + c] = (unsigned char)(col * 255.0f);
    }
    rgba[i * 4 + 3] = 255;
  }

  if (_gradient == nullptr) {
    _gradient = OkTextureHandler::getInstance()->createTextureFromRawData(
        "ok_skybox_gradient", rgba, 1, SKY_GRAD_H, 4);
    if (_dome != nullptr && _gradient != nullptr) {
      _dome->setTexture("ok_skybox_gradient", _gradient);
    }
  } else {
    _gradient->updateRawData(rgba, 1, SKY_GRAD_H);
  }
  for (int c = 0; c < 3; c++) {
    _builtFog[c]    = fog[c];
    _builtZenith[c] = zenith[c];
  }
}

/**
 * @brief Draw the dome centred on the camera, depth writes off, so the
 *        whole scene paints over it and it never occludes anything.
 */
void OkSkybox::draw(float camX, float camY, float camZ) {
  ensure();
  refreshGradient();
  if (_dome == nullptr) {
    return;
  }

  _dome->setPosition(camX, camY, camZ);
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  glDisable(GL_CULL_FACE);
  _dome->draw();
  glEnable(GL_CULL_FACE);
  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);
}

void OkSkybox::shutdown() {
  delete _dome;
  _dome     = nullptr;
  _gradient = nullptr;  // owned by the texture handler
}
