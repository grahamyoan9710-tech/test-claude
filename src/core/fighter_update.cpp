// Fighter state machine: reads the controller and decides what happens.
#include "core/match.hpp"

namespace {
using IS = InputState;

bool isAerial(int m) { return m >= NAir && m <= DAir; }
bool isSpecialMove(int m) { return (m >= NSpecial && m <= DSpecial) || m >= Extra1; }

int smashFor(Fighter& f, Vec2 d) {
  if (std::fabs(d.y) > std::fabs(d.x)) return d.y < 0 ? USmash : DSmash;
  if (d.x != 0) f.facing = d.x > 0 ? 1 : -1;
  return FSmash;
}

int tiltFor(Fighter& f) {
  Vec2 s = f.in.stick;
  if (s.y < -0.5f && std::fabs(s.y) >= std::fabs(s.x) * 0.8f) return UTilt;
  if (s.y > 0.5f && std::fabs(s.y) >= std::fabs(s.x) * 0.8f) return DTilt;
  if (std::fabs(s.x) > 0.3f) {
    f.facing = s.x > 0 ? 1 : -1;
    return FTilt;
  }
  return Jab;
}

int aerialFor(const Fighter& f, Vec2 d) {
  if (d.len() < 0.3f) return NAir;
  if (std::fabs(d.y) >= std::fabs(d.x)) return d.y < 0 ? UAir : DAir;
  return (d.x > 0 ? 1 : -1) == f.facing ? FAir : BAir;
}

int specialFor(Fighter& f) {
  Vec2 s = f.in.stick;
  if (s.y < -0.5f && std::fabs(s.y) >= std::fabs(s.x) * 0.7f) return USpecial;
  if (s.y > 0.5f && std::fabs(s.y) >= std::fabs(s.x) * 0.7f) return DSpecial;
  if (std::fabs(s.x) > 0.4f) {
    if (!f.grounded && f.sideSpecialUsed) return -1;
    f.facing = s.x > 0 ? 1 : -1;
    return SSpecial;
  }
  return NSpecial;
}
}  // namespace

void Match::setState(Fighter& f, FState s, int timer) {
  f.state = s;
  f.stateFrame = 0;
  f.timer = timer;
  if (s != FState::Attack) {
    f.move = -1;
    f.charging = false;
  }
  if (s != FState::Hitstun) f.tumble = false;
}

void Match::startMove(Fighter& f, int id) {
  const MoveDef& m = f.def->moves[id];
  if (!m.exists()) return;
  f.state = FState::Attack;
  f.stateFrame = 0;
  f.move = id;
  f.moveFrame = 0;
  f.charge = 0;
  f.charging = m.chargeFrame >= 0;
  f.comboQueued = false;
  f.counterTriggered = false;
  f.moveLeftGround = false;
  f.moveStartedAir = !f.grounded;
  f.hitList.clear();
  if (m.intangibleStart == 0) f.intangible = std::max(f.intangible, 1);  // covers the start frame too
  if (id == SSpecial && !f.grounded) f.sideSpecialUsed = true;
}

