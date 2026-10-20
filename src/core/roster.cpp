// The seventeen fighters of Super Clash Bros. (docs/refonte/FICHES.md). They have no names yet: each one is a
// number (P01..P17), its weapons NN-A and NN-B, its transformation TNN. Every fighter starts bare-handed (the
// Smash normals, four skills and an ultimate, or its transformation) and summons its two weapons with the weapon
// button: each weapon is a kit (a 3-hit M1, an air M1, four skills and an ultimate).
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
// 04 · astral
// ---------------------------------------------------------------------------------------------
CharacterDef make04() {
  Fighter k = start(4, "Astre", Build::Small, F::Star,
                    colours({236, 234, 240}, {30, 28, 36}, {150, 100, 230}, {220, 222, 232}, {110, 170, 240}),
                    colours({40, 36, 60}, {236, 234, 240}, {230, 190, 90}, {220, 222, 232}, {110, 170, 240}));
  const F e = k.e;
  skills(k, -1, shot("Étincelle", e, PV::Star, k.fx("x", "n"), 10, 12, 5),
         rush("Pirouette", e, k.fx("x", "s"), 11, 2, 7, L::LegFront),
         rise("Saut d'étoile", e, k.fx("x", "u"), 17, 2, L::Body),
         counter("Bulle", e, k.fx("x", "d"), true, L::Body));
  ultimateOf(k, -1, ult::storm("CONSTELLATION", e, k.fx("x", "ult"), 100, 300));

  arm(k, 0, "Anneau-chakram", "Anneau à quatre pointes", WeaponVisual::Held, Kit::Blade, 45, 0.95f,
      {"Lancer court", "Revers", "Double rotation"}, 0.9f);
  skills(k, 0, boomerang(shot("Boomerang", e, PV::Blade, k.fx("a", "n"), 10, 18, 7, 50)),
         along(shot("Roue tranchante", e, PV::Blade, k.fx("a", "s"), 8, 20, 8)),
         rise("Anneau porteur", e, k.fx("a", "u"), 16.5f, 4), burst("Orbite", e, k.fx("a", "d"), 60, 5, 4));
  ultimateOf(k, 0, ult::ambush("ANNEAUX CÉLESTES", e, k.fx("a", "ult"), L::Weapon, 8));

  arm(k, 1, "Médaillon-orbe", "Orbe qui flotte à son épaule", WeaponVisual::Floating, Kit::Magic, 0, 0.95f,
      {"Rayon court", "Second rayon", "Éclat de l'orbe"});
  skills(k, 1, piercing(shot("Rayon arcane", e, PV::Bolt, k.fx("b", "n"), 16, 14, 8, 30)),
         piercing(shot("Sceau", e, PV::Star, k.fx("b", "s"), 4, 26, 3, 70), 8),
         blink("Lévitation", e, k.fx("b", "u"), 240),
         pulling(shot("Gravité", e, PV::Orb, k.fx("b", "d"), 0, 30, 3, 60), 2.0f, 160));
  ultimateOf(k, 1, ult::beam("ŒIL CÉLESTE", e, k.fx("b", "ult"), 0, 900, 110));
  return k.c;
}

