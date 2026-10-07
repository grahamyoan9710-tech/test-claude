// The six fighters of Super Clash Bros.
#include "core/moves.hpp"

namespace {
using L = Limb;

void sc(MoveDef& m, float S, float P) {
  for (auto& h : m.hitboxes) {
    h.offset = h.offset * S;
    h.radius *= S;
    h.damage *= P;
  }
  m.projectile.spawnOffset = m.projectile.spawnOffset * S;
  m.projectile.hit.damage *= P;
}

VelEvent ve(int frame, VelMode mode, float x, float y, float stickX = 0.0f, bool airOnly = false) {
  VelEvent e;
  e.frame = frame;
  e.mode = mode;
  e.v = {x, y};
  e.stickX = stickX;
  e.airOnly = airOnly;
  return e;
}

ProjectileDef proj(Vec2 spawn, Vec2 vel, float grav, int life, float radius, float dmg, float bkb,
                   float kbg, HitFx fx, ProjVisual vis) {
  ProjectileDef p;
  p.spawnOffset = spawn;
  p.velocity = vel;
  p.gravity = grav;
  p.lifetime = life;
  p.radius = radius;
  p.hit = hb(0, 0, 0, 0, radius, dmg, 361, bkb, kbg, L::None, 0, fx);
  p.visual = vis;
  return p;
}

Palette pal(Color3 a, Color3 b, Color3 c, Color3 skin) { return {a, b, c, skin}; }

CharacterDef makeBlaze() {
  CharacterDef c;
  c.name = "BLAZE";
  c.title = "Le poing ardent";
  c.style = BodyStyle::Brawler;
  c.palettes = {pal({215, 48, 40}, {45, 60, 140}, {255, 170, 40}, {240, 200, 160}),
                pal({240, 240, 240}, {200, 40, 40}, {255, 120, 30}, {240, 200, 160})};
  c.width = 46; c.height = 100; c.weight = 98;
  c.walkSpeed = 4.0f; c.runSpeed = 8.2f; c.airSpeed = 5.2f;
  c.gravity = 0.6f; c.maxFall = 10.0f; c.fastFall = 15.0f;
  c.jumpVel = 15.0f; c.shortHopVel = 10.0f; c.airJumpVel = 14.0f; c.jumpSquat = 3;
  const float S = 1.0f, P = 0.95f;
  buildBrawlerNormals(c, S, P, HitFx::Fire);
  auto& m = c.moves;
  m[FAir] = mv("Poing meteore", 46, {hb(14, 18, 42, 10, 24, 14, 285, 20, 85, L::ArmFront, 0, HitFx::Fire)}, 16);

  m[NSpecial] = mv("Boule de feu", 38, {});
  m[NSpecial].projectileFrame = 14;
  m[NSpecial].projectile = proj({40, -10}, {7.5f, -1.0f}, 0.3f, 100, 12, 5, 15, 35, HitFx::Fire, ProjVisual::Fireball);
  m[NSpecial].projectile.bounces = true;

  m[SSpecial] = mv("Poing comete", 42, {hb(8, 18, 36, -6, 24, 11, 40, 50, 78, L::ArmFront, 0, HitFx::Fire)});
  m[SSpecial].vel = {ve(8, VelMode::SetX, 13, 0), ve(8, VelMode::SetY, 0, -1.5f, 0, true),
                     ve(20, VelMode::Scale, 0.3f, 1.0f)};
  m[SSpecial].gravityMul = 0.35f; m[SSpecial].frictionMul = 0.15f;

  m[USpecial] = mv("Uppercut solaire", 48, {
      hb(3, 6, 26, -24, 24, 7, 80, 55, 50, L::ArmFront, 0, HitFx::Fire),
      hb(6, 9, 16, -56, 22, 2, 90, 40, 0, L::ArmFront, 1, HitFx::Fire),
      hb(9, 12, 16, -56, 22, 2, 90, 40, 0, L::ArmFront, 2, HitFx::Fire),
      hb(12, 15, 16, -56, 22, 2, 90, 40, 0, L::ArmFront, 3, HitFx::Fire),
      hb(15, 19, 12, -64, 26, 5, 75, 60, 110, L::ArmFront, 4, HitFx::Fire)});
  m[USpecial].vel = {ve(3, VelMode::Set, 1.5f, -15.5f, 3.0f)};
  m[USpecial].gravityMul = 0.5f; m[USpecial].helplessAfter = true;

  std::vector<HitboxDef> spin;
  for (int k = 0; k < 5; ++k) spin.push_back(hb(8 + 6 * k, 11 + 6 * k, 0, -4, 42, 2.5f, 361, 25, 20, L::Body, k));
  spin.push_back(hb(38, 41, 0, -4, 46, 5, 55, 60, 115, L::Body, 5, HitFx::Fire));
  m[DSpecial] = mv("Tornade", 50, spin);
  m[DSpecial].vel = {ve(8, VelMode::SetY, 0, -4.0f, 0, true)};
  m[DSpecial].gravityMul = 0.35f; m[DSpecial].frictionMul = 0.5f;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);
  return c;
}

