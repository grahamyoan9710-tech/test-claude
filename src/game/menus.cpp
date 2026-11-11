// Character select (« Encre d'horizon ») and stage select screens.
#include <array>
#include <string>

#include "game/app.hpp"
#include "game/fighter_art.hpp"
#include "game/fx.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
int cardCount(bool training) { return training ? 2 : 4; }
constexpr FontKind kT = FontKind::Title, kU = FontKind::Ui;
constexpr Color kPanel{38, 36, 80, 255}, kPanelDeep{23, 23, 58, 255}, kGoldLight{111, 227, 255, 255};  // « Épure »: the accent is cyan
constexpr Color kNoFighter{42, 53, 88, 255};  // "?": a fighter drawn at random

// The roster on top, tall tiles slanted like the hits, each fighter's face; under it a sheet per player, its portrait
// standing out of it, its stats: two wide sheets, or four narrow ones past two players. The mode, the rules and who
// plays (a human or the computer) are on the « Règles & mode » sheet, behind the button top right.
Rectangle g_panel[4];     // each card's sheet, set each frame by App::layoutCss (width 0: not on screen)
Rectangle g_chip[4];      // its player chip (width 0: a free card, its whole sheet joins it)
Rectangle g_rulesBtn;     // the « Règles & mode » button
bool g_wide = true;       // two wide sheets
std::string g_rulesLine;  // what the button says of the rules
constexpr float kTileW = 96, kTileH = 120, kTileGap = 10, kTileY = 118, kTileSlant = 0;  // « Épure »: round tiles

// Roster tile i; "?" (a fighter at random) last.
Rectangle rosterRect(int i) {
  const int n = int(roster().size()) + 1;
  const float w = std::min(kTileW, (kScreenW - 76.0f - kTileSlant - (n - 1) * kTileGap) / n);
  const float x0 = (kScreenW - (n * w + (n - 1) * kTileGap + kTileSlant)) * 0.5f;
  return {x0 + i * (w + kTileGap), kTileY, w + kTileSlant, kTileH};
}
// The strip of a roster tile between the fractions f0 and f1 of its height.
void tileBand(Rectangle r, float f0, float f1, Color c0, Color c1) {
  const float w = r.width - kTileSlant, y0 = r.y + r.height * f0, y1 = r.y + r.height * f1;
  const float l0 = r.x + kTileSlant * (1.0f - f0), l1 = r.x + kTileSlant * (1.0f - f1);
  ui::quad({l0, y0}, {l0 + w, y0}, {l1 + w, y1}, {l1, y1}, c0, c0, c1, c1);
}
// Fields on the screen: the button (row -1); each card's sheet (1: its fighter) and its chip (0: who plays it; a free
// card's whole sheet).
Rectangle fieldRect(int col, int row) {
  if (row < 0) return g_rulesBtn;
  return row == 0 && g_chip[col].width > 0.0f ? g_chip[col] : g_panel[col];
}

// The « Règles & mode » sheet: the mode and the rules on the left, who plays on the right.
constexpr Rectangle kSheet{190, 64, 900, 592};
constexpr float kLeftX = 224, kLeftW = 320, kRightX = 576, kRightW = 480, kColY = 196;
Rectangle modeRect() { return {kLeftX, kColY + 30, kLeftW, 92}; }
Rectangle ruleRowRect(int i) { return {kLeftX, kColY + 154 + i * 62.0f, kLeftW, 48}; }  // lives, orb, weapons
Rectangle playerRowRect(int col) { return {kRightX, kColY + 28 + col * 74.0f, kRightW, 64}; }
Rectangle closeRect() {
  const float w = 22 + ui::promptWidth("ÉCHAP", ui::Btn::B, 24) + textWidth("Fermer", 14);
  return {kSheet.x + kSheet.width - 26 - w, kSheet.y + 30, w, 36};
}
Rectangle validateRect() {
  const float w = 40 + ui::promptWidth("ENTRÉE", ui::Btn::Start, 24) + textWidth("VALIDER", 15, FontKind::Display);
  return {kSheet.x + kSheet.width - 34 - w, kSheet.y + kSheet.height - 70, w, 44};
}
// Field (col, row) of the sheet: the mode (col 3) and the rules (lives, orb, weapons) on row -1; a player's type (0),
// device or level (2) and team (3).
Rectangle sheetField(int col, int row, bool teams) {
  if (row < 0) return col == 3 ? modeRect() : ruleRowRect(col);
  const Rectangle r = playerRowRect(col);
  const float tx = r.x + 104 + (teams ? 0.0f : 96.0f), dx = tx + 140, y = r.y + r.height * 0.5f - 18;
  if (row == 0) return {tx, y, 132, 36};
  if (row == 3) return {r.x + r.width - 72, y, 62, 36};
  return {dx, y, r.x + r.width - 10 - (teams ? 80.0f : 0.0f) - dx, 36};
}
// The type toggle's parts: HUMAIN, IA, nobody (a cross), and the type each one sets.
Rectangle toggleSeg(Rectangle f, int k) {
  const float x[3] = {0, 66, 102}, w[3] = {66, 36, 30};
  return {f.x + x[k], f.y, w[k], f.height};
}
constexpr int kSegType[3] = {1, 2, 0};
struct SheetItem {
  int col, row;
};
// The sheet's fields in keyboard / gamepad order. Training: player 1's device only (the dummy is set in the pause).
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
  if (training) return col == 0 ? std::string("VOUS · ") + deviceName(s.device) : "MANNEQUIN · RÉGLABLE EN PAUSE";
  if (s.type == 2)
    return s.level >= kMaxCpuLevel ? "ORDINATEUR · NIV. 10 x2" : "ORDINATEUR · NIVEAU " + std::to_string(s.level);
  return std::string("HUMAIN · ") + deviceName(s.device);
}

