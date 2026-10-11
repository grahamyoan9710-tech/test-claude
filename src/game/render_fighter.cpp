// Fighter poses: every state and every move is animated procedurally.
// Attacks move the limb tagged on the hitbox towards the hitbox position, with a wind-up
// (anticipation), an arc and a follow-through taken from the move timing.
// drawFighter adds what needs memory: inertial blending between poses, squash & stretch,
// cloth drag, smears and afterimages. That memory is render-only: it never feeds the simulation.
#include <cstdio>

#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
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
Color smearColor(HitFx fx) { return fx == HitFx::Normal ? Color{255, 234, 190, 255} : fxColor(fx); }

float easeOut(float t) {
  t = clampf(t, 0.0f, 1.0f);
  return 1.0f - (1.0f - t) * (1.0f - t);
}
float wrapPi(float a) {
  a = std::fmod(a + kPi, 2.0f * kPi);
  return (a < 0.0f ? a + 2.0f * kPi : a) - kPi;
}
void tri(Vec2 a, Vec2 b, Vec2 c, Color col) {
  float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
  if (cross > 0) std::swap(b, c);
  DrawTriangle(V(a), V(b), V(c), col);
}
// Quad with one colour per corner (same batching path as raylib's shapes).
void gradQuad(Vec2 a, Color ca, Vec2 b, Color cb, Vec2 c, Color cc, Vec2 d, Color cd) {
  const Texture2D tex = GetShapesTexture();
  const Rectangle r = GetShapesTextureRectangle();
  const float u0 = r.x / tex.width, v0 = r.y / tex.height;
  const float u1 = (r.x + r.width) / tex.width, v1 = (r.y + r.height) / tex.height;
  rlSetTexture(tex.id);
  rlBegin(RL_QUADS);
  rlColor4ub(ca.r, ca.g, ca.b, ca.a);
  rlTexCoord2f(u0, v0);
  rlVertex2f(a.x, a.y);
  rlColor4ub(cb.r, cb.g, cb.b, cb.a);
  rlTexCoord2f(u0, v1);
  rlVertex2f(b.x, b.y);
  rlColor4ub(cc.r, cc.g, cc.b, cc.a);
  rlTexCoord2f(u1, v1);
  rlVertex2f(c.x, c.y);
  rlColor4ub(cd.r, cd.g, cd.b, cd.a);
  rlTexCoord2f(u1, v0);
  rlVertex2f(d.x, d.y);
  rlEnd();
  rlSetTexture(0);
}
// Glows add light; the frame blend (opaque off-screen image) is restored afterwards.
void additive(bool on) {
  if (on) BeginBlendMode(BLEND_ADDITIVE);
  else ui::frameBlend();
}

// flip: the air-jump somersault (never for attacks: the striking limb must stay on its hitbox).
Pose airPose(const Fighter& f, const Match& m, float time, bool flip = true) {
  const float H = f.height();
  Pose p = restPose(*f.def, time);
  float k = smoothstep((f.vel.y + 3.0f) / 6.0f);  // rising -> falling, no pop at the apex
  p.footF = lerpv({0.10f * H, 0.34f * H}, {0.08f * H, 0.46f * H}, k);
  p.footB = lerpv({-0.08f * H, 0.42f * H}, {-0.10f * H + std::sin(time * 8.0f) * 0.02f * H, 0.50f * H}, k);
  p.handF = lerpv({0.22f * H, -0.10f * H}, {0.24f * H, -0.17f * H}, k);
  p.handB = lerpv({-0.16f * H, -0.08f * H}, {-0.19f * H, -0.15f * H}, k);
  float st = f.vel.y < 0.0f ? std::min(0.12f, -f.vel.y * 0.009f) : std::min(0.05f, f.vel.y * 0.004f);
  p.squash = 1.0f + st;  // stretched along the vertical speed
  p.sx = 1.0f - st * 0.6f;
  int since = m.frame - f.airJumpFrame;
  if (flip && since >= 0 && since < 24) {  // somersault, tucked in the middle
    float u = since / 24.0f;
    float tuck = std::sin(u * kPi);
    p.rot = smoothstep(u) * 2.0f * kPi;
    p.crouch = 0.1f + 0.5f * tuck;
    p.footF = lerpv(p.footF, {0.14f * H, 0.22f * H}, tuck);
    p.footB = lerpv(p.footB, {0.0f, 0.26f * H}, tuck);
    p.handF = lerpv(p.handF, {0.16f * H, 0.02f * H}, tuck);
    p.handB = lerpv(p.handB, {0.06f * H, 0.04f * H}, tuck);
  }
  return p;
}

// One limb through a move: wind-up (anticipation), strike (whip into the hitbox), active
// (exactly on the hitbox), follow-through (overshoot, then back to rest).
enum class Phase { None, Wind, Strike, Active, Follow };
struct Strike {
  Phase phase = Phase::None;
  float k = 0.0f;      // progress inside the phase
  float e = 0.0f;      // 0 = rest, 1 = on the hitbox, < 0 = wound up behind, > 1 = overshoot
  float antic = 0.0f;  // wind-up amount (body coil)
  const HitboxDef* hb = nullptr;
};
Strike limbPhase(const MoveDef& m, Limb limb, int t, bool charging) {
  const HitboxDef *act = nullptr, *last = nullptr, *next = nullptr;
  for (const auto& h : m.hitboxes) {
    if (h.limb != limb) continue;
    if (t >= h.start && t < h.end) {
      if (!act || h.radius > act->radius) act = &h;
    } else if (h.end <= t) {
      if (!last || h.end > last->end) last = &h;
    } else if (!next || h.start < next->start) {
      next = &h;
    }
  }
  Strike s;
  if (act) {
    s.phase = Phase::Active;
    s.hb = act;
    s.k = float(t - act->start + 1) / float(std::max(1, act->end - act->start));
    s.e = 1.0f;
    return s;
  }
  if (last && (!next || t - last->end < (next->start - last->end) / 2)) {
    int rec = std::max(6, std::min(16, (next ? next->start : m.duration) - last->end - 2));
    s.phase = Phase::Follow;
    s.hb = last;
    s.k = clampf(float(t - last->end) / rec, 0.0f, 1.0f);
    s.e = (1.0f - smoothstep(s.k)) + 0.2f * std::sin(kPi * std::min(1.0f, s.k * 2.0f)) * (1.0f - s.k);
    return s;
  }
  if (!next) return s;
  s.hb = next;
  const int t0 = last ? (last->end + next->start) / 2 : 0;
  const int strike = std::min(2, next->start - t0);
  const int windEnd = next->start - strike;
  if (charging) {
    s.phase = Phase::Wind;
    s.k = 1.0f;
    s.e = -0.45f;
    s.antic = 1.0f;
  } else if (t < windEnd) {
    s.phase = Phase::Wind;
    s.k = float(t - t0 + 1) / float(windEnd - t0 + 1);
    s.antic = easeOut(s.k);
    s.e = -0.4f * s.antic;
  } else {
    s.phase = Phase::Strike;
    s.k = float(t - windEnd + 1) / float(strike + 1);
    s.e = -0.4f + 1.4f * s.k;
    s.antic = 1.0f - s.k;
  }
  return s;
}

// Limb path from rest to the hitbox: bowed upwards (or forwards for vertical strikes).
Vec2 strikePath(Vec2 rest, Vec2 target, float e) {
  Vec2 pos = lerpv(rest, target, e);
  if (e > 0.0f && e < 1.0f) {
    Vec2 d = target - rest;
    Vec2 n{d.y, -d.x};
    if (std::fabs(d.x) >= std::fabs(d.y) ? n.y > 0.0f : n.x < 0.0f) n = -n;
    pos += n * (0.22f * std::sin(kPi * e));
  }
  return pos;
}

