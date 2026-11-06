// --export-pose: one fighter in its rest pose (optionally holding a picked-up weapon), facing right, alone on a
// transparent square PNG — the first frame PixelLab animates for a clip. The fighter's feet are at a known pixel
// and its height a known number of pixels, so the frames PixelLab returns import without any guessing.
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <vector>

#include "game/render.hpp"
#include "game/ui.hpp"
#include "rlgl.h"

int exportPose(const ExportPose& e) {
  const auto& r = roster();
  int ci = -1;
  for (int i = 0; i < int(r.size()); ++i) {
    std::string name = r[i].name;
    for (auto& ch : name) ch = char(std::tolower(static_cast<unsigned char>(ch)));
    if (name == e.fighter) ci = i;
  }
  if (ci < 0) {
    std::fprintf(stderr, "unknown fighter %s\n", e.fighter.c_str());
    return 1;
  }
  const CharacterDef& d = r[ci];
  SetTraceLogLevel(LOG_WARNING);
  SetConfigFlags(FLAG_WINDOW_HIDDEN);
  InitWindow(320, 240, "export");
  rlDisableBackfaceCulling();  // shapes are wound both ways

  MatchConfig cfg;
  cfg.items = false;
  cfg.weapons = false;
  cfg.slots[0] = {true, ci, true, 5};
  cfg.slots[1] = {true, ci == 0 ? 1 : 0, true, 5};
  Match m(cfg);
  Fighter& f = m.fighters[0];
  if (e.slot >= 0) m.equipWeapon(f, e.slot);
  f.facing = 1;
  f.grounded = true;
  f.state = FState::Idle;
  f.stateFrame = 0;
  Pose p = computePose(f, m, 0.0f);
  p.sheet = -1;  // the skeleton, not a clip

  constexpr int kSuper = 4;  // supersampling: render textures get no MSAA
  const int n = e.size * kSuper;
  const float keep = g_renderScale;
  g_renderScale = float(kSuper);
  RenderTexture2D rt = LoadRenderTexture(n, n);
  BeginTextureMode(rt);
  ClearBackground(BLANK);
  rlPushMatrix();
  rlScalef(float(kSuper), float(kSuper), 1.0f);
  ui::frameBlend();
  drawCharacter(d, 0, p, {e.ax, e.ay}, 1, e.pxh / d.height, 1.0f, WHITE, 0.0f, 0.0f);
  EndBlendMode();
  rlPopMatrix();
  EndTextureMode();
  g_renderScale = keep;

  Image big = LoadImageFromTexture(rt.texture);
  ImageFlipVertical(&big);
  ImageFormat(&big, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
  const auto* px = static_cast<const unsigned char*>(big.data);
  Image out = GenImageColor(e.size, e.size, BLANK);
  auto* o = static_cast<unsigned char*>(out.data);
  for (int y = 0; y < e.size; ++y)
    for (int x = 0; x < e.size; ++x) {  // average the premultiplied block, then undo the premultiplication
      float c[4] = {0, 0, 0, 0};
      for (int j = 0; j < kSuper; ++j)
        for (int i = 0; i < kSuper; ++i) {
          const unsigned char* q = px + ((y * kSuper + j) * n + (x * kSuper + i)) * 4;
          for (int k = 0; k < 4; ++k) c[k] += q[k];
        }
      const float a = c[3] / (kSuper * kSuper);
      unsigned char* w = o + (y * e.size + x) * 4;
      if (a < 128.0f) {
        w[0] = w[1] = w[2] = w[3] = 0;
        continue;
      }
      for (int k = 0; k < 3; ++k) w[k] = (unsigned char)std::clamp(c[k] / c[3] * 255.0f, 0.0f, 255.0f);
      w[3] = 255;
    }
  const bool ok = ExportImage(out, e.out.c_str());
  UnloadImage(out);
  UnloadImage(big);
  UnloadRenderTexture(rt);
  CloseWindow();
  std::printf("{\"fighter\":\"%s\",\"slot\":%d,\"size\":%d,\"anchor\":[%.1f,%.1f],\"pxh\":%.1f}\n", e.fighter.c_str(),
              e.slot, e.size, e.ax, e.ay, e.pxh);
  return ok ? 0 : 1;
}
