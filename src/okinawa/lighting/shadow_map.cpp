#include "shadow_map.hpp"
#include "../config/config.hpp"
#include "../item/item.hpp"
#include "../math/frustum.hpp"
#include "../scene/scene.hpp"
#include "../shaders/shaders.hpp"
#include "../utils/assets.hpp"
#include "../utils/logger.hpp"
#include "lighting.hpp"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GLuint OkShadowMap::_fbo            = 0;
GLuint OkShadowMap::_depthTex       = 0;
GLuint OkShadowMap::_emptyShadowTex = 0;
GLuint OkShadowMap::_program        = 0;
int    OkShadowMap::_size           = 0;
// What the map was last drawn for, so an identical redraw is skipped.
bool      OkShadowMap::_neverDrawn = true;
float     OkShadowMap::_lastDir[3] = {0.0f, 0.0f, 0.0f};
float     OkShadowMap::_lastExtent[OkShadowMap::MAX_CASCADES] = {0.0f};
float     OkShadowMap::_lastCx[OkShadowMap::MAX_CASCADES]     = {0.0f};
float     OkShadowMap::_lastCz[OkShadowMap::MAX_CASCADES]     = {0.0f};
size_t    OkShadowMap::_lastObjects                           = 0;
int       OkShadowMap::_layers                                = 0;
int       OkShadowMap::_count                                 = 1;
// Out of line, because std::min binds its arguments by reference and so
// takes the constant's address. Declared inside the class it has a value
// but no storage, and the link fails only where something wants a
// reference to it.
const int OkShadowMap::MAX_CASCADES;

glm::mat4 OkShadowMap::_lightSpace[OkShadowMap::MAX_CASCADES];
float     OkShadowMap::_splitFar[OkShadowMap::MAX_CASCADES] = {0.0f};
float     OkShadowMap::_strength                            = 0.0f;

