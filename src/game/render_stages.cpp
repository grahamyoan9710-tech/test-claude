// The five arenas, one per fighter, painted with PixelLab (tools/pixellab/pxa/arenas, scripts/import_art.py):
// assets/sprites/arena_<key>_bg.png is the backdrop, arena_<key>_solid.png, _thin.png (and _small.png, the small
// islands) the platforms, each generated at the size it is drawn.
#include <string>

#include "game/fighter_art.hpp"
#include "game/render.hpp"
#include "game/stage_art.hpp"

namespace {
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
};
const Arena* arena(StageTheme theme) {
  static const Arena kLunar{"lunaire", 26, 0, 0, 3, 48, 0.40f, false, 30, 4, 0.9f},  // polished stone
      kFrost{"givre", 18, 12, 0, 10, 31, 0.35f, false, 76, 62, 0.6f},                  // icy roof tiles, snow around
      kCrimson{"ecarlate", 22, 14, 0, 4, 26, 0.0f, false, 0, 0, 1.0f},                 // dry sandstone
      kKurenai{"kurenai", 54, 10, 0, 30, 77, 1.0f, true, 142, 108, 1.6f},              // wet stone in the rain, at night
      kClouds{"nuees", 72, 18, 10, 58, 115, 0.22f, false, 72, 4, 0.5f};                // polished flagstones, in daylight
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
}  // namespace

namespace stageart {
void drawBackground(StageTheme theme, Vec2, float skyX, float skyY, float) {
  const Arena* a = arena(theme);
  float w, h;
  if (!a || !art::pictureSize(pictureName(*a, "bg").c_str(), w, h)) return;
  // Covers the screen plus the 80 px the parallax slides it through ((skyX, skyY) go from -80 to 0).
  const float bw = kScreenW + 80.0f, bh = kScreenH + 80.0f, k = std::max(bw / w, bh / h);
  art::drawPicture(pictureName(*a, "bg").c_str(), {0, 0, w, h},
                   {skyX + (bw - w * k) * 0.5f, skyY + (bh - h * k) * 0.5f, w * k, h * k});
}

void drawPlatform(const Platform& p, StageTheme theme, float) {
  const Arena* a = arena(theme);
  if (!a) return;
  std::string name = pictureName(*a, p.solid ? "solid" : "thin");
  float top = p.solid ? a->solidTop : a->thinTop, w, h;
  if (p.solid && p.rect.w < 300.0f && art::pictureSize(pictureName(*a, "small").c_str(), w, h)) {  // a small island
    name = pictureName(*a, "small");
    top = a->smallTop;
  }
  if (!art::pictureSize(name.c_str(), w, h)) return;
  // Scaled to the platform's width, its proportions kept (stretched pixel art smears); the ends hang a little past
  // the platform's edges.
  const float pad = 10.0f, dw = p.rect.w + 2.0f * pad, k = dw / w;
  art::drawPicture(name.c_str(), {0, 0, w, h},
                   {p.rect.x - pad, p.rect.y - top * k, dw, h * k});
}

void drawForeground(StageTheme, Vec2, float) {}

float lightGain(StageTheme theme) {
  const Arena* a = arena(theme);
  return a ? a->light : 1.0f;
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
}  // namespace stageart
