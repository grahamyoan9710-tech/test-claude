// Character select (« Prestige ») and stage select screens.
#include <array>
#include <string>
#include <vector>

#include "game/app.hpp"
#include "game/fighter_art.hpp"
#include "game/fx.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"
#include "rlgl.h"

namespace {
int cardCount(bool training) { return training ? 2 : 4; }
constexpr FontKind kB = FontKind::Body;

// « Prestige »: the select as a match poster. Two players: each one's fighter in its colour on its side, cut by two
// parallel blades leaning 12°, in its own pixel art (x4), its name on a night plate; between the blades the roster,
// the rules, the head to head and FIGHT. Three or four: four cards leaning the same way under the roster. Who plays
// (a human or the computer) and the rules are on the « Règles & mode » drawer, behind its button.
Rectangle g_panel[4];     // each card's area, set each frame by App::layoutCss (width 0: not on screen)
Rectangle g_chip[4];      // its player plate (width 0: a free card, its whole area joins it)
int g_side[4];            // two players: the side of each card (0 left, 1 right, -1 not shown)
Rectangle g_rulesBtn;     // the « Règles & mode » button
bool g_wide = true;       // two sides (else four cards)
float g_tagW = 40.0f;     // the drawer's player tags
std::string g_rulesLine;  // what the button says of the rules
constexpr float kTile = 80, kTileH = 96, kTileGap = 8;

// The middle of the column between the blades at height y: it leans with them.
float midX(float y) { return 716.0f - ui::kLean * y; }
// Four cards: card i's left edge passes through (cardX(i), 0); its colour down to kCardInfo, its plate under it.
constexpr float kCardY0 = 206, kCardInfo = 524, kCardY1 = 640, kCardW = 272;
float cardX(int i) { return 151.0f + i * 296.0f; }
float cardLeft(int i, float y) { return cardX(i) - ui::kLean * y; }

// Roster tile i; "?" (a fighter at random) last. Two players: rows of 4 between the blades; four: one row on top.
// shortcut: 11 fighters fit between the blades and 13 in the top row; past that, smaller tiles.
Rectangle rosterRect(int i) {
  const int n = int(roster().size()) + 1;
  if (!g_wide) {
    const float x0 = (kScreenW - (n * kTile + (n - 1) * kTileGap)) * 0.5f;
    return {x0 + i * (kTile + kTileGap), 82, kTile, kTileH};
  }
  const int row = i / 4, inRow = std::min(4, n - row * 4);
  const float y = 134.0f + row * 104.0f, x0 = midX(y + kTileH * 0.5f) - (inRow * kTile + (inRow - 1) * kTileGap) * 0.5f;
  return {x0 + (i % 4) * (kTile + kTileGap), y, kTile, kTileH};
}
// FIGHT: a white plate leaning like the blade, on the horizon (two players) or bottom right (four).
Rectangle goRect() { return g_wide ? Rectangle{midX(594) - 150, 566, 300, 56} : Rectangle{kScreenW - 336, 648, 300, 56}; }
// The arrows on both sides of a fighter when it is focused: the one before (-1), after (1).
Rectangle chevRect(int col, int dir) {
  if (!g_wide) {
    const float l = cardLeft(col, 360);
    return {dir < 0 ? l + 10 : l + kCardW - 42, 344, 32, 32};
  }
  const bool L = g_side[col] == 0;
  return {L ? (dir < 0 ? 16.0f : 376.0f) : (dir < 0 ? kScreenW - 416.0f : kScreenW - 56.0f), 360, 40, 40};
}
// Fields on the screen: the button (row -1); each card's fighter (1) and its plate (0: who plays it; a free card's
// whole area).
Rectangle fieldRect(int col, int row) {
  if (row < 0) return g_rulesBtn;
  return row == 0 && g_chip[col].width > 0.0f ? g_chip[col] : g_panel[col];
}

// The « Règles & mode » drawer, on the right, its edge cut by the blade: the mode, the rules, then who plays.
constexpr float kSheetX = 810, kSX = 840, kSW = 404;
bool inSheet(Vector2 p) { return p.x > kSheetX - ui::kLean * p.y; }
Rectangle closeRect() { return {1204, 36, 40, 40}; }
Rectangle modeRect() { return {kSX, 150, kSW, 48}; }
Rectangle modeSeg(int k) { return {kSX + k * (kSW + 8) * 0.5f, 150, (kSW - 8) * 0.5f, 48}; }
Rectangle ruleRowRect(int i) { return {kSX, 238 + i * 50.0f, kSW, 48}; }  // lives, orb, weapons
Rectangle playerRowRect(int col) { return {kSX, 424 + col * 48.0f, kSW, 44}; }
Rectangle validateRect() { return {kSX, 650, kSW, 52}; }
// A player's type toggle: HUMAIN, IA, nobody (a dash), and the type each part sets.
constexpr const char* kSegLabel[2] = {"HUMAIN", "IA"};
constexpr int kSegType[3] = {1, 2, 0};
float segW(int k) { return k < 2 ? 20.0f + ui::capsWidth(kSegLabel[k], 10, 0.1f) : 34.0f; }
// Field (col, row) of the drawer: the mode (col 3) and the rules (lives, orb, weapons) on row -1; a player's type (0),
// device or level (2) and team (3).
Rectangle sheetField(int col, int row, bool teams) {
  if (row < 0) return col == 3 ? modeRect() : ruleRowRect(col);
  const Rectangle r = playerRowRect(col);
  const float y = r.y + r.height * 0.5f - 17, tx = r.x + g_tagW + 10, tw = segW(0) + segW(1) + segW(2) + 2;
  if (row == 0) return {tx, y, tw, 34};
  if (row == 3) return {r.x + r.width - 64, y, 64, 34};
  return {tx + tw + 10, y, r.x + r.width - (teams ? 72.0f : 0.0f) - (tx + tw + 10), 34};
}
Rectangle toggleSeg(Rectangle f, int k) {
  float x = f.x + 1;
  for (int j = 0; j < k; ++j) x += segW(j);
  return {x, f.y + 1, segW(k), f.height - 2};
}
// Where a click splits a field into "less" (left) and "more": a rule's value sits on the right of its row.
float fieldSplit(Rectangle r, int col, int row) { return row < 0 && col < 3 ? r.x + r.width - 63 : r.x + r.width * 0.5f; }
struct SheetItem {
  int col, row;
};
// The drawer's fields in keyboard / gamepad order. Training: player 1's device only (the dummy is set in the pause).
int sheetItems(bool training, const std::array<SlotSetup, 4>& s, bool teams, SheetItem* out) {
  int n = 0;
  if (training) {
    out[n++] = {0, 2};
    return n;
  }
  for (int c : {3, 0, 1, 2}) out[n++] = {c, -1};
  for (int c = 0; c < 4; ++c) {
    out[n++] = {c, 0};
    if (s[c].type != 0) out[n++] = {c, 2};
    if (s[c].type != 0 && teams) out[n++] = {c, 3};
  }
  return n;
}

// A fighter's five bars, 1 to 10, on fixed ranges around the roster's own: power (each move's hardest hit, on
// average), speed (running), weight, jump (height of a full jump), air (drift).
constexpr const char* kStatName[5][2] = {
    {"PUISSANCE", "PUI"}, {"VITESSE", "VIT"}, {"POIDS", "POI"}, {"SAUT", "SAU"}, {"AÉRIEN", "AIR"}};
std::array<int, 5> statBars(const CharacterDef& d) {
  float sum = 0.0f;
  int n = 0;
  for (int m = Jab; m <= DSpecial; ++m) {
    float best = 0.0f;
    for (const HitboxDef& h : d.moves[m].hitboxes) best = std::max(best, h.damage);
    if (best > 0.0f) sum += best, ++n;
  }
  auto bar = [](float v, float lo, float hi) {
    return std::clamp(int(std::lround(1.0f + 9.0f * (v - lo) / (hi - lo))), 1, 10);
  };
  return {bar(n > 0 ? sum / n : 0.0f, 8.5f, 12.0f), bar(d.runSpeed, 7.5f, 10.5f), bar(d.weight, 75.0f, 125.0f),
          bar(d.jumpVel * d.jumpVel / (2.0f * d.gravity), 170.0f, 215.0f), bar(d.airSpeed, 4.5f, 6.0f)};
}

const char* whoOf(bool training, const SlotSetup& s, int col) {
  return training ? (col == 0 ? "J1" : "CIBLE") : s.type == 2 ? "CPU" : TextFormat("J%d", col + 1);
}
std::string howOf(bool training, const SlotSetup& s, int col) {
  if (training && col == 1) return "MANNEQUIN · RÉGLABLE EN PAUSE";
  if (!training && s.type == 2) return s.level >= kMaxCpuLevel ? "IA · NIVEAU 10 X2" : "IA · NIVEAU " + std::to_string(s.level);
  return upperFr(deviceName(s.device));
}
// A player plate's text sizes: wide (two players) or in a card.
float chipWidth(const char* who, const std::string& how, bool wide) {
  return wide ? 30.0f + ui::capsWidth(who, 12, 0.08f) + 0.96f + ui::capsWidth(how.c_str(), 11, 0.2f) + 2.2f
              : 28.0f + ui::capsWidth(who, 11, 0.08f) + ui::capsWidth(how.c_str(), 10, 0.16f);
}

// A roster tile's face: the pixel portrait (his framed larger: his portrait leaves room around him).
void tileFace(const CharacterDef& d, Rectangle r, float a) {
  const std::string pic = d.id + "_portrait";
  float pw, ph;
  if (!art::pictureSize(pic.c_str(), pw, ph)) return;
  const bool roomy = d.id == "h01";
  const float w = roomy ? 150.0f : 110.0f;
  beginClip(r);
  art::drawPicture(pic.c_str(), {0, 0, pw, ph}, {r.x + (roomy ? -16.0f : -14.0f), r.y + (roomy ? -20.0f : -8.0f), w, ph * w / pw}, a);
  endClip();
}
std::string firstWord(const std::string& s) { return s.substr(0, s.find(' ')); }

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
    cssCur_[dev] = {true, col, 1};  // the newcomer picks a fighter right away
    audio_.play(Sfx::MenuSelect);
    return true;
  }
  return false;
}

