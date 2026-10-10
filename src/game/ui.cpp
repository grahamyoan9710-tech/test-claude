#include "game/ui.hpp"

#include <string>
#include <vector>

#include "game/render.hpp"
#include "game/settings.hpp"

namespace ui {
namespace {
float hash(int i) {
  uint32_t x = uint32_t(i) * 2654435761u + 0x9E37u;
  x ^= x >> 13;
  x *= 0x5bd1e995u;
  x ^= x >> 15;
  return float(x & 0xFFFF) / 65535.0f;
}
void vtx(Vector2 p, Color k) {
  rlColor4ub(k.r, k.g, k.b, k.a);
  rlVertex2f(p.x, p.y);
}
void triGrad(Vector2 a, Vector2 b, Vector2 c, Color ca, Color cb, Color cc) {
  rlBegin(RL_TRIANGLES);
  vtx(a, ca);
  vtx(b, cb);
  vtx(c, cc);
  rlEnd();
}
float easeInOut(float t) { return t < 0.5f ? 4 * t * t * t : 1 - std::pow(-2 * t + 2, 3) / 2; }
void roundLine(Vector2 a, Vector2 b, float t, Color col) {
  DrawLineEx(a, b, t, col);
  DrawCircleV(a, t * 0.5f, col);
  DrawCircleV(b, t * 0.5f, col);
}
// Radial gradient (float centre: no 1-px steps on slow movements); sy squashes it vertically.
void radial(Vector2 c, float r, Color in, Color out, int segs = 36, float sy = 1.0f) {
  rlBegin(RL_TRIANGLES);
  for (int i = 0; i < segs; ++i) {
    const float a0 = i * 2.0f * kPi / segs, a1 = (i + 1) * 2.0f * kPi / segs;
    vtx(c, in);
    vtx({c.x + std::cos(a1) * r, c.y + std::sin(a1) * r * sy}, out);
    vtx({c.x + std::cos(a0) * r, c.y + std::sin(a0) * r * sy}, out);
  }
  rlEnd();
}
// Filled ellipse, vertical gradient.
void ellipseGrad(Vector2 c, float rx, float ry, Color top, Color bottom, int segs = 48) {
  const Color mid = mixColor(top, bottom, 0.5f);
  rlBegin(RL_TRIANGLES);
  for (int i = 0; i < segs; ++i) {
    const float a0 = i * 2.0f * kPi / segs, a1 = (i + 1) * 2.0f * kPi / segs;
    vtx(c, mid);
    vtx({c.x + std::cos(a1) * rx, c.y + std::sin(a1) * ry}, mixColor(top, bottom, 0.5f + 0.5f * std::sin(a1)));
    vtx({c.x + std::cos(a0) * rx, c.y + std::sin(a0) * ry}, mixColor(top, bottom, 0.5f + 0.5f * std::sin(a0)));
  }
  rlEnd();
}
// Polyline as one mitred strip: no overlapping joints, so it stays clean when translucent.
void polyline(const Vector2* p, int n, float t, Color col) {
  if (n < 2) return;
  auto normal = [&](int i) {  // of segment i -> i + 1
    const float dx = p[i + 1].x - p[i].x, dy = p[i + 1].y - p[i].y, l = std::max(1e-4f, std::sqrt(dx * dx + dy * dy));
    return Vector2{-dy / l, dx / l};
  };
  Vector2 pl{}, pr{};
  rlBegin(RL_TRIANGLES);
  for (int i = 0; i < n; ++i) {
    Vector2 m = normal(std::min(i, n - 2));
    float len = t * 0.5f;
    if (i > 0 && i < n - 1) {
      const Vector2 a = normal(i - 1), b = normal(i);
      const float mx = a.x + b.x, my = a.y + b.y, ml = std::sqrt(mx * mx + my * my);
      if (ml > 1e-3f) {
        m = {mx / ml, my / ml};
        len /= std::max(0.35f, m.x * a.x + m.y * a.y);
      }
    }
    const Vector2 l{p[i].x + m.x * len, p[i].y + m.y * len}, r{p[i].x - m.x * len, p[i].y - m.y * len};
    if (i > 0) {
      vtx(pl, col), vtx(pr, col), vtx(r, col);
      vtx(pl, col), vtx(r, col), vtx(l, col);
    }
    pl = l;
    pr = r;
  }
  rlEnd();
}
// Elliptic arc (radians, y down: 0..pi is the front half).
void ellipseArc(Vector2 c, float rx, float ry, float a0, float a1, float t, Color col, int segs = 40) {
  Vector2 pts[129];
  segs = std::min(segs, 128);
  for (int i = 0; i <= segs; ++i) {
    const float a = a0 + (a1 - a0) * i / segs;
    pts[i] = {c.x + std::cos(a) * rx, c.y + std::sin(a) * ry};
  }
  polyline(pts, segs + 1, t, col);
}
void spiral(Vector2 c, float r, float turns, float rot, float t, Color col) {
  const int n = 32;
  Vector2 pts[n + 1];
  for (int i = 0; i <= n; ++i) {
    const float k = float(i) / n, a = rot + k * turns * 2.0f * kPi, rr = r * (0.08f + 0.92f * k);
    pts[i] = {c.x + std::cos(a) * rr, c.y + std::sin(a) * rr};
  }
  polyline(pts, n + 1, t, col);
}
}  // namespace

// ---- Shapes -------------------------------------------------------------------
void quad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color ca, Color cb, Color cc, Color cd) {
  rlBegin(RL_TRIANGLES);
  vtx(a, ca);
  vtx(d, cd);
  vtx(c, cc);
  vtx(a, ca);
  vtx(c, cc);
  vtx(b, cb);
  rlEnd();
}

void slanted(Rectangle r, float s, Color top, Color bottom) {
  quad({r.x + s, r.y}, {r.x + r.width, r.y}, {r.x + r.width - s, r.y + r.height}, {r.x, r.y + r.height}, top, top,
       bottom, bottom);
}

void slantedOutline(Rectangle r, float s, float t, Color c) {
  Vector2 p[4] = {{r.x + s, r.y}, {r.x + r.width, r.y}, {r.x + r.width - s, r.y + r.height}, {r.x, r.y + r.height}};
  for (int i = 0; i < 4; ++i) {
    DrawLineEx(p[i], p[(i + 1) % 4], t, c);
    DrawCircleV(p[i], t * 0.5f, c);
  }
}

Rectangle grow(Rectangle r, float px) { return {r.x - px, r.y - px, r.width + 2 * px, r.height + 2 * px}; }