void applyMove(Pose& p, const Fighter& f, float time) {
  const CharacterDef& d = *f.def;
  const MoveDef& m = f.moveDef(f.move);
  const float H = f.height();
  const int t = f.moveFrame;
  const Vec2 shoulder{0.03f * H, -0.17f * H};

  if (m.isGrab || m.throwFrame >= 0) {
    float k = m.isGrab ? smoothstep(t / 6.0f) : smoothstep(float(t) / std::max(1, m.throwFrame));
    float rel = 0.0f;  // after the throw: let go with a follow-through
    if (m.throwFrame >= 0 && t > m.throwFrame) {
      rel = clampf((t - m.throwFrame) / 12.0f, 0.0f, 1.0f);
      k = 1.0f - smoothstep(rel) * 0.8f;
    }
    Vec2 reach{0.34f * H, -0.04f * H};
    if (f.move == UThrow) reach = {0.12f * H, -0.5f * H};
    if (f.move == DThrow) { reach = {0.3f * H, 0.25f * H}; p.crouch = k; }
    if (f.move == BThrow) { reach = {-0.25f * H, -0.3f * H}; p.lean = -0.5f * k; }
    if (f.move == Pummel) {
      reach = {0.3f * H, 0.05f * H};
      float knee = clampf(1.0f - std::fabs(float(t - m.throwFrame)) / 5.0f, 0.0f, 1.0f);
      p.footF = lerpv(p.footF, {0.2f * H, 0.14f * H}, knee);
    }
    if (f.move == FThrow) p.lean += 0.3f * std::sin(kPi * rel);
    p.handF = lerpv(p.handF, reach, k);
    p.handB = lerpv(p.handB, reach + Vec2{-0.04f * H, 0.05f * H}, k);
    if (m.isGrab) {  // lunge
      float l = std::sin(kPi * clampf(t / 12.0f, 0.0f, 1.0f));
      p.lean += 0.2f * l;
      p.sx *= 1.0f + 0.06f * l;
    }
    return;
  }
  if (m.hitboxes.empty()) {  // projectile throw: wind up, thrust, follow through
    const int pf = std::max(4, m.projectileFrame);
    const Vec2 wind{-0.10f * H, -0.22f * H}, thrust{0.36f * H, -0.12f * H};
    if (t < pf - 3) {
      float k = easeOut(float(t + 1) / float(pf - 3));
      p.handF = lerpv(p.handF, wind, k);
      p.lean -= 0.14f * k;
      p.crouch += 0.12f * k;
    } else if (t < pf) {
      float u = float(t - pf + 4) / 4.0f;
      p.handF = lerpv(wind, thrust, u * u);
      p.lean += 0.12f * u;
    } else {
      float u = clampf((t - pf) / 14.0f, 0.0f, 1.0f);
      p.handF = lerpv(thrust, p.handF, smoothstep(u));
      p.lean += 0.2f * (1.0f - u);
      p.sx *= 1.0f + 0.06f * (1.0f - u);
    }
    return;
  }
  if (m.counterStart >= 0 && !f.counterTriggered) {  // guard stance
    p.handF = {0.14f * H, -0.22f * H};
    p.handB = {0.10f * H, -0.12f * H};
    p.crouch = 0.3f + 0.04f * std::sin(time * 9.0f);
    if (d.weaponLength > 0) {
      p.weapon = true;
      p.weaponDir = Vec2{0.2f, -1.0f}.norm();
    }
    return;
  }
  // Primary hitbox (drives the body): active one, else the next one, else the last one.
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
  const Strike pr = limbPhase(m, hb->limb, t, f.charging);

  // Every limb follows its own timeline (two kicks of one move do not pop between them).
  for (Limb L : {Limb::ArmFront, Limb::ArmBack, Limb::LegFront, Limb::LegBack, Limb::Head, Limb::Body, Limb::Weapon}) {
    const Strike s = L == hb->limb ? pr : limbPhase(m, L, t, f.charging);
    if (!s.hb) continue;
    const Vec2 target = s.hb->offset;
    const float e = s.e, ep = clampf(e, 0.0f, 1.0f);
    switch (L) {
      case Limb::ArmFront: p.handF = strikePath(p.handF, target, e); break;
      case Limb::ArmBack: p.handB = strikePath(p.handB, target, e); break;
      case Limb::LegFront: p.footF = strikePath(p.footF, target, e); p.lean -= 0.25f * ep; break;
      case Limb::LegBack: p.footB = strikePath(p.footB, target, e); p.lean += 0.25f * ep; break;
      case Limb::Head:
        p.shift.y -= 0.1f * H * ep;
        p.squash *= 1.0f + 0.12f * ep;
        p.crouch += 0.3f * s.antic;
        break;
      case Limb::Body: {
        int groups = 0;
        for (const auto& h : m.hitboxes) groups = std::max(groups, h.group + 1);
        if (groups >= 3) {
          p.rot = t * 0.55f;
          p.crouch = 0.4f;
          p.handF = {0.3f * H, -0.06f * H};
          p.handB = {-0.26f * H, -0.04f * H};
        } else {
          p.crouch = 0.8f * ep;
          p.footF = lerpv(p.footF, {0.14f * H, 0.24f * H}, ep);
          p.footB = lerpv(p.footB, {-0.08f * H, 0.3f * H}, ep);
          p.handF = lerpv(p.handF, {0.18f * H, 0.0f}, ep);
        }
        break;
      }
      case Limb::Weapon: {
        p.weapon = true;
        Vec2 rel = target - shoulder;
        float tgt = std::atan2(rel.y, rel.x);
        // The swing goes from the wind-up angle to a little past the target: the blade
        // sweeps through the hitbox during the active frames.
        float rest = 0.6f;
        float wind = tgt - 2.0f, end = tgt + 0.3f;
        float a = rest;
        switch (s.phase) {
          case Phase::Wind: a = lerpf(rest, wind, f.charging ? 1.0f : easeOut(s.k)); break;
          case Phase::Strike: a = lerpf(wind, end, 0.6f * s.k); break;
          case Phase::Active: a = lerpf(wind, end, 0.6f + 0.4f * smoothstep(s.k)); break;
          case Phase::Follow:
            a = lerpf(end, rest, smoothstep(s.k)) + 0.35f * std::sin(kPi * std::min(1.0f, s.k * 1.6f)) * (1.0f - s.k);
            break;
          case Phase::None: break;
        }
        p.weaponDir = {std::cos(a), std::sin(a)};
        p.handF = shoulder + p.weaponDir * (0.28f * H);
        break;
      }
      case Limb::None: break;
    }
  }

  // Body: coil away from the strike, then drive into it (stretched along it).
  const Vec2 target = hb->offset;
  const float dirX = target.x >= 0.0f ? 1.0f : -1.0f;
  float drive = 0.0f;
  switch (pr.phase) {
    case Phase::Strike: drive = pr.k; break;
    case Phase::Active: drive = 1.0f; break;
    case Phase::Follow: drive = 1.0f - smoothstep(pr.k); break;
    default: break;
  }
  p.lean -= 0.22f * pr.antic * dirX;
  p.crouch += 0.18f * pr.antic;
  p.squash *= 1.0f - 0.06f * pr.antic;
  p.sx *= 1.0f + 0.05f * pr.antic;
  if (std::fabs(target.x) > std::fabs(target.y)) {
    if (hb->limb != Limb::Weapon) p.lean += 0.15f * drive * dirX;
    p.sx *= 1.0f + 0.07f * drive;
    p.squash *= 1.0f - 0.04f * drive;
  } else if (target.y < 0.0f) {
    p.squash *= 1.0f + 0.07f * drive;
    p.sx *= 1.0f - 0.05f * drive;
  } else {
    p.squash *= 1.0f - 0.05f * drive;
    p.sx *= 1.0f + 0.04f * drive;
  }
  if (f.charging) {
    p.shift.x += std::sin(time * 60.0f) * 1.5f;
    p.crouch += 0.1f;
  }
  // Smear on the striking limb: starts with the whip, fades during the follow-through.
  p.trailLimb = hb->limb;
  p.trailFx = hb->fx;
  p.trailPos = target;
  p.trailRadius = hb->radius;
  if (pr.phase == Phase::Active) p.trail = 1.0f;
  else if (pr.phase == Phase::Strike) p.trail = 0.5f;
  else if (pr.phase == Phase::Follow) p.trail = clampf(1.0f - pr.k / 0.4f, 0.0f, 1.0f) * 0.9f;
}

