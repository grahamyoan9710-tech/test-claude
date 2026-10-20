#pragma once
// Authoring kit shared by the roster files (roster.cpp, roster_ultimates.cpp).
#include <algorithm>

#include "core/moves.hpp"

namespace kit {
using L = Limb;
using F = HitFx;
using PV = ProjVisual;

inline void sc(MoveDef& m, float S, float P) {
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

inline VelEvent ve(int frame, VelMode mode, float x, float y, float stickX = 0.0f, bool airOnly = false) {
  VelEvent e;
  e.frame = frame;
  e.mode = mode;
  e.v = {x, y};
  e.stickX = stickX;
  e.airOnly = airOnly;
  return e;
}

inline ProjectileDef proj(Vec2 spawn, Vec2 vel, float grav, int life, float radius, float dmg, float bkb, float kbg, F fx,
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

inline Palette pal(Color3 a, Color3 b, Color3 c, Color3 skin, Color3 hair, Color3 eyes) {
  Palette p;
  p.primary = a;
  p.secondary = b;
  p.accent = c;
  p.skin = skin;
  p.hair = hair;
  p.eyes = eyes;
  return p;
}

inline HitboxDef imp(HitboxDef h) {
  h.impact = true;
  return h;
}

// n light hits every `every` frames (groups g0, g0+1...): the caller adds the finisher.
inline void multi(std::vector<HitboxDef>& v, int start, int n, int every, float ox, float oy, float r, float dmg, L limb,
           F fx, int g0 = 0, float angle = 361.0f) {
  for (int k = 0; k < n; ++k)
    v.push_back(hb(start + k * every, start + k * every + 3, ox, oy, r, dmg, angle, 18, 6, limb, g0 + k, fx));
}

// Ultimates: invulnerable, hanging in the air; the cinematic freeze covers the first 30 frames.
inline MoveDef ultimate(const char* name, int duration, std::vector<HitboxDef> boxes, PoseHint pose) {
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
inline std::vector<HitboxDef> storm(int start, int hits, int every, float r0, float r1, float dmg, F fx, float finalDmg,
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

inline WeaponDef weapon(const char* name, const char* desc, WeaponVisual vis, F fx, float dmg, float kb, float reach,
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

inline void stats(CharacterDef& c, float w, float h, float weight, float walk, float run, float air, float grav,
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

// Cinematic ultimates (see HitboxDef::capture): catches the victim / releases it with its knockback.
inline HitboxDef cap(HitboxDef h) {
  h.capture = true;
  return h;
}
inline HitboxDef lau(HitboxDef h) {
  h.launch = true;
  h.impact = true;
  return h;
}
// The last group of hits launches whoever the ultimate holds (beams, storms).
inline void launchLast(std::vector<HitboxDef>& v) {
  int g = 0;
  for (const auto& h : v) g = std::max(g, h.group);
  for (auto& h : v)
    if (h.group == g) h.launch = true;
}
// A cinematic ultimate: rises during the cut-in, then comes back down to fight at the same height.
inline MoveDef cine(const char* name, int duration, std::vector<HitboxDef> boxes, PoseHint pose, UltFx fx) {
  MoveDef m = ultimate(name, duration, std::move(boxes), pose);
  m.ultFx = fx;
  m.vel = {ve(1, VelMode::Set, 0.0f, -3.0f), ve(22, VelMode::Set, 0.0f, 0.0f), ve(30, VelMode::Set, 0.0f, 3.0f),
           ve(51, VelMode::Set, 0.0f, 0.0f)};
  return m;
}
}  // namespace kit

// The shapes of the ultimates and the transformations (roster_ultimates.cpp). `fx`: the move's PixelLab effect.
namespace ult {
int form(CharacterDef& c, const char* aura, HitFx fx, int seconds, float damage, float kbTaken);
MoveDef awaken(const char* name, int form, HitFx e, const char* fx);
MoveDef ambush(const char* name, HitFx e, const char* fx, Limb limb, int hits);
MoveDef storm(const char* name, HitFx e, const char* fx, float r0, float r1);
MoveDef beam(const char* name, HitFx e, const char* fx, float angle, float length, float width);
MoveDef rain(const char* name, HitFx e, const char* fx, ProjVisual vis, int strikes);
MoveDef blast(const char* name, HitFx e, const char* fx, ProjVisual vis, float speed, float radius, float explode);
}  // namespace ult
