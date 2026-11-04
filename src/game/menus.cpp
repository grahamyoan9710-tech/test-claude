// Character select and stage select screens (« Ligne d'horizon »).
#include "game/app.hpp"
#include "game/fighter_art.hpp"
#include "game/fx.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
int cardCount(bool training) { return training ? 2 : 4; }
constexpr FontKind kT = FontKind::Title;

// The fighters stand big on the horizon on both sides of a "VS" (in a team battle, red on the left against blue on
// the right), the first of each side closest to the middle, each with its sheet under the line; the roster above
// the "VS", the rules under it. A free card waits as a small "join" chip at the bottom left.
float g_slotX[4] = {210, 1070, 210, 1070};  // set each frame by App::layoutCss
float g_slotW[4] = {380, 380, 380, 380};
int g_slotSide[4] = {0, 1, 0, 1};
bool g_slotShown[4] = {true, true, false, false};  // on the horizon (false: a join chip)
float g_chipX[4] = {};
constexpr float kHorizonY = 469, kSheetY = 484, kSheetH = 170, kChipW = 170;

Rectangle sheetRect(int col) {
  const float w = g_slotW[col] - 12.0f;
  return {g_slotX[col] - w * 0.5f, kSheetY, w, kSheetH};
}
bool wideSheet(int col) { return g_slotW[col] >= 300.0f; }  // alone on its side
// The rules under the "VS": lives, orb, weapons, mode.
Rectangle ruleRect(int i) { return {430.0f + (i % 2) * 213.0f, 496.0f + (i / 2) * 48.0f, 207, 40}; }
// Fields: the rules (row -1), then for each player the type (0), the fighter (1: the big picture), the device or
// level (2) and, in a team battle, the team chip (3). A free card has only its chip (row 0).
Rectangle fieldRect(int col, int row) {
  if (row < 0) return ruleRect(std::clamp(col, 0, 3));
  if (!g_slotShown[col]) return row == 0 ? Rectangle{g_chipX[col], kScreenH - 44.0f, kChipW, 30} : Rectangle{};
  const float w = g_slotW[col];
  const Rectangle sh = sheetRect(col);
  if (row == 1) return {g_slotX[col] - w * 0.5f + 6, 150, w - 12, kHorizonY - 150};
  if (row == 3) return {g_slotX[col] + w * 0.5f - 98, 156, 86, 26};
  if (wideSheet(col)) {  // the type and the device side by side, at the bottom of the sheet
    const float hw = sh.width * 0.5f - 3;
    return {row == 0 ? sh.x : sh.x + hw + 6, sh.y + sh.height - 30, hw, 30};
  }
  return {sh.x, sh.y + sh.height - (row == 0 ? 64.0f : 30.0f), sh.width, 30};  // one above the other
}
// The roster above the "VS": rows of 6, "?" (a fighter drawn at random) last.
Rectangle rosterRect(int i) {
  const int n = int(roster().size()) + 1;
  constexpr int perRow = 6;
  constexpr float s = 70, gap = 6;
  const int row = i / perRow, col = i % perRow, inRow = std::min(perRow, n - row * perRow);
  return {kScreenW * 0.5f - (inRow * s + (inRow - 1) * gap) * 0.5f + col * (s + gap), 76.0f + row * (s + gap), s, s};
}
// The character select's sky at p (drawn by drawCss): what shows through the cut corners of a tile.
constexpr Color kSkyTop{5, 10, 30, 255}, kSkyLow{10, 22, 56, 255};
constexpr Color kAzureLight{46, 104, 214, 255}, kAmberLight{224, 122, 31, 255};
constexpr Vector2 kLightL{200, 420}, kLightR{1080, 420};
constexpr float kLightR0 = 470;
Color cssSky(Vector2 p) {
  Color c = mixColor(kSkyTop, kSkyLow, std::clamp(p.y / 461.0f, 0.0f, 1.0f));
  auto light = [&](Vector2 at, Color col, float a) {  // ui::glow: two radial gradients
    const float d = std::hypot(p.x - at.x, p.y - at.y);
    c = mixColor(c, col, a * (0.55f * std::max(0.0f, 1.0f - d / kLightR0) + 0.45f * std::max(0.0f, 1.0f - d * 2.0f / kLightR0)));
  };
  light(kLightL, kAzureLight, 0.55f);
  light(kLightR, kAmberLight, 0.45f);
  return c;
}
ui::Hint cssGo(bool training) { return {"ENTRÉE", ui::Btn::Start, training ? "S'ENTRAÎNER !" : "COMBATTRE !"}; }

// Stage select: the arena fills the screen, its sheet and the plan of its platforms on the left, every arena below.
constexpr int kStageCols = 6;
// The last card, after every stage, is "?": an arena drawn at random.
// shortcut: one row of 6 cards fits above the footer; past 5 arenas, make the cards smaller or scroll them.
Rectangle stageCardRect(int i) {
  const int n = int(stageList().size()) + 1, row = i / kStageCols, col = i % kStageCols;
  const int inRow = std::min(kStageCols, n - row * kStageCols);
  constexpr float w = 189, h = 112, gap = 10;
  return {(kScreenW - (inRow * w + (inRow - 1) * gap)) * 0.5f + col * (w + gap), 548.0f + row * (h + 12), w, h};
}
ui::Hint stageGo(bool browse) { return {"ENTRÉE", ui::Btn::A, browse ? "S'ENTRAÎNER ICI" : "COMBATTRE ICI"}; }

// Input device selector: "MANETTE 2" with, under it, the model of the gamepad (or "non branchée").
void deviceSelector(Rectangle r, int device, bool focused, float t) {
  const bool plugged = deviceAvailable(device);
  const char* sub = !plugged ? "non branchée" : device >= kPad1 ? padModel(device - kPad1) : nullptr;
  ui::selector(r, sub ? "" : deviceName(device), focused, t);
  if (!sub) return;
  const float cx = r.x + r.width * 0.5f;
  drawTextCentered(deviceName(device), cx, r.y + 2, 15, ui::kCream, false);
  drawTextCentered(sub, cx, r.y + 19, 13, plugged ? ui::kMuted : Color{240, 150, 126, 255}, false);
}
}  // namespace

