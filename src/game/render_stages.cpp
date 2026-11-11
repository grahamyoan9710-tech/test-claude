// The five arenas, one per fighter, painted with PixelLab (tools/pixellab/pxa/arenas, scripts/import_art.py):
// assets/sprites/arena_<key>_bg.png is the backdrop, arena_<key>_solid.png, _thin.png (and _small.png, the small
// islands) the platforms, each generated at the size it is drawn.
#include <string>

#include "game/fighter_art.hpp"
#include "game/render.hpp"
#include "game/stage_art.hpp"

namespace {
struct Top {
  float back, front, gloss, rough;
};
struct Key {  // RTX: the moon or the sun, in its backdrop's pixels (beyond the picture: off the screen)
  float x, y, reach;  // where, how far it lights
  float light[3];     // its colour times its strength (near white: a warm one overheated the warm arenas)
};
struct Arena {
  const char* key;
  // Where the fighters walk on each platform picture, in pixels below its first row: the middle of its top floor, seen
  // in depth as on a Smash stage.
  float solidTop, thinTop, smallTop;
  // The top floor of the solid picture: its back and front edges (rows), how much it mirrors, rain on it.
  float floorBack, floorFront, gloss;
  bool rain;
  float insetBack, insetFront;  // from each end of the picture to the floor, on its back and front rows (pixels)
  float light;  // how much the effects' light shows: more at night than in daylight
  float lamps;  // RTX: how much its lanterns light the scene; 0 where its sky or walls are as bright (they would glow)
  // RTX, what the reflections show of each matter: how blurred the floor mirrors (1: polished; more: rougher), and the
  // top of the thin platforms and of the small islands (rows in their picture, how much and how blurred it mirrors).
  float floorRough;
  Top thin, small;
  Key sky;
};
const Arena* arena(StageTheme theme) {
  // Polished stone; icy roof tiles, snow logs; dry sandstone, dry planks; wet stone and wet planks in the rain, at
  // night; polished flagstones, red lacquered planks and stone slabs, in daylight.
  // Their key lights: the big pale moon; the low dusk sun on the left; the sun setting behind the right cliff; the
  // small red moon behind the rain; the high day sun, up on the right.
  static const Arena kLunar{"lunaire", 26, 0, 0, 3, 48, 0.40f, false, 30, 4, 0.9f, 0.0f, 0.8f, {}, {},
                            {935, 150, 600, {0.8f, 0.76f, 0.9f}}},
      kFrost{"givre", 18, 12, 0, 10, 31, 0.35f, false, 76, 62, 0.6f, 0.0f, 1.6f, {3, 13, 0.1f, 4.0f}, {},
             {-250, 330, 2000, {0.76f, 0.66f, 0.6f}}},
      kCrimson{"ecarlate", 22, 14, 0, 4, 26, 0.0f, false, 0, 0, 1.0f, 0.0f, 1.0f, {2, 27, 0.12f, 3.0f}, {},
               {1180, 230, 1600, {0.82f, 0.74f, 0.62f}}},
      kKurenai{"kurenai", 54, 10, 0, 30, 77, 1.0f, true, 142, 108, 1.6f, 0.6f, 0.6f, {2, 12, 0.45f, 1.6f}, {},
               {874, 87, 700, {0.38f, 0.28f, 0.27f}}},
      kClouds{"nuees", 72, 18, 10, 58, 115, 0.22f, false, 72, 4, 0.5f, 0.0f, 1.0f, {4, 31, 0.3f, 0.8f},
              {2, 16, 0.22f, 1.0f}, {1250, -200, 2500, {0.86f, 0.84f, 0.78f}}};
  switch (theme) {
    case StageTheme::Lunar: return &kLunar;
    case StageTheme::Frost: return &kFrost;
    case StageTheme::Crimson: return &kCrimson;
    case StageTheme::Kurenai: return &kKurenai;
    case StageTheme::Clouds: return &kClouds;
    default: return nullptr;
  }
}
std::string pictureName(const Arena& a, const char* part) { return std::string("arena_") + a.key + "_" + part; }
struct {
  float x, y, k;
} g_sky{0, 0, 1};  // where the backdrop was last drawn on the screen, and its scale
}  // namespace

