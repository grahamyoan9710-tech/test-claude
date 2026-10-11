// Weapons: the fighters' own blades and the weapons that drop during a match.
// Every weapon is drawn in a frame tied to its grip: `a` runs along the weapon (grip -> tip),
// `b` across it (+ = cutting edge side). Two passes, like the bodies: warm umber outlines, then fills
// with a bevel / highlight; energy (glows, petals, lightning, chains of fire) goes on top.
#include "game/pixel.hpp"
#include "game/render.hpp"

namespace {
const Color kInk = {56, 35, 28, 255};
constexpr int kMaxPts = 12;

void triW(Vector2 a, Vector2 b, Vector2 c, Color k) {
  float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
  if (cross > 0) std::swap(b, c);
  DrawTriangle(a, b, c, k);
}
int segsFor(float r) { return std::clamp(int(r * 1.2f) + 6, 8, 28); }
void discW(Vec2 c, float r, Color k) {
  if (r > 0.25f) DrawCircleSector(V(c), r, 0, 360, segsFor(r), k);
}
void fan(const Vec2* q, int n, Color k) {
  for (int i = 1; i + 1 < n; ++i) triW(V(q[0]), V(q[i]), V(q[i + 1]), k);
}
// Outward mitred offset of a convex polygon.
void grow(const Vec2* q, int n, float d, Vec2* out) {
  float area = 0.0f;
  for (int i = 0; i < n; ++i) area += q[i].x * q[(i + 1) % n].y - q[(i + 1) % n].x * q[i].y;
  const float sg = area > 0.0f ? 1.0f : -1.0f;
  for (int i = 0; i < n; ++i) {
    Vec2 e0 = (q[i] - q[(i + n - 1) % n]).norm(), e1 = (q[(i + 1) % n] - q[i]).norm();
    Vec2 n0 = Vec2{e0.y, -e0.x} * sg, n1 = Vec2{e1.y, -e1.x} * sg;
    Vec2 m = (n0 + n1).norm();
    if (m.len2() < 0.5f) m = n0.len2() > 0.5f ? n0 : n1;
    float k = std::max(0.45f, dot(m, n0.len2() > 0.5f ? n0 : n1));
    out[i] = q[i] + m * (d / k);
  }
}
// Flower-like petal (diamond) at c.
void petal(Vec2 c, float r, float ang, Color k) {
  Vec2 d{std::cos(ang), std::sin(ang)}, n{-d.y, d.x};
  Vector2 a = V(c + d * (r * 1.3f)), b = V(c + n * (r * 0.6f)), e = V(c - d * (r * 0.9f)), f = V(c - n * (r * 0.6f));
  triW(a, b, e, k);
  triW(a, e, f, k);
}
float hash(float x) { return x - std::floor(x); }

// Weapon painter: coordinates in weapon units (a along, b across), scaled.
struct WP {
  Vec2 o, u, v;
  float s = 1.0f, alpha = 1.0f, ow = 2.0f, time = 0.0f;
  bool line = false, ghost = false;
  Color tint = WHITE;
  Color ink = kInk;  // outline colour (the near-black sprite ink in pixel art)

