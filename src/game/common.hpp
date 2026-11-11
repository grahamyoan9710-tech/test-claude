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
constexpr Color kOutline = {10, 14, 36, 255};  // default text outline (navy ink)

// The colour of each card, a CPU's too (« Ligne d'horizon »): J1 azure, J2 gold, J3 green, J4 pink.
inline Color portColor(int port) {
  static const Color cols[4] = {{61, 139, 255, 255}, {255, 181, 71, 255}, {61, 220, 151, 255}, {255, 111, 156, 255}};
  return cols[port & 3];
}
// Team battles: red, blue.
inline Color teamColor(int team) { return team == 1 ? Color{61, 139, 255, 255} : Color{255, 96, 82, 255}; }
inline const char* teamName(int team) { return team == 1 ? "BLEUE" : "ROUGE"; }
inline Color slotColor(const Fighter& f) { return f.inTeam ? teamColor(f.team) : portColor(f.port); }

// ---- Text ------------------------------------------------------------------
inline float textSpacing(float size, FontKind k) {
  return k == FontKind::Title ? size * 0.02f : k == FontKind::Damage ? 0.0f : size * 0.01f;
}
// Saira's capitals are smaller and lower in their line box than those of the fonts every layout was made for
// (Luckiest Guy, Poppins): the text is drawn larger and raised so that its capitals keep their place.
inline float fontScale(FontKind k) { return k == FontKind::Title ? 1.45f : k == FontKind::Damage ? 1.1f : 1.15f; }
inline float fontRaise(FontKind k) { return k == FontKind::Title ? 0.373f : k == FontKind::Damage ? 0.0f : 0.08f; }  // in asked sizes
// The y to draw a line of text at so that its capitals are centred on cy (Exo 2: caps from 0.26 to 0.83 of its line).
inline float capsY(float cy, float size, FontKind k) {
  return cy - size * (k == FontKind::Title ? 0.356f : k == FontKind::Damage ? 0.6f : 0.498f);
}
inline float textWidth(const char* text, float size, FontKind k = FontKind::Ui) {
  const float s = size * fontScale(k);
  return MeasureTextEx(gameFont(k), text, s, textSpacing(s, k)).x;
}
inline void drawText(const char* text, float x, float y, float size, Color col, FontKind k = FontKind::Ui) {
  const float s = size * fontScale(k);
  DrawTextEx(gameFont(k), text, {x, y - size * fontRaise(k)}, s, textSpacing(s, k), col);
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
    case HitFx::Fire: return {255, 120, 40, 255};
    case HitFx::Electric: return {120, 230, 255, 255};
    case HitFx::Slash: return {210, 230, 255, 255};
    case HitFx::Dark: return {150, 70, 220, 255};
    case HitFx::Magic: return {255, 130, 220, 255};
    case HitFx::Shadow: return {150, 90, 230, 255};
    case HitFx::Ice: return {140, 210, 255, 255};
    case HitFx::Star: return {190, 140, 255, 255};
    case HitFx::Water: return {60, 170, 240, 255};
    case HitFx::Rose: return {245, 100, 170, 255};
    case HitFx::Lightning: return {240, 50, 60, 255};
    case HitFx::Light: return {255, 226, 140, 255};
    case HitFx::Wind: return {80, 220, 190, 255};
    case HitFx::Arcane: return {170, 110, 240, 255};
    case HitFx::Sand: return {230, 180, 70, 255};
    case HitFx::Blood: return {200, 30, 44, 255};
    case HitFx::Moon: return {130, 100, 230, 255};
    case HitFx::Sun: return {255, 190, 70, 255};
    case HitFx::Nature: return {90, 210, 140, 255};
    default: return {255, 234, 190, 255};
  }
}
inline Color hitFxGlow(HitFx fx) {
  switch (fx) {
    case HitFx::Blood: return {255, 150, 150, 255};
    case HitFx::Dark: return {220, 180, 255, 255};
    case HitFx::Lightning: return {255, 170, 170, 255};
    default: return mixColor(hitFxColor(fx), WHITE, 0.55f);
  }
}

inline Color percentColor(float p) {  // « Ligne d'horizon »: Éclat, gold from 60 %, coral from 110 %, then red
  if (p < 60) return mixColor({238, 243, 255, 255}, {255, 200, 112, 255}, p / 60.0f);
  if (p < 110) return mixColor({255, 200, 112, 255}, {255, 122, 82, 255}, (p - 60) / 50.0f);
  if (p < 170) return mixColor({255, 122, 82, 255}, {232, 52, 44, 255}, (p - 110) / 60.0f);
  return mixColor({232, 52, 44, 255}, {150, 20, 28, 255}, std::min(1.0f, (p - 170) / 80.0f));
}
