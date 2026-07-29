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
class OkLighting {
public:
  OkLighting() = delete;

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
  static const float *getSceneTint() { return _tint; }      // rgb
  static const float *getFogColor() { return _fogColor; }   // rgb
  static float        getFogDensity() { return _fogDensity; }
  static const float *getSunColor() { return _sunColor; }   // rgb
  static const float *getSunDirection() { return _sunDir; } // xyz, normalized
  static const float *getSkyZenith() { return _zenith; }    // rgb, sky top

  // Evaluate the curve for an arbitrary hour (pure; unit-testable).
  static void evaluate(float hours, float outTint[3], float outFogColor[3],
                       float &outFogDensity, float outSunColor[3],
                       float outSunDir[3], float outZenith[3] = nullptr);

private:
  static float _tint[3];
  static float _fogColor[3];
  static float _fogDensity;
  static float _sunColor[3];
  static float _sunDir[3];
  static float _zenith[3];
};

#endif
