// Character select and stage select screens.
#include "game/app.hpp"
#include "game/fx.hpp"
#include "game/menu_theme.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
int cardCount(bool training) { return training ? 2 : 4; }

// Fighting-game layout (in the spirit of Dragon Ball FighterZ): the fighters stand big on both sides of a "VS",
// one side against the other (J1 / J3 on the left, J2 / J4 on the right; in a team battle, red against blue),
// the first of each side closest to the middle, with its plate under it; every fighter in a grid at the bottom.
float g_slotX[4] = {410, 870, 140, 1140};  // set each frame by App::layoutCss
float g_slotW[4] = {270, 270, 270, 270};
int g_slotSide[4] = {0, 1, 0, 1};
// The light of each side of the character select (left, right): rgb, sliding towards the colour it should have.
float g_sideLight[2][3];
bool g_sideLightSet = false;
constexpr float kFeetY = 396, kPlateY = 404, kPlateH = 74;

Rectangle plateRect(int col) {
  const float pw = std::min(250.0f, g_slotW[col] - 12.0f);
  return {g_slotX[col] - pw * 0.5f, kPlateY, pw, kPlateH};
}
// Fields: the rules in the top corners (row -1: lives, orb, weapons, mode), then for each player the type (0),
// the fighter (1: the big picture), the device or level (2) and, in a team battle, the team chip (3).
Rectangle fieldRect(int col, int row, bool) {
  if (row < 0) {
    const Rectangle r[4] = {{16, 16, 150, 36}, {172, 16, 160, 36}, {948, 16, 160, 36}, {1114, 16, 150, 36}};
    return r[std::clamp(col, 0, 3)];
  }
  const float w = g_slotW[col];
  const Rectangle pl = plateRect(col);
  switch (row) {
    case 3: return {g_slotX[col] + w * 0.5f - 92, 138, 82, 26};
    case 0: return {pl.x, pl.y + 44, pl.width * 0.5f - 3, 28};
    case 1: return {g_slotX[col] - w * 0.5f + 6, 134, w - 12, kFeetY - 130};
    default: return {pl.x + pl.width * 0.5f + 3, pl.y + 44, pl.width * 0.5f - 3, 28};
  }
}
// The roster: every fighter in a grid at the bottom (rows of 9), and "?" (a fighter drawn at random) in the middle,
// under the "VS".
Rectangle rosterRect(int i) {
  const int n = int(roster().size());
  if (i >= n) return {kScreenW * 0.5f - 52, 262, 104, 124};  // a tall tile: the "?" over its name
  constexpr int perRow = 9;
  constexpr float w = 134, h = 74, pitch = 140;
  const int row = i / perRow, col = i % perRow, inRow = std::min(perRow, n - row * perRow);
  return {(kScreenW - (pitch * inRow - 6)) * 0.5f + col * pitch, 516.0f + row * 80.0f, w, h};
}
// Tile tint per universe (the fighters of one series sit side by side): bright, one hue each.
Color seriesColor(const std::string& series) {
  if (series == "Black Clover") return {72, 164, 58, 255};     // grimoire green
  if (series == "Bleach") return {56, 128, 236, 255};          // reishi blue
  if (series.rfind("Zelda", 0) == 0) return {22, 170, 164, 255};  // Zonai teal
  if (series == "God of War") return {208, 40, 50, 255};       // Spartan red
  if (series == "Jujutsu Kaisen") return {138, 72, 228, 255};  // hollow purple
  if (series == "Naruto") return {246, 128, 30, 255};          // Konoha orange
  return {214, 166, 52, 255};                                 // Monster Hunter: guild gold
}
constexpr Color kRandomTint = {74, 64, 176, 255};  // "?": a fighter or an arena drawn at random
constexpr float kTileSlant = 14.0f;
Rectangle readyRect() { return {0, 480, float(kScreenW), 34}; }
// Stage select: a big live preview, its sheet on the right, every stage below in rows of thumbnails.
constexpr int kStageCols = 6;
Rectangle bigPreviewRect() { return {40, 108, 576, 324}; }
// The last card, after every stage, is "?": an arena drawn at random.
Rectangle stageCardRect(int i) {
  const int n = int(stageList().size()) + 1, row = i / kStageCols, col = i % kStageCols;
  const int inRow = std::min(kStageCols, n - row * kStageCols);
  constexpr float w = 190, h = 96, gap = 12;
  return {(kScreenW - (inRow * w + (inRow - 1) * gap)) * 0.5f + col * (w + gap), 450.0f + row * (h + 10), w, h};
}

