// Fighter poses: every state and every move is animated procedurally.
// Attacks move the limb tagged on the hitbox towards the hitbox position.
#include <cstdio>

#include "game/render.hpp"
#include "game/settings.hpp"

namespace {
bool isPuff(const CharacterDef& d) { return d.style == BodyStyle::Puff; }

Color fxColor(HitFx fx) {
  switch (fx) {
    case HitFx::Fire: return {255, 140, 40, 255};
    case HitFx::Electric: return {120, 230, 255, 255};
    case HitFx::Slash: return {210, 230, 255, 255};
    case HitFx::Dark: return {170, 90, 255, 255};
    case HitFx::Magic: return {255, 130, 220, 255};
    default: return {255, 255, 255, 255};
  }
}

Pose airPose(const Fighter& f, const Match& m, float time) {
  const float H = f.height();
  Pose p = restPose(*f.def, time);
  if (f.vel.y < 0) {
    p.footF = {0.10f * H, 0.34f * H};
    p.footB = {-0.08f * H, 0.42f * H};
  } else {
    p.footF = {0.08f * H, 0.46f * H};
    p.footB = {-0.10f * H, 0.50f * H};
  }
  p.handF = {0.22f * H, -0.10f * H};
  p.handB = {-0.16f * H, -0.08f * H};
  int since = m.frame - f.airJumpFrame;
  if (since >= 0 && since < 24) p.rot = smoothstep(since / 24.0f) * 2.0f * kPi;
  return p;
}

void applyMove(Pose& p, const Fighter& f, float time) {
  const CharacterDef& d = *f.def;
  const MoveDef& m = d.moves[f.move];
  const float H = f.height();
  const int t = f.moveFrame;
  const Vec2 shoulder{0.03f * H, -0.17f * H};

  if (m.isGrab || m.throwFrame >= 0) {
    float k = m.isGrab ? smoothstep(t / 6.0f) : smoothstep(float(t) / std::max(1, m.throwFrame));
    Vec2 reach{0.34f * H, -0.04f * H};
    if (f.move == UThrow) reach = {0.12f * H, -0.5f * H};
    if (f.move == DThrow) { reach = {0.3f * H, 0.25f * H}; p.crouch = k; }
    if (f.move == BThrow) { reach = {-0.25f * H, -0.3f * H}; p.lean = -0.5f * k; }
    if (f.move == Pummel) reach = {0.3f * H, 0.05f * H};
    p.handF = lerpv(p.handF, reach, k);
    p.handB = lerpv(p.handB, reach + Vec2{-0.04f * H, 0.05f * H}, k);
    return;
  }
  if (m.hitboxes.empty()) {  // projectile throw
    float k = m.projectileFrame > 0 ? 1.0f - std::fabs(float(t - m.projectileFrame)) / 8.0f : 0.0f;
    k = clampf(k, 0.0f, 1.0f);
    p.handF = lerpv(p.handF, {0.36f * H, -0.12f * H}, k);
    p.lean += 0.2f * k;
    return;
  }
  if (m.counterStart >= 0 && !f.counterTriggered) {  // guard stance
    p.handF = {0.14f * H, -0.22f * H};
    p.handB = {0.10f * H, -0.12f * H};
    p.crouch = 0.3f;
    if (d.weaponLength > 0) {
      p.weapon = true;
      p.weaponDir = Vec2{0.2f, -1.0f}.norm();
    }
    return;
  }
  // Primary hitbox: active one, else the next one, else the last one.
  const HitboxDef* hb = nullptr;
  for (const auto& h : m.hitboxes)
    if (t >= h.start && t < h.end && (!hb || h.radius > hb->radius)) hb = &h;
  if (!hb)
    for (const auto& h : m.hitboxes)
      if (h.start > t && (!hb || h.start < hb->start)) hb = &h;
  if (!hb)
    for (const auto& h : m.hitboxes)
      if (!hb || h.end > hb->end) hb = &h;
  if (!hb) return;

  float e;
  if (f.charging) e = -0.35f;
  else if (t < hb->start) e = -0.35f * smoothstep(float(t) / std::max(1, hb->start - 1));
  else if (t < hb->end) e = 1.0f;
  else e = 1.0f - smoothstep((t - hb->end) / 10.0f);
  if (t >= hb->start - 2 && t < hb->start) e = 0.6f;  // snap into the strike

  Vec2 target = hb->offset;
  switch (hb->limb) {
    case Limb::ArmFront: p.handF = lerpv(p.handF, target, e); break;
    case Limb::ArmBack: p.handB = lerpv(p.handB, target, e); break;
    case Limb::LegFront: p.footF = lerpv(p.footF, target, e); p.lean -= 0.25f * std::max(0.0f, e); break;
    case Limb::LegBack: p.footB = lerpv(p.footB, target, e); p.lean += 0.25f * std::max(0.0f, e); break;
    case Limb::Head: p.shift.y -= 0.1f * H * std::max(0.0f, e); p.squash = 1.0f + 0.12f * std::max(0.0f, e); break;
    case Limb::Body: {
      int groups = 0;
      for (const auto& h : m.hitboxes) groups = std::max(groups, h.group + 1);
      if (groups >= 3) {
        p.rot = t * 0.55f;
        p.crouch = 0.4f;
      } else {
        p.crouch = 0.8f * std::max(0.0f, e);
        p.footF = lerpv(p.footF, {0.14f * H, 0.24f * H}, std::max(0.0f, e));
        p.footB = lerpv(p.footB, {-0.08f * H, 0.3f * H}, std::max(0.0f, e));
        p.handF = lerpv(p.handF, {0.18f * H, 0.0f}, std::max(0.0f, e));
      }
      break;
    }
    case Limb::Weapon: {
      p.weapon = true;
      Vec2 rel = target - shoulder;
      float tgt = std::atan2(rel.y, rel.x);
      float rest = 0.6f;
      float wind = tgt - 2.0f;
      float a;
      if (f.charging) a = wind;
      else if (t < hb->start) a = lerpf(rest, wind, smoothstep(float(t) / std::max(1, hb->start)));
      else if (t < hb->end) a = lerpf(wind, tgt, smoothstep(float(t - hb->start + 1) / std::max(1, hb->end - hb->start)));
      else a = lerpf(tgt, rest, smoothstep((t - hb->end) / 12.0f));
      p.weaponDir = {std::cos(a), std::sin(a)};
      p.handF = shoulder + p.weaponDir * (0.28f * H);
      break;
    }
    case Limb::None: break;
  }
  if (std::fabs(target.x) > std::fabs(target.y) && hb->limb != Limb::Weapon)
    p.lean += 0.15f * std::max(0.0f, e) * signf(target.x);
  if (f.charging) p.shift.x += std::sin(time * 60.0f) * 1.5f;
  if (t >= hb->start && t < hb->end) {
    p.trail = 1.0f;
    p.trailPos = target;
    p.trailRadius = hb->radius;
    p.trailFx = hb->fx;
  }
}
}  // namespace

