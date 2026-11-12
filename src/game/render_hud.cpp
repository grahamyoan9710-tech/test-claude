// In-match HUD (« Prestige »): transparent cards along the bottom as in Smash (a square of the player's colour leaning
// like the blade with the fighter's bust over it, the stocks, the damage warming up, the name on a night strip, the
// weapons taken, "ULTIME PRÊT"), the Clash Orb's banner at the top while it is in play, off-screen bubbles, and the
// countdown: « PRÊTS ? », then 3, 2, 1 slammed on slabs cut like the blade, then GO torn red / blue.
#include <cstdio>

#include "game/app.hpp"
#include "game/fighter_art.hpp"
#include "game/fx.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
constexpr FontKind kT = FontKind::Title, kB = FontKind::Body;

using ui::exo;
using ui::exoWidth;
// Exo 2 outlined 3 px all round in night (the mock-up's eight text-shadows).
void exoOut(const char* s, float x, float base, float em, Color c, float a) {
  for (int i = 0; i < 8; ++i)
    exo(s, x + 3.0f * std::cos(i * kPi / 4), base + 3.0f * std::sin(i * kPi / 4), em, withAlpha(ui::kNight, a));
  exo(s, x, base, em, withAlpha(c, a));
}
// Small condensed capitals (Archivo 62 %), the top of their line box (line-height .84) at `top`.
float condWidth(const char* s, float em, float ls) { return MeasureTextEx(gameFont(kT), s, 1.088f * em, ls * em).x; }
void cond(const char* s, float x, float top, float em, float ls, Color c) {
  DrawTextEx(gameFont(kT), s, {x, top + (0.84f - 1.088f) * 0.5f * em}, 1.088f * em, ls * em, c);
}

// The damage warms up as in Smash: white, then yellow, orange, red and a deep red.
Color heat(float p) {
  return p < 30 ? WHITE : p < 60 ? Color{255, 233, 168, 255} : p < 100 ? Color{255, 179, 107, 255}
         : p < 150 ? Color{255, 107, 74, 255} : Color{229, 35, 58, 255};
}

// The Clash Orb's banner, top centre, while it is in play: the orb, when it leaves, its life in ten bars.
void orbBanner(const Match& m, float time) {
  const Orb& o = m.orb;
  if (!o.active) return;
  const float w = 236.0f, h = 46.0f, x = (kScreenW - w) * 0.5f, y = 14.0f;
  DrawRectangleRec({x, y, w, h}, withAlpha(ui::kNight, 0.82f));
  drawClashOrb({x + 27, y + h * 0.5f}, 9.5f, time, 1);
  const float tx = x + 54;
  const float lw = ui::caps("CLASH ORB", tx, y + 17, 11, 0.3f, ui::kSilver);
  const int left = std::max(1, (o.life + 59) / 60);
  drawText(TextFormat("part dans %d s", left), tx + lw + 10, capsY(y + 17, 13, kB), 13, ui::kMist, kB);
  const int lit = int(std::ceil(std::max(0.0f, o.hp) / 4.0f));  // 40 PV: 4 a bar
  for (int i = 0; i < 10; ++i)
    DrawRectangleRec({tx + i * 15.2f, y + 30, 13.2f, 5}, i < lit ? (o.flash > 0 ? WHITE : ui::kBlue) : withAlpha(ui::kSilver, 0.16f));
}

// "ULTIME PRÊT": Éclat with night capitals, a rim breathing out of it; returns its left edge.
float ultBadge(float right, float bottom, float time) {
  const float w = ui::capsWidth("ULTIME PRÊT", 11, 0.14f) + 20.0f;
  const Rectangle b{right - w, bottom - 22, w, 22};
  const float k = 0.5f - 0.5f * std::cos(time * 2.0f * kPi / 1.4f);
  DrawRectangleLinesEx(ui::grow(b, 2.0f + 4.0f * k), 2.0f, withAlpha(ui::kBlue, 0.6f * (1.0f - k)));
  DrawRectangleRec(b, ui::kBlue);
  ui::caps("ULTIME PRÊT", b.x + 10, b.y + 11, 11, 0.14f, ui::kNight);
  return b.x;
}

