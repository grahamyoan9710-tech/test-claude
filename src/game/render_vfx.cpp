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

BeamStyle beamStyle(HitFx fx) {
  switch (fx) {
    case HitFx::Blood:  // Cero: crimson blast, black rim
      return {{190, 10, 30, 255}, {222, 26, 46, 255}, {255, 236, 236, 255}, {26, 0, 6, 255}, false, false};
    case HitFx::Dark:  // pure darkness edged with red
      return {{235, 24, 44, 255}, {6, 3, 8, 255}, {34, 0, 10, 255}, {255, 60, 70, 255}, true, false};
    case HitFx::Light:  // Seal of light: gold and white
      return {{255, 186, 70, 255}, {255, 214, 120, 255}, {255, 255, 246, 255}, {255, 248, 200, 255}, false, false};
    case HitFx::Gloom:  // malice breath: red-black smoke
      return {{200, 16, 46, 255}, {54, 2, 14, 255}, {255, 96, 116, 255}, {255, 70, 90, 255}, true, true};
    case HitFx::Hollow:  // Cero Oscuras: black, edged with green
      return {{60, 230, 120, 255}, {6, 12, 8, 255}, {30, 80, 46, 255}, {120, 255, 170, 255}, true, false};
    case HitFx::Fire:  // Wyvern's Fire
      return {{255, 90, 20, 255}, {255, 140, 40, 255}, {255, 248, 210, 255}, {255, 200, 90, 255}, false, true};
    case HitFx::Wolf:  // Starrk's cero: pale blue, white heart
      return {{60, 120, 255, 255}, {120, 176, 255, 255}, {246, 250, 255, 255}, {200, 226, 255, 255}, false, false};
    case HitFx::Claw:  // Gran Rey Cero: cyan, his blood swirling in it
      return {{20, 170, 230, 255}, {70, 220, 250, 255}, {240, 255, 255, 255}, {170, 250, 255, 255}, false, false};
    default: {
      const Color c = hitFxColor(fx), g = hitFxGlow(fx);
      return {c, c, WHITE, g, false, false};
    }
  }
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

// GOJO: Infinite Void. A bottomless cosmos, stars, and the endless information streaming at you.
void voidBack(const Fighter& f, float k, float t, const Rect& v) {
  const Vec2 c = f.center();
  fillView(v, A({3, 5, 18, 255}, 0.94f * k), A({12, 6, 34, 255}, 0.94f * k));
  vfx::glow(c, 1100.0f, A({30, 56, 150, 255}, 0.55f * k), A({30, 56, 150, 255}, 0.0f), 40);
  vfx::glow(c + Vec2{-420.0f, -220.0f}, 520.0f, A({110, 40, 170, 255}, 0.3f * k), A({110, 40, 170, 255}, 0.0f), 28);
  vfx::glow(c + Vec2{460.0f, 160.0f}, 460.0f, A({20, 120, 170, 255}, 0.28f * k), A({20, 120, 170, 255}, 0.0f), 28);
  for (int i = 0; i < 110; ++i) {  // stars
    const Vec2 p{v.x + hash01(i * 3 + 1) * v.w, v.y + hash01(i * 3 + 2) * v.h};
    const float tw = 0.45f + 0.55f * std::sin(t * (2.0f + 3.0f * hash01(i * 5 + 3)) + i);
    DrawPoly(V(p), 4, 1.2f + 2.4f * hash01(i * 7 + 4), 45.0f, A(i % 5 ? WHITE : Color{150, 210, 255, 255}, k * tw));
  }
  vfx::additive(true);
  for (int i = 0; i < 72; ++i) {  // information streams: warp lines rushing out of the centre
    const float ang = hash01(i * 7 + 3) * 2.0f * kPi;
    const float sp = 0.35f + hash01(i * 7 + 4) * 0.7f;
    const float ph = std::fmod(t * sp + hash01(i * 7 + 5), 1.0f);
    const Vec2 d = dirOf(ang);
    const float r0 = 70.0f + ph * ph * 1500.0f, l = 30.0f + ph * 300.0f, w = 1.0f + ph * 4.0f;
    const Color col = i % 3 == 0 ? WHITE : i % 3 == 1 ? Color{120, 220, 255, 255} : Color{190, 150, 255, 255};
    const float a = k * (0.2f + 0.7f * ph) * std::min(1.0f, (1.0f - ph) * 4.0f);
    vfx::wedge(c + d * r0, c + d * (r0 + l), w * 0.2f, w, A(col, 0.1f * a), A(col, a));
  }
  const float R = 240.0f + 14.0f * std::sin(t * 2.0f);  // the horizon of the void
  vfx::softRing(c, R, 70.0f, A({120, 190, 255, 255}, 0.32f * k));
  vfx::softRing(c, R, 10.0f, A(WHITE, 0.55f * k));
  vfx::softRing(c, R * 1.9f, 40.0f, A({150, 120, 255, 255}, 0.18f * k));
  vfx::glow(c, 170.0f, A({210, 235, 255, 255}, 0.3f * k), A({210, 235, 255, 255}, 0.0f));
  vfx::additive(false);
}

// SUKUNA: Malevolent Shrine. Blood-red sky, the shrine of bones, a pool of blood.
void shrine(float x, float base, float s, Color ink, Color rim) {
  auto box = [&](float cx, float y0, float w, float h) {  // y0 = bottom
    vfx::quad({cx - w * 0.5f, y0 - h}, {cx + w * 0.5f, y0 - h}, {cx + w * 0.5f, y0}, {cx - w * 0.5f, y0}, ink, ink, ink, ink);
  };
  auto roof = [&](float cx, float y0, float w, float h) {  // wide roof with up-turned eaves
    const float hw = w * 0.5f;
    vfx::quad({cx - hw, y0}, {cx + hw, y0}, {cx + hw * 0.42f, y0 - h}, {cx - hw * 0.42f, y0 - h}, ink, ink, ink, ink);
    vfx::tri({cx - hw - 26 * s, y0 - 22 * s}, {cx - hw + 30 * s, y0}, {cx - hw + 40 * s, y0 - h * 0.45f}, ink, ink, ink);
    vfx::tri({cx + hw + 26 * s, y0 - 22 * s}, {cx + hw - 30 * s, y0}, {cx + hw - 40 * s, y0 - h * 0.45f}, ink, ink, ink);
    DrawLineEx({cx - hw - 26 * s, y0 - 22 * s}, {cx - hw + 30 * s, y0}, 3.0f * s, rim);
    DrawLineEx({cx + hw + 26 * s, y0 - 22 * s}, {cx + hw - 30 * s, y0}, 3.0f * s, rim);
    DrawLineEx({cx - hw + 30 * s, y0}, {cx + hw - 30 * s, y0}, 3.0f * s, rim);
  };
  // Platform of the shrine (a jaw: two rows of teeth around a glowing maw)
  box(x, base, 700 * s, 46 * s);
  box(x, base - 46 * s, 560 * s, 150 * s);
  const float my = base - 46 * s - 75 * s;
  glowE({x, my}, 190 * s, 58 * s, A({255, 40, 50, 255}, rim.a / 255.0f), A({120, 0, 10, 255}, rim.a / 255.0f));
  for (int i = 0; i < 9; ++i) {
    const float tx = x - 170 * s + i * 42.5f * s;
    vfx::tri({tx - 14 * s, my - 52 * s}, {tx + 14 * s, my - 52 * s}, {tx, my - 14 * s}, A({236, 220, 200, 255}, rim.a / 255.0f),
             A({236, 220, 200, 255}, rim.a / 255.0f), A({200, 170, 150, 255}, rim.a / 255.0f));
    vfx::tri({tx - 12 * s, my + 52 * s}, {tx + 12 * s, my + 52 * s}, {tx, my + 18 * s}, A({236, 220, 200, 255}, rim.a / 255.0f),
             A({236, 220, 200, 255}, rim.a / 255.0f), A({200, 170, 150, 255}, rim.a / 255.0f));
  }
  // Three storeys of roofs
  float y = base - 196 * s;
  const float widths[3] = {680, 520, 360};
  for (int i = 0; i < 3; ++i) {
    const float w = widths[i] * s;
    box(x, y, w * 0.62f, 54 * s);  // storey
    for (int p = -2; p <= 2; ++p) DrawLineEx({x + p * w * 0.13f, y}, {x + p * w * 0.13f, y - 54 * s}, 2.0f * s, A(rim, rim.a / 400.0f));
    y -= 54 * s;
    roof(x, y, w, 70 * s);
    y -= 66 * s;
  }
  box(x, y, 14 * s, 90 * s);  // spire
  vfx::glow({x, y - 96 * s}, 16 * s, rim, A(rim, 0.0f), 12);
  // Skulls piled at the foot
  for (int i = 0; i < 12; ++i) {
    const float sx = x + (i < 6 ? -1.0f : 1.0f) * (300 + (i % 6) * 24) * s, sy = base - 8 * s - (i % 3) * 14 * s;
    const Color bone = A({226, 214, 196, 255}, rim.a / 255.0f);
    DrawCircleV({sx, sy}, 11 * s, bone);
    DrawCircleV({sx - 4 * s, sy}, 3 * s, ink);
    DrawCircleV({sx + 4 * s, sy}, 3 * s, ink);
  }
}

void shrineBack(const Fighter& f, float k, float t, const Rect& v) {
  const Vec2 c = f.center();
  const float base = f.pos.y + 150.0f;
  fillView(v, A({30, 0, 6, 255}, 0.94f * k), A({150, 14, 24, 255}, 0.94f * k));
  vfx::glow({c.x, base - 470.0f}, 620.0f, A({255, 40, 40, 255}, 0.45f * k), A({255, 40, 40, 255}, 0.0f), 36);
  vfx::glow({c.x, base - 470.0f}, 150.0f, A({255, 200, 170, 255}, 0.5f * k), A({255, 120, 100, 255}, 0.0f), 24);  // red moon
  // Pool of blood under the shrine
  vfx::quad({v.x, base}, {v.right(), base}, {v.right(), v.bottom()}, {v.x, v.bottom()}, A({110, 0, 12, 255}, 0.95f * k),
            A({110, 0, 12, 255}, 0.95f * k), A({30, 0, 4, 255}, 0.98f * k), A({30, 0, 4, 255}, 0.98f * k));
  for (int i = 0; i < 6; ++i) {  // ripples on the blood
    const float u = std::fmod(t * 0.4f + i / 6.0f, 1.0f);
    glowE({c.x, base + 30.0f + u * 60.0f}, 140.0f + u * 700.0f, 6.0f + u * 30.0f, A({255, 60, 60, 255}, 0.0f),
          A({255, 80, 80, 255}, 0.16f * k * (1.0f - u)), 28);
  }
  shrine(c.x, base, 1.0f, A({16, 2, 6, 255}, 0.97f * k), A({230, 40, 52, 255}, 0.85f * k));
}

void shrineFront(const Match& m, const Fighter& f, float k, int mf, const Rect& v) {
  // Countless invisible cuts: thin white slashes flashing everywhere.
  const int n = mf < 34 ? 10 : 34;
  vfx::additive(true);
  for (int i = 0; i < n; ++i) {
    const int cyc = 7, ph = (m.frame + i * 5) % cyc;
    const uint32_t s = uint32_t((m.frame + i * 5) / cyc) * 977u + i * 131u + f.id * 7u;
    const Vec2 p{v.x + hash01(s) * v.w, v.y + hash01(s + 1) * v.h};
    const float ang = hash01(s + 2) * kPi, len = 90.0f + hash01(s + 3) * 320.0f;
    const Vec2 d = dirOf(ang) * (len * 0.5f * std::min(1.0f, (ph + 1) / 2.0f));
    const float a = (1.0f - float(ph) / cyc) * k;
    vfx::streak(p - d, p + d, 2.0f, 9.0f, A({255, 60, 70, 255}, 0.55f * a), A({255, 60, 70, 255}, 0.55f * a));
    vfx::wedge(p - d, p + d, 0.5f, 2.6f, A(WHITE, a), A(WHITE, a));
  }
  vfx::additive(false);
}

// BYAKUYA: Senbonzakura Kageyoshi. A rose night, rows of blades, a storm of petals.
void petalBack(const Fighter& f, float k, float t, const Rect& v) {
  const Vec2 c = f.center();
  fillView(v, A({26, 8, 22, 255}, 0.86f * k), A({96, 34, 70, 255}, 0.86f * k));
  vfx::glow(c, 900.0f, A({255, 140, 190, 255}, 0.3f * k), A({255, 140, 190, 255}, 0.0f), 36);
  // Senkei: a ring of a thousand blades standing around him (the back half)
  vfx::additive(true);
  for (int i = 0; i < 18; ++i) {
    const float a = i * 2.0f * kPi / 18.0f + t * 0.25f;
    if (std::sin(a) > 0.15f) continue;  // the front half is drawn by petalFront
    const Vec2 base = c + Vec2{std::cos(a) * 520.0f, 150.0f + std::sin(a) * 90.0f};
    const float h = 300.0f + 40.0f * std::sin(a * 3.0f);
    vfx::wedge(base, base + Vec2{0.0f, -h}, 16.0f, 2.0f, A({255, 190, 220, 255}, 0.35f * k), A(WHITE, 0.6f * k));
    vfx::glow(base + Vec2{0.0f, -h * 0.5f}, 60.0f, A({255, 150, 200, 255}, 0.14f * k), A({255, 150, 200, 255}, 0.0f), 12);
  }
  vfx::additive(false);
}

void petal(Vec2 p, Vec2 d, float len, float w, Color col) {
  const Vec2 n{-d.y, d.x};
  vfx::quad(p - d * len, p + n * w, p + d * len, p - n * w, col, col, mixColor(col, WHITE, 0.5f), col);
}

void petalFront(const Fighter& f, float k, float t) {
  const Vec2 c = f.center();
  for (int i = 0; i < 150; ++i) {  // the vortex of petal blades
    const float r = 110.0f + hash01(i * 11 + 1) * 560.0f;
    const float w = (1.1f + 1.8f * hash01(i * 11 + 2)) * (220.0f / (r + 120.0f)) * 1.6f;
    const float a = hash01(i * 11 + 3) * 2.0f * kPi + t * w;
    const Vec2 p = c + Vec2{std::cos(a) * r, std::sin(a) * r * 0.55f - 40.0f};
    const Vec2 d = Vec2{-std::sin(a), std::cos(a) * 0.55f}.norm();
    const Color col = i % 4 == 0 ? Color{255, 236, 244, 255} : i % 4 == 1 ? Color{240, 110, 160, 255} : Color{255, 160, 196, 255};
    petal(p, d, 6.0f + 6.0f * hash01(i * 11 + 4), 2.2f + 2.0f * hash01(i * 11 + 5), A(col, 0.9f * k));
  }
  vfx::additive(true);
  for (int i = 0; i < 18; ++i) {  // front half of the ring of blades
    const float a = i * 2.0f * kPi / 18.0f + t * 0.25f;
    if (std::sin(a) <= 0.15f) continue;
    const Vec2 base = c + Vec2{std::cos(a) * 520.0f, 150.0f + std::sin(a) * 90.0f};
    const float h = 300.0f + 40.0f * std::sin(a * 3.0f);
    vfx::wedge(base, base + Vec2{0.0f, -h}, 16.0f, 2.0f, A({255, 190, 220, 255}, 0.3f * k), A(WHITE, 0.55f * k));
  }
  for (int r = 0; r < 4; ++r) {  // swirling ribbons
    Vec2 pts[20];
    for (int i = 0; i < 20; ++i) {
      const float u = i / 19.0f, rad = 160.0f + r * 120.0f;
      const float a = t * (1.6f - r * 0.2f) + r * 1.7f + u * 2.4f;
      pts[i] = c + Vec2{std::cos(a) * rad, std::sin(a) * rad * 0.55f - 40.0f};
    }
    polyline(pts, 20, 1.0f, 9.0f, A({255, 150, 200, 255}, 0.0f), A({255, 190, 220, 255}, 0.35f * k));
  }
  vfx::additive(false);
}

// ASTA: the black storm of anti-magic, the five-leaf clover.
void stormBack(const Fighter& f, float k, float t, const Rect& v, int frame) {
  const Vec2 c = f.center();
  fillView(v, A({8, 8, 12, 255}, 0.93f * k), A({44, 40, 52, 255}, 0.93f * k));
  for (int i = 0; i < 10; ++i) {  // swirling storm clouds
    const float a = i * 2.0f * kPi / 10.0f + t * 0.35f;
    const Vec2 p = c + Vec2{std::cos(a) * 560.0f, std::sin(a) * 260.0f - 120.0f};
    const float r = 230.0f + 90.0f * hash01(i * 3);
    vfx::glow(p, r, A(i % 2 ? Color{30, 28, 36, 255} : Color{58, 54, 66, 255}, 0.6f * k), A({30, 28, 36, 255}, 0.0f), 22);
  }
  // The five-leaf clover of the grimoire, dark, turning slowly
  const Vec2 cc = c + Vec2{0.0f, -40.0f};
  const float rot = t * 0.3f;
  for (int pass = 0; pass < 2; ++pass) {
    const Color col = pass == 0 ? A({120, 118, 130, 255}, 0.5f * k) : A({6, 6, 8, 255}, 0.92f * k);
    const float grow = pass == 0 ? 5.0f : 0.0f;
    for (int i = 0; i < 5; ++i) {
      const float a = rot + i * 2.0f * kPi / 5.0f - kPi * 0.5f;
      const Vec2 d = dirOf(a), n{-d.y, d.x};
      vfx::glow(cc + d * 170.0f + n * 52.0f, 74.0f + grow, col, col, 22);
      vfx::glow(cc + d * 170.0f - n * 52.0f, 74.0f + grow, col, col, 22);
      vfx::tri(cc + d * (20.0f - grow), cc + d * 190.0f + n * (118.0f + grow), cc + d * 190.0f - n * (118.0f + grow), col, col, col);
    }
  }
  // Black lightning
  for (int i = 0; i < 4; ++i) {
    const int cyc = 6, ph = (frame + i * 3) % cyc;
    if (ph > 3) continue;
    const uint32_t s = uint32_t((frame + i * 3) / cyc) * 613u + i * 97u;
    const Vec2 a{v.x + hash01(s) * v.w, v.y - 20.0f};
    const Vec2 b{a.x + (hash01(s + 1) - 0.5f) * 500.0f, v.y + v.h * (0.5f + 0.5f * hash01(s + 2))};
    const float fa = k * (1.0f - ph / 4.0f);
    vfx::bolt(a, b, 70.0f, 12, s, 11.0f, A({210, 210, 225, 255}, 0.55f * fa));
    vfx::bolt(a, b, 70.0f, 12, s, 6.0f, A({4, 4, 6, 255}, fa));
  }
}

void stormFront(const Fighter& f, float k, float t) {
  const Vec2 c = f.center();
  for (int i = 0; i < 46; ++i) {  // black flakes of anti-magic swirling around
    const float r = 120.0f + hash01(i * 7 + 1) * 480.0f;
    const float a = hash01(i * 7 + 2) * 2.0f * kPi + t * (2.4f - r / 400.0f);
    const Vec2 p = c + Vec2{std::cos(a) * r, std::sin(a) * r * 0.5f};
    DrawPoly(V(p), 4, 3.0f + 4.0f * hash01(i * 7 + 3), a * 120.0f, A(i % 4 ? Color{10, 10, 14, 255} : Color{230, 230, 240, 255}, 0.9f * k));
  }
}

// YUNO: the wind spirit Sylph rises behind him, spirals of wind and leaves.
void windBack(const Fighter& f, float k, float t, const Rect& v) {
  const Vec2 c = f.center();
  const float fc = float(f.facing);
  fillView(v, A({12, 44, 44, 255}, 0.86f * k), A({60, 150, 120, 255}, 0.86f * k));
  vfx::glow(c, 900.0f, A({170, 255, 210, 255}, 0.3f * k), A({170, 255, 210, 255}, 0.0f), 36);
  vfx::additive(true);
  for (int arm = 0; arm < 6; ++arm) {  // spirals
    Vec2 pts[24];
    for (int i = 0; i < 24; ++i) {
      const float u = i / 23.0f;
      const float a = arm * kPi / 3.0f + t * 1.4f + u * 3.2f;
      const float r = 60.0f + u * 760.0f;
      pts[i] = c + Vec2{std::cos(a) * r, std::sin(a) * r * 0.6f};
    }
    polyline(pts, 24, 2.0f, 16.0f, A({220, 255, 235, 255}, 0.4f * k), A({140, 240, 190, 255}, 0.0f));
  }
  // Sylph: a giant translucent spirit with open arms and wings
  const Vec2 s = c + Vec2{-fc * 30.0f, -150.0f + std::sin(t * 1.5f) * 10.0f};
  const float S = 1.0f;
  const Color sp = A({190, 255, 225, 255}, 0.24f * k), spz = A({190, 255, 225, 255}, 0.0f);
  for (int side = -1; side <= 1; side += 2) {  // wings
    for (int fth = 0; fth < 4; ++fth) {
      const float a = -kPi * 0.5f + side * (0.5f + fth * 0.28f) + std::sin(t * 3.0f) * 0.08f * side;
      const Vec2 root = s + Vec2{side * 24.0f, -40.0f};
      vfx::streak(root, root + dirOf(a) * (300.0f - fth * 40.0f) * S, 60.0f, 10.0f, sp, spz);
    }
  }
  glowE(s + Vec2{0.0f, -200.0f * S}, 42.0f * S, 48.0f * S, A({230, 255, 240, 255}, 0.4f * k), spz, 20);  // head
  glowE(s + Vec2{0.0f, -60.0f * S}, 70.0f * S, 120.0f * S, sp, spz, 22);                                // body
  for (int side = -1; side <= 1; side += 2)                                                               // open arms
    vfx::streak(s + Vec2{side * 40.0f, -130.0f}, s + Vec2{side * 260.0f, -190.0f + std::sin(t * 2.0f + side) * 20.0f}, 34.0f,
                10.0f, sp, A(sp, 0.12f * k));
  for (int i = 0; i < 5; ++i) {  // hair streaming in the wind
    const Vec2 h0 = s + Vec2{0.0f, -215.0f};
    const Vec2 h1 = h0 + Vec2{-fc * (170.0f + i * 40.0f), -40.0f + i * 34.0f + std::sin(t * 4.0f + i) * 26.0f};
    vfx::streak(h0, h1, 26.0f, 4.0f, A({220, 255, 235, 255}, 0.3f * k), spz);
  }
  Vec2 tail[10];  // the dress dissolving into a whirlwind
  for (int i = 0; i < 10; ++i) {
    const float u = i / 9.0f;
    tail[i] = s + Vec2{std::sin(t * 3.0f + u * 5.0f) * 50.0f * u, 40.0f + u * 330.0f};
  }
  polyline(tail, 10, 120.0f, 8.0f, sp, spz);
  vfx::additive(false);
}

void windFront(const Fighter& f, float k, float t, const Rect& v) {
  const Vec2 c = f.center();
  vfx::additive(true);
  for (int i = 0; i < 26; ++i) {  // gusts crossing the screen
    const float u = std::fmod(t * (0.8f + hash01(i * 3) * 0.8f) + hash01(i * 3 + 1), 1.0f);
    const float y = v.y + hash01(i * 3 + 2) * v.h;
    const float x = v.x + u * (v.w + 600.0f) - 300.0f;
    vfx::streak({x - 260.0f, y}, {x, y + std::sin(t * 3.0f + i) * 10.0f}, 1.0f, 4.0f, A({220, 255, 235, 255}, 0.0f),
                A({220, 255, 235, 255}, 0.4f * k));
  }
  vfx::additive(false);
  for (int i = 0; i < 40; ++i) {  // leaves riding the vortex
    const float r = 90.0f + hash01(i * 5 + 1) * 520.0f;
    const float a = hash01(i * 5 + 2) * 2.0f * kPi + t * (3.0f - r / 300.0f);
    const Vec2 p = c + Vec2{std::cos(a) * r, std::sin(a) * r * 0.5f};
    petal(p, dirOf(a + t * 3.0f + i), 7.0f, 3.0f, A(i % 3 ? Color{90, 200, 110, 255} : Color{200, 250, 170, 255}, 0.9f * k));
  }
}

// LINK: a golden seal of light with Zonai runes.
void sealBack(const Fighter& f, float k, int mf, float t, const Rect& v) {
  const Vec2 c = f.center();
  fillView(v, A({48, 30, 8, 255}, 0.8f * k), A({18, 12, 6, 255}, 0.8f * k));
  vfx::additive(true);
  vfx::glow(c, 1000.0f, A({255, 190, 80, 255}, 0.45f * k), A({255, 190, 80, 255}, 0.0f), 40);
  vfx::burst(c, 20, 60.0f, 1500.0f, 0.1f, t * 0.15f, A({255, 220, 140, 255}, 0.22f * k), A({255, 200, 100, 255}, 0.0f));
  const float R = 250.0f * (0.5f + 0.5f * smoothstep(mf / 22.0f));
  const Color gold = A({255, 226, 140, 255}, 0.85f * k), teal = A(kZonaiGlow, 0.8f * k);
  vfx::softRing(c, R, 34.0f, A({255, 200, 90, 255}, 0.45f * k));
  vfx::additive(false);
  DrawRing(V(c), R - 3.0f, R + 3.0f, 0, 360, 72, gold);
  DrawRing(V(c), R * 1.24f - 2.0f, R * 1.24f + 2.0f, 0, 360, 72, gold);
  DrawRing(V(c), R * 0.62f - 1.5f, R * 0.62f + 1.5f, 0, 360, 60, A(gold, 0.6f * k));
  for (int i = 0; i < 60; ++i) {  // ticks between the outer rings
    const Vec2 d = dirOf(-t * 0.4f + i * 2.0f * kPi / 60.0f);
    DrawLineEx(V(c + d * (R * 1.04f)), V(c + d * (R * (i % 5 ? 1.12f : 1.2f))), 2.0f, gold);
  }
  for (int tr = 0; tr < 2; ++tr) {  // hexagram
    Vector2 p[4];
    for (int i = 0; i < 4; ++i) {
      const Vec2 d = dirOf(t * 0.5f + tr * kPi / 3.0f + i * 2.0f * kPi / 3.0f);
      p[i] = V(c + d * (R * 0.98f));
    }
    for (int i = 0; i < 3; ++i) DrawLineEx(p[i], p[i + 1], 3.0f, A(gold, 0.75f * k));
  }
  for (int i = 0; i < 8; ++i) {  // Zonai runes riding the seal
    const Vec2 d = dirOf(t * 0.5f + i * kPi / 4.0f + kPi / 8.0f);
    ui::rune(V(c + d * (R * 0.8f)), 20.0f, i, t * 2.0f, i % 2 ? teal : gold, 2.5f);
  }
}

void sealFront(const Fighter& f, float k, float t) {
  const Vec2 c = f.center();
  vfx::additive(true);
  for (int i = 0; i < 50; ++i) {  // motes of light rising
    const float u = std::fmod(t * (0.25f + 0.3f * hash01(i * 3)) + hash01(i * 3 + 1), 1.0f);
    const Vec2 p = c + Vec2{(hash01(i * 3 + 2) - 0.5f) * 900.0f + std::sin(t * 2.0f + i) * 16.0f, 300.0f - u * 700.0f};
    const float a = k * std::sin(u * kPi);
    vfx::glow(p, 9.0f, A({255, 230, 150, 255}, 0.5f * a), A({255, 230, 150, 255}, 0.0f), 10);
    DrawPoly(V(p), 4, 2.4f, 45.0f, A(WHITE, a));
  }
  vfx::additive(false);
}

// GANONDORF: gloom. Red-black miasma and the Demon Dragon coiling above him.
void gloomBack(const Fighter& f, float k, float t, const Rect& v) {
  const Vec2 c = f.center();
  const float fc = float(f.facing);
  fillView(v, A({16, 0, 6, 255}, 0.93f * k), A({64, 6, 18, 255}, 0.93f * k));
  for (int i = 0; i < 12; ++i) {  // miasma clouds
    const Vec2 p = c + Vec2{(hash01(i * 5 + 1) - 0.5f) * 1700.0f + std::sin(t * 0.7f + i) * 60.0f,
                            (hash01(i * 5 + 2) - 0.6f) * 900.0f + std::cos(t * 0.5f + i) * 40.0f};
    const float r = 200.0f + 160.0f * hash01(i * 5 + 3);
    const Color col = i % 3 ? Color{130, 10, 34, 255} : Color{6, 0, 4, 255};
    vfx::glow(p, r * (1.0f + 0.08f * std::sin(t * 2.0f + i)), A(col, 0.45f * k), A(col, 0.0f), 22);
  }
  // The Demon Dragon: a coiling body of gloom, red-rimmed, head above the Demon King
  const Vec2 head = c + Vec2{fc * 120.0f, -330.0f + std::sin(t * 1.8f) * 14.0f};
  constexpr int kN = 30;
  Vec2 body[kN];
  float rad[kN];
  for (int i = 0; i < kN; ++i) {
    const float s = i / float(kN - 1);
    body[i] = head + Vec2{-fc * (s * 1500.0f), s * 260.0f + std::sin(s * 7.0f - t * 2.2f) * 130.0f * s};
    rad[i] = 70.0f * (1.0f - 0.6f * s) + 10.0f;
  }
  const Color ink = A({12, 2, 6, 255}, 0.97f * k), rim = A({220, 24, 48, 255}, 0.85f * k);
  for (int i = kN - 1; i >= 0; --i) vfx::glow(body[i], rad[i] + 6.0f, rim, rim, 18);
  for (int i = kN - 1; i >= 0; --i) vfx::glow(body[i], rad[i], ink, ink, 18);
  for (int i = 2; i < kN - 2; i += 2) {  // spines along the back
    const Vec2 d = (body[i - 1] - body[i + 1]).norm();
    Vec2 n{-d.y, d.x};
    if (n.y > 0.0f) n = -n;
    const Vec2 b = body[i] + n * (rad[i] * 0.8f);
    vfx::tri(b + d * (rad[i] * 0.4f), b - d * (rad[i] * 0.4f), b + n * (rad[i] * 0.9f) + d * (rad[i] * 0.3f), ink, ink, rim);
  }
  // Head: jaw, horns, glowing eyes, maw full of red light
  const Vec2 fw{fc, 0.25f};
  vfx::tri(head + Vec2{0, -60}, head + Vec2{0, 50}, head + fw * 190.0f + Vec2{0, -10}, ink, ink, ink);
  vfx::tri(head + Vec2{0, 30}, head + Vec2{0, 70}, head + fw * 150.0f + Vec2{0, 60}, ink, ink, ink);
  vfx::additive(true);
  vfx::glow(head + fw * 110.0f + Vec2{0, 32}, 46.0f, A({255, 70, 60, 255}, 0.8f * k), A({255, 30, 40, 255}, 0.0f), 16);
  vfx::additive(false);
  for (int side = 0; side < 2; ++side) {  // horns
    const Vec2 r0 = head + Vec2{fc * (10.0f + side * 30.0f), -50.0f};
    vfx::wedge(r0, r0 + Vec2{-fc * (150.0f - side * 30.0f), -110.0f + side * 20.0f}, 24.0f, 2.0f, ink, rim);
  }
  vfx::additive(true);
  for (int e = 0; e < 2; ++e) {
    const Vec2 ep = head + Vec2{fc * (52.0f + e * 26.0f), -24.0f + e * 4.0f};
    vfx::glow(ep, 26.0f, A({255, 60, 40, 255}, 0.9f * k), A({255, 60, 40, 255}, 0.0f), 14);
    vfx::glow(ep, 7.0f, A({255, 240, 160, 255}, k), A({255, 200, 120, 255}, 0.0f), 10);
  }
  vfx::additive(false);
}

void gloomFront(const Fighter& f, float k, float t, const Rect& v) {
  (void)f;
  vfx::additive(true);
  for (int i = 0; i < 9; ++i) {  // tendrils of malice rising from below
    Vec2 pts[12];
    const float x0 = v.x + (i + 0.5f) * v.w / 9.0f;
    for (int j = 0; j < 12; ++j) {
      const float u = j / 11.0f;
      pts[j] = {x0 + std::sin(t * 2.0f + i * 1.3f + u * 5.0f) * 50.0f * u, v.bottom() - u * v.h * (0.35f + 0.2f * hash01(i))};
    }
    polyline(pts, 12, 26.0f, 2.0f, A({200, 20, 50, 255}, 0.4f * k), A({255, 60, 80, 255}, 0.0f));
  }
  for (int i = 0; i < 36; ++i) {  // embers of malice
    const float u = std::fmod(t * (0.3f + 0.3f * hash01(i)) + hash01(i + 50), 1.0f);
    const Vec2 p{v.x + hash01(i + 100) * v.w + std::sin(t * 2.0f + i) * 20.0f, v.bottom() - u * v.h};
    vfx::glow(p, 5.0f, A({255, 60, 70, 255}, 0.8f * k * std::sin(u * kPi)), A({255, 60, 70, 255}, 0.0f), 8);
  }
  vfx::additive(false);
}

// KRATOS: Spartan rage. A world on fire, the omega of Sparta burning behind him.
void rageBack(const Fighter& f, float k, float t, const Rect& v) {
  const Vec2 c = f.center();
  fillView(v, A({36, 4, 0, 255}, 0.88f * k), A({130, 30, 4, 255}, 0.88f * k));
  vfx::additive(true);
  vfx::glow(c, 800.0f, A({255, 90, 20, 255}, 0.4f * k), A({255, 90, 20, 255}, 0.0f), 36);
  // Omega
  const Vec2 o = c + Vec2{0.0f, -60.0f};
  const float R = 230.0f;
  Vec2 arc[26];
  for (int i = 0; i < 26; ++i) {
    const float a = kPi * 0.5f + 0.62f + i / 25.0f * (2.0f * kPi - 1.24f);
    arc[i] = o + dirOf(a) * R;
  }
  const Color red = A({255, 60, 30, 255}, (0.4f + 0.1f * std::sin(t * 8.0f)) * k);
  polyline(arc, 26, 34.0f, 34.0f, red, red);
  vfx::wedge(arc[0], arc[0] + Vec2{-R * 0.5f, 0.0f}, 34.0f, 34.0f, red, red);
  vfx::wedge(arc[25], arc[25] + Vec2{R * 0.5f, 0.0f}, 34.0f, 34.0f, red, red);
  vfx::additive(false);
}

void rageFront(const Fighter& f, float k, float t, const Rect& v) {
  (void)f;
  vfx::additive(true);
  const int n = 20;
  for (int layer = 0; layer < 2; ++layer) {  // wall of flames rising from below
    for (int i = 0; i < n; ++i) {
      const float w = v.w / n;
      const float x = v.x + (i + 0.5f) * w + (layer ? w * 0.5f : 0.0f);
      const float h = v.h * (0.22f + 0.12f * std::sin(t * 7.0f + i * 1.7f + layer) + 0.1f * hash01(i * 3 + layer)) * (layer ? 0.7f : 1.0f);
      const Color base = layer ? Color{255, 220, 120, 255} : Color{255, 90, 20, 255};
      vfx::tri({x - w * 0.9f, v.bottom()}, {x + w * 0.9f, v.bottom()}, {x + std::sin(t * 5.0f + i) * 30.0f, v.bottom() - h},
               A(base, 0.55f * k), A(base, 0.55f * k), A(base, 0.0f));
    }
  }
  for (int i = 0; i < 50; ++i) {  // embers
    const float u = std::fmod(t * (0.35f + 0.4f * hash01(i * 3)) + hash01(i * 3 + 1), 1.0f);
    const Vec2 p{v.x + hash01(i * 3 + 2) * v.w + std::sin(t * 3.0f + i) * 30.0f, v.bottom() - u * v.h};
    vfx::glow(p, 6.0f, A({255, 170, 60, 255}, 0.9f * k * std::sin(u * kPi)), A({255, 120, 30, 255}, 0.0f), 8);
  }
  vfx::additive(false);
}
}  // namespace

