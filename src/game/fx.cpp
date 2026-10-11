#include "game/fx.hpp"

#include "game/render.hpp"
#include "game/ui.hpp"

namespace {
constexpr size_t kMaxParticles = 1400;

// Blade-like elements leave slash streaks across the impact; blunt ones a heavier shock ring.
bool bladeFx(HitFx fx) {
  switch (fx) {
    case HitFx::Slash: case HitFx::AntiMagic: case HitFx::Reiatsu: case HitFx::Petal: case HitFx::Blood:
    case HitFx::Wind: case HitFx::Ice: case HitFx::Light: case HitFx::Dark:
      return true;
    default: return false;
  }
}

// DBZ-like ki aura colours per hero: outer flame, inner light; dark auras are drawn with normal blending.
struct AuraLook {
  Color outer, inner;
  bool dark;
};
AuraLook auraLook(Hero h) {
  switch (h) {
    case Hero::Asta: return {{14, 12, 18, 255}, {230, 230, 240, 255}, true};
    case Hero::Yuno: return {{80, 220, 150, 255}, {225, 255, 238, 255}, false};
    case Hero::Ichigo: return {{60, 130, 255, 255}, {215, 235, 255, 255}, false};
    case Hero::Byakuya: return {{250, 110, 170, 255}, {255, 228, 240, 255}, false};
    case Hero::Link: return {{255, 180, 60, 255}, {255, 248, 214, 255}, false};
    case Hero::Ganondorf: return {{36, 0, 12, 255}, {255, 60, 80, 255}, true};
    case Hero::Kratos: return {{255, 76, 20, 255}, {255, 216, 120, 255}, false};
    case Hero::Gojo: return {{80, 140, 255, 255}, {226, 238, 255, 255}, false};
    case Hero::Sukuna: return {{200, 14, 36, 255}, {255, 150, 150, 255}, false};
    default: return {{255, 220, 120, 255}, {255, 255, 230, 255}, false};
  }
}

// One layer of flame: a soft blob whose rim licks upwards in flickering tongues.
void kiFlame(Vec2 c, float H, float scale, float t, Color col, float alpha, float seed) {
  constexpr int kN = 30;
  const Vec2 o = c + Vec2{0.0f, -H * 0.1f};
  const Color in = withAlpha(col, alpha), out = withAlpha(col, 0.0f);
  Vec2 pts[kN];
  for (int i = 0; i < kN; ++i) {
    const float a = 2.0f * kPi * i / kN;
    const Vec2 d{std::cos(a), std::sin(a)};
    const float up = std::max(0.0f, -d.y);
    float r = H * scale * (0.5f + 0.06f * std::sin(t * 11.0f + i * 1.7f + seed));
    float tongue = std::pow(up, 1.4f) * H * scale * (0.42f + 0.32f * (0.5f + 0.5f * std::sin(t * 15.0f + i * 2.3f + seed)));
    if (i % 2) {  // notches between the tongues
      r *= 0.86f;
      tongue *= 0.45f;
    }
    pts[i] = o + Vec2{d.x * r * 0.78f, d.y * r * 1.02f - tongue};
  }
  for (int i = 0; i < kN; ++i) vfx::tri(o, pts[i], pts[(i + 1) % kN], in, out, out);
}
}  // namespace

Fx::Particle& Fx::add(PK kind, Vec2 pos, Vec2 vel, float life, float size, float size1, Color c, float gravity,
                      float drag) {
  if (parts_.size() >= kMaxParticles) {
    scratch_ = Particle{};
    return scratch_;
  }
  Particle p;
  p.kind = kind;
  p.pos = pos;
  p.vel = vel;
  p.life = p.maxLife = std::max(1.0f, life);
  p.size = size;
  p.size1 = size1;
  p.color = c;
  p.gravity = gravity;
  p.rot = rng_.range(0, 360);
  p.rotVel = kind == PK::Hex ? rng_.range(-1.5f, 1.5f) : rng_.range(-12, 12);
  if (kind == PK::Streak || kind == PK::Zap || kind == PK::Cut || kind == PK::Burst) p.rotVel = 0.0f;
  p.drag = drag;
  p.seed = rng_.next();
  parts_.push_back(p);
  return parts_.back();
}

void Fx::burst(Vec2 pos, int count, float speed, Color c, PK kind, float life, float size) {
  for (int i = 0; i < count; ++i) {
    float s = speed * rng_.range(0.4f, 1.0f);
    add(kind, pos, rdir() * s, life * rng_.range(0.7f, 1.0f), size, size * 0.2f, c, kind == PK::Shard ? 0.35f : 0.0f);
  }
}

void Fx::dust(Vec2 feet, int count, float spread) {
  for (int i = 0; i < count; ++i) {
    float dir = (i % 2 == 0) ? 1.0f : -1.0f;
    add(PK::Smoke, feet + Vec2{rng_.range(-8, 8), -4}, {dir * rng_.range(1.0f, spread), rng_.range(-1.2f, -0.2f)},
        rng_.range(18, 28), rng_.range(5, 8), rng_.range(12, 18), {235, 225, 210, 200});
  }
}

void Fx::motes(Vec2 pos, int count, float radius, float speed, float rise, Color col) {
  for (int i = 0; i < count; ++i) {
    Vec2 dir = rdir();
    add(PK::Mote, pos + dir * rng_.range(0.0f, radius), dir * (speed * rng_.range(0.35f, 1.0f)) + Vec2{0.0f, -rise},
        rng_.range(30, 55), rng_.range(2.0f, 3.5f), 0.6f, i % 4 ? col : mixColor(col, WHITE, 0.6f), -0.03f, 0.94f);
  }
}

// ---- Hit sparks ----------------------------------------------------------------------
void Fx::hitSpark(Vec2 pos, Vec2 dir, float kb, HitFx fx, bool big) {
  const float heavy = clampf(kb / 120.0f, 0.12f, 1.6f) * (big ? 1.3f : 1.0f);
  const bool black = fx == HitFx::AntiMagic;
  const Color c = hitFxColor(fx), g = hitFxGlow(fx);
  const Color light = black ? Color{236, 236, 246, 255} : g;
  if (dir.len2() < 0.01f) dir = {1.0f, 0.0f};
  // Flash disc, white core and a star burst of rays
  add(PK::Flash, pos, {}, 6 + heavy * 3, 14 + 26 * heavy, 36 + 58 * heavy, light);
  add(PK::Flash, pos, {}, 5, 6 + 12 * heavy, 18 + 26 * heavy, WHITE);
  Particle& b = add(PK::Burst, pos, {}, 8 + heavy * 2, 24 + 40 * heavy, 50 + 84 * heavy, light);
  b.rot = rng_.range(0, 2 * kPi);
  // Directional slash streaks across the impact
  if (bladeFx(fx)) {
    const int n = 1 + int(heavy * 1.6f);
    const float base = std::atan2(dir.y, dir.x) + kPi * 0.5f;
    for (int i = 0; i < n; ++i) {
      Particle& s = add(PK::Streak, pos + dir * rng_.range(-8, 8), dir * 1.2f, 9 + heavy * 3, 70 + 120 * heavy,
                        6 + 10 * heavy, black ? Color{14, 12, 18, 255} : c);
      s.rot = base + rng_.range(-0.45f, 0.45f) + i * 0.5f;
      s.color2 = black ? Color{240, 240, 250, 255} : WHITE;
    }
  }
  // Shock waves
  add(PK::Ring, pos, {}, 12 + heavy * 4, 8, 36 + 56 * heavy, withAlpha(black ? Color{40, 36, 48, 255} : c, 0.9f));
  if (heavy > 0.55f || !bladeFx(fx)) add(PK::Ring, pos, {}, 16 + heavy * 4, 12, 56 + 74 * heavy, withAlpha(light, 0.6f));
  // Sparks
  const int n = std::min(30, 6 + int(kb / 8.0f));
  for (int i = 0; i < n; ++i) {
    Vec2 v = rdir() * rng_.range(4.0f, 9.0f + kb * 0.06f) + dir * 3.0f;
    Color col = i % 3 == 0 ? WHITE : i % 3 == 1 ? (black ? Color{20, 18, 26, 255} : c) : light;
    add(PK::Spark, pos, v, rng_.range(10, 18), rng_.range(2.0f, 4.0f) + heavy, 0.0f, col);
  }
  elementBurst(pos, dir, fx, heavy);
}