int App::freeDevice(int col) const {
  // A free input device for card col, a plugged-in one first.
  for (int pass = 0; pass < 2; ++pass)
    for (int d = 0; d < kDeviceCount; ++d) {
      bool used = pass == 0 && !deviceAvailable(d);
      for (int j = 0; j < 4; ++j)
        if (j != col && slots_[j].type == 1 && slots_[j].device == d) used = true;
      if (!used) return d;
    }
  return slots_[col].device;
}

void App::assignDevice(int col, int device) {
  const int old = slots_[col].type == 1 ? slots_[col].device : -1;
  slots_[col].type = 1;
  slots_[col].device = device;
  for (int j = 0; j < 4; ++j)
    if (j != col && slots_[j].type == 1 && slots_[j].device == device)
      slots_[j].device = old >= 0 && old != device ? old : freeDevice(j);
}

void App::changeField(int col, int row, int delta, int by) {
  audio_.play(Sfx::MenuMove);
  if (row < 0) {
    if (col == 0) settings().stocks = 1 + (settings().stocks - 1 + delta + 5) % 5;
    else if (col == 1) settings().items = !settings().items;
    else if (col == 2) settings().weapons = !settings().weapons;
    else {
      const bool on = !teamBattle();
      settings().teams = on;
      teamsForced_ = false;
      if (on) audio_.voice("teambattle");
    }
    saveSettings();
    return;
  }
  SlotSetup& s = slots_[col];
  if (row == 3) {  // the team chip: red <-> blue
    s.team = 1 - slotTeam(col);
    return;
  }
  if (training_) {
    if (row == 1) stepFighter(col, delta);
    if (row == 2 && col == 0) s.device = (s.device + delta + kDeviceCount) % kDeviceCount;
    return;
  }
  if (s.type == 0 && row > 0) row = 0;  // pressing on an empty card joins it
  switch (row) {
    case 0: {
      if (s.type == 1 && s.device >= kPad1 && s.device == by) padLeft_[s.device - kPad1] = true;  // it leaves
      s.type = (s.type + delta + 3) % 3;
      if (s.type == 2) s.level = settings().cpuLevel;
      if (s.type == 1) {
        bool byFree = by >= 0 && by < kDeviceCount;  // a device that plays nobody takes the card itself
        for (int j = 0; j < 4 && byFree; ++j)
          if (j != col && slots_[j].type == 1 && slots_[j].device == by) byFree = false;
        if (byFree && by >= kPad1) padLeft_[by - kPad1] = false;
        s.device = byFree ? by : freeDevice(col);
      }
      break;
    }
    case 1:
      stepFighter(col, delta);
      break;
    default:
      if (s.type == 1)  // another player on that device takes this one's: no two players on one device
        assignDevice(col, (s.device + delta + kDeviceCount) % kDeviceCount);
      else if (s.type == 2) s.level = std::clamp(s.level + delta, 1, kMaxCpuLevel);  // 1..10, no wrap
      break;
  }
}

// A gamepad that plays nobody joins with A or START: it takes the first card that is not a
// human player (player 1 in training, the dummy is never played).
bool App::padJoins() {
  for (int p = 0; p < kMaxPads; ++p) {
    if (padLeft_[p] || (!padPressed(p, kPadA) && !padPressed(p, kPadStart))) continue;
    const int dev = kPad1 + p;
    bool assigned = false;
    for (int i = 0; i < (training_ ? 1 : 4); ++i)
      assigned = assigned || ((training_ || slots_[i].type == 1) && slots_[i].device == dev);
    int col = training_ ? 0 : -1;
    for (int i = 0; i < 4 && col < 0; ++i)
      if (slots_[i].type != 1) col = i;
    if (assigned || col < 0) continue;
    assignDevice(col, dev);
    joinedAt_[col] = frames_;
    cssCur_[dev] = {true, col, 1, 0};  // the newcomer picks a fighter right away
    audio_.play(Sfx::MenuSelect);
    return true;
  }
  return false;
}

void App::layoutCss() const {
  const int cols = cardCount(training_);
  const bool teams = teamBattle();
  int count[2] = {0, 0}, rank[4] = {}, shown = 0;
  float chipX = 28;
  for (int i = 0; i < 4; ++i) {
    g_slotShown[i] = i < cols && (training_ || slots_[i].type != 0);
    if (!g_slotShown[i]) {
      g_chipX[i] = chipX;
      if (i < cols) chipX += kChipW + 8;
      continue;
    }
    g_slotSide[i] = teams ? slotTeam(i) : shown % 2;  // one on the left, the next on the right...
    ++shown;
    rank[i] = count[g_slotSide[i]]++;
  }
  for (int i = 0; i < 4; ++i) {
    if (!g_slotShown[i]) continue;
    const float w = 380.0f / std::max(1, count[g_slotSide[i]]);
    g_slotW[i] = w;
    g_slotX[i] = g_slotSide[i] == 0 ? 400.0f - (rank[i] + 0.5f) * w : 880.0f + (rank[i] + 0.5f) * w;
  }
}

int App::ownedCard(int device) const {
  if (training_) return slots_[0].device == device ? 0 : -1;
  for (int i = 0; i < 4; ++i)
    if (slots_[i].type == 1 && slots_[i].device == device) return i;
  return -1;
}

bool App::canPick(int col, int device) const {
  if (training_) return col == 1 || slots_[0].device == device;  // the dummy is anyone's
  const SlotSetup& s = slots_[col];
  return s.type == 2 || (s.type == 1 && s.device == device);  // a CPU, or your own fighter
}

void App::stepFighter(int col, int delta) {
  SlotSetup& s = slots_[col];
  const int n = int(roster().size());  // n: the "?" tile
  const int at = ((s.random ? n : s.character) + delta + (n + 1)) % (n + 1);
  s.random = at == n;
  if (!s.random) s.character = at;
}

