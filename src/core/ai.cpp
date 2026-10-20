#include "core/ai.hpp"
#include "core/swing.hpp"

namespace {
using IS = InputState;

// How far in front of its center a move hits (blade swings: the furthest point of the sweep).
float moveReach(const Fighter& f, const MoveDef& m) {
  float r = 0.0f;
  for (const auto& h : m.hitboxes) {
    if (!h.swing) {
      r = std::max(r, std::fabs(h.offset.x) + h.radius);
      continue;
    }
    for (int k = 0; k <= 8; ++k) {
      const BladeSeg s = swingSegment(h, h.a0 + (h.a1 - h.a0) * k / 8.0f, f.height(), f.bladeLength());
      r = std::max(r, std::max(std::fabs(s.a.x), std::fabs(s.b.x)) + h.radius);
    }
  }
  return r;
}

bool hasProjectile(const CharacterDef& d) { return d.moves[NSpecial].projectileFrame >= 0; }

bool sideSpecialTravels(const CharacterDef& d) {
  for (const auto& e : d.moves[SSpecial].vel)
    if (e.mode == VelMode::SetX && e.v.x > 8.0f) return true;
  return false;
}

// What a special move is good for (the held weapon may replace it).
struct SpecialUse {
  bool projectile = false, dash = false, counter = false, grab = false, beam = false, melee = false;
  float reach = 0.0f;
  float landX = 0.0f;  // dashes: distance at which the finishing hit lands
};
SpecialUse specialUse(const Fighter& f, int id) {
  const MoveDef& m = f.moveDef(id);
  SpecialUse u;
  if (!m.exists() || f.moveCooldown[id] > 0) return u;  // recharging: as if it did not exist
  u.projectile = m.projectileFrame >= 0;
  u.counter = m.counterStart >= 0;
  u.grab = m.isGrab;
  u.beam = m.beam.start >= 0;
  int f0 = -1, fStop = 1000;
  float vx = 0.0f;
  for (const auto& e : m.vel) {
    if (e.mode == VelMode::SetX && e.v.x > 8.0f && f0 < 0) {
      u.dash = true;
      f0 = e.frame;
      vx = e.v.x;
    } else if (f0 >= 0 && (e.mode == VelMode::Scale || e.mode == VelMode::Set) && e.frame > f0) {
      fStop = std::min(fStop, e.frame);
    }
  }
  const HitboxDef* fin = m.hitboxes.empty() ? nullptr : &m.hitboxes.back();
  if (u.dash && fin) u.landX = vx * float(std::max(0, std::min(fin->start, fStop) - f0)) + fin->offset.x;
  if (m.teleportForward) {
    u.dash = true;
    u.landX = m.teleportDist - (fin ? fin->offset.x : 0.0f);
  }
  u.melee = !m.hitboxes.empty() && !u.counter && !u.grab;
  u.reach = moveReach(f, m);
  if (u.beam) u.reach = m.beam.from.x + m.beam.length;
  return u;
}

// How the ultimate reaches its target: anywhere (strikes on the target, teleports), straight ahead
// (beams, shots: the fighter must face it), or around the fighter (storms, domains, bursts).
struct UltReach {
  float range = 320.0f;
  bool anywhere = false, ahead = false;
};
UltReach ultReach(const Fighter& f) {
  const MoveDef& m = f.moveDef(FinalSmash);
  UltReach u;
  if (m.teleportToTarget) u.anywhere = true;
  if (m.projectileFrame >= 0) {
    if (m.projectile.target != ProjTarget::Self) {
      u.anywhere = true;
    } else {
      u.ahead = true;
      u.range = std::max(u.range, std::fabs(m.projectile.velocity.x) * float(m.projectile.lifetime) * 0.8f);
    }
  }
  for (const auto& sh : m.shots)
    if (sh.def.target != ProjTarget::Self) u.anywhere = true;
  if (m.beam.start >= 0) {
    u.ahead = true;
    u.range = std::max(u.range, m.beam.from.x + m.beam.length * 0.9f);
  }
  float r = 0.0f;
  for (const auto& h : m.hitboxes)
    if (h.limb == Limb::None || h.limb == Limb::Body) r = std::max(r, std::fabs(h.offset.x) + h.radius);
  if (!u.ahead) u.range = std::max(u.range, r * 0.85f);
  return u;
}

// Is fighter t about to hit fighter f?
bool isThreat(const Fighter& t, const Fighter& f) {
  const MoveDef* m = t.currentMove();
  if (!m || m->hitboxes.empty() || m->isGrab) return false;
  int lastEnd = 0;
  for (const auto& h : m->hitboxes) lastEnd = std::max(lastEnd, h.end);
  if (t.moveFrame >= lastEnd) return false;
  float reach = moveReach(t, *m) + 30.0f;
  Vec2 d = f.center() - t.center();
  return std::fabs(d.x) < reach && std::fabs(d.y) < reach;
}

float nearestPlatformX(const Match& m, Vec2 p) {
  float best = 0.0f, bd = 1e9f;
  for (const auto& pl : m.platforms) {
    float x = clampf(p.x, pl.rect.left() + 20.0f, pl.rect.right() - 20.0f);
    float d = std::fabs(x - p.x) + std::fabs(pl.rect.top() - p.y) * 0.5f;
    if (d < bd) {
      bd = d;
      best = x;
    }
  }
  return best;
}

int nearestLedge(const Match& m, Vec2 p) {
  int best = -1;
  float bd = 1e9f;
  for (int l = 0; l < m.ledgeCount(); ++l) {
    if (!m.ledgeValid(l)) continue;
    float d = (m.ledgePoint(l) - p).len2();
    if (d < bd) {
      bd = d;
      best = l;
    }
  }
  return best;
}
// The closest ledge in direction dir (-1 / 1) from p, or -1.
int ledgeToward(const Match& m, Vec2 p, float dir) {
  int best = -1;
  float bd = 1e9f;
  for (int l = 0; l < m.ledgeCount(); ++l) {
    if (!m.ledgeValid(l) || (m.ledgePoint(l).x - p.x) * dir <= 0.0f) continue;
    float d = (m.ledgePoint(l) - p).len2();
    if (d < bd) {
      bd = d;
      best = l;
    }
  }
  return best;
}
}  // namespace