void Fx::elementBurst(Vec2 pos, Vec2 dir, HitFx fx, float heavy) {
  const int n = std::min(22, int(4 + heavy * 10));
  const Color c = hitFxColor(fx), g = hitFxGlow(fx);
  switch (fx) {
    case HitFx::Fire:
      for (int i = 0; i < n * 3 / 2; ++i)
        add(PK::Ember, pos, rdir() * rng_.range(2.0f, 8.0f) * (0.6f + heavy) + dir * 2.0f, rng_.range(20, 34),
            rng_.range(2.0f, 3.5f), 0.6f, {255, 170, 60, 255}, -0.06f, 0.93f);
      for (int i = 0; i < 2 + int(heavy * 3); ++i) {
        Particle& p = add(PK::Fireball, pos + rdir() * rng_.range(0, 12), rdir() * 1.5f + Vec2{0, -1.0f}, rng_.range(14, 20),
                          6 + heavy * 6, 16 + heavy * 20, {255, 238, 170, 255}, -0.03f, 0.92f);
        p.color2 = {70, 50, 46, 255};
      }
      break;
    case HitFx::Electric:
      for (int i = 0; i < 3 + int(heavy * 4); ++i) {
        Particle& z = add(PK::Zap, pos, {}, rng_.range(6, 9), 30 + 60 * heavy * rng_.range(0.6f, 1.0f), 3.0f, c);
        z.rot = rng_.range(0, 2 * kPi);
        z.color2 = WHITE;
      }
      break;
    case HitFx::Slash:
      for (int i = 0; i < n / 2; ++i)
        add(PK::Shard, pos, rdir() * rng_.range(3.0f, 8.0f), rng_.range(16, 26), rng_.range(3.0f, 5.0f), 1.0f,
            {230, 240, 255, 255}, 0.3f);
      break;
    case HitFx::Dark:
      for (int i = 0; i < n / 2; ++i)
        add(PK::Smoke, pos, rdir() * rng_.range(1.0f, 4.0f), rng_.range(20, 30), 8, 22 + heavy * 12, {60, 20, 90, 220});
      for (int i = 0; i < n / 2; ++i)
        add(PK::Wisp, pos, rdir() * rng_.range(2.0f, 6.0f), rng_.range(16, 26), 10 + heavy * 6, 2, c, -0.05f);
      break;
    case HitFx::Magic: burst(pos, n / 2, 7.0f, c, PK::Star, 26, 5); break;
    case HitFx::AntiMagic:  // black lightning that cancels magic, dark flakes
      for (int i = 0; i < 3 + int(heavy * 5); ++i) {
        Particle& z = add(PK::Zap, pos, {}, rng_.range(6, 10), 40 + 80 * heavy * rng_.range(0.5f, 1.0f), 5.0f,
                          {12, 10, 16, 255});
        z.rot = rng_.range(0, 2 * kPi);
        z.color2 = {235, 235, 245, 255};
      }
      for (int i = 0; i < n; ++i)
        add(PK::Shard, pos, rdir() * rng_.range(3.0f, 9.0f), rng_.range(18, 30), rng_.range(3.0f, 6.0f), 1.0f,
            i % 3 ? Color{16, 14, 20, 255} : Color{200, 200, 210, 255}, 0.25f);
      break;
    case HitFx::Wind:
      for (int i = 0; i < 2 + int(heavy * 3); ++i) {
        Particle& gst = add(PK::Gust, pos, {}, rng_.range(12, 18), 16 + 10 * i, 60 + 50 * heavy + 20 * i,
                            {220, 255, 235, 255});
        gst.rot = rng_.range(0, 360);
      }
      for (int i = 0; i < n; ++i)
        add(PK::Leaf, pos, rdir() * rng_.range(3.0f, 8.0f) + dir * 2.0f, rng_.range(26, 40), rng_.range(4.0f, 6.0f), 0.0f,
            i % 3 ? Color{90, 200, 110, 255} : Color{190, 245, 160, 255}, 0.06f, 0.9f);
      break;
    case HitFx::Reiatsu:
      for (int i = 0; i < n; ++i)
        add(PK::Wisp, pos + rdir() * rng_.range(0, 14), rdir() * rng_.range(1.5f, 5.0f) + Vec2{0, -1.0f},
            rng_.range(18, 30), 8 + 8 * heavy, 2, i % 2 ? c : g, -0.08f, 0.93f);
      break;
    case HitFx::Petal:
      for (int i = 0; i < n * 2; ++i)
        add(PK::Petal, pos, rdir() * rng_.range(3.0f, 10.0f) + dir * 2.0f, rng_.range(30, 46), rng_.range(4.0f, 6.5f), 0.0f,
            i % 4 == 0 ? Color{255, 236, 244, 255} : i % 4 == 1 ? Color{236, 100, 150, 255} : c, 0.05f, 0.9f);
      break;
    case HitFx::Light:
      for (int i = 0; i < n; ++i)
        add(PK::Mote, pos + rdir() * rng_.range(0, 10), rdir() * rng_.range(2.0f, 7.0f), rng_.range(24, 40),
            rng_.range(2.0f, 3.5f), 0.6f, i % 3 ? c : WHITE, -0.04f, 0.92f);
      {
        Particle& r = add(PK::Burst, pos, {}, 12, 40 + 50 * heavy, 110 + 140 * heavy, {255, 236, 160, 255});
        r.rot = rng_.range(0, 2 * kPi);
      }
      break;
    case HitFx::Gloom:
      for (int i = 0; i < n; ++i)
        add(PK::Smoke, pos + rdir() * rng_.range(0, 10), rdir() * rng_.range(1.0f, 4.5f) + Vec2{0, -0.6f}, rng_.range(22, 34),
            10, 26 + heavy * 18, i % 3 ? Color{24, 4, 10, 235} : Color{150, 16, 40, 220}, -0.02f, 0.92f);
      for (int i = 0; i < n / 2; ++i)
        add(PK::Ember, pos, rdir() * rng_.range(3.0f, 8.0f), rng_.range(16, 26), 2.5f, 0.5f, {255, 70, 90, 255}, -0.04f);
      break;
    case HitFx::Ice:
      for (int i = 0; i < n; ++i)
        add(PK::Ice, pos, rdir() * rng_.range(3.0f, 10.0f) + Vec2{0, -2.0f}, rng_.range(24, 36), rng_.range(5.0f, 9.0f), 0.0f,
            i % 3 ? Color{170, 230, 255, 255} : WHITE, 0.35f, 0.96f);
      for (int i = 0; i < 4; ++i)
        add(PK::Smoke, pos, rdir() * rng_.range(1.0f, 3.0f), rng_.range(20, 30), 8, 24 + heavy * 10, {225, 245, 255, 200});
      break;
    case HitFx::Cursed:
      for (int i = 0; i < n; ++i)
        add(PK::Wisp, pos + rdir() * rng_.range(0, 12), rdir() * rng_.range(2.0f, 6.0f), rng_.range(16, 28), 8 + 7 * heavy, 2,
            i % 2 ? Color{90, 130, 255, 255} : Color{170, 90, 255, 255}, -0.04f);
      for (int i = 0; i < 2 + int(heavy * 2); ++i) {
        Particle& z = add(PK::Zap, pos, {}, 7, 30 + 50 * heavy, 2.5f, {130, 120, 255, 255});
        z.rot = rng_.range(0, 2 * kPi);
        z.color2 = WHITE;
      }
      break;
    case HitFx::Blood: {
      const float base = std::atan2(dir.y, dir.x);
      for (int i = 0; i < 2 + int(heavy * 2); ++i) {  // crossing slash marks
        Particle& s = add(PK::Cut, pos + rdir() * rng_.range(0, 16), {}, 14 + heavy * 4, 60 + 60 * heavy, 5 + 4 * heavy,
                          {230, 30, 44, 255});
        s.rot = base + kPi * 0.5f + (i % 2 ? 0.7f : -0.7f) + rng_.range(-0.2f, 0.2f);
      }
      for (int i = 0; i < n; ++i)
        add(PK::Drop, pos, rdir() * rng_.range(3.0f, 9.0f) + dir * 2.0f, rng_.range(18, 30), rng_.range(2.0f, 3.5f), 0.0f,
            i % 3 ? Color{190, 16, 30, 255} : Color{120, 6, 16, 255}, 0.4f, 0.97f);
      break;
    }
    default: burst(pos, n / 3, 6.0f, {255, 240, 190, 255}, PK::Star, 20, 4); break;
  }
}