void App::updateCss(const MenuNav& nav) {
  layoutCss();
  if (padJoins()) return;  // that press is used up
  const int cols = cardCount(training_);
  const int minRow = training_ ? 0 : -1;

  // Each device moves its own cursor. A keyboard that plays nobody while the other keyboard plays (one player
  // using either set of keys) drives that player's cursor; a gamepad that plays nobody joins with A (padJoins).
  for (int d = 0; d < kDeviceCount; ++d) {
    const MenuNav n = readMenuNavFor(d);
    if (!(n.up || n.down || n.left || n.right || n.confirm || n.prev)) continue;
    if (!training_ && d <= kKeyboard2 && ownedCard(d) < 0 && n.confirm) {
      // the other keyboard's attack key joins, like A on a gamepad: the first card nobody plays
      int col = -1;
      for (int i = 0; i < 4 && col < 0; ++i)
        if (slots_[i].type != 1) col = i;
      if (col >= 0) {
        assignDevice(col, d);
        joinedAt_[col] = frames_;
        cssCur_[d] = {true, col, 1, 0};
        audio_.play(Sfx::MenuSelect);
        continue;
      }
    }
    int e = d;
    if (ownedCard(d) < 0 && d <= kKeyboard2) {
      const int other = d == kKeyboard1 ? kKeyboard2 : kKeyboard1;
      if (ownedCard(other) >= 0) e = other;
    }
    if (e >= kPad1 && ownedCard(e) < 0) continue;
    CssCursor& c = cssCur_[e];
    if (!c.on) {  // the first press shows the cursor, on the device's own card
      c = {true, std::max(0, ownedCard(e)), 1, 0};
      audio_.play(Sfx::MenuMove, 0.6f);
      continue;
    }
    c.col = std::clamp(c.col, 0, cols - 1);
    // Rows from the top: the team chip (team battles), the fighter, the type, the device / level, the rules.
    const bool teamRow = teamBattle();
    const int order[5] = {3, 1, 0, 2, -1};
    auto valid = [&](int i) {  // a free card has only its chip (the type)
      return order[i] >= minRow && (teamRow || order[i] != 3) && (g_slotShown[c.col] || order[i] <= 0);
    };
    int at = 0;
    while (at < 5 && order[at] != c.row) ++at;
    if (at == 5 || !valid(at)) at = valid(1) ? 1 : 2;  // back on the fighter (a free card: its chip)
    bool moved = false;
    if (n.up) {
      int i = at - 1;
      while (i >= 0 && !valid(i)) --i;
      if (i >= 0) at = i, moved = true;
    }
    if (n.down) {
      int i = at + 1;
      while (i < 5 && !valid(i)) ++i;
      if (i < 5) at = i, moved = true;
    }
    c.row = order[at];
    const int dx = (n.right ? 1 : 0) - (n.left ? 1 : 0);
    // On your own card (or a CPU's), left / right change the fighter and the device / level; the top row (the
    // type) moves to another card.
    const bool pick = (c.row == 1 || c.row == 2) && canPick(c.col, e) && (c.row == 1 || !training_ || c.col == 0);
    if (dx != 0) {
      if (c.row < 0) c.rule = (c.rule + dx + 4) % 4, moved = true;
      else if (pick) changeField(c.col, c.row, dx, e);
      else c.col = (c.col + dx + cols) % cols, moved = true;
    }
    if (moved) audio_.play(Sfx::MenuMove, 0.6f);
    const int fieldCol = c.row < 0 ? c.rule : c.col;
    if (n.confirm) {
      if (pick && c.row == 1) {  // the fighter is chosen: the announcer calls the name
        audio_.play(Sfx::MenuSelect, 0.7f);
        const SlotSetup& sl = slots_[c.col];
        audio_.voice(sl.random ? "random" : voiceKey(roster()[sl.character].name).c_str());
      }
      else if (c.row != 1 || (!training_ && slots_[c.col].type == 0)) changeField(fieldCol, c.row, 1, e);
    }
    if (n.prev && (c.row != 1 || pick)) changeField(fieldCol, c.row, -1, e);
  }

  // Mouse: fields (the arrows of a fighter go back or forth), roster tiles, ready banner
  const int lastRow = teamBattle() ? 3 : 2;
  for (int row = minRow; row <= lastRow; ++row)
    for (int col = 0; col < (row < 0 ? 4 : cols); ++col) {
      const Rectangle f = fieldRect(col, row);
      if (!ui::hovered(f)) continue;
      cssRow_ = row;
      if (row < 0) rulesCol_ = col;
      else cssCol_ = col;
      int delta = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ? 1 : IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) ? -1 : 0;
      // The left arrow goes back: the left part of a fighter, the left half of a selector (type, level, rules).
      if (delta != 0 && row == 1 && GetMousePosition().x < f.x + 48) delta = -1;
      if (delta != 0 && row != 1 && row != 3 && GetMousePosition().x < f.x + f.width * 0.5f) delta = -1;
      if (delta != 0) changeField(col, row, delta);
    }
  const int nTiles = int(roster().size()) + 1;
  for (int i = 0; i < nTiles; ++i)
    if (ui::clicked(rosterRect(i))) {
      SlotSetup& s = slots_[cssCol_];
      if (!training_ && s.type == 0) {
        s.type = 1;
        s.device = freeDevice(cssCol_);
      }
      s.random = i == nTiles - 1;
      if (!s.random) s.character = i;
      audio_.play(Sfx::MenuSelect, 0.7f);
      audio_.voice(s.random ? "random" : voiceKey(roster()[i].name).c_str());
    }
  bool ready = readyToFight();
  bool clickGo = ready && ui::clicked(ui::goRect(cssGo(training_)));

  if (nav.start || clickGo) {
    if (ready) {
      audio_.play(Sfx::MenuSelect);
      goTo(Screen::StageSelect);
    } else {
      audio_.play(Sfx::MenuBack);
    }
  } else if (nav.back && !nav.prev) {
    audio_.play(Sfx::MenuBack);
    goTo(Screen::MainMenu);
  }
}

