#include "game/ui.hpp"

#include <string>
#include <vector>

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
  DrawCircleGradient(int(c.x), int(c.y), radius, withAlpha(col, a * 0.55f), clear);
  DrawCircleGradient(int(c.x), int(c.y), radius * 0.5f, withAlpha(col, a * 0.45f), clear);
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
  Color dark = withAlpha(BLACK, s), clear = withAlpha(BLACK, 0.0f);
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

// ---- Backgrounds ----------------------------------------------------------------
void backdrop(float time, Color top, Color bottom, Color bands) {
  DrawRectangleGradientV(0, 0, kScreenW, kScreenH, top, bottom);
  const float period = 230.0f;
  for (int i = 0; i < 9; ++i) {
    float x = std::fmod(i * period + time * 22.0f, period * 9) - period * 2;
    float w = 70.0f + 60.0f * hash(i * 7);
    slanted({x, 0, w + 320, float(kScreenH)}, 320, withAlpha(bands, 0.05f + 0.04f * hash(i)),
            withAlpha(bands, 0.02f));
  }
  glow({kScreenW * 0.5f, kScreenH * 0.32f}, 620, withAlpha(bands, 0.22f), 12);
  for (int i = 0; i < 46; ++i) {  // rising sparks
    float speed = 18.0f + 40.0f * hash(i * 3);
    float x = hash(i) * kScreenW + std::sin(time * 0.7f + i) * 18.0f;
    float y = kScreenH + 20 - std::fmod(time * speed + hash(i + 77) * (kScreenH + 40), float(kScreenH + 40));
    float tw = 0.4f + 0.6f * (0.5f + 0.5f * std::sin(time * 3.0f + i));
    DrawCircleV({x, y}, 1.2f + 2.2f * hash(i * 5), withAlpha(mixColor(bands, WHITE, 0.6f), 0.45f * tw));
  }
  vignette(0.55f);
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
const SpriteSpec kSpecs[int(Sprite::Count)] = {
    {"CLASH BROS.", 150, {255, 248, 190, 255}, {255, 206, 64, 255}, {242, 124, 34, 255}, {110, 16, 34, 255}, {70, 8, 30, 255}, 9},
    {"COMBAT", 92, {255, 255, 255, 255}, {255, 240, 220, 255}, {255, 214, 190, 255}, kInk, {90, 10, 30, 255}, 5},
    {"VICTOIRE !", 92, {255, 248, 190, 255}, {255, 206, 64, 255}, {242, 124, 34, 255}, {110, 16, 34, 255}, {70, 8, 30, 255}, 7},
    {"PAUSE", 96, {255, 255, 255, 255}, {235, 238, 255, 255}, {200, 206, 240, 255}, kInk, {40, 30, 80, 255}, 6},
    {"GAME !", 260, {255, 248, 190, 255}, {255, 206, 64, 255}, {242, 124, 34, 255}, {110, 16, 34, 255}, {70, 8, 30, 255}, 9},
    {"3", 240, {255, 170, 160, 255}, {255, 90, 80, 255}, {200, 40, 50, 255}, {30, 10, 30, 255}, {20, 6, 24, 255}, 7},
    {"2", 240, {255, 240, 170, 255}, {255, 200, 60, 255}, {230, 140, 30, 255}, {30, 10, 30, 255}, {20, 6, 24, 255}, 7},
    {"1", 240, {190, 255, 190, 255}, {110, 230, 120, 255}, {40, 160, 70, 255}, {30, 10, 30, 255}, {20, 6, 24, 255}, 7},
    {"GO !", 230, {255, 248, 190, 255}, {255, 206, 64, 255}, {242, 124, 34, 255}, {30, 10, 30, 255}, {20, 6, 24, 255}, 7},
    {"PRÊTS ?", 96, {255, 255, 255, 255}, {240, 236, 255, 255}, {210, 200, 240, 255}, {30, 10, 30, 255}, {20, 6, 24, 255}, 7},
    {"MATCH NUL !", 80, WHITE, {230, 230, 240, 255}, {200, 200, 220, 255}, kInk, {40, 30, 60, 255}, 5},
};
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
  Rectangle r{-60, 24, 640, 72};
  slanted({r.x + 8, r.y + 8, r.width, r.height}, 40, withAlpha(BLACK, 0.35f), withAlpha(BLACK, 0.35f));
  slanted(r, 40, mixColor(accent, WHITE, 0.15f), shade(accent, 0.6f));
  slanted({r.x, r.y, r.width - 8, 8}, 5, withAlpha(WHITE, 0.35f), withAlpha(WHITE, 0.1f));
  slanted({r.x + r.width + 12, r.y, 26, r.height}, 40, kGold, shade(kGold, 0.7f));
  slanted({r.x + r.width + 48, r.y, 12, r.height}, 40, withAlpha(kGold, 0.6f), withAlpha(kGold, 0.3f));
  drawTextOutlined(title, 36, r.y + 10, 52, kCream, kInk, 4.0f, FontKind::Title);
  if (subtitle) drawTextOutlined(subtitle, 44, r.y + r.height + 10, 19, kMuted, kInk, 2.0f);
  (void)time;
}

