#pragma once
// Shared helpers for the raylib front-end.
#include "core/match.hpp"
#include "raylib.h"

constexpr int kScreenW = 1280;
constexpr int kScreenH = 720;

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

inline Color portColor(int port) {
  static const Color cols[4] = {{235, 60, 60, 255}, {60, 120, 245, 255}, {245, 200, 40, 255}, {60, 200, 90, 255}};
  return cols[port & 3];
}
inline Color cpuColor() { return {150, 150, 160, 255}; }
inline Color slotColor(const Fighter& f) { return f.cpu ? cpuColor() : portColor(f.port); }

// Text with a dark outline, drawn with the default font.
inline void drawTextOutlined(const char* text, float x, float y, float size, Color col,
                             Color outline = {15, 15, 25, 255}, float thick = 2.0f) {
  Font font = GetFontDefault();
  float spacing = size / 10.0f;
  for (int dx = -1; dx <= 1; ++dx)
    for (int dy = -1; dy <= 1; ++dy)
      if (dx || dy) DrawTextEx(font, text, {x + dx * thick, y + dy * thick}, size, spacing, outline);
  DrawTextEx(font, text, {x, y}, size, spacing, col);
}
inline float textWidth(const char* text, float size) {
  return MeasureTextEx(GetFontDefault(), text, size, size / 10.0f).x;
}
inline void drawTextCentered(const char* text, float cx, float y, float size, Color col,
                             bool outline = true) {
  float w = textWidth(text, size);
  if (outline) drawTextOutlined(text, cx - w * 0.5f, y, size, col);
  else DrawTextEx(GetFontDefault(), text, {cx - w * 0.5f, y}, size, size / 10.0f, col);
}

// Percent colour, white -> yellow -> orange -> red -> dark red.
inline Color percentColor(float p) {
  if (p < 50) return mixColor(WHITE, {255, 240, 120, 255}, p / 50.0f);
  if (p < 100) return mixColor({255, 240, 120, 255}, {255, 150, 40, 255}, (p - 50) / 50.0f);
  if (p < 150) return mixColor({255, 150, 40, 255}, {235, 40, 30, 255}, (p - 100) / 50.0f);
  return mixColor({235, 40, 30, 255}, {140, 10, 20, 255}, (p - 150) / 80.0f);
}
