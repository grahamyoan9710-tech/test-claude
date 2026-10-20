// Big move effects: the VFX drawing kit, beams and the ultimates' cinematic domains.
// Everything is made of a few large gradient triangles (cheap on software GL), additive where it glows.
#include "game/fighter_art.hpp"
#include "game/render.hpp"
#include "game/ui.hpp"

// ---- Drawing kit ----------------------------------------------------------------------
namespace vfx {
namespace {
inline void vtx(Vec2 p, Color c) {
  rlColor4ub(c.r, c.g, c.b, c.a);
  rlVertex2f(p.x, p.y);
}
inline Color clear(Color c) { return {c.r, c.g, c.b, 0}; }
int segsFor(float r) { return std::clamp(int(r * 0.45f), 10, 44); }
}  // namespace

float hash01(uint32_t i) {
  uint32_t x = i * 2654435761u;
  x ^= x >> 13;
  x *= 0x5bd1e995u;
  x ^= x >> 15;
  return float(x & 0xFFFF) / 65535.0f;
}

void additive(bool on) {
  if (on) BeginBlendMode(BLEND_ADDITIVE);
  else ui::frameBlend();
}

void tri(Vec2 a, Vec2 b, Vec2 c, Color ca, Color cb, Color cc) {
  rlBegin(RL_TRIANGLES);
  vtx(a, ca);
  vtx(b, cb);
  vtx(c, cc);
  rlEnd();
}

void quad(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Color ca, Color cb, Color cc, Color cd) {
  rlBegin(RL_TRIANGLES);
  vtx(a, ca);
  vtx(b, cb);
  vtx(c, cc);
  vtx(a, ca);
  vtx(c, cc);
  vtx(d, cd);
  rlEnd();
}

void glow(Vec2 c, float r, Color in, Color out, int seg) {
  if (r <= 0.5f) return;
  if (seg <= 0) seg = segsFor(r);
  rlBegin(RL_TRIANGLES);
  for (int i = 0; i < seg; ++i) {
    const float a0 = 2.0f * kPi * i / seg, a1 = 2.0f * kPi * (i + 1) / seg;
    vtx(c, in);
    vtx({c.x + std::cos(a0) * r, c.y + std::sin(a0) * r}, out);
    vtx({c.x + std::cos(a1) * r, c.y + std::sin(a1) * r}, out);
  }
  rlEnd();
}

void ring(Vec2 c, float r0, float r1, Color in, Color out, int seg) {
  if (r1 <= 0.5f) return;
  if (seg <= 0) seg = segsFor(std::max(r0, r1));
  r0 = std::max(0.0f, r0);
  rlBegin(RL_TRIANGLES);
  for (int i = 0; i < seg; ++i) {
    const float a0 = 2.0f * kPi * i / seg, a1 = 2.0f * kPi * (i + 1) / seg;
    const Vec2 d0{std::cos(a0), std::sin(a0)}, d1{std::cos(a1), std::sin(a1)};
    const Vec2 p0 = c + d0 * r0, p1 = c + d1 * r0, q0 = c + d0 * r1, q1 = c + d1 * r1;
    vtx(p0, in);
    vtx(q0, out);
    vtx(q1, out);
    vtx(p0, in);
    vtx(q1, out);
    vtx(p1, in);
  }
  rlEnd();
}

void softRing(Vec2 c, float r, float thick, Color col, int seg) {
  ring(c, r - thick * 0.5f, r, clear(col), col, seg);
  ring(c, r, r + thick * 0.5f, col, clear(col), seg);
}

void streak(Vec2 a, Vec2 b, float w0, float w1, Color c0, Color c1) {
  const Vec2 d = (b - a).norm();
  if (d.len2() < 0.5f) return;
  const Vec2 n{-d.y, d.x};
  const Vec2 a0 = a + n * (w0 * 0.5f), a1 = a - n * (w0 * 0.5f), b0 = b + n * (w1 * 0.5f), b1 = b - n * (w1 * 0.5f);
  quad(a, b, b0, a0, c0, c1, clear(c1), clear(c0));
  quad(a, b, b1, a1, c0, c1, clear(c1), clear(c0));
}

void wedge(Vec2 a, Vec2 b, float w0, float w1, Color c0, Color c1) {
  const Vec2 d = (b - a).norm();
  if (d.len2() < 0.5f) return;
  const Vec2 n{-d.y, d.x};
  quad(a + n * (w0 * 0.5f), b + n * (w1 * 0.5f), b - n * (w1 * 0.5f), a - n * (w0 * 0.5f), c0, c1, c1, c0);
}

void bolt(Vec2 a, Vec2 b, float jag, int segs, uint32_t seed, float w, Color col, Color core) {
  segs = std::clamp(segs, 1, 24);
  const Vec2 d = b - a;
  const Vec2 n = Vec2{-d.y, d.x}.norm();
  Vec2 pts[25];
  for (int i = 0; i <= segs; ++i) {
    const float t = float(i) / segs;
    const float off = (i == 0 || i == segs) ? 0.0f : (hash01(seed + i * 7919u) - 0.5f) * 2.0f * jag;
    pts[i] = a + d * t + n * off;
  }
  for (int i = 0; i < segs; ++i) {
    const float t0 = float(i) / segs, t1 = float(i + 1) / segs;  // thins out towards the end
    wedge(pts[i], pts[i + 1], w * (1.0f - 0.5f * t0), w * (1.0f - 0.5f * t1), col, col);
  }
  if (core.a == 0) return;
  for (int i = 0; i < segs; ++i) {
    const float t0 = float(i) / segs, t1 = float(i + 1) / segs;
    wedge(pts[i], pts[i + 1], w * 0.4f * (1.0f - 0.5f * t0), w * 0.4f * (1.0f - 0.5f * t1), core, core);
  }
}

void crescent(Vec2 c, Vec2 fwd, float r, float thick, float span, Color body, Color edge) {
  constexpr int kSeg = 14;
  const float base = std::atan2(fwd.y, fwd.x);
  Vec2 out[kSeg + 1], in[kSeg + 1];
  for (int i = 0; i <= kSeg; ++i) {
    const float u = float(i) / kSeg * 2.0f - 1.0f;  // -1 .. 1
    const float a = base + u * span;
    const Vec2 d{std::cos(a), std::sin(a)};
    const float th = thick * std::cos(u * kPi * 0.5f);
    out[i] = c + d * r;
    in[i] = c + d * (r - th);
  }
  for (int i = 0; i < kSeg; ++i) quad(out[i], out[i + 1], in[i + 1], in[i], edge, edge, body, body);
}

void burst(Vec2 c, int rays, float r0, float r1, float width, float rot, Color in, Color out, uint32_t seed) {
  rlBegin(RL_TRIANGLES);
  for (int i = 0; i < rays; ++i) {
    const float h = seed ? hash01(seed + i * 131u) : 0.5f;
    const float a = rot + i * 2.0f * kPi / rays + (h - 0.5f) * 0.5f * (2.0f * kPi / rays);
    const float len = seed ? lerpf(r0, r1, 0.45f + 0.55f * hash01(seed * 3u + i * 977u)) : r1;
    const Vec2 p0 = c + Vec2{std::cos(a - width * 0.5f), std::sin(a - width * 0.5f)} * r0;
    const Vec2 p1 = c + Vec2{std::cos(a + width * 0.5f), std::sin(a + width * 0.5f)} * r0;
    const Vec2 tip = c + Vec2{std::cos(a), std::sin(a)} * len;
    vtx(p0, in);
    vtx(p1, in);
    vtx(tip, out);
  }
  rlEnd();
}

Rect viewRect(float margin) {
  // Inverse of the current 2D transform (camera inside beginWorld): target pixels -> world.
  const Matrix m = rlGetMatrixModelview();
  const float det = m.m0 * m.m5 - m.m4 * m.m1;
  const float W = kScreenW * g_renderScale, H = kScreenH * g_renderScale;
  if (std::fabs(det) < 1e-9f) return {-margin, -margin, W + margin * 2.0f, H + margin * 2.0f};
  float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
  for (Vec2 s : {Vec2{0, 0}, Vec2{W, 0}, Vec2{0, H}, Vec2{W, H}}) {
    const float sx = s.x - m.m12, sy = s.y - m.m13;
    const float x = (m.m5 * sx - m.m4 * sy) / det, y = (-m.m1 * sx + m.m0 * sy) / det;
    x0 = std::min(x0, x);
    y0 = std::min(y0, y);
    x1 = std::max(x1, x);
    y1 = std::max(y1, y);
  }
  return {x0 - margin, y0 - margin, x1 - x0 + margin * 2.0f, y1 - y0 + margin * 2.0f};
}
}  // namespace vfx

