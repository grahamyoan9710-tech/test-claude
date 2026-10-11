// Weapons lying on the stage: a glowing pedestal of light, the weapon floating above it,
// the owner's portrait and a blink when they are about to vanish.
#include "game/render.hpp"

void drawItems(const Match& m, float time) {
  const auto& chars = roster();
  for (const auto& it : m.items) {
    const CharacterDef& d = chars[std::clamp(it.character, 0, int(chars.size()) - 1)];
    const WeaponDef& w = d.weapons[it.slot & 1];
    if (it.life < 120 && (it.life / 6) % 2 == 0) continue;  // about to vanish
    const Color c = hitFxColor(w.fx), g = hitFxGlow(w.fx);
    const float bob = it.grounded ? std::sin(time * 3.0f + it.age * 0.02f) * 5.0f : 0.0f;
    const Vec2 base = it.pos;
    const Vec2 center = base + Vec2{0.0f, -46.0f + bob};
    // Pillar of light while it falls / beacon once landed
    if (!it.grounded) {
      DrawRectangleGradientV(int(base.x - 10), int(base.y - 160), 20, 140, withAlpha(g, 0.0f), withAlpha(g, 0.35f));
    } else {
      float pulse = 0.5f + 0.5f * std::sin(time * 4.0f);
      DrawRectangleGradientV(int(base.x - 22), int(base.y - 140), 44, 140, withAlpha(g, 0.0f), withAlpha(c, 0.22f + 0.1f * pulse));
      DrawEllipse(int(base.x), int(base.y - 2), 34 + 6 * pulse, 8, withAlpha(c, 0.45f));
      DrawEllipseLines(int(base.x), int(base.y - 2), 40 + 6 * pulse, 10, withAlpha(g, 0.8f));
    }
    DrawCircleGradient(int(center.x), int(center.y), 44, withAlpha(g, 0.35f), withAlpha(c, 0.0f));
    // The weapon itself, tilted, slowly turning
    const float a = -1.1f + std::sin(time * 1.6f + it.age * 0.01f) * 0.25f;
    const Vec2 dir{std::cos(a), std::sin(a)};
    const float len = std::max(30.0f, weaponVisualLength(w.visual) * 0.8f);
    drawWeaponVisual(w.visual, center - dir * (len * 0.5f), dir, 0.8f, 1.0f, time);
    if (w.visual == WeaponVisual::SixEyes || w.visual == WeaponVisual::CursedFinger || w.visual == WeaponVisual::Grimoire ||
        w.visual == WeaponVisual::HollowPurple)
      DrawCircleV(V(center), 10.0f, withAlpha(g, 0.9f));
    // Whose weapon: the owner's portrait in a ring
    const Vec2 tag = center + Vec2{0.0f, -58.0f};
    DrawCircleV(V(tag), 21, withAlpha(c, 0.9f));
    DrawCircleV(V(tag), 18, {24, 30, 34, 235});
    drawPortrait(d, 0, tag, 30, time);
  }
}