float keycapWidth(const char* label, float h) { return std::max(h, textWidth(label, h * 0.48f) + h * 0.7f); }

float keycap(const char* label, float x, float y, float h, bool lit) {
  float ts = h * 0.48f;
  float w = keycapWidth(label, h);
  DrawRectangleRounded({x, y + 3, w, h}, 0.35f, 6, shade(lit ? kGold : kCream, 0.55f));
  DrawRectangleRounded({x, y, w, h}, 0.35f, 6, lit ? kGold : kCream);
  drawText(label, x + (w - textWidth(label, ts)) * 0.5f, y + (h - ts) * 0.5f - 1, ts, kInk);
  return w;
}

// ---- Button prompts -------------------------------------------------------------------
namespace {
const Color kPlastic = {46, 44, 66, 255};    // dark buttons
const Color kRim = {146, 144, 182, 255};     // their edge (visible on dark backgrounds)
const Color kLight = {230, 228, 244, 255};   // shoulders, sticks, d-pad

// Text centred on c (capitals look centred).
void centred(const char* s, Vector2 c, float size, Color col) {
  drawText(s, c.x - textWidth(s, size) * 0.5f, c.y - size * 0.5f, size, col);
}
void roundLine(Vector2 a, Vector2 b, float t, Color col) {
  DrawLineEx(a, b, t, col);
  DrawCircleV(a, t * 0.5f, col);
  DrawCircleV(b, t * 0.5f, col);
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
    darkDisc(c, r, {66, 64, 84, 255});
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
void startGlyph(float x, float y, float h, PadFamily f) {
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
  if (f == PadFamily::Nintendo) {
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
    case Btn::Start: return f == PadFamily::PlayStation ? h * 1.1f : h * 0.9f;
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

float actionPromptWidth(int device, int action, float h) {
  if (action < 0 || action >= kActionCount || device < 0 || device >= kDeviceCount) return 0.0f;
  if (device >= kPad1) return padGlyphWidth(padButtonFor(action), h, deviceFamily(device));
  return keycapWidth(actionKey(device, action).c_str(), h);
}

float actionPrompt(int device, int action, float x, float y, float h, bool lit) {
  if (action < 0 || action >= kActionCount || device < 0 || device >= kDeviceCount) return 0.0f;
  if (device < kPad1) return keycap(actionKey(device, action).c_str(), x, y, h, lit);
  const Btn b = padButtonFor(action);
  const PadFamily f = deviceFamily(device);
  if (lit) glow({x + padGlyphWidth(b, h, f) * 0.5f, y + h * 0.5f}, h * 1.2f, kGold);
  return padGlyph(b, x, y, h, f);
}

void footer(std::initializer_list<Hint> hints, const char* info) {
  const float h = 46, y = kScreenH - h, kh = 28;
  DrawRectangleGradientV(0, int(y - 30), kScreenW, 30, withAlpha(kInk, 0.0f), withAlpha(kInk, 0.6f));
  DrawRectangle(0, int(y), kScreenW, int(h), withAlpha(kInk, 0.88f));
  DrawRectangle(0, int(y), kScreenW, 2, withAlpha(kGold, 0.7f));
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
  DrawRectangleRoundedLinesEx(r, roundness, 8, 3, border);
}

void focusFrame(Rectangle r, float time, float roundness) {
  float pulse = 0.5f + 0.5f * std::sin(time * 7.0f);
  DrawRectangleRoundedLinesEx(grow(r, 5 + pulse * 2), roundness, 8, 6, withAlpha(kGold, 0.25f + 0.2f * pulse));
  DrawRectangleRoundedLinesEx(grow(r, 2), roundness, 8, 3.5f, withAlpha(kGold, 0.75f + 0.25f * pulse));
}

void arrowTri(Vector2 c, float s, int dir, Color col) {
  DrawTriangle({c.x + dir * s * 0.6f, c.y}, {c.x - dir * s * 0.4f, c.y - s * 0.6f},
               {c.x - dir * s * 0.4f, c.y + s * 0.6f}, col);
}

void slider(Rectangle r, float v, bool focused, Color fill, float time) {
  v = clampf(v, 0.0f, 1.0f);
  DrawRectangleRounded(grow(r, 2), 1.0f, 12, kInk);
  DrawRectangleRounded(r, 1.0f, 12, {44, 42, 74, 255});
  if (v > 0.01f) {
    Rectangle f{r.x, r.y, std::max(r.height, r.width * v), r.height};
    DrawRectangleRounded(f, 1.0f, 12, fill);
    DrawRectangleRounded({f.x + 4, f.y + 2, f.width - 8, f.height * 0.35f}, 1.0f, 8, withAlpha(WHITE, 0.3f));
  }
  Vector2 k{r.x + r.width * v, r.y + r.height * 0.5f};
  float kr = r.height * 0.95f;
  if (focused) glow(k, kr * 2.2f, withAlpha(kGold, 0.8f), 6);
  DrawCircleV(k, kr + 2.5f, kInk);
  DrawCircleV(k, kr, focused ? kGold : kCream);
  DrawCircleV({k.x - kr * 0.3f, k.y - kr * 0.3f}, kr * 0.3f, withAlpha(WHITE, 0.6f));
  (void)time;
}

void toggle(Rectangle r, bool on, bool focused, float time) {
  Color bg = on ? Color{60, 190, 105, 255} : Color{90, 88, 115, 255};
  DrawRectangleRounded(grow(r, 2), 1.0f, 12, kInk);
  DrawRectangleRounded(r, 1.0f, 12, bg);
  const char* txt = on ? "OUI" : "NON";
  float ts = r.height * 0.5f;
  float kr = r.height * 0.5f - 4;
  float kx = on ? r.x + r.width - kr - 5 : r.x + kr + 5;
  float tx = on ? r.x + 12 : r.x + r.width - 12 - textWidth(txt, ts);
  drawText(txt, tx, r.y + (r.height - ts) * 0.5f - 1, ts, kCream);
  DrawCircleV({kx, r.y + r.height * 0.5f}, kr, focused ? kGold : kCream);
  if (focused) DrawRectangleRoundedLinesEx(grow(r, 4), 1.0f, 12, 3, withAlpha(kGold, 0.6f + 0.4f * std::sin(time * 7)));
}

void selector(Rectangle r, const char* value, bool focused, float time) {
  DrawRectangleRounded(grow(r, 2), 0.5f, 10, kInk);
  DrawRectangleRounded(r, 0.5f, 10, {44, 42, 74, 255});
  float ts = r.height * 0.48f;
  drawText(value, r.x + (r.width - textWidth(value, ts)) * 0.5f, r.y + (r.height - ts) * 0.5f - 1, ts, kCream);
  float bob = focused ? std::sin(time * 8.0f) * 2.0f : 0.0f;
  Color ac = focused ? kGold : kMuted;
  arrowTri({r.x + 16 - bob, r.y + r.height * 0.5f}, r.height * 0.42f, -1, ac);
  arrowTri({r.x + r.width - 16 + bob, r.y + r.height * 0.5f}, r.height * 0.42f, 1, ac);
  if (focused) DrawRectangleRoundedLinesEx(grow(r, 4), 0.5f, 10, 3, withAlpha(kGold, 0.6f + 0.4f * std::sin(time * 7)));
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
  band(12, {225, 55, 55, 255});
  band(0, kInk);
}

}  // namespace ui
