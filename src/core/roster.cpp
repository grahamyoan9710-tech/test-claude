// The fighters of Super Clash Bros. (docs/refonte/FICHES.md). They have no names yet: each one is a number
// (P01..P17), its weapons NN-A and NN-B, its transformation TNN. Five are playable for now (01, 02, 03, 07, 08);
// the twelve others come back with their clips (their kits are in the git history, commit c349427).
// Every fighter starts bare-handed (the Smash normals, four skills and an ultimate, or its transformation) and
// takes its weapon where it drops with the weapon button, which then switches its two weapons: each weapon is a
// kit (a 3-hit M1, an air M1, four skills and an ultimate).
//
// Every move names its PixelLab effect "<id>_<x|a|b>_<n|s|u|d|ult>" (x: bare hands, a / b: the weapons); until
// that effect is imported, the game draws a placeholder.
#include <cmath>
#include <cstdio>
#include <deque>
#include <iterator>
#include <string>

#include "core/roster_kit.hpp"

namespace {
using namespace kit;

// ---------------------------------------------------------------------------------------------
// Authoring helpers
// ---------------------------------------------------------------------------------------------
// The effect names live as long as the roster.
const char* intern(const std::string& s) {
  static std::deque<std::string> pool;
  for (const auto& p : pool)
    if (p == s) return p.c_str();
  pool.push_back(s);
  return pool.back().c_str();
}

enum class Build { Small, Light, Medium, Heavy, Huge };

struct Fighter {
  CharacterDef c;
  float S = 1.0f, P = 1.0f;
  F e = F::Normal;
  const char* fx(const char* slot, const char* what) const { return intern(c.id + "_" + slot + "_" + what); }
};

Fighter start(int number, const char* element, Build b, F e, Palette main, Palette alt) {
  Fighter k;
  CharacterDef& c = k.c;
  char id[8], name[16];
  std::snprintf(id, sizeof id, "p%02d", number);
  std::snprintf(name, sizeof name, "N° %02d", number);
  c.id = id;
  c.name = name;
  c.title = element;
  c.series = element;
  c.hero = Hero(number - 1);
  c.element = e;
  c.palettes = {main, alt};
  switch (b) {
    case Build::Small: stats(c, 40, 86, 82, 4.4f, 9.4f, 5.6f, 0.62f, 10.0f, 15.0f, 16.0f, 10.5f, 15.0f, 3); break;
    case Build::Light: stats(c, 42, 96, 88, 4.6f, 9.8f, 5.6f, 0.64f, 10.2f, 15.5f, 16.0f, 10.5f, 15.0f, 3); break;
    case Build::Medium: stats(c, 46, 100, 100, 4.3f, 9.0f, 5.2f, 0.66f, 10.5f, 15.5f, 16.0f, 10.5f, 14.5f, 3); break;
    case Build::Heavy: stats(c, 50, 104, 112, 4.0f, 8.4f, 4.9f, 0.70f, 11.0f, 16.0f, 16.0f, 10.5f, 14.0f, 4); break;
    case Build::Huge: stats(c, 54, 108, 122, 3.8f, 8.0f, 4.7f, 0.72f, 11.5f, 16.5f, 15.5f, 10.0f, 13.5f, 4); break;
  }
  c.style = b == Build::Huge || b == Build::Heavy ? BodyStyle::Brute
            : b == Build::Small || b == Build::Light ? BodyStyle::Speedster : BodyStyle::Brawler;
  k.S = c.height / 100.0f;
  k.P = 1.0f;
  k.e = e;
  buildBrawlerNormals(c, k.S, k.P, e);
  return k;
}

// The four skills of the bare hands (slot < 0) or of a weapon, scaled to the fighter.
void skills(Fighter& k, int slot, MoveDef n, MoveDef s, MoveDef u, MoveDef d) {
  for (MoveDef* m : {&n, &s, &u, &d}) sc(*m, k.S, k.P);
  if (slot < 0) {
    k.c.moves[NSpecial] = n, k.c.moves[SSpecial] = s, k.c.moves[USpecial] = u, k.c.moves[DSpecial] = d;
    return;
  }
  auto& w = k.c.weapons[slot].moves;
  w.insert(w.end(), {{NSpecial, n}, {SSpecial, s}, {USpecial, u}, {DSpecial, d}});
}

void ultimateOf(Fighter& k, int slot, MoveDef m) {
  if (slot < 0) k.c.moves[FinalSmash] = std::move(m);
  else k.c.weapons[slot].moves.push_back({FinalSmash, std::move(m)});
}

enum class Kit { Blade, Lance, Fist, Gauntlet, Magic };

// A weapon: its M1 combo comes from the kit shape, renamed after its sheet; `blade`: grip to tip (0 for fists).
WeaponDef& arm(Fighter& k, int slot, const char* name, const char* desc, WeaponVisual vis, Kit shape, float blade,
               float dmg, std::initializer_list<const char*> m1, float speed = 1.0f) {
  WeaponDef& w = k.c.weapons[slot];
  w = weapon(name, desc, vis, k.e, dmg, 1.05f, 1.0f);
  w.blade = blade * k.S;
  switch (shape) {
    case Kit::Blade: addBladeKit(w, k.S, k.P, k.e, speed); break;
    case Kit::Lance: addLanceKit(w, k.S, k.P, k.e, speed); break;
    case Kit::Fist: addFistKit(w, k.S, k.P, k.e, speed); break;
    case Kit::Gauntlet: addFistKit(w, k.S, k.P, k.e, speed, 1.25f); break;
    case Kit::Magic: addFistKit(w, k.S, k.P, k.e, speed, 1.5f); break;
  }
  int i = 0;
  for (const char* n : m1) {
    for (auto& [id, m] : w.moves)
      if (id == Jab + i) m.name = n;
    ++i;
  }
  return w;
}

// --- skill shapes (a 100 px fighter facing right; skills() scales them) ---
// A projectile fired forward.
MoveDef shot(const char* name, F e, PV vis, const char* fx, float speed, float radius, float dmg, int life = 50) {
  MoveDef m = mv(name, 40, {});
  m.projectileFrame = 14;
  m.projectile = proj({42, -10}, {speed, 0}, 0, life, radius, dmg, 30, 55, e, vis);
  m.projectile.fx = fx;
  m.pose = PoseHint::Cast;
  return m;
}
MoveDef piercing(MoveDef m, int rehit = 0) {
  m.projectile.piercing = true;
  m.projectile.rehit = rehit;
  return m;
}
MoveDef boomerang(MoveDef m) {
  m.projectile.boomerang = true;
  m.projectile.piercing = true;
  m.pose = PoseHint::Throw;
  return m;
}
MoveDef lob(MoveDef m, float vy, float gravity) {
  m.projectile.velocity.y = vy;
  m.projectile.gravity = gravity;
  m.pose = PoseHint::Throw;
  return m;
}
MoveDef along(MoveDef m) {  // runs along the ground
  m.projectile.spawnOffset = {44, 34};
  m.projectile.piercing = true;
  m.pose = PoseHint::Stomp;
  return m;
}
MoveDef spread(MoveDef m, int count, float deg) {
  m.projectile.count = count;
  m.projectile.spreadDeg = deg;
  return m;
}
MoveDef exploding(MoveDef m, float radius, float dmg) {
  m.projectile.explodeRadius = radius;
  m.projectile.explode = imp(hb(0, 0, 0, 0, radius, dmg, 60, 50, 85, L::None, 0, m.projectile.hit.fx));
  return m;
}
MoveDef pulling(MoveDef m, float strength, float radius) {
  m.projectile.pull = strength;
  m.projectile.pullRadius = radius;
  m.projectile.piercing = true;
  m.projectile.rehit = 10;
  return m;
}

// A dash forward: `hits` light hits on the way, then a launching blow.
MoveDef rush(const char* name, F e, const char* fx, float speed, int hits, float dmg, Limb limb = L::Weapon) {
  std::vector<HitboxDef> v;
  multi(v, 8, hits, 4, 40, -4, 28, 2.5f, limb, e);
  const int f = 8 + hits * 4 + 2;
  v.push_back(imp(hb(f, f + 4, 50, -6, 32, dmg, 40, 55, 80, limb, hits, e)));
  MoveDef m = mv(name, f + 22, v);
  m.vel = {ve(6, VelMode::SetX, speed, 0), ve(6, VelMode::SetY, 0, -1.0f, 0, true), ve(f + 2, VelMode::Scale, 0.2f, 1.0f)};
  m.gravityMul = 0.3f;
  m.frictionMul = 0.1f;
  m.pose = PoseHint::Rush;
  m.castFx = fx;
  m.castFxFrame = 6;
  m.castFxStays = true;
  return m;
}

// Vanishes and strikes from behind the opponent.
MoveDef through(const char* name, F e, const char* fx, float dmg) {
  MoveDef m = mv(name, 40, {imp(hb(14, 18, 50, -8, 30, dmg, 50, 55, 80, L::Weapon, 0, e))});
  m.teleportFrame = 10;
  m.teleportDist = 150;
  m.teleportForward = true;
  m.turnAfterTeleport = true;
  m.intangibleStart = 6;
  m.intangibleEnd = 14;
  m.vel = {ve(2, VelMode::Set, 0, 0)};
  m.gravityMul = 0.2f;
  m.pose = PoseHint::Thrust;
  m.castFx = fx;
  m.castFxFrame = 6;
  m.castFxStays = true;
  return m;
}

// The recovery: rises with `hits` hits on the way up, helpless after.
MoveDef rise(const char* name, F e, const char* fx, float vy, int hits, Limb limb = L::Weapon) {
  std::vector<HitboxDef> v;
  multi(v, 4, hits, 4, 0, -30, 34, 2.0f, limb, e, 0, 85);
  const int f = 4 + hits * 4 + 2;
  v.push_back(hb(f, f + 4, 0, -50, 38, 5.5f, 80, 60, 105, limb, hits, e));
  MoveDef m = mv(name, f + 22, v);
  m.vel = {ve(4, VelMode::Set, 1.5f, -vy, 3.5f)};
  m.gravityMul = 0.5f;
  m.helplessAfter = true;
  m.pose = PoseHint::Spin;
  m.castFx = fx;
  m.castFxFrame = 4;
  return m;
}

// A teleport recovery towards the stick.
MoveDef blink(const char* name, F e, const char* fx, float dist = 300) {
  MoveDef m = mv(name, 40, {hb(22, 26, 0, -6, 40, 7, 80, 55, 75, L::Body, 0, e)});
  m.teleportFrame = 20;
  m.teleportDist = dist;
  m.intangibleStart = 6;
  m.intangibleEnd = 22;
  m.vel = {ve(2, VelMode::Set, 0, 0)};
  m.gravityMul = 0.0f;
  m.allowDrift = false;
  m.helplessAfter = true;
  m.castFx = fx;
  m.castFxFrame = 6;
  m.castFxStays = true;
  return m;
}

// A counter (a blow if it is hit in its window); `reflect`: sends projectiles back.
MoveDef counter(const char* name, F e, const char* fx, bool reflect, Limb limb = L::Weapon) {
  MoveDef m = mv(name, 52, {imp(hb(26, 30, 56, -8, 46, 12, 361, 60, 95, limb, 0, e))});
  m.counterStart = 5;
  m.counterEnd = 26;
  m.reflects = reflect;
  m.gravityMul = 0.3f;
  m.pose = PoseHint::HandSign;
  m.castFx = fx;
  m.castFxFrame = 5;
  return m;
}

// A burst all around the fighter; `hits` > 1: a whirl of light hits before it.
MoveDef burst(const char* name, F e, const char* fx, float radius, float dmg, int hits = 1) {
  std::vector<HitboxDef> v;
  if (hits > 1) multi(v, 10, hits - 1, 4, 0, -10, radius * 0.8f, 2.0f, L::Body, e);
  const int f = 10 + (hits - 1) * 4 + 2;
  v.push_back(imp(hb(f, f + 5, 0, -10, radius, dmg, 70, 50, 85, L::Body, hits, e)));
  MoveDef m = mv(name, f + 26, v);
  m.gravityMul = 0.4f;
  m.pose = hits > 1 ? PoseHint::Spin : PoseHint::Stomp;
  m.castFx = fx;
  m.castFxFrame = 8;
  return m;
}

// A mine set in front of the fighter: it waits, then bursts when someone touches it.
MoveDef trap(const char* name, F e, const char* fx, float dmg) {
  MoveDef m = mv(name, 36, {});
  m.projectileFrame = 14;
  m.projectile = proj({70, 40}, {0, 0}, 0, 300, 22, dmg, 50, 70, e, PV::Star);
  m.projectile.fx = fx;
  m.projectile.armDelay = 20;
  m.projectile.ignoreGround = true;
  m.projectile.explodeRadius = 70;
  m.projectile.explode = imp(hb(0, 0, 0, 0, 70, dmg, 75, 55, 80, L::None, 0, e));
  m.pose = PoseHint::Stomp;
  return m;
}

// A column erupting from the ground in front of the fighter.
MoveDef pillar(const char* name, F e, const char* fx, float dist, float dmg) {
  MoveDef m = mv(name, 46, {});
  m.projectileFrame = 16;
  m.projectile = proj({dist, 10}, {0, 0}, 0, 26, 36, dmg, 55, 80, e, PV::Pillar);
  m.projectile.fx = fx;
  m.projectile.armDelay = 8;
  m.projectile.piercing = true;
  m.projectile.ignoreGround = true;
  m.projectile.hit.angle = 80;
  m.pose = PoseHint::Stomp;
  return m;
}

// `n` projectiles falling in front of the fighter.
MoveDef rain(const char* name, F e, PV vis, const char* fx, int n, float dmg) {
  MoveDef m = mv(name, 50, {});
  m.pose = PoseHint::Raise;
  for (int i = 0; i < n; ++i) {
    ProjectileDef p = proj({70.0f + i * 45.0f, -220}, {0, 10}, 0, 40, 18, dmg, 30, 60, e, vis);
    p.fx = fx;
    p.hit.angle = 280;
    m.shots.push_back({14 + i * 5, p});
  }
  return m;
}

// --- per weapon shape: M1 names and blade lengths are on the sheets ---
Palette colours(Color3 coat, Color3 trim, Color3 accent, Color3 hair, Color3 eyes) {
  return pal(coat, trim, accent, {238, 204, 178}, hair, eyes);
}

// ---------------------------------------------------------------------------------------------
// 01 · violet shadow · T01
// ---------------------------------------------------------------------------------------------
CharacterDef make01() {
  Fighter k = start(1, "Ombre", Build::Medium, F::Shadow,
                    colours({30, 28, 40}, {96, 60, 160}, {150, 90, 230}, {28, 26, 34}, {150, 90, 230}),
                    colours({220, 220, 230}, {96, 60, 160}, {150, 90, 230}, {28, 26, 34}, {150, 90, 230}));
  const F e = k.e;
  skills(k, -1, shot("Orbe d'ombre", e, PV::Orb, k.fx("x", "n"), 5.5f, 22, 9),
         through("Pas de l'ombre", e, k.fx("x", "s"), 9), blink("Ascension spectrale", e, k.fx("x", "u")),
         counter("Riposte d'ombre", e, k.fx("x", "d"), false, L::ArmFront));
  ultimateOf(k, -1, ult::awaken("ÉVEIL DU NÉANT", ult::form(k.c, k.fx("t", "aura"), e, 15, 1.15f, 0.9f), e,
                                k.fx("t", "burst")));

  arm(k, 0, "Odachi", "Longue lame courbe", WeaponVisual::Held, Kit::Blade, 95, 1.0f,
      {"Coupe horizontale", "Coupe montante", "Grande coupe descendante"});
  skills(k, 0, along(shot("Croissant du néant", e, PV::Wave, k.fx("a", "n"), 9, 24, 9)),
         rush("Iaï", e, k.fx("a", "s"), 17, 0, 11), rise("Lune montante", e, k.fx("a", "u"), 15.5f, 4),
         counter("Garde-lame", e, k.fx("a", "d"), false));
  ultimateOf(k, 0, ult::ambush("MILLE LUNES", e, k.fx("a", "ult"), L::Weapon, 7));

  arm(k, 1, "Dagues jumelles", "Deux dagues spectrales", WeaponVisual::Held, Kit::Blade, 42, 0.95f,
      {"Entaille gauche", "Entaille droite", "Double estoc en X"}, 0.85f);
  skills(k, 1, boomerang(spread(shot("Dagues spectrales", e, PV::Blade, k.fx("b", "n"), 10, 14, 5, 44), 2, 12)),
         rush("Assaut fantôme", e, k.fx("b", "s"), 14, 4, 7), rise("Vrille", e, k.fx("b", "u"), 15, 5),
         trap("Marque d'ombre", e, k.fx("b", "d"), 10));
  ultimateOf(k, 1, ult::ambush("DANSE DU NÉANT", e, k.fx("b", "ult"), L::Weapon, 9));
  return k.c;
}

// ---------------------------------------------------------------------------------------------
// 02 · stellar frost · T02
// ---------------------------------------------------------------------------------------------
CharacterDef make02() {
  Fighter k = start(2, "Givre", Build::Light, F::Ice,
                    colours({34, 32, 42}, {230, 232, 240}, {110, 170, 240}, {214, 218, 232}, {150, 130, 220}),
                    colours({200, 220, 245}, {60, 80, 130}, {110, 170, 240}, {214, 218, 232}, {150, 130, 220}));
  const F e = k.e;
  skills(k, -1, shot("Éclat de givre", e, PV::Star, k.fx("x", "n"), 9, 16, 7),
         rush("Glissade gelée", e, k.fx("x", "s"), 13, 0, 9, L::LegFront),
         rise("Pilier de glace", e, k.fx("x", "u"), 15.5f, 3, L::Body),
         counter("Miroir de glace", e, k.fx("x", "d"), true, L::ArmFront));
  ultimateOf(k, -1, ult::awaken("GIVRE CÉLESTE", ult::form(k.c, k.fx("t", "aura"), e, 15, 1.15f, 0.9f), e,
                                k.fx("t", "burst")));

  arm(k, 0, "Lance argentée", "Lance à rubans blancs", WeaponVisual::Held, Kit::Lance, 110, 1.0f,
      {"Estoc", "Double estoc", "Balayage circulaire"});
  skills(k, 0, boomerang(shot("Lance de givre", e, PV::Spear, k.fx("a", "n"), 11, 18, 8, 50)),
         rush("Charge polaire", e, k.fx("a", "s"), 15, 2, 9), rise("Spirale céleste", e, k.fx("a", "u"), 15.5f, 4),
         pillar("Pieux de glace", e, k.fx("a", "d"), 80, 11));
  ultimateOf(k, 0, ult::storm("ZÉRO ABSOLU", e, k.fx("a", "ult"), 120, 320));

  arm(k, 1, "Sceptre-étoile", "Bâton à l'étoile de cristal", WeaponVisual::Held, Kit::Blade, 90, 0.95f,
      {"Coup de sceptre", "Revers", "Éclat d'étoile"});
  skills(k, 1, lob(shot("Étoile filante", e, PV::Star, k.fx("b", "n"), 8, 18, 9, 60), -6, 0.3f),
         spread(shot("Constellation", e, PV::Star, k.fx("b", "s"), 8, 14, 5, 50), 3, 20),
         blink("Saut stellaire", e, k.fx("b", "u")), counter("Dôme astral", e, k.fx("b", "d"), true, L::Body));
  ultimateOf(k, 1, ult::rain("PLUIE D'ÉTOILES", e, k.fx("b", "ult"), PV::Star, 8));
  return k.c;
}

// ---------------------------------------------------------------------------------------------
// 03 · red blaze · T03
// ---------------------------------------------------------------------------------------------
CharacterDef make03() {
  Fighter k = start(3, "Brasier", Build::Heavy, F::Fire,
                    colours({30, 26, 28}, {200, 40, 40}, {240, 110, 40}, {30, 24, 26}, {220, 50, 50}),
                    colours({240, 236, 230}, {200, 40, 40}, {240, 110, 40}, {30, 24, 26}, {220, 50, 50}));
  const F e = k.e;
  k.P = 1.05f;
  skills(k, -1, shot("Poing ardent", e, PV::Orb, k.fx("x", "n"), 8, 20, 9, 30),
         rush("Charge brûlante", e, k.fx("x", "s"), 14, 0, 11, L::Body),
         rise("Uppercut volcanique", e, k.fx("x", "u"), 15, 4, L::ArmFront),
         burst("Éruption", e, k.fx("x", "d"), 70, 12));
  ultimateOf(k, -1, ult::awaken("BRASIER DÉMONIAQUE", ult::form(k.c, k.fx("t", "aura"), e, 15, 1.15f, 0.9f), e,
                                k.fx("t", "burst")));

  arm(k, 0, "Grande épée noire", "Lame lourde aux veines rouges", WeaponVisual::Held, Kit::Blade, 100, 1.05f,
      {"Taille lourde", "Revers", "Frappe au sol"}, 1.15f);
  skills(k, 0, along(shot("Onde tranchante", e, PV::Wave, k.fx("a", "n"), 8, 26, 10)),
         rush("Brise-garde", e, k.fx("a", "s"), 13, 0, 13), rise("Lame ascendante", e, k.fx("a", "u"), 15, 3),
         burst("Séisme", e, k.fx("a", "d"), 80, 13));
  ultimateOf(k, 0, ult::rain("EXÉCUTION ÉCARLATE", e, k.fx("a", "ult"), PV::Pillar, 2));

  arm(k, 1, "Grande épée éveillée", "La même lame, embrasée", WeaponVisual::Held, Kit::Blade, 100, 1.05f,
      {"Coupe enflammée", "Coupe en croix", "Frappe explosive"}, 1.15f);
  skills(k, 1, exploding(shot("Météore", e, PV::Bomb, k.fx("b", "n"), 9, 22, 7, 45), 60, 8),
         rush("Traînée de feu", e, k.fx("b", "s"), 15, 3, 9), rise("Phénix", e, k.fx("b", "u"), 16, 4),
         burst("Cercle de brasier", e, k.fx("b", "d"), 80, 6, 4));
  ultimateOf(k, 1, ult::blast("APOCALYPSE", e, k.fx("b", "ult"), PV::Bomb, 8, 70, 300));
  return k.c;
}

// ---------------------------------------------------------------------------------------------
// 07 · red lightning
// ---------------------------------------------------------------------------------------------
CharacterDef make07() {
  Fighter k = start(7, "Foudre rouge", Build::Light, F::Lightning,
                    colours({28, 24, 28}, {200, 30, 40}, {255, 80, 80}, {26, 22, 26}, {220, 50, 50}),
                    colours({236, 232, 230}, {200, 30, 40}, {255, 80, 80}, {26, 22, 26}, {220, 50, 50}));
  const F e = k.e;
  skills(k, -1, piercing(shot("Éclair rouge", e, PV::Bolt, k.fx("x", "n"), 15, 12, 6, 30)),
         through("Pas-éclair", e, k.fx("x", "s"), 8),
         rise("Coup de pied foudre", e, k.fx("x", "u"), 16.5f, 3, L::LegFront),
         burst("Décharge", e, k.fx("x", "d"), 70, 4, 4));
  ultimateOf(k, -1, ult::rain("TEMPÊTE ÉCARLATE", e, k.fx("x", "ult"), PV::Pillar, 7));

  arm(k, 0, "Épée large", "Lame à rubans rouges", WeaponVisual::Held, Kit::Blade, 85, 1.0f,
      {"Coupe", "Seconde coupe", "Estoc"}, 0.9f);
  skills(k, 0, piercing(shot("Lame-éclair", e, PV::Crescent, k.fx("a", "n"), 12, 18, 8)),
         rush("Iaï foudroyant", e, k.fx("a", "s"), 19, 0, 11), rise("Saut-foudre", e, k.fx("a", "u"), 16, 3),
         counter("Riposte", e, k.fx("a", "d"), false));
  ultimateOf(k, 0, ult::ambush("CENT ÉCLAIRS", e, k.fx("a", "ult"), L::Weapon, 9));

  arm(k, 1, "Épée éveillée", "La même épée, éclairs rouges", WeaponVisual::Held, Kit::Blade, 85, 1.0f,
      {"Coupe électrique", "Coupe", "Estoc foudroyant"}, 0.9f);
  skills(k, 1, piercing(shot("Lance de foudre", e, PV::Spear, k.fx("b", "n"), 14, 16, 9, 35)),
         rush("Tranchant écarlate", e, k.fx("b", "s"), 16, 2, 9), rise("Spirale rouge", e, k.fx("b", "u"), 16, 5),
         pillar("Foudre au sol", e, k.fx("b", "d"), 90, 11));
  ultimateOf(k, 1, ult::rain("JUGEMENT ROUGE", e, k.fx("b", "ult"), PV::Bolt, 4));
  return k.c;
}

// ---------------------------------------------------------------------------------------------
// 08 · white light · T08
// ---------------------------------------------------------------------------------------------
CharacterDef make08() {
  Fighter k = start(8, "Lumière", Build::Medium, F::Light,
                    colours({236, 236, 240}, {40, 40, 46}, {230, 196, 90}, {232, 232, 238}, {110, 170, 220}),
                    colours({40, 40, 46}, {236, 236, 240}, {230, 196, 90}, {232, 232, 238}, {110, 170, 220}));
  const F e = k.e;
  skills(k, -1, shot("Plume de lumière", e, PV::Feather, k.fx("x", "n"), 11, 14, 7),
         rush("Rafale", e, k.fx("x", "s"), 15, 0, 10, L::Body),
         rise("Ailes de vent", e, k.fx("x", "u"), 16.5f, 2, L::Body),
         counter("Barrière de lumière", e, k.fx("x", "d"), true, L::Body));
  ultimateOf(k, -1, ult::awaken("ANGE BLANC", ult::form(k.c, k.fx("t", "aura"), e, 15, 1.15f, 0.9f), e,
                                k.fx("t", "burst")));

  arm(k, 0, "Épée blanche et or", "Lame blanche à rubans", WeaponVisual::Held, Kit::Blade, 85, 1.0f,
      {"Coupe", "Seconde coupe", "Coupe en croix"});
  skills(k, 0, piercing(shot("Lame de lumière", e, PV::Crescent, k.fx("a", "n"), 11, 20, 9)),
         rush("Éclair blanc", e, k.fx("a", "s"), 18, 0, 11), rise("Ascension sacrée", e, k.fx("a", "u"), 16, 4),
         counter("Contre sacré", e, k.fx("a", "d"), false));
  ultimateOf(k, 0, ult::rain("JUGEMENT CÉLESTE", e, k.fx("a", "ult"), PV::Pillar, 3));

  arm(k, 1, "Lance dorée", "Hallebarde d'or", WeaponVisual::Held, Kit::Lance, 115, 1.0f,
      {"Estoc", "Balayage", "Estoc tournant"});
  skills(k, 1, piercing(shot("Lance de lumière", e, PV::Spear, k.fx("b", "n"), 13, 16, 9, 40)),
         rush("Charge du paladin", e, k.fx("b", "s"), 15, 2, 10), rise("Rayon montant", e, k.fx("b", "u"), 16, 3),
         pillar("Pilier sacré", e, k.fx("b", "d"), 85, 12));
  ultimateOf(k, 1, ult::rain("LANCES DIVINES", e, k.fx("b", "ult"), PV::Spear, 9));
  return k.c;
}

}  // namespace