void App::drawCss() {
  const float t = time_;
  const auto& r = roster();
  layoutCss();
  const int cols = cardCount(training_);
  const bool teamsOn = teamBattle(), ready = readyToFight();
  const Settings& st = settings();
  auto tint = [&](int col) { return teamsOn ? teamColor(slotTeam(col)) : portColor(col); };
  auto whoOf = [&](int col) {
    return training_ ? (col == 0 ? "J1" : "CIBLE") : slots_[col].type == 2 ? "CPU" : TextFormat("J%d", col + 1);
  };

  // The night over the horizon, azure light on the left, amber on the right; the line, a glow under it.
  DrawRectangleGradientV(0, 0, kScreenW, 461, kSkyTop, kSkyLow);
  DrawRectangleGradientV(0, 461, kScreenW, 14, kSkyLow, ui::kInk);
  DrawRectangleGradientV(0, 475, kScreenW, kScreenH - 475, ui::kInk, {4, 7, 15, 255});
  ui::glow(kLightL, kLightR0, withAlpha(kAzureLight, 0.55f));
  ui::glow(kLightR, kLightR0, withAlpha(kAmberLight, 0.45f));
  ui::horizonLine(0, kScreenW, kHorizonY, 3.0f);
  DrawRectangleGradientV(0, int(kHorizonY) + 2, kScreenW, 80, withAlpha(WHITE, 0.06f), withAlpha(WHITE, 0.0f));
  // The number of each side's first fighter, huge and faint behind it
  for (int k = 0; k < 2; ++k)
    for (int col = 0; col < cols; ++col) {
      if (!g_slotShown[col] || g_slotSide[col] != k) continue;
      const SlotSetup& s = slots_[col];
      const std::string num = s.random ? std::string("?") : r[s.character].id.substr(1);
      const float size = 325.0f, w = textWidth(num.c_str(), size, kT);
      drawText(num.c_str(), k == 0 ? 30.0f : kScreenW - 30.0f - w, capsY(250, size, kT), size,
               withAlpha(tint(col), k == 0 ? 0.13f : 0.12f), kT);
      break;
    }
  const char* title = training_ ? "ENTRAÎNEMENT" : teamsOn ? "COMBAT EN ÉQUIPES" : "CHOIX DES COMBATTANTS";
  drawText(title, (kScreenW - textWidth(title, 35, kT)) * 0.5f, capsY(36, 35, kT), 35, ui::kCream, kT);

  // The bottom bar first: its shade must not darken the sheets above it
  const ui::Hint go = cssGo(training_);
  ui::footer({{"FLÈCHES", ui::Btn::Move, "Naviguer"}, {"ESPACE", ui::Btn::A, "Modifier"}, {"K", ui::Btn::X, "Précédent"},
              {"ÉCHAP", ui::Btn::B, "Retour"}},
             training_ ? "Vies infinies  ·  mannequin réglable en pause" : nullptr, &go, ready);

  // A field is focused when a player's cursor is on it, or under the mouse.
  auto cursorOn = [&](int col, int row) {
    for (int d = 0; d < kDeviceCount; ++d) {
      const CssCursor& c = cssCur_[d];
      if (c.on && c.row == row && (row < 0 ? c.rule : std::min(c.col, cols - 1)) == col) return true;
    }
    return false;
  };
  auto focusOn = [&](int col, int row) { return cursorOn(col, row) || ui::hovered(fieldRect(col, row)); };

  // The roster: each face on the night, its colour along the bottom, its number; who picked it in the corner.
  constexpr float cut = 8.0f;
  for (int i = 0; i <= int(r.size()); ++i) {
    const Rectangle rr = rosterRect(i);
    const bool rnd = i == int(r.size()), hov = ui::hovered(rr);
    int pickers[4], np = 0;
    for (int s = 0; s < cols; ++s) {
      const SlotSetup& sl = slots_[s];
      if (g_slotShown[s] && (rnd ? sl.random : !sl.random && sl.character == i)) pickers[np++] = s;
    }
    const Color bar = rnd ? Color{42, 53, 88, 255} : auraColor(r[i].hero);
    if (rnd) {
      ui::bevel(rr, cut, ui::kNavyDeep, ui::kNavyDeep);
      ui::bevelLines(ui::grow(rr, -0.5f), cut, 1.0f, withAlpha(ui::kSky, 0.3f));
      drawText("?", rr.x + (rr.width - textWidth("?", 37, kT)) * 0.5f, capsY(rr.y + 33, 37, kT), 37, ui::kSky, kT);
    } else {
      ui::bevel(rr, cut, ui::kNavy, mixColor(ui::kNavy, bar, 0.45f));
      drawPortraitIn(r[i], 0, {rr.x + 35, rr.y + 40}, 86.0f, rr, 0, t + i);
    }
    if (hov) DrawRectangleRec(rr, withAlpha(WHITE, 0.12f));
    DrawRectangleRec({rr.x, rr.y + rr.height - 4, rr.width, 4}, bar);
    const Vector2 tl{rr.x, rr.y}, br{rr.x + rr.width, rr.y + rr.height};  // the cut corners: the sky shows through
    DrawTriangle(tl, {tl.x, tl.y + cut}, {tl.x + cut, tl.y}, cssSky({tl.x + 2, tl.y + 2}));
    DrawTriangle(br, {br.x, br.y - cut}, {br.x - cut, br.y}, cssSky({br.x - 2, br.y - 2}));
    if (!rnd) ui::bold(r[i].id.substr(1).c_str(), rr.x + 6, capsY(rr.y + 11, 14, kT), 14);
    float tx = rr.x + rr.width;
    for (int k = 0; k < np; ++k) {
      const char* who = whoOf(pickers[k]);
      const float w = textWidth(who, 13, kT) + 10;
      tx -= w;
      DrawRectangleRec({tx, rr.y, w, 17}, tint(pickers[k]));
      drawText(who, tx + 5, capsY(rr.y + 8.5f, 13, kT), 13, ui::kInk, kT);
      tx -= 2;
    }
    if (np > 0) ui::bevelLines(ui::grow(rr, -1.5f), cut - 0.6f, 3.0f, tint(pickers[0]));
  }
  const Rectangle last = rosterRect(int(r.size()));
  drawTextCentered("← → ou un clic pour choisir  ·  « ? » : au hasard", kScreenW * 0.5f, last.y + last.height + 9, 16,
                   ui::kMuted, false);

  // "VS" on the horizon, its spark pulsing on the line
  const float pulse = 0.5f + 0.5f * std::sin(t * 2.0f * kPi / 2.2f);
  ui::eclat({kScreenW * 0.5f, kHorizonY}, 13.0f * (0.9f + 0.25f * pulse), 0.55f + 0.45f * pulse);
  const float vw = textWidth("VS", 69, kT);
  ui::gradientText("VS", kScreenW * 0.5f - vw * 0.5f, capsY(kHorizonY - 49, 69, kT), 69, {143, 193, 255, 255}, WHITE,
                   {255, 200, 112, 255});

  // An inert field: a dark plate, a word on it
  auto inert = [&](Rectangle f, const char* text, bool foc) {
    ui::bevel(f, 8, withAlpha(ui::kInk, 0.7f), withAlpha(ui::kInk, 0.7f));
    drawText(text, f.x + (f.width - textWidth(text, 15)) * 0.5f, capsY(f.y + f.height * 0.5f, 15, FontKind::Ui), 15, ui::kMuted);
    if (foc) ui::rim(f, 8, t);
  };
  // The players, big on the horizon, facing the other side; under the line, their sheets
  for (int col = 0; col < cols; ++col) {
    if (!g_slotShown[col]) continue;
    const SlotSetup& s = slots_[col];
    const int type = training_ ? (col == 0 ? 1 : 2) : s.type;
    const Color pc = tint(col), light = mixColor(pc, WHITE, 0.45f);
    const int facing = g_slotSide[col] == 0 ? 1 : -1;
    const Rectangle f1 = fieldRect(col, 1);
    const bool foc1 = focusOn(col, 1);
    const float cx = g_slotX[col], h = std::min(300.0f, g_slotW[col] * 1.05f);
    const float joined = std::max(0.0f, 1.0f - (frames_ - joinedAt_[col]) / 45.0f);  // a gamepad just joined
    ui::glow({cx, kHorizonY - h * 0.5f}, h * 0.7f, withAlpha(pc, 0.3f + 0.35f * joined));
    DrawEllipse(int(cx), int(kHorizonY + 1), h * 0.3f, 7.0f, withAlpha(BLACK, 0.5f));
    const CharacterDef* d = s.random ? nullptr : &r[s.character];
    if (!d) {  // "?": drawn at random when the match starts
      const float qp = 0.5f + 0.5f * std::sin(t * 3.0f + col), qs = h * 0.7f;
      drawText("?", cx - textWidth("?", qs, kT) * 0.5f, capsY(kHorizonY - h * 0.45f, qs, kT), qs, mixColor(pc, WHITE, qp), kT);
    } else {
      int pal = 0;
      for (int j = 0; j < col; ++j)
        if (g_slotShown[j] && !slots_[j].random && slots_[j].character == s.character) pal = 1 - pal;
      const float hop = foc1 ? std::max(0.0f, std::sin(t * 5.0f)) * 6.0f : 0.0f;
      drawShowcase(*d, pal, restPose(*d, t + col), {cx, kHorizonY - hop}, facing, h, t + col);
    }
    if (foc1) {  // the arrows: previous / next fighter
      ui::arrowTri({f1.x + 14, kHorizonY - h * 0.5f}, 22, -1, ui::kGold);
      ui::arrowTri({f1.x + f1.width - 14, kHorizonY - h * 0.5f}, 22, 1, ui::kGold);
    }
    if (teamsOn) {  // the team chip: press it to change sides
      const Rectangle chip = fieldRect(col, 3);
      const bool fc = focusOn(col, 3);
      const char* tn = teamName(slotTeam(col));
      ui::chunky(chip, fc ? mixColor(pc, WHITE, 0.3f) : pc, shade(pc, 0.7f), 1.0f, 8.0f);
      drawText(tn, chip.x + (chip.width - textWidth(tn, 15, kT)) * 0.5f, capsY(chip.y + chip.height * 0.5f, 15, kT), 15,
               ui::kInk, kT);
      if (fc) ui::rim(chip, 8.0f, t);
    }

    // The sheet: whose it is, the fighter's number, style, name and build, its two weapons, then the type and the
    // device or level
    const Rectangle sh = sheetRect(col);
    const bool wide = wideSheet(col);
    const char* who = whoOf(col);
    const float chipW = textWidth(who, 18, kT) + 18;
    DrawRectangleRec({sh.x, sh.y, chipW, 24}, pc);
    drawText(who, sh.x + 9, capsY(sh.y + 12, 18, kT), 18, ui::kInk, kT);
    const Fiche fi = d ? fiche(*d) : Fiche{"ALÉATOIRE", "", "", 0};
    const char* num = d ? d->name.c_str() : "N° ?";
    float x = sh.x + chipW + 10;
    drawText(num, x, capsY(sh.y + 12, 18, kT), 18, light, kT);
    x += textWidth(num, 18, kT) + 12;
    if (wide) {
      const std::string meta = !d ? std::string("Tiré au sort au début du combat")
                               : fi.style.empty() ? fi.art : fi.art + "  ·  " + fi.style;
      float ms = 16.0f;
      while (ms > 11.0f && textWidth(meta.c_str(), ms) > sh.x + sh.width - x) ms -= 0.5f;
      drawText(meta.c_str(), x, capsY(sh.y + 12, ms, FontKind::Ui), ms, ui::kMuted);
    }
    float ns = wide ? 42.0f : 30.0f;
    while (ns > 14.0f && textWidth(fi.nick.c_str(), ns, kT) > sh.width) ns -= 1.0f;
    drawText(fi.nick.c_str(), sh.x, capsY(sh.y + 46, ns, kT), ns, ui::kCream, kT);
    if (wide && d) {  // the build (five marks) and the transformation, if it has one
      const float gy = sh.y + 79;
      drawText("GABARIT", sh.x, capsY(gy, 14, FontKind::Ui), 14, ui::kMuted);
      float gx = sh.x + textWidth("GABARIT", 14) + 8;
      for (int g = 1; g <= 5; ++g, gx += 17) {
        const Color gc = g <= fi.build ? pc : ui::kTrack;
        ui::slanted({gx, gy - 4, 14, 8}, 3, gc, gc);
      }
      if (!d->forms.empty()) {
        const std::string tr = "TRANSFORMATION T" + d->id.substr(1);
        const float tw = textWidth(tr.c_str(), 13, kT) + 16, tx0 = gx + 12;
        DrawRectangleLinesEx({tx0, gy - 10, tw, 20}, 1.0f, light);
        drawText(tr.c_str(), tx0 + 8, capsY(gy, 13, kT), 13, ui::kCream, kT);
      }
    }
    if (d) {  // the two weapons that will fall on the arena
      const float wy = wide ? sh.y + 94 : sh.y + 60, wh = wide ? 42.0f : 36.0f, ww = (sh.width - 8) * 0.5f;
      for (int ws = 0; ws < 2; ++ws) {
        const Rectangle wr{sh.x + ws * (ww + 8), wy, ww, wh};
        ui::bevel(wr, 8, withAlpha(ui::kNavyDeep, 0.92f), withAlpha(ui::kNavyDeep, 0.92f));
        DrawRectangleRec({wr.x, wr.y + wr.height - 2, wr.width - 8, 2}, pc);
        drawWeaponIcon(*d, ws, {wr.x + 20, wr.y + wh * 0.5f}, wh - 8);
        if (wide) {
          drawText(ws == 0 ? "ARME A" : "ARME B", wr.x + 40, capsY(wr.y + 12, 13, FontKind::Ui), 13, ui::kMuted);
          const char* wn = d->weapons[ws].name.c_str();
          float fs = 16.0f;
          while (fs > 11.0f && textWidth(wn, fs) > wr.width - 46) fs -= 0.5f;
          drawText(wn, wr.x + 40, capsY(wr.y + 29, fs, FontKind::Ui), fs, ui::kCream);
        } else {
          drawText(ws == 0 ? "A" : "B", wr.x + 42, capsY(wr.y + wh * 0.5f, 16, kT), 16, ui::kMuted, kT);
        }
      }
    }
    const Rectangle f0 = fieldRect(col, 0), f2 = fieldRect(col, 2);
    const bool foc0 = focusOn(col, 0), foc2 = focusOn(col, 2);
    if (training_) inert(f0, col == 0 ? "VOUS" : "CIBLE", foc0);
    else ui::selector(f0, type == 1 ? "HUMAIN" : "ORDINATEUR", foc0, t);
    if (type == 1) deviceSelector(f2, s.device, foc2, t);
    else if (!training_) ui::selector(f2, s.level >= kMaxCpuLevel ? "NIV. 10 x2" : TextFormat("NIVEAU %d", s.level), foc2, t);
    else inert(f2, "réglable en pause", foc2);
  }

  // The free cards: a chip each, to join (or press it: a human, then a CPU)
  for (int col = 0; col < cols; ++col) {
    if (g_slotShown[col]) continue;
    const Rectangle f = fieldRect(col, 0);
    const Color pc = tint(col);
    ui::bevel(f, 6, withAlpha(ui::kNavyDeep, 0.85f), withAlpha(ui::kNavyDeep, 0.85f));
    DrawRectangleRec({f.x, f.y, 3, f.height}, pc);
    const char* who = TextFormat("J%d", col + 1);
    drawText(who, f.x + 12, capsY(f.y + f.height * 0.5f, 17, kT), 17, pc, kT);
    float px = f.x + 20 + textWidth(who, 17, kT);
    px += ui::prompt("ESPACE", ui::Btn::A, px, f.y + 5, f.height - 10) + 8;
    drawText("Rejoindre", px, capsY(f.y + f.height * 0.5f, 16, FontKind::Ui), 16, ui::kSky);
    if (focusOn(col, 0)) ui::rim(f, 6, t);
  }

  // The rules, under the "VS"; a line about them (or what is missing to fight)
  if (!training_) {
    auto rule = [&](int i, const char* label, bool on, bool line) {
      const Rectangle rr = ruleRect(i);
      const Color face = on ? ui::kNavy : Color{10, 16, 36, 255};
      ui::bevel(rr, 10, ui::hovered(rr) ? mixColor(face, WHITE, 0.08f) : face, face);
      if (line) DrawRectangleRec({rr.x, rr.y + rr.height - 2, rr.width - 10, 2}, ui::kGold);
      drawText(label, rr.x + (rr.width - textWidth(label, 19, kT)) * 0.5f, capsY(rr.y + rr.height * 0.5f, 19, kT), 19,
               on ? ui::kCream : ui::kDisabled, kT);
      if (focusOn(i, -1)) ui::rim(rr, 10, t);
    };
    rule(0, TextFormat("VIES : %d", st.stocks), true, false);
    rule(1, st.items ? "ORBE : OUI" : "ORBE : NON", st.items, st.items);
    rule(2, st.weapons ? "ARMES : OUI" : "ARMES : NON", st.weapons, st.weapons);
    rule(3, teamsOn ? "EN ÉQUIPES" : "CHACUN POUR SOI", true, false);
    bool cpu10 = false;
    for (int col = 0; col < cols; ++col) cpu10 = cpu10 || (g_slotShown[col] && slots_[col].type == 2 && slots_[col].level >= kMaxCpuLevel);
    const char* hint = !ready ? (activeSlots() >= 2 ? "Il faut un combattant dans chaque équipe." : "Il faut au moins 2 combattants.")
                       : !st.items ? "Sans orbe, personne ne pourra lancer son ultime."
                       : cpu10 ? "Niveau 10 : l'ordinateur frappe deux fois plus fort."
                       : st.weapons ? "Tout le monde commence à mains nues." : "Sans armes : tout le combat à mains nues.";
    drawTextCentered(hint, kScreenW * 0.5f, 597, 16, ready ? ui::kMuted : Color{255, 200, 112, 255}, false);
  }

  // Whose cursor is where: a tag in the player's colour on the corner of the field.
  std::array<int, 24> stacked{};
  for (int dev = 0; dev < kDeviceCount; ++dev) {
    const CssCursor& c = cssCur_[dev];
    if (!c.on) continue;
    const int own = ownedCard(dev);
    const int col = c.row < 0 ? c.rule : std::min(c.col, cols - 1);
    const int row = c.row < 0 || g_slotShown[col] ? c.row : 0;  // a free card: its chip
    const int slot = (row + 1) * 4 + col;
    const Rectangle f = fieldRect(col, row);
    const char* label = own >= 0 ? TextFormat("J%d", own + 1) : dev == kKeyboard1 ? "C1" : dev == kKeyboard2 ? "C2" : "M";
    const float w = textWidth(label, 14, kT) + 12;
    const Rectangle tag{f.x - 6 + stacked[slot] * (w + 4), f.y - 12, w, 20};
    ++stacked[slot];
    DrawRectangleRec(ui::grow(tag, 1.5f), ui::kInk);
    DrawRectangleRec(tag, own >= 0 ? tint(own) : ui::kSky);
    drawText(label, tag.x + 6, capsY(tag.y + 10, 14, kT), 14, ui::kInk, kT);
  }
}

