#include "game/fx.hpp"

#include "game/render.hpp"
#include "game/ui.hpp"

namespace {
Color fxColor(HitFx fx) {
  switch (fx) {
    case HitFx::Fire: return {255, 140, 40, 255};
    case HitFx::Electric: return {110, 225, 255, 255};
    case HitFx::Slash: return {200, 225, 255, 255};
    case HitFx::Dark: return {170, 90, 255, 255};
    case HitFx::Magic: return {255, 130, 220, 255};
    default: return {255, 230, 120, 255};
  }
}
}  // namespace

void Fx::add(PK kind, Vec2 pos, Vec2 vel, float life, float size, float size1, Color c, float gravity, float drag) {
  if (parts_.size() > 1500) return;
  Particle p;
  p.kind = kind;
  p.pos = pos;
  p.vel = vel;
  p.life = p.maxLife = life;
  p.size = size;
  p.size1 = size1;
  p.color = c;
  p.gravity = gravity;
  p.rot = rng_.range(0, 360);
  p.rotVel = kind == PK::Hex ? rng_.range(-1.5f, 1.5f) : rng_.range(-12, 12);
  p.drag = drag;
  parts_.push_back(p);
}

void Fx::burst(Vec2 pos, int count, float speed, Color c, PK kind, float life, float size) {
  for (int i = 0; i < count; ++i) {
    float a = rng_.range(0, 2 * kPi);
    float s = speed * rng_.range(0.4f, 1.0f);
    add(kind, pos, {std::cos(a) * s, std::sin(a) * s}, life * rng_.range(0.7f, 1.0f), size, size * 0.2f, c,
        kind == PK::Shard ? 0.35f : 0.0f);
  }
}

void Fx::dust(Vec2 feet, int count, float spread) {
  for (int i = 0; i < count; ++i) {
    float dir = (i % 2 == 0) ? 1.0f : -1.0f;
    add(PK::Smoke, feet + Vec2{rng_.range(-8, 8), -4}, {dir * rng_.range(1.0f, spread), rng_.range(-1.2f, -0.2f)},
        rng_.range(18, 28), rng_.range(5, 8), rng_.range(12, 18), {235, 225, 210, 200});
  }
}

void Fx::motes(Vec2 pos, int count, float radius, float speed, float rise) {
  for (int i = 0; i < count; ++i) {
    float a = rng_.range(0, 2 * kPi);
    Vec2 dir{std::cos(a), std::sin(a)};
    add(PK::Mote, pos + dir * rng_.range(0.0f, radius), dir * (speed * rng_.range(0.35f, 1.0f)) + Vec2{0.0f, -rise},
        rng_.range(30, 55), rng_.range(2.0f, 3.5f), 0.6f, i % 4 ? kZonaiGlow : mixColor(kZonaiGlow, WHITE, 0.6f),
        -0.03f, 0.94f);
  }
}

