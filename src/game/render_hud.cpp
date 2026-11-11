// In-match HUD (« Épure »): a see-through Smash-like card per fighter along the bottom (its face, stocks,
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
constexpr Color kGoldLight = ui::kBlue;  // the accent

// The Clash Orb's banner, top centre, while it is in play: the orb, when it leaves, its life in ten bars.
void orbBanner(const Match& m, float time) {
  const Orb& o = m.orb;
  if (!o.active) return;
  const float w = 226.0f, h = 46.0f, x = (kScreenW - w) * 0.5f, y = 14.0f;
  ui::roundRect({x, y, w, h}, 14, withAlpha(ui::kDeep, 0.82f), withAlpha(ui::kDeep, 0.82f));
  ui::roundLines({x, y, w, h}, 14, 1.0f, withAlpha(ui::kCream, 0.14f));
  drawClashOrb({x + 27, y + h * 0.5f}, 9.5f, time, 1);
  const float tx = x + 54, cy = y + 17;
  const float lw = ui::tracked("CLASH ORB", tx, capsY(cy, 15, kT), 15, 0.16f, ui::kCream);
  const int left = std::max(1, (o.life + 59) / 60);
  drawText(TextFormat("part dans %d s", left), tx + lw + 10, capsY(cy, 14, kT), 14, kGoldLight, kT);
  const int lit = int(std::ceil(std::max(0.0f, o.hp) / 4.0f));  // 40 PV: 4 a bar
  for (int i = 0; i < 10; ++i)
    ui::roundRect({tx + i * 15.2f, y + 30, 13.2f, 5}, 2, i < lit ? (o.flash > 0 ? WHITE : ui::kCream) : ui::kTrack, i < lit ? (o.flash > 0 ? WHITE : ui::kCream) : ui::kTrack);
}

// "ULTIME PRÊT": the gold badge, the logo's spark in ink, glowing on and off; returns its left edge.
float ultBadge(float right, float bottom, float time) {
  const float size = 14.0f, tw = ui::trackedWidth("ULTIME PRÊT", size, 0.08f);
  const Rectangle b{right - (8 + 11 + 5 + tw + 8), bottom - 24, 8 + 11 + 5 + tw + 8, 24};
  const float k = 0.5f - 0.5f * std::cos(time * 2.0f * kPi / 1.4f);
  for (int i = 3; i >= 1; --i) ui::roundRect(ui::grow(b, 2.0f + i * 3.0f * k), 12 + 2.0f + i * 3.0f * k, withAlpha(ui::kBlue, 0.14f * k), withAlpha(ui::kBlue, 0.14f * k));
  ui::roundRect(b, 12, ui::kCream, ui::kCream);
  ui::spark({b.x + 13.5f, b.y + 12}, 6.5f, 0.0f, {40, 150, 190, 255});
  ui::tracked("ULTIME PRÊT", b.x + 24, capsY(b.y + 12, size, kT), size, 0.08f, ui::kInk);
  return b.x;
}