// ---------------------------------------------------------------------------
// Stage select
// ---------------------------------------------------------------------------
void App::updateStage(const MenuNav& nav) {
  // n cards: every stage, then "?" (random)
  const int n = int(stageList().size()) + 1, rows = (n + kStageCols - 1) / kStageCols;
  int sel = stageRandom_ ? n - 1 : stage_;
  const int before = sel;
  if (nav.left) sel = (sel + n - 1) % n;
  if (nav.right) sel = (sel + 1) % n;
  if (nav.up || nav.down) {  // same column in the other row (the last row may be shorter)
    const int row = (sel / kStageCols + (nav.down ? 1 : rows - 1)) % rows;
    sel = std::min(n - 1, row * kStageCols + sel % kStageCols);
  }
  bool go = nav.confirm || nav.start || ui::clicked(ui::goRect(stageGo(stageBrowse_)));
  for (int i = 0; i < n; ++i) {
    if (!ui::hovered(stageCardRect(i))) continue;
    if (ui::mouseMoved()) sel = i;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      sel = i;
      go = true;
    }
  }
  stageRandom_ = sel == n - 1;
  if (!stageRandom_) stage_ = sel;
  if (sel != before) audio_.play(Sfx::MenuMove);
  if (go && stageBrowse_) {  // opened from the main menu: a training in that arena
    activateMenu(2, nav.device);  // ENTRAÎNEMENT
  } else if (go) {
    audio_.play(Sfx::MenuSelect);
    startMatch();
  } else if (nav.back) {
    audio_.play(Sfx::MenuBack);
    goTo(stageBrowse_ ? Screen::MainMenu : Screen::CharSelect);
  }
}