bool Match::tryGroundActions(Fighter& f) {
  const IS& in = f.in;
  if (f.finalSmash && freezeTimer == 0 && f.pressed(&IS::ultimate)) {
    startFinalSmash(f);
    return true;
  }
  if (f.pressed(&IS::jump)) {
    setState(f, FState::JumpSquat, f.def->jumpSquat);
    return true;
  }
  if (f.pressed(&IS::grab) || (in.shield && f.pressed(&IS::attack))) {
    startMove(f, Grab);
    return true;
  }
  if (f.pressed(&IS::special)) {
    int sp = specialFor(f);
    if (sp >= 0) {
      startMove(f, sp);
      return true;
    }
  }
  if (f.cstickPressed()) {
    startMove(f, smashFor(f, f.in.cstick));
    return true;
  }
  if (f.pressed(&IS::smash)) {
    startMove(f, smashFor(f, in.stick.len() > 0.3f ? in.stick : Vec2{float(f.facing), 0.0f}));
    return true;
  }
  if (f.pressed(&IS::attack)) {
    // A flick (direction + attack together) is a smash attack; holding the direction a
    // little first gives a tilt, even if the fighter already started to dash.
    if (f.state == FState::Run && f.stateFrame > 10 && std::fabs(in.stick.x) > 0.5f) {
      startMove(f, DashAttack);
    } else if (f.flickX() || (f.flickY() && in.stick.y < 0) ||
               (f.flickY() && in.stick.y > 0 && (f.state != FState::Crouch || f.stateFrame <= 3))) {
      startMove(f, smashFor(f, in.stick));
    } else {
      startMove(f, tiltFor(f));
    }
    return true;
  }
  if (in.shield) {
    setState(f, FState::Shield);
    return true;
  }
  return false;
}

bool Match::tryAirActions(Fighter& f) {
  if (f.finalSmash && freezeTimer == 0 && f.pressed(&IS::ultimate)) {
    startFinalSmash(f);
    return true;
  }
  if (f.pressed(&IS::special)) {
    int sp = specialFor(f);
    if (sp >= 0) {
      startMove(f, sp);
      return true;
    }
  }
  if (f.cstickPressed()) {
    startMove(f, aerialFor(f, f.in.cstick));
    return true;
  }
  if (f.pressed(&IS::smash)) {
    startMove(f, aerialFor(f, f.in.stick.len() > 0.3f ? f.in.stick : Vec2{float(f.facing), 0.0f}));
    return true;
  }
  if (f.pressed(&IS::attack)) {
    startMove(f, aerialFor(f, f.in.stick));
    return true;
  }
  if (f.pressed(&IS::jump) && f.airJumps > 0) {
    airJump(f);
    return true;
  }
  if ((f.pressed(&IS::shield) || f.pressed(&IS::grab)) && !f.airDodgeUsed) {
    setState(f, FState::AirDodge, 34);
    f.airDodgeUsed = true;
    f.fastFall = false;
    if (f.in.stick.len() > 0.3f) {
      f.vel = f.in.stick.norm() * 10.0f;
      f.actionDir = 1;
    } else {
      f.vel *= 0.5f;
      f.actionDir = 0;
    }
    return true;
  }
  return false;
}

void Match::airControl(Fighter& f, float mul) {
  const CharacterDef& d = *f.def;
  float sx = f.in.stick.x;
  if (std::fabs(sx) > 0.15f)
    f.vel.x = approach(f.vel.x, sx * d.airSpeed * std::max(mul, 0.3f), d.airAccel * mul);
  else
    f.vel.x = approach(f.vel.x, 0.0f, d.airFriction);
  if (!f.fastFall && f.vel.y > -2.0f && f.in.stick.y > 0.7f && f.flickY()) f.fastFall = true;
}

void Match::applyGravity(Fighter& f, float mul) {
  if (mul <= 0.0f) return;
  f.vel.y += f.def->gravity * mul;
  if (f.fastFall && f.vel.y > 0.0f)
    f.vel.y = f.def->fastFall;
  else if (f.vel.y > f.def->maxFall)
    f.vel.y = approach(f.vel.y, f.def->maxFall, 1.0f);
}

void Match::groundFriction(Fighter& f, float mul) {
  f.vel.x = approach(f.vel.x, 0.0f, f.def->friction * mul);
}

void Match::startJump(Fighter& f) {
  const CharacterDef& d = *f.def;
  f.vel.y = -(f.in.jump ? d.jumpVel : d.shortHopVel);
  f.vel.x = clampf(f.vel.x + f.in.stick.x * 1.5f, -d.airSpeed, d.airSpeed);
  f.grounded = false;
  f.groundPlat = -1;
  f.fastFall = false;
  setState(f, FState::Air);
  emit(EvType::Jump, f.pos, f.id);
}

