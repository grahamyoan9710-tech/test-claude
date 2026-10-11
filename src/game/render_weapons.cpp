// Weapons: the fighters' own blades and the weapons that drop during a match.
#include "game/render.hpp"

float weaponVisualLength(WeaponVisual v) {
  switch (v) {
    case WeaponVisual::None: case WeaponVisual::SixEyes: case WeaponVisual::CursedFinger: return 0.0f;
    case WeaponVisual::Grimoire: case WeaponVisual::HollowPurple: return 30.0f;
    case WeaponVisual::DemonSlayer: return 96.0f;
    case WeaponVisual::Zangetsu: return 92.0f;
    case WeaponVisual::BladesOfChaos: return 70.0f;
    default: return 74.0f;
  }
}

void drawWeaponVisual(WeaponVisual v, Vec2 grip, Vec2 dir, float scale, float alpha, float time) {
  (void)time;
  const float len = weaponVisualLength(v) * scale;
  if (len <= 0.0f) return;
  Vec2 tip = grip + dir * len;
  DrawLineEx(V(grip), V(tip), 7.0f * scale, withAlpha({40, 32, 30, 255}, alpha));
  DrawLineEx(V(grip), V(tip), 4.0f * scale, withAlpha({220, 224, 232, 255}, alpha));
}
