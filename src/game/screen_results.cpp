// Results screen (« Épure »): the winner on the left, in its colour (rays, its name huge and faint, two slabs, the
// manga portrait behind its victory pose); the ranking on the right (with an award each for the most K.O., the Clash
// Orb breaker and the biggest blow), then the rematch: each player says it is ready (a CPU always is) and it starts by
// itself; or one of four buttons goes elsewhere.
#include <algorithm>
#include <cmath>

#include "game/app.hpp"
#include "game/fighter_art.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
constexpr FontKind kD = FontKind::Display, kB = FontKind::Body;
const char* kButtons[4] = {"REVANCHE", "CHANGER DE PERSO", "CHANGER D'ARÈNE", "MENU PRINCIPAL"};
const ui::Icon kButtonIcons[4] = {ui::Icon::Rematch, ui::Icon::Fighters, ui::Icon::Arenas, ui::Icon::Home};
constexpr Color kCpuGrey{163, 167, 175, 255}, kInk{10, 11, 14, 255};
constexpr float kRowsTop = 134, kRowStep = 72;  // rows of 68, 4 apart

// The rematch line sits under the ranking; the buttons under it, in two columns.
float rematchY(int n) { return kRowsTop + n * kRowStep + 34; }
Rectangle buttonRect(int i, int n) { return {620.0f + (i % 2) * 303.0f, rematchY(n) + 30 + (i / 2) * 66.0f, 293.0f, 56.0f}; }
Rectangle chipRect(int i, int n) {  // the ready chips, right-aligned on the rematch line
  constexpr float w = 118, gap = 8;
  return {1216.0f - (n - i) * (w + gap) + gap, rematchY(n) - 14, w, 28};
}
Color rowColor(bool cpu, int port, int team) { return cpu ? kCpuGrey : team >= 0 ? teamColor(team) : portColor(port); }

// The key or button that says "ready" on that device (the menus' confirm).
float confirmWidth(int device, float h) {
  return device >= kPad1 ? ui::padGlyphWidth(ui::Btn::A, h, promptFamily())
                         : ui::keycapWidth(device == kKeyboard2 ? "." : "ESPACE", h);
}
void confirmGlyph(int device, float x, float y, float h) {
  if (device >= kPad1) ui::padGlyph(ui::Btn::A, x, y, h, promptFamily());
  else ui::keycap(device == kKeyboard2 ? "." : "ESPACE", x, y, h);
}

// A slab rotated -12° about its left middle, grown to `g` of its length.
void slab(Vector2 origin, float len, float h, float g, Color col) {
  const float a = -12.0f * kPi / 180.0f, ca = std::cos(a), sa = std::sin(a), l = len * g;
  auto at = [&](float x, float y) { return Vector2{origin.x + x * ca - y * sa, origin.y + x * sa + y * ca}; };
  ui::quad(at(0, -h * 0.5f), at(l, -h * 0.5f), at(l, h * 0.5f), at(0, h * 0.5f), col, col, col, col);
}

// The left side's light: a glow of the winner's colour low down, slow rays, its name faint, two slabs.
void drawVictoryLight(Color cc, const char* name, float t, float f) {
  ui::dusk();
  auto ease = [&](float delay, float len) {
    const float u = clampf((f - delay) / len, 0.0f, 1.0f);
    return 1.0f - (1.0f - u) * (1.0f - u) * (1.0f - u);
  };
  beginClip({0, 0, 580, kScreenH});
  ui::glow({232, 650}, 460, withAlpha(cc, 0.3f));
  const Vec2 c{220, 520};
  const float rot = t * 2.0f * kPi / 60.0f, len = 380;
  const Color ray = withAlpha(cc, 0.2f), none = withAlpha(cc, 0.0f);
  for (int k = 0; k < 24; ++k) {  // 5° rays every 15°, fading out
    const float a0 = rot + k * kPi / 12.0f, a1 = a0 + kPi / 36.0f;
    const Vec2 d0{std::cos(a0), std::sin(a0)}, d1{std::cos(a1), std::sin(a1)};
    vfx::quad(c, c + d0 * len, c + d1 * len, c, ray, none, none, ray);
  }
  if (name) {
    float gs = 150;
    while (gs > 90 && textWidth(name, gs, kD) > 540) gs -= 2;
    drawText(name, 24, capsY(275, gs, kD), gs, withAlpha(cc, 0.12f), kD);
  }
  const float g = ease(9, 42);
  slab({-120, 615}, 820, 110, g, cc);
  slab({-120, 695}, 820, 10, g, withAlpha(ui::kCream, 0.8f));
  endClip();
}
}  // namespace

