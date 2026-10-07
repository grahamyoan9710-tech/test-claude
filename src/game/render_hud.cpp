// In-match HUD: damage cards, stocks, off-screen bubbles and countdown.
#include <cstdio>

#include "game/render.hpp"
#include "game/ui.hpp"

void drawHud(const Match& m, const std::array<float, 4>& shake, float time) {
  const int n = int(m.fighters.size());
  const float pw = 240.0f, ph = 90.0f, gap = 18.0f;
  const float total = n * pw + (n - 1) * gap;
  float x = (kScreenW - total) * 0.5f;
  const float y = kScreenH - ph - 12.0f;
  for (const auto& f : m.fighters) {
    Color pc = slotColor(f);
    Rectangle card{x, y, pw, ph};
    ui::slanted({x + 5, y + 6, pw, ph}, 16, withAlpha(BLACK, 0.4f), withAlpha(BLACK, 0.4f));
    if (f.eliminated) {
      ui::slanted(card, 16, {80, 80, 92, 235}, {40, 40, 50, 235});
    } else {
      ui::slanted(card, 16, withAlpha(mixColor(pc, BLACK, 0.3f), 0.95f), withAlpha(mixColor(pc, BLACK, 0.72f), 0.95f));
      ui::slanted({x + 14, y, pw - 14, 10}, 2, withAlpha(WHITE, 0.25f), withAlpha(WHITE, 0.0f));
    }
    ui::slantedOutline(card, 16, 3, withAlpha(ui::kInk, 0.95f));
    if (f.finalSmash) {
      Color rb = ColorFromHSV(std::fmod(time * 300.0f, 360.0f), 0.8f, 1.0f);
      ui::slantedOutline(ui::grow(card, 3), 16, 4, rb);
      drawTextOutlined("SUPER !", x + pw - 92, y - 18, 24, rb, ui::kInk, 3.0f, FontKind::Title);
    }
    DrawCircleV({x + 50, y + 46}, 36, withAlpha(BLACK, 0.4f));
    DrawRing({x + 50, y + 46}, 34, 37, 0, 360, 36, withAlpha(pc, 0.9f));
    drawPortrait(*f.def, f.palette, {x + 50, y + 44}, 62, time + f.id);

    char label[16];
    std::snprintf(label, sizeof label, f.cpu ? "CPU" : "J%d", f.port + 1);
    drawTextOutlined(f.def->name.c_str(), x + 94, y + 6, 22, WHITE, ui::kInk, 2.5f, FontKind::Title);
    float lw = textWidth(label, 15) + 16;
    DrawRectangleRounded({x + pw - lw - 10, y + 8, lw, 20}, 0.5f, 6, pc);
    drawText(label, x + pw - lw - 2, y + 9, 15, WHITE);

    if (f.eliminated) {
      drawTextOutlined("K.O.", x + 104, y + 34, 42, {210, 210, 220, 255}, ui::kInk, 3.0f, FontKind::Title);
    } else {
      char pct[16];
      std::snprintf(pct, sizeof pct, "%d", int(f.percent));
      float sh = shake[f.id & 3];
      float jx = sh > 0 ? std::sin(time * 90.0f) * sh : 0.0f;
      float jy = sh > 0 ? std::cos(time * 77.0f) * sh : 0.0f;
      Color col = percentColor(f.percent);
      float size = 50.0f + std::min(8.0f, sh);
      float w = textWidth(pct, size, FontKind::Title);
      drawTextOutlined(pct, x + 180 - w + jx, y + 30 + jy, size, col, ui::kInk, 4.0f, FontKind::Title);
      drawTextOutlined("%", x + 184 + jx, y + 50 + jy, 26, col, ui::kInk, 3.0f, FontKind::Title);
      if (!m.cfg.training) {
        for (int i = 0; i < std::min(f.stocks, 6); ++i) {
          Vector2 c{x + 104.0f + i * 17.0f, y + 78.0f};
          DrawCircleV(c, 7, ui::kInk);
          DrawCircleV(c, 5.5f, C(f.def->palettes[f.palette].primary));
          DrawCircleV({c.x - 1.5f, c.y - 1.5f}, 2.0f, withAlpha(WHITE, 0.6f));
        }
        if (f.stocks > 6) drawTextOutlined(TextFormat("x%d", f.stocks), x + 210, y + 70, 15, WHITE);
      }
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
  auto big = [&](ui::Sprite sp, float size) {
    ui::drawSprite(sp, cx - ui::spriteWidth(sp, size) * 0.5f, cy - size * 0.55f, size);
  };
  if (m.countdown > 0) {
    int n = (m.countdown + 59) / 60;
    if (m.countdown > 180) {
      big(ui::Sprite::Ready, 96);
      return;
    }
    float k = float(m.countdown % 60) / 60.0f;
    float size = 150.0f + 90.0f * k * k;
    big(n == 3 ? ui::Sprite::Count3 : n == 2 ? ui::Sprite::Count2 : ui::Sprite::Count1, size);
  } else if (m.frame < 200 + 50) {
    float k = (m.frame - 200) / 50.0f;
    if (k < 0.8f) big(ui::Sprite::Go, 190.0f + 40.0f * k);
  }
}
