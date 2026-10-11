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
Rectangle fieldRect(int col, int row, bool training) {
  if (row < 0) return col == 0 ? Rectangle{652, 40, 176, 44} : col == 1 ? Rectangle{840, 40, 226, 44}
                                                                       : Rectangle{1078, 40, 178, 44};
  float x = cardX(col, training);
  switch (row) {
    case 0: return {x + 24, kCardY + 54, kCardW - 48, 36};
    case 1: return {x + 14, kCardY + 98, kCardW - 28, 206};
    default: return {x + 20, kCardY + 316, kCardW - 48, 36};
  }
}
// The roster: one strip of tall tiles, or two rows of wide tiles when there are many fighters.
Rectangle rosterRect(int i) {
  const int n = std::max(1, int(roster().size()));
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
  return {120, 120, 130, 255};
}
Rectangle readyRect() { return {0, 632, float(kScreenW), 38}; }
Rectangle bigPreviewRect() { return {60, 116, 768, 432}; }
Rectangle stageCardRect(int i) { return {866.0f - i * 10.0f, 116.0f + i * 150.0f, 360, 134}; }

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
    else settings().weapons = !settings().weapons;
    saveSettings();
    return;
  }
  SlotSetup& s = slots_[col];
  const int nChars = int(roster().size());
  if (training_) {
    if (row == 1) s.character = (s.character + delta + nChars) % nChars;
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
        bool byFreePad = by >= kPad1 && by < kDeviceCount;  // a pad that plays nobody takes the card itself
        for (int j = 0; j < 4 && byFreePad; ++j)
          if (j != col && slots_[j].type == 1 && slots_[j].device == by) byFreePad = false;
        if (byFreePad) padLeft_[by - kPad1] = false;
        s.device = byFreePad ? by : freeDevice(col);
      }
      break;
    }
    case 1:
      s.character = (s.character + delta + nChars) % nChars;
      break;
    default:
      if (s.type == 1) s.device = (s.device + delta + kDeviceCount) % kDeviceCount;
      else if (s.type == 2) s.level = 1 + (s.level - 1 + delta + kMaxCpuLevel) % kMaxCpuLevel;
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
    cssCol_ = col;  // the newcomer picks a fighter right away
    cssRow_ = 1;
    audio_.play(Sfx::MenuSelect);
    return true;
  }
  return false;
}