void Match::airJump(Fighter& f) {
  const CharacterDef& d = *f.def;
  f.vel.y = -d.airJumpVel;
  f.vel.x = f.in.stick.x * d.airSpeed;
  f.airJumps--;
  f.fastFall = false;
  f.airJumpFrame = frame;
  setState(f, FState::Air);
  emit(EvType::AirJump, f.pos, f.id);
}

void Match::updateFighter(Fighter& f) {
  if (f.eliminated) return;
  if (f.state == FState::Dead) {
    if (--f.respawnTimer <= 0 && f.stocks > 0) respawnFighter(f);
    return;
  }
  if (f.flash > 0) f.flash--;
  if (f.hitlag > 0) {
    f.hitlag--;
    return;
  }
  f.stateFrame++;
  if (f.intangible > 0) f.intangible--;
  if (f.invincible > 0) f.invincible--;
  if (f.ledgeCooldown > 0) f.ledgeCooldown--;
  if (f.dropThrough > 0) f.dropThrough--;
  if (f.techWindow > 0) f.techWindow--;
  if (f.techLockout > 0) f.techLockout--;
  if (f.pressed(&IS::shield) && f.techLockout == 0) {
    f.techWindow = 20;
    f.techLockout = 40;
  }
  if (f.state != FState::Shield && f.state != FState::ShieldStun)
    f.shield = std::min(kMaxShield, f.shield + 0.08f);

  const CharacterDef& d = *f.def;
  const IS& in = f.in;
  switch (f.state) {
    case FState::Idle:
      groundFriction(f);
      if (!f.grounded) { setState(f, FState::Air); break; }
      if (tryGroundActions(f)) break;
      if (in.stick.y > 0.6f) {
        if (f.groundPlat >= 0 && !platforms[f.groundPlat].solid && f.flickY()) {
          f.dropThrough = 12;
          f.grounded = false;
          f.groundPlat = -1;
          f.pos.y += 2.0f;
          setState(f, FState::Air);
        } else {
          setState(f, FState::Crouch);
        }
        break;
      }
      if (std::fabs(in.stick.x) > 0.25f) {
        f.facing = in.stick.x > 0 ? 1 : -1;
        if (f.flickX()) {
          setState(f, FState::Run);
          emit(EvType::Dash, f.pos, f.id);
        } else {
          setState(f, FState::Walk);
        }
      }
      break;
    case FState::Walk:
      if (!f.grounded) { setState(f, FState::Air); break; }
      if (tryGroundActions(f)) break;
      if (in.stick.y > 0.6f) { setState(f, FState::Crouch); break; }
      if (std::fabs(in.stick.x) < 0.2f) { setState(f, FState::Idle); break; }
      f.facing = in.stick.x > 0 ? 1 : -1;
      if (f.flickX()) { setState(f, FState::Run); emit(EvType::Dash, f.pos, f.id); break; }
      f.vel.x = approach(f.vel.x, in.stick.x * d.walkSpeed, d.groundAccel);
      break;
    case FState::Run:
      if (!f.grounded) { setState(f, FState::Air); break; }
      if (tryGroundActions(f)) break;
      if (in.stick.y > 0.6f) { setState(f, FState::Crouch); break; }
      if (std::fabs(in.stick.x) < 0.2f) { setState(f, FState::Idle); break; }
      if ((in.stick.x > 0 ? 1 : -1) != f.facing) {
        f.facing = -f.facing;
        f.vel.x *= 0.25f;
        emit(EvType::Dash, f.pos, f.id);
      }
      f.vel.x = approach(f.vel.x, f.facing * d.runSpeed, d.groundAccel);
      break;
    case FState::Crouch:
      groundFriction(f);
      if (!f.grounded) { setState(f, FState::Air); break; }
      if (f.groundPlat >= 0 && !platforms[f.groundPlat].solid && f.flickY() && in.stick.y > 0) {
        f.dropThrough = 12;
        f.grounded = false;
        f.groundPlat = -1;
        f.pos.y += 2.0f;
        setState(f, FState::Air);
        break;
      }
      if (tryGroundActions(f)) break;
      if (in.stick.y < 0.5f) setState(f, FState::Idle);
      break;
    case FState::JumpSquat:
      groundFriction(f, 0.5f);
      if (f.stateFrame >= f.timer) startJump(f);
      break;
    case FState::Air:
      airControl(f);
      applyGravity(f);
      tryAirActions(f);
      break;
    case FState::Landing:
      groundFriction(f);
      if (!f.grounded) { airControl(f); applyGravity(f); }
      if (f.stateFrame >= f.timer) setState(f, f.grounded ? FState::Idle : FState::Air);
      break;
    case FState::Attack:
      updateMove(f);
      break;
    case FState::Shield:
      groundFriction(f);
      if (!f.grounded) { setState(f, FState::Air); break; }
      f.shield -= 0.13f;
      if (f.shield <= 0.0f) {
        f.shield = 0.0f;
        setState(f, FState::ShieldBreak, 220);
        f.vel = {0, -9};
        f.grounded = false;
        emit(EvType::ShieldBreak, f.center(), f.id);
        break;
      }
      if (f.pressed(&IS::jump)) { setState(f, FState::JumpSquat, d.jumpSquat); break; }
      if (f.pressed(&IS::attack) || f.pressed(&IS::grab)) { startMove(f, Grab); break; }
      if (f.flickX()) {
        setState(f, FState::Roll, 28);
        f.actionDir = in.stick.x > 0 ? 1 : -1;
        break;
      }
      if (f.flickY() && in.stick.y > 0) { setState(f, FState::SpotDodge, 24); break; }
      if (!in.shield) setState(f, FState::Landing, 7);
      break;
    case FState::ShieldStun:
      groundFriction(f, 0.5f);
      if (f.stateFrame >= f.timer) setState(f, in.shield ? FState::Shield : FState::Idle);
      break;
    case FState::Roll:
      if (f.stateFrame == 3) f.intangible = 15;
      if (f.stateFrame >= 3 && f.stateFrame < 20)
        f.vel.x = f.actionDir * 7.0f;
      else
        groundFriction(f, 2.0f);
      if (f.stateFrame >= f.timer) {
        f.facing = -f.actionDir;
        setState(f, FState::Idle);
      }
      break;
    case FState::SpotDodge:
      groundFriction(f, 2.0f);
      if (f.stateFrame == 2) f.intangible = 15;
      if (f.stateFrame >= f.timer) setState(f, FState::Idle);
      break;
    case FState::AirDodge:
      if (f.stateFrame == 2) f.intangible = 18;
      if (f.actionDir == 1 && f.stateFrame < 20) {
        f.vel *= 0.9f;
      } else {
        airControl(f, 0.5f);
        applyGravity(f);
      }
      if (f.stateFrame >= f.timer) setState(f, FState::Air);
      break;
    case FState::Hitstun:
      applyGravity(f);
      if (f.grounded) groundFriction(f);
      if (f.tumble) f.spin += 0.25f * f.facing;
      if (--f.hitstun <= 0) {
        bool tumbled = f.tumble;
        setState(f, f.grounded ? FState::Idle : FState::Air);
        f.tumble = tumbled && !f.grounded;
      }
      break;
    case FState::Knockdown:
      groundFriction(f, 1.5f);
      if (!f.grounded) {  // slid off an edge: free to act again
        setState(f, FState::Air);
        applyGravity(f);
        break;
      }
      if (f.stateFrame > 60) {
        setState(f, FState::GetUp, 22);
        f.intangible = 22;
      } else if (f.stateFrame > 10) {
        if (f.pressed(&IS::attack)) {
          startMove(f, GetupAttack);
        } else if (std::fabs(in.stick.x) > 0.6f) {
          setState(f, FState::Roll, 28);
          f.actionDir = in.stick.x > 0 ? 1 : -1;
        } else if (f.pressed(&IS::jump) || f.pressed(&IS::shield) || in.stick.y < -0.6f) {
          setState(f, FState::GetUp, 22);
          f.intangible = 22;
        }
      }
      break;
    case FState::GetUp:
      groundFriction(f);
      if (f.stateFrame >= f.timer) setState(f, FState::Idle);
      break;
    case FState::Helpless:
      airControl(f, 0.7f);
      applyGravity(f);
      break;
    case FState::LedgeHang:
    case FState::LedgeClimb:
      updateLedge(f);
      return;  // ledge positions are set directly
    case FState::Holding: {
      groundFriction(f);
      Fighter* v = f.grabPartner >= 0 ? &fighters[f.grabPartner] : nullptr;
      if (!v || v->state != FState::Grabbed) {
        f.grabPartner = -1;
        setState(f, FState::Idle);
        break;
      }
      if (f.stateFrame > 4) {
        Vec2 s = in.stick;
        if (f.pressed(&IS::attack)) {
          startMove(f, Pummel);
        } else if (s.len() > 0.6f) {
          int id;
          if (std::fabs(s.y) > std::fabs(s.x)) id = s.y < 0 ? UThrow : DThrow;
          else id = (s.x > 0 ? 1 : -1) == f.facing ? FThrow : BThrow;
          startMove(f, id);
        } else if (f.stateFrame > 200) {
          releaseGrab(f, true);
        }
      }
      break;
    }
    case FState::Grabbed:
      updateGrabbed(f);
      return;
    case FState::ShieldBreak:
      if (f.grounded) {
        groundFriction(f);
        if (--f.timer <= 0) {
          f.shield = kMaxShield * 0.6f;
          setState(f, FState::Idle);
        }
      } else {
        applyGravity(f);
      }
      break;
    case FState::Respawn:
      f.pos.y = stageDef->respawn.y;  // x keeps the side-by-side offset from respawnFighter
      f.vel = {0, 0};
      if (f.stateFrame > 300 ||
          (f.stateFrame > 30 && (in.stick.len() > 0.4f || in.attack || in.special || in.jump || in.shield))) {
        setState(f, FState::Air);
        f.invincible = 120;
      }
      return;
    case FState::Dead:
      return;
  }
  moveFighter(f);
}