void OkShadowMap::initialize() {
  // NOLINTBEGIN(readability-magic-numbers)
  OkConfig::setBool("shadows", true);
  OkConfig::setInt("shadows.size", 2048);       // depth map resolution
  OkConfig::setFloat("shadows.extent", 90.0f);  // metres covered, half-width
  // How far the sun must turn before the depth map is redrawn, as
  // 1 - cos(angle). 0 redraws whenever the sun moves at all; the skip
  // still saves the pass when nothing is moving.
  //
  // The step has to move a shadow less than a texel or it is seen as
  // one. What matters is the far end of a long shadow: a 20 m building
  // with the sun 20 degrees up throws one 55 m long, and keeping its
  // tip inside a 2 cm texel needs about 0.018 degrees. Coarser than
  // that -- the 0.05 this used to be -- and the shadow visibly advances
  // in steps while the light on everything around it moves smoothly.
  OkConfig::setFloat("shadows.refresh.turn", 0.00000005f);
  // How far shadows are worth drawing. The shadowed box is fitted to
  // the camera's volume clipped to this, so it follows the view instead
  // of sitting on the viewer. Past it there are no cast shadows.
  OkConfig::setFloat("shadows.distance", 260.0f);
  // Cull the shadow pass against the light's volume. On by default;
  // off draws the whole scene into the map, which is what it did
  // before and is useful to tell a culling bug from a shadowing one.
  OkConfig::setBool("shadows.cull", true);
  // Debug view: paint every fragment by the cascade that shadowed it,
  // magenta where none could. An artefact that only appears while
  // moving cannot be read off a still -- a cascade handover, a hole in
  // a cascade's coverage and a bias washout all look like an edge going
  // soft, and they have nothing to do with each other. In this view
  // they are three different colours.
  OkConfig::setBool("shadows.debug", false);
  // Snap each cascade's box to whole texels. Off, which wants
  // explaining, since snapping is the usual cure for crawling edges.
  //
  // It cures them when the box TRANSLATES with the viewer: the grid
  // stays put in the world while the box slides over it. It can do
  // nothing when the light ROTATES, because then the grid rotates with
  // it -- and it adds an artefact of its own, since the correction
  // grows until it rolls over half a texel and the whole pattern jumps.
  // Under a moving sun that rollover is continuous, and it reads as a
  // shadow trembling its way across the ground.
  //
  // Measured against the thing it was there to prevent: with 2048
  // texels over a 20 m box a texel is 2 cm, and moving the viewer with
  // the snap off shows no crawling at all. So it buys nothing here and
  // costs the tremble. Kept as a switch rather than deleted -- a
  // coarser map, or a much larger box, would want it back.
  OkConfig::setBool("shadows.snap", false);
  // Normal offset: the receiver samples the map from slightly off its
  // own surface, which cures acne by moving the sample rather than the
  // shadow. It scales with the cascade's texel, since that is what sets
  // how coarse the comparison is -- and it is capped, in metres,
  // because past a few centimetres it stops curing anything and starts
  // pushing the sample out of the shadow the surface is standing in.
  OkConfig::setFloat("shadows.normaloffset", 1.0f);
  OkConfig::setFloat("shadows.normaloffset.max", 0.06f);
  // Cascades: how many bands the shadow distance is split into. More
  // bands means each one covers less ground and so resolves finer, at
  // one depth pass each.
  OkConfig::setInt("shadows.cascades", 3);
  // How the splits are spaced, 0 even and 1 logarithmic. The usual
  // answer is most of the way towards logarithmic, which packs
  // resolution into the first few metres -- and puts the first
  // changeover about twenty metres from the player, in the middle of
  // the ground they are walking over, with a threefold jump in texel
  // size across it. Two neighbouring cascades that differ that much
  // cannot be made to agree, and the seam rides along with the player.
  // Backing off towards even spacing costs some sharpness underfoot and
  // buys neighbours that resemble each other.
  OkConfig::setFloat("shadows.cascades.blend", 0.4f);
  OkConfig::setFloat("shadows.strength", 0.62f);
  // In metres: how far a surface has to be behind its own recorded
  // depth before it counts as shadowed. Divided per cascade by that
  // box's depth range before it reaches the shader, so every cascade
  // gets the same margin on the ground.
  OkConfig::setFloat("shadows.bias", 0.045f);
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
  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24, size, size, layers,
               0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
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
  glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _depthTex, 0,
                            0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    OkLogger::error("ShadowMap", "Depth framebuffer incomplete");
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  if (_program == 0) {
    _program =
        OkShader::createProgram(OkAssets::loadShaderSource("shadow.vert.glsl"),
                                OkAssets::loadShaderSource("shadow.frag.glsl"));
  }
  OkLogger::info("ShadowMap", "Depth map " + std::to_string(size) + "x" +
                                  std::to_string(size) + " x " +
                                  std::to_string(layers) + " cascade(s)");
}

/**
 * @brief Render the scene's depth from the light into the map.
 *
 *        The projection is orthographic (a directional light has no
 *        perspective) over a box centred on the viewer, and its origin
 *        is snapped to whole texels: without that snap the sampling
 *        grid slides under the geometry as the camera moves and every
 *        shadow edge shimmers.
 */
