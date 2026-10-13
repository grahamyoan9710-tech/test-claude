// Character select and stage select screens.
#include "game/app.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
constexpr float kCardW = 290, kCardH = 364, kCardGap = 16, kCardY = 262, kCardSlant = 16;

int cardCount(bool training) { return training ? 2 : 4; }
float cardX(int col, bool training) {
  int n = cardCount(training);
  return (kScreenW - (n * kCardW + (n - 1) * kCardGap)) * 0.5f + col * (kCardW + kCardGap);
}
Rectangle cardRect(int col, bool training) { return {cardX(col, training), kCardY, kCardW, kCardH}; }
// Fields: the rules on top (row -1: lives, orb, weapons, mode), then on each card the type (0), the fighter (1),
// the device or level (2) and, in a team battle, the team chip in the card's band (3).
Rectangle fieldRect(int col, int row, bool training) {
  if (row < 0) {
    const Rectangle r[4] = {{640, 40, 140, 44}, {788, 40, 158, 44}, {954, 40, 158, 44}, {1120, 40, 140, 44}};
    return r[std::clamp(col, 0, 3)];
  }
  float x = cardX(col, training);
  switch (row) {
    case 3: return {x + kCardW - 112, kCardY + 6, 96, 32};
    case 0: return {x + 24, kCardY + 54, kCardW - 48, 36};
    case 1: return {x + 14, kCardY + 98, kCardW - 28, 206};
    default: return {x + 20, kCardY + 316, kCardW - 48, 36};
  }
}
// The roster: one strip of tall tiles, or two rows of wide tiles when there are many fighters.
// The last tile, after every fighter, is "?": a fighter drawn at random.
Rectangle rosterRect(int i) {
  const int n = int(roster().size()) + 1;
  if (n <= 12) {
    const float pitch = std::min(188.0f, 1150.0f / n), w = pitch - 8.0f;
    return {(kScreenW - pitch * n) * 0.5f + 8.0f + i * pitch, 112, w, 132};
  }
  const int perRow = (n + 1) / 2, row = i / perRow, col = i % perRow, inRow = row == 0 ? perRow : n - perRow;
  const float pitch = std::min(150.0f, 1200.0f / perRow), w = pitch - 8.0f;
  return {(kScreenW - pitch * inRow) * 0.5f + 4.0f + col * pitch, 104.0f + row * 76.0f, w, 70.0f};
}
// Tile tint per universe (the fighters of one series sit side by side).
Color seriesColor(const std::string& series) {
  if (series == "Black Clover") return {64, 120, 72, 255};
  if (series == "Bleach") return {200, 108, 44, 255};
  if (series.rfind("Zelda", 0) == 0) return {52, 136, 132, 255};
  if (series == "God of War") return {168, 44, 38, 255};
  if (series == "Jujutsu Kaisen") return {98, 72, 176, 255};
  if (series == "Naruto") return {226, 140, 36, 255};
  return {120, 120, 130, 255};
}
Rectangle readyRect() { return {0, 632, float(kScreenW), 38}; }
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
      settings().teams = !settings().teams;
      if (settings().teams) audio_.voice("teambattle");
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
      if (s.type == 1) s.device = (s.device + delta + kDeviceCount) % kDeviceCount;
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
    const bool teamRow = !training_ && settings().teams;
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
  const int lastRow = (!training_ && settings().teams) ? 3 : 2;
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
  const Color accent = training_ ? Color{98, 136, 76, 255} : Color{178, 82, 58, 255};
  ui::stageBackdrop(1, t, 0.66f);
  ui::header(training_ ? "ENTRAÎNEMENT" : "COMBAT", nullptr, accent, t);
  const int cols = cardCount(training_);
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
    ui::selector(fieldRect(3, -1, false), s.teams ? "2 CONTRE 2" : "SOLO", focusOn(3, -1), t);
  } else {
    Rectangle info{820, 42, 410, 40};
    ui::panel(info, withAlpha(ui::kInk, 0.85f), withAlpha(ui::kGold, 0.7f), 0.5f);
    drawTextCentered("Vies infinies  •  Mannequin réglable en pause", info.x + info.width * 0.5f, info.y + 9, 18,
                     ui::kCream, false);
  }

  // Roster strip, then the "?" tile (random fighter)
  for (int i = 0; i <= int(r.size()); ++i) {
    Rectangle rr = rosterRect(i);
    bool hov = ui::hovered(rr);
    const bool rnd = i == int(r.size());
    Color pc = rnd ? Color{120, 120, 130, 255} : seriesColor(r[i].series);
    const bool wide = rr.width > rr.height;  // two rows: the portrait on the left, the name beside it
    ui::slanted({rr.x + 5, rr.y + 6, rr.width, rr.height}, 14, withAlpha(BLACK, 0.35f), withAlpha(BLACK, 0.35f));
    ui::slanted(rr, 14, withAlpha(mixColor(pc, ui::kInk, hov ? 0.4f : 0.55f), 0.95f), withAlpha(mixColor(pc, ui::kInk, 0.86f), 0.95f));
    if (rnd) {
      const float qs = std::min(rr.height * 0.9f, 60.0f), pulse = 0.5f + 0.5f * std::sin(t * 3.0f);
      const float qx = wide ? rr.x + 38.0f : rr.x + rr.width * 0.5f, qy = wide ? rr.y + rr.height * 0.5f : rr.y + 54;
      ui::glow({qx, qy}, 40, withAlpha(ui::kGold, 0.15f + 0.1f * pulse), 6);
      drawTextCentered("?", qx, qy - qs * 0.55f, qs, mixColor(ui::kGold, ui::kCream, pulse), true, FontKind::Title);
      if (wide) {
        drawText("ALÉATOIRE", rr.x + 72.0f, rr.y + 12, 18, ui::kCream, FontKind::Title);
        drawText("au hasard", rr.x + 72.0f, rr.y + 40, 13, withAlpha(ui::kCream, 0.6f));
      } else {
        drawTextCentered("ALÉATOIRE", rr.x + rr.width * 0.5f - 4, rr.y + rr.height - 26, 16, ui::kCream, false,
                         FontKind::Title);
      }
    } else if (wide) {
      const Vec2 pc0{rr.x + 38.0f, rr.y + rr.height * 0.5f};
      ui::glow({pc0.x, pc0.y}, 40, withAlpha({255, 236, 196, 255}, 0.22f), 6);
      drawPortrait(r[i], 0, pc0, rr.height - 8.0f, t + i);
      const float tx = rr.x + 72.0f, tw = rr.width - 80.0f;
      float ns = 22.0f;
      while (ns > 13.0f && textWidth(r[i].name.c_str(), ns, FontKind::Title) > tw) ns -= 1.0f;
      drawText(r[i].name.c_str(), tx, rr.y + 12, ns, ui::kCream, FontKind::Title);
      std::string series = r[i].series.rfind("Zelda", 0) == 0 ? "Zelda" : r[i].series;
      if (textWidth(series.c_str(), 13) > tw) series = series == "Jujutsu Kaisen" ? "JJK" : series.substr(0, 9) + ".";
      drawText(series.c_str(), tx, rr.y + 40, 13, withAlpha(ui::kCream, 0.6f));
    } else {
      ui::rune({rr.x + rr.width * 0.5f + 4, rr.y + 54}, 50, i, t * 0.2f, withAlpha(ui::kCream, 0.07f), 3.0f);
      ui::glow({rr.x + rr.width * 0.5f, rr.y + 60}, 60, withAlpha({255, 236, 196, 255}, 0.2f), 6);
      drawPortrait(r[i], 0, {rr.x + rr.width * 0.5f, rr.y + 54}, std::min(92.0f, rr.width * 0.78f), t + i);
      ui::slanted({rr.x, rr.y + rr.height - 30, rr.width - 4, 30}, 6, withAlpha(ui::kInk, 0.88f), withAlpha(ui::kInk, 0.88f));
      DrawLineEx({rr.x + 6, rr.y + rr.height - 30}, {rr.x + rr.width - 6, rr.y + rr.height - 30}, 1.0f, withAlpha(ui::kGold, 0.4f));
      float ns = 24.0f;  // long names shrink to fit the tile
      while (ns > 14.0f && textWidth(r[i].name.c_str(), ns, FontKind::Title) > rr.width - 16.0f) ns -= 1.0f;
      drawTextCentered(r[i].name.c_str(), rr.x + rr.width * 0.5f - 4, rr.y + rr.height - 15 - ns * 0.58f, ns, ui::kCream,
                       false, FontKind::Title);
    }
    int tags = 0;
    bool chosen = false;
    for (int s = 0; s < cardCount(training_); ++s) {
      const SlotSetup& sl = slots_[s];
      bool active = training_ || sl.type != 0;
      if (!active || (rnd ? !sl.random : sl.random || sl.character != i)) continue;
      bool cpu = training_ ? s == 1 : sl.type == 2;
      Color tc = (!training_ && settings().teams) ? teamColor(slotTeam(s)) : cpu ? cpuColor() : portColor(s);
      chosen = true;
      Rectangle tag = wide ? Rectangle{rr.x + 4 + tags * 40.0f, rr.y + rr.height - 24, 38, 22}
                           : Rectangle{rr.x + 14 + tags * 42.0f, rr.y + 6, 38, 22};
      DrawRectangleRounded(ui::grow(tag, 1.5f), 0.5f, 6, ui::kInk);
      DrawRectangleRounded(tag, 0.5f, 6, shade(tc, 0.85f));
      drawTextCentered(cpu ? "CPU" : TextFormat("J%d", s + 1), tag.x + tag.width * 0.5f, tag.y + 3, 15, WHITE, false);
      ++tags;
    }
    ui::slantedOutline(rr, 14, chosen ? 2.5f : hov ? 2.0f : 1.5f,
                       chosen ? ui::kGold : hov ? withAlpha(ui::kGold, 0.6f) : withAlpha(ui::kStone, 0.55f));
    if (chosen) ui::corners(ui::grow(rr, 4), 14, 14, 2.5f, ui::kGold);
  }

  // Player cards
  for (int col = 0; col < cardCount(training_); ++col) {
    const SlotSetup& s = slots_[col];
    const int type = training_ ? (col == 0 ? 1 : 2) : s.type;
    Rectangle cr = cardRect(col, training_);
    const bool teamBattle = !training_ && settings().teams;
    Color pc = type == 0 ? Color{70, 78, 80, 255}
               : teamBattle ? teamColor(slotTeam(col))
               : type == 1 ? portColor(col) : cpuColor();
    ui::slanted({cr.x + 7, cr.y + 9, cr.width, cr.height}, kCardSlant, withAlpha(BLACK, 0.4f), withAlpha(BLACK, 0.4f));
    ui::slanted(cr, kCardSlant, withAlpha(mixColor(ui::kSlate, pc, type ? 0.2f : 0.05f), 0.94f),
                withAlpha(mixColor(ui::kInk, pc, type ? 0.08f : 0.02f), 0.95f));
    const Rectangle band{cr.x + kCardSlant * 0.85f, cr.y, cr.width - kCardSlant * 0.85f, 44};
    ui::slanted(band, kCardSlant * 44 / kCardH, type ? shade(pc, 0.86f) : Color{58, 66, 68, 255},
                type ? shade(pc, 0.55f) : Color{40, 48, 50, 255});
    DrawLineEx({band.x + 2, band.y + band.height}, {band.x + band.width - kCardSlant * 44 / kCardH, band.y + band.height},
               1.5f, withAlpha(ui::kGold, type ? 0.8f : 0.3f));
    ui::rune({band.x + band.width - 30, band.y + 22}, 13, col, t * 0.3f, withAlpha(ui::kCream, type ? 0.45f : 0.15f), 2.0f);
    ui::slantedOutline(cr, kCardSlant, 1.5f, type ? withAlpha(ui::kGold, 0.55f) : withAlpha(ui::kStone, 0.4f));
    ui::corners(cr, kCardSlant, 16, 2.5f, type ? ui::kGold : withAlpha(ui::kStone, 0.6f));
    const float joined = 1.0f - (frames_ - joinedAt_[col]) / 45.0f;  // a gamepad just joined
    if (joined > 0.0f) ui::slantedOutline(ui::grow(cr, 3), kCardSlant, 6, withAlpha(ui::kGold, joined));
    const char* head = training_ ? (col == 0 ? "JOUEUR 1" : "MANNEQUIN") : TextFormat("JOUEUR %d", col + 1);
    drawTextOutlined(head, cr.x + 30, cr.y + 7, 30, type ? ui::kCream : ui::kMuted, ui::kInk, 3.0f, FontKind::Title);
    if (teamBattle && type != 0) {  // the team chip: press it to change sides
      const Rectangle chip = fieldRect(col, 3, false);
      const Color tc = teamColor(slotTeam(col));
      DrawRectangleRounded(ui::grow(chip, 2), 0.5f, 8, ui::kInk);
      DrawRectangleRounded(chip, 0.5f, 8, focusOn(col, 3) ? mixColor(tc, WHITE, 0.25f) : shade(tc, 0.85f));
      DrawRectangleRoundedLinesEx(ui::grow(chip, -2), 0.5f, 8, 1.0f, withAlpha(ui::kCream, 0.6f));
      drawTextCentered(teamName(slotTeam(col)), chip.x + chip.width * 0.5f, chip.y + 7, 17, WHITE, true, FontKind::Title);
      if (focusOn(col, 3)) ui::focusFrame(chip, t, 0.5f);
    }

    Rectangle f1 = fieldRect(col, 1, training_);
    bool foc1 = focusOn(col, 1);
    if (type != 0 && s.random) {  // "?": the fighter is drawn when the match starts
      Vector2 base{f1.x + f1.width * 0.5f, f1.y + 136};
      ui::glow({base.x, base.y - 50}, 95, withAlpha(mixColor(pc, ui::kCream, 0.5f), 0.4f), 8);
      ui::stoneDisc({base.x, base.y + 2}, 74, 11, t + col, false);
      const float pulse = 0.5f + 0.5f * std::sin(t * 3.0f + col);
      drawTextCentered("?", base.x, base.y - 128, 110, mixColor(ui::kGold, ui::kCream, pulse), true, FontKind::Title);
      drawTextCentered("ALÉATOIRE", base.x, f1.y + 146, 34, ui::kCream, true, FontKind::Title);
      drawTextCentered("Tiré au sort au début du combat", base.x, f1.y + 184, 15, ui::kMuted, false);
      if (foc1) {
        ui::focusFrame(f1, t, 0.12f);
        ui::arrowTri({f1.x + 16, f1.y + 80}, 22, -1, ui::kGold);
        ui::arrowTri({f1.x + f1.width - 16, f1.y + 80}, 22, 1, ui::kGold);
      }
    } else if (type != 0) {
      const CharacterDef& d = r[s.character];
      int pal = 0;
      for (int j = 0; j < col; ++j)
        if ((training_ || slots_[j].type != 0) && !slots_[j].random && slots_[j].character == s.character) pal = 1 - pal;
      Vector2 base{f1.x + f1.width * 0.5f, f1.y + 136};
      ui::glow({base.x, base.y - 50}, 95, withAlpha(mixColor(pc, ui::kCream, 0.5f), 0.4f), 8);
      ui::stoneDisc({base.x, base.y + 2}, 74, 11, t + col, false);
      Pose p = restPose(d, t + col);
      float hop = foc1 ? std::max(0.0f, std::sin(t * 5.0f)) * 6.0f : 0.0f;
      drawShowcase(d, pal, p, {base.x, base.y - hop}, 1, 128.0f, t + col);
      drawTextCentered(d.name.c_str(), base.x, f1.y + 146, 34, ui::kCream, true, FontKind::Title);
      drawTextCentered(d.title.c_str(), base.x, f1.y + 184, 15, ui::kMuted, false);
      if (foc1) {
        ui::focusFrame(f1, t, 0.12f);
        ui::arrowTri({f1.x + 16, f1.y + 80}, 22, -1, ui::kGold);
        ui::arrowTri({f1.x + f1.width - 16, f1.y + 80}, 22, 1, ui::kGold);
      }
    } else {  // join hint: A on any free gamepad, or ESPACE on this card
      ui::rune({f1.x + f1.width * 0.5f, f1.y + 86}, 64, 3, t * 0.15f, withAlpha(ui::kStone, 0.18f), 3.0f);
      if (padPrompts()) ui::padGlyph(ui::Btn::A, f1.x + f1.width * 0.5f - 26, f1.y + 46, 52, promptFamily());
      else drawTextCentered("ESPACE", f1.x + f1.width * 0.5f, f1.y + 64, 30, ui::kMuted, true, FontKind::Title);
      drawTextCentered("pour rejoindre", f1.x + f1.width * 0.5f, f1.y + 108, 18, ui::kMuted, false);
      if (foc1) ui::focusFrame(f1, t, 0.12f);
    }

    // over the fighter: a cloth or a lock of hair flying above the head passes behind it
    const char* typeName = type == 1 ? "HUMAIN" : type == 2 ? "ORDINATEUR" : "VIDE";
    if (training_) typeName = col == 0 ? "VOUS" : "CIBLE D'ENTRAÎNEMENT";
    Rectangle f0 = fieldRect(col, 0, training_);
    if (training_) {
      DrawRectangleRounded(f0, 0.5f, 10, withAlpha(ui::kInk, 0.6f));
      drawTextCentered(typeName, f0.x + f0.width * 0.5f, f0.y + 8, 18, ui::kMuted, false);
      if (focusOn(col, 0)) ui::focusFrame(f0, t, 0.5f);
    } else {
      ui::selector(f0, typeName, focusOn(col, 0), t);
    }

    Rectangle f2 = fieldRect(col, 2, training_);
    bool foc2 = focusOn(col, 2);
    if (type == 1) {
      deviceSelector(f2, s.device, foc2, t);
    } else if (type == 2 && !training_) {
      ui::selector(f2, s.level >= kMaxCpuLevel ? "NIVEAU 10 : PUISSANCE x2" : TextFormat("NIVEAU %d", s.level), foc2, t);
    } else {
      DrawRectangleRounded(f2, 0.5f, 10, withAlpha(ui::kInk, 0.5f));
      if (training_) drawTextCentered("Réglable en pause", f2.x + f2.width * 0.5f, f2.y + 9, 16, ui::kMuted, false);
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
  bool ready = readyToFight();
  Rectangle rb = readyRect();
  if (ready) {
    float pulse = 0.5f + 0.5f * std::sin(t * 5.0f);
    ui::slanted({-40, rb.y, kScreenW + 80.0f, rb.height}, 20, withAlpha({34, 56, 58, 255}, 0.97f), withAlpha(ui::kInk, 0.97f));
    const int gy = int(rb.y), gh = int(rb.height);
    DrawRectangleGradientH(240, gy, 400, gh, withAlpha(ui::kTeal, 0.0f), withAlpha(ui::kTeal, 0.16f + 0.1f * pulse));
    DrawRectangleGradientH(640, gy, 400, gh, withAlpha(ui::kTeal, 0.16f + 0.1f * pulse), withAlpha(ui::kTeal, 0.0f));
    ui::slanted({-40, rb.y, kScreenW + 80.0f, 2}, 1, withAlpha(ui::kGold, 0.6f + 0.4f * pulse), withAlpha(ui::kGold, 0.6f + 0.4f * pulse));
    ui::slanted({-40, rb.y + rb.height - 2, kScreenW + 80.0f, 2}, 1, withAlpha(ui::kGold, 0.6f), withAlpha(ui::kGold, 0.6f));
    const char* txt = training_ ? "PRÊT POUR L'ENTRAÎNEMENT !" : "PRÊTS AU COMBAT !";
    float w = textWidth(txt, 30, FontKind::Title);
    const float tx = kScreenW * 0.5f - w * 0.5f - 60;
    ui::diamond({tx - 22, rb.y + rb.height * 0.5f}, 6, ui::kGold);
    drawTextOutlined(txt, tx, rb.y + 4, 30, mixColor(ui::kGold, ui::kCream, 0.35f), ui::kInk, 3.0f, FontKind::Title);
    ui::prompt("ENTRÉE", ui::Btn::Start, kScreenW * 0.5f + w * 0.5f - 40, rb.y + 6, 26, true);
  }
  ui::footer({{"FLÈCHES", ui::Btn::Move, "Perso / Naviguer"}, {"ESPACE", ui::Btn::A, "Modifier"},
              {"K", ui::Btn::X, "Précédent"}, {"ÉCHAP", ui::Btn::B, "Retour"}},
             ready ? nullptr
             : activeSlots() >= 2 ? "En équipes : il faut au moins un combattant dans chaque équipe."
                                  : "Il faut au moins 2 combattants.");
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
  const Color sc = stageRandom_ ? Color{120, 120, 130, 255} : seriesColor(cur.series);
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