Pose restPose(const CharacterDef& d, float time) {
  const float H = d.height;
  Pose p;
  float bob = std::sin(time * 3.0f) * 0.012f * H;
  p.lean = 0.06f;
  p.crouch = 0.05f + std::sin(time * 3.0f) * 0.03f;
  p.handF = {0.20f * H, -0.02f * H + bob};
  p.handB = {0.08f * H, 0.03f * H + bob};
  p.footF = {0.10f * H, 0.5f * H};
  p.footB = {-0.09f * H, 0.5f * H};
  if (isPuff(d)) {
    p.handF = {0.33f * H, 0.08f * H + bob};
    p.handB = {-0.30f * H, 0.08f * H + bob};
    p.footF = {0.14f * H, 0.5f * H};
    p.footB = {-0.14f * H, 0.5f * H};
  }
  if (d.style == BodyStyle::Knight) {
    p.weapon = true;
    p.weaponDir = Vec2{1.0f, 0.55f}.norm();
  }
  p.blink = std::fmod(time + d.height, 3.7f) < 0.12f;
  return p;
}

Pose computePose(const Fighter& f, const Match& m, float time) {
  const CharacterDef& d = *f.def;
  const float H = d.height;
  const float tt = time + f.id * 1.7f;
  Pose p = restPose(d, tt);
  const float sf = float(f.stateFrame);
  switch (f.state) {
    case FState::Walk: {
      float ph = tt * 10.0f;
      p.footF = {0.12f * H * std::sin(ph), 0.5f * H - std::max(0.0f, -std::cos(ph)) * 0.06f * H};
      p.footB = {-0.12f * H * std::sin(ph), 0.5f * H - std::max(0.0f, std::cos(ph)) * 0.06f * H};
      p.handF.x -= 0.06f * H * std::sin(ph);
      p.handB.x += 0.06f * H * std::sin(ph);
      break;
    }
    case FState::Run: {
      float ph = sf * 0.42f;
      p.lean = 0.35f;
      p.footF = {0.2f * H * std::sin(ph), 0.5f * H - std::max(0.0f, -std::cos(ph)) * 0.14f * H};
      p.footB = {-0.2f * H * std::sin(ph), 0.5f * H - std::max(0.0f, std::cos(ph)) * 0.14f * H};
      p.handF = {0.12f * H - 0.12f * H * std::sin(ph), -0.04f * H};
      p.handB = {0.12f * H * std::sin(ph) - 0.04f * H, 0.0f};
      p.shift.y = -std::fabs(std::sin(ph)) * 0.03f * H;
      break;
    }
    case FState::Crouch: case FState::JumpSquat: case FState::Landing:
      p.crouch = f.state == FState::Landing ? 0.7f * (1.0f - sf / std::max(1, f.timer)) : 1.0f;
      p.footF.x = 0.15f * H;
      p.footB.x = -0.14f * H;
      p.squash = 1.0f - 0.08f * p.crouch;
      break;
    case FState::Air: case FState::AirDodge:
      p = airPose(f, m, tt);
      if (f.state == FState::AirDodge) {
        p.crouch = 0.6f;
        p.handF = {0.1f * H, 0.0f};
      }
      break;
    case FState::Helpless:
      p = airPose(f, m, tt);
      p.handF = {0.10f * H, -0.45f * H};
      p.handB = {-0.10f * H, -0.42f * H};
      break;
    case FState::Hitstun:
      p.hurt = true;
      if (f.tumble) {
        p.rot = f.spin;
        p.handF = {0.25f * H, -0.3f * H};
        p.handB = {-0.25f * H, -0.25f * H};
        p.footF = {0.18f * H, 0.45f * H};
        p.footB = {-0.15f * H, 0.40f * H};
      } else {
        p.lean = -0.35f;
        p.handF = {-0.02f * H, -0.18f * H};
        p.handB = {-0.12f * H, -0.10f * H};
      }
      break;
    case FState::Knockdown:
      p.rot = -kPi * 0.5f;
      p.shift = {0.0f, 0.36f * H};
      p.footF = {0.12f * H, 0.5f * H};
      p.hurt = sf < 20;
      break;
    case FState::GetUp: {
      float k = smoothstep(sf / std::max(1, f.timer));
      p.rot = -kPi * 0.5f * (1.0f - k);
      p.shift = {0.0f, 0.36f * H * (1.0f - k)};
      break;
    }
    case FState::Roll:
      p.rot = (sf / 28.0f) * 2.0f * kPi * float(f.actionDir * f.facing);
      p.crouch = 1.0f;
      p.handF = {0.1f * H, 0.05f * H};
      p.handB = {0.05f * H, 0.08f * H};
      break;
    case FState::SpotDodge:
      p.lean = -0.25f;
      p.crouch = 0.5f;
      break;
    case FState::Shield: case FState::ShieldStun:
      p.crouch = 0.35f;
      p.handF = {0.12f * H, -0.08f * H};
      p.handB = {0.08f * H, -0.04f * H};
      break;
    case FState::LedgeHang:
      p.handF = {0.18f * H, -0.62f * H};
      p.handB = {0.12f * H, -0.60f * H};
      p.footF = {0.03f * H, 0.5f * H};
      p.footB = {-0.04f * H, 0.47f * H};
      p.lean = 0.1f;
      break;
    case FState::LedgeClimb:
      p.crouch = 0.6f;
      p.handF = {0.2f * H, 0.1f * H};
      break;
    case FState::Holding:
      p.handF = {0.32f * H, -0.05f * H};
      p.handB = {0.28f * H, 0.02f * H};
      p.lean = 0.15f;
      break;
    case FState::Grabbed:
      p.hurt = true;
      p.lean = -0.2f;
      p.handF = {0.1f * H + std::sin(tt * 25.0f) * 0.06f * H, -0.25f * H};
      p.handB = {-0.1f * H, -0.2f * H + std::cos(tt * 25.0f) * 0.06f * H};
      p.footF = {0.06f * H, 0.45f * H};
      break;
    case FState::ShieldBreak:
      p.hurt = true;
      p.lean = std::sin(tt * 4.0f) * 0.25f;
      p.handF = {0.08f * H, 0.15f * H};
      p.handB = {-0.05f * H, 0.15f * H};
      break;
    case FState::Attack:
      if (f.move == FinalSmash) {
        float k = smoothstep(f.moveFrame / 20.0f);
        p.handF = lerpv(p.handF, {0.28f * H, -0.48f * H}, k);
        p.handB = lerpv(p.handB, {-0.26f * H, -0.46f * H}, k);
        p.footF = {0.10f * H, 0.46f * H};
        p.footB = {-0.08f * H, 0.48f * H};
        p.lean = -0.1f * k;
        break;
      }
      if (!f.grounded) p = airPose(f, m, tt);
      applyMove(p, f, tt);
      break;
    default:
      break;
  }
  return p;
}

