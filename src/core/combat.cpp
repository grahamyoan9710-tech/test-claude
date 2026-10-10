// Hit detection, knockback, shields, counters, grabs and projectiles.
#include <algorithm>

#include "core/match.hpp"

namespace {
constexpr float kLaunchScale = 0.15f;  // knockback units -> px/frame

int hitlagFor(float dmg, float kb) {
  int lag = int(clampf(dmg * 0.45f + 4.0f, 4.0f, 18.0f));
  if (kb > 130.0f) lag += 4;  // dramatic freeze on strong hits
  return lag;
}
}  // namespace

// Super Smash Bros. style knockback formula.
float computeKnockback(float p, float d, float w, float g, float b) {
  return (((p / 10.0f + p * d / 20.0f) * (200.0f / (w + 100.0f)) * 1.4f) + 18.0f) * (g / 100.0f) + b;
}

bool Match::tryCounter(Fighter& v, Fighter* attacker) {
  const MoveDef* m = v.currentMove();
  if (!m || m->counterStart < 0 || v.counterTriggered) return false;
  if (v.moveFrame < m->counterStart || v.moveFrame >= m->counterEnd) return false;
  v.counterTriggered = true;
  v.moveFrame = m->counterEnd - 1;
  v.intangible = 20;
  v.hitlag = 6;
  if (attacker) {
    v.facing = attacker->pos.x > v.pos.x ? 1 : -1;
    attacker->hitlag = 14;
  }
  emit(EvType::Counter, v.center(), v.id, attacker ? attacker->id : -1);
  return true;
}

void Match::resolveHits() {
  struct Pending {
    int a, v;
    const HitboxDef* h;
    Vec2 pos;
    float mul;
  };
  std::vector<Pending> hits;
  std::vector<std::pair<int, int>> grabs;

  for (auto& a : fighters) {
    const MoveDef* m = a.currentMove();
    if (!m || !a.active() || a.hitlag > 0 || a.charging || m->throwFrame >= 0) continue;
    if (m->counterStart >= 0 && !a.counterTriggered) continue;
    float mul = 1.0f;
    if (m->chargeFrame >= 0) mul += m->chargeDamageMul * float(a.charge) / float(m->maxCharge);
    if (orb.active && !m->isGrab) {
      for (const auto& h : m->hitboxes) {
        if (a.moveFrame < h.start || a.moveFrame >= h.end) continue;
        int key = 63 * 64 + h.group;
        if (std::find(a.hitList.begin(), a.hitList.end(), key) != a.hitList.end()) continue;
        Vec2 hp = a.center() + Vec2{h.offset.x * a.facing, h.offset.y};
        if ((hp - orb.pos).len() > h.radius + orb.radius) continue;
        a.hitList.push_back(key);
        a.hitlag = std::max(a.hitlag, 4);
        hitOrb(a.id, h.damage * mul, a.center(), h.fx);
        break;
      }
    }
    for (auto& v : fighters) {
      if (v.id == a.id || !v.active() || v.state == FState::Respawn || v.isIntangible()) continue;
      if (v.state == FState::Grabbed && v.grabPartner == a.id) continue;
      if (m->isGrab && (v.state == FState::Grabbed || v.state == FState::LedgeHang ||
                        (!v.grounded && v.pos.y < a.pos.y - 40.0f)))
        continue;
      Rect hurt = v.hurtbox();
      for (const auto& h : m->hitboxes) {
        if (a.moveFrame < h.start || a.moveFrame >= h.end) continue;
        int key = v.id * 64 + h.group;
        if (std::find(a.hitList.begin(), a.hitList.end(), key) != a.hitList.end()) continue;
        Vec2 hp = a.center() + Vec2{h.offset.x * a.facing, h.offset.y};
        if (!circleRect(hp, h.radius, hurt)) continue;
        if (m->isGrab) grabs.push_back({a.id, v.id});
        else hits.push_back({a.id, v.id, &h, hp, mul});
        a.hitList.push_back(key);
        break;  // one hitbox per victim per frame (first listed = priority)
      }
    }
  }

  for (const auto& ph : hits) {
    Fighter& a = fighters[ph.a];
    Fighter& v = fighters[ph.v];
    if (!v.active()) continue;
    if (tryCounter(v, &a)) continue;
    int dir = ph.h->reverse ? -a.facing : a.facing;
    applyHit(&a, v, *ph.h, ph.pos, dir, ph.mul, false);
  }

  for (auto [ai, vi] : grabs) {
    Fighter& a = fighters[ai];
    Fighter& v = fighters[vi];
    if (a.state != FState::Attack || a.move != Grab) continue;  // got hit meanwhile
    if (v.state == FState::Grabbed || !v.active() || v.isIntangible()) continue;
    if (v.grabPartner >= 0) releaseGrab(v, false);
    setState(a, FState::Holding);
    a.grabPartner = vi;
    a.vel = {0, 0};
    setState(v, FState::Grabbed);
    v.grabPartner = ai;
    v.escapeTimer = 50 + int(v.percent * 0.9f);
    v.vel = v.kbVel = {0, 0};
    v.hitstun = 0;
    v.ledge = -1;
    emit(EvType::Grab, v.center(), ai, vi);
  }
}

