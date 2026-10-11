// The nine fighters of Super Clash Bros.: fan-made homages to Black Clover, Bleach, Zelda: Tears of
// the Kingdom, God of War and Jujutsu Kaisen. Each one has its normals, four specials, an ultimate
// and two weapons that drop during a match.
#include "core/moves.hpp"

namespace {
using L = Limb;
using F = HitFx;
using PV = ProjVisual;

void sc(MoveDef& m, float S, float P) {
  for (auto& h : m.hitboxes) {
    h.offset = h.offset * S;
    h.radius *= S;
    h.damage *= P;
  }
  m.projectile.spawnOffset = m.projectile.spawnOffset * S;
  m.projectile.hit.damage *= P;
  m.projectile.explode.damage *= P;
  m.beam.from = m.beam.from * S;
  m.beam.length *= S;
  m.beam.width *= S;
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

ProjectileDef proj(Vec2 spawn, Vec2 vel, float grav, int life, float radius, float dmg, float bkb, float kbg, F fx,
                   PV vis) {
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

Palette pal(Color3 a, Color3 b, Color3 c, Color3 skin, Color3 hair, Color3 eyes) {
  Palette p;
  p.primary = a;
  p.secondary = b;
  p.accent = c;
  p.skin = skin;
  p.hair = hair;
  p.eyes = eyes;
  return p;
}

HitboxDef imp(HitboxDef h) {
  h.impact = true;
  return h;
}

// n light hits every `every` frames (groups g0, g0+1...): the caller adds the finisher.
void multi(std::vector<HitboxDef>& v, int start, int n, int every, float ox, float oy, float r, float dmg, L limb,
           F fx, int g0 = 0, float angle = 361.0f) {
  for (int k = 0; k < n; ++k)
    v.push_back(hb(start + k * every, start + k * every + 3, ox, oy, r, dmg, angle, 18, 6, limb, g0 + k, fx));
}

// Ultimates: invulnerable, hanging in the air; the cinematic freeze covers the first 30 frames.
MoveDef ultimate(const char* name, int duration, std::vector<HitboxDef> boxes, PoseHint pose) {
  MoveDef m = mv(name, duration, std::move(boxes));
  m.intangibleStart = 0;
  m.intangibleEnd = duration;
  m.vel = {ve(1, VelMode::Set, 0.0f, -3.0f), ve(22, VelMode::Set, 0.0f, 0.0f)};
  m.gravityMul = 0.0f;
  m.allowDrift = false;
  m.pose = pose;
  return m;
}

// Area storm around the user: light hits every `every` frames, then a launching finisher.
std::vector<HitboxDef> storm(int start, int hits, int every, float r0, float r1, float dmg, F fx, float finalDmg,
                             float finalR, float angle = 75.0f) {
  std::vector<HitboxDef> v;
  for (int k = 0; k < hits; ++k) {
    float r = r0 + (r1 - r0) * float(k) / std::max(1, hits - 1);
    v.push_back(hb(start + k * every, start + k * every + 3, 0, -10, r, dmg, 361, 10, 0, L::None, k, fx));
  }
  const int f = start + hits * every + 2;
  v.push_back(imp(hb(f, f + 6, 0, -10, finalR, finalDmg, angle, 92, 106, L::None, hits, fx)));
  return v;
}

WeaponDef weapon(const char* name, const char* desc, WeaponVisual vis, F fx, float dmg, float kb, float reach,
                 int seconds = 18) {
  WeaponDef w;
  w.name = name;
  w.desc = desc;
  w.visual = vis;
  w.fx = fx;
  w.damageMul = dmg;
  w.kbMul = kb;
  w.reachMul = reach;
  w.duration = seconds * 60;
  return w;
}

void stats(CharacterDef& c, float w, float h, float weight, float walk, float run, float air, float grav,
           float maxFall, float fastFall, float jump, float shortHop, float airJump, int squat) {
  c.width = w;
  c.height = h;
  c.weight = weight;
  c.walkSpeed = walk;
  c.runSpeed = run;
  c.airSpeed = air;
  c.gravity = grav;
  c.maxFall = maxFall;
  c.fastFall = fastFall;
  c.jumpVel = jump;
  c.shortHopVel = shortHop;
  c.airJumpVel = airJump;
  c.jumpSquat = squat;
}

// ---------------------------------------------------------------------------------------------
// Black Clover
// ---------------------------------------------------------------------------------------------
CharacterDef makeAsta() {
  CharacterDef c;
  c.name = "ASTA";
  c.title = "Le briseur de magie";
  c.series = "Black Clover";
  c.hero = Hero::Asta;
  c.style = BodyStyle::Knight;
  c.baseWeapon = WeaponVisual::AstaSword;
  c.palettes = {pal({44, 42, 50}, {96, 92, 100}, {222, 172, 64}, {238, 198, 162}, {226, 226, 214}, {80, 150, 70}),
                pal({236, 232, 220}, {60, 58, 66}, {200, 48, 52}, {238, 198, 162}, {226, 226, 214}, {80, 150, 70})};
  stats(c, 46, 98, 102, 4.3f, 9.2f, 5.3f, 0.66f, 10.5f, 15.5f, 16.0f, 10.5f, 14.5f, 3);
  c.weaponLength = 84;
  const float S = 1.05f, P = 1.28f;
  buildSwordNormals(c, S, P, F::AntiMagic);
  auto& m = c.moves;

  m[NSpecial] = mv("Lame anti-magie", 42, {});
  m[NSpecial].projectileFrame = 14;
  m[NSpecial].projectile = proj({46, -8}, {11, 0}, 0, 34, 22, 8, 30, 62, F::AntiMagic, PV::BlackSlash);
  m[NSpecial].projectile.piercing = true;
  m[NSpecial].pose = PoseHint::Overhead;

  std::vector<HitboxDef> dash;
  multi(dash, 8, 3, 4, 40, -4, 28, 2.5f, L::Weapon, F::AntiMagic);
  dash.push_back(imp(hb(20, 24, 52, -6, 32, 7.5f, 40, 55, 80, L::Weapon, 3, F::AntiMagic)));
  m[SSpecial] = mv("Météore noir", 48, dash);
  m[SSpecial].vel = {ve(8, VelMode::SetX, 15, 0), ve(8, VelMode::SetY, 0, -1.0f, 0, true),
                     ve(26, VelMode::Scale, 0.2f, 1.0f)};
  m[SSpecial].gravityMul = 0.3f;
  m[SSpecial].frictionMul = 0.1f;
  m[SSpecial].pose = PoseHint::Thrust;

  std::vector<HitboxDef> up;
  multi(up, 4, 4, 4, 0, -30, 34, 2.0f, L::Weapon, F::AntiMagic, 0, 85);
  up.push_back(hb(20, 24, 0, -50, 38, 5.5f, 80, 60, 105, L::Weapon, 4, F::AntiMagic));
  m[USpecial] = mv("Ouragan noir", 46, up);
  m[USpecial].vel = {ve(4, VelMode::Set, 1.5f, -16.5f, 3.5f)};
  m[USpecial].gravityMul = 0.5f;
  m[USpecial].helplessAfter = true;
  m[USpecial].pose = PoseHint::Spin;

  m[DSpecial] = mv("Renvoi anti-magie", 52, {imp(hb(26, 30, 60, -8, 46, 12, 361, 60, 95, L::Weapon, 0, F::AntiMagic))});
  m[DSpecial].counterStart = 5;
  m[DSpecial].counterEnd = 26;
  m[DSpecial].reflects = true;
  m[DSpecial].gravityMul = 0.3f;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  std::vector<HitboxDef> ub;
  multi(ub, 34, 5, 4, 30, -6, 74, 4.0f, L::Weapon, F::AntiMagic, 0, 0);
  m[FinalSmash] = ultimate("BLACK DIVIDER", 100, ub, PoseHint::Thrust);
  m[FinalSmash].vel = {ve(1, VelMode::Set, 0, -2), ve(22, VelMode::Set, 0, 0), ve(34, VelMode::SetX, 15, 0),
                       ve(52, VelMode::Set, 0, 0)};
  m[FinalSmash].projectileFrame = 60;
  m[FinalSmash].projectile = proj({60, -10}, {17, 0}, 0, 60, 90, 24, 92, 108, F::AntiMagic, PV::BlackSlash);
  m[FinalSmash].projectile.piercing = true;
  m[FinalSmash].projectile.hit.impact = true;

  c.weapons[0] = weapon("Épée pourfendeuse", "Lame géante : frappe plus fort et plus loin", WeaponVisual::DemonSlayer,
                        F::AntiMagic, 1.3f, 1.12f, 1.25f);
  MoveDef big = m[NSpecial];
  big.name = "Fendeur anti-magie";
  big.projectile.radius *= 1.6f;
  big.projectile.hit.radius = big.projectile.radius;
  big.projectile.hit.damage *= 1.4f;
  big.projectile.velocity.x = 13;
  big.projectile.hit.impact = true;
  c.weapons[0].moves = {{NSpecial, big}};
  c.weapons[1] = weapon("Épée habitante", "Lame rapide : la ruée devient Black Divider", WeaponVisual::DemonDweller,
                        F::AntiMagic, 1.15f, 1.05f, 1.1f);
  MoveDef div = m[SSpecial];
  div.name = "Black Divider";
  div.vel = {ve(6, VelMode::SetX, 19, 0), ve(6, VelMode::SetY, 0, -1.0f, 0, true), ve(28, VelMode::Scale, 0.2f, 1.0f)};
  div.hitboxes.clear();
  multi(div.hitboxes, 6, 5, 4, 46, -4, 32, 2.5f, L::Weapon, F::AntiMagic);
  div.hitboxes.push_back(imp(hb(26, 30, 56, -6, 38, 11, 40, 65, 92, L::Weapon, 5, F::AntiMagic)));
  sc(div, S, P);
  c.weapons[1].moves = {{SSpecial, div}};
  return c;
}

CharacterDef makeYuno() {
  CharacterDef c;
  c.name = "YUNO";
  c.title = "Le prodige du vent";
  c.series = "Black Clover";
  c.hero = Hero::Yuno;
  c.style = BodyStyle::Ninja;
  c.palettes = {pal({236, 232, 220}, {44, 52, 86}, {214, 176, 72}, {240, 206, 176}, {28, 28, 36}, {214, 170, 60}),
                pal({44, 44, 52}, {58, 72, 62}, {120, 210, 150}, {240, 206, 176}, {28, 28, 36}, {214, 170, 60})};
  stats(c, 44, 100, 84, 4.4f, 9.6f, 5.8f, 0.52f, 8.6f, 13.5f, 14.8f, 9.8f, 13.5f, 3);
  c.groundAccel = 1.4f;
  c.airAccel = 0.55f;
  c.airJumps = 2;
  const float S = 0.97f, P = 0.93f;
  buildBrawlerNormals(c, S, P, F::Wind);
  auto& m = c.moves;

  m[NSpecial] = mv("Lames de vent", 34, {});
  m[NSpecial].projectileFrame = 10;
  m[NSpecial].projectile = proj({34, -8}, {12.5f, 0}, 0, 38, 10, 3.6f, 12, 32, F::Wind, PV::WindBlade);
  m[NSpecial].projectile.count = 3;
  m[NSpecial].projectile.spreadDeg = 7;
  m[NSpecial].pose = PoseHint::Cast;

  m[SSpecial] = mv("Tempête tranchante", 46, {});
  m[SSpecial].projectileFrame = 14;
  m[SSpecial].projectile = proj({46, 0}, {4.5f, 0}, 0, 80, 30, 1.8f, 12, 8, F::Wind, PV::Tornado);
  m[SSpecial].projectile.piercing = true;
  m[SSpecial].projectile.rehit = 7;
  m[SSpecial].projectile.pull = 1.0f;
  m[SSpecial].projectile.pullRadius = 90;
  m[SSpecial].pose = PoseHint::Cast;

  m[USpecial] = mv("Envol de Sylphe", 38, {hb(8, 22, 0, 0, 26, 3.5f, 80, 45, 30, L::Body, 0, F::Wind)});
  m[USpecial].vel = {ve(5, VelMode::Set, 0, 0), ve(8, VelMode::StickDir, 20, 0), ve(24, VelMode::Scale, 0.3f, 0.3f)};
  m[USpecial].gravityMul = 0.0f;
  m[USpecial].allowDrift = false;
  m[USpecial].helplessAfter = true;
  m[USpecial].pose = PoseHint::Rush;

  std::vector<HitboxDef> vortex;
  multi(vortex, 8, 4, 5, 0, -6, 52, 2.0f, L::Body, F::Wind);
  vortex.push_back(hb(30, 34, 0, -6, 58, 6, 60, 65, 100, L::Body, 4, F::Wind));
  m[DSpecial] = mv("Tourbillon", 50, vortex);
  m[DSpecial].vel = {ve(8, VelMode::SetY, 0, -3, 0, true)};
  m[DSpecial].gravityMul = 0.4f;
  m[DSpecial].pose = PoseHint::Spin;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  m[FinalSmash] = ultimate("TEMPÊTE DE ZÉPHYR", 100, storm(34, 6, 6, 200, 360, 3, F::Wind, 18, 430, 80),
                           PoseHint::Raise);

  c.weapons[0] = weapon("Grimoire à quatre trèfles", "Cinq lames de vent d'un coup", WeaponVisual::Grimoire, F::Wind,
                        1.2f, 1.1f, 1.0f);
  MoveDef five = m[NSpecial];
  five.name = "Lames de Sylphe";
  five.projectile.count = 5;
  five.projectile.spreadDeg = 6;
  five.projectile.hit.damage *= 1.25f;
  c.weapons[0].moves = {{NSpecial, five}};
  c.weapons[1] = weapon("Lance de l'esprit", "Allonge du vent, la tempête devient une plongée", WeaponVisual::WindSpear,
                        F::Wind, 1.2f, 1.08f, 1.45f);
  std::vector<HitboxDef> dive;
  multi(dive, 8, 3, 4, 50, -4, 30, 2.5f, L::ArmFront, F::Wind);
  dive.push_back(imp(hb(20, 26, 60, -6, 36, 9, 40, 60, 86, L::ArmFront, 3, F::Wind)));
  MoveDef spirit = mv("Plongée de l'esprit", 46, dive);
  spirit.vel = {ve(8, VelMode::SetX, 16, 0), ve(8, VelMode::SetY, 0, -1, 0, true), ve(26, VelMode::Scale, 0.2f, 1.0f)};
  spirit.gravityMul = 0.3f;
  spirit.frictionMul = 0.1f;
  spirit.pose = PoseHint::Thrust;
  sc(spirit, S, P);
  c.weapons[1].moves = {{SSpecial, spirit}};
  return c;
}

// ---------------------------------------------------------------------------------------------
// Bleach
// ---------------------------------------------------------------------------------------------
CharacterDef makeIchigo() {
  CharacterDef c;
  c.name = "ICHIGO";
  c.title = "Le shinigami remplaçant";
  c.series = "Bleach";
  c.hero = Hero::Ichigo;
  c.style = BodyStyle::Knight;
  c.baseWeapon = WeaponVisual::Zangetsu;
  c.palettes = {pal({32, 32, 38}, {236, 236, 240}, {200, 50, 40}, {242, 206, 170}, {240, 128, 40}, {120, 80, 50}),
                pal({240, 240, 244}, {32, 32, 38}, {200, 30, 30}, {232, 228, 226}, {240, 128, 40}, {230, 200, 40})};
  stats(c, 46, 104, 102, 4.2f, 9.2f, 5.2f, 0.64f, 10.2f, 15.0f, 15.5f, 10.2f, 14.2f, 4);
  c.weaponLength = 88;
  const float S = 1.08f, P = 1.1f;
  buildSwordNormals(c, S, P, F::Reiatsu);
  auto& m = c.moves;

  m[NSpecial] = mv("Getsuga Tenshō", 50, {});
  m[NSpecial].projectileFrame = 18;
  m[NSpecial].projectile = proj({52, -10}, {13, 0}, 0, 40, 30, 11, 45, 80, F::Reiatsu, PV::Getsuga);
  m[NSpecial].projectile.piercing = true;
  m[NSpecial].projectile.grow = 0.5f;
  m[NSpecial].pose = PoseHint::Overhead;

  m[SSpecial] = mv("Shunpo", 40, {hb(12, 16, 56, -6, 30, 10, 40, 50, 78, L::Weapon, 0, F::Slash)});
  m[SSpecial].intangibleStart = 4;
  m[SSpecial].intangibleEnd = 12;
  m[SSpecial].vel = {ve(4, VelMode::SetX, 20, 0), ve(4, VelMode::SetY, 0, 0, 0, true), ve(12, VelMode::Scale, 0.15f, 1.0f)};
  m[SSpecial].gravityMul = 0.2f;
  m[SSpecial].frictionMul = 0.05f;
  m[SSpecial].pose = PoseHint::Thrust;

  m[USpecial] = mv("Tsukiyomi", 44, {hb(4, 7, 36, -30, 30, 11, 80, 50, 72, L::Weapon, 0, F::Reiatsu),
                                     hb(7, 14, 20, -64, 26, 6, 85, 40, 60, L::Weapon, 1, F::Reiatsu)});
  m[USpecial].vel = {ve(4, VelMode::Set, 1.5f, -17.5f, 3.0f)};
  m[USpecial].gravityMul = 0.5f;
  m[USpecial].helplessAfter = true;
  m[USpecial].pose = PoseHint::Uppercut;

  BeamDef cero;
  cero.start = 22;
  cero.end = 34;
  cero.from = {36, -14};
  cero.length = 250;
  cero.width = 44;
  cero.fx = F::Blood;
  m[DSpecial] = mv("Cero", 54, beamHits(cero, 7, 30, 50, 80, 3));
  m[DSpecial].beam = cero;
  m[DSpecial].gravityMul = 0.3f;
  m[DSpecial].pose = PoseHint::Cast;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  BeamDef mugetsu;
  mugetsu.start = 44;
  mugetsu.end = 76;
  mugetsu.from = {30, -10};
  mugetsu.length = 1100;
  mugetsu.width = 240;
  mugetsu.fx = F::Dark;
  m[FinalSmash] = ultimate("MUGETSU", 104, beamHits(mugetsu, 20, 30, 95, 110, 6), PoseHint::Overhead);
  m[FinalSmash].beam = mugetsu;

  c.weapons[0] = weapon("Zangetsu (Shikai)", "Getsuga Tenshō géant", WeaponVisual::Zangetsu, F::Reiatsu, 1.25f, 1.1f,
                        1.15f);
  MoveDef huge = m[NSpecial];
  huge.name = "Getsuga Tenshō !";
  huge.projectile.radius *= 1.45f;
  huge.projectile.hit.radius = huge.projectile.radius;
  huge.projectile.hit.damage *= 1.35f;
  huge.projectile.grow = 0.9f;
  huge.projectile.hit.impact = true;
  c.weapons[0].moves = {{NSpecial, huge}};
  c.weapons[1] = weapon("Tensa Zangetsu (Bankai)", "Vitesse du Bankai : Getsuga noir", WeaponVisual::TensaZangetsu,
                        F::Dark, 1.15f, 1.1f, 1.05f);
  MoveDef black = m[NSpecial];
  black.name = "Getsuga noir";
  black.projectile.velocity.x = 17;
  black.projectile.hit.fx = F::Dark;
  black.projectile.hit.damage *= 1.2f;
  black.projectileFrame = 12;
  black.duration = 38;
  MoveDef flash = m[SSpecial];
  flash.name = "Shunpo du Bankai";
  flash.hitboxes[0].damage *= 1.3f;
  flash.hitboxes[0].fx = F::Dark;
  flash.hitboxes[0].impact = true;
  c.weapons[1].moves = {{NSpecial, black}, {SSpecial, flash}};
  return c;
}

CharacterDef makeByakuya() {
  CharacterDef c;
  c.name = "BYAKUYA";
  c.title = "Le noble aux mille lames";
  c.series = "Bleach";
  c.hero = Hero::Byakuya;
  c.style = BodyStyle::Knight;
  c.baseWeapon = WeaponVisual::Katana;
  c.palettes = {pal({236, 236, 240}, {32, 32, 38}, {240, 150, 180}, {240, 210, 184}, {26, 26, 32}, {90, 90, 110}),
                pal({40, 40, 48}, {200, 200, 210}, {200, 120, 220}, {240, 210, 184}, {26, 26, 32}, {90, 90, 110})};
  stats(c, 44, 106, 92, 4.4f, 9.8f, 5.2f, 0.6f, 10.0f, 15.0f, 15.0f, 10.0f, 14.0f, 3);
  c.groundAccel = 1.4f;
  c.weaponLength = 72;
  const float S = 1.02f, P = 1.12f;
  buildSwordNormals(c, S, P, F::Petal);
  auto& m = c.moves;

  m[NSpecial] = mv("Senbonzakura", 46, {});
  m[NSpecial].projectileFrame = 16;
  m[NSpecial].projectile = proj({44, -12}, {5.5f, 0}, 0, 72, 32, 2.0f, 8, 6, F::Petal, PV::Petals);
  m[NSpecial].projectile.piercing = true;
  m[NSpecial].projectile.rehit = 6;
  m[NSpecial].pose = PoseHint::Raise;

  m[SSpecial] = mv("Senka", 40, {imp(hb(14, 18, 50, -8, 30, 11, 50, 55, 80, L::Weapon, 0, F::Slash))});
  m[SSpecial].teleportFrame = 10;
  m[SSpecial].teleportDist = 150;
  m[SSpecial].teleportForward = true;
  m[SSpecial].turnAfterTeleport = true;
  m[SSpecial].intangibleStart = 6;
  m[SSpecial].intangibleEnd = 14;
  m[SSpecial].vel = {ve(2, VelMode::Set, 0, 0)};
  m[SSpecial].gravityMul = 0.2f;
  m[SSpecial].pose = PoseHint::Thrust;

  m[USpecial] = mv("Shunpo céleste", 40, {hb(22, 26, 0, -6, 40, 8, 80, 55, 75, L::Body, 0, F::Petal)});
  m[USpecial].teleportFrame = 20;
  m[USpecial].teleportDist = 260;
  m[USpecial].intangibleStart = 6;
  m[USpecial].intangibleEnd = 22;
  m[USpecial].vel = {ve(2, VelMode::Set, 0, 0)};
  m[USpecial].gravityMul = 0.0f;
  m[USpecial].allowDrift = false;
  m[USpecial].helplessAfter = true;

  m[DSpecial] = mv("Danku", 52, {imp(hb(26, 30, 52, -6, 48, 12, 361, 60, 92, L::Body, 0, F::Petal))});
  m[DSpecial].counterStart = 4;
  m[DSpecial].counterEnd = 26;
  m[DSpecial].reflects = true;
  m[DSpecial].gravityMul = 0.3f;
  m[DSpecial].pose = PoseHint::HandSign;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  m[FinalSmash] = ultimate("SENBONZAKURA KAGEYOSHI", 104, storm(34, 9, 5, 300, 380, 2.2f, F::Petal, 16, 400, 70),
                           PoseHint::HandSign);

  c.weapons[0] = weapon("Senbonzakura (Shikai)", "La nuée de pétales grossit", WeaponVisual::Senbonzakura, F::Petal,
                        1.2f, 1.08f, 1.2f);
  MoveDef swarm = m[NSpecial];
  swarm.name = "Chire, Senbonzakura";
  swarm.projectile.radius *= 1.4f;
  swarm.projectile.hit.radius = swarm.projectile.radius;
  swarm.projectile.lifetime = 90;
  swarm.projectile.hit.damage *= 1.25f;
  c.weapons[0].moves = {{NSpecial, swarm}};
  c.weapons[1] = weapon("Kageyoshi (Bankai)", "Tempête de lames autour de lui", WeaponVisual::Kageyoshi, F::Petal,
                        1.25f, 1.12f, 1.3f);
  std::vector<HitboxDef> senkei;
  multi(senkei, 8, 5, 5, 0, -8, 74, 2.2f, L::Body, F::Petal);
  senkei.push_back(imp(hb(34, 38, 0, -8, 84, 9, 65, 65, 100, L::Body, 5, F::Petal)));
  MoveDef bankai = mv("Senkei", 54, senkei);
  bankai.gravityMul = 0.3f;
  bankai.pose = PoseHint::HandSign;
  sc(bankai, S, P);
  c.weapons[1].moves = {{DSpecial, bankai}};
  return c;
}

// ---------------------------------------------------------------------------------------------
// Zelda: Tears of the Kingdom
// ---------------------------------------------------------------------------------------------
CharacterDef makeLink() {
  CharacterDef c;
  c.name = "LINK";
  c.title = "Le héros d'Hyrule";
  c.series = "Zelda: Tears of the Kingdom";
  c.hero = Hero::Link;
  c.style = BodyStyle::Knight;
  c.baseWeapon = WeaponVisual::ShieldAndSword;
  c.palettes = {pal({62, 104, 170}, {120, 88, 58}, {222, 184, 90}, {244, 210, 176}, {214, 164, 92}, {70, 130, 200}),
                pal({70, 128, 70}, {120, 88, 58}, {222, 184, 90}, {244, 210, 176}, {214, 164, 92}, {70, 130, 200})};
  stats(c, 44, 98, 98, 4.2f, 8.9f, 5.1f, 0.62f, 10.2f, 15.0f, 15.2f, 10.0f, 14.2f, 4);
  c.weaponLength = 64;
  const float S = 1.0f, P = 1.12f;
  buildSwordNormals(c, S, P, F::Light);
  auto& m = c.moves;

  m[NSpecial] = mv("Arc", 40, {});
  m[NSpecial].projectileFrame = 20;
  m[NSpecial].projectile = proj({36, -14}, {15, -0.6f}, 0.14f, 60, 8, 6, 15, 50, F::Normal, PV::Arrow);
  m[NSpecial].pose = PoseHint::Bow;

  m[SSpecial] = mv("Bombe Zonaï", 38, {});
  m[SSpecial].projectileFrame = 14;
  m[SSpecial].projectile = proj({28, -26}, {6.5f, -6.5f}, 0.38f, 100, 12, 3, 10, 20, F::Fire, PV::Bomb);
  m[SSpecial].projectile.explodeRadius = 70;
  m[SSpecial].projectile.explode = hb(0, 0, 0, 0, 70, 10, 60, 50, 72, L::None, 0, F::Fire);
  m[SSpecial].pose = PoseHint::Throw;

  m[USpecial] = mv("Ascension", 44, {hb(4, 8, 0, -30, 30, 5, 85, 50, 40, L::Body, 0, F::Light)});
  m[USpecial].vel = {ve(4, VelMode::Set, 1.5f, -18.0f, 3.0f)};
  m[USpecial].intangibleStart = 4;
  m[USpecial].intangibleEnd = 18;
  m[USpecial].gravityMul = 0.45f;
  m[USpecial].helplessAfter = true;
  m[USpecial].pose = PoseHint::Raise;

  m[DSpecial] = mv("Emprise", 44, {hb(10, 18, 96, -8, 34, 0, 0, 0, 0, L::ArmFront)});
  m[DSpecial].isGrab = true;
  m[DSpecial].pose = PoseHint::Cast;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  BeamDef seal;
  seal.start = 40;
  seal.end = 68;
  seal.from = {24, -20};
  seal.length = 1100;
  seal.width = 170;
  seal.fx = F::Light;
  m[FinalSmash] = ultimate("SCEAU DE LUMIÈRE", 100, beamHits(seal, 19, 35, 92, 108, 5), PoseHint::Overhead);
  m[FinalSmash].beam = seal;

  c.weapons[0] = weapon("Épée de légende", "L'épée tire un rayon de lumière", WeaponVisual::MasterSword, F::Light, 1.25f,
                        1.1f, 1.2f);
  MoveDef beam = mv("Rayon de l'épée", 40, {});
  beam.projectileFrame = 14;
  beam.projectile = proj({50, -10}, {14, 0}, 0, 40, 20, 9, 35, 60, F::Light, PV::SwordBeam);
  beam.projectile.piercing = true;
  beam.pose = PoseHint::Overhead;
  sc(beam, S, P);
  c.weapons[0].moves = {{NSpecial, beam}};
  c.weapons[1] = weapon("Arc de lumière", "Flèches de lumière perçantes", WeaponVisual::BowOfLight, F::Light, 1.15f,
                        1.08f, 1.0f);
  MoveDef light = mv("Flèche de lumière", 38, {});
  light.projectileFrame = 16;
  light.projectile = proj({36, -14}, {20, 0}, 0, 50, 12, 13, 40, 80, F::Light, PV::LightArrow);
  light.projectile.piercing = true;
  light.projectile.hit.impact = true;
  light.pose = PoseHint::Bow;
  sc(light, S, P);
  c.weapons[1].moves = {{NSpecial, light}};
  return c;
}

CharacterDef makeGanondorf() {
  CharacterDef c;
  c.name = "GANONDORF";
  c.title = "Le Roi Démon";
  c.series = "Zelda: Tears of the Kingdom";
  c.hero = Hero::Ganondorf;
  c.style = BodyStyle::Brute;
  c.palettes = {pal({46, 42, 50}, {120, 30, 36}, {216, 176, 80}, {124, 104, 96}, {196, 58, 40}, {236, 200, 80}),
                pal({72, 32, 42}, {30, 24, 30}, {200, 60, 60}, {112, 98, 102}, {150, 40, 40}, {255, 90, 60})};
  stats(c, 64, 128, 132, 3.2f, 7.2f, 4.5f, 0.68f, 11.2f, 15.8f, 15.2f, 10.0f, 13.8f, 6);
  c.groundAccel = 0.95f;
  c.friction = 0.7f;
  c.airAccel = 0.36f;
  const float S = 1.25f, P = 1.27f;
  buildBrawlerNormals(c, S, P, F::Gloom);
  auto& m = c.moves;

  m[NSpecial] = mv("Sphère de malice", 50, {});
  m[NSpecial].projectileFrame = 20;
  m[NSpecial].projectile = proj({44, -12}, {6.5f, 0}, 0, 80, 20, 12, 40, 80, F::Gloom, PV::DarkOrb);
  m[NSpecial].projectile.grow = 0.12f;
  m[NSpecial].pose = PoseHint::Cast;

  m[SSpecial] = mv("Ruée du Roi Démon", 52, {hb(12, 28, 40, -6, 32, 12, 40, 55, 82, L::ArmFront, 0, F::Gloom)});
  m[SSpecial].armorStart = 6;
  m[SSpecial].armorEnd = 28;
  m[SSpecial].vel = {ve(12, VelMode::SetX, 12, 0), ve(30, VelMode::Scale, 0.2f, 1.0f)};
  m[SSpecial].gravityMul = 0.5f;
  m[SSpecial].frictionMul = 0.12f;
  m[SSpecial].pose = PoseHint::Rush;

  std::vector<HitboxDef> rise;
  multi(rise, 4, 3, 5, 24, -30, 30, 2.5f, L::ArmFront, F::Gloom, 0, 85);
  rise.push_back(hb(19, 23, 16, -60, 34, 6, 75, 60, 108, L::ArmFront, 3, F::Gloom));
  m[USpecial] = mv("Ascension maudite", 52, rise);
  m[USpecial].vel = {ve(4, VelMode::Set, 1.5f, -14.5f, 4.5f)};
  m[USpecial].gravityMul = 0.45f;
  m[USpecial].helplessAfter = true;
  m[USpecial].pose = PoseHint::Uppercut;

  m[DSpecial] = mv("Fléau de malice", 60, {hb(14, 60, 0, 34, 34, 11, 270, 25, 80, L::Body, 0, F::Gloom)});
  m[DSpecial].vel = {ve(2, VelMode::Set, 0, -7), ve(14, VelMode::SetY, 0, 18)};
  m[DSpecial].loopFrame = 16;
  m[DSpecial].landMove = Extra1;
  m[DSpecial].allowDrift = false;
  m[DSpecial].pose = PoseHint::Stomp;
  HitboxDef wave = imp(hb(1, 5, 60, 40, 42, 9, 55, 60, 82, L::Body, 0, F::Gloom));
  HitboxDef waveB = wave;
  waveB.offset.x = -60;
  waveB.reverse = true;
  m[Extra1] = mv("Onde de malice", 30, {wave, waveB});
  m[Extra1].pose = PoseHint::Stomp;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial, Extra1}) sc(m[id], S, P);

  BeamDef breath;
  breath.start = 46;
  breath.end = 80;
  breath.from = {30, -20};
  breath.length = 1000;
  breath.width = 260;
  breath.fx = F::Gloom;
  m[FinalSmash] = ultimate("BÊTE DU FLÉAU", 104, beamHits(breath, 21, 35, 95, 110, 6), PoseHint::Roar);
  m[FinalSmash].beam = breath;

  c.weapons[0] = weapon("Épée du Roi Démon", "Lame de malice à grande portée", WeaponVisual::DemonKingSword, F::Gloom,
                        1.3f, 1.12f, 1.5f);
  MoveDef blade = mv("Lame de malice", 52, {imp(hb(10, 22, 70, -6, 38, 15, 40, 60, 88, L::ArmFront, 0, F::Gloom))});
  blade.armorStart = 4;
  blade.armorEnd = 22;
  blade.vel = {ve(10, VelMode::SetX, 13, 0), ve(26, VelMode::Scale, 0.2f, 1.0f)};
  blade.gravityMul = 0.5f;
  blade.frictionMul = 0.12f;
  blade.pose = PoseHint::Thrust;
  sc(blade, S, P);
  c.weapons[0].moves = {{SSpecial, blade}};
  c.weapons[1] = weapon("Lances de malice", "Trois lances de malice", WeaponVisual::GloomSpear, F::Gloom, 1.15f, 1.08f,
                        1.2f);
  MoveDef spears = mv("Lances de malice", 46, {});
  spears.projectileFrame = 16;
  spears.projectile = proj({40, -16}, {13, 0}, 0, 45, 12, 7, 30, 60, F::Gloom, PV::GloomSpear);
  spears.projectile.count = 3;
  spears.projectile.spreadDeg = 8;
  spears.pose = PoseHint::Throw;
  sc(spears, S, P);
  c.weapons[1].moves = {{NSpecial, spears}};
  return c;
}

// ---------------------------------------------------------------------------------------------
// God of War
// ---------------------------------------------------------------------------------------------
CharacterDef makeKratos() {
  CharacterDef c;
  c.name = "KRATOS";
  c.title = "Le Fantôme de Sparte";
  c.series = "God of War";
  c.hero = Hero::Kratos;
  c.style = BodyStyle::Brute;
  c.baseWeapon = WeaponVisual::GuardianShield;
  c.palettes = {pal({104, 40, 34}, {96, 72, 52}, {196, 40, 36}, {214, 210, 202}, {44, 34, 30}, {200, 160, 80}),
                pal({160, 130, 60}, {80, 60, 44}, {196, 40, 36}, {214, 210, 202}, {196, 196, 196}, {200, 160, 80})};
  stats(c, 56, 114, 118, 3.6f, 8.0f, 4.8f, 0.66f, 11.0f, 15.5f, 15.6f, 10.2f, 14.0f, 5);
  c.groundAccel = 1.1f;
  const float S = 1.14f, P = 1.24f;
  buildBrawlerNormals(c, S, P, F::Fire);
  auto& m = c.moves;

  m[NSpecial] = mv("Rage spartiate", 48, {imp(hb(14, 18, 0, -10, 66, 8, 45, 70, 60, L::Body, 0, F::Fire))});
  m[NSpecial].armorStart = 4;
  m[NSpecial].armorEnd = 18;
  m[NSpecial].pose = PoseHint::Roar;

  m[SSpecial] = mv("Charge du bouclier", 46, {hb(8, 22, 36, -8, 30, 10, 40, 55, 78, L::ArmBack, 0, F::Normal)});
  m[SSpecial].armorStart = 4;
  m[SSpecial].armorEnd = 22;
  m[SSpecial].vel = {ve(8, VelMode::SetX, 12.5f, 0), ve(24, VelMode::Scale, 0.2f, 1.0f)};
  m[SSpecial].gravityMul = 0.45f;
  m[SSpecial].frictionMul = 0.12f;
  m[SSpecial].pose = PoseHint::Rush;

  m[USpecial] = mv("Bond de Sparte", 46, {hb(5, 9, 26, -26, 28, 8, 80, 55, 70, L::ArmFront, 0, F::Fire),
                                          hb(9, 16, 20, -56, 24, 5, 85, 45, 60, L::ArmFront, 1, F::Fire)});
  m[USpecial].vel = {ve(5, VelMode::Set, 1.5f, -16.0f, 3.5f)};
  m[USpecial].gravityMul = 0.5f;
  m[USpecial].helplessAfter = true;
  m[USpecial].pose = PoseHint::Uppercut;

  m[DSpecial] = mv("Parade du bouclier", 48, {imp(hb(22, 26, 48, -8, 40, 12, 361, 60, 95, L::ArmBack, 0, F::Fire))});
  m[DSpecial].counterStart = 4;
  m[DSpecial].counterEnd = 22;
  m[DSpecial].reflects = true;
  m[DSpecial].gravityMul = 0.3f;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  std::vector<HitboxDef> rage;
  multi(rage, 34, 9, 4, 40, -6, 60, 3.0f, L::ArmFront, F::Fire);
  rage.push_back(imp(hb(74, 80, 40, 0, 150, 20, 50, 95, 110, L::Body, 9, F::Fire)));
  m[FinalSmash] = ultimate("RAGE DE SPARTE", 100, rage, PoseHint::Rush);
  m[FinalSmash].vel = {ve(1, VelMode::Set, 0, -2), ve(22, VelMode::Set, 0, 0), ve(34, VelMode::SetX, 9, 0),
                       ve(70, VelMode::Set, 0, 0)};

  c.weapons[0] = weapon("Hache Léviathan", "La hache gèle et revient dans la main", WeaponVisual::LeviathanAxe, F::Ice,
                        1.25f, 1.12f, 1.35f);
  MoveDef axe = mv("Lancer de Léviathan", 36, {});
  axe.projectileFrame = 14;
  axe.projectile = proj({30, -20}, {15, 0}, 0, 56, 18, 9, 30, 62, F::Ice, PV::Axe);
  axe.projectile.boomerang = true;
  axe.projectile.piercing = true;
  axe.pose = PoseHint::Throw;
  sc(axe, S, P);
  c.weapons[0].moves = {{NSpecial, axe}};
  c.weapons[1] = weapon("Lames du Chaos", "Chaînes de feu : allonge énorme", WeaponVisual::BladesOfChaos, F::Fire, 1.15f,
                        1.08f, 1.6f);
  std::vector<HitboxDef> whirl;
  multi(whirl, 8, 4, 5, 0, -6, 104, 2.5f, L::Weapon, F::Fire);
  whirl.push_back(imp(hb(30, 34, 0, -6, 112, 8, 55, 65, 100, L::Weapon, 4, F::Fire)));
  MoveDef chaos = mv("Fléau du Chaos", 50, whirl);
  chaos.gravityMul = 0.4f;
  chaos.pose = PoseHint::Spin;
  sc(chaos, S, P);
  c.weapons[1].moves = {{NSpecial, chaos}};
  return c;
}

// ---------------------------------------------------------------------------------------------
// Jujutsu Kaisen
// ---------------------------------------------------------------------------------------------
CharacterDef makeGojo() {
  CharacterDef c;
  c.name = "GOJO";
  c.title = "Le plus fort";
  c.series = "Jujutsu Kaisen";
  c.hero = Hero::Gojo;
  c.style = BodyStyle::Ninja;
  c.palettes = {pal({30, 32, 44}, {44, 48, 64}, {90, 170, 255}, {244, 222, 206}, {244, 244, 252}, {120, 210, 255}),
                pal({236, 236, 240}, {44, 48, 64}, {180, 110, 255}, {244, 222, 206}, {244, 244, 252}, {120, 210, 255})};
  stats(c, 44, 114, 90, 4.4f, 9.4f, 5.4f, 0.58f, 9.6f, 14.5f, 15.4f, 10.0f, 14.4f, 3);
  c.groundAccel = 1.4f;
  const float S = 1.04f, P = 1.03f;
  buildBrawlerNormals(c, S, P, F::Cursed);
  auto& m = c.moves;

  m[NSpecial] = mv("Rouge", 40, {});
  m[NSpecial].projectileFrame = 16;
  m[NSpecial].projectile = proj({40, -12}, {13, 0}, 0, 30, 15, 8, 50, 68, F::Cursed, PV::RedOrb);
  m[NSpecial].projectile.explodeRadius = 62;
  m[NSpecial].projectile.explode = hb(0, 0, 0, 0, 62, 7, 40, 60, 75, L::None, 0, F::Cursed);
  m[NSpecial].pose = PoseHint::Cast;

  m[SSpecial] = mv("Bleu", 44, {});
  m[SSpecial].projectileFrame = 14;
  m[SSpecial].projectile = proj({64, -10}, {3.2f, 0}, 0, 90, 26, 1.4f, 6, 5, F::Cursed, PV::BlueOrb);
  m[SSpecial].projectile.piercing = true;
  m[SSpecial].projectile.rehit = 7;
  m[SSpecial].projectile.pull = 2.4f;
  m[SSpecial].projectile.pullRadius = 170;
  m[SSpecial].pose = PoseHint::Raise;

  m[USpecial] = mv("Téléportation", 38, {hb(20, 24, 0, -6, 40, 7, 80, 55, 70, L::Body, 0, F::Cursed)});
  m[USpecial].teleportFrame = 18;
  m[USpecial].teleportDist = 280;
  m[USpecial].intangibleStart = 6;
  m[USpecial].intangibleEnd = 20;
  m[USpecial].vel = {ve(2, VelMode::Set, 0, 0)};
  m[USpecial].gravityMul = 0.0f;
  m[USpecial].allowDrift = false;
  m[USpecial].helplessAfter = true;

  m[DSpecial] = mv("Infini", 54, {imp(hb(30, 34, 40, -6, 52, 10, 361, 70, 90, L::Body, 0, F::Cursed))});
  m[DSpecial].counterStart = 3;
  m[DSpecial].counterEnd = 30;
  m[DSpecial].reflects = true;
  m[DSpecial].gravityMul = 0.2f;
  m[DSpecial].pose = PoseHint::HandSign;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  m[FinalSmash] = ultimate("SPHÈRE DE L'INFINI", 104,
                           {hb(34, 38, 0, 0, 620, 6, 90, 10, 0, L::None, 0, F::Cursed)}, PoseHint::HandSign);
  m[FinalSmash].projectileFrame = 56;
  m[FinalSmash].projectile = proj({40, -10}, {14, 0}, 0, 80, 110, 28, 95, 110, F::Cursed, PV::PurpleOrb);
  m[FinalSmash].projectile.piercing = true;
  m[FinalSmash].projectile.hit.impact = true;

  c.weapons[0] = weapon("Technique : Violet", "Le Rouge devient un Violet qui efface tout", WeaponVisual::HollowPurple,
                        F::Cursed, 1.1f, 1.05f, 1.0f);
  MoveDef purple = mv("Violet", 52, {});
  purple.projectileFrame = 24;
  purple.projectile = proj({50, -12}, {9, 0}, 0, 70, 40, 18, 60, 90, F::Cursed, PV::PurpleOrb);
  purple.projectile.piercing = true;
  purple.projectile.hit.impact = true;
  purple.pose = PoseHint::HandSign;
  sc(purple, S, P);
  c.weapons[0].moves = {{NSpecial, purple}};
  c.weapons[1] = weapon("Six Yeux", "Bandeau retiré : tout frappe plus fort", WeaponVisual::SixEyes, F::Cursed, 1.35f,
                        1.2f, 1.0f, 15);
  return c;
}

CharacterDef makeSukuna() {
  CharacterDef c;
  c.name = "SUKUNA";
  c.title = "Le Roi des Fléaux";
  c.series = "Jujutsu Kaisen";
  c.hero = Hero::Sukuna;
  c.style = BodyStyle::Brawler;
  c.palettes = {pal({236, 232, 226}, {30, 28, 30}, {200, 40, 48}, {242, 212, 190}, {240, 150, 170}, {220, 40, 40}),
                pal({36, 36, 44}, {120, 30, 40}, {220, 60, 60}, {242, 212, 190}, {240, 150, 170}, {220, 40, 40})};
  stats(c, 46, 104, 100, 4.3f, 9.4f, 5.3f, 0.64f, 10.4f, 15.5f, 15.6f, 10.3f, 14.4f, 3);
  c.groundAccel = 1.4f;
  const float S = 1.0f, P = 0.88f;
  buildBrawlerNormals(c, S, P, F::Blood);
  auto& m = c.moves;

  m[NSpecial] = mv("Démantèlement", 30, {});
  m[NSpecial].projectileFrame = 8;
  m[NSpecial].projectile = proj({30, -8}, {16, 0}, 0, 24, 14, 3.4f, 12, 40, F::Blood, PV::Dismantle);
  m[NSpecial].projectile.count = 2;
  m[NSpecial].projectile.spreadDeg = 10;
  m[NSpecial].projectile.piercing = true;
  m[NSpecial].pose = PoseHint::Cast;

  std::vector<HitboxDef> cleave;
  multi(cleave, 6, 3, 4, 40, -4, 30, 2.5f, L::ArmFront, F::Blood);
  cleave.push_back(imp(hb(18, 22, 48, -6, 34, 8, 45, 55, 82, L::ArmFront, 3, F::Blood)));
  m[SSpecial] = mv("Clivage", 44, cleave);
  m[SSpecial].vel = {ve(6, VelMode::SetX, 13, 0), ve(6, VelMode::SetY, 0, -1, 0, true), ve(24, VelMode::Scale, 0.2f, 1.0f)};
  m[SSpecial].gravityMul = 0.3f;
  m[SSpecial].frictionMul = 0.1f;
  m[SSpecial].pose = PoseHint::Thrust;

  m[USpecial] = mv("Envol maudit", 44, {hb(4, 7, 30, -30, 28, 9, 80, 50, 70, L::ArmFront, 0, F::Blood),
                                        hb(7, 14, 18, -62, 24, 5, 85, 40, 60, L::ArmFront, 1, F::Blood)});
  m[USpecial].vel = {ve(4, VelMode::Set, 1.5f, -16.8f, 3.0f)};
  m[USpecial].gravityMul = 0.5f;
  m[USpecial].helplessAfter = true;
  m[USpecial].pose = PoseHint::Uppercut;

  m[DSpecial] = mv("Fûga", 56, {});
  m[DSpecial].projectileFrame = 26;
  m[DSpecial].projectile = proj({36, -12}, {11, 0}, 0, 60, 15, 7, 30, 60, F::Fire, PV::FireArrow);
  m[DSpecial].projectile.explodeRadius = 92;
  m[DSpecial].projectile.explode = hb(0, 0, 0, 0, 92, 12, 50, 60, 85, L::None, 0, F::Fire);
  m[DSpecial].projectile.explode.impact = true;
  m[DSpecial].pose = PoseHint::Bow;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  m[FinalSmash] = ultimate("SANCTUAIRE DÉMONIAQUE", 110, storm(34, 11, 4, 480, 540, 1.8f, F::Blood, 16, 560, 70),
                           PoseHint::HandSign);
  m[FinalSmash].hitboxes.back().fx = F::Fire;

  c.weapons[0] = weapon("Kamutoke", "La foudre remplace le Démantèlement", WeaponVisual::Kamutoke, F::Electric, 1.15f,
                        1.08f, 1.1f);
  MoveDef bolt = mv("Kamutoke", 40, {});
  bolt.projectileFrame = 14;
  bolt.projectile = proj({40, -14}, {18, 0}, 0, 36, 20, 12, 45, 75, F::Electric, PV::Lightning);
  bolt.projectile.piercing = true;
  bolt.projectile.hit.impact = true;
  bolt.pose = PoseHint::Raise;
  sc(bolt, S, P);
  c.weapons[0].moves = {{NSpecial, bolt}};
  c.weapons[1] = weapon("Doigt de Sukuna", "Puissance maudite : tout frappe plus fort", WeaponVisual::CursedFinger,
                        F::Blood, 1.35f, 1.2f, 1.0f, 15);
  return c;
}
}  // namespace

const std::vector<CharacterDef>& roster() {
  static const std::vector<CharacterDef> r = {makeAsta(),   makeYuno(),   makeIchigo(), makeByakuya(), makeLink(),
                                              makeGanondorf(), makeKratos(), makeGojo(), makeSukuna()};
  return r;
}