// ---------------------------------------------------------------------------------------------
// 05 · blue flame
// ---------------------------------------------------------------------------------------------
CharacterDef make05() {
  Fighter k = start(5, "Flamme bleue", Build::Medium, F::Water,
                    colours({26, 28, 40}, {40, 160, 220}, {90, 210, 250}, {30, 40, 80}, {80, 190, 240}),
                    colours({220, 226, 236}, {40, 160, 220}, {90, 210, 250}, {30, 40, 80}, {80, 190, 240}));
  const F e = k.e;
  skills(k, -1, shot("Flamme bleue", e, PV::Orb, k.fx("x", "n"), 8, 18, 8),
         rush("Ruée de l'eau", e, k.fx("x", "s"), 15, 0, 10, L::Body),
         rise("Geyser", e, k.fx("x", "u"), 16, 3, L::Body), burst("Vague", e, k.fx("x", "d"), 75, 11));
  ultimateOf(k, -1, ult::blast("RAZ-DE-MARÉE", e, k.fx("x", "ult"), PV::Wave, 9, 90, 220));

  arm(k, 0, "Lance sombre", "Lance à rubans bleus", WeaponVisual::Held, Kit::Lance, 112, 1.0f,
      {"Estoc", "Estoc appuyé", "Balayage"});
  skills(k, 0, piercing(shot("Javelot", e, PV::Spear, k.fx("a", "n"), 13, 16, 9, 40)),
         rush("Percée", e, k.fx("a", "s"), 17, 0, 11), rise("Saut du dragon", e, k.fx("a", "u"), 16, 3),
         burst("Tourbillon", e, k.fx("a", "d"), 70, 5, 4));
  ultimateOf(k, 0, ult::blast("DRAGON DES ABYSSES", e, k.fx("a", "ult"), PV::Beast, 11, 80, 200));

  arm(k, 1, "Lance éveillée", "La même lance, en flammes bleues", WeaponVisual::Held, Kit::Lance, 112, 1.0f,
      {"Estoc enflammé", "Double estoc", "Balayage de flammes"});
  skills(k, 1, exploding(shot("Comète bleue", e, PV::Bomb, k.fx("b", "n"), 10, 20, 7), 55, 7),
         rush("Charge flamboyante", e, k.fx("b", "s"), 15, 3, 9), rise("Pilier de flammes", e, k.fx("b", "u"), 16, 4),
         rain("Pluie de lances", e, PV::Spear, k.fx("b", "d"), 3, 7));
  ultimateOf(k, 1, ult::storm("BRASIER AZUR", e, k.fx("b", "ult"), 130, 330));
  return k.c;
}