void Match::updateMove(Fighter& f) {
  const MoveDef& m = f.def->moves[f.move];
  const IS& in = f.in;
  if (f.charging && f.moveFrame == m.chargeFrame) {
    bool held = isSpecialMove(f.move) ? in.special : (in.attack || in.smash || in.cstick.len() > 0.5f);
    if (held && f.charge < m.maxCharge) {
      f.charge++;
      if (f.grounded) groundFriction(f);
      else applyGravity(f, 0.5f);
      return;
    }
    f.charging = false;
  }
  f.moveFrame++;
  const int t = f.moveFrame;
  if (!f.grounded) f.moveLeftGround = true;

  if (m.counterStart < 0 && !m.isGrab && m.throwFrame < 0) {
    for (const auto& h : m.hitboxes)
      if (h.start == t) {
        emit(EvType::Swing, f.center(), f.id, -1, h.damage, {}, h.fx);
        break;
      }
  }
  for (const auto& e : m.vel) {
    if (e.frame != t || (e.airOnly && f.grounded)) continue;
    Vec2 v{e.v.x * f.facing + in.stick.x * e.stickX, e.v.y};
    switch (e.mode) {
      case VelMode::Set: f.vel = v; break;
      case VelMode::SetX: f.vel.x = v.x; break;
      case VelMode::SetY: f.vel.y = v.y; break;
      case VelMode::Add: f.vel += v; break;
      case VelMode::Scale: f.vel.x *= e.v.x; f.vel.y *= e.v.y; break;
      case VelMode::StickDir: {
        Vec2 dir = in.stick.len() > 0.3f ? in.stick.norm() : Vec2{0, -1};
        if (std::fabs(dir.x) > 0.2f) f.facing = dir.x > 0 ? 1 : -1;
        f.vel = dir * e.v.x;
        break;
      }
    }
    if (f.vel.y < -0.5f) f.grounded = false;
  }
  if (m.projectileFrame == t) spawnProjectiles(f, m.projectile);
  if (m.intangibleStart >= 0 && t == std::max(1, m.intangibleStart))  // t starts at 1
    f.intangible = m.intangibleEnd - t;
  if (m.teleportFrame == t) {
    Vec2 dir = in.stick.len() > 0.3f ? in.stick.norm() : Vec2{0, -1};
    if (std::fabs(dir.x) > 0.2f) f.facing = dir.x > 0 ? 1 : -1;
    f.pos += dir * m.teleportDist;
    f.grounded = false;
    f.vel = {0, 0};
    for (const auto& p : platforms) {  // never end up inside the stage
      if (p.solid && p.rect.contains({f.pos.x, f.pos.y - 1.0f})) {
        if (f.pos.y - p.rect.top() < p.rect.h * 0.6f) f.pos.y = p.rect.top();
        else f.pos.y = p.rect.bottom() + f.height() + 1.0f;
      }
    }
  }
  if (m.throwFrame == t && f.grabPartner >= 0) {
    Fighter& v = fighters[f.grabPartner];
    const HitboxDef& h = m.hitboxes[0];
    if (m.pummel) {
      v.percent += h.damage;
      v.flash = 6;
      f.damageDealt += h.damage;
      gainUlt(f, h.damage * kUltPerDamageDealt);
      gainUlt(v, h.damage * kUltPerDamageTaken);
      f.hitlag = v.hitlag = 3;
      emit(EvType::Hit, v.center(), f.id, v.id, h.damage, {float(f.facing), 0});
    } else {
      f.grabPartner = -1;
      v.grabPartner = -1;
      setState(v, FState::Air);
      emit(EvType::Throw, v.center(), f.id, v.id, h.damage);
      applyHit(&f, v, h, v.center(), f.facing, 1.0f, true);
    }
  }
  if (m.nextMove >= 0) {
    bool p = m.comboOnSpecial ? f.pressed(&IS::special) : f.pressed(&IS::attack);
    if (p && t >= m.comboStart && t <= m.comboEnd) f.comboQueued = true;
    if (f.comboQueued && t >= m.comboAt) {
      startMove(f, m.nextMove);
      return;
    }
  }
  if (m.loopFrame == t && !f.grounded) f.moveFrame--;

  if (f.grounded) {
    groundFriction(f, m.frictionMul);
  } else {
    if (m.allowDrift) airControl(f, (isAerial(f.move) || f.moveStartedAir) ? 1.0f : 0.6f);
    applyGravity(f, m.gravityMul);
  }

  if (f.moveFrame >= m.duration) {
    if (f.move == Pummel || ((f.move >= FThrow && f.move <= DThrow) && f.grabPartner >= 0)) {
      setState(f, FState::Holding);
      f.stateFrame = 7;
      return;
    }
    if (m.helplessAfter && !f.grounded) setState(f, FState::Helpless);
    else setState(f, f.grounded ? FState::Idle : FState::Air);
  }
}