void Fx::explosion(Vec2 pos, float r, HitFx fx) {
  const bool cursed = fx == HitFx::Cursed;
  const Color hot = cursed ? Color{255, 150, 190, 255} : Color{255, 238, 170, 255};
  const Color mid = cursed ? Color{220, 40, 90, 255} : Color{255, 120, 30, 255};
  add(PK::Flash, pos, {}, 12, r * 0.6f, r * 2.0f, hot);
  Particle& b = add(PK::Burst, pos, {}, 11, r * 0.9f, r * 2.4f, hot);
  b.rot = rng_.range(0, 2 * kPi);
  for (int i = 0; i < 7; ++i) {
    Particle& p = add(PK::Fireball, pos + rdir() * rng_.range(0.0f, r * 0.35f), rdir() * 1.6f + Vec2{0.0f, -0.6f},
                      rng_.range(22, 34), r * 0.25f, r * rng_.range(0.6f, 0.9f), hot, -0.02f, 0.93f);
    p.color2 = cursed ? Color{50, 10, 50, 255} : Color{56, 46, 44, 255};
  }
  add(PK::Ring, pos, {}, 16, r * 0.3f, r * 1.9f, withAlpha(WHITE, 0.9f));
  add(PK::Ring, pos, {}, 24, r * 0.2f, r * 2.6f, withAlpha(mid, 0.7f));
  add(PK::Shock, pos + Vec2{0.0f, r * 0.3f}, {}, 20, r * 0.4f, r * 2.4f, withAlpha(hot, 0.8f));
  for (int i = 0; i < 10; ++i) {
    Vec2 v = rdir() * rng_.range(4.0f, 11.0f);
    v.y = -std::fabs(v.y) - 2.0f;
    add(PK::Debris, pos, v, rng_.range(30, 46), rng_.range(3.0f, 6.0f), 0.0f, cursed ? Color{80, 30, 70, 255} : Color{86, 74, 66, 255},
        0.45f, 0.98f);
  }
  for (int i = 0; i < 14; ++i)
    add(PK::Ember, pos, rdir() * rng_.range(3.0f, 10.0f), rng_.range(20, 36), 2.5f, 0.5f, mid, -0.05f, 0.93f);
  for (int i = 0; i < 6; ++i)
    add(PK::Smoke, pos + rdir() * rng_.range(0.0f, r * 0.4f), {rng_.range(-1, 1), rng_.range(-2.0f, -0.8f)}, rng_.range(40, 60),
        r * 0.3f, r * 0.7f, {60, 54, 54, 190}, -0.01f, 0.96f);
  shake_ = std::max(shake_, std::min(20.0f, r / 5.0f));
  flash_ = std::max(flash_, 0.14f);
  flashCol_ = hot;
  audio_->play(Sfx::Explosion, clampf(r / 90.0f, 0.5f, 1.0f), rng_.range(0.9f, 1.1f));
}

void Fx::startImpact(const Match& m, Vec2 pos, Vec2 dir, int attacker, int victim, HitFx fx, int len, bool ko) {
  auto bit = [](int id) { return id >= 0 && id < 32 ? 1u << id : 0u; };
  if (impact_.t < impact_.len) {  // already on screen (or about to be): one picture for everybody
    impact_.mask |= bit(attacker) | bit(victim);
    impact_.len = std::max(impact_.len, len);
    return;
  }
  if (!ko && m.frame - lastImpact_ < 16) return;  // never a strobe
  lastImpact_ = m.frame;
  impact_ = Impact{};
  impact_.t = -1;  // update() brings it to frame 0 before the first draw
  impact_.len = len;
  impact_.pos = pos;
  impact_.dir = dir;
  impact_.mask = bit(attacker) | bit(victim);
  impact_.attacker = attacker;
  impact_.fx = fx;
  impact_.ko = ko;
  impact_.seed = rng_.next();
  punch_ = std::max(punch_, ko ? 0.1f : 0.16f);
  punchPos_ = pos;
  shake_ = std::max(shake_, ko ? 26.0f : 22.0f);
  audio_->play(Sfx::ImpactBoom, 1.0f, rng_.range(0.95f, 1.05f));
}

void Fx::hitSound(HitFx fx, float kb, bool big) {
  Audio& au = *audio_;
  const float p = rng_.range(0.92f, 1.08f);
  switch (fx) {
    case HitFx::Fire: au.play(Sfx::HitFire, 0.7f, p); break;
    case HitFx::Electric: au.play(Sfx::HitElectric, 0.7f, p); break;
    case HitFx::Slash: au.play(Sfx::HitSlash, 0.8f, p); break;
    case HitFx::AntiMagic:
      au.play(Sfx::HitElectric, 0.5f, 0.6f * p);
      au.play(Sfx::HitSlash, 0.75f, 0.85f * p);
      break;
    case HitFx::Wind:
      au.play(Sfx::WindGust, 0.55f, 1.25f * p);
      au.play(Sfx::HitSlash, 0.4f, 1.1f * p);
      break;
    case HitFx::Reiatsu:
      au.play(Sfx::HitSlash, 0.8f, 0.85f * p);
      au.play(Sfx::WindGust, 0.35f, 0.7f * p);
      break;
    case HitFx::Petal:
      au.play(Sfx::PetalSwish, 0.8f, p);
      au.play(Sfx::HitSlash, 0.45f, 1.2f * p);
      break;
    case HitFx::Light: au.play(Sfx::HitHoly, 0.75f, p); break;
    case HitFx::Gloom: au.play(Sfx::HitGloom, 0.8f, p); break;
    case HitFx::Dark: au.play(Sfx::HitGloom, 0.7f, 0.9f * p); au.play(Sfx::HitSlash, 0.5f, 0.8f * p); break;
    case HitFx::Ice: au.play(Sfx::IceCrack, 0.8f, p); break;
    case HitFx::Cursed: au.play(Sfx::HitGloom, 0.55f, 1.3f * p); au.play(Sfx::HitElectric, 0.25f, 1.5f * p); break;
    case HitFx::Blood: au.play(Sfx::HitSlash, 0.85f, 0.8f * p); break;
    case HitFx::Magic: au.play(Sfx::HitElectric, 0.35f, 1.4f * p); break;
    default: break;
  }
  Sfx s = kb < 40 ? Sfx::HitLight : kb < 100 ? Sfx::HitMedium : Sfx::HitHeavy;
  au.play(s, clampf(0.5f + kb / 150.0f, 0.5f, 1.0f) * (big ? 1.0f : 0.9f), rng_.range(0.92f, 1.08f));
}

