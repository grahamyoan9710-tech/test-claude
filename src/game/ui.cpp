#include "game/ui.hpp"

#include <vector>

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

float keycap(const char* label, float x, float y, float h, bool lit) {
  float ts = h * 0.48f;
  float w = std::max(h, textWidth(label, ts) + h * 0.7f);
  DrawRectangleRounded({x, y + 3, w, h}, 0.35f, 6, shade(lit ? kGold : kCream, 0.55f));
  DrawRectangleRounded({x, y, w, h}, 0.35f, 6, lit ? kGold : kCream);
  drawText(label, x + (w - textWidth(label, ts)) * 0.5f, y + (h - ts) * 0.5f - 1, ts, kInk);
  return w;
}

void footer(std::initializer_list<std::pair<const char*, const char*>> hints, const char* info) {
  const float h = 46, y = kScreenH - h;
  DrawRectangleGradientV(0, int(y - 30), kScreenW, 30, withAlpha(kInk, 0.0f), withAlpha(kInk, 0.6f));
  DrawRectangle(0, int(y), kScreenW, int(h), withAlpha(kInk, 0.88f));
  DrawRectangle(0, int(y), kScreenW, 2, withAlpha(kGold, 0.7f));
  // Measure from the right
  float total = 0;
  for (const auto& [key, label] : hints) {
    float kw = std::max(28.0f, textWidth(key, 28 * 0.48f) + 28 * 0.7f);
    total += kw + 8 + textWidth(label, 18) + 26;
  }
  float x = kScreenW - 24 - total;
  for (const auto& [key, label] : hints) {
    x += keycap(key, x, y + 9, 28) + 8;
    drawText(label, x, y + 13, 18, kCream);
    x += textWidth(label, 18) + 26;
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