void Match::updateLedge(Fighter& f) {
  Vec2 L = ledgePoint(f.ledge);
  int toStage = (f.ledge % 2 == 0) ? 1 : -1;
  const IS& in = f.in;
  if (f.state == FState::LedgeHang) {
    f.facing = toStage;
    f.pos = {L.x - toStage * (f.width() * 0.5f + 2.0f), L.y + f.height() * 0.75f};
    f.vel = f.kbVel = {0, 0};
    if (f.stateFrame < 8) return;
    float toward = in.stick.x * toStage;
    int action = -1;
    if (f.pressed(&IS::jump) || (in.stick.y < -0.6f && f.flickY())) {
      setState(f, FState::Air);
      f.pos = {L.x - toStage * f.width() * 0.25f, L.y - 2.0f};
      f.vel = {toStage * 2.0f, -f.def->jumpVel};
      f.intangible = 6;
      f.ledgeCooldown = 30;
      f.ledge = -1;
      emit(EvType::Jump, f.pos, f.id);
      return;
    } else if (f.pressed(&IS::attack) || f.pressed(&IS::special)) {
      action = 1;
    } else if (f.pressed(&IS::shield)) {
      action = 2;
    } else if (toward > 0.5f || in.stick.y < -0.6f) {
      action = 0;
    } else if (toward < -0.5f || in.stick.y > 0.6f || f.stateFrame > 360) {
      setState(f, FState::Air);
      f.pos.x -= toStage * 6.0f;
      f.ledgeCooldown = 40;
      f.ledge = -1;
      return;
    }
    if (action >= 0) {
      f.ledgeFrom = f.pos;
      f.ledgeTo = {L.x + toStage * f.width() * 0.7f, L.y};
      f.ledgeAction = action;
      setState(f, FState::LedgeClimb);
      f.intangible = 24;
    }
    return;
  }
  // Climbing up
  float t = smoothstep(f.stateFrame / 18.0f);
  f.pos = lerpv(f.ledgeFrom, f.ledgeTo, t);
  f.pos.y -= std::sin(t * kPi) * 20.0f;
  if (f.stateFrame >= 18) {
    f.pos = f.ledgeTo;
    f.grounded = true;
    f.groundPlat = f.ledge / 2;
    f.ledge = -1;
    f.ledgeCooldown = 20;
    f.facing = toStage;
    if (f.ledgeAction == 1) {
      startMove(f, LedgeAttack);
    } else if (f.ledgeAction == 2) {
      setState(f, FState::Roll, 28);
      f.actionDir = toStage;
    } else {
      setState(f, FState::Idle);
    }
  }
}