// The manga screentone: a dot every 6 px.
void screentone(Rectangle r, Color c) {
  for (float y = r.y + 3; y < r.y + r.height; y += 6)
    for (float x = r.x + 3; x < r.x + r.width; x += 6) DrawRectangleRec({x - 0.8f, y - 0.8f, 1.6f, 1.6f}, c);
}
// The « Règles & mode » emblem: a gold gear around the night, the logo's spark in it.
void emblem(Vector2 c, float r, float t) {
  const float k = r / 32.0f;
  const Color light{255, 224, 163, 255}, deep = ui::kGoldDeep;
  for (int i = 0; i < 8; ++i)  // the teeth, lit from the top left
    DrawRectanglePro({c.x, c.y, 7 * k, 10 * k}, {3.5f * k, 30 * k}, i * 45.0f,
                     mixColor(light, deep, 0.5f - 0.5f * std::cos((i * 45.0f - 315.0f) * kPi / 180.0f)));
  DrawCircleV(c, 22 * k, deep);
  DrawCircleV({c.x - k, c.y - k}, 21 * k, mixColor(light, ui::kGold, 0.5f));
  DrawCircleV(c, 18 * k, {11, 18, 48, 255});
  DrawRing(c, 17.2f * k, 18.7f * k, 0, 360, 48, withAlpha(ui::kBlue, 0.55f));
  ui::glow(c, 14 * k, withAlpha(WHITE, 0.35f));
  ui::spark(c, 17 * k * (0.96f + 0.04f * std::sin(t * 3.0f)), 0.0f, WHITE);
}
void dashed(Rectangle r, Color c) {
  for (float x = r.x; x < r.x + r.width; x += 12) {
    const float w = std::min(7.0f, r.x + r.width - x);
    DrawRectangleRec({x, r.y, w, 2}, c);
    DrawRectangleRec({x, r.y + r.height - 2, w, 2}, c);
  }
  for (float y = r.y; y < r.y + r.height; y += 12) {
    const float h = std::min(7.0f, r.y + r.height - y);
    DrawRectangleRec({r.x, y, 2, h}, c);
    DrawRectangleRec({r.x + r.width - 2, y, 2, h}, c);
  }
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
  for (Rectangle& p : g_panel) p = {};
  if (g_wide) {  // the players, then free cards, up to two sheets
    int order[2], k = 0;
    for (int j = 0; j < nj; ++j) order[k++] = joined[j];
    for (int i = 0; i < cols && k < 2; ++i)
      if (!training_ && slots_[i].type == 0) order[k++] = i;
    for (int j = 0; j < k; ++j) g_panel[order[j]] = {j == 0 ? 64.0f : 656.0f, 288, 560, 340};
  } else {
    for (int i = 0; i < cols; ++i) g_panel[i] = {40.0f + i * 300.0f, 300, 290, 360};
  }
  for (int i = 0; i < 4; ++i) {
    const Rectangle p = g_panel[i];
    g_chip[i] = {};
    if (p.width <= 0.0f || !(training_ || slots_[i].type != 0)) continue;
    const char* who = whoOf(training_, slots_[i], i);
    if (g_wide) g_chip[i] = {p.x + (p.x < kScreenW * 0.5f ? 270.0f : 40.0f), p.y + 18, ui::labelWidth(who, 11, 0.04f) + 16, 22};
    else g_chip[i] = {p.x + 16, 518, textWidth(who, 15, kT) + 16, 20};
  }
  const Settings& st = settings();
  g_rulesLine = training_ ? std::string("Entraînement · ") + deviceName(slots_[0].device)
                          : std::string(teamBattle() ? "En équipes" : "Chacun pour soi") + " · " +
                                std::to_string(st.stocks) + (st.stocks > 1 ? " vies" : " vie") +
                                (st.items ? " · Orbe" : " · Sans orbe") + (st.weapons ? " · Armes" : " · Sans armes");
  const float w = 62 + std::max(textWidth("RÈGLES & MODE", 13, FontKind::Display), textWidth(g_rulesLine.c_str(), 12, kU)) + 12 +
                  ui::promptWidth("R", ui::Btn::Y, 24) + 12;
  g_rulesBtn = {kScreenW - 64 - w, 34, w, 58};
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
    const bool outside = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !ui::hovered(kSheet);
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
      const bool leftHalf = GetMousePosition().x < r.x + r.width * 0.5f;
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

  // Mouse: a sheet picks the card the roster tiles go to (its edges: the fighter before / after); a chip changes who
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
      const float mx = GetMousePosition().x;
      const int step = mx < p.x + 48 ? -1 : mx > p.x + p.width - 48 ? 1 : 0;
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

// « Épure »: no plate. The fighter's number, huge and faint, its pixel art over a soft shadow, the arrows; beside it who
// plays it, its number and martial art, its name, its style, its stats in ten segments, its two weapons.
void App::drawWideSheet(int col, const CharacterDef* d, bool left, Color pc, Color cc, int palette, bool chipFocus,
                        bool fighterFocus) {
  const float t = time_;
  const Rectangle p = g_panel[col];
  const SlotSetup& sl = slots_[col];
  constexpr FontKind D = FontKind::Display;
  const std::string num = d ? d->id.substr(1) : std::string("?");
  const Fiche fi = d ? fiche(*d) : Fiche{"ALÉATOIRE", "", "Tiré au sort au début du combat", 0};
  const float in = 1.0f - std::pow(1.0f - clampf((screenFrames_ - 12) / 42.0f, 0.0f, 1.0f), 3.0f);  // slides in
  const float ox = (left ? -50.0f : 50.0f) * (1.0f - in);

  // The huge number, the shadow, the fighter, the arrows
  const float nw = textWidth(num.c_str(), 230, D);
  drawText(num.c_str(), ox + (left ? p.x + 150 : p.x + p.width - 150 - nw), capsY(p.y + 96, 230, D), 230,
           withAlpha(cc, 0.1f * in), D);
  const float bx = ox + (left ? p.x + 118 : p.x + p.width - 112), feet = p.y + p.height - 18;
  ui::glow({bx, feet}, 110, withAlpha(cc, 0.22f * in));
  DrawEllipse(int(bx), int(feet + 2), 100, 6, withAlpha(cc, 0.3f * in));
  if (!d) {
    const float qp = 0.5f + 0.5f * std::sin(t * 3.0f + col);
    drawText("?", bx - textWidth("?", 200, D) * 0.5f, capsY(feet - 150, 200, D), 200, withAlpha(mixColor(pc, WHITE, qp), in), D);
  } else {
    drawShowcase(*d, palette, restPose(*d, t + col), {bx, feet}, left ? 1 : -1, 290.0f, t + col);
  }
  for (int side = -1; fighterFocus && side <= 1; side += 2) {  // the fighter before / after
    const Vector2 c{bx + side * 108.0f, p.y + 170};
    const float a = (fighterFocus ? 1.0f : 0.55f) * in;
    if (fighterFocus) DrawCircleV(c, 20, withAlpha(ui::kCream, 0.08f));
    DrawRing(c, 19.5f, 20.5f, 0, 360, 40, withAlpha(ui::kCream, 0.2f * a / 0.55f));
    ui::icon(side < 0 ? ui::Icon::ChevronL : ui::Icon::ChevronR, c, 18, withAlpha(ui::kCream, a), 2.0f);
  }

  // Who plays it: its chip, how
  const float ix = ox + (left ? p.x + 270 : p.x + 40), top = p.y + 18;
  Rectangle chip = g_chip[col];
  chip.x += ox;
  ui::roundRect(chip, 6, pc, pc);
  const char* who = whoOf(training_, sl, col);
  ui::label(who, chip.x + 8, chip.y + 11, 11, 0.04f, {10, 11, 14, 255});
  if (chipFocus) ui::rim(chip, 6, t);
  const std::string how = upperFr(howOf(training_, sl, col));
  ui::label(how.c_str(), chip.x + chip.width + 10, chip.y + 11, 11, 0.16f, ui::kMuted);

  // The fighter: number and martial art, name, style
  const std::string line = "N° " + num + (fi.art.empty() ? std::string() : " · " + upperFr(fi.art));
  ui::label(line.c_str(), ix, top + 50, 11, 0.16f, ui::kMuted);
  float ns = 44.0f;
  while (ns > 20.0f && textWidth(fi.nick.c_str(), ns, D) > 290) ns -= 1.0f;
  drawText(fi.nick.c_str(), ix, capsY(top + 84, ns, D), ns, ui::kCream, D);
  drawText(fi.style.c_str(), ix, capsY(top + 118, 15, FontKind::Body), 15, ui::kSoft, FontKind::Body);

  // Its stats, ten segments each, growing in
  const float grow = clampf((screenFrames_ - 33) / 36.0f, 0.0f, 1.0f);
  const std::array<int, 5> bars = d ? statBars(*d) : std::array<int, 5>{};
  const Color on = sl.type == 2 ? ui::kSoft : pc;
  for (int k = 0; d && k < 5; ++k) {
    const float y = top + 148 + k * 19.0f;
    ui::label(kStatName[k][0], ix, y, 10, 0.16f, ui::kMuted);
    for (int g = 0; g < 10; ++g) {
      const Rectangle sg{ix + 94 + g * 16.0f, y - 2.5f, 13, 5};
      ui::roundRect(sg, 2, withAlpha(ui::kCream, 0.12f), withAlpha(ui::kCream, 0.12f));
      if (g < bars[k] && g < grow * 10.0f) ui::roundRect(sg, 2, on, on);
    }
    const std::string val = std::to_string(bars[k]);
    drawText(val.c_str(), ix + 272 - textWidth(val.c_str(), 12, FontKind::Body), capsY(y, 12, FontKind::Body), 12, ui::kCream,
             FontKind::Body);
  }

  // Its two weapons, which fall on the arena
  float wx = ix, wy = top + 250;
  for (int ws = 0; d && ws < 2; ++ws) {
    const std::string wn = d->weapons[ws].name;
    const float tw = 9 + 18 + 8 + textWidth(ws == 0 ? "A" : "B", 12, FontKind::Body) + 6 + textWidth(wn.c_str(), 12, FontKind::Body) + 12;
    if (wx > ix && wx + tw > ix + 300) wx = ix, wy += 38;
    const Rectangle wr{wx, wy, tw, 32};
    ui::roundLines(wr, 9, 1.0f, withAlpha(ui::kCream, 0.16f));
    drawWeaponIcon(*d, ws, {wr.x + 18, wr.y + 16}, 18);
    const float lx = wr.x + 35;
    drawText(ws == 0 ? "A" : "B", lx, capsY(wr.y + 16, 12, FontKind::Body), 12, ui::kCream, FontKind::Body);
    drawText(wn.c_str(), lx + textWidth("A", 12, FontKind::Body) + 6, capsY(wr.y + 16, 12, FontKind::Body), 12, ui::kSoft,
             FontKind::Body);
    wx += tw + 8;
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
  auto paletteOf = [&](int col) {  // the second colours when an earlier card plays the same fighter
    int pal = 0;
    for (int j = 0; j < col; ++j)
      if (g_chip[j].width > 0.0f && !slots_[j].random && slots_[j].character == slots_[col].character) pal = 1 - pal;
    return pal;
  };

  // « Épure »: the dusk, nothing else
  ui::dusk();

  // The bottom bar first: its shade must not darken the sheets above it. Its left: what is missing, or a tip.
  bool cpu10 = false;
  for (int col = 0; col < cols; ++col) cpu10 = cpu10 || (slots_[col].type == 2 && slots_[col].level >= kMaxCpuLevel);
  const char* tip = training_ ? "Vies infinies  ·  mannequin réglable en pause"
                    : !ready ? (activeSlots() >= 2 ? "Il faut un combattant dans chaque équipe." : "Il faut au moins 2 combattants.")
                    : !st.items ? "Sans orbe, personne ne pourra lancer son ultime."
                    : cpu10 ? "Niveau 10 : l'ordinateur frappe deux fois plus fort."
                    : st.weapons ? "Tout le monde commence à mains nues." : "Sans armes : tout le combat à mains nues.";
  const ui::Hint go = cssGo(training_);
  ui::footer({{"FLÈCHES", ui::Btn::Move, "Naviguer"}, {"ESPACE", ui::Btn::A, "Choisir"}, {"R", ui::Btn::Y, "Règles & mode"},
              {"ÉCHAP", ui::Btn::B, "Retour"}},
             nullptr, &go, ready);
  drawText(tip, 40, capsY(kScreenH - 38, 13, kU), 13, ready ? ui::kMuted : ui::kRed, kU);

  // A field is focused when a player's cursor is on it, or under the mouse.
  auto cursorOn = [&](int col, int row) {
    for (const CssCursor& c : cssCur_)
      if (c.on && c.row == row && (row < 0 || c.col == col)) return true;
    return false;
  };
  auto focusOn = [&](int col, int row) { return cursorOn(col, row) || ui::hovered(fieldRect(col, row)); };

  // The title; the « Règles & mode » button: its icon, what the rules are now, its key
  {
    const std::string lab = training_ ? std::string("ENTRAÎNEMENT · ") + upperFr(deviceName(slots_[0].device))
                                      : std::string(activeSlots() > 2 ? "VERSUS · " : "COMBAT · ") +
                                            (teamsOn ? "EN ÉQUIPES" : "CHACUN POUR SOI");
    ui::label(lab.c_str(), 64, 46, 11, 0.16f, ui::kMuted);
    drawText("COMBATTANTS", 64, capsY(74, 28, FontKind::Display), 28, ui::kCream, FontKind::Display);
    const Rectangle b = g_rulesBtn;
    const bool hot = focusOn(0, -1);
    if (hot) ui::roundRect(b, 16, withAlpha(ui::kCream, 0.06f), withAlpha(ui::kCream, 0.06f));
    ui::roundLines(b, 16, 1.0f, withAlpha(ui::kCream, hot ? 0.35f : 0.16f));
    const Rectangle ib{b.x + 10, b.y + 10, 38, 38};
    ui::roundRect(ib, 11, ui::kCream, ui::kCream);
    ui::icon(ui::Icon::Sliders, {ib.x + 19, ib.y + 19}, 20, ui::kInk, 1.9f);
    drawText("RÈGLES & MODE", b.x + 62, capsY(b.y + 21, 13, FontKind::Display), 13, ui::kCream, FontKind::Display);
    drawText(g_rulesLine.c_str(), b.x + 62, capsY(b.y + 39, 12, kU), 12, ui::kMuted, kU);
    ui::prompt("R", ui::Btn::Y, b.x + b.width - 12 - ui::promptWidth("R", ui::Btn::Y, 24), b.y + 17, 24);
    if (cursorOn(0, -1)) ui::rim(b, 16, t);
  }

  // The roster: each face in a round tile, its number and name over a dark fade; who picked it in the corner, a ring of
  // their colour.
  constexpr float kTileR = 14;
  for (int i = 0; i <= int(r.size()); ++i) {
    const Rectangle rr = rosterRect(i);
    const bool rnd = i == int(r.size());
    int pickers[4], np = 0;
    for (int c = 0; c < cols; ++c) {
      const SlotSetup& sl = slots_[c];
      if (g_chip[c].width > 0.0f && (rnd ? sl.random : !sl.random && sl.character == i)) pickers[np++] = c;
    }
    const bool hov = ui::hovered(rr);
    if (rnd) {
      ui::roundLines(rr, kTileR, 1.0f, withAlpha(ui::kCream, hov ? 0.4f : 0.22f));
      ui::icon(ui::Icon::Dice, {rr.x + rr.width * 0.5f, rr.y + 50}, 28, ui::kSoft, 1.6f);
      const float lw = ui::labelWidth("HASARD", 11, 0.1f);
      ui::label("HASARD", rr.x + (rr.width - lw) * 0.5f, rr.y + 82, 11, 0.1f, ui::kSoft);
    } else {
      const CharacterDef& d = r[i];
      ui::roundRect(rr, kTileR, hov ? Color{46, 43, 94, 255} : Color{36, 34, 79, 255}, hov ? Color{46, 43, 94, 255} : Color{36, 34, 79, 255});
      Vector2 pts[24];  // the tile's round outline, for the textured picture
      for (int k = 0, n = 0; k < 4; ++k)
        for (int j = 0; j < 6; ++j, ++n) {
          const float a = (k - 1 + j / 5.0f) * kPi * 0.5f;
          const float cx = k == 0 || k == 1 ? rr.x + rr.width - kTileR : rr.x + kTileR;
          const float cy = k == 1 || k == 2 ? rr.y + rr.height - kTileR : rr.y + kTileR;
          pts[n] = {cx + std::cos(a) * kTileR, cy + std::sin(a) * kTileR};
        }
      float aw, ah;
      Vec2 face;
      if (artGeometry(d, aw, ah, face)) {
        const float k = rr.height / (0.82f * ah);  // the whole head and its shoulders, the picture centred
        drawArtPoly(d, pts, 24, {rr.x + rr.width * 0.5f + (face.x - aw * 0.5f) * k, rr.y + (face.y - 0.03f * ah) * k}, k);
      } else if (art::pictureSize((d.id + "_portrait").c_str(), aw, ah)) {  // the pixel portrait, as wide as the tile
        const float k = rr.width * 1.15f / aw;
        Vector2 src[24];
        for (int j = 0; j < 24; ++j)
          src[j] = {aw * 0.5f + (pts[j].x - rr.x - rr.width * 0.5f) / k, ah * 0.08f + (pts[j].y - rr.y) / k};
        art::drawPicturePoly((d.id + "_portrait").c_str(), src, pts, 24);
      }
      ui::roundRect({rr.x, rr.y + rr.height - 50, rr.width, 50}, kTileR, withAlpha(ui::kDeep, 0.0f), withAlpha(ui::kDeep, 0.94f));
      const std::string nick = fiche(d).nick;
      float fs = 11.0f;
      while (fs > 7.0f && ui::labelWidth(nick.c_str(), fs, 0.06f) > rr.width - 14) fs -= 0.5f;
      ui::label(d.id.substr(1).c_str(), rr.x + 8, rr.y + rr.height - 29, 10, 0.08f, ui::kMuted);
      ui::label(nick.c_str(), rr.x + 8, rr.y + rr.height - 13, fs, 0.06f, ui::kCream);
    }
    float tx = rr.x + rr.width - 6;
    for (int k = 0; k < np; ++k) {
      const char* who = whoOf(training_, slots_[pickers[k]], pickers[k]);
      const float tw = ui::labelWidth(who, 10, 0.06f) + 12;
      tx -= tw;
      ui::roundRect({tx, rr.y + 6, tw, 18}, 5, tint(pickers[k]), tint(pickers[k]));
      ui::label(who, tx + 6, rr.y + 15, 10, 0.06f, {10, 11, 14, 255});
      tx -= 3;
    }
    if (np > 0) ui::roundLines(ui::grow(rr, -1.0f), kTileR - 1, 2.0f, tint(pickers[0]));
  }
  {
    const char* help = "← → ou un clic pour choisir  ·  le dé : un combattant au hasard";
    drawText(help, (kScreenW - textWidth(help, 12, kU)) * 0.5f, capsY(259, 12, kU), 12, ui::kMuted, kU);
  }

  // The sheets: the fighter's colour and number, its portrait standing out of it, who plays it, its stats, its weapons
  for (int col = 0; col < cols; ++col) {
    const Rectangle p = g_panel[col];
    if (p.width <= 0.0f) continue;
    const SlotSetup& sl = slots_[col];
    const Color pc = tint(col);
    const float pcx = p.x + p.width * 0.5f, pcy = p.y + p.height * 0.5f;
    if (g_chip[col].width <= 0.0f) {  // a free card: join it
      const bool foc = focusOn(col, 0);
      DrawRectangleRec(p, withAlpha(ui::kNavyDeep, foc ? 0.75f : 0.55f));
      dashed(p, withAlpha(pc, 0.53f));
      const char* who = TextFormat("J%d", col + 1);
      drawText(who, pcx - textWidth(who, 72, kT) * 0.5f, capsY(pcy - 40, 72, kT), 72, withAlpha(pc, 0.55f), kT);
      const float kw = ui::promptWidth("ESPACE", ui::Btn::A, 24), px = pcx - (kw + 8 + textWidth("Rejoindre", 15)) * 0.5f;
      ui::prompt("ESPACE", ui::Btn::A, px, pcy + 8, 24);
      drawText("Rejoindre", px + kw + 8, capsY(pcy + 20, 15, kU), 15, ui::kSky, kU);
      drawTextCentered("ou A sur une manette", pcx, pcy + 42, 12, ui::kMuted, false);
      if (foc) ui::rim(p, 0, t);
      continue;
    }
    const CharacterDef* d = sl.random ? nullptr : &r[sl.character];
    const Color cc = d ? auraColor(d->hero) : kNoFighter, lit = mixColor(kPanel, cc, 0.33f);
    const bool left = p.x < kScreenW * 0.5f;
    if (g_wide) {
      drawWideSheet(col, d, left, pc, cc, paletteOf(col), focusOn(col, 0), focusOn(col, 1));
      continue;
    }
    const float cut = g_wide ? 22.0f : 16.0f;
    if (g_wide) ui::bevel(p, cut, left ? lit : kPanelDeep, left ? kPanelDeep : lit, true);
    else ui::bevel(p, cut, lit, kPanelDeep);
    DrawRectangleRec({p.x + cut, p.y, p.width - cut, 3}, pc);
    const std::string num = d ? d->id.substr(1) : std::string("?");
    const Fiche fi = d ? fiche(*d) : Fiche{"ALÉATOIRE", "", "Tiré au sort au début du combat", 0};

    // The fighter: wide, beside the sheet's text; narrow, on top, cut where the text starts.
    const float bx = g_wide ? (left ? p.x + 137.5f : p.x + p.width - 137.5f) : pcx;
    const float bustH = g_wide ? 400.0f : 355.0f, bottom = g_wide ? p.y + p.height : 621.0f;
    if (g_wide) {
      beginClip(p);
      const float nw = textWidth(num.c_str(), 230, kT);
      drawText(num.c_str(), left ? p.x + 10 : p.x + p.width - 10 - nw, capsY(p.y + 66, 230, kT), 230, withAlpha(cc, 0.16f), kT);
      endClip();
    } else {
      beginClip({p.x - 40, 0, p.width + 80, 550});
    }
    ui::glow({bx, bottom - bustH * 0.45f}, bustH * 0.55f, withAlpha(cc, 0.3f));
    if (!d) {
      const float qs = g_wide ? 220.0f : 160.0f, qp = 0.5f + 0.5f * std::sin(t * 3.0f + col);
      drawText("?", bx - textWidth("?", qs, kT) * 0.5f, capsY(bottom - bustH * 0.42f, qs, kT), qs, mixColor(pc, WHITE, qp), kT);
    } else {  // the fighter itself, in pixel art (its portrait is on its roster tile)
      drawShowcase(*d, paletteOf(col), restPose(*d, t + col), {bx, g_wide ? p.y + p.height - 16 : 545.0f}, left || !g_wide ? 1 : -1,
                   g_wide ? 290.0f : 220.0f, t + col);
    }
    if (!g_wide) endClip();
    if (focusOn(col, 1)) {  // the arrows on both sides of the portrait: the fighter before / after
      const float ay = g_wide ? pcy : p.y + 110, ax = g_wide ? 122.0f : p.width * 0.5f - 16;
      ui::arrowTri({bx - ax, ay}, 22, -1, ui::kGold);
      ui::arrowTri({bx + ax, ay}, 22, 1, ui::kGold);
    }

    // Who plays it, then the fighter: number, martial art, name, its stats and its two weapons
    const Rectangle chip = g_chip[col];
    const float ix = chip.x, iw = g_wide ? 270.0f : p.width - 32.0f;
    if (!g_wide) {  // the text on a dark fade over the bottom of the portrait (the cut corner stays cut)
      const Color ink = withAlpha(ui::kInk, 0.92f);
      DrawRectangleGradientV(int(p.x), 470, int(p.width), 84, withAlpha(ui::kInk, 0.0f), ink);
      DrawRectangleRec({p.x, 554, p.width, 106 - cut}, ink);
      DrawRectangleRec({p.x, 660 - cut, p.width - cut, cut}, ink);
      DrawTriangle({p.x + p.width - cut, 660 - cut}, {p.x + p.width - cut, 660}, {p.x + p.width, 660 - cut}, ink);
    }
    ui::slanted(chip, 6, pc, pc);
    const char* who = whoOf(training_, sl, col);
    const float cs = g_wide ? 18.0f : 15.0f;
    drawText(who, chip.x + (chip.width - textWidth(who, cs, kT)) * 0.5f, capsY(chip.y + chip.height * 0.5f, cs, kT), cs, ui::kInk, kT);
    if (focusOn(col, 0)) ui::rim(chip, 0, t);
    const std::string how = howOf(training_, sl, col);
    float hs = g_wide ? 13.0f : 12.0f;
    while (hs > 9.0f && textWidth(how.c_str(), hs) > iw - chip.width - 10) hs -= 0.5f;
    drawText(how.c_str(), chip.x + chip.width + 10, capsY(chip.y + chip.height * 0.5f, hs, kU), hs, ui::kMuted, kU);
    const std::array<int, 5> bars = d ? statBars(*d) : std::array<int, 5>{};
    if (g_wide) {
      const std::string line = "N° " + num + (fi.art.empty() ? std::string() : "  ·  " + upperFr(fi.art));
      ui::tracked(line.c_str(), ix, capsY(383, 15, kT), 15, 0.1f, ui::kSky);
      float ns = 54.0f;
      while (ns > 24.0f && textWidth(fi.nick.c_str(), ns, kT) > iw) ns -= 1.0f;
      drawText(fi.nick.c_str(), ix, capsY(418, ns, kT), ns, ui::kCream, kT);
      drawText(fi.style.c_str(), ix, capsY(453, 14, kU), 14, ui::kSoft, kU);
      for (int k = 0; d && k < 5; ++k) {
        const float y = 478 + k * 16.0f;
        ui::tracked(kStatName[k][0], ix, capsY(y, 13, kT), 13, 0.12f, ui::kMuted);
        for (int g = 0; g < 10; ++g) {
          const Color sc = g < bars[k] ? pc : ui::kTrack;
          ui::slanted({ix + 86 + g * 16.0f, y - 4.5f, 16.3f, 9}, 3.3f, sc, sc);
        }
        const std::string val = std::to_string(bars[k]);
        drawText(val.c_str(), ix + iw - 2 - textWidth(val.c_str(), 15, kT), capsY(y, 15, kT), 15, ui::kCream, kT);
      }
      float wx = ix, wy = 566;
      for (int ws = 0; d && ws < 2; ++ws) {  // the two weapons that will fall on the arena
        const std::string wn = std::string(ws == 0 ? "A  ·  " : "B  ·  ") + d->weapons[ws].name;
        const float tw = textWidth(wn.c_str(), 12) + 38;
        if (wx > ix && wx + tw > ix + iw) wx = ix, wy += 30;
        const Rectangle wr{wx, wy, tw, 24};
        DrawRectangleRec(wr, withAlpha(ui::kInk, 0.6f));
        DrawRectangleRec({wr.x, wr.y + wr.height - 2, wr.width, 2}, pc);
        drawWeaponIcon(*d, ws, {wr.x + 13, wr.y + 11}, 18);
        drawText(wn.c_str(), wr.x + 26, capsY(wr.y + 11, 12, kU), 12, ui::kSoft, kU);
        wx += tw + 8;
      }
    } else {
      float ns = 32.0f;
      while (ns > 16.0f && textWidth(fi.nick.c_str(), ns, kT) > iw) ns -= 1.0f;
      drawText(fi.nick.c_str(), ix, capsY(556, ns, kT), ns, ui::kCream, kT);
      if (!d) drawText(fi.style.c_str(), ix, capsY(586, 13, kU), 13, ui::kSoft, kU);
      for (int k = 0; d && k < 5; ++k) {
        const float y = 585 + k * 10.0f;
        ui::tracked(kStatName[k][1], ix, capsY(y, 12, kT), 12, 0.08f, ui::kMuted);
        for (int g = 0; g < 10; ++g) {
          const Color sc = g < bars[k] ? pc : ui::kTrack;
          ui::slanted({ix + 36 + g * 20.0f, y - 3, 20.2f, 6}, 2.2f, sc, sc);
        }
      }
    }
  }
  if (g_wide && g_chip[0].width + g_chip[1].width + g_chip[2].width + g_chip[3].width > 0.0f) {  // "VS" between the two
    const float cx = kScreenW * 0.5f;
    ui::quad({cx - 0.5f, 300}, {cx + 0.5f, 300}, {cx + 0.5f, 458}, {cx - 0.5f, 458}, withAlpha(ui::kCream, 0.0f),
             withAlpha(ui::kCream, 0.0f), withAlpha(ui::kCream, 0.22f), withAlpha(ui::kCream, 0.22f));
    ui::quad({cx - 0.5f, 458}, {cx + 0.5f, 458}, {cx + 0.5f, 616}, {cx - 0.5f, 616}, withAlpha(ui::kCream, 0.22f),
             withAlpha(ui::kCream, 0.22f), withAlpha(ui::kCream, 0.0f), withAlpha(ui::kCream, 0.0f));
    DrawCircleV({cx, 457}, 21, withAlpha(ui::kCream, 0.22f));
    DrawCircleV({cx, 457}, 20, ui::kDeep);
    drawText("VS", cx - textWidth("VS", 12, FontKind::Display) * 0.5f, capsY(457, 12, FontKind::Display), 12, ui::kCream,
             FontKind::Display);
  }

  // Whose cursor is where: a tag in the player's colour on the corner of the field.
  std::array<int, 12> stacked{};
  for (int dev = 0; dev < kDeviceCount; ++dev) {
    const CssCursor& c = cssCur_[dev];
    if (!c.on) continue;
    const int col = std::clamp(c.col, 0, cols - 1);
    if (c.row >= 0 && g_panel[col].width <= 0.0f) continue;
    const int row = c.row == 1 && g_chip[col].width <= 0.0f ? 0 : c.row;
    const int slot = row < 0 ? 0 : 1 + row * 4 + col;
    const Rectangle f = fieldRect(col, row);
    const int own = ownedCard(dev);
    const char* label = own >= 0 ? TextFormat("J%d", own + 1) : dev == kKeyboard1 ? "C1" : dev == kKeyboard2 ? "C2" : "M";
    const float w = textWidth(label, 14, kT) + 12;
    const Rectangle tag{f.x + (row == 1 ? 24.0f : -6.0f) + stacked[slot] * (w + 4), f.y - 12, w, 20};
    ++stacked[slot];
    ui::roundRect(ui::grow(tag, 1.5f), 6.5f, ui::kDeep, ui::kDeep);
    ui::roundRect(tag, 5, own >= 0 ? tint(own) : ui::kSky, own >= 0 ? tint(own) : ui::kSky);
    drawText(label, tag.x + 6, capsY(tag.y + 10, 14, kT), 14, {10, 11, 14, 255}, kT);
  }
  if (!rulesOpen_) return;

  // ---- The « Règles & mode » sheet, over the dimmed screen -------------------------------------------------------
  SheetItem items[16];
  const int nItems = sheetItems(training_, slots_, teamsOn, items);
  const SheetItem sel = items[std::clamp(rulesSel_, 0, nItems - 1)];
  auto sheetFocus = [&](int col, int row) {
    return (sel.col == col && sel.row == row) || ui::hovered(sheetField(col, row, teamsOn));
  };
  DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(ui::kInk, 0.78f));
  const Rectangle S = kSheet;
  constexpr FontKind kDsp = FontKind::Display, kB = FontKind::Body;
  ui::roundRect(S, 24, kPanel, kPanelDeep);
  ui::roundLines(S, 24, 1.0f, withAlpha(ui::kCream, 0.14f));
  const Rectangle ib{S.x + 34, S.y + 34, 58, 58};  // the button's icon, bigger
  ui::roundRect(ib, 14, ui::kCream, ui::kCream);
  ui::icon(ui::Icon::Sliders, {ib.x + 29, ib.y + 29}, 28, ui::kInk, 2.0f);
  drawText("RÈGLES & MODE", S.x + 112, capsY(S.y + 52, 30, kDsp), 30, ui::kCream, kDsp);
  drawText(training_ ? "Votre clavier ou votre manette pour l'entraînement." : "Ce combat, et qui joue : un humain ou l'ordinateur.",
           S.x + 112, capsY(S.y + 82, 14, kB), 14, ui::kMuted, kB);
  {
    const Rectangle cl = closeRect();
    ui::roundRect(cl, 10, withAlpha(ui::kInk, ui::hovered(cl) ? 0.85f : 0.5f), withAlpha(ui::kInk, ui::hovered(cl) ? 0.85f : 0.5f));
    ui::roundLines(cl, 10, 1.0f, withAlpha(ui::kCream, 0.16f));
    const float kw = ui::prompt("ÉCHAP", ui::Btn::B, cl.x + 10, cl.y + 6, 24);
    drawText("Fermer", cl.x + 18 + kw, capsY(cl.y + 18, 14, kU), 14, ui::kSoft, kU);
  }
  // An inert field: a dark plate, a word on it
  auto inert = [&](Rectangle f, const char* text) {
    ui::roundRect(f, 10, withAlpha(ui::kInk, 0.7f), withAlpha(ui::kInk, 0.7f));
    drawText(text, f.x + (f.width - textWidth(text, 14)) * 0.5f, capsY(f.y + f.height * 0.5f, 14, kU), 14, ui::kMuted, kU);
  };

  // Left: the mode, then the rules
  if (!training_) {
    ui::label("MODE DE JEU", kLeftX, kColY + 8, 11, 0.16f, ui::kMuted);
    const Rectangle mr = modeRect();
    for (int k = 0; k < 2; ++k) {
      const Rectangle c{mr.x + k * (mr.width * 0.5f + 5), mr.y, mr.width * 0.5f - 5, mr.height};
      const bool on = (k == 1) == teamsOn;
      const Color face = ui::hovered(c) ? mixColor(ui::kNavy, WHITE, 0.06f) : ui::kNavy;
      ui::roundRect(c, 14, on ? ui::kCream : face, on ? ui::kCream : face);
      if (!on) ui::roundLines(c, 14, 1.0f, withAlpha(ui::kCream, 0.16f));
      const Color ink = on ? ui::kInk : ui::kSoft;
      const Vector2 ic{c.x + c.width * 0.5f, c.y + 32};
      if (k == 0) {  // each for themselves: three marks; teams: a red shield and a blue one
        for (int j = -1; j <= 1; ++j) ui::diamond({ic.x + j * 10.0f, ic.y}, 6, ink);
      } else {
        for (int j = 0; j < 2; ++j) {
          const Vector2 sc{ic.x + (j == 0 ? -9.5f : 9.5f), ic.y - 2};
          const Color tc = teamColor(j);
          DrawRectangleRec({sc.x - 6.5f, sc.y - 7, 13, 7}, tc);
          DrawTriangle({sc.x - 6.5f, sc.y}, {sc.x, sc.y + 9}, {sc.x + 6.5f, sc.y}, tc);
        }
      }
      const char* lab = k == 0 ? "CHACUN POUR SOI" : "EN ÉQUIPES";
      drawText(lab, ic.x - textWidth(lab, 11, kDsp) * 0.5f, capsY(c.y + 66, 11, kDsp), 11, ink, kDsp);
    }
    if (sheetFocus(3, -1)) ui::rim(mr, 14, t);
    DrawRectangleRec({kLeftX, kColY + 138, kLeftW, 1}, withAlpha(ui::kSky, 0.12f));
    const char* labels[3] = {"VIES", "CLASH ORB", "ARMES"};
    const char* helps[3] = {"Par combattant", "La seule source des ultimes", "Elles tombent sur l'arène"};
    for (int i = 0; i < 3; ++i) {
      const Rectangle rr = ruleRowRect(i);
      const Color rc = withAlpha(ui::kInk, ui::hovered(rr) ? 0.65f : 0.45f);
      ui::roundRect(rr, 12, rc, rc);
      drawText(labels[i], rr.x + 14, capsY(rr.y + 17, 13, kDsp), 13, ui::kCream, kDsp);
      drawText(helps[i], rr.x + 14, capsY(rr.y + 35, 12, kB), 12, ui::kMuted, kB);
      const bool on = i == 0 || (i == 1 ? st.items : st.weapons);
      const std::string val = i == 0 ? std::to_string(st.stocks) : on ? "OUI" : "NON";
      const float vx = rr.x + rr.width - 54, vy = rr.y + rr.height * 0.5f;
      ui::arrowTri({vx - 36, vy}, 13, -1, ui::kGold);
      ui::arrowTri({vx + 36, vy}, 13, 1, ui::kGold);
      drawText(val.c_str(), vx - textWidth(val.c_str(), 16, kDsp) * 0.5f, capsY(vy, 16, kDsp), 16,
               i == 0 ? ui::kCream : on ? kGoldLight : ui::kMuted, kDsp);
      if (sheetFocus(i, -1)) ui::rim(rr, 12, t);
    }
  } else {
    ui::label("ENTRAÎNEMENT", kLeftX, kColY + 8, 11, 0.16f, ui::kMuted);
    ui::drawWrapped("Vies infinies, pas de minuteur. Le mannequin se règle dans le menu pause.", kLeftX, kColY + 30, kLeftW,
                    16, ui::kSoft);
  }

  // Right: who plays, a human (on which device) or the computer (at which level), in which team
  auto toggle = [&](Rectangle f, int type, bool foc) {
    ui::roundRect(f, 10, withAlpha(ui::kInk, 0.7f), withAlpha(ui::kInk, 0.7f));
    ui::roundLines(f, 10, 1.0f, withAlpha(ui::kCream, 0.16f));
    for (int k = 0; k < 3; ++k) {
      const Rectangle g = toggleSeg(f, k);
      const bool on = type != 0 && type == kSegType[k];
      if (on) ui::roundRect(ui::grow(g, -3.0f), 8, ui::kCream, ui::kCream);
      else if (ui::hovered(g)) ui::roundRect(ui::grow(g, -3.0f), 8, withAlpha(WHITE, 0.08f), withAlpha(WHITE, 0.08f));
      const Color ink = on ? ui::kInk : ui::kMuted;
      if (k < 2) {
        const char* lab = k == 0 ? "HUMAIN" : "IA";
        drawText(lab, g.x + (g.width - textWidth(lab, 10, kDsp)) * 0.5f, capsY(g.y + g.height * 0.5f, 10, kDsp), 10, ink, kDsp);
      } else {  // nobody: the card is free again
        const Vector2 m{g.x + g.width * 0.5f, g.y + g.height * 0.5f};
        DrawLineEx({m.x - 5, m.y - 5}, {m.x + 5, m.y + 5}, 2.0f, ink);
        DrawLineEx({m.x - 5, m.y + 5}, {m.x + 5, m.y - 5}, 2.0f, ink);
      }
    }
    if (foc) ui::rim(f, 10, t);
  };
  ui::label("JOUEURS", kRightX, kColY + 8, 11, 0.16f, ui::kMuted);
  for (int c = 0; c < cols; ++c) {
    const Rectangle rw = playerRowRect(c);
    const SlotSetup& sl = slots_[c];
    const int type = training_ ? (c == 0 ? 1 : 2) : sl.type;
    const Color pc = tint(c);
    const float cy = rw.y + rw.height * 0.5f;
    ui::roundRect(rw, 12, withAlpha(ui::kInk, 0.45f), withAlpha(ui::kInk, 0.45f));
    DrawRectangleRec({rw.x, rw.y + 16, 3, rw.height - 32}, pc);
    const char* who = training_ && c == 1 ? "CIBLE" : TextFormat("J%d", c + 1);
    float ws = 16.0f;
    while (ws > 9.0f && textWidth(who, ws, kDsp) > 36) ws -= 0.5f;
    drawText(who, rw.x + 12, capsY(cy, ws, kDsp), ws, pc, kDsp);
    const Rectangle f0 = sheetField(c, 0, teamsOn), f2 = sheetField(c, 2, teamsOn);
    if (type == 0) {  // a free card
      if (!teamsOn) drawText("Place libre", rw.x + 64, capsY(cy, 15, kU), 15, ui::kMuted, kU);
      toggle(f0, 0, sheetFocus(c, 0));
      drawText(teamsOn ? "Place libre  ·  A sur une manette" : "ou A sur une manette", f2.x, capsY(cy, 13, kU), 13, ui::kMuted, kU);
      continue;
    }
    const CharacterDef* d = sl.random ? nullptr : &r[sl.character];
    const Vector2 fc{rw.x + 74, cy};
    DrawCircleV(fc, 24, pc);
    DrawCircleV(fc, 22, d ? auraColor(d->hero) : kNoFighter);
    if (d) drawArtDisc(*d, paletteOf(c), {fc.x, fc.y}, 22, t);
    else drawText("?", fc.x - textWidth("?", 26, kT) * 0.5f, capsY(fc.y, 26, kT), 26, ui::kSky, kT);
    if (!teamsOn) {
      const std::string nick = d ? fiche(*d).nick : std::string("ALÉATOIRE");
      float ns = 14.0f;
      while (ns > 9.0f && textWidth(nick.c_str(), ns, kB) > 90) ns -= 0.5f;
      drawText(nick.c_str(), rw.x + 104, capsY(cy, ns, kB), ns, ui::kCream, kB);
    }
    if (training_) inert(f0, c == 0 ? "VOUS" : "CIBLE");
    else toggle(f0, type, sheetFocus(c, 0));
    if (type == 1) deviceSelector(f2, sl.device, sheetFocus(c, 2), t);
    else if (!training_) ui::selector(f2, sl.level >= kMaxCpuLevel ? "NIV. 10 x2" : TextFormat("NIVEAU %d", sl.level), sheetFocus(c, 2), t);
    else inert(f2, "réglable en pause");
    if (teamsOn) {
      const Rectangle f3 = sheetField(c, 3, teamsOn);
      const bool foc = sheetFocus(c, 3);
      const char* tn = teamName(slotTeam(c));
      const Color tc = foc ? mixColor(pc, WHITE, 0.3f) : pc;
      ui::roundRect(f3, 10, tc, tc);
      drawText(tn, f3.x + (f3.width - textWidth(tn, 10, kDsp)) * 0.5f, capsY(f3.y + f3.height * 0.5f, 10, kDsp), 10, ui::kInk, kDsp);
      if (foc) ui::rim(f3, 10, t);
    }
  }
  if (!training_)
    drawText("IA : 9 niveaux, puis « 10 x2 » qui frappe deux fois plus fort.", kRightX,
             capsY(playerRowRect(cols - 1).y + 80, 13, kU), 13, ui::kMuted, kU);

  // The foot: what the rules mean (or what is missing), and done
  drawText(tip, S.x + 34, capsY(S.y + S.height - 48, 14, kB), 14, ready ? kGoldLight : ui::kRed, kB);
  const Rectangle vr = validateRect();
  const Color vc = ui::hovered(vr) ? WHITE : ui::kCream;
  ui::roundRect(vr, 14, vc, vc);
  const float kw = ui::prompt("ENTRÉE", ui::Btn::Start, vr.x + 10, vr.y + 10, 24);
  drawText("VALIDER", vr.x + 20 + kw, capsY(vr.y + 22, 15, kDsp), 15, ui::kInk, kDsp);
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
