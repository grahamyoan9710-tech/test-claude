#include "game/fx.hpp"

#include "game/fighter_art.hpp"

#include "game/render.hpp"
#include "game/ui.hpp"

namespace {
constexpr size_t kMaxParticles = 1400;

// DBZ-like ki aura colours per hero: outer flame, inner light; dark auras are drawn with normal blending.
struct AuraLook {
  Color outer, inner;
  bool dark;
};
AuraLook auraLook(Hero h) {
  const HitFx e = heroElement(h);
  return {hitFxColor(e), hitFxGlow(e), e == HitFx::Dark};
}

}  // namespace

Color auraColor(Hero h) {
  const AuraLook a = auraLook(h);
  return a.dark ? a.inner : a.outer;
}

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

void Fx::motes(Vec2 pos, int count, float radius, float speed, float rise, Color col) {
  for (int i = 0; i < count; ++i) {
    Vec2 dir = rdir();
    add(PK::Mote, pos + dir * rng_.range(0.0f, radius), dir * (speed * rng_.range(0.35f, 1.0f)) + Vec2{0.0f, -rise},
        rng_.range(30, 55), rng_.range(2.0f, 3.5f), 0.6f, i % 4 ? col : mixColor(col, WHITE, 0.6f), -0.03f, 0.94f);
  }
}

// The owner's shock wave of a fighter (sheet ref_effets.png, waves 1-5): its transformation's, also the impact
// wave of its powerful blows. By hero: 01 -> 5, 02 -> 3, 03 -> 2, 07 -> 4 (no transformation), 08 -> 1...
constexpr Color kShieldTint{170, 220, 255, 255};
constexpr float kWaveSize = 1.5f;  // the waves engulf the fighter
static const char* waveOf(Hero h) {
  static const char* const kWave[] = {"move_wave_5", "move_wave_3", "move_wave_2", nullptr, nullptr,
                                      "move_wave_4", "move_wave_4", "move_wave_1", nullptr, "move_wave_4",
                                      nullptr, "move_wave_5", nullptr, "move_wave_1", nullptr, "move_wave_2"};
  const int i = int(h);
  return i >= 0 && i < int(std::size(kWave)) && kWave[i] ? kWave[i] : "move_wave_1";
}

// ---- Hit sparks ----------------------------------------------------------------------
// The PixelLab blast of an element's explosions (nullptr: the particles alone).
static const char* explosionSprite(HitFx fx) {
  switch (fx) {
    case HitFx::Fire: case HitFx::Normal: case HitFx::Slash: return "explosion_fire";
    case HitFx::Electric: case HitFx::Lightning: return "explosion_electric";
    default: return nullptr;
  }
}

void Fx::hitSpark(Vec2 pos, Vec2 dir, float kb, HitFx fx, bool big) {
  // The owner's sheet 1 (docs/refonte/vfx_maquette_1_systeme.png), in its own colours for everyone: a critical
  // blow, a blade's cut, a launch upward or a quick hit.
  const float heavy = clampf(kb / 120.0f, 0.12f, 1.6f) * (big ? 1.3f : 1.0f);
  if (dir.len2() < 0.01f) dir = {1.0f, 0.0f};
  const bool crit = big || kb >= 110.0f, cut = fx == HitFx::Slash, up = dir.y < -0.8f && kb >= 40.0f;
  const char* s = crit ? "m_crit" : cut ? "m_slash" : up ? "m_rise" : "m_quick";
  const bool flip = dir.x < 0.0f;
  const float rot = cut && !crit ? (flip ? std::atan2(-dir.y, -dir.x) : std::atan2(dir.y, dir.x)) : 0.0f;
  playFx(s, pos, 0.55f + 0.25f * std::min(heavy, 1.2f), rot, flip);
}