void App::updateResults(const MenuNav& nav) {
  if (screenFrames_ == 25) audio_.voice("winner");  // "The winner is..." then the name
  if (screenFrames_ == 105 && winnerTeam_ >= 0) audio_.voice(winnerTeam_ == 1 ? "blueteam" : "redteam");
  else if (screenFrames_ == 105 && winnerChar_ >= 0) audio_.voice(voiceKey(roster()[winnerChar_].name).c_str());
  if (screenFrames_ <= 40) return;  // the fight's last presses never skip the results

  auto go = [&](int b) {
    audio_.play(Sfx::MenuSelect);
    if (b == 0) startMatch();
    else goTo(b == 1 ? Screen::CharSelect : b == 2 ? Screen::StageSelect : Screen::MainMenu);
  };
  const int before = resSel_, rows = int(results_.size());
  if (nav.left || nav.right) resSel_ ^= 1;
  if (nav.up || nav.down) resSel_ ^= 2;
  for (int b = 0; b < 4; ++b) {
    if (ui::hovered(buttonRect(b, rows)) && ui::mouseMoved()) resSel_ = b;
    if (ui::clicked(buttonRect(b, rows))) return go(b);
  }
  if (resSel_ != before) audio_.play(Sfx::MenuMove, 0.6f);

  // On the rematch, each player's own confirm says it is ready (or not any more); ENTRÉE / START starts it now
  int humans = 0, ready = 0;
  const int n = int(results_.size());
  for (int i = 0; i < n; ++i) {
    const ResultRow& r = results_[i];
    if (r.cpu) continue;
    ++humans;
    bool& on = resReady_[r.port & 3];
    const bool press = (resSel_ == 0 && readMenuNavFor(slots_[r.port & 3].device).confirm) || ui::clicked(chipRect(i, n));
    if (press) {
      on = !on;
      audio_.play(on ? Sfx::MenuSelect : Sfx::MenuBack);
    }
    ready += on;
  }
  if (nav.back) {
    audio_.play(Sfx::MenuBack);
    goTo(Screen::CharSelect);
    return;
  }
  if (resSel_ == 0 ? nav.start || (humans == 0 && nav.confirm) : nav.confirm || nav.start) return go(resSel_);
  if (humans > 0 && ready == humans) {  // everyone is: it starts by itself (sooner for a lone player)
    if (resGo_ < 0) resGo_ = humans > 1 ? 180 : 45;
    if (--resGo_ <= 0) return go(0);
  } else {
    resGo_ = -1;
  }
}