// SUPER CLASH: gather (curled up), rise (arms raised), then a star burst on every shock wave.
Pose finalSmashPose(const Fighter& f, Pose p) {
  const float H = f.height();
  const float t = float(f.moveFrame);
  struct Key {
    Vec2 hF, hB, fF, fB;
    float crouch, lean, squash;
  };
  const Key gather{{0.10f, -0.02f}, {0.06f, 0.0f}, {0.12f, 0.36f}, {-0.06f, 0.4f}, 0.75f, 0.25f, 0.9f};
  const Key raise{{0.2f, -0.52f}, {-0.18f, -0.5f}, {0.06f, 0.48f}, {-0.05f, 0.5f}, 0.0f, -0.15f, 1.08f};
  const Key curl{{0.12f, -0.1f}, {0.03f, -0.06f}, {0.12f, 0.34f}, {-0.08f, 0.38f}, 0.55f, 0.15f, 0.92f};
  const Key star{{0.42f, -0.3f}, {-0.4f, -0.28f}, {0.22f, 0.46f}, {-0.2f, 0.46f}, 0.0f, -0.08f, 1.06f};
  const Key rest{{p.handF.x / H, p.handF.y / H}, {p.handB.x / H, p.handB.y / H}, {p.footF.x / H, p.footF.y / H},
                 {p.footB.x / H, p.footB.y / H}, p.crouch, p.lean, p.squash};
  auto mix = [](const Key& a, const Key& b, float k) {
    return Key{lerpv(a.hF, b.hF, k), lerpv(a.hB, b.hB, k), lerpv(a.fF, b.fF, k), lerpv(a.fB, b.fB, k),
               lerpf(a.crouch, b.crouch, k), lerpf(a.lean, b.lean, k), lerpf(a.squash, b.squash, k)};
  };
  Key k;
  if (t < 12.0f) {
    k = mix(rest, gather, easeOut(t / 12.0f));
  } else if (t < 28.0f) {
    k = mix(gather, raise, smoothstep((t - 12.0f) / 16.0f));
  } else if (t < 76.0f) {
    float pre = 0.0f, hit = 0.0f;
    for (int b : {34, 46, 60}) {
      if (t >= b - 5 && t < b) pre = std::max(pre, (t - (b - 5)) / 5.0f);
      if (t >= b && t < b + 12) hit = std::max(hit, 1.0f - (t - b) / 12.0f);
    }
    k = mix(mix(raise, curl, smoothstep(pre)), star, easeOut(hit * 1.4f));
  } else {
    k = mix(raise, rest, smoothstep((t - 76.0f) / 30.0f));
  }
  p.handF = k.hF * H;
  p.handB = k.hB * H;
  p.footF = k.fF * H;
  p.footB = k.fB * H;
  p.crouch = k.crouch;
  p.lean = k.lean;
  p.squash = k.squash;
  p.sx = 2.0f - k.squash;
  p.glow = 1.4f;
  return p;
}
}  // namespace

