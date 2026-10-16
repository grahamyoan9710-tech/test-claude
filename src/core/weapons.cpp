// Weapons that drop during a match: each one belongs to a character (its two signature weapons),
// lands on a platform and is picked up by walking over it. It boosts its owner for a while.
#include <algorithm>

#include "core/match.hpp"

namespace {
constexpr int kMaxItems = 2;
constexpr int kItemLife = 60 * 16;
constexpr float kItemGravity = 0.45f;
constexpr float kItemMaxFall = 9.0f;
}  // namespace

void Match::spawnWeapon(int character, int slot, Vec2 pos) {
  WeaponItem it;
  it.character = character;
  it.slot = slot & 1;
  it.pos = pos;
  it.vel = {0.0f, 1.0f};
  it.life = kItemLife;
  items.push_back(it);
  emit(EvType::WeaponSpawn, pos, -1, it.slot, float(character));
}

void Match::dropWeaponFor(int id, int slot) {
  if (id < 0 || id >= int(fighters.size())) return;
  const Fighter& f = fighters[id];
  spawnWeapon(f.charIndex, slot, {f.pos.x + f.facing * 90.0f, f.pos.y - 260.0f});
}

void Match::equipWeapon(Fighter& f, int slot) {
  f.weapon = slot & 1;
  f.weaponTimer = f.def->weapons[f.weapon].duration;
  emit(EvType::WeaponPickup, f.center(), f.id, f.weapon, float(f.charIndex));
}

void Match::loseWeapon(Fighter& f, bool broken) {
  if (f.weapon < 0) return;
  if (broken) emit(EvType::WeaponBreak, f.center(), f.id, f.weapon, float(f.charIndex));
  f.weapon = -1;
  f.weaponTimer = 0;
}

void Match::updateItems() {
  if (!started()) return;
  // Held weapons wear out (not in the middle of a move: the weapon may be what that move is made of)
  for (auto& f : fighters) {
    if (f.weapon < 0) continue;
    if (f.weaponTimer > 0) --f.weaponTimer;
    if (f.weaponTimer <= 0 && f.state != FState::Attack) loseWeapon(f, true);
  }

  // A new drop now and then, for a fighter who has no weapon
  if (cfg.weapons && --itemTimer <= 0) {
    itemTimer = rng.irange(60 * 10, 60 * 17);
    std::vector<int> wanting;
    for (const auto& f : fighters) {
      if (!f.active() || f.weapon >= 0) continue;
      bool onStage = false;
      for (const auto& it : items) onStage = onStage || it.character == f.charIndex;
      if (!onStage) wanting.push_back(f.id);
    }
    if (!wanting.empty() && int(items.size()) < kMaxItems) {
      const Fighter& f = fighters[wanting[rng.irange(0, int(wanting.size()) - 1)]];
      // Above a random platform (the main one more often), inside the camera's usual view
      std::vector<int> plats;
      for (int i = 0; i < int(platforms.size()); ++i) {
        plats.push_back(i);
        if (platforms[i].solid) plats.push_back(i);
      }
      const Rect& r = platforms[plats[rng.irange(0, int(plats.size()) - 1)]].rect;
      const float x = rng.range(r.left() + 30.0f, std::max(r.left() + 30.0f, r.right() - 30.0f));
      spawnWeapon(f.charIndex, rng.irange(0, 1), {x, r.top() - 620.0f});
    }
  }

  const Rect& blast = stageDef->blast;
  for (auto& it : items) {
    ++it.age;
    --it.life;
    // Fall, land on the first platform top under it (moving platforms carry it)
    it.grounded = false;
    if (it.vel.y >= 0.0f) {
      for (const auto& p : platforms) {
        const Rect& r = p.rect;
        if (it.pos.x < r.left() || it.pos.x > r.right()) continue;
        if (it.pos.y <= r.top() + 6.0f && it.pos.y + it.vel.y + 2.0f >= r.top()) {
          if (it.vel.y > 3.0f) emit(EvType::WeaponLand, {it.pos.x, r.top()}, -1, it.slot, float(it.character));
          it.pos.y = r.top();
          it.vel = {0.0f, 0.0f};
          it.grounded = true;
          break;
        }
      }
    }
    if (!it.grounded) {
      it.vel.y = std::min(kItemMaxFall, it.vel.y + kItemGravity);
      it.pos += it.vel;
    }
    if (it.pos.y > blast.bottom()) it.life = 0;
    // Picked up by its owner walking over it
    if (it.life <= 0) continue;
    for (auto& f : fighters) {
      if (f.charIndex != it.character || !f.active() || f.weapon >= 0) continue;
      if (f.state == FState::Respawn || f.state == FState::Grabbed || f.state == FState::Holding ||
          f.state == FState::LedgeHang || f.state == FState::Hitstun || f.state == FState::Captured)
        continue;
      if (f.state == FState::Attack) {  // not if it would change the move being performed
        bool changes = false;
        for (const auto& mm : f.def->weapons[it.slot].moves) changes = changes || mm.first == f.move;
        if (changes) continue;
      }
      Rect hurt = f.hurtbox();
      if (!circleRect({it.pos.x, it.pos.y - 18.0f}, 26.0f, hurt)) continue;
      equipWeapon(f, it.slot);
      it.life = 0;
      break;
    }
  }
  items.erase(std::remove_if(items.begin(), items.end(), [](const WeaponItem& i) { return i.life <= 0; }), items.end());
}