namespace {
using vfx::hash01;
inline Color A(Color c, float a) { return withAlpha(c, a); }
inline Color Z(Color c) { return withAlpha(c, 0.0f); }
inline Vec2 dirOf(float a) { return {std::cos(a), std::sin(a)}; }

void fillView(const Rect& v, Color top, Color bottom) {
  vfx::quad({v.x, v.y}, {v.right(), v.y}, {v.right(), v.bottom()}, {v.x, v.bottom()}, top, top, bottom, bottom);
}
void glowE(Vec2 c, float rx, float ry, Color in, Color out, int seg = 24) {
  rlBegin(RL_TRIANGLES);
  for (int i = 0; i < seg; ++i) {
    const float a0 = 2.0f * kPi * i / seg, a1 = 2.0f * kPi * (i + 1) / seg;
    rlColor4ub(in.r, in.g, in.b, in.a);
    rlVertex2f(c.x, c.y);
    rlColor4ub(out.r, out.g, out.b, out.a);
    rlVertex2f(c.x + std::cos(a0) * rx, c.y + std::sin(a0) * ry);
    rlVertex2f(c.x + std::cos(a1) * rx, c.y + std::sin(a1) * ry);
  }
  rlEnd();
}
// Thick polyline made of wedges (round joints are not needed at these widths).
void polyline(const Vec2* p, int n, float w0, float w1, Color c0, Color c1) {
  for (int i = 0; i + 1 < n; ++i) {
    const float t0 = float(i) / std::max(1, n - 1), t1 = float(i + 1) / std::max(1, n - 1);
    vfx::wedge(p[i], p[i + 1], lerpf(w0, w1, t0), lerpf(w0, w1, t1), mixColor(c0, c1, t0), mixColor(c0, c1, t1));
  }
}

// ---- Beams ----------------------------------------------------------------------------
struct BeamStyle {
  Color halo, body, core, edge;
  bool dark;   // black beams: body with normal blending, light only on the rim
  bool smoke;  // smoke billowing along the beam
};

// The placeholder beam (until its PixelLab effect is imported): the element's colours.
BeamStyle beamStyle(HitFx fx) {
  const Color c = hitFxColor(fx), g = hitFxGlow(fx);
  return {c, c, WHITE, g, fx == HitFx::Dark || fx == HitFx::Shadow, false};
}

void drawBeam(Vec2 a, Vec2 dir, float len, float w, const BeamStyle& s, float time, uint32_t seed) {
  const Vec2 n{-dir.y, dir.x};
  const Vec2 e = a + dir * len;
  const uint32_t fseed = seed + uint32_t(time * 30.0f) * 7919u;  // crackles change every other frame

  // 1. Halo (light around the beam) and its ends.
  vfx::additive(true);
  vfx::streak(a, e, w * 1.9f, w * 2.1f, A(s.halo, s.dark ? 0.5f : 0.42f), A(s.halo, s.dark ? 0.42f : 0.3f));
  vfx::glow(a, w * 1.25f, A(s.halo, 0.6f), Z(s.halo));
  vfx::glow(e, w * 1.1f, A(s.halo, 0.45f), Z(s.halo));
  vfx::additive(false);

  // 2. Body.
  if (s.dark) {
    vfx::streak(a, e, w * 1.25f, w * 1.3f, A(s.body, 0.85f), A(s.body, 0.8f));
    vfx::wedge(a, e, w * 0.78f, w * 0.82f, A(s.body, 0.96f), A(s.body, 0.94f));
    vfx::glow(e, w * 0.62f, A(s.body, 0.95f), A(s.body, 0.0f));
  } else {
    vfx::streak(a, e, w * 1.05f, w * 1.1f, A(s.edge, 0.55f), A(s.edge, 0.4f));  // darker rim (Cero) / glow
    vfx::streak(a, e, w * 0.95f, w * 1.0f, A(s.body, 0.95f), A(s.body, 0.9f));
  }

  // 3. Smoke billowing along malice beams.
  if (s.smoke) {
    for (int i = 0; i < 12; ++i) {
      const float u = std::fmod(time * 1.6f + hash01(seed + i * 37u), 1.0f);
      const float side = (hash01(seed + i * 53u) - 0.5f) * 1.1f;
      const Vec2 p = a + dir * (len * u) + n * (w * side) + Vec2{0.0f, -u * w * 0.3f};
      const float r = w * (0.28f + 0.3f * u);
      vfx::glow(p, r, A({18, 2, 6, 255}, 0.55f * (1.0f - u)), A({18, 2, 6, 255}, 0.0f), 16);
    }
  }

  vfx::additive(!s.dark);
  // 4. Ripples running along the beam (rings seen from the side).
  for (int i = 0; i < 6; ++i) {
    const float u = std::fmod(time * 2.4f + i / 6.0f, 1.0f);
    const Vec2 p = a + dir * (len * u);
    Vec2 prev{};
    for (int k = 0; k <= 12; ++k) {
      const float t = k * 2.0f * kPi / 12.0f;
      const Vec2 q = p + dir * (std::cos(t) * w * 0.13f) + n * (std::sin(t) * w * 0.56f);
      if (k > 0) vfx::wedge(prev, q, 3.0f, 3.0f, A(s.dark ? s.edge : s.core, 0.35f * (1.0f - u)), A(s.dark ? s.edge : s.core, 0.35f * (1.0f - u)));
      prev = q;
    }
  }
  // 5. White-hot core (dark beams keep a smouldering one).
  if (s.dark) {
    vfx::streak(a, e, w * 0.34f, w * 0.3f, A(s.edge, 0.22f), A(s.edge, 0.12f));
  } else {
    vfx::streak(a, e, w * 0.5f, w * 0.44f, A(s.core, 0.9f), A(s.core, 0.75f));
    vfx::streak(a, e, w * 0.2f, w * 0.18f, A(WHITE, 1.0f), A(WHITE, 0.9f));
  }
  vfx::additive(false);

  // 6. Crackling jagged edges (and arcs leaping off them).
  if (s.dark) vfx::additive(true);
  const int pts = std::clamp(int(len / 38.0f), 3, 32);
  for (int side = -1; side <= 1; side += 2) {
    Vec2 prev = a + n * (side * w * 0.42f);
    for (int i = 1; i <= pts; ++i) {
      const float t = float(i) / pts;
      const float off = side * w * (0.47f + (hash01(fseed + i * 31u + side * 7u) - 0.5f) * 0.16f);
      const Vec2 q = a + dir * (len * t) + n * off;
      vfx::wedge(prev, q, 3.2f, 3.2f, A(s.edge, 0.9f), A(s.edge, 0.9f));
      prev = q;
    }
  }
  for (int i = 0; i < 5; ++i) {
    const float t = hash01(fseed + i * 101u);
    const float side = hash01(fseed + i * 211u) < 0.5f ? -1.0f : 1.0f;
    const Vec2 p = a + dir * (len * t) + n * (side * w * 0.5f);
    const Vec2 q = p + n * (side * w * (0.25f + 0.35f * hash01(fseed + i * 307u))) + dir * (w * (hash01(fseed + i * 401u) - 0.5f) * 0.6f);
    vfx::bolt(p, q, w * 0.08f, 4, fseed + i * 17u, 3.0f, A(s.edge, 0.85f));
  }
  if (s.dark) vfx::additive(false);

  // 7. Start burst at the hands.
  const float spin = time * 7.0f;
  if (s.dark) {
    vfx::additive(true);
    vfx::burst(a, 9, w * 0.25f, w * 1.15f, 0.32f, spin, A(s.halo, 0.7f), Z(s.halo), fseed);
    vfx::additive(false);
    vfx::glow(a, w * 0.6f, A(s.body, 1.0f), A(s.body, 0.85f));
    vfx::softRing(a, w * 0.62f, 10.0f, A(s.edge, 0.9f));
  } else {
    vfx::additive(true);
    vfx::burst(a, 10, w * 0.22f, w * 1.3f, 0.3f, spin, A(s.core, 0.8f), Z(s.halo), fseed);
    vfx::glow(a, w * 0.62f, A(s.core, 0.95f), A(s.body, 0.0f));
    vfx::additive(false);
  }

  // 8. Splash where the beam ends: spray, ripples.
  const float ph = std::fmod(time * 5.0f, 1.0f);
  vfx::additive(true);
  rlBegin(RL_TRIANGLES);
  for (int i = 0; i < 9; ++i) {
    const float ang = std::atan2(dir.y, dir.x) + (hash01(fseed + i * 59u) - 0.5f) * 2.6f;
    const float l = w * (0.5f + 1.1f * hash01(fseed + i * 61u));
    const Vec2 d = dirOf(ang), nn{-d.y, d.x};
    const Color c0 = A(s.dark ? s.edge : s.core, 0.75f), c1 = Z(s.halo);
    rlColor4ub(c0.r, c0.g, c0.b, c0.a);
    rlVertex2f(e.x + nn.x * w * 0.08f, e.y + nn.y * w * 0.08f);
    rlVertex2f(e.x - nn.x * w * 0.08f, e.y - nn.y * w * 0.08f);
    rlColor4ub(c1.r, c1.g, c1.b, c1.a);
    rlVertex2f(e.x + d.x * l, e.y + d.y * l);
  }
  rlEnd();
  vfx::softRing(e, w * (0.3f + 0.7f * ph), w * 0.12f, A(s.dark ? s.edge : s.core, 0.6f * (1.0f - ph)));
  vfx::additive(false);
}

// ---- Ultimate domains ------------------------------------------------------------------
template <class Fn>
void forEachUltimate(const Match& m, Fn fn) {
  for (const auto& f : m.fighters) {
    if (!f.active() || f.state != FState::Attack || f.move != FinalSmash) continue;
    const int D = std::max(1, f.moveDef(FinalSmash).duration), mf = f.moveFrame;
    const float k = smoothstep(mf / 14.0f) * smoothstep((D - mf) / 16.0f);
    if (k > 0.01f) fn(f, k, mf);
  }
}

}  // namespace

