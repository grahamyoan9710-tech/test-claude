// In-match HUD (« Encre d'horizon »): a see-through Smash-like card per fighter along the bottom (its face, stocks,
// name, damage, weapons, "ULTIME PRÊT"), the Clash Orb's banner at the top while it is in play, off-screen bubbles
// and the countdown.
#include <cstdio>

#include "game/app.hpp"
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

// « Encre d'horizon », Smash-like: no plate, the arena shows through. A band of the player's colour leaning like a
// slash and fading out, the fighter's face in a disc ringed with that colour, its stocks as small faces, its name,
// the damage big in Exo 2 Black Italic, the weapons taken.
void fighterCard(const Match& m, const Fighter& f, Rectangle r, float shake, float time, const std::array<int, 4>* devices) {
  const float x = r.x, y = r.y, w = r.width;
  const Color pc = slotColor(f), ink = ui::kInk, face = auraColor(f.def->hero);
  const bool out = f.eliminated;
  const float a = out ? 0.35f : 1.0f;

  // The band (skewed by 16°), a shade over it for the text, its bright bottom edge
  constexpr float bw = 236, bh = 58, sk = 8.3f;  // sk: half of the 16° lean over its height
  const float bx = x + 30, by = y + r.height - 6 - bh;
  auto band = [&](float u0, float u1, Color c0, Color c1, float top = 0.0f) {
    const float x0 = bx + bw * u0, x1 = bx + bw * u1, t = sk * (1.0f - 2.0f * top / bh);
    ui::quad({x0 + t, by + top}, {x1 + t, by + top}, {x1 - sk, by + bh}, {x0 - sk, by + bh}, c0, c1, c1, c0);
  };
  band(0.0f, 0.55f, withAlpha(pc, 0.8f * a), withAlpha(pc, 0.4f * a));
  band(0.55f, 1.0f, withAlpha(pc, 0.4f * a), withAlpha(pc, 0.0f));
  const float sx0 = bx + 4, sx1 = bx + 204;
  ui::quad({sx0 + sk, by}, {sx1 + sk, by}, {sx1 - sk, by + bh}, {sx0 - sk, by + bh}, withAlpha(ink, 0.0f),
           withAlpha(ink, 0.0f), withAlpha(ink, 0.35f), withAlpha(ink, 0.35f));
  band(0.0f, 1.0f, withAlpha(pc, a), withAlpha(pc, 0.25f * a), bh - 3.0f);

  // The icon: its face in a disc ringed with the player's colour
  const Vec2 ic{x + 47, y + r.height - 43};
  constexpr float ir = 41;
  DrawCircleV({ic.x, ic.y + 4}, ir + 6, withAlpha(BLACK, 0.3f));
  DrawCircleV(V(ic), ir + 5, ink);
  DrawCircleV(V(ic), ir + 3, out ? ui::kDisabled : pc);
  DrawCircleGradient(int(ic.x), int(ic.y), ir, out ? Color{40, 46, 66, 255} : mixColor(face, WHITE, 0.1f), {11, 18, 48, 255});
  drawArtDisc(*f.def, f.palette, ic, ir, time + f.id, 0.58f, out ? Color{120, 124, 140, 255} : WHITE);

  // Who: "J1" (or CPU) on the icon's corner
  char tag[8];
  std::snprintf(tag, sizeof tag, f.cpu ? "CPU" : "J%d", f.port + 1);
  const float tw = textWidth(tag, 14, kT) + 12;
  DrawRectangleRec({x + 6, ic.y - ir - 12, tw, 19}, out ? ui::kDisabled : pc);
  drawText(tag, x + 12, capsY(ic.y - ir - 2.5f, 14, kT), 14, ink, kT);

  // Its name under the damage
  const std::string nick = fiche(*f.def).nick;
  drawTextOutlined(nick.c_str(), x + 100, capsY(y + r.height - 17, 16, kT), 16, out ? ui::kDisabled : ui::kCream, ink, 2.0f, kT);

  // Its stocks: its face, small; a lost one dim (no art: diamonds of its colour, hollow once lost)
  if (!out && (!m.cfg.training || m.trainFight)) {
    const int total = std::max(m.cfg.stocks, f.stocks), shown = std::min(total, 5);
    const bool art = hasArt(*f.def);
    for (int i = 0; i < shown; ++i) {
      const Vec2 c{x + 110.0f + i * 23.0f, y + 19.0f};
      const bool lost = i >= f.stocks;
      if (art) {
        DrawCircleV(V(c), 12, withAlpha(ink, lost ? 0.5f : 1.0f));
        DrawCircleV(V(c), 10, withAlpha(face, lost ? 0.2f : 1.0f));
        drawArtDisc(*f.def, f.palette, c, 10, time, 0.5f, lost ? Color{160, 166, 186, 80} : WHITE);
      } else {
        ui::diamond(V(c), 8.0f, ink);
        ui::diamond(V(c), 6.5f, pc);
        if (lost) ui::diamond(V(c), 4.5f, withAlpha(ink, 0.95f));
      }
    }
    if (total > 5) drawTextOutlined(TextFormat("×%d", f.stocks), x + 110 + 5 * 23 - 4, capsY(y + 19, 14, kT), 14, ui::kCream, ink, 2.0f, kT);
  }

  // The damage, right, big and leaning (its caps centred 50 px down); "K.O." once out
  constexpr FontKind kD = FontKind::Damage;
  constexpr float kNum = 60.0f, kPct = 29.0f;  // drawn at 66 and 32 px
  const float right = x + w - 16, numTop = capsY(y + 50, kNum, kD), base = numTop + 0.8325f * kNum * fontScale(kD);
  if (out) {
    const float kw = textWidth("K.O.", 44, kD);
    drawTextOutlined("K.O.", right - kw, capsY(y + 50, 44, kD), 44, ui::kDisabled, ink, 3.0f, kD);
  } else {
    char pct[16];
    std::snprintf(pct, sizeof pct, "%d", int(f.percent));
    const float jx = shake > 0 ? std::sin(time * 90.0f) * shake : 0.0f, jy = shake > 0 ? std::cos(time * 77.0f) * shake : 0.0f;
    const float grow = 1.0f + std::min(8.0f, shake) / 60.0f;
    const Color col = percentColor(f.percent), drop = withAlpha(ink, 0.55f);
    const float nw = textWidth(pct, kNum * grow, kD), sw = textWidth("%", kPct, kD);
    const float nx = right - sw - 2 - nw + jx, ny = base - 0.8325f * kNum * grow * fontScale(kD) + jy;
    const float px = right - sw + jx, py = base - 0.8325f * kPct * fontScale(kD) + jy;
    drawTextOutlined(pct, nx, ny + 3, kNum * grow, drop, drop, 4.0f, kD);  // its drop shadow
    drawTextOutlined("%", px, py + 3, kPct, drop, drop, 3.0f, kD);
    drawTextOutlined(pct, nx, ny, kNum * grow, col, ink, 4.0f, kD);
    drawTextOutlined("%", px, py, kPct, col, ink, 3.0f, kD);
  }

  // The weapons taken (the one in hand first, lit), bottom right; its special's cooldown over it
  float wx = right;
  if (!out) {
    for (int k = 0; k < 2; ++k) {
      const int slot = k == 0 ? f.weapon : f.weapon ^ 1;
      if (slot < 0 || !((f.weapons >> slot) & 1)) continue;
      const Rectangle b{wx - 26, y + r.height - 32, 26, 26};
      DrawRectangleRec(b, withAlpha(ink, k == 0 ? 0.55f : 0.35f));
      if (k == 0) DrawRectangleRec({b.x, b.y + b.height - 2, b.width, 2}, pc);
      drawWeaponIcon(*f.def, slot, {b.x + 13, b.y + 13}, 24.0f, k == 0 ? 1.0f : 0.45f);
      if (k == 0 && f.moveDef(NSpecial).cooldown > 0 && f.moveCooldown[NSpecial] > 0) {  // its special recharging
        DrawRectangleRec(b, withAlpha(ink, 0.6f));
        const char* sec = TextFormat("%d", (f.moveCooldown[NSpecial] + 59) / 60);
        drawText(sec, b.x + (b.width - textWidth(sec, 16, kT)) * 0.5f, capsY(b.y + 13, 16, kT), 16, ui::kCream, kT);
      }
      wx -= 30;
    }
  }

  // The Clash Orb's prize, over the card: the badge, and the button that unleashes it
  if (f.finalSmash && !out) {
    float bl = ultBadge(right, y - 2, time) - 6;
    if (devices && !f.cpu) {
      const int dev = (*devices)[f.port & 3];
      const float pw = ui::actionPromptWidth(dev, kActUltimate, 20);
      ui::actionPrompt(dev, kActUltimate, bl - pw, y - 24, 20, true);
    }
  }
}
}  // namespace

void drawHud(const Match& m, const std::array<float, 4>& shake, float time, const std::array<int, 4>* devices) {
  orbBanner(m, time);
  const int n = int(m.fighters.size());
  const float cw = 300.0f, ch = 110.0f, gap = n <= 2 ? 120.0f : 14.0f;
  float x = (kScreenW - (n * cw + (n - 1) * gap)) * 0.5f;
  for (const auto& f : m.fighters) {
    fighterCard(m, f, {x, kScreenH - 8.0f - ch, cw, ch}, shake[f.id & 3], time, devices);
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
