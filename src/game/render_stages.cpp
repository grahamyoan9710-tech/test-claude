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
  float edge;  // from each end of the solid picture to its floor, on the walk row (pixels): rails and eaves hang past
};
const Arena* arena(StageTheme theme) {
  // Polished stone; icy roof tiles, snow logs; dry sandstone, dry planks; wet stone and wet planks in the rain, at
  // night; polished flagstones, red lacquered planks and stone slabs, in daylight.
  // Their key lights: the big pale moon; the low dusk sun on the left; the sun setting behind the right cliff; the
  // small red moon behind the rain; the high day sun, up on the right.
  static const Arena kLunar{"lunaire", 26, 0, 0, 3, 48, 0.40f, false, 30, 4, 0.9f, 0.0f, 0.8f, {}, {},
                            {935, 150, 600, {0.8f, 0.76f, 0.9f}}, 9},
      kFrost{"givre", 18, 12, 0, 10, 31, 0.35f, false, 76, 62, 0.6f, 0.0f, 1.6f, {3, 13, 0.1f, 4.0f}, {},
             {-250, 330, 2000, {0.76f, 0.66f, 0.6f}}, 72},
      kCrimson{"ecarlate", 22, 14, 0, 4, 26, 0.0f, false, 0, 0, 1.0f, 0.0f, 1.0f, {2, 27, 0.12f, 3.0f}, {},
               {1180, 230, 1600, {0.82f, 0.74f, 0.62f}}, 12},
      kKurenai{"kurenai", 54, 10, 0, 30, 77, 1.0f, true, 142, 108, 1.6f, 0.6f, 0.6f, {2, 12, 0.45f, 1.6f}, {},
               {874, 87, 700, {0.38f, 0.28f, 0.27f}}, 125},
      kClouds{"nuees", 72, 18, 10, 58, 115, 0.22f, false, 72, 4, 0.5f, 0.0f, 1.0f, {4, 31, 0.3f, 0.8f},
              {2, 16, 0.22f, 1.0f}, {1250, -200, 2500, {0.86f, 0.84f, 0.78f}}, 10};
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
// Where a platform's picture goes, in world units: its left end, its scale and the row the platform's top sits on. A
// solid platform is its floor's width, the picture's ends hanging past it; a thin one or a small island overhangs by 10.
struct Placed {
  std::string name;
  float top, l, k, w, h;
  bool small;
};
bool place(const Platform& p, const Arena& a, Placed& out) {
  out.small = p.solid && p.rect.w < 300.0f && art::pictureSize(pictureName(a, "small").c_str(), out.w, out.h);
  out.name = pictureName(a, out.small ? "small" : p.solid ? "solid" : "thin");
  out.top = out.small ? a.smallTop : p.solid ? a.solidTop : a.thinTop;
  if (!art::pictureSize(out.name.c_str(), out.w, out.h)) return false;
  if (p.solid && !out.small) {
    out.k = p.rect.w / (out.w - 2.0f * a.edge);
    out.l = p.rect.x - a.edge * out.k;
  } else {
    out.k = (p.rect.w + 20.0f) / out.w;
    out.l = p.rect.x - 10.0f;
  }
  return true;
}
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

void drawPlatform(const Platform& p, StageTheme theme, float) {
  const Arena* a = arena(theme);
  Placed pl;
  if (!a || !place(p, *a, pl)) return;
  // Scaled as a whole, its proportions kept (stretched pixel art smears).
  art::drawPicture(pl.name.c_str(), {0, 0, pl.w, pl.h}, {pl.l, p.rect.y - pl.top * pl.k, pl.w * pl.k, pl.h * pl.k});
}

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
  Placed pl;
  if (!a || !p.solid || p.rect.w < 300.0f || !place(p, *a, pl) || pl.small) return false;
  const float k = pl.k, l = pl.l, r = l + pl.w * k;
  out = {p.rect.y - (a->solidTop - a->floorBack) * k, p.rect.y, p.rect.y + (a->floorFront - a->solidTop) * k, a->gloss,
         a->rain, l + a->insetBack * k, r - a->insetBack * k, l + a->insetFront * k, r - a->insetFront * k};
  return true;
}

bool mirrorOf(const Platform& p, StageTheme theme, Floor& out) {
  const Arena* a = arena(theme);
  if (!a) return false;
  if (floorOf(p, theme, out)) {
    out.rough = a->floorRough;
    return out.gloss > 0.0f;
  }
  // A thin platform, or a small island.
  Placed pl;
  if (!place(p, *a, pl) || (p.solid && !pl.small)) return false;
  const Top& t = pl.small ? a->small : a->thin;
  if (t.gloss <= 0.0f) return false;
  const float k = pl.k, l = pl.l, r = l + pl.w * k;
  out = {p.rect.y - (pl.top - t.back) * k, p.rect.y, p.rect.y + (t.front - pl.top) * k, t.gloss, a->rain, l, r, l, r, t.rough};
  return true;
}
}  // namespace stageart
