#include "game/ui.hpp"

#include <cstring>
#include <string>
#include <vector>

#include "game/render.hpp"
#include "game/settings.hpp"

namespace ui {
namespace {
int g_uiFrames = 0;  // frames seen by newFrame: the clock of what the kit animates by itself
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

// ---- The Clash Horizon look (« Ligne d'horizon ») -------------------------------------
Color toneTop(Tone t) {
  switch (t) {
    case Tone::Gold: return kCream;
    case Tone::Blue: return {50, 47, 102, 255};
    case Tone::Navy: return {38, 36, 80, 255};
    case Tone::Red: return {214, 58, 64, 255};
    case Tone::Green: return {78, 228, 162, 255};
    default: return {30, 29, 66, 255};
  }
}
Color toneBottom(Tone t) {
  switch (t) {
    case Tone::Gold: return kCream;
    case Tone::Blue: return {44, 41, 92, 255};
    case Tone::Navy: return {33, 31, 72, 255};
    case Tone::Red: return {176, 40, 52, 255};
    case Tone::Green: return {50, 200, 136, 255};
    default: return {28, 27, 62, 255};
  }
}
Color toneInk(Tone t) {
  switch (t) {
    case Tone::Gold: case Tone::Green: return kInk;
    case Tone::Grey: return kDisabled;
    default: return kCream;
  }
}

namespace {
// The six corners of a box with its top-left and bottom-right corners cut, clockwise from the top left.
void bevelPoints(Rectangle r, float cut, Vector2* p) {
  cut = clampf(cut, 0.0f, std::min(r.width, r.height) * 0.45f);
  p[0] = {r.x + cut, r.y};
  p[1] = {r.x + r.width, r.y};
  p[2] = {r.x + r.width, r.y + r.height - cut};
  p[3] = {r.x + r.width - cut, r.y + r.height};
  p[4] = {r.x, r.y + r.height};
  p[5] = {r.x, r.y + cut};
}
float cutFor(Rectangle r, float rad) { return rad < 0.0f ? std::min(14.0f, r.height * 0.26f) : std::min(rad, 14.0f); }
float radFor(Rectangle r, float rad) { return rad < 0.0f ? std::min(14.0f, r.height * 0.3f) : rad; }
}  // namespace

void bevel(Rectangle r, float cut, Color top, Color bottom, bool across) {
  if (r.width <= 0.0f || r.height <= 0.0f) return;
  Vector2 p[6];
  bevelPoints(r, cut, p);
  auto at = [&](Vector2 v) { return mixColor(top, bottom, across ? (v.x - r.x) / r.width : (v.y - r.y) / r.height); };
  const Vector2 m{r.x + r.width * 0.5f, r.y + r.height * 0.5f};
  const Color mc = at(m);
  rlBegin(RL_TRIANGLES);
  for (int i = 0; i < 6; ++i) {  // a fan from the centre: the gradient stays exact
    const Vector2 a = p[i], b = p[(i + 1) % 6];
    vtx(m, mc);
    vtx(b, at(b));
    vtx(a, at(a));
  }
  rlEnd();
}

void bevelLines(Rectangle r, float cut, float thick, Color c) {
  Vector2 p[6];
  bevelPoints(r, cut, p);
  const Vector2 mid{(p[0].x + p[1].x) * 0.5f, r.y};  // start and end on a straight part: no notch at a corner
  const Vector2 loop[8] = {mid, p[1], p[2], p[3], p[4], p[5], p[0], mid};
  polyline(loop, 8, thick, c);
}

void horizonLine(float x0, float x1, float y, float thick, float a) {
  if (x1 <= x0) return;
  const float stops[5] = {0.0f, 0.18f, 0.5f, 0.82f, 1.0f};
  const Color cols[5] = {withAlpha(kCream, 0.0f), withAlpha(kCream, 0.8f * a), withAlpha(kCream, a),
                         withAlpha(kCream, 0.8f * a), withAlpha(kCream, 0.0f)};
  const float y0 = y - thick * 0.5f, y1 = y + thick * 0.5f;
  for (int i = 0; i < 4; ++i) {
    const float l = x0 + (x1 - x0) * stops[i], r = x0 + (x1 - x0) * stops[i + 1];
    quad({l, y0}, {r, y0}, {r, y1}, {l, y1}, cols[i], cols[i + 1], cols[i + 1], cols[i]);
  }
}

void eclat(Vector2 c, float r, float a, Color col) {
  radial(c, r * 3.2f, withAlpha(col, 0.45f * a), withAlpha(col, 0.0f), 24);
  diamond(c, r, withAlpha(col, a));
}

void roundRect(Rectangle r, float rad, Color top, Color bottom) {
  if (r.width <= 0.0f || r.height <= 0.0f) return;
  rad = clampf(rad, 0.0f, std::min(r.width, r.height) * 0.5f);
  constexpr int kSeg = 6;
  Vector2 p[4 * (kSeg + 1)];
  int n = 0;
  const Vector2 c[4] = {{r.x + r.width - rad, r.y + rad}, {r.x + r.width - rad, r.y + r.height - rad},
                        {r.x + rad, r.y + r.height - rad}, {r.x + rad, r.y + rad}};
  for (int k = 0; k < 4; ++k)  // the corners in turn, clockwise from the top right
    for (int i = 0; i <= kSeg; ++i) {
      const float a = (k - 1 + float(i) / kSeg) * kPi * 0.5f;
      p[n++] = {c[k].x + std::cos(a) * rad, c[k].y + std::sin(a) * rad};
    }
  auto at = [&](float y) { return mixColor(top, bottom, (y - r.y) / r.height); };
  const Vector2 m{r.x + r.width * 0.5f, r.y + r.height * 0.5f};
  const Color mc = at(m.y);
  rlBegin(RL_TRIANGLES);
  for (int i = 0; i < n; ++i) {  // a fan from the centre: the gradient stays exact
    const Vector2 a = p[i], b = p[(i + 1) % n];
    vtx(m, mc);
    vtx(b, at(b.y));
    vtx(a, at(a.y));
  }
  rlEnd();
}

void roundLines(Rectangle r, float rad, float thick, Color c) {
  const float m = std::min(r.width, r.height);
  if (m <= 0.0f) return;
  DrawRectangleRoundedLinesEx(r, clampf(rad * 2.0f / m, 0.0f, 1.0f), 8, thick, c);
}

float chunkyLip(Rectangle) { return 0.0f; }

void chunky(Rectangle r, Color top, Color bottom, float a, float rad) {
  roundRect(r, radFor(r, rad), withAlpha(top, a * top.a / 255.0f), withAlpha(bottom, a * bottom.a / 255.0f));
}
void chunky(Rectangle r, Tone t, float a, float rad) {
  chunky(r, toneTop(t), toneBottom(t), a, rad);
  if (t == Tone::Navy || t == Tone::Blue || t == Tone::Grey)  // the dark plates: a cream hairline around them
    roundLines(grow(r, -0.5f), radFor(r, rad), 1.0f, withAlpha(kCream, (t == Tone::Blue ? 0.3f : 0.14f) * a));
}

void bold(const char* text, float x, float y, float size, Color fill, float a) {
  const float th = std::max(1.5f, size * 0.05f);
  const Color ink = withAlpha(kInk, a);
  drawTextOutlined(text, x, y + std::max(1.5f, size * 0.045f), size, withAlpha(kInk, 0.45f * a),
                   withAlpha(kInk, 0.45f * a), th, FontKind::Title);  // a soft drop
  drawTextOutlined(text, x, y, size, withAlpha(fill, a * fill.a / 255.0f), ink, th, FontKind::Title);
}
void boldCentered(const char* text, float cx, float y, float size, Color fill, float a) {
  bold(text, cx - textWidth(text, size, FontKind::Title) * 0.5f, y, size, fill, a);
}

void rim(Rectangle r, float rad, float time) {
  const float p = 0.5f + 0.5f * std::sin(time * 4.0f);
  roundLines(grow(r, 4.0f), radFor(r, rad) + 4.0f, 2.0f, withAlpha(kCream, 0.55f + 0.35f * p));
}

void shimmer(Rectangle r, float time, float a) {
  // A soft white band slanted like the bevels, crossing the face every 2.6 s.
  const float u = std::fmod(time, 2.6f) / 2.6f, e = 1.0f - (1.0f - u) * (1.0f - u) * (1.0f - u);
  const float w = r.height * 0.85f, k = r.height * 0.36f, x = r.x - w - k + e * (r.width + 2.0f * (w + k));
  const Color peak = withAlpha(WHITE, 0.5f * a), clear = withAlpha(WHITE, 0.0f);
  const float y0 = r.y, y1 = r.y + r.height, xm = x + w * 0.5f;
  beginClip(r);
  quad({x + k, y0}, {xm + k, y0}, {xm - k, y1}, {x - k, y1}, clear, peak, peak, clear);
  quad({xm + k, y0}, {x + w + k, y0}, {x + w - k, y1}, {xm - k, y1}, peak, clear, clear, peak);
  endClip();
}

void button(Rectangle r, const char* label, Tone tone, bool focused, float time, float a, float size) {
  const float rad = radFor(r, -1.0f);
  chunky(r, focused && tone != Tone::Gold && tone != Tone::Grey ? Tone::Blue : tone, a, rad);
  if (focused) rim(r, rad, time);
  if (label && *label) {
    constexpr FontKind D = FontKind::Display;
    size = size <= 0.0f ? std::min(20.0f, r.height * 0.34f) : size * 0.62f;  // (sizes asked for the old condensed type)
    while (size > 9.0f && textWidth(label, size, D) > r.width - 20.0f) size -= 0.5f;
    const float w = textWidth(label, size, D);
    drawText(label, r.x + (r.width - w) * 0.5f, capsY(r.y + r.height * 0.5f, size, D), size, withAlpha(toneInk(tone), a), D);
  }
}

void card(Rectangle r, float a, Color top, Color bottom) {
  const float rad = std::min(16.0f, std::min(r.width, r.height) * 0.2f);
  roundRect(r, rad, withAlpha(top, a * top.a / 255.0f), withAlpha(bottom, a * bottom.a / 255.0f));
  roundLines(grow(r, -0.5f), rad, 1.0f, withAlpha(kCream, 0.12f * a));
}

float ribbonWidth(const char* text, float size) { return textWidth(text, size * 0.62f, FontKind::Display) + size * 1.2f; }

float ribbon(const char* text, float cx, float y, float size, Tone tone, float a) {
  const float w = ribbonWidth(text, size), h = size * 1.4f, ds = size * 0.62f;
  chunky({cx - w * 0.5f, y, w, h}, tone, a, h * 0.5f);
  drawText(text, cx - textWidth(text, ds, FontKind::Display) * 0.5f, capsY(y + h * 0.5f, ds, FontKind::Display), ds,
           withAlpha(toneInk(tone), a), FontKind::Display);
  return w;
}

void menuPlate(Rectangle r, float s, bool chosen, float a) {
  (void)s;
  chunky(r, chosen ? Tone::Gold : Tone::Navy, a);
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
  const Color k = kDuskTop;
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

void spark(Vector2 c, float r, float rot, Color col, float waist) {
  Vector2 p[8];
  for (int i = 0; i < 8; ++i) {
    const float a = rot + i * kPi / 4.0f - kPi / 2.0f, q = i % 2 == 0 ? r : r * waist * 1.41f;
    p[i] = {c.x + std::cos(a) * q, c.y + std::sin(a) * q};
  }
  rlBegin(RL_TRIANGLES);
  for (int i = 0; i < 8; ++i) {
    vtx(c, col);
    vtx(p[i], col);
    vtx(p[(i + 1) % 8], col);
  }
  rlEnd();
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
  (void)s;
  (void)line;
  card(r, 1.0f, top, bottom);
}

void rule(float x0, float x1, float y, Color col) {
  DrawLineEx({x0 + 6, y}, {x1 - 6, y}, 1.5f, col);
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

void dusk(float a) {
  const int mid = int(kScreenH * 0.55f);
  DrawRectangleGradientV(0, 0, kScreenW, mid, withAlpha(kDuskTop, a), withAlpha(kDuskMid, a));
  DrawRectangleGradientV(0, mid, kScreenW, kScreenH - mid, withAlpha(kDuskMid, a), withAlpha(kDuskLow, a));
}

void backdrop(float time, Color top, Color bottom, Color moteCol) {
  (void)time;
  (void)moteCol;
  (void)top;
  (void)bottom;
  dusk();
}

void stageBackdrop(int stage, float time, float dim) {
  const auto& list = stageList();
  const StageDef& s = list[std::clamp(stage, 0, int(list.size()) - 1)];
  drawBackground(s, {std::sin(time * 0.045f) * 800.0f, -150.0f}, 1.0f, time);
  if (dim > 0.0f)
    dusk(dim * 0.8f);
  vignette(0.2f + 0.3f * dim);
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
  bool split;  // the red / blue split of the big moments (else: a soft drop)
};
// « Épure »: flat cream Archivo Expanded; the big moments split red and blue at their edges.
const SpriteSpec kSpecs[int(Sprite::Count)] = {
    {"CLASH HORIZON", 120, false}, {"COMBAT", 92, false}, {"VICTOIRE", 92, false}, {"PAUSE", 96, false},
    {"GAME", 260, true},           {"3", 240, true},      {"2", 240, true},        {"1", 240, true},
    {"GO", 230, true},             {"PRÊTS ?", 96, false}, {"MATCH NUL", 80, false},
};
constexpr float kSpriteType = 0.72f;  // the type's size in a sprite of size s (Archivo Expanded is wide)
float spriteTextWidth(const SpriteSpec& sp) { return textWidth(sp.text, sp.size * kSpriteType, FontKind::Display); }
void spriteText(const SpriteSpec& sp, float x, float y) {
  const float ts = sp.size * kSpriteType, off = sp.size * 0.02f;
  const float ty = y + sp.size * 0.5f - ts * 0.535f;  // caps centred in the sprite's line
  if (sp.split) {
    drawText(sp.text, x - off, ty, ts, {255, 79, 79, 190}, FontKind::Display);
    drawText(sp.text, x + off, ty, ts, {79, 155, 255, 190}, FontKind::Display);
  } else {
    drawText(sp.text, x, ty + off, ts, withAlpha(kDeep, 0.6f), FontKind::Display);
  }
  drawText(sp.text, x, ty, ts, kCream, FontKind::Display);
}
}  // namespace

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
    b.w = spriteTextWidth(sp) + b.pad * 2;
    b.h = sp.size * 1.3f + b.pad * 2;
    b.rt = LoadRenderTexture(int(b.w * kBakeScale), int(b.h * kBakeScale));
    SetTextureFilter(b.rt.texture, TEXTURE_FILTER_BILINEAR);
    BeginTextureMode(b.rt);
    ClearBackground(BLANK);
    rlScalef(kBakeScale, kBakeScale, 1.0f);
    frameBlend();  // "over" for alpha too: the texture ends up premultiplied
    spriteText(sp, b.pad, b.pad);
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
  // The screen's name, big at the top left, on the horizon line.
  (void)accent;
  (void)time;
  constexpr FontKind D = FontKind::Display;
  const float size = 30.0f, x = 64.0f, y = 34.0f, w = textWidth(title, size, D);
  drawText(title, x, y, size, kCream, D);
  DrawRectangleRec({x, y + size + 12, 40, 2}, kCream);
  DrawCircleV({x + 43, y + size + 13}, 3.5f, kBlue);
  (void)w;
  if (subtitle) drawText(subtitle, x + 56, capsY(y + size + 13, 14, FontKind::Body), 14, kMuted, FontKind::Body);
}

float keycapWidth(const char* label, float h) {
  return std::max(h, textWidth(label, h * 0.44f, FontKind::Body) + h * 0.66f);
}

float keycap(const char* label, float x, float y, float h, bool lit) {
  // Outlined with a thicker bottom edge, like a key; lit: filled cream.
  const float ts = h * 0.44f, w = keycapWidth(label, h), rad = std::max(3.0f, h * 0.28f);
  const Rectangle r{x, y, w, h};
  if (lit) {
    roundRect(r, rad, kCream, kCream);
  } else {
    roundRect(r, rad, withAlpha(kCream, 0.32f), withAlpha(kCream, 0.32f));
    roundRect({x + 1, y + 1, w - 2, h - 3}, rad - 1, withAlpha(kDeep, 0.92f), withAlpha(kDeep, 0.92f));
  }
  drawText(label, x + (w - textWidth(label, ts, FontKind::Body)) * 0.5f, capsY(y + h * 0.5f - 1, ts, FontKind::Body), ts,
           lit ? kInk : kCream, FontKind::Body);
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

// pressed: the stick click (L3 / R3), drawn pushed in with a darker cap.
void stickGlyph(Vector2 c, float r, const char* letter, bool pressed = false) {
  DrawCircleV({c.x, c.y + r * 0.16f}, r, shade(kPlastic, 0.45f));
  DrawCircleV(c, r, kPlastic);
  DrawCircleV(c, r * 0.8f, pressed ? shade(kLight, 0.82f) : kLight);
  DrawRing(c, r * 0.6f, r * 0.68f, 0, 360, 36, shade(kLight, 0.78f));
  centred(letter, {c.x, c.y + r * 0.03f}, std::strlen(letter) > 1 ? r * 0.9f : r * 1.05f, kInk);
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
    case Btn::L3: stickGlyph(c, r, f == PadFamily::PlayStation ? "L3" : "LS", true); break;
    case Btn::R3: stickGlyph(c, r, f == PadFamily::PlayStation ? "R3" : "RS", true); break;
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

Btn btnFor(int b) {
  static const Btn kMap[kPadButtonCount] = {Btn::A,  Btn::B,  Btn::X,     Btn::Y,  Btn::LB, Btn::RB,
                                            Btn::LT, Btn::RT, Btn::Back,  Btn::Start, Btn::L3, Btn::R3,
                                            Btn::DPad, Btn::DPad, Btn::DPad, Btn::DPad};
  return b >= 0 && b < kPadButtonCount ? kMap[b] : Btn::None;
}

namespace {
// Fallbacks when no button is bound: moving and smash attacks always have their stick.
Btn boundOr(int button, int action) {
  if (button >= 0) return btnFor(button);
  return action < kActJump ? Btn::LStick : action == kActSmash ? Btn::RStick : Btn::None;
}
}  // namespace

Btn padButtonFor(int action) {
  if (action < kActJump || action >= kActionCount) return Btn::LStick;  // directions
  const auto& b = settings().pad[action];
  return boundOr(b[0] >= 0 ? b[0] : b[1], action);
}

namespace {
PadFamily deviceFamily(int device) {
  const int p = device - kPad1;
  return padConnected(p) ? padFamily(p) : promptFamily();
}
std::string actionKey(int device, int action) { return keyName(settings().keys[device][action]); }
}  // namespace

Btn deviceButtonFor(int device, int action) {
  const int p = device - kPad1;
  if (!padConnected(p) || action < kActJump || action >= kActionCount) return padButtonFor(action);
  const int b0 = padBinding(p, action, 0);
  return boundOr(b0 >= 0 ? b0 : padBinding(p, action, 1), action);
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

Rectangle goRect(const Hint& go) {
  const float w = 46 + textWidth(go.label, 15, FontKind::Display) + 12 + promptWidth(go.key, go.pad, 24) + 10;
  return {kScreenW - 40 - w, kScreenH - 62.0f, w, 48};
}

void footer(std::initializer_list<Hint> hints, const char* info, const Hint* go, bool goOn) {
  // The hints on a dark fade at the bottom, right-aligned; the info on the left.
  // A hairline, the hints right-aligned, the info on the left (« Épure »: no bar, the dusk shows through).
  constexpr FontKind B = FontKind::Body;
  const float y = kScreenH - 76.0f, cy = kScreenH - 38.0f, kh = 24, ts = 13;
  DrawRectangleGradientV(0, int(y), kScreenW, kScreenH - int(y), withAlpha(kDeep, 0.0f), withAlpha(kDeep, 0.55f));
  DrawRectangleRec({0, y, float(kScreenW), 1}, kHair);
  float right = kScreenW - 40;
  if (go) {  // the call to action: cream when it can be pressed, a play icon, its key at its right
    const Rectangle r = goRect(*go);
    const bool hot = goOn && hovered(r);
    if (goOn) {
      const float p = 0.5f - 0.5f * std::cos(g_uiFrames / 60.0f * 2.0f * kPi / 2.4f);
      roundRect(grow(r, 6.0f * p), 14 + 6.0f * p, withAlpha(kCream, 0.12f), withAlpha(kCream, 0.12f));
    }
    roundRect(r, 14, goOn ? (hot ? WHITE : kCream) : kNavy, goOn ? (hot ? WHITE : kCream) : kNavyDeep);
    const Color ink = goOn ? kInk : kDisabled;
    const float ix = r.x + 22, icy = r.y + r.height * 0.5f;
    quad({ix - 5, icy - 8}, {ix + 8, icy}, {ix + 8, icy}, {ix - 5, icy + 8}, ink, ink, ink, ink);
    drawText(go->label, r.x + 40, capsY(icy, 15, FontKind::Display), 15, ink, FontKind::Display);
    prompt(go->key, go->pad, r.x + 40 + textWidth(go->label, 15, FontKind::Display) + 12, icy - 12, 24);
    right = r.x - 16;
  }
  float total = 0;
  for (const Hint& hint : hints) total += promptWidth(hint.key, hint.pad, kh) + 8 + textWidth(hint.label, ts, B) + 22;
  float x = right - total;
  for (const Hint& hint : hints) {
    x += prompt(hint.key, hint.pad, x, cy - kh * 0.5f, kh) + 8;
    drawText(hint.label, x, capsY(cy, ts, B), ts, kSoft, B);
    x += textWidth(hint.label, ts, B) + 22;
  }
  if (info) drawText(info, 40, capsY(cy, ts, B), ts, kMuted, B);
}

// ---- Widgets ----------------------------------------------------------------------
void panel(Rectangle r, Color fill, Color border, float roundness) {
  const float rad = clampf(roundness * std::min(r.width, r.height) * 0.5f, 6.0f, 16.0f);
  roundRect(r, rad, fill, mixColor(fill, kNavyDeep, 0.35f));
  roundLines(grow(r, -0.5f), rad, 1.0f, withAlpha(border, border.a / 255.0f * 0.4f));
}

void focusFrame(Rectangle r, float time, float roundness) {
  rim(r, roundness * std::min(r.width, r.height) * 0.5f, time);
}

void arrowTri(Vector2 c, float s, int dir, Color col) {
  DrawTriangle({c.x + dir * s * 0.6f, c.y}, {c.x - dir * s * 0.4f, c.y - s * 0.6f},
               {c.x - dir * s * 0.4f, c.y + s * 0.6f}, col);
}

void slider(Rectangle r, float v, bool focused, Color fill, float time) {
  // A thin track, its filled part from the colour to white, a diamond knob (gold when focused).
  v = clampf(v, 0.0f, 1.0f);
  const float th = 6.0f, cy = r.y + r.height * 0.5f, x0 = r.x, x1 = r.x + r.width * v;
  DrawRectangleRec({r.x, cy - th * 0.5f, r.width, th}, kTrack);
  if (v > 0.0f) quad({x0, cy - th * 0.5f}, {x1, cy - th * 0.5f}, {x1, cy + th * 0.5f}, {x0, cy + th * 0.5f}, kCream, kCream, kCream, kCream);
  (void)fill;
  const Vector2 k{x1, cy};
  const float kr = focused ? 10.0f + std::sin(time * 6.0f) : 8.0f;
  if (focused) radial(k, kr * 3.0f, withAlpha(kBlue, 0.4f), withAlpha(kBlue, 0.0f), 24);
  DrawCircleV(k, kr + 2.0f, kInk);
  DrawCircleV(k, kr, focused ? kCream : kSoft);
}

void toggle(Rectangle r, bool on, bool focused, float time) {
  // Two segments, OUI and NON: the current one gold.
  const float half = std::floor(r.width * 0.5f), ts = std::min(26.0f, r.height * 0.5f);
  const Rectangle yes{r.x, r.y, half, r.height}, no{r.x + half, r.y, r.width - half, r.height};
  roundRect(r, 10, kTrack, kTrack);
  roundRect(grow(on ? yes : no, -3.0f), 8, kCream, kCream);
  const Rectangle seg[2] = {yes, no};
  const char* txt[2] = {"OUI", "NON"};
  for (int i = 0; i < 2; ++i) {
    const bool cur = (i == 0) == on;
    const float ds = ts * 0.6f;
    drawText(txt[i], seg[i].x + (seg[i].width - textWidth(txt[i], ds, FontKind::Display)) * 0.5f,
             capsY(seg[i].y + seg[i].height * 0.5f, ds, FontKind::Display), ds, cur ? kInk : kMuted, FontKind::Display);
  }
  if (focused) rim(r, 10.0f, time);
}

void selector(Rectangle r, const char* value, bool focused, float time) {
  const float cut = std::min(10.0f, r.height * 0.3f), cy = r.y + r.height * 0.5f, ts = r.height * 0.48f;
  chunky(r, focused ? Tone::Blue : Tone::Navy, 1.0f, cut);
  if (focused) rim(r, cut, time);
  float ds = ts * 0.6f;
  while (ds > 9.0f && textWidth(value, ds, FontKind::Display) > r.width - 64.0f) ds -= 0.5f;
  drawText(value, r.x + (r.width - textWidth(value, ds, FontKind::Display)) * 0.5f, capsY(cy, ds, FontKind::Display), ds,
           kCream, FontKind::Display);
  const float bob = focused ? std::sin(time * 8.0f) * 1.5f : 0.0f, s = r.height * 0.36f;
  for (int side = -1; side <= 1; side += 2)  // a chevron at each end, gold when focused
    arrowTri({side < 0 ? r.x + 14 - bob : r.x + r.width - 14 + bob, cy}, s, side, focused ? kCream : kMuted);
}

float labelWidth(const char* text, float size, float em) {
  const float s = size * fontScale(FontKind::Body);
  return MeasureTextEx(gameFont(FontKind::Body), text, s, em * size).x;
}

float label(const char* text, float x, float cy, float size, float em, Color col) {
  const float s = size * fontScale(FontKind::Body);
  DrawTextEx(gameFont(FontKind::Body), text, {x, capsY(cy, size, FontKind::Body)}, s, em * size, col);
  return labelWidth(text, size, em);
}

namespace {
// A path of the 24-unit grid: points, with a break (NaN) between strokes.
constexpr float B = 1e9f;
struct P { float x, y; };
void strokes(std::initializer_list<P> pts, Vector2 c, float k, float t, Color col) {
  std::vector<Vector2> run;
  auto flush = [&] {
    if (run.size() >= 2) polyline(run.data(), int(run.size()), t, col);
    for (const Vector2& v : run) DrawCircleV(v, t * 0.5f, col);  // round caps and joins
    run.clear();
  };
  for (const P& p : pts) {
    if (p.x >= B) flush();
    else run.push_back({c.x + (p.x - 12.0f) * k, c.y + (p.y - 12.0f) * k});
  }
  flush();
}
void circleStroke(Vector2 c, float k, float cx, float cy, float r, float t, Color col) {
  const Vector2 m{c.x + (cx - 12.0f) * k, c.y + (cy - 12.0f) * k};
  DrawRing(m, r * k - t * 0.5f, r * k + t * 0.5f, 0, 360, 32, col);
}
void dot(Vector2 c, float k, float x, float y, float r, Color col) { DrawCircleV({c.x + (x - 12.0f) * k, c.y + (y - 12.0f) * k}, r * k, col); }
}  // namespace

void icon(Icon i, Vector2 c, float size, Color col, float stroke) {
  const float k = size / 24.0f, t = std::max(1.2f, stroke * k);
  switch (i) {
    case Icon::Play: {
      const Vector2 a{c.x - 4 * k, c.y - 6.5f * k}, b{c.x + 6.5f * k, c.y}, d{c.x - 4 * k, c.y + 6.5f * k};
      DrawTriangle(a, d, b, col);
      break;
    }
    case Icon::Versus:
      strokes({{4, 4}, {14.5f, 14.5f}, {B, 0}, {12.5f, 17}, {17, 12.5f}, {B, 0}, {15.5f, 15.5f}, {20, 20}, {B, 0}, {20, 4},
               {9.5f, 14.5f}, {B, 0}, {7, 12.5f}, {11.5f, 17}, {B, 0}, {8.5f, 15.5f}, {4, 20}}, c, k, t, col);
      break;
    case Icon::Training:  // a target
      circleStroke(c, k, 12, 12, 8, t, col);
      circleStroke(c, k, 12, 12, 4, t, col);
      dot(c, k, 12, 12, 1.3f, col);
      break;
    case Icon::Online:  // a globe
      circleStroke(c, k, 12, 12, 8.5f, t, col);
      strokes({{3.5f, 12}, {20.5f, 12}, {B, 0}, {12, 3.5f}, {9.5f, 8}, {9, 12}, {9.5f, 16}, {12, 20.5f}, {B, 0}, {12, 3.5f},
               {14.5f, 8}, {15, 12}, {14.5f, 16}, {12, 20.5f}}, c, k, t, col);
      break;
    case Icon::Fighters:  // two busts
      circleStroke(c, k, 9, 8.5f, 3.5f, t, col);
      strokes({{3, 19.5f}, {4, 16}, {9, 14}, {14, 16}, {15, 19.5f}, {B, 0}, {15.5f, 5.5f}, {18, 7}, {18, 10.5f}, {15.5f, 12},
               {B, 0}, {17, 14.2f}, {20, 16}, {21, 19.5f}}, c, k, t, col);
      break;
    case Icon::Arenas:  // a platform over the ground
      strokes({{6, 8}, {18, 8}, {B, 0}, {4, 13.5f}, {20, 13.5f}, {B, 0}, {3, 19}, {21, 19}}, c, k, t, col);
      break;
    case Icon::Options:  // a gear
      circleStroke(c, k, 12, 12, 3, t, col);
      for (int j = 0; j < 8; ++j) {
        const float a = j * kPi / 4.0f;
        strokes({{12 + std::cos(a) * 6.5f, 12 + std::sin(a) * 6.5f}, {12 + std::cos(a) * 9, 12 + std::sin(a) * 9}}, c, k, t, col);
      }
      circleStroke(c, k, 12, 12, 6.5f, t, col);
      break;
    case Icon::Quit:  // a door and an arrow out
      strokes({{14, 4}, {6, 4}, {6, 20}, {14, 20}, {B, 0}, {10, 12}, {20, 12}, {B, 0}, {16.5f, 8.5f}, {20, 12}, {16.5f, 15.5f}}, c, k,
              t, col);
      break;
    case Icon::Sliders:
      strokes({{4, 7}, {13, 7}, {B, 0}, {17, 7}, {20, 7}, {B, 0}, {4, 17}, {7, 17}, {B, 0}, {11, 17}, {20, 17}}, c, k, t, col);
      circleStroke(c, k, 15, 7, 2, t, col);
      circleStroke(c, k, 9, 17, 2, t, col);
      break;
    case Icon::Dice:
      roundLines({c.x - 8 * k, c.y - 8 * k, 16 * k, 16 * k}, 3.5f * k, t, col);
      for (P p : {P{8.5f, 8.5f}, P{15.5f, 8.5f}, P{12, 12}, P{8.5f, 15.5f}, P{15.5f, 15.5f}}) dot(c, k, p.x, p.y, 1.2f, col);
      break;
    case Icon::Sword:
      strokes({{19, 5}, {9.5f, 14.5f}, {B, 0}, {19, 5}, {15.5f, 5}, {B, 0}, {19, 5}, {19, 8.5f}, {B, 0}, {7, 12}, {12, 17}, {B, 0},
               {8.5f, 15.5f}, {5, 19}}, c, k, t, col);
      break;
    case Icon::Trophy:
      strokes({{7, 4}, {17, 4}, {17, 10}, {15, 13}, {12, 14}, {9, 13}, {7, 10}, {7, 4}, {B, 0}, {7, 6}, {4, 6}, {4, 8}, {7, 10.5f}, {B, 0},
               {17, 6}, {20, 6}, {20, 8}, {17, 10.5f}, {B, 0}, {12, 14}, {12, 18}, {B, 0}, {8, 20}, {16, 20}}, c, k, t, col);
      break;
    case Icon::Rematch: {  // a circle arrow
      std::vector<P> arc;
      for (int j = 0; j <= 14; ++j) {
        const float a = -kPi * 0.25f + j * (kPi * 1.6f) / 14.0f;
        arc.push_back({12 + std::cos(a) * 7.5f, 12 + std::sin(a) * 7.5f});
      }
      Vector2 pts[15];
      for (int j = 0; j < 15; ++j) pts[j] = {c.x + (arc[j].x - 12) * k, c.y + (arc[j].y - 12) * k};
      polyline(pts, 15, t, col);
      strokes({{17.3f, 3.5f}, {17.3f, 6.7f}, {14, 6.7f}}, c, k, t, col);
      break;
    }
    case Icon::Home:
      strokes({{4, 11}, {12, 4}, {20, 11}, {B, 0}, {6.5f, 9.5f}, {6.5f, 20}, {17.5f, 20}, {17.5f, 9.5f}, {B, 0}, {10, 20}, {10, 14},
               {14, 14}, {14, 20}}, c, k, t, col);
      break;
    case Icon::Close: strokes({{6, 6}, {18, 18}, {B, 0}, {18, 6}, {6, 18}}, c, k, t, col); break;
    case Icon::ChevronL: strokes({{14.5f, 6}, {8.5f, 12}, {14.5f, 18}}, c, k, t, col); break;
    case Icon::ChevronR: strokes({{9.5f, 6}, {15.5f, 12}, {9.5f, 18}}, c, k, t, col); break;
    case Icon::Arrow: strokes({{5, 12}, {19, 12}, {B, 0}, {13, 6}, {19, 12}, {13, 18}}, c, k, t, col); break;
  }
}

float trackedWidth(const char* text, float size, float em) {
  const float s = size * fontScale(FontKind::Title);
  return MeasureTextEx(gameFont(FontKind::Title), text, s, em * s / 1.574f).x;  // the font's em is 1/1.574 of s
}

float tracked(const char* text, float x, float y, float size, float em, Color col) {
  const float s = size * fontScale(FontKind::Title);
  DrawTextEx(gameFont(FontKind::Title), text, {x, y - size * fontRaise(FontKind::Title)}, s, em * s / 1.574f, col);
  return trackedWidth(text, size, em);
}

float drawWrapped(const std::string& text, float x, float y, float width, float size, Color col, bool draw) {
  std::string line, word;
  auto flush = [&] {
    if (line.empty()) return;
    if (draw) drawTextOutlined(line.c_str(), x, y, size, col, kInk, 2.0f);
    y += size * 1.3f;
    line.clear();
  };
  for (size_t i = 0; i <= text.size(); ++i) {
    if (i < text.size() && text[i] != ' ') {
      word += text[i];
      continue;
    }
    const std::string next = line.empty() ? word : line + " " + word;
    if (!line.empty() && textWidth(next.c_str(), size) > width) {
      flush();
      line = word;
    } else {
      line = next;
    }
    word.clear();
  }
  flush();
  return y;
}

void gradientText(const char* text, float x, float y, float size, Color left, Color mid, Color right) {
  // Drawn once per thin vertical strip, each in its own colour.
  constexpr int kStrips = 24;
  const float w = textWidth(text, size, FontKind::Title);
  for (int i = 0; i < kStrips; ++i) {
    const float u = (i + 0.5f) / kStrips;
    const Color c = u < 0.5f ? mixColor(left, mid, u * 2.0f) : mixColor(mid, right, u * 2.0f - 1.0f);
    beginClip({x + w * i / kStrips, y - size, w / kStrips + 1.0f, size * 3.0f});
    drawText(text, x, y, size, c, FontKind::Title);
    endClip();
  }
}

// ---- Input helpers ----------------------------------------------------------------
bool hovered(Rectangle r) { return CheckCollisionPointRec(GetMousePosition(), r); }
bool clicked(Rectangle r) { return hovered(r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT); }
namespace {
Vector2 g_prevMouse{-1.0f, -1.0f};
bool g_mouseMoved = false;
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
float wipeSwap(Wipe w) { return w == Wipe::Cut ? 0.0f : w == Wipe::Flash ? 0.4f : 0.5f; }
int wipeFrames(Wipe w) { return w == Wipe::Horizon ? 54 : w == Wipe::Flash ? 54 : w == Wipe::Cut ? 57 : 40; }

namespace {
float seg(float t, float a, float b) { return clampf((t - a) / (b - a), 0.0f, 1.0f); }
// A glowing cream line from a to b.
void bladeLine(Vector2 a, Vector2 b, float alpha) {
  if (alpha <= 0.0f) return;
  DrawLineEx(a, b, 16.0f, withAlpha(kBlue, 0.12f * alpha));
  DrawLineEx(a, b, 7.0f, withAlpha(WHITE, 0.3f * alpha));
  DrawLineEx(a, b, 3.0f, withAlpha(kCream, alpha));
}
// Part of a snapshot (y flipped, like a render texture) as a textured quad, its corners p moved by off.
void snapQuad(const Texture2D& tex, const Vector2* p, Vector2 off, float alpha) {
  const float W = float(kScreenW), H = float(kScreenH);
  rlSetTexture(tex.id);
  rlBegin(RL_QUADS);
  const unsigned char a = (unsigned char)(255 * clampf(alpha, 0.0f, 1.0f));
  rlColor4ub(255, 255, 255, a);
  for (int k : {0, 3, 2, 1}) {  // counter-clockwise on screen
    rlTexCoord2f(p[k].x / W, 1.0f - p[k].y / H);
    rlVertex2f(p[k].x + off.x, p[k].y + off.y);
  }
  rlEnd();
  rlSetTexture(0);
}
}  // namespace

void wipe(Wipe w, float t, const Texture2D* old) {
  if (t < 0.0f || t >= 1.0f) return;
  const float W = float(kScreenW), H = float(kScreenH), mid = H * 0.5f;
  switch (w) {
    case Wipe::Horizon: {  // the line draws (0-0.4), the halves close on it (0-0.42), hold, open again (0.58-1)
      const float close = t < 0.5f ? easeInOut(seg(t, 0.0f, 0.42f)) : 1.0f - easeInOut(seg(t, 0.58f, 1.0f));
      const float hh = (mid + 2.0f) * close;
      DrawRectangleRec({0, 0, W, hh}, kDeep);
      DrawRectangleRec({0, H - hh, W, hh}, kDeep);
      const float draw = easeInOut(seg(t, 0.0f, 0.4f)), fade = 1.0f - seg(t, 0.62f, 1.0f);
      const float lineY = t < 0.5f ? mid : hh;  // opening: the line rides the top half's edge
      bladeLine({0, lineY}, {W * draw, lineY}, fade);
      const float dx = -20.0f + 1310.0f * easeInOut(seg(t, 0.0f, 1.0f));
      glow({dx, lineY}, 26, withAlpha(kBlue, 0.8f * fade));
      DrawCircleV({dx, lineY}, 6, withAlpha(kBlue, fade));
      break;
    }
    case Wipe::Flash: {  // the line opens into a white screen (0-0.36), it holds, then fades (0.46-1)
      const float open = easeInOut(seg(t, 0.0f, 0.36f)), fade = 1.0f - seg(t, 0.46f, 1.0f);
      const float hh = std::max(1.5f, mid * open);
      DrawRectangleRec({0, mid - hh, W, hh * 2.0f}, withAlpha(kCream, fade));
      if (open < 1.0f) bladeLine({0, mid}, {W, mid}, 1.0f);
      break;
    }
    case Wipe::Cut: {  // the blade crosses (0-0.24), the old screen's halves fly apart (0.26-1)
      const Vector2 a{0, H * 0.75f}, b{W, H * 0.25f};
      const float slash = easeInOut(seg(t, 0.0f, 0.24f)), fly = seg(t, 0.26f, 1.0f), k = fly * fly * (3.0f - 2.0f * fly);
      if (old) {
        const Vector2 top[4] = {{0, 0}, {W, 0}, b, a}, bottom[4] = {a, b, {W, H}, {0, H}};
        snapQuad(*old, top, {-110.0f * k, -340.0f * k}, 1.0f - k);
        snapQuad(*old, bottom, {110.0f * k, 340.0f * k}, 1.0f - k);
      }
      const Vector2 d{b.x - a.x, b.y - a.y}, ext{-d.x * 0.05f, -d.y * 0.05f};
      bladeLine({a.x + ext.x, a.y + ext.y}, {a.x + ext.x + d.x * 1.1f * slash, a.y + ext.y + d.y * 1.1f * slash},
                1.0f - seg(t, 0.55f, 1.0f));
      break;
    }
    case Wipe::Game: {  // a cream panel sweeps in from the left (0-0.5), then on out to the right (0.5-1)
      const float x = t < 0.5f ? -W + W * easeInOut(seg(t, 0.0f, 0.5f)) : W * easeInOut(seg(t, 0.5f, 1.0f));
      DrawRectangleRec({x, 0, W, H}, kCream);
      DrawRectangleRec({x + W - 6, 0, 6, H}, withAlpha(Color{255, 79, 79, 255}, 0.7f));
      DrawRectangleRec({x, 0, 6, H}, withAlpha(Color{79, 155, 255, 255}, 0.7f));
      break;
    }
  }
}

}  // namespace ui
