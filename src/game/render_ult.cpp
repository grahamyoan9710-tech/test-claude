// A transformed fighter's aura: its PixelLab effect (FormDef::aura), looping around it, behind the fighter.
#include <cmath>
#include <cstring>

#include "game/fighter_art.hpp"

void drawFormAvatar(const Fighter& f, float time, bool front) {
  const FormDef* fd = f.formDef();
  if (front || !fd || !fd->aura || !f.active() || (!kVfx && std::strncmp(fd->aura, "pfx_", 4) != 0)) return;  // (ParticleFX: always)
  art::drawFx(fd->aura, art::fxLoopTime(fd->aura, time), f.pos, art::fxSize(fd->aura, *f.def), 0.0f, f.facing < 0, 0.95f);
}
