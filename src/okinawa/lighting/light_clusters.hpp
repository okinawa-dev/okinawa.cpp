#ifndef OK_LIGHT_CLUSTERS_HPP
#define OK_LIGHT_CLUSTERS_HPP

#include "../core/gl_config.hpp"
#include <glm/ext/matrix_float4x4.hpp>

/**
 * @brief Clustered forward light assignment.
 *
 *        The view frustum is divided into a 3D grid of clusters (tiles
 *        across the screen x exponential depth slices). Every frame the
 *        CPU assigns each registered light to the clusters its sphere of
 *        influence touches, and uploads two buffers:
 *
 *        - the LIGHT buffer: position+radius, colour+intensity,
 *          direction+cone for every light that reaches the frustum;
 *        - the INDEX buffer plus a per-cluster (offset, count) table.
 *
 *        The world fragment shader then looks up its own cluster from
 *        gl_FragCoord and the fragment depth, and iterates only those
 *        lights. Light selection becomes PER PIXEL instead of per item,
 *        which is what a city of huge ground meshes needs: a sidewalk
 *        spanning a whole block gets every lamp along it, not the four
 *        nearest to its centre.
 *
 *        Assignment happens on the CPU because the engine targets
 *        OpenGL 4.1 (no compute shaders); at our light counts this is
 *        cheap, and lights that do not reach the frustum are skipped
 *        entirely -- culling by SPHERE OF INFLUENCE, so a lamp around
 *        the corner still lights the street it spills into.
 */
class OkLightClusters {
public:
  OkLightClusters() = delete;

  // Grid dimensions. Depth slices are distributed exponentially so near
  // clusters are short and far ones long (matching perspective).
  static const int CLUSTERS_X = 16;
  static const int CLUSTERS_Y = 9;
  static const int CLUSTERS_Z = 24;
  static const int CLUSTER_COUNT = CLUSTERS_X * CLUSTERS_Y * CLUSTERS_Z;
  // Cap on light references across all clusters (a light in N clusters
  // costs N references) and per single cluster.
  static const int MAX_LIGHT_REFS        = 262144;
  static const int MAX_LIGHTS_PER_CLUSTER = 16;
  // Clustering depth range, INDEPENDENT of the camera planes: the camera
  // far plane is kilometres away, which would make the near exponential
  // slices microscopic and blow the reference budget. Past this distance
  // the fog has swallowed everything anyway, so point lights stop.
  static const int CLUSTER_NEAR = 1;    // metres
  static const int CLUSTER_FAR  = 350;  // metres
  // Cap on lights reaching a single frustum.
  static const int MAX_VISIBLE_LIGHTS = 1024;

  // Register config defaults. Called by OkCore::initialize.
  static void initialize();

  // Rebuild the clusters for this frame from the current light registry
  // and camera, and upload the buffers. Called by the world pass before
  // drawing the scene.
  static void update(const glm::mat4 &view, const glm::mat4 &projection,
                     float nearPlane, float farPlane);

  // Bind the buffers to their texture units and set the shader uniforms
  // (grid dimensions, depth slice parameters, screen size).
  static void bind(GLuint program, int screenWidth, int screenHeight,
                   float nearPlane, float farPlane);

  // Diagnostics: lights that reached the frustum, and total references.
  static int getVisibleLightCount();
  static int getLightRefCount();

  static void shutdown();

private:
  static void ensureBuffers();
};

#endif