// Draws `text` wrapped to `width`, line after line; returns the y under the last line.
float drawWrapped(const std::string& text, float x, float y, float width, float size, Color col) {
  std::string line, word;
  auto flush = [&] {
    if (line.empty()) return;
    drawTextOutlined(line.c_str(), x, y, size, col, ui::kInk, 2.0f);
    y += size * 1.3f;
    line.clear();
  };
  for (size_t i = 0; i <= text.size(); ++i) {
    if (i < text.size() && text[i] != ' ') {
      word += text[i];
      continue;
    }
    const std::string next = line.empty() ? word : line + " " + word;
    if (!line.empty() && textWidth(next.c_str(), size) > width) {
      flush();
      line = word;
    } else {
      line = next;
    }
    word.clear();
  }
  flush();
  return y;
}

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
  int count[2] = {0, 0}, rank[4] = {};
  for (int i = 0; i < cols; ++i) {
    g_slotSide[i] = teams ? slotTeam(i) : i % 2;  // left: J1, J3 (red); right: J2, J4 (blue)
    rank[i] = count[g_slotSide[i]]++;
  }
  for (int i = 0; i < cols; ++i) {
    const int n = std::max(2, count[g_slotSide[i]]);
    const float w = std::min(270.0f, 520.0f / n);
    g_slotW[i] = w;
    g_slotX[i] = g_slotSide[i] == 0 ? 545.0f - (rank[i] + 0.5f) * w : 735.0f + (rank[i] + 0.5f) * w;
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
    // Rows from the top: the rules, the team chip (team battles), the type, the fighter, the device / level.
    const bool teamRow = teamBattle();
    const int order[5] = {-1, 3, 0, 1, 2};
    int at = 0;
    while (at < 5 && order[at] != c.row) ++at;
    if (at == 5 || (!teamRow && c.row == 3) || c.row < minRow) at = 3;  // back on the fighter
    auto valid = [&](int i) { return order[i] >= minRow && (teamRow || order[i] != 3); };
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
      const Rectangle f = fieldRect(col, row, training_);
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
  bool clickGo = ready && ui::clicked(readyRect());

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
  const bool teamsOn = teamBattle();
  // Backdrop: a twilight sky, the two sides in their colours (crimson against azure), gold rays behind the "VS"
  const MenuTheme& th = menuTheme();
  const Color night = th.night;
  ui::stageBackdrop(1, t, th.cssDim);
  DrawRectangleGradientV(0, 0, kScreenW, kScreenH, withAlpha(night, 0.55f), withAlpha(night, 0.2f));
  // The light of each side takes the aura colour of the fighter picked by the first player standing there (J1 on
  // the left, J2 on the right: the others do not change it) and slides to it; "?" runs through every colour.
  Color side[2];
  for (int k = 0; k < 2; ++k) {
    Color want = k == 0 ? th.left : th.right;
    for (int col = 0; col < cols; ++col) {
      if (g_slotSide[col] != k) continue;
      const SlotSetup& s = slots_[col];
      if ((training_ || s.type != 0) && !s.random) {
        want = auraColor(r[s.character].hero);
      } else if ((training_ || s.type != 0) && s.random) {
        const int n = int(r.size());
        const float f = t * 0.8f + k * 5.0f;
        const int i0 = int(f) % n;
        want = mixColor(auraColor(r[i0].hero), auraColor(r[(i0 + 1) % n].hero), f - std::floor(f));
      }
      break;  // only the first player of the side
    }
    const float rgb[3] = {float(want.r), float(want.g), float(want.b)};
    for (int c = 0; c < 3; ++c) g_sideLight[k][c] = g_sideLightSet ? lerpf(g_sideLight[k][c], rgb[c], 0.12f) : rgb[c];
    side[k] = {(unsigned char)(g_sideLight[k][0] + 0.5f), (unsigned char)(g_sideLight[k][1] + 0.5f),
               (unsigned char)(g_sideLight[k][2] + 0.5f), 255};
  }
  g_sideLightSet = true;
  DrawRectangleGradientH(0, 0, kScreenW / 2, kScreenH, withAlpha(side[0], 0.55f), withAlpha(side[0], 0.0f));
  DrawRectangleGradientH(kScreenW / 2, 0, kScreenW / 2, kScreenH, withAlpha(side[1], 0.0f), withAlpha(side[1], 0.55f));
  {
    const Vector2 c{kScreenW * 0.5f, 62};
    for (int k = 0; k < 18; ++k) {
      const float a0 = t * 0.12f + k * (2.0f * kPi / 18), a1 = a0 + 0.12f;
      DrawTriangle(c, {c.x + std::cos(a1) * 900, c.y + std::sin(a1) * 900}, {c.x + std::cos(a0) * 900, c.y + std::sin(a0) * 900},
                   withAlpha(th.rays, 0.07f));
    }
    ui::glow(c, 150, withAlpha(mixColor(th.rays, ui::kCream, 0.3f), 0.3f), 8);
    const float pulse = 0.5f + 0.5f * std::sin(t * 4.0f);
    drawTextOutlined("VS", c.x - textWidth("VS", 86, FontKind::Title) * 0.5f, 6, 86, mixColor(ui::kGold, ui::kCream, 0.3f * pulse),
                     ui::kInk, 5.0f, FontKind::Title);
    const char* title = training_ ? "ENTRAÎNEMENT" : teamsOn ? "COMBAT EN ÉQUIPES" : "CHOIX DES COMBATTANTS";
    const float tw = textWidth(title, 20, FontKind::Title) + 40;
    const Rectangle tb{c.x - tw * 0.5f, 96, tw, 30};
    ui::slanted(tb, 10, withAlpha(ui::kInk, 0.92f), withAlpha(ui::kInk, 0.92f));
    ui::slantedOutline(tb, 10, 1.5f, ui::kGold);
    drawTextCentered(title, c.x, tb.y + 5, 20, ui::kCream, true, FontKind::Title);
  }
  // The grid's ground: deep night, a gold rule on top
  DrawRectangleGradientV(0, 500, kScreenW, kScreenH - 500, withAlpha(night, 0.7f), withAlpha(shade(night, 0.45f), 0.96f));
  ui::rule(40, kScreenW - 40.0f, 500, withAlpha(ui::kGold, 0.45f));
  // A field is focused when a player's cursor is on it, or under the mouse.
  auto cursorOn = [&](int col, int row) {
    for (int d = 0; d < kDeviceCount; ++d) {
      const CssCursor& c = cssCur_[d];
      if (c.on && c.row == row && (row < 0 ? c.rule : std::min(c.col, cols - 1)) == col) return true;
    }
    return false;
  };
  auto focusOn = [&](int col, int row) { return cursorOn(col, row) || ui::hovered(fieldRect(col, row, training_)); };

  // Rules
  if (!training_) {
    const Settings& s = settings();
    ui::selector(fieldRect(0, -1, false), TextFormat("VIES : %d", s.stocks), focusOn(0, -1), t);
    ui::selector(fieldRect(1, -1, false), s.items ? "ORBE : OUI" : "ORBE : NON", focusOn(1, -1), t);
    ui::selector(fieldRect(2, -1, false), s.weapons ? "ARMES : OUI" : "ARMES : NON", focusOn(2, -1), t);
    ui::selector(fieldRect(3, -1, false), teamBattle() ? "2 CONTRE 2" : "SOLO", focusOn(3, -1), t);
  } else {
    Rectangle info{864, 16, 400, 36};
    ui::panel(info, withAlpha(ui::kInk, 0.85f), withAlpha(ui::kGold, 0.7f), 0.5f);
    drawTextCentered("Vies infinies  •  Mannequin réglable en pause", info.x + info.width * 0.5f, info.y + 9, 18,
                     ui::kCream, false);
  }

  // The bottom bar first: its shade must not darken the last row of the grid, just above it
  const bool ready = readyToFight();
  ui::footer({{"FLÈCHES", ui::Btn::Move, "Perso / Naviguer"}, {"ESPACE", ui::Btn::A, "Modifier"},
              {"K", ui::Btn::X, "Précédent"}, {"ÉCHAP", ui::Btn::B, "Retour"}},
             ready ? nullptr
             : activeSlots() >= 2 ? "En équipes : il faut au moins un combattant dans chaque équipe."
                                  : "Il faut au moins 2 combattants.");

  // Roster grid, then the "?" tile (random fighter). Each fighter: a close-up of the face on the colour of its
  // universe (as in a fighting game), its name on a band at the bottom.
  for (int i = 0; i <= int(r.size()); ++i) {
    Rectangle rr = rosterRect(i);
    bool hov = ui::hovered(rr);
    const bool rnd = i == int(r.size());
    Color pc = rnd ? kRandomTint : seriesColor(r[i].series);
    bool chosen = false;
    for (int s = 0; s < cardCount(training_); ++s) {
      const SlotSetup& sl = slots_[s];
      if ((training_ || sl.type != 0) && (rnd ? sl.random : !sl.random && sl.character == i)) chosen = true;
    }
    const bool lit = hov || chosen;
    ui::slanted({rr.x + 4, rr.y + 4, rr.width, rr.height}, kTileSlant, withAlpha(BLACK, 0.4f), withAlpha(BLACK, 0.4f));
    ui::slanted(rr, kTileSlant, withAlpha(mixColor(pc, ui::kInk, lit ? 0.12f : 0.3f), 0.97f),
                withAlpha(mixColor(pc, ui::kInk, lit ? 0.62f : 0.76f), 0.97f));
    ui::slanted({rr.x + kTileSlant * 0.7f, rr.y, rr.width - kTileSlant * 0.7f, rr.height * 0.3f}, kTileSlant * 0.3f,
                withAlpha(ui::kCream, lit ? 0.2f : 0.12f), withAlpha(ui::kCream, 0.0f));
    if (rnd) {
      const float qs = std::min(rr.height * 0.9f, 60.0f), pulse = 0.5f + 0.5f * std::sin(t * 3.0f);
      const float qx = rr.x + rr.width * 0.5f, qy = rr.y + 54;
      ui::rune({qx + 4, qy}, 46, 0, t * 0.3f, withAlpha(ui::kCream, 0.1f), 3.0f);
      ui::glow({qx, qy}, 44, withAlpha(ui::kGold, 0.18f + 0.12f * pulse), 6);
      drawTextCentered("?", qx, qy - qs * 0.55f, qs, mixColor(ui::kGold, ui::kCream, pulse), true, FontKind::Title);
      ui::slanted({rr.x, rr.y + rr.height - 30, rr.width - 4, 30}, 6, withAlpha(ui::kInk, 0.9f), withAlpha(ui::kInk, 0.9f));
      DrawLineEx({rr.x + 6, rr.y + rr.height - 30}, {rr.x + rr.width - 6, rr.y + rr.height - 30}, 1.5f,
                 withAlpha(ui::kGold, 0.6f));
      drawTextCentered("ALÉATOIRE", rr.x + rr.width * 0.5f - 4, rr.y + rr.height - 15 - 16 * 0.58f, 16,
                       lit ? ui::kGold : ui::kCream, false, FontKind::Title);
    } else {
      // The close-up fills the tile above the band (the box of that part of the slanted tile)
      const float bh = 21.0f, sb = kTileSlant * bh / rr.height;
      const Rectangle pic{rr.x + sb + 1.5f, rr.y + 1.5f, rr.width - sb - 3.0f, rr.height - bh - 1.5f};
      const Vec2 head{rr.x + rr.width * 0.57f, rr.y + 29.0f};
      ui::rune({head.x, head.y}, 30, i % 4, t * 0.2f, withAlpha(ui::kCream, 0.1f), 2.0f);
      ui::glow({head.x, head.y}, 44, withAlpha(mixColor(pc, ui::kCream, 0.6f), lit ? 0.42f : 0.3f), 6);
      constexpr float size = 104.0f;  // the head (and its hair) is a fifth of it
      drawPortraitIn(r[i], 0, {head.x - size * 0.02f, head.y + size * 0.12f}, size, pic,
                     kTileSlant * pic.height / rr.height, t + i);
      // The name, on a band slanted like the tile, shrunk until it fits
      const Rectangle band{rr.x, rr.y + rr.height - bh, rr.width - kTileSlant + sb, bh};
      ui::slanted(band, sb, withAlpha(ui::kInk, 0.94f), withAlpha(ui::kInk, 0.98f));
      DrawLineEx({band.x + sb, band.y}, {band.x + band.width, band.y}, 1.5f, withAlpha(mixColor(pc, ui::kGold, 0.45f), 0.95f));
      const char* name = r[i].name.c_str();
      const float room = band.width - sb - 14.0f;
      float ns = 17.0f;
      while (ns > 9.0f && textWidth(name, ns, FontKind::Title) > room) ns -= 0.5f;
      drawTextCentered(name, band.x + band.width * 0.5f, band.y + bh * 0.5f - ns * 0.58f + 1.0f, ns,
                       lit ? ui::kGold : ui::kCream, false, FontKind::Title);
    }
    int tags = 0;
    for (int s = 0; s < cardCount(training_); ++s) {
      const SlotSetup& sl = slots_[s];
      bool active = training_ || sl.type != 0;
      if (!active || (rnd ? !sl.random : sl.random || sl.character != i)) continue;
      bool cpu = training_ ? s == 1 : sl.type == 2;
      Color tc = teamBattle() ? teamColor(slotTeam(s)) : cpu ? cpuColor() : portColor(s);
      // in the top corner (the face is a little to the right)
      const Rectangle tag = rnd ? Rectangle{rr.x + 14 + tags * 42.0f, rr.y + 6, 38, 22}
                                : Rectangle{rr.x + kTileSlant + 3 + tags * 36.0f, rr.y + 4, 34, 18};
      DrawRectangleRounded(ui::grow(tag, 1.5f), 0.5f, 6, ui::kInk);
      DrawRectangleRounded(tag, 0.5f, 6, shade(tc, 0.85f));
      drawTextCentered(cpu ? "CPU" : TextFormat("J%d", s + 1), tag.x + tag.width * 0.5f, tag.y + (rnd ? 3 : 2),
                       rnd ? 15 : 13, WHITE, false);
      ++tags;
    }
    ui::slantedOutline(rr, kTileSlant, chosen ? 2.5f : hov ? 2.0f : 1.5f,
                       chosen ? ui::kGold : hov ? withAlpha(ui::kGold, 0.7f) : withAlpha(ui::kInk, 0.95f));
    if (chosen) ui::corners(ui::grow(rr, 4), kTileSlant, 14, 2.5f, ui::kGold);
  }

  // The players, big, on both sides of the "VS"
  for (int col = 0; col < cols; ++col) {
    const SlotSetup& s = slots_[col];
    const int type = training_ ? (col == 0 ? 1 : 2) : s.type;
    const int facing = g_slotSide[col] == 0 ? 1 : -1;  // facing the other side
    Color pc = type == 0 ? Color{90, 98, 100, 255}
               : teamsOn ? teamColor(slotTeam(col))
               : type == 1 ? portColor(col) : cpuColor();
    const Rectangle f1 = fieldRect(col, 1, training_);
    const bool foc1 = focusOn(col, 1);
    const float cx = g_slotX[col], h = std::min(205.0f, g_slotW[col] * 0.76f);
    const float joined = 1.0f - (frames_ - joinedAt_[col]) / 45.0f;  // a gamepad just joined
    if (type != 0) {
      ui::glow({cx, kFeetY - h * 0.5f}, h * 0.75f, withAlpha(mixColor(pc, ui::kCream, 0.35f), 0.35f + 0.3f * std::max(0.0f, joined)), 8);
      ui::stoneDisc({cx, kFeetY + 2}, h * 0.42f, 9, t + col, false);
    }
    if (type != 0 && s.random) {  // "?": drawn at random when the match starts
      const float pulse = 0.5f + 0.5f * std::sin(t * 3.0f + col);
      drawTextCentered("?", cx, kFeetY - h - 10, h, mixColor(ui::kGold, ui::kCream, pulse), true, FontKind::Title);
    } else if (type != 0) {
      const CharacterDef& d = r[s.character];
      int pal = 0;
      for (int j = 0; j < col; ++j)
        if ((training_ || slots_[j].type != 0) && !slots_[j].random && slots_[j].character == s.character) pal = 1 - pal;
      Pose p = restPose(d, t + col);
      const float hop = foc1 ? std::max(0.0f, std::sin(t * 5.0f)) * 6.0f : 0.0f;
      drawShowcase(d, pal, p, {cx, kFeetY - hop}, facing, h, t + col);
    } else {  // join hint: A on any free gamepad, or ESPACE
      ui::rune({cx, kFeetY - 110}, 60, 3, t * 0.15f, withAlpha(ui::kStone, 0.25f), 3.0f);
      if (padPrompts()) ui::padGlyph(ui::Btn::A, cx - 24, kFeetY - 160, 48, promptFamily());
      else drawTextCentered("ESPACE", cx, kFeetY - 150, 28, ui::kMuted, true, FontKind::Title);
      drawTextCentered("pour rejoindre", cx, kFeetY - 108, 17, ui::kMuted, false);
    }
    if (foc1 && type != 0) {  // the arrows: previous / next fighter
      ui::arrowTri({f1.x + 14, kFeetY - h * 0.5f}, 22, -1, ui::kGold);
      ui::arrowTri({f1.x + f1.width - 14, kFeetY - h * 0.5f}, 22, 1, ui::kGold);
    }
    if (foc1) ui::focusFrame(f1, t, 0.08f);

    // The plate: whose it is and the fighter's name, then the type and the device / level
    const Rectangle pl = plateRect(col);
    const Rectangle band{pl.x, pl.y, pl.width, 40};
    ui::slanted({band.x + 4, band.y + 5, band.width, band.height}, 12, withAlpha(BLACK, 0.4f), withAlpha(BLACK, 0.4f));
    ui::slanted(band, 12, type ? shade(pc, 0.9f) : Color{56, 62, 64, 240}, type ? shade(pc, 0.55f) : Color{36, 42, 44, 240});
    ui::slantedOutline(band, 12, joined > 0.0f ? 3.0f : 1.5f, joined > 0.0f ? ui::kGold : withAlpha(ui::kGold, type ? 0.7f : 0.25f));
    const char* who = training_ ? (col == 0 ? "J1" : "CIBLE") : type == 2 ? "CPU" : TextFormat("J%d", col + 1);
    drawTextOutlined(who, band.x + 14, band.y + 9, 20, type ? WHITE : ui::kMuted, ui::kInk, 2.0f, FontKind::Title);
    const std::string name = type == 0 ? std::string("—") : s.random ? std::string("ALÉATOIRE") : r[s.character].name;
    float ns = 26.0f;
    const float nameW = band.width - 70.0f;
    while (ns > 14.0f && textWidth(name.c_str(), ns, FontKind::Title) > nameW) ns -= 1.0f;
    drawTextOutlined(name.c_str(), band.x + band.width - 12 - textWidth(name.c_str(), ns, FontKind::Title),
                     band.y + 20 - ns * 0.55f, ns, ui::kCream, ui::kInk, 3.0f, FontKind::Title);
    if (teamsOn && type != 0) {  // the team chip: press it to change sides
      const Rectangle chip = fieldRect(col, 3, false);
      const Color tc = teamColor(slotTeam(col));
      DrawRectangleRounded(ui::grow(chip, 2), 0.5f, 8, ui::kInk);
      DrawRectangleRounded(chip, 0.5f, 8, focusOn(col, 3) ? mixColor(tc, WHITE, 0.25f) : shade(tc, 0.85f));
      DrawRectangleRoundedLinesEx(ui::grow(chip, -2), 0.5f, 8, 1.0f, withAlpha(ui::kCream, 0.6f));
      drawTextCentered(teamName(slotTeam(col)), chip.x + chip.width * 0.5f, chip.y + 5, 15, WHITE, true, FontKind::Title);
      if (focusOn(col, 3)) ui::focusFrame(chip, t, 0.5f);
    }
    const char* typeName = type == 1 ? "HUMAIN" : type == 2 ? "ORDINATEUR" : "VIDE";
    if (training_) typeName = col == 0 ? "VOUS" : "CIBLE";
    const Rectangle f0 = fieldRect(col, 0, training_);
    if (training_) {
      DrawRectangleRounded(f0, 0.5f, 10, withAlpha(ui::kInk, 0.7f));
      drawTextCentered(typeName, f0.x + f0.width * 0.5f, f0.y + 6, 15, ui::kMuted, false);
      if (focusOn(col, 0)) ui::focusFrame(f0, t, 0.5f);
    } else {
      ui::selector(f0, typeName, focusOn(col, 0), t);
    }
    const Rectangle f2 = fieldRect(col, 2, training_);
    const bool foc2 = focusOn(col, 2);
    if (type == 1) {
      deviceSelector(f2, s.device, foc2, t);
    } else if (type == 2 && !training_) {
      ui::selector(f2, s.level >= kMaxCpuLevel ? "NIV. 10 x2" : TextFormat("NIVEAU %d", s.level), foc2, t);
    } else {
      DrawRectangleRounded(f2, 0.5f, 10, withAlpha(ui::kInk, 0.6f));
      if (training_) drawTextCentered("en pause", f2.x + f2.width * 0.5f, f2.y + 6, 15, ui::kMuted, false);
      if (foc2) ui::focusFrame(f2, t, 0.5f);
    }
  }

  // Whose cursor is where: a tag in the player's colour on the corner of the field.
  std::array<int, 24> stacked{};
  for (int dev = 0; dev < kDeviceCount; ++dev) {
    const CssCursor& c = cssCur_[dev];
    if (!c.on) continue;
    const int own = ownedCard(dev);
    const int col = c.row < 0 ? c.rule : std::min(c.col, cols - 1);
    const int slot = (c.row + 1) * 4 + col;
    const Rectangle f = fieldRect(col, c.row, training_);
    const Color tc = own >= 0 ? portColor(own) : cpuColor();
    const char* label = own >= 0 ? TextFormat("J%d", own + 1) : dev == kKeyboard1 ? "C1" : dev == kKeyboard2 ? "C2" : "M";
    Rectangle tag{f.x - 6 + stacked[slot] * 40.0f, f.y - 12, 36, 20};
    ++stacked[slot];
    DrawRectangleRounded(ui::grow(tag, 1.5f), 0.5f, 6, ui::kInk);
    DrawRectangleRounded(tag, 0.5f, 6, tc);
    drawTextCentered(label, tag.x + tag.width * 0.5f, tag.y + 2, 14, WHITE, false);
  }

  // "Ready to fight" banner: teal ink, gold edges, a Zonai glow behind the words
  Rectangle rb = readyRect();
  if (ready) {
    float pulse = 0.5f + 0.5f * std::sin(t * 5.0f);
    ui::slanted({-40, rb.y, kScreenW + 80.0f, rb.height}, 20, withAlpha(mixColor(night, th.glow, 0.18f), 0.97f),
                withAlpha(mixColor(night, ui::kInk, 0.6f), 0.97f));
    const int gy = int(rb.y), gh = int(rb.height);
    DrawRectangleGradientH(240, gy, 400, gh, withAlpha(th.glow, 0.0f), withAlpha(th.glow, 0.16f + 0.1f * pulse));
    DrawRectangleGradientH(640, gy, 400, gh, withAlpha(th.glow, 0.16f + 0.1f * pulse), withAlpha(th.glow, 0.0f));
    ui::slanted({-40, rb.y, kScreenW + 80.0f, 2}, 1, withAlpha(ui::kGold, 0.6f + 0.4f * pulse), withAlpha(ui::kGold, 0.6f + 0.4f * pulse));
    ui::slanted({-40, rb.y + rb.height - 2, kScreenW + 80.0f, 2}, 1, withAlpha(ui::kGold, 0.6f), withAlpha(ui::kGold, 0.6f));
    const char* txt = training_ ? "PRÊT POUR L'ENTRAÎNEMENT !" : "PRÊTS AU COMBAT !";
    float w = textWidth(txt, 30, FontKind::Title);
    const float tx = kScreenW * 0.5f - w * 0.5f - 60;
    ui::diamond({tx - 22, rb.y + rb.height * 0.5f}, 6, ui::kGold);
    drawTextOutlined(txt, tx, rb.y + 4, 30, mixColor(ui::kGold, ui::kCream, 0.35f), ui::kInk, 3.0f, FontKind::Title);
    ui::prompt("ENTRÉE", ui::Btn::Start, kScreenW * 0.5f + w * 0.5f - 40, rb.y + 6, 26, true);
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
  bool go = nav.confirm || nav.start || ui::clicked(bigPreviewRect());
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
  if (go) {
    audio_.play(Sfx::MenuSelect);
    startMatch();
  } else if (nav.back) {
    audio_.play(Sfx::MenuBack);
    goTo(Screen::CharSelect);
  }
}

void App::drawStage() {
  const float t = time_;
  const auto& stages = stageList();
  // "?": the preview flips through the arenas, one of them is drawn when the match starts
  const int shown = stageRandom_ ? int(t * 1.5f) % int(stages.size()) : stage_;
  const StageDef& cur = stages[shown];
  ui::stageBackdrop(shown, t, 0.8f);
  ui::header("ARÈNE", nullptr, {66, 112, 132, 255}, t);

  // The chosen stage, live.
  Rectangle big = bigPreviewRect();
  DrawRectangleRounded({big.x + 8, big.y + 10, big.width, big.height}, 0.04f, 8, withAlpha(BLACK, 0.45f));
  drawStagePreview(cur, big, t);
  DrawRectangleRoundedLinesEx(ui::grow(big, 3), 0.04f, 8, 3, ui::kGold);
  DrawRectangleRoundedLinesEx(ui::grow(big, -5), 0.03f, 8, 1, withAlpha(ui::kGold, 0.4f));
  ui::corners(ui::grow(big, 9), 0, 30, 3, ui::kGold);
  if (stageRandom_) {
    DrawRectangleRec(big, withAlpha(ui::kInk, 0.45f));
    const float pulse = 0.5f + 0.5f * std::sin(t * 3.0f);
    drawTextCentered("?", big.x + big.width * 0.5f, big.y + big.height * 0.5f - 90, 170,
                     mixColor(ui::kGold, ui::kCream, pulse), true, FontKind::Title);
    drawTextCentered("ARÈNE ALÉATOIRE", big.x + big.width * 0.5f, big.y + big.height - 60, 34, ui::kCream, true,
                     FontKind::Title);
  }

  // Its sheet: series, name, a line about it, the platforms, the rules of the match.
  const float px = big.x + big.width + 32, pw = kScreenW - 40 - px;
  const Color sc = stageRandom_ ? kRandomTint : seriesColor(cur.series);
  const std::string series = stageRandom_ ? std::string("TOUTES LES SÉRIES") : upperFr(cur.series);
  const std::string name = stageRandom_ ? std::string("Aléatoire") : cur.name;
  const float chipW = textWidth(series.c_str(), 17) + 26;
  DrawRectangleRounded({px, big.y + 6, chipW, 28}, 0.5f, 8, withAlpha(sc, 0.9f));
  DrawRectangleRoundedLinesEx({px, big.y + 6, chipW, 28}, 0.5f, 8, 1.5f, withAlpha(mixColor(sc, WHITE, 0.5f), 0.9f));
  drawText(series.c_str(), px + 13, big.y + 11, 17, ui::kCream);
  float nameSize = 46.0f;
  nameSize = std::min(nameSize, nameSize * pw / std::max(1.0f, textWidth(name.c_str(), nameSize, FontKind::Title)));
  drawTextOutlined(name.c_str(), px, big.y + 46, nameSize, ui::kCream, ui::kInk, 3.5f, FontKind::Title);
  float y = big.y + 46 + nameSize * 1.25f;
  DrawRectangleGradientH(int(px), int(y), int(pw * 0.8f), 2, withAlpha(ui::kGold, 0.8f), withAlpha(ui::kGold, 0.0f));
  y = drawWrapped(stageRandom_ ? std::string("Une des arènes, tirée au sort au début du combat.") : cur.blurb, px,
                  y + 14, pw, 21, ui::kCream) + 10;
  int solids = 0, plats = 0, moving = 0;
  for (const Platform& p : cur.platforms) {
    if (p.solid) ++solids;
    else ++plats;
    if (p.period > 0) ++moving;
  }
  std::string layout = solids > 1 ? TextFormat("%d sols", solids) : std::string("1 sol");
  if (plats > 0) layout += TextFormat(", %d plateforme%s", plats, plats > 1 ? "s" : "");
  if (moving > 0) layout += moving == plats ? (plats > 1 ? " mobiles" : " mobile") : TextFormat(" dont %d mobile%s", moving, moving > 1 ? "s" : "");
  if (!stageRandom_) {
    ui::diamond({px + 5, y + 11}, 4, ui::kGold);
    drawTextOutlined(layout.c_str(), px + 18, y, 19, ui::kMuted, ui::kInk, 2.0f);
  }
  const Settings& s = settings();
  std::string info = training_ ? "Entraînement  •  vies infinies"
                               : TextFormat("%d combattants  •  %d vie%s  •  orbe %s", activeSlots(), s.stocks,
                                            s.stocks > 1 ? "s" : "", s.items ? "activée" : "désactivée");
  ui::diamond({px + 5, big.y + big.height - 12}, 4, ui::kGold);
  drawTextOutlined(info.c_str(), px + 18, big.y + big.height - 23, 19, ui::kCream, ui::kInk, 2.0f);

  // Every stage: a still each, the series as a coloured edge.
  for (int i = 0; i < int(stages.size()); ++i) {
    Rectangle rr = stageCardRect(i);
    const bool sel = !stageRandom_ && i == stage_;
    if (sel) rr = ui::grow(rr, 4);
    DrawRectangleRounded({rr.x + 5, rr.y + 7, rr.width, rr.height}, 0.08f, 8, withAlpha(BLACK, 0.4f));
    drawStageThumb(i, stages[i], stageCardRect(i));
    if (sel) drawStageThumb(i, stages[i], stageCardRect(i));
    if (!sel) DrawRectangleRec(stageCardRect(i), withAlpha(ui::kInk, 0.4f));
    const Rectangle base = stageCardRect(i);
    DrawRectangle(int(base.x), int(base.y + base.height - 26), int(base.width), 26, withAlpha(ui::kInk, 0.88f));
    DrawRectangle(int(base.x), int(base.y + base.height - 26), 5, 26, seriesColor(stages[i].series));
    float ns = 17.0f;
    ns = std::min(ns, ns * (base.width - 20) / std::max(1.0f, textWidth(stages[i].name.c_str(), ns)));
    drawText(stages[i].name.c_str(), base.x + 12, base.y + base.height - 13 - ns * 0.62f, ns, sel ? ui::kGold : ui::kCream);
    if (sel) {
      const float pulse = 0.5f + 0.5f * std::sin(t * 6.0f);
      DrawRectangleRoundedLinesEx(ui::grow(rr, 4 + pulse * 2), 0.08f, 8, 4, withAlpha(ui::kTeal, 0.14f + 0.14f * pulse));
      DrawRectangleRoundedLinesEx(rr, 0.08f, 8, 3.0f, ui::kGold);
      ui::corners(ui::grow(rr, 6 + pulse * 2), 0, 14, 3, ui::kGold);
    } else {
      DrawRectangleRoundedLinesEx(base, 0.08f, 8, 1.5f, withAlpha(ui::kStone, 0.6f));
    }
  }
  {  // the "?" card: an arena drawn at random
    const Rectangle base = stageCardRect(int(stages.size()));
    const Rectangle rr = stageRandom_ ? ui::grow(base, 4) : base;
    const float pulse = 0.5f + 0.5f * std::sin(t * 3.0f);
    DrawRectangleRounded({rr.x + 5, rr.y + 7, rr.width, rr.height}, 0.08f, 8, withAlpha(BLACK, 0.4f));
    DrawRectangleRounded(rr, 0.08f, 8, withAlpha(mixColor(ui::kSlate, ui::kInk, 0.4f), 0.95f));
    drawTextCentered("?", base.x + base.width * 0.5f, base.y + 2, 56, stageRandom_ ? mixColor(ui::kGold, ui::kCream, pulse)
                                                                                 : ui::kCream, true, FontKind::Title);
    DrawRectangle(int(base.x), int(base.y + base.height - 26), int(base.width), 26, withAlpha(ui::kInk, 0.88f));
    drawText("Aléatoire", base.x + 12, base.y + base.height - 24, 17, stageRandom_ ? ui::kGold : ui::kCream);
    if (stageRandom_) {
      DrawRectangleRoundedLinesEx(rr, 0.08f, 8, 3.0f, ui::kGold);
      ui::corners(ui::grow(rr, 6 + pulse * 2), 0, 14, 3, ui::kGold);
    } else {
      DrawRectangleRoundedLinesEx(base, 0.08f, 8, 1.5f, withAlpha(ui::kStone, 0.6f));
    }
  }
  ui::footer({{"FLÈCHES", ui::Btn::Move, "Choisir"}, {"ENTRÉE", ui::Btn::A, "Combattre !"}, {"ÉCHAP", ui::Btn::B, "Retour"}});
}