  Vec2 at(float x, float y) const { return o + u * (x * s) + v * (y * s); }
  Vec2 at(Vec2 p) const { return at(p.x, p.y); }
  Color col(Color k) const {
    if (ghost) k = {tint.r, tint.g, tint.b, k.a};
    return withAlpha(k, alpha * k.a / 255.0f);
  }
  bool details() const { return !line && !ghost; }
  void polyW(const Vec2* q, int n, Color k) const {
    if (n < 3) return;
    if (line) {
      if (ghost) return;
      Vec2 g[kMaxPts];
      grow(q, n, ow, g);
      fan(g, n, col(ink));
    } else {
      fan(q, n, col(k));
    }
  }
  void poly(std::initializer_list<Vec2> pts, Color k) const {
    Vec2 q[kMaxPts];
    int n = 0;
    for (Vec2 p : pts)
      if (n < kMaxPts) q[n++] = at(p);
    polyW(q, n, k);
  }
  // Painted on (fill pass only, no outline).
  void paint(std::initializer_list<Vec2> pts, Color k) const {
    if (!details()) return;
    Vec2 q[kMaxPts];
    int n = 0;
    for (Vec2 p : pts)
      if (n < kMaxPts) q[n++] = at(p);
    fan(q, n, col(k));
  }
  void seg(Vec2 a, Vec2 b, float w, Color k) const {
    if (line && ghost) return;
    const Vec2 A = at(a), B = at(b);
    const float th = w * s + (line ? 2.0f * ow : 0.0f);
    const Color c = line ? col(ink) : col(k);
    DrawLineEx(V(A), V(B), th, c);
    discW(A, th * 0.5f, c);
    discW(B, th * 0.5f, c);
  }
  void dot(Vec2 p, float r, Color k) const {
    if (line && ghost) return;
    discW(at(p), r * s + (line ? ow : 0.0f), line ? col(ink) : col(k));
  }
  void stroke(Vec2 a, Vec2 b, float w, Color k) const {
    if (details()) DrawLineEx(V(at(a)), V(at(b)), std::max(0.7f, w * s), col(k));
  }
  void spot(Vec2 p, float r, Color k) const {
    if (details()) discW(at(p), r * s, col(k));
  }
  // Soft energy halo (fill pass).
  void halo(Vec2 a, Vec2 b, float w, Color k, float strength) const {
    if (!details()) return;
    DrawLineEx(V(at(a)), V(at(b)), w * s, col(withAlpha(k, 0.16f * strength)));
    DrawLineEx(V(at(a)), V(at(b)), w * 0.5f * s, col(withAlpha(k, 0.22f * strength)));
  }
};

// Handle from a0 to a1 with a wrapping (diagonal stripes, or diamonds for katanas).
void handle(const WP& p, float a0, float a1, float w, Color base, Color wrap, float step, bool diamonds = false) {
  p.seg({a0, 0}, {a1, 0}, w, base);
  if (!p.details()) return;
  const float h = w * 0.42f;
  for (float a = a0 + step * 0.5f; a < a1 - step * 0.3f; a += step) {
    p.stroke({a - step * 0.35f, -h}, {a + step * 0.35f, h}, 1.0f, wrap);
    if (diamonds) p.stroke({a - step * 0.35f, h}, {a + step * 0.35f, -h}, 1.0f, wrap);
  }
}

// ---- Black Clover ------------------------------------------------------------------------------
void astaSword(const WP& p) {
  const Color blade{40, 38, 46, 255}, bevel{108, 104, 120, 255}, base{26, 24, 30, 255};
  const float fl = std::sin(p.time * 7.0f) * 1.8f;
  p.poly({{-11, -1.5f}, {-9, 2.2f}, {-16, 12.5f + fl}, {-19.5f, 9.5f + fl}}, {214, 192, 144, 255});  // grimoire scrap
  p.dot({-13, 0}, 3.4f, {76, 72, 82, 255});
  handle(p, -12, 7, 5.8f, {60, 44, 46, 255}, {200, 176, 130, 255}, 4.0f);
  p.poly({{6, -8}, {12.5f, -8}, {12.5f, 10.5f}, {6, 10.5f}}, base);
  p.poly({{12, -6.6f}, {42, -6.7f}, {40, 9.5f}, {12, 9.7f}}, blade);
  p.poly({{40, -6.7f}, {66, -6.5f}, {64, 9.0f}, {43.5f, 9.2f}}, blade);
  p.poly({{64, -6.5f}, {84, -6.2f}, {84.5f, -1.0f}, {80, 4.0f}, {76, 8.5f}, {66, 8.8f}}, blade);
  if (p.details()) {
    p.stroke({13, 7.9f}, {39.4f, 7.9f}, 2.0f, bevel);
    p.stroke({44.6f, 7.6f}, {63.4f, 7.4f}, 2.0f, bevel);
    p.stroke({66.6f, 7.2f}, {75.6f, 7.0f}, 2.0f, bevel);
    p.stroke({14, -3.0f}, {82, -2.8f}, 1.1f, {88, 84, 100, 255});
    p.spot({-16.4f, 8.6f + fl * 0.85f}, 1.5f, {64, 112, 60, 255});  // clover on the page
    p.spot({9.2f, 1.2f}, 1.5f, {120, 116, 130, 255});
  }
}

void demonSlayer(const WP& p) {
  const Color blade{34, 32, 40, 255}, bevel{94, 90, 106, 255};
  p.dot({-18, 0}, 4.3f, {66, 62, 72, 255});
  handle(p, -17, 8, 6.8f, {46, 34, 38, 255}, {156, 132, 108, 255}, 4.4f);
  p.poly({{8, -12}, {15, -12}, {15, 14}, {8, 14}}, {28, 26, 32, 255});
  p.poly({{15, -10}, {95, -10}, {100, -6}, {100, 8}, {94, 13}, {15, 13}}, blade);
  if (p.details()) {
    p.stroke({17, 10.8f}, {92, 10.8f}, 3.0f, bevel);
    p.stroke({18, 1.5f}, {90, 1.5f}, 2.8f, {22, 20, 26, 255});
    p.stroke({18, -6.5f}, {94, -6.5f}, 1.0f, {70, 66, 80, 255});
    for (float a : {34.0f, 57.0f, 78.0f}) p.paint({{a - 3.2f, 13.3f}, {a, 8.6f}, {a + 3.2f, 13.3f}}, kInk);
    p.spot({11.5f, -7}, 1.5f, {146, 140, 156, 255});
    p.spot({11.5f, 9}, 1.5f, {146, 140, 156, 255});
  }
}

void demonDweller(const WP& p) {
  const Color blade{30, 28, 34, 255}, edge{210, 210, 226, 255};
  p.dot({-15, 0}, 2.6f, {46, 42, 50, 255});
  handle(p, -14, 4, 4.4f, {30, 28, 34, 255}, {226, 226, 232, 255}, 3.6f, true);
  p.dot({5.5f, 0}, 4.6f, {70, 64, 76, 255});
  p.poly({{7, -1.9f}, {44, -2.5f}, {44, 2.5f}, {7, 2.7f}}, blade);
  p.poly({{44, -2.5f}, {77, -4.8f}, {80, -4.3f}, {72, 0.3f}, {44, 2.5f}}, blade);
  if (p.details()) {
    p.stroke({8, 2.0f}, {44, 1.9f}, 1.0f, edge);
    p.stroke({44, 1.9f}, {72, -0.1f}, 1.0f, edge);
    p.spot({5.5f, 0}, 1.6f, {150, 40, 50, 255});
  }
}

// Yuno's four-leaf grimoire, centred on a = 15 (open: pages spread, the clover glowing).
void grimoire(const WP& p, float open) {
  const Color cover{48, 106, 68, 255}, gold{218, 176, 74, 255}, page{246, 238, 210, 255};
  const Vec2 c{15, 0};
  if (p.details()) discW(p.at(c), 19.0f * p.s * (1.0f + 0.3f * open), p.col(withAlpha({150, 255, 190, 255}, 0.2f + 0.15f * open)));
  if (open < 0.5f) {
    p.poly({{7, -10.5f}, {23, -10.5f}, {23, 10.5f}, {7, 10.5f}}, cover);
    if (p.details()) {
      p.paint({{7, -10.5f}, {10, -10.5f}, {10, 10.5f}, {7, 10.5f}}, gold);
      p.paint({{21.6f, -9.4f}, {23, -9.4f}, {23, 9.4f}, {21.6f, 9.4f}}, page);
      for (int i = 0; i < 4; ++i) {
        float a = i * kPi * 0.5f + kPi * 0.25f;
        p.spot({16.0f + std::cos(a) * 2.7f, std::sin(a) * 2.7f}, 2.5f, gold);
      }
      p.spot({16, 0}, 1.2f, {255, 246, 200, 255});
      for (Vec2 k : {Vec2{8.5f, -9}, Vec2{21.5f, -9}, Vec2{8.5f, 9}, Vec2{21.5f, 9}}) p.spot(k, 1.1f, gold);
    }
  } else {  // open: two pages in a shallow V, the clover floating above
    p.poly({{4, -12}, {26, -12}, {27, 12}, {3, 12}}, cover);
    if (p.details()) {
      p.paint({{5.5f, -10.5f}, {14.6f, -9.5f}, {14.6f, 10.5f}, {5, 10.5f}}, page);
      p.paint({{15.4f, -9.5f}, {24.5f, -10.5f}, {25, 10.5f}, {15.4f, 10.5f}}, page);
      for (int i = 0; i < 4; ++i) {
        float y = -6.0f + i * 3.6f;
        p.stroke({7, y}, {13.5f, y + 0.3f}, 0.7f, {150, 140, 120, 255});
        p.stroke({16.5f, y + 0.3f}, {23, y}, 0.7f, {150, 140, 120, 255});
      }
      const float bob = std::sin(p.time * 3.0f) * 1.5f;
      for (int i = 0; i < 4; ++i) {
        float a = i * kPi * 0.5f + kPi * 0.25f + p.time * 1.5f;
        p.spot({15.0f + std::cos(a) * 3.2f - 9.0f + bob, std::sin(a) * 3.2f}, 3.0f, {120, 236, 150, 255});
      }
      p.spot({6.0f + bob, 0}, 1.6f, {255, 250, 210, 255});
    }
  }
}

void windSpear(const WP& p) {
  const Color shaft{206, 246, 222, 255}, blade{108, 220, 158, 255};
  p.halo({-14, 0}, {94, 0}, 10.0f, {140, 255, 190, 255}, 1.0f);
  p.seg({-18, 0}, {77, 0}, 3.2f, shaft);
  p.poly({{68, -1.4f}, {74, -6.5f}, {77, -1.4f}}, blade);
  p.poly({{68, 1.4f}, {74, 6.5f}, {77, 1.4f}}, blade);
  p.poly({{75, 0}, {84, -5}, {98, 0}, {84, 5}}, blade);
  if (p.details()) {
    p.stroke({-16, 0}, {76, 0}, 1.1f, WHITE);
    p.stroke({79, 0}, {95, 0}, 1.0f, {236, 255, 244, 255});
    for (float a : {-8.0f, 0.0f}) p.stroke({a, -2.4f}, {a, 2.4f}, 2.0f, {70, 170, 120, 255});
    for (int k = 0; k < 2; ++k) {  // wind spiralling up the shaft
      Vec2 prev{};
      for (int i = 0; i <= 8; ++i) {
        float a = 6.0f + i * 8.0f;
        Vec2 q{a, std::sin(a * 0.16f - p.time * 11.0f + k * kPi) * (3.0f + 2.5f * (i / 8.0f))};
        if (i > 0) p.stroke(prev, q, 1.1f, {196, 255, 224, 170});
        prev = q;
      }
    }
  }
}

// ---- Bleach ---------------------------------------------------------------------------------------
void zangetsu(const WP& p) {
  const Color steel{200, 206, 216, 255}, spine{56, 58, 66, 255}, cloth{238, 236, 228, 255};
  Vec2 q[5] = {{-15, 0}};
  for (int k = 1; k < 5; ++k) q[k] = q[k - 1] + Vec2{-6.0f, 1.6f + std::sin(p.time * 8.0f - k * 1.1f) * 2.2f};
  for (int k = 0; k < 4; ++k) p.seg(q[k], q[k + 1], 3.4f - k * 0.45f, cloth);
  handle(p, -15, 5, 5.2f, {44, 40, 46, 255}, cloth, 3.4f, true);
  p.poly({{4, -6}, {88, -6}, {92, -3}, {83, 14}, {9, 14}, {4, 10}}, steel);
  if (p.details()) {
    p.paint({{4, -6}, {88, -6}, {89.6f, -2.6f}, {4, -2.6f}}, spine);
    p.stroke({9, 12.3f}, {82.6f, 12.3f}, 2.3f, {244, 248, 252, 255});
    p.stroke({8, 4.0f}, {80, 4.0f}, 1.2f, {222, 226, 236, 255});
    p.stroke({10, 7.5f}, {60, 7.5f}, 0.8f, {170, 176, 190, 255});
  }
}

void tensaZangetsu(const WP& p) {
  const Color black{24, 22, 28, 255}, steel{160, 160, 172, 255};
  Vec2 c{-14, 0};
  for (int k = 0; k < 5; ++k) {  // short chain hanging from the pommel
    Vec2 nx = c + Vec2{-3.8f, 1.4f + 0.5f * k + std::sin(p.time * 5.0f + k) * 0.5f};
    if (k % 2 == 0) p.seg(c, nx, 1.5f, steel);
    else {
      p.dot((c + nx) * 0.5f, 2.3f, steel);
      p.spot((c + nx) * 0.5f, 1.1f, black);
    }
    c = nx;
  }
  handle(p, -13, 4, 4.4f, {30, 28, 32, 255}, {120, 40, 44, 255}, 3.4f, true);
  // manji guard
  p.seg({5, -5.5f}, {5, 5.5f}, 1.9f, black);
  p.seg({5, -5.5f}, {8.5f, -5.5f}, 1.9f, black);
  p.seg({5, 5.5f}, {1.5f, 5.5f}, 1.9f, black);
  p.seg({2, 0}, {8, 0}, 1.9f, black);
  p.poly({{7, -2.4f}, {46, -2.6f}, {46, 2.8f}, {7, 3.0f}}, black);
  p.poly({{46, -2.6f}, {80, -4.0f}, {82, -3.4f}, {74, 0.7f}, {46, 2.8f}}, black);
  if (p.details()) {
    p.stroke({8, 2.2f}, {46, 2.0f}, 1.0f, {186, 188, 206, 255});
    p.stroke({46, 2.0f}, {74, 0.1f}, 1.0f, {186, 188, 206, 255});
    p.stroke({9, -1.0f}, {70, -1.6f}, 0.7f, {70, 60, 76, 255});
  }
}

// Byakuya's katana; `blade` = fraction of the blade still there (Senbonzakura scatters the rest).
void katana(const WP& p, float blade, Color guard) {
  const Color steel{214, 220, 232, 255};
  p.dot({-13, 0}, 2.3f, {196, 164, 84, 255});
  handle(p, -12, 3, 4.2f, {42, 36, 50, 255}, {224, 224, 232, 255}, 3.3f, true);
  p.poly({{3, -5}, {5.8f, -5.6f}, {5.8f, 5.6f}, {3, 5}}, guard);
  if (blade >= 0.99f) {
    p.poly({{6, -1.6f}, {40, -2.0f}, {40, 2.3f}, {6, 2.4f}}, steel);
    p.poly({{40, -2.0f}, {72, -3.6f}, {74, -3.2f}, {68, 0.6f}, {40, 2.3f}}, steel);
  } else {
    const float e = 6.0f + 34.0f * blade;
    p.poly({{6, -1.6f}, {e, -1.9f}, {e - 2.0f, 2.3f}, {6, 2.4f}}, steel);
  }
  if (p.details()) {
    const float e = blade >= 0.99f ? 68.0f : 6.0f + 32.0f * blade;
    p.stroke({7, 1.4f}, {std::min(e, 40.0f), 1.3f}, 0.9f, {252, 253, 255, 255});
    if (e > 40.0f) p.stroke({40, 1.3f}, {e, -0.2f}, 0.9f, {252, 253, 255, 255});
    p.stroke({7, -1.0f}, {std::min(e, 40.0f), -1.3f}, 0.8f, {150, 156, 172, 255});
    p.paint({{6, -1.8f}, {8.2f, -1.8f}, {8.2f, 2.4f}, {6, 2.4f}}, {214, 178, 92, 255});
  }
}

void senbonzakura(const WP& p) {
  katana(p, 0.4f, {150, 120, 64, 255});
  if (!p.details()) return;
  for (int k = 0; k < 16; ++k) {  // the blade comes apart into petals drifting away
    float ph = hash(p.time * 0.55f + k * 0.137f);
    float a = 20.0f + 56.0f * ph;
    float b = (std::sin(k * 2.3f) * 1.8f + std::sin(p.time * 4.0f + k) * 1.2f) * (0.4f + 1.6f * ph);
    float r = 2.4f * (1.0f - 0.35f * ph) * p.s;
    Color c = k % 3 ? Color{255, 168, 198, 255} : Color{255, 214, 228, 255};
    petal(p.at(a, b), r, k * 1.3f + p.time * 3.0f, p.col(withAlpha(c, 1.0f - 0.6f * ph)));
  }
}

void kageyoshi(const WP& p) {
  katana(p, 0.0f, {200, 120, 160, 255});
  if (!p.details()) return;
  p.halo({8, 0}, {82, 0}, 16.0f, {255, 150, 190, 255}, 1.2f);
  for (int k = 0; k < 20; ++k) {  // a storm of blade petals spiralling along the blade
    float ph = hash(k / 20.0f + p.time * 0.9f);
    float a = 8.0f + 74.0f * ph;
    float b = std::sin(a * 0.22f + p.time * 7.0f + k * 0.9f) * (3.0f + 4.0f * ph);
    Color c = k % 4 ? Color{255, 160, 196, 255} : Color{255, 236, 244, 255};
    petal(p.at(a, b), 2.3f * p.s, k + p.time * 5.0f, p.col(c));
  }
}

// ---- Zelda ------------------------------------------------------------------------------------------
void linkSword(const WP& p) {
  const Color steel{220, 226, 236, 255};
  p.dot({-11, 0}, 2.6f, {170, 140, 80, 255});
  handle(p, -10, 3, 4.0f, {98, 64, 42, 255}, {150, 108, 70, 255}, 3.2f);
  p.seg({4, -6.2f}, {4, 6.2f}, 2.6f, {150, 120, 70, 255});
  p.poly({{5.5f, -2.5f}, {58, -2.5f}, {65, 0}, {58, 2.5f}, {5.5f, 2.5f}}, steel);
  if (p.details()) {
    p.stroke({7, 0}, {56, 0}, 0.9f, {170, 180, 198, 255});
    p.stroke({7, 1.7f}, {57, 1.7f}, 0.9f, {250, 252, 255, 255});
  }
}

void masterSword(const WP& p) {
  const Color steel{226, 232, 242, 255}, hilt{72, 64, 168, 255}, wing{84, 96, 210, 255}, gold{234, 192, 80, 255};
  p.dot({-13, 0}, 2.8f, hilt);
  handle(p, -12, 3, 4.2f, hilt, {46, 40, 122, 255}, 3.2f, true);
  p.poly({{3, -1.5f}, {5.5f, -9.8f}, {9.8f, -8.6f}, {7.5f, -1.5f}}, wing);
  p.poly({{3, 1.5f}, {5.5f, 9.8f}, {9.8f, 8.6f}, {7.5f, 1.5f}}, wing);
  p.dot({6, 0}, 2.8f, gold);
  p.poly({{8, -2.9f}, {70, -2.9f}, {79, 0}, {70, 2.9f}, {8, 2.9f}}, steel);
  if (p.details()) {
    p.stroke({4.2f, -2.5f}, {6.6f, -8.8f}, 0.9f, gold);
    p.stroke({4.2f, 2.5f}, {6.6f, 8.8f}, 0.9f, gold);
    p.spot({6, 0}, 1.4f, {255, 240, 150, 255});
    p.stroke({14, 0}, {66, 0}, 1.0f, {176, 186, 206, 255});
    p.stroke({10, 2.0f}, {70, 2.0f}, 0.9f, {255, 255, 255, 255});
    p.paint({{10.5f, -1.6f}, {13.8f, 0}, {10.5f, 1.6f}}, gold);
  }
}

// Bow centred on a = 20 (limbs across the aim, belly forward): golden bow of light, plain wooden bow
// (variant 1) or a bow of flames (variant 2).
void bowOfLight(const WP& p, float pull, int variant) {
  const Color body = variant == 1   ? Color{132, 90, 54, 255}
                     : variant == 2 ? Color{255, 120, 36, 255}
                     : variant == 3 ? Color{150, 70, 230, 255}
                                    : Color{246, 206, 96, 255};
  const Color core = variant == 1   ? Color{176, 128, 80, 255}
                     : variant == 2 ? Color{255, 230, 140, 255}
                     : variant == 3 ? Color{230, 190, 255, 255}
                                    : Color{255, 248, 210, 255};
  Vec2 pts[7];
  for (int i = 0; i < 7; ++i) {
    float t = -1.0f + i / 3.0f;
    pts[i] = {23.0f - 10.0f * t * t, 24.0f * t};
  }
  if (p.details() && variant != 1)
    discW(p.at(20, 0), 26.0f * p.s,
          p.col(variant == 2 ? Color{255, 100, 30, 50} : variant == 3 ? Color{170, 80, 255, 60} : Color{255, 220, 120, 40}));
  for (int i = 0; i < 6; ++i) p.seg(pts[i], pts[i + 1], 3.4f - std::fabs(i - 2.5f) * 0.4f, body);
  p.seg(pts[0], pts[0] + Vec2{3.5f, -2.5f}, 2.0f, body);
  p.seg(pts[6], pts[6] + Vec2{3.5f, 2.5f}, 2.0f, body);
  p.seg({23, -3.5f}, {23, 3.5f}, 4.4f, variant == 1 ? Color{90, 60, 40, 255} : Color{176, 122, 60, 255});
  if (p.details()) {
    for (int i = 0; i < 6; ++i) p.stroke(pts[i] + Vec2{0.6f, 0}, pts[i + 1] + Vec2{0.6f, 0}, 1.1f, core);
    if (variant == 2 || variant == 3)
      for (int i = 0; i < 5; ++i) {  // flames licking off the limbs
        float ph = hash(p.time * 1.7f + i * 0.21f);
        Vec2 b = pts[1 + i % 5] + Vec2{-ph * 6.0f, std::sin(i * 2.0f + p.time * 9.0f) * 1.5f};
        p.spot(b, 2.2f * (1.0f - ph), variant == 3 ? Color{200, 140, 255, 220} : Color{255, 200, 80, 220});
      }
    const Vec2 nock{13.0f - 24.0f * std::max(0.0f, pull), 0.0f};
    const Color str = variant == 1 ? Color{220, 214, 200, 255} : Color{255, 252, 230, 255};
    for (Vec2 tip : {pts[0], pts[6]}) {
      if (variant != 1) DrawLineEx(V(p.at(tip)), V(p.at(nock)), 3.0f * p.s, p.col({255, 230, 140, 80}));
      DrawLineEx(V(p.at(tip)), V(p.at(nock)), std::max(0.8f, 0.9f * p.s), p.col(str));
    }
  }
}

// Round shield seen in three quarter (squashed across x in world space).
void sector(Vec2 c, float rx, float ry, float a0, float a1, Color k) {
  const int n = std::max(2, int((a1 - a0) / 0.35f) + 1);
  Vec2 prev = c + Vec2{std::cos(a0) * rx, std::sin(a0) * ry};
  for (int i = 1; i <= n; ++i) {
    float a = a0 + (a1 - a0) * i / n;
    Vec2 q = c + Vec2{std::cos(a) * rx, std::sin(a) * ry};
    triW(V(c), V(prev), V(q), k);
    prev = q;
  }
}

// ---- Ganondorf ----------------------------------------------------------------------------------
void demonKingSword(const WP& p) {
  const Color blade{32, 24, 30, 255}, red{255, 70, 90, 255};
  p.halo({10, 2}, {94, 1}, 12.0f, {200, 20, 50, 255}, 0.9f);
  p.dot({-16, 0}, 3.3f, {64, 44, 54, 255});
  handle(p, -15, 4, 5.2f, {38, 28, 34, 255}, {120, 30, 40, 255}, 3.8f);
  p.poly({{3, -3}, {7, -12.5f}, {10.5f, -11.5f}, {8.5f, -2}}, {44, 32, 40, 255});
  p.poly({{3, 3}, {7, 12.5f}, {10.5f, 11.5f}, {8.5f, 2}}, {44, 32, 40, 255});
  p.poly({{30, -5.4f}, {38, -5.7f}, {35.5f, -10}}, blade);
  p.poly({{50, -5.8f}, {58, -6.0f}, {56, -10.5f}}, blade);
  p.poly({{68, -6.0f}, {75, -6.0f}, {74, -9.8f}}, blade);
  p.poly({{9, -5}, {80, -6}, {96, -1}, {96, 1}, {82, 7.5f}, {9, 6}}, blade);
  if (p.details()) {
    p.stroke({10, 5.2f}, {82, 6.4f}, 1.5f, red);
    p.stroke({82, 6.4f}, {95, 0.6f}, 1.5f, red);
    p.stroke({14, 0.4f}, {40, -1.0f}, 0.8f, {190, 30, 50, 255});
    p.stroke({40, -1.0f}, {62, 1.2f}, 0.8f, {190, 30, 50, 255});
    p.stroke({62, 1.2f}, {84, 0}, 0.8f, {190, 30, 50, 255});
    p.dot({8, 0}, 2.5f, {226, 40, 64, 255});
    p.spot({7.4f, -0.6f}, 0.9f, {255, 190, 200, 255});
    p.spot({-16, 0}, 1.4f, {230, 60, 80, 255});
  }
}

void gloomWisp(const WP& p, Vec2 a, Vec2 b, float amp, float seed) {
  if (!p.details()) return;
  Vec2 prev = a;
  for (int i = 1; i <= 6; ++i) {
    float t = i / 6.0f;
    Vec2 q = lerpv(a, b, t) + Vec2{0, std::sin(t * 7.0f + p.time * 9.0f + seed) * amp * t};
    p.stroke(prev, q, 2.4f * (1.0f - t * 0.6f), {120, 10, 34, 150});
    p.stroke(prev, q, 1.0f * (1.0f - t * 0.6f), {255, 90, 110, 200});
    prev = q;
  }
}

void gloomSpear(const WP& p) {
  const Color shaft{36, 24, 30, 255}, head{44, 20, 28, 255}, red{255, 80, 100, 255};
  p.halo({60, 0}, {104, 0}, 12.0f, {200, 20, 50, 255}, 1.0f);
  p.seg({-20, 0}, {82, 0}, 3.4f, shaft);
  p.poly({{84, -3}, {79, -9.5f}, {88, -5}}, head);
  p.poly({{84, 3}, {79, 9.5f}, {88, 5}}, head);
  p.poly({{80, -2.6f}, {90, -6.8f}, {105, 0}, {90, 6.8f}, {80, 2.6f}}, head);
  if (p.details()) {
    for (int i = 0; i < 5; ++i) p.stroke({-14.0f + i * 19.0f, 0}, {-6.0f + i * 19.0f, 0}, 1.2f, {200, 30, 50, 255});
    p.stroke({82, 0}, {101, 0}, 1.3f, red);
    gloomWisp(p, {20, -2}, {74, -7}, 3.0f, 0.0f);
    gloomWisp(p, {36, 2}, {90, 8}, 3.0f, 2.0f);
  }
}

// ---- God of War -----------------------------------------------------------------------------------
void leviathanAxe(const WP& p) {
  const Color wood{112, 78, 50, 255}, steel{184, 204, 222, 255}, frost{150, 220, 255, 255};
  p.halo({52, 10}, {70, 18}, 16.0f, frost, 0.8f);
  p.dot({-11, 0}, 2.8f, {164, 172, 186, 255});
  handle(p, -10, 66, 4.0f, wood, {80, 54, 36, 255}, 5.0f);
  p.poly({{55, -3.6f}, {64, -3.6f}, {60, -8.5f}}, {150, 164, 180, 255});
  p.poly({{52, -3.8f}, {66, -3.8f}, {66, 3.8f}, {52, 3.8f}}, {150, 164, 180, 255});
  p.poly({{53, 3}, {66, 3}, {75, 12}, {71, 21}, {58, 23}, {49, 14}}, steel);
  if (p.details()) {
    p.stroke({72.4f, 12}, {69.2f, 20.2f}, 2.2f, {236, 248, 255, 255});
    p.stroke({69.2f, 20.2f}, {58.5f, 22.0f}, 2.2f, {236, 248, 255, 255});
    p.stroke({-8, -1.2f}, {-8, 1.2f}, 6.0f, {74, 50, 34, 255});
    const float gl = 0.75f + 0.25f * std::sin(p.time * 4.0f);
    const Color rune = withAlpha(mixColor(frost, WHITE, 0.3f), gl);
    p.stroke({56, 8}, {56, 14}, 0.9f, rune);  // runes
    p.stroke({56, 10}, {58.5f, 8}, 0.9f, rune);
    p.stroke({61, 9}, {61, 15}, 0.9f, rune);
    p.stroke({61, 12}, {63.5f, 14}, 0.9f, rune);
    p.stroke({65.5f, 11}, {65.5f, 17}, 0.9f, rune);
    p.stroke({64.5f, 12.5f}, {66.5f, 12.5f}, 0.9f, rune);
    for (int i = 0; i < 3; ++i) {  // frost sparkles
      float t = hash(p.time * 0.8f + i * 0.33f);
      p.spot({70.0f - 10.0f * t, 15.0f + 6.0f * t}, 1.2f * (1.0f - t), {240, 252, 255, 255});
    }
  }
}

// One Blade of Chaos, its fiery chain trailing behind the hand.
void chaosBlade(const WP& p, float side) {
  const Color steel{206, 194, 180, 255}, hot{255, 140, 40, 255};
  if (p.details()) {
    Vec2 c{-6, 0};
    for (int k = 0; k < 6; ++k) {
      Vec2 nx = c + Vec2{-5.5f, side * (1.6f + 0.6f * k) + std::sin(p.time * 6.0f + k) * 0.8f};
      DrawLineEx(V(p.at(c)), V(p.at(nx)), 4.5f * p.s, p.col({255, 110, 30, 60}));
      p.stroke(c, nx, 1.8f, {96, 84, 76, 255});
      p.spot(nx, 1.3f, {255, 170, 70, 255});
      c = nx;
    }
  }
  p.dot({-6, 0}, 2.6f, {200, 160, 70, 255});
  handle(p, -5, 4, 4.2f, {74, 46, 34, 255}, {200, 160, 70, 255}, 3.0f);
  p.poly({{4, -3.4f}, {26, -4.8f}, {28, 4.2f}, {4, 3.6f}}, steel);
  p.poly({{26, -4.8f}, {42, -2.6f}, {54, 5.6f}, {46, 6.6f}, {28, 4.2f}}, steel);
  if (p.details()) {
    p.halo({8, 3}, {50, 6}, 9.0f, hot, 1.0f);
    p.stroke({6, 2.8f}, {27, 3.4f}, 1.6f, hot);
    p.stroke({28, 3.4f}, {47, 5.6f}, 1.6f, hot);
    p.stroke({8, -1.0f}, {38, -1.4f}, 0.9f, {120, 104, 92, 255});
  }
}

// ---- Jujutsu Kaisen ---------------------------------------------------------------------------------
void hollowPurple(const WP& p) {
  if (p.line) return;  // pure energy: no outline
  const Vec2 c = p.at(15, 0);
  const float r = 11.0f * p.s;
  const float pulse = 1.0f + 0.06f * std::sin(p.time * 9.0f);
  discW(c, r * 2.0f * pulse, p.col({170, 90, 255, 46}));
  discW(c, r * 1.35f * pulse, p.col({160, 80, 240, 90}));
  discW(c, r, p.col({128, 54, 210, 255}));
  if (!p.ghost) {
    for (int i = 0; i < 2; ++i) {
      float a = p.time * 6.0f + i * kPi;
      Vec2 o{std::cos(a) * r * 0.45f, std::sin(a) * r * 0.3f};
      discW(c + o, r * 0.36f, p.col(i ? Color{255, 90, 100, 230} : Color{90, 170, 255, 230}));
    }
    discW(c, r * 0.42f, p.col({236, 210, 255, 255}));
    DrawRing(V(c), r * 1.02f, r * 1.18f, p.time * 200.0f, p.time * 200.0f + 250.0f, 18, p.col({230, 200, 255, 200}));
  }
}

// Six Eyes: Gojo's folded blindfold with the glowing eye.
void sixEyes(const WP& p) {
  const Color band{28, 28, 36, 255};
  if (p.details()) discW(p.at(15, 0), 17.0f * p.s, p.col({120, 210, 255, 60}));
  p.poly({{1, -4.5f}, {29, -6.5f}, {29, 0.5f}, {1, 2.5f}}, band);
  p.poly({{4, 1.0f}, {27, -1.0f}, {26, 6.5f}, {6, 7.5f}}, band);
  if (p.details()) {
    p.stroke({3, 1.8f}, {27, -0.2f}, 0.8f, {70, 70, 86, 255});
    const Vec2 e = p.at(15, -0.5f);
    const float s = p.s;
    for (int i = 0; i < 2; ++i) {  // almond eye
      Vector2 a = V(e + p.u * (-8.0f * s)), b = V(e + p.u * (8.0f * s));
      Vector2 m = V(e + p.v * ((i ? 4.6f : -4.6f) * s));
      triW(a, m, b, p.col({230, 246, 255, 255}));
    }
    discW(e, 3.6f * s, p.col({70, 180, 255, 255}));
    discW(e, 1.6f * s, p.col({20, 40, 90, 255}));
    discW(e + p.u * (-1.0f * s) + p.v * (-1.2f * s), 0.9f * s, p.col(WHITE));
  }
}

void kamutoke(const WP& p) {
  const Color shaft{58, 54, 78, 255}, gold{232, 214, 150, 255}, bolt{210, 244, 255, 255};
  p.halo({70, 0}, {96, 0}, 18.0f, {140, 220, 255, 255}, 1.0f);
  p.seg({-14, 0}, {74, 0}, 3.0f, shaft);
  Vec2 arc[5];
  for (int i = 0; i < 5; ++i) {
    float a = deg2rad(-110.0f + i * 55.0f);
    arc[i] = Vec2{80, 0} + Vec2{std::cos(a) * 9.0f, std::sin(a) * 11.0f};
  }
  for (int i = 0; i < 4; ++i) p.seg(arc[i], arc[i + 1], 2.8f, gold);
  p.poly({{72, -1.8f}, {94, 0}, {72, 1.8f}}, {236, 240, 255, 255});
  if (p.details()) {
    for (float a : {0.0f, 30.0f, 60.0f}) p.stroke({a, -1.8f}, {a, 1.8f}, 2.2f, gold);
    const int seed = int(p.time * 18.0f);
    for (int k = 0; k < 2; ++k) {  // crackling lightning
      Vec2 prev{-6.0f + k * 20.0f, 0};
      for (int i = 1; i <= 5; ++i) {
        Vec2 q{prev.x + 15.0f, (hash(std::sin(seed * 12.9898f + i * 78.233f + k * 3.1f) * 43758.5f) - 0.5f) * 12.0f};
        DrawLineEx(V(p.at(prev)), V(p.at(q)), 3.6f * p.s, p.col({120, 200, 255, 70}));
        p.stroke(prev, q, 1.1f, bolt);
        prev = q;
      }
    }
  }
}

// Sukuna's finger: mummified, three joints, black markings and a talisman.
void cursedFinger(const WP& p) {
  const Color skin{122, 86, 92, 255}, dark{70, 40, 48, 255};
  if (p.details()) discW(p.at(14, 0), 15.0f * p.s, p.col({200, 30, 50, 50}));
  p.seg({0, 0}, {10, 0.6f}, 7.2f, skin);
  p.seg({10, 0.6f}, {19, 0.3f}, 6.4f, skin);
  p.seg({19, 0.3f}, {26, -0.4f}, 5.6f, skin);
  p.poly({{-1, -4.6f}, {5, -4.8f}, {5, 4.8f}, {-1, 4.6f}}, {228, 214, 178, 255});
  if (p.details()) {
    p.stroke({10, -2.6f}, {10.6f, 3.0f}, 1.0f, dark);
    p.stroke({19, -2.4f}, {19.4f, 2.6f}, 1.0f, dark);
    p.stroke({14, -2.8f}, {14.4f, 3.0f}, 1.6f, {20, 14, 18, 255});
    p.stroke({22.5f, -2.4f}, {22.8f, 2.2f}, 1.4f, {20, 14, 18, 255});
    p.paint({{22.5f, -2.9f}, {27.6f, -2.6f}, {28.4f, -0.4f}, {23, -0.2f}}, {84, 54, 58, 255});  // nail
    p.stroke({0.5f, -2.5f}, {3.5f, 2.5f}, 1.0f, {200, 40, 50, 255});
    p.stroke({0.5f, 2.5f}, {3.5f, -2.5f}, 1.0f, {200, 40, 50, 255});
  }
}


// ---- Bleach (2) ----------------------------------------------------------------------------------
// Kenpachi's sword: long, notched and chipped, a plain guard and a bare grip.
void kenpachiSword(const WP& p) {
  const Color steel{182, 186, 196, 255}, dull{128, 130, 142, 255};
  p.dot({-14, 0}, 2.4f, {60, 58, 64, 255});
  handle(p, -13, 5, 4.6f, {92, 88, 92, 255}, {150, 146, 150, 255}, 4.6f);
  p.poly({{4.5f, -4.6f}, {8, -4.6f}, {8, 4.6f}, {4.5f, 4.6f}}, {64, 62, 70, 255});
  p.poly({{8, -2.2f}, {60, -2.6f}, {60, 2.9f}, {8, 2.7f}}, steel);
  p.poly({{60, -2.6f}, {94, -3.4f}, {96, -2.8f}, {90, 1.0f}, {60, 2.9f}}, steel);
  if (p.details()) {
    p.stroke({9, -0.6f}, {88, -1.6f}, 0.9f, dull);
    p.stroke({9, 2.0f}, {60, 2.1f}, 0.9f, {236, 240, 248, 255});
    for (float a : {21.0f, 38.0f, 52.0f, 71.0f, 83.0f}) p.paint({{a - 1.6f, 3.2f}, {a, 1.0f}, {a + 1.4f, 3.2f}}, kInk);
    p.spot({30, 0.4f}, 1.0f, {150, 60, 54, 255});
  }
}

// Nozarashi: a giant cleaver-axe on a long haft.
void nozarashi(const WP& p) {
  const Color steel{170, 174, 184, 255}, edge{232, 236, 244, 255}, haft{70, 64, 70, 255};
  handle(p, -16, 30, 5.2f, haft, {150, 140, 120, 255}, 5.0f);
  p.poly({{28, -6}, {104, -9}, {110, -4}, {106, 14}, {94, 24}, {40, 22}, {28, 10}}, steel);
  if (p.details()) {
    p.paint({{30, -5}, {103, -8}, {104, -4.5f}, {30, -1.5f}}, {120, 124, 136, 255});
    p.stroke({42, 20.6f}, {93, 22.4f}, 2.2f, edge);
    p.stroke({94, 22.4f}, {105, 13}, 2.0f, edge);
    p.spot({36, 4}, 3.2f, {48, 46, 54, 255});
    p.spot({36, 4}, 1.4f, {200, 190, 140, 255});
    for (float a : {58.0f, 79.0f}) p.paint({{a - 2.0f, 23.6f}, {a, 20.0f}, {a + 2.0f, 23.4f}}, kInk);
  }
}

// Bankai: a huge crimson cleaver dripping red reiatsu.
void redBankai(const WP& p) {
  const Color body{150, 18, 30, 255}, dark{70, 6, 14, 255}, glow{255, 70, 70, 255};
  p.halo({30, 6}, {108, 6}, 34.0f, glow, 1.0f);
  handle(p, -16, 30, 5.6f, {40, 12, 18, 255}, {200, 40, 50, 255}, 4.6f);
  p.poly({{28, -8}, {106, -11}, {113, -5}, {109, 16}, {96, 27}, {40, 24}, {28, 11}}, body);
  if (p.details()) {
    p.paint({{30, -7}, {105, -10}, {106, -5}, {30, -2}}, dark);
    p.stroke({42, 22.6f}, {95, 24.8f}, 2.4f, {255, 150, 140, 255});
    for (int i = 0; i < 4; ++i) {  // drops of reiatsu falling off the edge
      const float ph = hash(p.time * 1.3f + i * 0.29f);
      p.spot({48.0f + i * 15.0f, 25.0f + ph * 14.0f}, 1.8f * (1.0f - ph * 0.6f), {255, 60, 60, 230});
    }
  }
}

// Murcielago's spear of green light.
void luzDeLaLuna(const WP& p) {
  const Color core{214, 255, 226, 255}, body{80, 230, 140, 255};
  p.halo({-12, 0}, {104, 0}, 14.0f, {90, 255, 150, 255}, 1.2f);
  p.seg({-14, 0}, {80, 0}, 3.0f, body);
  p.poly({{78, -3.5f}, {104, 0}, {78, 3.5f}}, body);
  p.poly({{70, -1.6f}, {78, -9}, {82, -1.6f}}, body);
  p.poly({{70, 1.6f}, {78, 9}, {82, 1.6f}}, body);
  if (p.details()) {
    p.stroke({-12, 0}, {100, 0}, 1.0f, core);
    for (int i = 0; i < 3; ++i) p.spot({hash(p.time * 0.8f + i * 0.33f) * 90.0f, 0}, 1.6f, WHITE);
  }
}

// Segunda Etapa: a black-green crystal of power floating over the hand.
void segundaCore(const WP& p) {
  if (p.line) return;
  const Vec2 c = p.at(15, 0);
  const float r = 9.0f * p.s, pulse = 1.0f + 0.08f * std::sin(p.time * 8.0f);
  discW(c, r * 2.2f * pulse, p.col({60, 255, 140, 40}));
  const Vec2 up{0, -1}, rt{1, 0};
  triW(V(c + up * (r * 1.6f)), V(c + rt * r), V(c - rt * r), p.col({20, 30, 24, 255}));
  triW(V(c - up * (r * 1.3f)), V(c + rt * r), V(c - rt * r), p.col({10, 16, 12, 255}));
  if (!p.ghost) {
    DrawLineEx(V(c + up * (r * 1.4f)), V(c - up * (r * 1.1f)), std::max(0.8f, r * 0.18f), p.col({120, 255, 170, 255}));
    discW(c, r * 0.3f, p.col({200, 255, 220, 255}));
  }
}

// ---- Monster Hunter ------------------------------------------------------------------------------
// Great sword: a slab of steel framed in wyvern bone.
void greatSword(const WP& p) {
  const Color steel{168, 172, 180, 255}, bone{226, 214, 186, 255}, leather{96, 64, 44, 255};
  p.dot({-15, 0}, 3.2f, {120, 100, 70, 255});
  handle(p, -14, 8, 5.6f, leather, {170, 130, 84, 255}, 3.6f);
  p.poly({{7, -10}, {15, -12}, {15, 12}, {7, 10}}, bone);
  p.poly({{15, -8}, {96, -8.5f}, {108, 0}, {96, 9}, {15, 9}}, steel);
  if (p.details()) {
    p.paint({{17, -6.5f}, {94, -7}, {94, -3.5f}, {17, -3}}, bone);
    p.stroke({17, 7.2f}, {95, 7.4f}, 1.6f, {236, 240, 246, 255});
    p.stroke({20, 1.0f}, {86, 1.0f}, 1.2f, {120, 124, 134, 255});
    for (float a : {30.0f, 55.0f, 78.0f}) p.spot({a, -5}, 1.4f, {170, 140, 100, 255});
  }
}

// Charge blade: sword mode, glowing phials at the guard.
void chargeBlade(const WP& p) {
  const Color steel{196, 200, 210, 255}, body{60, 74, 96, 255}, phial{255, 220, 90, 255};
  p.dot({-13, 0}, 2.6f, body);
  handle(p, -12, 5, 4.6f, {54, 50, 56, 255}, {150, 150, 160, 255}, 3.6f);
  p.poly({{5, -6}, {16, -6}, {16, 7}, {5, 7}}, body);
  p.poly({{16, -3.6f}, {70, -4.2f}, {82, 0}, {70, 4.6f}, {16, 4.2f}}, steel);
  if (p.details()) {
    const float g = 0.6f + 0.4f * std::sin(p.time * 6.0f);
    for (int i = 0; i < 3; ++i) p.spot({7.5f + i * 3.6f, 0.5f}, 1.4f, withAlpha(phial, 0.6f + 0.4f * g));
    p.stroke({18, 3.0f}, {70, 3.3f}, 1.0f, {240, 244, 250, 255});
    p.stroke({20, 0}, {68, 0}, 1.4f, {90, 110, 150, 255});
  }
}

// Gunlance: a heavy lance with a shell chamber.
void gunlance(const WP& p) {
  const Color steel{150, 156, 168, 255}, dark{70, 74, 84, 255}, brass{200, 160, 80, 255};
  handle(p, -12, 6, 5.0f, {60, 50, 46, 255}, {140, 120, 90, 255}, 3.4f);
  p.poly({{6, -7}, {26, -7}, {26, 8}, {6, 8}}, dark);  // shell chamber
  p.poly({{26, -5}, {92, -3.5f}, {108, 0.5f}, {92, 4.5f}, {26, 6}}, steel);
  if (p.details()) {
    p.stroke({8, -3}, {24, -3}, 1.4f, brass);
    p.stroke({8, 3.5f}, {24, 3.5f}, 1.4f, brass);
    p.stroke({28, -2.4f}, {92, -1.6f}, 1.0f, {214, 220, 230, 255});
    p.spot({100, 0.5f}, 1.2f, {255, 200, 120, 255});
  }
}

// ---- Naruto -----------------------------------------------------------------------------------------
// Sasuke's Kusanagi: a straight sword without a guard.
void kusanagi(const WP& p) {
  const Color steel{210, 214, 226, 255};
  handle(p, -12, 3, 4.0f, {36, 32, 40, 255}, {120, 110, 130, 255}, 3.0f, true);
  p.poly({{3, -3.2f}, {5, -3.2f}, {5, 3.2f}, {3, 3.2f}}, {60, 56, 64, 255});
  p.poly({{5, -1.8f}, {64, -1.9f}, {70, 0.2f}, {64, 2.2f}, {5, 2.1f}}, steel);
  if (p.details()) {
    p.stroke({6, 1.2f}, {64, 1.3f}, 0.9f, {250, 252, 255, 255});
    p.stroke({6, -0.8f}, {62, -0.9f}, 0.7f, {150, 156, 172, 255});
  }
}

// Kakashi's kunai: ring pommel, wrapped grip, leaf blade.
void kunai(const WP& p) {
  const Color steel{150, 156, 168, 255};
  if (!p.line || !p.ghost) {
    p.dot({-7, 0}, 3.0f, steel);
    p.spot({-7, 0}, 1.6f, {40, 40, 46, 255});
  }
  handle(p, -4, 7, 3.0f, {40, 40, 46, 255}, {120, 120, 130, 255}, 2.2f);
  p.poly({{7, -1.2f}, {12, -4.2f}, {28, 0}, {12, 4.2f}, {7, 1.2f}}, steel);
  if (p.details()) p.stroke({9, 0}, {26, 0}, 0.9f, {220, 226, 236, 255});
}

// Eyes and chakra worn as power-ups (drawn by the body: see render_body.cpp).
void sageScroll(const WP& p) {
  const Color paper{236, 220, 180, 255}, wood{150, 60, 40, 255};
  p.seg({2, 0}, {28, 0}, 9.0f, paper);
  p.seg({0, 0}, {2, 0}, 10.5f, wood);
  p.seg({28, 0}, {30, 0}, 10.5f, wood);
  if (p.details()) p.stroke({8, -1.5f}, {22, -1.5f}, 1.2f, {60, 40, 30, 255});
}

void guardianShieldDisc(Vec2 c, float r, float open, Vec2 fold, float alpha, float ow, bool ghost, Color tint,
                        float time);
void roundShieldDisc(Vec2 c, float r, float alpha, float ow, bool ghost, Color tint);

void drawVisual(WeaponVisual v, const WP& p, const WeaponStyle& st) {
  switch (v) {
    case WeaponVisual::AstaSword: astaSword(p); break;
    case WeaponVisual::DemonSlayer: demonSlayer(p); break;
    case WeaponVisual::DemonDweller: demonDweller(p); break;
    case WeaponVisual::Grimoire: grimoire(p, st.open); break;
    case WeaponVisual::WindSpear: windSpear(p); break;
    case WeaponVisual::Zangetsu: zangetsu(p); break;
    case WeaponVisual::TensaZangetsu: tensaZangetsu(p); break;
    case WeaponVisual::Katana: katana(p, 1.0f, {150, 120, 64, 255}); break;
    case WeaponVisual::Senbonzakura: senbonzakura(p); break;
    case WeaponVisual::Kageyoshi: kageyoshi(p); break;
    case WeaponVisual::MasterSword: masterSword(p); break;
    case WeaponVisual::BowOfLight: bowOfLight(p, st.pull, st.variant); break;
    case WeaponVisual::DemonKingSword: demonKingSword(p); break;
    case WeaponVisual::GloomSpear: gloomSpear(p); break;
    case WeaponVisual::LeviathanAxe: leviathanAxe(p); break;
    case WeaponVisual::BladesOfChaos:
      if (st.single) {
        chaosBlade(p, 1.0f);
      } else {  // a crossed pair, chains looping between them
        WP q = p;
        q.o = p.at(-2, 7);
        q.u = rotate(p.u, -0.35f);
        q.v = rotate(p.v, -0.35f);
        chaosBlade(q, 1.0f);
        q.o = p.at(4, -7);
        q.u = rotate(p.u, 0.35f);
        q.v = rotate(p.v, 0.35f) * -1.0f;
        chaosBlade(q, 1.0f);
      }
      break;
    case WeaponVisual::HollowPurple: hollowPurple(p); break;
    case WeaponVisual::SixEyes: sixEyes(p); break;
    case WeaponVisual::Kamutoke: kamutoke(p); break;
    case WeaponVisual::CursedFinger: cursedFinger(p); break;
    case WeaponVisual::ShieldAndSword:
      if (!st.single && p.line) roundShieldDisc(p.at(30, -2), 17.0f * p.s, p.alpha, p.ow, p.ghost, p.tint);
      linkSword(p);
      break;
    case WeaponVisual::GuardianShield:
      if (p.line) guardianShieldDisc(p.at(18, 0), 17.0f * p.s, 1.0f, p.u, p.alpha, p.ow, p.ghost, p.tint, p.time);
      break;
    case WeaponVisual::KenpachiSword: kenpachiSword(p); break;
    case WeaponVisual::Nozarashi: nozarashi(p); break;
    case WeaponVisual::RedBankai: redBankai(p); break;
    case WeaponVisual::LuzDeLaLuna: luzDeLaLuna(p); break;
    case WeaponVisual::SegundaCore: segundaCore(p); break;
    case WeaponVisual::GreatSword: greatSword(p); break;
    case WeaponVisual::ChargeBlade: chargeBlade(p); break;
    case WeaponVisual::Gunlance: gunlance(p); break;
    case WeaponVisual::Kusanagi: kusanagi(p); break;
    case WeaponVisual::KakashiKunai: kunai(p); break;
    case WeaponVisual::SageScroll: sageScroll(p); break;
    case WeaponVisual::SusanooBow: bowOfLight(p, st.pull, 3); break;
    default: break;
  }
}

// Link's round wooden shield (face towards the viewer).
void roundShieldDisc(Vec2 c, float r, float alpha, float ow, bool ghost, Color tint) {
  auto col = [&](Color k) { return withAlpha(ghost ? Color{tint.r, tint.g, tint.b, k.a} : k, alpha * k.a / 255.0f); };
  const float rx = r * 0.8f;
  if (!ghost) sector(c, rx + ow, r + ow, 0.0f, 2.0f * kPi, col(kInk));
  sector(c, rx, r, 0.0f, 2.0f * kPi, col({150, 152, 162, 255}));
  sector(c, rx * 0.84f, r * 0.84f, 0.0f, 2.0f * kPi, col({138, 94, 58, 255}));
  if (ghost) return;
  for (float x : {-0.42f, 0.0f, 0.42f})
    DrawLineEx(V(c + Vec2{x * rx, -r * 0.8f}), V(c + Vec2{x * rx, r * 0.8f}), std::max(0.8f, r * 0.05f),
               col({96, 62, 40, 255}));
  sector(c + Vec2{-rx * 0.18f, -r * 0.2f}, rx * 0.5f, r * 0.45f, 0.0f, 2.0f * kPi, col({160, 112, 70, 120}));
  discW(c, r * 0.26f + ow * 0.6f, col(kInk));
  discW(c, r * 0.26f, col({176, 180, 192, 255}));
  discW(c + Vec2{-r * 0.07f, -r * 0.08f}, r * 0.11f, col({236, 238, 244, 255}));
}

// Kratos's Guardian Shield: a fan of wood and metal wedges that unfolds around its boss.
void guardianShieldDisc(Vec2 c, float r, float open, Vec2 fold, float alpha, float ow, bool ghost, Color tint,
                        float time) {
  (void)time;
  auto col = [&](Color k) { return withAlpha(ghost ? Color{tint.r, tint.g, tint.b, k.a} : k, alpha * k.a / 255.0f); };
  open = clampf(open, 0.0f, 1.0f);
  const int n = 8;
  const float base = std::atan2(fold.y, fold.x);
  const float step = lerpf(0.16f, 2.0f * kPi / n, open);
  const float hw = lerpf(0.3f, kPi / n + 0.01f, open);
  const float R = r * lerpf(0.55f, 1.0f, open);
  const float rx = R * 0.82f;
  auto wedge = [&](int i, float grow, Color k) {
    float a = base + (i - (n - 1) * 0.5f) * step;
    sector(c, rx + grow, R + grow, a - hw - (grow > 0 ? 0.06f : 0.0f), a + hw + (grow > 0 ? 0.06f : 0.0f), k);
  };
  if (!ghost)
    for (int i = 0; i < n; ++i) wedge(i, ow, col(kInk));
  for (int i = 0; i < n; ++i) wedge(i, 0.0f, col(i % 2 ? Color{126, 84, 52, 255} : Color{150, 140, 128, 255}));
  if (ghost) return;
  if (open > 0.6f) {
    DrawRing(V(c), R * 0.9f, R, 0, 360, 36, col({110, 100, 92, 255}));
    for (int i = 0; i < n; ++i) {
      float a = base + (i - (n - 1) * 0.5f) * step + step * 0.5f;
      DrawLineEx(V(c), V(c + Vec2{std::cos(a) * rx, std::sin(a) * R}), std::max(0.8f, r * 0.04f), col({70, 48, 34, 255}));
    }
  }
  discW(c, r * 0.3f + ow * 0.6f, col(kInk));
  discW(c, r * 0.3f, col({196, 150, 70, 255}));
  DrawRing(V(c), r * 0.15f, r * 0.21f, 0, 360, 16, col({120, 84, 40, 255}));
  discW(c + Vec2{-r * 0.08f, -r * 0.09f}, r * 0.08f, col({255, 226, 150, 255}));
}

// Aura of a picked-up weapon: soft halo of its element along the blade, a few motes drifting up.
void elementAura(const WP& p, WeaponVisual v, HitFx fx, float k) {
  const float len = weaponVisualLength(v);
  if (len <= 0.0f) return;
  const Color c = hitFxColor(fx), g = hitFxGlow(fx);
  const float pulse = 0.75f + 0.25f * std::sin(p.time * 5.0f);
  const float a0 = 4.0f, a1 = len - 2.0f;
  DrawLineEx(V(p.at(a0, 0)), V(p.at(a1, 0)), 22.0f * p.s, p.col(withAlpha(c, 0.13f * k * pulse)));
  DrawLineEx(V(p.at(a0, 0)), V(p.at(a1, 0)), 12.0f * p.s, p.col(withAlpha(g, 0.18f * k * pulse)));
  for (int i = 0; i < 4; ++i) {
    float t = hash(p.time * 0.9f + i * 0.27f);
    Vec2 q = p.at(lerpf(a0, a1, hash(i * 0.618f + 0.2f)), 0) + Vec2{std::sin(i * 2.0f + p.time * 3.0f) * 4.0f, -t * 18.0f};
    discW(q, (1.8f - 1.2f * t) * p.s, p.col(withAlpha(g, (1.0f - t) * 0.8f * k)));
  }
}
}  // namespace

