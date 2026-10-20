// The fighters' weapons outside their clips: their icons (assets/sprites/<id>_icon_a.png and _icon_b.png, from the
// owner's weapon sheet) for the HUD, the menus and the items, and the floating weapons (WeaponVisual::Floating:
// the orb of 04, the grimoires of 11), drawn beside the fighter. A held weapon is in the fighter's clips.
#include <algorithm>
#include <cmath>
#include <string>

#include "game/fighter_art.hpp"

namespace {
std::string iconName(const CharacterDef& d, int slot) { return d.id + ((slot & 1) ? "_icon_b" : "_icon_a"); }
}  // namespace

bool drawWeaponIcon(const CharacterDef& d, int slot, Vec2 center, float size, float alpha) {
  const std::string name = iconName(d, slot);
  float w = 0.0f, h = 0.0f;
  if (!art::pictureSize(name.c_str(), w, h) || w <= 0.0f || h <= 0.0f) return false;
  const float k = size / std::max(w, h);
  return art::drawPicture(name.c_str(), {0, 0, w, h}, {center.x - w * k * 0.5f, center.y - h * k * 0.5f, w * k, h * k},
                          alpha);
}

void drawFloatingWeapon(const Fighter& f, float time, float alpha) {
  const WeaponDef* w = f.weaponDef();
  if (!w || w->visual != WeaponVisual::Floating || !f.active()) return;
  const float H = f.def->height;
  const Vec2 at = f.center() + Vec2{-0.32f * H * f.facing, -0.42f * H + std::sin(time * 2.4f + f.id) * 0.04f * H};
  drawWeaponIcon(*f.def, f.weapon, at, 0.42f * H, alpha);
}