void Fx::groundImpact(const Match& m, Vec2 feet, float strength) {
  for (const auto& p : m.platforms) {  // the solid ground right under the feet, if any
    const Rect& r = p.rect;
    if (!p.solid || feet.x < r.left() + 6.0f || feet.x > r.right() - 6.0f || feet.y < r.top() - 30.0f || feet.y > r.top() + 40.0f)
      continue;
    playFx(strength >= 160.0f ? "m_ground_heavy" : "m_ground_big", {feet.x, r.top()}, 0.8f);
    return;
  }
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
    case HitFx::Shadow:  // black lightning that cancels magic, dark flakes
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
    case HitFx::Arcane:
      for (int i = 0; i < n; ++i)
        add(PK::Wisp, pos + rdir() * rng_.range(0, 14), rdir() * rng_.range(1.5f, 5.0f) + Vec2{0, -1.0f},
            rng_.range(18, 30), 8 + 8 * heavy, 2, i % 2 ? c : g, -0.08f, 0.93f);
      break;
    case HitFx::Rose:
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
    case HitFx::Moon:
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
    case HitFx::Star:
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
    case HitFx::Sun:  // swirls of golden flame, a few pale sparks
      for (int i = 0; i < n; ++i)
        add(PK::Wisp, pos + rdir() * rng_.range(0, 12), rdir() * rng_.range(2.0f, 6.0f), rng_.range(16, 28), 8 + 7 * heavy, 2,
            i % 3 ? Color{255, 150, 40, 255} : Color{110, 170, 255, 255}, -0.05f);
      for (int i = 0; i < 2; ++i) {
        Particle& g = add(PK::Gust, pos, {}, 14, 18 + 10 * i, 50 + 40 * heavy, {255, 200, 120, 255});
        g.rot = rng_.range(0, 360);
      }
      break;
    case HitFx::Lightning:
      for (int i = 0; i < 4 + int(heavy * 5); ++i) {
        Particle& z = add(PK::Zap, pos, {}, rng_.range(6, 10), 40 + 80 * heavy * rng_.range(0.6f, 1.0f), 3.5f, {150, 200, 255, 255});
        z.rot = rng_.range(0, 2 * kPi);
        z.color2 = WHITE;
      }
      for (int i = 0; i < n / 2; ++i)
        add(PK::Mote, pos, rdir() * rng_.range(3.0f, 9.0f), rng_.range(12, 20), 2.5f, 0.5f, {220, 240, 255, 255}, 0.0f, 0.9f);
      break;
    case HitFx::Nature:  // black-green blast
      for (int i = 0; i < n; ++i)
        add(PK::Wisp, pos + rdir() * rng_.range(0, 10), rdir() * rng_.range(2.0f, 6.0f), rng_.range(16, 26), 8 + 7 * heavy, 2,
            i % 2 ? Color{70, 230, 130, 255} : Color{10, 30, 18, 255}, -0.03f);
      {
        Particle& r = add(PK::Burst, pos, {}, 10, 40 + 40 * heavy, 100 + 120 * heavy, {120, 255, 170, 255});
        r.rot = rng_.range(0, 2 * kPi);
      }
      break;
    case HitFx::Sand:  // wild yellow reiatsu and cut marks
      for (int i = 0; i < n; ++i)
        add(PK::Wisp, pos + rdir() * rng_.range(0, 14), rdir() * rng_.range(2.0f, 6.0f) + Vec2{0, -1.0f}, rng_.range(18, 30),
            9 + 8 * heavy, 2, i % 2 ? Color{255, 220, 60, 255} : Color{255, 250, 200, 255}, -0.08f, 0.93f);
      for (int i = 0; i < 1 + int(heavy * 2); ++i) {
        Particle& c2 = add(PK::Cut, pos + rdir() * rng_.range(0, 12), {}, 14 + heavy * 4, 60 + 60 * heavy, 5 + 4 * heavy,
                           {255, 230, 90, 255});
        c2.rot = std::atan2(dir.y, dir.x) + kPi * 0.5f + rng_.range(-0.6f, 0.6f);
      }
      break;
    case HitFx::Water:
      for (int i = 0; i < n * 3 / 2; ++i)
        add(PK::Drop, pos, rdir() * rng_.range(3.0f, 9.0f) + Vec2{0, -2.5f}, rng_.range(22, 34), rng_.range(2.5f, 4.0f), 0.0f,
            i % 3 ? Color{90, 170, 240, 255} : Color{220, 240, 255, 255}, 0.4f, 0.97f);
      add(PK::Ring, pos, {}, 16, 10, 80 + 60 * heavy, withAlpha({160, 210, 255, 255}, 0.8f));
      break;
    default: burst(pos, n / 3, 6.0f, {255, 240, 190, 255}, PK::Star, 20, 4); break;
  }
}