Pose restPose(const CharacterDef& d, float time) {
  const float H = d.height;
  Pose p;
  const float br = std::sin(time * 2.6f);  // breathing
  const float bob = br * 0.012f * H;
  p.lean = 0.06f + std::sin(time * 1.3f) * 0.015f;  // slow weight shift
  p.crouch = 0.05f + br * 0.03f;
  p.squash = 1.0f + br * 0.012f;
  p.sx = 1.0f - br * 0.008f;
  p.handF = {0.20f * H, -0.02f * H + bob};
  p.handB = {0.08f * H, 0.03f * H + bob};
  p.footF = {0.10f * H, 0.5f * H};
  p.footB = {-0.09f * H, 0.5f * H};
  auto stance = [&](Vec2 hF, Vec2 hB, float fF, float fB, float lean, float crouch) {
    p.handF = Vec2{hF.x * H, hF.y * H + bob};
    p.handB = Vec2{hB.x * H, hB.y * H + bob};
    p.footF.x = fF * H;
    p.footB.x = fB * H;
    p.lean += lean;
    p.crouch += crouch;
  };
  auto blade = [&](Vec2 dir) {
    p.weapon = true;
    p.weaponDir = rotate(dir.norm(), br * 0.03f);
  };
  switch (d.hero) {  // each hero's guard
    case Hero::Asta: stance({0.17f, 0.0f}, {-0.03f, 0.06f}, 0.14f, -0.13f, 0.06f, 0.12f); blade({1.0f, 0.62f}); break;
    case Hero::Yuno: stance({0.11f, 0.07f}, {-0.05f, 0.07f}, 0.08f, -0.07f, -0.02f, 0.0f); break;
    case Hero::Ichigo: stance({0.2f, 0.0f}, {0.03f, 0.07f}, 0.12f, -0.11f, 0.03f, 0.08f); blade({1.0f, 0.42f}); break;
    case Hero::Byakuya: stance({0.12f, 0.07f}, {-0.03f, 0.08f}, 0.08f, -0.07f, -0.06f, 0.0f); blade({0.4f, 1.0f}); break;
    case Hero::Link: stance({0.17f, 0.05f}, {0.17f, -0.07f}, 0.12f, -0.12f, 0.04f, 0.1f); blade({1.0f, 0.28f}); break;
    case Hero::Ganondorf: stance({0.17f, 0.05f}, {-0.12f, 0.05f}, 0.12f, -0.13f, 0.0f, 0.06f); break;
    case Hero::Kratos: stance({0.16f, 0.02f}, {0.03f, 0.05f}, 0.13f, -0.12f, 0.06f, 0.1f); break;
    case Hero::Gojo: stance({0.04f, 0.09f}, {-0.05f, 0.09f}, 0.07f, -0.07f, -0.1f, 0.0f); break;
    case Hero::Sukuna: stance({0.1f, 0.08f}, {-0.06f, 0.08f}, 0.1f, -0.09f, 0.04f, 0.04f); break;
    default: break;
  }
  float bl = std::fmod(time + d.height, 3.7f);
  p.blink = bl < 0.12f || (std::fmod(time * 0.37f + d.height, 3.0f) < 0.4f && bl > 0.3f && bl < 0.4f);  // sometimes twice
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
      p.handF.x -= 0.07f * H * std::sin(ph);
      p.handB.x += 0.07f * H * std::sin(ph);
      p.shift.y = -std::fabs(std::cos(ph)) * 0.015f * H;  // up when a foot passes under the body
      p.lean = 0.1f;
      break;
    }
    case FState::Run: {
      float ph = sf * 0.42f;
      float s1 = std::sin(ph), c1 = std::cos(ph);
      p.lean = 0.38f;
      p.footF = {0.21f * H * s1, 0.5f * H - std::max(0.0f, -c1) * 0.16f * H};
      p.footB = {-0.21f * H * s1, 0.5f * H - std::max(0.0f, c1) * 0.16f * H};
      // arms pump against the legs, elbows bent
      p.handF = {0.10f * H - 0.15f * H * s1, -0.06f * H - std::max(0.0f, -s1) * 0.06f * H};
      p.handB = {0.04f * H + 0.15f * H * s1, -0.02f * H - std::max(0.0f, s1) * 0.06f * H};
      float up = std::fabs(c1);  // a leg passes under the body: highest point
      p.shift.y = -up * 0.04f * H;
      p.squash = 0.97f + 0.05f * up;
      p.sx = 1.02f - 0.03f * up;
      if (sf < 8) {  // dash start: lunge
        float k = 1.0f - sf / 8.0f;
        p.lean += 0.2f * k;
        p.shift.x += 0.04f * H * k;
        p.squash -= 0.06f * k;
        p.sx += 0.06f * k;
      }
      break;
    }
    case FState::Crouch: {
      float k = easeOut((sf + 1.0f) / 5.0f);
      p.crouch = lerpf(0.5f, 1.0f, k);
      p.footF.x = 0.15f * H;
      p.footB.x = -0.14f * H;
      p.squash = 1.0f - 0.08f * p.crouch;
      p.sx = 1.0f + 0.05f * p.crouch;
      break;
    }
    case FState::JumpSquat: {  // anticipation: deeper and wider until the take-off
      float k = clampf((sf + 1.0f) / std::max(1, f.timer), 0.0f, 1.0f);
      p.crouch = 0.5f + 0.5f * k;
      p.footF.x = 0.15f * H;
      p.footB.x = -0.14f * H;
      p.squash = 1.0f - 0.14f * k;
      p.sx = 1.0f + 0.1f * k;
      p.handF = lerpv(p.handF, {0.12f * H, 0.12f * H}, k);
      p.handB = lerpv(p.handB, {-0.12f * H, 0.1f * H}, k);
      break;
    }
    case FState::Landing: {
      float k = 1.0f - sf / std::max(1, f.timer);
      float heavy = f.timer > 8 ? 1.0f : 0.7f;  // lag landings sink deeper
      p.crouch = heavy * k;
      p.footF.x = 0.15f * H;
      p.footB.x = -0.14f * H;
      p.squash = 1.0f - 0.12f * k * heavy;
      p.sx = 1.0f + 0.08f * k * heavy;
      p.handF = lerpv(p.handF, {0.24f * H, 0.08f * H}, k);
      p.handB = lerpv(p.handB, {-0.14f * H, 0.08f * H}, k);
      break;
    }
    case FState::Air: p = airPose(f, m, tt); break;
    case FState::AirDodge: {
      p = airPose(f, m, tt);
      float k = std::sin(clampf(sf / 20.0f, 0.0f, 1.0f) * kPi);  // curl up, then open again
      p.crouch = 0.1f + 0.6f * k;
      p.handF = lerpv(p.handF, {0.12f * H, 0.02f * H}, k);
      p.handB = lerpv(p.handB, {0.04f * H, 0.04f * H}, k);
      p.footF = lerpv(p.footF, {0.1f * H, 0.3f * H}, k);
      p.footB = lerpv(p.footB, {-0.04f * H, 0.34f * H}, k);
      if (f.actionDir == 1) p.lean += 0.25f * k;
      p.squash = 1.0f - 0.08f * k;
      p.sx = 1.0f + 0.05f * k;
      break;
    }
    case FState::Helpless: {  // limp, arms up, legs flailing slowly
      p = airPose(f, m, tt);
      float w = std::sin(tt * 7.0f);
      p.handF = {0.12f * H + w * 0.03f * H, -0.45f * H};
      p.handB = {-0.12f * H - w * 0.03f * H, -0.42f * H};
      p.footF.x += w * 0.04f * H;
      p.footB.x -= w * 0.04f * H;
      p.lean = -0.1f + std::sin(tt * 3.0f) * 0.08f;
      break;
    }
    case FState::Hitstun:
      p.hurt = true;
      if (f.tumble) {
        float w = std::sin(tt * 16.0f);
        p.rot = f.spin;
        p.crouch = 0.2f;
        p.handF = {0.25f * H, -0.3f * H + w * 0.05f * H};
        p.handB = {-0.25f * H, -0.25f * H - w * 0.05f * H};
        p.footF = {0.18f * H, 0.45f * H - w * 0.04f * H};
        p.footB = {-0.15f * H, 0.40f * H + w * 0.04f * H};
      } else {  // flinch: head snaps back, arms fly up, then it recovers
        float k = 1.0f - clampf(sf / 14.0f, 0.0f, 1.0f);
        p.lean = -0.35f - 0.15f * k;
        p.handF = {-0.02f * H, -0.18f * H - 0.06f * H * k};
        p.handB = {-0.14f * H, -0.10f * H - 0.04f * H * k};
        p.footF.x += 0.03f * H * k;
        p.squash = 1.0f - 0.06f * k;
        p.sx = 1.0f + 0.05f * k;
      }
      break;
    case FState::Knockdown: {
      float bounce = sf < 10 ? std::sin(sf / 10.0f * kPi) * 0.06f * H : 0.0f;  // bounces on the floor
      p.rot = -kPi * 0.5f;
      p.shift = {0.0f, 0.36f * H - bounce};
      p.footF = {0.12f * H, 0.5f * H};
      p.crouch = 0.1f + 0.04f * std::sin(tt * 2.0f);
      p.hurt = sf < 20;
      break;
    }
    case FState::GetUp: {
      float k = smoothstep(sf / std::max(1, f.timer));
      p.rot = -kPi * 0.5f * (1.0f - k);
      p.shift = {0.0f, 0.36f * H * (1.0f - k)};
      p.crouch = 0.6f * std::sin(k * kPi);
      break;
    }
    case FState::Roll:
      p.rot = (sf / 28.0f) * 2.0f * kPi * float(f.actionDir * f.facing);
      p.crouch = 1.0f;
      p.handF = {0.1f * H, 0.05f * H};
      p.handB = {0.05f * H, 0.08f * H};
      p.squash = 0.92f;
      break;
    case FState::SpotDodge: {
      float k = std::sin(clampf(sf / 18.0f, 0.0f, 1.0f) * kPi);
      p.lean = -0.25f * k;
      p.crouch = 0.5f * k + 0.05f;
      p.squash = 1.0f - 0.08f * k;
      p.sx = 1.0f - 0.08f * k;  // shrinks out of the way
      break;
    }
    case FState::Shield: case FState::ShieldStun: {  // braced, arms crossed in front
      p.crouch = 0.45f;
      p.lean = -0.05f;
      p.handF = {0.14f * H, -0.12f * H};
      p.handB = {0.16f * H, -0.04f * H};
      p.footF.x = 0.14f * H;
      p.footB.x = -0.14f * H;
      if (f.state == FState::ShieldStun) {
        float k = 1.0f - sf / std::max(1, f.timer);
        p.shift.x -= 0.05f * H * k;
        p.lean -= 0.12f * k;
      }
      break;
    }
    case FState::LedgeHang: {  // hangs on, legs swaying
      float sw = std::sin(tt * 2.2f);
      p.handF = {0.18f * H, -0.62f * H};
      p.handB = {0.12f * H, -0.60f * H};
      p.footF = {0.03f * H + sw * 0.04f * H, 0.5f * H};
      p.footB = {-0.04f * H - sw * 0.04f * H, 0.47f * H - std::max(0.0f, sw) * 0.04f * H};
      p.lean = 0.1f + sw * 0.03f;
      if (sf < 8) p.squash = 1.0f + 0.08f * (1.0f - sf / 8.0f);  // caught it: stretched
      break;
    }
    case FState::LedgeClimb: {
      float k = clampf(sf / 18.0f, 0.0f, 1.0f);
      p.crouch = 0.7f * std::sin(k * kPi) + 0.1f;
      p.handF = lerpv({0.2f * H, -0.3f * H}, {0.2f * H, 0.1f * H}, k);
      p.footB = lerpv({-0.04f * H, 0.3f * H}, p.footB, k);
      break;
    }
    case FState::Holding:
      p.handF = {0.32f * H, -0.05f * H};
      p.handB = {0.28f * H, 0.02f * H};
      p.lean = 0.15f + std::sin(tt * 4.0f) * 0.02f;
      break;
    case FState::Grabbed: {
      float w = std::sin(tt * 25.0f);
      p.hurt = true;
      p.lean = -0.2f + w * 0.04f;
      p.handF = {0.1f * H + w * 0.06f * H, -0.25f * H};
      p.handB = {-0.1f * H, -0.2f * H + std::cos(tt * 25.0f) * 0.06f * H};
      p.footF = {0.06f * H, 0.45f * H};
      p.footB = {-0.09f * H - w * 0.03f * H, 0.47f * H};
      p.squash = 1.0f + 0.03f * w;
      break;
    }
    case FState::ShieldBreak:
      p.hurt = true;
      p.lean = std::sin(tt * 4.0f) * 0.25f;
      p.handF = {0.08f * H, 0.15f * H};
      p.handB = {-0.05f * H, 0.15f * H};
      p.crouch = 0.25f + 0.1f * std::sin(tt * 8.0f);
      break;
    case FState::Attack:
      if (f.move == FinalSmash) {
        p = finalSmashPose(f, f.grounded ? p : airPose(f, m, tt, false));
        break;
      }
      if (!f.grounded) p = airPose(f, m, tt, false);
      applyMove(p, f, tt);
      break;
    default:
      break;
  }
  if (const WeaponDef* w = f.weaponDef()) {  // picked-up weapon: held all the time
    p.held = w->visual;
    p.heldFx = w->fx;
    if (isHandWeapon(w->visual) && !p.weapon) {
      p.weapon = true;
      p.weaponDir = Vec2{1.0f, 0.5f}.norm();
    }
  }
  return p;
}