CpuBrain::CpuBrain(int level, uint32_t seed)
    : level_(std::clamp(level, 1, kMaxCpuLevel)), skill_(std::clamp(level, 1, 9) / 9.0f), rng_(seed * 2654435761u + 7u) {}

InputState CpuBrain::think(const Match& m, int self) {
  InputState in;
  const Fighter& f = m.fighters[self];
  if (!m.started() || !f.active() || m.finished) {
    last_ = in;
    return in;
  }
  if (dummy_ != kDummyCpu) {  // training dummy
    bool free = f.state == FState::Idle || f.state == FState::Walk || f.state == FState::Landing;
    if (f.state == FState::Respawn && f.stateFrame > 60) in.stick.y = 1.0f;
    if (dummy_ == kDummyJump && free && f.grounded) tap(in, &IS::jump);
    if (dummy_ == kDummyShield && f.grounded && f.state != FState::Respawn) in.shield = true;
    last_ = in;
    return in;
  }

  switch (f.state) {
    case FState::Respawn:
      if (f.stateFrame > 40 + rng_.irange(0, 60)) in.stick.y = 1.0f;
      last_ = in;
      return in;
    case FState::Hitstun: {
      Vec2 toCenter = (Vec2{0, -200} - f.pos).norm();
      in.stick = toCenter * skill_;
      if (f.tumble && m.groundBelow(f.pos, 60.0f) && rng_.chance(skill_ * 0.5f)) tap(in, &IS::shield);
      last_ = in;
      return in;
    }
    case FState::Grabbed:
      if (rng_.chance(0.3f + 0.6f * skill_)) tap(in, &IS::attack);
      last_ = in;
      return in;
    case FState::Holding: {
      if (waitTimer_ < 0) waitTimer_ = rng_.irange(4, 20);
      if (--waitTimer_ > 0) {
        if (level_ >= 5 && rng_.chance(0.08f)) tap(in, &IS::attack);  // pummel
        last_ = in;
        return in;
      }
      waitTimer_ = -1;
      const Rect& main = m.platforms[0].rect;
      float edgeAhead = f.facing > 0 ? main.right() - f.pos.x : f.pos.x - main.left();
      float edgeBehind = f.facing > 0 ? f.pos.x - main.left() : main.right() - f.pos.x;
      const Fighter& v = m.fighters[std::max(0, f.grabPartner)];
      if (edgeAhead < 200) in.stick.x = float(f.facing);
      else if (edgeBehind < 200) in.stick.x = float(-f.facing);
      else if (v.percent > 100) in.stick.y = -1.0f;
      else in.stick = rng_.chance(0.5f) ? Vec2{0, 1} : Vec2{float(f.facing), 0};
      last_ = in;
      return in;
    }
    case FState::LedgeHang: {
      if (waitTimer_ < 0) waitTimer_ = rng_.irange(8, 50 - level_ * 3);
      if (--waitTimer_ <= 0) {
        waitTimer_ = -1;
        int toStage = (f.ledge % 2 == 0) ? 1 : -1;
        float r = rng_.uniform();
        if (r < 0.5f) in.stick.x = float(toStage);
        else if (r < 0.7f) tap(in, &IS::jump);
        else if (r < 0.85f) tap(in, &IS::attack);
        else tap(in, &IS::shield);
      }
      last_ = in;
      return in;
    }
    case FState::Knockdown:
      if (f.stateFrame > 12 && rng_.chance(0.1f + 0.2f * skill_)) {
        if (rng_.chance(0.5f)) tap(in, &IS::attack);
        else tap(in, &IS::jump);
      }
      last_ = in;
      return in;
    default:
      break;
  }
  waitTimer_ = -1;
  if (f.grounded) crossTimer_ = 0;
  else if (crossTimer_ > 0) --crossTimer_;

  if (!f.grounded && recover(m, f, in)) {
    last_ = in;
    return in;
  }
  manageWeapon(m, f, in);
  if (m.orb.active && chaseOrb(m, f, in)) {
    last_ = in;
    return in;
  }
  if (f.weapon < 0 && !m.items.empty() && fetchWeapon(m, f, in)) {
    last_ = in;
    return in;
  }

  // Pick the closest opponent.
  const Fighter* target = nullptr;
  float best = 1e9f;
  for (const auto& o : m.fighters) {
    if (!m.foes(f, o) || !o.active()) continue;
    float d = (o.pos - f.pos).len() + (o.state == FState::Respawn ? 600.0f : 0.0f);
    if (d < best) {
      best = d;
      target = &o;
    }
  }
  if (target) fight(m, f, *target, in);
  last_ = in;
  return in;
}

