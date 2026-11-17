// A transformed fighter's aura: its PixelLab effect (FormDef::aura), looping around it, behind the fighter; else the
// aura of the weapon it holds (WeaponDef::aura).
#include <cmath>
#include <cstring>

#include "game/fighter_art.hpp"

void drawFormAvatar(const Fighter& f, float time, bool front) {
  const FormDef* fd = f.formDef();
  const WeaponDef* wd = f.weaponDef();
  const char* aura = fd && fd->aura ? fd->aura : wd ? wd->aura : nullptr;
  if (front || !aura || !f.active() || (!kVfx && std::strncmp(aura, "pfx_", 4) != 0)) return;  // (ParticleFX: always)
  art::drawFx(aura, art::fxLoopTime(aura, time), f.pos, art::fxSize(aura, *f.def), 0.0f, f.facing < 0, 0.95f);
}
