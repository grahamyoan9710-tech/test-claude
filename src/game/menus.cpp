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
constexpr Color kPanel{18, 26, 62, 255}, kPanelDeep{12, 19, 48, 255}, kGoldLight{255, 200, 112, 255};
constexpr Color kNoFighter{42, 53, 88, 255};  // "?": a fighter drawn at random

// The roster on top, tall tiles slanted like the hits, each fighter's face; under it a sheet per player, its portrait
// standing out of it, its stats: two wide sheets, or four narrow ones past two players. The mode, the rules and who
// plays (a human or the computer) are on the « Règles & mode » sheet, behind the button top right.
Rectangle g_panel[4];     // each card's sheet, set each frame by App::layoutCss (width 0: not on screen)
Rectangle g_chip[4];      // its player chip (width 0: a free card, its whole sheet joins it)
Rectangle g_rulesBtn;     // the « Règles & mode » button
bool g_wide = true;       // two wide sheets
std::string g_rulesLine;  // what the button says of the rules
constexpr float kTileW = 104, kTileH = 152, kTileGap = 6, kTileY = 92, kTileSlant = kTileH * 0.1405f;  // tan 8°

// Roster tile i, as the box of its slanted shape (ui::slanted with kTileSlant); "?" (a fighter at random) last.
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
  const float w = 40 + ui::promptWidth("ENTRÉE", ui::Btn::Start, 24) + textWidth("VALIDER", 21, kT);
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
    for (int j = 0; j < k; ++j) g_panel[order[j]] = {j == 0 ? 40.0f : 680.0f, 318, 560, 336};
  } else {
    for (int i = 0; i < cols; ++i) g_panel[i] = {40.0f + i * 300.0f, 300, 290, 360};
  }
  for (int i = 0; i < 4; ++i) {
    const Rectangle p = g_panel[i];
    g_chip[i] = {};
    if (p.width <= 0.0f || !(training_ || slots_[i].type != 0)) continue;
    const char* who = whoOf(training_, slots_[i], i);
    if (g_wide) g_chip[i] = {p.x + (p.x < kScreenW * 0.5f ? 282.0f : 26.0f), 340, textWidth(who, 18, kT) + 22, 24};
    else g_chip[i] = {p.x + 16, 518, textWidth(who, 15, kT) + 16, 20};
  }
  const Settings& st = settings();
  g_rulesLine = training_ ? std::string("Entraînement · ") + deviceName(slots_[0].device)
                          : std::string(teamBattle() ? "En équipes" : "Chacun pour soi") + " · " +
                                std::to_string(st.stocks) + (st.stocks > 1 ? " vies" : " vie") +
                                (st.items ? " · Orbe" : " · Sans orbe") + (st.weapons ? " · Armes" : " · Sans armes");
  const float w = 68 + std::max(textWidth("RÈGLES & MODE", 20, kT), textWidth(g_rulesLine.c_str(), 13)) + 14 +
                  ui::promptWidth("R", ui::Btn::Y, 24) + 16;
  g_rulesBtn = {kScreenW - 40 - w, 14, w, 60};
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

  // The night, azure light on the left and amber on the right, speed lines slanted like the hits, a screentone band
  constexpr Color kSkyTop{17, 27, 69, 255}, kSkyMid{10, 17, 49, 255};
  DrawRectangleGradientV(0, 0, kScreenW, 331, kSkyTop, kSkyMid);
  DrawRectangleGradientV(0, 331, kScreenW, kScreenH - 331, kSkyMid, ui::kInk);
  ui::glow({150, 470}, 410, withAlpha(ui::kBlue, 0.3f));
  ui::glow({1130, 470}, 410, withAlpha(ui::kGold, 0.24f));
  for (float x = 0; x < kScreenW + 130; x += 23) DrawLineEx({x, 0}, {x - 127, float(kScreenH)}, 2, withAlpha(WHITE, 0.035f));
  screentone({0, 80, float(kScreenW), 186}, withAlpha(ui::kSky, 0.09f));

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
  drawText(tip, 28, kScreenH - 33, 16, ready ? ui::kMuted : kGoldLight);

  // A field is focused when a player's cursor is on it, or under the mouse.
  auto cursorOn = [&](int col, int row) {
    for (const CssCursor& c : cssCur_)
      if (c.on && c.row == row && (row < 0 || c.col == col)) return true;
    return false;
  };
  auto focusOn = [&](int col, int row) { return cursorOn(col, row) || ui::hovered(fieldRect(col, row)); };

  // The title; the « Règles & mode » button, its emblem and what the rules are now
  const char* title = training_ ? "ENTRAÎNEMENT" : teamsOn ? "COMBAT EN ÉQUIPES" : "CHOIX DES COMBATTANTS";
  ui::tracked(title, 40, capsY(36, 30, kT), 30, 0.05f, ui::kCream);
  ui::horizonLine(40, 340, 60, 3.0f);
  {
    const Rectangle b = g_rulesBtn;
    const bool hot = focusOn(0, -1);
    for (int k = 2; k >= 1; --k)  // a soft gold halo
      ui::roundRect(ui::grow(b, 3.0f * k), 30 + 3.0f * k, withAlpha(ui::kGold, hot ? 0.09f : 0.05f), withAlpha(ui::kGold, hot ? 0.09f : 0.05f));
    ui::roundRect(b, 30, {20, 29, 64, 242}, {10, 16, 38, 242});
    ui::roundLines(b, 30, 1.0f, withAlpha(ui::kGold, hot ? 0.95f : 0.55f));
    emblem({b.x + 31, b.y + 30}, 25, t);
    drawText("RÈGLES & MODE", b.x + 68, capsY(b.y + 22, 20, kT), 20, ui::kCream, kT);
    drawText(g_rulesLine.c_str(), b.x + 68, capsY(b.y + 41, 13, kU), 13, ui::kMuted, kU);
    ui::prompt("R", ui::Btn::Y, b.x + b.width - 16 - ui::promptWidth("R", ui::Btn::Y, 24), b.y + 18, 24);
    if (cursorOn(0, -1)) ui::rim(b, 14, t);
  }

  // The roster: each face in a tall slanted tile, its colour along the bottom, its number and name; who picked it in
  // the corner.
  for (int i = 0; i <= int(r.size()); ++i) {
    const Rectangle rr = rosterRect(i);
    const bool rnd = i == int(r.size());
    int pickers[4], np = 0;
    for (int c = 0; c < cols; ++c) {
      const SlotSetup& sl = slots_[c];
      if (g_chip[c].width > 0.0f && (rnd ? sl.random : !sl.random && sl.character == i)) pickers[np++] = c;
    }
    const Color cc = rnd ? kNoFighter : auraColor(r[i].hero);
    const float s = kTileSlant, w = rr.width - s, cx = rr.x + s * 0.5f + w * 0.5f;
    if (rnd) {
      tileBand(rr, 0, 1, ui::kNavyDeep, ui::kNavyDeep);
      ui::slantedOutline(rr, s, 1.0f, withAlpha(ui::kSky, 0.22f));
      drawText("?", cx - textWidth("?", 64, kT) * 0.5f, capsY(rr.y + 62, 64, kT), 64, ui::kSky, kT);
      ui::tracked("AU HASARD", cx - ui::trackedWidth("AU HASARD", 13, 0.12f) * 0.5f, capsY(rr.y + 104, 13, kT), 13, 0.12f,
                  ui::kMuted);
    } else {
      const CharacterDef& d = r[i];
      tileBand(rr, 0, 0.7f, mixColor(ui::kNavy, cc, 0.25f), ui::kNavy);
      tileBand(rr, 0.7f, 1, ui::kNavy, ui::kNavy);
      const Vector2 pts[4] = {{rr.x + s, rr.y}, {rr.x + rr.width, rr.y}, {rr.x + rr.width - s, rr.y + rr.height},
                              {rr.x, rr.y + rr.height}};
      float aw, ah;
      Vec2 face;
      if (artGeometry(d, aw, ah, face)) {
        const float k = rr.height / (0.82f * ah);  // the whole head and its shoulders, the picture centred
        drawArtPoly(d, pts, 4, {rr.x + rr.width * 0.5f + (face.x - aw * 0.5f) * k, rr.y + (face.y - 0.03f * ah) * k}, k);
      } else if (art::pictureSize((d.id + "_portrait").c_str(), aw, ah)) {  // the pixel portrait, as tall as the tile
        const float k = rr.height / std::max(aw, ah);
        Vector2 src[4];
        for (int j = 0; j < 4; ++j)
          src[j] = {aw * 0.5f + (pts[j].x - rr.x - rr.width * 0.5f) / k, ah * 0.5f + (pts[j].y - rr.y - rr.height * 0.5f) / k};
        art::drawPicturePoly((d.id + "_portrait").c_str(), src, pts, 4);
      }
      tileBand(rr, 1.0f - 46.0f / kTileH, 1, withAlpha(ui::kInk, 0.0f), withAlpha(ui::kInk, 0.92f));
      const std::string nick = fiche(d).nick;
      float fs = 15.0f;
      while (fs > 10.0f && ui::trackedWidth(nick.c_str(), fs, 0.06f) > w - 8) fs -= 0.5f;
      const float nx = rr.x + s * 0.1f + w * 0.5f;
      ui::tracked(nick.c_str(), nx - ui::trackedWidth(nick.c_str(), fs, 0.06f) * 0.5f, capsY(rr.y + rr.height - 17, fs, kT), fs,
                  0.06f, ui::kCream);
      ui::bold(d.id.substr(1).c_str(), rr.x + s + 6, capsY(rr.y + 14, 17, kT), 17);
    }
    tileBand(rr, 1.0f - 4.0f / kTileH, 1, cc, cc);
    if (ui::hovered(rr)) tileBand(rr, 0, 1, withAlpha(WHITE, 0.1f), withAlpha(WHITE, 0.1f));
    float tx = rr.x + rr.width - 4;
    for (int k = 0; k < np; ++k) {
      const char* who = whoOf(training_, slots_[pickers[k]], pickers[k]);
      const float tw = textWidth(who, 13, kT) + 12;
      tx -= tw;
      DrawRectangleRec({tx, rr.y + 4, tw, 18}, tint(pickers[k]));
      drawText(who, tx + 6, capsY(rr.y + 13, 13, kT), 13, ui::kInk, kT);
      tx -= 3;
    }
    if (np > 0) ui::slantedOutline({rr.x + 1.5f, rr.y + 1.5f, rr.width - 3, rr.height - 3}, s, 3.0f, tint(pickers[0]));
  }
  drawTextCentered("← → ou un clic pour choisir  ·  « ? » : un combattant au hasard", kScreenW * 0.5f, 252, 14, ui::kMuted,
                   false);

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
    ui::eclat({kScreenW * 0.5f, 472}, 12, 0.7f);
    const float vw = textWidth("VS", 64, kT);
    ui::gradientText("VS", kScreenW * 0.5f - vw * 0.5f, capsY(474, 64, kT), 64, {143, 193, 255, 255}, WHITE, kGoldLight);
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
    DrawRectangleRec(ui::grow(tag, 1.5f), ui::kInk);
    DrawRectangleRec(tag, own >= 0 ? tint(own) : ui::kSky);
    drawText(label, tag.x + 6, capsY(tag.y + 10, 14, kT), 14, ui::kInk, kT);
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
  ui::bevel(S, 26, {22, 32, 74, 255}, kPanelDeep);
  screentone({S.x + 26, S.y, S.width - 26, 120}, withAlpha(ui::kSky, 0.07f));
  ui::bevelLines(ui::grow(S, -0.5f), 26, 1.0f, withAlpha(ui::kSky, 0.18f));
  ui::horizonLine(S.x + 26, S.x + S.width, S.y + 1.5f, 3.0f);
  emblem({S.x + 72, S.y + 64}, 38, t);
  ui::tracked("RÈGLES & MODE", S.x + 126, capsY(S.y + 52, 38, kT), 38, 0.05f, ui::kCream);
  drawText(training_ ? "Votre clavier ou votre manette pour l'entraînement." : "Ce combat, et qui joue : un humain ou l'ordinateur.",
           S.x + 126, capsY(S.y + 84, 15, kU), 15, ui::kMuted, kU);
  {
    const Rectangle cl = closeRect();
    DrawRectangleRec(cl, withAlpha(ui::kInk, ui::hovered(cl) ? 0.85f : 0.5f));
    DrawRectangleLinesEx(cl, 1.0f, withAlpha(ui::kSky, 0.25f));
    const float kw = ui::prompt("ÉCHAP", ui::Btn::B, cl.x + 10, cl.y + 6, 24);
    drawText("Fermer", cl.x + 18 + kw, capsY(cl.y + 18, 14, kU), 14, ui::kSoft, kU);
  }
  // An inert field: a dark plate, a word on it
  auto inert = [&](Rectangle f, const char* text) {
    ui::bevel(f, 8, withAlpha(ui::kInk, 0.7f), withAlpha(ui::kInk, 0.7f));
    drawText(text, f.x + (f.width - textWidth(text, 14)) * 0.5f, capsY(f.y + f.height * 0.5f, 14, kU), 14, ui::kMuted, kU);
  };

  // Left: the mode, then the rules
  if (!training_) {
    ui::tracked("MODE DE JEU", kLeftX, capsY(kColY + 8, 14, kT), 14, 0.16f, ui::kMuted);
    const Rectangle mr = modeRect();
    for (int k = 0; k < 2; ++k) {
      const Rectangle c{mr.x + k * (mr.width * 0.5f + 5), mr.y, mr.width * 0.5f - 5, mr.height};
      const bool on = (k == 1) == teamsOn;
      const Color face = ui::hovered(c) ? mixColor(ui::kNavy, WHITE, 0.06f) : ui::kNavy;
      ui::bevel(c, 12, on ? kGoldLight : face, on ? ui::kGold : face);
      if (!on) ui::bevelLines(ui::grow(c, -0.5f), 12, 1.0f, withAlpha(ui::kSky, 0.2f));
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
      drawText(lab, ic.x - textWidth(lab, 18, kT) * 0.5f, capsY(c.y + 66, 18, kT), 18, ink, kT);
    }
    if (sheetFocus(3, -1)) ui::rim(mr, 12, t);
    DrawRectangleRec({kLeftX, kColY + 138, kLeftW, 1}, withAlpha(ui::kSky, 0.12f));
    const char* labels[3] = {"VIES", "CLASH ORB", "ARMES"};
    const char* helps[3] = {"Par combattant", "La seule source des ultimes", "Elles tombent sur l'arène"};
    for (int i = 0; i < 3; ++i) {
      const Rectangle rr = ruleRowRect(i);
      DrawRectangleRec(rr, withAlpha(ui::kInk, ui::hovered(rr) ? 0.65f : 0.45f));
      drawText(labels[i], rr.x + 14, capsY(rr.y + 17, 19, kT), 19, ui::kCream, kT);
      drawText(helps[i], rr.x + 14, capsY(rr.y + 35, 12, kU), 12, ui::kMuted, kU);
      const bool on = i == 0 || (i == 1 ? st.items : st.weapons);
      const std::string val = i == 0 ? std::to_string(st.stocks) : on ? "OUI" : "NON";
      const float vx = rr.x + rr.width - 54, vy = rr.y + rr.height * 0.5f;
      ui::arrowTri({vx - 36, vy}, 13, -1, ui::kGold);
      ui::arrowTri({vx + 36, vy}, 13, 1, ui::kGold);
      drawText(val.c_str(), vx - textWidth(val.c_str(), 20, kT) * 0.5f, capsY(vy, 20, kT), 20,
               i == 0 ? ui::kCream : on ? kGoldLight : ui::kMuted, kT);
      if (sheetFocus(i, -1)) ui::rim(rr, 0, t);
    }
  } else {
    ui::tracked("ENTRAÎNEMENT", kLeftX, capsY(kColY + 8, 14, kT), 14, 0.16f, ui::kMuted);
    ui::drawWrapped("Vies infinies, pas de minuteur. Le mannequin se règle dans le menu pause.", kLeftX, kColY + 30, kLeftW,
                    16, ui::kSoft);
  }

  // Right: who plays, a human (on which device) or the computer (at which level), in which team
  auto toggle = [&](Rectangle f, int type, bool foc) {
    DrawRectangleRec(f, {10, 16, 40, 255});
    DrawRectangleLinesEx(f, 1.0f, withAlpha(ui::kSky, 0.18f));
    for (int k = 0; k < 3; ++k) {
      const Rectangle g = toggleSeg(f, k);
      const bool on = type != 0 && type == kSegType[k];
      if (on) DrawRectangleRec(ui::grow(g, -2.0f), ui::kGold);
      else if (ui::hovered(g)) DrawRectangleRec(ui::grow(g, -2.0f), withAlpha(WHITE, 0.08f));
      const Color ink = on ? ui::kInk : ui::kMuted;
      if (k < 2) {
        const char* lab = k == 0 ? "HUMAIN" : "IA";
        drawText(lab, g.x + (g.width - textWidth(lab, 14, kT)) * 0.5f, capsY(g.y + g.height * 0.5f, 14, kT), 14, ink, kT);
      } else {  // nobody: the card is free again
        const Vector2 m{g.x + g.width * 0.5f, g.y + g.height * 0.5f};
        DrawLineEx({m.x - 5, m.y - 5}, {m.x + 5, m.y + 5}, 2.0f, ink);
        DrawLineEx({m.x - 5, m.y + 5}, {m.x + 5, m.y - 5}, 2.0f, ink);
      }
    }
    if (foc) ui::rim(f, 0, t);
  };
  ui::tracked("JOUEURS", kRightX, capsY(kColY + 8, 14, kT), 14, 0.16f, ui::kMuted);
  for (int c = 0; c < cols; ++c) {
    const Rectangle rw = playerRowRect(c);
    const SlotSetup& sl = slots_[c];
    const int type = training_ ? (c == 0 ? 1 : 2) : sl.type;
    const Color pc = tint(c);
    const float cy = rw.y + rw.height * 0.5f;
    DrawRectangleRec(rw, withAlpha(ui::kInk, 0.45f));
    DrawRectangleRec({rw.x, rw.y, 3, rw.height}, pc);
    const char* who = training_ && c == 1 ? "CIBLE" : TextFormat("J%d", c + 1);
    float ws = 22.0f;
    while (ws > 12.0f && textWidth(who, ws, kT) > 34) ws -= 1.0f;
    drawText(who, rw.x + 12, capsY(cy, ws, kT), ws, pc, kT);
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
      float ns = 16.0f;
      while (ns > 10.0f && textWidth(nick.c_str(), ns, kT) > 88) ns -= 0.5f;
      drawText(nick.c_str(), rw.x + 104, capsY(cy, ns, kT), ns, ui::kCream, kT);
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
      ui::chunky(f3, foc ? mixColor(pc, WHITE, 0.3f) : pc, shade(pc, 0.7f), 1.0f, 8.0f);
      drawText(tn, f3.x + (f3.width - textWidth(tn, 15, kT)) * 0.5f, capsY(f3.y + f3.height * 0.5f, 15, kT), 15, ui::kInk, kT);
      if (foc) ui::rim(f3, 8, t);
    }
  }
  if (!training_)
    drawText("IA : 9 niveaux, puis « 10 x2 » qui frappe deux fois plus fort.", kRightX,
             capsY(playerRowRect(cols - 1).y + 80, 13, kU), 13, ui::kMuted, kU);

  // The foot: what the rules mean (or what is missing), and done
  drawText(tip, S.x + 34, capsY(S.y + S.height - 48, 14, kU), 14, ready ? kGoldLight : ui::kRed, kU);
  const Rectangle vr = validateRect();
  ui::chunky(vr, ui::hovered(vr) ? mixColor(ui::kGold, WHITE, 0.25f) : kGoldLight, ui::kGold, 1.0f, 10.0f);
  const float kw = ui::prompt("ENTRÉE", ui::Btn::Start, vr.x + 10, vr.y + 10, 24);
  drawText("VALIDER", vr.x + 20 + kw, capsY(vr.y + 22, 21, kT), 21, ui::kInk, kT);
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