void glow(Vector2 c, float radius, Color col, int layers) {
  // Two radial gradients instead of many stacked circles: same soft look, a fraction of the fill cost.
  (void)layers;
  const float a = col.a / 255.0f;
  const Color clear = withAlpha(col, 0.0f);
  radial(c, radius, withAlpha(col, a * 0.55f), clear);
  radial(c, radius * 0.5f, withAlpha(col, a * 0.45f), clear);
}

void sunburst(Vector2 c, float radius, int rays, float rot, Color col) {
  Color clear = withAlpha(col, 0.0f);
  for (int i = 0; i < rays; ++i) {
    float a0 = rot + i * 2.0f * kPi / rays;
    float a1 = a0 + kPi / rays;
    triGrad(c, {c.x + std::cos(a0) * radius, c.y + std::sin(a0) * radius},
            {c.x + std::cos(a1) * radius, c.y + std::sin(a1) * radius}, col, clear, clear);
  }
}

void vignette(float s) {
  const Color k{4, 8, 10, 255};
  Color dark = withAlpha(k, s), clear = withAlpha(k, 0.0f);
  DrawRectangleGradientV(0, 0, kScreenW, 150, dark, clear);
  DrawRectangleGradientV(0, kScreenH - 170, kScreenW, 170, clear, dark);
  DrawRectangleGradientH(0, 0, 200, kScreenH, dark, clear);
  DrawRectangleGradientH(kScreenW - 200, 0, 200, kScreenH, clear, dark);
}

void star(Vector2 c, float rOut, float rIn, float rot, Color col) {
  Vector2 pts[10];
  for (int i = 0; i < 10; ++i) {
    float a = rot + i * kPi / 5.0f - kPi / 2.0f;
    float r = (i % 2 == 0) ? rOut : rIn;
    pts[i] = {c.x + std::cos(a) * r, c.y + std::sin(a) * r};
  }
  for (int i = 0; i < 10; ++i) DrawTriangle(c, pts[i], pts[(i + 1) % 10], col);
}

void diamond(Vector2 c, float r, Color col) {
  quad({c.x, c.y - r}, {c.x + r, c.y}, {c.x, c.y + r}, {c.x - r, c.y}, col, col, col, col);
}

void corners(Rectangle r, float s, float len, float t, Color col) {
  const Vector2 p[4] = {{r.x + s, r.y}, {r.x + r.width, r.y}, {r.x + r.width - s, r.y + r.height}, {r.x, r.y + r.height}};
  for (int i = 0; i < 4; ++i) {
    Vector2 q[3] = {p[(i + 3) % 4], p[i], p[(i + 1) % 4]};
    for (int j : {0, 2}) {
      const float dx = q[j].x - p[i].x, dy = q[j].y - p[i].y, l = std::max(1.0f, std::sqrt(dx * dx + dy * dy));
      const float k = std::min(len, l * 0.45f) / l;
      q[j] = {p[i].x + dx * k, p[i].y + dy * k};
    }
    polyline(q, 3, t, col);
  }
}

void plate(Rectangle r, float s, Color top, Color bottom, Color line) {
  slanted({r.x + 6, r.y + 8, r.width, r.height}, s, withAlpha(BLACK, 0.35f), withAlpha(BLACK, 0.35f));
  slanted(r, s, top, bottom);
  slantedOutline(r, s, 1.5f, withAlpha(line, line.a / 255.0f * 0.6f));
  corners(r, s, std::min(18.0f, r.height * 0.3f), 2.5f, line);
}

void rule(float x0, float x1, float y, Color col) {
  DrawLineEx({x0 + 5, y}, {x1 - 5, y}, 1.5f, col);
  diamond({x0, y}, 4, col);
  diamond({x1, y}, 4, col);
}

void rune(Vector2 c, float r, int kind, float rot, Color col, float t) {
  DrawRing(c, r - t, r, 0, 360, 32, col);
  switch (kind & 3) {
    case 0: spiral(c, r * 0.62f, 1.6f, rot, t, col); break;
    case 1: {  // eye: lens + pupil
      Vector2 pts[17];
      for (int i = 0; i <= 16; ++i) {
        const float k = i <= 8 ? i / 8.0f : (16 - i) / 8.0f, sgn = i <= 8 ? -1.0f : 1.0f;
        pts[i] = {c.x - r * 0.62f + k * r * 1.24f, c.y + sgn * r * 0.34f * std::sin(k * kPi)};
      }
      polyline(pts, 17, t, col);
      DrawCircleV(c, r * 0.16f + t * 0.3f, col);
      break;
    }
    case 2:
      for (int i = 0; i < 3; ++i) {
        const float a = rot + i * 2.0f * kPi / 3.0f;
        DrawCircleV({c.x + std::cos(a) * r * 0.42f, c.y + std::sin(a) * r * 0.42f}, r * 0.16f + t * 0.2f, col);
      }
      break;
    default:
      for (int i = 0; i < 4; ++i) {
        const float a = rot + i * kPi * 0.5f;
        const Vector2 d{std::cos(a), std::sin(a)};
        roundLine({c.x + d.x * r * 0.3f, c.y + d.y * r * 0.3f}, {c.x + d.x * r * 0.62f, c.y + d.y * r * 0.62f}, t, col);
      }
      DrawCircleV(c, r * 0.12f + t * 0.3f, col);
      break;
  }
}

void zonaiRing(Vector2 c, float r, float time, float alpha) {
  auto A = [&](Color k, float a) { return withAlpha(k, a * alpha); };
  const Color pale = mixColor(kTeal, WHITE, 0.55f);
  glow(c, r * 1.35f, A(kTeal, 0.5f));
  // Outer band: dark stone between two glowing edges, rune marks turning slowly.
  DrawRing(c, r * 0.84f, r, 0, 360, 120, A(kInk, 0.42f));
  DrawRing(c, r - 2.0f, r, 0, 360, 120, A(pale, 0.75f));
  DrawRing(c, r * 0.84f, r * 0.84f + 1.5f, 0, 360, 120, A(kTeal, 0.6f));
  const float rot = time * 0.06f;
  for (int i = 0; i < 24; ++i) {
    const float a = rot + i * 2.0f * kPi / 24.0f;
    const Vector2 d{std::cos(a), std::sin(a)}, p{c.x + d.x * r * 0.92f, c.y + d.y * r * 0.92f};
    if (i % 6 == 0) {
      rune(p, r * 0.062f, i / 6, a, A(pale, 0.95f), 2.0f);
    } else if (i % 2 == 0) {
      DrawCircleV(p, r * 0.016f + 1.0f, A(kTeal, 0.9f));
    } else {
      roundLine({p.x - d.x * r * 0.03f, p.y - d.y * r * 0.03f}, {p.x + d.x * r * 0.03f, p.y + d.y * r * 0.03f}, 2.5f,
                A(kTeal, 0.75f));
    }
  }
  // Segmented middle ring turning the other way, then a hairline with three diamonds.
  const float rot2 = -time * 6.0f;
  for (int i = 0; i < 6; ++i)
    DrawRing(c, r * 0.7f, r * 0.74f, rot2 + i * 60.0f + 6.0f, rot2 + i * 60.0f + 50.0f, 14, A(kTeal, 0.55f));
  DrawRing(c, r * 0.6f - 1.0f, r * 0.6f + 1.0f, 0, 360, 96, A(pale, 0.35f));
  for (int i = 0; i < 3; ++i) {
    const float a = time * 0.2f + i * 2.0f * kPi / 3.0f;
    diamond({c.x + std::cos(a) * r * 0.6f, c.y + std::sin(a) * r * 0.6f}, r * 0.03f, A(pale, 0.9f));
  }
}

