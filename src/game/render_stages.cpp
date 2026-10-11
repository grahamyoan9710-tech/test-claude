// The anime worlds: Las Noches and the Seireitei (Bleach), the Black Bulls' hideout and the Grimoire Tower
// (Black Clover), Shibuya and Jujutsu High (Jujutsu Kaisen), Konoha and the Valley of the End (Naruto).
// Each one bakes its sky and a few parallax layers once, then draws its platforms and light animated
// touches (particles, water, neon, rain) every frame, in the painterly look of the original stages.
#include "game/render.hpp"
#include "game/stage_art.hpp"

using namespace paint;

namespace {
constexpr Color rgb(int r, int g, int b) { return {(unsigned char)r, (unsigned char)g, (unsigned char)b, 255}; }
inline Color A(Color c, float a) { return withAlpha(c, a); }

// ---- Shared motifs -----------------------------------------------------------------------------
// Crescent moon lit from the angle `toward`: the terminator bulges toward the light by m (0 = half moon,
// 1 = new moon).
void crescentMoon(Vector2 c, float r, float toward, float m, Color lit, Color rim) {
  constexpr int kSeg = 32;
  const Vec2 t{std::cos(toward), std::sin(toward)}, p{-t.y, t.x};
  Vector2 out[kSeg + 1], in[kSeg + 1];
  for (int i = 0; i <= kSeg; ++i) {
    const float th = (float(i) / kSeg * 2.0f - 1.0f) * kPi * 0.5f, along = r * std::cos(th), side = r * std::sin(th);
    out[i] = {c.x + t.x * along + p.x * side, c.y + t.y * along + p.y * side};
    in[i] = {c.x + t.x * along * m + p.x * side, c.y + t.y * along * m + p.y * side};
  }
  for (int i = 0; i < kSeg; ++i) gquad(out[i], out[i + 1], in[i + 1], in[i], rim, rim, lit, lit);
}

// Dune profile 0..1, tileable over `period`: sharp wind-blown crests, smooth troughs.
float duneProfile(float x, float period, int seed) {
  const float u = x / period * 2.0f * kPi, s = float(seed);
  const float crest = 1.0f - std::fabs(std::sin(u * 2.0f + s));
  return 0.55f * std::pow(crest, 1.6f) + 0.3f * (0.5f + 0.5f * std::sin(u * 3.0f + s * 1.3f)) +
         0.15f * (0.5f + 0.5f * std::sin(u * 7.0f + s * 2.1f));
}
// Dunes in moonlight coming from the right: a body of sand, the faces turned to the light brightened and
// the others darkened along the surface (smooth troughs, crisp crests).
void dunes(float w, float base, float amp, int seed, Color lit, Color dark, Color low, float lowY) {
  const float step = 5.0f, fade = amp * 0.75f;
  auto yAt = [&](float x) { return base - amp * duneProfile(x, w, seed); };
  auto tone = [&](float x) {
    const float k = clampf((yAt(x + 1.5f) - yAt(x - 1.5f)) / 3.0f * 2.2f, -1.0f, 1.0f);  // > 0: facing the moon
    return k > 0.0f ? A(lit, 0.9f * k) : A(dark, -0.75f * k);
  };
  const Color body = mixColor(dark, lit, 0.4f);
  for (float x = 0; x < w; x += step) {
    const float y0 = yAt(x), y1 = yAt(x + step);
    gquad({x, y0}, {x + step, y1}, {x + step, lowY}, {x, lowY}, body, body, low, low);
    const Color c0 = tone(x), c1 = tone(x + step);
    gquad({x, y0}, {x + step, y1}, {x + step, y1 + fade}, {x, y0 + fade}, c0, c1, A(c1, 0.0f), A(c0, 0.0f));
    if (c0.a > 150 && c1.a > 150 && c0.r == lit.r) DrawLineEx({x, y0}, {x + step, y1}, 1.5f, A(mixColor(lit, WHITE, 0.3f), 0.5f));
  }
}

// Tapered segment from a to b (widths w0 -> w1), lit on its left side.
void limb(Vector2 a, Vector2 b, float w0, float w1, Color lit, Color dark) {
  Vec2 d{b.x - a.x, b.y - a.y};
  const float len = std::max(0.001f, std::sqrt(d.x * d.x + d.y * d.y));
  const Vec2 n{-d.y / len, d.x / len};
  const Vector2 a0{a.x + n.x * w0 * 0.5f, a.y + n.y * w0 * 0.5f}, a1{a.x - n.x * w0 * 0.5f, a.y - n.y * w0 * 0.5f};
  const Vector2 b0{b.x + n.x * w1 * 0.5f, b.y + n.y * w1 * 0.5f}, b1{b.x - n.x * w1 * 0.5f, b.y - n.y * w1 * 0.5f};
  gquad(a0, b0, b, a, dark, dark, mixColor(dark, lit, 0.5f), mixColor(dark, lit, 0.5f));
  gquad(a, b, b1, a1, mixColor(dark, lit, 0.5f), mixColor(dark, lit, 0.5f), lit, lit);
}

// Hueco Mundo's quartz tree: white crystal, straight angular branches ending in points.
void quartzBranch(Vector2 p, float ang, float len, float wid, int depth, int seed, Color lit, Color dark) {
  const Vector2 q{p.x + std::cos(ang) * len, p.y + std::sin(ang) * len};
  if (depth == 0) {
    const Vec2 n{-std::sin(ang), std::cos(ang)};
    tri({p.x + n.x * wid * 0.5f, p.y + n.y * wid * 0.5f}, {p.x - n.x * wid * 0.5f, p.y - n.y * wid * 0.5f}, q,
        mixColor(dark, lit, 0.7f));
    return;
  }
  limb(p, q, wid, wid * 0.7f, lit, dark);
  const int kids = 2 + (hash(seed) > 0.6f);
  for (int k = 0; k < kids; ++k) {
    const float spread = (k - (kids - 1) * 0.5f) * (0.55f + 0.25f * hash(seed + k * 7));
    const float t = 0.55f + 0.45f * hash(seed + k * 13);
    const Vector2 s{lerpf(p.x, q.x, t), lerpf(p.y, q.y, t)};
    quartzBranch(k == kids - 1 ? q : s, ang + spread, len * (0.55f + 0.2f * hash(seed + k * 3)), wid * 0.62f,
                 depth - 1, seed * 7 + k * 31 + 5, lit, dark);
  }
}
void quartzTree(Vector2 base, float h, int seed, Color lit, Color dark) {
  glow({base.x, base.y - h * 0.5f}, h * 0.55f, rgb(150, 200, 255), 0.06f);
  quartzBranch(base, -kPi * 0.5f + (hash(seed) - 0.5f) * 0.2f, h * 0.45f, h * 0.09f, 3, seed, lit, dark);
}

// Sand pouring off a ledge: a fading veil and grains streaming down.
void sandfall(float x, float y0, float len, float w, float time, Color sand) {
  gquad({x - w * 0.5f, y0}, {x + w * 0.5f, y0}, {x + w * 1.1f, y0 + len}, {x - w * 1.1f, y0 + len}, A(sand, 0.85f),
        A(sand, 0.85f), A(sand, 0.0f), A(sand, 0.0f));
  for (int k = 0; k < 12; ++k) {
    const float t = wrapf(time * 0.55f + k / 12.0f, 1.0f);
    dot({x + (hash(k * 7) - 0.5f) * w * (0.6f + 1.2f * t), y0 + t * len}, 1.6f, A(mixColor(sand, WHITE, 0.3f), 0.9f * (1.0f - t)));
  }
}

// ==== Las Noches (Bleach): Hueco Mundo by night ===================================================
// Kept in mid-dark moonlit blues so that the fighters (many dressed in white) stand out.
constexpr Color kLnHorizon{40, 46, 74, 255};
constexpr Color kLnLow{34, 38, 62, 255};      // foot of the near dunes
constexpr Color kLnSand{168, 176, 202, 255};  // moonlit sand

// The palace of the Espada: an immense white dome and its towers, far away under the moon (upper right).
void lasNochesDome(Vector2 c, float rx, float ry) {
  const Color lit = rgb(156, 166, 198), dark = rgb(50, 56, 88), mid = mixColor(dark, lit, 0.5f);
  const Vec2 L{0.62f, -0.78f};
  // Towers first: they rise from behind the dome and from its crown.
  struct Tower {
    float x, w, h;
  };
  const Tower towers[6] = {{-0.62f, 16, 120}, {-0.34f, 20, 170}, {-0.12f, 26, 230}, {0.06f, 34, 330}, {0.24f, 22, 200},
                           {0.5f, 16, 130}};
  for (const Tower& t : towers) {
    const float x = c.x + t.x * rx, base = c.y - ry * std::sqrt(std::max(0.0f, 1.0f - t.x * t.x)) * 0.9f;
    const float top = base - t.h;
    const Color tl = mixColor(dark, lit, 0.92f), td = mixColor(dark, lit, 0.3f);
    gquad({x - t.w * 0.5f, top}, {x, top}, {x, base + 20}, {x - t.w * 0.5f, base + 20}, td, td, dark, dark);
    gquad({x, top}, {x + t.w * 0.5f, top}, {x + t.w * 0.5f, base + 20}, {x, base + 20}, tl, tl, mid, mid);
    ellipse({x, top}, t.w * 0.56f, t.w * 0.16f, tl);  // a ledge, then a slender spire
    tri({x - t.w * 0.38f, top}, {x + t.w * 0.38f, top}, {x + t.w * 0.04f, top - t.w * 1.4f}, mixColor(dark, lit, 0.75f));
    tri({x, top}, {x + t.w * 0.38f, top}, {x + t.w * 0.04f, top - t.w * 1.4f}, tl);
    for (float y = top + 12; y < base - 6; y += 19) DrawRectangleRec({x + t.w * 0.1f, y, std::max(2.0f, t.w * 0.12f), 7}, A(rgb(24, 28, 50), 0.75f));
  }
  constexpr int n = 48;
  for (int i = 0; i < n; ++i) {  // the dome: a fan from the base, shaded by the angle to the moon
    const float a0 = kPi + kPi * i / n, a1 = kPi + kPi * (i + 1) / n;
    const Vector2 p0{c.x + std::cos(a0) * rx, c.y + std::sin(a0) * ry}, p1{c.x + std::cos(a1) * rx, c.y + std::sin(a1) * ry};
    const float k0 = clampf(0.45f + 0.65f * (std::cos(a0) * L.x + std::sin(a0) * L.y), 0, 1);
    const float k1 = clampf(0.45f + 0.65f * (std::cos(a1) * L.x + std::sin(a1) * L.y), 0, 1);
    gtri(c, p0, p1, mid, mixColor(dark, lit, k0), mixColor(dark, lit, k1));
  }
  for (int k = 1; k <= 4; ++k) {  // stone courses: flattened rings around the dome
    const float t = k / 5.0f, yy = c.y - ry * t, rr = rx * std::sqrt(1.0f - t * t);
    Vector2 prev{c.x - rr, yy};
    for (int i = 1; i <= 24; ++i) {
      const float an = kPi - kPi * i / 24.0f;
      const Vector2 p{c.x + std::cos(an) * rr, yy + std::sin(an) * ry * 0.05f};
      DrawLineEx(prev, p, 1.4f, A(dark, 0.5f));
      prev = p;
    }
  }
  for (int k = -3; k <= 3; ++k) {  // ribs running up to the crown
    const float phi = k * 0.38f;
    Vector2 prev{c.x + rx * std::sin(phi), c.y};
    for (int i = 1; i <= 16; ++i) {
      const float t = i / 16.0f * kPi * 0.5f;
      const Vector2 p{c.x + rx * std::sin(phi) * std::cos(t), c.y - ry * std::sin(t)};
      DrawLineEx(prev, p, 1.2f, A(dark, 0.4f));
      prev = p;
    }
  }
  // Moonlight on the dome's shoulder, haze at its foot.
  glow({c.x + rx * 0.35f, c.y - ry * 0.7f}, rx * 0.45f, ry * 0.35f, A(rgb(200, 214, 255), 0.18f), A(rgb(200, 214, 255), 0.0f));
  vgrad(c.x - rx - 10, c.y - ry * 0.35f, rx * 2 + 20, ry * 0.36f, A(kLnHorizon, 0.0f), A(kLnHorizon, 0.75f));
}

// A white pillar of the palace standing out of the sand, its top broken off or crowned.
void lasNochesPillar(Vector2 base, float w, float h, Color lit, Color dark, bool broken) {
  const float top = base.y - h;
  gquad({base.x - w * 0.5f, top}, {base.x + w * 0.1f, top}, {base.x + w * 0.1f, base.y}, {base.x - w * 0.5f, base.y},
        mixColor(dark, lit, 0.35f), mixColor(dark, lit, 0.35f), dark, dark);
  gquad({base.x + w * 0.1f, top}, {base.x + w * 0.5f, top}, {base.x + w * 0.5f, base.y}, {base.x + w * 0.1f, base.y}, lit, lit,
        mixColor(dark, lit, 0.55f), mixColor(dark, lit, 0.55f));
  if (broken) {
    tri({base.x - w * 0.5f, top}, {base.x + w * 0.5f, top}, {base.x + w * 0.2f, top - w * 0.7f}, mixColor(dark, lit, 0.6f));
    tri({base.x - w * 0.5f, top}, {base.x - w * 0.1f, top}, {base.x - w * 0.35f, top - w * 0.35f}, mixColor(dark, lit, 0.4f));
  } else {
    DrawRectangleRec({base.x - w * 0.64f, top - w * 0.22f, w * 1.28f, w * 0.24f}, lit);
    DrawRectangleRec({base.x - w * 0.64f, top + w * 0.02f, w * 1.28f, w * 0.06f}, A(dark, 0.8f));
  }
}

void bakeLasNoches(Art& a) {
  a.sky = bakeOpaque(1360, 800, 1.0f, [&] {
    gradient(1360, 800, {{0.0f, rgb(3, 4, 12)}, {0.32f, rgb(7, 10, 24)}, {0.55f, rgb(16, 21, 42)}, {0.66f, rgb(30, 36, 62)},
                         {0.71f, kLnHorizon}, {1.0f, kLnHorizon}});
    for (int i = 0; i < 170; ++i) {  // stars, denser up high
      const float x = hash(i * 3) * 1360, y = std::pow(hash(i * 7), 1.4f) * 520, r = 0.6f + 1.3f * std::pow(hash(i * 11), 3.0f);
      if (r > 1.3f) glow({x, y}, r * 4.0f, rgb(190, 210, 255), 0.25f, 10);
      dot({x, y}, r, A(rgb(230, 236, 255), 0.45f + 0.5f * hash(i * 13)));
    }
    const Vector2 moon{1060, 150};  // the eternal crescent moon
    glow(moon, 440, 380, A(rgb(150, 170, 220), 0.14f), A(rgb(150, 170, 220), 0.0f));
    glow(moon, 140, rgb(220, 230, 255), 0.26f);
    crescentMoon(moon, 62, kPi * 0.82f, 0.45f, rgb(250, 252, 255), rgb(196, 208, 238));
    dunes(1360, 568, 34, 3, rgb(104, 112, 146), rgb(46, 52, 82), kLnHorizon, 800);
    lasNochesPillar({110, 566}, 22, 190, rgb(132, 140, 172), rgb(48, 54, 84), false);
    lasNochesPillar({1250, 566}, 18, 130, rgb(126, 134, 166), rgb(46, 52, 82), true);
    lasNochesDome({620, 566}, 380, 205);
    for (int i = 0; i < 7; ++i)  // a far forest of quartz trees
      quartzTree({40 + i * 200 + hash(i * 5) * 90, 586 + hash(i) * 10}, 36 + hash(i * 3) * 26, 40 + i, rgb(126, 136, 170),
                 rgb(56, 62, 94));
    dunes(1360, 650, 46, 11, rgb(116, 124, 158), rgb(42, 48, 78), kLnHorizon, 800);
  });
  // Mid dunes: quartz trees and broken pillars of the palace.
  a.mid = bake(2000, 440, 1.25f, [&] {
    const Color lit = rgb(150, 160, 194), dark = rgb(56, 62, 96);
    const float px[5] = {210, 560, 1180, 1560, 1830}, ph[5] = {150, 90, 120, 200, 70};
    for (int k = 0; k < 5; ++k)
      lasNochesPillar({px[k], 268 - 26 * duneProfile(px[k], 2000, 5)}, 26 + 6 * hash(k), ph[k], lit, dark, k % 2 == 0);
    for (int i = 0; i < 12; ++i) {
      const float x = 60 + i * 165 + hash(i * 17) * 80;
      quartzTree({x, 262 - 40 * duneProfile(x, 2000, 5)}, 70 + hash(i * 7) * 80, 100 + i * 3, lit, dark);
    }
    dunes(2000, 300, 60, 5, rgb(132, 140, 174), rgb(46, 52, 84), rgb(40, 44, 72), 440);
  });
  // Near dunes with big quartz trees at the edges of the screen.
  a.near = bake(1600, 320, 1.25f, [&] {
    const Color lit = rgb(176, 186, 218), dark = rgb(60, 66, 100);
    const float tx[3] = {110, 790, 1430}, th[3] = {230, 160, 250};
    for (int k = 0; k < 3; ++k)
      wrapX(tx[k], 160, 1600, [&](float x) { quartzTree({x, 186 - 20 * duneProfile(x, 1600, 17)}, th[k], 300 + k * 11, lit, dark); });
    dunes(1600, 230, 80, 17, rgb(150, 158, 192), rgb(44, 48, 78), kLnLow, 320);
    for (int i = 0; i < 46; ++i) {  // ripples drawn by the wind
      const float x = hash(i * 3) * 1600, y = 236 + hash(i * 5) * 70;
      wrapX(x, 40, 1600, [&](float xx) { DrawLineEx({xx - 28, y}, {xx + 28, y - 3}, 1.5f, A(rgb(96, 104, 140), 0.55f)); });
    }
  });
}

void bgLasNoches(const Art& a, Vec2 pc, float skyX, float skyY, float time) {
  drawBackdrop(a.sky, skyX, skyY);
  drawLayer(a.mid, -360.0f - pc.x * 0.12f, 300.0f - pc.y * 0.12f, false);
  const float ny = 420.0f - pc.y * 0.22f;
  drawLayer(a.near, -pc.x * 0.22f, ny, true);
  if (ny + 319.0f < kScreenH) DrawRectangleRec({0, ny + 319.0f, float(kScreenW), kScreenH - ny - 319.0f}, kLnLow);
  for (int i = 0; i < 5; ++i) {  // gusts of sand skimming over the dunes
    const float t = wrapf(time * 0.09f + i * 0.21f, 1.0f), x = kScreenW + 200.0f - t * (kScreenW + 600.0f);
    const float y = ny + 150.0f + 40.0f * hash(i * 7), len = 260.0f + 160.0f * hash(i * 3), k = std::sin(t * kPi);
    gquad({x, y - 3}, {x + len, y - 8}, {x + len, y - 4}, {x, y + 4}, A(kLnSand, 0.16f * k), A(kLnSand, 0.0f), A(kLnSand, 0.0f),
          A(kLnSand, 0.16f * k));
  }
  motes(34, 51, pc, time, -36.0f, 3.0f, rgb(200, 208, 236), rgb(150, 200, 255), 1.0f);  // blowing sand, reishi
}

// Palace stone in the moonlight: a tiered underside breaking off into a point, sand pouring from the cracks.
void lasNochesSolid(const Rect& r, float time, int seed) {
  const float L = r.left(), R = r.right(), T = r.top(), w = r.w, h = r.h, cx = L + w * 0.5f;
  const Color face0 = rgb(134, 142, 174), face1 = rgb(86, 92, 124), seam = rgb(52, 56, 86), deep = rgb(30, 32, 56);
  float y = T + h;
  const float tierH = 30.0f;
  float iw = w;
  for (int s = 0; s < 3; ++s) {
    iw = (w - 24) * (0.9f - 0.19f * s);
    vgrad(cx - iw * 0.5f, y, iw, tierH, shade(face1, 1.0f - 0.14f * s), shade(face1, 0.78f - 0.14f * s));
    DrawRectangleRec({cx - iw * 0.5f, y, iw, 3}, A(face0, 0.5f));
    DrawRectangleRec({cx - iw * 0.5f, y + tierH - 3, iw, 3}, A(deep, 0.6f));
    for (float x = cx - iw * 0.5f + 34 + 17 * s; x < cx + iw * 0.5f - 16; x += 58) DrawRectangleRec({x, y + 5, 2, tierH - 10}, A(seam, 0.6f));
    y += tierH;
  }
  constexpr int kN = 9;  // jagged broken point
  Vector2 jag[kN + 1];
  for (int i = 0; i <= kN; ++i) {
    const float u = float(i) / kN;
    jag[i] = {cx - iw * 0.5f + iw * u, y + std::sin(u * kPi) * 70.0f * (0.6f + 0.4f * hash(seed + i * 7)) * (i % 2 ? 1.0f : 0.7f)};
  }
  for (int i = 0; i < kN; ++i)
    gquad({jag[i].x, y}, {jag[i + 1].x, y}, jag[i + 1], jag[i], shade(face1, 0.5f), shade(face1, 0.5f), deep, deep);
  sandfall(cx - iw * 0.3f, y + 10, 190, 9, time, kLnSand);
  sandfall(cx + iw * 0.36f, y + 4, 150, 7, time + 0.4f, kLnSand);
  // Front face with long recessed panels and a faint line of reiatsu.
  gquad({L, T}, {R, T}, {R - 10, T + h}, {L + 10, T + h}, face0, face0, face1, face1);
  DrawRectangleRec({L + 8, T + h - 9, w - 16, 3}, A(deep, 0.5f));
  for (float x = L + 70; x < R - 40; x += 120) {
    DrawRectangleRec({x, T + 18, 90, h - 34}, A(seam, 0.3f));
    DrawRectangleLinesEx({x, T + 18, 90, h - 34}, 1.5f, A(seam, 0.7f));
    DrawRectangleRec({x + 2, T + h - 18, 88, 2}, A(face0, 0.5f));
  }
  const float g = 0.5f + 0.5f * std::sin(time * 1.4f);
  DrawRectangleRec({L + 12, T + h * 0.5f - 1, w - 24, 2}, A(rgb(120, 210, 255), 0.22f + 0.2f * g));
  // The walking surface: a pale coping catching the moon, drifts of sand on it.
  Rectangle cap{L - 6, T - 5, w + 12, 14};
  DrawRectangleRounded({cap.x, cap.y + 4, cap.width, cap.height}, 0.5f, 6, rgb(70, 76, 108));
  DrawRectangleRounded(cap, 0.5f, 6, rgb(178, 186, 214));
  DrawRectangleRounded({cap.x + 3, cap.y, cap.width - 6, 4}, 1.0f, 6, rgb(222, 228, 246));
  for (int i = 0; i < int(w / 120.0f); ++i) {
    const float x = L + 30 + hash(seed + i * 5) * (w - 60), dw = 20 + 26 * hash(seed + i * 9);
    ellipse({x, T - 4}, dw, 3.5f, A(rgb(196, 202, 224), 0.85f));
  }
}

// A broken white column carrying a slab, held up by a faint glow of reiatsu.
void lasNochesThin(const Platform& p, float time, int seed) {
  const Rect& r = p.rect;
  const float L = r.x, T = r.y, w = r.w, cx = L + w * 0.5f;
  const float cw = w * 0.32f, g = 0.6f + 0.4f * std::sin(time * 2.0f + seed);
  glow({cx, T + 64}, w * 0.5f, 64, A(rgb(110, 200, 255), 0.16f * g), A(rgb(110, 200, 255), 0.0f));
  const Color lit = rgb(132, 140, 172), dark = rgb(62, 68, 100);
  gquad({cx - cw * 0.5f, T + 8}, {cx, T + 8}, {cx, T + 66}, {cx - cw * 0.5f, T + 60}, dark, dark, shade(dark, 0.7f), shade(dark, 0.7f));
  gquad({cx, T + 8}, {cx + cw * 0.5f, T + 8}, {cx + cw * 0.5f, T + 56}, {cx, T + 66}, lit, lit, shade(lit, 0.75f), shade(lit, 0.75f));
  for (int k = 0; k < 4; ++k) {  // broken end
    const float x0 = cx - cw * 0.5f + cw * k / 4.0f, x1 = x0 + cw / 4.0f;
    const float y0 = k < 2 ? T + 60 + k * 3.0f : T + 62 - (k - 2) * 3.0f;
    tri({x0, y0}, {x1, y0}, {(x0 + x1) * 0.5f, y0 + 6 + 10 * hash(seed + k)}, shade(dark, 0.8f));
  }
  for (int k = 1; k < 4; ++k)
    DrawLineEx({cx - cw * 0.5f + cw * k / 4.0f, T + 10}, {cx - cw * 0.5f + cw * k / 4.0f, T + 58}, 1.2f, A(rgb(40, 44, 72), 0.5f));
  DrawRectangleRounded({L, T - 1, w, 14}, 0.4f, 6, rgb(70, 76, 108));
  DrawRectangleRounded({L, T - 3, w, 11}, 0.4f, 6, rgb(178, 186, 214));
  DrawRectangleRounded({L + 3, T - 3, w - 6, 4}, 1.0f, 6, rgb(222, 228, 246));
}

// ==== Seireitei (Bleach): the Court of Pure Souls on a clear morning ==================================
constexpr Color kSeiHaze{206, 222, 234, 255};
constexpr Color kSeiRoof{58, 66, 92, 255};
constexpr Color kSeiLow{70, 78, 100, 255};  // below the near rooftops

// A building of the Seireitei seen from the front: white walls between dark timber, a roof of slate tiles
// with upturned eaves (two storeys if `tiers` is 2). Lit from the left. `fog` blends it into the haze.
void seiBuilding(float x, float base, float w, float wallH, float roofH, int tiers, int seed, float fog, Color haze) {
  auto F = [&](Color c) { return mixColor(c, haze, fog); };
  const Color wallLit = F(rgb(244, 242, 234)), wallDark = F(rgb(196, 198, 204)), beam = F(rgb(74, 58, 52));
  const Color roof = F(kSeiRoof), roofLit = F(rgb(104, 116, 148)), roofEdge = F(rgb(34, 38, 56));
  float y = base;
  for (int t = 0; t < tiers; ++t) {
    const float ww = w * (t == 0 ? 1.0f : 0.62f), wh = wallH * (t == 0 ? 1.0f : 0.7f), rh = roofH * (t == 0 ? 1.0f : 0.85f);
    const float x0 = x - ww * 0.5f, x1 = x + ww * 0.5f, top = y - wh;
    gquad({x0, top}, {x1, top}, {x1, y}, {x0, y}, wallLit, wallDark, wallDark, wallLit);
    DrawRectangleRec({x0, top, ww, std::max(2.0f, wh * 0.08f)}, beam);
    for (float px = x0; px <= x1 + 0.5f; px += std::max(14.0f, ww / std::max(2.0f, std::floor(ww / 34.0f))))
      DrawRectangleRec({px - 1.2f, top, 2.4f, wh}, A(beam, 0.85f));
    if (wh > 26.0f && hash(seed + t) > 0.35f) {  // a latticed window
      const float sx = x0 + ww * (0.25f + 0.4f * hash(seed + t * 7)), sy = top + wh * 0.3f, sw = ww * 0.16f, sh = wh * 0.32f;
      DrawRectangleRec({sx, sy, sw, sh}, F(rgb(92, 84, 80)));
      for (float lx = sx + 3; lx < sx + sw - 1; lx += 4) DrawLineEx({lx, sy}, {lx, sy + sh}, 1.0f, F(rgb(214, 206, 190)));
    }
    // Hip roof: the eave overhangs the wall, the ends curl up.
    const float over = ww * 0.12f + 6.0f, rt = top - rh;
    const Vector2 e0{x0 - over, top + 2}, e1{x1 + over, top + 2}, r0{x - ww * 0.3f, rt}, r1{x + ww * 0.3f, rt};
    gquad(r0, r1, e1, e0, roofLit, roof, roof, roofLit);
    for (float u = 0.06f; u < 1.0f; u += 0.07f)  // rows of tiles running down the roof
      DrawLineEx({lerpf(r0.x, r1.x, u), rt + 1}, {lerpf(e0.x, e1.x, u), top + 1}, 1.0f, A(roofEdge, 0.5f));
    DrawLineEx(e0, e1, std::max(2.0f, rh * 0.12f), roofEdge);
    tri({e0.x - over * 0.25f, e0.y - rh * 0.32f}, {e0.x + over * 0.5f, e0.y - 1}, {e0.x + over * 0.2f, e0.y + 2}, roofEdge);
    tri({e1.x + over * 0.25f, e1.y - rh * 0.32f}, {e1.x - over * 0.5f, e1.y - 1}, {e1.x - over * 0.2f, e1.y + 2}, roofEdge);
    DrawLineEx({r0.x - 3, rt}, {r1.x + 3, rt}, std::max(2.0f, rh * 0.16f), roofEdge);  // ridge
    y = rt + rh * 0.3f;
  }
}

// The Senzaikyū: the white tower of penitence, slender and crowned with a dark roof.
void senzaikyu(float x, float base, float h, Color haze, float fog) {
  auto F = [&](Color c) { return mixColor(c, haze, fog); };
  const float w = h * 0.075f, top = base - h;
  gquad({x - w * 0.5f, top}, {x + w * 0.5f, top}, {x + w * 0.62f, base}, {x - w * 0.62f, base}, F(rgb(246, 244, 238)),
        F(rgb(206, 208, 214)), F(rgb(196, 198, 206)), F(rgb(236, 234, 228)));
  for (float y = top + h * 0.12f; y < base - 10; y += h * 0.06f) DrawRectangleRec({x - w * 0.12f, y, w * 0.24f, h * 0.018f}, F(rgb(120, 116, 116)));
  const Color roof = F(kSeiRoof);
  quad({x - w * 1.4f, top + 4}, {x + w * 1.4f, top + 4}, {x + w * 0.5f, top - w * 0.9f}, {x - w * 0.5f, top - w * 0.9f}, roof);
  quad({x - w * 0.9f, top - w * 0.9f}, {x + w * 0.9f, top - w * 0.9f}, {x + w * 0.3f, top - w * 1.6f}, {x - w * 0.3f, top - w * 1.6f}, roof);
  DrawLineEx({x, top - w * 1.6f}, {x, top - w * 2.4f}, 2.0f, roof);
  DrawLineEx({x - w * 0.5f, base - h * 0.42f}, {x - h * 0.9f, base - h * 0.2f}, std::max(2.0f, w * 0.18f), F(rgb(226, 224, 220)));  // bridge
}

// The Sōkyoku hill: a mesa of grey rock, the execution halberd standing in its scaffold on top.
void sokyokuHill(float x, float base, float h, Color haze, float fog) {
  auto F = [&](Color c) { return mixColor(c, haze, fog); };
  const float tw = h * 0.95f, bw = h * 2.1f;
  const Vector2 l0{x - bw * 0.5f, base}, l1{x - bw * 0.36f, base - h * 0.4f}, l2{x - tw * 0.56f, base - h * 0.62f},
      l3{x - tw * 0.5f, base - h}, r3{x + tw * 0.5f, base - h * 0.97f}, r2{x + tw * 0.58f, base - h * 0.6f},
      r1{x + bw * 0.38f, base - h * 0.36f}, r0{x + bw * 0.5f, base};
  const Color lit = F(rgb(150, 154, 156)), dark = F(rgb(96, 102, 112)), foot = F(rgb(150, 164, 170));
  gquad(l2, l3, {x - tw * 0.2f, base - h}, {x - tw * 0.2f, base - h * 0.6f}, lit, lit, F(rgb(132, 136, 140)), lit);  // cliffs
  gquad({x - tw * 0.2f, base - h * 0.6f}, {x - tw * 0.2f, base - h}, r3, r2, F(rgb(124, 128, 134)), F(rgb(132, 136, 140)), dark, dark);
  gquad(l0, l1, l2, {x - tw * 0.2f, base}, foot, lit, lit, foot);
  gquad({x - tw * 0.2f, base}, {x - tw * 0.2f, base - h * 0.6f}, r2, r1, foot, F(rgb(124, 128, 134)), dark, dark);
  gquad({x - tw * 0.2f, base}, r1, r0, r0, foot, dark, foot, foot);
  gtri(l0, l2, {x - tw * 0.2f, base}, foot, lit, foot);
  for (int i = 0; i < 9; ++i) {  // vertical fissures
    const float u = (i + 0.5f) / 9.0f, xx = lerpf(l3.x, r3.x, u);
    DrawLineEx({xx, base - h * (0.96f - 0.03f * hash(i))}, {xx + (u - 0.5f) * 30.0f, base - h * (0.5f - 0.2f * hash(i * 3))}, 1.6f,
               F(rgb(84, 88, 98)));
  }
  for (int i = 0; i < 7; ++i) {  // trees along the rim
    const float u = (i + 0.5f) / 7.0f;
    ellipse({lerpf(l3.x, r3.x, u), base - h - 4 + 3 * hash(i * 5)}, 12 + 6 * hash(i), 7, F(rgb(64, 104, 78)));
  }
  // The Sōkyoku: a giant halberd held upright in a tall wooden scaffold.
  const float sx = x + tw * 0.12f, sy = base - h, sh = h * 0.8f;
  const Color wood = F(rgb(92, 66, 52)), blade = F(rgb(214, 218, 224)), bladeDark = F(rgb(150, 156, 168));
  for (int side = -1; side <= 1; side += 2) {
    DrawLineEx({sx + side * sh * 0.2f, sy}, {sx + side * sh * 0.07f, sy - sh * 0.62f}, 5.0f, wood);
    DrawLineEx({sx + side * sh * 0.2f, sy}, {sx - side * sh * 0.1f, sy - sh * 0.3f}, 3.0f, wood);
  }
  for (int k = 1; k <= 3; ++k) {
    const float y = sy - sh * 0.2f * k, half = sh * (0.2f - 0.045f * k);
    DrawLineEx({sx - half, y}, {sx + half, y}, 3.5f, wood);
  }
  DrawLineEx({sx, sy - sh * 0.1f}, {sx, sy - sh * 0.72f}, 5.0f, F(rgb(66, 48, 40)));  // the shaft
  DrawLineEx({sx - sh * 0.1f, sy - sh * 0.7f}, {sx + sh * 0.1f, sy - sh * 0.7f}, 4.0f, F(rgb(66, 48, 40)));
  const Vector2 b0{sx - sh * 0.06f, sy - sh * 0.72f}, b1{sx + sh * 0.06f, sy - sh * 0.72f}, tip{sx + sh * 0.02f, sy - sh * 1.3f};
  quad(b0, b1, {sx + sh * 0.07f, sy - sh * 1.0f}, {sx - sh * 0.03f, sy - sh * 1.05f}, blade);
  tri({sx - sh * 0.03f, sy - sh * 1.05f}, {sx + sh * 0.07f, sy - sh * 1.0f}, tip, blade);
  tri(b0, {sx - sh * 0.03f, sy - sh * 1.05f}, tip, bladeDark);
}

// Japanese black pine: a leaning trunk and flat clouds of needles.
void pine(Vector2 base, float s, int seed, Color trunk, Color dark, Color lit) {
  const float lean = (hash(seed) - 0.5f) * 0.5f;
  const Vector2 top{base.x + lean * s, base.y - s};
  DrawLineEx(base, top, s * 0.09f, trunk);
  for (int k = 0; k < 4; ++k) {
    const float t = 0.45f + 0.18f * k, side = (k % 2 ? 1.0f : -1.0f) * (0.2f + 0.25f * hash(seed + k));
    const Vector2 p{lerpf(base.x, top.x, t) + side * s * 0.6f, lerpf(base.y, top.y, t) - s * 0.05f};
    DrawLineEx({lerpf(base.x, top.x, t), lerpf(base.y, top.y, t)}, p, s * 0.045f, trunk);
    ellipse({p.x, p.y + s * 0.02f}, s * (0.3f - 0.04f * k), s * 0.1f, dark);
    ellipse({p.x - s * 0.04f, p.y - s * 0.03f}, s * (0.22f - 0.03f * k), s * 0.06f, lit);
  }
  ellipse({top.x, top.y}, s * 0.24f, s * 0.09f, dark);
  ellipse({top.x - s * 0.03f, top.y - s * 0.03f}, s * 0.16f, s * 0.05f, lit);
}

// Hell butterfly (jigokuchō): black wings with a violet sheen, flapping.
void hellButterfly(Vector2 p, float s, float flap, float face) {
  const float open = 0.25f + 0.75f * std::fabs(std::sin(flap));
  glow(p, s * 2.2f, rgb(150, 70, 200), 0.12f, 12);
  const Color wing = rgb(18, 12, 26), sheen = rgb(110, 60, 150);
  for (int side = -1; side <= 1; side += 2) {
    const float dx = side * s * open;
    tri(p, {p.x + dx * 1.1f, p.y - s * 0.95f}, {p.x + dx * 1.25f, p.y - s * 0.1f}, wing);
    tri(p, {p.x + dx * 0.95f, p.y + s * 0.1f}, {p.x + dx * 0.55f, p.y + s * 0.8f}, wing);
    DrawLineEx({p.x + dx * 0.4f, p.y - s * 0.45f}, {p.x + dx * 0.95f, p.y - s * 0.7f}, 1.0f, A(sheen, 0.8f));
  }
  DrawLineEx({p.x, p.y - s * 0.3f}, {p.x, p.y + s * 0.5f}, std::max(1.0f, s * 0.14f), wing);
  DrawLineEx({p.x, p.y - s * 0.3f}, {p.x + face * s * 0.35f, p.y - s * 0.75f}, 1.0f, wing);
}

void bakeSeireitei(Art& a) {
  const Color haze = kSeiHaze;
  a.sky = bakeOpaque(1360, 800, 1.0f, [&] {
    gradient(1360, 800, {{0.0f, rgb(46, 102, 196)}, {0.38f, rgb(96, 158, 226)}, {0.6f, rgb(170, 208, 238)}, {0.7f, haze}, {1.0f, haze}});
    const Vector2 sun{230, 110};
    glow(sun, 620, 480, A(rgb(255, 250, 230), 0.4f), A(rgb(255, 250, 230), 0.0f));
    glow(sun, 150, rgb(255, 252, 238), 0.7f);
    DrawCircleV(sun, 30, rgb(255, 254, 246));
    const Vector2 cl[5] = {{560, 110}, {960, 60}, {1250, 200}, {90, 290}, {760, 300}};
    for (int i = 0; i < 5; ++i)
      cumulus(cl[i], 170 + hash(i * 5) * 150, 56 + hash(i * 3) * 46, i * 41, rgb(170, 194, 226), rgb(246, 249, 255),
              A(WHITE, 0.55f), -0.6f);
    range(1360, 560, 70, 4, rgb(150, 176, 204), rgb(196, 214, 230), 800, rgb(240, 246, 252), false);
    sokyokuHill(1080, 600, 230, haze, 0.42f);
    senzaikyu(300, 600, 300, haze, 0.38f);
    for (int i = 0; i < 26; ++i)  // the endless roofs of the Seireitei, lost in the haze
      seiBuilding(20 + i * 54 + hash(i * 3) * 20, 610 + hash(i * 7) * 14, 46 + hash(i) * 20, 14, 10, 1, i, 0.62f, haze);
    vgrad(0, 600, 1360, 200, A(haze, 0.4f), haze);
  });
  // Rows of walled compounds, the nearer ones larger and clearer.
  a.mid = bake(2000, 420, 1.25f, [&] {
    for (int row = 0; row < 3; ++row) {
      const float base = 150.0f + row * 90.0f, s = 0.7f + 0.3f * row, fog = 0.5f - 0.17f * row;
      for (int i = 0; i < 14 - row * 3; ++i) {
        const float x = 40 + i * (150.0f + row * 50.0f) + hash(row * 31 + i * 7) * 60.0f;
        const int tiers = hash(row * 17 + i * 3) > 0.72f ? 2 : 1;
        seiBuilding(x, base, (90 + hash(i * 5 + row) * 60) * s, 34 * s, 26 * s, tiers, row * 50 + i, fog, haze);
      }
      // the long white walls between the compounds
      const float wy = base + 6.0f;
      vgrad(0, wy - 16 * s, 2000, 16 * s, mixColor(rgb(240, 238, 230), haze, fog), mixColor(rgb(206, 206, 210), haze, fog));
      DrawRectangleRec({0, wy - 19 * s, 2000, 4 * s}, mixColor(kSeiRoof, haze, fog));
    }
    for (int i = 0; i < 8; ++i)
      pine({100 + i * 260 + hash(i * 9) * 80, 334}, 70 + hash(i * 4) * 40, 70 + i, rgb(80, 64, 58), mixColor(rgb(46, 92, 70), haze, 0.25f),
           mixColor(rgb(84, 136, 96), haze, 0.25f));
  });
  // Rooftops right below the fighters.
  a.near = bake(1600, 300, 1.25f, [&] {
    for (int i = 0; i < 7; ++i) {
      const float x = 60 + i * 240 + hash(i * 13) * 60;
      wrapX(x, 160, 1600, [&](float xx) { seiBuilding(xx, 300, 230 + hash(i) * 60, 70, 46, 1, 400 + i, 0.08f, haze); });
    }
    for (int i = 0; i < 4; ++i) {
      const float x = 200 + i * 410 + hash(i * 5) * 60;
      wrapX(x, 80, 1600, [&](float xx) {
        pine({xx, 250}, 110, 90 + i, rgb(70, 54, 48), rgb(38, 78, 60), rgb(70, 120, 84));
      });
    }
  });
}

void bgSeireitei(const Art& a, Vec2 pc, float skyX, float skyY, float time) {
  drawBackdrop(a.sky, skyX, skyY);
  drawLayer(a.mid, -360.0f - pc.x * 0.12f, 290.0f - pc.y * 0.12f, false);
  const float ny = 470.0f - pc.y * 0.22f;
  drawLayer(a.near, -pc.x * 0.22f, ny, true);
  if (ny + 299.0f < kScreenH) DrawRectangleRec({0, ny + 299.0f, float(kScreenW), kScreenH - ny - 299.0f}, kSeiLow);
  motes(22, 61, pc, time, 8.0f, -5.0f, rgb(255, 252, 236), rgb(255, 236, 240), 1.1f);
  for (int i = 0; i < 4; ++i) {  // hell butterflies drifting across
    const float sp = 26.0f + 14.0f * hash(i * 3), t = time * sp + hash(i * 7) * 3000.0f;
    const float x = wrapf(t - pc.x * 0.3f, kScreenW + 200.0f) - 100.0f;
    const float y = 140.0f + 300.0f * hash(i * 11) + std::sin(time * 1.3f + i * 2.0f) * 40.0f - pc.y * 0.3f;
    hellButterfly({x, y}, 6.0f + 2.0f * hash(i), time * 9.0f + i, 1.0f);
  }
}

// A section of the white walls: tiled coping to walk on, plaster and timber, a stone foundation beneath.
void seireiteiSolid(const Rect& r, float time, int seed) {
  (void)time;
  const float L = r.left(), R = r.right(), T = r.top(), w = r.w, h = r.h, cx = L + w * 0.5f;
  const float depth = std::min(140.0f, w * 0.2f);
  // Ishigaki foundation: courses of fitted stones narrowing downward.
  const int rows = 5;
  for (int row = 0; row < rows; ++row) {
    const float y0 = T + h + row * depth / rows, y1 = y0 + depth / rows;
    const float k0 = float(row) / rows, k1 = float(row + 1) / rows;
    const float hw0 = (w * 0.5f - 6) * (1.0f - 0.55f * k0 * k0), hw1 = (w * 0.5f - 6) * (1.0f - 0.55f * k1 * k1);
    const Color c0 = shade(rgb(124, 122, 120), 1.0f - 0.4f * k0), c1 = shade(rgb(124, 122, 120), 1.0f - 0.4f * k1);
    gquad({cx - hw0, y0}, {cx + hw0, y0}, {cx + hw1, y1}, {cx - hw1, y1}, c0, c0, c1, c1);
    float x = cx - hw0 + (row % 2) * 18.0f;
    int k = 0;
    while (x < cx + hw0 - 8) {  // the joints between the stones
      const float sw = 34 + 30 * hash(seed + row * 37 + k * 11);
      DrawLineEx({x, y0 + 1}, {x + (hw1 - hw0) * 0.3f, y1 - 1}, 1.6f, A(rgb(70, 70, 74), 0.7f));
      DrawRectangleRec({x + 3, y0 + 2, sw - 8, 2}, A(WHITE, 0.12f));
      if (hash(seed + row * 13 + k * 5) > 0.82f) ellipse({x + sw * 0.5f, y0 + 4}, sw * 0.3f, 3, A(rgb(90, 120, 70), 0.7f));  // moss
      x += sw;
      ++k;
    }
    DrawLineEx({cx - hw1, y1}, {cx + hw1, y1}, 1.8f, A(rgb(60, 60, 66), 0.8f));
  }
  const float by = T + h + depth;  // rubble breaking off at the bottom
  for (int i = 0; i < 6; ++i) {
    const float u0 = i / 6.0f, u1 = (i + 1) / 6.0f, hw = (w * 0.5f - 6) * 0.45f;
    tri({cx - hw + 2 * hw * u0, by}, {cx - hw + 2 * hw * u1, by}, {cx - hw + 2 * hw * (u0 + u1) * 0.5f, by + 30 + 40 * hash(seed + i)},
        rgb(84, 84, 88));
  }
  // The wall: white plaster between dark timber posts, a grey stone plinth.
  gquad({L, T}, {R, T}, {R - 6, T + h}, {L + 6, T + h}, rgb(246, 244, 236), rgb(222, 222, 220), rgb(206, 206, 206), rgb(232, 230, 224));
  DrawRectangleRec({L + 4, T + h - 14, w - 8, 14}, rgb(132, 132, 134));
  DrawRectangleRec({L + 4, T + h - 14, w - 8, 3}, rgb(170, 170, 170));
  for (float x = L + 40; x < R - 20; x += 150) {
    DrawRectangleRec({x - 4, T + 8, 8, h - 22}, rgb(82, 62, 54));
    DrawRectangleRec({x - 4, T + 8, 3, h - 22}, rgb(110, 86, 74));
  }
  DrawRectangleRec({L + 2, T + 8, w - 4, 6}, rgb(82, 62, 54));
  // Tiled coping: a dark ridge with round tile ends along its edge.
  DrawRectangleRounded({L - 8, T - 6, w + 16, 16}, 0.5f, 6, rgb(40, 44, 62));
  DrawRectangleRounded({L - 8, T - 7, w + 16, 12}, 0.5f, 6, kSeiRoof);
  DrawRectangleRounded({L - 6, T - 7, w + 12, 4}, 1.0f, 6, rgb(122, 134, 166));
  for (float x = L - 2; x < R + 4; x += 14) {
    DrawCircleV({x, T + 7}, 4.2f, rgb(40, 44, 62));
    DrawCircleV({x - 0.8f, T + 6.2f}, 2.4f, rgb(92, 102, 132));
  }
}

// A rooftop rising from the mist: tiled roof to stand on, a white wall fading away below.
void seireiteiThin(const Platform& p, float time, int seed) {
  (void)time;
  (void)seed;
  const Rect& r = p.rect;
  const float L = r.x, R = r.right(), T = r.y, w = r.w;
  const float over = 14.0f;
  gquad({L + 8, T + 22}, {R - 8, T + 22}, {R - 8, T + 66}, {L + 8, T + 66}, rgb(240, 238, 230), rgb(214, 214, 214),
        A(rgb(214, 214, 214), 0.0f), A(rgb(240, 238, 230), 0.0f));
  for (float x = L + 20; x < R - 12; x += 46)
    gquad({x, T + 22}, {x + 5, T + 22}, {x + 5, T + 60}, {x, T + 60}, rgb(82, 62, 54), rgb(82, 62, 54), A(rgb(82, 62, 54), 0.0f),
          A(rgb(82, 62, 54), 0.0f));
  DrawRectangleRec({L + 8, T + 24, w - 16, 4}, rgb(82, 62, 54));
  // roof slope toward the viewer, eaves curling up at both ends
  gquad({L - 2, T + 2}, {R + 2, T + 2}, {R + over, T + 24}, {L - over, T + 24}, rgb(104, 116, 148), kSeiRoof, rgb(48, 54, 76),
        rgb(70, 80, 106));
  for (float u = 0.04f; u < 1.0f; u += 0.06f)
    DrawLineEx({lerpf(L - 2, R + 2, u), T + 3}, {lerpf(L - over, R + over, u), T + 23}, 1.0f, A(rgb(30, 34, 50), 0.5f));
  DrawLineEx({L - over, T + 24}, {R + over, T + 24}, 3.0f, rgb(30, 34, 50));
  tri({L - over - 6, T + 15}, {L - over + 8, T + 23}, {L - over + 2, T + 26}, rgb(30, 34, 50));
  tri({R + over + 6, T + 15}, {R + over - 8, T + 23}, {R + over - 2, T + 26}, rgb(30, 34, 50));
  DrawRectangleRounded({L - 4, T - 5, w + 8, 10}, 0.6f, 6, rgb(40, 44, 62));
  DrawRectangleRounded({L - 4, T - 6, w + 8, 7}, 0.6f, 6, kSeiRoof);
  DrawRectangleRounded({L - 2, T - 6, w + 4, 3}, 1.0f, 6, rgb(122, 134, 166));
  for (int s = 0; s < 2; ++s) DrawCircleV({s ? R + 2 : L - 2, T - 2}, 6.0f, rgb(40, 44, 62));  // ridge-end tiles
}

// ==== Black Clover: the Black Bulls' hideout, a golden afternoon in the forest ===========================
constexpr Color kBbHaze{246, 210, 160, 255};
constexpr Color kBbLow{40, 52, 38, 255};  // forest floor below the near layer

// Broad-leaved tree: a trunk and clumps of foliage lit from the left (the low sun).
void leafyTree(Vector2 base, float s, int seed, Color trunk, Color dark, Color mid, Color lit) {
  DrawLineEx(base, {base.x + s * 0.04f, base.y - s * 0.7f}, s * 0.12f, trunk);
  DrawLineEx({base.x, base.y - s * 0.45f}, {base.x - s * 0.25f, base.y - s * 0.8f}, s * 0.05f, trunk);
  DrawLineEx({base.x, base.y - s * 0.5f}, {base.x + s * 0.28f, base.y - s * 0.85f}, s * 0.05f, trunk);
  constexpr int n = 7;
  for (int pass = 0; pass < 3; ++pass)
    for (int i = 0; i < n; ++i) {
      const float a = i / float(n) * 2.0f * kPi, rr = s * (0.26f + 0.08f * hash(seed + i * 3));
      Vector2 p{base.x + std::cos(a) * s * 0.3f, base.y - s * 0.95f + std::sin(a) * s * 0.22f};
      if (pass == 0) softEllipse({p.x + s * 0.03f, p.y + s * 0.04f}, rr, rr * 0.85f, dark, 3.0f, 18);
      else if (pass == 1) softEllipse({p.x - s * 0.02f, p.y}, rr * 0.86f, rr * 0.72f, mid, 3.0f, 18);
      else if (std::cos(a) < 0.3f) softEllipse({p.x - rr * 0.3f, p.y - rr * 0.25f}, rr * 0.45f, rr * 0.35f, lit, 3.0f, 14);
    }
}

// Pointed tower roof (a cone seen from the side), lit on its left.
void coneRoof(float x, float y, float w, float h, float lean, Color lit, Color dark) {
  const Vector2 tip{x + lean * h, y - h};
  gtri({x - w * 0.5f, y}, {x, y}, tip, lit, mixColor(lit, dark, 0.5f), lit);
  gtri({x, y}, {x + w * 0.5f, y}, tip, mixColor(lit, dark, 0.5f), dark, dark);
  DrawLineEx({x - w * 0.56f, y}, {x + w * 0.56f, y}, 3.0f, shade(dark, 0.7f));
}

// The Black Bulls' hideout: a crooked castle of dark stone, its towers and the horned bull's head.
void bullHideout(float x, float base) {
  const Color stoneLit = rgb(118, 108, 112), stoneDark = rgb(62, 58, 70), roofLit = rgb(70, 74, 96), roofDark = rgb(34, 36, 52);
  const Color win = rgb(255, 214, 120);
  auto block = [&](float cx, float w, float top, float bottom) {
    gquad({cx - w * 0.5f, top}, {cx + w * 0.5f, top}, {cx + w * 0.5f, bottom}, {cx - w * 0.5f, bottom}, stoneLit, stoneDark, stoneDark,
          stoneLit);
    for (float y = top + 14; y < bottom - 6; y += 16)  // courses of stone
      DrawLineEx({cx - w * 0.5f, y}, {cx + w * 0.5f, y}, 1.0f, A(rgb(40, 36, 46), 0.45f));
  };
  auto window = [&](float cx, float y, float w, float h) {
    glow({cx, y + h * 0.5f}, w * 1.6f, win, 0.25f, 12);
    DrawRectangleRec({cx - w * 0.5f, y + w * 0.5f, w, h - w * 0.5f}, win);
    DrawCircleV({cx, y + w * 0.5f}, w * 0.5f, win);
    DrawLineEx({cx, y + 2}, {cx, y + h}, 1.2f, rgb(70, 50, 40));
  };
  // Side towers (behind), the main hall, the crooked keep.
  block(x - 190, 70, base - 260, base);
  coneRoof(x - 190, base - 260, 96, 110, -0.08f, roofLit, roofDark);
  block(x + 200, 80, base - 300, base);
  coneRoof(x + 200, base - 300, 106, 130, 0.1f, roofLit, roofDark);
  block(x, 300, base - 200, base);
  quad({x - 166, base - 200}, {x + 166, base - 200}, {x + 120, base - 268}, {x - 120, base - 268}, roofDark);  // hall roof
  quad({x - 166, base - 200}, {x - 30, base - 200}, {x - 50, base - 268}, {x - 120, base - 268}, roofLit);
  block(x - 40, 90, base - 390, base - 200);  // the keep, leaning a little
  coneRoof(x - 36, base - 390, 124, 150, 0.06f, roofLit, roofDark);
  block(x + 90, 56, base - 330, base - 200);
  coneRoof(x + 90, base - 330, 78, 96, -0.12f, roofLit, roofDark);
  for (int i = 0; i < 6; ++i) window(x - 120 + i * 48.0f, base - 160 + (i % 2) * 6.0f, 14, 30);
  for (int i = 0; i < 3; ++i) window(x - 40, base - 370 + i * 56.0f, 12, 24);
  window(x + 90, base - 300, 10, 22);
  window(x - 190, base - 220, 12, 26);
  window(x + 200, base - 250, 12, 26);
  window(x + 200, base - 180, 12, 26);
  DrawRectangleRounded({x - 26, base - 70, 52, 70}, 0.5f, 6, rgb(70, 48, 36));  // the great door
  DrawLineEx({x, base - 66}, {x, base}, 2.0f, rgb(40, 28, 22));
  // The bull's head on the keep: dark muzzle, white horns sweeping up.
  const Vector2 hc{x - 38, base - 318};
  for (int s = -1; s <= 1; s += 2) {
    Vector2 horn[5] = {{hc.x + s * 22.0f, hc.y - 6}, {hc.x + s * 48.0f, hc.y - 14}, {hc.x + s * 66.0f, hc.y - 34},
                       {hc.x + s * 70.0f, hc.y - 58}, {hc.x + s * 62.0f, hc.y - 74}};
    stroke(horn, 5, 12.0f, 3.0f, rgb(236, 226, 206));
    DrawLineEx(horn[0], horn[2], 3.0f, rgb(180, 168, 150));
  }
  ellipse(hc, 30, 26, rgb(30, 26, 32));
  ellipse({hc.x, hc.y + 22}, 20, 14, rgb(44, 38, 44));
  for (int s = -1; s <= 1; s += 2) {
    tri({hc.x + s * 28.0f, hc.y - 10}, {hc.x + s * 46.0f, hc.y - 2}, {hc.x + s * 26.0f, hc.y + 4}, rgb(30, 26, 32));  // ears
    dot({hc.x + s * 12.0f, hc.y - 4}, 4.0f, rgb(255, 80, 60));  // eyes
  }
  DrawRing({hc.x, hc.y + 30}, 6, 8.5f, 0, 360, 16, rgb(214, 186, 90));  // nose ring
}

void bakeBlackBulls(Art& a) {
  const Color haze = kBbHaze;
  a.sky = bakeOpaque(1360, 800, 1.0f, [&] {
    gradient(1360, 800, {{0.0f, rgb(66, 116, 186)}, {0.34f, rgb(136, 168, 208)}, {0.56f, rgb(232, 196, 160)},
                         {0.68f, rgb(252, 214, 158)}, {1.0f, rgb(244, 204, 156)}});
    const Vector2 sun{250, 440};
    glow(sun, 700, 460, A(rgb(255, 214, 140), 0.45f), A(rgb(255, 214, 140), 0.0f));
    for (int i = 0; i < 12; ++i) {  // rays
      const float ang = -kPi * 0.95f + i * 0.16f, sp = 0.025f + 0.025f * hash(i * 7);
      const Vector2 p1{sun.x + std::cos(ang - sp) * 1400, sun.y + std::sin(ang - sp) * 1400};
      const Vector2 p2{sun.x + std::cos(ang + sp) * 1400, sun.y + std::sin(ang + sp) * 1400};
      gtri(sun, p1, p2, A(rgb(255, 236, 190), 0.12f), A(rgb(255, 236, 190), 0.0f), A(rgb(255, 236, 190), 0.0f));
    }
    glow(sun, 160, rgb(255, 240, 200), 0.7f);
    DrawCircleV(sun, 40, rgb(255, 248, 226));
    const Vector2 cl[5] = {{520, 120}, {960, 90}, {1220, 210}, {760, 260}, {120, 200}};
    for (int i = 0; i < 5; ++i)
      stratus(cl[i], 260 + hash(i) * 240, 22 + hash(i * 5) * 14, i * 23, rgb(170, 150, 170), rgb(255, 222, 170));
    range(1360, 560, 80, 7, rgb(150, 150, 140), rgb(226, 196, 156), 800, rgb(255, 226, 180), false);
    rlPushMatrix();  // nearer forested hills
    rlTranslatef(0, 470, 0);
    range(1360, 120, 70, 13, rgb(96, 118, 86), rgb(196, 176, 130), 330, rgb(255, 214, 150), false);
    range(1360, 190, 60, 19, rgb(66, 92, 62), rgb(150, 150, 104), 330, rgb(250, 200, 130), false);
    rlPopMatrix();
  });
  // The hideout among the trees.
  a.mid = bake(2000, 480, 1.25f, [&] {
    for (int i = 0; i < 16; ++i) {  // the forest behind
      const float x = 30 + i * 128 + hash(i * 3) * 40, s = 120 + 50 * hash(i * 7);
      leafyTree({x, 470}, s, i * 9, rgb(70, 52, 40), mixColor(rgb(40, 66, 44), haze, 0.3f), mixColor(rgb(66, 104, 58), haze, 0.3f),
                mixColor(rgb(150, 170, 80), haze, 0.25f));
    }
    bullHideout(1080, 470);
    for (int i = 0; i < 10; ++i) {  // trees in front of its foot
      const float x = 60 + i * 210 + hash(i * 13) * 60;
      if (std::fabs(x - 1080) < 70) continue;
      leafyTree({x, 500}, 90 + 30 * hash(i * 5), 100 + i, rgb(60, 44, 34), rgb(34, 58, 38), rgb(58, 94, 50), rgb(150, 176, 74));
    }
  });
  // Near: big trees at the edges and the bushes of the clearing.
  a.near = bake(1600, 320, 1.25f, [&] {
    const float tx[3] = {90, 760, 1460}, ts[3] = {260, 190, 240};
    for (int k = 0; k < 3; ++k)
      wrapX(tx[k], 160, 1600, [&](float x) {
        leafyTree({x, 320}, ts[k], 300 + k * 7, rgb(56, 40, 30), rgb(26, 46, 30), rgb(44, 76, 40), rgb(130, 160, 64));
      });
    for (int i = 0; i < 26; ++i) {
      const float x = hash(i * 3) * 1600, r = 30 + 30 * hash(i * 5);
      wrapX(x, r, 1600, [&](float xx) {
        softEllipse({xx, 300}, r, r * 0.6f, rgb(34, 56, 34), 3.0f, 18);
        softEllipse({xx - r * 0.2f, 290}, r * 0.6f, r * 0.35f, rgb(70, 104, 48), 3.0f, 16);
      });
    }
  });
}

void bgBlackBulls(const Art& a, Vec2 pc, float skyX, float skyY, float time) {
  drawBackdrop(a.sky, skyX, skyY);
  drawLayer(a.mid, -360.0f - pc.x * 0.12f, 200.0f - pc.y * 0.12f, false);
  const float ny = 430.0f - pc.y * 0.22f;
  drawLayer(a.near, -pc.x * 0.22f, ny, true);
  if (ny + 319.0f < kScreenH) DrawRectangleRec({0, ny + 319.0f, float(kScreenW), kScreenH - ny - 319.0f}, kBbLow);
  motes(30, 71, pc, time, 5.0f, -7.0f, rgb(255, 230, 150), rgb(255, 250, 220), 1.3f);  // dust and mana in the sun
  leaves(10, 13, pc, time, rgb(150, 170, 70), rgb(230, 190, 80));
}

// The Bulls' hideout terrace: a grassy top, a wall of rough stone with the bull banner, earth and roots.
void blackBullsSolid(const Rect& r, float time, int seed) {
  const float L = r.left(), R = r.right(), T = r.top(), w = r.w, h = r.h, cx = L + w * 0.5f;
  const float depth = h + std::min(170.0f, w * 0.3f);
  constexpr int kN = 16;
  Vector2 bot[kN + 1];
  for (int i = 0; i <= kN; ++i) {
    const float u = float(i) / kN, k = std::pow(std::sin(u * kPi), 0.7f);
    bot[i] = {lerpf(L + 12, R - 12, u) + (0.5f - u) * w * 0.1f * k, T + h + (depth - h) * k * (0.75f + 0.25f * hash(seed + i * 13))};
  }
  for (int i = 1; i < kN; ++i) {  // roots hanging behind the earth
    if (hash(seed + i * 17) < 0.4f) continue;
    const float len = (depth - h) * (0.3f + 0.5f * hash(seed + i * 5));
    Vector2 p[5] = {bot[i]};
    for (int k = 1; k < 5; ++k)
      p[k] = {p[k - 1].x + (hash(seed + i * 7 + k) - 0.5f) * 10.0f + std::sin(time * 1.1f + i + k) * k * 1.0f, p[k - 1].y + len / 4.0f};
    stroke(p, 5, 4.0f, 1.5f, rgb(84, 60, 44));
  }
  for (int i = 0; i < kN; ++i) {  // earth: warm brown going dark at the tip
    const float u0 = float(i) / kN, u1 = float(i + 1) / kN;
    const Color e0 = shade(rgb(120, 86, 60), 1.1f - 0.35f * u0), e1 = shade(rgb(120, 86, 60), 1.1f - 0.35f * u1);
    gquad({lerpf(L + 12, R - 12, u0), T + h}, {lerpf(L + 12, R - 12, u1), T + h}, bot[i + 1], bot[i], e0, e1, rgb(46, 34, 30), rgb(46, 34, 30));
  }
  for (int i = 1; i < kN - 1; i += 2) {  // stones in the earth
    const Vector2 p{lerpf(lerpf(L + 12, R - 12, float(i) / kN), bot[i].x, 0.35f), lerpf(T + h, bot[i].y, 0.35f)};
    ellipse(p, 10 + 8 * hash(seed + i), 6 + 4 * hash(seed + i * 3), rgb(110, 104, 100));
    ellipse({p.x - 2, p.y - 2}, 6 + 5 * hash(seed + i), 3, rgb(150, 144, 136));
  }
  // Wall of rough stones.
  gquad({L, T}, {R, T}, {R - 8, T + h}, {L + 8, T + h}, rgb(150, 136, 124), rgb(124, 112, 104), rgb(92, 82, 80), rgb(110, 98, 92));
  for (int row = 0; row < 3; ++row) {
    const float y = T + 8 + row * (h - 8) / 3.0f;
    DrawLineEx({L + 4, y}, {R - 4, y}, 1.5f, A(rgb(60, 52, 50), 0.6f));
    for (float x = L + 20 + (row % 2) * 30 + hash(seed + row) * 20; x < R - 10; x += 50 + 20 * hash(int(x) + row))
      DrawLineEx({x, y}, {x, y + (h - 8) / 3.0f}, 1.5f, A(rgb(60, 52, 50), 0.6f));
  }
  // The Black Bulls banner: a black cloth with the bull's head and a gold trim, stirring in the wind.
  const float bw = 64, bh = h + 34, sway = std::sin(time * 1.7f) * 2.0f;
  quad({cx - bw * 0.5f, T + 4}, {cx + bw * 0.5f, T + 4}, {cx + bw * 0.5f + sway, T + bh}, {cx - bw * 0.5f + sway, T + bh},
       rgb(24, 22, 28));
  tri({cx - bw * 0.5f + sway, T + bh}, {cx + sway, T + bh - 12}, {cx + bw * 0.5f + sway, T + bh}, rgb(150, 136, 124));
  DrawLineEx({cx - bw * 0.5f, T + 6}, {cx + bw * 0.5f, T + 6}, 3.0f, rgb(214, 180, 80));
  const Vector2 hc{cx + sway * 0.5f, T + bh * 0.45f};
  for (int s = -1; s <= 1; s += 2) {
    Vector2 horn[3] = {{hc.x + s * 8.0f, hc.y - 4}, {hc.x + s * 20.0f, hc.y - 8}, {hc.x + s * 22.0f, hc.y - 20}};
    stroke(horn, 3, 5.0f, 2.0f, rgb(236, 226, 206));
  }
  ellipse(hc, 11, 10, rgb(200, 190, 176));
  ellipse({hc.x, hc.y + 8}, 7, 5, rgb(170, 160, 150));
  for (int s = -1; s <= 1; s += 2) dot({hc.x + s * 4.5f, hc.y - 1}, 1.8f, rgb(24, 22, 28));
  // Grass cap with tufts and a few flowers.
  DrawRectangleRounded({L - 6, T - 6, w + 12, 16}, 0.6f, 6, rgb(70, 112, 52));
  DrawRectangleRounded({L - 6, T - 7, w + 12, 11}, 0.6f, 6, rgb(112, 164, 70));
  DrawRectangleRounded({L - 4, T - 7, w + 8, 4}, 1.0f, 6, rgb(176, 214, 104));
  for (float x = L + 6; x < R - 6; x += 20) {
    const float sw = std::sin(time * 2.0f + x * 0.05f) * 2.0f;
    tri({x - 4, T - 4}, {x + 3, T - 4}, {x + sw, T - 12 - 4 * hash(int(x) * 3)}, rgb(150, 200, 90));
    ellipse({x + 8, T + 8}, 6, 3 + 5 * hash(seed + int(x)), rgb(70, 112, 52));
  }
  for (int i = 0; i < int(w / 90.0f); ++i)
    dot({L + 20 + hash(seed + i * 7) * (w - 40), T - 6}, 2.5f, i % 2 ? rgb(255, 250, 236) : rgb(255, 196, 90));
}

// A wooden balcony on struts; the moving one is a magic broom trailing wind.
void blackBullsThin(const Platform& p, float time, int seed) {
  const Rect& r = p.rect;
  const float L = r.x, R = r.right(), T = r.y, w = r.w, cx = L + w * 0.5f;
  if (p.period > 0) {  // the broom: a long handle, a bundle of straw at the back, wind under it
    const float dir = p.delta.x < 0.0f ? -1.0f : 1.0f;
    const float bob = std::sin(time * 3.0f) * 1.5f;
    for (int k = 0; k < 4; ++k) {  // wind streaks behind it
      const float t = wrapf(time * 2.2f + k * 0.25f, 1.0f), x = cx - dir * (w * 0.5f + t * 80.0f);
      DrawLineEx({x, T + 8 + k * 4.0f}, {x - dir * 40.0f, T + 8 + k * 4.0f}, 2.0f, A(rgb(230, 250, 255), 0.5f * (1.0f - t)));
    }
    glow({cx, T + 14}, w * 0.55f, 16, A(rgb(180, 255, 210), 0.25f), A(rgb(180, 255, 210), 0.0f));
    const float back = cx - dir * w * 0.5f;
    DrawLineEx({cx + dir * w * 0.55f, T + 3 + bob}, {back, T + 4 + bob}, 7.0f, rgb(110, 74, 46));
    DrawLineEx({cx + dir * w * 0.55f, T + 1 + bob}, {back, T + 2 + bob}, 2.5f, rgb(160, 116, 74));
    for (int k = 0; k < 9; ++k) {  // straw
      const float t = k / 8.0f - 0.5f;
      DrawLineEx({back, T + 4 + bob}, {back - dir * (34.0f + 6.0f * std::fabs(t)), T + 4 + bob + t * 22.0f}, 3.0f,
                 k % 2 ? rgb(226, 186, 96) : rgb(196, 152, 70));
    }
    DrawLineEx({back - dir * 2.0f, T - 2 + bob}, {back - dir * 2.0f, T + 10 + bob}, 4.0f, rgb(150, 40, 40));  // binding
    return;
  }
  for (int s = -1; s <= 1; s += 2)  // struts
    DrawLineEx({cx + s * w * 0.3f, T + 8}, {cx + s * w * 0.1f, T + 46}, 6.0f, rgb(84, 58, 40));
  DrawLineEx({cx - w * 0.12f, T + 46}, {cx + w * 0.12f, T + 46}, 6.0f, rgb(70, 48, 34));
  DrawRectangleRounded({L, T - 1, w, 14}, 0.3f, 6, rgb(92, 62, 42));
  DrawRectangleRounded({L, T - 3, w, 11}, 0.3f, 6, rgb(150, 104, 66));
  DrawRectangleRec({L + 3, T - 3, w - 6, 3}, rgb(190, 140, 92));
  for (float x = L + 26; x < R - 8; x += 26) DrawLineEx({x, T - 2}, {x, T + 8}, 1.5f, rgb(92, 62, 42));  // planks
  for (int s = 0; s < 2; ++s) dot({s ? R - 8 : L + 8, T + 3}, 2.0f, rgb(60, 56, 56));  // nails
  (void)seed;
}

// ==== Black Clover: the Grimoire Tower of Hage at twilight ===============================================
constexpr Color kGtHaze{214, 150, 140, 255};
constexpr Color kGtLow{30, 26, 44, 255};
constexpr Color kClover{140, 255, 170, 255};  // the glow of the grimoires

// A three-leaf clover (the mark of the grimoires).
void clover(Vector2 c, float s, Color col) {
  for (int k = 0; k < 3; ++k) {
    const float a = -kPi * 0.5f + k * 2.0f * kPi / 3.0f;
    const Vector2 lc{c.x + std::cos(a) * s * 0.55f, c.y + std::sin(a) * s * 0.55f};
    DrawCircleV({lc.x + std::cos(a + 1.2f) * s * 0.18f, lc.y + std::sin(a + 1.2f) * s * 0.18f}, s * 0.36f, col);
    DrawCircleV({lc.x + std::cos(a - 1.2f) * s * 0.18f, lc.y + std::sin(a - 1.2f) * s * 0.18f}, s * 0.36f, col);
  }
  DrawLineEx(c, {c.x + s * 0.3f, c.y + s * 1.0f}, std::max(1.0f, s * 0.14f), col);
}

// An open grimoire floating in the air: cover, fanned pages, a glow beneath (scale s = its width).
void grimoire(Vector2 c, float s, float tilt, Color cover, float glowA, float flutter) {
  const float h = s * 0.42f;
  glow({c.x, c.y + h * 0.6f}, s * 0.75f, kClover, glowA * 0.4f, 16);
  const Vec2 d = rotate({1, 0}, tilt), n = rotate({0, 1}, tilt);
  auto P = [&](float u, float v) { return Vector2{c.x + d.x * u * s + n.x * v * s, c.y + d.y * u * s + n.y * v * s}; };
  quad(P(-0.52f, 0.0f), P(0.52f, 0.0f), P(0.5f, 0.16f), P(-0.5f, 0.16f), cover);  // the cover under the pages
  quad(P(-0.52f, 0.12f), P(0.52f, 0.12f), P(0.5f, 0.17f), P(-0.5f, 0.17f), shade(cover, 0.6f));
  for (int side = -1; side <= 1; side += 2) {  // the two halves of the pages, curving up at the gutter
    const float f = side < 0 ? flutter : -flutter;
    gquad(P(side * 0.02f, -0.02f), P(side * 0.48f, -0.08f + f * 0.04f), P(side * 0.48f, 0.08f), P(side * 0.02f, 0.1f),
          rgb(250, 246, 230), rgb(236, 226, 200), rgb(214, 202, 176), rgb(230, 222, 200));
    for (int k = 1; k <= 3; ++k)  // lines of text
      DrawLineEx(P(side * 0.1f, -0.03f + k * 0.03f), P(side * 0.4f, -0.06f + k * 0.03f + f * 0.01f), 1.0f, A(rgb(120, 104, 90), 0.6f));
  }
  DrawLineEx(P(0, -0.03f), P(0, 0.12f), 2.0f, shade(cover, 0.7f));
  clover(P(-0.25f, 0.02f), s * 0.08f, A(kClover, 0.9f));
}

// The Grimoire Tower: a vast round tower of old stone ringed with lit windows, books circling it.
void grimoireTower(float x, float base, float w, float h) {
  const Color lit = rgb(150, 120, 140), dark = rgb(58, 46, 74), win = rgb(255, 220, 140);
  const float top = base - h;
  gquad({x - w * 0.5f, top}, {x + w * 0.5f, top}, {x + w * 0.56f, base}, {x - w * 0.56f, base}, lit, dark, dark, lit);
  gquad({x - w * 0.5f, top}, {x - w * 0.2f, top}, {x - w * 0.22f, base}, {x - w * 0.56f, base}, A(rgb(255, 190, 150), 0.25f),
        A(rgb(255, 190, 150), 0.0f), A(rgb(255, 190, 150), 0.0f), A(rgb(255, 190, 150), 0.18f));  // sunset on its side
  for (float y = top + 40; y < base - 20; y += 70) {  // rings of stone and windows
    DrawLineEx({x - w * 0.52f, y}, {x + w * 0.52f, y}, 3.0f, A(rgb(36, 28, 48), 0.6f));
    for (int k = -2; k <= 2; ++k) {
      const float wx = x + k * w * 0.18f, ww = w * 0.05f * (1.0f - std::fabs(k) * 0.15f);
      const bool on = hash(int(y) + k * 7) > 0.35f;
      if (on) glow({wx, y + 30}, ww * 3.0f, win, 0.22f, 12);
      DrawRectangleRec({wx - ww * 0.5f, y + 18, ww, 26}, on ? win : rgb(40, 32, 52));
      DrawCircleV({wx, y + 18}, ww * 0.5f, on ? win : rgb(40, 32, 52));
    }
  }
  for (int k = 0; k < 3; ++k) {  // balconies
    const float y = top + h * (0.25f + 0.25f * k);
    DrawRectangleRec({x - w * 0.6f, y, w * 1.2f, 8}, rgb(84, 66, 92));
    DrawRectangleRec({x - w * 0.6f, y, w * 1.2f, 3}, rgb(170, 130, 140));
  }
  // The crown and its spire, lost in the evening sky.
  quad({x - w * 0.62f, top}, {x + w * 0.62f, top}, {x + w * 0.4f, top - w * 0.3f}, {x - w * 0.4f, top - w * 0.3f}, rgb(70, 56, 86));
  tri({x - w * 0.4f, top - w * 0.3f}, {x + w * 0.4f, top - w * 0.3f}, {x, top - w * 1.3f}, rgb(52, 42, 70));
  tri({x - w * 0.4f, top - w * 0.3f}, {x - w * 0.05f, top - w * 0.3f}, {x, top - w * 1.3f}, rgb(110, 84, 110));
}

// Twilight village of Hage: little houses with lit windows and the church.
void hageHouse(float x, float base, float w, float h, int seed) {
  const Color wall = rgb(96, 74, 96), roof = rgb(52, 40, 62), win = rgb(255, 210, 130);
  DrawRectangleRec({x - w * 0.5f, base - h, w, h}, wall);
  tri({x - w * 0.6f, base - h}, {x + w * 0.6f, base - h}, {x, base - h - w * 0.5f}, roof);
  if (hash(seed) > 0.3f) {
    glow({x, base - h * 0.5f}, w * 0.5f, win, 0.3f, 10);
    DrawRectangleRec({x - w * 0.12f, base - h * 0.65f, w * 0.24f, h * 0.3f}, win);
  }
}

void bakeGrimoireTower(Art& a) {
  const Color haze = kGtHaze;
  a.sky = bakeOpaque(1360, 800, 1.0f, [&] {
    gradient(1360, 800, {{0.0f, rgb(16, 18, 52)}, {0.3f, rgb(44, 40, 96)}, {0.5f, rgb(122, 80, 128)}, {0.62f, rgb(226, 132, 116)},
                         {0.7f, rgb(250, 182, 122)}, {0.74f, rgb(120, 84, 110)}, {1.0f, rgb(60, 46, 78)}});
    for (int i = 0; i < 110; ++i) {
      const float x = hash(i * 3) * 1360, y = std::pow(hash(i * 7), 1.6f) * 380, r = 0.6f + 1.2f * std::pow(hash(i * 11), 3.0f);
      dot({x, y}, r, A(rgb(255, 246, 230), (0.4f + 0.5f * hash(i * 13)) * (1.0f - y / 420.0f)));
    }
    const Vector2 moon{1110, 170};
    glow(moon, 200, rgb(255, 230, 200), 0.18f);
    DrawCircleV(moon, 44, rgb(255, 244, 222));
    DrawCircleV({moon.x + 12, moon.y - 8}, 9, rgb(244, 228, 206));
    DrawCircleV({moon.x - 14, moon.y + 12}, 6, rgb(244, 228, 206));
    for (int i = 0; i < 4; ++i)
      stratus({200 + i * 330 + hash(i) * 80, 330 + hash(i * 5) * 60}, 280, 18, i * 31, rgb(120, 78, 112), rgb(255, 170, 130));
    range(1360, 590, 60, 9, rgb(84, 64, 104), rgb(150, 100, 120), 800, rgb(255, 170, 130), false);
    for (int i = 0; i < 18; ++i)  // the village
      hageHouse(40 + i * 74 + hash(i * 3) * 30, 610 + hash(i * 5) * 8, 30 + hash(i) * 14, 20 + hash(i * 7) * 10, i);
    const float cx = 980;  // the church and its bell tower
    DrawRectangleRec({cx - 30, 560, 60, 50}, rgb(90, 70, 92));
    DrawRectangleRec({cx + 20, 520, 22, 90}, rgb(80, 62, 84));
    tri({cx + 16, 520}, {cx + 46, 520}, {cx + 31, 480}, rgb(52, 40, 62));
    glow({cx + 31, 538}, 14, rgb(255, 220, 140), 0.5f, 10);
    vgrad(0, 600, 1360, 200, A(rgb(60, 46, 78), 0.0f), rgb(60, 46, 78));
  });
  // The tower itself and the grimoires circling it.
  a.mid = bake(1800, 700, 1.0f, [&] {
    grimoireTower(560, 700, 230, 640);
    const Color covers[4] = {rgb(150, 50, 50), rgb(60, 100, 160), rgb(80, 130, 70), rgb(120, 70, 140)};
    for (int i = 0; i < 9; ++i) {
      const float x = 300 + i * 150 + hash(i * 5) * 60, y = 120 + hash(i * 3) * 360;
      grimoire({x, y}, 26 + 16 * hash(i * 7), (hash(i) - 0.5f) * 0.6f, covers[i % 4], 0.8f, 0.0f);
    }
  });
  // Fields of wheat in the last light, a stone wall.
  a.near = bake(1600, 260, 1.25f, [&] {
    vgrad(0, 120, 1600, 140, rgb(122, 92, 82), kGtLow);
    for (int i = 0; i < 220; ++i) {
      const float x = hash(i * 3) * 1600, y = 120 + hash(i * 5) * 60, hh = 18 + 16 * hash(i * 7);
      wrapX(x, 6, 1600, [&](float xx) {
        DrawLineEx({xx, y + hh}, {xx + 3, y}, 2.0f, mixColor(rgb(214, 160, 96), rgb(150, 100, 90), hash(i)));
        ellipse({xx + 3, y}, 2.0f, 4.5f, rgb(240, 190, 110));
      });
    }
    for (float x = 0; x < 1600; x += 40) {  // dry stone wall
      DrawRectangleRounded({x + 2, 186, 36, 18}, 0.4f, 4, rgb(96, 78, 92));
      DrawRectangleRounded({x + 20, 168, 36, 18}, 0.4f, 4, rgb(110, 90, 104));
    }
  });
}

void bgGrimoireTower(const Art& a, Vec2 pc, float skyX, float skyY, float time) {
  drawBackdrop(a.sky, skyX, skyY);
  drawLayer(a.mid, -280.0f - pc.x * 0.1f, -60.0f - pc.y * 0.1f, false);
  const float ny = 470.0f - pc.y * 0.22f;
  drawLayer(a.near, -pc.x * 0.22f, ny, true);
  if (ny + 259.0f < kScreenH) DrawRectangleRec({0, ny + 259.0f, float(kScreenW), kScreenH - ny - 259.0f}, kGtLow);
  motes(32, 83, pc, time, 2.0f, -10.0f, kClover, rgb(255, 230, 160), 1.3f);  // mana rising from the grimoires
  for (int i = 0; i < 6; ++i) {  // loose pages fluttering down
    const float t = wrapf(time * (0.05f + 0.02f * hash(i)) + hash(i * 3), 1.0f);
    const float x = wrapf(hash(i * 7) * kScreenW - pc.x * 0.4f + std::sin(time + i) * 60.0f, kScreenW + 60.0f) - 30.0f;
    const float y = -30.0f + t * (kScreenH + 60.0f), ang = std::sin(time * 2.0f + i) * 0.8f;
    const Vec2 d = rotate({7, 0}, ang), n = rotate({0, 5}, ang);
    quad({x - d.x - n.x, y - d.y - n.y}, {x + d.x - n.x, y + d.y - n.y}, {x + d.x + n.x, y + d.y + n.y}, {x - d.x + n.x, y - d.y + n.y},
         A(rgb(250, 240, 214), 0.85f));
  }
}

// The tower's courtyard: flagstones, a wall of ashlar with the clover carved in it, masonry below.
void grimoireSolid(const Rect& r, float time, int seed) {
  const float L = r.left(), R = r.right(), T = r.top(), w = r.w, h = r.h, cx = L + w * 0.5f;
  const float depth = std::min(170.0f, w * 0.26f);
  constexpr int kN = 12;  // masonry tapering into a broken point
  for (int i = 0; i < kN; ++i) {
    const float u0 = float(i) / kN, u1 = float(i + 1) / kN;
    const float k0 = std::pow(std::sin(u0 * kPi), 0.6f), k1 = std::pow(std::sin(u1 * kPi), 0.6f);
    const Vector2 a0{lerpf(L + 10, R - 10, u0), T + h}, a1{lerpf(L + 10, R - 10, u1), T + h};
    const Vector2 b0{a0.x + (0.5f - u0) * w * 0.1f, T + h + depth * k0 * (0.8f + 0.2f * hash(seed + i))};
    const Vector2 b1{a1.x + (0.5f - u1) * w * 0.1f, T + h + depth * k1 * (0.8f + 0.2f * hash(seed + i + 1))};
    gquad(a0, a1, b1, b0, shade(rgb(110, 92, 112), 1.05f - 0.3f * u0), shade(rgb(110, 92, 112), 1.05f - 0.3f * u1), rgb(36, 30, 48),
          rgb(36, 30, 48));
  }
  for (int row = 1; row <= 3; ++row) {  // courses of masonry
    const float y = T + h + depth * 0.2f * row, half = (w * 0.5f - 14) * (1.0f - 0.18f * row * row / 3.0f);
    DrawLineEx({cx - half, y}, {cx + half, y}, 2.0f, A(rgb(40, 32, 52), 0.55f));
  }
  for (int k = -1; k <= 1; k += 2) {  // two lit arrow slits
    const float x = cx + k * w * 0.22f, y = T + h + depth * 0.25f;
    glow({x, y + 12}, 26, rgb(255, 210, 140), 0.25f, 12);
    DrawRectangleRec({x - 4, y, 8, 26}, rgb(255, 210, 140));
  }
  // Ashlar wall with the glowing clover in a roundel.
  gquad({L, T}, {R, T}, {R - 8, T + h}, {L + 8, T + h}, rgb(170, 146, 150), rgb(132, 112, 126), rgb(96, 80, 100), rgb(126, 106, 120));
  for (int row = 0; row < 2; ++row) {
    const float y = T + 10 + row * (h - 10) * 0.5f;
    DrawLineEx({L + 4, y}, {R - 4, y}, 1.5f, A(rgb(60, 48, 70), 0.6f));
    for (float x = L + 30 + row * 35; x < R - 10; x += 70) DrawLineEx({x, y}, {x, y + (h - 10) * 0.5f}, 1.5f, A(rgb(60, 48, 70), 0.6f));
  }
  const float g = 0.6f + 0.4f * std::sin(time * 2.0f);
  DrawCircleV({cx, T + h * 0.55f}, h * 0.42f, rgb(84, 68, 92));
  DrawRing({cx, T + h * 0.55f}, h * 0.36f, h * 0.42f, 0, 360, 32, rgb(196, 168, 120));
  glow({cx, T + h * 0.55f}, h * 0.7f, kClover, 0.25f * g, 20);
  clover({cx, T + h * 0.5f}, h * 0.2f, A(mixColor(kClover, WHITE, 0.3f), 0.6f + 0.4f * g));
  // Flagstones.
  DrawRectangleRounded({L - 6, T - 6, w + 12, 16}, 0.5f, 6, rgb(110, 92, 108));
  DrawRectangleRounded({L - 6, T - 7, w + 12, 11}, 0.5f, 6, rgb(196, 176, 168));
  DrawRectangleRounded({L - 4, T - 7, w + 8, 4}, 1.0f, 6, rgb(236, 220, 204));
  for (float x = L + 30; x < R - 10; x += 52) DrawLineEx({x, T - 6}, {x - 3, T + 3}, 1.5f, A(rgb(110, 92, 108), 0.8f));
}

// An open grimoire as big as a raft, bobbing, pages stirring; its colour varies with the platform.
void grimoireThin(const Platform& p, float time, int seed) {
  const Rect& r = p.rect;
  const Color covers[3] = {rgb(150, 44, 50), rgb(54, 92, 160), rgb(70, 124, 64)};
  const float flutter = std::sin(time * 3.0f + seed) * 0.6f;
  grimoire({r.x + r.w * 0.5f, r.y + 6.0f}, r.w * 1.02f, 0.0f, covers[(seed / 101) % 3], 0.8f, flutter);
}

// ==== Jujutsu Kaisen: Shibuya on Halloween night, under the veil =========================================
constexpr Color kShLow{16, 12, 24, 255};
constexpr Color kNeonPink{255, 70, 170, 255}, kNeonCyan{70, 230, 255, 255}, kNeonYellow{255, 214, 90, 255},
    kNeonViolet{170, 100, 255, 255};
const Color kNeons[4] = {kNeonPink, kNeonCyan, kNeonYellow, kNeonViolet};

// An office tower: dark mass, a lit edge toward the city glow, rows of windows (some lit).
void tower(float x, float base, float w, float h, int seed, Color body, Color edge, float lit) {
  vgrad(x, base - h, w, h, body, shade(body, 0.75f));
  DrawRectangleRec({x, base - h, 2.5f, h}, edge);
  const float cw = std::max(5.0f, w / std::max(3.0f, std::floor(w / 9.0f))), rh = 10.0f;
  for (float y = base - h + 8; y < base - 6; y += rh)
    for (float wx = x + 4; wx < x + w - 5; wx += cw) {
      const float hv = hash(seed + int(y) * 31 + int(wx) * 7);
      if (hv > lit) continue;
      DrawRectangleRec({wx, y, cw * 0.55f, rh * 0.45f}, hv < lit * 0.25f ? rgb(255, 236, 190) : A(rgb(170, 190, 255), 0.55f));
    }
  if (hash(seed * 3) > 0.5f) {  // antenna with a red light
    DrawLineEx({x + w * 0.6f, base - h}, {x + w * 0.6f, base - h - 26}, 2.0f, body);
    glow({x + w * 0.6f, base - h - 26}, 7, rgb(255, 60, 60), 0.5f, 10);
  }
}

// A vertical neon sign: a frame with glowing blocks for its letters.
void neonSign(float x, float y, float w, float h, Color col, int seed) {
  glow({x + w * 0.5f, y + h * 0.5f}, std::max(w, h) * 0.9f, col, 0.18f, 16);
  DrawRectangleRec({x - 2, y - 2, w + 4, h + 4}, rgb(20, 16, 28));
  DrawRectangleLinesEx({x, y, w, h}, 2.0f, col);
  const bool vertical = h > w;
  const int n = int((vertical ? h : w) / (vertical ? w : h));
  for (int k = 0; k < n; ++k) {
    const float u = (k + 0.5f) / n;
    const Vector2 c = vertical ? Vector2{x + w * 0.5f, y + h * u} : Vector2{x + w * u, y + h * 0.5f};
    const float s = (vertical ? w : h) * 0.28f;
    if (hash(seed + k) > 0.5f) DrawRectangleRec({c.x - s, c.y - s * 0.3f, s * 2, s * 0.6f}, mixColor(col, WHITE, 0.4f));
    else DrawRectangleRec({c.x - s * 0.3f, c.y - s, s * 0.6f, s * 2}, mixColor(col, WHITE, 0.4f));
  }
}

// Shibuya 109: the round tower and its sign at the top.
void tower109(float x, float base, float w, float h) {
  const Color body = rgb(70, 64, 86);
  gquad({x - w * 0.5f, base - h}, {x + w * 0.5f, base - h}, {x + w * 0.5f, base}, {x - w * 0.5f, base}, rgb(110, 96, 120), shade(body, 0.6f),
        shade(body, 0.5f), rgb(90, 80, 104));
  for (float y = base - h + 30; y < base - 10; y += 14) DrawLineEx({x - w * 0.5f, y}, {x + w * 0.5f, y}, 2.0f, A(rgb(200, 190, 255), 0.25f));
  const Rectangle sign{x - w * 0.42f, base - h - 6, w * 0.84f, w * 0.42f};
  glow({x, sign.y + sign.height * 0.5f}, w * 0.9f, rgb(255, 255, 255), 0.18f, 16);
  DrawRectangleRec(sign, rgb(236, 236, 244));
  const float s = sign.height * 0.6f, cy = sign.y + sign.height * 0.5f;  // "109" in blocks
  DrawRectangleRec({x - s * 1.3f, cy - s * 0.5f, s * 0.22f, s}, rgb(30, 26, 40));
  DrawRing({x, cy}, s * 0.26f, s * 0.48f, 0, 360, 20, rgb(30, 26, 40));
  DrawRing({x + s * 1.05f, cy - s * 0.2f}, s * 0.14f, s * 0.3f, 0, 360, 16, rgb(30, 26, 40));
  DrawRectangleRec({x + s * 1.2f, cy - s * 0.2f, s * 0.16f, s * 0.7f}, rgb(30, 26, 40));
}

void bakeShibuya(Art& a) {
  a.sky = bakeOpaque(1360, 800, 1.0f, [&] {
    gradient(1360, 800, {{0.0f, rgb(8, 10, 26)}, {0.4f, rgb(16, 14, 36)}, {0.62f, rgb(52, 24, 60)}, {0.7f, rgb(110, 46, 84)},
                         {0.76f, rgb(70, 30, 62)}, {1.0f, rgb(40, 20, 44)}});
    for (int i = 0; i < 60; ++i) dot({hash(i * 3) * 1360, hash(i * 5) * 300}, 0.8f + hash(i * 7), A(WHITE, 0.4f));
    const Vector2 moon{190, 120};  // a blood-tinged Halloween moon
    glow(moon, 160, rgb(255, 150, 120), 0.2f);
    DrawCircleV(moon, 36, rgb(255, 214, 190));
    DrawCircleV({moon.x + 8, moon.y + 6}, 8, rgb(240, 190, 170));
    // The veil: a dome of black curtain over the district, an inky violet rim and drips along its edge.
    const Vector2 vc{720, 1500};
    const float vr = 1360;
    constexpr int kSeg = 96;
    for (int i = 0; i < kSeg; ++i) {
      const float a0 = kPi + kPi * i / kSeg, a1 = kPi + kPi * (i + 1) / kSeg;
      const Vector2 p0{vc.x + std::cos(a0) * vr, vc.y + std::sin(a0) * vr}, p1{vc.x + std::cos(a1) * vr, vc.y + std::sin(a1) * vr};
      const Vector2 q0{vc.x + std::cos(a0) * (vr + 26), vc.y + std::sin(a0) * (vr + 26)};
      const Vector2 q1{vc.x + std::cos(a1) * (vr + 26), vc.y + std::sin(a1) * (vr + 26)};
      gquad(q0, q1, p1, p0, A(rgb(150, 80, 230), 0.0f), A(rgb(150, 80, 230), 0.0f), A(rgb(150, 80, 230), 0.5f), A(rgb(150, 80, 230), 0.5f));
      gtri(vc, p0, p1, A(rgb(4, 2, 10), 0.86f), A(rgb(12, 6, 22), 0.9f), A(rgb(12, 6, 22), 0.9f));
    }
    for (int i = 0; i < 40; ++i) {  // ink running down from the rim
      const float a = kPi * (1.08f + 0.84f * hash(i * 3)), len = 30 + 120 * std::pow(hash(i * 5), 2.0f);
      const Vector2 p{vc.x + std::cos(a) * vr, vc.y + std::sin(a) * vr};
      gquad({p.x - 3, p.y}, {p.x + 3, p.y}, {p.x + 1, p.y + len}, {p.x - 1, p.y + len}, rgb(6, 3, 14), rgb(6, 3, 14), A(rgb(6, 3, 14), 0.0f),
            A(rgb(6, 3, 14), 0.0f));
      DrawCircleV({p.x, p.y + len * 0.8f}, 2.6f, A(rgb(6, 3, 14), 0.8f));
    }
    // The skyline, glowing at its foot.
    for (int i = 0; i < 30; ++i) {
      const float x = i * 46 + hash(i * 3) * 20, w = 40 + hash(i * 5) * 40, h = 80 + hash(i * 7) * 200;
      tower(x, 620, w, h, i * 13, rgb(30, 24, 46), rgb(120, 60, 120), 0.35f);
    }
    tower109(240, 620, 70, 250);
    vgrad(0, 540, 1360, 90, A(rgb(255, 90, 140), 0.0f), A(rgb(255, 90, 140), 0.25f));
    vgrad(0, 620, 1360, 180, rgb(50, 22, 50), rgb(26, 14, 34));
  });
  // Nearer blocks, signs and the great screen.
  a.mid = bake(2000, 520, 1.0f, [&] {
    for (int i = 0; i < 16; ++i) {
      const float x = i * 128 + hash(i * 11) * 40, w = 100 + hash(i * 3) * 60, h = 200 + hash(i * 9) * 220;
      tower(x, 520, w, h, 300 + i, rgb(24, 20, 38), rgb(150, 70, 140), 0.45f);
      if (hash(i * 17) > 0.35f)
        neonSign(x + w * (0.15f + 0.5f * hash(i * 19)), 520 - h + 30 + 60 * hash(i), 20, 70 + 50 * hash(i * 23), kNeons[i % 4], i);
    }
    // The great screen over the crossing.
    const Rectangle sc{820, 250, 220, 124};
    glow({sc.x + sc.width * 0.5f, sc.y + sc.height * 0.5f}, 200, rgb(120, 180, 255), 0.18f, 24);
    DrawRectangleRec({sc.x - 8, sc.y - 8, sc.width + 16, sc.height + 16}, rgb(18, 14, 26));
    gquad({sc.x, sc.y}, {sc.x + sc.width, sc.y}, {sc.x + sc.width, sc.y + sc.height}, {sc.x, sc.y + sc.height}, rgb(255, 90, 170),
          rgb(110, 90, 255), rgb(60, 220, 255), rgb(255, 170, 90));
    DrawCircleV({sc.x + sc.width * 0.35f, sc.y + sc.height * 0.5f}, 30, A(WHITE, 0.6f));
    for (int k = 0; k < 4; ++k) DrawRectangleRec({sc.x + 130, sc.y + 28 + k * 18.0f, 70 - k * 10.0f, 8}, A(WHITE, 0.7f));
    for (int i = 0; i < 5; ++i)  // horizontal signs along the street
      neonSign(80 + i * 380 + hash(i * 41) * 60, 360 + hash(i * 43) * 60, 120, 32, kNeons[(i + 2) % 4], 90 + i);
  });
  // The elevated railway, its arches lit from below.
  a.near = bake(1600, 260, 1.25f, [&] {
    DrawRectangleRec({0, 60, 1600, 34}, rgb(44, 40, 56));
    DrawRectangleRec({0, 60, 1600, 4}, rgb(120, 110, 140));
    for (float x = 0; x < 1600; x += 160) {
      DrawRectangleRec({x, 94, 24, 166}, rgb(34, 30, 46));
      DrawRectangleRec({x + 24, 94, 136, 18}, rgb(38, 34, 50));
      glow({x + 92, 200}, 70, rgb(255, 170, 90), 0.2f, 16);
    }
    for (float x = 8; x < 1600; x += 24) DrawLineEx({x, 60}, {x, 40}, 2.0f, rgb(60, 56, 74));  // railing
    DrawLineEx({0, 40}, {1600, 40}, 2.0f, rgb(60, 56, 74));
    vgrad(0, 200, 1600, 60, A(kShLow, 0.0f), kShLow);
  });
}

void bgShibuya(const Art& a, Vec2 pc, float skyX, float skyY, float time) {
  drawBackdrop(a.sky, skyX, skyY);
  const float mx = -360.0f - pc.x * 0.12f, my = 150.0f - pc.y * 0.12f;
  drawLayer(a.mid, mx, my, false);
  for (int i = 0; i < 16; ++i) {  // neon flickering and the screen's changing light
    if (hash(i * 17) <= 0.35f) continue;
    const float x = i * 128 + hash(i * 11) * 40, w = 100 + hash(i * 3) * 60, h = 200 + hash(i * 9) * 220;
    const float sx = x + w * (0.15f + 0.5f * hash(i * 19)) + 10, sy = 520 - h + 30 + 60 * hash(i) + (70 + 50 * hash(i * 23)) * 0.5f;
    const float f = std::sin(time * (2.0f + 3.0f * hash(i)) + i) > 0.92f ? 0.0f : 1.0f;
    glow({mx + sx, my + sy}, 40, kNeons[i % 4], 0.18f * f, 12);
  }
  const float sp = 0.5f + 0.5f * std::sin(time * 1.3f);
  glow({mx + 930, my + 312}, 150, mixColor(rgb(255, 90, 170), rgb(70, 200, 255), sp), 0.14f, 20);
  const float ny = 480.0f - pc.y * 0.22f, nx = -pc.x * 0.22f;
  drawLayer(a.near, nx, ny, true);
  // A train crossing the viaduct now and then.
  const float tt = wrapf(time / 14.0f, 1.0f), tx = -900.0f + tt * 4200.0f;
  if (tx < kScreenW + 50.0f) {
    for (int c = 0; c < 6; ++c) {
      const float cx = tx - c * 150.0f;
      DrawRectangleRounded({cx, ny + 22, 144, 36}, 0.25f, 6, rgb(200, 204, 214));
      DrawRectangleRec({cx, ny + 44, 144, 5}, rgb(80, 180, 90));
      for (int k = 0; k < 6; ++k) DrawRectangleRec({cx + 8 + k * 22.0f, ny + 28, 14, 11}, rgb(255, 238, 190));
    }
  }
  if (ny + 259.0f < kScreenH) {
    DrawRectangleRec({0, ny + 259.0f, float(kScreenW), kScreenH - ny - 259.0f}, kShLow);
    for (int i = 0; i < 10; ++i) {  // headlights and tail lights in the street below
      const bool right = i % 2 == 0;
      const float t = wrapf(time * (0.12f + 0.05f * hash(i)) + hash(i * 3), 1.0f);
      const float x = right ? -100.0f + t * (kScreenW + 200.0f) : kScreenW + 100.0f - t * (kScreenW + 200.0f);
      const float y = ny + 280.0f + (right ? 0.0f : 16.0f);
      const Color c = right ? rgb(255, 240, 200) : rgb(255, 60, 60);
      gquad({x, y - 2}, {x - (right ? 60.0f : -60.0f), y - 1}, {x - (right ? 60.0f : -60.0f), y + 1}, {x, y + 2}, A(c, 0.8f), A(c, 0.0f),
            A(c, 0.0f), A(c, 0.8f));
    }
  }
  motes(26, 97, pc, time, 3.0f, -16.0f, rgb(150, 90, 255), rgb(60, 20, 90), 1.4f);  // cursed energy rising
}

// The elevated expressway: asphalt and a concrete parapet, a box girder and piers lit orange beneath.
void shibuyaSolid(const Rect& r, float time, int seed) {
  (void)seed;
  const float L = r.left(), R = r.right(), T = r.top(), w = r.w, h = r.h, cx = L + w * 0.5f;
  for (int k = -1; k <= 1; k += 2) {  // piers going down into the dark
    const float px = cx + k * w * 0.26f;
    gquad({px - 26, T + h + 30}, {px + 26, T + h + 30}, {px + 26, T + h + 420}, {px - 26, T + h + 420}, rgb(92, 88, 104), rgb(56, 52, 68),
          A(rgb(56, 52, 68), 0.0f), A(rgb(92, 88, 104), 0.0f));
  }
  vgrad(L + 14, T + h, w - 28, 34, rgb(56, 54, 70), rgb(30, 28, 40));  // box girder
  for (float x = L + 30; x < R - 20; x += 60) DrawRectangleRec({x, T + h + 2, 3, 30}, rgb(24, 22, 32));
  for (float x = L + 70; x < R - 40; x += 180) {  // sodium lamps under the deck
    glow({x, T + h + 40}, 60, rgb(255, 170, 80), 0.3f + 0.05f * std::sin(time * 3.0f + x), 16);
    DrawRectangleRec({x - 8, T + h + 32, 16, 5}, rgb(255, 210, 140));
  }
  // Concrete parapet with expansion joints, tinted by the neon.
  gquad({L, T}, {R, T}, {R - 6, T + h}, {L + 6, T + h}, rgb(150, 144, 164), rgb(120, 112, 140), rgb(84, 78, 100), rgb(104, 94, 120));
  gquad({L, T}, {L + w * 0.4f, T}, {L + w * 0.4f, T + h}, {L + 6, T + h}, A(kNeonPink, 0.12f), A(kNeonPink, 0.0f), A(kNeonPink, 0.0f),
        A(kNeonPink, 0.08f));
  for (float x = L + 80; x < R - 30; x += 140) DrawRectangleRec({x, T + 6, 2.5f, h - 10}, rgb(70, 66, 86));
  DrawRectangleRec({L + 6, T + h - 10, w - 12, 3}, rgb(60, 56, 74));
  // A green expressway sign.
  const Rectangle sg{cx - 70, T + 10, 140, h - 22};
  DrawRectangleRec({sg.x - 3, sg.y - 3, sg.width + 6, sg.height + 6}, rgb(230, 230, 230));
  DrawRectangleRec(sg, rgb(20, 112, 72));
  drawText("SHIBUYA", sg.x + 16, sg.y + sg.height * 0.5f - 11, 22, rgb(240, 244, 240), FontKind::Title);
  // Asphalt with a lit curb.
  DrawRectangleRounded({L - 6, T - 6, w + 12, 15}, 0.4f, 6, rgb(36, 34, 44));
  DrawRectangleRounded({L - 6, T - 7, w + 12, 10}, 0.4f, 6, rgb(70, 68, 82));
  DrawRectangleRounded({L - 4, T - 7, w + 8, 3}, 1.0f, 6, rgb(180, 170, 200));
  for (float x = L + 20; x < R - 20; x += 46) DrawRectangleRec({x, T - 4, 22, 2}, rgb(240, 210, 90));
}

// A billboard: a steel catwalk on top, the lit panel and its frame below.
void shibuyaThin(const Platform& p, float time, int seed) {
  const Rect& r = p.rect;
  const float L = r.x, R = r.right(), T = r.y, w = r.w, cx = L + w * 0.5f;
  const Color col = kNeons[(seed / 101) % 4], col2 = kNeons[(seed / 101 + 1) % 4];
  const float f = 0.85f + 0.15f * std::sin(time * 6.0f + seed);
  glow({cx, T + 40}, w * 0.7f, 50, A(col, 0.2f * f), A(col, 0.0f));
  DrawRectangleRec({L + 6, T + 8, w - 12, 64}, rgb(20, 16, 28));
  gquad({L + 10, T + 12}, {R - 10, T + 12}, {R - 10, T + 68}, {L + 10, T + 68}, A(col, f), A(col2, f), A(shade(col2, 0.6f), f),
        A(shade(col, 0.6f), f));
  DrawCircleV({L + 40, T + 40}, 16, A(WHITE, 0.75f));
  for (int k = 0; k < 3; ++k) DrawRectangleRec({L + 66, T + 24 + k * 13.0f, w - 96 - k * 14.0f, 6}, A(WHITE, 0.8f));
  for (int s = -1; s <= 1; s += 2) DrawLineEx({cx + s * w * 0.3f, T + 72}, {cx + s * w * 0.3f, T + 120}, 5.0f, rgb(46, 42, 58));
  DrawRectangleRounded({L, T - 1, w, 13}, 0.3f, 6, rgb(40, 38, 50));
  DrawRectangleRounded({L, T - 3, w, 9}, 0.3f, 6, rgb(110, 108, 124));
  for (float x = L + 6; x < R - 4; x += 8) DrawLineEx({x, T - 2}, {x, T + 5}, 1.0f, rgb(60, 58, 72));  // grating
  DrawRectangleRec({L + 2, T - 3, w - 4, 2}, rgb(170, 166, 186));
}

// ==== Jujutsu Kaisen: the Tokyo school of jujutsu in autumn ===============================================
constexpr Color kJhHaze{238, 214, 186, 255};
constexpr Color kJhLow{70, 46, 36, 255};
const Color kMaple[4] = {rgb(214, 64, 40), rgb(238, 116, 44), rgb(250, 170, 60), rgb(180, 40, 44)};

// Maple tree ablaze with autumn: a dark trunk, clumps of red and orange leaves.
void mapleTree(Vector2 base, float s, int seed) {
  DrawLineEx(base, {base.x + s * 0.05f, base.y - s * 0.65f}, s * 0.1f, rgb(70, 46, 40));
  DrawLineEx({base.x, base.y - s * 0.4f}, {base.x - s * 0.3f, base.y - s * 0.75f}, s * 0.045f, rgb(70, 46, 40));
  DrawLineEx({base.x, base.y - s * 0.45f}, {base.x + s * 0.32f, base.y - s * 0.78f}, s * 0.045f, rgb(70, 46, 40));
  for (int i = 0; i < 9; ++i) {
    const float a = hash(seed + i * 3) * 2.0f * kPi, d = s * 0.3f * std::sqrt(hash(seed + i * 5));
    const Vector2 p{base.x + std::cos(a) * d * 1.2f, base.y - s * 0.9f + std::sin(a) * d * 0.7f};
    const float rr = s * (0.16f + 0.1f * hash(seed + i * 7));
    softEllipse({p.x + rr * 0.15f, p.y + rr * 0.2f}, rr, rr * 0.8f, shade(kMaple[i % 4], 0.6f), 3.0f, 16);
    softEllipse(p, rr * 0.9f, rr * 0.72f, kMaple[(i + seed) % 4], 3.0f, 16);
    softEllipse({p.x + rr * 0.25f, p.y - rr * 0.25f}, rr * 0.4f, rr * 0.3f, mixColor(kMaple[(i + seed) % 4], rgb(255, 230, 150), 0.5f), 2.0f, 12);
  }
}

// Japanese cedar: a tall dark cone.
void cedar(Vector2 base, float s, Color dark, Color lit) {
  DrawLineEx(base, {base.x, base.y - s * 0.3f}, s * 0.06f, rgb(70, 50, 40));
  gtri({base.x - s * 0.2f, base.y - s * 0.15f}, {base.x + s * 0.2f, base.y - s * 0.15f}, {base.x, base.y - s}, dark, shade(dark, 0.8f), lit);
}

// Temple roof seen from the front: dark tiles, a ridge with golden ends, eaves curling up.
void templeRoof(float cx, float y, float w, float h, Color tile, Color edge) {
  const Vector2 e0{cx - w * 0.5f, y}, e1{cx + w * 0.5f, y}, r0{cx - w * 0.28f, y - h}, r1{cx + w * 0.28f, y - h};
  gquad(r0, r1, e1, e0, mixColor(tile, WHITE, 0.15f), tile, shade(tile, 0.8f), mixColor(tile, WHITE, 0.1f));
  for (float u = 0.05f; u < 1.0f; u += 0.05f)
    DrawLineEx({lerpf(r0.x, r1.x, u), y - h + 2}, {lerpf(e0.x, e1.x, u), y}, 1.0f, A(edge, 0.5f));
  DrawLineEx(e0, e1, std::max(2.0f, h * 0.1f), edge);
  tri({e0.x - h * 0.3f, y - h * 0.38f}, {e0.x + h * 0.5f, y - 1}, {e0.x + h * 0.2f, y + 2}, edge);
  tri({e1.x + h * 0.3f, y - h * 0.38f}, {e1.x - h * 0.5f, y - 1}, {e1.x - h * 0.2f, y + 2}, edge);
  DrawLineEx({r0.x - 4, y - h}, {r1.x + 4, y - h}, std::max(3.0f, h * 0.16f), edge);
  for (int s = -1; s <= 1; s += 2) DrawCircleV({s < 0 ? r0.x - 4 : r1.x + 4, y - h - 2}, std::max(2.0f, h * 0.1f), rgb(214, 170, 70));
}

// A temple hall: vermilion pillars, white panels, a great roof.
void templeHall(float cx, float base, float w, float h, Color haze, float fog) {
  auto F = [&](Color c) { return mixColor(c, haze, fog); };
  DrawRectangleRec({cx - w * 0.55f, base - 10, w * 1.1f, 10}, F(rgb(150, 140, 130)));  // stone base
  vgrad(cx - w * 0.5f, base - 10 - h, w, h, F(rgb(240, 232, 214)), F(rgb(206, 196, 180)));
  for (int k = 0; k <= 6; ++k) DrawRectangleRec({cx - w * 0.5f + k * w / 6.0f - 3, base - 10 - h, 6, h}, F(rgb(190, 60, 40)));
  DrawRectangleRec({cx - w * 0.5f, base - 10 - h, w, 7}, F(rgb(170, 50, 36)));
  for (int k = 0; k < 6; ++k)  // latticed doors
    DrawRectangleRec({cx - w * 0.5f + (k + 0.25f) * w / 6.0f, base - 10 - h * 0.7f, w / 12.0f, h * 0.6f}, F(rgb(120, 70, 50)));
  templeRoof(cx, base - 10 - h + 2, w * 1.3f, h * 0.9f, F(rgb(60, 62, 74)), F(rgb(34, 34, 44)));
}

// A five-storey pagoda.
void pagoda(float cx, float base, float s, Color haze, float fog) {
  auto F = [&](Color c) { return mixColor(c, haze, fog); };
  float y = base;
  for (int k = 0; k < 5; ++k) {
    const float w = s * (0.5f - 0.06f * k), h = s * 0.16f;
    DrawRectangleRec({cx - w * 0.32f, y - h, w * 0.64f, h}, F(rgb(186, 66, 44)));
    DrawRectangleRec({cx - w * 0.2f, y - h * 0.8f, w * 0.4f, h * 0.6f}, F(rgb(236, 226, 206)));
    templeRoof(cx, y - h + 2, w, h * 0.55f, F(rgb(58, 60, 72)), F(rgb(32, 32, 42)));
    y -= h + h * 0.4f;
  }
  DrawLineEx({cx, y + s * 0.05f}, {cx, y - s * 0.28f}, std::max(2.0f, s * 0.025f), F(rgb(200, 160, 70)));
  for (int k = 0; k < 5; ++k) DrawCircleV({cx, y - s * 0.04f * k}, std::max(1.5f, s * 0.018f), F(rgb(214, 176, 80)));
}

// A vermilion torii.
void torii(float cx, float base, float s, Color haze, float fog) {
  const Color red = mixColor(rgb(214, 62, 40), haze, fog), black = mixColor(rgb(36, 30, 32), haze, fog);
  for (int k = -1; k <= 1; k += 2) DrawLineEx({cx + k * s * 0.32f, base}, {cx + k * s * 0.29f, base - s * 0.86f}, s * 0.07f, red);
  DrawLineEx({cx - s * 0.42f, base - s * 0.7f}, {cx + s * 0.42f, base - s * 0.7f}, s * 0.05f, red);
  quad({cx - s * 0.56f, base - s * 0.86f}, {cx + s * 0.56f, base - s * 0.86f}, {cx + s * 0.6f, base - s * 0.97f},
       {cx - s * 0.6f, base - s * 0.97f}, red);
  quad({cx - s * 0.6f, base - s * 0.97f}, {cx + s * 0.6f, base - s * 0.97f}, {cx + s * 0.66f, base - s * 1.04f},
       {cx - s * 0.66f, base - s * 1.04f}, black);
}

// A stone lantern.
void stoneLantern(Vector2 base, float s, Color stone) {
  DrawRectangleRec({base.x - s * 0.22f, base.y - s * 0.12f, s * 0.44f, s * 0.12f}, stone);
  DrawRectangleRec({base.x - s * 0.07f, base.y - s * 0.55f, s * 0.14f, s * 0.43f}, stone);
  DrawRectangleRec({base.x - s * 0.18f, base.y - s * 0.78f, s * 0.36f, s * 0.24f}, stone);
  glow({base.x, base.y - s * 0.66f}, s * 0.3f, rgb(255, 200, 120), 0.4f, 12);
  DrawRectangleRec({base.x - s * 0.08f, base.y - s * 0.72f, s * 0.16f, s * 0.12f}, rgb(255, 214, 140));
  quad({base.x - s * 0.32f, base.y - s * 0.78f}, {base.x + s * 0.32f, base.y - s * 0.78f}, {base.x + s * 0.1f, base.y - s * 0.95f},
       {base.x - s * 0.1f, base.y - s * 0.95f}, shade(stone, 0.85f));
  DrawCircleV({base.x, base.y - s * 0.98f}, s * 0.05f, stone);
}

void bakeJujutsuHigh(Art& a) {
  const Color haze = kJhHaze;
  a.sky = bakeOpaque(1360, 800, 1.0f, [&] {
    gradient(1360, 800, {{0.0f, rgb(86, 136, 198)}, {0.36f, rgb(150, 184, 220)}, {0.6f, rgb(236, 212, 184)}, {0.7f, haze}, {1.0f, haze}});
    const Vector2 sun{1120, 380};
    glow(sun, 640, 420, A(rgb(255, 222, 170), 0.45f), A(rgb(255, 222, 170), 0.0f));
    glow(sun, 130, rgb(255, 240, 210), 0.6f);
    const Vector2 cl[4] = {{300, 120}, {720, 80}, {1000, 200}, {140, 260}};
    for (int i = 0; i < 4; ++i)
      cumulus(cl[i], 160 + hash(i * 5) * 120, 50 + hash(i * 3) * 40, i * 53, rgb(200, 190, 200), rgb(255, 246, 236), A(WHITE, 0.5f), 0.6f);
    range(1360, 520, 120, 6, rgb(140, 150, 170), rgb(214, 204, 196), 800, rgb(255, 236, 200), true);
    rlPushMatrix();  // forested ridges in autumn
    rlTranslatef(0, 430, 0);
    range(1360, 170, 90, 15, rgb(120, 116, 104), rgb(200, 180, 156), 370, rgb(255, 214, 160), true);
    for (int i = 0; i < 70; ++i) {
      const float x = hash(i * 3) * 1360, y = 170 - 90 * ridge(x, 1360, 15) + 10 + hash(i * 5) * 50;
      softEllipse({x, y}, 10 + 8 * hash(i), 7, mixColor(kMaple[i % 4], haze, 0.45f), 2.0f, 12);
    }
    range(1360, 250, 70, 23, rgb(110, 92, 70), rgb(170, 140, 110), 370, rgb(255, 200, 140), true);
    for (int i = 0; i < 60; ++i) {
      const float x = hash(i * 7) * 1360, y = 250 - 70 * ridge(x, 1360, 23) + 8 + hash(i * 11) * 40;
      softEllipse({x, y}, 12 + 10 * hash(i * 3), 9, i % 3 ? mixColor(kMaple[i % 4], haze, 0.3f) : mixColor(rgb(60, 90, 60), haze, 0.3f),
                  2.0f, 12);
    }
    rlPopMatrix();
  });
  // The school: halls, the pagoda, a torii at the top of the steps, cedars and maples around.
  a.mid = bake(2000, 460, 1.25f, [&] {
    for (int i = 0; i < 14; ++i) cedar({60 + i * 150 + hash(i * 3) * 60, 400}, 150 + 60 * hash(i), mixColor(rgb(40, 70, 56), haze, 0.3f),
                                       mixColor(rgb(80, 116, 80), haze, 0.3f));
    pagoda(420, 420, 300, haze, 0.25f);
    templeHall(980, 420, 280, 90, haze, 0.2f);
    templeHall(1460, 430, 200, 70, haze, 0.28f);
    torii(1720, 430, 120, haze, 0.2f);
    for (int k = 0; k < 6; ++k)
      DrawRectangleRec({1660 + k * 4.0f, 430 + k * 6.0f, 120 - k * 8.0f, 6}, mixColor(rgb(170, 160, 150), haze, 0.2f));  // steps
    for (int i = 0; i < 9; ++i) mapleTree({160 + i * 220 + hash(i * 7) * 70, 450}, 90 + 40 * hash(i * 3), 20 + i);
  });
  // Near: big maples and stone lanterns.
  a.near = bake(1600, 300, 1.25f, [&] {
    const float tx[3] = {100, 820, 1450}, ts[3] = {240, 180, 220};
    for (int k = 0; k < 3; ++k) wrapX(tx[k], 150, 1600, [&](float x) { mapleTree({x, 300}, ts[k], 40 + k * 5); });
    for (int k = 0; k < 4; ++k) wrapX(300 + k * 380.0f, 30, 1600, [&](float x) { stoneLantern({x, 296}, 90, rgb(150, 144, 136)); });
    vgrad(0, 270, 1600, 30, rgb(120, 84, 60), kJhLow);
  });
}

void bgJujutsuHigh(const Art& a, Vec2 pc, float skyX, float skyY, float time) {
  drawBackdrop(a.sky, skyX, skyY);
  drawLayer(a.mid, -360.0f - pc.x * 0.12f, 160.0f - pc.y * 0.12f, false);
  const float ny = 440.0f - pc.y * 0.22f;
  drawLayer(a.near, -pc.x * 0.22f, ny, true);
  if (ny + 299.0f < kScreenH) DrawRectangleRec({0, ny + 299.0f, float(kScreenW), kScreenH - ny - 299.0f}, kJhLow);
  motes(20, 107, pc, time, 4.0f, -5.0f, rgb(255, 220, 160), rgb(255, 250, 220), 1.2f);
  leaves(18, 21, pc, time, kMaple[0], kMaple[2]);  // falling maple leaves
}

// The temple terrace: a wooden floor, a stone base with a vermilion beam, an earthen mound beneath.
void jujutsuSolid(const Rect& r, float time, int seed) {
  (void)time;
  const float L = r.left(), R = r.right(), T = r.top(), w = r.w, h = r.h;
  const float depth = h + std::min(160.0f, w * 0.26f);
  constexpr int kN = 14;
  Vector2 bot[kN + 1];
  for (int i = 0; i <= kN; ++i) {
    const float u = float(i) / kN, k = std::pow(std::sin(u * kPi), 0.65f);
    bot[i] = {lerpf(L + 10, R - 10, u) + (0.5f - u) * w * 0.12f * k, T + h + (depth - h) * k * (0.8f + 0.2f * hash(seed + i * 13))};
  }
  for (int i = 0; i < kN; ++i) {  // the mound: earth under autumn grass
    const float u0 = float(i) / kN, u1 = float(i + 1) / kN;
    gquad({lerpf(L + 10, R - 10, u0), T + h}, {lerpf(L + 10, R - 10, u1), T + h}, bot[i + 1], bot[i], shade(rgb(150, 104, 70), 1.05f - 0.3f * u0),
          shade(rgb(150, 104, 70), 1.05f - 0.3f * u1), rgb(58, 38, 34), rgb(58, 38, 34));
  }
  for (int i = 1; i < kN; i += 2) {  // fallen leaves caught on the mound
    const Vector2 p{lerpf(lerpf(L + 10, R - 10, float(i) / kN), bot[i].x, 0.2f), lerpf(T + h, bot[i].y, 0.2f)};
    ellipse(p, 12 + 6 * hash(seed + i), 4, A(kMaple[i % 4], 0.85f));
  }
  // Granite base, a vermilion beam, the white wall of the veranda.
  gquad({L, T}, {R, T}, {R - 8, T + h}, {L + 8, T + h}, rgb(176, 168, 160), rgb(150, 142, 136), rgb(112, 106, 104), rgb(132, 126, 120));
  for (int row = 0; row < 2; ++row) {
    const float y = T + 16 + row * (h - 16) * 0.5f;
    DrawLineEx({L + 6, y}, {R - 6, y}, 1.5f, A(rgb(80, 74, 72), 0.6f));
    for (float x = L + 40 + row * 50; x < R - 20; x += 100) DrawLineEx({x, y}, {x, y + (h - 16) * 0.5f}, 1.5f, A(rgb(80, 74, 72), 0.6f));
  }
  DrawRectangleRec({L - 2, T + 4, w + 4, 10}, rgb(196, 62, 42));
  DrawRectangleRec({L - 2, T + 4, w + 4, 3}, rgb(236, 110, 80));
  // The wooden floor.
  DrawRectangleRounded({L - 6, T - 6, w + 12, 13}, 0.3f, 6, rgb(92, 58, 40));
  DrawRectangleRounded({L - 6, T - 7, w + 12, 9}, 0.3f, 6, rgb(170, 116, 72));
  DrawRectangleRec({L - 4, T - 7, w + 8, 3}, rgb(214, 160, 104));
  for (float x = L + 24; x < R - 10; x += 34) DrawLineEx({x, T - 6}, {x, T + 1}, 1.2f, rgb(110, 72, 48));
  for (int i = 0; i < int(w / 100.0f); ++i)  // a few leaves on the floor
    ellipse({L + 20 + hash(seed + i * 7) * (w - 40), T - 5}, 5, 2.5f, kMaple[i % 4]);
}

// A walkway of planks on vermilion posts.
void jujutsuThin(const Platform& p, float time, int seed) {
  (void)time;
  (void)seed;
  const Rect& r = p.rect;
  const float L = r.x, R = r.right(), T = r.y, w = r.w;
  for (int s = 0; s < 2; ++s) {
    const float x = s ? R - 18 : L + 18;
    gquad({x - 5, T + 8}, {x + 5, T + 8}, {x + 5, T + 90}, {x - 5, T + 90}, rgb(196, 62, 42), rgb(150, 44, 32), A(rgb(150, 44, 32), 0.0f),
          A(rgb(196, 62, 42), 0.0f));
  }
  DrawRectangleRec({L + 2, T + 8, w - 4, 7}, rgb(176, 54, 38));
  DrawRectangleRounded({L, T - 2, w, 12}, 0.3f, 6, rgb(92, 58, 40));
  DrawRectangleRounded({L, T - 3, w, 9}, 0.3f, 6, rgb(170, 116, 72));
  DrawRectangleRec({L + 2, T - 3, w - 4, 3}, rgb(214, 160, 104));
  for (float x = L + 22; x < R - 8; x += 22) DrawLineEx({x, T - 2}, {x, T + 5}, 1.2f, rgb(110, 72, 48));
}

// ==== Naruto: Konoha, the Hidden Leaf, under the Hokage Rock ===============================================
constexpr Color kKoHaze{214, 230, 238, 255};
constexpr Color kKoLow{46, 70, 46, 255};

// The leaf of Konoha: a spiral ending in a point (the village's mark).
void leafMark(Vector2 c, float s, Color col, float thick) {
  Vector2 prev{c.x + s * 0.05f, c.y};
  for (int i = 1; i <= 16; ++i) {
    const float t = i / 16.0f, a = t * 1.7f * 2.0f * kPi, r = s * 0.42f * t;
    const Vector2 p{c.x + std::cos(a) * r, c.y + std::sin(a) * r};
    DrawLineEx(prev, p, thick, col);
    prev = p;
  }
  DrawLineEx(prev, {c.x + s * 0.55f, c.y - s * 0.45f}, thick, col);
  DrawLineEx({c.x + s * 0.55f, c.y - s * 0.45f}, {c.x + s * 0.2f, c.y - s * 0.55f}, thick, col);
}

// One face carved in the Hokage Rock; `kind` picks the hair (0 long parted, 1 short with a headband,
// 2 old with a beard, 3 spiky bangs).
void hokageFace(Vector2 c, float s, int kind, Color lit, Color mid, Color dark) {
  const float w = s * 0.62f;
  // Hair behind and around the face.
  switch (kind) {
    case 0:
      quad({c.x - w * 0.62f, c.y - s * 0.45f}, {c.x + w * 0.62f, c.y - s * 0.45f}, {c.x + w * 0.74f, c.y + s * 0.42f},
           {c.x - w * 0.74f, c.y + s * 0.42f}, mid);
      break;
    case 1:
      for (int k = -2; k <= 2; ++k) tri({c.x + k * w * 0.24f - w * 0.16f, c.y - s * 0.36f}, {c.x + k * w * 0.24f + w * 0.16f, c.y - s * 0.36f},
                                        {c.x + k * w * 0.3f, c.y - s * 0.56f}, mid);
      break;
    case 2:
      ellipse({c.x, c.y - s * 0.36f}, w * 0.62f, s * 0.2f, mid);
      break;
    default:
      for (int k = -3; k <= 3; ++k) tri({c.x + k * w * 0.18f - w * 0.14f, c.y - s * 0.3f}, {c.x + k * w * 0.18f + w * 0.14f, c.y - s * 0.3f},
                                        {c.x + k * w * 0.26f, c.y - s * 0.6f - (k % 2) * s * 0.06f}, mid);
      break;
  }
  // The face, lit from the upper left.
  gquad({c.x - w * 0.5f, c.y - s * 0.34f}, {c.x + w * 0.5f, c.y - s * 0.34f}, {c.x + w * 0.42f, c.y + s * 0.3f}, {c.x - w * 0.42f, c.y + s * 0.3f},
        lit, mid, shade(mid, 0.9f), mixColor(lit, mid, 0.4f));
  tri({c.x - w * 0.42f, c.y + s * 0.3f}, {c.x + w * 0.42f, c.y + s * 0.3f}, {c.x, c.y + s * 0.46f}, shade(mid, 0.92f));  // chin
  if (kind == 2) tri({c.x - w * 0.36f, c.y + s * 0.22f}, {c.x + w * 0.36f, c.y + s * 0.22f}, {c.x, c.y + s * 0.5f}, shade(mid, 0.75f));
  if (kind == 1 || kind == 3) DrawRectangleRec({c.x - w * 0.5f, c.y - s * 0.3f, w, s * 0.1f}, shade(mid, 0.8f));  // headband
  for (int e = -1; e <= 1; e += 2) {  // brows and eyes
    DrawLineEx({c.x + e * w * 0.1f, c.y - s * 0.14f}, {c.x + e * w * 0.36f, c.y - s * 0.17f}, std::max(1.5f, s * 0.035f), dark);
    ellipse({c.x + e * w * 0.22f, c.y - s * 0.06f}, w * 0.11f, s * 0.03f, dark);
  }
  quad({c.x - w * 0.02f, c.y - s * 0.08f}, {c.x + w * 0.04f, c.y - s * 0.08f}, {c.x + w * 0.1f, c.y + s * 0.1f}, {c.x - w * 0.02f, c.y + s * 0.1f},
       shade(mid, 0.8f));  // nose shadow
  DrawLineEx({c.x - w * 0.16f, c.y + s * 0.2f}, {c.x + w * 0.16f, c.y + s * 0.2f}, std::max(1.5f, s * 0.03f), dark);
}

// A building of the village: wall, windows, a flat roof with a water tank or a rounded roof.
void konohaHouse(float x, float base, float w, float h, int seed, float fog, Color haze) {
  auto F = [&](Color c) { return mixColor(c, haze, fog); };
  const Color walls[4] = {rgb(238, 222, 196), rgb(226, 196, 170), rgb(240, 232, 214), rgb(214, 184, 160)};
  const Color roofs[4] = {rgb(200, 84, 56), rgb(220, 120, 60), rgb(90, 140, 140), rgb(170, 70, 60)};
  const Color wall = F(walls[seed % 4]), roof = F(roofs[(seed / 3) % 4]);
  gquad({x - w * 0.5f, base - h}, {x + w * 0.5f, base - h}, {x + w * 0.5f, base}, {x - w * 0.5f, base}, wall, shade(wall, 0.85f),
        shade(wall, 0.8f), wall);
  DrawRectangleRec({x - w * 0.5f, base - h * 0.55f, w, std::max(2.0f, h * 0.06f)}, F(rgb(170, 60, 50)));
  for (float wy = base - h + h * 0.18f; wy < base - h * 0.2f; wy += h * 0.3f)
    for (float wx = x - w * 0.36f; wx < x + w * 0.4f; wx += w * 0.26f)
      DrawRectangleRec({wx, wy, w * 0.12f, h * 0.12f}, F(rgb(70, 110, 140)));
  if (hash(seed * 7) > 0.5f) {  // rounded roof
    DrawCircleSector({x, base - h}, w * 0.52f, 180, 360, 16, roof);
    DrawLineEx({x - w * 0.52f, base - h}, {x + w * 0.52f, base - h}, 2.0f, shade(roof, 0.7f));
  } else {  // flat roof, a water tank on legs
    DrawRectangleRec({x - w * 0.54f, base - h - 4, w * 1.08f, 5}, roof);
    const float tx = x + w * (hash(seed) - 0.5f) * 0.5f, ts = w * 0.18f;
    DrawLineEx({tx - ts * 0.5f, base - h - 4}, {tx - ts * 0.5f, base - h - ts * 0.6f}, 1.5f, F(rgb(90, 80, 70)));
    DrawLineEx({tx + ts * 0.5f, base - h - 4}, {tx + ts * 0.5f, base - h - ts * 0.6f}, 1.5f, F(rgb(90, 80, 70)));
    DrawRectangleRec({tx - ts * 0.6f, base - h - ts * 1.5f, ts * 1.2f, ts * 0.9f}, F(rgb(150, 160, 170)));
    tri({tx - ts * 0.66f, base - h - ts * 1.5f}, {tx + ts * 0.66f, base - h - ts * 1.5f}, {tx, base - h - ts * 1.9f}, F(rgb(120, 130, 140)));
  }
}

// The Hokage's residence: a great red drum, its round roof and the 火 of the Land of Fire.
void hokageTower(float x, float base, float w, float h, float fog, Color haze) {
  auto F = [&](Color c) { return mixColor(c, haze, fog); };
  gquad({x - w * 0.5f, base - h}, {x + w * 0.5f, base - h}, {x + w * 0.5f, base}, {x - w * 0.5f, base}, F(rgb(226, 90, 64)),
        F(rgb(160, 50, 40)), F(rgb(150, 46, 38)), F(rgb(206, 80, 58)));
  for (float y = base - h * 0.75f; y < base - 6; y += h * 0.25f) DrawRectangleRec({x - w * 0.5f, y, w, 3}, F(rgb(120, 36, 30)));
  DrawCircleSector({x, base - h}, w * 0.56f, 180, 360, 24, F(rgb(196, 64, 48)));
  DrawRing({x, base - h}, w * 0.5f, w * 0.56f, 180, 360, 24, F(rgb(140, 40, 34)));
  const Vector2 c{x, base - h - w * 0.2f};  // 火
  const float s = w * 0.2f, t = std::max(2.0f, w * 0.035f);
  const Color white = F(rgb(250, 246, 236));
  DrawLineEx({c.x - s * 0.7f, c.y - s * 0.35f}, {c.x - s * 0.45f, c.y}, t, white);
  DrawLineEx({c.x + s * 0.7f, c.y - s * 0.4f}, {c.x + s * 0.45f, c.y - s * 0.05f}, t, white);
  DrawLineEx({c.x, c.y - s * 0.75f}, {c.x - s * 0.1f, c.y}, t, white);
  DrawLineEx({c.x - s * 0.1f, c.y}, {c.x - s * 0.8f, c.y + s * 0.65f}, t, white);
  DrawLineEx({c.x - s * 0.05f, c.y - s * 0.05f}, {c.x + s * 0.75f, c.y + s * 0.65f}, t, white);
}

void bakeKonoha(Art& a) {
  const Color haze = kKoHaze;
  a.sky = bakeOpaque(1360, 800, 1.0f, [&] {
    gradient(1360, 800, {{0.0f, rgb(52, 122, 214)}, {0.4f, rgb(106, 170, 232)}, {0.62f, rgb(186, 220, 240)}, {0.7f, haze}, {1.0f, haze}});
    const Vector2 sun{1180, 90};
    glow(sun, 520, 420, A(rgb(255, 250, 226), 0.35f), A(rgb(255, 250, 226), 0.0f));
    glow(sun, 120, rgb(255, 252, 236), 0.7f);
    DrawCircleV(sun, 28, rgb(255, 254, 246));
    const Vector2 cl[5] = {{200, 90}, {560, 160}, {880, 70}, {1100, 250}, {80, 300}};
    for (int i = 0; i < 5; ++i)
      cumulus(cl[i], 160 + hash(i * 5) * 160, 56 + hash(i * 3) * 50, i * 61, rgb(176, 204, 232), rgb(250, 252, 255), A(WHITE, 0.6f), 0.5f);
    range(1360, 560, 60, 8, rgb(150, 180, 196), rgb(196, 220, 232), 800, rgb(240, 248, 252), true);
    // The Hokage Rock: a sheer cliff with its four faces, forest on top.
    const float cx = 640, top = 250, base = 560;
    const Color rlit = rgb(214, 182, 146), rmid = rgb(176, 140, 108), rdark = rgb(110, 82, 64);
    constexpr int n = 24;
    for (int i = 0; i < n; ++i) {
      const float u0 = float(i) / n, u1 = float(i + 1) / n;
      auto yTop = [&](float u) { return top + 40.0f * std::pow(std::fabs(u - 0.5f) * 2.0f, 2.0f) + 8.0f * std::sin(u * 31.0f); };
      const float x0 = cx - 560 + 1120 * u0, x1 = cx - 560 + 1120 * u1;
      gquad({x0, yTop(u0)}, {x1, yTop(u1)}, {x1, base}, {x0, base}, mixColor(rlit, rmid, u0), mixColor(rlit, rmid, u1), rdark, rdark);
    }
    for (int i = 0; i < 26; ++i) {  // strata and cracks
      const float x = cx - 540 + i * 42 + hash(i * 3) * 20;
      DrawLineEx({x, top + 60 + hash(i) * 40}, {x + (hash(i * 5) - 0.5f) * 20, base}, 1.5f, A(rdark, 0.45f));
    }
    for (int k = 0; k < 4; ++k) hokageFace({cx - 300 + k * 200.0f, top + 140}, 150, k, rlit, rmid, rgb(80, 58, 46));
    for (int i = 0; i < 30; ++i) {  // the forest on top of the rock
      const float x = cx - 560 + i * 39 + hash(i * 7) * 20;
      softEllipse({x, top + 30.0f * std::pow(std::fabs(x - cx) / 560.0f, 2.0f) - 4}, 26 + 12 * hash(i), 16, rgb(70, 120, 66), 3.0f, 16);
      softEllipse({x - 6, top + 30.0f * std::pow(std::fabs(x - cx) / 560.0f, 2.0f) - 10}, 16, 8, rgb(120, 170, 90), 3.0f, 12);
    }
    vgrad(0, 470, 1360, 120, A(haze, 0.0f), A(haze, 0.75f));
    for (int i = 0; i < 24; ++i)  // the far roofs of the village
      konohaHouse(20 + i * 58 + hash(i * 3) * 20, 600, 44 + hash(i) * 20, 26 + hash(i * 5) * 20, i, 0.45f, haze);
    vgrad(0, 600, 1360, 200, A(haze, 0.5f), haze);
  });
  // The village: houses, the Hokage's residence, poles and wires.
  a.mid = bake(2000, 440, 1.25f, [&] {
    for (int row = 0; row < 2; ++row)
      for (int i = 0; i < 16 - row * 4; ++i) {
        const float x = 40 + i * (126.0f + row * 40.0f) + hash(row * 7 + i * 13) * 50.0f, s = 0.8f + 0.35f * row;
        if (row == 0 && std::fabs(x - 1040) < 120) continue;
        konohaHouse(x, 330 + row * 80.0f, (80 + 40 * hash(i * 3 + row)) * s, (70 + 50 * hash(i * 5 + row)) * s, 100 + row * 20 + i,
                    0.28f - 0.14f * row, haze);
      }
    hokageTower(1040, 340, 170, 150, 0.2f, haze);
    for (int i = 0; i < 6; ++i) {  // telephone poles and sagging wires
      const float x = 140 + i * 330.0f;
      DrawLineEx({x, 420}, {x, 250}, 4.0f, rgb(96, 80, 70));
      DrawLineEx({x - 16, 262}, {x + 16, 262}, 3.0f, rgb(96, 80, 70));
      if (i < 5)
        for (int k = 0; k < 2; ++k) {
          Vector2 prev{x + (k ? 14.0f : -14.0f), 262};
          for (int j = 1; j <= 10; ++j) {
            const float t = j / 10.0f;
            const Vector2 p{x + (k ? 14.0f : -14.0f) + 330 * t, 262 + std::sin(t * kPi) * 24};
            DrawLineEx(prev, p, 1.2f, rgb(60, 54, 50));
            prev = p;
          }
        }
    }
  });
  // The great trees of the forest around the village.
  a.near = bake(1600, 320, 1.25f, [&] {
    const float tx[3] = {80, 800, 1460}, ts[3] = {280, 200, 260};
    for (int k = 0; k < 3; ++k)
      wrapX(tx[k], 170, 1600, [&](float x) {
        leafyTree({x, 320}, ts[k], 500 + k * 7, rgb(84, 60, 44), rgb(36, 80, 40), rgb(60, 120, 52), rgb(150, 200, 90));
      });
    for (int i = 0; i < 24; ++i) {
      const float x = hash(i * 3) * 1600, r = 30 + 26 * hash(i * 5);
      wrapX(x, r, 1600, [&](float xx) {
        softEllipse({xx, 304}, r, r * 0.6f, rgb(40, 82, 42), 3.0f, 18);
        softEllipse({xx - r * 0.2f, 294}, r * 0.6f, r * 0.35f, rgb(90, 150, 66), 3.0f, 16);
      });
    }
  });
}

void bgKonoha(const Art& a, Vec2 pc, float skyX, float skyY, float time) {
  drawBackdrop(a.sky, skyX, skyY);
  drawLayer(a.mid, -360.0f - pc.x * 0.12f, 250.0f - pc.y * 0.12f, false);
  const float ny = 440.0f - pc.y * 0.22f;
  drawLayer(a.near, -pc.x * 0.22f, ny, true);
  if (ny + 319.0f < kScreenH) DrawRectangleRec({0, ny + 319.0f, float(kScreenW), kScreenH - ny - 319.0f}, kKoLow);
  motes(16, 117, pc, time, 6.0f, -4.0f, rgb(255, 252, 230), rgb(220, 255, 200), 1.1f);
  leaves(16, 31, pc, time, rgb(90, 170, 70), rgb(170, 210, 90));
}

// A rooftop of the village: parapet to stand on, a façade with windows and the leaf, lower floors fading.
void konohaSolid(const Rect& r, float time, int seed) {
  (void)time;
  (void)seed;
  const float L = r.left(), R = r.right(), T = r.top(), w = r.w, h = r.h, cx = L + w * 0.5f;
  const float fl = L + 16, fr = R - 16, fy = T + h, fh = 240;  // the lower floors
  gquad({fl, fy}, {fr, fy}, {fr, fy + fh}, {fl, fy + fh}, rgb(214, 196, 170), rgb(176, 156, 136), A(rgb(150, 130, 116), 0.0f),
        A(rgb(190, 170, 150), 0.0f));
  for (int row = 0; row < 3; ++row) {
    const float y = fy + 24 + row * 70.0f, a = 1.0f - row * 0.3f;
    for (float x = fl + 30; x < fr - 50; x += 90) {
      DrawRectangleRec({x - 3, y - 3, 46, 38}, A(rgb(150, 120, 100), a));
      gquad({x, y}, {x + 40, y}, {x + 40, y + 32}, {x, y + 32}, A(rgb(150, 200, 230), a), A(rgb(90, 140, 180), a), A(rgb(60, 100, 140), a),
            A(rgb(110, 160, 200), a));
      DrawLineEx({x + 6, y + 26}, {x + 20, y + 6}, 2.0f, A(WHITE, 0.4f * a));
    }
    DrawRectangleRec({fl, y + 46, fr - fl, 4}, A(rgb(190, 70, 56), a));
  }
  // The top floor: cream façade with a red band and the leaf of Konoha.
  gquad({L, T}, {R, T}, {R - 6, T + h}, {L + 6, T + h}, rgb(244, 232, 210), rgb(222, 206, 180), rgb(200, 184, 160), rgb(230, 216, 192));
  DrawRectangleRec({L + 4, T + h - 16, w - 8, 10}, rgb(196, 70, 54));
  DrawRectangleRec({L + 4, T + h - 16, w - 8, 3}, rgb(236, 120, 96));
  const Vector2 lc{cx, T + h * 0.42f};
  DrawCircleV(lc, h * 0.3f, rgb(196, 70, 54));
  leafMark(lc, h * 0.38f, rgb(250, 244, 230), 2.5f);
  // Parapet.
  DrawRectangleRounded({L - 6, T - 7, w + 12, 15}, 0.3f, 6, rgb(150, 120, 100));
  DrawRectangleRounded({L - 6, T - 8, w + 12, 10}, 0.3f, 6, rgb(226, 206, 178));
  DrawRectangleRec({L - 4, T - 8, w + 8, 3}, rgb(252, 242, 222));
}

// A great branch of the forest, bound with a plank where the ninjas train.
void konohaThin(const Platform& p, float time, int seed) {
  const Rect& r = p.rect;
  const float L = r.x, R = r.right(), T = r.y, w = r.w;
  Vector2 br[5];
  for (int k = 0; k < 5; ++k) br[k] = {lerpf(L - 40, R + 40, k / 4.0f), T + 12 + std::sin(k * 1.3f + seed) * 3.0f};
  stroke(br, 5, 18.0f, 10.0f, rgb(96, 66, 46));
  for (int k = 0; k < 4; ++k) DrawLineEx({br[k].x + 6, br[k].y - 3}, {br[k + 1].x - 6, br[k + 1].y - 2}, 1.5f, rgb(70, 48, 34));
  for (int k = 0; k < 7; ++k) {  // foliage around the ends and hanging under the branch
    const bool end = k < 4;
    const float u = end ? (k % 2 ? 1.0f : 0.0f) : 0.25f + 0.25f * (k - 4);
    const float x = lerpf(L - 50, R + 50, u) + (end ? (k < 2 ? -8.0f : 10.0f) : 0.0f);
    const float y = T + (end ? (k < 2 ? 2.0f : 22.0f) : 30.0f), rr = end ? 30.0f : 18.0f;
    const float sw = std::sin(time * 1.5f + k + seed) * 2.0f;
    softEllipse({x + sw, y + 4}, rr * 1.15f, rr * 0.75f, rgb(40, 86, 42), 3.0f, 18);
    softEllipse({x - rr * 0.25f + sw, y - rr * 0.1f}, rr * 0.75f, rr * 0.45f, rgb(78, 140, 62), 3.0f, 16);
    softEllipse({x - rr * 0.4f + sw, y - rr * 0.28f}, rr * 0.35f, rr * 0.2f, rgb(150, 200, 96), 2.0f, 12);
  }
  DrawRectangleRounded({L + 6, T - 3, w - 12, 9}, 0.3f, 6, rgb(170, 130, 86));  // the plank
  DrawRectangleRec({L + 8, T - 3, w - 16, 3}, rgb(214, 176, 120));
  for (int s = 0; s < 2; ++s) DrawRectangleRec({s ? R - 34 : L + 26, T - 4, 8, 14}, rgb(150, 40, 40));  // bindings
}

// ==== Naruto: the Valley of the End, in the rain ===========================================================
constexpr Color kVeLow{40, 56, 62, 255};
constexpr Color kVeStone{140, 148, 142, 255};

void bakeValleyOfTheEnd(Art& a) {
  const Color haze = rgb(150, 164, 170);
  a.sky = bakeOpaque(1360, 800, 1.0f, [&] {
    gradient(1360, 800, {{0.0f, rgb(58, 70, 86)}, {0.36f, rgb(98, 114, 128)}, {0.62f, rgb(140, 154, 164)}, {0.72f, haze}, {1.0f, rgb(110, 128, 136)}});
    for (int i = 0; i < 7; ++i)  // low storm clouds
      stratus({hash(i * 3) * 1360, 70 + i * 50 + hash(i * 5) * 30}, 420 + hash(i) * 300, 30 + hash(i * 7) * 20, i * 37, rgb(70, 82, 96),
              rgb(150, 162, 172));
    range(1360, 520, 90, 12, rgb(96, 112, 116), rgb(140, 154, 160), 800, rgb(176, 188, 194), false);
    // The walls of the valley closing in on both sides, forest on their tops.
    for (int side = 0; side < 2; ++side) {
      const float x0 = side ? 1360 : 0, dir = side ? -1.0f : 1.0f;
      for (int k = 0; k < 3; ++k) {
        const float reach = 300 + k * 110, topY = 200 + k * 60, fog = 0.55f - k * 0.18f;
        const Color c = mixColor(rgb(70, 84, 80), haze, fog), lit = mixColor(rgb(110, 124, 116), haze, fog);
        gquad({x0, topY - 30}, {x0 + dir * reach, topY + 40}, {x0 + dir * (reach + 40), 800}, {x0, 800}, side ? c : lit, side ? c : lit, c, c);
        for (int i = 0; i < 8; ++i)
          softEllipse({x0 + dir * reach * (i / 8.0f), topY - 30 + 70 * (i / 8.0f) - 6}, 30 - k * 4.0f, 14,
                      mixColor(rgb(46, 76, 52), haze, fog), 3.0f, 14);
        for (int i = 0; i < 6; ++i)
          DrawLineEx({x0 + dir * reach * (0.2f + i * 0.14f), topY + 40}, {x0 + dir * reach * (0.25f + i * 0.14f), 760}, 1.5f,
                     A(mixColor(rgb(50, 60, 60), haze, fog), 0.5f));
      }
    }
  });
  // The river below, foaming where the fall comes down.
  a.near = bake(1600, 280, 1.25f, [&] {
    vgrad(0, 90, 1600, 190, rgb(92, 120, 130), kVeLow);
    for (int i = 0; i < 60; ++i) {
      const float x = hash(i * 3) * 1600, y = 100 + hash(i * 5) * 170, len = 30 + 50 * hash(i * 7);
      wrapX(x, len, 1600, [&](float xx) { DrawLineEx({xx - len * 0.5f, y}, {xx + len * 0.5f, y}, 1.5f, A(rgb(200, 220, 226), 0.35f)); });
    }
    for (int i = 0; i < 9; ++i) {  // rocks in the stream
      const float x = 80 + i * 180 + hash(i * 11) * 60, y = 110 + hash(i * 13) * 120, s = 20 + 24 * hash(i);
      wrapX(x, s, 1600, [&](float xx) {
        ellipse({xx, y + s * 0.3f}, s * 1.3f, s * 0.35f, A(WHITE, 0.25f));
        ellipse({xx, y}, s, s * 0.55f, rgb(70, 80, 80));
        ellipse({xx - s * 0.2f, y - s * 0.18f}, s * 0.6f, s * 0.25f, rgb(110, 120, 118));
      });
    }
  });
}

void bgValleyOfTheEnd(const Art& a, Vec2 pc, float skyX, float skyY, float time) {
  drawBackdrop(a.sky, skyX, skyY);
  const float ny = 470.0f - pc.y * 0.25f;
  drawLayer(a.near, -pc.x * 0.25f - time * 14.0f, ny, true);
  if (ny + 279.0f < kScreenH) DrawRectangleRec({0, ny + 279.0f, float(kScreenW), kScreenH - ny - 279.0f}, kVeLow);
  for (int k = 0; k < 4; ++k) {  // drifting curtains of mist
    const float x = wrapf(time * (10.0f + 4.0f * k) + k * 400.0f - pc.x * 0.2f, kScreenW + 800.0f) - 400.0f;
    glow({x, ny - 20.0f + k * 30.0f}, 420, 70, A(rgb(220, 230, 236), 0.12f), A(rgb(220, 230, 236), 0.0f));
  }
}

// The waterfall pouring between the statues, in the world (behind the heads): x0..x1 at the lip y.
void valleyFall(float x0, float x1, float y, float time) {
  const float len = 900.0f;
  gquad({x0, y}, {x1, y}, {x1 + 20, y + len}, {x0 - 20, y + len}, rgb(196, 214, 222), rgb(176, 198, 210), A(rgb(160, 186, 200), 0.0f),
        A(rgb(180, 200, 212), 0.0f));
  for (int k = 0; k < 24; ++k) {  // streaks running down
    const float u = hash(k * 7), t = wrapf(time * (0.9f + 0.4f * hash(k * 3)) + hash(k * 5), 1.0f);
    const float x = lerpf(x0, x1, u), yy = y + t * len * 0.8f;
    DrawLineEx({x, yy}, {x, yy + 60 + 60 * hash(k)}, 2.0f + 2.0f * hash(k * 9), A(WHITE, 0.55f * (1.0f - t)));
  }
  DrawRectangleRec({x0 - 6, y - 6, x1 - x0 + 12, 10}, rgb(80, 90, 88));  // the lip of the cliff
  DrawRectangleRec({x0 - 6, y - 6, x1 - x0 + 12, 3}, rgb(130, 140, 136));
  for (int k = 0; k < 6; ++k) {
    const float p = 0.5f + 0.5f * std::sin(time * 1.3f + k * 1.7f);
    glow({lerpf(x0, x1, k / 5.0f), y + 520.0f + 30.0f * (k % 2)}, 90.0f + 20.0f * p, WHITE, 0.16f, 20);
  }
}

// A statue: the platform is the top of its head. Hashirama (left, long hair parted in the middle) and Madara
// (right, a mane of spikes), facing each other across the fall.
void valleySolid(const Rect& r, float time, int seed) {
  (void)seed;
  const float L = r.left(), R = r.right(), T = r.top(), w = r.w, h = r.h, cx = L + w * 0.5f;
  const bool left = cx < 0.0f, madara = !left;
  const float face = left ? 1.0f : -1.0f;  // toward the centre
  if (left) valleyFall(R + 2, 0.0f, T + 40, time);
  else valleyFall(0.0f, L - 2, T + 40, time + 3.0f);
  const Color lit = kVeStone, mid = rgb(104, 112, 108), dark = rgb(58, 66, 66), moss = rgb(84, 104, 72);
  const Color litSide = face > 0 ? mid : lit, farSide = face > 0 ? lit : mid;  // the light comes from the valley's axis
  // Body: armoured shoulders and chest going down into the mist; hands joined in a seal.
  const float sw = w * 1.3f, sy = T + h + 230;
  gquad({cx - sw * 0.5f, sy + 20}, {cx + sw * 0.5f, sy + 20}, {cx + sw * 0.42f, T + 900}, {cx - sw * 0.42f, T + 900}, mid, dark,
        A(dark, 0.0f), A(mid, 0.0f));
  for (int k = 0; k < 4; ++k) DrawLineEx({cx - sw * 0.32f, sy + 110 + k * 44.0f}, {cx + sw * 0.32f, sy + 110 + k * 44.0f}, 3.0f, A(dark, 0.7f));
  for (int sd = -1; sd <= 1; sd += 2) {  // shoulder plates
    const float px = cx + sd * sw * 0.34f;
    DrawRectangleRounded({px - sw * 0.19f, sy - 6, sw * 0.38f, 92}, 0.4f, 8, sd * face < 0 ? lit : mid);
    for (int k = 1; k <= 2; ++k) DrawLineEx({px - sw * 0.18f, sy - 6 + k * 30.0f}, {px + sw * 0.18f, sy - 6 + k * 30.0f}, 3.0f, dark);
  }
  const Vector2 hc{cx + face * w * 0.05f, sy + 190};  // the seal: two hands, two fingers raised
  DrawRectangleRounded({hc.x - 34, hc.y - 46, 68, 66}, 0.4f, 8, lit);
  DrawRectangleRounded({hc.x - 9, hc.y - 86, 18, 46}, 0.5f, 6, lit);
  DrawLineEx({hc.x - 34, hc.y - 16}, {hc.x + 34, hc.y - 16}, 2.0f, dark);
  DrawLineEx({hc.x, hc.y - 86}, {hc.x, hc.y - 46}, 2.0f, dark);
  // Hair: a mass falling behind the face down to the shoulders.
  const float hairY = T + h + 260;
  if (madara) {
    quad({L - 4, T + 6}, {R + 4, T + 6}, {R + 20, hairY}, {L - 20, hairY}, mid);
    const float out = face > 0 ? L - 4 : R + 4;
    for (int k = 0; k < 9; ++k) {  // a mane of spikes on the outer side
      const float y0 = T + 10 + k * 34.0f;
      tri({out + face * 26, y0}, {out + face * 26, y0 + 60}, {out - face * (46 + 26 * hash(k)), y0 + 40}, k % 2 ? mid : dark);
    }
  } else {
    quad({L - 4, T + 6}, {R + 4, T + 6}, {R + 24, hairY + 20}, {L - 24, hairY + 20}, mid);
    for (int k = 1; k < 7; ++k)
      DrawLineEx({L + k * w / 7.0f, T + h}, {L - 24 + k * (w + 48) / 7.0f, hairY + 16}, 2.0f, A(dark, 0.55f));
  }
  // The face, just under the brow, turned a little toward the fall.
  const float fx = cx + face * w * 0.06f, fw = w * 0.66f, fy = T + h - 4;
  gquad({fx - fw * 0.5f, fy}, {fx + fw * 0.5f, fy}, {fx + fw * 0.44f, fy + 150}, {fx - fw * 0.44f, fy + 150}, litSide, farSide,
        shade(farSide, 0.85f), shade(litSide, 0.85f));
  tri({fx - fw * 0.44f, fy + 150}, {fx + fw * 0.44f, fy + 150}, {fx + face * fw * 0.06f, fy + 196}, shade(mid, 0.85f));  // chin
  for (int e = -1; e <= 1; e += 2) {
    const float ex = fx + e * fw * 0.22f + face * fw * 0.04f;
    DrawLineEx({ex - fw * 0.13f, fy + 30 - e * face * 3.0f}, {ex + fw * 0.13f, fy + 28 + e * face * 3.0f}, 5.0f, dark);  // brows
    ellipse({ex, fy + 46}, fw * 0.1f, 5.0f, dark);  // eyes closed in meditation
    DrawLineEx({ex - fw * 0.08f, fy + 52}, {ex + fw * 0.08f, fy + 52}, 2.0f, A(dark, 0.5f));
  }
  quad({fx + face * fw * 0.02f, fy + 48}, {fx + face * fw * 0.1f, fy + 48}, {fx + face * fw * 0.17f, fy + 96}, {fx, fy + 96}, shade(mid, 0.72f));
  DrawLineEx({fx - fw * 0.15f + face * 6, fy + 122}, {fx + fw * 0.15f + face * 6, fy + 122}, 4.0f, dark);  // mouth
  if (madara) {  // his bangs over one eye
    tri({fx - face * fw * 0.52f, fy - 2}, {fx + face * fw * 0.08f, fy - 2}, {fx - face * fw * 0.22f, fy + 78}, mid);
    tri({fx - face * fw * 0.52f, fy - 2}, {fx - face * fw * 0.2f, fy - 2}, {fx - face * fw * 0.5f, fy + 110}, dark);
  } else {  // long hair framing the face on both sides
    tri({fx - fw * 0.56f, fy - 2}, {fx - fw * 0.36f, fy - 2}, {fx - fw * 0.5f, fy + 150}, mid);
    tri({fx + fw * 0.56f, fy - 2}, {fx + fw * 0.36f, fy - 2}, {fx + fw * 0.5f, fy + 150}, mid);
  }
  for (int k = 0; k < 10; ++k) {  // rain streaking down the stone
    const float x = L + hash(k * 7 + int(cx)) * w;
    DrawLineEx({x, fy + 10}, {x, fy + 50 + 120 * hash(k * 3)}, 2.0f, A(dark, 0.3f));
  }
  // The crown of the head (the walking surface): hair combed back, the forehead protector at its front.
  gquad({L, T}, {R, T}, {R + 4, T + h}, {L - 4, T + h}, lit, lit, mid, mid);
  if (madara) {
    for (int k = 0; k < 9; ++k) {  // wild locks
      const float x = L + (k + 0.5f) * w / 9.0f;
      tri({x - 16, T + 8}, {x + 16, T + 8}, {x + face * 10, T + h - 22}, k % 2 ? mid : shade(mid, 0.85f));
    }
  } else {
    DrawLineEx({cx, T + 4}, {cx, T + h - 22}, 3.0f, dark);  // the parting
    for (int k = 1; k < 6; ++k) {
      DrawLineEx({cx - k * w * 0.08f, T + 6}, {cx - k * w * 0.1f, T + h - 22}, 2.0f, A(dark, 0.5f));
      DrawLineEx({cx + k * w * 0.08f, T + 6}, {cx + k * w * 0.1f, T + h - 22}, 2.0f, A(dark, 0.5f));
    }
  }
  DrawRectangleRec({fx - fw * 0.56f, T + h - 24, fw * 1.12f, 22}, dark);  // forehead protector
  DrawRectangleRounded({fx - fw * 0.17f, T + h - 22, fw * 0.34f, 18}, 0.3f, 6, mid);
  DrawCircleV({fx - fw * 0.1f, T + h - 13}, 2.5f, dark);
  DrawCircleV({fx + fw * 0.1f, T + h - 13}, 2.5f, dark);
  for (int k = 0; k < int(w / 60.0f); ++k) ellipse({L + 20 + hash(k * 5 + int(cx)) * (w - 40), T + 4}, 20, 5, moss);
  DrawRectangleRounded({L - 4, T - 6, w + 8, 14}, 0.5f, 6, rgb(92, 100, 96));
  DrawRectangleRounded({L - 4, T - 7, w + 8, 10}, 0.5f, 6, rgb(156, 164, 158));
  DrawRectangleRounded({L - 2, T - 7, w + 4, 3}, 1.0f, 6, rgb(200, 208, 204));
}

// A rock standing in the middle of the fall.
void valleyThin(const Platform& p, float time, int seed) {
  (void)seed;
  const Rect& r = p.rect;
  const float L = r.x, R = r.right(), T = r.y, w = r.w, cx = L + w * 0.5f;
  quad({L + 4, T + 4}, {R - 4, T + 4}, {cx + w * 0.2f, T + 70}, {cx - w * 0.25f, T + 60}, rgb(84, 92, 90));
  quad({L + 4, T + 4}, {cx, T + 4}, {cx - w * 0.05f, T + 66}, {cx - w * 0.25f, T + 60}, rgb(116, 124, 120));
  for (int k = 0; k < 4; ++k) {  // water splashing off it
    const float t = wrapf(time * 1.4f + k * 0.25f, 1.0f);
    dot({L + 10 + k * (w - 20) / 3.0f, T + 6 + t * 40.0f}, 2.5f * (1.0f - t), A(WHITE, 0.7f * (1.0f - t)));
  }
  DrawRectangleRounded({L, T - 4, w, 12}, 0.5f, 6, rgb(84, 104, 72));
  DrawRectangleRounded({L, T - 5, w, 8}, 0.5f, 6, rgb(120, 146, 92));
  DrawRectangleRounded({L + 3, T - 5, w - 6, 3}, 1.0f, 6, rgb(170, 196, 130));
}

// Rain over the whole scene, slanting with the wind.
void valleyForeground(Vec2 cam, float time) {
  for (int i = 0; i < 70; ++i) {
    const float sp = 900.0f + 300.0f * hash(i * 3);
    const float x = wrapf(hash(i * 7) * 1400.0f - time * sp * 0.25f - cam.x * 0.6f, 1400.0f) - 60.0f;
    const float y = wrapf(hash(i * 11) * 800.0f + time * sp - cam.y * 0.6f, 800.0f) - 40.0f;
    DrawLineEx({x, y}, {x - 8.0f, y + 26.0f}, 1.5f, A(rgb(220, 232, 240), 0.28f + 0.2f * hash(i * 13)));
  }
}

// ---- The worlds -------------------------------------------------------------------------------------
struct World {
  void (*bake)(Art&);
  void (*background)(const Art&, Vec2 cam, float skyX, float skyY, float time);
  void (*solid)(const Rect&, float time, int seed);
  void (*thin)(const Platform&, float time, int seed);
  void (*foreground)(Vec2 cam, float time);  // optional
};
const World* world(StageTheme theme) {
  static const World kLasNoches{bakeLasNoches, bgLasNoches, lasNochesSolid, lasNochesThin, nullptr};
  static const World kSeireitei{bakeSeireitei, bgSeireitei, seireiteiSolid, seireiteiThin, nullptr};
  static const World kBlackBulls{bakeBlackBulls, bgBlackBulls, blackBullsSolid, blackBullsThin, nullptr};
  static const World kGrimoire{bakeGrimoireTower, bgGrimoireTower, grimoireSolid, grimoireThin, nullptr};
  static const World kShibuya{bakeShibuya, bgShibuya, shibuyaSolid, shibuyaThin, nullptr};
  static const World kJujutsuHigh{bakeJujutsuHigh, bgJujutsuHigh, jujutsuSolid, jujutsuThin, nullptr};
  static const World kKonoha{bakeKonoha, bgKonoha, konohaSolid, konohaThin, nullptr};
  static const World kValley{bakeValleyOfTheEnd, bgValleyOfTheEnd, valleySolid, valleyThin, valleyForeground};
  switch (theme) {
    case StageTheme::LasNoches: return &kLasNoches;
    case StageTheme::Seireitei: return &kSeireitei;
    case StageTheme::BlackBulls: return &kBlackBulls;
    case StageTheme::GrimoireTower: return &kGrimoire;
    case StageTheme::Shibuya: return &kShibuya;
    case StageTheme::JujutsuHigh: return &kJujutsuHigh;
    case StageTheme::Konoha: return &kKonoha;
    case StageTheme::ValleyOfTheEnd: return &kValley;
    default: return nullptr;
  }
}
}  // namespace

namespace stageart {
void bake(StageTheme theme, Art& a) {
  if (const World* w = world(theme)) w->bake(a);
}

void drawBackground(StageTheme theme, const Art& a, Vec2 cam, float skyX, float skyY, float time) {
  if (const World* w = world(theme)) w->background(a, cam, skyX, skyY, time);
}

bool drawPlatform(const Platform& p, StageTheme theme, float time, int seed) {
  const World* w = world(theme);
  if (!w) return false;
  if (p.solid) w->solid(p.rect, time, seed);
  else w->thin(p, time, seed);
  return true;
}

void drawForeground(StageTheme theme, Vec2 cam, float time) {
  if (const World* w = world(theme); w && w->foreground) w->foreground(cam, time);
}
}  // namespace stageart