CharacterDef makeVolt() {
  CharacterDef c;
  c.name = "VOLT";
  c.title = "L'eclair vivant";
  c.style = BodyStyle::Speedster;
  c.palettes = {pal({250, 210, 40}, {70, 60, 60}, {90, 220, 255}, {250, 210, 40}),
                pal({250, 140, 40}, {60, 40, 40}, {255, 255, 120}, {250, 140, 40})};
  c.width = 44; c.height = 85; c.weight = 76;
  c.walkSpeed = 4.6f; c.runSpeed = 10.2f; c.groundAccel = 1.6f; c.airSpeed = 5.6f; c.airAccel = 0.5f;
  c.gravity = 0.75f; c.maxFall = 11.5f; c.fastFall = 17.0f;
  c.jumpVel = 16.5f; c.shortHopVel = 11.0f; c.airJumpVel = 15.0f; c.jumpSquat = 3;
  const float S = 0.85f, P = 1.0f;
  buildBrawlerNormals(c, S, P, HitFx::Electric);
  auto& m = c.moves;

  m[NSpecial] = mv("Eclair", 34, {});
  m[NSpecial].projectileFrame = 10;
  m[NSpecial].projectile = proj({34, -6}, {11, 0}, 0, 60, 11, 5.3f, 12, 25, HitFx::Electric, ProjVisual::Bolt);

  m[SSpecial] = mv("Charge foudre", 52, {hb(10, 26, 22, -4, 26, 9.5f, 40, 40, 75, L::Body, 0, HitFx::Electric)});
  m[SSpecial].chargeFrame = 6; m[SSpecial].maxCharge = 45; m[SSpecial].chargeDamageMul = 1.0f;
  m[SSpecial].vel = {ve(10, VelMode::SetX, 14, 0), ve(10, VelMode::SetY, 0, -2, 0, true),
                     ve(28, VelMode::Scale, 0.2f, 1.0f)};
  m[SSpecial].gravityMul = 0.25f; m[SSpecial].frictionMul = 0.1f;

  m[USpecial] = mv("Zip", 36, {hb(8, 22, 0, 0, 26, 3.5f, 80, 45, 30, L::Body, 0, HitFx::Electric)});
  m[USpecial].vel = {ve(5, VelMode::Set, 0, 0), ve(8, VelMode::StickDir, 21, 0), ve(22, VelMode::Scale, 0.25f, 0.25f)};
  m[USpecial].gravityMul = 0.0f; m[USpecial].allowDrift = false; m[USpecial].helplessAfter = true;

  m[DSpecial] = mv("Tonnerre", 54, {hb(16, 26, 0, -10, 58, 15, 80, 60, 88, L::Body, 0, HitFx::Electric)});
  m[DSpecial].gravityMul = 0.5f;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);
  return c;
}