// One card, 260 x 112 from (L, T), `a` its entrance. The square leans 12° about its middle (46, 60): its lower half
// deeper, a light rim inside the upper one, night around. The bust is the pixel portrait, cut at the square's foot.
void fighterCard(const Match& m, const Fighter& f, float L, float T, float a, int hitAge, float time,
                 const std::array<int, 4>* devices) {
  const bool out = f.eliminated;
  const Color pc = f.cpu && !f.inTeam ? Color{163, 167, 175, 255} : slotColor(f);
  const Color sq = out ? ui::kDim : pc, line = out ? ui::kDim : f.cpu && !f.inTeam ? ui::kCpuTag : pc;

  auto P = [&](float x, float y) { return Vector2{L + x + ui::kLean * (60.0f - y), T + y}; };
  auto quadP = [&](float x0, float y0, float x1, float y1, Color c, float k = 1.0f) {
    c = withAlpha(c, k * a);
    ui::quad(P(x0, y0), P(x1, y0), P(x1, y1), P(x0, y1), c, c, c, c);
  };
  quadP(8, 22, 84, 98, ui::kNight);
  quadP(10, 24, 82, 96, sq);
  quadP(10, 24, 82, 25, WHITE, 0.35f);
  quadP(10, 25, 11, 60, WHITE, 0.35f);
  quadP(81, 25, 82, 60, WHITE, 0.35f);
  quadP(10, 60, 82, 96, mixColor(sq, ui::kNight, 0.42f));
  {
    const std::string pic = f.def->id + "_portrait";
    float pw, ph;
    if (art::pictureSize(pic.c_str(), pw, ph)) {
      const bool roomy = f.def->id == "h01";  // his portrait leaves room around him: drawn larger
      const float s = (roomy ? 140.0f : 101.0f) / 240.0f;
      beginClip({L - 4, T - 26, 100, 122});
      art::drawPicture(pic.c_str(), {0, 0, pw, ph}, {L + (roomy ? -7.0f : -11.0f), T + (roomy ? -34.0f : -26.0f), pw * s, ph * s},
                       a, false, out ? Color{120, 124, 140, 255} : WHITE);
      endClip();
    }
  }

  // The stocks: silver dots ringed with night, a lost one hollow
  if (!out && (!m.cfg.training || m.trainFight)) {
    const int total = std::max(m.cfg.stocks, f.stocks), shown = std::min(total, 5);
    for (int i = 0; i < shown; ++i) {
      const Vector2 c{L + 102.5f + i * 14.0f, T + 12.5f};
      if (i < f.stocks) {
        DrawCircleV(c, 6.0f, withAlpha(ui::kNight, a));
        DrawCircleV(c, 4.5f, withAlpha(ui::kSilver, a));
      } else {
        DrawRing(c, 3.0f, 4.5f, 0, 360, 24, withAlpha(ui::kSilver, 0.45f * a));
      }
    }
    if (total > 5) cond(TextFormat("×%d", f.stocks), L + 98 + 5 * 14.0f, T + 6, 15, 0.06f, withAlpha(ui::kSilver, a));
  }

  // The damage (60 px, its line box at 22): hit, it jolts, its red and blue copies split off and two bands of it jump
  const float base = T + 75.94f, nx = L + 96.0f;
  if (out) {
    exoOut("K.O.", nx, base, 44, ui::kDim, a);
  } else {
    char pct[16];
    std::snprintf(pct, sizeof pct, "%d", int(f.percent));
    const Color col = heat(f.percent);
    const float s = hitAge / 60.0f, nw = exoWidth(pct, 60);
    Vector2 j{0, 0};
    if (s < 0.256f) {
      static const Vector2 kj[] = {{6, -3}, {-5, 2}, {4, 1}, {-2, -1}, {0, 0}};
      const int k = std::min(3, int(s / 0.064f));
      const float v = s / 0.064f - k;
      j = {kj[k].x + (kj[k + 1].x - kj[k].x) * v, kj[k].y + (kj[k + 1].y - kj[k].y) * v};
    }
    auto number = [&](float dx, Color c, bool outlined, float k = 1.0f) {
      const float x = nx + j.x + dx, b = base + j.y;
      if (outlined) {
        exoOut(pct, x, b, 60, c, a);
        exoOut("%", x + nw + 2, b, 27, c, a);
      } else {
        exo(pct, x, b, 60, withAlpha(c, k * a));
        exo("%", x + nw + 2, b, 27, withAlpha(c, k * a));
      }
    };
    if (s < 0.288f) {
      const float sp = s < 0.096f ? 9.0f : s < 0.192f ? 6.0f : 3.0f;
      number(-sp, ui::kSplitR, false, 0.85f);
      number(sp, ui::kSplitB, false, 0.85f);
    }
    number(0, col, true);
    if (s < 0.224f) {
      static const float k1[] = {14, -6, 8}, k2[] = {-10, 9, -4};
      const int st = s < 0.064f ? 0 : s < 0.128f ? 1 : 2;
      const float top = T + 22 + j.y;
      beginClip({L + 60, top + 10.8f, 200, 14.4f});
      number(k1[st], col, true);
      endClip();
      beginClip({L + 60, top + 36.0f, 200, 14.4f});
      number(k2[st], col, true);
      endClip();
    }
  }

  // The name on a night strip topped with the player's colour
  DrawRectangleRec({L + 58, T + 88, 190, 20}, withAlpha(ui::kNight, 0.78f * a));
  DrawRectangleRec({L + 58, T + 88, 190, 2}, withAlpha(line, a));
  const std::string nick = upperFr(fiche(*f.def).nick);
  float em = 15.0f;
  while (em > 10.0f && condWidth(nick.c_str(), em, 0.06f) > 144.0f) em -= 0.5f;
  cond(nick.c_str(), L + 98, T + 98 - 0.42f * em, em, 0.06f, withAlpha(out ? ui::kDim : ui::kSilver, a));

  // The weapons taken, top right (the one in hand first, rimmed); its special's cooldown over it
  const float right = L + 248;
  float wx = right;
  if (!out) {
    for (int k = 0; k < 2; ++k) {
      const int slot = k == 0 ? f.weapon : f.weapon ^ 1;
      if (slot < 0 || !((f.weapons >> slot) & 1)) continue;
      const Rectangle b{wx - 26, T + 1, 26, 26};
      DrawRectangleRec(b, withAlpha(ui::kNight, (k == 0 ? 0.78f : 0.5f) * a));
      if (k == 0) DrawRectangleLinesEx(b, 1.5f, withAlpha(line, a));
      drawWeaponIcon(*f.def, slot, {b.x + 13, b.y + 13}, 20.0f, (k == 0 ? 1.0f : 0.45f) * a);
      if (k == 0 && f.moveDef(NSpecial).cooldown > 0 && f.moveCooldown[NSpecial] > 0) {  // its special recharging
        DrawRectangleRec(b, withAlpha(ui::kNight, 0.65f * a));
        const char* sec = TextFormat("%d", (f.moveCooldown[NSpecial] + 59) / 60);
        drawText(sec, b.x + (b.width - textWidth(sec, 14, kB)) * 0.5f, capsY(b.y + 13, 14, kB), 14, withAlpha(ui::kSilver, a), kB);
      }
      wx -= 30;
    }
  }

  // The Clash Orb's prize, over the card: the badge, and the button that unleashes it
  if (f.finalSmash && !out) {
    const float bl = ultBadge(right, T - 4, time) - 6;
    if (devices && !f.cpu) {
      const int dev = (*devices)[f.port & 3];
      const float pw = ui::actionPromptWidth(dev, kActUltimate, 20);
      ui::actionPrompt(dev, kActUltimate, bl - pw, T - 25, 20, true);
    }
  }
}
}  // namespace