void Fx::explosion(Vec2 pos, float r, HitFx fx, bool drawn) {
  if (drawn) {  // a PixelLab blast plays: only the shake, the flash and the sound
    shake_ = std::max(shake_, std::min(20.0f, r / 5.0f));
    flash_ = std::max(flash_, 0.14f);
    flashCol_ = hitFxGlow(fx);
    audio_->play(Sfx::Explosion, clampf(r / 90.0f, 0.5f, 1.0f), rng_.range(0.9f, 1.1f));
    return;
  }
  // Fire and plain blasts burn orange; the other elements explode in their own colours.
  const bool fiery = fx == HitFx::Fire || fx == HitFx::Normal || fx == HitFx::Slash;
  const bool elemental = !fiery;
  const Color hot = fx == HitFx::Star ? Color{255, 150, 190, 255} : fiery ? Color{255, 238, 170, 255} : hitFxGlow(fx);
  const Color mid = fx == HitFx::Star ? Color{220, 40, 90, 255} : fiery ? Color{255, 120, 30, 255} : hitFxColor(fx);
  add(PK::Flash, pos, {}, 12, r * 0.6f, r * 2.0f, hot);
  Particle& b = add(PK::Burst, pos, {}, 11, r * 0.9f, r * 2.4f, hot);
  b.rot = rng_.range(0, 2 * kPi);
  for (int i = 0; i < (drawn ? 0 : 7); ++i) {  // drawn: a PixelLab effect is the fireball
    Particle& p = add(PK::Fireball, pos + rdir() * rng_.range(0.0f, r * 0.35f), rdir() * 1.6f + Vec2{0.0f, -0.6f},
                      rng_.range(22, 34), r * 0.25f, r * rng_.range(0.6f, 0.9f), hot, -0.02f, 0.93f);
    p.color2 = elemental ? shade(mid, 0.3f) : Color{56, 46, 44, 255};
  }
  add(PK::Ring, pos, {}, 16, r * 0.3f, r * 1.9f, withAlpha(WHITE, 0.9f));
  add(PK::Ring, pos, {}, 24, r * 0.2f, r * 2.6f, withAlpha(mid, 0.7f));
  add(PK::Shock, pos + Vec2{0.0f, r * 0.3f}, {}, 20, r * 0.4f, r * 2.4f, withAlpha(hot, 0.8f));
  for (int i = 0; i < 10; ++i) {
    Vec2 v = rdir() * rng_.range(4.0f, 11.0f);
    v.y = -std::fabs(v.y) - 2.0f;
    add(PK::Debris, pos, v, rng_.range(30, 46), rng_.range(3.0f, 6.0f), 0.0f, elemental ? shade(mid, 0.45f) : Color{86, 74, 66, 255},
        0.45f, 0.98f);
  }
  for (int i = 0; i < 14; ++i)
    add(PK::Ember, pos, rdir() * rng_.range(3.0f, 10.0f), rng_.range(20, 36), 2.5f, 0.5f, mid, -0.05f, 0.93f);
  for (int i = 0; i < (drawn ? 0 : 6); ++i)
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
    case HitFx::Shadow:
      au.play(Sfx::HitElectric, 0.5f, 0.6f * p);
      au.play(Sfx::HitSlash, 0.75f, 0.85f * p);
      break;
    case HitFx::Wind:
      au.play(Sfx::WindGust, 0.55f, 1.25f * p);
      au.play(Sfx::HitSlash, 0.4f, 1.1f * p);
      break;
    case HitFx::Arcane:
      au.play(Sfx::HitSlash, 0.8f, 0.85f * p);
      au.play(Sfx::WindGust, 0.35f, 0.7f * p);
      break;
    case HitFx::Rose:
      au.play(Sfx::PetalSwish, 0.8f, p);
      au.play(Sfx::HitSlash, 0.45f, 1.2f * p);
      break;
    case HitFx::Light: au.play(Sfx::HitHoly, 0.75f, p); break;
    case HitFx::Moon: au.play(Sfx::HitGloom, 0.8f, p); break;
    case HitFx::Dark: au.play(Sfx::HitGloom, 0.7f, 0.9f * p); au.play(Sfx::HitSlash, 0.5f, 0.8f * p); break;
    case HitFx::Ice: au.play(Sfx::IceCrack, 0.8f, p); break;
    case HitFx::Star: au.play(Sfx::HitGloom, 0.55f, 1.3f * p); au.play(Sfx::HitElectric, 0.25f, 1.5f * p); break;
    case HitFx::Blood: au.play(Sfx::HitSlash, 0.85f, 0.8f * p); break;
    case HitFx::Magic: au.play(Sfx::HitElectric, 0.35f, 1.4f * p); break;
    case HitFx::Sun: au.play(Sfx::WindGust, 0.45f, 1.5f * p); au.play(Sfx::HitHoly, 0.35f, 0.8f * p); break;
    case HitFx::Lightning: au.play(Sfx::HitElectric, 0.8f, 1.15f * p); break;
    case HitFx::Nature: au.play(Sfx::HitGloom, 0.6f, 1.15f * p); au.play(Sfx::BeamBlast, 0.3f, 1.4f * p); break;
    case HitFx::Sand: au.play(Sfx::HitSlash, 0.9f, 0.75f * p); au.play(Sfx::HitHeavy, 0.3f, 0.8f * p); break;
    case HitFx::Water: au.play(Sfx::WindGust, 0.5f, 0.6f * p); au.play(Sfx::HitLight, 0.4f, 0.7f * p); break;
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
        if ((kb >= 110.0f || e.impact) && e.b >= 0 && e.b < nf)  // a powerful blow: the ground bursts under the victim
          groundImpact(m, m.fighters[e.b].pos, kb);
        if (e.b >= 0 && e.b < nf && !m.cfg.training) {  // combo counter (training has its own panel)
          const Fighter& v = m.fighters[e.b];
          Combo& c = combos_[e.b & 3];
          const int atk = e.a >= 0 ? e.a : v.lastHitBy;
          if (!(c.live && c.attacker == atk && stunnedAt_[e.b & 3] >= m.frame - 2)) {
            c = Combo{};
            c.attacker = atk;
            c.base = calmPercent_[e.b & 3];
            c.live = true;
            if (atk >= 0 && atk < nf) c.color = hitFxGlow(heroElement(m.fighters[atk].def->hero));
          }
          if (c.lastHit != m.frame) ++c.hits;  // (several hitboxes landing on one frame count once)
          c.lastHit = m.frame;
          c.since = 0;
        }
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
        playFx("sys_shield_hit", e.pos, 0.6f, 0.0f, false, kShieldTint);
        shake_ = std::max(shake_, 2.5f);
        au.play(Sfx::ShieldHit, 0.6f, rng_.range(0.95f, 1.1f));
        break;
      case EvType::ShieldBreak:
        playFx("sys_shield_break", e.pos, 0.8f, 0.0f, false, kShieldTint);
        shake_ = std::max(shake_, 12.0f);
        au.play(Sfx::ShieldBreak, 1.0f);
        break;
      case EvType::KO: {
        const Color c = e.a >= 0 ? slotColor(m.fighters[e.a]) : WHITE;
        playFx("sys_ko", e.pos, 1.3f, 0.0f, false, mixColor(c, WHITE, 0.35f));
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
        moveFx(m, e.a, "m_jump", e.pos);
        au.play(Sfx::Jump, 0.35f, rng_.range(0.95f, 1.05f));
        break;
      case EvType::AirJump:
        moveFx(m, e.a, "m_jump", e.pos, 0.8f);
        au.play(Sfx::AirJump, 0.35f, rng_.range(0.95f, 1.05f));
        break;
      case EvType::Land: {  // a landing, a slide when it lands running, the body thrown down on the ground
        const Fighter* f = e.a >= 0 && e.a < nf ? &m.fighters[e.a] : nullptr;
        const bool down = f && (f->state == FState::Knockdown || f->state == FState::Hitstun);
        const bool sliding = f && std::fabs(f->vel.x) > 5.0f;
        moveFx(m, e.a, down ? "m_ground" : sliding ? "m_dash" : "m_land", e.pos);
        au.play(Sfx::Land, clampf(0.2f + e.strength / 25.0f, 0.2f, 0.8f));
        break;
      }
      case EvType::Dash:  // the dust on the ground only
        moveFx(m, e.a, "m_dash", e.pos);
        break;
      case EvType::Grab: au.play(Sfx::Grab, 0.8f); break;
      case EvType::Throw: au.play(Sfx::Swing, 0.8f, 0.8f); break;
      case EvType::Projectile: {
        const float p = rng_.range(0.95f, 1.05f);
        switch (e.fx) {
          case HitFx::Wind: au.play(Sfx::WindGust, 0.6f, 1.3f * p); break;
          case HitFx::Rose: au.play(Sfx::PetalSwish, 0.7f, 0.9f * p); break;
          case HitFx::Light: au.play(Sfx::HitHoly, 0.4f, 1.3f * p); au.play(Sfx::Projectile, 0.4f, 1.2f * p); break;
          case HitFx::Moon: au.play(Sfx::HitGloom, 0.5f, 1.2f * p); break;
          case HitFx::Ice: au.play(Sfx::Swing, 0.7f, 0.8f * p); au.play(Sfx::IceCrack, 0.3f, 1.3f * p); break;
          case HitFx::Star: au.play(Sfx::BeamBlast, 0.4f, 1.6f * p); break;
          case HitFx::Shadow: au.play(Sfx::HitSlash, 0.6f, 0.7f * p); au.play(Sfx::WindGust, 0.4f, 0.8f * p); break;
          case HitFx::Arcane: au.play(Sfx::BeamBlast, 0.5f, 1.3f * p); break;
          case HitFx::Blood: au.play(Sfx::HitSlash, 0.5f, 1.4f * p); break;
          case HitFx::Electric: case HitFx::Lightning: au.play(Sfx::HitElectric, 0.7f, 0.8f * p); break;
          case HitFx::Sun: au.play(Sfx::WindGust, 0.6f, 1.4f * p); break;
          case HitFx::Nature: au.play(Sfx::BeamBlast, 0.5f, 1.2f * p); break;
          case HitFx::Water: au.play(Sfx::WindGust, 0.7f, 0.6f * p); break;
          case HitFx::Sand: au.play(Sfx::BeamBlast, 0.5f, 0.9f * p); break;
          default:
            au.play(Sfx::Projectile, 0.5f, e.fx == HitFx::Fire ? 0.8f : e.fx == HitFx::Dark ? 1.4f : 1.0f);
            break;
        }
        break;
      }
      case EvType::Respawn:
        if (!moveFx(m, e.a, "move_appear", e.pos, 1.0f, true)) burst(e.pos + Vec2{0, -40}, 20, 5, {255, 255, 220, 255}, PK::Star, 30, 5);
        break;
      case EvType::Tech:
        playFx("sys_tech", e.pos, 0.7f, 0.0f, false, kShieldTint);
        au.play(Sfx::ShieldHit, 0.6f, 1.5f);
        break;
      case EvType::LedgeGrab: au.play(Sfx::Ledge, 0.6f); break;
      case EvType::Counter:
        if (art::fxInfo("counter_flash")) {  // the PixelLab parry, at the clips' pixel size
          playFx("counter_flash", e.pos, 0.62f);
        } else {
          add(PK::Ring, e.pos, {}, 20, 10, 90, {140, 200, 255, 255});
          add(PK::Star, e.pos, {}, 14, 30, 60, WHITE);
          add(PK::Flash, e.pos, {}, 10, 30, 110, {170, 215, 255, 255});
        }
        flash_ = std::max(flash_, 0.15f);
        flashCol_ = {140, 200, 255, 255};
        shake_ = std::max(shake_, 6.0f);
        au.play(Sfx::Counter, 0.9f);
        break;
      case EvType::Reflect: {
        playFx("sys_reflect", e.pos, 0.7f, 0.0f, false, mixColor(hitFxGlow(e.fx), WHITE, 0.4f));
        shake_ = std::max(shake_, 5.0f);
        au.play(Sfx::Reflect, 0.9f, rng_.range(0.95f, 1.05f));
        break;
      }
      case EvType::Swing:
        au.play(Sfx::Swing, clampf(0.2f + e.strength / 30.0f, 0.2f, 0.7f), clampf(1.3f - e.strength / 40.0f, 0.7f, 1.3f));
        break;
      case EvType::Countdown:
        au.play(Sfx::Beep, 0.4f);
        au.voice(e.strength >= 2.5f ? "three" : e.strength >= 1.5f ? "two" : "one");
        break;
      case EvType::Go:
        au.play(Sfx::Go, 0.8f);
        au.voice("go");
        break;
      case EvType::GameSet:
        au.play(Sfx::GameSet, 0.9f);
        au.voice("game");
        au.play(Sfx::Crowd, 0.9f, 0.9f);
        break;
      case EvType::Clank: playFx("sys_clank", e.pos, 0.6f); break;
      case EvType::OrbSpawn:
        playFx("sys_orb_hit", e.pos, 1.2f, 0.0f, false, {255, 246, 200, 255});
        au.play(Sfx::Go, 0.6f, 1.4f);
        break;
      case EvType::OrbHit: {
        playFx("sys_orb_hit", e.pos, 0.7f, 0.0f, false, ColorFromHSV(rng_.range(0, 360), 0.35f, 1.0f));
        shake_ = std::max(shake_, 3.0f);
        au.play(Sfx::HitMedium, 0.6f, 1.4f);
        au.play(Sfx::ShieldHit, 0.4f, 1.3f);
        break;
      }
      case EvType::OrbBreak:
        playFx("sys_orb_break", e.pos, 1.2f, 0.0f, false, {255, 246, 200, 255});
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
        playFx("sys_final_smash", e.pos, 1.6f, 0.0f, false, mixColor(el, WHITE, 0.3f));
        au.play(Sfx::UltUnleash, 1.0f);
        au.play(Sfx::CutIn, 0.9f);
        au.voice("superclash");
        au.play(Sfx::DomainDrone, 0.85f, e.a >= 0 ? 0.85f + 0.05f * (e.a % 4) : 1.0f);
        au.play(Sfx::Crowd, 0.9f, 1.15f);
        break;
      }
      case EvType::UltReady: {  // meter full: runes light up in the hero's element
        const Color el = e.a >= 0 && e.a < nf ? hitFxGlow(heroElement(m.fighters[e.a].def->hero)) : kZonaiGlow;
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
      case EvType::Explosion:
        {  // the element's PixelLab blast, else the grey one tinted, its radius on the blast's
          const char* x = explosionSprite(e.fx);
          const bool own = x && art::fxInfo(x);
          if (!own) x = "sys_explosion";
          if (const art::FxInfo* fi = art::fxInfo(x))
            playFx(x, e.pos, std::max(20.0f, e.strength) / (fi->hit > 0.0f ? fi->hit : 0.4f * std::min(fi->fw, fi->fh)), 0.0f,
                   false, own ? WHITE : mixColor(hitFxColor(e.fx), WHITE, 0.35f));
        }
        explosion(e.pos, std::max(20.0f, e.strength), e.fx, !sprites_.empty() && sprites_.back().t == 0);
        break;
      case EvType::WeaponSpawn: {  // a pillar of light guides the weapon down from the sky
        const auto& chars = roster();
        const int ch = std::clamp(int(e.strength), 0, int(chars.size()) - 1);
        const WeaponDef& w = chars[ch].weapons[e.b & 1];
        Pillar p;
        p.character = ch;
        p.slot = e.b & 1;
        p.pos = e.pos;
        p.color = hitFxColor(w.fx);
        p.glow = w.fx == HitFx::Shadow ? Color{230, 230, 240, 255} : hitFxGlow(w.fx);
        pillars_.push_back(p);
        au.play(Sfx::WeaponPickup, 0.4f, 0.7f);
        break;
      }
      case EvType::WeaponLand: {
        playFx("sys_weapon_land", e.pos, 0.8f, 0.0f, false, {255, 250, 225, 255});
        shake_ = std::max(shake_, 4.0f);
        au.play(Sfx::Land, 0.7f, 0.8f);
        au.play(Sfx::ShieldHit, 0.35f, 0.9f);
        break;
      }
      case EvType::WeaponSummon: {  // the weapon appears in a flash of its colour (the shared "appear" effect)
        const auto& chars = roster();
        const int ch = std::clamp(int(e.strength), 0, int(chars.size()) - 1);
        const WeaponDef& w = chars[ch].weapons[e.b & 1];
        const Color c = hitFxColor(w.fx), g = hitFxGlow(w.fx);
        // The weapon's name, above the fighter's J1 / CPU tag (render_fighter.cpp, tagY)
        const Vec2 name = e.a >= 0 && e.a < nf ? m.fighters[e.a].pos - Vec2{0.0f, m.fighters[e.a].height() + 100.0f}
                                               : e.pos - Vec2{0.0f, 70.0f};
        if (art::fxInfo("move_appear")) {
          const Vec2 feet = e.a >= 0 && e.a < nf ? m.fighters[e.a].pos : e.pos;
          playFx("move_appear", feet, chars[ch].height / 170.0f, 0.0f, false, mixColor(c, WHITE, 0.25f));
          texts_.push_back({name, w.name, g, 0});
          au.play(Sfx::WeaponPickup, 0.9f);
          break;
        }
        add(PK::Flash, e.pos, {}, 12, 30, 130, g);
        add(PK::Ring, e.pos, {}, 18, 10, 120, withAlpha(w.fx == HitFx::Shadow ? Color{20, 18, 24, 255} : c, 0.9f));
        add(PK::Ring, e.pos, {}, 26, 10, 170, withAlpha(g, 0.6f));
        Particle& b = add(PK::Burst, e.pos, {}, 14, 50, 170, g);
        b.rot = rng_.range(0, 2 * kPi);
        for (int i = 0; i < 18; ++i)
          add(PK::Star, e.pos, rdir() * rng_.range(3, 9), rng_.range(24, 40), rng_.range(3, 6), 1, i % 2 ? g : WHITE);
        elementBurst(e.pos, {0, -1}, w.fx, 0.6f);
        texts_.push_back({name, w.name, g, 0});
        au.play(Sfx::WeaponPickup, 0.9f);
        break;
      }
      case EvType::Transform: {  // a transformation: its shock wave (the owner's sheet: one per transformation)
        if (e.a >= 0 && e.a < nf && moveFx(m, e.a, waveOf(m.fighters[e.a].def->hero), m.fighters[e.a].pos, kWaveSize, true)) {
          shake_ = std::max(shake_, 12.0f);
          au.play(Sfx::UltReady, 0.9f, 0.8f);
          au.play(Sfx::BeamBlast, 0.7f, 0.7f);
          break;
        }
        const Color el = e.a >= 0 && e.a < nf ? hitFxGlow(heroElement(m.fighters[e.a].def->hero)) : WHITE;
        add(PK::Flash, e.pos, {}, 14, 40, 260, el);
        add(PK::Ring, e.pos, {}, 26, 16, 300, el);
        Particle& b = add(PK::Burst, e.pos, {}, 18, 80, 420, el);
        b.rot = rng_.range(0, 2 * kPi);
        motes(e.pos, 30, 24, 8, 2, el);
        shake_ = std::max(shake_, 12.0f);
        au.play(Sfx::UltReady, 0.9f, 0.8f);
        au.play(Sfx::BeamBlast, 0.7f, 0.7f);
        break;
      }
      case EvType::FormEnd:
        if (e.a >= 0 && e.a < nf) moveFx(m, e.a, "sys_form_end", m.fighters[e.a].pos, 1.0f, true);
        break;
      case EvType::Capture:
        flash_ = std::max(flash_, 0.2f);
        flashCol_ = WHITE;
        shake_ = std::max(shake_, 8.0f);
        add(PK::Ring, e.pos, {}, 14, 20, 140, WHITE);
        au.play(Sfx::Counter, 0.8f, 0.8f);
        break;
      case EvType::WeaponVanish: {  // the weapon fades away (the shared "vanish" effect)
        const auto& chars = roster();
        const int ch = std::clamp(int(e.strength), 0, int(chars.size()) - 1);
        const WeaponDef& w = chars[ch].weapons[e.b & 1];
        const Color c = hitFxColor(w.fx);
        if (art::fxInfo("move_vanish")) {
          const Vec2 feet = e.a >= 0 && e.a < nf ? m.fighters[e.a].pos : e.pos;
          playFx("move_vanish", feet, chars[ch].height / 170.0f, 0.0f, false, mixColor(c, WHITE, 0.25f));
          break;
        }
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
  // A projectile drawn by its PixelLab effect carries its own trail; a placeholder leaves a few dots of its colour.
  for (const auto& p : m.projectiles) {
    if ((p.fx && art::fxInfo(p.fx)) || m.frame % 2) continue;
    const Color c = withAlpha(hitFxColor(p.hit.fx), 0.8f);
    add(PK::Dot, p.pos, {rng_.range(-0.5f, 0.5f), rng_.range(-0.5f, 0.5f)}, 14, p.radius * 0.5f, 0, c);
  }
}

void Fx::playFx(const char* name, Vec2 at, float size, float rot, bool flip) {
  playFx(name, at, size, rot, flip, WHITE);
}

bool Fx::moveFx(const Match& m, int who, const char* name, Vec2 at, float size, bool tinted) {
  if (who < 0 || who >= int(m.fighters.size()) || !art::fxInfo(name)) return false;
  const Fighter& f = m.fighters[who];
  const Color tint = tinted ? mixColor(hitFxColor(heroElement(f.def->hero)), WHITE, 0.45f) : WHITE;
  playFx(name, at, size * f.def->height / 170.0f, 0.0f, f.facing < 0, tint);
  return true;
}

void Fx::playFx(const char* name, Vec2 at, float size, float rot, bool flip, Color tint) {
  if (!art::fxInfo(name)) return;
  SpriteFx s{name, at, size, rot, flip, 0};
  s.tint = tint;
  sprites_.push_back(s);
}

void Fx::update(const Match& m) {
  for (auto& f : sprites_) ++f.t;
  // The PixelLab effects of the techniques: started on the move's frame, they follow their fighter.
  for (size_t i = 0; i < m.fighters.size() && i < castSeen_.size(); ++i) {
    const Fighter& f = m.fighters[i];
    const MoveDef* md = f.currentMove();
    const std::pair<int, int> now{md ? f.move : -1, md ? f.moveFrame : -1};
    if (md && md->castFx && f.moveFrame == md->castFxFrame && now != castSeen_[i] && art::fxInfo(md->castFx)) {
      SpriteFx s{md->castFx, f.pos, f.def->height / 170.0f * md->castFxScale, 0.0f, f.facing < 0, 0};
      s.follow = int(i);
      s.off = md->castFxAt;
      s.pos = f.pos + Vec2{s.off.x * float(f.facing), s.off.y};
      if (md->castFxStays) s.follow = -2;  // behind, but left where it began
      sprites_.push_back(s);
    }
    castSeen_[i] = now;
  }
  for (auto& s : sprites_)
    if (s.follow >= 0 && s.follow < int(m.fighters.size())) {
      const Fighter& f = m.fighters[s.follow];
      s.pos = f.pos + Vec2{s.off.x * float(f.facing), s.off.y};
    }
  sprites_.erase(std::remove_if(sprites_.begin(), sprites_.end(),
                                [](const SpriteFx& f) {
                                  const art::FxInfo* i = art::fxInfo(f.name);
                                  return !i || f.t / 60.0f >= i->duration();
                                }),
                 sprites_.end());
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
    if (p.landT < 0 && (!it || it->grounded)) p.landT = p.t;
  }
  pillars_.erase(std::remove_if(pillars_.begin(), pillars_.end(),
                                [](const Pillar& p) { return (p.landT >= 0 && p.t - p.landT > 40) || p.t > 600; }),
                 pillars_.end());
  for (const auto& f : m.fighters) {  // combos: who is stunned, the counters follow their victims
    const int i = f.id & 3;
    const bool stunned = f.active() && (f.state == FState::Hitstun || f.state == FState::Grabbed ||
                                        f.state == FState::Captured || f.hitlag > 0);
    if (stunned) stunnedAt_[i] = m.frame;
    else calmPercent_[i] = f.percent;
    Combo& c = combos_[i];
    if (!c.live && c.hits == 0) continue;
    ++c.since;
    if (f.active()) {
      c.pos = f.center() - Vec2{0.0f, f.height() * 0.5f + 80.0f};  // above the J1 / CPU tags, even stacked
      c.damage = std::max(0.0f, f.percent - c.base);
    }
    if (c.live && (!stunned || !f.active())) c.live = false;  // it got away: the counter lingers, then fades
    if (!c.live && c.since > 70) c = Combo{};
  }
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
      shake_ = std::max(shake_, 2.0f + 3.0f * clampf(mv->beam.width / 160.0f, 0.0f, 1.0f));
    } else if (!mv) {
      beamMark_[id] = -1;
    }
    if (f.hitlag > 0) continue;
    const float sp = f.kbVel.len();
    if (f.state == FState::Hitstun && sp > 8.0f && m.frame % 3 == 0 && art::fxInfo("smoke_puff"))  // a launch trails smoke
      playFx("smoke_puff", f.center(), clampf(0.25f + sp / 60.0f, 0.3f, 0.7f), rng_.range(0.0f, 6.28f), false,
             sp > 16.0f ? Color{255, 190, 140, 255} : WHITE);
  }
  projectileTrails(m);
  shake_ *= 0.86f;
  flash_ *= 0.85f;
  for (auto& s : hudShake) s *= 0.88f;
}