bool CpuBrain::recover(const Match& m, const Fighter& f, InputState& in) {
  if (m.groundBelow(f.pos, 1500.0f)) return false;
  int l = crossTimer_ > 0 ? ledgeToward(m, f.pos, crossDir_) : -1;  // flying over a gap: the far side
  if (l < 0) l = nearestLedge(m, f.pos);
  if (l < 0) return false;
  Vec2 L = m.ledgePoint(l);
  int toStage = (l % 2 == 0) ? 1 : -1;
  Vec2 goal = L + Vec2{toStage * 30.0f, -40.0f};
  in.stick.x = signf(goal.x - f.pos.x);
  if (f.state == FState::Helpless || f.state == FState::AirDodge) return true;
  if (f.state == FState::Attack) {
    in.stick = (goal - f.center()).norm();  // steer teleports / zips
    const MoveDef* mv = f.currentMove();
    if (mv && mv->teleportFrame < 0 && in.stick.y > 0.5f)  // never fast-fall into the void mid-attack
      in.stick = Vec2{in.stick.x, 0.5f}.norm() * 0.6f;
    in.special = last_.special;              // keep charging-type specials short
    return true;
  }
  if (f.state != FState::Air) return true;
  float below = f.pos.y - L.y;  // > 0 : feet under the ledge
  float dist = std::fabs(f.pos.x - L.x);
  bool falling = f.vel.y > -1.0f;
  // Under the stage (feet below its underside, body not clear of its side): get out from under it first,
  // or the jump and the special only bump into the underside.
  const Rect& plat = m.platforms[l / 2].rect;
  const float clearance = (L.x - f.pos.x) * toStage - f.def->width * 0.5f;  // > 0: clear of the side
  if (f.pos.y > plat.bottom() && clearance < 6.0f) {
    in.stick = {float(-toStage), 0.0f};
    if (f.pos.y < plat.bottom() + 220.0f) return true;  // still time to drift out
    if (falling && f.airJumps > 0) {
      tap(in, &IS::jump);
    } else if (falling) {
      in.stick = Vec2{float(-toStage), -1.0f}.norm();
      tap(in, &IS::special);
    }
    return true;
  }
  if (falling && below > -80.0f && f.airJumps > 0) {
    tap(in, &IS::jump);
  } else if (falling && f.airJumps == 0 && dist > 260.0f && below < 120.0f && !f.sideSpecialUsed &&
             sideSpecialTravels(*f.def)) {
    in.stick = {float(toStage), 0.0f};
    tap(in, &IS::special);
  } else if (falling && f.airJumps == 0 && (below > -20.0f || dist > 200.0f)) {
    Vec2 aim = (goal - f.center()).norm();
    if (aim.y > -0.6f) aim = Vec2{aim.x * 0.6f, -0.8f}.norm();
    in.stick = aim;
    tap(in, &IS::special);
  }
  return true;
}