float weaponVisualLength(WeaponVisual v) {
  switch (v) {
    case WeaponVisual::None: return 0.0f;
    case WeaponVisual::AstaSword: return 86.0f;
    case WeaponVisual::DemonSlayer: return 100.0f;
    case WeaponVisual::DemonDweller: return 80.0f;
    case WeaponVisual::Grimoire: return 30.0f;
    case WeaponVisual::WindSpear: return 98.0f;
    case WeaponVisual::Zangetsu: return 90.0f;
    case WeaponVisual::TensaZangetsu: return 82.0f;
    case WeaponVisual::Katana: return 72.0f;
    case WeaponVisual::Senbonzakura: return 74.0f;
    case WeaponVisual::Kageyoshi: return 80.0f;
    case WeaponVisual::MasterSword: return 79.0f;
    case WeaponVisual::BowOfLight: return 40.0f;
    case WeaponVisual::DemonKingSword: return 96.0f;
    case WeaponVisual::GloomSpear: return 105.0f;
    case WeaponVisual::LeviathanAxe: return 72.0f;
    case WeaponVisual::BladesOfChaos: return 54.0f;
    case WeaponVisual::HollowPurple: return 30.0f;
    case WeaponVisual::SixEyes: return 30.0f;
    case WeaponVisual::Kamutoke: return 94.0f;
    case WeaponVisual::CursedFinger: return 28.0f;
    case WeaponVisual::ShieldAndSword: return 65.0f;
    case WeaponVisual::GuardianShield: return 36.0f;
    case WeaponVisual::KenpachiSword: return 96.0f;
    case WeaponVisual::Nozarashi: return 110.0f;
    case WeaponVisual::RedBankai: return 113.0f;
    case WeaponVisual::LuzDeLaLuna: return 104.0f;
    case WeaponVisual::SegundaCore: return 30.0f;
    case WeaponVisual::GreatSword: return 108.0f;
    case WeaponVisual::ChargeBlade: return 82.0f;
    case WeaponVisual::Gunlance: return 108.0f;
    case WeaponVisual::Kusanagi: return 70.0f;
    case WeaponVisual::KakashiKunai: return 28.0f;
    case WeaponVisual::SageScroll: return 30.0f;
    case WeaponVisual::SusanooBow: return 40.0f;
    default: return 30.0f;
  }
}