namespace stageart {
void drawBackground(StageTheme theme, Vec2, float skyX, float skyY, float) {
  const Arena* a = arena(theme);
  float w, h;
  if (!a || !art::pictureSize(pictureName(*a, "bg").c_str(), w, h)) return;
  // Covers the screen plus the 80 px the parallax slides it through ((skyX, skyY) go from -80 to 0).
  const float bw = kScreenW + 80.0f, bh = kScreenH + 80.0f, k = std::max(bw / w, bh / h);
  g_sky = {skyX + (bw - w * k) * 0.5f, skyY + (bh - h * k) * 0.5f, k};
  art::drawPicture(pictureName(*a, "bg").c_str(), {0, 0, w, h}, {g_sky.x, g_sky.y, w * k, h * k});
}

// part 0: the whole picture; 1: the top of a thin platform or a small island (its Top rows, down to its front edge);
// 2: the rest, its body (all of it without a top).
static void drawRows(const Platform& p, StageTheme theme, int part) {
  const Arena* a = arena(theme);
  if (!a) return;
  std::string name = pictureName(*a, p.solid ? "solid" : "thin");
  float top = p.solid ? a->solidTop : a->thinTop, w, h;
  const Top* t = p.solid ? nullptr : &a->thin;
  if (p.solid && p.rect.w < 300.0f && art::pictureSize(pictureName(*a, "small").c_str(), w, h)) {  // a small island
    name = pictureName(*a, "small");
    top = a->smallTop;
    t = &a->small;
  }
  if (!art::pictureSize(name.c_str(), w, h)) return;
  const float cut = part != 0 && t && t->front > t->back ? t->front : 0.0f;
  const float r0 = part == 2 ? cut : 0.0f, r1 = part == 1 ? cut : h;
  if (r1 <= r0) return;
  // Scaled to the platform's width, its proportions kept (stretched pixel art smears); the ends hang a little past
  // the platform's edges.
  const float pad = 10.0f, dw = p.rect.w + 2.0f * pad, k = dw / w;
  art::drawPicture(name.c_str(), {0, r0, w, r1 - r0}, {p.rect.x - pad, p.rect.y + (r0 - top) * k, dw, (r1 - r0) * k});
}

void drawPlatform(const Platform& p, StageTheme theme, float) { drawRows(p, theme, 0); }

void drawPlatformPart(const Platform& p, StageTheme theme, bool top) { drawRows(p, theme, top ? 1 : 2); }

void drawForeground(StageTheme, Vec2, float) {}

float lightGain(StageTheme theme) {
  const Arena* a = arena(theme);
  return a ? a->light : 1.0f;
}

float lampGain(StageTheme theme) {
  const Arena* a = arena(theme);
  return a ? a->lamps : 0.0f;
}

bool keyLight(StageTheme theme, KeyLight& out) {
  const Arena* a = arena(theme);
  if (!a || a->sky.reach <= 0.0f) return false;
  const Key& s = a->sky;
  out = {{g_sky.x + s.x * g_sky.k, g_sky.y + s.y * g_sky.k}, s.reach * g_sky.k, {s.light[0], s.light[1], s.light[2]}};
  return true;
}

bool floorOf(const Platform& p, StageTheme theme, Floor& out) {
  const Arena* a = arena(theme);
  float w, h;
  if (!a || !p.solid || p.rect.w < 300.0f || !art::pictureSize(pictureName(*a, "solid").c_str(), w, h)) return false;
  const float k = (p.rect.w + 20.0f) / w;  // as drawPlatform scales it
  const float l = p.rect.x - 10.0f, r = l + w * k;
  out = {p.rect.y - (a->solidTop - a->floorBack) * k, p.rect.y, p.rect.y + (a->floorFront - a->solidTop) * k, a->gloss,
         a->rain, l + a->insetBack * k, r - a->insetBack * k, l + a->insetFront * k, r - a->insetFront * k};
  return true;
}

bool topOf(const Platform& p, StageTheme theme, Floor& out) {
  const Arena* a = arena(theme);
  if (!a) return false;
  if (floorOf(p, theme, out)) {
    out.rough = a->floorRough;
    return true;
  }
  // As drawPlatform places the picture: a thin platform, or a small island.
  float w, h;
  const bool small = p.solid && p.rect.w < 300.0f && art::pictureSize(pictureName(*a, "small").c_str(), w, h);
  if (p.solid && !small) return false;
  const Top& t = small ? a->small : a->thin;
  const float top = small ? a->smallTop : a->thinTop;
  if (t.front <= t.back || !art::pictureSize(pictureName(*a, small ? "small" : "thin").c_str(), w, h)) return false;
  const float k = (p.rect.w + 20.0f) / w, l = p.rect.x - 10.0f, r = l + w * k;
  out = {p.rect.y - (top - t.back) * k, p.rect.y, p.rect.y + (t.front - top) * k, t.gloss, a->rain, l, r, l, r, t.rough};
  return true;
}

bool mirrorOf(const Platform& p, StageTheme theme, Floor& out) { return topOf(p, theme, out) && out.gloss > 0.0f; }
}  // namespace stageart
