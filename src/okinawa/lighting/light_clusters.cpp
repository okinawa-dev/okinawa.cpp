#include "light_clusters.hpp"
#include "../config/config.hpp"
#include "../utils/logger.hpp"
#include "lighting.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <utility>
#include <vector>

namespace {

  // GPU-side light record: 3 vec4 per light (see the shader struct).
  const int LIGHT_FLOATS = 12;

  GLuint g_lightTex = 0;  // buffer texture: light records
  GLuint g_lightTbo = 0;
  GLuint g_indexTex = 0;  // buffer texture: light indices per cluster
  GLuint g_indexTbo = 0;
  GLuint g_gridTex  = 0;  // buffer texture: (offset, count) per cluster
  GLuint g_gridTbo  = 0;

  std::vector<float>            g_lightData;
  std::vector<int>              g_indexData;
  std::vector<int>              g_gridData;
  std::vector<std::vector<int>> g_clusterLists;

  int g_visibleLights = 0;
  int g_lightRefs     = 0;

  // View-space depth of a slice boundary. Exponential distribution:
  // z(k) = near * (far/near)^(k / SLICES).
  float sliceDepth(int k, float nearPlane, float farPlane) {
    float t = (float)k / (float)OkLightClusters::CLUSTERS_Z;
    return nearPlane * std::pow(farPlane / nearPlane, t);
  }

}  // namespace

void OkLightClusters::initialize() {
  OkConfig::setBool("lighting.clustered", true);
  // Depth range the cluster grid spans, in world units (see the header).
  OkConfig::setFloat("lighting.cluster.near", 1.0f);
  OkConfig::setFloat("lighting.cluster.far", 350.0f);
  g_clusterLists.resize(CLUSTER_COUNT);
  OkLogger::info("LightClusters", "Config defaults registered");
}

void OkLightClusters::ensureBuffers() {
  if (g_lightTbo != 0) {
    return;
  }
  glGenBuffers(1, &g_lightTbo);
  glGenTextures(1, &g_lightTex);
  glGenBuffers(1, &g_indexTbo);
  glGenTextures(1, &g_indexTex);
  glGenBuffers(1, &g_gridTbo);
  glGenTextures(1, &g_gridTex);
}

/**
 * @brief Assign every light that reaches the frustum to the clusters its
 *        sphere touches, then upload the light records, the index list
 *        and the per-cluster (offset, count) table.
 *
 *        A light's screen-space extent is computed by projecting its
 *        bounding sphere: the tile range comes from the projected
 *        bounding box, the depth slice range from its view-space z
 *        extent. This over-estimates slightly (a box around a sphere),
 *        which is the standard trade-off: cheap and conservative.
 */
