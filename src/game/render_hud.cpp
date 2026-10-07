// In-match HUD: damage cards, stocks, off-screen bubbles and countdown.
#include <cstdio>

#include "game/render.hpp"

void drawHud(const Match& m, const std::array<float, 4>& shake, float time) {
  const int n = int(m.fighters.size());
  const float pw = 236.0f, ph = 92.0f, gap = 18.0f;
  const float total = n * pw + (n - 1) * gap;
  float x = (kScreenW - total) * 0.5f;
  const float y = kScreenH - ph - 12.0f;
  for (const auto& f : m.fighters) {
    Color pc = slotColor(f);
    Color bg = f.eliminated ? Color{70, 70, 80, 220} : withAlpha(shade(pc, 0.55f), 0.9f);
    DrawRectangleRounded({x + 4, y + 5, pw, ph}, 0.3f, 8, withAlpha(BLACK, 0.35f));
    DrawRectangleRounded({x, y, pw, ph}, 0.3f, 8, bg);
    DrawRectangleRoundedLinesEx({x, y, pw, ph}, 0.3f, 8, 3, f.eliminated ? GRAY : pc);
    DrawCircleV({x + 48, y + 46}, 38, withAlpha(BLACK, 0.35f));
    drawPortrait(*f.def, f.palette, {x + 48, y + 44}, 64, time + f.id);

    char label[16];
    std::snprintf(label, sizeof label, f.cpu ? "CPU" : "J%d", f.port + 1);
    drawTextOutlined(f.def->name.c_str(), x + 92, y + 8, 18, WHITE);
    drawTextOutlined(label, x + pw - 12 - textWidth(label, 16), y + 9, 16, pc);

    if (f.eliminated) {
      drawTextOutlined("K.O.", x + 100, y + 32, 40, {200, 200, 210, 255});
    } else {
      char pct[16];
      std::snprintf(pct, sizeof pct, "%d", int(f.percent));
      float s = shake[f.id & 3];
      float jx = s > 0 ? std::sin(time * 90.0f) * s : 0.0f;
      float jy = s > 0 ? std::cos(time * 77.0f) * s : 0.0f;
      Color col = percentColor(f.percent);
      float size = 46.0f + std::min(8.0f, s);
      float w = textWidth(pct, size);
      drawTextOutlined(pct, x + 168 - w + jx, y + 28 + jy, size, col, {15, 15, 25, 255}, 3.0f);
      drawTextOutlined("%", x + 172 + jx, y + 46 + jy, 24, col);
      for (int i = 0; i < std::min(f.stocks, 6); ++i) {
        Vector2 c{x + 100.0f + i * 18.0f, y + 79.0f};
        DrawCircleV(c, 7, {20, 18, 30, 255});
        DrawCircleV(c, 5.5f, C(f.def->palettes[f.palette].primary));
      }
      if (f.stocks > 6) drawTextOutlined(TextFormat("x%d", f.stocks), x + 210, y + 72, 14, WHITE);
    }
    x += pw + gap;
  }
}

void drawOffscreenBubbles(const Match& m, const Camera2D& cam, float time) {
  const float margin = 56.0f;
  const float bottom = kScreenH - 130.0f;
  for (const auto& f : m.fighters) {
    if (!f.active()) continue;
    Vector2 sp = GetWorldToScreen2D(V(f.center()), cam);
    if (sp.x > -10 && sp.x < kScreenW + 10 && sp.y > -10 && sp.y < kScreenH + 10) continue;
    Vector2 b{clampf(sp.x, margin, kScreenW - margin), clampf(sp.y, margin, bottom)};
    Color pc = slotColor(f);
    Vec2 dir = (W(sp) - W(b)).norm();
    Vec2 tip = W(b) + dir * 56.0f;
    Vec2 perp{-dir.y, dir.x};
    Vector2 t0 = V(W(b) + dir * 38.0f + perp * 12.0f), t1 = V(tip), t2 = V(W(b) + dir * 38.0f - perp * 12.0f);
    float cross = (t1.x - t0.x) * (t2.y - t0.y) - (t1.y - t0.y) * (t2.x - t0.x);
    if (cross > 0) std::swap(t1, t2);
    DrawTriangle(t0, t1, t2, pc);
    DrawCircleV(b, 44, pc);
    DrawCircleV(b, 39, withAlpha({235, 240, 250, 255}, 0.9f));
    float scale = 52.0f / f.def->height;
    Pose pose = computePose(f, m, time);
    drawCharacter(*f.def, f.palette, pose, {b.x, b.y + f.def->height * scale * 0.5f}, f.facing, scale, 1.0f, WHITE,
                  f.flash > 0 ? 0.5f : 0.0f, time);
    float dist = (W(sp) - W(b)).len();
    if (dist > 200.0f) drawTextCentered(TextFormat("%dm", int(dist / 30.0f)), b.x, b.y + 46, 14, WHITE);
  }
}

void drawCountdown(const Match& m) {
  const float cx = kScreenW * 0.5f, cy = kScreenH * 0.38f;
  if (m.countdown > 0) {
    int n = (m.countdown + 59) / 60;
    if (m.countdown > 180) {
      drawTextCentered("PRETS ?", cx, cy - 30, 64, {255, 240, 200, 255});
      return;
    }
    float k = float(m.countdown % 60) / 60.0f;
    float size = 120.0f + 80.0f * k;
    Color col = n == 3 ? Color{255, 90, 90, 255} : n == 2 ? Color{255, 200, 60, 255} : Color{120, 230, 120, 255};
    drawTextCentered(TextFormat("%d", n), cx, cy - size * 0.5f, size, withAlpha(col, 0.4f + 0.6f * k));
  } else if (m.frame < 200 + 50) {
    float k = (m.frame - 200) / 50.0f;
    float size = 150.0f + 60.0f * k;
    drawTextCentered("GO !", cx, cy - size * 0.5f, size, withAlpha({255, 255, 255, 255}, 1.0f - k * k));
  }
}