void drawFighter(const Fighter& f, const Match& m, float time, bool debugBoxes) {
  if (!f.active()) return;
  const CharacterDef& d = *f.def;
  const float H = d.height;
  Pose pose = computePose(f, m, time);

  float alpha = 1.0f;
  if (f.intangible > 0 && f.state != FState::LedgeHang) alpha = 0.6f;
  if (f.invincible > 0) alpha = (m.frame / 4) % 2 ? 0.45f : 0.9f;
  if (const MoveDef* mv = f.currentMove()) {
    if (mv->teleportFrame >= 0 && f.moveFrame >= mv->intangibleStart && f.moveFrame < mv->teleportFrame)
      alpha = 0.12f;
  }
  Color flashCol = WHITE;
  float flash = f.flash > 0 ? f.flash / 12.0f : 0.0f;
  if (const MoveDef* mv = f.currentMove()) {
    if (f.charging) {
      flashCol = {255, 235, 120, 255};
      flash = 0.25f + 0.2f * std::sin(time * 30.0f);
    } else if (mv->counterStart >= 0 && !f.counterTriggered && f.moveFrame >= mv->counterStart &&
               f.moveFrame < mv->counterEnd) {
      flashCol = {130, 190, 255, 255};
      flash = 0.3f + 0.15f * std::sin(time * 25.0f);
    } else if (mv->armorStart >= 0 && f.moveFrame >= mv->armorStart && f.moveFrame < mv->armorEnd) {
      flashCol = {255, 150, 60, 255};
      flash = std::max(flash, 0.25f);
    }
  }
  if (f.state == FState::Helpless) {
    flashCol = {30, 30, 50, 255};
    flash = 0.35f;
  }
  const bool usingFs = f.state == FState::Attack && f.move == FinalSmash;
  if (f.finalSmash || usingFs) {  // rainbow aura
    float hue = std::fmod(time * 200.0f, 360.0f);
    Vec2 c0 = f.center();
    for (int k = 0; k < 5; ++k)
      DrawCircleV(V(c0), H * (0.75f - k * 0.08f) + std::sin(time * 10.0f + k) * 4.0f,
                  withAlpha(ColorFromHSV(std::fmod(hue + k * 50.0f, 360.0f), 0.8f, 1.0f), 0.08f));
    flashCol = ColorFromHSV(hue, 0.6f, 1.0f);
    flash = std::max(flash, 0.22f + 0.1f * std::sin(time * 12.0f));
  }
  Vec2 feet = f.pos;
  if (f.hitlag > 0 && f.state == FState::Hitstun) feet.x += std::sin(m.frame * 2.7f) * 4.0f;

  if (f.state == FState::Respawn) {
    Color pc = slotColor(f);
    DrawEllipse(int(feet.x), int(feet.y + 6), 60, 12, withAlpha(pc, 0.35f));
    DrawEllipseLines(int(feet.x), int(feet.y + 6), 60, 12, withAlpha(WHITE, 0.8f));
    for (int i = 0; i < 3; ++i) {
      float a = time * 2.0f + i * 2.1f;
      DrawCircleV({feet.x + std::cos(a) * 55.0f, feet.y + 6 + std::sin(a) * 10.0f}, 4, withAlpha(WHITE, 0.9f));
    }
  }

  drawCharacter(d, f.palette, pose, feet, f.facing, 1.0f, alpha, flashCol, flash, time + f.id);

  Vec2 c = f.center();
  if (usingFs) {
    const MoveDef& mv = d.moves[FinalSmash];
    Color col = fxColor(mv.hitboxes.empty() ? HitFx::Magic : mv.hitboxes[0].fx);
    if (f.moveFrame < 34) {  // gathering energy
      float k = f.moveFrame / 34.0f;
      for (int i = 0; i < 10; ++i) {
        float a = time * 3.0f + i * kPi / 5.0f;
        float rr = H * (1.6f - k);
        DrawLineEx(V(c + Vec2{std::cos(a), std::sin(a)} * rr), V(c + Vec2{std::cos(a), std::sin(a)} * (rr * 0.6f)), 4,
                   withAlpha(col, 0.7f));
      }
      DrawCircleV(V(c), H * 0.3f * k, withAlpha(WHITE, 0.6f));
    }
    for (const auto& h : mv.hitboxes) {
      int t = f.moveFrame - h.start + 4;
      if (t < 0 || t > 22) continue;
      float k = smoothstep(t / 12.0f);
      float r = h.radius * k;
      float a = 1.0f - t / 22.0f;
      DrawCircleV(V(c), r, withAlpha(col, 0.18f * a));
      DrawRing(V(c), std::max(0.0f, r - 18.0f), r, 0, 360, 64, withAlpha(col, 0.85f * a));
      DrawRing(V(c), std::max(0.0f, r - 6.0f), r, 0, 360, 64, withAlpha(WHITE, 0.9f * a));
    }
  }
  if (pose.trail > 0.0f) {
    Vec2 tp = c + Vec2{pose.trailPos.x * f.facing, pose.trailPos.y};
    Color col = fxColor(pose.trailFx);
    float r = pose.trailRadius;
    float dirAng = std::atan2(pose.trailPos.y, pose.trailPos.x * f.facing) * 180.0f / kPi;
    DrawRing(V(tp), r * 0.55f, r * 1.05f, dirAng - 70.0f, dirAng + 70.0f, 24, withAlpha(col, 0.55f));
    DrawCircleV(V(tp), r * 0.5f, withAlpha(col, 0.18f));
  }
  if (f.state == FState::Shield || f.state == FState::ShieldStun) {
    float r = H * 0.62f * (0.35f + 0.65f * f.shield / kMaxShield);
    Color pc = slotColor(f);
    float k = f.shield / kMaxShield;
    Color sc = mixColor({255, 60, 60, 255}, pc, k);
    DrawCircleV(V(c), r, withAlpha(sc, 0.32f));
    DrawRing(V(c), r - 3.0f, r, 0, 360, 48, withAlpha(mixColor(sc, WHITE, 0.4f), 0.85f));
    DrawCircleV({c.x - r * 0.35f, c.y - r * 0.4f}, r * 0.18f, withAlpha(WHITE, 0.35f));
  }
  if (f.state == FState::ShieldBreak && f.grounded) {
    for (int i = 0; i < 3; ++i) {
      float a = time * 5.0f + i * 2.094f;
      Vec2 sp{c.x + std::cos(a) * H * 0.3f, c.y - H * 0.55f + std::sin(a) * H * 0.08f};
      DrawPoly(V(sp), 5, 7, a * 50.0f, {255, 230, 80, 255});
    }
  }
  // Player tag
  if (f.state != FState::Grabbed && settings().playerTags) {
    Color pc = slotColor(f);
    char tag[8];
    std::snprintf(tag, sizeof tag, f.cpu ? "CPU" : "J%d", f.port + 1);
    float ty = f.pos.y - H - 36.0f;
    if (f.state == FState::Knockdown) ty = f.pos.y - H * 0.5f - 30.0f;
    DrawTriangle({f.pos.x - 8, ty + 20}, {f.pos.x, ty + 29}, {f.pos.x + 8, ty + 20}, pc);
    drawTextCentered(tag, f.pos.x, ty, 20, pc, true, FontKind::Title);
  }
  if (debugBoxes) {
    Rect hr = f.hurtbox();
    DrawRectangleLinesEx({hr.x, hr.y, hr.w, hr.h}, 2, f.isIntangible() ? BLUE : YELLOW);
    if (const MoveDef* mv = f.currentMove()) {
      for (const auto& h : mv->hitboxes) {
        if (f.moveFrame < h.start || f.moveFrame >= h.end) continue;
        Vec2 hp = c + Vec2{h.offset.x * f.facing, h.offset.y};
        DrawCircleV(V(hp), h.radius, withAlpha(mv->isGrab ? PURPLE : RED, 0.45f));
      }
    }
  }
}

void drawPortrait(const CharacterDef& d, int palette, Vec2 center, float size, float time) {
  float scale = size / d.height;
  Pose p = restPose(d, time);
  drawCharacter(d, palette, p, center + Vec2{0, size * 0.5f}, 1, scale, 1.0f, WHITE, 0.0f, time);
}
