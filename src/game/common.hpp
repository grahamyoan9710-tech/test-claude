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
inline Color percentColor(float p) {
  if (p < 50) return mixColor(WHITE, {255, 240, 120, 255}, p / 50.0f);
  if (p < 100) return mixColor({255, 240, 120, 255}, {255, 150, 40, 255}, (p - 50) / 50.0f);
  if (p < 150) return mixColor({255, 150, 40, 255}, {235, 40, 30, 255}, (p - 100) / 50.0f);
  return mixColor({235, 40, 30, 255}, {140, 10, 20, 255}, (p - 150) / 80.0f);
}