void stoneDisc(Vector2 c, float rx, float ry, float time, bool rock) {
  const float th = ry * 0.85f;  // slab thickness
  const float pulse = 0.5f + 0.5f * std::sin(time * 1.6f);
  const Color top{170, 162, 134, 255}, topDk{120, 118, 98, 255}, side{88, 90, 80, 255}, sideDk{40, 46, 44, 255};
  const Color moss{78, 116, 58, 255}, mossHi{138, 170, 84, 255};
  radial({c.x, c.y + th + ry * 0.4f}, rx * 1.08f, withAlpha(kTeal, 0.22f + 0.08f * pulse), withAlpha(kTeal, 0.0f), 36,
         ry * 2.6f / rx);
  if (rock) {  // the floating rock under the slab
    const int n = 9;
    Vector2 prev{c.x - rx * 0.97f, c.y + th};
    for (int i = 1; i <= n; ++i) {
      const float k = float(i) / n, x = c.x - rx * 0.97f + k * rx * 1.94f;
      const float depth = ry * (1.2f + 2.6f * std::sin(k * kPi)) * (0.8f + 0.4f * hash(i * 7));
      const Vector2 p{x, c.y + th + (i == n ? 0.0f : depth)};
      triGrad({prev.x, c.y + th}, {p.x, c.y + th}, p, side, side, sideDk);
      triGrad({prev.x, c.y + th}, p, prev, side, sideDk, i == 1 ? side : sideDk);
      prev = p;
    }
  }
  ellipseGrad({c.x, c.y + th}, rx, ry, side, sideDk);
  DrawRectangleGradientV(int(c.x - rx), int(c.y), int(rx * 2), int(th), side, mixColor(side, sideDk, 0.5f));
  // Carved slots on the side, glowing
  const int slots = std::max(5, int(rx / 34.0f));
  for (int i = 0; i < slots; ++i) {
    const float a = (i + 0.5f) / slots * kPi, k = std::sin(a);
    const float x = c.x + std::cos(a) * rx * 0.97f, y = c.y + k * ry + th * 0.5f, w = std::max(2.0f, k * rx * 0.07f);
    DrawRectangleRec({x - w * 0.5f, y - 1.5f, w, 3.0f}, withAlpha(kTeal, (0.35f + 0.45f * pulse) * k));
  }
  ellipseGrad(c, rx, ry, top, topDk);
  ellipseArc(c, rx * 0.8f, ry * 0.8f, 0.0f, 2.0f * kPi, 2.0f, withAlpha(kInk, 0.3f), 56);
  ellipseArc(c, rx * 0.8f, ry * 0.8f + 1.5f, 0.15f, kPi - 0.15f, 1.5f, withAlpha(kTeal, 0.25f + 0.25f * pulse), 28);
  ellipseArc(c, rx * 0.46f, ry * 0.46f, 0.0f, 2.0f * kPi, 1.5f, withAlpha(kInk, 0.22f), 40);
  ellipseArc(c, rx, ry, kPi + 0.05f, 2.0f * kPi - 0.05f, 1.5f, withAlpha(WHITE, 0.3f), 40);
  ellipseArc(c, rx, ry, 0.03f, kPi - 0.03f, 2.5f, withAlpha(kTeal, 0.45f + 0.35f * pulse), 40);
  // Moss growing over the front rim, a few strands hanging down
  const int tufts = std::max(3, int(rx / 50.0f));
  const float w = std::max(3.0f, ry * 0.22f);
  for (int i = 0; i < tufts; ++i) {
    const float a = (i + 0.15f + 0.7f * hash(i * 13)) / tufts * kPi, da = (0.03f + 0.04f * hash(i * 5)) * 600.0f / rx;
    for (int pass = 0; pass < 2; ++pass)
      for (int j = 0; j < 6; ++j) {
        const float b = a - da + 2.0f * da * (j + 0.5f) / 6.0f, br = w * (0.45f + 0.35f * hash(i * 31 + j));
        const Vector2 p{c.x + std::cos(b) * rx, c.y + std::sin(b) * ry + w * 0.15f};
        if (pass == 0) DrawCircleV(p, br, moss);
        else DrawCircleV({p.x - br * 0.2f, p.y - br * 0.4f}, br * 0.5f, mossHi);
      }
    if (rock && hash(i * 3) > 0.35f) {
      const Vector2 d{c.x + std::cos(a) * rx, c.y + std::sin(a) * ry + w * 0.5f};
      roundLine(d, {d.x + 1.5f, d.y + th * (0.5f + 0.6f * hash(i * 17))}, 2.0f, moss);
    }
  }
}

// ---- Backgrounds ----------------------------------------------------------------
void motes(int count, float time, Color col, float alpha) {
  const float H = kScreenH + 60.0f;
  for (int i = 0; i < count; ++i) {
    const float speed = 9.0f + 20.0f * hash(i * 3);
    const float x = hash(i) * kScreenW + std::sin(time * 0.5f + i * 1.7f) * 26.0f;
    const float y = kScreenH + 30 - std::fmod(time * speed + hash(i + 77) * H, H);
    const float tw = 0.5f + 0.5f * std::sin(time * 2.3f + i * 2.1f), r = 1.3f + 1.8f * hash(i * 5);
    radial({x, y}, r * 4.5f, withAlpha(col, 0.2f * tw * alpha), withAlpha(col, 0.0f), 12);
    DrawCircleV({x, y}, r, withAlpha(mixColor(col, WHITE, 0.55f), (0.35f + 0.5f * tw) * alpha));
  }
}

void backdrop(float time, Color top, Color bottom, Color moteCol) {
  DrawRectangleGradientV(0, 0, kScreenW, kScreenH, top, bottom);
  for (int i = 0; i < 6; ++i) {  // soft cloud banks drifting by
    const float w = 420.0f + 260.0f * hash(i * 9), x = std::fmod(i * 310.0f + time * (6.0f + 4.0f * hash(i)), kScreenW + w) - w * 0.5f;
    radial({x, 180.0f + i * 90.0f}, w * 0.5f, withAlpha(kCream, 0.06f), withAlpha(kCream, 0.0f), 28, 0.22f);
  }
  motes(36, time, moteCol);
  vignette(0.5f);
}

