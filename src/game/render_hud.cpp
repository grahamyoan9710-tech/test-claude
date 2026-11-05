// In-match HUD (« Ligne d'horizon »): a night card per fighter along the bottom (portrait, stocks, weapon, damage,
// "ULTIME PRÊT"), the Clash Orb's banner at the top while it is in play, off-screen bubbles and the countdown.
#include <cstdio>

#include "game/fx.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
constexpr FontKind kT = FontKind::Title;
constexpr Color kGoldLight{255, 200, 112, 255};

// The Clash Orb's banner, top centre, while it is in play: the orb, when it leaves, its life in ten bars.
void orbBanner(const Match& m, float time) {
  const Orb& o = m.orb;
  if (!o.active) return;
  const float w = 226.0f, h = 46.0f, x = (kScreenW - w) * 0.5f, y = 14.0f;
  const Color bg = withAlpha(ui::kInk, 0.82f);
  ui::quad({x + 12, y}, {x + w - 12, y}, {x + w, y + h}, {x, y + h}, bg, bg, bg, bg);
  drawClashOrb({x + 27, y + h * 0.5f}, 9.5f, time, 1);
  const float tx = x + 54, cy = y + 17;
  const float lw = ui::tracked("CLASH ORB", tx, capsY(cy, 15, kT), 15, 0.16f, ui::kCream);
  const int left = std::max(1, (o.life + 59) / 60);
  drawText(TextFormat("part dans %d s", left), tx + lw + 10, capsY(cy, 14, kT), 14, kGoldLight, kT);
  const int lit = int(std::ceil(std::max(0.0f, o.hp) / 4.0f));  // 40 PV: 4 a bar
  for (int i = 0; i < 10; ++i)
    DrawRectangleRec({tx + i * 15.2f, y + 30, 13.2f, 5}, i < lit ? (o.flash > 0 ? WHITE : ui::kCream) : Color{42, 53, 88, 255});
}

// "ULTIME PRÊT": the gold badge, the logo's spark in ink, glowing on and off; returns its left edge.
float ultBadge(float right, float bottom, float time) {
  const float size = 14.0f, tw = ui::trackedWidth("ULTIME PRÊT", size, 0.08f);
  const Rectangle b{right - (8 + 11 + 5 + tw + 8), bottom - 24, 8 + 11 + 5 + tw + 8, 24};
  const float k = 0.5f - 0.5f * std::cos(time * 2.0f * kPi / 1.4f);
  for (int i = 3; i >= 1; --i) DrawRectangleRec(ui::grow(b, 2.0f + i * 3.0f * k), withAlpha(ui::kGold, 0.16f * k));
  DrawRectangleRec(b, ui::kGold);
  ui::spark({b.x + 13.5f, b.y + 12}, 5.5f, 0.0f, ui::kInk);
  ui::tracked("ULTIME PRÊT", b.x + 24, capsY(b.y + 12, size, kT), size, 0.08f, ui::kInk);
  return b.x;
}

