#include "skybox.hpp"
#include "../handlers/textures.hpp"
#include "../item/item.hpp"
#include "../item/texture.hpp"
#include "../item/billboard.hpp"
#include "../math/point.hpp"
#include "lighting.hpp"
#include <cmath>
#include <vector>

OkItem    *OkSkybox::_dome           = nullptr;
OkItem    *OkSkybox::_sunDisc        = nullptr;
OkTexture *OkSkybox::_sunTex         = nullptr;
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
      // Faces point inward (the camera lives inside the dome).
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
  // The sky is not an occluder; it is the light.
  _dome->setCastsShadow(false);
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
 * @brief The light's visible body: a camera-facing quad with a soft
 *        radial falloff, placed on the dome along the light's own
 *        direction, so what casts the shadows is what is seen in the
 *        sky.
 */
void OkSkybox::ensureSunDisc() {
  if (_sunDisc != nullptr) {
    return;
  }
  const int     SUN_TEX = 64;
  unsigned char rgba[SUN_TEX * SUN_TEX * 4];
  for (int y = 0; y < SUN_TEX; y++) {
    for (int x = 0; x < SUN_TEX; x++) {
      float dx = (x + 0.5f) / SUN_TEX - 0.5f;
      float dy = (y + 0.5f) / SUN_TEX - 0.5f;
      float d  = std::sqrt(dx * dx + dy * dy) * 2.0f;
      // A solid core inside a wide soft corona, which is how a bright
      // source reads through atmosphere.
      float core = 1.0f - d / 0.42f;
      if (core < 0.0f) {
        core = 0.0f;
      }
      float glow = 1.0f - d;
      if (glow < 0.0f) {
        glow = 0.0f;
      }
      float a = core + glow * glow * 0.55f;
      if (a > 1.0f) {
        a = 1.0f;
      }
      int off       = (y * SUN_TEX + x) * 4;
      rgba[off]     = 255;
      rgba[off + 1] = 255;
      rgba[off + 2] = 255;
      rgba[off + 3] = (unsigned char)(a * 255.0f);
    }
  }
  _sunTex = OkTextureHandler::getInstance()->createTextureFromRawData(
      "ok_sun", rgba, SUN_TEX, SUN_TEX, 4);

  // Angular size. The real sun subtends about half a degree; a disc
  // that small reads as a dot, so the core is drawn slightly larger and
  // the corona around it carries the perceived size. At the dome
  // distance below, this quad spans ~4 degrees with a ~1.7 degree core.
  const float S = 28.0f;
  float verts[20] = {-S, -S, 0.0f, 0.0f, 0.0f, S,  -S, 0.0f, 1.0f, 0.0f,
                     S,  S,  0.0f, 1.0f, 1.0f, -S, S,  0.0f, 0.0f, 1.0f};
  unsigned int idx[6] = {0, 1, 2, 0, 2, 3};
  _sunDisc = new OkItem("ok_sun_disc", verts, 20, idx, 6);
  _sunDisc->setCastsShadow(false);
  if (_sunTex != nullptr) {
    _sunDisc->setTexture("ok_sun", _sunTex);
  }
  _sunDisc->setAdditive(true);
  _sunDisc->setUnlit(true);
}

/**
 * @brief Place the sun on the dome along the cycle's own light
 *        direction, tinted by its current colour, fading out as it
 *        sinks below the horizon.
 */
void OkSkybox::drawSun(float camX, float camY, float camZ) {
  ensureSunDisc();
  if (_sunDisc == nullptr) {
    return;
  }
  const float *dir = OkLighting::getSunDirection();
  // The direction points from the light toward the scene, so the body
  // sits the other way.
  float sx  = -dir[0];
  float sy  = -dir[1];
  float sz  = -dir[2];
  float len = std::sqrt(sx * sx + sy * sy + sz * sz);
  if (len < 1e-6f) {
    return;
  }
  sx /= len;
  sy /= len;
  sz /= len;

  float horizon = sy + 0.06f;
  if (horizon <= 0.0f) {
    return;               // below the horizon: nothing to draw
  }
  float fade = horizon / 0.20f;
  if (fade > 1.0f) {
    fade = 1.0f;
  }

  const float DIST = 820.0f;   // inside the dome
  OkPoint     pos(camX + sx * DIST, camY + sy * DIST, camZ + sz * DIST);
  _sunDisc->setPosition(pos.x(), pos.y(), pos.z());
  _sunDisc->setRotation(
      OkBillboard::facingRotation(pos, OkPoint(camX, camY, camZ)));

  const float *col = OkLighting::getSunColor();
  float        r = col[0], g = col[1], b = col[2];
  float        m = r > g ? (r > b ? r : b) : (g > b ? g : b);
  if (m < 0.35f) {
    // Deep dusk: the curve's colour has gone dark, but the body itself
    // should still read as a light, only a red one.
    r = 1.0f;
    g = 0.45f;
    b = 0.25f;
  }
  _sunDisc->setTintColor(r, g, b, fade);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  _sunDisc->draw();
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_BLEND);
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
  drawSun(camX, camY, camZ);
  glEnable(GL_CULL_FACE);
  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);
}

void OkSkybox::shutdown() {
  delete _dome;
  delete _sunDisc;
  _dome     = nullptr;
  _sunDisc  = nullptr;
  _gradient = nullptr;  // owned by the texture handler
  _sunTex   = nullptr;
}