// Walks to one of its own weapons when it is close and nobody is in its face, and takes it (the weapon button).
bool CpuBrain::fetchWeapon(const Match& m, const Fighter& f, InputState& in) {
  const WeaponItem* best = nullptr;
  float bd = 650.0f;
  for (const auto& it : m.items) {
    if (it.character != f.charIndex || !it.grounded) continue;
    float d = std::fabs(it.pos.x - f.pos.x) + std::fabs(it.pos.y - f.pos.y);
    if (d < bd) {
      bd = d;
      best = &it;
    }
  }
  if (!best || !f.grounded) return false;
  for (const auto& o : m.fighters)
    if (m.foes(f, o) && o.active() && (o.pos - f.pos).len() < 110.0f) return false;  // busy fighting
  const float dx = best->pos.x - f.pos.x, dy = best->pos.y - f.pos.y;
  in.stick.x = std::fabs(dx) > 12.0f ? signf(dx) : 0.0f;
  if (!m.groundBelow({f.pos.x + in.stick.x * 40.0f, f.pos.y}, 30.0f) && dy > -20.0f) in.stick.x = 0.0f;  // no walking off
  if (std::fabs(dx) < 24.0f && std::fabs(dy) < 30.0f) tap(in, &IS::weapon);  // on it: take it
  if (dy < -50.0f && std::fabs(dx) < 160.0f && f.state != FState::JumpSquat) tap(in, &IS::jump);
  else if (dy > 30.0f && std::fabs(dx) < 30.0f) in.stick.y = 1.0f;  // drop through a thin platform
  return true;
}