CharacterDef makeTitan() {
  CharacterDef c;
  c.name = "TITAN";
  c.title = "Le colosse de roc";
  c.style = BodyStyle::Brute;
  c.palettes = {pal({70, 140, 60}, {120, 80, 40}, {240, 230, 200}, {95, 165, 85}),
                pal({130, 60, 140}, {60, 50, 60}, {255, 200, 80}, {165, 115, 175})};
  c.width = 68; c.height = 125; c.weight = 132;
  c.walkSpeed = 3.2f; c.runSpeed = 6.8f; c.groundAccel = 0.9f; c.friction = 0.7f;
  c.airSpeed = 4.4f; c.airAccel = 0.35f;
  c.gravity = 0.65f; c.maxFall = 11.0f; c.fastFall = 15.5f;
  c.jumpVel = 15.0f; c.shortHopVel = 10.0f; c.airJumpVel = 13.5f; c.jumpSquat = 6;
  const float S = 1.25f, P = 1.3f;
  buildBrawlerNormals(c, S, P, HitFx::Normal);
  auto& m = c.moves;

  m[NSpecial] = mv("Poing titan", 54, {hb(18, 22, 62, -10, 32, 12, 361, 35, 95, L::ArmFront)});
  m[NSpecial].chargeFrame = 8; m[NSpecial].maxCharge = 90; m[NSpecial].chargeDamageMul = 1.3f;

  m[SSpecial] = mv("Charge belier", 54, {hb(10, 32, 40, -6, 32, 10, 40, 55, 80, L::Body)});
  m[SSpecial].armorStart = 4; m[SSpecial].armorEnd = 32;
  m[SSpecial].vel = {ve(10, VelMode::SetX, 11, 0), ve(34, VelMode::Scale, 0.2f, 1.0f)};
  m[SSpecial].gravityMul = 0.5f; m[SSpecial].frictionMul = 0.12f;

  std::vector<HitboxDef> fort;
  for (int k = 0; k < 3; ++k) fort.push_back(hb(4 + 6 * k, 8 + 6 * k, 0, -10, 50, 2.5f, 80, 40, 30, L::Body, k));
  fort.push_back(hb(22, 26, 0, -16, 54, 5, 60, 60, 105, L::Body, 3));
  m[USpecial] = mv("Forteresse", 58, fort);
  m[USpecial].vel = {ve(4, VelMode::Set, 0, -12.5f, 5.0f)};
  m[USpecial].gravityMul = 0.4f; m[USpecial].helplessAfter = true;

  m[DSpecial] = mv("Seisme", 60, {hb(14, 60, 0, 34, 34, 11, 270, 25, 80, L::Body)});
  m[DSpecial].vel = {ve(2, VelMode::Set, 0, -7), ve(14, VelMode::SetY, 0, 18)};
  m[DSpecial].loopFrame = 16; m[DSpecial].landMove = Extra1; m[DSpecial].allowDrift = false;
  HitboxDef wave = hb(1, 5, 60, 40, 42, 8, 55, 60, 80, L::Body);
  HitboxDef waveB = wave;
  waveB.offset.x = -60; waveB.reverse = true;
  m[Extra1] = mv("Onde sismique", 30, {wave, waveB});
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial, Extra1}) sc(m[id], S, P);
  return c;
}