void App::drawStage() {
  const float t = time_;
  const auto& stages = stageList();
  const int n = int(stages.size());
  // "?": the arena behind flips through them all, one of them is drawn when the match starts
  const int shown = stageRandom_ ? int(t * 1.5f) % n : stage_;
  const StageDef& cur = stages[shown];
  // The arena itself fills the screen; night on the left under its sheet, and along the bottom under the cards.
  ui::stageBackdrop(shown, t, 0.0f);
  const Color night{4, 7, 18, 255};
  DrawRectangleGradientH(0, 0, 410, kScreenH, withAlpha(night, 0.94f), withAlpha(night, 0.8f));
  DrawRectangleGradientH(410, 0, 384, kScreenH, withAlpha(night, 0.8f), withAlpha(night, 0.15f));
  DrawRectangleGradientH(794, 0, kScreenW - 794, kScreenH, withAlpha(night, 0.15f), withAlpha(night, 0.25f));
  DrawRectangleGradientV(0, 460, kScreenW, 182, withAlpha(night, 0.0f), withAlpha(night, 0.95f));
  DrawRectangle(0, 642, kScreenW, kScreenH - 642, withAlpha(night, 0.95f));

  const char* title = stageBrowse_ ? "ARÈNES" : "CHOIX DE L'ARÈNE";
  drawText(title, 48, capsY(46, 35, kT), 35, ui::kCream, kT);
  drawText(stageRandom_ ? TextFormat("? / %d", n) : TextFormat("%d / %d", stage_ + 1, n), 48 + textWidth(title, 35, kT) + 14,
           capsY(48, 17, FontKind::Ui), 17, ui::kMuted);
  ui::quad({48, 75}, {306, 75}, {306, 78}, {48, 78}, ui::kBlue, WHITE, WHITE, ui::kBlue);
  ui::quad({306, 75}, {518, 75}, {518, 78}, {306, 78}, WHITE, withAlpha(ui::kGold, 0.0f), withAlpha(ui::kGold, 0.0f), WHITE);

  // Its sheet: where it comes from, its name, a line about it, the plan of its platforms.
  const float x = 48, w = 470;
  const std::string series = stageRandom_ ? std::string("TOUTES LES ARÈNES") : upperFr(cur.series);
  const char* name = stageRandom_ ? "Au hasard" : cur.name.c_str();
  drawText(series.c_str(), x, capsY(113, 15, kT), 15, ui::kGold, kT);
  float ns = 56.0f;
  while (ns > 24.0f && textWidth(name, ns, kT) > w) ns -= 1.0f;
  drawText(name, x, capsY(156, ns, kT), ns, ui::kCream, kT);
  float y = ui::drawWrapped(stageRandom_ ? std::string("Une des arènes, tirée au sort au début du combat.") : cur.blurb, x,
                            190, w, 21, ui::kSoft);
  drawText("PLATEFORMES", x, capsY(y + 12, 14, kT), 14, ui::kSky, kT);
  const Rectangle map{x, y + 26, 420, 170};
  ui::card(map, 1.0f, withAlpha(ui::kNavyDeep, 0.72f), withAlpha(ui::kNavyDeep, 0.72f));
  // The arena seen from far: x -700..700, y -320..140 (feet on the ground at 0)
  auto sx = [&](float wx) { return map.x + 10 + (wx + 700.0f) * 400.0f / 1400.0f; };
  auto sy = [&](float wy) { return map.y + 10 + (wy + 320.0f) * 150.0f / 460.0f; };
  if (stageRandom_) {
    drawText("?", map.x + (map.width - textWidth("?", 80, kT)) * 0.5f, capsY(map.y + map.height * 0.5f, 80, kT), 80,
             withAlpha(ui::kSky, 0.6f), kT);
  } else {
    DrawRectangleRec({map.x, sy(0), map.width, 1}, withAlpha(ui::kSky, 0.12f));
    for (const Platform& p : cur.platforms) {
      const Rect& b = p.base;
      if (p.solid) {  // a ground: azure, lit on top
        const Rectangle g{sx(b.x), sy(b.y), sx(b.x + b.w) - sx(b.x), b.h * 150.0f / 460.0f};
        DrawRectangleGradientV(int(g.x), int(g.y), int(g.width), int(g.height), withAlpha(ui::kBlue, 0.55f), withAlpha(ui::kBlue, 0.12f));
        DrawRectangleRec({g.x, g.y, g.width, 3}, ui::kCream);
        continue;
      }
      Vector2 off{0, 0};  // a thin platform: a gold line; a moving one swings over its dashed course
      if (p.period > 0) {
        const float ph = std::sin(t * 60.0f * 2.0f * kPi / p.period);
        off = {p.amplitude.x * ph, p.amplitude.y * ph};
        const float x0 = sx(b.x - std::fabs(p.amplitude.x)), x1 = sx(b.x + b.w + std::fabs(p.amplitude.x));
        for (float dx = x0; dx < x1; dx += 6) DrawRectangleRec({dx, sy(b.y) + 1, std::min(3.0f, x1 - dx), 1}, withAlpha(ui::kGold, 0.6f));
        ui::glow({sx(b.x + b.w * 0.5f + off.x), sy(b.y + off.y)}, 22, withAlpha(ui::kGold, 0.5f));
      }
      DrawRectangleRec({sx(b.x + off.x), sy(b.y + off.y), sx(b.x + b.w) - sx(b.x), 3}, ui::kGold);
    }
  }
  // The legend
  float lx = x;
  const float ly = map.y + map.height + 16;
  DrawRectangleGradientV(int(lx), int(ly - 4), 14, 8, withAlpha(ui::kBlue, 0.55f), withAlpha(ui::kBlue, 0.12f));
  DrawRectangleRec({lx, ly - 4, 14, 2}, ui::kCream);
  drawText("Sol", lx + 20, capsY(ly, 14, FontKind::Ui), 14, ui::kSky);
  lx += 20 + textWidth("Sol", 14) + 18;
  DrawRectangleRec({lx, ly - 1, 14, 3}, ui::kGold);
  drawText("Plateforme fine", lx + 20, capsY(ly, 14, FontKind::Ui), 14, ui::kSky);
  lx += 20 + textWidth("Plateforme fine", 14) + 18;
  for (int i = 0; i < 3; ++i) DrawRectangleRec({lx + i * 5.0f, ly, 3, 1}, ui::kGold);
  drawText("Mobile", lx + 20, capsY(ly, 14, FontKind::Ui), 14, ui::kSky);

  // Every arena: its still, its name at the bottom; the chosen one lifted, in gold.
  auto frame = [&](int i, const char* label, bool sel) {
    Rectangle rr = stageCardRect(i);
    if (sel) rr.y -= 6;
    constexpr float cut = 12.0f;
    if (i < n) {
      drawStageThumb(i, stages[i], rr, cut);
      if (!sel) ui::bevel(rr, cut, withAlpha(ui::kInk, 0.3f), withAlpha(ui::kInk, 0.3f));
    } else {
      ui::bevel(rr, cut, ui::kNavyDeep, ui::kNavyDeep);
      ui::bevelLines(ui::grow(rr, -0.5f), cut, 1.0f, withAlpha(ui::kSky, 0.3f));
      drawText("?", rr.x + (rr.width - textWidth("?", 48, kT)) * 0.5f, capsY(rr.y + rr.height * 0.42f, 48, kT), 48, ui::kSky, kT);
    }
    if (ui::hovered(rr)) ui::bevel(rr, cut, withAlpha(WHITE, 0.1f), withAlpha(WHITE, 0.1f));
    ui::quad({rr.x, rr.y + rr.height - 44}, {rr.x + rr.width, rr.y + rr.height - 44}, {rr.x + rr.width, rr.y + rr.height - cut},
             {rr.x, rr.y + rr.height - cut}, withAlpha(night, 0.0f), withAlpha(night, 0.0f), withAlpha(night, 0.85f),
             withAlpha(night, 0.85f));
    ui::quad({rr.x, rr.y + rr.height - cut}, {rr.x + rr.width, rr.y + rr.height - cut}, {rr.x + rr.width - cut, rr.y + rr.height},
             {rr.x, rr.y + rr.height}, withAlpha(night, 0.85f), withAlpha(night, 0.85f), withAlpha(night, 0.92f),
             withAlpha(night, 0.92f));  // along the cut corner
    float fs = 17.0f;
    while (fs > 11.0f && textWidth(label, fs, kT) > rr.width - 20) fs -= 0.5f;
    drawText(label, rr.x + 10, capsY(rr.y + rr.height - 15, fs, kT), fs, sel ? ui::kGold : ui::kCream, kT);
    if (sel) ui::bevelLines(ui::grow(rr, -1.5f), cut - 0.6f, 3.0f, ui::kGold);
  };
  for (int i = 0; i < n; ++i) frame(i, stages[i].name.c_str(), !stageRandom_ && i == stage_);
  frame(n, "Au hasard", stageRandom_);
  const ui::Hint go = stageGo(stageBrowse_);
  ui::footer({{"FLÈCHES", ui::Btn::Move, "Choisir"}, {"ÉCHAP", ui::Btn::B, "Retour"}}, nullptr, &go);
}