void Fx::onEvents(const Match& m) {
  Audio& au = *audio_;
  for (const auto& e : m.events) {
    switch (e.type) {
      case EvType::Hit: {
        float kb = e.strength;
        Color c = fxColor(e.fx);
        Vec2 pos = e.pos;
        if (e.b >= 0 && e.b < int(m.fighters.size())) {  // huge hitboxes (final smash): sparks on the victim
          const Fighter& v = m.fighters[e.b];
          Vec2 d = pos - v.center();
          if (d.len() > v.height() * 0.6f) pos = v.center() + d.norm() * (v.height() * 0.3f);
        }
        int n = 6 + int(kb / 9.0f);
        for (int i = 0; i < n; ++i) {
          float a = rng_.range(0, 2 * kPi);
          Vec2 v = Vec2{std::cos(a), std::sin(a)} * rng_.range(4.0f, 9.0f + kb * 0.06f) + e.dir * 3.0f;
          add(PK::Spark, pos, v, rng_.range(10, 18), rng_.range(2.0f, 4.0f), 0.0f, i % 3 ? c : WHITE);
        }
        add(PK::Ring, pos, {}, 14, 8, 30 + kb * 0.4f, withAlpha(c, 0.9f));
        if (kb > 100.0f) {
          add(PK::Star, pos, {}, 16, std::min(30.0f + kb * 0.1f, 44.0f), std::min(50.0f + kb * 0.2f, 76.0f), WHITE);
          flash_ = std::max(flash_, 0.18f);
          flashCol_ = WHITE;
        }
        if (e.fx == HitFx::Fire)
          for (int i = 0; i < 6; ++i)
            add(PK::Smoke, pos, {rng_.range(-2, 2), rng_.range(-3, -1)}, 24, 6, 16, {255, 120, 40, 200});
        shake_ = std::max(shake_, std::min(18.0f, kb / 11.0f));
        if (e.b >= 0) hudShake[e.b & 3] = std::min(9.0f, 2.0f + kb / 14.0f);
        Sfx s = kb < 40 ? Sfx::HitLight : kb < 100 ? Sfx::HitMedium : Sfx::HitHeavy;
        if (e.fx == HitFx::Fire) au.play(Sfx::HitFire, 0.7f, rng_.range(0.9f, 1.1f));
        if (e.fx == HitFx::Electric) au.play(Sfx::HitElectric, 0.7f, rng_.range(0.9f, 1.1f));
        if (e.fx == HitFx::Slash) au.play(Sfx::HitSlash, 0.8f, rng_.range(0.9f, 1.1f));
        au.play(s, clampf(0.5f + kb / 150.0f, 0.5f, 1.0f), rng_.range(0.92f, 1.08f));
        break;
      }
      case EvType::ShieldHit:
        burst(e.pos, 8, 6, {170, 220, 255, 255}, PK::Spark, 12, 3);
        shake_ = std::max(shake_, 2.5f);
        au.play(Sfx::ShieldHit, 0.6f, rng_.range(0.95f, 1.1f));
        break;
      case EvType::ShieldBreak:
        burst(e.pos, 26, 12, {170, 220, 255, 255}, PK::Shard, 50, 9);
        add(PK::Ring, e.pos, {}, 26, 10, 140, WHITE);
        shake_ = std::max(shake_, 12.0f);
        au.play(Sfx::ShieldBreak, 1.0f);
        break;
      case EvType::KO: {
        Color c = e.a >= 0 ? slotColor(m.fighters[e.a]) : WHITE;
        kos_.push_back({e.pos, e.dir, c, 0});
        shake_ = std::max(shake_, 24.0f);
        flash_ = std::max(flash_, 0.3f);
        flashCol_ = WHITE;
        au.play(Sfx::KO, 1.0f);
        au.play(Sfx::Crowd, 0.6f, rng_.range(0.95f, 1.05f));
        break;
      }
      case EvType::Jump:
        dust(e.pos, 4, 3.0f);
        au.play(Sfx::Jump, 0.35f, rng_.range(0.95f, 1.05f));
        break;
      case EvType::AirJump:
        add(PK::Ring, e.pos, {}, 16, 6, 40, withAlpha(WHITE, 0.8f));
        au.play(Sfx::AirJump, 0.35f, rng_.range(0.95f, 1.05f));
        break;
      case EvType::Land:
        dust(e.pos, 2 + int(std::min(6.0f, e.strength * 0.4f)), 2.5f + e.strength * 0.2f);
        au.play(Sfx::Land, clampf(0.2f + e.strength / 25.0f, 0.2f, 0.8f));
        break;
      case EvType::Dash:
        if (e.a >= 0) {
          const Fighter& f = m.fighters[e.a];
          for (int i = 0; i < 3; ++i)
            add(PK::Smoke, e.pos + Vec2{-f.facing * 10.0f, -4}, {-f.facing * rng_.range(1, 3), rng_.range(-1, 0)}, 20, 5, 13,
                {235, 225, 210, 200});
        }
        break;
      case EvType::Grab: au.play(Sfx::Grab, 0.8f); break;
      case EvType::Throw: au.play(Sfx::Swing, 0.8f, 0.8f); break;
      case EvType::Projectile: au.play(Sfx::Projectile, 0.5f, e.fx == HitFx::Fire ? 0.8f : e.fx == HitFx::Dark ? 1.4f : 1.0f); break;
      case EvType::Respawn:
        burst(e.pos + Vec2{0, -40}, 20, 5, {255, 255, 220, 255}, PK::Star, 30, 5);
        break;
      case EvType::Tech:
        add(PK::Ring, e.pos + Vec2{0, -10}, {}, 14, 10, 50, withAlpha({200, 240, 255, 255}, 0.9f));
        au.play(Sfx::ShieldHit, 0.6f, 1.5f);
        break;
      case EvType::LedgeGrab: au.play(Sfx::Ledge, 0.6f); break;
      case EvType::Counter:
        add(PK::Ring, e.pos, {}, 20, 10, 90, {140, 200, 255, 255});
        add(PK::Star, e.pos, {}, 14, 30, 60, WHITE);
        flash_ = std::max(flash_, 0.15f);
        flashCol_ = {140, 200, 255, 255};
        shake_ = std::max(shake_, 6.0f);
        au.play(Sfx::Counter, 0.9f);
        break;
      case EvType::Swing:
        au.play(Sfx::Swing, clampf(0.2f + e.strength / 30.0f, 0.2f, 0.7f), clampf(1.3f - e.strength / 40.0f, 0.7f, 1.3f));
        break;
      case EvType::Countdown: au.play(Sfx::Beep, 0.7f); break;
      case EvType::Go: au.play(Sfx::Go, 0.8f); break;
      case EvType::GameSet:
        au.play(Sfx::GameSet, 0.9f);
        au.play(Sfx::Crowd, 0.9f, 0.9f);
        break;
      case EvType::Clank: break;
      case EvType::OrbSpawn:
        burst(e.pos, 24, 6, {255, 250, 200, 255}, PK::Star, 40, 6);
        au.play(Sfx::Go, 0.6f, 1.4f);
        break;
      case EvType::OrbHit: {
        for (int i = 0; i < 10; ++i) {
          float a = rng_.range(0, 2 * kPi);
          add(PK::Spark, e.pos, Vec2{std::cos(a), std::sin(a)} * rng_.range(4, 9), 14, 3, 0,
              ColorFromHSV(rng_.range(0, 360), 0.7f, 1.0f));
        }
        shake_ = std::max(shake_, 3.0f);
        au.play(Sfx::HitMedium, 0.6f, 1.4f);
        au.play(Sfx::ShieldHit, 0.4f, 1.3f);
        break;
      }
      case EvType::OrbBreak:
        for (int i = 0; i < 50; ++i) {
          float a = rng_.range(0, 2 * kPi);
          add(PK::Star, e.pos, Vec2{std::cos(a), std::sin(a)} * rng_.range(3, 14), rng_.range(30, 60), rng_.range(5, 10), 1,
              ColorFromHSV(rng_.range(0, 360), 0.7f, 1.0f));
        }
        add(PK::Ring, e.pos, {}, 30, 10, 220, WHITE);
        flash_ = std::max(flash_, 0.45f);
        flashCol_ = WHITE;
        shake_ = std::max(shake_, 14.0f);
        au.play(Sfx::ShieldBreak, 1.0f, 1.2f);
        au.play(Sfx::GameSet, 0.8f, 1.25f);
        au.play(Sfx::Crowd, 0.8f, 1.1f);
        break;
      case EvType::FinalSmash:  // SUPER CLASH: the ancient power is unleashed
        flash_ = std::max(flash_, 0.55f);
        flashCol_ = mixColor(kZonaiGlow, WHITE, 0.55f);
        shake_ = std::max(shake_, 10.0f);
        add(PK::Hex, e.pos, {}, 40, 30, 520, kZonaiGlow);
        add(PK::Ring, e.pos, {}, 34, 20, 380, WHITE);
        motes(e.pos, 40, 30, 10, 0);
        au.play(Sfx::UltUnleash, 1.0f);
        au.play(Sfx::Crowd, 0.9f, 1.15f);
        break;
      case EvType::UltReady:  // meter full: runes light up
        add(PK::Hex, e.pos, {}, 30, 16, 150, kZonaiGlow);
        add(PK::Hex, e.pos, {}, 42, 10, 105, mixColor(kZonaiGlow, WHITE, 0.5f));
        add(PK::Ring, e.pos, {}, 22, 10, 90, withAlpha(mixColor(kZonaiGlow, WHITE, 0.6f), 0.9f));
        motes(e.pos, 26, 20, 5.5f, 2.5f);
        flash_ = std::max(flash_, 0.1f);
        flashCol_ = kZonaiGlow;
        shake_ = std::max(shake_, 3.0f);
        au.play(Sfx::UltReady, 0.95f);
        break;
    }
  }
}

