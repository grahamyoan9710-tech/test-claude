#include "core/ai.hpp"

namespace {
using IS = InputState;

float moveReach(const MoveDef& m) {
  float r = 0.0f;
  for (const auto& h : m.hitboxes) r = std::max(r, std::fabs(h.offset.x) + h.radius);
  return r;
}

bool hasProjectile(const CharacterDef& d) { return d.moves[NSpecial].projectileFrame >= 0; }

bool sideSpecialTravels(const CharacterDef& d) {
  for (const auto& e : d.moves[SSpecial].vel)
    if (e.mode == VelMode::SetX && e.v.x > 8.0f) return true;
  return false;
}

// Is fighter t about to hit fighter f?
bool isThreat(const Fighter& t, const Fighter& f) {
  const MoveDef* m = t.currentMove();
  if (!m || m->hitboxes.empty() || m->isGrab) return false;
  int lastEnd = 0;
  for (const auto& h : m->hitboxes) lastEnd = std::max(lastEnd, h.end);
  if (t.moveFrame >= lastEnd) return false;
  float reach = moveReach(*m) + 30.0f;
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
}  // namespace

CpuBrain::CpuBrain(int level, uint32_t seed)
    : level_(std::clamp(level, 1, 9)), skill_(std::clamp(level, 1, 9) / 9.0f), rng_(seed * 2654435761u + 7u) {}

InputState CpuBrain::think(const Match& m, int self) {
  InputState in;
  const Fighter& f = m.fighters[self];
  if (!m.started() || !f.active() || m.finished) {
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

  if (!f.grounded && recover(m, f, in)) {
    last_ = in;
    return in;
  }

  // Pick the closest opponent.
  const Fighter* target = nullptr;
  float best = 1e9f;
  for (const auto& o : m.fighters) {
    if (o.id == f.id || !o.active()) continue;
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
  int l = nearestLedge(m, f.pos);
  if (l < 0) return false;
  Vec2 L = m.ledgePoint(l);
  int toStage = (l % 2 == 0) ? 1 : -1;
  Vec2 goal = L + Vec2{toStage * 30.0f, -40.0f};
  in.stick.x = signf(goal.x - f.pos.x);
  if (f.state == FState::Helpless || f.state == FState::AirDodge) return true;
  if (f.state == FState::Attack) {
    in.stick = (goal - f.center()).norm();  // steer teleports / zips
    in.special = last_.special;              // keep charging-type specials short
    return true;
  }
  if (f.state != FState::Air) return true;
  float below = f.pos.y - L.y;  // > 0 : feet under the ledge
  float dist = std::fabs(f.pos.x - L.x);
  bool falling = f.vel.y > -1.0f;
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

void CpuBrain::fight(const Match& m, const Fighter& f, const Fighter& t, InputState& in) {
  const CharacterDef& d = *f.def;
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
  // Shielding: keep it up, punish when the threat is over.
  if (shieldHold_ > 0) {
    --shieldHold_;
    in.shield = true;
    if (!isThreat(t, f) && adx < 70.0f && t.state == FState::Attack && rng_.chance(skill_)) {
      tap(in, &IS::attack);  // shield + attack = grab out of shield
      shieldHold_ = 0;
    }
    return;
  }
  if (f.grounded && (f.state == FState::Idle || f.state == FState::Walk || f.state == FState::Run ||
                     f.state == FState::Crouch)) {
    bool threat = isThreat(t, f);
    for (const auto& p : m.projectiles) {
      if (p.owner == f.id) continue;
      Vec2 rel = f.center() - p.pos;
      if (rel.len() < 160.0f && dot(rel, p.vel) > 0.0f) threat = true;
    }
    if (threat && rng_.chance(0.15f * skill_ + 0.03f)) {
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
    if (f.vel.y > 0 && dy > 40.0f && rng_.chance(0.3f * skill_)) in.stick.y = 1.0f;  // fast fall
  } else {
    // stop at the edge, unless the target waits across a gap: then jump over it
    float ahead = f.pos.x + in.stick.x * 40.0f;
    if (in.stick.x != 0.0f && !m.groundBelow({ahead, f.pos.y}, 30.0f)) {
      if (targetGrounded && signf(dx) == in.stick.x && adx < 600.0f) tap(in, &IS::jump);
      else in.stick.x = 0.0f;
    }
  }

  if (--decisionTimer_ > 0) return;
  decisionTimer_ = std::max(2, 22 - level_ * 2) + rng_.irange(0, 6);
  if (rng_.chance(0.25f * (1.0f - skill_))) return;  // low levels hesitate

  bool killRange = t.percent > 110.0f - level_ * 3.0f;
  // Spacing depends on the fighter's reach (swords hit from further away).
  const float close = std::max(70.0f, moveReach(d.moves[FTilt]) * 0.75f);
  const float mid = close + 60.0f;
  const bool tShield = t.state == FState::Shield || t.state == FState::ShieldStun;

  if (!f.grounded) {
    if (f.state != FState::Air) return;
    if (adx < 110.0f && std::fabs(dy) < 110.0f) {
      Vec2 dir{0, 0};
      if (dy < -40.0f && adx < 60.0f) dir = {0, -1};
      else if (dy > 40.0f && adx < 60.0f) dir = {0, 1};
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
    } else if (adx > 350.0f && hasProjectile(d) && rng_.chance(0.35f)) {
      in.stick = {0, 0};
      if (f.facing != toward) in.stick.x = 0.3f * toward;
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
