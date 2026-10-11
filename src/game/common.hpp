#pragma once
// Shared helpers for the raylib front-end.
#include "core/match.hpp"
#include "game/fonts.hpp"
#include "raylib.h"
#include "rlgl.h"

constexpr int kScreenW = 1280;
constexpr int kScreenH = 720;

// The whole frame is drawn in a 1280x720 virtual space into an off-screen
// texture that is `g_renderScale` times larger (supersampling = smooth edges).
inline float g_renderScale = 2.0f;

inline Vector2 V(Vec2 v) { return {v.x, v.y}; }
inline Vec2 W(Vector2 v) { return {v.x, v.y}; }
inline Color C(Color3 c, unsigned char a = 255) { return {c.r, c.g, c.b, a}; }
inline Color withAlpha(Color c, float a) {
  c.a = (unsigned char)clampf(a * 255.0f, 0.0f, 255.0f);
  return c;
}
inline Color mixColor(Color a, Color b, float t) {
  t = clampf(t, 0.0f, 1.0f);
  return {(unsigned char)lerpf(a.r, b.r, t), (unsigned char)lerpf(a.g, b.g, t),
          (unsigned char)lerpf(a.b, b.b, t), (unsigned char)lerpf(a.a, b.a, t)};
}
inline Color shade(Color c, float k) {
  return {(unsigned char)clampf(c.r * k, 0, 255), (unsigned char)clampf(c.g * k, 0, 255),
          (unsigned char)clampf(c.b * k, 0, 255), c.a};
}

// World drawing with a camera, compatible with the supersampled frame.
inline void beginWorld(Camera2D cam) {
  cam.offset.x *= g_renderScale;
  cam.offset.y *= g_renderScale;
  cam.zoom *= g_renderScale;
  BeginMode2D(cam);
}
inline void endWorld() {
  EndMode2D();
  rlScalef(g_renderScale, g_renderScale, 1.0f);
}
// Clip drawing to a rectangle given in virtual coordinates.
inline void beginClip(Rectangle r) {
  BeginScissorMode(int(r.x * g_renderScale), int(r.y * g_renderScale), int(r.width * g_renderScale),
                   int(r.height * g_renderScale));
}
inline void endClip() { EndScissorMode(); }

// ---- Palette ----------------------------------------------------------------
// Ancient-tech glow (TotK Zonai teal-green): runes on outfits, ultimate aura, menu selection.
constexpr Color kZonaiGlow = {72, 228, 190, 255};
constexpr Color kOutline = {12, 18, 22, 255};  // default text outline (teal ink)

inline Color portColor(int port) {
  static const Color cols[4] = {{230, 68, 56, 255}, {62, 124, 236, 255}, {240, 196, 52, 255}, {68, 192, 100, 255}};
  return cols[port & 3];
}
inline Color cpuColor() { return {150, 156, 152, 255}; }
inline Color slotColor(const Fighter& f) { return f.cpu ? cpuColor() : portColor(f.port); }

// ---- Text ------------------------------------------------------------------
inline float textSpacing(float size, FontKind k) { return k == FontKind::Title ? size * 0.02f : size * 0.01f; }
inline float textWidth(const char* text, float size, FontKind k = FontKind::Ui) {
  return MeasureTextEx(gameFont(k), text, size, textSpacing(size, k)).x;
}
inline void drawText(const char* text, float x, float y, float size, Color col, FontKind k = FontKind::Ui) {
  DrawTextEx(gameFont(k), text, {x, y}, size, textSpacing(size, k), col);
}
// Text with a smooth dark outline. thick < 0 picks a thickness from the size.
inline void drawTextOutlined(const char* text, float x, float y, float size, Color col,
                             Color outline = kOutline, float thick = -1.0f,
                             FontKind k = FontKind::Ui) {
  if (thick < 0.0f) thick = std::max(1.5f, size * 0.07f);
  if (thick > 0.0f && outline.a > 0) {
    const int dirs = thick > 3.0f ? 16 : 8;
    for (int i = 0; i < dirs; ++i) {
      float a = i * 2.0f * kPi / dirs;
      drawText(text, x + std::cos(a) * thick, y + std::sin(a) * thick, size, outline, k);
    }
  }
  drawText(text, x, y, size, col, k);
}
inline void drawTextCentered(const char* text, float cx, float y, float size, Color col, bool outline = true,
                             FontKind k = FontKind::Ui) {
  float w = textWidth(text, size, k);
  if (outline) drawTextOutlined(text, cx - w * 0.5f, y, size, col, kOutline, -1.0f, k);
  else drawText(text, cx - w * 0.5f, y, size, col, k);
}

