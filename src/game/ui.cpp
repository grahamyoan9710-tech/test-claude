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
    case Tone::Gold: return {255, 190, 92, 255};
    case Tone::Blue: return {30, 66, 148, 255};
    case Tone::Navy: return {22, 32, 70, 255};
    case Tone::Red: return {214, 58, 64, 255};
    case Tone::Green: return {78, 228, 162, 255};
    default: return {12, 18, 40, 255};
  }
}
Color toneBottom(Tone t) {
  switch (t) {
    case Tone::Gold: return {255, 176, 62, 255};
    case Tone::Blue: return {22, 52, 122, 255};
    case Tone::Navy: return {16, 24, 54, 255};
    case Tone::Red: return {176, 40, 52, 255};
    case Tone::Green: return {50, 200, 136, 255};
    default: return {10, 16, 36, 255};
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
  const Color cols[5] = {withAlpha(kBlue, 0.0f), withAlpha(kBlue, a), withAlpha(WHITE, a), withAlpha(kGold, a),
                         withAlpha(kGold, 0.0f)};
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
  bevel(r, cutFor(r, rad), withAlpha(top, a * top.a / 255.0f), withAlpha(bottom, a * bottom.a / 255.0f));
}
void chunky(Rectangle r, Tone t, float a, float rad) {
  chunky(r, toneTop(t), toneBottom(t), a, rad);
  if (t != Tone::Navy && t != Tone::Blue) return;
  const float cut = cutFor(r, rad), b = r.y + r.height, th = 2.0f;  // the azure line along the bottom edge
  const Color c = withAlpha(kBlue, (t == Tone::Blue ? 1.0f : 0.6f) * a);
  quad({r.x, b - th}, {r.x + r.width - cut + th, b - th}, {r.x + r.width - cut, b}, {r.x, b}, c, c, c, c);
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
  const float p = 0.5f + 0.5f * std::sin(time * 5.0f);
  bevelLines(grow(r, 4.0f), cutFor(r, rad) + 1.5f, 2.5f, withAlpha(WHITE, 0.7f + 0.3f * p));
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
  const float cut = cutFor(r, -1.0f);
  chunky(r, tone, a, cut);
  if (focused) {
    shimmer(r, time, a);
    rim(r, cut, time);
  }
  if (label && *label) {
    if (size <= 0.0f) size = std::min(30.0f, r.height * 0.56f);
    const float w = textWidth(label, size, FontKind::Title);
    drawText(label, r.x + (r.width - w) * 0.5f, capsY(r.y + r.height * 0.5f, size, FontKind::Title), size, withAlpha(toneInk(tone), a),
             FontKind::Title);
  }
}

void card(Rectangle r, float a, Color top, Color bottom) {
  const float cut = std::min(14.0f, std::min(r.width, r.height) * 0.2f);
  bevel(r, cut, withAlpha(top, a * top.a / 255.0f), withAlpha(bottom, a * bottom.a / 255.0f));
  bevelLines(grow(r, -1.0f), cut - 0.4f, 1.0f, withAlpha(kSky, 0.16f * a));
}

float ribbonWidth(const char* text, float size) { return textWidth(text, size, FontKind::Title) + size * 1.2f; }

float ribbon(const char* text, float cx, float y, float size, Tone tone, float a) {
  const float w = ribbonWidth(text, size), h = size * 1.4f;
  chunky({cx - w * 0.5f, y, w, h}, tone, a, h * 0.3f);
  drawText(text, cx - textWidth(text, size, FontKind::Title) * 0.5f, capsY(y + h * 0.5f, size, FontKind::Title), size,
           withAlpha(toneInk(tone), a), FontKind::Title);
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
  const Color k{4, 8, 26, 255};
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

void backdrop(float time, Color top, Color bottom, Color moteCol) {
  (void)time;
  (void)moteCol;
  DrawRectangleGradientV(0, 0, kScreenW, kScreenH, top, bottom);
  radial({kScreenW * 0.5f, kScreenH * 0.15f}, kScreenW * 0.6f, withAlpha(WHITE, 0.12f), withAlpha(WHITE, 0.0f), 40, 0.5f);
  vignette(0.45f);
}

void stageBackdrop(int stage, float time, float dim) {
  const auto& list = stageList();
  const StageDef& s = list[std::clamp(stage, 0, int(list.size()) - 1)];
  drawBackground(s, {std::sin(time * 0.045f) * 800.0f, -150.0f}, 1.0f, time);
  if (dim > 0.0f)
    DrawRectangleGradientV(0, 0, kScreenW, kScreenH, withAlpha(kNavyDeep, dim * 0.62f), withAlpha(kInk, std::min(1.0f, dim * 1.05f)));
  vignette(0.25f + 0.35f * dim);
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
// The logo's look: silver-blue type, a navy extrusion; gold for the big moments.
constexpr Color kSilver[3] = {{255, 255, 255, 255}, {222, 238, 255, 255}, {116, 172, 255, 255}};
constexpr Color kGoldT[3] = {{255, 250, 214, 255}, {255, 214, 84, 255}, {242, 146, 28, 255}};
constexpr Color kRedT[3] = {{255, 236, 228, 255}, {255, 128, 108, 255}, {214, 44, 56, 255}};
constexpr Color kSkyT[3] = {{255, 255, 255, 255}, {176, 226, 255, 255}, {64, 150, 255, 255}};
constexpr Color kDepthBlue{12, 32, 96, 255}, kDepthGold{90, 40, 8, 255}, kDepthRed{84, 10, 22, 255};
const SpriteSpec kSpecs[int(Sprite::Count)] = {
    {"CLASH HORIZON", 120, kSilver[0], kSilver[1], kSilver[2], kInk, kDepthBlue, 9},
    {"COMBAT", 92, kSilver[0], kSilver[1], kSilver[2], kInk, kDepthBlue, 6},
    {"VICTOIRE !", 92, kGoldT[0], kGoldT[1], kGoldT[2], kInk, kDepthGold, 7},
    {"PAUSE", 96, kSilver[0], kSilver[1], kSilver[2], kInk, kDepthBlue, 6},
    {"GAME !", 260, kGoldT[0], kGoldT[1], kGoldT[2], kInk, kDepthGold, 9},
    {"3", 240, kRedT[0], kRedT[1], kRedT[2], kInk, kDepthRed, 7},
    {"2", 240, kGoldT[0], kGoldT[1], kGoldT[2], kInk, kDepthGold, 7},
    {"1", 240, kSkyT[0], kSkyT[1], kSkyT[2], kInk, kDepthBlue, 7},
    {"GO !", 230, kGoldT[0], kGoldT[1], kGoldT[2], kInk, kDepthGold, 8},
    {"PRÊTS ?", 96, kSilver[0], kSilver[1], kSilver[2], kInk, kDepthBlue, 7},
    {"MATCH NUL !", 80, kSilver[0], kSilver[1], kSilver[2], kInk, kDepthBlue, 5},
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
  // The screen's name, big at the top left, on the horizon line.
  (void)accent;
  (void)time;
  const float size = 54.0f, x = 64.0f, y = 18.0f, w = textWidth(title, size, FontKind::Title);
  bold(title, x, y, size);
  horizonLine(x - 24, x + w + 64, y + size + 8, 2.0f);
  eclat({x + w * 0.5f + 20, y + size + 8}, 4.0f);
  if (subtitle) drawTextOutlined(subtitle, x, y + size + 18, 16, kSky, kInk, 1.5f);
}

float keycapWidth(const char* label, float h) {
  return std::max(h, textWidth(label, h * 0.52f, FontKind::Title) + h * 0.62f);
}

float keycap(const char* label, float x, float y, float h, bool lit) {
  // Outlined, like on the charte; lit: filled gold.
  const float ts = h * 0.52f, w = keycapWidth(label, h), rad = std::max(3.0f, h * 0.18f);
  const Rectangle r{x, y, w, h};
  if (lit) {
    roundRect(r, rad, kGold, kGold);
  } else {
    roundRect(r, rad, withAlpha(kInk, 0.65f), withAlpha(kInk, 0.65f));
    roundLines(grow(r, -1.0f), rad, 2.0f, kCream);
  }
  drawText(label, x + (w - textWidth(label, ts, FontKind::Title)) * 0.5f, capsY(y + h * 0.5f, ts, FontKind::Title), ts, lit ? kInk : kCream,
           FontKind::Title);
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
  const float w = 20 + textWidth(go.label, 22, FontKind::Title) + 10 + promptWidth(go.key, go.pad, 22) + 12;
  return {kScreenW - 24 - w, kScreenH - 43.0f, w, 36};
}

void footer(std::initializer_list<Hint> hints, const char* info, const Hint* go, bool goOn) {
  // The hints on a dark fade at the bottom, right-aligned; the info on the left.
  const float h = 50, y = kScreenH - h, kh = 26, ts = 16;
  DrawRectangleGradientV(0, int(y - 46), kScreenW, 46, withAlpha(kInk, 0.0f), withAlpha(kInk, 0.7f));
  DrawRectangle(0, int(y), kScreenW, int(h), withAlpha(kInk, 0.7f));
  horizonLine(0, kScreenW, y, 1.0f, 0.3f);
  float right = kScreenW - 28;
  if (go) {  // the call to action: gold when it can be pressed, its key at its right
    const Rectangle r = goRect(*go);
    const bool hot = goOn && hovered(r);
    chunky(r, goOn ? (hot ? mixColor(kGold, WHITE, 0.2f) : kGold) : kNavy, goOn ? kGoldDeep : kNavyDeep, 1.0f, 10.0f);
    if (goOn) shimmer(r, g_uiFrames / 60.0f, 0.8f);
    const float lx = r.x + 20;
    drawText(go->label, lx, capsY(r.y + r.height * 0.5f, 22, FontKind::Title), 22, goOn ? kInk : kDisabled, FontKind::Title);
    prompt(go->key, go->pad, lx + textWidth(go->label, 22, FontKind::Title) + 10, r.y + 7, 22);
    right = r.x;
  }
  float total = 0;
  for (const Hint& hint : hints) total += promptWidth(hint.key, hint.pad, kh) + 8 + textWidth(hint.label, ts) + 24;
  float x = right - total;
  for (const Hint& hint : hints) {
    x += prompt(hint.key, hint.pad, x, y + (h - kh) * 0.5f, kh) + 8;
    drawText(hint.label, x, y + (h - ts) * 0.5f, ts, kSoft);
    x += textWidth(hint.label, ts) + 24;
  }
  if (info) drawText(info, 28, y + (h - ts) * 0.5f, ts, kMuted);
}

// ---- Widgets ----------------------------------------------------------------------
void panel(Rectangle r, Color fill, Color border, float roundness) {
  const float cut = clampf(roundness * std::min(r.width, r.height) * 0.5f, 4.0f, 14.0f);
  bevel(r, cut, fill, mixColor(fill, kNavyDeep, 0.35f));
  bevelLines(grow(r, -1.0f), cut - 0.4f, 1.0f, withAlpha(border, border.a / 255.0f * 0.5f));
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
  if (v > 0.0f) quad({x0, cy - th * 0.5f}, {x1, cy - th * 0.5f}, {x1, cy + th * 0.5f}, {x0, cy + th * 0.5f}, fill, WHITE, WHITE, fill);
  const Vector2 k{x1, cy};
  const float kr = focused ? 10.0f + std::sin(time * 6.0f) : 8.0f;
  if (focused) radial(k, kr * 3.0f, withAlpha(kGold, 0.45f), withAlpha(kGold, 0.0f), 24);
  diamond(k, kr + 2.0f, kInk);
  diamond(k, kr, focused ? kGold : WHITE);
}

void toggle(Rectangle r, bool on, bool focused, float time) {
  // Two segments, OUI and NON: the current one gold.
  const float half = std::floor(r.width * 0.5f), ts = std::min(26.0f, r.height * 0.5f);
  const Rectangle yes{r.x, r.y, half, r.height}, no{r.x + half, r.y, r.width - half, r.height};
  DrawRectangleRec(on ? yes : no, kGold);
  DrawRectangleRec(on ? no : yes, kTrack);
  const Rectangle seg[2] = {yes, no};
  const char* txt[2] = {"OUI", "NON"};
  for (int i = 0; i < 2; ++i) {
    const bool cur = (i == 0) == on;
    drawText(txt[i], seg[i].x + (seg[i].width - textWidth(txt[i], ts, FontKind::Title)) * 0.5f,
             capsY(seg[i].y + seg[i].height * 0.5f, ts, FontKind::Title), ts, cur ? kInk : kMuted, FontKind::Title);
  }
  if (focused) rim(r, 2.0f, time);
}

void selector(Rectangle r, const char* value, bool focused, float time) {
  const float cut = std::min(10.0f, r.height * 0.3f), cy = r.y + r.height * 0.5f, ts = r.height * 0.48f;
  chunky(r, focused ? Tone::Blue : Tone::Navy, 1.0f, cut);
  if (focused) rim(r, cut, time);
  drawText(value, r.x + (r.width - textWidth(value, ts, FontKind::Title)) * 0.5f, capsY(cy, ts, FontKind::Title), ts, kCream,
           FontKind::Title);
  const float bob = focused ? std::sin(time * 8.0f) * 1.5f : 0.0f, s = r.height * 0.36f;
  for (int side = -1; side <= 1; side += 2)  // a chevron at each end, gold when focused
    arrowTri({side < 0 ? r.x + 14 - bob : r.x + r.width - 14 + bob, cy}, s, side, focused ? kGold : kSky);
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
  band(30, kGold);
  band(16, toneBottom(Tone::Blue));
  band(0, kInk);
}

}  // namespace ui