void fighterCard(const Match& m, const Fighter& f, Rectangle r, float shake, float time, const std::array<int, 4>* devices) {
  const float x = r.x, y = r.y, w = r.width;
  const Color pc = slotColor(f);
  const bool out = f.eliminated;
  ui::bevel(r, 16, withAlpha(ui::kInk, 0.86f), withAlpha(ui::kInk, 0.86f));
  ui::quad({x + 16, y}, {x + w, y}, {x + w, y + 3}, {x + 13, y + 3}, pc, pc, pc, pc);  // its colour along the top

  // The portrait on its fighter's colour, its top-left corner cut
  const Rectangle pr{x + 10, y + 12, 76, 76};
  const Color tint = mixColor(auraColor(f.def->hero), ui::kInk, 0.45f), deep{11, 18, 48, 255};
  ui::quad({pr.x, pr.y}, {pr.x + pr.width, pr.y}, {pr.x + pr.width, pr.y + pr.height}, {pr.x, pr.y + pr.height}, tint,
           mixColor(tint, deep, 0.35f), deep, mixColor(tint, deep, 0.75f));
  drawPortraitIn(*f.def, f.palette, {pr.x + 38, pr.y + 46}, 98.0f, pr, 0.0f, time + f.id);
  if (out) DrawRectangleRec(pr, withAlpha(ui::kInk, 0.6f));
  DrawTriangle({pr.x, pr.y}, {pr.x, pr.y + 12}, {pr.x + 12, pr.y}, withAlpha(ui::kInk, 0.92f));

  // Who: "J1" (or CPU) in its colour, its number
  char tag[8];
  std::snprintf(tag, sizeof tag, f.cpu ? "CPU" : "J%d", f.port + 1);
  const float tw = textWidth(tag, 16, kT);
  drawText(tag, x + 96, capsY(y + 22, 16, kT), 16, out ? ui::kDisabled : pc, kT);
  drawText(TextFormat("N° %s", f.def->id.substr(1).c_str()), x + 96 + tw + 6, capsY(y + 22, 14, kT), 14, ui::kSky, kT);

  // The damage, right; "K.O." once out
  if (out) {
    const float kw = textWidth("K.O.", 44, kT);
    drawTextOutlined("K.O.", x + w - 16 - kw, capsY(y + 32, 44, kT), 44, ui::kDisabled, ui::kInk, 2.0f, kT);
  } else {
    char pct[16];
    std::snprintf(pct, sizeof pct, "%d", int(f.percent));
    const float jx = shake > 0 ? std::sin(time * 90.0f) * shake : 0.0f, jy = shake > 0 ? std::cos(time * 77.0f) * shake : 0.0f;
    const Color col = percentColor(f.percent);
    const float size = 61.0f + std::min(8.0f, shake), sw = textWidth("%", 28, kT), nw = textWidth(pct, size, kT);
    const float right = x + w - 14 + jx;
    drawTextOutlined("%", right - sw, capsY(y + 41 + jy, 28, kT), 28, col, ui::kInk, 2.0f, kT);
    drawTextOutlined(pct, right - sw - nw, capsY(y + 30 + jy, size, kT), size, col, ui::kInk, 2.0f, kT);
  }

  // Its stocks: diamonds of its colour, hollow once lost
  if (!out && (!m.cfg.training || m.trainFight)) {
    const int total = std::max(m.cfg.stocks, f.stocks), shown = std::min(total, 5);
    for (int i = 0; i < shown; ++i) {
      const Vector2 c{x + 101.0f + i * 16.0f, y + 37.0f};
      ui::diamond(c, 7.0f, pc);
      if (i >= f.stocks) ui::diamond(c, 5.0f, withAlpha(ui::kInk, 0.95f));
    }
    if (total > 5) drawText(TextFormat("×%d", f.stocks), x + 101 + 5 * 16, capsY(y + 37, 14, kT), 14, pc, kT);
  }

  // The weapons taken (the one in hand first, lit), and the name of what it fights with
  float wx = x + 96;
  const float wy = y + 52;
  if (!out) {
    for (int k = 0; k < 2; ++k) {
      const int slot = k == 0 ? f.weapon : f.weapon ^ 1;
      if (slot < 0 || !((f.weapons >> slot) & 1)) continue;
      const Rectangle b{wx, wy, 30, 34};
      DrawRectangleRec(b, withAlpha(ui::kCream, k == 0 ? 0.08f : 0.04f));
      if (k == 0) DrawRectangleRec({b.x, b.y + b.height - 2, b.width, 2}, pc);
      drawWeaponIcon(*f.def, slot, {b.x + 15, b.y + 17}, 28.0f, k == 0 ? 1.0f : 0.45f);
      if (k == 0 && f.moveDef(NSpecial).cooldown > 0 && f.moveCooldown[NSpecial] > 0) {  // its special recharging
        DrawRectangleRec(b, withAlpha(ui::kInk, 0.6f));
        const char* sec = TextFormat("%d", (f.moveCooldown[NSpecial] + 59) / 60);
        drawText(sec, b.x + (b.width - textWidth(sec, 18, kT)) * 0.5f, capsY(b.y + 17, 18, kT), 18, ui::kCream, kT);
      }
      wx += 34;
    }
  }
  float right = x + w - 12;
  if (f.finalSmash && !out) {  // the Clash Orb's prize: the badge, and the button that unleashes it
    right = ultBadge(right, y + 90, time) - 6;
    if (devices && !f.cpu) {
      const int dev = (*devices)[f.port & 3];
      const float pw = ui::actionPromptWidth(dev, kActUltimate, 20);
      ui::actionPrompt(dev, kActUltimate, right - pw, y + 68, 20, true);
      right -= pw + 6;
    }
  } else if (!out) {
    const WeaponDef* wd = f.weaponDef();
    drawText(wd ? wd->name.c_str() : "Mains nues", wx + (wx > x + 96 ? 2 : 0), capsY(wy + 17, 15, FontKind::Ui), 15,
             ui::kSky);
  }
}
}  // namespace

