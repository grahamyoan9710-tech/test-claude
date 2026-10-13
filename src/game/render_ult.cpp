// The cinematics of the v5 ultimates (one per UltFx: the backdrop behind the stage, a foreground layer),
// what a held victim goes through (ice, chains, clones), and the transformations' avatars (Susanoo,
// Kurama, wings). Same kit as render_vfx.cpp: a few big gradient triangles, additive where it glows.
#include "game/fighter_art.hpp"
#include "game/render.hpp"

namespace {
using vfx::hash01;
inline Color A(Color c, float a) { return withAlpha(c, a); }
inline Vec2 dirOf(float a) { return {std::cos(a), std::sin(a)}; }

void sky(const Rect& v, Color top, Color bottom, float k) {
  vfx::quad({v.x, v.y}, {v.right(), v.y}, {v.right(), v.bottom()}, {v.x, v.bottom()}, A(top, 0.93f * k), A(top, 0.93f * k),
            A(bottom, 0.93f * k), A(bottom, 0.93f * k));
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
void polyline(const Vec2* p, int n, float w0, float w1, Color c0, Color c1) {
  for (int i = 0; i + 1 < n; ++i) {
    const float t0 = float(i) / std::max(1, n - 1), t1 = float(i + 1) / std::max(1, n - 1);
    vfx::wedge(p[i], p[i + 1], lerpf(w0, w1, t0), lerpf(w0, w1, t1), mixColor(c0, c1, t0), mixColor(c0, c1, t1));
  }
}

// ---- Scenery pieces ---------------------------------------------------------------------------------
void clouds(const Rect& v, float t, float k, Color col, int n, float band, uint32_t seed) {
  for (int i = 0; i < n; ++i) {
    const float x = v.x + std::fmod(hash01(seed + i * 3) * v.w + t * (20.0f + 30.0f * hash01(seed + i * 3 + 1)), v.w);
    const float y = v.y + hash01(seed + i * 3 + 2) * v.h * band;
    const float r = 160.0f + 160.0f * hash01(seed + i * 5);
    glowE({x, y}, r * 1.6f, r * 0.7f, A(col, 0.7f * k), A(col, 0.0f), 20);
  }
}
void flashes(const Rect& v, int frame, float k, Color col, uint32_t seed, int every = 9) {
  for (int i = 0; i < 3; ++i) {
    const int cyc = every + i * 4, ph = (frame + i * 5) % cyc;
    if (ph > 3) continue;
    const uint32_t s = uint32_t((frame + i * 5) / cyc) * 613u + i * 97u + seed;
    const Vec2 a{v.x + hash01(s) * v.w, v.y - 20.0f};
    const Vec2 b{a.x + (hash01(s + 1) - 0.5f) * 400.0f, v.y + v.h * (0.4f + 0.4f * hash01(s + 2))};
    const float fa = k * (1.0f - ph / 4.0f);
    vfx::additive(true);
    vfx::bolt(a, b, 60.0f, 12, s, 14.0f, A(col, 0.4f * fa));
    vfx::bolt(a, b, 60.0f, 12, s, 5.0f, A(WHITE, 0.9f * fa));
    vfx::additive(false);
  }
}
// Anime speed lines rushing towards a point.
void speedLines(const Rect& v, Vec2 c, float t, float k, Color col, int n = 46) {
  vfx::additive(true);
  for (int i = 0; i < n; ++i) {
    const float a = hash01(i * 7 + 1) * 2.0f * kPi;
    const float ph = std::fmod(t * (1.5f + hash01(i * 7 + 2)) + hash01(i * 7 + 3), 1.0f);
    const float r0 = 260.0f + 900.0f * (1.0f - ph), l = 120.0f + 260.0f * hash01(i * 7 + 4);
    vfx::wedge(c + dirOf(a) * r0, c + dirOf(a) * (r0 + l), 1.0f, 5.0f, A(col, 0.0f), A(col, 0.45f * k * ph));
  }
  vfx::additive(false);
  (void)v;
}
void embers(const Rect& v, float t, float k, Color col, int n = 40) {
  vfx::additive(true);
  for (int i = 0; i < n; ++i) {
    const float u = std::fmod(t * (0.3f + 0.4f * hash01(i * 3)) + hash01(i * 3 + 1), 1.0f);
    const Vec2 p{v.x + hash01(i * 3 + 2) * v.w + std::sin(t * 2.0f + i) * 24.0f, v.bottom() - u * v.h};
    vfx::glow(p, 6.0f, A(col, 0.85f * k * std::sin(u * kPi)), A(col, 0.0f), 8);
  }
  vfx::additive(false);
}
void flamesFromBelow(const Rect& v, float t, float k, Color base, Color tip, float height = 0.28f) {
  vfx::additive(true);
  const int n = 18;
  for (int layer = 0; layer < 2; ++layer)
    for (int i = 0; i < n; ++i) {
      const float w = v.w / n, x = v.x + (i + 0.5f) * w + (layer ? w * 0.5f : 0.0f);
      const float h = v.h * (height + 0.1f * std::sin(t * 7.0f + i * 1.7f + layer) + 0.08f * hash01(i * 3 + layer)) * (layer ? 0.7f : 1.0f);
      const Color c = layer ? tip : base;
      vfx::tri({x - w * 0.9f, v.bottom()}, {x + w * 0.9f, v.bottom()}, {x + std::sin(t * 5.0f + i) * 30.0f, v.bottom() - h},
               A(c, 0.5f * k), A(c, 0.5f * k), A(c, 0.0f));
    }
  vfx::additive(false);
}
void moon(Vec2 c, float r, Color col, Color sky, float k, float phase) {
  vfx::additive(true);
  vfx::glow(c, r * 2.4f, A(col, 0.3f * k), A(col, 0.0f), 32);
  vfx::additive(false);
  vfx::glow(c, r, A(col, k), A(col, k), 36);
  vfx::glow(c + Vec2{r * phase, -r * 0.15f}, r * 0.92f, A(sky, k), A(sky, k), 36);  // the shadow makes the crescent
}
// The Sharingan / Mangekyo: red iris, black pupil, tomoe (or the blades of the Mangekyo) turning.
void sharingan(Vec2 c, float r, float rot, float k, bool mangekyo) {
  vfx::glow(c, r * 1.15f, A({20, 0, 4, 255}, k), A({20, 0, 4, 255}, k), 36);
  vfx::glow(c, r, A({220, 20, 30, 255}, k), A({150, 0, 14, 255}, k), 36);
  const Color ink = A({10, 2, 4, 255}, k);
  if (mangekyo) {
    for (int i = 0; i < 3; ++i) {  // three curved blades
      const float a = rot + i * 2.0f * kPi / 3.0f;
      Vec2 pts[8];
      for (int j = 0; j < 8; ++j) {
        const float u = j / 7.0f;
        pts[j] = c + dirOf(a + u * 1.6f) * (r * (0.25f + 0.7f * u));
      }
      polyline(pts, 8, r * 0.2f, r * 0.04f, ink, ink);
    }
  } else {
    vfx::softRing(c, r * 0.6f, r * 0.05f, ink);
    for (int i = 0; i < 3; ++i) {  // tomoe
      const float a = rot + i * 2.0f * kPi / 3.0f;
      const Vec2 p = c + dirOf(a) * (r * 0.6f);
      vfx::glow(p, r * 0.13f, ink, ink, 12);
      vfx::wedge(p, p + dirOf(a + 1.4f) * (r * 0.25f), r * 0.12f, 1.0f, ink, ink);
    }
  }
  vfx::glow(c, r * 0.2f, ink, ink, 16);
}
// A big winged silhouette (wyvern, bat wings, the devil Liebe): wing membranes from a shoulder point.
void wing(Vec2 root, float side, float span, float t, Color col, Color rim, int fingers = 4) {
  const float flap = std::sin(t * 2.0f) * 0.12f;
  Vec2 tips[6];
  for (int i = 0; i < fingers; ++i) {
    const float a = -kPi * 0.5f + side * (0.35f + i * 0.38f + flap);
    tips[i] = root + dirOf(a) * (span * (1.0f - i * 0.12f));
  }
  for (int i = 0; i + 1 < fingers; ++i) {
    const Vec2 mid = lerpv(tips[i], tips[i + 1], 0.5f) + (root - lerpv(tips[i], tips[i + 1], 0.5f)) * 0.25f;
    vfx::tri(root, tips[i], mid, col, col, col);
    vfx::tri(root, mid, tips[i + 1], col, col, col);
  }
  for (int i = 0; i < fingers; ++i) vfx::wedge(root, tips[i], span * 0.05f, 2.0f, rim, rim);
}

// ---- The cinematics ----------------------------------------------------------------------------------
struct Ctx {
  const Match& m;
  const Fighter& f;
  float k;   // fade in / out
  int mf;    // move frame
  float t;   // time
  Rect v;    // view
  Vec2 c;    // the fighter's center
  float fc;  // facing
};

const Fighter* heldVictim(const Ctx& x) {
  for (const auto& o : x.m.fighters)
    if (o.state == FState::Captured && o.captor == x.f.id) return &o;
  return nullptr;
}

void blackMeteorite(const Ctx& x, bool front) {
  if (!front) {
    sky(x.v, {6, 6, 10, 255}, {40, 36, 48, 255}, x.k);
    clouds(x.v, x.t, x.k, {24, 22, 30, 255}, 8, 0.5f, 11);
    flashes(x.v, x.m.frame, x.k, {200, 200, 220, 255}, 5);
    return;
  }
  if (x.mf >= 60 && x.mf < 80) {  // the black meteor: a trail of anti-magic behind the dive
    const Vec2 a = x.c - Vec2{0.0f, 420.0f}, b = x.c + Vec2{0.0f, 40.0f};
    vfx::streak(a, b, 30.0f, 120.0f, A({8, 6, 12, 255}, 0.0f), A({8, 6, 12, 255}, 0.9f * x.k));
    vfx::additive(true);
    vfx::streak(a, b, 50.0f, 160.0f, A(WHITE, 0.0f), A(WHITE, 0.25f * x.k));
    vfx::additive(false);
  }
}

void demonUnion(const Ctx& x, bool front) {
  if (front) return;
  sky(x.v, {4, 4, 6, 255}, {30, 30, 36, 255}, x.k);
  // Liebe, the devil of the grimoire, looming behind: horns, white eyes, one great wing
  const Vec2 h = x.c + Vec2{-x.fc * 60.0f, -260.0f};
  const Color ink = A({6, 6, 8, 255}, 0.95f * x.k), rim = A({220, 220, 230, 255}, 0.4f * x.k);
  wing(h + Vec2{0.0f, 120.0f}, -x.fc, 520.0f, x.t, ink, rim, 5);
  glowE(h, 110.0f, 130.0f, ink, ink);
  vfx::tri(h + Vec2{-70, -60}, h + Vec2{-30, -90}, h + Vec2{-130, -230}, ink, ink, rim);
  vfx::tri(h + Vec2{70, -60}, h + Vec2{30, -90}, h + Vec2{120, -200}, ink, ink, rim);
  vfx::additive(true);
  for (int e = -1; e <= 1; e += 2) vfx::glow(h + Vec2{e * 40.0f, -10.0f}, 22.0f, A(WHITE, x.k), A(WHITE, 0.0f), 12);
  vfx::additive(false);
  flashes(x.v, x.m.frame, x.k, {220, 220, 235, 255}, 9, 7);
}

void windRain(const Ctx& x) {  // a sky full of falling wind blades
  vfx::additive(true);
  for (int i = 0; i < 40; ++i) {
    const float u = std::fmod(x.t * (0.9f + hash01(i * 3) * 0.7f) + hash01(i * 3 + 1), 1.0f);
    const Vec2 p{x.v.x + hash01(i * 3 + 2) * x.v.w, x.v.y + u * x.v.h};
    vfx::streak(p - Vec2{30.0f, 160.0f}, p, 1.0f, 4.0f, A({220, 255, 235, 255}, 0.0f), A({220, 255, 235, 255}, 0.5f * x.k));
  }
  vfx::additive(false);
}

// A blade of reiatsu along the fighter's own blade (Ichigo's giant Zangetsu): it grows, then follows his swing.
// Dark: a black blade edged with red (the hollow's reiatsu).
void spiritBlade(const Ctx& x, float len, float width, Color glow, Color core, float k, bool dark) {
  if (k <= 0.01f) return;
  const Pose p = computePose(x.f, x.m, x.t);
  const Vec2 hand = limbWorld(*x.f.def, p, x.f.pos, x.f.facing, 1.0f, Limb::ArmFront);
  Vec2 d = p.weaponDir.norm();
  d.x *= x.fc;
  const Vec2 n{-d.y, d.x};
  // A cleaver: the back of the blade is cut short at the tip, like Zangetsu.
  const Vec2 b0 = hand + d * 8.0f - n * (width * 0.5f), b1 = hand + d * 8.0f + n * (width * 0.5f);
  const Vec2 t0 = hand + d * (len - width * 0.8f) - n * (width * 0.5f), t1 = hand + d * len + n * (width * 0.5f);
  vfx::additive(true);
  vfx::glow(lerpv(hand, hand + d * len, 0.55f), len * 0.62f, A(glow, 0.32f * k), A(glow, 0.0f));
  if (!dark) vfx::quad(b0, b1, t1, t0, A(glow, 0.5f * k), A(glow, 0.5f * k), A(core, 0.85f * k), A(core, 0.85f * k));
  vfx::additive(false);
  if (dark) vfx::quad(b0, b1, t1, t0, A({8, 2, 6, 255}, 0.9f * k), A({8, 2, 6, 255}, 0.9f * k), A({20, 4, 8, 255}, 0.9f * k),
                      A({20, 4, 8, 255}, 0.9f * k));
  vfx::additive(true);
  vfx::wedge(b1, t1, 7.0f, 4.0f, A(dark ? Color{255, 50, 60, 255} : WHITE, 0.95f * k), A(dark ? Color{255, 90, 90, 255} : WHITE, 0.95f * k));
  vfx::wedge(t0, t1, 5.0f, 5.0f, A(dark ? Color{255, 50, 60, 255} : WHITE, 0.8f * k), A(dark ? Color{255, 50, 60, 255} : WHITE, 0.8f * k));
  vfx::wedge(b0, t0, 3.0f, 3.0f, A(core, 0.5f * k), A(core, 0.5f * k));
  vfx::additive(false);
}

// Reiatsu streaming into the fighter from all around.
void converge(const Ctx& x, Vec2 at, Color col, int n, float r0) {
  vfx::additive(true);
  for (int i = 0; i < n; ++i) {
    const float a = hash01(i * 7 + 3) * 2.0f * kPi;
    const float u = std::fmod(x.t * (0.9f + 0.6f * hash01(i * 7 + 4)) + hash01(i * 7 + 5), 1.0f);
    const float r = r0 * (1.0f - u) + 20.0f;
    vfx::streak(at + dirOf(a) * (r + 70.0f), at + dirOf(a) * r, 0.0f, 4.0f, A(col, 0.0f), A(col, 0.7f * x.k * u));
  }
  vfx::additive(false);
}

void giantGetsuga(const Ctx& x, bool front) {
  const MoveDef& mv = x.f.moveDef(FinalSmash);
  const int fire = mv.projectileFrame;
  if (front) {  // Zangetsu becomes a blade of reiatsu as tall as a building, then the swing
    const float grow = smoothstep(float(x.mf - 24) / 20.0f);
    const float fade = clampf(float(fire + 8 - x.mf) / 8.0f, 0.0f, 1.0f);
    spiritBlade(x, 90.0f + 380.0f * grow, 30.0f + 46.0f * grow, {90, 170, 255, 255}, {225, 240, 255, 255}, x.k * fade * grow, false);
    if (x.mf < fire) converge(x, x.c + Vec2{0.0f, -80.0f}, {170, 215, 255, 255}, 24, 520.0f);
    if (x.mf >= fire && x.mf < fire + 10) {  // the release
      const float u = (x.mf - fire) / 10.0f;
      vfx::additive(true);
      vfx::glow(x.c + Vec2{x.fc * 90.0f, -10.0f}, 160.0f + 260.0f * u, A({200, 230, 255, 255}, 0.7f * (1.0f - u) * x.k),
                A({90, 170, 255, 255}, 0.0f));
      vfx::additive(false);
    }
    return;
  }
  sky(x.v, {4, 10, 30, 255}, {20, 40, 90, 255}, x.k);
  moon(x.c + Vec2{x.fc * 260.0f, -330.0f}, 150.0f, {220, 236, 255, 255}, {6, 14, 40, 255}, 0.42f * x.fc, x.k);
  vfx::additive(true);
  vfx::streak(x.c + Vec2{0.0f, 200.0f}, x.c + Vec2{0.0f, -900.0f}, 120.0f, 40.0f, A({90, 170, 255, 255}, 0.45f * x.k),
              A({90, 170, 255, 255}, 0.0f));
  vfx::softRing(x.c, 140.0f + 30.0f * std::sin(x.t * 6.0f), 26.0f, A({120, 190, 255, 255}, 0.4f * x.k));
  vfx::additive(false);
}

// Getsuga Jujisho: the hollow mask, black reiatsu edged with red, two blades of darkness crossing.
void getsugaJujisho(const Ctx& x, bool front) {
  const MoveDef& mv = x.f.moveDef(FinalSmash);
  const int fire = mv.projectileFrame;
  if (front) {
    const float grow = smoothstep(float(x.mf - 26) / 18.0f);
    const float fade = clampf(float(fire + 6 - x.mf) / 6.0f, 0.0f, 1.0f);
    spiritBlade(x, 80.0f + 300.0f * grow, 26.0f + 34.0f * grow, {255, 40, 60, 255}, {255, 120, 120, 255}, x.k * fade * grow, true);
    if (x.mf >= fire - 10 && x.mf < fire + 8) {  // the two cuts of the cross, drawn in the air
      const float u = clampf(float(x.mf - fire + 10) / 8.0f, 0.0f, 1.0f), out = clampf(float(fire + 8 - x.mf) / 8.0f, 0.0f, 1.0f);
      const Vec2 o = x.c + Vec2{x.fc * 110.0f, -10.0f};
      for (int sgn = -1; sgn <= 1; sgn += 2) {
        const Vec2 a = o + Vec2{-x.fc * 90.0f, sgn * -130.0f}, b = o + Vec2{x.fc * 90.0f, sgn * 130.0f};
        const Vec2 e = lerpv(a, b, u);
        vfx::wedge(a, e, 10.0f, 30.0f, A({6, 2, 6, 255}, 0.95f * out * x.k), A({6, 2, 6, 255}, 0.95f * out * x.k));
        vfx::additive(true);
        vfx::wedge(a, e, 18.0f, 44.0f, A({255, 40, 60, 255}, 0.0f), A({255, 40, 60, 255}, 0.6f * out * x.k));
        vfx::additive(false);
      }
    }
    return;
  }
  sky(x.v, {6, 0, 2, 255}, {54, 4, 12, 255}, x.k);
  const Vec2 mc = x.c + Vec2{x.fc * 280.0f, -340.0f};  // a black moon with a red corona
  vfx::additive(true);
  vfx::glow(mc, 330.0f, A({200, 20, 40, 255}, 0.35f * x.k), A({200, 20, 40, 255}, 0.0f));
  vfx::additive(false);
  glowE(mc, 140.0f, 140.0f, A({4, 0, 2, 255}, x.k), A({4, 0, 2, 255}, x.k), 36);
  // the mask, faint and huge, behind him
  const Vec2 h = x.c + Vec2{-x.fc * 30.0f, -230.0f};
  const Color bone = A({240, 236, 228, 255}, 0.14f * x.k), red = A({200, 20, 30, 255}, 0.28f * x.k);
  glowE(h, 210.0f, 250.0f, bone, bone);
  vfx::wedge(h + Vec2{x.fc * 40.0f, -220.0f}, h + Vec2{x.fc * 50.0f, 200.0f}, 22.0f, 10.0f, red, red);
  vfx::wedge(h + Vec2{x.fc * 100.0f, -200.0f}, h + Vec2{x.fc * 105.0f, 160.0f}, 16.0f, 8.0f, red, red);
  for (int s = -1; s <= 1; s += 2)
    glowE(h + Vec2{s * 80.0f, -10.0f}, 46.0f, 30.0f, A({0, 0, 0, 255}, 0.5f * x.k), A({0, 0, 0, 255}, 0.5f * x.k));
  for (int i = 0; i < 14; ++i) {  // black flames of reiatsu rising from him
    const float xo = (hash01(i * 7 + 1) - 0.5f) * 160.0f;
    const float ht = 160.0f + 160.0f * hash01(i * 7 + 2) * (0.8f + 0.2f * std::sin(x.t * 9.0f + i));
    const Vec2 b = x.c + Vec2{xo, 60.0f}, tip = b + Vec2{std::sin(x.t * 4.0f + i) * 26.0f, -ht};
    vfx::additive(true);
    vfx::wedge(b, tip, 30.0f, 2.0f, A({255, 30, 50, 255}, 0.4f * x.k), A({255, 30, 50, 255}, 0.0f));
    vfx::additive(false);
    vfx::wedge(b, tip, 20.0f, 1.0f, A({4, 2, 6, 255}, 0.85f * x.k), A({4, 2, 6, 255}, 0.15f * x.k));
  }
}

void vastoLorde(const Ctx& x, bool front) {
  if (front) {
    if (x.mf >= 44 && x.mf < 58) {  // the cero charging between the horns
      vfx::additive(true);
      const Vec2 p = x.c + Vec2{x.fc * 30.0f, -46.0f};
      const float r = 20.0f + (x.mf - 44) * 4.0f;
      vfx::glow(p, r * 2.0f, A({255, 40, 50, 255}, 0.6f * x.k), A({255, 40, 50, 255}, 0.0f));
      vfx::glow(p, r * 0.6f, A(WHITE, x.k), A({255, 120, 120, 255}, 0.0f));
      vfx::additive(false);
    }
    return;
  }
  sky(x.v, {20, 0, 4, 255}, {70, 6, 16, 255}, x.k);
  // the hollow mask, enormous and faint, behind him
  const Vec2 h = x.c + Vec2{0.0f, -200.0f};
  const Color bone = A({240, 236, 228, 255}, 0.18f * x.k), red = A({200, 20, 30, 255}, 0.3f * x.k);
  glowE(h, 260.0f, 300.0f, bone, bone);
  for (int s = -1; s <= 1; s += 2) {
    vfx::tri(h + Vec2{s * 150.0f, -170.0f}, h + Vec2{s * 220.0f, -120.0f}, h + Vec2{s * 300.0f, -520.0f}, bone, bone, A(bone, 0.0f));
    vfx::wedge(h + Vec2{s * 90.0f, -230.0f}, h + Vec2{s * 110.0f, 240.0f}, 24.0f, 10.0f, red, red);
    glowE(h + Vec2{s * 95.0f, -20.0f}, 50.0f, 34.0f, A({0, 0, 0, 255}, 0.4f * x.k), A({0, 0, 0, 255}, 0.4f * x.k));
  }
  for (int i = 0; i < 6; ++i) {  // red cracks in the dark
    const float a = hash01(i * 17 + 5) * 2.0f * kPi;
    vfx::additive(true);
    vfx::bolt(x.c + dirOf(a) * 140.0f, x.c + dirOf(a) * 1200.0f, 60.0f, 9, i * 31u + uint32_t(x.t * 4.0f), 3.0f,
              A({220, 20, 40, 255}, 0.4f * x.k));
    vfx::additive(false);
  }
}

void hakuteiken(const Ctx& x, bool front) {
  if (front) {
    vfx::additive(true);
    for (int i = 0; i < 26; ++i) {  // white feathers of light drifting
      const float u = std::fmod(x.t * 0.3f + hash01(i * 5), 1.0f);
      const Vec2 p{x.v.x + hash01(i * 5 + 1) * x.v.w + std::sin(x.t + i) * 30.0f, x.v.y + u * x.v.h};
      vfx::wedge(p, p + dirOf(1.2f + std::sin(x.t * 2.0f + i) * 0.5f) * 18.0f, 6.0f, 1.0f, A(WHITE, 0.7f * x.k), A(WHITE, 0.0f));
    }
    vfx::additive(false);
    return;
  }
  sky(x.v, {250, 236, 244, 255}, {255, 200, 220, 255}, 0.85f * x.k);
  vfx::additive(true);
  const Vec2 back = x.c + Vec2{-x.fc * 10.0f, -30.0f};
  for (int s = -1; s <= 1; s += 2)  // wings of white light
    for (int i = 0; i < 6; ++i) {
      const float a = -kPi * 0.5f + s * (0.5f + i * 0.22f) + std::sin(x.t * 2.0f) * 0.05f * s;
      vfx::streak(back, back + dirOf(a) * (420.0f - i * 40.0f), 50.0f, 12.0f, A(WHITE, 0.7f * x.k), A({255, 220, 236, 255}, 0.0f));
    }
  vfx::softRing(x.c + Vec2{0.0f, -90.0f}, 60.0f, 10.0f, A(WHITE, 0.9f * x.k));  // halo
  vfx::additive(false);
}

void lightDragon(const Ctx& x) {  // the Light Dragon flying along the beam of the seal
  const MoveDef& mv = x.f.moveDef(FinalSmash);
  if (x.mf < mv.beam.start - 6 || x.mf >= mv.beam.end + 6) return;
  const float u = clampf(float(x.mf - mv.beam.start + 6) / float(mv.beam.end - mv.beam.start + 12), 0.0f, 1.0f);
  const Vec2 a = x.c + Vec2{mv.beam.from.x * x.fc, mv.beam.from.y};
  const Vec2 d{x.fc, 0.0f};
  const Vec2 head = a + d * (mv.beam.length * u);
  constexpr int kN = 16;
  Vec2 body[kN];
  for (int i = 0; i < kN; ++i) body[i] = head - d * (i * 55.0f) + Vec2{0.0f, std::sin(x.t * 6.0f - i * 0.6f) * 40.0f};
  vfx::additive(true);
  for (int i = kN - 1; i >= 0; --i) vfx::glow(body[i], 46.0f - i * 1.8f, A({255, 236, 170, 255}, 0.7f * x.k), A({255, 200, 90, 255}, 0.0f), 16);
  polyline(body, kN, 34.0f, 6.0f, A(WHITE, 0.8f * x.k), A({255, 220, 140, 255}, 0.2f * x.k));
  vfx::glow(head + d * 30.0f, 50.0f, A(WHITE, x.k), A({255, 220, 140, 255}, 0.0f), 16);
  for (int s = -1; s <= 1; s += 2)  // antlers
    vfx::wedge(head, head + Vec2{-x.fc * 60.0f, s * 50.0f - 30.0f}, 8.0f, 1.0f, A({255, 240, 200, 255}, x.k), A(WHITE, 0.0f));
  vfx::additive(false);
}

void masterSword(const Ctx& x) {  // a giant Master Sword of light behind Link
  const Vec2 base = x.c + Vec2{-x.fc * 40.0f, 120.0f};
  vfx::additive(true);
  vfx::streak(base, base + Vec2{0.0f, -560.0f}, 70.0f, 30.0f, A({200, 230, 255, 255}, 0.55f * x.k), A(WHITE, 0.2f * x.k));
  vfx::wedge(base + Vec2{-90.0f, -80.0f}, base + Vec2{90.0f, -80.0f}, 26.0f, 26.0f, A({120, 120, 255, 255}, 0.5f * x.k),
             A({120, 120, 255, 255}, 0.5f * x.k));
  vfx::additive(false);
}

void lightArrowRain(const Ctx& x, bool front) {
  if (front) {
    vfx::additive(true);
    for (int i = 0; i < 30; ++i) {  // arrows of light falling
      const float u = std::fmod(x.t * (1.1f + hash01(i * 3) * 0.6f) + hash01(i * 3 + 1), 1.0f);
      const Vec2 p{x.v.x + hash01(i * 3 + 2) * x.v.w, x.v.y + u * x.v.h};
      vfx::streak(p - Vec2{0.0f, 200.0f}, p, 1.0f, 5.0f, A({255, 230, 150, 255}, 0.0f), A({255, 240, 200, 255}, 0.7f * x.k));
    }
    vfx::additive(false);
    return;
  }
  sky(x.v, {10, 14, 40, 255}, {60, 50, 90, 255}, x.k);
  for (int i = 0; i < 70; ++i) {
    const Vec2 p{x.v.x + hash01(i * 3 + 1) * x.v.w, x.v.y + hash01(i * 3 + 2) * x.v.h * 0.7f};
    DrawPoly(V(p), 4, 1.5f + 2.0f * hash01(i * 7), 45.0f, A({255, 230, 160, 255}, x.k * (0.5f + 0.5f * std::sin(x.t * 3.0f + i))));
  }
}

void gloomNoDragon(const Ctx& x) {
  sky(x.v, {16, 0, 6, 255}, {64, 6, 18, 255}, x.k);
  clouds(x.v, x.t, x.k, {120, 10, 30, 255}, 7, 0.9f, 23);
  moon(x.c + Vec2{x.fc * 240.0f, -340.0f}, 110.0f, {255, 70, 60, 255}, {30, 0, 8, 255}, 0.0f, x.k);  // blood moon
}

void fimbulwinter(const Ctx& x, bool front) {
  if (!front) {
    sky(x.v, {150, 180, 210, 255}, {230, 240, 250, 255}, x.k);
    vfx::additive(true);
    for (int i = 0; i < 6; ++i) {  // runes of frost circling
      const float a = x.t * 0.4f + i * kPi / 3.0f;
      vfx::softRing(x.c + dirOf(a) * 260.0f, 30.0f, 6.0f, A({160, 230, 255, 255}, 0.6f * x.k));
    }
    vfx::additive(false);
    return;
  }
  for (int i = 0; i < 70; ++i) {  // blizzard
    const float u = std::fmod(x.t * (0.8f + hash01(i * 3) * 0.6f) + hash01(i * 3 + 1), 1.0f);
    const Vec2 p{x.v.x + u * (x.v.w + 400.0f) - 200.0f, x.v.y + hash01(i * 3 + 2) * x.v.h};
    vfx::wedge(p, p + Vec2{-60.0f, 14.0f}, 3.0f, 0.5f, A(WHITE, 0.7f * x.k), A(WHITE, 0.0f));
  }
  for (const auto& o : x.m.fighters) {  // the frozen victims: blocks of ice
    if (o.state != FState::Captured || o.captor != x.f.id) continue;
    const Vec2 oc = o.center();
    const float w = o.width() * 0.9f, h = o.height() * 0.62f;
    const Color ice = A({170, 230, 255, 255}, 0.55f * x.k), edge = A(WHITE, 0.85f * x.k);
    vfx::quad(oc + Vec2{-w, -h}, oc + Vec2{w * 0.8f, -h * 1.1f}, oc + Vec2{w, h}, oc + Vec2{-w * 0.9f, h * 1.05f}, ice, ice, ice, ice);
    DrawLineEx(V(oc + Vec2{-w, -h}), V(oc + Vec2{w * 0.8f, -h * 1.1f}), 3.0f, edge);
    DrawLineEx(V(oc + Vec2{-w * 0.4f, -h}), V(oc + Vec2{0.1f * w, h}), 2.0f, A(WHITE, 0.4f * x.k));
  }
}

void chaosWhip(const Ctx& x) {  // the chains of the Blades of Chaos, burning, to the victim
  const Fighter* v = heldVictim(x);
  if (!v) return;
  const Vec2 a = x.c + Vec2{x.fc * 20.0f, -10.0f}, b = v->center();
  for (int s = -1; s <= 1; s += 2) {
    Vec2 pts[10];
    for (int i = 0; i < 10; ++i) {
      const float u = i / 9.0f;
      pts[i] = lerpv(a, b, u) + Vec2{0.0f, std::sin(u * kPi) * (30.0f * s + std::sin(x.t * 20.0f + i) * 6.0f)};
    }
    polyline(pts, 10, 5.0f, 5.0f, A({90, 70, 60, 255}, x.k), A({90, 70, 60, 255}, x.k));
    vfx::additive(true);
    polyline(pts, 10, 14.0f, 14.0f, A({255, 110, 30, 255}, 0.4f * x.k), A({255, 200, 80, 255}, 0.4f * x.k));
    vfx::additive(false);
  }
}

void maxBlue(const Ctx& x, bool front) {
  if (front) return;
  sky(x.v, {2, 6, 24, 255}, {10, 20, 60, 255}, x.k);
  Vec2 c = x.c;
  for (const auto& p : x.m.projectiles)
    if (p.owner == x.f.id && p.visual == ProjVisual::MaxBlue) c = p.pos;
  vfx::additive(true);
  for (int arm = 0; arm < 8; ++arm) {
    Vec2 pts[16];
    for (int i = 0; i < 16; ++i) {
      const float u = i / 15.0f;
      pts[i] = c + dirOf(-x.t * 1.5f + arm * kPi / 4.0f + u * 3.0f) * (1300.0f * (1.0f - u) + 40.0f);
    }
    polyline(pts, 16, 30.0f, 4.0f, A({60, 120, 255, 255}, 0.0f), A({160, 210, 255, 255}, 0.5f * x.k));
  }
  vfx::additive(false);
}

void imaginaryPurple(const Ctx& x, bool front) {
  if (!front) {
    sky(x.v, {8, 0, 16, 255}, {40, 10, 60, 255}, x.k);
    return;
  }
  const MoveDef& mv = x.f.moveDef(FinalSmash);
  if (x.mf >= mv.projectileFrame) return;
  // red from one side, blue from the other, colliding in front of him
  const float u = smoothstep(float(x.mf - 20) / float(mv.projectileFrame - 22));
  const Vec2 meet = x.c + Vec2{x.fc * 70.0f, -10.0f};
  const Vec2 red = lerpv(meet + Vec2{-x.fc * 180.0f, -140.0f}, meet, u), blue = lerpv(meet + Vec2{x.fc * 180.0f, -140.0f}, meet, u);
  vfx::additive(true);
  vfx::glow(red, 40.0f, A({255, 60, 80, 255}, x.k), A({255, 60, 80, 255}, 0.0f));
  vfx::glow(blue, 40.0f, A({80, 150, 255, 255}, x.k), A({80, 150, 255, 255}, 0.0f));
  if (u > 0.85f) vfx::glow(meet, 60.0f + 200.0f * (u - 0.85f), A({200, 120, 255, 255}, x.k), A({170, 80, 255, 255}, 0.0f));
  vfx::additive(false);
}

void kamutoke(const Ctx& x, bool front) {
  if (front) return;
  sky(x.v, {14, 14, 26, 255}, {50, 46, 70, 255}, x.k);
  clouds(x.v, x.t, x.k, {30, 30, 46, 255}, 9, 0.45f, 41);
  flashes(x.v, x.m.frame, x.k, {170, 200, 255, 255}, 13, 6);
}

void fuga(const Ctx& x, bool front) {
  if (front) {
    flamesFromBelow(x.v, x.t, x.k, {255, 80, 20, 255}, {255, 210, 110, 255}, 0.24f);
    embers(x.v, x.t, x.k, {255, 160, 60, 255});
    return;
  }
  sky(x.v, {60, 6, 0, 255}, {200, 70, 10, 255}, x.k);
  vfx::additive(true);
  vfx::softRing(x.c, 300.0f, 60.0f, A({255, 120, 30, 255}, 0.5f * x.k));  // a circle of fire behind him
  vfx::additive(false);
}

// Kenpachi's reiatsu: a giant grinning skull of yellow light behind him.
void reiatsuSkull(const Ctx& x, Color col) {
  const Vec2 h = x.c + Vec2{-x.fc * 20.0f, -220.0f};
  vfx::additive(true);
  glowE(h, 230.0f, 260.0f, A(col, 0.35f * x.k), A(col, 0.0f));
  vfx::additive(false);
  const Color hole = A({0, 0, 0, 255}, 0.55f * x.k);
  for (int s = -1; s <= 1; s += 2) glowE(h + Vec2{s * 80.0f, -30.0f}, 50.0f, 60.0f, hole, A({0, 0, 0, 255}, 0.0f));
  for (int i = -3; i <= 3; ++i) vfx::wedge(h + Vec2{i * 30.0f, 120.0f}, h + Vec2{i * 30.0f, 170.0f}, 18.0f, 6.0f, hole, A(hole, 0.0f));
}

void kendo(const Ctx& x, bool front) {
  if (front) return;
  sky(x.v, {10, 8, 2, 255}, {50, 40, 10, 255}, x.k);
  reiatsuSkull(x, {255, 214, 60, 255});
  speedLines(x.v, x.c, x.t, x.k, {255, 230, 120, 255}, 30);
}

void nozarashi(const Ctx& x, bool front) {
  if (front) {
    const MoveDef& mv = x.f.moveDef(FinalSmash);
    if (x.mf >= mv.projectileFrame && x.mf < mv.projectileFrame + 30) {  // the stage split in two
      const float u = (x.mf - mv.projectileFrame) / 30.0f;
      vfx::additive(true);
      vfx::streak(x.c + Vec2{0.0f, 60.0f}, x.c + Vec2{x.fc * 1500.0f, 60.0f}, 4.0f, 30.0f * (1.0f - u), A({255, 240, 160, 255}, x.k),
                  A({255, 220, 80, 255}, 0.0f));
      vfx::additive(false);
    }
    return;
  }
  sky(x.v, {12, 10, 4, 255}, {60, 50, 14, 255}, x.k);
  reiatsuSkull(x, {255, 200, 50, 255});
}

void redDemon(const Ctx& x, bool front) {
  if (front) {
    embers(x.v, x.t, x.k, {255, 60, 60, 255}, 30);
    return;
  }
  sky(x.v, {30, 0, 0, 255}, {120, 10, 16, 255}, x.k);
  reiatsuSkull(x, {255, 50, 50, 255});
}

// Hueco Mundo: black sky, a white crescent moon, white dunes.
void huecoMundo(const Ctx& x) {
  sky(x.v, {2, 4, 4, 255}, {14, 22, 18, 255}, x.k);
  moon(x.c + Vec2{x.fc * 300.0f, -380.0f}, 120.0f, {240, 244, 236, 255}, {2, 4, 4, 255}, 0.45f, x.k);
  const float base = x.f.pos.y + 160.0f;
  for (int i = 0; i < 4; ++i) {
    const float cx = x.v.x + (i + 0.5f) * x.v.w / 4.0f;
    glowE({cx, base + 60.0f}, x.v.w * 0.32f, 120.0f, A({200, 204, 196, 255}, 0.75f * x.k), A({200, 204, 196, 255}, 0.75f * x.k), 28);
  }
}

void ceroOscuras(const Ctx& x, bool front) {
  if (front) return;
  huecoMundo(x);
}

void lanza(const Ctx& x, bool front) {
  if (front) {  // the javelin of light forms in his raised hand, then he throws it (shotLanza)
    const int pf = x.f.moveDef(FinalSmash).projectileFrame;
    if (x.mf >= 16 && x.mf < pf) {
      const float u = clampf((x.mf - 16) / float(std::max(1, pf - 16)), 0.0f, 1.0f);
      const float size = (0.4f + 0.6f * smoothstep(u)) * 1.7f, a = x.fc > 0 ? -0.22f : kPi + 0.22f;
      const Vec2 hand = x.c + Vec2{-x.fc * 8.0f, -66.0f}, dir{std::cos(a), std::sin(a)};
      art::drawFx("lanza", x.t, hand + dir * (110.0f * size), size, a, false, std::min(1.0f, 3.0f * u) * x.k);  // held by the shaft
    }
    return;
  }
  huecoMundo(x);
  flashes(x.v, x.m.frame, x.k, {100, 255, 160, 255}, 17, 8);
}

void segunda(const Ctx& x, bool front) {
  if (front) return;
  huecoMundo(x);
}

// A wyvern roaring in the distance (Monster Hunter).
void wyvern(const Ctx& x, Color body, Color rim) {
  const Vec2 h = x.c + Vec2{-x.fc * 320.0f, -280.0f};
  const Color b = A(body, 0.85f * x.k), r = A(rim, 0.6f * x.k);
  wing(h + Vec2{0.0f, 40.0f}, -1.0f, 360.0f, x.t * 0.7f, b, r);
  wing(h + Vec2{0.0f, 40.0f}, 1.0f, 360.0f, x.t * 0.7f + 0.5f, b, r);
  glowE(h + Vec2{0.0f, 60.0f}, 90.0f, 130.0f, b, b);
  const Vec2 jaw = h + Vec2{x.fc * 40.0f, -40.0f};
  vfx::tri(jaw + Vec2{0, -30}, jaw + Vec2{0, 30}, jaw + Vec2{x.fc * 140.0f, -10.0f}, b, b, b);
  vfx::additive(true);
  vfx::glow(jaw + Vec2{x.fc * 120.0f, 0.0f}, 40.0f, A({255, 140, 40, 255}, 0.7f * x.k), A({255, 80, 20, 255}, 0.0f));
  vfx::additive(false);
}

void trueCharged(const Ctx& x, bool front) {
  if (front) return;
  sky(x.v, {60, 14, 4, 255}, {220, 110, 40, 255}, x.k);
  wyvern(x, {30, 10, 8, 255}, {255, 120, 60, 255});
  embers(x.v, x.t, x.k, {255, 170, 70, 255}, 30);
}

void ampedDischarge(const Ctx& x, bool front) {
  if (front) return;
  sky(x.v, {6, 8, 30, 255}, {40, 30, 80, 255}, x.k);
  flashes(x.v, x.m.frame, x.k, {255, 230, 100, 255}, 19, 5);
}

void wyvernFire(const Ctx& x, bool front) {
  if (front) {
    flamesFromBelow(x.v, x.t, x.k, {255, 90, 20, 255}, {255, 210, 110, 255}, 0.2f);
    return;
  }
  sky(x.v, {50, 10, 0, 255}, {200, 80, 20, 255}, x.k);
  wyvern(x, {20, 6, 4, 255}, {255, 160, 60, 255});
}

void uzumaki(const Ctx& x, bool front) {
  if (!front) {
    sky(x.v, {255, 170, 70, 255}, {255, 230, 170, 255}, 0.8f * x.k);
    speedLines(x.v, x.c, x.t, x.k, WHITE, 60);
    return;
  }
  const Fighter* v = heldVictim(x);
  if (!v) return;
  for (int i = 0; i < 5; ++i) {  // shadow clones leaping at the victim from every side
    const float a = x.t * 3.0f + i * 2.0f * kPi / 5.0f;
    const Vec2 p = v->center() + dirOf(a) * 70.0f;
    Pose pose = restPose(*x.f.def, x.t * 2.0f + i);
    const float H = x.f.def->height;
    pose.footF = {0.3f * H, 0.1f * H};  // a flying kick
    pose.handF = {0.15f * H, -0.2f * H};
    pose.lean = 0.4f;
    drawCharacterSprite(*x.f.def, x.f.palette, pose, p + Vec2{0.0f, H * 0.5f}, std::cos(a) > 0 ? -1 : 1, 0.85f, 0.85f * x.k,
                        WHITE, 0.0f, x.t);
  }
}

void rasenshuriken(const Ctx& x, bool front) {
  if (front) return;
  sky(x.v, {120, 190, 255, 255}, {220, 240, 255, 255}, 0.8f * x.k);
  vfx::additive(true);
  for (int arm = 0; arm < 4; ++arm) {
    Vec2 pts[14];
    for (int i = 0; i < 14; ++i) {
      const float u = i / 13.0f;
      pts[i] = x.c + dirOf(x.t * 2.0f + arm * kPi * 0.5f + u * 2.5f) * (60.0f + 900.0f * u);
    }
    polyline(pts, 14, 40.0f, 6.0f, A(WHITE, 0.45f * x.k), A(WHITE, 0.0f));
  }
  vfx::additive(false);
}

void kurama(const Ctx& x, bool front) {
  if (front) return;
  sky(x.v, {20, 8, 4, 255}, {90, 40, 10, 255}, x.k);
  // the nine-tailed fox towering behind him
  const Vec2 h = x.c + Vec2{-x.fc * 40.0f, -330.0f};
  const Color fur = A({255, 140, 30, 255}, 0.55f * x.k), dark = A({120, 40, 10, 255}, 0.6f * x.k);
  for (int i = 0; i < 9; ++i) {  // tails
    Vec2 pts[8];
    const float a0 = -kPi * 0.5f - x.fc * (0.5f + i * 0.18f);
    for (int j = 0; j < 8; ++j) {
      const float u = j / 7.0f;
      pts[j] = h + Vec2{0.0f, 260.0f} + dirOf(a0 + std::sin(x.t * 2.0f + i + u * 3.0f) * 0.3f * u) * (u * 520.0f);
    }
    vfx::additive(true);
    polyline(pts, 8, 70.0f, 20.0f, fur, A(fur, 0.0f));
    vfx::additive(false);
  }
  vfx::additive(true);
  glowE(h + Vec2{0.0f, 160.0f}, 200.0f, 230.0f, fur, A(fur, 0.0f));
  glowE(h, 150.0f, 120.0f, fur, A(fur, 0.0f));
  for (int s = -1; s <= 1; s += 2) {
    vfx::tri(h + Vec2{s * 60.0f, -60.0f}, h + Vec2{s * 130.0f, -40.0f}, h + Vec2{s * 120.0f, -220.0f}, fur, fur, A(fur, 0.2f));
    vfx::glow(h + Vec2{s * 50.0f + x.fc * 20.0f, 0.0f}, 20.0f, A({255, 60, 30, 255}, x.k), A({255, 60, 30, 255}, 0.0f), 12);
  }
  vfx::additive(false);
  (void)dark;
}

void kirin(const Ctx& x, bool front) {
  if (front) return;
  sky(x.v, {6, 6, 20, 255}, {40, 30, 70, 255}, x.k);
  clouds(x.v, x.t, x.k, {24, 20, 44, 255}, 10, 0.4f, 61);
  flashes(x.v, x.m.frame, x.k, {170, 200, 255, 255}, 23, 5);
}

void indra(const Ctx& x, bool front) {
  if (front) return;
  sky(x.v, {10, 2, 20, 255}, {50, 16, 80, 255}, x.k);
  embers(x.v, x.t, x.k, {190, 120, 255, 255}, 30);
}

void amaterasu(const Ctx& x, bool front) {
  if (front) return;
  sky(x.v, {60, 0, 6, 255}, {160, 20, 30, 255}, x.k);
  sharingan(x.c + Vec2{x.fc * 260.0f, -320.0f}, 150.0f, x.t * 0.8f, x.k, true);  // the eternal Mangekyo, a black sun
}

void raikiri(const Ctx& x, bool front) {
  if (!front) {
    sky(x.v, {4, 8, 24, 255}, {20, 40, 80, 255}, x.k);
    if (x.mf >= 40) speedLines(x.v, x.c, x.t, x.k, {200, 230, 255, 255}, 50);
    return;
  }
  const uint32_t fs = uint32_t(x.m.frame) * 37u;  // the chirping of a thousand birds in his hand
  const Vec2 hand = x.c + Vec2{x.fc * 34.0f, -6.0f};
  vfx::additive(true);
  vfx::glow(hand, 40.0f, A({150, 200, 255, 255}, 0.8f * x.k), A({150, 200, 255, 255}, 0.0f));
  for (int i = 0; i < 6; ++i) {
    const float a = hash01(fs + i) * 2.0f * kPi;
    vfx::bolt(hand, hand + dirOf(a) * (40.0f + 40.0f * hash01(fs + i + 9)), 10.0f, 4, fs + i * 3u, 2.5f,
              A({200, 230, 255, 255}, x.k), A(WHITE, x.k));
  }
  vfx::additive(false);
}

void kamui(const Ctx& x, bool front) {
  if (front) return;
  sky(x.v, {30, 28, 36, 255}, {80, 76, 90, 255}, x.k);
  sharingan(x.c + Vec2{-x.fc * 200.0f, -280.0f}, 170.0f, -x.t * 1.2f, 0.85f * x.k, true);
}

// ---- Grimmjow ---------------------------------------------------------------------------------------------------
// Las Noches: the white dome of the Espada's palace standing on the dunes of Hueco Mundo.
void lasNoches(const Ctx& x) {
  huecoMundo(x);
  const float base = x.f.pos.y + 150.0f;
  const Vec2 dc{x.c.x - x.fc * 460.0f, base};
  const Color wall = A({222, 224, 216, 255}, 0.7f * x.k), shade2 = A({170, 172, 166, 255}, 0.7f * x.k);
  glowE(dc, 560.0f, 330.0f, wall, wall, 40);
  glowE(dc + Vec2{x.fc * 120.0f, 40.0f}, 420.0f, 250.0f, shade2, A(shade2, 0.0f), 32);
  for (int i = 0; i < 3; ++i) {  // its towers
    const Vec2 b = dc + Vec2{(i - 1) * 260.0f, -200.0f + std::fabs(i - 1.0f) * 80.0f};
    vfx::wedge(b, b + Vec2{0.0f, -320.0f + std::fabs(i - 1.0f) * 90.0f}, 46.0f, 30.0f, wall, wall);
  }
}

void granReyCero(const Ctx& x, bool front) {
  if (front) {  // a drop of his blood falls into the cero, which swells, cyan and white
    if (x.mf >= 26 && x.mf < 48) {
      const Vec2 hand = x.c + Vec2{x.fc * 34.0f, -14.0f};
      const float g = (x.mf - 26) / 22.0f;
      vfx::additive(true);
      vfx::glow(hand, 20.0f + 70.0f * g, A({60, 214, 240, 255}, 0.7f * x.k), A({60, 214, 240, 255}, 0.0f));
      vfx::glow(hand, 8.0f + 26.0f * g, A(WHITE, x.k), A({170, 250, 255, 255}, 0.0f));
      vfx::additive(false);
      if (g < 0.55f) vfx::glow(hand + Vec2{0.0f, -60.0f + 110.0f * g}, 6.0f, A({210, 20, 30, 255}, x.k), A({210, 20, 30, 255}, x.k), 10);
    }
    return;
  }
  lasNoches(x);
}

// Rake marks of a giant claw across the sky (five parallel cuts), drawn as they sweep.
void clawRake(Vec2 c, Vec2 dir, float len, float gap, float u, float k, Color col) {
  const Vec2 n{-dir.y, dir.x};
  vfx::additive(true);
  for (int i = 0; i < 5; ++i) {
    const Vec2 a = c - dir * len + n * ((i - 2) * gap), b = c + dir * len + n * ((i - 2) * gap);
    const Vec2 e = lerpv(a, b, u);
    vfx::streak(a, e, 2.0f, 26.0f, A(col, 0.0f), A(col, 0.8f * k));
    vfx::streak(a, e, 0.0f, 8.0f, A(WHITE, 0.0f), A(WHITE, k));
  }
  vfx::additive(false);
}

void panteraRelease(const Ctx& x, bool front) {
  if (front) {  // "Kishire, Pantera": the claws tear the air open
    if (x.mf >= 30 && x.mf < 52) {
      const float u = clampf((x.mf - 30) / 6.0f, 0.0f, 1.0f), out = clampf((52 - x.mf) / 12.0f, 0.0f, 1.0f);
      clawRake(x.c + Vec2{x.fc * 40.0f, -60.0f}, Vec2{x.fc * 0.7f, 1.0f}.norm(), 340.0f, 60.0f, u, out * x.k, {60, 214, 240, 255});
    }
    return;
  }
  sky(x.v, {2, 10, 20, 255}, {10, 44, 64, 255}, x.k);
  const Vec2 h = x.c + Vec2{-x.fc * 40.0f, -250.0f};  // a panther of light roaring behind him
  const Color col = A({60, 214, 240, 255}, 0.35f * x.k);
  vfx::additive(true);
  glowE(h, 230.0f, 170.0f, col, A(col, 0.0f));
  for (int sgn = -1; sgn <= 1; sgn += 2) {
    vfx::tri(h + Vec2{sgn * 90.0f, -90.0f}, h + Vec2{sgn * 170.0f, -60.0f}, h + Vec2{sgn * 160.0f, -230.0f}, col, col, A(col, 0.0f));
    vfx::glow(h + Vec2{sgn * 70.0f, -10.0f}, 26.0f, A(WHITE, 0.9f * x.k), A({120, 240, 255, 255}, 0.0f), 12);
  }
  vfx::tri(h + Vec2{-80.0f, 70.0f}, h + Vec2{80.0f, 70.0f}, h + Vec2{0.0f, 160.0f}, col, col, A(col, 0.0f));  // the jaw
  vfx::additive(false);
  flashes(x.v, x.m.frame, x.k, {120, 240, 255, 255}, 29, 6);
}

void desgarronClaws(const Ctx& x, bool front) {
  if (!front) {
    sky(x.v, {0, 6, 14, 255}, {8, 30, 50, 255}, x.k);
    speedLines(x.v, x.c, x.t, x.k, {120, 240, 255, 255}, 40);
    return;
  }
  const MoveDef& mv = x.f.moveDef(FinalSmash);
  const int fin = mv.hitboxes.empty() ? 72 : mv.hitboxes.back().start;
  if (x.mf >= fin - 4 && x.mf < fin + 14) {  // ten claws of light across the victim
    const float u = clampf((x.mf - fin + 4) / 8.0f, 0.0f, 1.0f), out = clampf((fin + 14 - x.mf) / 10.0f, 0.0f, 1.0f);
    const Vec2 c0 = x.c + Vec2{mv.holdAt.x * x.fc, mv.holdAt.y};
    clawRake(c0, Vec2{x.fc * 0.8f, 1.0f}.norm(), 320.0f, 44.0f, u, out * x.k, {60, 214, 240, 255});
    clawRake(c0, Vec2{-x.fc * 0.8f, 1.0f}.norm(), 320.0f, 44.0f, clampf(u * 1.4f - 0.4f, 0.0f, 1.0f), out * x.k, {60, 214, 240, 255});
  }
}

// ---- Starrk ------------------------------------------------------------------------------------------------------
void ceroMetralleta(const Ctx& x, bool front) {
  if (front) return;
  sky(x.v, {4, 8, 24, 255}, {24, 40, 80, 255}, x.k);
  vfx::additive(true);  // a wall of a thousand small ceros lighting up behind him
  for (int i = 0; i < 105; ++i) {
    const float gx = (i % 15 - 7) * 52.0f + (vfx::hash01(i * 3) - 0.5f) * 24.0f;
    const float gy = (i / 15 - 3) * 52.0f + (vfx::hash01(i * 3 + 1) - 0.5f) * 24.0f;
    const Vec2 p = x.c + Vec2{-x.fc * 80.0f + gx, -70.0f + gy};
    const float on = smoothstep((x.mf - 22 - vfx::hash01(i * 3 + 2) * 14.0f) / 6.0f) * (0.75f + 0.25f * std::sin(x.t * 20.0f + i));
    if (on <= 0.01f) continue;
    vfx::glow(p, 10.0f * on, A({120, 180, 255, 255}, 0.8f * on * x.k), A({120, 180, 255, 255}, 0.0f), 8);
    vfx::glow(p, 3.5f * on, A(WHITE, on * x.k), A(WHITE, 0.0f), 6);
  }
  vfx::additive(false);
}

// A wolf howling at the moon: a black silhouette on the ridge.
void howlingWolf(Vec2 b, float sz, float side, Color ink) {
  glowE(b, sz * 0.9f, sz * 0.36f, ink, ink, 16);
  vfx::wedge(b + Vec2{side * sz * 0.5f, -sz * 0.1f}, b + Vec2{side * sz * 0.9f, -sz * 1.0f}, sz * 0.36f, sz * 0.2f, ink, ink);
  vfx::wedge(b + Vec2{side * sz * 0.86f, -sz * 0.98f}, b + Vec2{side * sz * 1.05f, -sz * 1.45f}, sz * 0.18f, sz * 0.05f, ink, ink);
  vfx::tri(b + Vec2{side * sz * 0.7f, -sz * 1.0f}, b + Vec2{side * sz * 0.82f, -sz * 0.95f}, b + Vec2{side * sz * 0.66f, -sz * 1.3f},
           ink, ink, ink);
  vfx::wedge(b + Vec2{-side * sz * 0.8f, 0.0f}, b + Vec2{-side * sz * 1.4f, -sz * 0.2f}, sz * 0.18f, sz * 0.04f, ink, ink);
  for (int i = 0; i < 4; ++i)
    vfx::wedge(b + Vec2{(i < 2 ? 0.5f : -0.5f) * sz + (i % 2) * 8.0f, 0.0f}, b + Vec2{(i < 2 ? 0.5f : -0.5f) * sz, sz * 0.7f},
               sz * 0.14f, sz * 0.1f, ink, ink);
}

void losLobos(const Ctx& x, bool front) {
  if (front) return;
  sky(x.v, {4, 6, 18, 255}, {30, 40, 70, 255}, x.k);
  moon(x.c + Vec2{x.fc * 240.0f, -360.0f}, 170.0f, {230, 236, 250, 255}, {4, 6, 18, 255}, 3.0f, x.k);
  const Color ink = A({6, 8, 16, 255}, 0.95f * x.k);
  const float ridge = x.f.pos.y + 90.0f;
  vfx::quad({x.v.x, ridge + 30.0f}, {x.v.right(), ridge - 40.0f}, {x.v.right(), x.v.bottom()}, {x.v.x, x.v.bottom()}, ink, ink, ink, ink);
  for (int i = 0; i < 5; ++i) {
    const float wx = x.v.x + x.v.w * (0.12f + 0.19f * i);
    const float wy = ridge + 30.0f - 70.0f * (wx - x.v.x) / x.v.w;
    howlingWolf({wx, wy - 26.0f}, 40.0f + 10.0f * vfx::hash01(i * 5 + 1), i % 2 ? 1.0f : -1.0f, ink);
  }
  vfx::additive(true);
  for (int i = 0; i < 3; ++i)  // mist over the ridge
    glowE({x.v.x + x.v.w * (0.2f + 0.3f * i) + std::sin(x.t * 0.5f + i) * 60.0f, ridge}, 420.0f, 60.0f,
          A({140, 170, 230, 255}, 0.18f * x.k), A({140, 170, 230, 255}, 0.0f));
  vfx::additive(false);
}

void starrkRelease(const Ctx& x, bool front) {
  if (front) return;
  sky(x.v, {24, 30, 46, 255}, {110, 122, 146, 255}, x.k);
  const Vec2 l = x.c + Vec2{-x.fc * 170.0f, -130.0f};  // Lilynette turns into light that flows into his hands
  const float u = smoothstep((x.mf - 18) / 30.0f);
  vfx::additive(true);
  glowE(l, 30.0f + 30.0f * (1.0f - u), 60.0f * (1.0f - u) + 12.0f, A(WHITE, 0.7f * (1.0f - u) * x.k), A(WHITE, 0.0f));
  for (int sIdx = 0; sIdx < 5; ++sIdx) {
    Vec2 pts[12];
    for (int i = 0; i < 12; ++i) {
      const float v = i / 11.0f * u;
      pts[i] = lerpv(l, x.c + Vec2{x.fc * 30.0f, -10.0f}, v) + Vec2{0.0f, std::sin(v * 9.0f + sIdx * 1.3f + x.t * 4.0f) * 26.0f};
    }
    polyline(pts, 12, 10.0f, 3.0f, A({200, 225, 255, 255}, 0.0f), A({230, 240, 255, 255}, 0.7f * x.k));
  }
  for (int i = 0; i < 8; ++i) {  // the spirits of the pack circling
    const float a = x.t * 0.8f + i * kPi / 4.0f;
    vfx::glow(x.c + Vec2{std::cos(a) * 330.0f, std::sin(a) * 120.0f - 60.0f}, 34.0f, A({150, 196, 255, 255}, 0.4f * x.k),
              A({150, 196, 255, 255}, 0.0f), 14);
  }
  vfx::additive(false);
}

void susanooSky(const Ctx& x, bool front, Color top, Color bottom) {
  if (front) return;
  sky(x.v, top, bottom, x.k);
}
}  // namespace

bool drawUltCinematic(const Match& m, const Fighter& f, UltFx fx, float k, int mf, float time, const Rect& v, bool front) {
  const Ctx x{m, f, k, mf, time, v, f.center(), float(f.facing)};
  switch (fx) {
    case UltFx::BlackMeteorite: blackMeteorite(x, front); return true;
    case UltFx::DemonUnion: demonUnion(x, front); return true;
    case UltFx::WindBladeShower: if (front) windRain(x); return false;  // + Yuno's spirit storm
    case UltFx::GiantGetsuga: giantGetsuga(x, front); return true;
    case UltFx::GetsugaJujisho: getsugaJujisho(x, front); return true;
    case UltFx::VastoLordeCero: vastoLorde(x, front); return true;
    case UltFx::Hakuteiken: hakuteiken(x, front); return true;
    case UltFx::LightDragon: if (front) lightDragon(x); return false;  // + Link's seal
    case UltFx::MasterSwordBeam: if (!front) masterSword(x); return false;
    case UltFx::LightArrowRain: lightArrowRain(x, front); return true;
    case UltFx::DemonKingRampage: case UltFx::GloomPillars: if (!front) gloomNoDragon(x); return !front;
    case UltFx::Fimbulwinter: fimbulwinter(x, front); return true;
    case UltFx::ChaosWhip: if (front) chaosWhip(x); return false;  // + Kratos's rage
    case UltFx::MaxBlue: maxBlue(x, front); return true;
    case UltFx::ImaginaryPurple: imaginaryPurple(x, front); return true;
    case UltFx::KamutokeStrike: kamutoke(x, front); return true;
    case UltFx::FugaBlast: fuga(x, front); return true;
    case UltFx::Kendo: kendo(x, front); return true;
    case UltFx::NozarashiCleave: nozarashi(x, front); return true;
    case UltFx::RedDemon: redDemon(x, front); return true;
    case UltFx::CeroOscuras: ceroOscuras(x, front); return true;
    case UltFx::LanzaRelampago: lanza(x, front); return true;
    case UltFx::SegundaEtapa: segunda(x, front); return true;
    case UltFx::TrueChargedSlash: trueCharged(x, front); return true;
    case UltFx::AmpedDischarge: ampedDischarge(x, front); return true;
    case UltFx::WyvernFire: wyvernFire(x, front); return true;
    case UltFx::UzumakiBarrage: uzumaki(x, front); return true;
    case UltFx::Rasenshuriken: rasenshuriken(x, front); return true;
    case UltFx::KuramaBijudama: kurama(x, front); return true;
    case UltFx::Kirin: kirin(x, front); return true;
    case UltFx::IndraArrow: indra(x, front); return true;
    case UltFx::Amaterasu: amaterasu(x, front); return true;
    case UltFx::Raikiri: raikiri(x, front); return true;
    case UltFx::Kamui: kamui(x, front); return true;
    case UltFx::KamuiShuriken: susanooSky(x, front, {4, 10, 30, 255}, {20, 40, 90, 255}); return true;
    case UltFx::GranReyCero: granReyCero(x, front); return true;
    case UltFx::PanteraRelease: panteraRelease(x, front); return true;
    case UltFx::DesgarronClaws: desgarronClaws(x, front); return true;
    case UltFx::CeroMetralleta: ceroMetralleta(x, front); return true;
    case UltFx::LosLobos: losLobos(x, front); return true;
    case UltFx::StarrkRelease: starrkRelease(x, front); return true;
    default: return false;
  }
}

// ---- Transformations: the avatars around the fighter ----------------------------------------------------
// Drawn behind (front = false) and over (front = true) the fighter's sprite.
void drawFormAvatar(const Fighter& f, float time, bool front) {
  const Form form = f.formKind();
  if (form == Form::None || !f.active()) return;
  const Vec2 c = f.center();
  const float H = f.height(), fc = float(f.facing);
  switch (form) {
    case Form::SusanooPurple: case Form::SusanooBlue: {  // the giant warrior of chakra: ribcage, arms, horned head
      const Color col = form == Form::SusanooPurple ? Color{170, 90, 255, 255} : Color{80, 150, 255, 255};
      const float k = front ? 0.25f : 0.45f;
      const Vec2 chest = c + Vec2{0.0f, -H * 0.55f}, head = chest + Vec2{fc * 20.0f, -H * 1.0f};
      vfx::additive(true);
      glowE(chest, H * 1.2f, H * 1.3f, A(col, 0.3f * k), A(col, 0.0f));
      for (int i = 0; i < 4; ++i) {  // ribs
        const float y = chest.y - H * 0.3f + i * H * 0.28f;
        for (int s = -1; s <= 1; s += 2)
          vfx::wedge({chest.x, y}, {chest.x + s * H * (0.7f - i * 0.08f), y + H * 0.2f}, 14.0f, 4.0f, A(col, 0.8f * k), A(col, 0.4f * k));
      }
      vfx::wedge(chest + Vec2{0.0f, -H * 0.5f}, chest + Vec2{0.0f, H * 0.9f}, 20.0f, 10.0f, A(col, 0.8f * k), A(col, 0.6f * k));
      glowE(head, H * 0.32f, H * 0.38f, A(col, 0.8f * k), A(col, 0.3f * k), 20);
      for (int s = -1; s <= 1; s += 2)
        vfx::wedge(head + Vec2{s * H * 0.2f, -H * 0.2f}, head + Vec2{s * H * 0.55f, -H * 0.75f}, 16.0f, 2.0f, A(col, k), A(col, 0.2f * k));
      vfx::glow(head + Vec2{fc * H * 0.1f, 0.0f}, H * 0.08f, A(WHITE, 2.0f * k), A(col, 0.0f), 10);
      for (int s = -1; s <= 1; s += 2) {  // great arms
        const Vec2 sh = chest + Vec2{s * H * 0.75f, -H * 0.3f};
        const Vec2 hand = sh + Vec2{fc * H * 0.6f, H * (0.3f + 0.05f * std::sin(time * 2.0f + s))};
        vfx::wedge(sh, hand, 36.0f, 26.0f, A(col, 0.6f * k), A(col, 0.5f * k));
      }
      vfx::additive(false);
      break;
    }
    case Form::KuramaAvatar: {  // a cloak of golden chakra with the fox's head over him
      const Color gold{255, 180, 40, 255};
      vfx::additive(true);
      glowE(c, H * 0.7f, H * 0.85f, A(gold, front ? 0.12f : 0.4f), A(gold, 0.0f));
      if (!front) {
        const Vec2 h = c + Vec2{fc * 10.0f, -H * 1.05f};
        glowE(h, H * 0.5f, H * 0.4f, A(gold, 0.35f), A(gold, 0.0f));
        for (int s = -1; s <= 1; s += 2)
          vfx::tri(h + Vec2{s * H * 0.2f, -H * 0.15f}, h + Vec2{s * H * 0.45f, -H * 0.1f}, h + Vec2{s * H * 0.4f, -H * 0.6f},
                   A(gold, 0.5f), A(gold, 0.5f), A(gold, 0.0f));
        for (int i = 0; i < 9; ++i) {  // tails of chakra
          const float a = -kPi * 0.5f - fc * (0.8f + i * 0.17f) + std::sin(time * 3.0f + i) * 0.1f;
          vfx::streak(c + Vec2{0.0f, H * 0.2f}, c + Vec2{0.0f, H * 0.2f} + dirOf(a) * (H * 1.1f), H * 0.2f, 4.0f, A(gold, 0.35f),
                      A(gold, 0.0f));
        }
      }
      vfx::additive(false);
      break;
    }
    case Form::Segunda: {  // great black bat wings and a tail
      if (front) break;
      const Vec2 back = c + Vec2{-fc * H * 0.1f, -H * 0.25f};
      const Color ink{14, 16, 14, 255}, rim{80, 220, 130, 255};
      wing(back, -1.0f, H * 1.4f, time, A(ink, 0.95f), A(rim, 0.5f));
      wing(back, 1.0f, H * 1.4f, time + 0.4f, A(ink, 0.95f), A(rim, 0.5f));
      Vec2 tail[8];
      for (int i = 0; i < 8; ++i) {
        const float u = i / 7.0f;
        tail[i] = c + Vec2{-fc * (H * 0.1f + u * H * 0.9f), H * 0.2f + std::sin(time * 3.0f + u * 4.0f) * H * 0.15f * u};
      }
      polyline(tail, 8, H * 0.06f, 2.0f, A(ink, 0.95f), A(ink, 0.95f));
      break;
    }
    case Form::DemonUnion: {  // one black wing of the devil
      if (front) break;
      const Vec2 back = c + Vec2{-fc * H * 0.12f, -H * 0.25f};
      wing(back, -fc, H * 1.2f, time, A({10, 10, 12, 255}, 0.95f), A({220, 220, 230, 255}, 0.4f), 5);
      break;
    }
    case Form::Pantera: {  // crackling cyan reiatsu around the panther
      if (!front) break;
      const uint32_t fs = uint32_t(time * 12.0f) * 97u + uint32_t(f.id) * 13u;
      vfx::additive(true);
      for (int i = 0; i < 3; ++i) {
        const float a = vfx::hash01(fs + i) * 2.0f * kPi;
        const Vec2 p0 = c + dirOf(a) * (H * 0.3f);
        vfx::bolt(p0, p0 + dirOf(a + 0.5f) * (H * 0.3f), 8.0f, 4, fs + i * 7u, 2.0f, A({120, 240, 255, 255}, 0.7f));
      }
      vfx::additive(false);
      break;
    }
    case Form::SpartanRage: case Form::RedDemon: {  // burning aura
      const Color col = form == Form::SpartanRage ? Color{255, 110, 30, 255} : Color{255, 40, 40, 255};
      vfx::additive(true);
      for (int i = 0; i < 7; ++i) {
        const float ph = std::fmod(time * 1.8f + i / 7.0f, 1.0f);
        const Vec2 p = c + Vec2{(i - 3) * H * 0.09f + std::sin(time * 5.0f + i) * H * 0.04f, H * 0.4f - ph * H * 1.2f};
        vfx::glow(p, H * 0.12f * (1.0f - ph) + 2.0f, A(col, (front ? 0.25f : 0.5f) * (1.0f - ph)), A(col, 0.0f), 10);
      }
      vfx::additive(false);
      break;
    }
    default: break;
  }
}
