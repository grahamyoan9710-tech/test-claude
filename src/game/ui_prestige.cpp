// « Prestige » (12/11): each screen a match poster (see ui.hpp). The big capitals come from distance fields
// (fonts.cpp), the hand-drawn strokes are the Blender 2D masks of assets/sprites (ui_pinceau, ui_vitesse, ui_souligne:
// white, tinted here).
#include <cmath>

#include "game/fighter_art.hpp"
#include "game/fx.hpp"
#include "game/pixel.hpp"
#include "game/ui.hpp"

namespace ui {

Flood flood(Hero h) {
  switch (h) {
    case Hero::H01: return {{229, 64, 79, 255}, {158, 31, 49, 255}};    // Hakuen
    case Hero::P01: return {{139, 92, 246, 255}, {76, 42, 158, 255}};   // Ombre
    case Hero::P02: return {{143, 200, 255, 255}, {63, 127, 196, 255}};  // Givre
    case Hero::P03: return {{255, 122, 61, 255}, {184, 70, 28, 255}};   // Brasier
    case Hero::P07: return {{255, 46, 99, 255}, {163, 22, 62, 255}};    // Foudre
    case Hero::P08: return {{244, 210, 122, 255}, {184, 145, 58, 255}};  // Lumière
    default: {
      const Color c = auraColor(h);
      return {c, shade(c, 0.6f)};
    }
  }
}

void lean(float x0, float x1, Color c, float k, float y0, float y1) {
  quad({x0 - k * y0, y0}, {x1 - k * y0, y0}, {x1 - k * y1, y1}, {x0 - k * y1, y1}, c, c, c, c);
}

float bezier(float t, float x1, float y1, float x2, float y2) {
  t = clampf(t, 0.0f, 1.0f);
  auto at = [](float u, float a, float b) { return 3 * a * u * (1 - u) * (1 - u) + 3 * b * u * u * (1 - u) + u * u * u; };
  float u = t;
  for (int i = 0; i < 8; ++i) {  // Newton on x(u) = t, then a few bisection-safe clamps
    const float x = at(u, x1, x2) - t;
    const float dx = 3 * x1 * (1 - u) * (1 - u) + 6 * (x2 - x1) * u * (1 - u) + 3 * (1 - x2) * u * u;
    if (std::fabs(x) < 1e-5f || std::fabs(dx) < 1e-6f) break;
    u = clampf(u - x / dx, 0.0f, 1.0f);
  }
  return at(u, y1, y2);
}

float seg(float t, float a, float b, float x1, float y1, float x2, float y2) {
  return bezier((t - a) / std::max(1e-6f, b - a), x1, y1, x2, y2);
}

Vector2 slam(float s) {
  static const float kt[] = {0.0f, 0.15f, 0.30f, 0.50f, 0.70f, 1.0f};
  static const Vector2 kv[] = {{-10, 5}, {9, -6}, {-6, 4}, {4, -2}, {-2, 1}, {0, 0}};
  if (s < 0.0f || s >= 0.45f) return {0, 0};
  const float u = s / 0.45f;
  int k = 0;
  while (k < 4 && u >= kt[k + 1]) ++k;
  const float v = (u - kt[k]) / (kt[k + 1] - kt[k]);
  return {kv[k].x + (kv[k + 1].x - kv[k].x) * v, kv[k].y + (kv[k + 1].y - kv[k].y) * v};
}

void scaled(float s, float ox, float oy, const std::function<void()>& draw) {
  if (std::fabs(s - 1.0f) < 1e-4f) {
    draw();
    return;
  }
  rlPushMatrix();
  rlTranslatef(ox, oy, 0.0f);
  rlScalef(s, s, 1.0f);
  rlTranslatef(-ox, -oy, 0.0f);
  draw();
  rlPopMatrix();
}

// ---- Big type ---------------------------------------------------------------------------
namespace {
FontKind kindOf(Face f) { return f == Face::Wide ? FontKind::Display : f == Face::Exo ? FontKind::Damage : FontKind::Title; }
float boxOf(Face f) { return f == Face::Exo ? 1.2f : 1.088f; }  // ascent - descent, in em: raylib's size
}  // namespace

float bigWidth(const char* s, float em, Face f, float ls) {
  return MeasureTextEx(bigFont(kindOf(f)), s, em * boxOf(f), ls * em).x;
}

void big(const char* s, float x, float top, float em, Face f, Color fill, float lh, float ls, float stroke, Color strokeCol) {
  if (fill.a == 0) return;
  const float size = em * boxOf(f), y = top + (lh - boxOf(f)) * 0.5f * em;
  beginBig(stroke > 0.0f ? 0.5f * stroke * bigFieldPerPx(size) : 0.0f, strokeCol, false);
  DrawTextEx(bigFont(kindOf(f)), s, {x, y}, size, ls * em, fill);
  endBig();
}

void bigHollow(const char* s, float x, float top, float em, Face f, float stroke, Color col, float lh, float ls) {
  if (col.a == 0) return;
  const float size = em * boxOf(f), y = top + (lh - boxOf(f)) * 0.5f * em;
  beginBig(0.5f * stroke * bigFieldPerPx(size), BLANK, true);
  DrawTextEx(bigFont(kindOf(f)), s, {x, y}, size, ls * em, col);
  endBig();
}

void bigClip(float x0, bool keepLeft, float k) {
  const RenderTexture2D* t = pix::frameTarget();
  const float h = t ? float(t->texture.height) : float(kScreenH) * g_renderScale, sg = keepLeft ? 1.0f : -1.0f;
  // the line x + k y = x0: kept where (x + k y - x0) has the sign asked
  setBigClip(true, sg, sg * k, sg * x0, h, g_renderScale);
}

void bigClipOff() { setBigClip(false); }

void choc(const char* s, float cx, float top, float em, float split, float alpha, float stroke, Face f, float lh, float ls) {
  if (alpha <= 0.0f) return;
  const float w = bigWidth(s, em, f, ls), x = cx - w * 0.5f;
  // the copies are the outlined shape (as far as the stroke reaches), in their colour
  big(s, x - split, top, em, f, withAlpha(kSplitR, 0.95f * alpha), lh, ls, stroke, withAlpha(kSplitR, 0.95f * alpha));
  big(s, x + split, top, em, f, withAlpha(kSplitB, 0.95f * alpha), lh, ls, stroke, withAlpha(kSplitB, 0.95f * alpha));
  big(s, x, top, em, f, withAlpha(WHITE, alpha), lh, ls, stroke, withAlpha(kNight, alpha));
}

float exoWidth(const char* s, float em) { return MeasureTextEx(gameFont(FontKind::Damage), s, 1.2f * em, 0.0f).x; }

void exo(const char* s, float x, float base, float em, Color c) {
  DrawTextEx(gameFont(FontKind::Damage), s, {x, base - 0.999f * em}, 1.2f * em, 0.0f, c);  // Exo 2: ascent .999, box 1.2
}

float capsWidth(const char* s, float size, float ls) {
  return MeasureTextEx(gameFont(FontKind::Display), s, size * 1.088f, ls * size).x;
}

float caps(const char* s, float x, float cy, float size, float ls, Color col) {
  DrawTextEx(gameFont(FontKind::Display), s, {x, cy - 0.535f * size}, size * 1.088f, ls * size, col);
  return capsWidth(s, size, ls);
}

std::vector<std::string> nameLines(const std::string& name, float em, float maxW) {
  if (bigWidth(name.c_str(), em, Face::Cond) <= maxW || name.find(' ') == std::string::npos) return {name};
  std::vector<std::string> out;
  for (size_t a = 0;;) {
    const size_t b = name.find(' ', a);
    out.push_back(name.substr(a, b == std::string::npos ? std::string::npos : b - a));
    if (b == std::string::npos) return out;
    a = b + 1;
  }
}

float pkeyWidth(const char* key, Btn pad, float h) {
  if (padPrompts() && pad != Btn::None) return padGlyphWidth(pad, h, promptFamily());
  return std::max(h, capsWidth(key, h * 0.4f, 0.1f) + h * 0.6f);
}

float pkey(const char* key, Btn pad, float x, float y, float h, bool filled, float alpha) {
  if (padPrompts() && pad != Btn::None) return padGlyph(pad, x, y, h, promptFamily());
  const float w = pkeyWidth(key, pad, h), ts = h * 0.4f;
  const Rectangle r{x, y, w, h};
  if (filled) {
    DrawRectangleRec(r, withAlpha(kSilver, alpha));
  } else {
    DrawRectangleRec(r, withAlpha(kNight, 0.6f * alpha));
    DrawRectangleLinesEx(r, 1.0f, withAlpha(kSilver, 0.55f * alpha));
  }
  caps(key, x + (w - capsWidth(key, ts, 0.1f)) * 0.5f, y + h * 0.5f, ts, 0.1f, withAlpha(filled ? kNight : kSilver, alpha));
  return w;
}

// ---- Blender 2D masks ---------------------------------------------------------------------
void brushFrame(int frame, Color col, float alpha) {
  if (frame < 0 || frame >= 20 || alpha <= 0.0f) return;  // 20 frames of 640 x 360, 5 a row
  const Rectangle src{float(frame % 5) * 640.0f, float(frame / 5) * 360.0f, 640, 360};
  art::drawPicture("ui_pinceau", src, {0, 0, float(kScreenW), float(kScreenH)}, alpha, false, col);
}

void speedFrame(int frame, Rectangle band, Color col) {
  frame = ((frame % 12) + 12) % 12;  // 12 frames of 1280 x 328, 3 a row
  const Rectangle src{float(frame % 3) * 1280.0f, float(frame / 3) * 328.0f, 1280, 328};
  art::drawPicture("ui_vitesse", src, band, 1.0f, false, col);
}

void swooshFrame(int frame, Rectangle dst, Color col) {
  if (frame < 0) return;
  frame = std::min(frame, 9);  // 10 frames of 900 x 160, 5 a row; it stays on the last
  const Rectangle src{float(frame % 5) * 900.0f, float(frame / 5) * 160.0f, 900, 160};
  art::drawPicture("ui_souligne", src, dst, 1.0f, false, col);
}

// ---- The logo ----------------------------------------------------------------------------
void prestigeLogo(float x, float y, float s, float time, const LogoIn& in) {
  constexpr float kCut = 64.0f / 300.0f;  // the cut's lean (626 -> 562 over the 300 px of CLASH)
  auto box = [&](float bx, float by) { return Vector2{x + bx * s, y + by * s}; };
  rlPushMatrix();
  rlTranslatef(x, y, 0.0f);
  rlScalef(s, s, 1.0f);
  // CLASH, 300 px, centred, its line box at 93; the halves either side of the cut slide in along it
  if (in.cut >= 0.0f) {
    const float w = bigWidth("CLASH", 300, Face::Cond), cx = (1200.0f - w) * 0.5f;
    const float k = 1.0f - in.cut;
    for (int half = 0; half < 2; ++half) {
      const Vector2 off = half == 0 ? Vector2{-62.0f * k, 293.0f * k} : Vector2{3.0f + 59.0f * k, -14.0f - 279.0f * k};
      const Vector2 p = box(half == 0 ? 626.0f + off.x : 636.0f + off.x, 93.0f + off.y);  // the cut, on screen
      const float c = p.x + kCut * p.y;
      const RenderTexture2D* t = pix::frameTarget();
      const float th = t ? float(t->texture.height) : float(kScreenH) * g_renderScale, sg = half == 0 ? 1.0f : -1.0f;
      setBigClip(true, sg, sg * kCut, sg * c, th, g_renderScale);
      big("CLASH", cx + off.x, 93.0f + off.y, 300, Face::Cond, WHITE, 1.0f);
    }
    setBigClip(false);
  }
  // H◎RIZON, 64 px spaced 0.26 em, centred (the padding balances the last letter's spacing), between two lines
  const Font& F = gameFont(FontKind::Display);
  const float em = 64.0f, ls = 0.26f * em, size = em * 1.088f;
  const float hW = MeasureTextEx(F, "H", size, ls).x + ls, rW = MeasureTextEx(F, "RIZON", size, ls).x + ls;
  const float ringW = 64.8f, total = hW + ringW + ls + rW, x0 = ls + (1200.0f - ls - total) * 0.5f;
  const float wy = 380.0f + 16.0f * (1.0f - in.word), ty = wy + (1.0f - 1.088f) * 0.5f * em;
  const Color ink = withAlpha(kSilver, in.word);
  if (in.word > 0.0f) {
    DrawTextEx(F, "H", {x0, ty}, size, ls, ink);
    DrawTextEx(F, "RIZON", {x0 + hW + ringW + ls, ty}, size, ls, ink);
    // The ring: the svg's ellipse (24.5 x 18.5, 10 thick) in a 65 x 64 box
    const Vector2 oc{x0 + hW + 32.5f * ringW / 65.0f, wy + 31.0f};
    for (int i = 0; i < 72; ++i) {
      const float a0 = i * 2.0f * kPi / 72, a1 = (i + 1) * 2.0f * kPi / 72;
      auto pt = [&](float a, float rx, float ry) { return Vector2{oc.x + std::cos(a) * rx, oc.y + std::sin(a) * ry}; };
      quad(pt(a0, 29.5f, 23.5f), pt(a1, 29.5f, 23.5f), pt(a1, 19.5f, 13.5f), pt(a0, 19.5f, 13.5f), ink, ink, ink, ink);
    }
    // The star: it pops in, turns once in 15.7 s and breathes (x1.12 every 1.2 s)
    if (in.starScale > 0.0f) {
      const float breathe = 1.0f + 0.12f * (0.5f - 0.5f * std::cos(time * kPi / 1.2f));
      const float sc = breathe * in.starScale, rot = (time / 15.7f * 360.0f + in.starRot) * DEG2RAD;
      glow(oc, 17.0f * sc, withAlpha(kBlue, 0.6f * in.word));
      spark(oc, 12.5f * sc, rot, withAlpha(kBlue, in.word), 0.2307f);
    }
  }
  // The two thin lines, growing from the word outwards
  if (in.lines > 0.0f) {
    const float l = 87.0f * in.lines;
    DrawRectangleRec({244.0f + 87.0f - l, 410, l, 3}, kSilver);
    DrawRectangleRec({869.0f, 410, l, 3}, kSilver);
  }
  rlPopMatrix();
}

}  // namespace ui