// Beams drawn by a PixelLab effect: a segment that tiles along the beam (fx_<name>) and its end (fx_<name>_head), both
// sized on the beam's width (the effect's hit radius is half the beam's width). The beams by element.
static const char* beamSprite(HitFx fx) {
  switch (fx) {
    case HitFx::Claw: return "gran_rey_cero";
    case HitFx::Hollow: return "cero_oscuras";
    case HitFx::Blood: return "vasto_cero";
    case HitFx::Light: return "light_dragon";
    case HitFx::Gloom: return "demon_dragon";
    case HitFx::Fire: return "wyvern_fire";
    default: return nullptr;
  }
}

static bool drawSpriteBeam(Vec2 a, Vec2 dir, float len, float w, HitFx fx, float time) {
  const char* name = beamSprite(fx);
  const art::FxInfo* seg = name ? art::fxInfo(name) : nullptr;
  if (!seg) return false;
  const float size = 0.5f * w / (seg->hit > 0.0f ? seg->hit : 0.4f * seg->fh);
  art::drawFxStrip(name, time, a, dir, len, size);
  const std::string head = std::string(name) + "_head";
  if (art::fxInfo(head.c_str())) {
    const bool flip = dir.x < 0.0f;
    art::drawFx(head.c_str(), time, a + dir * len, size, flip ? std::atan2(-dir.y, -dir.x) : std::atan2(dir.y, dir.x), flip);
  }
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
    if (drawSpriteBeam(a, dir, len, b.width * env * pulse, b.fx, time)) continue;
    drawBeam(a, dir, len, b.width * env * pulse, beamStyle(b.fx), time, uint32_t(f.id * 977 + 13));
  }
}