// Beams drawn by a PixelLab effect: a segment that tiles along the beam (fx_<name>) and its end (fx_<name>_head), both
// sized on the beam's width (the effect's hit radius is half the beam's width). The beams by element.
static bool drawSpriteBeam(Vec2 a, Vec2 dir, float len, float w, const char* name, HitFx fx, float time) {
  const art::FxInfo* seg = name ? art::fxInfo(name) : nullptr;
  if (!seg) return false;
  const float size = 0.5f * w / (seg->hit > 0.0f ? seg->hit : 0.4f * seg->fh);
  art::drawFxStrip(name, time, a, dir, len, size, 1.0f, std::min(len, 0.6f * w));
  const std::string head = std::string(name) + "_head";
  if (art::fxInfo(head.c_str())) {
    const bool flip = dir.x < 0.0f;
    art::drawFx(head.c_str(), time, a + dir * len, size, flip ? std::atan2(-dir.y, -dir.x) : std::atan2(dir.y, dir.x), flip);
  }
  // The flare at the hand, as on the drawn beams: the segment fades in under it.
  const BeamStyle s = beamStyle(fx);
  vfx::additive(true);
  vfx::glow(a, w * 0.5f, A(s.halo, 0.35f), Z(s.halo));
  vfx::glow(a, w * 0.24f, A(s.dark ? s.edge : s.core, 0.6f), Z(s.dark ? s.edge : s.core));
  vfx::additive(false);
  return true;
}

