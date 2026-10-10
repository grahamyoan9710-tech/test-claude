// In-match HUD: damage cards, stocks, off-screen bubbles and countdown.
#include <cstdio>

#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
// Ultimate meter: a row of glowing "Zonai battery" cells above the damage card.
void drawUltBar(const Fighter& f, float x, float y, float w, float fill, float time) {
  const int cells = 5;
  const float h = 13.0f, slant = 3.0f, gap = 4.0f, labelW = 44.0f;
  const bool ready = f.finalSmash;
  const float pulse = ready ? 0.5f + 0.5f * std::sin(time * 7.0f) : 0.0f;
  const Color hi = {150, 255, 214, 255}, lo = {24, 176, 140, 255};
  ui::slanted({x - 4, y - 3, w + 8, h + 6}, slant + 1, withAlpha(ui::kInk, 0.82f), withAlpha(ui::kInk, 0.92f));
  ui::slantedOutline({x - 4, y - 3, w + 8, h + 6}, slant + 1, 1.5f,
                     ready ? mixColor(lo, hi, pulse) : withAlpha({120, 210, 190, 255}, 0.55f));
  drawText("ULTI", x + 2, y - 2.5f, 15, ready ? mixColor(hi, WHITE, pulse) : Color{170, 220, 205, 255});
  const float cx0 = x + labelW, cw = (w - labelW - gap * (cells - 1)) / cells;
  for (int i = 0; i < cells; ++i) {
    Rectangle c{cx0 + i * (cw + gap), y, cw, h};
    ui::slanted(c, slant, {38, 52, 58, 255}, {22, 30, 36, 255});
    float k = clampf(fill * cells - i, 0.0f, 1.0f);
    if (k <= 0.0f) continue;
    Rectangle in{c.x, c.y, c.width * k, c.height};
    Color top = ready ? mixColor(hi, WHITE, 0.35f * pulse) : hi;
    Color bottom = ready ? mixColor(lo, hi, 0.5f * pulse) : lo;
    ui::slanted(in, slant * k, top, bottom);
    ui::slanted({in.x + 2, in.y + 1, in.width - 3, 3}, 0.5f, withAlpha(WHITE, 0.45f), withAlpha(WHITE, 0.0f));
  }
  if (ready) {  // shimmer running along the cells + soft glow
    float sx = cx0 + std::fmod(time * 260.0f, w - labelW + 60.0f) - 30.0f;
    for (int i = 0; i < 3; ++i)
      ui::glow({cx0 + (w - labelW) * (0.17f + 0.33f * i), y + h * 0.5f}, 30.0f + 6.0f * pulse,
               withAlpha(hi, 0.22f + 0.12f * pulse), 4);
    if (sx > cx0 && sx < x + w - 10)
      ui::quad({sx + slant, y}, {sx + 10 + slant, y}, {sx + 10, y + h}, {sx, y + h}, withAlpha(WHITE, 0.0f),
               withAlpha(WHITE, 0.7f), withAlpha(WHITE, 0.7f), withAlpha(WHITE, 0.0f));
  }
}
}  // namespace

void drawHud(const Match& m, const std::array<float, 4>& shake, float time, const std::array<int, 4>* devices) {
  static float shown[4] = {0, 0, 0, 0};  // displayed ultimate fill, eased towards the real meter
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
    float& sh = shown[f.id & 3];
    const float fill = f.ult / kUltMax;
    sh = fill < sh ? std::max(fill, sh - 0.08f) : sh + (fill - sh) * 0.12f;
    if (!f.eliminated) drawUltBar(f, x + 22, y - 22, pw - 30, sh, time + f.id);
    if (f.finalSmash) {
      float pulse = 0.5f + 0.5f * std::sin(time * 7.0f);
      Color zon = mixColor({90, 240, 200, 255}, {220, 255, 240, 255}, pulse);
      ui::slantedOutline(ui::grow(card, 3), 16, 3.5f, zon);
      const char* ready = "ULTI PRÊTE !";
      float tw = textWidth(ready, 22, FontKind::Title);
      float ty = y - 50 - 3 * pulse;
      float bw = devices && !f.cpu ? ui::actionPromptWidth((*devices)[f.port & 3], kActUltimate, 24) : 0.0f;
      float tx = x + pw - tw - 4 - (bw > 0 ? bw + 6 : 0);
      drawTextOutlined(ready, tx, ty, 22, zon, ui::kInk, 3.0f, FontKind::Title);
      if (bw > 0) ui::actionPrompt((*devices)[f.port & 3], kActUltimate, tx + tw + 6, ty, 24, true);
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