namespace {
// ---- Animation memory ---------------------------------------------------------
// Per fighter: the pose shown last frame. On a state change the new pose is reached through
// critically damped offsets (inertial blending); strikes snap exactly on their hitboxes.
constexpr int kChan = 16;
constexpr int kTrail = 7;

void packPose(const Pose& p, float* v) {
  const float c[kChan] = {p.rot, p.shift.x, p.shift.y, p.lean, p.crouch, p.squash, p.sx, p.handF.x, p.handF.y,
                          p.handB.x, p.handB.y, p.footF.x, p.footF.y, p.footB.x, p.footB.y,
                          std::atan2(p.weaponDir.y, p.weaponDir.x)};
  std::copy(c, c + kChan, v);
}
void unpackPose(Pose& p, const float* v) {
  p.rot = v[0];
  p.shift = {v[1], v[2]};
  p.lean = v[3];
  p.crouch = v[4];
  p.squash = v[5];
  p.sx = v[6];
  p.handF = {v[7], v[8]};
  p.handB = {v[9], v[10]};
  p.footF = {v[11], v[12]};
  p.footB = {v[13], v[14]};
  p.weaponDir = {std::cos(v[15]), std::sin(v[15])};
}
bool isAngle(int i) { return i == 0 || i == 15; }
// Per-frame jump above which a channel counts as a discontinuity to blend over.
float jumpLimit(int i, float H) {
  if (isAngle(i)) return 0.9f;
  if (i == 3 || i == 4) return 0.25f;
  if (i == 5 || i == 6) return 0.1f;
  return 0.1f * H;
}
// Critically damped spring pulling an offset back to zero (exact for one frame).
void decay(float& x, float& v, float omega) {
  float j = v + x * omega;
  float e = std::exp(-omega);
  x = e * (x + j);
  v = e * (v - j * omega);
}

struct Ghost {
  Pose pose;
  Vec2 feet;
  int facing = 1;
  int life = 0;
  Color tint = WHITE;
};
struct Anim {
  int frame = -1;
  FState state = FState::Idle;
  int move = -1, facing = 1;
  float clock = 0.0f;  // animation time: stops during hit-stop and the final smash freeze
  float tgt[kChan] = {}, shown[kChan] = {}, vel[kChan] = {}, off[kChan] = {}, offV[kChan] = {};
  float omega = 0.4f;
  float jig = 0.0f, jigV = 0.0f;  // squash & stretch spring (a little jelly)
  Vec2 lastPos, speed;
  Vec2 flow, flowV, tip, tipV;  // cloth / tail drag
  Vec2 trailA[kTrail], trailB[kTrail];
  int trailN = 0, trailMove = -1;
  Limb trailLimb = Limb::None;
  Ghost ghosts[4];
  int ghostNext = 0;
  Pose pose;  // final pose of the frame
};
Anim g_anim[4];
const Match* g_animMatch = nullptr;

void resetAnim(Anim& a, const Fighter& f, const Match& m) {
  a = Anim{};
  a.frame = m.frame;
  a.state = f.state;
  a.move = f.move;
  a.facing = f.facing;
  a.clock = f.id * 0.37f;
  a.lastPos = f.pos;
  a.pose = computePose(f, m, a.clock);
  packPose(a.pose, a.tgt);
  std::copy(a.tgt, a.tgt + kChan, a.shown);
}

void stepAnim(Anim& a, const Fighter& f, const Match& m) {
  a.frame = m.frame;
  const float H = f.height();
  Vec2 v = f.pos - a.lastPos;
  a.lastPos = f.pos;
  if (v.len() > 60.0f) v = {};  // teleport, respawn, training reset
  const bool frozen = f.hitlag > 0 || (m.freezeTimer > 0 && m.freezeOwner != f.id);
  const bool changed = f.state != a.state || f.move != a.move;
  if (frozen && !changed) return;  // hit-stop: hold the pose
  if (!frozen) a.clock += 1.0f / 60.0f;
  Pose p = computePose(f, m, a.clock);
  float tv[kChan];
  packPose(p, tv);

  if (changed) {
    bool snap = false;
    a.omega = 0.38f;
    switch (f.state) {
      case FState::Hitstun: case FState::Grabbed: case FState::ShieldBreak: case FState::Respawn: snap = true; break;
      case FState::Attack: a.omega = 0.9f; break;
      case FState::Landing: case FState::JumpSquat: a.omega = 0.7f; break;
      case FState::Run: a.omega = 0.6f; break;
      case FState::Air: a.omega = a.state == FState::JumpSquat ? 0.55f : 0.3f; break;
      default: break;
    }
    for (int i = 0; i < kChan; ++i) {
      float d = a.shown[i] - tv[i];
      a.off[i] = snap ? 0.0f : isAngle(i) ? wrapPi(d) : d;
      a.offV[i] = snap ? 0.0f : a.vel[i];
    }
    // squash & stretch kicks
    if (f.state == FState::Air && a.state == FState::JumpSquat) a.jig += 0.16f;
    if (f.state == FState::Landing) a.jig -= std::min(0.24f, 0.06f + std::max(0.0f, a.speed.y) * 0.016f);
    if (f.state == FState::Hitstun) {
      a.jig = -std::min(0.24f, 0.1f + f.kbVel.len() * 0.006f);
      a.jigV = 0.0f;
    }
    if (f.state == FState::Run && a.state != FState::Run) a.jig -= 0.08f;
    if (f.state == FState::ShieldStun) a.jig -= 0.06f;
  } else if (p.trail <= 0.0f) {  // jumps inside a state are blended over too
    for (int i = 0; i < kChan; ++i) {
      float d = tv[i] - a.tgt[i];
      if (isAngle(i)) d = wrapPi(d);
      if (std::fabs(d) > jumpLimit(i, H)) a.off[i] = isAngle(i) ? wrapPi(a.off[i] - d) : a.off[i] - d;
    }
  }
  if (m.frame - f.airJumpFrame == 1) a.jig += 0.12f;
  if (f.facing != a.facing) {  // turn around: thin for an instant; the cloth swings through
    if (f.grounded && (f.state == FState::Idle || f.state == FState::Walk || f.state == FState::Run)) a.jig += 0.1f;
    a.flow.x = -a.flow.x;
    a.flowV.x = -a.flowV.x;
    a.tip.x = -a.tip.x;
    a.tipV.x = -a.tipV.x;
  }
  if (p.trail >= 1.0f)  // strikes stay exactly on their hitboxes
    for (int i = 0; i < kChan; ++i) a.off[i] = a.offV[i] = 0.0f;
  for (int i = 0; i < kChan; ++i) {
    if (!frozen) decay(a.off[i], a.offV[i], a.omega);
    float prev = a.shown[i];
    a.shown[i] = tv[i] + a.off[i];
    float dv = a.shown[i] - prev;
    if (isAngle(i)) dv = wrapPi(dv);
    a.vel[i] = clampf(dv, -jumpLimit(i, H), jumpLimit(i, H));
    a.tgt[i] = tv[i];
  }
  if (!frozen) {
    a.jigV = (a.jigV - a.jig * 0.3f) * 0.7f;
    a.jig += a.jigV;
    Vec2 target = Vec2{v.x * f.facing, v.y} * -0.075f;  // cloth trails behind the motion
    if (target.len() > 1.4f) target = target.norm() * 1.4f;
    a.flowV = (a.flowV + (target - a.flow) * 0.16f) * 0.8f;
    a.flow += a.flowV;
    a.tipV = (a.tipV + (a.flow - a.tip) * 0.2f) * 0.76f;
    a.tip += a.tipV;
    a.speed = v;
  }
  a.state = f.state;
  a.move = f.move;
  a.facing = f.facing;

  unpackPose(p, a.shown);
  p.squash *= 1.0f + a.jig;
  p.sx *= 1.0f - 0.6f * a.jig;
  p.flow = a.flow;
  p.flowTip = a.tip;
  a.pose = p;

  // Path of the striking limb (world space) for the smear.
  const bool attacking = f.state == FState::Attack;
  if (!attacking || f.move != a.trailMove || p.trailLimb != a.trailLimb) {
    a.trailN = 0;
    a.trailMove = attacking ? f.move : -1;
    a.trailLimb = p.trailLimb;
  }
  if (attacking && p.trailLimb != Limb::None && p.trailLimb != Limb::Body) {
    const bool blade = (p.trailLimb == Limb::Weapon || (p.trailLimb == Limb::ArmFront && isHandWeapon(p.held))) &&
                       frontWeaponLength(*f.def, p) > 0.0f;
    for (int i = std::min(a.trailN, kTrail - 1); i > 0; --i) {
      a.trailA[i] = a.trailA[i - 1];
      a.trailB[i] = a.trailB[i - 1];
    }
    a.trailA[0] = limbWorld(*f.def, p, f.pos, f.facing, 1.0f, p.trailLimb);
    a.trailB[0] = blade ? limbWorld(*f.def, p, f.pos, f.facing, 1.0f, p.trailLimb, true) : a.trailA[0];
    a.trailN = std::min(a.trailN + 1, kTrail);
  }

  // Afterimages: dash, dodges, very fast moves and the final smash.
  for (auto& g : a.ghosts)
    if (g.life > 0) --g.life;
  const bool fs = attacking && f.move == FinalSmash;
  const bool dash = f.state == FState::Run && f.stateFrame < 12;
  const bool dodge = (f.state == FState::AirDodge && f.stateFrame < 20) ||
                     (f.state == FState::Roll && f.stateFrame >= 3 && f.stateFrame < 20);
  const bool fast = attacking && v.len() > 11.0f;
  if ((dash || dodge || fast || fs) && m.frame % 3 == 0) {
    Ghost& g = a.ghosts[a.ghostNext];
    a.ghostNext = (a.ghostNext + 1) % 4;
    g.pose = p;
    g.pose.ghost = true;
    g.feet = f.pos;
    g.facing = f.facing;
    g.life = 12;
    g.tint = fs ? kZonaiGlow : dodge ? Color{170, 225, 255, 255} : mixColor(slotColor(f), WHITE, 0.45f);
  }
}

Anim& animFor(const Fighter& f, const Match& m) {
  if (g_animMatch != &m) {
    g_animMatch = &m;
    for (auto& a : g_anim) a.frame = -1;
  }
  Anim& a = g_anim[f.id & 3];
  if (a.frame < 0 || m.frame < a.frame || m.frame > a.frame + 20) resetAnim(a, f, m);
  else if (m.frame > a.frame) stepAnim(a, f, m);
  return a;
}

// ---- Effects around the fighter ------------------------------------------------
Vec2 catmull(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float t) {
  float t2 = t * t, t3 = t2 * t;
  return (p1 * 2.0f + (p2 - p0) * t + (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2 +
          (p1 * 3.0f - p0 - p2 * 3.0f + p3) * t3) * 0.5f;
}
// Smear: a tapered ribbon along the path of the striking limb, a fan for blades.
void drawSmear(const Anim& a, const Pose& pose, float alpha) {
  if (pose.trail <= 0.0f || a.trailN < 3) return;
  const int n = a.trailN;
  Vec2 A[kTrail], B[kTrail];  // oldest -> newest
  for (int i = 0; i < n; ++i) {
    A[i] = a.trailA[n - 1 - i];
    B[i] = a.trailB[n - 1 - i];
  }
  float len = 0.0f;
  for (int i = 1; i < n; ++i) len += std::max((A[i] - A[i - 1]).len(), (B[i] - B[i - 1]).len());
  if (len < 10.0f) return;
  constexpr int kSub = 3;
  constexpr int kMax = (kTrail - 1) * kSub + 1;
  Vec2 SA[kMax], SB[kMax];
  int ns = 0;
  for (int i = 0; i + 1 < n; ++i)
    for (int j = 0; j < kSub; ++j) {
      float t = float(j) / kSub;
      int i0 = std::max(0, i - 1), i3 = std::min(n - 1, i + 2);
      SA[ns] = catmull(A[i0], A[i], A[i + 1], A[i3], t);
      SB[ns] = catmull(B[i0], B[i], B[i + 1], B[i3], t);
      ++ns;
    }
  SA[ns] = A[n - 1];
  SB[ns] = B[n - 1];
  ++ns;
  const Color col = smearColor(pose.trailFx);
  const Color hot = mixColor(col, WHITE, 0.55f);
  const float k0 = pose.trail * alpha;
  const bool blade = (B[n - 1] - A[n - 1]).len() > 4.0f;
  for (int i = 0; i + 1 < ns; ++i) {
    float u0 = float(i) / (ns - 1), u1 = float(i + 1) / (ns - 1);  // 0 = oldest
    if (blade) {  // crescent: bright along the tip path, fading towards the hilt
      Vec2 i0 = lerpv(SA[i], SB[i], 0.3f), i1 = lerpv(SA[i + 1], SB[i + 1], 0.3f);
      gradQuad(i0, withAlpha(col, 0.0f), SB[i], withAlpha(hot, 0.75f * k0 * u0), SB[i + 1],
               withAlpha(hot, 0.75f * k0 * u1), i1, withAlpha(col, 0.0f));
      DrawLineEx(V(SB[i]), V(SB[i + 1]), 1.0f + 2.5f * u1, withAlpha(WHITE, 0.9f * k0 * u1));
    } else {  // soft streak: bright core, transparent edges, tapering into the past
      Vec2 d0 = (SA[std::min(i + 1, ns - 1)] - SA[std::max(i - 1, 0)]).norm();
      Vec2 d1 = (SA[std::min(i + 2, ns - 1)] - SA[i]).norm();
      Vec2 n0{-d0.y, d0.x}, n1{-d1.y, d1.x};
      float w0 = pose.trailRadius * 0.7f * u0, w1 = pose.trailRadius * 0.7f * u1;
      Color c0 = withAlpha(hot, 0.85f * k0 * u0), c1 = withAlpha(hot, 0.85f * k0 * u1), e = withAlpha(col, 0.0f);
      gradQuad(SA[i], c0, SA[i] + n0 * w0, e, SA[i + 1] + n1 * w1, e, SA[i + 1], c1);
      gradQuad(SA[i], c0, SA[i] - n0 * w0, e, SA[i + 1] - n1 * w1, e, SA[i + 1], c1);
    }
  }
}

void drawGroundShadow(const Fighter& f, const Match& m, float alpha) {
  if (f.state == FState::LedgeHang || f.state == FState::Respawn) return;
  float ground = 1e9f;
  for (const auto& p : m.platforms) {
    if (f.pos.x < p.rect.left() - 4.0f || f.pos.x > p.rect.right() + 4.0f || p.rect.top() < f.pos.y - 4.0f) continue;
    ground = std::min(ground, p.rect.top());
  }
  float h = ground - f.pos.y;
  if (h > 480.0f) return;
  float k = 1.0f - std::max(0.0f, h) / 480.0f;
  DrawEllipse(int(f.pos.x), int(ground), f.width() * (0.5f + 0.4f * k), 4.0f + 3.0f * k,
              withAlpha({44, 24, 18, 255}, 0.32f * k * alpha));
}

// Ultimate ready: soft teal glow, a turning hexagon and a tilted rune ring orbiting the
// body (its front half passes in front of the fighter).
void drawUltAura(Vec2 c, float H, float t, bool front, float k) {
  const Color z = kZonaiGlow;
  if (!front) {
    additive(true);
    DrawCircleGradient(int(c.x), int(c.y), 0.95f * H, withAlpha(z, (0.26f + 0.08f * std::sin(t * 5.0f)) * k),
                       withAlpha(z, 0.0f));
    additive(false);
    DrawPolyLinesEx(V(c), 6, 0.84f * H, t * 18.0f, 2.0f, withAlpha(z, 0.24f * k));
    DrawPolyLinesEx(V(c), 6, 0.72f * H, -t * 26.0f + 30.0f, 1.5f, withAlpha(z, 0.14f * k));
  }
  const float rx = 0.66f * H, ry = 0.17f * H, tilt = -0.2f;
  const Vec2 oc = c + Vec2{0.0f, 0.06f * H};
  auto at = [&](float ang) { return oc + rotate({std::cos(ang) * rx, std::sin(ang) * ry}, tilt); };
  const int n = 32;
  for (int i = 0; i < n; ++i) {
    float a0 = i * 2.0f * kPi / n, a1 = (i + 1) * 2.0f * kPi / n;
    if ((std::sin((a0 + a1) * 0.5f) > 0.0f) != front) continue;
    Vec2 p0 = at(a0), p1 = at(a1);
    DrawLineEx(V(p0), V(p1), 6.0f, withAlpha(z, 0.16f * k));
    DrawLineEx(V(p0), V(p1), 2.0f, withAlpha(mixColor(z, WHITE, 0.35f), (front ? 0.8f : 0.45f) * k));
  }
  for (int i = 0; i < 6; ++i) {  // rune glyphs riding the ring
    float ang = t * 1.3f + i * kPi / 3.0f;
    if ((std::sin(ang) > 0.0f) != front) continue;
    Vec2 p = at(ang);
    float sz = (i % 2 ? 4.5f : 3.4f) * (0.75f + 0.25f * std::sin(ang));
    DrawCircleV(V(p), sz * 2.2f, withAlpha(z, 0.22f * k));
    DrawPoly(V(p), i % 2 ? 4 : 3, sz, ang * 40.0f, withAlpha(mixColor(z, WHITE, 0.5f), (front ? 1.0f : 0.6f) * k));
  }
}

// SUPER CLASH: light pillar, magic circle and converging energy behind; core and shock waves in front.
void drawFinalSmashFx(const Fighter& f, Vec2 c, float t, bool front) {
  const MoveDef& mv = f.def->moves[FinalSmash];
  const float H = f.height();
  const int ft = f.moveFrame;
  const Color z = kZonaiGlow;
  const Color sig = fxColor(mv.hitboxes.empty() ? HitFx::Magic : mv.hitboxes[0].fx);
  if (!front) {
    float pil = clampf(ft / 8.0f, 0.0f, 1.0f) * clampf((40 - ft) / 8.0f, 0.0f, 1.0f);
    additive(true);
    if (pil > 0.0f) {  // light pillar: soft edges, brightest on the fighter, fading out under the feet
      const float top = c.y - 1600.0f, bot = c.y + 0.7f * H;
      auto beam = [&](float w, Color col, float a) {
        const Color e = withAlpha(col, 0.0f), up = withAlpha(col, 0.25f * a), mid = withAlpha(col, a);
        for (float s : {-1.0f, 1.0f}) {
          const float x = c.x + s * w;
          gradQuad({x, top}, e, {x, c.y}, e, {c.x, c.y}, mid, {c.x, top}, up);
          gradQuad({x, c.y}, e, {x, bot}, e, {c.x, bot}, e, {c.x, c.y}, mid);
        }
      };
      beam(H * (0.3f + 0.25f * pil), z, 0.32f * pil);
      beam(H * (0.1f + 0.08f * pil), WHITE, 0.3f * pil);
    }
    float core = ft < 34 ? ft / 34.0f : clampf(1.0f - (ft - 60) / 20.0f, 0.0f, 1.0f);
    if (core > 0.0f)
      DrawCircleGradient(int(c.x), int(c.y), H * (0.45f + 0.35f * core), withAlpha(mixColor(z, WHITE, 0.5f), 0.5f * core),
                         withAlpha(z, 0.0f));
    for (const auto& h : mv.hitboxes) {  // light rays of each shock wave
      int tt = ft - h.start + 4;
      if (tt < 0 || tt > 10) continue;
      float rk = 1.0f - tt / 10.0f;
      float r = h.radius * easeOut(tt / 12.0f);
      for (int i = 0; i < 12; ++i) {
        float ang = i * kPi / 6.0f + h.start * 0.3f;
        Vec2 dir{std::cos(ang), std::sin(ang)}, perp{-dir.y, dir.x};
        Vec2 p0 = c + dir * (r * 0.2f);
        tri(p0 + perp * 7.0f, p0 - perp * 7.0f, c + dir * (r * 1.2f), withAlpha(mixColor(z, WHITE, 0.6f), 0.4f * rk));
      }
    }
    additive(false);
    for (const auto& h : mv.hitboxes) {  // shock waves
      int tt = ft - h.start + 4;
      if (tt < 0 || tt > 24) continue;
      float r = h.radius * easeOut(tt / 12.0f);
      float a = 1.0f - tt / 24.0f;
      DrawCircleV(V(c), r, withAlpha(z, 0.1f * a));
      DrawRing(V(c), std::max(0.0f, r - 20.0f), r, 0, 360, 72, withAlpha(sig, 0.5f * a));
    }
    float grow = easeOut(ft / 26.0f) * clampf((96 - ft) / 20.0f, 0.0f, 1.0f);
    if (grow > 0.0f) {  // magic circle
      float R = H * 1.25f * grow;
      DrawRing(V(c), R - 3.0f, R, 0, 360, 64, withAlpha(z, 0.65f * grow));
      DrawRing(V(c), R * 0.84f - 1.5f, R * 0.84f, 0, 360, 64, withAlpha(z, 0.4f * grow));
      DrawPolyLinesEx(V(c), 6, R * 0.82f, t * 40.0f, 2.5f, withAlpha(mixColor(z, WHITE, 0.3f), 0.55f * grow));
      DrawPolyLinesEx(V(c), 3, R * 0.8f, -t * 30.0f, 2.0f, withAlpha(sig, 0.45f * grow));
      DrawPolyLinesEx(V(c), 3, R * 0.8f, -t * 30.0f + 60.0f, 2.0f, withAlpha(sig, 0.45f * grow));
      for (int i = 0; i < 12; ++i) {
        float a = t * 0.8f + i * kPi / 6.0f;
        Vec2 dir{std::cos(a), std::sin(a)};
        DrawLineEx(V(c + dir * (R * 0.87f)), V(c + dir * (R * 0.97f)), i % 3 ? 2.0f : 4.0f, withAlpha(z, 0.75f * grow));
      }
    }
    if (ft < 34) {  // energy converging on the fighter
      for (int i = 0; i < 12; ++i) {
        float a = i * kPi / 6.0f + std::sin(i * 3.1f) * 0.3f;
        float u = std::fmod(ft / 17.0f + i * 0.37f, 1.0f);
        float rr = H * (2.0f - 1.6f * u);
        Vec2 dir{std::cos(a), std::sin(a)};
        DrawLineEx(V(c + dir * rr), V(c + dir * (rr * 0.82f)), 3.0f, withAlpha(i % 3 ? z : sig, 0.8f * u));
      }
    }
    return;
  }
  for (const auto& h : mv.hitboxes) {  // bright fronts of the shock waves, over everything
    int tt = ft - h.start + 4;
    if (tt < 0 || tt > 24) continue;
    float r = h.radius * easeOut(tt / 12.0f);
    float a = 1.0f - tt / 24.0f;
    if (r < 0.6f * H) continue;  // still inside the body: drawn behind only
    DrawRing(V(c), std::max(0.0f, r - 8.0f), r, 0, 360, 72, withAlpha(mixColor(z, WHITE, 0.45f), 0.9f * a));
    DrawPolyLinesEx(V(c), 6, r * 1.07f, tt * 4.0f + h.start * 10.0f, 3.0f, withAlpha(z, 0.6f * a));
  }
}
}  // namespace

namespace {
// Height of a fighter's name tag; tags of fighters standing close together stack up.
float tagY(const Match& m, int id) {
  float ys[8];
  for (int i = 0; i <= id && i < int(m.fighters.size()) && i < 8; ++i) {
    const Fighter& o = m.fighters[i];
    float y = o.state == FState::Knockdown ? o.pos.y - o.height() * 0.5f - 30.0f : o.pos.y - o.height() - 36.0f;
    for (bool moved = true; moved;) {
      moved = false;
      for (int j = 0; j < i; ++j) {
        const Fighter& p = m.fighters[j];
        if (!p.active() || p.state == FState::Grabbed) continue;
        if (std::fabs(p.pos.x - o.pos.x) < 56.0f && std::fabs(ys[j] - y) < 22.0f) {
          y = ys[j] - 24.0f;
          moved = true;
        }
      }
    }
    ys[i] = y;
  }
  return ys[std::min(id, 7)];
}
}  // namespace

void drawFighterTags(const Match& m) {
  if (!settings().playerTags || m.freezeTimer > 0) return;  // no tags during an ultimate cinematic
  for (const auto& f : m.fighters) {
    if (!f.active() || f.state == FState::Grabbed) continue;
    Color pc = slotColor(f);
    char tag[8];
    std::snprintf(tag, sizeof tag, f.cpu ? "CPU" : "J%d", f.port + 1);
    float ty = tagY(m, f.id);
    DrawTriangle({f.pos.x - 8, ty + 20}, {f.pos.x, ty + 29}, {f.pos.x + 8, ty + 20}, pc);
    drawTextCentered(tag, f.pos.x, ty, 20, pc, true, FontKind::Title);
  }
}

void drawFighter(const Fighter& f, const Match& m, float time, bool debugBoxes) {
  if (!f.active()) return;
  const CharacterDef& d = *f.def;
  const float H = d.height;
  const Anim& anim = animFor(f, m);
  const float at = anim.clock + f.id;  // animation time (stops in hit-stop)
  Pose pose = anim.pose;
  const bool usingFs = f.state == FState::Attack && f.move == FinalSmash;
  if (f.finalSmash) pose.glow = std::max(pose.glow, 0.8f + 0.3f * std::sin(at * 5.0f));

  float alpha = 1.0f;
  if (f.intangible > 0 && f.state != FState::LedgeHang && !usingFs) alpha = 0.6f;
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
  if (f.finalSmash || usingFs) {  // ancient power: teal pulse
    flashCol = mixColor(kZonaiGlow, WHITE, 0.3f);
    flash = std::max(flash, (usingFs ? 0.1f : 0.05f) + 0.04f * std::sin(at * 6.0f));
  }
  Vec2 feet = f.pos;
  if (f.hitlag > 0 && f.state == FState::Hitstun) feet.x += std::sin(m.frame * 2.7f) * 4.0f;
  const Vec2 c = f.center();

  drawGroundShadow(f, m, alpha);
  if (f.state == FState::Respawn) {
    Color pc = slotColor(f);
    DrawEllipse(int(feet.x), int(feet.y + 6), 60, 12, withAlpha(pc, 0.35f));
    DrawEllipseLines(int(feet.x), int(feet.y + 6), 60, 12, withAlpha(WHITE, 0.8f));
    for (int i = 0; i < 3; ++i) {
      float a = time * 2.0f + i * 2.1f;
      DrawCircleV({feet.x + std::cos(a) * 55.0f, feet.y + 6 + std::sin(a) * 10.0f}, 4, withAlpha(WHITE, 0.9f));
    }
  }
  for (int i = 0; i < 4; ++i) {  // afterimages, oldest first
    const Ghost& g = anim.ghosts[(anim.ghostNext + i) % 4];
    if (g.life > 0) drawCharacter(d, f.palette, g.pose, g.feet, g.facing, 1.0f, 0.34f * g.life / 12.0f, g.tint, 1.0f, at);
  }
  const float ultK = f.finalSmash ? alpha : 0.0f;
  if (ultK > 0.0f) drawUltAura(c, H, at, false, ultK);
  if (usingFs) drawFinalSmashFx(f, c, at, false);

  drawCharacter(d, f.palette, pose, feet, f.facing, 1.0f, alpha, flashCol, flash, at);

  drawSmear(anim, pose, alpha);
  if (ultK > 0.0f) drawUltAura(c, H, at, true, ultK);
  if (usingFs) drawFinalSmashFx(f, c, at, true);
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
      float a = at * 5.0f + i * 2.094f;
      Vec2 sp{c.x + std::cos(a) * H * 0.3f, c.y - H * 0.55f + std::sin(a) * H * 0.08f};
      DrawPoly(V(sp), 5, 7, a * 50.0f, {255, 230, 80, 255});
    }
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
