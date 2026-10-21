// In-match HUD: damage cards, stocks, off-screen bubbles and countdown.
#include <cstdio>

#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

void drawHud(const Match& m, const std::array<float, 4>& shake, float time, const std::array<int, 4>* devices) {
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
      ui::slanted(card, 16, {70, 76, 76, 235}, {34, 40, 42, 235});
    } else {  // the main menu's dark plate, a bright band of the player colour on top
      ui::slanted(card, 16, withAlpha({36, 36, 48, 255}, 0.94f), withAlpha({14, 14, 22, 255}, 0.94f));
      ui::slanted({x + 16 - 16 * 6 / ph, y, pw - 16 + 16 * 6 / ph - 2, 6}, 16 * 6 / ph, pc, shade(pc, 0.8f));
    }
    ui::slantedOutline(card, 16, 1.5f, withAlpha({130, 130, 150, 255}, f.eliminated ? 0.3f : 0.55f));
    if (f.finalSmash) {
      float pulse = 0.5f + 0.5f * std::sin(time * 7.0f);
      Color zon = mixColor(ui::kRed, WHITE, pulse);
      ui::slantedOutline(ui::grow(card, 3), 16, 3.5f, zon);
      const char* ready = "ULTI PRÊTE !";
      float tw = textWidth(ready, 22, FontKind::Title);
      float ty = y - 50 - 3 * pulse;
      float bw = devices && !f.cpu ? ui::actionPromptWidth((*devices)[f.port & 3], kActUltimate, 24) : 0.0f;
      float tx = x + pw - tw - 4 - (bw > 0 ? bw + 6 : 0);
      drawTextOutlined(ready, tx, ty, 22, zon, Color{12, 12, 18, 255}, 3.0f, FontKind::Title);
      if (bw > 0) ui::actionPrompt((*devices)[f.port & 3], kActUltimate, tx + tw + 6, ty, 24, true);
    }
    DrawCircleV({x + 50, y + 46}, 36, withAlpha({12, 12, 18, 255}, 0.6f));
    DrawRing({x + 50, y + 46}, 33.5f, 37, 0, 360, 36, withAlpha(pc, 0.9f));
    drawPortraitDisc(*f.def, f.palette, {x + 50, y + 46}, 36, time + f.id);
    if (const WeaponDef* w = f.weaponDef()) {  // the weapon in hand: a badge on the portrait
      const Vector2 b{x + 80, y + 72};
      const Color c = hitFxColor(w->fx), g = hitFxGlow(w->fx);
      ui::glow(b, 26, withAlpha(g, 0.35f), 4);
      DrawCircleV(b, 16, ui::kInk);
      DrawCircleV(b, 13.5f, mixColor(c, ui::kInk, 0.55f));
      DrawRing(b, 13.5f, 16.5f, 0, 360, 32, g);
      drawWeaponIcon(*f.def, f.weapon, W(b), 26.0f);
      if (f.moveDef(NSpecial).cooldown > 0 && f.moveCooldown[NSpecial] > 0) {  // its special recharging
        DrawCircleV(b, 13.5f, withAlpha(ui::kInk, 0.65f));
        char sec[16];
        std::snprintf(sec, sizeof sec, "%d", (f.moveCooldown[NSpecial] + 59) / 60);
        drawTextOutlined(sec, b.x - textWidth(sec, 16) * 0.5f, b.y - 9, 16, ui::kCream, ui::kInk, 2.0f);
      }
    }

    char label[16];
    std::snprintf(label, sizeof label, f.cpu ? "CPU" : "J%d", f.port + 1);
    float lw = textWidth(label, 15) + 16;
    float ns = 22.0f;  // long names shrink to leave room for the player label
    while (ns > 13.0f && textWidth(f.def->name.c_str(), ns, FontKind::Title) > pw - 94 - lw - 20) ns -= 1.0f;
    drawTextOutlined(f.def->name.c_str(), x + 94, y + 9 + (22 - ns) * 0.5f, ns, WHITE, withAlpha(BLACK, 0.7f), 2.5f,
                     FontKind::Title);
    DrawRectangleRounded({x + pw - lw - 11.5f, y + 9.5f, lw + 3, 23}, 0.5f, 6, ui::kInk);
    DrawRectangleRounded({x + pw - lw - 10, y + 11, lw, 20}, 0.5f, 6, shade(pc, 0.85f));
    drawText(label, x + pw - lw - 2, y + 12, 15, WHITE);

    if (f.eliminated) {
      drawTextOutlined("K.O.", x + 104, y + 34, 42, {200, 204, 196, 255}, ui::kInk, 3.0f, FontKind::Title);
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
      if (!m.cfg.training || m.trainFight) {
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
    drawCharacterSprite(*f.def, f.palette, pose, {b.x, b.y + f.def->height * scale * 0.5f}, f.facing, scale, 1.0f, WHITE,
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
