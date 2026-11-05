// Results screen (« Ligne d'horizon »): the winner in its light on the left, the ranking on the right (with an award
// each for the most K.O., the Clash Orb breaker and the biggest blow), then the rematch: each player says it is ready
// (a CPU always is) and it starts by itself; or a button goes elsewhere.
#include <algorithm>
#include <cmath>

#include "game/app.hpp"
#include "game/fighter_art.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
constexpr FontKind kT = FontKind::Title, kU = FontKind::Ui;
const char* kButtons[4] = {"REVANCHE", "CHANGER DE PERSO", "CHANGER D'ARÈNE", "MENU PRINCIPAL"};
constexpr Color kDim{154, 168, 199, 255};    // #9AA8C7
constexpr Color kSoft{212, 222, 245, 255};   // #D4DEF5
constexpr Color kLight{255, 200, 112, 255};  // #FFC870

Rectangle buttonRect(int i) { return {664.0f + (i % 2) * 292.0f, 556.0f + (i / 2) * 60.0f, 284.0f, 52.0f}; }
Rectangle chipRect(int i, int n) {
  const float w = (576.0f - (n - 1) * 6.0f) / n;
  return {664.0f + i * (w + 6.0f), 500.0f, w, 40.0f};
}
Color rowColor(int port, int team) { return team >= 0 ? teamColor(team) : portColor(port); }

// Text coloured from `top` (its upper third) to `bottom`, drawn once per thin horizontal strip; caps centred on cy.
void vgradText(const char* text, float x, float cy, float size, Color top, Color bottom) {
  constexpr int kStrips = 14;
  const float w = textWidth(text, size, kT), y = capsY(cy, size, kT), cap = 0.64f * size;
  for (int i = 0; i < kStrips; ++i) {
    const float u = (i + 0.5f) / kStrips;
    const float y0 = i == 0 ? cy - size : cy - cap * 0.5f + cap * i / kStrips;
    const float y1 = i == kStrips - 1 ? cy + size : cy - cap * 0.5f + cap * (i + 1) / kStrips + 1.0f;
    beginClip({x - 8, y0, w + 16, y1 - y0});
    drawText(text, x, y, size, u < 0.3f ? top : mixColor(top, bottom, (u - 0.3f) / 0.7f), kT);
    endClip();
  }
}

// The key or button that says "ready" on that device (the menus' confirm).
float confirmWidth(int device, float h) {
  return device >= kPad1 ? ui::padGlyphWidth(ui::Btn::A, h, promptFamily())
                         : ui::keycapWidth(device == kKeyboard2 ? "." : "ESPACE", h);
}
void confirmGlyph(int device, float x, float y, float h) {
  if (device >= kPad1) ui::padGlyph(ui::Btn::A, x, y, h, promptFamily());
  else ui::keycap(device == kKeyboard2 ? "." : "ESPACE", x, y, h);
}