// ---------------------------------------------------------------------------------------------
// 06 · pink demon · T06
// ---------------------------------------------------------------------------------------------
CharacterDef make06() {
  Fighter k = start(6, "Démon rose", Build::Medium, F::Rose,
                    colours({30, 24, 30}, {190, 40, 70}, {240, 100, 170}, {240, 140, 160}, {120, 200, 230}),
                    colours({236, 230, 236}, {190, 40, 70}, {240, 100, 170}, {240, 140, 160}, {120, 200, 230}));
  const F e = k.e;
  skills(k, -1, shot("Orbe cramoisi", e, PV::Orb, k.fx("x", "n"), 8, 18, 8),
         rush("Griffe", e, k.fx("x", "s"), 15, 2, 9, L::ArmFront),
         rise("Ailes d'ombre", e, k.fx("x", "u"), 16, 2, L::Body), trap("Malédiction", e, k.fx("x", "d"), 9));
  ultimateOf(k, -1, ult::awaken("ÉVEIL ÉCARLATE", ult::form(k.c, k.fx("t", "aura"), e, 15, 1.15f, 0.9f), e,
                                k.fx("t", "burst")));

  arm(k, 0, "Faux sombre", "Grande faux", WeaponVisual::Held, Kit::Blade, 100, 1.0f,
      {"Fauche", "Revers", "Fauche circulaire"}, 1.05f);
  skills(k, 0, piercing(shot("Croissant rose", e, PV::Crescent, k.fx("a", "n"), 10, 22, 9)),
         rush("Moisson", e, k.fx("a", "s"), 15, 2, 10), rise("Rotor", e, k.fx("a", "u"), 15.5f, 5),
         pulling(shot("Crochet", e, PV::Blade, k.fx("a", "d"), 6, 20, 4, 30), 4.0f, 110));
  ultimateOf(k, 0, ult::storm("MOISSON ÉCARLATE", e, k.fx("a", "ult"), 150, 360));

  arm(k, 1, "Faux éveillée", "La même faux, aura rose", WeaponVisual::Held, Kit::Blade, 100, 1.0f,
      {"Fauche", "Double fauche", "Coup de manche"}, 1.05f);
  skills(k, 1, spread(piercing(shot("Lames jumelles", e, PV::Crescent, k.fx("b", "n"), 10, 18, 6)), 2, 14),
         rush("Chaîne de ronces", e, k.fx("b", "s"), 13, 3, 8), rise("Envol démoniaque", e, k.fx("b", "u"), 16.5f, 3),
         burst("Cercle maudit", e, k.fx("b", "d"), 85, 4, 5));
  ultimateOf(k, 1, ult::rain("TEMPÊTE DE PÉTALES", e, k.fx("b", "ult"), PV::Feather, 9));
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

// ---------------------------------------------------------------------------------------------
// 09 · sea ice
// ---------------------------------------------------------------------------------------------
CharacterDef make09() {
  Fighter k = start(9, "Glace marine", Build::Medium, F::Ice,
                    colours({236, 240, 248}, {60, 120, 210}, {130, 200, 250}, {150, 200, 245}, {80, 160, 230}),
                    colours({40, 60, 110}, {236, 240, 248}, {130, 200, 250}, {150, 200, 245}, {80, 160, 230}));
  const F e = k.e;
  skills(k, -1, shot("Bulle", e, PV::Orb, k.fx("x", "n"), 6, 20, 7, 70),
         rush("Glissade", e, k.fx("x", "s"), 14, 0, 9, L::LegFront),
         rise("Jet d'eau", e, k.fx("x", "u"), 17, 2, L::Body), trap("Flaque gelée", e, k.fx("x", "d"), 8));
  ultimateOf(k, -1, ult::blast("VAGUE GÉANTE", e, k.fx("x", "ult"), PV::Wave, 9, 90, 220));

  arm(k, 0, "Gantelets", "Gantelets noirs à liserés bleus", WeaponVisual::Held, Kit::Gauntlet, 0, 1.0f,
      {"Direct", "Crochet", "Uppercut"});
  skills(k, 0, shot("Poing-canon", e, PV::Orb, k.fx("a", "n"), 12, 22, 9, 25),
         rush("Ruée de poings", e, k.fx("a", "s"), 14, 4, 8, L::ArmFront),
         rise("Uppercut glacé", e, k.fx("a", "u"), 16, 3, L::ArmFront), pillar("Frappe au sol", e, k.fx("a", "d"), 60, 11));
  ultimateOf(k, 0, ult::ambush("MILLE POINGS", e, k.fx("a", "ult"), L::ArmFront, 10));

  arm(k, 1, "Gantelets éveillés", "Les mêmes, aura bleue", WeaponVisual::Held, Kit::Gauntlet, 0, 1.0f,
      {"Direct lourd", "Crochet glacé", "Double marteau"}, 1.1f);
  skills(k, 1, shot("Poing de glace", e, PV::Bomb, k.fx("b", "n"), 9, 26, 11, 35),
         rush("Bélier", e, k.fx("b", "s"), 16, 0, 13, L::Body), rise("Fusée", e, k.fx("b", "u"), 17.5f, 2, L::Body),
         pillar("Iceberg", e, k.fx("b", "d"), 70, 13));
  ultimateOf(k, 1, ult::ambush("GLACIER", e, k.fx("b", "ult"), L::ArmFront, 4));
  return k.c;
}

// ---------------------------------------------------------------------------------------------
// 10 · jade storm · T10
// ---------------------------------------------------------------------------------------------
CharacterDef make10() {
  Fighter k = start(10, "Tempête de jade", Build::Medium, F::Wind,
                    colours({30, 50, 46}, {40, 120, 110}, {90, 230, 200}, {24, 44, 46}, {60, 200, 180}),
                    colours({220, 232, 228}, {40, 120, 110}, {90, 230, 200}, {24, 44, 46}, {60, 200, 180}));
  const F e = k.e;
  skills(k, -1, piercing(shot("Lame de vent", e, PV::Crescent, k.fx("x", "n"), 12, 16, 7)),
         rush("Rafale", e, k.fx("x", "s"), 16, 0, 10, L::Body),
         rise("Tornade", e, k.fx("x", "u"), 16, 5, L::Body), counter("Contre-vent", e, k.fx("x", "d"), true, L::Body));
  ultimateOf(k, -1, ult::awaken("TEMPÊTE JADE", ult::form(k.c, k.fx("t", "aura"), e, 15, 1.15f, 0.9f), e,
                                k.fx("t", "burst")));

  arm(k, 0, "Lance de jade", "Lance à rubans verts", WeaponVisual::Held, Kit::Lance, 110, 1.0f,
      {"Estoc", "Estoc appuyé", "Balayage en tourbillon"});
  skills(k, 0, piercing(shot("Lance-vent", e, PV::Crescent, k.fx("a", "n"), 12, 18, 8)),
         rush("Percée du vent", e, k.fx("a", "s"), 17, 0, 11), rise("Saut de la grue", e, k.fx("a", "u"), 16.5f, 3),
         burst("Moulinet", e, k.fx("a", "d"), 70, 5, 4));
  ultimateOf(k, 0, ult::storm("OURAGAN DE JADE", e, k.fx("a", "ult"), 140, 340));

  arm(k, 1, "Lance de jade éveillée", "La même lance, flammes vertes", WeaponVisual::Held, Kit::Lance, 110, 1.0f,
      {"Estoc électrique", "Estoc", "Balayage foudroyant"});
  skills(k, 1, piercing(shot("Javelot-éclair", e, PV::Bolt, k.fx("b", "n"), 15, 14, 8, 35)),
         rush("Ruée du tonnerre", e, k.fx("b", "s"), 17, 2, 9), rise("Éclair ascendant", e, k.fx("b", "u"), 16.5f, 4),
         burst("Champ électrique", e, k.fx("b", "d"), 80, 4, 5));
  ultimateOf(k, 1, ult::blast("DRAGON DE TEMPÊTE", e, k.fx("b", "ult"), PV::Beast, 12, 80, 200));
  return k.c;
}

// ---------------------------------------------------------------------------------------------
// 11 · violet arcane
// ---------------------------------------------------------------------------------------------
CharacterDef make11() {
  Fighter k = start(11, "Arcane", Build::Small, F::Arcane,
                    colours({30, 26, 40}, {110, 60, 170}, {170, 110, 240}, {226, 226, 236}, {150, 90, 230}),
                    colours({226, 222, 236}, {110, 60, 170}, {170, 110, 240}, {226, 226, 236}, {150, 90, 230}));
  const F e = k.e;
  skills(k, -1, shot("Sphère magique", e, PV::Orb, k.fx("x", "n"), 8, 18, 8),
         piercing(shot("Glyphe", e, PV::Star, k.fx("x", "s"), 5, 24, 3, 60), 8),
         blink("Lévitation", e, k.fx("x", "u"), 250), trap("Sceau", e, k.fx("x", "d"), 10));
  ultimateOf(k, -1, ult::storm("GRAND SORTILÈGE", e, k.fx("x", "ult"), 120, 330));

  arm(k, 0, "Grimoire violet", "Grimoire qui flotte devant elle", WeaponVisual::Floating, Kit::Magic, 0, 1.0f,
      {"Trait magique", "Second trait", "Troisième trait"});
  skills(k, 0, spread(shot("Flèches arcaniques", e, PV::Arrow, k.fx("a", "n"), 12, 12, 5, 40), 3, 10),
         piercing(shot("Pages tranchantes", e, PV::Feather, k.fx("a", "s"), 9, 22, 3, 45), 7),
         blink("Portail", e, k.fx("a", "u"), 270), counter("Cercle de protection", e, k.fx("a", "d"), true, L::Body));
  ultimateOf(k, 0, ult::rain("BIBLIOTHÈQUE INTERDITE", e, k.fx("a", "ult"), PV::Feather, 10));

  arm(k, 1, "Grimoire éveillé", "Le même livre, l'œil ouvert", WeaponVisual::Floating, Kit::Magic, 0, 1.0f,
      {"Rayon court", "Second rayon", "Troisième rayon"});
  skills(k, 1, piercing(shot("Œil du grimoire", e, PV::Bolt, k.fx("b", "n"), 16, 14, 9, 30)),
         shot("Chaînes magiques", e, PV::Spear, k.fx("b", "s"), 10, 16, 6, 40),
         blink("Ascension runique", e, k.fx("b", "u"), 260),
         pulling(shot("Trou noir", e, PV::Orb, k.fx("b", "d"), 2, 30, 3, 70), 2.5f, 170));
  ultimateOf(k, 1, ult::ambush("INVOCATION", e, k.fx("b", "ult"), L::Body, 5));
  return k.c;
}

// ---------------------------------------------------------------------------------------------
// 12 · golden sun · T12
// ---------------------------------------------------------------------------------------------
CharacterDef make12() {
  Fighter k = start(12, "Soleil d'or", Build::Medium, F::Sand,
                    colours({30, 28, 30}, {236, 232, 220}, {230, 180, 70}, {30, 26, 26}, {230, 180, 70}),
                    colours({236, 232, 220}, {30, 28, 30}, {230, 180, 70}, {30, 26, 26}, {230, 180, 70}));
  const F e = k.e;
  skills(k, -1, shot("Sable doré", e, PV::Feather, k.fx("x", "n"), 9, 18, 7),
         through("Pas de sable", e, k.fx("x", "s"), 8), rise("Tourbillon de sable", e, k.fx("x", "u"), 16, 4, L::Body),
         pulling(shot("Sables mouvants", e, PV::Wave, k.fx("x", "d"), 0, 34, 3, 60), 1.8f, 130));
  ultimateOf(k, -1, ult::awaken("SOLEIL D'OR", ult::form(k.c, k.fx("t", "aura"), e, 15, 1.15f, 0.9f), e,
                                k.fx("t", "burst")));

  arm(k, 0, "Katana", "Katana au pompon blanc", WeaponVisual::Held, Kit::Blade, 88, 1.0f,
      {"Coupe", "Seconde coupe", "Iaï"}, 0.9f);
  skills(k, 0, piercing(shot("Vague dorée", e, PV::Crescent, k.fx("a", "n"), 11, 20, 8)),
         rush("Iaï", e, k.fx("a", "s"), 19, 0, 12), rise("Coupe ascendante", e, k.fx("a", "u"), 16, 3),
         counter("Contre", e, k.fx("a", "d"), false));
  ultimateOf(k, 0, ult::beam("LAME DU CRÉPUSCULE", e, k.fx("a", "ult"), 0, 900, 90));

  arm(k, 1, "Grande lame dorée", "Grande lame à rubans", WeaponVisual::Held, Kit::Blade, 102, 1.05f,
      {"Coupe lourde", "Revers", "Fendoir"}, 1.15f);
  skills(k, 1, boomerang(shot("Lame d'or", e, PV::Blade, k.fx("b", "n"), 10, 22, 9, 50)),
         rush("Charge du lion", e, k.fx("b", "s"), 15, 0, 13), rise("Saut-fendoir", e, k.fx("b", "u"), 15.5f, 2),
         pillar("Pilier de sable", e, k.fx("b", "d"), 80, 12));
  ultimateOf(k, 1, ult::rain("SOLEIL COUCHANT", e, k.fx("b", "ult"), PV::Blade, 2));
  return k.c;
}

// ---------------------------------------------------------------------------------------------
// 13 · raven
// ---------------------------------------------------------------------------------------------
CharacterDef make13() {
  Fighter k = start(13, "Corbeau", Build::Huge, F::Blood,
                    colours({24, 22, 26}, {170, 30, 36}, {220, 60, 60}, {226, 226, 230}, {200, 50, 50}),
                    colours({60, 58, 64}, {170, 30, 36}, {220, 60, 60}, {226, 226, 230}, {200, 50, 50}));
  const F e = k.e;
  k.P = 1.08f;
  skills(k, -1, spread(shot("Plumes noires", e, PV::Feather, k.fx("x", "n"), 11, 12, 4, 40), 3, 12),
         rush("Charge du corbeau", e, k.fx("x", "s"), 13, 0, 13, L::Body),
         rise("Ailes de corbeau", e, k.fx("x", "u"), 15.5f, 2, L::Body), burst("Écrasement", e, k.fx("x", "d"), 80, 14));
  ultimateOf(k, -1, ult::blast("NUÉE DE CORBEAUX", e, k.fx("x", "ult"), PV::Beast, 10, 90, 200));

  arm(k, 0, "Hache de guerre", "Hache noire et rouge", WeaponVisual::Held, Kit::Blade, 90, 1.05f,
      {"Taille", "Revers", "Frappe au sol"}, 1.2f);
  skills(k, 0, boomerang(shot("Hache lancée", e, PV::Blade, k.fx("a", "n"), 10, 24, 10, 50)),
         rush("Charge broyeuse", e, k.fx("a", "s"), 13, 2, 11), rise("Moulinet montant", e, k.fx("a", "u"), 15, 4),
         burst("Séisme", e, k.fx("a", "d"), 85, 14));
  ultimateOf(k, 0, ult::ambush("EXÉCUTEUR", e, k.fx("a", "ult"), L::Weapon, 3));

  arm(k, 1, "Hache éveillée", "La même hache, en feu", WeaponVisual::Held, Kit::Blade, 90, 1.05f,
      {"Taille enflammée", "Revers", "Frappe explosive"}, 1.2f);
  skills(k, 1, exploding(shot("Comète de feu", e, PV::Bomb, k.fx("b", "n"), 9, 22, 8, 45), 60, 8),
         rush("Tourbillon de feu", e, k.fx("b", "s"), 12, 4, 9), rise("Envol ardent", e, k.fx("b", "u"), 15.5f, 3),
         along(shot("Fissure de lave", e, PV::Wave, k.fx("b", "d"), 7, 26, 11)));
  ultimateOf(k, 1, ult::rain("GUILLOTINE", e, k.fx("b", "ult"), PV::Blade, 1));
  return k.c;
}

// ---------------------------------------------------------------------------------------------
// 14 · crimson rose · T14
// ---------------------------------------------------------------------------------------------
CharacterDef make14() {
  Fighter k = start(14, "Rose cramoisie", Build::Small, F::Rose,
                    colours({30, 26, 30}, {236, 232, 236}, {240, 90, 140}, {245, 150, 175}, {220, 60, 90}),
                    colours({236, 232, 236}, {30, 26, 30}, {240, 90, 140}, {245, 150, 175}, {220, 60, 90}));
  const F e = k.e;
  skills(k, -1, shot("Pétale", e, PV::Feather, k.fx("x", "n"), 10, 14, 6),
         rush("Pirouette", e, k.fx("x", "s"), 12, 2, 7, L::LegFront),
         rise("Ruban", e, k.fx("x", "u"), 17, 2, L::ArmFront), trap("Ronces", e, k.fx("x", "d"), 9));
  ultimateOf(k, -1, ult::awaken("ROSE CRAMOISIE", ult::form(k.c, k.fx("t", "aura"), e, 15, 1.15f, 0.9f), e,
                                k.fx("t", "burst")));

  arm(k, 0, "Faucilles à chaîne", "Deux faucilles reliées par une chaîne", WeaponVisual::Held, Kit::Blade, 55, 0.95f,
      {"Entaille", "Seconde entaille", "Double lancer"}, 0.85f);
  skills(k, 0, pulling(shot("Chaîne", e, PV::Blade, k.fx("a", "n"), 9, 16, 4, 30), 4.0f, 110),
         rush("Toupie", e, k.fx("a", "s"), 12, 4, 7), rise("Grappin", e, k.fx("a", "u"), 17.5f, 2),
         along(shot("Ronces d'acier", e, PV::Wave, k.fx("a", "d"), 7, 20, 8)));
  ultimateOf(k, 0, ult::ambush("VALSE DES LAMES", e, k.fx("a", "ult"), L::Weapon, 8));

  arm(k, 1, "Faucilles éveillées", "Les mêmes, aura rose", WeaponVisual::Held, Kit::Blade, 55, 0.95f,
      {"Entaille", "Croix", "Lancer tournoyant"}, 0.85f);
  skills(k, 1, spread(piercing(shot("Croissants jumeaux", e, PV::Crescent, k.fx("b", "n"), 10, 16, 5)), 2, 14),
         piercing(shot("Tempête de pétales", e, PV::Feather, k.fx("b", "s"), 7, 26, 2, 50), 6),
         rise("Spirale rose", e, k.fx("b", "u"), 17, 5), burst("Jardin de lames", e, k.fx("b", "d"), 80, 4, 5));
  ultimateOf(k, 1, ult::storm("FLORAISON", e, k.fx("b", "ult"), 120, 320));
  return k.c;
}

// ---------------------------------------------------------------------------------------------
// 15 · lunar night
// ---------------------------------------------------------------------------------------------
CharacterDef make15() {
  Fighter k = start(15, "Nuit lunaire", Build::Medium, F::Moon,
                    colours({26, 22, 36}, {90, 50, 160}, {150, 100, 240}, {40, 30, 60}, {140, 110, 240}),
                    colours({220, 216, 232}, {90, 50, 160}, {150, 100, 240}, {40, 30, 60}, {140, 110, 240}));
  const F e = k.e;
  skills(k, -1, shot("Orbe lunaire", e, PV::Orb, k.fx("x", "n"), 7, 20, 8),
         rush("Ombre glissante", e, k.fx("x", "s"), 15, 0, 9, L::Body),
         rise("Ailes de nuit", e, k.fx("x", "u"), 16.5f, 2, L::Body),
         pulling(shot("Gravité", e, PV::Orb, k.fx("x", "d"), 0, 30, 3, 50), 2.0f, 150));
  ultimateOf(k, -1, ult::blast("NUIT ÉTERNELLE", e, k.fx("x", "ult"), PV::Bomb, 6, 100, 300));

  arm(k, 0, "Faux de nuit", "Faux violette", WeaponVisual::Held, Kit::Blade, 100, 1.0f,
      {"Fauche", "Seconde fauche", "Crochet"}, 1.05f);
  skills(k, 0, piercing(shot("Croissant de lune", e, PV::Crescent, k.fx("a", "n"), 10, 22, 9)),
         rush("Fauche-ombre", e, k.fx("a", "s"), 15, 2, 10), rise("Hélice", e, k.fx("a", "u"), 15.5f, 5),
         pulling(shot("Puits", e, PV::Orb, k.fx("a", "d"), 0, 34, 3, 60), 2.2f, 160));
  ultimateOf(k, 0, ult::beam("ÉCLIPSE LUNAIRE", e, k.fx("a", "ult"), 0, 900, 120));

  arm(k, 1, "Faux éveillée", "La même faux, aura violette", WeaponVisual::Held, Kit::Blade, 100, 1.0f,
      {"Fauche", "Double fauche", "Coup de pointe"}, 1.05f);
  skills(k, 1, spread(piercing(shot("Lames du vide", e, PV::Crescent, k.fx("b", "n"), 10, 18, 6)), 2, 14),
         through("Faille", e, k.fx("b", "s"), 11), rise("Spirale du vide", e, k.fx("b", "u"), 16, 5),
         pulling(shot("Singularité", e, PV::Orb, k.fx("b", "d"), 3, 30, 3, 70), 2.6f, 170));
  ultimateOf(k, 1, ult::storm("TROU NOIR", e, k.fx("b", "ult"), 160, 360));
  return k.c;
}

// ---------------------------------------------------------------------------------------------
// 16 · rising sun · T16
// ---------------------------------------------------------------------------------------------
CharacterDef make16() {
  Fighter k = start(16, "Soleil levant", Build::Heavy, F::Sun,
                    colours({30, 28, 30}, {200, 160, 70}, {250, 200, 90}, {180, 140, 90}, {150, 130, 70}),
                    colours({236, 232, 220}, {200, 160, 70}, {250, 200, 90}, {180, 140, 90}, {150, 130, 70}));
  const F e = k.e;
  k.P = 1.04f;
  skills(k, -1, shot("Éclat solaire", e, PV::Orb, k.fx("x", "n"), 9, 18, 8),
         rush("Charge", e, k.fx("x", "s"), 14, 0, 11, L::Body),
         rise("Uppercut lumineux", e, k.fx("x", "u"), 15.5f, 3, L::ArmFront),
         burst("Éruption solaire", e, k.fx("x", "d"), 75, 12));
  ultimateOf(k, -1, ult::awaken("SOLEIL LEVANT", ult::form(k.c, k.fx("t", "aura"), e, 15, 1.15f, 0.9f), e,
                                k.fx("t", "burst")));

  arm(k, 0, "Épée noire et or", "Lame noire à ruban", WeaponVisual::Held, Kit::Blade, 88, 1.0f,
      {"Coupe", "Seconde coupe", "Estoc"});
  skills(k, 0, piercing(shot("Rayon solaire", e, PV::Crescent, k.fx("a", "n"), 11, 20, 9)),
         rush("Lame de l'aube", e, k.fx("a", "s"), 16, 0, 11), rise("Ascension", e, k.fx("a", "u"), 15.5f, 4),
         counter("Garde solaire", e, k.fx("a", "d"), false));
  ultimateOf(k, 0, ult::rain("ZÉNITH", e, k.fx("a", "ult"), PV::Pillar, 3));

  arm(k, 1, "Bouclier-soleil", "Disque solaire à lames", WeaponVisual::Held, Kit::Gauntlet, 0, 1.0f,
      {"Coup de bouclier", "Revers", "Charge"}, 1.1f);
  skills(k, 1, boomerang(shot("Disque lancé", e, PV::Blade, k.fx("b", "n"), 10, 24, 9, 50)),
         rush("Bélier solaire", e, k.fx("b", "s"), 14, 0, 13, L::ArmFront),
         rise("Disque porteur", e, k.fx("b", "u"), 15.5f, 3, L::ArmFront),
         counter("Rempart", e, k.fx("b", "d"), true, L::ArmFront));
  ultimateOf(k, 1, ult::storm("SUPERNOVA", e, k.fx("b", "ult"), 140, 360));
  return k.c;
}

// ---------------------------------------------------------------------------------------------
// 17 · emerald
// ---------------------------------------------------------------------------------------------
CharacterDef make17() {
  Fighter k = start(17, "Émeraude", Build::Light, F::Nature,
                    colours({30, 34, 30}, {60, 170, 120}, {130, 230, 180}, {150, 225, 195}, {60, 180, 140}),
                    colours({230, 236, 230}, {60, 170, 120}, {130, 230, 180}, {150, 225, 195}, {60, 180, 140}));
  const F e = k.e;
  skills(k, -1, lob(shot("Graine", e, PV::Orb, k.fx("x", "n"), 8, 14, 7, 60), -5, 0.35f),
         rush("Liane", e, k.fx("x", "s"), 10, 0, 9, L::ArmFront),
         rise("Feuille planante", e, k.fx("x", "u"), 17, 2, L::Body), trap("Racines", e, k.fx("x", "d"), 9));
  ultimateOf(k, -1, ult::rain("FORÊT", e, k.fx("x", "ult"), PV::Pillar, 6));

  arm(k, 0, "Arc à plumes", "Arc vert à plumes", WeaponVisual::Held, Kit::Blade, 70, 0.95f,
      {"Coup d'arc", "Revers", "Tir à bout portant"}, 0.9f);
  skills(k, 0, piercing(shot("Flèche chargée", e, PV::Arrow, k.fx("a", "n"), 16, 12, 9, 40)),
         spread(shot("Tir en roulade", e, PV::Arrow, k.fx("a", "s"), 15, 10, 5, 35), 2, 8),
         rise("Flèche-grappin", e, k.fx("a", "u"), 17, 2), trap("Flèche-piège", e, k.fx("a", "d"), 10));
  ultimateOf(k, 0, ult::rain("PLUIE DE FLÈCHES", e, k.fx("a", "ult"), PV::Arrow, 10));

  arm(k, 1, "Arc éveillé", "Le même arc, ailes de lumière", WeaponVisual::Held, Kit::Blade, 70, 0.95f,
      {"Coup d'arc", "Revers", "Tir d'étoile"}, 0.9f);
  skills(k, 1, piercing(shot("Flèche-étoile", e, PV::Star, k.fx("b", "n"), 18, 14, 10, 40)),
         spread(shot("Triple tir", e, PV::Arrow, k.fx("b", "s"), 15, 10, 5, 35), 3, 12),
         rise("Envol du faucon", e, k.fx("b", "u"), 17.5f, 2), pillar("Flèche plantée", e, k.fx("b", "d"), 70, 11));
  ultimateOf(k, 1, ult::beam("FLÈCHE CÉLESTE", e, k.fx("b", "ult"), 0, 900, 100));
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
    std::vector<CharacterDef> v = {make01(), make02(), make03(), make04(), make05(), make06(),
                                   make07(), make08(), make09(), make10(), make11(), make12(),
                                   make13(), make14(), make15(), make16(), make17()};
    if (!g_rawHitboxes) applyHitFit(v);
    return v;
  }();
  return r;
}
