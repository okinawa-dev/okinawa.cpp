#ifndef OK_LIGHTING_HPP
#define OK_LIGHTING_HPP

/**
 * @brief Static handler for the scene's global lighting and atmosphere.
 *
 *        L1 scope: a DAY CLOCK (hour 0..24 advancing at a configurable
 *        timescale) driving a keyframe curve — day, sunset, night, dawn —
 *        that interpolates the global atmosphere values every frame:
 *
 *        - scene tint: a colour multiplied over every world fragment (the
 *          cold teal that soaks the night, the warm cast of the sunset);
 *        - exponential distance fog (colour + density): the milky haze
 *          distance dissolves into;
 *        - sun colour and direction (stored now, consumed by the
 *          directional-lighting stage later).
 *
 *        The world render pass reads the fog/tint uniforms from here; the
 *        GUI pass resets them (the interface is never tinted or fogged).
 *
 *        Console commands: `time [hour]` reads or sets the clock,
 *        `timescale [x]` reads or sets how much faster than real time the
 *        clock runs.
 */
/**
 * @brief One keyframe of the atmosphere curve: what the sky and the
 *        light look like at a given hour. The engine interpolates
 *        between consecutive keys, wrapping around midnight.
 */
struct OkAtmosphereKey {
  float hour;        // 0..24, keys must be given in ascending order
  float tint[3];     // multiplied over every world fragment
  float fog[3];      // fog colour, and the sky at the horizon
  float fogDensity;  // exponential fog, per metre
  float sun[3];      // directional light colour (black = no sun)
  float zenith[3];   // sky colour straight up
  float ambient;     // flat ambient floor under the directional light
};

class OkLighting {
public:
  OkLighting() = delete;

  // Replace the atmosphere curve. The engine ships a neutral default
  // (clear day, blue-ish night) so any project runs out of the box;
  // a game with its own look supplies its own keys, which is where
  // artistic direction belongs. Keys are copied. Passing fewer than two
  // keys leaves the current curve untouched.
  static void setAtmosphereCurve(const OkAtmosphereKey *keys, int count);
  static int  getAtmosphereKeyCount();

  // Register config defaults' consumers and the console commands. Called
  // by OkCore::initialize (after OkConsole::initialize).
  static void initialize();

  // Advance the clock (dt in milliseconds, engine loop convention) and
  // re-evaluate the atmosphere curve.
  static void update(float dt);

  // Clock. Hours wrap into [0, 24).
  static float getTimeOfDay();
  static void  setTimeOfDay(float hours);

  // Current interpolated atmosphere values.
  static const float *getSceneTint() { return _tint; }     // rgb
  static const float *getFogColor() { return _fogColor; }  // rgb
  // 0 while lighting.fog is disabled (the console/game fog toggle).
  static float        getFogDensity();
  static const float *getSunColor() { return _sunColor; }    // rgb
  static const float *getSunDirection() { return _sunDir; }  // xyz, normalized
  static const float *getSkyZenith() { return _zenith; }     // rgb, sky top
  // Flat ambient floor under the Gouraud sun (L3).
  static float getAmbientLight() { return _ambient; }

  // --- Point lights (L4) ---------------------------------------------
  // Small registry of point lights. Each item is lit by its nearest
  // lights up to the per-item budget -- the classic cheap model, no
  // shadows. Lights are expected to be mostly static: items cache their
  // nearest set and refresh it only when the registry generation
  // changes.
  static const int MAX_LIGHTS          = 4096;
  static const int MAX_LIGHTS_PER_ITEM = 4;

  // Register an OMNI light (radiates equally in every direction);
  // returns its id, or -1 when the registry is full.
  static int registerLight(float x, float y, float z, float r, float g, float b,
                           float radius);
  // Register a SPOT light: same as registerLight plus a direction, a
  // cone half-angle (degrees) with a soft edge, and an intensity
  // multiplier over the colour. A spot aimed downward pools its light
  // on the surface below.
  static int  registerSpotLight(float x, float y, float z, float r, float g,
                                float b, float radius, float dirX, float dirY,
                                float dirZ, float coneDeg, float intensity);
  static void clearLights();
  static long getLightGeneration();
  static int  getLightCount();

  // Fill `outIdx` with up to `maxN` indices of the most relevant lights
  // for a point (nearest by distance/radius); returns how many.
  static int getNearestLights(float x, float y, float z, int *outIdx, int maxN);
  // Accessors for the shader uniforms (index from getNearestLights).
  static const float *getLightPosition(int idx);  // xyz
  static const float *getLightColor(int idx);     // rgb
  static float        getLightRadius(int idx);
  static const float *getLightDirection(int idx);  // xyz (spots)
  static float        getLightCosCone(int idx);    // <= -1.5 for omni
  static float        getLightIntensity(int idx);

  // Global point-light level: 0 by day, ramping to 1 as the sun drops
  // through the horizon (computed every update from the sun elevation).
  // The shader multiplies every point light by it, so artificial
  // lights come up at dusk with no per-light bookkeeping.
  static float getPointLightLevel() { return _pointLightLevel; }

  // Lazily-built shared radial halo texture ("ok_halo", additive white
  // falloff disc) for light glows; tint it per light.
  static class OkTexture *getHaloTexture();

  // Evaluate the curve for an arbitrary hour (pure; unit-testable).
  static void evaluate(float hours, float outTint[3], float outFogColor[3],
                       float &outFogDensity, float outSunColor[3],
                       float outSunDir[3], float outZenith[3] = nullptr,
                       float *outAmbient = nullptr);

private:
  static float _tint[3];
  static float _fogColor[3];
  static float _fogDensity;
  static float _sunColor[3];
  static float _sunDir[3];
  static float _zenith[3];
  static float _ambient;

  static float _lightPos[MAX_LIGHTS][3];
  static float _lightColor[MAX_LIGHTS][3];
  static float _lightRadius[MAX_LIGHTS];
  static float _lightDir[MAX_LIGHTS][3];
  static float _lightCosCone[MAX_LIGHTS];  // cos(half-angle); -2 = omni
  static float _lightIntensity[MAX_LIGHTS];
  static float _pointLightLevel;
  static int   _lightCount;
  static long  _lightGeneration;
};

#endif