// How far a weapon's neutral special flies (0: no projectile): the "distance" weapon is the one that reaches further.
static float weaponRange(const WeaponDef& w) {
  for (const auto& [id, mv] : w.moves)
    if (id == NSpecial && mv.projectileFrame >= 0) return std::fabs(mv.projectile.velocity.x) * mv.projectile.lifetime;
  return 0.0f;
}

// Armed, the weapon button switches A <-> B: the distance weapon when the opponent is far, the other one up close;
// it thinks it over every few seconds.
void CpuBrain::manageWeapon(const Match& m, const Fighter& f, InputState& in) {
  if (f.weapon < 0) return;  // bare hands: it goes and takes its weapon (fetchWeapon)
  if (weaponTimer_ < 0) weaponTimer_ = rng_.irange(20, std::max(30, 150 - level_ * 12));
  if (weaponTimer_ > 0) {
    --weaponTimer_;
    return;
  }
  if (f.state == FState::Respawn || f.state == FState::Attack || f.state == FState::Hitstun) return;
  float dist = 1e9f;
  for (const auto& o : m.fighters)
    if (m.foes(f, o) && o.active()) dist = std::min(dist, std::fabs(o.pos.x - f.pos.x));
  const int ranged = weaponRange(f.def->weapons[1]) > weaponRange(f.def->weapons[0]) ? 1 : 0;
  const int want = dist > 380.0f ? ranged : dist < 200.0f ? 1 - ranged : std::max(0, f.weapon);
  if (want != f.weapon) tap(in, &IS::weapon);
  weaponTimer_ = rng_.irange(90, 240);
}

bool CpuBrain::chaseOrb(const Match& m, const Fighter& f, InputState& in) {
  Vec2 d = m.orb.pos - f.center();
  if (d.len() > 520.0f || f.finalSmash) return false;
  for (const auto& o : m.fighters)
    if (m.foes(f, o) && o.active() && (o.pos - f.pos).len() < 120.0f) return false;  // busy fighting
  if (!f.grounded && !m.groundBelow({f.pos.x + f.vel.x * 12.0f, f.pos.y}, 1500.0f)) return false;
  in.stick.x = std::fabs(d.x) > 30.0f ? signf(d.x) : 0.0f;
  if (f.grounded && !m.groundBelow({f.pos.x + in.stick.x * 40.0f, f.pos.y}, 30.0f)) in.stick.x = 0.0f;
  if (--decisionTimer_ > 0) return true;
  decisionTimer_ = std::max(3, 14 - level_);
  bool canAct = f.state == FState::Idle || f.state == FState::Walk || f.state == FState::Run ||
                f.state == FState::Crouch || f.state == FState::Air;
  if (!canAct) return true;
  if (d.len() < 90.0f) {
    Vec2 dir = std::fabs(d.x) > std::fabs(d.y) ? Vec2{signf(d.x), 0.0f} : Vec2{0.0f, signf(d.y)};
    tapCStick(in, dir);
  } else if (f.grounded && d.y < -60.0f && std::fabs(d.x) < 140.0f) {
    tap(in, &IS::jump);
  } else if (!f.grounded && f.airJumps > 0 && d.y < -80.0f && f.vel.y > 0.0f) {
    tap(in, &IS::jump);
  }
  return true;
}