void stageBackdrop(int stage, float time, float dim) {
  const auto& list = stageList();
  const StageDef& s = list[std::clamp(stage, 0, int(list.size()) - 1)];
  drawBackground(s, {std::sin(time * 0.045f) * 800.0f, -150.0f}, 1.0f, time);
  if (dim > 0.0f)
    DrawRectangleGradientV(0, 0, kScreenW, kScreenH, withAlpha(kInk, dim * 0.8f), withAlpha(kInk, std::min(1.0f, dim * 1.12f)));
  motes(dim > 0.0f ? 30 : 16, time, kTeal, 0.6f + 0.4f * dim);
  vignette(0.25f + 0.4f * dim);
}

// ---- Text -----------------------------------------------------------------------
float logoWidth(const char* text, float size) { return textWidth(text, size, FontKind::Title); }

void logoText(const char* text, float x, float y, float size, Color light, Color mid, Color dark, Color outline,
              Color depth, int layers) {
  const FontKind T = FontKind::Title;
  const float th = std::max(2.0f, size * 0.055f);
  // Extrusion: each depth layer is the outlined silhouette (8 offset copies cover the inside too).
  for (int i = layers; i >= 1; --i) {
    const float ox = x + i * size * 0.010f, oy = y + i * size * 0.014f;
    for (int d = 0; d < 8; ++d) {
      const float a = d * kPi / 4.0f;
      drawText(text, ox + std::cos(a) * th, oy + std::sin(a) * th, size, depth, T);
    }
  }
  drawTextOutlined(text, x, y, size, dark, outline, th, T);
  const float w = logoWidth(text, size);
  beginClip({x - th, y - size, w + th * 2, size * 1.62f});
  drawText(text, x, y, size, mid, T);
  endClip();
  beginClip({x - th, y - size, w + th * 2, size * 1.40f});
  drawText(text, x, y, size, light, T);
  endClip();
}

// ---- Baked sprites ------------------------------------------------------------------
namespace {
struct Baked {
  RenderTexture2D rt{};
  float size = 0, pad = 0, w = 0, h = 0;  // virtual units
};
Baked g_sprites[int(Sprite::Count)];
constexpr float kBakeScale = 2.0f;

struct SpriteSpec {
  const char* text;
  float size;
  Color light, mid, dark, outline, depth;
  int layers;
};
// Gold and parchment with a warm umber extrusion (TotK title look).
constexpr Color kGoldLight{255, 246, 212, 255}, kGoldMid{242, 208, 122, 255}, kGoldDark{204, 146, 64, 255};
constexpr Color kDepth{36, 20, 12, 255};
const SpriteSpec kSpecs[int(Sprite::Count)] = {
    {"CLASH BROS.", 150, kGoldLight, kGoldMid, kGoldDark, kUmber, kDepth, 9},
    {"COMBAT", 92, {255, 252, 240, 255}, {250, 236, 204, 255}, {236, 206, 150, 255}, kUmber, kDepth, 5},
    {"VICTOIRE !", 92, kGoldLight, kGoldMid, kGoldDark, kUmber, kDepth, 7},
    {"PAUSE", 96, {255, 252, 240, 255}, {248, 232, 196, 255}, {226, 194, 128, 255}, kUmber, kDepth, 6},
    {"GAME !", 260, kGoldLight, kGoldMid, kGoldDark, kUmber, kDepth, 9},
    {"3", 240, {255, 228, 196, 255}, {240, 164, 104, 255}, {190, 96, 56, 255}, kUmber, kDepth, 7},
    {"2", 240, kGoldLight, kGoldMid, kGoldDark, kUmber, kDepth, 7},
    {"1", 240, {255, 252, 236, 255}, {246, 232, 188, 255}, {214, 184, 112, 255}, kUmber, kDepth, 7},
    {"GO !", 230, kGoldLight, kGoldMid, kGoldDark, kUmber, kDepth, 7},
    {"PRÊTS ?", 96, {255, 252, 240, 255}, {248, 236, 206, 255}, {226, 202, 150, 255}, kUmber, kDepth, 7},
    {"MATCH NUL !", 80, {250, 246, 236, 255}, {226, 224, 210, 255}, {184, 184, 168, 255}, kUmber, kDepth, 5},
};}  // namespace

void frameBlend() {
  rlSetBlendFactorsSeparate(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_ONE, RL_ONE_MINUS_SRC_ALPHA, RL_FUNC_ADD,
                            RL_FUNC_ADD);
  BeginBlendMode(BLEND_CUSTOM_SEPARATE);
}

void bakeSprites() {
  const float saved = g_renderScale;
  g_renderScale = kBakeScale;  // beginClip() inside logoText uses it
  for (int i = 0; i < int(Sprite::Count); ++i) {
    const SpriteSpec& sp = kSpecs[i];
    Baked& b = g_sprites[i];
    b.size = sp.size;
    b.pad = sp.size * 0.3f;
    b.w = logoWidth(sp.text, sp.size) + b.pad * 2;
    b.h = sp.size * 1.3f + b.pad * 2;
    b.rt = LoadRenderTexture(int(b.w * kBakeScale), int(b.h * kBakeScale));
    SetTextureFilter(b.rt.texture, TEXTURE_FILTER_BILINEAR);
    BeginTextureMode(b.rt);
    ClearBackground(BLANK);
    rlScalef(kBakeScale, kBakeScale, 1.0f);
    frameBlend();  // "over" for alpha too: the texture ends up premultiplied
    logoText(sp.text, b.pad, b.pad, sp.size, sp.light, sp.mid, sp.dark, sp.outline, sp.depth, sp.layers);
    EndBlendMode();
    EndTextureMode();
    GenTextureMipmaps(&b.rt.texture);
    SetTextureFilter(b.rt.texture, TEXTURE_FILTER_TRILINEAR);
  }
  g_renderScale = saved;
}

void unloadSprites() {
  for (auto& b : g_sprites)
    if (b.rt.id != 0) UnloadRenderTexture(b.rt);
}

float spriteWidth(Sprite s, float size) {
  const Baked& b = g_sprites[int(s)];
  return b.size > 0 ? (b.w - b.pad * 2) * size / b.size : 0.0f;
}