// ---- Drawing ---------------------------------------------------------------------------
void Fx::drawBack(const Match& m, float time) const {
  for (const auto& f : sprites_)  // the techniques' effects, behind their fighter
    if (f.follow != -1) art::drawFx(f.name, f.t / 60.0f, f.pos, f.size, f.rot, f.flip, 1.0f, f.tint);
  for (const auto& p : parts_) {  // cracks in the ground, under everything that moves
    if (p.kind != PK::Crack) continue;
    const float a = clampf(p.life / 40.0f, 0.0f, 1.0f), grow = clampf((p.maxLife - p.life) / 5.0f, 0.0f, 1.0f);
    const Color dark = withAlpha(p.color, 0.9f * a), lit = withAlpha({255, 240, 214, 255}, 0.3f * a);
    vfx::glow(p.pos + Vec2{0.0f, 3.0f}, p.size * 0.42f, withAlpha(p.color, 0.55f * a), withAlpha(p.color, 0.0f), 16);
    const int branches = 4 + int(p.seed % 3u);
    for (int b = 0; b < branches; ++b) {
      float ang = kPi * (0.12f + 0.76f * (b + 0.5f) / branches) + (vfx::hash01(p.seed + b * 7u) - 0.5f) * 0.35f;
      Vec2 q = p.pos;
      const float len = p.size * (0.6f + 0.5f * vfx::hash01(p.seed + b * 13u)) * grow;
      for (int k = 0; k < 4; ++k) {
        const float seg = len / 4.0f, w = 3.2f - 0.7f * k;
        const Vec2 n = q + Vec2{std::cos(ang), std::sin(ang)} * seg;
        DrawLineEx(V(q + Vec2{0.0f, 1.5f}), V(n + Vec2{0.0f, 1.5f}), w, lit);
        DrawLineEx(V(q), V(n), w, dark);
        if (k == 1 && vfx::hash01(p.seed + b * 31u) > 0.5f) {  // a fork
          const float fa = ang + (vfx::hash01(p.seed + b * 37u) > 0.5f ? 0.6f : -0.6f);
          DrawLineEx(V(n), V(n + Vec2{std::cos(fa), std::sin(fa)} * seg * 0.8f), w * 0.6f, dark);
        }
        q = n;
        ang += (vfx::hash01(p.seed + b * 17u + k * 5u) - 0.5f) * 0.7f;
        ang = clampf(ang, kPi * 0.08f, kPi * 0.92f);  // always down into the platform
      }
    }
    DrawEllipse(int(p.pos.x), int(p.pos.y + 1), p.size * 0.32f, 3.0f, withAlpha(p.color, 0.8f * a));  // the dent
  }
  // Pillars of light from the sky
  if (!pillars_.empty()) {
    for (const auto& p : pillars_) {  // the PixelLab pillar of light over the weapon
      const float a = p.landT < 0 ? std::min(1.0f, p.t / 8.0f) : 1.0f - (p.t - p.landT) / 40.0f;
      if (a <= 0.0f) continue;
      art::drawFx("sys_weapon_pillar", time, p.pos + Vec2{0.0f, 10.0f}, 1.6f, 0.0f, false, 0.75f * a,  // sky to weapon
                  mixColor(p.glow, WHITE, 0.3f));
    }
  }
  // Motes of light rising around the fighter, in its colour: full ultimate meter, charging a smash, unleashing the
  // ultimate (no flame aura).
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
    art::drawFx("sys_ult_motes", art::fxLoopTime("sys_ult_motes", time + f.id * 0.37f), f.pos,
                f.def->height / 170.0f * (0.95f + 0.1f * k), 0.0f, f.facing < 0, std::min(1.0f, k),
                look.dark ? look.inner : look.outer);
  }
}

