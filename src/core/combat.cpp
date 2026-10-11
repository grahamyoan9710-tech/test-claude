// Hit detection, knockback, shields, counters, grabs and projectiles.
#include <algorithm>

#include "core/match.hpp"
#include "core/swing.hpp"

namespace {
constexpr float kLaunchScale = 0.15f;  // knockback units -> px/frame

// Does the hitbox touch the circle / rectangle this frame? hp = contact point (world). Swing hitboxes
// sweep the blade from its angle on the previous frame to this frame's (no tunnelling through thin
// targets); the tip is tested first.
template <class Test>
bool hitboxTouches(const Fighter& a, const HitboxDef& x, Test touches, Vec2& hp) {
  if (!x.swing) {
    hp = a.center() + Vec2{x.offset.x * a.facing, x.offset.y};
    return touches(hp, x.radius);
  }
  const float H = a.height(), L = a.bladeLength(), t = float(a.moveFrame);
  const float ang1 = swingAngle(x, t), ang0 = swingAngle(x, t - 1.0f);
  const int sub = std::fabs(ang1 - ang0) > 30.0f ? 3 : 2;
  for (int k = sub; k >= 0; --k) {
    const BladeSeg s = swingSegment(x, ang0 + (ang1 - ang0) * float(k) / sub, H, L);
    const Vec2 p0 = a.center() + Vec2{s.a.x * a.facing, s.a.y};
    const Vec2 p1 = a.center() + Vec2{s.b.x * a.facing, s.b.y};
    const int n = std::max(1, int((p1 - p0).len() / std::max(6.0f, x.radius * 0.8f)));
    for (int i = n; i >= 0; --i) {
      const Vec2 p = lerpv(p0, p1, float(i) / n);
      if (touches(p, x.radius)) {
        hp = p;
        return true;
      }
    }
  }
  return false;
}

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
    HitboxDef h;  // copy: a held weapon changes its element and knockback
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
    // A held weapon: harder hits; the base moves also reach further and take its element.
    const WeaponDef* w = a.weaponDef();
    const bool baseMove = m == &a.def->moves[a.move];
    if (w) mul *= w->damageMul;
    auto worldBox = [&](const HitboxDef& h) {
      HitboxDef x = h;
      if (w) {
        x.growth *= w->kbMul;
        if (baseMove) {
          if (h.fx == HitFx::Normal || h.fx == HitFx::Slash) x.fx = w->fx;
          if (h.limb == Limb::Weapon || h.limb == Limb::ArmFront || h.limb == Limb::ArmBack) {
            x.offset = x.offset * w->reachMul;
            x.radius *= std::sqrt(w->reachMul);
          }
        }
      }
      return x;
    };
    if (orb.active && !m->isGrab) {
      for (const auto& h : m->hitboxes) {
        if (a.moveFrame < h.start || a.moveFrame >= h.end) continue;
        int key = 63 * 64 + h.group;
        if (std::find(a.hitList.begin(), a.hitList.end(), key) != a.hitList.end()) continue;
        const HitboxDef x = worldBox(h);
        Vec2 hp;
        auto touchOrb = [&](Vec2 c, float r) { return (c - orb.pos).len() <= r + orb.radius; };
        if (!hitboxTouches(a, x, touchOrb, hp)) continue;
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
        const HitboxDef x = worldBox(h);
        Vec2 hp;
        auto touchHurt = [&](Vec2 c, float r) { return circleRect(c, r, hurt); };
        if (!hitboxTouches(a, x, touchHurt, hp)) continue;
        if (m->isGrab) grabs.push_back({a.id, v.id});
        else hits.push_back({a.id, v.id, x, hp, mul});
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
    int dir = ph.h.reverse ? -a.facing : a.facing;
    applyHit(&a, v, ph.h, ph.pos, dir, ph.mul, false);
  }

  for (auto [ai, vi] : grabs) {
    Fighter& a = fighters[ai];
    Fighter& v = fighters[vi];
    const MoveDef* am = a.currentMove();
    if (!am || !am->isGrab) continue;  // got hit meanwhile
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
  events.back().impact = h.impact;
}

// Damage credit for projectiles and explosions (no attacker in applyHit).
void Match::creditProjectile(int owner, Fighter& v, float damage, bool wasShielding) {
  if (owner < 0 || owner >= int(fighters.size())) return;
  if (!wasShielding) {
    fighters[owner].damageDealt += damage;
    gainUlt(fighters[owner], damage * kUltPerDamageDealt);
    v.lastHitBy = owner;
    v.lastHitFrame = frame;
  } else {
    gainUlt(fighters[owner], damage * kUltPerShieldHit);
  }
}

void Match::explodeProjectile(Projectile& pr) {
  if (pr.explodeRadius <= 0.0f) return;
  const float r = pr.explodeRadius;
  pr.explodeRadius = 0.0f;  // once
  emit(EvType::Explosion, pr.pos, pr.owner, -1, r, {}, pr.explode.fx);
  for (auto& v : fighters) {
    if (v.id == pr.owner || !v.active() || v.state == FState::Respawn || v.isIntangible()) continue;
    if (!circleRect(pr.pos, r, v.hurtbox())) continue;
    const bool wasShielding = v.state == FState::Shield || v.state == FState::ShieldStun;
    applyHit(nullptr, v, pr.explode, pr.pos, v.center().x >= pr.pos.x ? 1 : -1, 1.0f, false);
    creditProjectile(pr.owner, v, pr.explode.damage, wasShielding);
  }
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
    pr.rehit = p.rehit;
    pr.pull = p.pull;
    pr.pullRadius = p.pullRadius;
    pr.boomerang = p.boomerang;
    pr.grow = p.grow;
    pr.explodeRadius = p.explodeRadius;
    pr.explode = p.explode;
    pr.spawnRadius = p.radius;
    if (const WeaponDef* w = f.weaponDef()) {
      pr.hit.damage *= w->damageMul;
      pr.explode.damage *= w->damageMul;
    }
    projectiles.push_back(pr);
  }
  emit(EvType::Projectile, f.center(), f.id, -1, 0.0f, {}, p.hit.fx);
}

void Match::updateProjectiles() {
  const Rect& blast = stageDef->blast;
  for (auto& pr : projectiles) {
    if (!pr.alive) continue;
    ++pr.age;
    pr.radius += pr.grow;
    pr.hit.radius = pr.radius;
    // Boomerang: after half its flight it homes back to its owner's hand.
    if (pr.boomerang && pr.owner >= 0) {
      const Fighter& o = fighters[pr.owner];
      if (!pr.returning && pr.age >= pr.maxLife / 2) {
        pr.returning = true;
        pr.hitList.clear();
      }
      if (pr.returning) {
        Vec2 d = o.center() - pr.pos;
        if (d.len() < 34.0f || !o.active() || pr.age > pr.maxLife * 3) {
          pr.alive = false;
          continue;
        }
        pr.vel = d.norm() * std::max(12.0f, pr.vel.len());
        pr.life = std::max(pr.life, 2);
      }
    }
    pr.vel.y += pr.gravity;
    Vec2 prev = pr.pos;
    pr.pos += pr.vel;
    if (!blast.contains(pr.pos)) {
      pr.alive = false;
      continue;
    }
    if (--pr.life <= 0) {
      pr.alive = false;
      explodeProjectile(pr);
      continue;
    }
    if (!pr.boomerang) {
      for (const auto& p : platforms) {
        if (!circleRect(pr.pos, pr.spawnRadius * 0.6f, p.rect)) continue;
        bool fromAbove = prev.y + pr.spawnRadius * 0.6f <= p.rect.top() + 2.0f && pr.vel.y >= 0.0f;
        if (pr.bounces && fromAbove) {
          pr.pos.y = p.rect.top() - pr.spawnRadius * 0.6f;
          pr.vel.y = -6.5f;
        } else if (p.solid || (pr.explodeRadius > 0.0f && fromAbove)) {
          pr.alive = false;
        }
      }
    }
    if (!pr.alive) {
      explodeProjectile(pr);
      continue;
    }
    // Vortex: drags the fighters around it
    if (pr.pull > 0.0f) {
      for (auto& v : fighters) {
        if (v.id == pr.owner || !v.active() || v.state == FState::Respawn || v.isIntangible() ||
            v.state == FState::Grabbed || v.state == FState::LedgeHang)
          continue;
        Vec2 d = pr.pos - v.center();
        float l = d.len();
        if (l > pr.pullRadius || l < 4.0f) continue;
        Vec2 a = d * (pr.pull * 0.25f / l);
        v.vel.x += a.x;
        if (!v.grounded) v.vel.y += a.y;
      }
    }
    if (orb.active && (pr.pos - orb.pos).len() < pr.radius + orb.radius &&
        std::find(pr.hitList.begin(), pr.hitList.end(), 63) == pr.hitList.end()) {
      pr.hitList.push_back(63);
      hitOrb(pr.owner, pr.hit.damage, pr.pos - pr.vel, pr.hit.fx);
      if (!pr.piercing) {
        pr.alive = false;
        explodeProjectile(pr);
        continue;
      }
    }
    for (auto& v : fighters) {
      if (v.id == pr.owner || !v.active() || v.state == FState::Respawn || v.isIntangible()) continue;
      if (pr.rehit > 0) {
        auto it = std::find_if(pr.lastHit.begin(), pr.lastHit.end(), [&](const auto& e) { return e.first == v.id; });
        if (it != pr.lastHit.end() && frame - it->second < pr.rehit) continue;
      } else if (std::find(pr.hitList.begin(), pr.hitList.end(), v.id) != pr.hitList.end()) {
        continue;
      }
      if (!circleRect(pr.pos, pr.radius, v.hurtbox())) continue;
      if (pr.rehit > 0) {
        auto it = std::find_if(pr.lastHit.begin(), pr.lastHit.end(), [&](const auto& e) { return e.first == v.id; });
        if (it != pr.lastHit.end()) it->second = frame;
        else pr.lastHit.push_back({v.id, frame});
      } else {
        pr.hitList.push_back(v.id);
      }
      if (tryCounter(v, nullptr)) {
        const MoveDef* cm = v.currentMove();
        if (cm && cm->reflects) {  // sent back to the sender, faster
          pr.owner = v.id;
          pr.vel = pr.vel * -1.15f;
          pr.life = pr.maxLife;
          pr.age = 0;
          pr.boomerang = pr.returning = false;
          pr.hitList.clear();
          pr.lastHit.clear();
          emit(EvType::Reflect, pr.pos, v.id, -1, 0.0f, pr.vel.norm(), pr.hit.fx);
        } else {
          pr.alive = false;
        }
        break;
      }
      int dir = pr.vel.x >= 0.0f ? 1 : -1;
      const bool wasShielding = v.state == FState::Shield || v.state == FState::ShieldStun;
      applyHit(nullptr, v, pr.hit, pr.pos, dir, 1.0f, false);
      creditProjectile(pr.owner, v, pr.hit.damage, wasShielding);
      if (!pr.piercing) {
        pr.alive = false;
        explodeProjectile(pr);
        break;
      }
    }
  }
  projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
                                   [](const Projectile& p) { return !p.alive; }),
                    projectiles.end());
}