void drawSprite(Sprite s, float x, float y, float size, float alpha) {
  const Baked& b = g_sprites[int(s)];
  if (b.rt.id == 0) return;
  const float k = size / b.size;
  const Rectangle src{0, 0, float(b.rt.texture.width), -float(b.rt.texture.height)};
  const Rectangle dst{x - b.pad * k, y - b.pad * k, b.w * k, b.h * k};
  const unsigned char a = (unsigned char)(clampf(alpha, 0.0f, 1.0f) * 255.0f);
  BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
  DrawTexturePro(b.rt.texture, src, dst, {0, 0}, 0.0f, {a, a, a, a});
  EndBlendMode();
  frameBlend();
}

void header(const char* title, const char* subtitle, Color accent, float time) {
  const Rectangle r{-60, 24, 640, 72};
  const float s = 40;
  slanted({r.x + 8, r.y + 8, r.width, r.height}, s, withAlpha(BLACK, 0.35f), withAlpha(BLACK, 0.35f));
  slanted(r, s, withAlpha({32, 50, 54, 255}, 0.96f), withAlpha(kInk, 0.96f));
  DrawRectangleGradientH(0, int(r.y), 340, int(r.height), withAlpha(accent, 0.38f), withAlpha(accent, 0.0f));
  DrawLineEx({0, r.y + r.height - 7}, {r.x + r.width - s * (r.height - 7) / r.height - 9, r.y + r.height - 7}, 1.0f,
             withAlpha(kGold, 0.35f));
  slantedOutline(r, s, 2.0f, kGold);
  rune({r.x + r.width - 86, r.y + r.height * 0.5f}, 17, 0, time * 0.4f, withAlpha(kTeal, 0.6f), 2.0f);
  // Trailing slashes: the screen's accent, then a gold one.
  const Rectangle a{r.x + r.width + 10, r.y, 24, r.height};
  slanted(a, s, accent, shade(accent, 0.6f));
  slantedOutline(a, s, 1.5f, withAlpha(kGold, 0.8f));
  slanted({r.x + r.width + 44, r.y, 6, r.height}, s, kGold, shade(kGold, 0.7f));
  drawTextOutlined(title, 36, r.y + 10, 52, kCream, kInk, 4.0f, FontKind::Title);
  if (subtitle) drawTextOutlined(subtitle, 44, r.y + r.height + 10, 19, kMuted, kInk, 2.0f);
}

float keycapWidth(const char* label, float h) { return std::max(h, textWidth(label, h * 0.48f) + h * 0.7f); }

float keycap(const char* label, float x, float y, float h, bool lit) {
  float ts = h * 0.48f;
  float w = keycapWidth(label, h);
  const Color face = lit ? kGold : Color{236, 226, 200, 255};
  DrawRectangleRounded({x, y + 3, w, h}, 0.35f, 6, shade(face, 0.5f));
  DrawRectangleRounded({x, y, w, h}, 0.35f, 6, face);
  drawText(label, x + (w - textWidth(label, ts)) * 0.5f, y + (h - ts) * 0.5f - 1, ts, kInk);
  return w;
}