void App::updateCss(const MenuNav& nav) {
  if (padJoins()) return;  // that press is used up
  const int cols = cardCount(training_);
  const int minRow = training_ ? 0 : -1;
  cssCol_ = std::min(cssCol_, cols - 1);
  cssRow_ = std::max(cssRow_, minRow);
  if (nav.up) cssRow_ = std::max(minRow, cssRow_ - 1);
  if (nav.down) cssRow_ = std::min(2, cssRow_ + 1);
  if (cssRow_ >= 0) {
    if (nav.left) cssCol_ = (cssCol_ + cols - 1) % cols;
    if (nav.right) cssCol_ = (cssCol_ + 1) % cols;
  } else if (nav.left || nav.right) {
    rulesCol_ = (rulesCol_ + (nav.left ? 2 : 1)) % 3;
  }
  if (nav.up || nav.down || nav.left || nav.right) audio_.play(Sfx::MenuMove, 0.6f);
  const int fieldCol = cssRow_ < 0 ? rulesCol_ : cssCol_;

  const bool prev = nav.prev;
  const bool back = nav.back && !prev;
  if (nav.confirm && !nav.start) changeField(fieldCol, cssRow_, 1, nav.device);
  if (prev) changeField(fieldCol, cssRow_, -1);

  // Mouse: fields, roster tiles, ready banner
  for (int row = minRow; row <= 2; ++row)
    for (int col = 0; col < (row < 0 ? 3 : cols); ++col) {
      if (!ui::hovered(fieldRect(col, row, training_))) continue;
      if (ui::mouseMoved()) {
        cssRow_ = row;
        if (row < 0) rulesCol_ = col;
        else cssCol_ = col;
      }
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) changeField(col, row, 1);
      if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) changeField(col, row, -1);
    }
  for (int i = 0; i < int(roster().size()); ++i)
    if (ui::clicked(rosterRect(i))) {
      SlotSetup& s = slots_[cssCol_];
      if (!training_ && s.type == 0) s.type = 1;
      s.character = i;
      audio_.play(Sfx::MenuSelect, 0.7f);
    }
  bool ready = activeSlots() >= 2;
  bool clickGo = ready && ui::clicked(readyRect());

  if (nav.start || clickGo) {
    if (ready) {
      audio_.play(Sfx::MenuSelect);
      goTo(Screen::StageSelect);
    } else {
      audio_.play(Sfx::MenuBack);
    }
  } else if (back) {
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

  // Rules
  if (!training_) {
    const Settings& s = settings();
    ui::selector(fieldRect(0, -1, false), TextFormat("VIES : %d", s.stocks), cssRow_ < 0 && rulesCol_ == 0, t);
    ui::selector(fieldRect(1, -1, false), s.items ? "ORBE CLASH : OUI" : "ORBE CLASH : NON",
                 cssRow_ < 0 && rulesCol_ == 1, t);
    ui::selector(fieldRect(2, -1, false), s.weapons ? "ARMES : OUI" : "ARMES : NON", cssRow_ < 0 && rulesCol_ == 2, t);
  } else {
    Rectangle info{820, 42, 410, 40};
    ui::panel(info, withAlpha(ui::kInk, 0.85f), withAlpha(ui::kGold, 0.7f), 0.5f);
    drawTextCentered("Vies infinies  •  Mannequin réglable en pause", info.x + info.width * 0.5f, info.y + 9, 18,
                     ui::kCream, false);
  }

  // Roster strip
  for (int i = 0; i < int(r.size()); ++i) {
    Rectangle rr = rosterRect(i);
    bool hov = ui::hovered(rr);
    Color pc = seriesColor(r[i].series);
    const bool wide = rr.width > rr.height;  // two rows: the portrait on the left, the name beside it
    ui::slanted({rr.x + 5, rr.y + 6, rr.width, rr.height}, 14, withAlpha(BLACK, 0.35f), withAlpha(BLACK, 0.35f));
    ui::slanted(rr, 14, withAlpha(mixColor(pc, ui::kInk, hov ? 0.4f : 0.55f), 0.95f), withAlpha(mixColor(pc, ui::kInk, 0.86f), 0.95f));
    if (wide) {
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
      if (!active || sl.character != i) continue;
      bool cpu = training_ ? s == 1 : sl.type == 2;
      Color tc = cpu ? cpuColor() : portColor(s);
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
    Color pc = type == 1 ? portColor(col) : type == 2 ? cpuColor() : Color{70, 78, 80, 255};
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

    const char* typeName = type == 1 ? "HUMAIN" : type == 2 ? "ORDINATEUR" : "VIDE";
    if (training_) typeName = col == 0 ? "VOUS" : "CIBLE D'ENTRAÎNEMENT";
    Rectangle f0 = fieldRect(col, 0, training_);
    if (training_) {
      DrawRectangleRounded(f0, 0.5f, 10, withAlpha(ui::kInk, 0.6f));
      drawTextCentered(typeName, f0.x + f0.width * 0.5f, f0.y + 8, 18, ui::kMuted, false);
      if (cssRow_ == 0 && cssCol_ == col) ui::focusFrame(f0, t, 0.5f);
    } else {
      ui::selector(f0, typeName, cssRow_ == 0 && cssCol_ == col, t);
    }

    Rectangle f1 = fieldRect(col, 1, training_);
    bool foc1 = cssRow_ == 1 && cssCol_ == col;
    if (type != 0) {
      const CharacterDef& d = r[s.character];
      int pal = 0;
      for (int j = 0; j < col; ++j)
        if ((training_ || slots_[j].type != 0) && slots_[j].character == s.character) pal = 1 - pal;
      Vector2 base{f1.x + f1.width * 0.5f, f1.y + 136};
      ui::glow({base.x, base.y - 50}, 95, withAlpha(mixColor(pc, ui::kCream, 0.5f), 0.4f), 8);
      ui::stoneDisc({base.x, base.y + 2}, 74, 11, t + col, false);
      Pose p = restPose(d, t + col);
      float hop = foc1 ? std::max(0.0f, std::sin(t * 5.0f)) * 6.0f : 0.0f;
      drawCharacterSprite(d, pal, p, {base.x, base.y - hop}, 1, 128.0f / d.height, 1.0f, WHITE, 0.0f, t + col);
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

    Rectangle f2 = fieldRect(col, 2, training_);
    bool foc2 = cssRow_ == 2 && cssCol_ == col;
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

  // "Ready to fight" banner: teal ink, gold edges, a Zonai glow behind the words
  bool ready = activeSlots() >= 2;
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
  ui::footer({{"FLÈCHES", ui::Btn::Move, "Naviguer"}, {"ESPACE", ui::Btn::A, "Modifier"},
              {"K", ui::Btn::X, "Précédent"}, {"ÉCHAP", ui::Btn::B, "Retour"}},
             ready ? nullptr : "Il faut au moins 2 combattants.");
}

// ---------------------------------------------------------------------------
// Stage select
// ---------------------------------------------------------------------------
void App::updateStage(const MenuNav& nav) {
  const int n = int(stageList().size());
  int before = stage_;
  if (nav.left || nav.up) stage_ = (stage_ + n - 1) % n;
  if (nav.right || nav.down) stage_ = (stage_ + 1) % n;
  bool go = nav.confirm || nav.start || ui::clicked(bigPreviewRect());
  for (int i = 0; i < n; ++i) {
    if (!ui::hovered(stageCardRect(i))) continue;
    if (ui::mouseMoved()) stage_ = i;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      stage_ = i;
      go = true;
    }
  }
  if (stage_ != before) audio_.play(Sfx::MenuMove);
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
  ui::stageBackdrop(stage_, t, 0.76f);
  ui::header("ARÈNE", nullptr, {66, 112, 132, 255}, t);

  Rectangle big = bigPreviewRect();
  DrawRectangleRounded({big.x + 8, big.y + 10, big.width, big.height}, 0.04f, 8, withAlpha(BLACK, 0.45f));
  drawStagePreview(stages[stage_], big, t);
  DrawRectangleRoundedLinesEx(ui::grow(big, 3), 0.04f, 8, 3, ui::kGold);
  DrawRectangleRoundedLinesEx(ui::grow(big, -5), 0.03f, 8, 1, withAlpha(ui::kGold, 0.4f));
  ui::corners(ui::grow(big, 9), 0, 30, 3, ui::kGold);
  const Rectangle rib{big.x - 10, big.y + big.height - 64, 460, 54};
  ui::slanted(rib, 22, withAlpha({34, 56, 60, 255}, 0.95f), withAlpha(ui::kInk, 0.95f));
  ui::slantedOutline(rib, 22, 1.5f, ui::kGold);
  drawTextOutlined(stages[stage_].name.c_str(), big.x + 20, big.y + big.height - 58, 38, ui::kCream, ui::kInk, 3.5f,
                   FontKind::Title);
  const Settings& s = settings();
  std::string info = training_ ? "Entraînement  •  vies infinies"
                               : TextFormat("%d combattants  •  %d vie%s  •  orbe %s", activeSlots(), s.stocks,
                                            s.stocks > 1 ? "s" : "", s.items ? "activée" : "désactivée");
  ui::diamond({big.x + 10, big.y + big.height + 28}, 4, ui::kGold);
  drawTextOutlined(info.c_str(), big.x + 22, big.y + big.height + 16, 20, ui::kCream, ui::kInk, 2.0f);

  for (int i = 0; i < int(stages.size()); ++i) {
    Rectangle rr = stageCardRect(i);
    bool sel = i == stage_;
    if (sel) rr = ui::grow(rr, 4);
    DrawRectangleRounded({rr.x + 6, rr.y + 8, rr.width, rr.height}, 0.08f, 8, withAlpha(BLACK, 0.4f));
    drawStagePreview(stages[i], rr, t);
    if (!sel) DrawRectangleRounded(rr, 0.08f, 8, withAlpha(ui::kInk, 0.45f));
    DrawRectangle(int(rr.x), int(rr.y + rr.height - 34), int(rr.width), 34, withAlpha(ui::kInk, 0.85f));
    DrawLineEx({rr.x, rr.y + rr.height - 34}, {rr.x + rr.width, rr.y + rr.height - 34}, 1.0f,
               withAlpha(ui::kGold, sel ? 0.7f : 0.3f));
    drawText(stages[i].name.c_str(), rr.x + 14, rr.y + rr.height - 31, 26, sel ? ui::kGold : ui::kCream,
             FontKind::Title);
    if (sel) {
      const float pulse = 0.5f + 0.5f * std::sin(t * 6.0f);
      DrawRectangleRoundedLinesEx(ui::grow(rr, 6 + pulse * 2), 0.08f, 8, 5, withAlpha(ui::kTeal, 0.14f + 0.14f * pulse));
      DrawRectangleRoundedLinesEx(rr, 0.08f, 8, 3.0f, ui::kGold);
      ui::corners(ui::grow(rr, 7 + pulse * 2), 0, 18, 3, ui::kGold);
    } else {
      DrawRectangleRoundedLinesEx(rr, 0.08f, 8, 1.5f, withAlpha(ui::kStone, 0.6f));
    }
  }
  ui::footer({{"FLÈCHES", ui::Btn::Move, "Choisir"}, {"ENTRÉE", ui::Btn::A, "Combattre !"}, {"ÉCHAP", ui::Btn::B, "Retour"}});
}
