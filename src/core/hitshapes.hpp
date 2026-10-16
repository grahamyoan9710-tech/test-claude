#pragma once
// The hitboxes a fighter has out on its current move frame, as shapes in world space: a circle (a == b) or a
// capsule along a swung blade. Exactly the geometry of the hit detection (combat.cpp), for the debug overlay
// (--hitboxes, --screen anim) and the check of hitboxes against the drawn animations (--check-hitboxes).
#include <vector>

#include "core/fighter.hpp"
#include "core/swing.hpp"

struct HitShape {
  Vec2 a, b;  // the segment's ends (equal for a circle)
  float radius = 0.0f;
  int index = 0;  // in MoveDef::hitboxes
};

// Shapes of the hitboxes of `md` active on move frame `frame` for a fighter at `center` facing `facing`.
inline std::vector<HitShape> hitShapes(const MoveDef& md, int frame, Vec2 center, int facing, float height,
                                       float bladeLength) {
  std::vector<HitShape> out;
  for (int i = 0; i < int(md.hitboxes.size()); ++i) {
    const HitboxDef& x = md.hitboxes[i];
    if (frame < x.start || frame >= x.end) continue;
    if (!x.swing) {
      const Vec2 p = center + Vec2{x.offset.x * facing, x.offset.y};
      out.push_back({p, p, x.radius, i});
      continue;
    }
    const BladeSeg s = swingSegment(x, swingAngle(x, float(frame)), height, bladeLength);
    out.push_back({center + Vec2{s.a.x * facing, s.a.y}, center + Vec2{s.b.x * facing, s.b.y}, x.radius, i});
  }
  return out;
}

inline std::vector<HitShape> hitShapes(const Fighter& f) {
  const MoveDef* md = f.currentMove();
  if (!md) return {};
  return hitShapes(*md, f.moveFrame, f.center(), f.facing, f.height(), f.bladeLength());
}