// ---- Button prompts -------------------------------------------------------------------
namespace {
const Color kPlastic = {40, 48, 52, 255};    // dark buttons
const Color kRim = {140, 158, 154, 255};     // their edge (visible on dark backgrounds)
const Color kLight = {234, 230, 218, 255};   // shoulders, sticks, d-pad

// Text centred on c (capitals look centred).
void centred(const char* s, Vector2 c, float size, Color col) {
  drawText(s, c.x - textWidth(s, size) * 0.5f, c.y - size * 0.5f, size, col);
}
// Dark round body with a light rim, and its drop shadow.
void darkDisc(Vector2 c, float r, Color body = kPlastic) {
  DrawCircleV({c.x, c.y + r * 0.16f}, r, shade(body, 0.45f));
  DrawCircleV(c, r, kRim);
  DrawCircleV(c, r - std::max(1.2f, r * 0.1f), body);
}

// Face button at its position on the pad: 0 bottom, 1 right, 2 left, 3 top.
void faceButton(int pos, Vector2 c, float r, PadFamily f) {
  if (f == PadFamily::Xbox) {
    static const Color cols[4] = {{82, 178, 60, 255}, {228, 60, 56, 255}, {46, 122, 232, 255}, {240, 182, 28, 255}};
    static const char* letters[4] = {"A", "B", "X", "Y"};
    const Color col = cols[pos];
    DrawCircleV({c.x, c.y + r * 0.16f}, r, shade(col, 0.45f));
    DrawCircleV(c, r, shade(col, 0.8f));
    DrawCircleV({c.x, c.y - r * 0.07f}, r * 0.9f, col);
    const float size = r * 1.75f;
    const Vector2 tc{c.x, c.y + r * 0.02f};
    drawTextOutlined(letters[pos], tc.x - textWidth(letters[pos], size) * 0.5f, tc.y - size * 0.5f, size, WHITE,
                     shade(col, 0.4f), std::max(1.0f, r * 0.08f));
  } else if (f == PadFamily::PlayStation) {
    darkDisc(c, r);
    const float t = std::max(1.6f, r * 0.17f);
    if (pos == 0) {  // cross
      const Color col = {124, 178, 242, 255};
      const float s = r * 0.4f;
      roundLine({c.x - s, c.y - s}, {c.x + s, c.y + s}, t, col);
      roundLine({c.x - s, c.y + s}, {c.x + s, c.y - s}, t, col);
    } else if (pos == 1) {  // circle
      DrawRing(c, r * 0.44f - t * 0.5f, r * 0.44f + t * 0.5f, 0, 360, 40, {255, 98, 106, 255});
    } else if (pos == 2) {  // square
      const Color col = {242, 130, 210, 255};
      const float s = r * 0.38f;
      const Vector2 p[4] = {{c.x - s, c.y - s}, {c.x + s, c.y - s}, {c.x + s, c.y + s}, {c.x - s, c.y + s}};
      for (int i = 0; i < 4; ++i) roundLine(p[i], p[(i + 1) % 4], t, col);
    } else {  // triangle
      const Color col = {66, 218, 178, 255};
      const Vector2 p[3] = {{c.x, c.y - r * 0.48f}, {c.x + r * 0.47f, c.y + r * 0.33f}, {c.x - r * 0.47f, c.y + r * 0.33f}};
      for (int i = 0; i < 3; ++i) roundLine(p[i], p[(i + 1) % 3], t, col);
    }
  } else {  // Nintendo: the letters sit elsewhere (bottom B, right A, left Y, top X)
    static const char* letters[4] = {"B", "A", "Y", "X"};
    darkDisc(c, r, {56, 64, 68, 255});
    centred(letters[pos], {c.x, c.y + r * 0.02f}, r * 1.7f, kCream);
  }
}

const char* shoulderText(Btn b, PadFamily f) {
  static const char* t[3][4] = {{"LB", "RB", "LT", "RT"}, {"L1", "R1", "L2", "R2"}, {"L", "R", "ZL", "ZR"}};
  return t[int(f)][int(b) - int(Btn::LB)];
}
float shoulderWidth(Btn b, float h, PadFamily f) {
  return std::max(h * 1.45f, textWidth(shoulderText(b, f), h * 0.52f) + h * 0.7f);
}

// Rounded tab: bumpers are flat pills, triggers have a domed top.
void shoulder(Btn b, float x, float y, float h, PadFamily f) {
  const float w = shoulderWidth(b, h, f);
  const bool trigger = b == Btn::LT || b == Btn::RT;
  const Color shadow = shade(kLight, 0.5f);
  if (trigger) {
    DrawRectangleRounded({x, y + 3, w, h * 0.7f}, 1.0f, 12, shadow);
    DrawRectangleRounded({x, y + h * 0.3f + 3, w, h * 0.7f}, 0.3f, 6, shadow);
    DrawRectangleRounded({x, y, w, h * 0.7f}, 1.0f, 12, kLight);
    DrawRectangleRounded({x, y + h * 0.3f, w, h * 0.7f}, 0.3f, 6, kLight);
    DrawRectangleRounded({x + w * 0.2f, y + h * 0.1f, w * 0.6f, h * 0.12f}, 1.0f, 6, withAlpha(WHITE, 0.7f));
  } else {
    const float top = y + h * 0.12f, hh = h * 0.76f;
    DrawRectangleRounded({x, top + 3, w, hh}, 0.6f, 10, shadow);
    DrawRectangleRounded({x, top, w, hh}, 0.6f, 10, kLight);
    DrawRectangleRounded({x + 3, top + hh * 0.62f, w - 6, hh * 0.3f}, 1.0f, 8, shade(kLight, 0.86f));
  }
  centred(shoulderText(b, f), {x + w * 0.5f, y + h * (trigger ? 0.56f : 0.47f)}, h * 0.52f, kInk);
}

void stickGlyph(Vector2 c, float r, const char* letter) {
  DrawCircleV({c.x, c.y + r * 0.16f}, r, shade(kPlastic, 0.45f));
  DrawCircleV(c, r, kPlastic);
  DrawCircleV(c, r * 0.8f, kLight);
  DrawRing(c, r * 0.6f, r * 0.68f, 0, 360, 36, shade(kLight, 0.78f));
  centred(letter, {c.x, c.y + r * 0.03f}, r * 1.05f, kInk);
}

// lit: arrows drawn dark (1 up, 2 down, 4 left, 8 right); the others are faded.
void dpadGlyph(Vector2 c, float h, int lit) {
  const float t = h * 0.36f, L = h * 0.98f;
  const Rectangle v{c.x - t * 0.5f, c.y - L * 0.5f, t, L}, hz{c.x - L * 0.5f, c.y - t * 0.5f, L, t};
  const Color shadow = shade(kLight, 0.5f);
  DrawRectangleRounded({v.x, v.y + 3, v.width, v.height}, 0.35f, 6, shadow);
  DrawRectangleRounded({hz.x, hz.y + 3, hz.width, hz.height}, 0.35f, 6, shadow);
  DrawRectangleRounded(v, 0.35f, 6, kLight);
  DrawRectangleRounded(hz, 0.35f, 6, kLight);
  DrawCircleV(c, t * 0.26f, shade(kLight, 0.82f));
  const Vector2 dirs[4] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
  for (int i = 0; i < 4; ++i) {
    const Vector2 d = dirs[i], n = {-d.y, d.x};
    const float tipD = L * 0.5f - t * 0.2f, baseD = tipD - t * 0.42f, s = t * 0.3f;
    const Vector2 tip{c.x + d.x * tipD, c.y + d.y * tipD};
    const Vector2 b1{c.x + d.x * baseD + n.x * s, c.y + d.y * baseD + n.y * s};
    const Vector2 b2{c.x + d.x * baseD - n.x * s, c.y + d.y * baseD - n.y * s};
    const Color col = (lit >> i) & 1 ? kInk : shade(kLight, 0.72f);
    DrawTriangle(tip, b1, b2, col);  // back-face culling is off: either winding shows
  }
}

// Start / Menu / Options / + button.
void startGlyph(float x, float y, float h, PadFamily f, bool back = false) {
  const float cy = y + h * 0.5f;
  if (f == PadFamily::PlayStation) {
    const float w = h * 1.1f, hh = h * 0.6f;
    const Rectangle r{x, cy - hh * 0.5f, w, hh};
    DrawRectangleRounded({r.x, r.y + 3, r.width, r.height}, 1.0f, 10, shade(kPlastic, 0.45f));
    DrawRectangleRounded(r, 1.0f, 10, kRim);
    DrawRectangleRounded(grow(r, -std::max(1.2f, h * 0.05f)), 1.0f, 10, kPlastic);
  } else {
    darkDisc({x + h * 0.45f, cy}, h * 0.42f);
  }
  const Vector2 c{x + (f == PadFamily::PlayStation ? h * 0.55f : h * 0.45f), cy};
  const float t = std::max(1.5f, h * 0.075f);
  if (back) {  // Back / View / Share / "-"
    if (f == PadFamily::Nintendo) {
      roundLine({c.x - h * 0.17f, c.y}, {c.x + h * 0.17f, c.y}, t * 1.3f, kCream);
    } else {  // two overlapping windows
      DrawRectangleLinesEx({c.x - h * 0.17f, c.y - h * 0.15f, h * 0.22f, h * 0.18f}, t * 0.8f, kCream);
      DrawRectangleLinesEx({c.x - h * 0.05f, c.y - h * 0.03f, h * 0.22f, h * 0.18f}, t * 0.8f, kCream);
    }
  } else if (f == PadFamily::Nintendo) {
    roundLine({c.x - h * 0.17f, c.y}, {c.x + h * 0.17f, c.y}, t * 1.3f, kCream);
    roundLine({c.x, c.y - h * 0.17f}, {c.x, c.y + h * 0.17f}, t * 1.3f, kCream);
  } else {  // "menu" lines
    for (int i = -1; i <= 1; ++i)
      roundLine({c.x - h * 0.15f, c.y + i * h * 0.12f}, {c.x + h * 0.15f, c.y + i * h * 0.12f}, t, kCream);
  }
}
}  // namespace

float padGlyphWidth(Btn b, float h, PadFamily f) {
  switch (b) {
    case Btn::None: return 0.0f;
    case Btn::LB: case Btn::RB: case Btn::LT: case Btn::RT: return shoulderWidth(b, h, f);
    case Btn::Start: case Btn::Back: return f == PadFamily::PlayStation ? h * 1.1f : h * 0.9f;
    case Btn::Move: return h * 2.15f;
    default: return h;
  }
}