// Percent colour, white -> yellow -> orange -> red -> dark red.
// Upper case for French UTF-8 text (é -> É, ç -> Ç, ō -> Ō...).
inline std::string upperFr(std::string s) {
  for (size_t i = 0; i < s.size(); ++i) {
    unsigned char c = (unsigned char)s[i];
    if (c >= 'a' && c <= 'z') {
      s[i] = char(c - 32);
    } else if (c == 0xC3 && i + 1 < s.size()) {
      unsigned char d = (unsigned char)s[i + 1];
      if (d >= 0xA0 && d <= 0xBE && d != 0xB7) s[i + 1] = char(d - 0x20);
      ++i;
    } else if (c == 0xC5 && i + 1 < s.size()) {
      unsigned char d = (unsigned char)s[i + 1];
      if (d == 0x8D || d == 0x93 || d == 0xAB) s[i + 1] = char(d - 1);  // ō, œ, ū
      ++i;
    }
  }
  return s;
}

// Signature colours of each element: the main tint and its bright glow.
inline Color hitFxColor(HitFx fx) {
  switch (fx) {
    case HitFx::Fire: return {255, 140, 40, 255};
    case HitFx::Electric: return {120, 230, 255, 255};
    case HitFx::Slash: return {210, 230, 255, 255};
    case HitFx::Dark: return {150, 70, 220, 255};
    case HitFx::Magic: return {255, 130, 220, 255};
    case HitFx::AntiMagic: return {28, 24, 34, 255};
    case HitFx::Wind: return {120, 232, 170, 255};
    case HitFx::Reiatsu: return {90, 170, 255, 255};
    case HitFx::Petal: return {255, 150, 190, 255};
    case HitFx::Light: return {255, 214, 110, 255};
    case HitFx::Gloom: return {176, 24, 52, 255};
    case HitFx::Ice: return {140, 220, 255, 255};
    case HitFx::Cursed: return {110, 130, 255, 255};
    case HitFx::Blood: return {214, 30, 44, 255};
    case HitFx::Chakra: return {255, 150, 40, 255};
    case HitFx::Lightning: return {150, 200, 255, 255};
    case HitFx::BlackFlame: return {22, 12, 26, 255};
    case HitFx::Hollow: return {70, 220, 130, 255};
    case HitFx::Berserk: return {240, 214, 70, 255};
    case HitFx::Water: return {70, 160, 240, 255};
    case HitFx::Wolf: return {110, 170, 255, 255};
    case HitFx::Claw: return {60, 214, 240, 255};
    default: return {255, 234, 190, 255};
  }
}
inline Color hitFxGlow(HitFx fx) {
  switch (fx) {
    case HitFx::AntiMagic: return {246, 246, 250, 255};
    case HitFx::Gloom: return {255, 90, 110, 255};
    case HitFx::Blood: return {255, 150, 150, 255};
    case HitFx::Cursed: return {200, 170, 255, 255};
    case HitFx::Dark: return {220, 180, 255, 255};
    case HitFx::BlackFlame: return {200, 60, 120, 255};
    case HitFx::Lightning: return {230, 244, 255, 255};
    case HitFx::Wolf: return {214, 232, 255, 255};
    case HitFx::Claw: return {210, 252, 255, 255};
    default: return mixColor(hitFxColor(fx), WHITE, 0.55f);
  }
}

inline Color percentColor(float p) {
  if (p < 50) return mixColor(WHITE, {255, 240, 120, 255}, p / 50.0f);
  if (p < 100) return mixColor({255, 240, 120, 255}, {255, 150, 40, 255}, (p - 50) / 50.0f);
  if (p < 150) return mixColor({255, 150, 40, 255}, {235, 40, 30, 255}, (p - 100) / 50.0f);
  return mixColor({235, 40, 30, 255}, {140, 10, 20, 255}, (p - 150) / 80.0f);
}