void CpuBrain::fight(const Match& m, const Fighter& f, const Fighter& t, InputState& in) {
  const CharacterDef& d = *f.def;
  if (projCooldown_ > 0) --projCooldown_;
  if (f.finalSmash && t.state != FState::Respawn && t.state != FState::Captured &&
      (f.state == FState::Idle || f.state == FState::Walk || f.state == FState::Run || f.state == FState::Air)) {
    const UltReach u = ultReach(f);
    const Vec2 rel = t.center() - f.center();
    const bool facing = (rel.x >= 0.0f ? 1 : -1) == f.facing;
    bool go = u.anywhere || rel.len() < u.range;
    if (u.ahead && !u.anywhere) go = go && facing && std::fabs(rel.y) < 140.0f && std::fabs(rel.x) < u.range;
    if (go) {
      tap(in, &IS::ultimate);
      return;
    }
    if (u.ahead && !facing && f.grounded) in.stick.x = signf(rel.x);  // turn round first
  }
  float dx = t.pos.x - f.pos.x;
  float dy = t.center().y - f.center().y;
  float adx = std::fabs(dx);
  int toward = dx >= 0 ? 1 : -1;

  // Charging a smash attack: keep holding.
  if (chargeHold_ > 0) {
    --chargeHold_;
    in.cstick = chargeDir_;
    return;
  }
  // Shielding: keep it up, punish when the threat is over (never in the middle of an ultimate).
  if (shieldHold_ > 0) {
    --shieldHold_;
    in.shield = true;
    if (!isThreat(t, f) && adx < 70.0f && t.state == FState::Attack && t.move != FinalSmash && rng_.chance(skill_)) {
      tap(in, &IS::attack);  // shield + attack = grab out of shield
      shieldHold_ = 0;
    }
    return;
  }
  // An opponent unleashes an ultimate: shield it or not, decided once (the better the CPU, the more often).
  if (f.grounded && (f.state == FState::Idle || f.state == FState::Walk || f.state == FState::Run ||
                     f.state == FState::Crouch || f.state == FState::Landing)) {
    for (const auto& o : m.fighters) {
      if (!m.foes(f, o) || !o.active() || o.state != FState::Attack || o.move != FinalSmash) continue;
      const int key = o.id * 1000003 + (m.frame - o.moveFrame);  // this ultimate (its first frame)
      if (key != ultKey_) {
        ultKey_ = key;
        ultBlock_ = rng_.chance(0.2f + 0.5f * skill_);
      }
      if (ultBlock_ && m.freezeTimer == 0 && (isThreat(o, f) || (o.center() - f.center()).len() < 420.0f)) {
        shieldHold_ = rng_.irange(36, 64);
        in.shield = true;
        return;
      }
    }
  }
  if (f.grounded && (f.state == FState::Idle || f.state == FState::Walk || f.state == FState::Run ||
                     f.state == FState::Crouch)) {
    bool threat = isThreat(t, f);
    for (const auto& p : m.projectiles) {
      if (!m.foes(p.owner, f)) continue;
      Vec2 rel = f.center() - p.pos;
      if (rel.len() < 160.0f && dot(rel, p.vel) > 0.0f) threat = true;
    }
    if (threat && rng_.chance(0.15f * skill_ + 0.03f)) {
      if (specialUse(f, DSpecial).counter && rng_.chance(0.35f + 0.3f * skill_)) {
        in.stick = {0, 1};
        tap(in, &IS::special);
        return;
      }
      if (rng_.chance(0.75f)) {
        shieldHold_ = rng_.irange(8, 20);
        in.shield = true;
      } else {
        in.stick = {0, 1};  // will spot dodge from shield next frames
        in.shield = true;
        shieldHold_ = 2;
      }
      return;
    }
  }

  // Movement intent (continuous)
  const Rect& home = m.platforms[0].rect;
  bool targetOffstage = !m.groundBelow(t.pos, 1500.0f);
  float goalX = t.pos.x;
  if (targetOffstage) {  // edge guard: wait at the edge
    goalX = clampf(t.pos.x, home.left() + 40.0f, home.right() - 40.0f);
  }
  float gdx = goalX - f.pos.x;
  if (std::fabs(gdx) > 40.0f) in.stick.x = signf(gdx);
  const bool targetGrounded = t.grounded && !targetOffstage;
  if (!f.grounded) {
    // never drift into the void: head for the target's platform or the closest one
    if (!m.groundBelow({f.pos.x + f.vel.x * 12.0f, f.pos.y}, 1500.0f)) {
      float safeX = targetGrounded ? t.pos.x : nearestPlatformX(m, f.pos);
      in.stick.x = signf(safeX - f.pos.x);
    }
    if (f.vel.y > 0 && dy > 40.0f && m.groundBelow(f.pos, 1500.0f) &&
        m.groundBelow({f.pos.x + f.vel.x * 20.0f, f.pos.y}, 1500.0f) && rng_.chance(0.3f * skill_))
      in.stick.y = 1.0f;  // fast fall (never over the void, nor drifting into it)
  } else {
    // stop at the edge, unless the target waits across a gap: then jump over it
    float ahead = f.pos.x + in.stick.x * 40.0f;
    if (in.stick.x != 0.0f && !m.groundBelow({ahead, f.pos.y}, 30.0f)) {
      if (targetGrounded && signf(dx) == in.stick.x && adx < 600.0f) {
        tap(in, &IS::jump);
        crossDir_ = in.stick.x;
        crossTimer_ = 150;
      } else {
        in.stick.x = 0.0f;
      }
    }
  }

  if (--decisionTimer_ > 0) return;
  decisionTimer_ = std::max(2, 22 - level_ * 2) + rng_.irange(0, 6);
  if (rng_.chance(0.25f * (1.0f - skill_))) return;  // low levels hesitate

  bool killRange = t.percent > 110.0f - level_ * 3.0f;
  // Spacing depends on the fighter's reach (swords hit from further away).
  const float close = std::max(70.0f, moveReach(f, f.moveDef(FTilt)) * 0.75f);
  const float mid = close + 60.0f;
  const bool tShield = t.state == FState::Shield || t.state == FState::ShieldStun;

  if (!f.grounded) {
    if (f.state != FState::Air) return;
    const bool overGround =
        m.groundBelow(f.pos, 1500.0f) && m.groundBelow({f.pos.x + f.vel.x * 20.0f, f.pos.y}, 1500.0f);
    if (adx < 110.0f && std::fabs(dy) < 110.0f && (overGround || f.airJumps > 0)) {
      Vec2 dir{0, 0};
      if (dy < -40.0f && adx < 60.0f) dir = {0, -1};
      else if (dy > 40.0f && adx < 60.0f) dir = overGround ? Vec2{0, 1} : Vec2{float(toward), 0};
      else if (adx > 30.0f) dir = {float(toward), 0};
      if (dir.len() < 0.1f) tap(in, &IS::attack);
      else tapCStick(in, dir);
    } else if (dy < -80.0f && f.airJumps > 0 && f.vel.y > 0 && rng_.chance(0.4f)) {
      tap(in, &IS::jump);
    }
    return;
  }

  if (f.state != FState::Idle && f.state != FState::Walk && f.state != FState::Run && f.state != FState::Crouch)
    return;

  if (targetOffstage) {
    if (adx < 140.0f && std::fabs(dy) < 120.0f) {
      chargeDir_ = {float(toward), 0};
      tapCStick(in, chargeDir_);
      chargeHold_ = rng_.irange(0, 6);
    }
    return;
  }

  // Specials: dashes, beams, ranged grabs, area attacks and the odd projectile
  bool ownShot = false;  // one of its projectiles still flying: no new one (no spamming)
  for (const auto& p : m.projectiles) ownShot = ownShot || p.owner == f.id;
  const bool canShoot = projCooldown_ == 0 && !ownShot;
  if (std::fabs(dy) < 70.0f && rng_.chance(0.16f + 0.1f * skill_)) {
    const SpecialUse n = specialUse(f, NSpecial), s = specialUse(f, SSpecial), dn = specialUse(f, DSpecial);
    auto use = [&](Vec2 stick) {
      in.stick = stick;
      if (f.facing != toward && stick.x == 0.0f) in.stick.x = 0.3f * toward;
      tap(in, &IS::special);
    };
    auto shoot = [&](Vec2 stick) {
      projCooldown_ = 70 + rng_.irange(0, 70) - level_ * 3;
      use(stick);
    };
    if (adx < close && n.melee) return use({0, 0});
    if (adx < close + 20.0f && dn.melee && !dn.projectile) return use({0, 1});
    if (s.dash && std::fabs(adx - s.landX) < 45.0f) return use({float(toward), 0});
    if (dn.grab && adx < dn.reach + 10.0f && adx > close * 0.6f) return use({0, 1});
    if (dn.beam && adx > close * 0.8f && adx < dn.reach && f.facing == toward && rng_.chance(0.6f)) return use({0, 1});
    if (n.beam && adx > close && adx < n.reach - 20.0f && f.facing == toward && rng_.chance(0.6f)) return use({0, 0});
    if (s.beam && adx > close && adx < s.reach - 20.0f && f.facing == toward && rng_.chance(0.6f)) return use({float(toward), 0});
    if (adx > mid && adx < 520.0f && canShoot) {
      if (s.projectile && rng_.chance(0.5f)) return shoot({float(toward), 0});
      if (dn.projectile && rng_.chance(0.5f)) return shoot({0, 1});
    }
  }

  if (std::fabs(dy) < 60.0f) {
    if (adx < close) {
      if (tShield && rng_.chance(0.6f + 0.3f * skill_)) {
        in.stick.x = float(toward);
        tap(in, &IS::grab);
      } else if (killRange && rng_.chance(0.6f)) {
        chargeDir_ = rng_.chance(0.7f) ? Vec2{float(toward), 0} : Vec2{0, 1};
        tapCStick(in, chargeDir_);
        chargeHold_ = rng_.irange(0, 4 + level_ * 2);
      } else {
        float r = rng_.uniform();
        if (r < 0.3f) {
          in.stick = {0, 0};
          tap(in, &IS::attack);
        } else if (r < 0.55f) {
          in.stick = {0.6f * toward, 0};
          tap(in, &IS::attack);
        } else if (r < 0.7f) {
          in.stick = {0, 1};
          tap(in, &IS::attack);
        } else if (r < 0.85f) {
          in.stick.x = float(toward);
          tap(in, &IS::grab);
        } else {
          tap(in, &IS::jump);
        }
      }
    } else if (adx < mid) {
      if (killRange && rng_.chance(0.5f)) {
        chargeDir_ = {float(toward), 0};
        tapCStick(in, chargeDir_);
        chargeHold_ = rng_.irange(0, 10);
      } else if (f.state == FState::Run && rng_.chance(0.4f)) {
        tap(in, &IS::attack);  // dash attack
      } else if (rng_.chance(0.5f)) {
        tap(in, &IS::jump);
      }
    } else if (adx > 350.0f && hasProjectile(d) && canShoot && rng_.chance(0.35f)) {
      in.stick = {0, 0};
      if (f.facing != toward) in.stick.x = 0.3f * toward;
      projCooldown_ = 70 + rng_.irange(0, 70) - level_ * 3;
      tap(in, &IS::special);
    } else if (adx < 230.0f && rng_.chance(0.25f)) {
      tap(in, &IS::jump);
    }
  } else if (dy < -60.0f) {
    if (adx < 60.0f && dy > -170.0f) {
      if (killRange || rng_.chance(0.4f)) {
        chargeDir_ = {0, -1};
        tapCStick(in, chargeDir_);
        chargeHold_ = rng_.irange(0, 8);
      } else {
        in.stick = {0, -1};
        tap(in, &IS::attack);
      }
    } else if (adx < 200.0f && rng_.chance(0.5f)) {
      tap(in, &IS::jump);
    }
  } else if (dy > 80.0f && f.groundPlat >= 0 && !m.platforms[f.groundPlat].solid) {
    in.stick = {0, 1};  // drop through the platform
    if (last_.stick.y > 0.5f) in.stick = {0, 0};
  }
}