float padGlyph(Btn b, float x, float y, float h, PadFamily f) {
  const Vector2 c{x + h * 0.5f, y + h * 0.5f};
  const float r = h * 0.48f;
  switch (b) {
    case Btn::None: break;
    case Btn::A: faceButton(0, c, r, f); break;
    case Btn::B: faceButton(1, c, r, f); break;
    case Btn::X: faceButton(2, c, r, f); break;
    case Btn::Y: faceButton(3, c, r, f); break;
    case Btn::LB: case Btn::RB: case Btn::LT: case Btn::RT: shoulder(b, x, y, h, f); break;
    case Btn::Start: startGlyph(x, y, h, f); break;
    case Btn::Back: startGlyph(x, y, h, f, true); break;
    case Btn::LStick: stickGlyph(c, r, "L"); break;
    case Btn::RStick: stickGlyph(c, r, "R"); break;
    case Btn::DPad: dpadGlyph(c, h, 15); break;
    case Btn::DPadUD: dpadGlyph(c, h, 3); break;
    case Btn::DPadLR: dpadGlyph(c, h, 12); break;
    case Btn::DPadDown: dpadGlyph(c, h, 2); break;
    case Btn::Move:
      stickGlyph(c, r, "L");
      dpadGlyph({x + h * 1.65f, c.y}, h, 15);
      break;
  }
  return padGlyphWidth(b, h, f);
}

float promptWidth(const char* key, Btn pad, float h) {
  if (padPrompts() && pad != Btn::None) return padGlyphWidth(pad, h, promptFamily());
  return keycapWidth(key, h);
}

float prompt(const char* key, Btn pad, float x, float y, float h, bool lit) {
  if (!padPrompts() || pad == Btn::None) return keycap(key, x, y, h, lit);
  const PadFamily f = promptFamily();
  if (lit) glow({x + padGlyphWidth(pad, h, f) * 0.5f, y + h * 0.5f}, h * 1.2f, kGold);
  return padGlyph(pad, x, y, h, f);
}

Btn padButtonFor(int action) {
  switch (action) {
    case kActJump: return Btn::X;
    case kActAttack: return Btn::A;
    case kActSpecial: return Btn::B;
    case kActShield: return Btn::RT;
    case kActGrab: return Btn::LB;
    case kActSmash: return Btn::RStick;
    case kActUltimate: return Btn::RB;
    default: return Btn::LStick;  // directions
  }
}

namespace {
PadFamily deviceFamily(int device) {
  const int p = device - kPad1;
  return padConnected(p) ? padFamily(p) : promptFamily();
}
std::string actionKey(int device, int action) { return keyName(settings().keys[device][action]); }
}  // namespace

// Pads without triggers shield with RB and unleash the ultimate with Back.
Btn deviceButtonFor(int device, int action) {
  const int p = device - kPad1;
  if (padConnected(p) && !padHasTriggers(p)) {
    if (action == kActShield) return Btn::RB;
    if (action == kActUltimate) return Btn::Back;
  }
  return padButtonFor(action);
}

float actionPromptWidth(int device, int action, float h) {
  if (action < 0 || action >= kActionCount || device < 0 || device >= kDeviceCount) return 0.0f;
  if (device >= kPad1) return padGlyphWidth(deviceButtonFor(device, action), h, deviceFamily(device));
  return keycapWidth(actionKey(device, action).c_str(), h);
}

float actionPrompt(int device, int action, float x, float y, float h, bool lit) {
  if (action < 0 || action >= kActionCount || device < 0 || device >= kDeviceCount) return 0.0f;
  if (device < kPad1) return keycap(actionKey(device, action).c_str(), x, y, h, lit);
  const Btn b = deviceButtonFor(device, action);
  const PadFamily f = deviceFamily(device);
  if (lit) glow({x + padGlyphWidth(b, h, f) * 0.5f, y + h * 0.5f}, h * 1.2f, kGold);
  return padGlyph(b, x, y, h, f);
}

void footer(std::initializer_list<Hint> hints, const char* info) {
  const float h = 46, y = kScreenH - h, kh = 28;
  DrawRectangleGradientV(0, int(y - 30), kScreenW, 30, withAlpha(kInk, 0.0f), withAlpha(kInk, 0.6f));
  DrawRectangle(0, int(y), kScreenW, int(h), withAlpha(kInk, 0.9f));
  DrawRectangleGradientH(0, int(y), kScreenW / 2, 2, withAlpha(kGold, 0.1f), withAlpha(kGold, 0.85f));
  DrawRectangleGradientH(kScreenW / 2, int(y), kScreenW / 2, 2, withAlpha(kGold, 0.85f), withAlpha(kGold, 0.1f));
  diamond({kScreenW * 0.5f, y + 1}, 5, kGold);
  // Measure from the right
  float total = 0;
  for (const Hint& hint : hints) total += promptWidth(hint.key, hint.pad, kh) + 8 + textWidth(hint.label, 18) + 26;
  float x = kScreenW - 24 - total;
  for (const Hint& hint : hints) {
    x += prompt(hint.key, hint.pad, x, y + 9, kh) + 8;
    drawText(hint.label, x, y + 13, 18, kCream);
    x += textWidth(hint.label, 18) + 26;
  }
  if (info) drawText(info, 24, y + 13, 18, kMuted);
}

// ---- Widgets ----------------------------------------------------------------------
void panel(Rectangle r, Color fill, Color border, float roundness) {
  DrawRectangleRounded({r.x + 6, r.y + 8, r.width, r.height}, roundness, 8, withAlpha(BLACK, 0.35f));
  DrawRectangleRounded(r, roundness, 8, fill);
  DrawRectangleRoundedLinesEx(r, roundness, 8, 2.0f, border);
  // Inner hairline with a diamond on each corner (TotK frame).
  const Rectangle in = grow(r, -6);
  const float k = std::min(r.width, r.height) / std::min(in.width, in.height);
  DrawRectangleRoundedLinesEx(in, roundness * k, 8, 1.0f, withAlpha(border, border.a / 255.0f * 0.35f));
  const float rad = roundness * k * std::min(in.width, in.height) * 0.5f * 0.3f;
  for (int i = 0; i < 4; ++i)
    diamond({i % 2 ? in.x + in.width - rad : in.x + rad, i / 2 ? in.y + in.height - rad : in.y + rad}, 4, border);
}

void focusFrame(Rectangle r, float time, float roundness) {
  const float pulse = 0.5f + 0.5f * std::sin(time * 6.0f);
  DrawRectangleRoundedLinesEx(grow(r, 5 + pulse * 2), roundness, 8, 5, withAlpha(kTeal, 0.14f + 0.14f * pulse));
  DrawRectangleRoundedLinesEx(grow(r, 2), roundness, 8, 2.5f, withAlpha(kGold, 0.8f + 0.2f * pulse));
  corners(grow(r, 6 + pulse * 2), 0, std::min(16.0f, r.height * 0.35f), 3.0f, kGold);
}