Vector2 goShake(const Match& m) { return m.countdown > 0 ? Vector2{0, 0} : ui::slam((m.frame - 200) / 60.0f); }

void drawHud(const Match& m, const std::array<int, 4>& hitAge, float time, const std::array<int, 4>* devices) {
  orbBanner(m, time);
  const int n = int(m.fighters.size());
  const float gap = n <= 2 ? 88.0f : n == 3 ? 24.0f : 12.0f;
  float x = (kScreenW - (n * 260.0f + (n - 1) * gap)) * 0.5f + 20.0f;
  int i = 0;
  for (const auto& f : m.fighters) {
    // They rise as GO lands, one after the other
    const float e = ui::seg((m.frame - 215.0f - 6.0f * i++) / 60.0f, 0.0f, 0.5f);
    if (e > 0.0f) fighterCard(m, f, x, 586.0f + 40.0f * (1.0f - e), e, hitAge[f.id & 3], time, devices);
    x += 260.0f + gap;
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
    DrawCircleV(b, 33, withAlpha(ui::kNight, 0.92f));
    const float scale = 52.0f / f.def->height;
    drawCharacterSprite(*f.def, f.palette, computePose(f, m, time), {b.x, b.y + f.def->height * scale * 0.5f}, f.facing,
                        scale, 1.0f, WHITE, f.flash > 0 ? 0.5f : 0.0f, time);
    char who[24];
    const int metres = int((W(sp) - W(b)).len() / 30.0f);
    if (f.cpu) std::snprintf(who, sizeof who, metres > 0 ? "CPU · %d m" : "CPU", metres);
    else std::snprintf(who, sizeof who, metres > 0 ? "J%d · %d m" : "J%d", f.port + 1, metres);
    const float ww = textWidth(who, 14, kT);
    drawTextOutlined(who, b.x - ww * 0.5f, capsY(b.y + 49, 14, kT), 14, WHITE, ui::kNight, 2.0f, kT);
  }
}