void Match::applyHit(Fighter* attacker, Fighter& v, const HitboxDef& h, Vec2 hitPos, int dirSign,
                     float damageMul, bool throwHit) {
  const float dmg = h.damage * damageMul;
  const bool shielding = (v.state == FState::Shield || v.state == FState::ShieldStun) && !throwHit;
  if (shielding) {
    v.shield -= dmg * 1.1f;
    int lag = hitlagFor(dmg, 0.0f);
    v.hitlag = lag;
    if (attacker) attacker->hitlag = lag;
    v.vel.x = dirSign * std::min(10.0f, 1.5f + dmg * 0.35f);
    if (attacker) gainUlt(*attacker, dmg * kUltPerShieldHit);
    emit(EvType::ShieldHit, hitPos, attacker ? attacker->id : -1, v.id, dmg, {float(dirSign), 0}, h.fx);
    if (v.shield <= 0.0f) {
      v.shield = 0.0f;
      setState(v, FState::ShieldBreak, 220);
      v.vel = {0, -9};
      v.grounded = false;
      v.groundPlat = -1;
      emit(EvType::ShieldBreak, v.center(), v.id);
    } else {
      setState(v, FState::ShieldStun, int(dmg * 0.6f + 3.0f));
    }
    return;
  }

  v.percent = std::min(999.0f, v.percent + dmg);
  gainUlt(v, dmg * kUltPerDamageTaken);
  if (attacker) {
    attacker->damageDealt += dmg;
    gainUlt(*attacker, dmg * kUltPerDamageDealt);
    v.lastHitBy = attacker->id;
    v.lastHitFrame = frame;
  }
  const float kb = computeKnockback(v.percent, dmg, v.def->weight, h.growth, h.baseKB);

  const MoveDef* vm = v.currentMove();
  if (!throwHit && vm && vm->armorStart >= 0 && v.moveFrame >= vm->armorStart && v.moveFrame < vm->armorEnd) {
    int lag = hitlagFor(dmg, 0.0f);
    v.hitlag = lag;
    if (attacker) attacker->hitlag = lag;
    v.flash = 8;
    emit(EvType::Hit, hitPos, attacker ? attacker->id : -1, v.id, dmg * 0.5f, {float(dirSign), 0}, h.fx);
    return;
  }

  float ang = h.angle;
  if (ang > 360.0f) ang = v.grounded ? (kb < 60.0f ? 5.0f : 40.0f) : 45.0f;
  if (v.grounded && ang > 180.0f && ang < 360.0f && !throwHit) ang = 80.0f;  // grounded spike pops up
  Vec2 dir{std::cos(deg2rad(ang)) * dirSign, -std::sin(deg2rad(ang))};
  Vec2 s = v.in.stick;  // Directional Influence
  if (s.len() > 0.2f) {
    Vec2 perp{-dir.y, dir.x};
    float amt = dot(s.norm(), perp) * std::min(1.0f, s.len());
    dir = (dir + perp * (amt * 0.27f)).norm();
  }

  if (v.grabPartner >= 0 && !throwHit) releaseGrab(v, false);
  setState(v, FState::Hitstun);
  v.kbVel = dir * (kb * kLaunchScale);
  v.vel = {0, 0};
  v.hitstun = std::max(4, int(kb * 0.4f));
  v.lastKnockback = kb;
  v.tumble = kb > 80.0f;
  v.ledge = -1;
  v.fastFall = false;
  v.spin = 0.0f;
  if (v.kbVel.y < -1.0f || v.tumble) {
    v.grounded = false;
    v.groundPlat = -1;
  }
  v.facing = dirSign > 0 ? -1 : 1;
  int lag = hitlagFor(dmg, kb);
  v.hitlag = lag;
  if (attacker && !throwHit) attacker->hitlag = lag;
  v.flash = 10;
  emit(EvType::Hit, hitPos, attacker ? attacker->id : -1, v.id, kb, dir, h.fx);
}