// The left half's light: a blue radial behind the winner, slow rays, its aura, diamonds rising.
void drawVictoryLight(Color aura, float t) {
  DrawRectangle(0, 0, kScreenW, kScreenH, {5, 12, 38, 255});
  const Vec2 c{300, 460};
  vfx::glow(c, 288, {36, 87, 184, 255}, {14, 42, 110, 255}, 64);
  vfx::ring(c, 288, 600, {14, 42, 110, 255}, {5, 12, 38, 255}, 64);
  const float rot = t * 2.0f * kPi / 40.0f;
  const Color ray{191, 217, 255, 33}, none{191, 217, 255, 0};
  for (int k = 0; k < 18; ++k) {  // 5° rays every 20°, faded out from a fifth of their length
    const float a0 = rot + k * kPi / 9.0f, a1 = a0 + kPi / 36.0f;
    const Vec2 d0{std::cos(a0), std::sin(a0)}, d1{std::cos(a1), std::sin(a1)};
    vfx::tri(c, c + d0 * 60.0f, c + d1 * 60.0f, ray, ray, ray);
    vfx::quad(c + d0 * 60.0f, c + d0 * 300.0f, c + d1 * 300.0f, c + d1 * 60.0f, ray, none, none, ray);
  }
  vfx::glow({300, 510}, 126, withAlpha(aura, 0.5f), withAlpha(aura, 0.12f));
  vfx::ring({300, 510}, 126, 210, withAlpha(aura, 0.12f), withAlpha(aura, 0.0f));
  static const float kX[12] = {40, 95, 150, 210, 260, 320, 370, 430, 480, 530, 120, 400};
  for (int i = 0; i < 12; ++i) {
    const float dur = 3.6f + (i % 3) * 0.7f, u = std::fmod(t - i * 0.37f + dur * 10.0f, dur) / dur;
    const float a = u < 0.15f ? u / 0.15f : 1.0f - (u - 0.15f) / 0.85f;
    const float s = 6.0f + (i % 3) * 3.0f;
    const Vec2 p{kX[i] + s * 0.5f, 600.0f + (i % 4) * 30.0f + s * 0.5f - 440.0f * (1.0f - (1.0f - u) * (1.0f - u))};
    const Color col = i % 2 ? kLight : WHITE;
    vfx::glow(p, s * 1.4f, withAlpha(col, 0.5f * a), withAlpha(col, 0.0f));
    ui::diamond(V(p), s * 0.71f, withAlpha(col, a));
  }
  // The right half, night; between them the horizon standing up (azure, white, gold)
  ui::quad({640, 0}, {kScreenW, 0}, {kScreenW, kScreenH}, {560, kScreenH}, ui::kInk, ui::kInk, ui::kInk, ui::kInk);
  const float ys[5] = {0, 144, 374, 576, 720};
  const Color cs[5] = {withAlpha(ui::kBlue, 0.0f), ui::kBlue, WHITE, ui::kGold, withAlpha(ui::kGold, 0.0f)};
  for (int i = 0; i < 4; ++i) {
    const float x0 = 637.0f - 80.0f * ys[i] / 720.0f, x1 = 637.0f - 80.0f * ys[i + 1] / 720.0f;
    ui::quad({x0, ys[i]}, {x0 + 6, ys[i]}, {x1 + 6, ys[i + 1]}, {x1, ys[i + 1]}, cs[i], cs[i], cs[i + 1], cs[i + 1]);
  }
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
  const int before = resSel_;
  if (nav.left || nav.right) resSel_ ^= 1;
  if (nav.up || nav.down) resSel_ ^= 2;
  for (int b = 0; b < 4; ++b) {
    if (ui::hovered(buttonRect(b)) && ui::mouseMoved()) resSel_ = b;
    if (ui::clicked(buttonRect(b))) return go(b);
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
  const float t = time_;
  const auto& R = roster();
  const int n = int(results_.size());
  const ResultRow* win = nullptr;
  for (const auto& r : results_)
    if (r.charIndex == winnerChar_ && r.port == winnerPort_) win = &r;
  const CharacterDef* wd = winnerChar_ >= 0 ? &R[winnerChar_] : nullptr;
  drawVictoryLight(wd ? auraColor(wd->hero) : ui::kBlue, t);

  // The winner: who, big, then its name and how it won
  const Color wc = winnerTeam_ >= 0 ? teamColor(winnerTeam_) : portColor(winnerPort_);
  if (wd) {
    Pose p = restPose(*wd, t);
    const float H = wd->height;
    p.handF = {0.42f * H, -0.46f * H};  // arms wide open, away from the face
    p.handB = {-0.32f * H, -0.44f * H};
    p.weaponDir = Vec2{0.35f, -1.0f}.norm();
    if (wd->style == BodyStyle::Puff) p.handF = {0.32f * H, -0.2f * H}, p.handB = {-0.32f * H, -0.2f * H};
    if (art::hasSheet(wd->hero, art::kSheetVictory)) {  // its victory pose, played again every 3 s
      p.sheet = art::kSheetVictory;
      p.sheetT = clampf(std::fmod(screenFrames_ / 60.0f, 3.0f) / 0.9f, 0.0f, 1.0f);
    }
    for (int i = 0; i < 3; ++i) DrawEllipse(305, 691, 150.0f - i * 48.0f, 15.0f - i * 4.5f, withAlpha(WHITE, 0.09f + i * 0.05f));
    drawShowcase(*wd, winnerPalette_, p, {305, 700}, 1, 330.0f, t);
  }
  vfx::glow({53, 50}, 16, withAlpha(WHITE, 0.6f), withAlpha(WHITE, 0.0f));
  ui::diamond({53, 50}, 7.0f, WHITE);
  ui::tracked(winnerTeam_ >= 0 ? "VICTOIRE DE L'ÉQUIPE" : wd ? "VICTOIRE" : "FIN DU COMBAT", 68, capsY(50, 19.5f, kT), 19.5f,
              0.32f, kLight);
  const char* big = winnerTeam_ >= 0 ? teamName(winnerTeam_)
                    : !wd           ? "ÉGALITÉ"
                    : winnerCpu_    ? "CPU"
                                    : TextFormat("J%d", winnerPort_ + 1);
  float bs = 139.0f;
  while (bs > 60.0f && textWidth(big, bs, kT) > 500.0f) bs -= 2.0f;
  vgradText(big, 46, 121, bs, WHITE, mixColor(wc, WHITE, 0.55f));
  if (wd) {
    const std::string nick = fiche(*wd).nick;
    float ns = 43.4f;
    while (ns > 24.0f && textWidth(nick.c_str(), ns, kT) > 520.0f) ns -= 1.0f;
    drawText(nick.c_str(), 48, capsY(198, ns, kT), ns, ui::kCream, kT);
    std::string info = "N° " + wd->id.substr(1);
    if (win) {
      info += " · " + (win->weapon >= 0 && win->weapon < int(wd->weapons.size()) ? wd->weapons[win->weapon].name
                                                                                : std::string("Mains nues"));
      if (win->stocks > 0) info += TextFormat(" · %d vie%s restante%s", win->stocks, win->stocks > 1 ? "s" : "", win->stocks > 1 ? "s" : "");
    }
    drawText(info.c_str(), 48, capsY(231, 19.2f, kU), 19.2f, ui::kSky, kU);
  }

  // The ranking: place, face, name and award, then K.O., falls, damage dealt and Clash Orbs broken
  ui::tracked("RÉSULTATS", 664, capsY(64.5f, 39, kT), 39, 0.04f, ui::kCream);
  const char* rules = TextFormat("%s · %d vie%s · %d:%02d", resultStage_.c_str(), resultStocks_, resultStocks_ > 1 ? "s" : "",
                                 resultSeconds_ / 60, resultSeconds_ % 60);
  drawText(rules, 1240 - textWidth(rules, 17.8f, kU), capsY(72.4f, 17.8f, kU), 17.8f, kDim, kU);
  const float colRight[4] = {1028, 1094, 1166, 1228};
  const char* heads[4] = {"KO", "CHUTES", "DÉGÂTS", "ORBES"};
  ui::tracked("PLACE", 664, capsY(110.5f, 13, kT), 13, 0.14f, kDim);
  ui::tracked("JOUEUR", 726, capsY(110.5f, 13, kT), 13, 0.14f, kDim);
  for (int c = 0; c < 4; ++c) {
    const float w = ui::trackedWidth(heads[c], 13, 0.14f);
    ui::tracked(heads[c], colRight[c] - w, capsY(110.5f, 13, kT), 13, 0.14f, kDim);
    if (c == 3) drawClashOrb({colRight[c] - w - 11, 110.5f}, 6.0f, t, 0);
  }
  int maxKo = 0, maxOrb = 0, bestHit = -1;
  for (int i = 0; i < n; ++i) {
    const ResultRow& r = results_[i];
    maxKo = std::max(maxKo, r.kos);
    maxOrb = std::max(maxOrb, r.orbs);
    if (r.bestHit >= 1.0f && (bestHit < 0 || r.bestHit > results_[bestHit].bestHit)) bestHit = i;
  }
  float y = 126;
  for (int i = 0; i < n; ++i) {
    const ResultRow& r = results_[i];
    const CharacterDef& d = R[r.charIndex];
    const bool first = i == 0;
    const float h = first ? 92.0f : 64.0f, cy = y + h * 0.5f;
    const float e = clampf((screenFrames_ - 12 - i * 5.4f) / 25.0f, 0.0f, 1.0f), ease = 1.0f - std::pow(1.0f - e, 3.0f);
    const float x = 640 + (1.0f - ease) * 40.0f, a = ease;
    const Color pc = rowColor(r.port, r.team);
    if (first) {
      const Color c0 = withAlpha({255, 181, 71, 255}, 0.2f * a), c1 = withAlpha(ui::kNavy, 0.9f * a);
      ui::quad({x, y}, {x + 330, y}, {x + 330, y + h}, {x, y + h}, c0, c1, c1, c0);
      DrawRectangleRec({x + 330, y, 270, h}, c1);
      DrawRectangleLinesEx({x, y, 600, h}, 1.0f, withAlpha(ui::kGold, 0.35f * a));
      DrawRectangleRec({x, y, 3, h}, withAlpha(ui::kGold, a));
    } else {
      DrawRectangleRec({x, y, 600, h}, withAlpha(ui::kNavy, 0.82f * a));
      DrawRectangleRec({x, y, 3, h}, withAlpha(pc, a));
    }
    // Place
    const float ps = first ? 56.4f : 41.2f, os = first ? 22.6f : 16.5f;
    const char* place = TextFormat("%d", i + 1);
    const Color placeCol = withAlpha(first ? kLight : kSoft, a);
    drawText(place, x + 24, capsY(cy, ps, kT), ps, placeCol, kT);
    drawText(first ? "er" : "e", x + 25 + textWidth(place, ps, kT), capsY(cy - (first ? 8.0f : 5.8f) - ps * 0.12f, os, kT), os,
             placeCol, kT);
    // Face
    const float pic = first ? 64.0f : 48.0f, px = x + 86;
    const Rectangle face{px, cy - pic * 0.5f, pic, pic};
    const Color tint = mixColor(auraColor(d.hero), ui::kInk, 0.45f);
    ui::bevel(face, 8, withAlpha(tint, a), withAlpha(mixColor(tint, ui::kInk, 0.4f), a));
    drawPortraitIn(d, r.palette, {face.x + pic * 0.5f, face.y + pic * 0.6f}, pic * 1.28f, face, 0.0f, t + i);
    if (a < 1.0f) DrawRectangleRec(face, withAlpha(ui::kInk, 1.0f - a));  // fading in with its row
    DrawRectangleRec({face.x, face.y + pic - 3, pic, 3}, withAlpha(pc, a));
    // Tag, name, award
    const float nx = px + pic + 10, right = colRight[0] - 44 - 10;
    // its first award (tied: everyone gets it)
    const char* award = maxKo > 0 && r.kos == maxKo     ? "Le plus de KO"
                        : maxOrb > 0 && r.orbs == maxOrb ? "Casseur d'orbe"
                        : i == bestHit                   ? TextFormat("Plus gros coup : %d %%", int(std::lround(r.bestHit)))
                                                         : nullptr;
    const float ly = award ? cy - 10.5f : cy;
    const char* tag = r.cpu ? "CPU" : TextFormat("J%d", r.port + 1);
    const float tw = textWidth(tag, 15.2f, kT) + 12;
    DrawRectangleRec({nx, ly - 11, tw, 22}, withAlpha(pc, a));
    drawText(tag, nx + 6, capsY(ly, 15.2f, kT), 15.2f, withAlpha(ui::kInk, a), kT);
    std::string nick = fiche(d).nick;
    if (r.cpu) nick += TextFormat(" · NIV. %d", r.level);
    float ns = first ? 28.2f : 21.7f;
    while (ns > 14.0f && textWidth(nick.c_str(), ns, kT) > right - nx - tw - 8) ns -= 0.5f;
    drawText(nick.c_str(), nx + tw + 8, capsY(ly, ns, kT), ns, withAlpha(ui::kCream, a), kT);
    if (award) {
      const float ay = cy + (first ? 14.5f : 12.5f), aw = textWidth(award, 15.8f, kU) + 27;
      DrawRectangleLinesEx({nx, ay - 9, aw, 18}, 1.0f, withAlpha(kLight, 0.55f * a));
      ui::diamond({nx + 10, ay}, 4.2f, withAlpha(kLight, a));
      drawText(award, nx + 19, capsY(ay, 15.8f, kU), 15.8f, withAlpha({255, 226, 181, 255}, a), kU);
    }
    // The numbers (gold: the best)
    const int vals[4] = {r.kos, r.falls, int(r.damage), r.orbs};
    for (int c = 0; c < 4; ++c) {
      const char* v = TextFormat("%d", vals[c]);
      const bool best = (c == 0 && r.kos == maxKo && maxKo > 0) || (c == 3 && r.orbs == maxOrb && maxOrb > 0);
      drawText(v, colRight[c] - textWidth(v, 26, kT), capsY(cy, 26, kT), 26, withAlpha(best ? kLight : kSoft, a), kT);
    }
    y += h + 6;
  }

  // The rematch: who is ready, then the four ways out
  int humans = 0, ready = 0;
  std::string waiting;
  for (const auto& r : results_) {
    if (r.cpu) {
      ++ready;
      continue;
    }
    ++humans;
    if (resReady_[r.port & 3]) ++ready;
    else waiting += (waiting.empty() ? "" : ", ") + std::string(TextFormat("J%d", r.port + 1));
  }
  ui::tracked("REVANCHE ?", 664, capsY(480.3f, 14.1f, kT), 14.1f, 0.22f, kDim);
  const bool all = humans > 0 && ready == n;
  const char* status = all ? TextFormat("Tout le monde est prêt, la revanche part dans %d s", std::max(1, (resGo_ + 59) / 60))
                       : humans == 0 ? "Les ordinateurs sont prêts"
                                     : TextFormat("En attente de %s", waiting.c_str());
  drawText(status, 1240 - textWidth(status, 17.8f, kU), capsY(480.3f, 17.8f, kU), 17.8f, all ? kLight : kDim, kU);
  for (int i = 0; i < n; ++i) {
    const ResultRow& r = results_[i];
    const Rectangle c = chipRect(i, n);
    const bool on = r.cpu || resReady_[r.port & 3];
    DrawRectangleRec(c, on ? Color{255, 181, 71, 31} : Color{10, 16, 36, 255});
    DrawRectangleLinesEx(c, 1.0f, on ? Color{255, 181, 71, 128} : Color{191, 217, 255, 36});
    const Color pc = rowColor(r.port, r.team);
    const char* tag = r.cpu ? "CPU" : TextFormat("J%d", r.port + 1);
    const float tw = textWidth(tag, 14.1f, kT) + 12, cy = c.y + c.height * 0.5f;
    DrawRectangleRec({c.x + 10, cy - 10, tw, 20}, pc);
    drawText(tag, c.x + 16, capsY(cy, 14.1f, kT), 14.1f, ui::kInk, kT);
    float lx = c.x + 10 + tw + 8;
    if (on) {
      ui::tracked(r.cpu ? "AUTO" : "PRÊT", lx, capsY(cy, 15.2f, kT), 15.2f, 0.06f, kLight);
    } else {  // blinking: the key that makes it ready
      const int dev = slots_[r.port & 3].device;
      const float blink = 0.7f + 0.3f * std::cos(t * 2.0f * kPi / 1.2f);
      const bool words = lx + ui::trackedWidth("APPUIE SUR", 15.2f, 0.06f) + 6 + confirmWidth(dev, 22) < c.x + c.width - 8;
      if (words) lx += ui::tracked("APPUIE SUR", lx, capsY(cy, 15.2f, kT), 15.2f, 0.06f, withAlpha(kSoft, blink)) + 6;
      confirmGlyph(dev, lx, cy - 11, 22);
    }
  }
  for (int b = 0; b < 4; ++b) {
    const Rectangle r = buttonRect(b);
    const bool gold = b == 0, hot = ui::hovered(r);
    if (gold && all) {  // ready to go: it beats
      const float k = 0.5f - 0.5f * std::cos(t * 2.0f * kPi / 1.4f);
      ui::bevel(ui::grow(r, 2.0f + 5.0f * k), 16, withAlpha(ui::kGold, 0.4f * k), withAlpha(ui::kGold, 0.4f * k));
    }
    const Color face = gold ? (hot ? mixColor(ui::kGold, WHITE, 0.15f) : ui::kGold) : hot ? mixColor(ui::kNavy, WHITE, 0.08f) : ui::kNavy;
    ui::bevel(r, 14, face, face);
    if (gold) ui::shimmer(r, t, 0.8f);
    ui::tracked(kButtons[b], r.x + 18, capsY(r.y + 26, 26, kT), 26, 0.06f, gold ? ui::kInk : ui::kCream);
    if (gold) {
      const char* cnt = TextFormat("%d / %d prêts", ready, n);
      drawText(cnt, r.x + r.width - 18 - textWidth(cnt, 17.8f, kU), capsY(r.y + 26, 17.8f, kU), 17.8f, ui::kInk, kU);
    }
    if (b == resSel_ && screenFrames_ > 40) ui::rim(r, 14, t);
  }
}