void drawBeams(const Match& m, float time) {
  for (const auto& f : m.fighters) {
    const MoveDef* mv = f.currentMove();
    if (!mv || mv->beam.start < 0 || f.moveFrame < mv->beam.start || f.moveFrame >= mv->beam.end) continue;
    const BeamDef& b = mv->beam;
    const Vec2 dir{std::cos(deg2rad(b.angle)) * f.facing, -std::sin(deg2rad(b.angle))};
    const Vec2 a = f.center() + Vec2{b.from.x * f.facing, b.from.y};
    const int u = f.moveFrame - b.start, left = b.end - f.moveFrame;
    // Bursts out thin, flares, settles; pulses; collapses at the end.
    float env = u < 2 ? 0.55f + 0.3f * u : u < 5 ? 1.15f - 0.05f * (u - 2) : 1.0f;
    env *= std::min(1.0f, left / 5.0f + 0.2f);
    const float pulse = 1.0f + 0.06f * std::sin(time * 47.0f) + 0.04f * std::sin(time * 91.0f + f.id);
    const float len = b.length * std::min(1.0f, (u + 1) / 3.0f);
    if (b.sprite && drawSpriteBeam(a, dir, len, b.width * env * pulse, b.sprite, b.fx, time)) continue;
    drawBeam(a, dir, len, b.width * env * pulse, beamStyle(b.fx), time, uint32_t(f.id * 977 + 13));
  }
}

// While an ultimate runs, the stage dims behind the fighters (its effects are the move's PixelLab effects).
void drawUltimateBackdrop(const Match& m, float time) {
  (void)time;
  const Rect v = vfx::viewRect(40.0f);
  float k = 0.0f;
  forEachUltimate(m, [&](const Fighter& f, float kk, int) {
    if (f.moveDef(FinalSmash).ultFx != UltFx::None) k = std::max(k, kk);
  });
  if (k > 0.0f) DrawRectangleRec({v.x, v.y, v.w, v.h}, withAlpha({10, 8, 18, 255}, 0.45f * k));
}

void drawUltimateFront(const Match& m, float time) {
  (void)m;
  (void)time;
}
