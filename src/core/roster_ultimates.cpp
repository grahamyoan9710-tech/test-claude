// The ultimates and transformations, built from a few shapes shared by the seventeen fighters (roster.cpp).
// Frames 1-30 are the cut-in freeze; the action starts around frame 34. Positions are in pixels from the
// fighter's center (facing right) and are not scaled by its size: an ultimate fills the screen. Every ultimate
// names its PixelLab effect: played behind the fighter (castFx), or drawn as its projectile or its beam.
#include "core/roster_kit.hpp"

namespace {
using namespace kit;
using VM = VelMode;

// Strikes on every opponent (pillars, lightning, falling stars): a warning, then the hit.
ProjectileDef strikeOnAll(float radius, int warn, int life, float dmg, float bkb, float kbg, F e, PV vis,
                          const char* fx) {
  ProjectileDef p = proj({0, 0}, {0, 0}, 0, life, radius, dmg, bkb, kbg, e, vis);
  p.target = ProjTarget::All;
  p.armDelay = warn;
  p.piercing = true;
  p.ignoreGround = true;
  p.hit.angle = 80;
  p.fx = fx;
  return p;
}

void effect(MoveDef& m, const char* fx, int frame) {
  m.castFx = fx;
  m.castFxFrame = frame;
  m.castFxScale = 1.6f;
}
}  // namespace

namespace ult {
using namespace kit;

int form(CharacterDef& c, const char* aura, HitFx fx, int seconds, float damage, float kbTaken) {
  FormDef d;
  d.form = Form::Awakened;
  d.duration = seconds * 60;
  d.fx = fx;
  d.damageMul = damage;
  d.kbTakenMul = kbTaken;
  d.aura = aura;
  c.forms.push_back(d);
  return int(c.forms.size()) - 1;
}

// A burst around the fighter as the transformation begins.
MoveDef awaken(const char* name, int formIndex, HitFx e, const char* fx) {
  MoveDef m = cine(name, 76, {imp(hb(38, 44, 0, -10, 200, 14, 60, 52, 62, L::Body, 0, e))}, PoseHint::Roar,
                   UltFx::Cinematic);
  m.formIndex = formIndex;
  m.formFrame = 34;
  effect(m, fx, 34);
  return m;
}

// Appears in front of the nearest opponent and catches it, `hits` blows while it is held, then a launcher.
MoveDef ambush(const char* name, HitFx e, const char* fx, Limb limb, int hits) {
  const int tp = 34, every = 5;
  std::vector<HitboxDef> v = {cap(hb(tp, tp + 6, 46, -6, 58, 6, 0, 0, 0, limb, 0, e))};
  multi(v, tp + 9, hits, every, 50, -6, 52, 2.6f, limb, e, 1, 0);
  const int f = tp + 9 + hits * every + 4;
  v.push_back(lau(hb(f, f + 6, 50, -20, 90, 22, 55, 94, 108, limb, hits + 1, e)));
  MoveDef m = cine(name, f + 28, v, PoseHint::Rush, UltFx::Cinematic);
  m.teleportFrame = tp;
  m.teleportToTarget = true;
  m.holdAt = {40, -10};
  effect(m, fx, tp);
  return m;
}

// Blows all around the fighter, growing from r0 to r1, then a launching finisher.
MoveDef storm(const char* name, HitFx e, const char* fx, float r0, float r1) {
  MoveDef m = cine(name, 104, kit::storm(38, 8, 5, r0, r1, 3.0f, e, 24, r1 + 40), PoseHint::Raise, UltFx::Cinematic);
  effect(m, fx, 34);
  return m;
}

// A beam from the fighter (angle 0: forward, 90: straight up).
MoveDef beam(const char* name, HitFx e, const char* fx, float angle, float length, float width) {
  BeamDef b;
  b.start = 38;
  b.end = 88;
  b.from = angle > 45.0f ? Vec2{0, -40} : Vec2{40, -20};
  b.angle = angle;
  b.length = length;
  b.width = width;
  b.fx = e;
  b.sprite = fx;
  MoveDef m = cine(name, 100, beamHits(b, 24, angle > 45.0f ? 85.0f : 30.0f, 95, 110, 6), PoseHint::Cast,
                   UltFx::Cinematic);
  m.beam = b;
  return m;
}

// Strikes falling on every opponent, the last one the biggest.
MoveDef rain(const char* name, HitFx e, const char* fx, ProjVisual vis, int strikes) {
  MoveDef m = cine(name, 112, {}, PoseHint::Raise, UltFx::Cinematic);
  for (int k = 0; k < strikes; ++k) {
    ProjectileDef p = strikeOnAll(54, 14, 26, 4, 20, 10, e, vis, fx);
    p.jitter = {90, 0};
    m.shots.push_back({36 + k * 7, p});
  }
  ProjectileDef last = strikeOnAll(110, 18, 34, 20, 92, 104, e, vis, fx);
  last.hit.impact = true;
  m.shots.push_back({36 + strikes * 7 + 4, last});
  return m;
}

// One huge projectile that explodes.
MoveDef blast(const char* name, HitFx e, const char* fx, ProjVisual vis, float speed, float radius, float explode) {
  MoveDef m = cine(name, 104, {}, PoseHint::Throw, UltFx::Cinematic);
  m.projectileFrame = 54;
  m.projectile = proj({60, -20}, {speed, 0}, 0, 70, radius, 6, 30, 20, e, vis);
  m.projectile.fx = fx;
  m.projectile.explodeRadius = explode;
  m.projectile.explode = imp(hb(0, 0, 0, 0, explode, 24, 70, 94, 108, L::None, 0, e));
  return m;
}

}  // namespace ult