void OkLightClusters::update(const glm::mat4 &view, const glm::mat4 &projection,
                             float nearPlane, float farPlane) {
  if (!OkConfig::getBool("lighting.clustered")) {
    g_visibleLights = 0;
    g_lightRefs     = 0;
    return;
  }
  ensureBuffers();

  // Clustering uses its own depth range (see the header): the camera's
  // 0.1 m near plane would make the first slices microscopic.
  nearPlane = OkConfig::getFloat("lighting.cluster.near");
  farPlane  = OkConfig::getFloat("lighting.cluster.far");

  for (int i = 0; i < CLUSTER_COUNT; i++) {
    g_clusterLists[(size_t)i].clear();
  }
  g_lightData.clear();
  g_visibleLights = 0;

  // Order lights by view distance BEFORE assigning them: clusters have a
  // per-cluster cap, and filling it with far lights would starve the
  // near ones that actually model the scene (the lamp overhead losing
  // to a dozen lamps in a row).
  int                                total = OkLighting::getLightCount();
  std::vector<std::pair<float, int>> ordered;
  ordered.reserve((size_t)total);
  for (int li = 0; li < total; li++) {
    const float *lp = OkLighting::getLightPosition(li);
    float        lr = OkLighting::getLightRadius(li);
    glm::vec4    vp = view * glm::vec4(lp[0], lp[1], lp[2], 1.0f);
    float        vz = -vp.z;
    if (vz + lr < nearPlane || vz - lr > farPlane) {
      continue;
    }
    // Order by DISTANCE to the eye, not by signed view depth: a lamp
    // behind the camera has negative depth and would sort first,
    // filling the near clusters before the lamp overhead gets in.
    float dist = std::sqrt(vp.x * vp.x + vp.y * vp.y + vp.z * vp.z);
    ordered.push_back(std::make_pair(dist, li));
  }
  std::sort(ordered.begin(), ordered.end());

  for (size_t oi = 0; oi < ordered.size(); oi++) {
    if (g_visibleLights >= MAX_VISIBLE_LIGHTS) {
      break;
    }
    int          li = ordered[oi].second;
    const float *lp = OkLighting::getLightPosition(li);
    float        lr = OkLighting::getLightRadius(li);
    glm::vec4    vp = view * glm::vec4(lp[0], lp[1], lp[2], 1.0f);
    float        vz = -vp.z;  // distance in front of the camera

    // Screen-space bounds of the sphere: project the centre and the
    // extents of its bounding box. Conservative and cheap.
    float minX = 1.0f, maxX = -1.0f, minY = 1.0f, maxY = -1.0f;
    bool  any = false;
    for (int c = 0; c < 8; c++) {
      glm::vec4 corner = vp;
      corner.x += ((c & 1) ? lr : -lr);
      corner.y += ((c & 2) ? lr : -lr);
      corner.z += ((c & 4) ? lr : -lr);
      if (-corner.z < nearPlane * 0.5f) {
        // Behind or across the near plane: treat as full-screen in x/y.
        minX = -1.0f;
        maxX = 1.0f;
        minY = -1.0f;
        maxY = 1.0f;
        any  = true;
        break;
      }
      glm::vec4 clip = projection * corner;
      float     ndcX = clip.x / clip.w;
      float     ndcY = clip.y / clip.w;
      if (!any) {
        minX = maxX = ndcX;
        minY = maxY = ndcY;
        any         = true;
      } else {
        if (ndcX < minX)
          minX = ndcX;
        if (ndcX > maxX)
          maxX = ndcX;
        if (ndcY < minY)
          minY = ndcY;
        if (ndcY > maxY)
          maxY = ndcY;
      }
    }
    if (!any) {
      continue;
    }
    if (maxX < -1.0f || minX > 1.0f || maxY < -1.0f || minY > 1.0f) {
      continue;
    }

    int x0 = (int)std::floor((minX * 0.5f + 0.5f) * CLUSTERS_X);
    int x1 = (int)std::floor((maxX * 0.5f + 0.5f) * CLUSTERS_X);
    int y0 = (int)std::floor((minY * 0.5f + 0.5f) * CLUSTERS_Y);
    int y1 = (int)std::floor((maxY * 0.5f + 0.5f) * CLUSTERS_Y);
    if (x0 < 0)
      x0 = 0;
    if (y0 < 0)
      y0 = 0;
    if (x1 >= CLUSTERS_X)
      x1 = CLUSTERS_X - 1;
    if (y1 >= CLUSTERS_Y)
      y1 = CLUSTERS_Y - 1;

    // Depth slice range from the sphere's z extent.
    float zNear = vz - lr;
    float zFar  = vz + lr;
    if (zNear < nearPlane)
      zNear = nearPlane;
    if (zFar > farPlane)
      zFar = farPlane;
    int z0 = (int)std::floor(std::log(zNear / nearPlane) /
                             std::log(farPlane / nearPlane) * CLUSTERS_Z);
    int z1 = (int)std::floor(std::log(zFar / nearPlane) /
                             std::log(farPlane / nearPlane) * CLUSTERS_Z);
    if (z0 < 0)
      z0 = 0;
    if (z1 >= CLUSTERS_Z)
      z1 = CLUSTERS_Z - 1;

    int          slot = g_visibleLights++;
    const float *lc   = OkLighting::getLightColor(li);
    const float *ld   = OkLighting::getLightDirection(li);
    g_lightData.push_back(lp[0]);
    g_lightData.push_back(lp[1]);
    g_lightData.push_back(lp[2]);
    g_lightData.push_back(lr);
    g_lightData.push_back(lc[0]);
    g_lightData.push_back(lc[1]);
    g_lightData.push_back(lc[2]);
    g_lightData.push_back(OkLighting::getLightIntensity(li));
    g_lightData.push_back(ld[0]);
    g_lightData.push_back(ld[1]);
    g_lightData.push_back(ld[2]);
    g_lightData.push_back(OkLighting::getLightCosCone(li));

    for (int z = z0; z <= z1; z++) {
      for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
          int ci = (z * CLUSTERS_Y + y) * CLUSTERS_X + x;
          if ((int)g_clusterLists[(size_t)ci].size() < MAX_LIGHTS_PER_CLUSTER) {
            g_clusterLists[(size_t)ci].push_back(slot);
          }
        }
      }
    }
  }

  // Flatten the per-cluster lists into one index array plus an
  // (offset, count) table.
  g_indexData.clear();
  g_gridData.clear();
  g_gridData.reserve(CLUSTER_COUNT * 2);
  for (int i = 0; i < CLUSTER_COUNT; i++) {
    const std::vector<int> &list   = g_clusterLists[(size_t)i];
    int                     offset = (int)g_indexData.size();
    int                     count  = (int)list.size();
    if (offset + count > MAX_LIGHT_REFS) {
      count = MAX_LIGHT_REFS - offset;
      if (count < 0) {
        count = 0;
      }
    }
    for (int k = 0; k < count; k++) {
      g_indexData.push_back(list[(size_t)k]);
    }
    g_gridData.push_back(offset);
    g_gridData.push_back(count);
  }
  g_lightRefs = (int)g_indexData.size();

  // Upload. Buffer textures keep this working on OpenGL 4.1 (no SSBOs).
  if (g_lightData.empty()) {
    g_lightData.resize(LIGHT_FLOATS, 0.0f);
  }
  if (g_indexData.empty()) {
    g_indexData.push_back(0);
  }
  glBindBuffer(GL_TEXTURE_BUFFER, g_lightTbo);
  glBufferData(GL_TEXTURE_BUFFER,
               (GLsizeiptr)(g_lightData.size() * sizeof(float)),
               g_lightData.data(), GL_DYNAMIC_DRAW);
  glBindBuffer(GL_TEXTURE_BUFFER, g_indexTbo);
  glBufferData(GL_TEXTURE_BUFFER,
               (GLsizeiptr)(g_indexData.size() * sizeof(int)),
               g_indexData.data(), GL_DYNAMIC_DRAW);
  glBindBuffer(GL_TEXTURE_BUFFER, g_gridTbo);
  glBufferData(GL_TEXTURE_BUFFER, (GLsizeiptr)(g_gridData.size() * sizeof(int)),
               g_gridData.data(), GL_DYNAMIC_DRAW);
  glBindBuffer(GL_TEXTURE_BUFFER, 0);
}

