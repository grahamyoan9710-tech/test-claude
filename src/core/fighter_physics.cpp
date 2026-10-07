// Movement, platform collisions, landings and ledge grabs.
#include "core/match.hpp"

namespace {
constexpr float kKnockbackDecay = 0.26f;
bool isAerial(int m) { return m >= NAir && m <= DAir; }
}  // namespace

void Match::moveFighter(Fighter& f) {
  switch (f.state) {
    case FState::LedgeHang: case FState::LedgeClimb: case FState::Grabbed:
    case FState::Respawn: case FState::Dead:
      return;
    default:
      break;
  }
  const float w = f.width(), h = f.height();

  float kl = f.kbVel.len();
  if (kl > 0.0f) f.kbVel = f.kbVel * (std::max(0.0f, kl - kKnockbackDecay) / kl);

  if (f.grounded && f.groundPlat >= 0) f.pos += platforms[f.groundPlat].delta;
  Vec2 total = f.vel + f.kbVel;
  const float prevFeet = f.pos.y;

  // Horizontal movement + walls (sides of solid platforms)
  f.pos.x += total.x;
  for (const auto& p : platforms) {
    if (!p.solid) continue;
    Rect body{f.pos.x - w * 0.5f, f.pos.y - h, w, h};
    if (!body.overlaps(p.rect) || f.pos.y <= p.rect.top() + 1.0f) continue;
    f.pos.x = (f.pos.x < p.rect.center().x) ? p.rect.left() - w * 0.5f - 0.01f
                                             : p.rect.right() + w * 0.5f + 0.01f;
    if (f.state == FState::Hitstun && std::fabs(f.kbVel.x) > 6.0f) f.kbVel.x *= -0.5f;
    else f.kbVel.x = 0.0f;
    f.vel.x = 0.0f;
  }

  // Vertical movement
  if (f.grounded) {
    const Platform& p = platforms[f.groundPlat];
    if (total.y < -0.5f) {
      f.grounded = false;
      f.groundPlat = -1;
      f.pos.y += total.y;
    } else if (f.pos.x < p.rect.left() || f.pos.x > p.rect.right()) {
      f.grounded = false;  // walked off the edge
      f.groundPlat = -1;
      f.vel.y = 0.0f;
    } else {
      f.pos.y = p.rect.top();
      f.vel.y = 0.0f;
      if (f.kbVel.y > 0.0f) f.kbVel.y = 0.0f;
    }
  } else {
    float newY = f.pos.y + total.y;
    int landOn = -1;
    float landTop = 0.0f;
    if (total.y >= 0.0f) {
      for (int i = 0; i < int(platforms.size()); ++i) {
        const Platform& p = platforms[i];
        if (f.pos.x < p.rect.left() || f.pos.x > p.rect.right()) continue;
        if (!p.solid && f.dropThrough > 0) continue;
        float top = p.rect.top();
        float prevTop = top - p.delta.y;
        if (prevFeet <= prevTop + 0.5f && newY >= top && (landOn < 0 || top < landTop)) {
          landOn = i;
          landTop = top;
        }
      }
    }
    if (landOn >= 0) {
      f.pos.y = landTop;
      f.groundPlat = landOn;
      f.vel.y = 0.0f;
      onLand(f, total.y);
    } else {
      f.pos.y = newY;
      if (total.y < 0.0f) {
        for (const auto& p : platforms) {
          if (!p.solid) continue;
          if (f.pos.x < p.rect.left() - w * 0.4f || f.pos.x > p.rect.right() + w * 0.4f) continue;
          float headPrev = prevFeet - h, headNew = f.pos.y - h;
          if (headPrev >= p.rect.bottom() - 1.0f && headNew < p.rect.bottom()) {
            f.pos.y = p.rect.bottom() + h;
            f.vel.y = 0.0f;
            if (f.kbVel.y < 0.0f) f.kbVel.y *= -0.4f;
          }
        }
      }
      // Diagonal entry into a solid block: push out sideways.
      for (const auto& p : platforms) {
        if (!p.solid) continue;
        Rect body{f.pos.x - w * 0.5f, f.pos.y - h, w, h};
        if (!body.overlaps(p.rect) || f.pos.y <= p.rect.top() + 1.0f) continue;
        f.pos.x = (f.pos.x < p.rect.center().x) ? p.rect.left() - w * 0.5f - 0.01f
                                                 : p.rect.right() + w * 0.5f + 0.01f;
        f.vel.x = 0.0f;
        f.kbVel.x = 0.0f;
      }
    }
  }
  if (!f.grounded) tryLedgeGrab(f);
}