void drawUltimateBackdrop(const Match& m, float time) {
  const Rect v = vfx::viewRect(40.0f);
  forEachUltimate(m, [&](const Fighter& f, float k, int mf) {
    const UltFx fx = f.moveDef(FinalSmash).ultFx;
    switch (fx) {
      case UltFx::InfiniteVoid: voidBack(f, k, time, v); break;
      case UltFx::MalevolentShrine: shrineBack(f, k, time, v); break;
      case UltFx::Kageyoshi: case UltFx::Gokei: petalBack(f, k, time, v); break;
      case UltFx::BlackDivider: stormBack(f, k, time, v, m.frame); break;
      case UltFx::SpiritStorm: case UltFx::WindBladeShower: case UltFx::ZephyrSpear: windBack(f, k, time, v); break;
      case UltFx::LightDragon: case UltFx::MasterSwordBeam:
        sealBack(f, k, mf, time, v);
        drawUltCinematic(m, f, fx, k, mf, time, v, false);
        break;
      case UltFx::DemonDragon: gloomBack(f, k, time, v); break;
      case UltFx::SpartanRage: case UltFx::ChaosWhip: rageBack(f, k, time, v); break;
      default: drawUltCinematic(m, f, fx, k, mf, time, v, false); break;
    }
  });
}