void drawCountdown(const Match& m, float shown) {
  const Vector2 sh = goShake(m);
  rlPushMatrix();
  rlTranslatef(sh.x, sh.y, 0.0f);
  // « PRÊTS ? » over its rule, from the moment the fight shows (still under the choc), gone as 3 lands
  if (m.countdown > 180) {
    const float in = ui::seg(shown, 0.0f, 0.216f);
    const float out = std::max(ui::seg(shown, 0.576f, 0.72f), clampf((188.0f - m.countdown) / 8.0f, 0.0f, 1.0f));
    const float a = in * (1.0f - out);
    if (a > 0.0f) {
      const float ls = 0.5f - 0.28f * in, w = ui::bigWidth("PRÊTS ?", 62, ui::Face::Wide, ls);
      ui::big("PRÊTS ?", 640.0f - w * 0.5f, 300, 62, ui::Face::Wide, withAlpha(WHITE, a), 1.088f, ls, 2.0f, withAlpha(ui::kNight, a));
      DrawRectangleRec({640.0f - 250.0f * in, 384, 500.0f * in, 3}, withAlpha(ui::kSilver, a));
    }
  }
  // 3, 2, 1: J1's red, J2's blue, then silver, each slab sliding in from the left and on to the right. The mock-up's
  // 0.45 s held for the game's second: the landing and the leaving keep their pace.
  if (m.countdown > 0 && m.countdown <= 180) {
    const int n = (m.countdown + 59) / 60;
    const float s = ((180 - m.countdown) % 60) / 60.0f;
    const float arrive = ui::seg(s, 0.0f, 0.081f), leave = ui::seg(s, 0.901f, 1.0f);
    const float dx = s < 0.081f ? -300.0f * (1.0f - arrive) : 140.0f * leave;
    ui::lean(546 + dx, 886 + dx, withAlpha(n == 3 ? ui::kSplitR : n == 2 ? ui::kSplitB : ui::kSilver, 1.0f - leave));
    const float sc = s < 0.081f ? 1.6f - 0.6f * arrive : s < 0.901f ? 1.0f - 0.02f * ui::seg(s, 0.081f, 0.901f) : 0.98f - 0.08f * leave;
    const float al = s < 0.901f ? arrive : 1.0f - leave, split = 34.0f - 26.0f * ui::seg(s, 0.0f, 0.351f);
    const char* num = n == 3 ? "3" : n == 2 ? "2" : "1";
    ui::scaled(sc, 640, 356, [&] { ui::choc(num, 640, 206, 300, split, al); });
  }
  // GO: the red and the blue halves meet on the blade and part, the word torn between them
  const float g = (m.frame - 200) / 60.0f;
  if (m.countdown <= 0 && g >= 0.0f && g < 0.9f) {
    const float u = g / 0.9f;
    const float a = 0.92f * (1.0f - ui::seg(u, 0.30f, 0.62f));
    const float off = u < 0.14f ? 720.0f * (1.0f - ui::seg(u, 0.0f, 0.14f)) : 120.0f * ui::seg(u, 0.30f, 0.62f);
    ui::lean(-400 - off, 716 - off, withAlpha(ui::kSplitR, a));
    ui::lean(716 + off, 1700 + off, withAlpha(ui::kSplitB, a));
    const float sc = u < 0.12f ? 1.9f - 0.93f * ui::seg(u, 0.0f, 0.12f) : u < 0.24f ? 0.97f + 0.05f * ui::seg(u, 0.12f, 0.24f)
                     : u < 0.72f ? 1.02f + 0.01f * ui::seg(u, 0.24f, 0.72f) : 1.03f + 0.11f * ui::seg(u, 0.72f, 1.0f);
    const float al = u < 0.12f ? ui::seg(u, 0.0f, 0.12f) : u < 0.72f ? 1.0f : 1.0f - ui::seg(u, 0.72f, 1.0f);
    const float split = u < 0.12f ? 48.0f - 32.0f * ui::seg(u, 0.0f, 0.12f) : 16.0f - 6.0f * ui::seg(u, 0.12f, 0.72f);
    ui::scaled(sc, 640, 346, [&] { ui::choc("GO !", 640, 226, 240, split, al); });
  }
  rlPopMatrix();
}