void Match::spawnProjectiles(Fighter& f, const ProjectileDef& p) {
  for (int i = 0; i < p.count; ++i) {
    float off = p.count > 1 ? (float(i) - float(p.count - 1) * 0.5f) * p.spreadDeg : 0.0f;
    Vec2 v = rotate(p.velocity, deg2rad(-off));
    Projectile pr;
    pr.owner = f.id;
    pr.pos = f.center() + Vec2{p.spawnOffset.x * f.facing, p.spawnOffset.y};
    pr.vel = {v.x * f.facing, v.y};
    pr.gravity = p.gravity;
    pr.radius = p.radius;
    pr.life = pr.maxLife = p.lifetime;
    pr.hit = p.hit;
    pr.bounces = p.bounces;
    pr.piercing = p.piercing;
    pr.visual = p.visual;
    projectiles.push_back(pr);
  }
  emit(EvType::Projectile, f.center(), f.id, -1, 0.0f, {}, p.hit.fx);
}

void Match::updateProjectiles() {
  const Rect& blast = stageDef->blast;
  for (auto& pr : projectiles) {
    if (!pr.alive) continue;
    pr.vel.y += pr.gravity;
    Vec2 prev = pr.pos;
    pr.pos += pr.vel;
    if (--pr.life <= 0 || !blast.contains(pr.pos)) {
      pr.alive = false;
      continue;
    }
    for (const auto& p : platforms) {
      if (!circleRect(pr.pos, pr.radius * 0.6f, p.rect)) continue;
      bool fromAbove = prev.y + pr.radius * 0.6f <= p.rect.top() + 2.0f && pr.vel.y >= 0.0f;
      if (pr.bounces && fromAbove) {
        pr.pos.y = p.rect.top() - pr.radius * 0.6f;
        pr.vel.y = -6.5f;
      } else if (p.solid) {
        pr.alive = false;
      }
    }
    if (!pr.alive) continue;
    if (orb.active && (pr.pos - orb.pos).len() < pr.radius + orb.radius &&
        std::find(pr.hitList.begin(), pr.hitList.end(), 63) == pr.hitList.end()) {
      pr.hitList.push_back(63);
      hitOrb(pr.owner, pr.hit.damage, pr.pos - pr.vel, pr.hit.fx);
      if (!pr.piercing) {
        pr.alive = false;
        continue;
      }
    }
    for (auto& v : fighters) {
      if (v.id == pr.owner || !v.active() || v.state == FState::Respawn || v.isIntangible()) continue;
      if (std::find(pr.hitList.begin(), pr.hitList.end(), v.id) != pr.hitList.end()) continue;
      if (!circleRect(pr.pos, pr.radius, v.hurtbox())) continue;
      pr.hitList.push_back(v.id);
      if (tryCounter(v, nullptr)) {
        pr.alive = false;
        break;
      }
      int dir = pr.vel.x >= 0.0f ? 1 : -1;
      bool wasShielding = v.state == FState::Shield || v.state == FState::ShieldStun;
      applyHit(nullptr, v, pr.hit, pr.pos, dir, 1.0f, false);
      if (!wasShielding && pr.owner >= 0) {
        fighters[pr.owner].damageDealt += pr.hit.damage;
        gainUlt(fighters[pr.owner], pr.hit.damage * kUltPerDamageDealt);
        v.lastHitBy = pr.owner;
        v.lastHitFrame = frame;
      } else if (wasShielding && pr.owner >= 0) {
        gainUlt(fighters[pr.owner], pr.hit.damage * kUltPerShieldHit);
      }
      if (!pr.piercing) {
        pr.alive = false;
        break;
      }
    }
  }
  projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
                                   [](const Projectile& p) { return !p.alive; }),
                    projectiles.end());
}
