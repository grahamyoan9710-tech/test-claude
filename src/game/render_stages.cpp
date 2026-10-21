// The five arenas, one per fighter, painted with PixelLab (tools/pixellab/pxa/arenas, scripts/import_arenas.py):
// assets/sprites/arena_<key>_bg.png is the backdrop, arena_<key>_solid.png and arena_<key>_thin.png the platforms.
#include <string>

#include "game/fighter_art.hpp"
#include "game/render.hpp"
#include "game/stage_art.hpp"

namespace {
struct Arena {
  const char* key;
  float solidTop, thinTop;  // the walkable top of each platform picture, in pixels below its first row
};
const Arena* arena(StageTheme theme) {
  static const Arena kLunar{"lunaire", 0, 0}, kFrost{"givre", 0, 4}, kCrimson{"ecarlate", 0, 6},
      kKurenai{"kurenai", 12, 2}, kClouds{"nuees", 76, 2};
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
  const std::string name = pictureName(*a, p.solid ? "solid" : "thin");
  float w, h;
  if (!art::pictureSize(name.c_str(), w, h)) return;
  // Scaled to the platform's width, its proportions kept (stretched pixel art smears); the ends hang a little past
  // the platform's edges.
  const float pad = 10.0f, dw = p.rect.w + 2.0f * pad, k = dw / w;
  art::drawPicture(name.c_str(), {0, 0, w, h},
                   {p.rect.x - pad, p.rect.y - (p.solid ? a->solidTop : a->thinTop) * k, dw, h * k});
}

void drawForeground(StageTheme, Vec2, float) {}
}  // namespace stageart