void Fx::update(const Match& m) {
  for (auto& p : parts_) {
    p.vel.y += p.gravity;
    p.vel *= p.drag;
    p.pos += p.vel;
    p.rot += p.rotVel;
    p.life -= 1.0f;
  }
  parts_.erase(std::remove_if(parts_.begin(), parts_.end(), [](const Particle& p) { return p.life <= 0; }), parts_.end());
  for (auto& k : kos_) ++k.t;
  kos_.erase(std::remove_if(kos_.begin(), kos_.end(), [](const KoBeam& k) { return k.t > 70; }), kos_.end());

  for (const auto& f : m.fighters) {
    if (!f.active()) continue;
    const float H = f.height();
    if (f.state == FState::Attack && f.move == FinalSmash) {
      if (f.moveFrame < 34 && f.hitlag == 0) {  // gathering: energy flows in
        for (int i = 0; i < 2; ++i) {
          float a = rng_.range(0, 2 * kPi);
          Vec2 dir{std::cos(a), std::sin(a)};
          add(PK::Mote, f.center() + dir * (1.6f * H), dir * -7.0f, 15, rng_.range(2.0f, 3.2f), 1.0f, kZonaiGlow, 0.0f, 1.0f);
        }
      }
      // Once per shock wave, on impact: the wave's own hit-stop holds the frame.
      int& wave = fsWave_[f.id & 3];
      for (const auto& h : f.def->moves[FinalSmash].hitboxes)
        if (h.start == f.moveFrame && wave != h.start) {
          wave = h.start;
          motes(f.center(), 24, 20, 13, 0);
          add(PK::Hex, f.center(), {}, 26, 20, h.radius * 1.1f, mixColor(kZonaiGlow, WHITE, 0.4f));
        }
    } else {
      fsWave_[f.id & 3] = -1;
    }
    if (f.hitlag > 0) continue;
    float sp = f.kbVel.len();
    if (f.state == FState::Hitstun && sp > 8.0f && m.frame % 2 == 0) {
      Color c = sp > 16.0f ? Color{255, 170, 90, 220} : Color{230, 230, 235, 200};
      add(PK::Smoke, f.center(), f.kbVel * -0.05f, 26, 8, 20 + sp, c);
    }
    if (f.finalSmash && m.frame % 3 == 0)  // ultimate ready: motes rising around the body
      add(PK::Mote, f.pos + Vec2{rng_.range(-0.45f, 0.45f) * H, -rng_.range(0.0f, 0.9f) * H},
          {rng_.range(-0.3f, 0.3f), rng_.range(-1.6f, -0.8f)}, rng_.range(30, 50), rng_.range(1.8f, 3.2f), 0.4f,
          m.frame % 4 ? kZonaiGlow : mixColor(kZonaiGlow, WHITE, 0.6f), -0.02f, 0.99f);
    if (f.charging && m.frame % 3 == 0) {
      float a = rng_.range(0, 2 * kPi);
      Vec2 o = Vec2{std::cos(a), std::sin(a)} * (f.height() * 0.6f);
      add(PK::Spark, f.center() + o, o * -0.08f, 10, 2.5f, 0, {255, 240, 150, 255});
    }
  }
  for (const auto& p : m.projectiles) {
    if (m.frame % 2 || p.visual == ProjVisual::Shuriken) continue;
    Color c = p.visual == ProjVisual::Fireball ? Color{255, 150, 50, 200}
              : p.visual == ProjVisual::Bolt  ? Color{150, 230, 255, 200}
                                              : Color{255, 230, 120, 200};
    add(PK::Dot, p.pos, {rng_.range(-0.5f, 0.5f), rng_.range(-0.5f, 0.5f)}, 14, p.radius * 0.5f, 0, c);
  }
  shake_ *= 0.86f;
  flash_ *= 0.85f;
  for (auto& s : hudShake) s *= 0.88f;
}