void arrowTri(Vector2 c, float s, int dir, Color col) {
  DrawTriangle({c.x + dir * s * 0.6f, c.y}, {c.x - dir * s * 0.4f, c.y - s * 0.6f},
               {c.x - dir * s * 0.4f, c.y + s * 0.6f}, col);
}

void slider(Rectangle r, float v, bool focused, Color fill, float time) {
  v = clampf(v, 0.0f, 1.0f);
  DrawRectangleRounded(grow(r, 2), 1.0f, 12, kInk);
  DrawRectangleRounded(r, 1.0f, 12, {32, 46, 48, 255});
  if (v > 0.01f) {
    Rectangle f{r.x, r.y, std::max(r.height, r.width * v), r.height};
    DrawRectangleRounded(f, 1.0f, 12, fill);
    DrawRectangleRounded({f.x + 4, f.y + 2, f.width - 8, f.height * 0.35f}, 1.0f, 8, withAlpha(WHITE, 0.22f));
  }
  for (int i = 1; i < 10; ++i)  // battery cells
    DrawRectangleRec({r.x + r.width * i / 10.0f - 0.75f, r.y + 3, 1.5f, r.height - 6}, withAlpha(kInk, 0.4f));
  Vector2 k{r.x + r.width * v, r.y + r.height * 0.5f};
  float kr = r.height * 0.95f;
  const float pulse = 0.5f + 0.5f * std::sin(time * 6.0f);
  if (focused) glow(k, kr * 2.6f, withAlpha(kTeal, 0.6f + 0.2f * pulse), 6);
  DrawCircleV(k, kr + 2.5f, kInk);
  DrawCircleV(k, kr, focused ? kGold : kCream);
  DrawCircleV(k, kr * 0.5f, withAlpha(kInk, 0.75f));
  DrawCircleV(k, kr * 0.32f, focused ? mixColor(kTeal, WHITE, 0.3f * pulse) : kStone);
}

void toggle(Rectangle r, bool on, bool focused, float time) {
  Color bg = on ? Color{38, 126, 106, 255} : Color{62, 70, 72, 255};
  DrawRectangleRounded(grow(r, 2), 1.0f, 12, kInk);
  DrawRectangleRounded(r, 1.0f, 12, bg);
  DrawRectangleRounded({r.x + 6, r.y + 3, r.width - 12, r.height * 0.32f}, 1.0f, 8, withAlpha(WHITE, 0.1f));
  const char* txt = on ? "OUI" : "NON";
  float ts = r.height * 0.5f;
  float kr = r.height * 0.5f - 4;
  float kx = on ? r.x + r.width - kr - 5 : r.x + kr + 5;
  float tx = on ? r.x + 12 : r.x + r.width - 12 - textWidth(txt, ts);
  drawText(txt, tx, r.y + (r.height - ts) * 0.5f - 1, ts, on ? kCream : kMuted);
  const Vector2 kc{kx, r.y + r.height * 0.5f};
  if (on) glow(kc, kr * 2.4f, withAlpha(kTeal, 0.5f), 6);
  DrawCircleV(kc, kr, focused ? kGold : kCream);
  DrawCircleV(kc, kr * 0.36f, on ? Color{40, 150, 124, 255} : kStone);
  if (focused) DrawRectangleRoundedLinesEx(grow(r, 4), 1.0f, 12, 2.5f, withAlpha(kGold, 0.6f + 0.4f * std::sin(time * 7)));
}

void selector(Rectangle r, const char* value, bool focused, float time) {
  DrawRectangleRounded(grow(r, 2), 0.5f, 10, kInk);
  DrawRectangleRounded(r, 0.5f, 10, focused ? Color{34, 54, 56, 255} : Color{26, 40, 44, 255});
  DrawRectangleRoundedLinesEx(grow(r, -2), 0.5f, 10, 1.0f, withAlpha(focused ? kGold : kStone, 0.4f));
  float ts = r.height * 0.48f;
  drawText(value, r.x + (r.width - textWidth(value, ts)) * 0.5f, r.y + (r.height - ts) * 0.5f - 1, ts, kCream);
  float bob = focused ? std::sin(time * 8.0f) * 2.0f : 0.0f;
  Color ac = focused ? kGold : kStone;
  arrowTri({r.x + 16 - bob, r.y + r.height * 0.5f}, r.height * 0.42f, -1, ac);
  arrowTri({r.x + r.width - 16 + bob, r.y + r.height * 0.5f}, r.height * 0.42f, 1, ac);
  if (focused) {
    DrawRectangleRoundedLinesEx(grow(r, 6), 0.5f, 10, 4, withAlpha(kTeal, 0.2f + 0.1f * std::sin(time * 6)));
    DrawRectangleRoundedLinesEx(grow(r, 3), 0.5f, 10, 2.5f, withAlpha(kGold, 0.7f + 0.3f * std::sin(time * 7)));
  }
}

// ---- Input helpers ----------------------------------------------------------------
bool hovered(Rectangle r) { return CheckCollisionPointRec(GetMousePosition(), r); }
bool clicked(Rectangle r) { return hovered(r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT); }
namespace {
Vector2 g_prevMouse{-1.0f, -1.0f};
bool g_mouseMoved = false;
int g_uiFrames = 0;
}  // namespace

void newFrame() {
  Vector2 m = GetMousePosition();
  // The first frames see the pointer "appear" somewhere: that is not a real movement.
  g_mouseMoved = g_uiFrames > 3 && (m.x != g_prevMouse.x || m.y != g_prevMouse.y);
  g_prevMouse = m;
  ++g_uiFrames;
}

bool mouseMoved() { return g_mouseMoved; }

// ---- Transition ---------------------------------------------------------------------
void wipe(float t) {
  if (t <= 0.0f || t >= 1.0f) return;
  const float W = float(kScreenW), H = float(kScreenH), s = 260.0f;
  float xl, xr;
  if (t < 0.5f) {
    xl = -s - 400;
    xr = lerpf(-s, W + s, easeInOut(t * 2.0f));
  } else {
    xl = lerpf(-s - 40, W + s, easeInOut((t - 0.5f) * 2.0f));
    xr = W + s + 400;
  }
  auto band = [&](float off, Color c) {
    float l = xl + off, r = xr + off;
    if (r <= l) return;
    quad({l + s, 0}, {r + s, 0}, {r, H}, {l, H}, c, c, c, c);
  };
  band(26, kGold);
  band(12, {34, 124, 108, 255});
  band(0, kInk);
}

}  // namespace ui