void drawHud(const Match& m, const std::array<float, 4>& shake, float time, const std::array<int, 4>* devices) {
  orbBanner(m, time);
  const int n = int(m.fighters.size());
  const float cw = 286.0f, ch = 100.0f, gap = 12.0f;
  float x = (kScreenW - (n * cw + (n - 1) * gap)) * 0.5f;
  for (const auto& f : m.fighters) {
    fighterCard(m, f, {x, kScreenH - 16.0f - ch, cw, ch}, shake[f.id & 3], time, devices);
    x += cw + gap;
  }
}

void drawOffscreenBubbles(const Match& m, const Camera2D& cam, float time) {
  // A bubble on the edge, pointing at the fighter out of sight: the fighter in it, who it is and how far.
  const float margin = 56.0f, bottom = 540.0f;  // clear of the cards
  for (const auto& f : m.fighters) {
    if (!f.active()) continue;
    Vector2 sp = GetWorldToScreen2D(V(f.center()), cam);
    if (sp.x > -10 && sp.x < kScreenW + 10 && sp.y > -10 && sp.y < kScreenH + 10) continue;
    const Vector2 b{clampf(sp.x, margin, kScreenW - margin), clampf(sp.y, margin, bottom)};
    const Color pc = slotColor(f);
    const Vec2 dir = (W(sp) - W(b)).norm(), perp{-dir.y, dir.x};
    DrawTriangle(V(W(b) + dir * 35.0f + perp * 9.0f), V(W(b) + dir * 49.0f), V(W(b) + dir * 35.0f - perp * 9.0f), pc);
    DrawCircleV(b, 37, pc);
    DrawCircleV(b, 33, withAlpha(ui::kCream, 0.92f));
    const float scale = 52.0f / f.def->height;
    drawCharacterSprite(*f.def, f.palette, computePose(f, m, time), {b.x, b.y + f.def->height * scale * 0.5f}, f.facing,
                        scale, 1.0f, WHITE, f.flash > 0 ? 0.5f : 0.0f, time);
    char who[24];
    const int metres = int((W(sp) - W(b)).len() / 30.0f);
    if (f.cpu) std::snprintf(who, sizeof who, metres > 0 ? "CPU · %d m" : "CPU", metres);
    else std::snprintf(who, sizeof who, metres > 0 ? "J%d · %d m" : "J%d", f.port + 1, metres);
    const float ww = textWidth(who, 14, kT);
    drawTextOutlined(who, b.x - ww * 0.5f, capsY(b.y + 49, 14, kT), 14, WHITE, ui::kInk, 2.0f, kT);
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