void Fx::drawWorld() const {
  bool glows = false;
  for (const auto& p : parts_) {
    float k = p.life / p.maxLife;  // 1 -> 0
    float size = lerpf(p.size1, p.size, k);
    switch (p.kind) {
      case PK::Mote: glows = true; break;
      case PK::Hex: DrawPolyLinesEx(V(p.pos), 6, size, p.rot, 1.5f + 3.0f * k, withAlpha(p.color, k)); break;
      case PK::Spark: {
        Vec2 tail = p.pos - p.vel * 2.2f;
        DrawLineEx(V(tail), V(p.pos), size, withAlpha(p.color, k));
        break;
      }
      case PK::Dot: DrawCircleV(V(p.pos), std::max(0.5f, size), withAlpha(p.color, k * p.color.a / 255.0f)); break;
      case PK::Smoke: DrawCircleV(V(p.pos), size, withAlpha(p.color, k * 0.6f * p.color.a / 255.0f)); break;
      case PK::Ring: DrawRing(V(p.pos), std::max(0.0f, size - 4.0f), size, 0, 360, 36, withAlpha(p.color, k)); break;
      case PK::Star: {
        if (size > 14.0f) {  // impact twinkle: two thin crossed diamonds around a soft core
          DrawCircleV(V(p.pos), size * 0.32f, withAlpha(p.color, k * 0.55f));
          for (int i = 0; i < 2; ++i) {
            float a = deg2rad(p.rot + i * 90.0f), len = size * (i ? 0.65f : 1.0f);
            Vec2 d{std::cos(a), std::sin(a)}, n{-d.y * size * 0.13f, d.x * size * 0.13f};
            Vector2 q[4] = {V(p.pos + d * len), V(p.pos + n), V(p.pos - d * len), V(p.pos - n)};
            DrawTriangleFan(q, 4, withAlpha(p.color, k));
          }
          break;
        }
        DrawPoly(V(p.pos), 4, size, p.rot, withAlpha(p.color, k * 0.9f));
        DrawPoly(V(p.pos), 4, size * 0.6f, p.rot + 45, withAlpha(p.color, k));
        break;
      }
      case PK::Shard: DrawPoly(V(p.pos), 3, size, p.rot, withAlpha(p.color, k)); break;
    }
  }
  if (!glows) return;
  BeginBlendMode(BLEND_ADDITIVE);  // motes add light
  for (const auto& p : parts_) {
    if (p.kind != PK::Mote) continue;
    float k = p.life / p.maxLife;
    float a = std::min(1.0f, (1.0f - k) * 8.0f) * std::min(1.0f, k * 2.5f);
    float size = lerpf(p.size1, p.size, k);
    Vector2 pos = V(p.pos + Vec2{std::sin(p.life * 0.15f + p.rot) * 2.5f, 0.0f});
    DrawCircleV(pos, size * 3.0f, withAlpha(p.color, 0.2f * a));
    DrawPoly(pos, 4, size, p.rot + p.life * 3.0f, withAlpha(p.color, 0.95f * a));
  }
  ui::frameBlend();
}