void OkShadowMap::render(OkScene *scene, const float *viewProj, float centreX,
                         float centreY, float centreZ) {
  if (scene == nullptr || !OkConfig::getBool("shadows")) {
    _strength = 0.0f;
    return;
  }
  const float *dir  = OkLighting::getSunDirection();
  float        elev = -dir[1];
  if (elev <= 0.02f) {
    _strength = 0.0f;  // light at or below the horizon
    return;
  }
  // Fade the shadows in as the light climbs: a source at the horizon
  // throws shadows too long and too hard to be believable.
  float fade = (elev - 0.02f) / 0.18f;
  fade = std::min(fade, 1.0f);
  _strength = OkConfig::getFloat("shadows.strength") * fade;

  int count = OkConfig::getInt("shadows.cascades");
  count = std::max(count, 1);
  count = std::min(count, MAX_CASCADES);
  _count = count;
  ensureTarget(OkConfig::getInt("shadows.size"), count);

  float shadowFar = OkConfig::getFloat("shadows.distance");
  if (shadowFar < 1.0f || viewProj == nullptr) {
    // Fitting off: one cascade over a fixed square on the viewer.
    _count    = 1;
    count     = 1;
    shadowFar = 0.0f;
  }

  // Where each cascade ends. Even spacing wastes the near cascade on
  // ground that is already close by; purely logarithmic makes the far
  // one enormous. The usual answer is a blend, mostly logarithmic.
  const float NEAR_START = 1.0f;
  float       blend      = OkConfig::getFloat("shadows.cascades.blend");
  for (int c = 0; c < count; c++) {
    float f      = static_cast<float>(c + 1) / static_cast<float>(count);
    float lin    = NEAR_START + (shadowFar - NEAR_START) * f;
    float lg     = NEAR_START * std::pow(shadowFar / NEAR_START, f);
    _splitFar[c] = lin + (lg - lin) * blend;
  }
  if (shadowFar <= 0.0f) {
    _splitFar[0] = 1e9f;  // the fixed box covers whatever it covers
  }

  glm::vec3 lightDir(dir[0], dir[1], dir[2]);
  glm::vec3 up = std::fabs(lightDir.y) > 0.98f ? glm::vec3(0.0f, 0.0f, 1.0f)
                                               : glm::vec3(0.0f, 1.0f, 0.0f);
  GLint     previousFbo = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFbo);
  // ...and the viewport with it. Drawing the map means resizing the
  // viewport to the map, and whatever draws next expects the window's.
  // Leaving it behind renders the whole frame into a 2048x2048 corner
  // of an 800x600 window: what reaches the screen is one hugely
  // magnified patch -- a flat colour -- and the interface falls off it
  // entirely. It went unnoticed because the post-process happens to set
  // the viewport again on its way in and out, so the damage only shows
  // when that pass is off or when it runs in the wrong order.
  GLint previousViewport[4] = {0, 0, 0, 0};
  glGetIntegerv(GL_VIEWPORT, previousViewport);
  bool bound = false;

  size_t objects = scene->getObjectCount();
  float  turn    = 1.0f - (dir[0] * _lastDir[0] + dir[1] * _lastDir[1] +
                           dir[2] * _lastDir[2]);
  float  turnMax = OkConfig::getFloat("shadows.refresh.turn");

  // The refresh is all or nothing, across every cascade at once.
  //
  // Deciding it per cascade lets them freeze at different moments, and
  // each then holds the sun as it was when its own turn came -- three
  // maps built from three directions a few degrees apart. Near a
  // changeover the two answers get mixed, so the shadow swings back and
  // forth between them as the blend moves: not a shimmer of texels but
  // a shadow visibly rocking through a few degrees. Whatever the maps
  // hold, they must all hold the same sun.
  bool redrawAll = _neverDrawn || turn > turnMax || objects != _lastObjects;

  // Pass one: work out where every cascade's box would sit this frame.
  // Nothing is committed yet -- the decision to redraw is taken once,
  // for all of them together, below.
  glm::mat4 candidate[MAX_CASCADES];
  float     candCx[MAX_CASCADES];
  float     candCz[MAX_CASCADES];
  float     candExtent[MAX_CASCADES];
  for (int c = 0; c < count; c++) {
    // Each cascade is a square centred on the viewer, sized by its own
    // band -- concentric, so the finest box sits inside the next.
    //
    // It used to be fitted to the camera's slice of the view instead,
    // which spends the resolution where the eye is looking and is the
    // usual arrangement. It was also wrong here, and visibly so: what
    // each box covered depended on where the camera pointed, so a wall
    // moved from the fine box to a coarse one as the player merely
    // turned or zoomed. Coarser box means a bigger offset on the
    // receiver's side (see the normal offset in the world shader) --
    // big enough, on a coarse cascade, to lift the sample clear of the
    // shadow the wall was standing in, so the shadow switched off. The
    // shadow of a building would end in mid-air and walk down the
    // ground as the player did.
    //
    // A sun shadow may not depend on where the camera is. Centring on
    // the viewer costs the half of each box that falls behind them, and
    // buys a shadow that only ever changes when the player moves.
    float extent = OkConfig::getFloat("shadows.extent");
    float focusX = centreX;
    float focusZ = centreZ;
    if (shadowFar > 0.0f) {
      extent = _splitFar[c];
    }
    // The box travels with the viewer, so it has to travel in whole
    // texels -- otherwise the grid the shadow is drawn on slides under
    // the world and every edge creeps as the player walks.
    //
    // The grid lies in the light's frame, not the world's, so the snap
    // belongs there too. Rounding the centre on the world axes, which
    // is what this did, only lands on a texel when the sun is straight
    // overhead; at any other hour the two frames are rotated with
    // respect to each other and the rounding achieves nothing. The
    // error is a function of position, so it does not jitter -- it
    // drifts, steadily, in the direction of travel. With the sun near
    // the horizon a fraction of a texel on the map becomes a long
    // stride on the ground, and the shadow visibly walks away from a
    // player walking towards it.
    //
    // So: build the box unsnapped, see where the world origin lands on
    // the map, and slide the projection by the fraction of a texel that
    // puts it on a whole one. Anchoring the origin rather than the
    // centre is what makes the grid world-fixed.
    float     depth = extent * 4.0f;
    glm::vec3 target(focusX, centreY, focusZ);
    glm::vec3 eye  = target - lightDir * (depth * 0.5f);
    glm::mat4 view = glm::lookAt(eye, target, up);
    glm::mat4 proj =
        glm::ortho(-extent, extent, -extent, extent, 0.1f, depth * 1.5f);

    glm::vec4 originLs = (proj * view) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    float     half     = static_cast<float>(_size) * 0.5f;
    float     ox       = originLs.x * half;
    float     oy       = originLs.y * half;
    float     dx       = std::floor(ox + 0.5f) - ox;
    float     dy       = std::floor(oy + 0.5f) - oy;
    if (!OkConfig::getBool("shadows.snap")) {
      // Off, to tell a snapping artefact from anything else. The snap
      // holds the grid still while the box TRANSLATES with the viewer;
      // it can do nothing about the grid ROTATING, which is what a
      // moving sun does to it, and its own rollover past half a texel
      // is a jump of its own.
      dx = 0.0f;
      dy = 0.0f;
    }
    proj[3][0] += dx / half;
    proj[3][1] += dy / half;

    // What the redraw test compares: where the box sits on the map,
    // which is what decides whether this cascade's picture would come
    // out any different.
    candidate[c]  = proj * view;
    candCx[c]     = ox + dx;
    candCz[c]     = oy + dy;
    candExtent[c] = extent;
    if (candCx[c] != _lastCx[c] || candCz[c] != _lastCz[c] ||
        candExtent[c] != _lastExtent[c]) {
      redrawAll = true;  // the viewer moved: every box moves with them
    }
  }

  // The matrix stays with the picture it addresses. Updating it every
  // frame while the map was only redrawn once the sun had turned far
  // enough meant looking up an old photograph with new coordinates, and
  // the shadow crept a texel at a time.
  if (!redrawAll) {
    return;
  }

  // Pass two: commit and draw. All of them, from the same sun.
  for (int c = 0; c < count; c++) {
    _lightSpace[c] = candidate[c];
    _lastCx[c]     = candCx[c];
    _lastCz[c]     = candCz[c];
    _lastExtent[c] = candExtent[c];

    if (!bound) {
      glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
      glViewport(0, 0, _size, _size);
      glUseProgram(_program);
      // Slope-scaled depth offset instead of front-face culling.
      // Culling front faces hides acne but records each caster as
      // starting at its back, so shadows visibly detach from the object
      // casting them (peter panning). A polygon offset that grows with
      // the surface's slope pushes only the problem cases, leaving
      // contact shadows in place.
      glEnable(GL_POLYGON_OFFSET_FILL);
      glPolygonOffset(2.2f, 3.5f);
      glDisable(GL_CULL_FACE);
      bound = true;
    }
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _depthTex, 0,
                              c);
    glClear(GL_DEPTH_BUFFER_BIT);
    glUniformMatrix4fv(glGetUniformLocation(_program, "lightSpace"), 1,
                       GL_FALSE, glm::value_ptr(_lightSpace[c]));

    // The camera's frustum is the wrong test here -- a caster behind
    // the viewer still casts into view -- but "no test at all" was
    // worse: the whole scene was drawn to fill a box a couple of hundred
    // metres across. The right test is the light's own volume, which
    // this orthographic box already is.
    OkFrustum lightFrustum;
    lightFrustum.setFromMatrix(_lightSpace[c]);
    const OkFrustum *saved = OkFrustum::getActive();
    OkFrustum::setActive(OkConfig::getBool("shadows.cull") ? &lightFrustum
                                                           : nullptr);
    OkItem::setShadowPass(true);
    scene->draw();
    OkItem::setShadowPass(false);
    OkFrustum::setActive(saved);
  }

  if (bound) {
    glDisable(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFbo));
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2],
               previousViewport[3]);
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
    // Nothing to sample -- but the sampler still has to have a
    // complete texture bound to it. A shader that declares a
    // sampler2DArray and
    // is drawn with nothing on that unit is an invalid operation in the
    // core profile, and every draw call in the frame fails: at night,
    // with the sun down and the strength at zero, that emptied the
    // whole screen. The shader skips the lookup anyway once
    // shadowStrength is 0, so a 1x1x1 stand-in is enough.
    if (_emptyShadowTex == 0) {
      glGenTextures(1, &_emptyShadowTex);
      glBindTexture(GL_TEXTURE_2D_ARRAY, _emptyShadowTex);
      glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24, 1, 1, 1, 0,
                   GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    }
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D_ARRAY, _emptyShadowTex);
    glActiveTexture(GL_TEXTURE0);
    GLint dloc = glGetUniformLocation(program, "shadowMap");
    if (dloc != -1) {
      glUniform1i(dloc, 3);
    }
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
    glUniformMatrix4fv(loc, _count, GL_FALSE, glm::value_ptr(_lightSpace[0]));
  }
  loc = glGetUniformLocation(program, "shadowSplit");
  if (loc != -1) {
    glUniform1fv(loc, _count, _splitFar);
  }
  loc = glGetUniformLocation(program, "shadowTexel");
  if (loc != -1) {
    glUniform1f(loc, 1.0f / static_cast<float>(_size));
  }
  // Depth bias, one per cascade, converted from metres.
  //
  // The comparison needs a margin, and the margin has to mean the same
  // thing in every cascade. Expressed in the map's own depth units it
  // does not: each cascade spans a different depth, so one number is a
  // few centimetres in the near box and three times that in the next.
  // The shadow then lands in a different place either side of the
  // changeover, and no amount of blending hides two shadows that are
  // not in the same place. Stated in metres and divided by each box's
  // own depth, every cascade gets the same margin on the ground.
  loc = glGetUniformLocation(program, "shadowBias");
  if (loc != -1) {
    float worldBias = OkConfig::getFloat("shadows.bias");
    float perCascade[MAX_CASCADES];
    for (int c = 0; c < _count; c++) {
      float range   = _lastExtent[c] * 6.0f;  // ortho far, see render()
      perCascade[c] = range > 0.0f ? worldBias / range : 0.0f;
    }
    glUniform1fv(loc, _count, perCascade);
  }
  // Debug view: paint each fragment by the cascade that shadowed it.
  // A shadow artefact seen while moving cannot be told apart in a
  // still -- a cascade handover, a coverage hole and a bias washout all
  // look like an edge going soft -- so the colour answers instead.
  loc = glGetUniformLocation(program, "shadowDebug");
  if (loc != -1) {
    glUniform1f(loc, OkConfig::getBool("shadows.debug") ? 1.0f : 0.0f);
  }
  loc = glGetUniformLocation(program, "shadowNormalOffset");
  if (loc != -1) {
    glUniform1f(loc, OkConfig::getFloat("shadows.normaloffset"));
  }
  loc = glGetUniformLocation(program, "shadowNormalOffsetMax");
  if (loc != -1) {
    glUniform1f(loc, OkConfig::getFloat("shadows.normaloffset.max"));
  }
  // World size of one shadow texel: the sampling point is nudged along
  // the receiving surface's normal by roughly this much, which removes
  // the remaining acne without moving the shadow along the ground.
  // World size of one texel, one per cascade: the far ones are much
  // coarser, and using the near one's figure for them leaves acne.
  loc = glGetUniformLocation(program, "shadowTexelWorld");
  if (loc != -1) {
    float perCascade[MAX_CASCADES];
    for (int c = 0; c < _count; c++) {
      perCascade[c] = (2.0f * _lastExtent[c]) / static_cast<float>(_size);
    }
    glUniform1fv(loc, _count, perCascade);
  }
}

float OkShadowMap::getStrength() {
  return _strength;
}

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
