// A transformed fighter's aura: its PixelLab effect (FormDef::aura), looping around it, behind the fighter.
#include <cmath>

#include "game/fighter_art.hpp"

void drawFormAvatar(const Fighter& f, float time, bool front) {
  const FormDef* fd = f.formDef();
  if (front || !fd || !fd->aura || !f.active()) return;
  art::drawFx(fd->aura, time, f.pos, f.def->height / 170.0f, 0.0f, f.facing < 0, 0.95f);
}