void Fx::onEvents(const Match& m) {
  Audio& au = *audio_;
  const int nf = int(m.fighters.size());
  for (const auto& e : m.events) {
    switch (e.type) {
      case EvType::Hit: {
        const float kb = e.strength;
        Vec2 pos = e.pos;
        if (e.b >= 0 && e.b < nf) {  // huge hitboxes (final smash): sparks on the victim
          const Fighter& v = m.fighters[e.b];
          Vec2 d = pos - v.center();
          if (d.len() > v.height() * 0.6f) pos = v.center() + d.norm() * (v.height() * 0.3f);
        }
        const bool big = e.impact || kb >= 140.0f;
        hitSpark(pos, e.dir, kb, e.fx, big);
        if (kb > 100.0f) {
          flash_ = std::max(flash_, 0.12f);
          flashCol_ = WHITE;
        }
        shake_ = std::max(shake_, std::min(18.0f, kb / 11.0f));
        if (e.b >= 0) hudShake[e.b & 3] = std::min(9.0f, 2.0f + kb / 14.0f);
        hitSound(e.fx, kb, big);
        if (big) {
          int attacker = e.a;  // projectiles hit without an attacker: credit their owner
          if (attacker < 0 && e.b >= 0 && e.b < nf && m.fighters[e.b].lastHitFrame >= m.frame - 1)
            attacker = m.fighters[e.b].lastHitBy;
          startImpact(m, pos, e.dir, attacker, e.b, e.fx, e.impact ? 4 : 3, false);
        }
        break;
      }
      case EvType::ShieldHit:
        burst(e.pos, 8, 6, {170, 220, 255, 255}, PK::Spark, 12, 3);
        add(PK::Flash, e.pos, {}, 6, 10, 34, {170, 220, 255, 255});
        shake_ = std::max(shake_, 2.5f);
        au.play(Sfx::ShieldHit, 0.6f, rng_.range(0.95f, 1.1f));
        break;
      case EvType::ShieldBreak:
        burst(e.pos, 26, 12, {170, 220, 255, 255}, PK::Shard, 50, 9);
        add(PK::Ring, e.pos, {}, 26, 10, 140, WHITE);
        add(PK::Flash, e.pos, {}, 12, 30, 140, {200, 235, 255, 255});
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
        const int killer = e.b >= 0 && e.b < nf && e.b != e.a && m.fighters[e.b].active() ? e.b : -1;
        startImpact(m, e.pos, e.dir, killer, -1, HitFx::Normal, 5, true);
        if (killer >= 0) punchPos_ = m.fighters[killer].center();
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
      case EvType::Projectile: {
        const float p = rng_.range(0.95f, 1.05f);
        switch (e.fx) {
          case HitFx::Wind: au.play(Sfx::WindGust, 0.6f, 1.3f * p); break;
          case HitFx::Petal: au.play(Sfx::PetalSwish, 0.7f, 0.9f * p); break;
          case HitFx::Light: au.play(Sfx::HitHoly, 0.4f, 1.3f * p); au.play(Sfx::Projectile, 0.4f, 1.2f * p); break;
          case HitFx::Gloom: au.play(Sfx::HitGloom, 0.5f, 1.2f * p); break;
          case HitFx::Ice: au.play(Sfx::Swing, 0.7f, 0.8f * p); au.play(Sfx::IceCrack, 0.3f, 1.3f * p); break;
          case HitFx::Cursed: au.play(Sfx::BeamBlast, 0.4f, 1.6f * p); break;
          case HitFx::AntiMagic: au.play(Sfx::HitSlash, 0.6f, 0.7f * p); au.play(Sfx::WindGust, 0.4f, 0.8f * p); break;
          case HitFx::Reiatsu: au.play(Sfx::BeamBlast, 0.5f, 1.3f * p); break;
          case HitFx::Blood: au.play(Sfx::HitSlash, 0.5f, 1.4f * p); break;
          case HitFx::Electric: au.play(Sfx::HitElectric, 0.7f, 0.8f * p); break;
          default:
            au.play(Sfx::Projectile, 0.5f, e.fx == HitFx::Fire ? 0.8f : e.fx == HitFx::Dark ? 1.4f : 1.0f);
            break;
        }
        break;
      }
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
        add(PK::Flash, e.pos, {}, 10, 30, 110, {170, 215, 255, 255});
        flash_ = std::max(flash_, 0.15f);
        flashCol_ = {140, 200, 255, 255};
        shake_ = std::max(shake_, 6.0f);
        au.play(Sfx::Counter, 0.9f);
        break;
      case EvType::Reflect: {
        const Color c{170, 225, 255, 255};
        add(PK::Flash, e.pos, {}, 9, 20, 80, c);
        add(PK::Ring, e.pos, {}, 14, 10, 90, withAlpha(c, 0.9f));
        Particle& b = add(PK::Burst, e.pos, {}, 9, 30, 90, WHITE);
        b.rot = rng_.range(0, 2 * kPi);
        const Vec2 n{-e.dir.y, e.dir.x};
        for (int i = 0; i < 14; ++i)
          add(PK::Spark, e.pos, e.dir * rng_.range(5.0f, 13.0f) + n * rng_.range(-4.0f, 4.0f), rng_.range(10, 16), 3.0f, 0.0f,
              i % 2 ? WHITE : hitFxGlow(e.fx));
        shake_ = std::max(shake_, 5.0f);
        au.play(Sfx::Reflect, 0.9f, rng_.range(0.95f, 1.05f));
        break;
      }
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
        for (int i = 0; i < 10; ++i)
          add(PK::Spark, e.pos, rdir() * rng_.range(4, 9), 14, 3, 0, ColorFromHSV(rng_.range(0, 360), 0.7f, 1.0f));
        shake_ = std::max(shake_, 3.0f);
        au.play(Sfx::HitMedium, 0.6f, 1.4f);
        au.play(Sfx::ShieldHit, 0.4f, 1.3f);
        break;
      }
      case EvType::OrbBreak:
        for (int i = 0; i < 50; ++i)
          add(PK::Star, e.pos, rdir() * rng_.range(3, 14), rng_.range(30, 60), rng_.range(5, 10), 1,
              ColorFromHSV(rng_.range(0, 360), 0.7f, 1.0f));
        add(PK::Ring, e.pos, {}, 30, 10, 220, WHITE);
        flash_ = std::max(flash_, 0.45f);
        flashCol_ = WHITE;
        shake_ = std::max(shake_, 14.0f);
        au.play(Sfx::ShieldBreak, 1.0f, 1.2f);
        au.play(Sfx::GameSet, 0.8f, 1.25f);
        au.play(Sfx::Crowd, 0.8f, 1.1f);
        break;
      case EvType::FinalSmash: {  // the ultimate is unleashed, in the hero's element
        const Color el = e.a >= 0 && e.a < nf ? hitFxGlow(heroElement(m.fighters[e.a].def->hero)) : kZonaiGlow;
        flash_ = std::max(flash_, 0.55f);
        flashCol_ = mixColor(el, WHITE, 0.55f);
        shake_ = std::max(shake_, 10.0f);
        add(PK::Hex, e.pos, {}, 40, 30, 520, el);
        add(PK::Ring, e.pos, {}, 34, 20, 380, WHITE);
        motes(e.pos, 40, 30, 10, 0, el);
        if (e.a >= 0 && e.a < nf) {
          add(PK::Flash, e.pos, {}, 18, 60, 420, el);
          Particle& b = add(PK::Burst, e.pos, {}, 20, 120, 640, el);
          b.rot = rng_.range(0, 2 * kPi);
        }
        au.play(Sfx::UltUnleash, 1.0f);
        au.play(Sfx::CutIn, 0.9f);
        au.play(Sfx::DomainDrone, 0.85f, e.a >= 0 ? 0.85f + 0.05f * (e.a % 4) : 1.0f);
        au.play(Sfx::Crowd, 0.9f, 1.15f);
        break;
      }
      case EvType::UltReady: {  // meter full: runes light up in the hero's element
        const Color el = e.a >= 0 && e.a < nf ? hitFxGlow(heroElement(m.fighters[e.a].def->hero)) : kZonaiGlow;
        add(PK::Hex, e.pos, {}, 30, 16, 150, el);
        add(PK::Hex, e.pos, {}, 42, 10, 105, mixColor(el, WHITE, 0.5f));
        add(PK::Ring, e.pos, {}, 22, 10, 90, withAlpha(mixColor(el, WHITE, 0.6f), 0.9f));
        motes(e.pos, 26, 20, 5.5f, 2.5f, el);
        flash_ = std::max(flash_, 0.1f);
        flashCol_ = el;
        shake_ = std::max(shake_, 3.0f);
        au.play(Sfx::UltReady, 0.95f);
        break;
      }
      case EvType::SuperFlash:
        flash_ = std::max(flash_, 0.25f);
        flashCol_ = WHITE;
        au.play(Sfx::CutIn, 0.7f, 1.1f);
        break;
      case EvType::Explosion: explosion(e.pos, std::max(20.0f, e.strength), e.fx); break;
      case EvType::WeaponSpawn: {  // a pillar of light guides the weapon down from the sky
        const auto& chars = roster();
        const int ch = std::clamp(int(e.strength), 0, int(chars.size()) - 1);
        const WeaponDef& w = chars[ch].weapons[e.b & 1];
        Pillar p;
        p.character = ch;
        p.slot = e.b & 1;
        p.pos = e.pos;
        p.color = hitFxColor(w.fx);
        p.glow = w.fx == HitFx::AntiMagic ? Color{230, 230, 240, 255} : hitFxGlow(w.fx);
        pillars_.push_back(p);
        au.play(Sfx::WeaponPickup, 0.4f, 0.7f);
        break;
      }
      case EvType::WeaponLand: {
        dust(e.pos, 8, 4.0f);
        add(PK::Shock, e.pos, {}, 18, 20, 120, withAlpha({255, 250, 230, 255}, 0.8f));
        add(PK::Flash, e.pos + Vec2{0, -30}, {}, 10, 20, 80, {255, 250, 220, 255});
        shake_ = std::max(shake_, 4.0f);
        au.play(Sfx::Land, 0.7f, 0.8f);
        au.play(Sfx::ShieldHit, 0.35f, 0.9f);
        break;
      }
      case EvType::WeaponPickup: {
        const auto& chars = roster();
        const int ch = std::clamp(int(e.strength), 0, int(chars.size()) - 1);
        const WeaponDef& w = chars[ch].weapons[e.b & 1];
        const Color c = hitFxColor(w.fx), g = w.fx == HitFx::AntiMagic ? Color{230, 230, 240, 255} : hitFxGlow(w.fx);
        add(PK::Flash, e.pos, {}, 12, 30, 130, g);
        add(PK::Ring, e.pos, {}, 18, 10, 120, withAlpha(w.fx == HitFx::AntiMagic ? Color{20, 18, 24, 255} : c, 0.9f));
        add(PK::Ring, e.pos, {}, 26, 10, 170, withAlpha(g, 0.6f));
        Particle& b = add(PK::Burst, e.pos, {}, 14, 50, 170, g);
        b.rot = rng_.range(0, 2 * kPi);
        for (int i = 0; i < 18; ++i)
          add(PK::Star, e.pos, rdir() * rng_.range(3, 9), rng_.range(24, 40), rng_.range(3, 6), 1, i % 2 ? g : WHITE);
        elementBurst(e.pos, {0, -1}, w.fx, 0.6f);
        texts_.push_back({e.pos + Vec2{0.0f, -70.0f}, w.name, g, 0});
        au.play(Sfx::WeaponPickup, 0.9f);
        break;
      }
      case EvType::WeaponBreak: {
        const auto& chars = roster();
        const int ch = std::clamp(int(e.strength), 0, int(chars.size()) - 1);
        const WeaponDef& w = chars[ch].weapons[e.b & 1];
        const Color c = hitFxColor(w.fx);
        for (int i = 0; i < 18; ++i)
          add(PK::Shard, e.pos, rdir() * rng_.range(3.0f, 10.0f) + Vec2{0, -2.0f}, rng_.range(30, 46), rng_.range(4.0f, 8.0f), 1.0f,
              i % 3 ? Color{200, 206, 216, 255} : c, 0.4f);
        add(PK::Flash, e.pos, {}, 9, 20, 90, WHITE);
        add(PK::Ring, e.pos, {}, 16, 10, 100, withAlpha(c, 0.8f));
        shake_ = std::max(shake_, 6.0f);
        au.play(Sfx::WeaponBreak, 0.9f);
        break;
      }
    }
  }
}

