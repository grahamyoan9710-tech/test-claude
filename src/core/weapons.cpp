// The fighters' weapons. Everyone starts bare-handed; now and then a fighter's weapon drops on the stage (only its
// owner can take it). Standing on it and pressing the weapon button takes it: weapon A. After that the same button
// switches A <-> B, with no cooldown, as soon as the fighter is free to act (a press during a move waits for it to
// end). The weapon stays until a KO; then a new one drops later.
#include <algorithm>

#include "core/match.hpp"

namespace {
constexpr int kMaxItems = 2;
constexpr int kItemLife = 60 * 20;
constexpr float kItemGravity = 0.45f;
constexpr float kItemMaxFall = 9.0f;
constexpr int kWeaponBuffer = 45;  // frames a press waits for the fighter to be free (a move to end)

bool canAct(const Fighter& f) {
  switch (f.state) {
    case FState::Idle: case FState::Walk: case FState::Run: case FState::Crouch: case FState::Air:
    case FState::Landing: case FState::JumpSquat: case FState::Shield:
      return f.active();
    default: return false;
  }
}
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
  if (f.weapon >= 0) emit(EvType::WeaponVanish, f.center(), f.id, f.weapon, float(f.charIndex));
  f.weapon = slot & 1;
  emit(EvType::WeaponSummon, f.center(), f.id, f.weapon, float(f.charIndex));
}

void Match::switchWeapon(Fighter& f) {
  if (f.weapon >= 0) equipWeapon(f, f.weapon ^ 1);
}

void Match::loseWeapon(Fighter& f) {
  if (f.weapon < 0) return;
  emit(EvType::WeaponVanish, f.center(), f.id, f.weapon, float(f.charIndex));
  f.weapon = -1;
}

void Match::updateItems() {
  if (!started()) return;

  // A new drop now and then, for a fighter who has no weapon and none waiting on the stage
  if (cfg.weapons && --itemTimer <= 0) {
    itemTimer = rng.irange(60 * 6, 60 * 11);
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
      spawnWeapon(f.charIndex, 0, {x, r.top() - 620.0f});
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
  }

  // The weapon button: takes the own weapon it stands on, else switches A <-> B
  for (auto& f : fighters) {
    if (f.pressed(&InputState::weapon)) f.weaponBuffer = kWeaponBuffer;
    if (f.weaponBuffer <= 0) continue;
    --f.weaponBuffer;
    if (f.weapon >= 0) {
      if (!canAct(f)) continue;
      f.weaponBuffer = 0;
      switchWeapon(f);
      continue;
    }
    WeaponItem* under = nullptr;
    const Rect hurt = f.hurtbox();
    for (auto& it : items)
      if (it.life > 0 && it.character == f.charIndex && circleRect({it.pos.x, it.pos.y - 18.0f}, 34.0f, hurt)) under = &it;
    if (!under) {  // nothing to take: the press is spent
      f.weaponBuffer = 0;
      continue;
    }
    if (!canAct(f)) continue;
    f.weaponBuffer = 0;
    equipWeapon(f, under->slot);
    under->life = 0;
  }
  items.erase(std::remove_if(items.begin(), items.end(), [](const WeaponItem& i) { return i.life <= 0; }), items.end());
}
