// Results screen (« Prestige », the mock-up's « Victoire »): the winner's colour floods the left, cut by the blade, his
// name scrolling in it; his own victory clip in pixel art x4; VICTOIRE white on a night plate over a brush stroke of
// his colour. On the right the ranking (with an award each for the most K.O., the Clash Orb breaker and the biggest
// blow), then the rematch: each player says it is ready (a CPU always is) and it starts by itself; or one of four
// buttons goes elsewhere.
#include <algorithm>
#include <cmath>

#include "game/app.hpp"
#include "game/fighter_art.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
constexpr FontKind kB = FontKind::Body;
const char* kButtons[4] = {"REVANCHE", "CHANGER DE PERSO", "CHANGER D'ARÈNE", "MENU PRINCIPAL"};
const ui::Icon kButtonIcons[4] = {ui::Icon::Rematch, ui::Icon::Fighters, ui::Icon::Arenas, ui::Icon::Home};
constexpr Color kRim{214, 226, 255, 255};

// The ranking's rows from 124, 80 apart; the rematch line and the four buttons (two columns) under them.
float rowsEnd(int n) { return 124.0f + n * 80.0f; }
Rectangle buttonRect(int i, int n) { return {660.0f + (i % 2) * 296.0f, rowsEnd(n) + 56 + (i / 2) * 66.0f, 288.0f, 56.0f}; }
Rectangle chipRect(int i, int n) {  // the ready chips on the rematch line, from 1000 (or right-aligned when many)
  constexpr float w = 96, gap = 8;
  const float x0 = std::min(1000.0f, 1244.0f - n * (w + gap) + gap);
  return {x0 + i * (w + gap), rowsEnd(n) + 14, w, 28};
}
Color rowColor(bool cpu, int port, int team) { return cpu ? ui::kCpuTag : team >= 0 ? teamColor(team) : portColor(port); }

// The key or button that says "ready" on that device (the menus' confirm).
float confirmWidth(int device, float h) {
  return device >= kPad1 ? ui::padGlyphWidth(ui::Btn::A, h, promptFamily())
                         : ui::keycapWidth(device == kKeyboard2 ? "." : "ESPACE", h);
}
void confirmGlyph(int device, float x, float y, float h) {
  if (device >= kPad1) ui::padGlyph(ui::Btn::A, x, y, h, promptFamily());
  else ui::keycap(device == kKeyboard2 ? "." : "ESPACE", x, y, h);
}

// Inter at 12-14 px, its line box top at y; returns its width.
float drawTextW(const char* s, float x, float y, float size, Color c) {
  drawText(s, x, y, size, c, kB);
  return textWidth(s, size, kB);
}