void OkLightClusters::bind(GLuint program, int screenWidth, int screenHeight,
                           float nearPlane, float farPlane) {
  bool  on    = OkConfig::getBool("lighting.clustered") && g_lightTbo != 0;
  GLint onLoc = glGetUniformLocation(program, "clusteredOn");
  if (onLoc != -1) {
    glUniform1f(onLoc, on ? 1.0f : 0.0f);
  }
  if (!on) {
    return;
  }

  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_BUFFER, g_lightTex);
  glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, g_lightTbo);
  glActiveTexture(GL_TEXTURE5);
  glBindTexture(GL_TEXTURE_BUFFER, g_indexTex);
  glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, g_indexTbo);
  glActiveTexture(GL_TEXTURE6);
  glBindTexture(GL_TEXTURE_BUFFER, g_gridTex);
  glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32I, g_gridTbo);
  glActiveTexture(GL_TEXTURE0);

  GLint loc = glGetUniformLocation(program, "clusterLights");
  if (loc != -1)
    glUniform1i(loc, 4);
  loc = glGetUniformLocation(program, "clusterIndices");
  if (loc != -1)
    glUniform1i(loc, 5);
  loc = glGetUniformLocation(program, "clusterGrid");
  if (loc != -1)
    glUniform1i(loc, 6);
  loc = glGetUniformLocation(program, "clusterDims");
  if (loc != -1) {
    glUniform3i(loc, CLUSTERS_X, CLUSTERS_Y, CLUSTERS_Z);
  }
  loc = glGetUniformLocation(program, "clusterScreen");
  if (loc != -1) {
    glUniform2f(loc, (float)screenWidth, (float)screenHeight);
  }
  loc = glGetUniformLocation(program, "clusterPlanes");
  if (loc != -1) {
    // The shader must slice with the SAME range update() used.
    (void)nearPlane;
    (void)farPlane;
    glUniform2f(loc, OkConfig::getFloat("lighting.cluster.near"),
                OkConfig::getFloat("lighting.cluster.far"));
  }
}

int OkLightClusters::getVisibleLightCount() {
  return g_visibleLights;
}

int OkLightClusters::getLightRefCount() {
  return g_lightRefs;
}

void OkLightClusters::shutdown() {
  if (g_lightTbo != 0) {
    glDeleteBuffers(1, &g_lightTbo);
    glDeleteBuffers(1, &g_indexTbo);
    glDeleteBuffers(1, &g_gridTbo);
    glDeleteTextures(1, &g_lightTex);
    glDeleteTextures(1, &g_indexTex);
    glDeleteTextures(1, &g_gridTex);
    g_lightTbo = 0;
  }
}