CharacterDef makeAria() {
  CharacterDef c;
  c.name = "ARIA";
  c.title = "La lame azur";
  c.style = BodyStyle::Knight;
  c.palettes = {pal({50, 90, 200}, {225, 230, 240}, {240, 200, 60}, {245, 210, 180}),
                pal({200, 40, 80}, {50, 45, 60}, {240, 240, 240}, {245, 210, 180})};
  c.width = 44; c.height = 105; c.weight = 90;
  c.walkSpeed = 4.2f; c.runSpeed = 8.8f; c.airSpeed = 5.0f;
  c.gravity = 0.6f; c.maxFall = 10.0f; c.fastFall = 15.0f;
  c.jumpVel = 15.0f; c.shortHopVel = 10.0f; c.airJumpVel = 14.0f; c.jumpSquat = 4;
  c.weaponLength = 74;
  const float S = 1.05f, P = 1.1f;
  buildSwordNormals(c, S, P);
  auto& m = c.moves;
  const HitFx sl = HitFx::Slash;

  m[NSpecial] = mv("Brise-garde", 52, {hb(20, 24, 86, -4, 22, 9, 361, 40, 92, L::Weapon, 0, sl),
                                       hb(20, 24, 48, -4, 20, 7, 361, 40, 85, L::Weapon, 0, sl)});
  m[NSpecial].chargeFrame = 10; m[NSpecial].chargeDamageMul = 1.2f;

  m[SSpecial] = mv("Lame dansante", 34, {hb(6, 9, 66, -8, 28, 4, 70, 30, 20, L::Weapon, 0, sl)});
  m[SSpecial].nextMove = Extra1; m[SSpecial].comboOnSpecial = true;
  m[SSpecial].comboStart = 6; m[SSpecial].comboEnd = 28; m[SSpecial].comboAt = 14;
  m[SSpecial].vel = {ve(0, VelMode::SetY, 0, -2, 0, true)};
  m[SSpecial].gravityMul = 0.4f;
  m[Extra1] = mv("Lame dansante 2", 34, {hb(6, 9, 66, -26, 28, 4, 80, 30, 20, L::Weapon, 0, sl)});
  m[Extra1].nextMove = Extra2; m[Extra1].comboOnSpecial = true;
  m[Extra1].comboStart = 6; m[Extra1].comboEnd = 28; m[Extra1].comboAt = 14;
  m[Extra1].gravityMul = 0.4f;
  m[Extra2] = mv("Lame dansante 3", 44, {hb(8, 12, 70, -10, 32, 8, 361, 45, 105, L::Weapon, 0, sl)});
  m[Extra2].gravityMul = 0.5f;

  m[USpecial] = mv("Envol du dauphin", 44, {hb(4, 7, 36, -30, 28, 11, 80, 50, 70, L::Weapon, 0, sl),
                                             hb(7, 14, 20, -62, 24, 6, 85, 40, 60, L::Weapon, 0, sl)});
  m[USpecial].vel = {ve(4, VelMode::Set, 1.5f, -17.0f, 3.0f)};
  m[USpecial].gravityMul = 0.5f; m[USpecial].helplessAfter = true;

  m[DSpecial] = mv("Contre", 54, {hb(28, 32, 62, -10, 44, 10, 361, 60, 92, L::Weapon, 0, sl)});
  m[DSpecial].counterStart = 5; m[DSpecial].counterEnd = 28; m[DSpecial].gravityMul = 0.3f;
  for (int id : {NSpecial, SSpecial, Extra1, Extra2, USpecial, DSpecial}) sc(m[id], S, P);
  return c;
}

CharacterDef makePuffo() {
  CharacterDef c;
  c.name = "PUFFO";
  c.title = "La boule celeste";
  c.style = BodyStyle::Puff;
  c.palettes = {pal({255, 150, 190}, {230, 60, 110}, {255, 255, 255}, {255, 150, 190}),
                pal({150, 200, 255}, {60, 100, 220}, {255, 255, 255}, {150, 200, 255})};
  c.width = 52; c.height = 80; c.weight = 68;
  c.walkSpeed = 3.5f; c.runSpeed = 6.8f; c.airSpeed = 6.0f; c.airAccel = 0.6f;
  c.gravity = 0.38f; c.maxFall = 6.5f; c.fastFall = 10.0f;
  c.jumpVel = 11.5f; c.shortHopVel = 7.5f; c.airJumpVel = 10.5f; c.airJumps = 5; c.jumpSquat = 3;
  c.weaponLength = 56;
  const float S = 0.8f, P = 0.95f;
  buildBrawlerNormals(c, S, P, HitFx::Magic);
  auto& m = c.moves;
  m[BAir].hitboxes[0].damage = 13.0f;  // a feared back kick

  m[NSpecial] = mv("Etoile", 40, {});
  m[NSpecial].projectileFrame = 14;
  m[NSpecial].projectile = proj({36, -6}, {6.5f, 0}, 0, 100, 16, 7, 30, 55, HitFx::Magic, ProjVisual::Star);

  m[SSpecial] = mv("Marteau", 52, {hb(17, 21, 60, -6, 32, 16, 361, 40, 92, L::Weapon)});
  m[SSpecial].gravityMul = 0.5f;

  m[USpecial] = mv("Tornade etoilee", 52, {hb(4, 14, 26, -20, 26, 6, 85, 60, 40, L::Weapon, 0, HitFx::Magic),
                                            hb(22, 52, 30, 16, 24, 5, 270, 30, 30, L::Weapon, 1, HitFx::Magic)});
  m[USpecial].vel = {ve(4, VelMode::Set, 1.5f, -14, 2.0f), ve(22, VelMode::Set, 0.5f, 15)};
  m[USpecial].gravityMul = 0.3f; m[USpecial].helplessAfter = true; m[USpecial].endOnLand = true;

  m[DSpecial] = mv("Pierre", 70, {hb(6, 62, 0, 10, 32, 14, 270, 30, 80, L::Body)});
  m[DSpecial].armorStart = 6; m[DSpecial].armorEnd = 62;
  m[DSpecial].vel = {ve(6, VelMode::Set, 0, 13)};
  m[DSpecial].gravityMul = 2.0f; m[DSpecial].allowDrift = false;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);
  return c;
}

