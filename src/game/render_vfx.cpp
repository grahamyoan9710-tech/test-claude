// Big move effects: beams and ultimate cinematics.
#include "game/render.hpp"

void drawBeams(const Match& m, float time) {
  (void)time;
  for (const auto& f : m.fighters) {
    const MoveDef* mv = f.currentMove();
    if (!mv || mv->beam.start < 0 || f.moveFrame < mv->beam.start || f.moveFrame >= mv->beam.end) continue;
    const BeamDef& b = mv->beam;
    Vec2 dir{std::cos(deg2rad(b.angle)) * f.facing, -std::sin(deg2rad(b.angle))};
    Vec2 a = f.center() + Vec2{b.from.x * f.facing, b.from.y};
    Vec2 e = a + dir * b.length;
    DrawLineEx(V(a), V(e), b.width, withAlpha(hitFxColor(b.fx), 0.6f));
    DrawLineEx(V(a), V(e), b.width * 0.4f, withAlpha(hitFxGlow(b.fx), 0.9f));
  }
}

void drawUltimateBackdrop(const Match& m, float time) {
  (void)m;
  (void)time;
}