void drawWeaponStyled(WeaponVisual v, Vec2 grip, Vec2 dir, float scale, float alpha, float time,
                      const WeaponStyle& st) {
  if (v == WeaponVisual::None || alpha <= 0.0f) return;
  dir = dir.norm();
  if (dir.len2() < 0.5f) dir = {1.0f, 0.0f};
  WP p;
  p.o = grip;
  p.u = dir;
  p.v = Vec2{-dir.y, dir.x} * (st.side >= 0 ? 1.0f : -1.0f);
  p.s = scale;
  p.alpha = alpha;
  p.time = time;
  p.ghost = st.ghost;
  p.tint = st.tint;
  p.ow = std::max(1.0f, 2.3f * scale);
  if (const float px = pix::artUnit(); px > 0.0f) {  // pixel art: one-pixel ink
    p.ow = px * 0.9f;
    p.ink = pix::kInk;
  }
  if (st.glow > 0.0f && !st.ghost) elementAura(p, v, st.fx, st.glow);
  for (int pass = 0; pass < 2; ++pass) {
    p.line = pass == 0;
    drawVisual(v, p, st);
  }
}

void drawWeaponVisual(WeaponVisual v, Vec2 grip, Vec2 dir, float scale, float alpha, float time) {
  drawWeaponStyled(v, grip, dir, scale, alpha, time, WeaponStyle{});
}