void App::layoutCss() const {
  const int cols = cardCount(training_);
  int joined[4], nj = 0;
  for (int i = 0; i < cols; ++i)
    if (training_ || slots_[i].type != 0) joined[nj++] = i;
  g_wide = nj <= 2;
  g_tagW = 18.0f + ui::capsWidth(training_ ? "CIBLE" : "J4", 12, 0.08f);
  for (int i = 0; i < 4; ++i) g_panel[i] = {}, g_chip[i] = {}, g_side[i] = -1;
  if (g_wide) {  // the players, then free cards: two sides
    int order[2], k = 0;
    for (int j = 0; j < nj; ++j) order[k++] = joined[j];
    for (int i = 0; i < cols && k < 2; ++i)
      if (!training_ && slots_[i].type == 0) order[k++] = i;
    for (int j = 0; j < k; ++j) g_side[order[j]] = j, g_panel[order[j]] = {j == 0 ? 0.0f : 850.0f, 0, 430, 720};
  } else {
    for (int i = 0; i < cols; ++i) g_panel[i] = {cardLeft(i, 423), kCardY0, kCardW, kCardY1 - kCardY0};
  }
  for (int i = 0; i < 4; ++i) {
    if (g_panel[i].width <= 0.0f || !(training_ || slots_[i].type != 0)) continue;
    const float w = chipWidth(whoOf(training_, slots_[i], i), howOf(training_, slots_[i], i), g_wide);
    if (g_wide) g_chip[i] = {g_side[i] == 0 ? 36.0f : kScreenW - 36.0f - w, 30, w, 28};
    else g_chip[i] = {cardLeft(i, 231) + 12, 218, std::min(w, kCardW - 24), 26};
  }
  const Settings& st = settings();
  g_rulesLine = training_ ? std::string("Entraînement · ") + deviceName(slots_[0].device)
                          : std::string(teamBattle() ? "En équipes" : "Chacun pour soi") + " · " +
                                std::to_string(st.stocks) + (st.stocks > 1 ? " vies" : " vie") +
                                (st.items ? " · Orbe" : " · Sans orbe") + (st.weapons ? " · Armes" : " · Sans armes");
  const float tw = std::max(ui::capsWidth("RÈGLES & MODE", 12, 0.14f) + 1.68f, textWidth(g_rulesLine.c_str(), 11.5f, kB));
  const float w = std::max(316.0f, 51.0f + tw + 11.0f + ui::pkeyWidth("R", ui::Btn::Y, 22) + 10.0f);
  g_rulesBtn = g_wide ? Rectangle{midX(90) - w * 0.5f, 66, w, 48} : Rectangle{kScreenW - 36 - w, 22, w, 48};
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
  bool sheetKey = IsKeyPressed(KEY_R);  // the « Règles & mode » sheet: R, Y on any gamepad, or its button
  for (int p = 0; p < kMaxPads; ++p) sheetKey = sheetKey || padPressed(p, kPadY);

  if (rulesOpen_) {  // the sheet takes every device: up / down a field, left / right (or A, X) its value
    SheetItem items[16];
    const bool teams = teamBattle();
    const int n = sheetItems(training_, slots_, teams, items);
    rulesSel_ = std::clamp(rulesSel_, 0, n - 1);
    const bool outside = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !inSheet(GetMousePosition());
    if (sheetKey || nav.start || (nav.back && !nav.prev) || ui::clicked(closeRect()) || ui::clicked(validateRect()) ||
        outside) {
      rulesOpen_ = false;
      audio_.play(Sfx::MenuBack);
      return;
    }
    if (nav.up || nav.down) {
      rulesSel_ = (rulesSel_ + (nav.down ? 1 : n - 1)) % n;
      audio_.play(Sfx::MenuMove, 0.6f);
    }
    const SheetItem f = items[rulesSel_];
    const int dx = (nav.right ? 1 : 0) - (nav.left ? 1 : 0);
    if (f.row < 0 && f.col == 3) {  // the mode: left, each for themselves; right, teams
      if ((dx < 0 && teams) || (dx > 0 && !teams) || nav.confirm) changeField(3, -1, 1, nav.device);
    } else {
      if (dx != 0) changeField(f.col, f.row, dx, nav.device);
      if (nav.confirm) changeField(f.col, f.row, 1, nav.device);
      if (nav.prev) changeField(f.col, f.row, -1, nav.device);
    }
    // The mouse: the field under it is focused; a click on its left half goes back (the toggle: the part clicked)
    const bool lc = IsMouseButtonPressed(MOUSE_BUTTON_LEFT), rc = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    for (int i = 0; i < n; ++i) {
      const SheetItem& it = items[i];
      const Rectangle r = sheetField(it.col, it.row, teams);
      if (!ui::hovered(r)) continue;
      if (ui::mouseMoved() || lc || rc) rulesSel_ = i;
      if (!lc && !rc) break;
      const bool leftHalf = GetMousePosition().x < fieldSplit(r, it.col, it.row);
      const int type = slots_[it.col].type;
      if (it.row < 0 && it.col == 3) {
        if (leftHalf == teams) changeField(3, -1, 1);
      } else if (it.row == 0 && lc) {
        for (int k = 0; k < 3; ++k)
          if (ui::hovered(toggleSeg(r, k)) && kSegType[k] != type)
            changeField(it.col, 0, (kSegType[k] - type + 3) % 3 == 1 ? 1 : -1);
      } else {
        changeField(it.col, it.row, rc || (leftHalf && it.row != 3) ? -1 : 1);
      }
      break;
    }
    return;
  }
  if (sheetKey || ui::clicked(g_rulesBtn)) {
    rulesOpen_ = true;
    rulesSel_ = 0;
    audio_.play(Sfx::MenuSelect);
    return;
  }
  if (padJoins()) return;  // that press is used up
  const int cols = cardCount(training_);
  auto shown = [&](int col) { return g_panel[col].width > 0.0f; };
  auto nextCard = [&](int col, int dir) {  // the next card on screen that way
    for (int k = 1; k <= cols; ++k) {
      const int c = ((col + dir * k) % cols + cols) % cols;
      if (shown(c)) return c;
    }
    return col;
  };

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
        cssCur_[d] = {true, col, 1};
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
      c = {true, std::max(0, ownedCard(e)), 1};
      audio_.play(Sfx::MenuMove, 0.6f);
      continue;
    }
    c.col = std::clamp(c.col, 0, cols - 1);
    if (!shown(c.col)) c.col = nextCard(c.col, 1);
    // Rows from the top: the « Règles & mode » button, the fighter (its sheet), the player chip; a free card's
    // sheet is its chip.
    const bool joined = g_chip[c.col].width > 0.0f;
    if (c.row == 1 && !joined) c.row = 0;
    bool moved = false;
    if (n.up && c.row != -1) c.row = c.row == 0 && joined ? 1 : -1, moved = true;
    else if (n.down && c.row != 0) c.row = c.row == -1 && joined ? 1 : 0, moved = true;
    const int dx = (n.right ? 1 : 0) - (n.left ? 1 : 0);
    // On your own fighter (or a CPU's), left / right change it; on a chip they move to another card.
    const bool pick = c.row == 1 && canPick(c.col, e);
    if (dx != 0 && c.row >= 0) {
      if (pick) changeField(c.col, 1, dx, e);
      else c.col = nextCard(c.col, dx), moved = true;
    }
    if (moved) audio_.play(Sfx::MenuMove, 0.6f);
    if (n.confirm) {
      if (c.row < 0) {
        rulesOpen_ = true;
        rulesSel_ = 0;
        audio_.play(Sfx::MenuSelect);
        return;
      }
      if (pick) {  // the fighter is chosen: the announcer calls the name
        audio_.play(Sfx::MenuSelect, 0.7f);
        const SlotSetup& sl = slots_[c.col];
        audio_.voice(sl.random ? "random" : voiceKey(roster()[sl.character].name).c_str());
      } else if (c.row == 0) {
        changeField(c.col, 0, 1, e);
      }
    }
    if (n.prev && (c.row == 0 || pick)) changeField(c.col, c.row, -1, e);
  }

  // Mouse: a card's area picks the card the roster tiles go to (its arrows: the fighter before / after); a plate changes who
  // plays it; a free sheet joins it (a right click: a CPU).
  const bool lc = IsMouseButtonPressed(MOUSE_BUTTON_LEFT), rc = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
  for (int col = 0; col < cols; ++col) {
    const Rectangle p = g_panel[col], chip = g_chip[col];
    if (!shown(col) || !ui::hovered(p)) continue;
    cssCol_ = col;
    cssRow_ = 1;
    if (!lc && !rc) continue;
    if (chip.width <= 0.0f || ui::hovered(chip)) {
      changeField(col, 0, lc ? 1 : -1);
    } else {
      const int step = ui::hovered(chevRect(col, -1)) ? -1 : ui::hovered(chevRect(col, 1)) ? 1 : 0;
      if (step != 0) changeField(col, 1, step);
    }
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
  bool clickGo = ready && ui::clicked(goRect());

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
  const float t = time_, sec = screenFrames_ / 60.0f;
  const auto& r = roster();
  layoutCss();
  const int cols = cardCount(training_);
  const bool teamsOn = teamBattle(), ready = readyToFight();
  const Settings& st = settings();
  auto in = [&](float s, float delay, float len, float x1 = 0.2f, float y1 = 0.9f, float x2 = 0.1f) {
    return ui::seg(s, delay, delay + len, x1, y1, x2, 1.0f);
  };
  auto up = [&](float delay) { return in(sec, delay, 0.4f, 0.2f, 0.8f, 0.2f); };  // rising 12 px as it fades in
  auto tint = [&](int col) { return teamsOn ? teamColor(slotTeam(col)) : portColor(col); };
  // a player's mark: its colour; the computer's grey (in a team battle, its team's)
  auto mark = [&](int col) { return !teamsOn && !training_ && slots_[col].type == 2 ? ui::kCpuTag : tint(col); };
  auto joined = [&](int col) { return g_chip[col].width > 0.0f; };
  auto fighterOf = [&](int col) -> const CharacterDef* {
    return joined(col) && !slots_[col].random ? &r[slots_[col].character] : nullptr;
  };
  auto floodOf = [&](int col) -> ui::Flood {
    if (!joined(col)) return {ui::kNightUp, {14, 21, 56, 255}};
    if (slots_[col].random) return {ui::kDim, {58, 68, 99, 255}};
    return ui::flood(r[slots_[col].character].hero);
  };
  auto ring = [&](Rectangle f) { DrawRectangleLinesEx(ui::grow(f, 3.0f), 2.0f, ui::kBlue); };  // the focus
  // A key cap in night ink on a white plate
  auto inkKey = [&](const char* key, ui::Btn pad, float x, float y, float a) {
    if (padPrompts()) return ui::pkey(key, pad, x, y, 22, false, a);
    const float w = ui::pkeyWidth(key, pad, 22);
    DrawRectangleLinesEx({x, y, w, 22}, 1.0f, withAlpha(ui::kNight, 0.4f * a));
    ui::caps(key, x + (w - ui::capsWidth(key, 8.8f, 0.1f)) * 0.5f, y + 11, 8.8f, 0.1f, withAlpha(ui::kNight, a));
    return w;
  };
  // A fighter that changes comes in again
  float since[4];
  for (int col = 0; col < 4; ++col) {
    const int pick = !joined(col) ? -2 : slots_[col].random ? -1 : slots_[col].character;
    if (pick != pickSeen_[col]) pickSeen_[col] = pick, pickAt_[col] = screenFrames_;
    since[col] = pickAt_[col] <= screenFrames_ ? (screenFrames_ - pickAt_[col]) / 60.0f : 9.0f;
  }
  // A field is focused when a player's cursor is on it, or under the mouse.
  auto cursorOn = [&](int col, int row) {
    for (const CssCursor& c : cssCur_)
      if (c.on && c.row == row && (row < 0 || c.col == col)) return true;
    return false;
  };
  auto focusOn = [&](int col, int row) { return cursorOn(col, row) || ui::hovered(fieldRect(col, row)); };
  Vector2 tagAt[4][2] = {};  // where the cursors' tags go on each card's plate (0) and fighter (1)
  bool stackLeft[4][2] = {};  // ... stacked to the left of it

  // What is missing, or a tip
  bool cpu10 = false;
  for (int col = 0; col < cols; ++col) cpu10 = cpu10 || (slots_[col].type == 2 && slots_[col].level >= kMaxCpuLevel);
  const char* tip = training_ ? "Vies infinies · mannequin réglable en pause"
                    : !ready ? (activeSlots() >= 2 ? "Il faut un combattant dans chaque équipe." : "Il faut au moins 2 combattants.")
                    : !st.items ? "Sans orbe, personne ne pourra lancer son ultime."
                    : cpu10 ? "Niveau 10 : l'ordinateur frappe deux fois plus fort."
                    : st.weapons ? "Tout le monde commence à mains nues." : "Sans armes : tout le combat à mains nues.";

  DrawRectangle(0, 0, kScreenW, kScreenH, ui::kNight);
  int n;
  float fw, fh, fps;
  if (g_wide) {
    // ---- Two sides: each fighter's colour slides in, cut by its blade -------------------------------------------
    for (int col = 0; col < cols; ++col) {
      if (g_side[col] < 0) continue;
      const bool L = g_side[col] == 0, free = !joined(col);
      const CharacterDef* d = fighterOf(col);
      const ui::Flood fl = floodOf(col);
      const float ox = (L ? -560.0f : 560.0f) * (1.0f - in(sec, 0.0f, 0.45f));
      if (L) {
        ui::lean(-400 + ox, 400 + ox, fl.fill);
        ui::lean(400 + ox, 520 + ox, fl.deep);
      } else {
        ui::lean(1032 + ox, 1700 + ox, fl.fill);
        ui::lean(912 + ox, 1032 + ox, fl.deep);
      }
      // its name scrolling in it, tone on tone (the right one the other way, slower)
      const std::string word = (free ? std::string(TextFormat("J%d", col + 1)) : d ? upperFr(fiche(*d).nick) : std::string("HASARD")) + " ";
      const float ww = ui::bigWidth(word.c_str(), 300, ui::Face::Cond), off = std::fmod(t * 3.0f * ww / (L ? 36.0f : 44.0f), 3.0f * ww);
      ui::bigClip((L ? 520.0f : 912.0f) + ox, L);
      for (int k = 0, copies = int(kScreenW / ww) + 5; k < copies; ++k)
        ui::bigHollow(word.c_str(), (L ? -off : -3.0f * ww + off) + k * ww + ox, 70, 300, ui::Face::Cond, 2.0f, withAlpha(ui::kNight, 0.18f));
      ui::bigClipOff();
      ui::lean((L ? 518.0f : 910.0f) + ox, (L ? 522.0f : 914.0f) + ox, ui::kSilver);

      // the fighter in his own pixel art (x4), facing the other; drawn at random: a "?" tone on tone
      const float fa = std::min(in(sec, 0.14f, 0.6f), in(since[col], 0.0f, 0.3f));
      const float fdx = (L ? -1.0f : 1.0f) * (90.0f * (1.0f - in(sec, 0.14f, 0.6f)) + 40.0f * (1.0f - in(since[col], 0.0f, 0.3f)));
      if (d && art::clipInfo(*d, art::kSheetIdle, n, fw, fh, fps)) {
        art::drawClipPx(*d, art::kSheetIdle, int(std::floor((t + col * 0.37f) * fps)) % std::max(1, n),
                        (L ? 4.0f : kScreenW - fw * 4.0f) + fdx, kScreenH - fh * 4.0f, 4.0f, !L, fa);
      } else if (!free) {
        const float qw = ui::bigWidth("?", 420, ui::Face::Cond);
        ui::big("?", (L ? 214.0f : 1066.0f) - qw * 0.5f + fdx, 150, 420, ui::Face::Cond, withAlpha(ui::kNight, 0.3f * fa));
      }

      if (free) {  // nobody plays it yet: how to join
        const float a = up(0.3f), dy = 12.0f * (1.0f - a), cx = L ? 214.0f : 1066.0f;
        const char* who = TextFormat("J%d", col + 1);
        ui::bigHollow(who, cx - ui::bigWidth(who, 160, ui::Face::Cond) * 0.5f, 236 + dy, 160, ui::Face::Cond, 2.0f,
                      withAlpha(ui::kSilver, 0.55f * a));
        const float kw = ui::pkeyWidth("ESPACE", ui::Btn::A, 26), lw = textWidth("Rejoindre", 16, kB);
        const Rectangle jr{cx - (kw + 10 + lw) * 0.5f, 404 + dy, kw + 10 + lw, 26};
        ui::pkey("ESPACE", ui::Btn::A, jr.x, jr.y, 26, false, a);
        drawText("Rejoindre", jr.x + kw + 10, jr.y + (26 - 1.21f * 16) * 0.5f, 16, withAlpha(ui::kSilver, a), kB);
        const char* sub = "ou A sur une manette";
        drawText(sub, cx - textWidth(sub, 13, kB) * 0.5f, jr.y + 38, 13, withAlpha(ui::kMist, a), kB);
        if (focusOn(col, 0)) ring({jr.x - 12, jr.y - 10, jr.width + 24, 70});
        tagAt[col][0] = {jr.x - 12, jr.y - 38};
        continue;
      }

      // Who plays it, on a night plate on top: its tag in its colour, its device or level
      {
        const float a = up(0.3f), dy = 12.0f * (1.0f - a);
        const Rectangle c{g_chip[col].x, g_chip[col].y + dy, g_chip[col].width, g_chip[col].height};
        const char* who = whoOf(training_, slots_[col], col);
        const std::string how = howOf(training_, slots_[col], col);
        const float whoW = ui::capsWidth(who, 12, 0.08f) + 0.96f, cy = c.y + 14;
        DrawRectangleRec(c, withAlpha(ui::kNight, a));
        ui::caps(who, L ? c.x + 10 : c.x + c.width - 10 - whoW, cy, 12, 0.08f, withAlpha(mark(col), a));
        ui::caps(how.c_str(), L ? c.x + 20 + whoW : c.x + 10, cy, 11, 0.2f, withAlpha(ui::kSilver, a));
        if (focusOn(col, 0)) ring(c);
        tagAt[col][0] = L ? Vector2{c.x + c.width + 10, c.y + 5} : Vector2{c.x - 10, c.y + 5};
        stackLeft[col][0] = stackLeft[col][1] = !L;
      }
      // His number and martial art on his colour, his name on a night plate (cut like the blade), his weapons
      const Fiche fi = d ? fiche(*d) : Fiche{"Aléatoire", "", "", 0};
      const std::vector<std::string> lines = ui::nameLines(upperFr(fi.nick), 96, 400);
      const float nh = 18.0f + 80.64f * lines.size(), top = 672.64f - nh;
      {
        const float a = std::min(up(0.36f), in(since[col], 0.05f, 0.3f)), dy = 12.0f * (1.0f - a);
        const std::string lab = d ? "N° " + d->id.substr(1) + (fi.art.empty() ? std::string() : " · " + upperFr(fi.art))
                                  : std::string("N° ? · TIRÉ AU SORT");
        const float lw = 18.0f + ui::capsWidth(lab.c_str(), 13, 0.3f) + 3.9f, lh = 10.0f + 1.088f * 13;
        const float lx = L ? 36.0f : kScreenW - 36.0f - lw, ly = top - 34.0f + dy;
        DrawRectangleRec({lx, ly, lw, lh}, withAlpha(fl.fill, a));
        ui::caps(lab.c_str(), lx + 9, ly + lh * 0.5f, 13, 0.3f, withAlpha(ui::kNight, a));
        tagAt[col][1] = {L ? 36.0f : kScreenW - 36.0f, top - 60.0f};
      }
      {
        const float a = std::min(in(sec, 0.3f, 0.45f), in(since[col], 0.0f, 0.3f)), py = top + 18.0f * (1.0f - a);
        float nw = 0.0f;
        for (const auto& l : lines) nw = std::max(nw, ui::bigWidth(l.c_str(), 96, ui::Face::Cond));
        const float pw = nw + 44.0f, px = L ? 36.0f : kScreenW - 36.0f - pw;
        const Color pc = withAlpha(ui::kNight, a);
        if (L) ui::quad({px, py}, {px + pw, py}, {px + pw - 24, py + nh}, {px, py + nh}, pc, pc, pc, pc);
        else ui::quad({px + 24, py}, {px + pw, py}, {px + pw, py + nh}, {px, py + nh}, pc, pc, pc, pc);
        for (size_t i = 0; i < lines.size(); ++i) {
          const float lw = ui::bigWidth(lines[i].c_str(), 96, ui::Face::Cond);
          ui::big(lines[i].c_str(), L ? px + 14 : px + pw - 14 - lw, py + 12 + 80.64f * i, 96, ui::Face::Cond, withAlpha(WHITE, a));
        }
      }
      if (d) {  // the two weapons that fall on the arena
        const float a = std::min(up(0.42f), in(since[col], 0.08f, 0.3f)), y0 = 684 + 12.0f * (1.0f - a);
        float w = 5.0f + 6.0f;
        for (int ws = 0; ws < 2; ++ws) w += 10.0f + ui::capsWidth(ws == 0 ? "A" : "B", 10, 0.0f) + 16.0f + textWidth(d->weapons[ws].name.c_str(), 13, kB);
        w += 9.0f - 8.0f;
        float x = L ? 40.0f : kScreenW - 40.0f - w;
        DrawRectangleRec({x, y0, w, 26}, withAlpha(ui::kNight, a));
        x += 5.0f;
        for (int ws = 0; ws < 2; ++ws) {
          const char* k = ws == 0 ? "A" : "B";
          const float cw = 10.0f + ui::capsWidth(k, 10, 0.0f);
          DrawRectangleRec({x, y0 + 4.5f, cw, 17}, withAlpha(ui::kSilver, a));
          ui::caps(k, x + 5, y0 + 13, 10, 0.0f, withAlpha(ui::kNight, a));
          x += cw + 8.0f;
          const char* wn = d->weapons[ws].name.c_str();
          drawText(wn, x, y0 + (26 - 1.21f * 13) * 0.5f, 13, withAlpha(WHITE, a), kB);
          x += textWidth(wn, 13, kB) + 14.0f;
        }
      }
      if (focusOn(col, 1)) {  // the fighter before / after
        for (int dir = -1; dir <= 1; dir += 2) {
          const Rectangle cr = chevRect(col, dir);
          DrawRectangleRec(cr, withAlpha(ui::kNight, ui::hovered(cr) ? 0.95f : 0.78f));
          ui::icon(dir < 0 ? ui::Icon::ChevronL : ui::Icon::ChevronR, {cr.x + cr.width * 0.5f, cr.y + cr.height * 0.5f}, 22,
                   ui::kSilver, 2.0f);
        }
      }
    }

    // ---- The centre column, leaning with the blades -------------------------------------------------------------
    {
      const float a = up(0.2f);
      const char* head = training_ ? "ENTRAÎNEMENT" : "CHOIX DES COMBATTANTS";
      ui::caps(head, midX(36) - ui::capsWidth(head, 12, 0.3f) * 0.5f, 30 + 12.0f * (1.0f - a) + 0.534f * 12, 12, 0.3f,
               withAlpha(ui::kSilver, a));
    }
    // Head to head, as on a match night: the left one's bars grow to the left, the right one's to the right
    int sc[2] = {-1, -1};
    for (int c = 0; c < 4; ++c)
      if (g_side[c] >= 0) sc[g_side[c]] = c;
    const CharacterDef* fd[2] = {sc[0] >= 0 ? fighterOf(sc[0]) : nullptr, sc[1] >= 0 ? fighterOf(sc[1]) : nullptr};
    const std::array<int, 5> bars[2] = {fd[0] ? statBars(*fd[0]) : std::array<int, 5>{}, fd[1] ? statBars(*fd[1]) : std::array<int, 5>{}};
    const Color on[2] = {sc[0] >= 0 ? mark(sc[0]) : ui::kDim, sc[1] >= 0 ? mark(sc[1]) : ui::kDim};
    {
      const float a = up(0.6f);
      ui::caps("FACE-À-FACE", midX(378) - ui::capsWidth("FACE-À-FACE", 11, 0.3f) * 0.5f, 372 + 12.0f * (1.0f - a) + 0.534f * 11, 11,
               0.3f, withAlpha(ui::kSilver, a));
    }
    const float grow = ui::seg(sec, 0.7f, 1.4f, 0.2f, 0.8f, 0.2f);
    for (int k = 0; k < 5; ++k) {
      const float a = up(0.62f + 0.04f * k), top = 396.0f + 26.0f * k + 12.0f * (1.0f - a), x = midX(407.0f + 26.0f * k) - 170.0f;
      const int v0 = bars[0][k], v1 = bars[1][k];
      const std::string s0 = fd[0] ? std::to_string(v0) : "-", s1 = fd[1] ? std::to_string(v1) : "-";
      ui::exo(s0.c_str(), x + 24 - ui::exoWidth(s0.c_str(), 19), top + 18.58f, 19, withAlpha(!fd[0] ? ui::kDim : v0 >= v1 ? WHITE : ui::kMist, a));
      ui::exo(s1.c_str(), x + 316, top + 18.58f, 19, withAlpha(!fd[1] ? ui::kDim : v1 >= v0 ? WHITE : ui::kMist, a));
      const float vis0 = x + 28 + 98.0f * (1.0f - grow), vis1 = x + 214 + 98.0f * grow;
      for (int j = 0; j < 10; ++j) {
        const float lx = x + 118 - 10.0f * j, l0 = std::max(lx, vis0), rx = x + 214 + 10.0f * j, r1 = std::min(rx + 8, vis1);
        if (l0 < lx + 8) DrawRectangleRec({l0, top + 8, lx + 8 - l0, 6}, j < v0 ? withAlpha(on[0], a) : withAlpha(ui::kSilver, 0.14f * a));
        if (r1 > rx) DrawRectangleRec({rx, top + 8, r1 - rx, 6}, j < v1 ? withAlpha(on[1], a) : withAlpha(ui::kSilver, 0.14f * a));
      }
      const char* sn = kStatName[k][0];
      ui::big(sn, x + 170 - ui::bigWidth(sn, 15, ui::Face::Cond, 0.06f) * 0.5f, top, 15, ui::Face::Cond, withAlpha(ui::kMist, a),
              22.0f / 15.0f, 0.06f);
    }
    {
      const float a = up(0.82f), dy = 12.0f * (1.0f - a);
      float ts = 12.0f;
      while (ts > 9.0f && textWidth(tip, ts, kB) > 400.0f) ts -= 0.5f;
      drawText(tip, midX(541) - textWidth(tip, ts, kB) * 0.5f, 534 + dy, ts, withAlpha(ready ? ui::kSilver : ui::kSplitR, a), kB);
    }
    // ESPACE choisir, ÉCHAP retour
    {
      const float a = up(0.9f), y = 652 + 12.0f * (1.0f - a);
      const float k1 = ui::pkeyWidth("ESPACE", ui::Btn::A, 22), k2 = ui::pkeyWidth("ÉCHAP", ui::Btn::B, 22);
      const float t1 = textWidth("Choisir", 12, kB), t2 = textWidth("Retour", 12, kB);
      float x = midX(663) - (k1 + t1 + k2 + t2 + 44) * 0.5f;
      x += ui::pkey("ESPACE", ui::Btn::A, x, y, 22, false, a) + 8;
      drawText("Choisir", x, y + (22 - 14.52f) * 0.5f, 12, withAlpha(ui::kMist, a), kB);
      x += t1 + 28;
      x += ui::pkey("ÉCHAP", ui::Btn::B, x, y, 22, false, a) + 8;
      drawText("Retour", x, y + (22 - 14.52f) * 0.5f, 12, withAlpha(ui::kMist, a), kB);
    }
  } else {
    // ---- Three or four players: four cards leaning like the blade ------------------------------------------------
    ui::caps(training_ ? "ENTRAÎNEMENT" : "CHOIX DES COMBATTANTS", 36, 30 + 0.534f * 12, 12, 0.3f, withAlpha(ui::kSilver, up(0.2f)));
    for (int col = 0; col < cols; ++col) {
      const bool free = !joined(col);
      const CharacterDef* d = fighterOf(col);
      const ui::Flood fl = floodOf(col);
      const float a = in(sec, 0.1f + 0.05f * col, 0.45f);
      rlPushMatrix();
      rlTranslatef(0.0f, 40.0f * (1.0f - a), 0.0f);
      const float x0 = cardX(col);
      ui::lean(x0, x0 + kCardW - 34, withAlpha(fl.fill, a), ui::kLean, kCardY0, kCardInfo);
      ui::lean(x0 + kCardW - 34, x0 + kCardW, withAlpha(fl.deep, a), ui::kLean, kCardY0, kCardInfo);
      ui::lean(x0, x0 + kCardW, withAlpha(ui::kNightUp, a), ui::kLean, kCardInfo, kCardY1);
      ui::lean(x0, x0 + kCardW, withAlpha(ui::kSilver, a), ui::kLean, kCardInfo - 1, kCardInfo + 1);
      const float fa = std::min(a, in(since[col], 0.0f, 0.3f));
      if (d && art::clipInfo(*d, art::kSheetIdle, n, fw, fh, fps)) {  // x2, his feet on the plate
        beginClip({cardLeft(col, kCardInfo), kCardY0 + 40.0f * (1.0f - a), kCardW + ui::kLean * (kCardInfo - kCardY0), kCardInfo - kCardY0});
        art::drawClipPx(*d, art::kSheetIdle, int(std::floor((t + col * 0.37f) * fps)) % std::max(1, n),
                        cardLeft(col, kCardInfo - fh) + (kCardW - fw * 2.0f) * 0.5f, kCardInfo - fh * 2.0f, 2.0f, false, fa);
        endClip();
      } else if (!free) {
        const float qw = ui::bigWidth("?", 240, ui::Face::Cond);
        ui::big("?", cardLeft(col, 360) + (kCardW - qw) * 0.5f, 264, 240, ui::Face::Cond, withAlpha(ui::kNight, 0.3f * fa));
      }
      if (free) {
        const char* who = TextFormat("J%d", col + 1);
        const float cx = cardLeft(col, 330) + kCardW * 0.5f;
        ui::bigHollow(who, cx - ui::bigWidth(who, 120, ui::Face::Cond) * 0.5f, 250, 120, ui::Face::Cond, 2.0f, withAlpha(ui::kSilver, 0.55f * a));
        const float kw = ui::pkeyWidth("ESPACE", ui::Btn::A, 24), lw = textWidth("Rejoindre", 14, kB);
        const Rectangle jr{cardLeft(col, 400) + (kCardW - kw - 8 - lw) * 0.5f, 388, kw + 8 + lw, 24};
        ui::pkey("ESPACE", ui::Btn::A, jr.x, jr.y, 24, false, a);
        drawText("Rejoindre", jr.x + kw + 8, jr.y + (24 - 1.21f * 14) * 0.5f, 14, withAlpha(ui::kSilver, a), kB);
        const char* sub = "ou A sur une manette";
        drawText(sub, cardLeft(col, 430) + (kCardW - textWidth(sub, 12, kB)) * 0.5f, 422, 12, withAlpha(ui::kMist, a), kB);
        if (focusOn(col, 0)) ring({jr.x - 10, jr.y - 8, jr.width + 20, 60});
        tagAt[col][0] = {jr.x - 10, jr.y - 36};
        rlPopMatrix();
        continue;
      }
      // who plays it
      const Rectangle c = g_chip[col];
      const char* who = whoOf(training_, slots_[col], col);
      const std::string how = howOf(training_, slots_[col], col);
      const float whoW = ui::capsWidth(who, 11, 0.08f);
      float hs = 10.0f;
      while (hs > 7.0f && 28.0f + whoW + ui::capsWidth(how.c_str(), hs, 0.16f) > c.width) hs -= 0.5f;
      DrawRectangleRec(c, withAlpha(ui::kNight, a));
      ui::caps(who, c.x + 9, c.y + 13, 11, 0.08f, withAlpha(mark(col), a));
      ui::caps(how.c_str(), c.x + 19 + whoW, c.y + 13, hs, 0.16f, withAlpha(ui::kSilver, a));
      if (focusOn(col, 0)) ring(c);
      tagAt[col][0] = {c.x + c.width + 8, c.y + 4};
      // its number and martial art, its name, its stats in a line
      const Fiche fi = d ? fiche(*d) : Fiche{"Aléatoire", "", "", 0};
      const std::string lab = d ? "N° " + d->id.substr(1) + (fi.art.empty() ? std::string() : " · " + upperFr(fi.art)) : std::string("N° ? · TIRÉ AU SORT");
      const float lw = 14.0f + ui::capsWidth(lab.c_str(), 10, 0.2f);
      DrawRectangleRec({cardLeft(col, 545) + 14, 536, lw, 18}, withAlpha(fl.fill, a));
      ui::caps(lab.c_str(), cardLeft(col, 545) + 21, 545, 10, 0.2f, withAlpha(ui::kNight, a));
      const std::string nick = upperFr(fi.nick);
      float ns = 34.0f;
      while (ns > 20.0f && ui::bigWidth(nick.c_str(), ns, ui::Face::Cond) > kCardW - 40) ns -= 1.0f;
      ui::big(nick.c_str(), cardLeft(col, 575) + 14, 562 + (34.0f - ns) * 0.84f, ns, ui::Face::Cond, withAlpha(WHITE, a));
      if (d) {
        const std::array<int, 5> b = statBars(*d);
        float x = cardLeft(col, 616) + 14;
        for (int k = 0; k < 5; ++k) {
          x += ui::caps(kStatName[k][1], x, 616, 10, 0.1f, withAlpha(ui::kMist, a)) + 5;
          const std::string v = std::to_string(b[k]);
          ui::exo(v.c_str(), x, 621, 14, withAlpha(WHITE, a));
          x += ui::exoWidth(v.c_str(), 14) + 11;
        }
      }
      tagAt[col][1] = {cardLeft(col, 520) + kCardW - 14, kCardInfo - 26};
      stackLeft[col][1] = true;
      if (focusOn(col, 1)) {
        for (int dir = -1; dir <= 1; dir += 2) {
          const Rectangle cr = chevRect(col, dir);
          DrawRectangleRec(cr, withAlpha(ui::kNight, ui::hovered(cr) ? 0.95f : 0.78f));
          ui::icon(dir < 0 ? ui::Icon::ChevronL : ui::Icon::ChevronR, {cr.x + 16, cr.y + 16}, 18, ui::kSilver, 2.0f);
        }
      }
      rlPopMatrix();
    }
    // the tip, the keys; FIGHT bottom right
    drawText(tip, 36, 650, 13, withAlpha(ready ? ui::kSilver : ui::kSplitR, up(0.5f)), kB);
    float x = 36;
    const float a = up(0.55f);
    x += ui::pkey("ESPACE", ui::Btn::A, x, 676, 22, false, a) + 8;
    drawText("Choisir", x, 676 + (22 - 14.52f) * 0.5f, 12, withAlpha(ui::kMist, a), kB);
    x += textWidth("Choisir", 12, kB) + 28;
    x += ui::pkey("ÉCHAP", ui::Btn::B, x, 676, 22, false, a) + 8;
    drawText("Retour", x, 676 + (22 - 14.52f) * 0.5f, 12, withAlpha(ui::kMist, a), kB);
  }

  // ---- The rules button, the roster, FIGHT ----------------------------------------------------------------------
  {
    const float a = up(0.26f);
    Rectangle b = g_rulesBtn;
    b.y += 12.0f * (1.0f - a);
    const bool hot = focusOn(0, -1);
    if (hot) DrawRectangleRec(b, withAlpha(ui::kSilver, 0.08f * a));
    DrawRectangleLinesEx(b, 1.0f, withAlpha(Color{214, 226, 255, 255}, 0.3f * a));
    DrawRectangleRec({b.x + 6, b.y + 7, 34, 34}, withAlpha(ui::kSilver, a));
    ui::icon(ui::Icon::Sliders, {b.x + 23, b.y + 24}, 20, withAlpha(ui::kNight, a), 1.9f);
    ui::caps("RÈGLES & MODE", b.x + 51, b.y + 15.05f, 12, 0.14f, withAlpha(ui::kSilver, a));
    drawText(g_rulesLine.c_str(), b.x + 51, b.y + 25.6f, 11.5f, withAlpha(ui::kMist, a), kB);
    const float kw = ui::pkeyWidth("R", ui::Btn::Y, 22);
    ui::pkey("R", ui::Btn::Y, b.x + b.width - 10 - kw, b.y + 13, 22, false, a);
    if (cursorOn(0, -1)) ring(b);
  }
  for (int i = 0; i <= int(r.size()); ++i) {
    Rectangle rr = rosterRect(i);
    const float a = in(sec, 0.3f + 0.04f * i, 0.4f);
    rr.y -= 16.0f * (1.0f - a);
    const bool rnd = i == int(r.size());
    int pickers[4], np = 0;
    for (int c = 0; c < cols; ++c) {
      const SlotSetup& sl = slots_[c];
      if (joined(c) && (rnd ? sl.random : !sl.random && sl.character == i)) pickers[np++] = c;
    }
    const bool hov = ui::hovered(rr);
    if (rnd) {
      ui::icon(ui::Icon::Dice, {rr.x + 40, rr.y + 38}, 28, withAlpha(ui::kMist, a), 1.5f);
      ui::caps("HASARD", rr.x + 40 - ui::capsWidth("HASARD", 9, 0.14f) * 0.5f, rr.y + 62 + 0.534f * 9, 9, 0.14f, withAlpha(ui::kMist, a));
    } else {
      DrawRectangleRec(rr, withAlpha(Color{14, 22, 64, 255}, a));
      tileFace(r[i], rr, a);
      DrawRectangleRec({rr.x, rr.y + 76, 80, 20}, withAlpha(ui::kNight, 0.86f * a));
      ui::big(firstWord(upperFr(fiche(r[i]).nick)).c_str(), rr.x + 7, rr.y + 80.12f, 14, ui::Face::Cond, withAlpha(ui::kSilver, a), 0.84f, 0.04f);
    }
    if (np > 0) DrawRectangleLinesEx(rr, 3.0f, withAlpha(mark(pickers[0]), a));
    else if (hov) DrawRectangleLinesEx(rr, 2.0f, withAlpha(ui::kSilver, a));
    else DrawRectangleLinesEx(rr, 1.0f, withAlpha(Color{214, 226, 255, 255}, (rnd ? 0.3f : 0.16f) * a));
    float tx = rr.x + rr.width;
    for (int k = 0; k < np; ++k) {  // who picked it, in the corner
      const char* who = whoOf(training_, slots_[pickers[k]], pickers[k]);
      const float tw = 12.0f + ui::capsWidth(who, 10, 0.08f) + 0.8f;
      tx -= tw;
      DrawRectangleRec({tx, rr.y, tw, 18}, withAlpha(mark(pickers[k]), a));
      ui::caps(who, tx + 6, rr.y + 9, 10, 0.08f, withAlpha(ui::kNight, a));
      tx -= 2;
    }
  }
  {
    const char* help = "← → ou un clic pour choisir · le dé : un combattant au hasard";
    const float a = up(0.58f), cx = g_wide ? midX(351) : kScreenW * 0.5f;
    drawText(help, cx - textWidth(help, 12, kB) * 0.5f, (g_wide ? 344.0f : 186.0f) + 12.0f * (1.0f - a), 12, withAlpha(ui::kMist, a), kB);
  }
  {
    const float a = up(0.86f);
    Rectangle g = goRect();
    g.y += 12.0f * (1.0f - a);
    const Color gc = withAlpha(!ready ? ui::kDim : ui::hovered(g) ? WHITE : ui::kSilver, a);
    ui::quad({g.x + 12, g.y}, {g.x + g.width, g.y}, {g.x + g.width - 12, g.y + g.height}, {g.x, g.y + g.height}, gc, gc, gc, gc);
    const char* go = training_ ? "S'ENTRAÎNER !" : "COMBATTRE !";
    const float kw = ui::pkeyWidth("ENTRÉE", ui::Btn::Start, 22), gw = ui::capsWidth(go, 19, 0.14f) + 2.66f;
    const float x = g.x + (g.width - kw - 14 - gw) * 0.5f;
    inkKey("ENTRÉE", ui::Btn::Start, x, g.y + 17, a);
    ui::caps(go, x + kw + 14, g.y + 28, 19, 0.14f, withAlpha(ui::kNight, a));
    if (ready && sec >= 1.1f) {  // an Éclat sweep along its foot
      const float p = std::fmod(sec - 1.1f, 1.4f) / 1.4f;
      const float w = 282.0f * (p < 0.55f ? ui::seg(p, 0.0f, 0.55f, 0.6f, 0.0f, 0.2f) : 1.0f - ui::seg(p, 0.56f, 1.0f, 0.6f, 0.0f, 0.2f));
      DrawRectangleRec({p < 0.55f ? g.x + 6 : g.x + 288 - w, g.y + 52, w, 4}, ui::kBlue);
    }
  }

  // Whose cursor is where: a tag in the player's colour by the field
  std::array<int, 12> stacked{};
  for (int dev = 0; dev < kDeviceCount; ++dev) {
    const CssCursor& c = cssCur_[dev];
    if (!c.on) continue;
    const int col = std::clamp(c.col, 0, cols - 1);
    if (c.row >= 0 && g_panel[col].width <= 0.0f) continue;
    const int row = c.row == 1 && !joined(col) ? 0 : c.row;
    const int slot = row < 0 ? 0 : 1 + row * 4 + col;
    const int own = ownedCard(dev);
    const char* label = own >= 0 ? TextFormat("J%d", own + 1) : dev == kKeyboard1 ? "C1" : dev == kKeyboard2 ? "C2" : "M";
    const float w = 12.0f + ui::capsWidth(label, 10, 0.08f) + 0.8f;
    const Vector2 at = row < 0 ? Vector2{g_rulesBtn.x, g_rulesBtn.y - 22} : tagAt[col][row];
    const bool left = row >= 0 && stackLeft[col][row];
    const float x = left ? at.x - w - stacked[slot] * (w + 4) : at.x + stacked[slot] * (w + 4);
    ++stacked[slot];
    DrawRectangleRec({x, at.y, w, 18}, own >= 0 ? mark(own) : ui::kBlue);
    ui::caps(label, x + 6, at.y + 9, 10, 0.08f, ui::kNight);
  }
  if (!rulesOpen_) {
    rulesAnim_ = 0;
    return;
  }

  // ---- The « Règles & mode » drawer, over the dimmed screen --------------------------------------------------------
  const float so = ++rulesAnim_ / 60.0f;
  SheetItem items[16];
  const int nItems = sheetItems(training_, slots_, teamsOn, items);
  const SheetItem sel = items[std::clamp(rulesSel_, 0, nItems - 1)];
  auto sheetFocus = [&](int col, int row) {
    return (sel.col == col && sel.row == row) || ui::hovered(sheetField(col, row, teamsOn));
  };
  constexpr Color kRim{214, 226, 255, 255};
  DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(ui::kNight, 0.7f * ui::seg(so, 0.0f, 0.3f, 0.0f, 0.0f, 0.58f)));
  rlPushMatrix();  // it comes in from the right
  rlTranslatef(640.0f * (1.0f - ui::seg(so, 0.0f, 0.42f)), 0.0f, 0.0f);
  ui::quad({kSheetX, 0}, {kScreenW + 1, 0}, {kScreenW + 1, kScreenH}, {kSheetX - ui::kLean * kScreenH, kScreenH}, ui::kNightUp,
           ui::kNightUp, ui::kNightUp, ui::kNightUp);
  ui::lean(kSheetX - 4, kSheetX, ui::kSilver);
  auto rin = [&](float delay) { return ui::seg(so, delay, delay + 0.36f); };  // each line slides in after it
  auto label = [&](const char* s, float y, float a) {
    ui::caps(s, kSX + 40.0f * (1.0f - a), y + 0.534f * 10, 10, 0.24f, withAlpha(ui::kMist, a));
  };
  {
    const float a = ui::seg(so, 0.18f, 0.58f), b = rin(0.2f);
    ui::big("RÈGLES & MODE", kSX + 30.0f * (1.0f - a), 36, 60, ui::Face::Cond, withAlpha(WHITE, a));
    drawText(training_ ? "Ton clavier ou ta manette pour l'entraînement." : "Ce combat, et qui joue : un humain ou l'ordinateur.",
             kSX + 40.0f * (1.0f - b), 96, 13, withAlpha(ui::kMist, b), kB);
    const Rectangle cl = closeRect();
    if (ui::hovered(cl)) DrawRectangleRec(cl, withAlpha(ui::kSilver, 0.1f));
    DrawRectangleLinesEx(cl, 1.0f, withAlpha(kRim, 0.2f));
    ui::icon(ui::Icon::Close, {cl.x + 20, cl.y + 20}, 18, ui::kSilver, 1.8f);
  }
  if (!training_) {
    // the mode
    label("MODE DE JEU", 132, rin(0.22f));
    const float a = rin(0.24f), dx = 40.0f * (1.0f - a);
    for (int k = 0; k < 2; ++k) {
      Rectangle c = modeSeg(k);
      c.x += dx;
      const bool chosen = (k == 1) == teamsOn;
      if (chosen) DrawRectangleRec(c, withAlpha(ui::kSilver, a));
      else {
        if (ui::hovered(c)) DrawRectangleRec(c, withAlpha(ui::kSilver, 0.06f * a));
        DrawRectangleLinesEx(c, 1.0f, withAlpha(kRim, 0.2f * a));
      }
      const Color ink = withAlpha(chosen ? ui::kNight : ui::kMist, a);
      const char* lab = k == 0 ? "CHACUN POUR SOI" : "EN ÉQUIPES";
      const float gx = c.x + (c.width - 27 - ui::capsWidth(lab, 11, 0.12f)) * 0.5f;
      ui::icon(k == 0 ? ui::Icon::Person : ui::Icon::Fighters, {gx + 9, c.y + 24}, 18, ink, 1.8f);
      ui::caps(lab, gx + 27, c.y + 24, 11, 0.12f, ink);
    }
    if (sheetFocus(3, -1)) ring({modeRect().x + dx, modeRect().y, modeRect().width, modeRect().height});
    // the rules
    label("RÈGLES", 224, rin(0.26f));
    const char* labels[3] = {"VIES", "CLASH ORB", "ARMES"};
    const char* helps[3] = {"Par combattant", "La seule source des ultimes", "Elles tombent sur l'arène"};
    for (int i = 0; i < 3; ++i) {
      const float b = rin(0.28f + 0.03f * i);
      Rectangle rr = ruleRowRect(i);
      rr.x += 40.0f * (1.0f - b);
      const bool foc = sheetFocus(i, -1);
      if (ui::hovered(rr)) DrawRectangleRec(rr, withAlpha(ui::kSilver, 0.04f * b));
      DrawRectangleRec({rr.x, rr.y + rr.height - 1, rr.width, 1}, withAlpha(kRim, 0.12f * b));
      ui::big(labels[i], rr.x, rr.y + 4.66f, 24, ui::Face::Cond, withAlpha(WHITE, b), 0.84f, 0.04f);
      drawText(helps[i], rr.x, rr.y + 28.82f, 12, withAlpha(ui::kMist, b), kB);
      const float vx = rr.x + rr.width - 126, cy = rr.y + rr.height * 0.5f;
      const Color chev = withAlpha(foc ? ui::kSilver : ui::kMist, b);
      ui::icon(ui::Icon::ChevronL, {vx + 16, cy}, 16, chev, 2.0f);
      ui::icon(ui::Icon::ChevronR, {vx + 110, cy}, 16, chev, 2.0f);
      if (i == 0) {
        const std::string v = std::to_string(st.stocks);
        ui::exo(v.c_str(), vx + 63 - ui::exoWidth(v.c_str(), 27) * 0.5f, cy + 10.77f, 27, withAlpha(WHITE, b));
      } else {
        const bool yes = i == 1 ? st.items : st.weapons;
        const char* v = yes ? "OUI" : "NON";
        ui::caps(v, vx + 63 - ui::capsWidth(v, 13, 0.12f) * 0.5f, cy, 13, 0.12f, withAlpha(yes ? ui::kSilver : ui::kMist, b));
      }
      if (foc) ring(rr);
    }
  } else {
    label("ENTRAÎNEMENT", 132, rin(0.22f));
    const float a = rin(0.24f);
    ui::drawWrapped("Vies infinies, pas de minuteur. Le mannequin se règle dans le menu pause.", kSX + 40.0f * (1.0f - a), 152, kSW,
                    15, withAlpha(ui::kSilver, a));
  }

  // Who plays: a human (on which device) or the computer (at which level), in which team
  label("JOUEURS", 404, rin(0.37f));
  if (!training_) {
    const float a = rin(0.37f);
    const char* ia = "IA : niveaux 1 à 9, puis « 10 x2 »";
    drawText(ia, kSX + kSW - textWidth(ia, 11, kB) + 40.0f * (1.0f - a), 402, 11, withAlpha(ui::kMist, a), kB);
  }
  for (int c = 0; c < cols; ++c) {
    const float a = rin(0.4f + 0.03f * c), dx = 40.0f * (1.0f - a);
    const Rectangle rw = playerRowRect(c);
    const SlotSetup& sl = slots_[c];
    const int type = training_ ? (c == 0 ? 1 : 2) : sl.type;
    const float cy = rw.y + rw.height * 0.5f;
    const char* who = training_ && c == 1 ? "CIBLE" : TextFormat("J%d", c + 1);
    DrawRectangleRec({rw.x + dx, cy - 13, g_tagW, 26}, withAlpha(tint(c), a));
    ui::caps(who, rw.x + dx + (g_tagW - ui::capsWidth(who, 12, 0.08f)) * 0.5f, cy, 12, 0.08f, withAlpha(ui::kNight, a));
    Rectangle f0 = sheetField(c, 0, teamsOn), f2 = sheetField(c, 2, teamsOn);
    f0.x += dx, f2.x += dx;
    DrawRectangleLinesEx(f0, 1.0f, withAlpha(kRim, 0.2f * a));
    if (training_) {  // you, and the dummy: nothing to choose here
      const char* v = c == 0 ? "VOUS" : "MANNEQUIN";
      ui::caps(v, f0.x + (f0.width - ui::capsWidth(v, 10, 0.1f)) * 0.5f, cy, 10, 0.1f, withAlpha(ui::kMist, a));
    } else {
      for (int k = 0; k < 3; ++k) {  // HUMAIN, IA, nobody
        const Rectangle g = toggleSeg(f0, k);
        const bool chosen = type != 0 && type == kSegType[k];
        if (chosen) DrawRectangleRec(g, withAlpha(ui::kSilver, a));
        else if (ui::hovered(g)) DrawRectangleRec(g, withAlpha(ui::kSilver, 0.08f * a));
        const Color ink = withAlpha(chosen ? ui::kNight : ui::kMist, a);
        if (k < 2) ui::caps(kSegLabel[k], g.x + 10, cy, 10, 0.1f, ink);
        else DrawRectangleRec({g.x + g.width * 0.5f - 6, cy - 1, 12, 2}, ink);
      }
      if (sheetFocus(c, 0)) ring(f0);
    }
    if (type == 0) {  // a free card
      drawText("ou A sur une manette", f2.x, cy - 1.21f * 6, 12, withAlpha(ui::kMist, a), kB);
      continue;
    }
    if (training_ && c == 1) {
      const char* v = "RÉGLABLE EN PAUSE";
      DrawRectangleLinesEx(f2, 1.0f, withAlpha(kRim, 0.12f * a));
      ui::caps(v, f2.x + (f2.width - ui::capsWidth(v, 10, 0.12f)) * 0.5f, cy, 10, 0.12f, withAlpha(ui::kMist, a));
    } else {  // its device (and which gamepad it is), or its level, between two arrows
      const bool foc = sheetFocus(c, 2);
      DrawRectangleLinesEx(f2, 1.0f, withAlpha(kRim, 0.2f * a));
      const Color chev = withAlpha(foc ? ui::kSilver : ui::kMist, a);
      ui::icon(ui::Icon::ChevronL, {f2.x + 16, cy}, 14, chev, 2.0f);
      ui::icon(ui::Icon::ChevronR, {f2.x + f2.width - 16, cy}, 14, chev, 2.0f);
      const bool plugged = type != 1 || deviceAvailable(sl.device);
      const char* sub = type != 1 ? nullptr : !plugged ? "non branchée" : sl.device >= kPad1 ? padModel(sl.device - kPad1) : nullptr;
      const std::string v = type == 1 ? upperFr(deviceName(sl.device)) : sl.level >= kMaxCpuLevel ? "NIVEAU 10 X2" : "NIVEAU " + std::to_string(sl.level);
      const float mid = f2.x + f2.width * 0.5f, room = f2.width - 64;
      float vs = 10.0f;
      while (vs > 7.0f && ui::capsWidth(v.c_str(), vs, 0.12f) > room) vs -= 0.5f;
      ui::caps(v.c_str(), mid - ui::capsWidth(v.c_str(), vs, 0.12f) * 0.5f, sub ? cy - 6 : cy, vs, 0.12f, withAlpha(ui::kSilver, a));
      if (sub) {
        float ss = 10.0f;
        while (ss > 7.0f && textWidth(sub, ss, kB) > room) ss -= 0.5f;
        drawText(sub, mid - textWidth(sub, ss, kB) * 0.5f, cy + 1, ss, withAlpha(plugged ? ui::kMist : ui::kSplitR, a), kB);
      }
      if (foc) ring(f2);
    }
    if (teamsOn) {
      Rectangle f3 = sheetField(c, 3, teamsOn);
      f3.x += dx;
      DrawRectangleRec(f3, withAlpha(tint(c), a));
      const char* tn = teamName(slotTeam(c));
      ui::caps(tn, f3.x + (f3.width - ui::capsWidth(tn, 10, 0.1f)) * 0.5f, cy, 10, 0.1f, withAlpha(ui::kNight, a));
      if (sheetFocus(c, 3)) ring(f3);
    }
  }

  // The foot: what the rules mean (or what is missing), and done
  {
    const float a = rin(0.48f), b = rin(0.5f);
    float ts = 13.0f;
    while (ts > 9.0f && textWidth(tip, ts, kB) > kSW) ts -= 0.5f;
    drawText(tip, kSX + 40.0f * (1.0f - a), 624, ts, withAlpha(ready ? ui::kSilver : ui::kSplitR, a), kB);
    Rectangle vr = validateRect();
    vr.x += 40.0f * (1.0f - b);
    const Color vc = withAlpha(ui::hovered(vr) ? WHITE : ui::kSilver, b);
    ui::quad({vr.x + 11, vr.y}, {vr.x + vr.width, vr.y}, {vr.x + vr.width - 11, vr.y + vr.height}, {vr.x, vr.y + vr.height}, vc, vc, vc, vc);
    const float kw = ui::pkeyWidth("ENTRÉE", ui::Btn::Start, 22), vw = ui::capsWidth("VALIDER", 16, 0.14f) + 2.24f;
    const float x = vr.x + (vr.width - kw - 12 - vw) * 0.5f;
    inkKey("ENTRÉE", ui::Btn::Start, x, vr.y + 15, b);
    ui::caps("VALIDER", x + kw + 12, vr.y + 26, 16, 0.14f, withAlpha(ui::kNight, b));
  }
  rlPopMatrix();
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
  constexpr FontKind kT = FontKind::Display, kB = FontKind::Body;  // « Épure »
  const auto& stages = stageList();
  const int n = int(stages.size());
  // "?": the arena behind flips through them all, one of them is drawn when the match starts
  const int shown = stageRandom_ ? int(t * 1.5f) % n : stage_;
  const StageDef& cur = stages[shown];
  // The arena itself fills the screen; night on the left under its sheet, and along the bottom under the cards.
  ui::stageBackdrop(shown, t, 0.0f);
  const Color night = ui::kDeep;
  DrawRectangleGradientH(0, 0, 410, kScreenH, withAlpha(night, 0.94f), withAlpha(night, 0.8f));
  DrawRectangleGradientH(410, 0, 384, kScreenH, withAlpha(night, 0.8f), withAlpha(night, 0.15f));
  DrawRectangleGradientH(794, 0, kScreenW - 794, kScreenH, withAlpha(night, 0.15f), withAlpha(night, 0.25f));
  DrawRectangleGradientV(0, 460, kScreenW, 182, withAlpha(night, 0.0f), withAlpha(night, 0.95f));
  DrawRectangle(0, 642, kScreenW, kScreenH - 642, withAlpha(night, 0.95f));

  const char* title = stageBrowse_ ? "ARÈNES" : "CHOIX DE L'ARÈNE";
  drawText(title, 48, capsY(48, 28, kT), 28, ui::kCream, kT);
  drawText(stageRandom_ ? TextFormat("? / %d", n) : TextFormat("%d / %d", stage_ + 1, n), 48 + textWidth(title, 28, kT) + 14,
           capsY(48, 13, kB), 13, ui::kMuted, kB);
  DrawRectangleRec({48, 74, 40, 2}, ui::kCream);
  DrawCircleV({92, 75}, 3, ui::kBlue);

  // Its sheet: where it comes from, its name, a line about it, the plan of its platforms.
  const float x = 48, w = 470;
  const std::string series = stageRandom_ ? std::string("TOUTES LES ARÈNES") : upperFr(cur.series);
  const char* name = stageRandom_ ? "Au hasard" : cur.name.c_str();
  ui::label(series.c_str(), x, 113, 11, 0.16f, ui::kBlue);
  float ns = 40.0f;
  while (ns > 24.0f && textWidth(name, ns, kT) > w) ns -= 1.0f;
  drawText(name, x, capsY(156, ns, kT), ns, ui::kCream, kT);
  float y = ui::drawWrapped(stageRandom_ ? std::string("Une des arènes, tirée au sort au début du combat.") : cur.blurb, x,
                            190, w, 21, ui::kSoft);
  ui::label("PLATEFORMES", x, y + 12, 11, 0.16f, ui::kMuted);
  const Rectangle map{x, y + 26, 420, 170};
  ui::roundRect(map, 14, withAlpha(ui::kNavyDeep, 0.72f), withAlpha(ui::kNavyDeep, 0.72f));
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
  drawText("Sol", lx + 20, capsY(ly, 13, kB), 13, ui::kSoft, kB);
  lx += 20 + textWidth("Sol", 13, kB) + 18;
  DrawRectangleRec({lx, ly - 1, 14, 3}, ui::kGold);
  drawText("Plateforme fine", lx + 20, capsY(ly, 13, kB), 13, ui::kSoft, kB);
  lx += 20 + textWidth("Plateforme fine", 13, kB) + 18;
  for (int i = 0; i < 3; ++i) DrawRectangleRec({lx + i * 5.0f, ly, 3, 1}, ui::kGold);
  drawText("Mobile", lx + 20, capsY(ly, 13, kB), 13, ui::kSoft, kB);

  // Every arena: its still, its name at the bottom; the chosen one lifted, in gold.
  auto frame = [&](int i, const char* label, bool sel) {
    Rectangle rr = stageCardRect(i);
    if (sel) rr.y -= 6;
    rr.y += 60.0f * std::pow(1.0f - clampf((screenFrames_ - 4 - i * 3.0f) / 27.0f, 0.0f, 1.0f), 3.0f);  // rising in
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
    float fs = 13.0f;
    while (fs > 10.0f && textWidth(label, fs, kB) > rr.width - 20) fs -= 0.5f;
    drawText(label, rr.x + 10, capsY(rr.y + rr.height - 15, fs, kB), fs, ui::kCream, kB);
    if (sel) ui::bevelLines(ui::grow(rr, -1.5f), cut - 0.6f, 3.0f, ui::kGold);
  };
  for (int i = 0; i < n; ++i) frame(i, stages[i].name.c_str(), !stageRandom_ && i == stage_);
  frame(n, "Au hasard", stageRandom_);
  const ui::Hint go = stageGo(stageBrowse_);
  ui::footer({{"FLÈCHES", ui::Btn::Move, "Choisir"}, {"ÉCHAP", ui::Btn::B, "Retour"}}, nullptr, &go);
}
