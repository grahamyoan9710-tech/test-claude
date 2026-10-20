// Weapons lying on the stage: a PixelLab beacon of light, the weapon floating above it,
// the owner's portrait and a blink when they are about to vanish.
#include "game/fighter_art.hpp"
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
    // Once landed, the PixelLab beacon (falling, the pillar of light of fx.cpp guides it down)
    if (it.grounded) art::drawFx("sys_weapon_beacon", time, base, 1.1f, 0.0f, false, 0.9f, mixColor(g, WHITE, 0.3f));
    // The weapon itself
    drawWeaponIcon(d, it.slot, center, 76.0f);
    // Whose weapon: the owner's portrait in a ring
    const Vec2 tag = center + Vec2{0.0f, -58.0f};
    DrawCircleV(V(tag), 21, withAlpha(c, 0.9f));
    DrawCircleV(V(tag), 18, {24, 30, 34, 235});
    drawPortraitDisc(d, 0, tag, 18, time);
  }
}