void Match::updateGrabbed(Fighter& f) {
  Fighter* g = f.grabPartner >= 0 ? &fighters[f.grabPartner] : nullptr;
  bool held = g && g->grabPartner == f.id &&
              (g->state == FState::Holding || (g->state == FState::Attack && g->move >= Pummel && g->move <= DThrow));
  if (!held) {
    f.grabPartner = -1;
    setState(f, f.grounded ? FState::Idle : FState::Air);
    return;
  }
  f.pos = {g->pos.x + g->facing * (g->width() * 0.5f + f.width() * 0.5f - 4.0f), g->pos.y};
  f.vel = f.kbVel = {0, 0};
  f.grounded = g->grounded;
  f.groundPlat = g->groundPlat;
  f.facing = -g->facing;
  bool mash = f.pressed(&IS::attack) || f.pressed(&IS::special) || f.pressed(&IS::jump) ||
              f.pressed(&IS::shield) || f.flickX() || f.flickY();
  f.escapeTimer -= mash ? 6 : 1;
  if (f.escapeTimer <= 0 && g->state == FState::Holding) releaseGrab(f, true);
}

void Match::releaseGrab(Fighter& f, bool escaped) {
  if (f.grabPartner < 0) return;
  Fighter& o = fighters[f.grabPartner];
  Fighter& victim = (f.state == FState::Grabbed) ? f : o;
  Fighter& grabber = (f.state == FState::Grabbed) ? o : f;
  victim.grabPartner = grabber.grabPartner = -1;
  if (victim.state == FState::Grabbed) {
    setState(victim, victim.grounded ? FState::Landing : FState::Air, 12);
    victim.vel.x = grabber.facing * 6.0f;
  }
  if (grabber.state == FState::Holding || grabber.state == FState::Attack) {
    setState(grabber, grabber.grounded ? FState::Landing : FState::Air, escaped ? 18 : 6);
    grabber.vel.x = -grabber.facing * 5.0f;
  }
}
