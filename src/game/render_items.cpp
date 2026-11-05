// Weapons lying on the stage: a PixelLab beacon of light, the weapon floating above it,
// the owner's portrait and a blink when they are about to vanish.
#include "game/fighter_art.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
const CharacterDef& ownerOf(const WeaponItem& it) {
  const auto& chars = roster();
  return chars[std::clamp(it.character, 0, int(chars.size()) - 1)];
}
float bobOf(const WeaponItem& it, float time) { return it.grounded ? std::sin(time * 3.0f + it.age * 0.02f) * 5.0f : 0.0f; }
bool blinkedOut(const WeaponItem& it) { return it.life < 120 && (it.life / 6) % 2 == 0; }  // about to vanish
}  // namespace

void drawItems(const Match& m, float time) {
  for (const auto& it : m.items) {
    const CharacterDef& d = ownerOf(it);
    const WeaponDef& w = d.weapons[it.slot & 1];
    if (blinkedOut(it)) continue;
    const Color g = hitFxGlow(w.fx);
    const float bob = bobOf(it, time);
    const Vec2 base = it.pos;
    const Vec2 center = base + Vec2{0.0f, -46.0f + bob};
    // Once landed, the PixelLab beacon (falling, the pillar of light of fx.cpp guides it down)
    if (kVfx && it.grounded) art::drawFx("sys_weapon_beacon", time, base, 1.1f, 0.0f, false, 0.9f, mixColor(g, WHITE, 0.3f));
    // The weapon itself (whose it is: drawItemPrompts)
    drawWeaponIcon(d, it.slot, center, 76.0f);
  }
}

void drawItemPrompts(const Match& m, const Camera2D& cam, const std::array<int, 4>& devices, float time) {
  for (const auto& it : m.items) {
    if (blinkedOut(it)) continue;
    // Whose weapon: the owner's portrait in a ring, on the screen (the portrait is clipped in screen space)
    const CharacterDef& d = ownerOf(it);
    const Vector2 tag = GetWorldToScreen2D(V(it.pos + Vec2{0.0f, -104.0f + bobOf(it, time)}), cam);
    DrawCircleV(tag, 21.0f * cam.zoom, withAlpha(hitFxColor(d.weapons[it.slot & 1].fx), 0.9f));
    DrawCircleV(tag, 18.0f * cam.zoom, {24, 30, 34, 235});
    drawPortraitDisc(d, 0, W(tag), 18.0f * cam.zoom, time);
    if (!it.grounded) continue;
    const Fighter* owner = nullptr;  // the closest one that does not have it yet (both, in a mirror match)
    for (const auto& f : m.fighters)
      if (f.active() && !((f.weapons >> it.slot) & 1) && f.charIndex == it.character &&
          (!owner || (f.pos - it.pos).len2() < (owner->pos - it.pos).len2()))
        owner = &f;
    if (!owner || (owner->cpu && it.hold <= 0)) continue;
    // Above the portrait: the hold so far (a hit pauses it), and over it the button to hold
    const Rectangle bar{tag.x - 34.0f, tag.y - 21.0f * cam.zoom - 17.0f, 68.0f, 9.0f};
    const float h = 26.0f;
    if (!owner->cpu) {
      const int dev = devices[owner->port & 3];
      ui::actionPrompt(dev, kActWeapon, tag.x - ui::actionPromptWidth(dev, kActWeapon, h) * 0.5f, bar.y - 8.0f - h, h,
                       it.hold > 0);
    }
    if (it.hold > 0) {
      const float k = std::min(1.0f, float(it.hold) / kPickupFrames);
      ui::slanted(ui::grow(bar, 2.0f), 4.0f, {12, 12, 18, 230}, {12, 12, 18, 230});
      if (k > 0.0f) ui::slanted({bar.x, bar.y, bar.width * k, bar.height}, 3.0f, ui::kRed, ui::kRedDark);
      ui::slantedOutline(ui::grow(bar, 2.0f), 4.0f, 1.5f, WHITE);
    }
  }
}