void Match::onLand(Fighter& f, float impact) {
  f.grounded = true;
  f.fastFall = false;
  f.airJumps = f.def->airJumps;
  f.airDodgeUsed = false;
  f.sideSpecialUsed = false;
  switch (f.state) {
    case FState::Air:
      setState(f, FState::Landing, 4);
      emit(EvType::Land, f.pos, f.id, -1, impact);
      break;
    case FState::Helpless:
      setState(f, FState::Landing, 18);
      emit(EvType::Land, f.pos, f.id, -1, impact);
      break;
    case FState::AirDodge:
      setState(f, FState::Landing, 10);
      emit(EvType::Land, f.pos, f.id, -1, impact);
      break;
    case FState::Attack: {
      const MoveDef& m = f.def->moves[f.move];
      if (isAerial(f.move)) {
        setState(f, FState::Landing, m.landingLag);
        emit(EvType::Land, f.pos, f.id, -1, impact);
      } else if (f.moveLeftGround || f.moveStartedAir) {
        if (m.landMove >= 0) startMove(f, m.landMove);
        else if (m.endOnLand) setState(f, FState::Landing, m.landingLag);
      }
      break;
    }
    case FState::Hitstun:
      if (f.tumble || impact > 8.0f) {
        if (f.techWindow > 0) {
          setState(f, FState::GetUp, 14);
          f.intangible = 20;
          f.kbVel = {0, 0};
          f.vel.x = 0.0f;
          emit(EvType::Tech, f.pos, f.id);
        } else if (f.kbVel.y > 9.0f) {
          f.grounded = false;
          f.groundPlat = -1;
          f.kbVel.y = -f.kbVel.y * 0.6f;
          f.pos.y -= 1.0f;
          emit(EvType::Land, f.pos, f.id, -1, impact);
        } else {
          setState(f, FState::Knockdown);
          f.kbVel = {f.kbVel.x * 0.5f, 0.0f};
          emit(EvType::Land, f.pos, f.id, -1, impact);
        }
      }
      break;
    default:
      break;
  }
}

void Match::tryLedgeGrab(Fighter& f) {
  if (f.grounded || f.ledgeCooldown > 0) return;
  bool ok = f.state == FState::Air || f.state == FState::Helpless ||
            (f.state == FState::Attack && f.move == USpecial && f.moveFrame > 8);
  if (!ok) return;
  if (f.state == FState::Air && f.vel.y + f.kbVel.y < 0.0f) return;
  const float w = f.width(), h = f.height();
  for (int l = 0; l < ledgeCount(); ++l) {
    if (!ledgeValid(l) || ledgeOwner(l) >= 0) continue;
    Vec2 L = ledgePoint(l);
    int toStage = (l % 2 == 0) ? 1 : -1;
    float dx = (f.pos.x - L.x) * toStage;  // negative = outside the stage
    if (dx > 8.0f || dx < -(w * 0.5f + 60.0f)) continue;
    float hand = f.pos.y - h * 0.8f;
    if (hand < L.y - 45.0f || hand > L.y + 50.0f) continue;
    setState(f, FState::LedgeHang);
    f.ledge = l;
    f.intangible = 40;
    f.airJumps = f.def->airJumps;
    f.fastFall = false;
    f.airDodgeUsed = false;
    f.sideSpecialUsed = false;
    f.vel = f.kbVel = {0, 0};
    emit(EvType::LedgeGrab, L, f.id);
    return;
  }
}

bool Match::groundBelow(Vec2 feet, float maxDist) const {
  for (const auto& p : platforms) {
    if (feet.x < p.rect.left() || feet.x > p.rect.right()) continue;
    float d = p.rect.top() - feet.y;
    if (d >= -1.0f && d <= maxDist) return true;
  }
  return false;
}