CharacterDef makeShade() {
  CharacterDef c;
  c.name = "SHADE";
  c.title = "L'ombre silencieuse";
  c.style = BodyStyle::Ninja;
  c.palettes = {pal({90, 50, 140}, {35, 30, 50}, {230, 50, 80}, {220, 190, 170}),
                pal({45, 45, 50}, {20, 20, 25}, {80, 220, 160}, {220, 190, 170})};
  c.width = 42; c.height = 95; c.weight = 80;
  c.walkSpeed = 4.6f; c.runSpeed = 10.5f; c.groundAccel = 1.6f; c.airSpeed = 5.4f;
  c.gravity = 0.7f; c.maxFall = 11.0f; c.fastFall = 16.5f;
  c.jumpVel = 17.0f; c.shortHopVel = 11.0f; c.airJumpVel = 15.0f; c.jumpSquat = 3;
  c.weaponLength = 30;
  const float S = 0.95f, P = 0.85f;
  buildBrawlerNormals(c, S, P, HitFx::Dark);
  auto& m = c.moves;

  m[NSpecial] = mv("Shuriken", 32, {});
  m[NSpecial].projectileFrame = 9;
  m[NSpecial].projectile = proj({30, -12}, {13, 0}, 0.04f, 50, 9, 3.5f, 6, 25, HitFx::Dark, ProjVisual::Shuriken);
  m[NSpecial].projectile.count = 3;
  m[NSpecial].projectile.spreadDeg = 9;

  m[SSpecial] = mv("Lame eclipse", 40, {hb(6, 16, 10, -4, 30, 10.5f, 40, 40, 72, L::Weapon, 0, HitFx::Slash)});
  m[SSpecial].intangibleStart = 6; m[SSpecial].intangibleEnd = 14;
  m[SSpecial].vel = {ve(6, VelMode::SetX, 17, 0), ve(6, VelMode::SetY, 0, 0, 0, true), ve(16, VelMode::Scale, 0.15f, 1.0f)};
  m[SSpecial].gravityMul = 0.3f; m[SSpecial].frictionMul = 0.05f;

  m[USpecial] = mv("Disparition", 42, {hb(26, 31, 0, -6, 42, 11, 80, 60, 80, L::Body, 0, HitFx::Dark)});
  m[USpecial].intangibleStart = 8; m[USpecial].intangibleEnd = 28;
  m[USpecial].teleportFrame = 26; m[USpecial].teleportDist = 270;
  m[USpecial].vel = {ve(2, VelMode::Set, 0, 0)};
  m[USpecial].gravityMul = 0.0f; m[USpecial].allowDrift = false; m[USpecial].helplessAfter = true;

  m[DSpecial] = mv("Contre-ombre", 50, {hb(24, 28, 56, -6, 42, 12, 361, 60, 95, L::Weapon, 0, HitFx::Dark)});
  m[DSpecial].counterStart = 4; m[DSpecial].counterEnd = 24; m[DSpecial].gravityMul = 0.3f;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);
  return c;
}
}  // namespace

const std::vector<CharacterDef>& roster() {
  static const std::vector<CharacterDef> r = {makeBlaze(), makeVolt(), makeTitan(),
                                              makeAria(), makePuffo(), makeShade()};
  return r;
}