void Fx::drawScreen(const Camera2D& cam) const {
  for (const auto& k : kos_) {
    Vector2 sp = GetWorldToScreen2D(V(k.pos), cam);
    Vec2 o{clampf(sp.x, 0.0f, float(kScreenW)), clampf(sp.y, 0.0f, float(kScreenH))};
    Vec2 in = -k.dir;
    Vec2 perp{-in.y, in.x};
    float t = k.t / 70.0f;
    float len = 900.0f * std::min(1.0f, t * 5.0f);
    float width = 200.0f * (1.0f - t);
    float a = 1.0f - t;
    auto wedge = [&](float wmul, float lmul, Color c) {
      Vector2 p0 = V(o + perp * (width * wmul)), p1 = V(o - perp * (width * wmul)), p2 = V(o + in * (len * lmul));
      float cross = (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x);
      if (cross > 0) std::swap(p1, p2);
      DrawTriangle(p0, p1, p2, c);
    };
    wedge(1.0f, 1.0f, withAlpha(k.color, 0.75f * a));
    wedge(0.55f, 0.8f, withAlpha(mixColor(k.color, WHITE, 0.5f), 0.85f * a));
    wedge(0.2f, 0.6f, withAlpha(WHITE, a));
    DrawCircleV(V(o), width * 0.9f, withAlpha(WHITE, a * 0.8f));
  }
  if (flash_ > 0.01f) DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(flashCol_, flash_));
}

Vec2 Fx::shakeOffset() {
  if (shake_ < 0.3f) return {};
  return {rng_.range(-shake_, shake_), rng_.range(-shake_, shake_)};
}