// A fighter's pixel portrait in a 48 px square (his framed larger: his portrait leaves room around him).
void face(const CharacterDef& d, float x, float y, float a) {
  const std::string pic = d.id + "_portrait";
  float pw, ph;
  if (!art::pictureSize(pic.c_str(), pw, ph)) return;
  const bool roomy = d.id == "h01";
  const float s = (roomy ? 96.0f : 68.0f) / 240.0f;
  beginClip({x, y, 48, 48});
  art::drawPicture(pic.c_str(), {0, 0, pw, ph}, {x + (roomy ? -12.0f : -11.0f), y + (roomy ? -10.0f : -7.0f), pw * s, ph * s}, a);
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
  const float t = time_, s = screenFrames_ / 60.0f;
  const auto& R = roster();
  const int n = int(results_.size());
  auto in = [&](float delay, float len, float x1 = 0.2f, float y1 = 0.9f, float x2 = 0.1f) {
    return ui::seg(s, delay, delay + len, x1, y1, x2, 1.0f);
  };
  const ResultRow* win = nullptr;
  for (const auto& r : results_)
    if (r.charIndex == winnerChar_ && r.port == winnerPort_) win = &r;
  const CharacterDef* wd = winnerChar_ >= 0 ? &R[winnerChar_] : nullptr;
  const ui::Flood fl = wd ? ui::flood(wd->hero) : ui::Flood{ui::kDim, ui::kNightUp};
  const std::string wname = wd ? upperFr(fiche(*wd).nick) : std::string();
  DrawRectangle(0, 0, kScreenW, kScreenH, ui::kNight);

  // The winner's side, jolted as VICTOIRE lands: his colour floods in cut by the blade, his name scrolling in it
  const Vector2 sh = ui::slam(s - 0.5f);
  rlPushMatrix();
  rlTranslatef(sh.x, sh.y, 0.0f);
  const float ox = -700.0f * (1.0f - in(0.0f, 0.42f, 0.7f, 0.0f, 0.2f));
  ui::lean(-400 + ox, 500 + ox, fl.fill);
  ui::lean(500 + ox, 620 + ox, fl.deep);
  if (wd) {
    const std::string word = wname + " ";
    const float ww = ui::bigWidth(word.c_str(), 300, ui::Face::Cond);
    ui::bigClip(620 + ox, true);
    for (int row = 0; row < 2; ++row) {  // the top row runs left, the bottom one right
      const float off = std::fmod(t / (row == 0 ? 34.0f : 44.0f), 1.0f) * 3.0f * ww, x0 = row == 0 ? -off : -3.0f * ww + off;
      for (int r = 0; r < 6; ++r)
        ui::bigHollow(word.c_str(), x0 + r * ww + ox, row == 0 ? 36.0f : 300.0f, 300, ui::Face::Cond, 2.0f, withAlpha(ui::kNight, 0.18f));
    }
    ui::bigClipOff();
  }
  ui::lean(618 + ox, 622 + ox, ui::kSilver);

  // His own victory clip, x4, played again every 3 s
  if (wd) {
    const float a = in(0.2f, 0.6f);
    int fn;
    float fw, fh, fps;
    int anim = art::kSheetVictory;
    if (!art::clipInfo(*wd, anim, fn, fw, fh, fps)) anim = art::kSheetIdle;
    if (art::clipInfo(*wd, anim, fn, fw, fh, fps) && a > 0.0f) {
      const float ct = anim == art::kSheetVictory ? std::fmod(std::max(0.0f, s - 0.3f), 3.0f) : t;
      const int frame = anim == art::kSheetVictory ? std::min(fn - 1, int(ct * fps)) : int(ct * fps) % std::max(1, fn);
      art::drawClipPx(*wd, anim, frame, 292.0f - fw * 2.0f, 680.0f - fh * 4.0f + 60.0f * (1.0f - a), 4.0f, false, a);
    }
  }

  // Who won, and how, on night plates
  if (wd) {
    const float u = in(0.62f, 0.4f, 0.2f, 0.8f, 0.2f), y = 452.0f + 12.0f * (1.0f - u);
    const bool team = winnerTeam_ >= 0;
    const std::string tag = team ? std::string("ÉQUIPE") : winnerCpu_ ? std::string("CPU") : std::string(TextFormat("J%d", winnerPort_ + 1));
    const std::string who = team ? upperFr(teamName(winnerTeam_)) : wname;
    const Color tc = team ? teamColor(winnerTeam_) : winnerCpu_ ? ui::kCpuTag : portColor(winnerPort_);
    const float tw = ui::capsWidth(tag.c_str(), 11, 0.08f) + 0.88f + 14.0f, nw = ui::capsWidth(who.c_str(), 18, 0.3f) + 5.4f;
    DrawRectangleRec({40, y, tw + 12 + nw + 12, 22}, withAlpha(ui::kNight, u));
    ui::caps(tag.c_str(), 47, y + 11, 11, 0.08f, withAlpha(tc, u));
    ui::caps(who.c_str(), 40 + tw + 12, y + 11, 18, 0.3f, withAlpha(ui::kSilver, u));
    std::string info = "N° " + wd->id.substr(1);
    if (team) info = wname + " · " + info;
    if (win) {
      info += " · " + (win->weapon >= 0 && win->weapon < int(wd->weapons.size()) ? wd->weapons[win->weapon].name : std::string("Mains nues"));
      if (win->stocks > 0) info += TextFormat(" · %d vie%s restante%s", win->stocks, win->stocks > 1 ? "s" : "", win->stocks > 1 ? "s" : "");
    }
    const float v = in(0.66f, 0.4f, 0.2f, 0.8f, 0.2f), iy = 484.0f + 12.0f * (1.0f - v);
    DrawRectangleRec({40, iy, textWidth(info.c_str(), 14, kB) + 18, 25}, withAlpha(ui::kNight, v));
    drawText(info.c_str(), 49, iy + 4, 14, withAlpha(WHITE, v), kB);
  }
  // VICTOIRE (ÉGALITÉ without a winner) on its plate, a brush stroke of his colour under it
  if (const float g = in(0.42f, 0.3f); g > 0.0f)
    ui::quad({36, 520}, {36 + 593 * g, 520}, {36 + 551 * g, 720}, {36, 720}, ui::kNight, ui::kNight, ui::kNight, ui::kNight);
  if (const float u = (s - 0.5f) / 0.5f; u > 0.0f) {
    const char* word = wd ? "VICTOIRE" : "ÉGALITÉ";
    const float sc = u < 0.35f ? 1.25f - 0.27f * ui::seg(u, 0.0f, 0.35f) : 0.98f + 0.02f * ui::seg(u, 0.35f, 1.0f);
    ui::scaled(sc, 52, 534 + 107.5f, [&] { ui::big(word, 52, 534, 160, ui::Face::Cond, withAlpha(WHITE, ui::seg(u, 0.0f, 0.35f))); });
  }
  if (s >= 0.85f) ui::swooshFrame(int((s - 0.85f) * 24.0f), {44, 634, 900 * 0.62f, 160 * 0.62f}, fl.fill);
  rlPopMatrix();

  // The ranking: place, face, tag, name and award, then K.O., falls, damage dealt and Clash Orbs broken
  auto slide = [&](float delay) { return in(delay, 0.4f); };
  {
    const float u = slide(0.5f), dx = 48.0f * (1.0f - u);
    ui::big("RÉSULTATS", 660 + dx, 44, 46, ui::Face::Cond, withAlpha(ui::kSilver, u));
    const char* rules = TextFormat("%s · %d vie%s · %d:%02d", resultStage_.c_str(), resultStocks_, resultStocks_ > 1 ? "s" : "",
                                   resultSeconds_ / 60, resultSeconds_ % 60);
    drawText(rules, 1244 - textWidth(rules, 12, kB) + dx, 56, 12, withAlpha(ui::kMist, u), kB);
  }
  const float colRight[4] = {340, 404, 500, 572};  // in a row: the right edges of K.O., falls, damage, orbs
  {
    const float u = slide(0.56f), dx = 48.0f * (1.0f - u), cy = 104 + 0.534f * 10;
    const Color lc = withAlpha(ui::kMist, u);
    ui::caps("PLACE", 660 + dx, cy, 10, 0.2f, lc);
    ui::caps("JOUEUR", 778 + dx, cy, 10, 0.2f, lc);
    const char* heads[4] = {"KO", "CHUTES", "DÉGÂTS", "ORBES"};
    const float rights[4] = {992, 1064, 1160, 1234};
    for (int c = 0; c < 4; ++c) ui::caps(heads[c], rights[c] - 2 - ui::capsWidth(heads[c], 10, 0.2f) + dx, cy, 10, 0.2f, lc);
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
    const float u = slide(0.62f + 0.06f * i), x = 660.0f + 48.0f * (1.0f - u), y = 124.0f + 80.0f * i;
    DrawRectangleLinesEx({x, y, 584, 72}, 1.0f, i == 0 ? withAlpha(ui::kSilver, 0.6f * u) : withAlpha(kRim, 0.16f * u));
    ui::exo(TextFormat("%d", i + 1), x + 12, y + 51.56f, 44, withAlpha(i == 0 ? ui::kSilver : ui::kMist, u));
    DrawRectangleRec({x + 56, y + 12, 48, 48}, withAlpha({14, 22, 64, 255}, u));
    face(d, x + 56, y + 12, u);
    // Tag and name; its first award under them (tied: everyone gets it)
    const char* award = maxKo > 0 && r.kos == maxKo     ? "Le plus de KO"
                        : maxOrb > 0 && r.orbs == maxOrb ? "Casseur d'orbe"
                        : i == bestHit                   ? TextFormat("Plus gros coup : %d %%", int(std::lround(r.bestHit)))
                                                         : nullptr;
    const char* tag = r.cpu ? "CPU" : TextFormat("J%d", r.port + 1);
    const float tw = ui::capsWidth(tag, 10, 0.08f) + 0.8f + 14.0f, cy = y + 23.08f;
    DrawRectangleRec({x + 118, cy - 9, tw, 18}, withAlpha(ui::kNight, u));
    ui::caps(tag, x + 125, cy, 10, 0.08f, withAlpha(rowColor(r.cpu, r.port, r.team), u));
    const std::string nick = upperFr(fiche(d).nick);
    float em = 24.0f;
    while (em > 14.0f && ui::bigWidth(nick.c_str(), em, ui::Face::Cond) > 296 - (118 + tw + 8)) em -= 0.5f;
    ui::big(nick.c_str(), x + 118 + tw + 8, cy - 0.42f * em, em, ui::Face::Cond, withAlpha(ui::kSilver, u));
    // Its award in Éclat, a CPU's level after it
    float ax = x + 118;
    if (award) ax += drawTextW(award, ax, y + 42, 12, withAlpha(ui::kBlue, u)) + (r.cpu ? 6.0f : 0.0f);
    if (r.cpu) drawTextW(TextFormat(award ? "· niveau %d" : "Niveau %d", r.level), ax, y + 42, 12, withAlpha(ui::kMist, u));
    const int vals[4] = {r.kos, r.falls, int(r.damage), r.orbs};
    for (int c = 0; c < 4; ++c) {
      const char* v = TextFormat("%d", vals[c]);
      ui::exo(v, x + colRight[c] - ui::exoWidth(v, 27), y + 46.27f, 27, withAlpha(i == 0 ? ui::kSilver : Color{201, 209, 230, 255}, u));
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
  const float base = rowsEnd(n);
  {
    const float u = slide(0.74f), dx = 48.0f * (1.0f - u);
    ui::caps("REVANCHE ?", 660 + dx, base + 22 + 0.534f * 11, 11, 0.2f, withAlpha(ui::kSilver, u));
    for (int i = 0; i < n; ++i) {
      const ResultRow& r = results_[i];
      Rectangle c = chipRect(i, n);
      c.x += dx;
      const bool on = r.cpu || resReady_[r.port & 3];
      const Color pc = rowColor(r.cpu, r.port, r.team);
      DrawRectangleLinesEx(c, 1.0f, on && !r.cpu ? withAlpha(pc, 0.7f * u) : withAlpha(kRim, 0.2f * u));
      const char* tag = r.cpu ? "CPU" : TextFormat("J%d", r.port + 1);
      const float cy = c.y + 14;
      float lx = c.x + 10 + ui::caps(tag, c.x + 10, cy, 10, 0.08f, withAlpha(r.cpu ? ui::kSilver : pc, u)) + 6;
      if (r.cpu) {
        drawText("Auto", lx, capsY(cy, 11, kB), 11, withAlpha(ui::kMist, u), kB);
      } else if (on) {
        drawText("Prêt", lx, capsY(cy, 11, kB), 11, withAlpha(ui::kSilver, u), kB);
        lx += textWidth("Prêt", 11, kB) + 6;
        const Color ck = withAlpha(ui::kSilver, u);
        DrawLineEx({lx, cy + 0.5f}, {lx + 3.5f, cy + 4}, 2.0f, ck);
        DrawLineEx({lx + 3.5f, cy + 4}, {lx + 10, cy - 4}, 2.0f, ck);
      } else {  // the key that makes it ready
        const int dev = slots_[r.port & 3].device;
        if (u < 0.95f) {}  // (the glyph has no fade: it comes once the line has landed)
        else if (lx + confirmWidth(dev, 18) <= c.x + c.width - 6) confirmGlyph(dev, lx, cy - 9, 18);
        else drawText("…", lx, capsY(cy, 11, kB), 11, withAlpha(ui::kMist, u), kB);
      }
    }
  }
  if (s >= 1.2f) {  // an Éclat sweep under REVANCHE
    const float p = std::fmod(s - 1.2f, 1.4f) / 1.4f;
    const float w = p < 0.55f ? 276.0f * ui::seg(p, 0.0f, 0.55f, 0.6f, 0.0f, 0.2f) : 276.0f * (1.0f - ui::seg(p, 0.56f, 1.0f, 0.6f, 0.0f, 0.2f));
    const Rectangle b0 = buttonRect(0, n);
    DrawRectangleRec({p < 0.55f ? b0.x : b0.x + 276.0f - w, b0.y + 60, w, 4}, ui::kBlue);
  }
  for (int b = 0; b < 4; ++b) {
    Rectangle r = buttonRect(b, n);
    const float u = slide(0.8f + 0.04f * b);
    r.x += 48.0f * (1.0f - u);
    const bool main = b == 0, sel = b == resSel_ && screenFrames_ > 40;
    if (main) {
      const Color c = withAlpha(ui::kSilver, u);
      ui::quad({r.x + 12, r.y}, {r.x + 288, r.y}, {r.x + 276, r.y + 56}, {r.x, r.y + 56}, c, c, c, c);
    } else {
      if (sel || ui::hovered(r)) DrawRectangleRec(r, withAlpha(ui::kSilver, 0.08f * u));
      DrawRectangleLinesEx(r, 1.0f, withAlpha(sel ? ui::kSilver : kRim, (sel ? 0.6f : 0.24f) * u));
    }
    const float ix = r.x + (main ? 20.0f : 10.0f);
    const Color tc = withAlpha(main ? ui::kNight : ui::kSilver, u);
    ui::icon(kButtonIcons[b], {ix + 18, r.y + 28}, 20, tc, 1.8f);
    ui::caps(kButtons[b], ix + 48, r.y + 28, 12, 0.12f, tc);
    if (main) {
      const char* cnt = all ? TextFormat("Départ dans %d s", std::max(1, (resGo_ + 59) / 60)) : TextFormat("%d / %d prêts", ready, n);
      drawText(cnt, r.x + 272 - textWidth(cnt, 12, kB), capsY(r.y + 28, 12, kB), 12, tc, kB);
    }
  }
}