void drawShieldVisual(bool guardian, Vec2 center, float radius, float open, Vec2 fold, float alpha, float time,
                      const WeaponStyle& st) {
  const float ow = std::max(1.0f, radius * 0.12f);
  if (guardian) guardianShieldDisc(center, radius, open, fold.norm(), alpha, ow, st.ghost, st.tint, time);
  else roundShieldDisc(center, radius, alpha, ow, st.ghost, st.tint);
}

bool isHandWeapon(WeaponVisual v) {
  switch (v) {
    case WeaponVisual::None: case WeaponVisual::Grimoire: case WeaponVisual::HollowPurple:
    case WeaponVisual::SixEyes: case WeaponVisual::CursedFinger: case WeaponVisual::GuardianShield:
    case WeaponVisual::BowOfLight: case WeaponVisual::SegundaCore: case WeaponVisual::SageScroll:
    case WeaponVisual::KuramaChakra: case WeaponVisual::SusanooBow: case WeaponVisual::Mangekyo:
    case WeaponVisual::SharinganEye: case WeaponVisual::DoubleMangekyo:
      return false;
    default: return true;
  }
}

WeaponVisual frontWeapon(const CharacterDef& def, const Pose& pose) {
  if (pose.hideWeapon) return WeaponVisual::None;
  if (isHandWeapon(pose.held)) return pose.held;
  if (!pose.weapon) return WeaponVisual::None;
  return isHandWeapon(def.baseWeapon) ? def.baseWeapon : WeaponVisual::None;
}

float frontWeaponLength(const CharacterDef& def, const Pose& pose) {
  const WeaponVisual v = frontWeapon(def, pose);
  if (v == WeaponVisual::None) return 0.0f;
  if (pose.bladeLen > 0.0f) return pose.bladeLen;  // the fighter's blade: exactly what its swings hit with
  if (v == def.baseWeapon && def.weaponLength > 0.0f) return def.weaponLength;
  return weaponVisualLength(v) * def.height / 100.0f;
}