void Fx::drawWorld() const {
  auto drawSprites = [&] {  // the PixelLab effects, on top of the particles
    for (const auto& f : sprites_)
      if (f.follow == -1) art::drawFx(f.name, f.t / 60.0f, f.pos, f.size, f.rot, f.flip, 1.0f, f.tint);
  };
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
      case PK::Crack: break;  // drawn in drawBack, under the fighters
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
  if (!glows) return drawSprites();
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
  drawSprites();
}

void Fx::drawScreen(const Camera2D& cam) const {
  for (const auto& k : kos_) {
    if (!kVfx) break;
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
  for (const auto& c : combos_) {  // "4 COUPS !" over the victim of a combo, popping on every hit
    if (c.hits < 3) continue;
    const float fade = c.live ? 1.0f : clampf((70 - c.since) / 25.0f, 0.0f, 1.0f);
    if (fade <= 0.0f) continue;
    const Vector2 sp = GetWorldToScreen2D(V(c.pos), cam);
    const float pop = c.since < 8 ? 1.0f + 0.45f * (1.0f - c.since / 8.0f) : 1.0f;
    const float size = std::min(52.0f, 34.0f + 2.0f * c.hits) * pop;
    const char* num = TextFormat("%d", c.hits);
    const float nw = textWidth(num, size, FontKind::Title), lw = textWidth("COUPS !", size * 0.5f, FontKind::Title);
    const float x = clampf(sp.x - (nw + 8 + lw) * 0.5f, 10.0f, kScreenW - nw - lw - 18.0f);
    const float y = clampf(sp.y - size - (c.live ? 0.0f : (1.0f - fade) * 20.0f), 70.0f, kScreenH - 170.0f);
    const Color col = withAlpha(mixColor(c.color, WHITE, 0.25f), fade), ink = withAlpha(kOutline, fade);
    drawTextOutlined(num, x, y, size, col, ink, 3.5f, FontKind::Title);
    drawTextOutlined("COUPS !", x + nw + 8, y + size * 0.42f, size * 0.5f, withAlpha(WHITE, fade), ink, 2.5f, FontKind::Title);
    if (c.damage >= 1.0f)
      drawTextOutlined(TextFormat("+%d %%", int(c.damage)), x + nw + 8, y + size * 0.42f + size * 0.5f, size * 0.36f,
                       withAlpha(mixColor(c.color, WHITE, 0.6f), fade), ink, 2.0f);
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
  if (kVfx && flash_ > 0.01f) DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(flashCol_, flash_));
}

Vec2 Fx::shakeOffset() {
  if (shake_ < 0.3f) return {};
  return {rng_.range(-shake_, shake_), rng_.range(-shake_, shake_)};
}