bool g_rawHitboxes = false;

namespace {
// One hitbox moved onto the drawing of its clip:
// fighter id, weapon slot (-1: bare-handed), move, index in the move's hitboxes, the radius it was written with (the
// entry is dropped if that changed: the roster moved on since the fit), its fitted offset and radius, and for a
// blade swing its fitted angles and reach (its offset is unused).
struct HitFit {
  const char* fighter;
  int slot, move, index;
  float was, x, y, r, a0, a1, reach1;
};
constexpr HitFit kHitFit[] = {
#include "core/roster_hitfit.inc"
    {nullptr, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

MoveDef* moveOf(CharacterDef& c, int slot, int move) {
  if (slot < 0) return move >= 0 && move < MOVE_COUNT ? &c.moves[move] : nullptr;
  if (slot >= int(std::size(c.weapons))) return nullptr;
  for (auto& [id, m] : c.weapons[slot].moves)
    if (id == move) return &m;
  return nullptr;
}

void applyHitFit(std::vector<CharacterDef>& v) {
  for (const HitFit& h : kHitFit) {
    if (!h.fighter) break;
    for (auto& c : v) {
      if (c.id != h.fighter) continue;
      MoveDef* m = moveOf(c, h.slot, h.move);
      if (!m || h.index < 0 || h.index >= int(m->hitboxes.size())) continue;
      HitboxDef& x = m->hitboxes[h.index];
      if (std::fabs(x.radius - h.was) > 0.01f) continue;
      x.radius = h.r;
      if (x.swing) {
        x.a0 = h.a0;
        x.a1 = h.a1;
        x.reach1 = h.reach1;
      } else {
        x.offset = {h.x, h.y};
      }
    }
  }
}
}  // namespace

const std::vector<CharacterDef>& roster() {
  static const std::vector<CharacterDef> r = [] {
    std::vector<CharacterDef> v = {make01(), make02(), make03(), make07(), make08()};
    if (!g_rawHitboxes) applyHitFit(v);
    return v;
  }();
  return r;
}
