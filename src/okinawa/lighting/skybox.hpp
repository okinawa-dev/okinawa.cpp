#ifndef OK_SKYBOX_HPP
#define OK_SKYBOX_HPP

class OkItem;
class OkTexture;

/**
 * @brief Procedural gradient sky dome driven by the day cycle.
 *
 *        A low-poly dome (camera-centred, drawn first with depth writes
 *        off, so everything else paints over it) textured with a 1-D
 *        vertical gradient regenerated from OkLighting: the HORIZON colour
 *        is the fog colour — so the fogged city always fades into the sky
 *        seamlessly — and the top is the curve's zenith colour. The dome
 *        reaches slightly below the horizon so no gap ever shows.
 *
 *        The emissive skyline belt (distant lit windows) is a later
 *        follow-up on the same dome.
 */
class OkSkybox {
public:
  OkSkybox() = delete;

  // Draw the dome centred on the camera position. Builds the dome and its
  // gradient texture lazily; refreshes the gradient when the cycle's
  // colours drift. Assumes the world view/projection uniforms are already
  // set and neutral fog/tint uniforms (the caller resets them after).
  static void draw(float camX, float camY, float camZ);

  // Destroy the internal resources. Called by OkCore::exit.
  static void shutdown();

private:
  static void ensure();
  static void refreshGradient();

  static OkItem    *_dome;      // owned
  static OkTexture *_gradient;  // owned by the texture handler
  static float      _builtFog[3];
  static float      _builtZenith[3];
};

#endif