// « Épure », Smash-like: no plate, the arena shows through a dark fade. The fighter's face in a disc ringed with the
// player's colour; beside it the player's tag and the stocks as dots, the damage big in Exo 2 Black Italic, the name;
// the weapons taken on the right.
void fighterCard(const Match& m, const Fighter& f, Rectangle r, float shake, float time, const std::array<int, 4>* devices) {
  const float x = r.x, bottom = r.y + r.height;
  const Color pc = f.cpu ? Color{163, 167, 175, 255} : slotColor(f), ink{10, 11, 14, 255}, face = auraColor(f.def->hero);
  const Color ring = f.inTeam ? slotColor(f) : pc;
  const bool out = f.eliminated;
  constexpr FontKind kB = FontKind::Body, kDsp = FontKind::Display, kD = FontKind::Damage;

  // The face: a disc ringed with the player's colour
  const Vec2 ic{x + 38, bottom - 20 - 46};
  constexpr float ir = 34;
  DrawCircleV({ic.x, ic.y + 3}, ir + 4, withAlpha(BLACK, 0.3f));
  DrawCircleV(V(ic), ir + 2, out ? ui::kDisabled : ring);
  DrawCircleV(V(ic), ir, {42, 39, 88, 255});
  DrawCircleGradient(int(ic.x), int(ic.y), ir, out ? Color{40, 46, 66, 255} : mixColor(face, {42, 39, 88, 255}, 0.55f), {42, 39, 88, 255});
  drawArtDisc(*f.def, f.palette, ic, ir, time + f.id, 1.1f, out ? Color{120, 124, 140, 255} : WHITE);

  // Who: the player's tag, the stocks as dots (a lost one hollow)
  const float tx = x + 86, rowY = bottom - 20 - 92 + 9;
  char tag[8];
  std::snprintf(tag, sizeof tag, f.cpu ? "CPU" : "J%d", f.port + 1);
  const float tw = ui::labelWidth(tag, 10, 0.06f) + 12;
  ui::roundRect({tx, rowY - 9, tw, 18}, 5, out ? ui::kDisabled : ring, out ? ui::kDisabled : ring);
  ui::label(tag, tx + 6, rowY, 10, 0.06f, ink);
  if (!out && (!m.cfg.training || m.trainFight)) {
    const int total = std::max(m.cfg.stocks, f.stocks), shown = std::min(total, 5);
    for (int i = 0; i < shown; ++i) {
      const Vector2 c{tx + tw + 12 + i * 14.0f, rowY};
      if (i < f.stocks) DrawCircleV(c, 4.5f, ring);
      else DrawRing(c, 3.0f, 4.5f, 0, 360, 20, withAlpha(ring, 0.6f));
    }
    if (total > 5) drawText(TextFormat("×%d", f.stocks), tx + tw + 12 + 5 * 14.0f, capsY(rowY, 12, kB), 12, ui::kCream, kB);
  }

  // The damage, big and leaning; "K.O." once out
  constexpr float kNum = 52.0f, kPct = 24.0f;
  const float numCy = bottom - 20 - 92 + 18 + 4 + 26;
  if (out) {
    drawTextOutlined("K.O.", tx, capsY(numCy, 40, kD), 40, ui::kDisabled, withAlpha(ui::kDeep, 0.6f), 2.0f, kD);
  } else {
    char pct[16];
    std::snprintf(pct, sizeof pct, "%d", int(f.percent));
    const float jx = shake > 0 ? std::sin(time * 90.0f) * shake : 0.0f, jy = shake > 0 ? std::cos(time * 77.0f) * shake : 0.0f;
    const float grow = 1.0f + std::min(8.0f, shake) / 60.0f;
    const Color col = percentColor(f.percent), drop = withAlpha(BLACK, 0.55f);
    const float ns = kNum * grow, base = capsY(numCy, kNum, kD) + 0.8325f * kNum * fontScale(kD);
    const float ny = base - 0.8325f * ns * fontScale(kD) + jy, py = base - 0.8325f * kPct * fontScale(kD) + jy;
    const float nw = textWidth(pct, ns, kD);
    drawText(pct, tx + jx, ny + 2, ns, drop, kD);
    drawText("%", tx + nw + 2 + jx, py + 2, kPct, drop, kD);
    drawTextOutlined(pct, tx + jx, ny, ns, col, withAlpha(ui::kDeep, 0.5f), 1.5f, kD);
    drawTextOutlined("%", tx + nw + 2 + jx, py, kPct, col, withAlpha(ui::kDeep, 0.5f), 1.0f, kD);
  }

  // Its name under the damage
  std::string nick = upperFr(fiche(*f.def).nick);
  float nsz = 12.0f;
  while (nsz > 8.0f && textWidth(nick.c_str(), nsz, kDsp) > r.width - 96) nsz -= 0.5f;
  drawText(nick.c_str(), tx, capsY(bottom - 20 - 6, nsz, kDsp), nsz, out ? ui::kDisabled : ui::kCream, kDsp);

  // The weapons taken (the one in hand first, lit), right of the damage; its special's cooldown over it
  const float right = x + r.width - 8;
  float wx = right;
  if (!out) {
    for (int k = 0; k < 2; ++k) {
      const int slot = k == 0 ? f.weapon : f.weapon ^ 1;
      if (slot < 0 || !((f.weapons >> slot) & 1)) continue;
      const Rectangle b{wx - 28, numCy - 14, 28, 28};
      ui::roundRect(b, 8, withAlpha(ui::kDeep, k == 0 ? 0.7f : 0.45f), withAlpha(ui::kDeep, k == 0 ? 0.7f : 0.45f));
      if (k == 0) ui::roundLines(b, 8, 1.5f, ring);
      drawWeaponIcon(*f.def, slot, {b.x + 14, b.y + 14}, 22.0f, k == 0 ? 1.0f : 0.45f);
      if (k == 0 && f.moveDef(NSpecial).cooldown > 0 && f.moveCooldown[NSpecial] > 0) {  // its special recharging
        ui::roundRect(b, 8, withAlpha(ui::kDeep, 0.65f), withAlpha(ui::kDeep, 0.65f));
        const char* sec = TextFormat("%d", (f.moveCooldown[NSpecial] + 59) / 60);
        drawText(sec, b.x + (b.width - textWidth(sec, 14, kB)) * 0.5f, capsY(b.y + 14, 14, kB), 14, ui::kCream, kB);
      }
      wx -= 32;
    }
  }

  // The Clash Orb's prize, over the card: the badge, and the button that unleashes it
  if (f.finalSmash && !out) {
    float bl = ultBadge(right, r.y + 4, time) - 6;
    if (devices && !f.cpu) {
      const int dev = (*devices)[f.port & 3];
      const float pw = ui::actionPromptWidth(dev, kActUltimate, 20);
      ui::actionPrompt(dev, kActUltimate, bl - pw, r.y - 18, 20, true);
    }
  }
}
}  // namespace

void drawHud(const Match& m, const std::array<float, 4>& shake, float time, const std::array<int, 4>* devices) {
  DrawRectangleGradientV(0, kScreenH - 170, kScreenW, 170, withAlpha(ui::kDeep, 0.0f), withAlpha(ui::kDeep, 0.78f));
  orbBanner(m, time);
  const int n = int(m.fighters.size());
  const float cw = 280.0f, ch = 110.0f, gap = n <= 2 ? 80.0f : 18.0f;
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
