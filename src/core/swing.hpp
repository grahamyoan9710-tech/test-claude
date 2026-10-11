#pragma once
// Blade swings (HitboxDef::swing): the geometry shared by the hit detection and the renderer, so that
// the hitbox is exactly where the blade is drawn.
#include <algorithm>

#include "core/moves.hpp"

// Blade angle (degrees, facing right, 90 = up) of a swing hitbox at move frame t: it sweeps from a0
// (just before its first active frame) to a1 (its last active frame), and holds a0 / a1 outside.
inline float swingAngle(const HitboxDef& h, float t) {
  const float n = float(std::max(1, h.end - h.start));
  const float u = clampf((t - float(h.start) + 1.0f) / n, 0.0f, 1.0f);
  return h.a0 + (h.a1 - h.a0) * u;
}

inline Vec2 swingDir(float angleDeg) { return {std::cos(deg2rad(angleDeg)), -std::sin(deg2rad(angleDeg))}; }

// Where the hand holds the blade (from the fighter's center, facing right).
inline Vec2 swingGrip(float angleDeg, float height) {
  return Vec2{kShoulderX * height, kShoulderY * height} + swingDir(angleDeg) * (kGripDist * height);
}

// The hitting part of the blade at that angle: from reach0 to reach1 of the blade (center-relative, facing right).
struct BladeSeg {
  Vec2 a, b;
};
inline BladeSeg swingSegment(const HitboxDef& h, float angleDeg, float height, float bladeLen) {
  const Vec2 g = swingGrip(angleDeg, height), d = swingDir(angleDeg);
  return {g + d * (h.reach0 * bladeLen), g + d * (h.reach1 * bladeLen)};
}