void drawUltimateFront(const Match& m, float time) {
  const Rect v = vfx::viewRect(40.0f);
  bool any = false;
  forEachUltimate(m, [&](const Fighter& f, float k, int mf) {
    any = true;
    const UltFx fx = f.moveDef(FinalSmash).ultFx;
    switch (fx) {
      case UltFx::MalevolentShrine: shrineFront(m, f, k, mf, v); break;
      case UltFx::Kageyoshi: case UltFx::Gokei: petalFront(f, k, time); break;
      case UltFx::BlackDivider: stormFront(f, k, time); break;
      case UltFx::SpiritStorm: case UltFx::ZephyrSpear: windFront(f, k, time, v); break;
      case UltFx::WindBladeShower:
        windFront(f, k, time, v);
        drawUltCinematic(m, f, fx, k, mf, time, v, true);
        break;
      case UltFx::LightDragon: case UltFx::MasterSwordBeam:
        sealFront(f, k, time);
        drawUltCinematic(m, f, fx, k, mf, time, v, true);
        break;
      case UltFx::DemonDragon: case UltFx::DemonKingRampage: case UltFx::GloomPillars: gloomFront(f, k, time, v); break;
      case UltFx::SpartanRage: rageFront(f, k, time, v); break;
      case UltFx::ChaosWhip:
        rageFront(f, k, time, v);
        drawUltCinematic(m, f, fx, k, mf, time, v, true);
        break;
      case UltFx::InfiniteVoid: break;
      default: drawUltCinematic(m, f, fx, k, mf, time, v, true); break;
    }
  });
  if (!any) return;
  // Keep everybody readable on the domain: a faint halo of light behind each fighter.
  float k = 0.0f;
  forEachUltimate(m, [&](const Fighter&, float kk, int) { k = std::max(k, kk); });
  vfx::additive(true);
  for (const auto& f : m.fighters) {
    if (!f.active() || f.state == FState::Respawn) continue;
    vfx::glow(f.center(), f.height() * 0.8f, A({255, 245, 230, 255}, 0.16f * k), A({255, 245, 230, 255}, 0.0f), 20);
  }
  vfx::additive(false);
}