void App::drawResults() {
  const float t = time_, f = float(screenFrames_);
  const auto& R = roster();
  const int n = int(results_.size());
  auto ease = [&](float delay, float len) {
    const float u = clampf((f - delay) / len, 0.0f, 1.0f);
    return 1.0f - (1.0f - u) * (1.0f - u) * (1.0f - u);
  };
  const ResultRow* win = nullptr;
  for (const auto& r : results_)
    if (r.charIndex == winnerChar_ && r.port == winnerPort_) win = &r;
  const CharacterDef* wd = winnerChar_ >= 0 ? &R[winnerChar_] : nullptr;
  const Color cc = wd ? auraColor(wd->hero) : ui::kStone;
  const std::string wname = wd ? upperFr(fiche(*wd).nick) : std::string();
  drawVictoryLight(cc, wd ? wname.c_str() : nullptr, t, f);

  // The winner: its manga portrait faint behind, then its victory pose (played again every 3 s) on the horizon
  if (wd) {
    float aw, ah;
    const std::string art = wd->id + "_art";
    if (art::pictureSize(art.c_str(), aw, ah)) {
      const float in = ease(0, 72), h = 640, w = h * aw / ah;
      beginClip({0, 0, 600, kScreenH});  // the ranking stays clean
      art::drawPictureFaded(art.c_str(), {210 - 40 * (1 - in), 720 - h, w, h}, -0.35f, 0.4f, 0.22f * in);
      endClip();
    }
    const float lw = 500 * ease(0, 48);
    ui::quad({40, 654}, {40 + lw * 0.3f, 654}, {40 + lw * 0.3f, 655}, {40, 655}, withAlpha(ui::kCream, 0), withAlpha(ui::kCream, 0.5f),
             withAlpha(ui::kCream, 0.5f), withAlpha(ui::kCream, 0));
    ui::quad({40 + lw * 0.3f, 654}, {40 + lw, 654}, {40 + lw, 655}, {40 + lw * 0.3f, 655}, withAlpha(ui::kCream, 0.5f),
             withAlpha(ui::kCream, 0), withAlpha(ui::kCream, 0), withAlpha(ui::kCream, 0.5f));
    Pose p = restPose(*wd, t);
    const float H = wd->height;
    p.handF = {0.42f * H, -0.46f * H};  // arms wide open, away from the face
    p.handB = {-0.32f * H, -0.44f * H};
    p.weaponDir = Vec2{0.35f, -1.0f}.norm();
    if (wd->style == BodyStyle::Puff) p.handF = {0.32f * H, -0.2f * H}, p.handB = {-0.32f * H, -0.2f * H};
    if (art::hasSheet(wd->hero, art::kSheetVictory)) {
      p.sheet = art::kSheetVictory;
      p.sheetT = clampf(std::fmod(f / 60.0f, 3.0f) / 0.9f, 0.0f, 1.0f);
    }
    DrawEllipse(214, 694, 110, 12, withAlpha(BLACK, 0.3f));
    drawShowcase(*wd, winnerPalette_, p, {214, 700}, 1, 330.0f, t);
  }

  // Who won: the label, its name, then how
  {
    const float in = ease(6, 42), dy = 26 * (1 - in);
    ui::icon(ui::Icon::Trophy, {73, 63 + dy}, 18, withAlpha(cc, in), 1.9f);
    ui::label(winnerTeam_ >= 0 ? "VICTOIRE DE L'ÉQUIPE" : wd ? "VICTOIRE" : "FIN DU COMBAT", 92, 63 + dy, 12, 0.32f, withAlpha(cc, in));
  }
  {
    const float in = ease(12, 42), dy = 26 * (1 - in);
    const std::string big = winnerTeam_ >= 0 ? upperFr(teamName(winnerTeam_)) : wd ? wname : std::string("ÉGALITÉ");
    float bs = 84;
    while (bs > 40 && textWidth(big.c_str(), bs, kD) > 500) bs -= 2;
    drawText(big.c_str(), 64, capsY(126 + dy, bs, kD), bs, withAlpha(ui::kCream, in), kD);
  }
  if (wd) {
    const float in = ease(18, 42), dy = 26 * (1 - in);
    std::string info = (winnerCpu_ ? std::string("CPU") : std::string(TextFormat("J%d", winnerPort_ + 1))) + " · N° " + wd->id.substr(1);
    if (winnerTeam_ >= 0) info = upperFr(fiche(*wd).nick) + " · " + info;
    if (win) {
      info += " · " + (win->weapon >= 0 && win->weapon < int(wd->weapons.size()) ? wd->weapons[win->weapon].name
                                                                                : std::string("Mains nues"));
      if (win->stocks > 0) info += TextFormat(" · %d vie%s restante%s", win->stocks, win->stocks > 1 ? "s" : "", win->stocks > 1 ? "s" : "");
    }
    drawText(info.c_str(), 64, capsY(186 + dy, 14, kB), 14, withAlpha(ui::kSoft, in), kB);
  }

  // The ranking: place, face, tag, name and award, then K.O., falls, damage dealt and Clash Orbs broken
  {
    const float in = ease(12, 42), dy = 26 * (1 - in);
    drawText("RÉSULTATS", 620, capsY(68 + dy, 24, kD), 24, withAlpha(ui::kCream, in), kD);
    const char* rules = TextFormat("%s · %d vie%s · %d:%02d", resultStage_.c_str(), resultStocks_, resultStocks_ > 1 ? "s" : "",
                                   resultSeconds_ / 60, resultSeconds_ % 60);
    drawText(rules, 1216 - textWidth(rules, 12, kB), capsY(70 + dy, 12, kB), 12, withAlpha(ui::kMuted, in), kB);
  }
  const float colRight[4] = {1004, 1068, 1140, 1200};
  {
    const float in = ease(18, 42), cy = 117 + 26 * (1 - in);
    const Color lc = withAlpha(ui::kMuted, in);
    ui::label("PLACE", 636, cy, 11, 0.16f, lc);
    ui::label("JOUEUR", 700, cy, 11, 0.16f, lc);
    const char* heads[4] = {"KO", "CHUTES", "DÉGÂTS", "ORBES"};
    for (int c = 0; c < 4; ++c) ui::label(heads[c], colRight[c] - ui::labelWidth(heads[c], 11, 0.16f), cy, 11, 0.16f, lc);
  }
  int maxKo = 0, maxOrb = 0, bestHit = -1;
  for (int i = 0; i < n; ++i) {
    const ResultRow& r = results_[i];
    maxKo = std::max(maxKo, r.kos);
    maxOrb = std::max(maxOrb, r.orbs);
    if (r.bestHit >= 1.0f && (bestHit < 0 || r.bestHit > results_[bestHit].bestHit)) bestHit = i;
  }
  for (int i = 0; i < n; ++i) {
    const ResultRow& r = results_[i];
    const CharacterDef& d = R[r.charIndex];
    const float a = ease(24 + i * 6, 42), y = kRowsTop + i * kRowStep + 26 * (1 - a), cy = y + 34;
    const Color pc = rowColor(r.cpu, r.port, r.team);
    if (i == 0) ui::roundRect({620, y, 596, 68}, 14, withAlpha(ui::kCream, 0.06f * a), withAlpha(ui::kCream, 0.06f * a));
    // Place, its suffix raised
    const char* place = TextFormat("%d", i + 1);
    const Color plc = withAlpha(i == 0 ? ui::kCream : ui::kMuted, a);
    drawText(place, 636, capsY(cy, 30, kD), 30, plc, kD);
    drawText(i == 0 ? "er" : "e", 638 + textWidth(place, 30, kD), capsY(cy - 7, 13, kD), 13, plc, kD);
    // Face in a ringed disc
    const Vec2 fc{722, cy};
    DrawCircleV(V(fc), 24, withAlpha(pc, a));
    DrawCircleV(V(fc), 22, withAlpha(ui::kNavy, a));
    DrawCircleGradient(int(fc.x), int(fc.y), 22, withAlpha(mixColor(auraColor(d.hero), ui::kNavy, 0.55f), a), withAlpha(ui::kNavy, a));
    drawArtDisc(d, r.palette, fc, 22, t + i, 1.1f, withAlpha(WHITE, a));
    // Tag, name; its first award under them (tied: everyone gets it)
    const char* award = maxKo > 0 && r.kos == maxKo     ? "Le plus de KO"
                        : maxOrb > 0 && r.orbs == maxOrb ? "Casseur d'orbe"
                        : i == bestHit                   ? TextFormat("Plus gros coup : %d %%", int(std::lround(r.bestHit)))
                                                         : nullptr;
    const float ly = award ? cy - 9 : cy, nx = 758;
    const char* tag = r.cpu ? "CPU" : TextFormat("J%d", r.port + 1);
    const float tw = ui::labelWidth(tag, 10, 0.04f) + 12;
    ui::roundRect({nx, ly - 10, tw, 20}, 5, withAlpha(pc, a), withAlpha(pc, a));
    ui::label(tag, nx + 6, ly, 10, 0.04f, withAlpha(kInk, a));
    std::string nick = upperFr(fiche(d).nick);
    if (r.cpu) nick += TextFormat(" · NIV. %d", r.level);
    const float room = colRight[0] - 40 - (nx + tw + 12);
    float ns = 14;
    while (ns > 9 && textWidth(nick.c_str(), ns, kD) > room) ns -= 0.5f;
    drawText(nick.c_str(), nx + tw + 12, capsY(ly, ns, kD), ns, withAlpha(ui::kCream, a), kD);
    if (award) drawText(award, nx, capsY(cy + 12, 12, kB), 12, withAlpha(ui::kBlue, a), kB);
    // The numbers (cyan: the best)
    const int vals[4] = {r.kos, r.falls, int(r.damage), r.orbs};
    for (int c = 0; c < 4; ++c) {
      const char* v = TextFormat("%d", vals[c]);
      const bool best = (c == 0 && r.kos == maxKo && maxKo > 0) || (c == 3 && r.orbs == maxOrb && maxOrb > 0);
      drawText(v, colRight[c] - textWidth(v, 18, kB), capsY(cy, 18, kB), 18, withAlpha(best ? ui::kBlue : ui::kCream, a), kB);
    }
  }

  // The rematch: who is ready, then the four ways out
  int humans = 0, ready = 0;
  for (const auto& r : results_) {
    if (r.cpu) {
      ++ready;
      continue;
    }
    ++humans;
    if (resReady_[r.port & 3]) ++ready;
  }
  const bool all = humans > 0 && ready == n;
  {
    const float a = ease(36, 42), ry = rematchY(n) + 26 * (1 - a);
    ui::label("REVANCHE ?", 620, ry, 11, 0.16f, withAlpha(ui::kMuted, a));
    for (int i = 0; i < n; ++i) {
      const ResultRow& r = results_[i];
      Rectangle c = chipRect(i, n);
      c.y += 26 * (1 - a);
      const bool on = r.cpu || resReady_[r.port & 3];
      const Color pc = rowColor(r.cpu, r.port, r.team);
      ui::roundLines(c, 8, 1.0f, on && !r.cpu ? withAlpha(pc, 0.6f * a) : withAlpha(ui::kCream, 0.16f * a));
      const char* tag = r.cpu ? "CPU" : TextFormat("J%d", r.port + 1);
      const float cy = c.y + 14;
      float lx = c.x + 10 + ui::label(tag, c.x + 10, cy, 11, 0.04f, withAlpha(r.cpu ? ui::kCream : pc, a)) + 6;
      if (r.cpu) {
        ui::label("Auto", lx, cy, 11, 0.02f, withAlpha(ui::kSoft, a));
      } else if (on) {
        lx += ui::label("Prêt", lx, cy, 11, 0.02f, withAlpha(ui::kCream, a)) + 6;
        const Color ck = withAlpha(ui::kCream, a);
        DrawLineEx({lx, cy + 0.5f}, {lx + 3.5f, cy + 4}, 2.0f, ck);
        DrawLineEx({lx + 3.5f, cy + 4}, {lx + 10, cy - 4}, 2.0f, ck);
      } else {  // blinking: the key that makes it ready
        const float blink = 0.7f + 0.3f * std::cos(t * 2.0f * kPi / 1.2f);
        const int dev = slots_[r.port & 3].device;
        if (lx + confirmWidth(dev, 18) <= c.x + c.width - 6) {
          confirmGlyph(dev, lx, cy - 9, 18);
        } else {
          ui::label("…", lx, cy, 11, 0.02f, withAlpha(ui::kSoft, blink * a));
        }
      }
    }
  }
  const float ba = ease(42, 42);
  for (int b = 0; b < 4; ++b) {
    Rectangle r = buttonRect(b, n);
    r.y += 26 * (1 - ba);
    const bool main = b == 0, hot = ui::hovered(r);
    if (main && all) {  // ready to go: it beats
      const float k = 0.5f - 0.5f * std::cos(t * 2.0f * kPi / 1.4f);
      ui::roundRect(ui::grow(r, 2.0f + 5.0f * k), 18, withAlpha(ui::kCream, 0.3f * k * ba), withAlpha(ui::kCream, 0.3f * k * ba));
    }
    if (main) ui::roundRect(r, 14, withAlpha(ui::kCream, ba), withAlpha(ui::kCream, ba));
    else {
      if (hot || b == resSel_) ui::roundRect(r, 14, withAlpha(ui::kCream, 0.06f * ba), withAlpha(ui::kCream, 0.06f * ba));
      ui::roundLines(r, 14, 1.0f, withAlpha(ui::kCream, 0.16f * ba));
    }
    const Rectangle box{r.x + 8, r.y + 9, 38, 38};
    if (main) ui::roundRect(box, 10, withAlpha(ui::kInk, ba), withAlpha(ui::kInk, ba));
    else ui::roundLines(box, 10, 1.0f, withAlpha(ui::kCream, 0.16f * ba));
    ui::icon(kButtonIcons[b], {box.x + 19, box.y + 19}, 20, withAlpha(main ? ui::kCream : ui::kSoft, ba), 1.8f);
    const Color tc = withAlpha(main ? ui::kInk : ui::kCream, ba);
    drawText(kButtons[b], r.x + 60, capsY(r.y + 28, 13, kD), 13, tc, kD);
    if (main) {
      const char* cnt = all ? TextFormat("Départ dans %d s", std::max(1, (resGo_ + 59) / 60)) : TextFormat("%d / %d prêts", ready, n);
      drawText(cnt, r.x + r.width - 16 - textWidth(cnt, 12, kB), capsY(r.y + 28, 12, kB), 12, tc, kB);
    }
    if (b == resSel_ && screenFrames_ > 40) ui::rim(r, 14, t);
  }
}