// ---- Per frame -------------------------------------------------------------------------
void Fx::projectileTrails(const Match& m) {
  for (const auto& p : m.projectiles) {
    const Vec2 v = p.vel;
    const Vec2 back = v.len2() > 0.01f ? v.norm() * -1.0f : Vec2{0.0f, 0.0f};
    const float r = p.radius;
    switch (p.visual) {
      case ProjVisual::Fireball: case ProjVisual::Bolt: case ProjVisual::Star:
        if (m.frame % 2 == 0) {
          Color c = p.visual == ProjVisual::Fireball ? Color{255, 150, 50, 200}
                    : p.visual == ProjVisual::Bolt  ? Color{150, 230, 255, 200}
                                                    : Color{255, 230, 120, 200};
          add(PK::Dot, p.pos, {rng_.range(-0.5f, 0.5f), rng_.range(-0.5f, 0.5f)}, 14, r * 0.5f, 0, c);
        }
        break;
      case ProjVisual::BlackSlash:
        add(PK::Shard, p.pos + rdir() * rng_.range(0.0f, r * 0.8f), back * rng_.range(1.0f, 3.0f) + rdir(), rng_.range(12, 20),
            rng_.range(2.5f, 2.5f + r * 0.08f), 1.0f, m.frame % 3 ? Color{16, 14, 20, 255} : Color{220, 220, 230, 255}, 0.05f);
        if (m.frame % 4 == 0) {
          Particle& z = add(PK::Zap, p.pos + rdir() * (r * 0.5f), {}, 5, r * 0.9f, 2.5f, {12, 10, 16, 255});
          z.rot = rng_.range(0, 2 * kPi);
          z.color2 = {235, 235, 245, 255};
        }
        break;
      case ProjVisual::WindBlade:
        if (m.frame % 2 == 0) add(PK::Spark, p.pos, back * 2.0f + rdir() * 0.5f, 8, 2.0f, 0.0f, {200, 255, 225, 255});
        break;
      case ProjVisual::Tornado:
        if (m.frame % 2 == 0)
          add(PK::Leaf, p.pos + Vec2{rng_.range(-r, r), rng_.range(-r * 1.6f, r)}, Vec2{rng_.range(-3, 3), rng_.range(-3.0f, -0.5f)},
              rng_.range(20, 30), 4.0f, 0.0f, m.frame % 4 ? Color{90, 200, 110, 255} : Color{200, 250, 180, 255}, 0.02f, 0.94f);
        if (m.frame % 5 == 0) dust(p.pos + Vec2{0.0f, r * 1.2f}, 1, 2.0f);
        break;
      case ProjVisual::Getsuga: {
        const bool dark = p.hit.fx == HitFx::Dark;
        add(dark ? PK::Smoke : PK::Wisp, p.pos + rdir() * rng_.range(0.0f, r), back * rng_.range(1.0f, 3.0f), rng_.range(14, 22),
            dark ? 10.0f : 9.0f, dark ? 24.0f : 2.0f, dark ? Color{14, 6, 10, 220} : Color{120, 190, 255, 255}, -0.03f);
        if (dark && m.frame % 2 == 0)
          add(PK::Ember, p.pos + rdir() * rng_.range(0.0f, r), back * 2.0f, 16, 2.5f, 0.5f, {255, 50, 60, 255}, -0.04f);
        break;
      }
      case ProjVisual::Cero:
        add(PK::Ember, p.pos + rdir() * (r * 0.5f), back * 2.0f + rdir(), 14, 3.0f, 0.5f, {255, 60, 70, 255});
        break;
      case ProjVisual::Petals:
        add(PK::Petal, p.pos + rdir() * rng_.range(0.0f, r), back * rng_.range(0.5f, 2.0f) + rdir() * 1.5f, rng_.range(22, 34),
            rng_.range(3.5f, 5.0f), 0.0f, m.frame % 3 ? Color{255, 160, 196, 255} : Color{255, 236, 244, 255}, 0.03f, 0.92f);
        break;
      case ProjVisual::LightArrow: case ProjVisual::SwordBeam:
        add(PK::Mote, p.pos + rdir() * rng_.range(0.0f, r * 0.6f), back * 1.5f + rdir() * 0.6f, rng_.range(16, 26), 2.4f, 0.5f,
            m.frame % 2 ? Color{255, 220, 130, 255} : WHITE, -0.02f, 0.95f);
        break;
      case ProjVisual::Bomb: {  // fuse sparks
        const float a = p.age * 0.25f;
        const Vec2 fuse = p.pos + Vec2{std::cos(a - 1.2f), std::sin(a - 1.2f)} * (r * 1.25f);
        add(PK::Spark, fuse, rdir() * rng_.range(1.5f, 4.0f) + Vec2{0, -1.0f}, rng_.range(6, 10), 2.0f, 0.0f,
            m.frame % 2 ? Color{255, 230, 120, 255} : WHITE, 0.1f);
        break;
      }
      case ProjVisual::DarkOrb: case ProjVisual::GloomSpear:
        if (m.frame % 2 == 0)
          add(PK::Smoke, p.pos + rdir() * rng_.range(0.0f, r * 0.6f), back * 1.2f + Vec2{0.0f, -0.6f}, rng_.range(20, 30),
              r * 0.35f, r * 0.9f, m.frame % 4 ? Color{20, 2, 8, 220} : Color{140, 14, 40, 200}, -0.02f, 0.94f);
        if (m.frame % 3 == 0)
          add(PK::Ember, p.pos + rdir() * (r * 0.7f), rdir() * 1.5f, 16, 2.5f, 0.5f, {255, 70, 90, 255}, -0.04f);
        break;
      case ProjVisual::Axe:  // frost trail
        add(PK::Ice, p.pos + rdir() * rng_.range(0.0f, r * 0.8f), back * rng_.range(0.5f, 2.0f) + rdir(), rng_.range(14, 22),
            rng_.range(3.0f, 5.0f), 0.0f, m.frame % 3 ? Color{180, 235, 255, 255} : WHITE, 0.15f, 0.95f);
        if (m.frame % 2 == 0)
          add(PK::Smoke, p.pos, back * 1.0f, 18, 6, 18, {220, 245, 255, 150});
        break;
      case ProjVisual::ChainBlade: case ProjVisual::FireArrow:
        add(PK::Ember, p.pos + rdir() * rng_.range(0.0f, r * 0.5f), back * rng_.range(1.0f, 3.0f) + rdir(), rng_.range(14, 24),
            rng_.range(2.0f, 3.5f), 0.5f, m.frame % 3 ? Color{255, 150, 40, 255} : Color{255, 230, 140, 255}, -0.05f, 0.94f);
        if (m.frame % 3 == 0)
          add(PK::Smoke, p.pos, back * 1.0f + Vec2{0, -0.5f}, 22, 5, 16, {70, 56, 50, 170}, -0.02f);
        break;
      case ProjVisual::BlueOrb:  // matter sucked into the void
        for (int i = 0; i < 2; ++i) {
          const Vec2 d = rdir();
          add(PK::Mote, p.pos + d * (r * rng_.range(2.2f, 3.4f)), d * -rng_.range(3.0f, 5.0f), 14, 2.4f, 0.4f,
              i ? Color{120, 190, 255, 255} : WHITE, 0.0f, 1.0f);
        }
        break;
      case ProjVisual::RedOrb:
        add(PK::Spark, p.pos, rdir() * rng_.range(3.0f, 6.0f) + back * 2.0f, 10, 2.5f, 0.0f,
            m.frame % 2 ? Color{255, 80, 90, 255} : Color{255, 200, 200, 255});
        break;
      case ProjVisual::PurpleOrb: {
        for (int i = 0; i < 2; ++i) {  // space being erased: debris pulled in
          const Vec2 d = rdir();
          add(PK::Mote, p.pos + d * (r * rng_.range(1.6f, 2.4f)), d * -rng_.range(3.0f, 6.0f) + v * 0.8f, 12, 3.0f, 0.5f,
              i ? Color{190, 120, 255, 255} : Color{255, 230, 255, 255}, 0.0f, 1.0f);
        }
        if (m.frame % 3 == 0) {
          Particle& z = add(PK::Zap, p.pos + rdir() * (r * 0.6f), v * 0.5f, 5, r * 0.9f, 3.0f, {200, 120, 255, 255});
          z.rot = rng_.range(0, 2 * kPi);
          z.color2 = WHITE;
        }
        break;
      }
      case ProjVisual::Lightning:
        if (m.frame % 2 == 0)
          add(PK::Spark, p.pos + rdir() * (r * 0.5f), rdir() * rng_.range(2.0f, 6.0f), 8, 2.0f, 0.0f, {200, 240, 255, 255});
        break;
      default: break;
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
  if (impact_.t < impact_.len) ++impact_.t;
  punch_ = impact() ? punch_ : punch_ * 0.82f;
  if (punch_ < 0.002f) punch_ = 0.0f;

  // Pillars of light follow their weapon down, then fade
  for (auto& p : pillars_) {
    ++p.t;
    const WeaponItem* it = nullptr;
    for (const auto& w : m.items)
      if (w.character == p.character && w.slot == p.slot) it = &w;
    if (it) p.pos = it->pos;
    if (p.landT < 0 && (!it || it->grounded)) {
      p.landT = p.t;
      if (it) {
        add(PK::Flash, p.pos + Vec2{0, -30}, {}, 14, 30, 150, p.glow);
        add(PK::Ring, p.pos + Vec2{0, -20}, {}, 20, 10, 130, withAlpha(p.glow, 0.8f));
      }
    }
    if (m.frame % 3 == 0 && p.landT < 0)  // sparkles sliding down the light
      add(PK::Mote, p.pos + Vec2{rng_.range(-18.0f, 18.0f), -rng_.range(40.0f, 500.0f)}, {0.0f, rng_.range(2.0f, 5.0f)}, 24, 2.6f,
          0.5f, p.glow, 0.0f, 1.0f);
  }
  pillars_.erase(std::remove_if(pillars_.begin(), pillars_.end(),
                                [](const Pillar& p) { return (p.landT >= 0 && p.t - p.landT > 40) || p.t > 600; }),
                 pillars_.end());
  for (auto& t : texts_) ++t.t;
  texts_.erase(std::remove_if(texts_.begin(), texts_.end(), [](const FloatText& t) { return t.t > 100; }), texts_.end());

  for (const auto& f : m.fighters) {
    if (!f.active()) continue;
    const float H = f.height();
    const int id = f.id & 3;
    if (f.state == FState::Attack && f.move == FinalSmash) {
      const Color el = hitFxGlow(heroElement(f.def->hero));
      if (f.moveFrame < 34 && f.hitlag == 0) {  // gathering: energy flows in
        for (int i = 0; i < 2; ++i) {
          Vec2 dir = rdir();
          add(PK::Mote, f.center() + dir * (1.6f * H), dir * -7.0f, 15, rng_.range(2.0f, 3.2f), 1.0f, el, 0.0f, 1.0f);
        }
      }
      // Once per shock wave, on impact: the wave's own hit-stop holds the frame.
      int& wave = fsWave_[id];
      for (const auto& h : f.def->moves[FinalSmash].hitboxes)
        if (h.start == f.moveFrame && wave != h.start) {
          wave = h.start;
          motes(f.center(), 24, 20, 13, 0, el);
          add(PK::Hex, f.center(), {}, 26, 20, h.radius * 1.1f, mixColor(el, WHITE, 0.4f));
        }
    } else {
      fsWave_[id] = -1;
    }
    // Beams: a blast when they fire, a hum while they last
    const MoveDef* mv = f.currentMove();
    if (mv && mv->beam.start >= 0 && f.moveFrame >= mv->beam.start && f.moveFrame < mv->beam.end) {
      const int mark = f.move * 1000 + mv->beam.start;
      if (beamMark_[id] != mark) {
        beamMark_[id] = mark;
        const float big = clampf(mv->beam.width / 160.0f, 0.5f, 1.0f);
        audio_->play(Sfx::BeamBlast, 0.6f + 0.4f * big, mv->beam.width > 100.0f ? 0.8f : 1.1f);
        audio_->play(Sfx::BeamHum, 0.5f + 0.4f * big, mv->beam.width > 100.0f ? 0.8f : 1.2f);
        shake_ = std::max(shake_, 4.0f + 8.0f * big);
      }
      if (m.frame % 2 == 0) {  // sparks spraying from the hands
        const BeamDef& b = mv->beam;
        const Vec2 a = f.center() + Vec2{b.from.x * f.facing, b.from.y};
        const Vec2 d{std::cos(deg2rad(b.angle)) * f.facing, -std::sin(deg2rad(b.angle))};
        add(PK::Spark, a, (d + rdir() * 0.8f) * rng_.range(5.0f, 11.0f), 10, 3.0f, 0.0f, hitFxGlow(b.fx));
        shake_ = std::max(shake_, 2.0f + 3.0f * clampf(b.width / 160.0f, 0.0f, 1.0f));
      }
    } else if (!mv) {
      beamMark_[id] = -1;
    }
    if (f.hitlag > 0) continue;
    float sp = f.kbVel.len();
    if (f.state == FState::Hitstun && sp > 8.0f && m.frame % 2 == 0) {
      Color c = sp > 16.0f ? Color{255, 170, 90, 220} : Color{230, 230, 235, 200};
      add(PK::Smoke, f.center(), f.kbVel * -0.05f, 26, 8, 20 + sp, c);
    }
    const AuraLook look = auraLook(f.def->hero);
    if (f.finalSmash && m.frame % 2 == 0) {  // ki: sparks shooting up through the aura
      add(PK::Spark, f.pos + Vec2{rng_.range(-0.45f, 0.45f) * H, -rng_.range(0.1f, 0.8f) * H},
          {rng_.range(-0.3f, 0.3f), -rng_.range(5.0f, 9.0f)}, rng_.range(8, 12), 2.0f, 0.0f,
          look.dark ? (m.frame % 4 ? look.outer : look.inner) : look.inner, 0.0f, 0.97f);
      if (m.frame % 6 == 0)
        add(PK::Mote, f.pos + Vec2{rng_.range(-0.45f, 0.45f) * H, -rng_.range(0.0f, 0.9f) * H},
            {rng_.range(-0.3f, 0.3f), rng_.range(-1.6f, -0.8f)}, rng_.range(30, 50), rng_.range(1.8f, 3.2f), 0.4f, look.inner,
            -0.02f, 0.99f);
    }
    if (f.charging && m.frame % 2 == 0) {  // gathering power: sparks converge
      Vec2 o = rdir() * (H * 0.7f);
      add(PK::Spark, f.center() + o, o * -0.09f, 10, 2.5f, 0, look.dark ? look.inner : mixColor(look.inner, {255, 240, 150, 255}, 0.4f));
    }
  }
  projectileTrails(m);
  shake_ *= 0.86f;
  flash_ *= 0.85f;
  for (auto& s : hudShake) s *= 0.88f;
}

// ---- Drawing ---------------------------------------------------------------------------
void Fx::drawBack(const Match& m, float time) const {
  // Pillars of light from the sky
  if (!pillars_.empty()) {
    vfx::additive(true);
    for (const auto& p : pillars_) {
      const float a = p.landT < 0 ? std::min(1.0f, p.t / 8.0f) : 1.0f - (p.t - p.landT) / 40.0f;
      if (a <= 0.0f) continue;
      const Vec2 top = p.pos + Vec2{0.0f, -2600.0f}, bot = p.pos + Vec2{0.0f, -10.0f};
      const float pulse = 1.0f + 0.15f * std::sin(time * 20.0f);
      vfx::streak(top, bot, 90.0f * pulse, 110.0f * pulse, withAlpha(p.color, 0.18f * a), withAlpha(p.color, 0.4f * a));
      vfx::streak(top, bot, 22.0f, 30.0f, withAlpha(p.glow, 0.55f * a), withAlpha(WHITE, 0.9f * a));
      vfx::glow(bot + Vec2{0.0f, -30.0f}, 70.0f, withAlpha(p.glow, 0.5f * a), withAlpha(p.glow, 0.0f), 20);
    }
    vfx::additive(false);
  }
  // DBZ ki auras: full ultimate meter, charging a smash, unleashing the ultimate
  for (const auto& f : m.fighters) {
    if (!f.active() || f.state == FState::Respawn) continue;
    const bool fs = f.state == FState::Attack && f.move == FinalSmash;
    float k = 0.0f;
    if (f.finalSmash) k = 1.0f;
    if (fs) k = 1.2f;
    if (f.charging) {
      const MoveDef* mv = f.currentMove();
      const float ratio = mv && mv->maxCharge > 0 ? clampf(float(f.charge) / mv->maxCharge, 0.0f, 1.0f) : 0.5f;
      k = std::max(k, 0.45f + 0.55f * ratio);
    }
    if (k <= 0.0f) continue;
    const AuraLook look = auraLook(f.def->hero);
    const float H = f.height(), t = time + f.id * 1.3f;
    const Vec2 c = f.center();
    const float s = 0.92f + 0.12f * k + 0.03f * std::sin(t * 20.0f);
    if (look.dark) {  // black flames, lit from inside
      kiFlame(c, H, s * 1.08f, t, look.outer, 0.62f * std::min(1.0f, k), 0.0f);
      vfx::additive(true);
      kiFlame(c, H, s * 0.82f, t * 1.3f, look.inner, 0.24f * std::min(1.0f, k), 2.0f);
      vfx::additive(false);
    } else {
      vfx::additive(true);
      kiFlame(c, H, s * 1.1f, t, look.outer, 0.42f * std::min(1.0f, k), 0.0f);
      kiFlame(c, H, s * 0.86f, t * 1.25f, mixColor(look.outer, look.inner, 0.5f), 0.38f * std::min(1.0f, k), 2.0f);
      kiFlame(c, H, s * 0.6f, t * 1.5f, look.inner, 0.3f * std::min(1.0f, k), 4.0f);
      vfx::additive(false);
    }
    vfx::additive(true);
    for (int i = 0; i < 7; ++i) {  // streaks of ki racing upwards
      const float ph = std::fmod(t * 2.2f + vfx::hash01(i * 13 + f.id * 7), 1.0f);
      const float x = c.x + (vfx::hash01(i * 17 + f.id) - 0.5f) * H * 0.85f;
      const float y = c.y + H * 0.45f - ph * H * 1.3f;
      const Color col = look.dark ? Color{230, 230, 240, 255} : look.inner;
      vfx::streak({x, y + H * 0.3f}, {x, y}, 1.0f, 3.5f, withAlpha(col, 0.0f), withAlpha(col, 0.6f * std::sin(ph * kPi) * std::min(1.0f, k)));
    }
    vfx::additive(false);
  }
}

void Fx::drawWorld() const {
  bool glows = false;
  for (const auto& p : parts_) {
    const float k = p.life / p.maxLife;  // 1 -> 0
    const float size = lerpf(p.size1, p.size, k);
    switch (p.kind) {
      case PK::Mote: case PK::Flash: case PK::Burst: case PK::Ember: case PK::Wisp: glows = true; break;
      case PK::Hex: DrawPolyLinesEx(V(p.pos), 6, size, p.rot, 1.5f + 3.0f * k, withAlpha(p.color, k)); break;
      case PK::Spark: {
        Vec2 tail = p.pos - p.vel * 2.2f;
        DrawLineEx(V(tail), V(p.pos), size, withAlpha(p.color, k));
        break;
      }
      case PK::Dot: DrawCircleV(V(p.pos), std::max(0.5f, size), withAlpha(p.color, k * p.color.a / 255.0f)); break;
      case PK::Smoke:
        vfx::glow(p.pos, size, withAlpha(p.color, k * 0.6f * p.color.a / 255.0f), withAlpha(p.color, k * 0.25f * p.color.a / 255.0f),
                  14);
        break;
      case PK::Ring: {
        const float th = std::max(3.0f, size * 0.07f) * (0.4f + 0.6f * k);
        DrawRing(V(p.pos), std::max(0.0f, size - th), size, 0, 360, 40, withAlpha(p.color, k * p.color.a / 255.0f));
        break;
      }
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
      case PK::Streak: {  // pointed at both ends, shoots out then thins away
        const float u = 1.0f - k;
        const float len = p.size * std::min(1.0f, 0.35f + u * 5.0f) * 0.5f, w = p.size1 * std::min(1.0f, k * 1.6f);
        const Vec2 d{std::cos(p.rot), std::sin(p.rot)};
        const Color col = withAlpha(p.color, std::min(1.0f, k * 1.5f));
        vfx::wedge(p.pos, p.pos + d * len, w, 0.0f, col, col);
        vfx::wedge(p.pos, p.pos - d * len, w, 0.0f, col, col);
        if (p.color2.a > 0) {
          const Color core = withAlpha(p.color2, std::min(1.0f, k * 1.5f));
          vfx::wedge(p.pos, p.pos + d * (len * 0.8f), w * 0.4f, 0.0f, core, core);
          vfx::wedge(p.pos, p.pos - d * (len * 0.8f), w * 0.4f, 0.0f, core, core);
        }
        break;
      }
      case PK::Petal: case PK::Leaf: {  // tumbling: the width flips as it turns in 3D
        const Vec2 d{std::cos(deg2rad(p.rot)), std::sin(deg2rad(p.rot))}, n{-d.y, d.x};
        const float w = p.size * 0.45f * (0.25f + 0.75f * std::fabs(std::sin(p.life * 0.25f + p.seed % 7)));
        const Color col = withAlpha(p.color, std::min(1.0f, k * 2.0f));
        vfx::quad(p.pos - d * p.size, p.pos + n * w, p.pos + d * p.size, p.pos - n * w, col, col,
                  p.kind == PK::Petal ? withAlpha(WHITE, col.a / 255.0f) : col, col);
        break;
      }
      case PK::Gust: {  // a curl of wind
        Vec2 prev{};
        const float a0 = deg2rad(p.rot);
        for (int i = 0; i <= 8; ++i) {
          const float a = a0 + i * 0.22f;
          const Vec2 q = p.pos + Vec2{std::cos(a), std::sin(a)} * (size * (1.0f - i * 0.04f));
          if (i > 0) vfx::wedge(prev, q, 1.0f + 3.5f * k * (i / 8.0f), 1.0f + 3.5f * k * ((i + 1) / 8.0f), withAlpha(p.color, 0.8f * k), withAlpha(p.color, 0.8f * k));
          prev = q;
        }
        break;
      }
      case PK::Ice: {
        const Vec2 d = p.vel.len2() > 0.1f ? p.vel.norm() : Vec2{std::cos(deg2rad(p.rot)), std::sin(deg2rad(p.rot))};
        const Vec2 n{-d.y, d.x};
        const Color col = withAlpha(p.color, std::min(1.0f, k * 2.0f)), edge = withAlpha(WHITE, std::min(1.0f, k * 2.0f));
        vfx::quad(p.pos + d * p.size, p.pos + n * (p.size * 0.3f), p.pos - d * (p.size * 0.6f), p.pos - n * (p.size * 0.3f), edge, col, col, col);
        break;
      }
      case PK::Zap: {
        const Vec2 d{std::cos(p.rot), std::sin(p.rot)};
        const uint32_t seed = p.seed + uint32_t(p.life / 2.0f) * 977u;  // flickers
        const float a = std::min(1.0f, k * 1.8f);
        vfx::bolt(p.pos, p.pos + d * p.size, p.size * 0.22f, 5, seed, p.size1 * 1.8f, withAlpha(p.color2, a * 0.5f));
        vfx::bolt(p.pos, p.pos + d * p.size, p.size * 0.22f, 5, seed, p.size1, withAlpha(p.color, a), withAlpha(p.color2, a));
        break;
      }
      case PK::Cut: {  // a crimson slash hanging in the air
        const float u = 1.0f - k;
        const float len = p.size * std::min(1.0f, u * 6.0f + 0.2f) * 0.5f, w = p.size1;
        const Vec2 d{std::cos(p.rot), std::sin(p.rot)};
        const Color col = withAlpha(p.color, std::min(1.0f, k * 1.6f)), core = withAlpha({255, 220, 220, 255}, std::min(1.0f, k * 1.6f));
        vfx::wedge(p.pos, p.pos + d * len, w, 0.0f, col, col);
        vfx::wedge(p.pos, p.pos - d * len, w, 0.0f, col, col);
        vfx::wedge(p.pos, p.pos + d * (len * 0.7f), w * 0.3f, 0.0f, core, core);
        vfx::wedge(p.pos, p.pos - d * (len * 0.7f), w * 0.3f, 0.0f, core, core);
        break;
      }
      case PK::Drop: {
        const Color col = withAlpha(p.color, std::min(1.0f, k * 2.0f));
        vfx::wedge(p.pos - p.vel * 1.4f, p.pos, p.size * 0.3f, p.size * 2.0f, col, col);
        break;
      }
      case PK::Fireball: {  // hot -> orange -> smoke
        const float u = 1.0f - k;
        const Color mid{p.color.r, uint8_t(p.color.g * 0.48f), uint8_t(p.color.b * 0.2f), 255};
        const Color col = u < 0.35f ? mixColor(p.color, mid, u / 0.35f) : mixColor(mid, p.color2, (u - 0.35f) / 0.65f);
        const float r = lerpf(p.size, p.size1, 1.0f - (1.0f - u) * (1.0f - u));
        const float a = std::min(1.0f, k * 1.7f);
        vfx::glow(p.pos, r, withAlpha(col, 0.95f * a), withAlpha(col, 0.55f * a), 20);
        break;
      }
      case PK::Debris: DrawPoly(V(p.pos), 5, size, p.rot, withAlpha(p.color, std::min(1.0f, k * 3.0f))); break;
      case PK::Shock: {  // flat ellipse expanding along the ground
        const float rx = lerpf(p.size1, p.size, k), ry = rx * 0.22f;
        Vec2 prev{};
        for (int i = 0; i <= 24; ++i) {
          const float a = i * 2.0f * kPi / 24.0f;
          const Vec2 q = p.pos + Vec2{std::cos(a) * rx, std::sin(a) * ry};
          if (i > 0) vfx::wedge(prev, q, 2.0f + 5.0f * k, 2.0f + 5.0f * k, withAlpha(p.color, k), withAlpha(p.color, k));
          prev = q;
        }
        break;
      }
    }
  }
  if (!glows) return;
  vfx::additive(true);  // light adds up
  for (const auto& p : parts_) {
    const float k = p.life / p.maxLife;
    const float size = lerpf(p.size1, p.size, k);
    switch (p.kind) {
      case PK::Mote: {
        float a = std::min(1.0f, (1.0f - k) * 8.0f) * std::min(1.0f, k * 2.5f);
        Vector2 pos = V(p.pos + Vec2{std::sin(p.life * 0.15f + p.rot) * 2.5f, 0.0f});
        DrawCircleV(pos, size * 3.0f, withAlpha(p.color, 0.2f * a));
        DrawPoly(pos, 4, size, p.rot + p.life * 3.0f, withAlpha(p.color, 0.95f * a));
        break;
      }
      case PK::Flash: {
        const float a = std::sqrt(k);
        vfx::glow(p.pos, size, withAlpha(p.color, 0.85f * a), withAlpha(p.color, 0.0f), 28);
        break;
      }
      case PK::Burst:
        vfx::burst(p.pos, 11, size * 0.12f, size, 0.2f, p.rot, withAlpha(p.color, k), withAlpha(p.color, 0.0f), p.seed);
        break;
      case PK::Ember:
        vfx::glow(p.pos, p.size * 3.0f, withAlpha(p.color, 0.35f * k), withAlpha(p.color, 0.0f), 10);
        DrawPoly(V(p.pos), 4, p.size, p.rot, withAlpha(mixColor(p.color, WHITE, 0.5f), k));
        break;
      case PK::Wisp: vfx::glow(p.pos, size, withAlpha(p.color, 0.5f * k), withAlpha(p.color, 0.0f), 14); break;
      default: break;
    }
  }
  vfx::additive(false);
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
  for (const auto& t : texts_) {  // weapon names
    const Vector2 sp = GetWorldToScreen2D(V(t.pos), cam);
    const float u = t.t / 100.0f;
    const float a = std::min(1.0f, (1.0f - u) * 5.0f);
    const float pop = t.t < 8 ? 1.0f + 0.5f * (1.0f - t.t / 8.0f) : 1.0f;
    const float size = 30.0f * pop;
    const char* s = t.text.c_str();
    const float w = textWidth(s, size, FontKind::Title);
    const float x = clampf(sp.x - w * 0.5f, 10.0f, kScreenW - w - 10.0f), y = clampf(sp.y - u * 50.0f, 10.0f, kScreenH - 140.0f);
    drawTextOutlined(s, x, y, size, withAlpha(t.color, a), withAlpha(kOutline, a), 3.0f, FontKind::Title);
  }
  if (flash_ > 0.01f) DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(flashCol_, flash_));
}

Vec2 Fx::shakeOffset() {
  if (shake_ < 0.3f) return {};
  return {rng_.range(-shake_, shake_), rng_.range(-shake_, shake_)};
}
