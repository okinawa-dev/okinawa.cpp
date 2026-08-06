#version 410
#pragma shader_stage(fragment)

// Depth-only pass: the depth buffer is the whole output, so there is
// nothing to write here -- except for the one thing a caster can still
// decide, which is whether it is there at all.
//
// The world pass dissolves one level of detail into the next with an
// ordered dither, both drawn at once, each dropping the pixels the
// other keeps. Neither behaviour transfers here unchanged:
//
//   - Ignoring the fade lets BOTH levels cast, solid, through the whole
//     handover: two casters of the same building, one coarse and one
//     detailed, laid over each other.
//   - Running the same dither is worse. Two half-covered colour buffers
//     add up to one surface; two half-covered DEPTH buffers do not. A
//     dropped texel does not mean "half occluded", it means "nothing
//     here, light passes" -- so the shadow comes out stippled.
//
// A shadow does not need to cross-fade, it needs to be consistent. So
// only the level that is more than half faded in casts, and the map
// always holds exactly one version of the building. The handover falls
// where the two levels are each half visible, which is where the
// difference between their shadows is least worth seeing.
uniform float itemFade;

void main() {
  if (itemFade < 0.5) {
    discard;
  }
}
