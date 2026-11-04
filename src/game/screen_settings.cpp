// Settings screen: audio, display, rules and keyboard bindings.
#include "game/app.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
const char* kTabNames[4] = {"AUDIO", "AFFICHAGE", "JEU", "COMMANDES"};
// Tab accents (the sliders' fill): orange, blue, green, violet.
const Color kTabColors[4] = {{255, 170, 40, 255}, {72, 156, 255, 255}, {126, 226, 74, 255}, {196, 120, 255, 255}};
// A binding cell: navy, blue when focused, gold while it waits for the new key or button.
void cell(Rectangle k, bool focused, bool waiting) {
  ui::chunky(k, waiting ? ui::Tone::Gold : focused ? ui::Tone::Blue : ui::Tone::Navy, 1.0f, 8.0f);
}
// The focused row: a lighter band and a gold dot.
void rowBand(Rectangle r) {
  ui::roundRect(r, 12, withAlpha(ui::kBlue, 0.28f), withAlpha(ui::kBlue, 0.12f));
  ui::roundLines(r, 12, 1.5f, withAlpha(ui::kGold, 0.5f));
  DrawCircleV({r.x + 13, r.y + r.height * 0.5f}, 6, ui::kInk);
  DrawCircleV({r.x + 13, r.y + r.height * 0.5f}, 4, ui::kGold);
}
const Rectangle kPanel{60, 198, 1160, 440};

struct RowInfo {
  const char* label;
  const char* help;
};
const RowInfo kRows[3][7] = {
    {{"Volume général", "Volume de tout le jeu."},
     {"Musique", "Volume des musiques des menus et des combats."},
     {"Effets sonores", "Coups, sauts, K.O., cris de la foule..."},
     {"Tester le son", "Joue un coup puissant et les cris de la foule."}},
    {{"Plein écran", "Bascule aussi avec la touche F11."},
     {"Qualité graphique", "Haute : contours lissés (rendu en double résolution). Normale : pour les petites machines."},
     {"Secousses de l'écran", "Tremblements de la caméra lors des gros coups et des K.O."},
     {"Noms au-dessus des joueurs", "Affiche J1, J2... ou CPU au-dessus de chaque combattant."},
     {"Compteur d'images (FPS)", "Affiche le nombre d'images par seconde en haut à gauche."},
     {"Zones de coup (hitboxes)", "Montre les zones d'attaque pendant les combats (touche F1)."},
     {"Personnages en pixel art", "Les combattants sont dessinés comme des sprites en pixel art (les menus restent lisses)."}},
    {{"Vies par défaut", "Nombre de vies de chaque combattant au début d'un match."},
     {"Orbe Clash", "L'orbe flottante qui remplit d'un coup la barre d'ulti."},
     {"Armes", "Les deux armes de chaque combattant tombent sur l'arène : il prend la sienne en restant dessus, touche Arme maintenue 1 s. Avec A et B, la touche alterne."},
     {"Niveau des ordinateurs", "Niveau par défaut des combattants contrôlés par l'ordinateur (1 à 10 : le niveau 10 frappe deux fois plus fort)."},
     {"Saut avec Haut", "Comme Smash Bros. : Haut (stick ou touche) fait sauter ; Haut + attaque ou spéciale ensemble reste un coup vers le haut."},
     {"Tout réinitialiser", "Remet tous les paramètres et toutes les touches par défaut."}},
};
const int kRowCounts[4] = {4, 7, 6, kActionCount + 1};  // controls: the actions + the reset buttons

Rectangle tabRect(int i) { return {72.0f + i * 252.0f, 136, 236, 50}; }
Rectangle rowRect(int row) { return {kPanel.x + 20, kPanel.y + 16 + row * 53.0f, kPanel.width - 40, 47}; }
Rectangle widgetRect(int row) {
  Rectangle r = rowRect(row);
  return {r.x + 640, r.y + 6.5f, 440, 34};
}
Rectangle sliderRect(int row) {
  Rectangle w = widgetRect(row);
  return {w.x + 12, w.y + 10, w.width - 110, 14};
}
Rectangle keyRect(int row, int col) { return {kPanel.x + 300 + col * 236.0f, kPanel.y + 48 + row * 28.0f, 216, 25}; }
// The reset row: keyboard 1, keyboard 2, then everything (both keyboards and the gamepad).
Rectangle resetRect(int col) {
  if (col == 2) return {kPanel.x + 824, kPanel.y + 52 + kActionCount * 28.0f, 302, 28};
  return {kPanel.x + 300 + col * 236.0f, kPanel.y + 52 + kActionCount * 28.0f, 216, 28};
}

enum class Widget { Slider, Toggle, Selector, Button };
Widget widgetFor(int tab, int row) {
  if (tab == 0) return row < 3 ? Widget::Slider : Widget::Button;
  if (tab == 1) return (row == 1 || row == 2) ? Widget::Selector : Widget::Toggle;
  if (row == 1 || row == 2 || row == 4) return Widget::Toggle;
  return row == 5 ? Widget::Button : Widget::Selector;
}
int* volumeFor(int row) {
  Settings& s = settings();
  return row == 0 ? &s.masterVolume : row == 1 ? &s.musicVolume : &s.sfxVolume;
}

// ---- Gamepad controls ------------------------------------------------------------------
// The game actions a pad can rebind (Saut ... Ultime), two buttons each, then the reset button.
constexpr int kPadActionRows = kActionCount - kActJump;
constexpr int kPadRows = kPadActionRows + 1;
int padAction(int row) { return kActJump + row; }
Rectangle padCellRect(int row, int col) { return {kPanel.x + 262 + col * 150.0f, kPanel.y + 76 + row * 34.0f, 136, 30}; }
// The reset row: the gamepad, then everything (the gamepad and both keyboards).
Rectangle padResetRect(int col = 0) { return {kPanel.x + 262 + col * 150.0f, kPanel.y + 84 + kPadActionRows * 34.0f, 136, 32}; }
void resetAllControls() {
  Settings& s = settings();
  s.resetKeys(0);
  s.resetKeys(1);
  s.resetPad();
  saveSettings();
}

// The pad the tab shows: the first connected one of that family (-1: none, the saved bindings).
int shownPad(PadFamily f) {
  for (int p = 0; p < kMaxPads; ++p)
    if (padConnected(p) && padFamily(p) == f) return p;
  return -1;
}
int boundButton(int pad, int action, int slot) {
  return pad >= 0 ? padBinding(pad, action, slot) : settings().pad[action][slot];
}
// A pad without triggers shows RB / Back while the bindings are the defaults: an edit starts from that.
void materializePadFallback() {
  Settings& s = settings();
  const int pad = shownPad(promptFamily());
  if (pad >= 0 && s.padIsDefault() && !padHasTriggers(pad)) {
    s.pad[kActShield] = {kPadRB, -1};
    s.pad[kActGrab] = {kPadLB, -1};
  }
}

// Draws one or two glyphs (vertically centred on cy), returns the width.
float glyphs(ui::Btn a, ui::Btn b, float x, float cy, float h, PadFamily f) {
  float w = ui::padGlyph(a, x, cy - h * 0.5f, h, f);
  if (b != ui::Btn::None) w += 6 + ui::padGlyph(b, x + w + 6, cy - h * 0.5f, h, f);
  return w;
}

void ellipse(Vector2 c, float rx, float ry, float rot, Color col) {
  Vector2 pts[42];
  pts[0] = c;
  for (int i = 0; i <= 40; ++i) {
    const float a = i * 2.0f * kPi / 40, ex = std::cos(a) * rx, ey = std::sin(a) * ry;
    pts[i + 1] = {c.x + ex * std::cos(rot) - ey * std::sin(rot), c.y + ex * std::sin(rot) + ey * std::cos(rot)};
  }
  DrawTriangleFan(pts, 42, col);
}

// Where things sit on the controller (from its centre).
struct PadLayout {
  Vector2 lstick, rstick, dpad, face, start, select;
};
PadLayout layoutFor(PadFamily f) {
  if (f == PadFamily::PlayStation) return {{-62, 38}, {62, 38}, {-122, -24}, {122, -24}, {86, -76}, {-86, -76}};
  return {{-122, -24}, {62, 38}, {-62, 38}, {122, -24}, {34, -46}, {-34, -46}};
}

// The controller of the family, centred on c, with button `hl` circled (the cell being edited).
void drawController(PadFamily f, Vector2 at0, float scale, int hl, float t) {
  rlPushMatrix();
  rlTranslatef(at0.x, at0.y, 0);
  rlScalef(scale, scale, 1);
  const Vector2 c{0, 0};
  const PadLayout L = layoutFor(f);
  auto at = [&](Vector2 v) { return Vector2{c.x + v.x, c.y + v.y}; };
  const Color body = f == PadFamily::PlayStation ? Color{214, 216, 222, 255}
                     : f == PadFamily::Nintendo  ? Color{58, 62, 68, 255}
                                                 : Color{70, 78, 84, 255};
  auto shapes = [&](float g, Vector2 o, Color col) {
    DrawRectangleRounded({c.x - 180 - g + o.x, c.y - 94 - g + o.y, 360 + 2 * g, 126 + 2 * g}, 0.62f, 16, col);
    ellipse({c.x - 130 + o.x, c.y + 44 + o.y}, 58 + g, 84 + g, 0.38f, col);
    ellipse({c.x + 130 + o.x, c.y + 44 + o.y}, 58 + g, 84 + g, -0.38f, col);
  };
  shapes(4, {7, 9}, withAlpha(BLACK, 0.35f));
  shapes(4, {0, 0}, ui::kInk);
  shapes(0, {0, 0}, body);
  DrawRectangleRounded({c.x - 140, c.y - 88, 280, 18}, 1.0f, 12, withAlpha(WHITE, 0.1f));
  const Color well = shade(body, 0.72f);
  if (f == PadFamily::PlayStation) {
    const Rectangle pad{c.x - 62, c.y - 92, 124, 66};
    DrawRectangleRounded(ui::grow(pad, 2), 0.25f, 8, ui::kInk);
    DrawRectangleRounded(pad, 0.25f, 8, shade(body, 0.86f));
    DrawCircleV(at({0, 14}), 9, ui::kInk);
  } else {
    DrawCircleV(at({0, -72}), 13, well);
    DrawCircleV(at({0, -72}), 9, shade(body, 1.25f));
  }
  for (Vector2 v : {L.lstick, L.rstick, L.dpad}) DrawCircleV(at(v), 32, well);
  DrawCircleV(at(L.face), 48, well);

  // Buttons, and where each one is (for the highlight)
  Vector2 pos[kPadButtonCount] = {};
  const float sh = 26, d = 27, fh = 28;
  for (int side = -1; side <= 1; side += 2) {
    const ui::Btn trig = side < 0 ? ui::Btn::LT : ui::Btn::RT, bump = side < 0 ? ui::Btn::LB : ui::Btn::RB;
    ui::padGlyph(trig, c.x + side * 124 - ui::padGlyphWidth(trig, sh, f) * 0.5f, c.y - 164, sh, f);
    ui::padGlyph(bump, c.x + side * 124 - ui::padGlyphWidth(bump, sh, f) * 0.5f, c.y - 128, sh, f);
    pos[side < 0 ? kPadLT : kPadRT] = {c.x + side * 124, c.y - 164 + sh * 0.5f};
    pos[side < 0 ? kPadLB : kPadRB] = {c.x + side * 124, c.y - 128 + sh * 0.5f};
  }
  auto centred = [&](ui::Btn b, Vector2 p, float h) { ui::padGlyph(b, p.x - ui::padGlyphWidth(b, h, f) * 0.5f, p.y - h * 0.5f, h, f); };
  centred(ui::Btn::Start, at(L.start), 24);
  centred(ui::Btn::Back, at(L.select), 24);
  centred(ui::Btn::LStick, at(L.lstick), 40);
  centred(ui::Btn::RStick, at(L.rstick), 40);
  centred(ui::Btn::DPad, at(L.dpad), 46);
  const Vector2 fc = at(L.face);
  pos[kPadY] = {fc.x, fc.y - d};
  pos[kPadX] = {fc.x - d, fc.y};
  pos[kPadB] = {fc.x + d, fc.y};
  pos[kPadA] = {fc.x, fc.y + d};
  for (int b : {kPadY, kPadX, kPadB, kPadA}) centred(ui::btnFor(b), pos[b], fh);
  pos[kPadStart] = at(L.start);
  pos[kPadBack] = at(L.select);
  pos[kPadL3] = at(L.lstick);
  pos[kPadR3] = at(L.rstick);
  if (hl >= 0 && hl < kPadButtonCount) {
    const float pulse = 0.5f + 0.5f * std::sin(t * 7.0f);
    const float r = (hl == kPadL3 || hl == kPadR3) ? 26.0f : (hl >= kPadLB && hl <= kPadRT) ? 24.0f : 18.0f;
    ui::glow(pos[hl], r * 2.2f, withAlpha(ui::kGold, 0.35f + 0.2f * pulse), 6);
    DrawRing(pos[hl], r + 2 + 2 * pulse, r + 5 + 2 * pulse, 0, 360, 40, ui::kGold);
  }
  rlPopMatrix();
}

// Gamepad prompt mode: the bindings table (2 buttons per action) next to the controller.
void drawPadBindings(PadFamily f, int row, int col, bool waiting, float t) {
  const int pad = shownPad(f);
  std::string title = pad >= 0 ? padModel(pad)
                      : f == PadFamily::PlayStation ? "Manette PlayStation"
                      : f == PadFamily::Nintendo    ? "Manette Nintendo"
                                                    : "Manette Xbox";
  for (char& ch : title)
    if (ch >= 'a' && ch <= 'z') ch = char(ch - 'a' + 'A');
  ui::bold(title.c_str(), kPanel.x + 30, kPanel.y + 14, 28, ui::kGold);

  drawText("ACTION", kPanel.x + 46, kPanel.y + 52, 17, ui::kMuted);
  for (int c = 0; c < 2; ++c) {
    Rectangle k = padCellRect(0, c);
    drawTextCentered(c == 0 ? "BOUTON 1" : "BOUTON 2", k.x + k.width * 0.5f, kPanel.y + 51, 17, ui::kMuted, false);
  }
  int hl = -1;
  for (int r = 0; r < kPadActionRows; ++r) {
    const int a = padAction(r);
    const bool rowFoc = r == row;
    const Rectangle k0 = padCellRect(r, 0);
    if (rowFoc) rowBand({kPanel.x + 22, k0.y - 3, 540, k0.height + 6});
    drawTextOutlined(actionName(a), kPanel.x + 46, k0.y + 4, 19,
                     a == kActUltimate ? ui::kGold : rowFoc ? ui::kCream : ui::kMuted, ui::kInk, 2.0f);
    for (int c = 0; c < 2; ++c) {
      const Rectangle k = padCellRect(r, c);
      const bool foc = rowFoc && c == col, wait = foc && waiting;
      cell(k, foc, wait);
      const int b = boundButton(pad, a, c);
      if (wait) {
        drawTextCentered(int(t * 3) % 2 ? "..." : "", k.x + k.width * 0.5f, k.y + 4, 17, ui::kInk, false);
      } else if (b >= 0) {
        const ui::Btn g = ui::btnFor(b);
        ui::padGlyph(g, k.x + k.width * 0.5f - ui::padGlyphWidth(g, 24, f) * 0.5f, k.y + 3, 24, f);
      } else {
        drawTextCentered("—", k.x + k.width * 0.5f, k.y + 4, 17, ui::kMuted, false);
      }
      if (foc) {
        hl = b;
        if (!wait) ui::focusFrame(k, t, 0.4f);
      }
    }
  }
  for (int c = 0; c < 2; ++c) {
    const Rectangle rb = padResetRect(c);
    const bool resetFoc = row == kPadActionRows && col == c;
    ui::button(rb, c == 0 ? "RÉINITIALISER" : "TOUT + CLAVIERS", c == 1 ? ui::Tone::Red : ui::Tone::Blue, resetFoc, t,
               1.0f, 15.0f);
  }

  // What never changes
  float x = kPanel.x + 30, y = kPanel.y + kPanel.height - 72;
  drawText("Toujours :", x, y - 11, 18, ui::kMuted);
  x += textWidth("Toujours :", 18) + 12;
  const struct { ui::Btn b; const char* label; } fixed[3] = {
      {ui::Btn::Move, "Se déplacer"}, {ui::Btn::RStick, "Attaque smash"}, {ui::Btn::Start, "Pause"}};
  for (const auto& fx : fixed) {
    x += glyphs(fx.b, ui::Btn::None, x, y, 24, f) + 8;
    drawText(fx.label, x, y - 11, 18, ui::kCream);
    x += textWidth(fx.label, 18) + 22;
  }
  // Menus
  x = kPanel.x + 30;
  y = kPanel.y + kPanel.height - 34;
  drawText("Menus :", x, y - 11, 18, ui::kMuted);
  x += textWidth("Menus :", 18) + 14;
  const struct { ui::Btn b; const char* label; } menu[4] = {
      {ui::Btn::A, "Valider"}, {ui::Btn::B, "Retour"}, {ui::Btn::X, "Valeur précédente"}, {ui::Btn::Start, "Commencer"}};
  for (const auto& m : menu) {
    x += glyphs(m.b, ui::Btn::None, x, y, 24, f) + 8;
    drawText(m.label, x, y - 11, 18, ui::kCream);
    x += textWidth(m.label, 18) + 24;
  }
  drawController(f, {kPanel.x + 868, kPanel.y + 238}, 0.82f, hl, t);
}
}  // namespace

int App::settingsRowCount() const {
  if (setTab_ == 3 && padPrompts()) return kPadRows;  // gamepad bindings
  return kRowCounts[setTab_];
}

// Gives `button` to an action's slot; an action that used it gets this slot's old button.
bool App::bindPadButton(int action, int slot, int button) {
  materializePadFallback();
  auto& P = settings().pad;
  const int old = P[action][slot];
  if (old == button) return true;
  for (int a = kActJump; a < kActionCount; ++a)
    for (int sl = 0; sl < 2; ++sl) {
      if ((a == action && sl == slot) || P[a][sl] != button) continue;
      if (old < 0 && a != action && a != kActSmash && P[a][1 - sl] < 0) {  // it would lose its only button
        toast(TextFormat("C'est le seul bouton de « %s »", actionName(a)));
        return false;
      }
      P[a][sl] = old;
    }
  P[action][slot] = button;
  saveSettings();
  return true;
}

void App::changeSetting(int row, int delta) {
  Settings& s = settings();
  switch (setTab_) {
    case 0:
      if (row < 3) {
        int* v = volumeFor(row);
        *v = std::clamp(*v + delta * 5, 0, 100);
      } else {
        audio_.play(Sfx::HitHeavy, 1.0f);
        audio_.play(Sfx::Crowd, 0.8f);
      }
      break;
    case 1:
      if (row == 0) s.fullscreen = !s.fullscreen;
      if (row == 1) s.highQuality = !s.highQuality;
      if (row == 2) s.screenShake = (s.screenShake + delta + 3) % 3;
      if (row == 3) s.playerTags = !s.playerTags;
      if (row == 4) s.showFps = !s.showFps;
      if (row == 5) s.showHitboxes = !s.showHitboxes;
      if (row == 6) s.pixelArt = !s.pixelArt;
      break;
    case 2:
      if (row == 0) s.stocks = 1 + (s.stocks - 1 + delta + 5) % 5;
      if (row == 1) s.items = !s.items;
      if (row == 2) s.weapons = !s.weapons;
      if (row == 3) s.cpuLevel = 1 + (s.cpuLevel - 1 + delta + kMaxCpuLevel) % kMaxCpuLevel;
      if (row == 4) s.tapJump = !s.tapJump;
      if (row == 5) {
        s = Settings();
        toast("Paramètres par défaut rétablis");
      }
      break;
    default:
      break;
  }
  applyDisplaySettings();
  saveSettings();
  if (!(setTab_ == 0 && row == 3)) audio_.play(Sfx::MenuMove, 0.8f);
}

void App::updateSettings(const MenuNav& nav) {
  Settings& s = settings();
  // Keyboard table <-> gamepad table: the press that switches only brings the focus to the top,
  // it never activates the row under it (e.g. RÉINITIALISER) and ends any capture.
  bool switched = false;
  if (padPrompts() != lastPadPrompts_) {
    lastPadPrompts_ = padPrompts();
    if (setTab_ == 3) {
      setRow_ = std::min(setRow_, 0);
      if (waitingKey_ || waitingPad_) audio_.play(Sfx::MenuBack);
      waitingKey_ = waitingPad_ = false;
      switched = true;
    }
  }
  const int rows = settingsRowCount();
  setRow_ = std::min(setRow_, rows - 1);  // the Commandes tab has fewer rows in gamepad mode
  const bool padTable = setTab_ == 3 && padPrompts();

  if (waitingPad_) {  // rebinding a gamepad button: the next button pressed on any pad
    if (!padTable || IsKeyPressed(KEY_ESCAPE) || padAnyPressed(kPadStart) || setRow_ < 0 ||
        setRow_ >= kPadActionRows) {
      waitingPad_ = false;
      audio_.play(Sfx::MenuBack);
      return;
    }
    for (int p = 0; p < kMaxPads; ++p)
      for (int b = 0; b < kPadButtonCount; ++b)
        if (padBindable(b) && padPressed(p, b)) {
          waitingPad_ = false;
          audio_.play(bindPadButton(padAction(setRow_), setCol_, b) ? Sfx::MenuSelect : Sfx::MenuBack);
          return;
        }
    return;
  }

  if (waitingKey_) {  // rebinding a key
    if (padTable || padAnyPressed(kPadB) || setRow_ < 0 || setRow_ >= kActionCount) {
      waitingKey_ = false;
      audio_.play(Sfx::MenuBack);
      return;
    }
    int key = GetKeyPressed();
    while (key != 0) {
      if (key == KEY_ESCAPE) {
        waitingKey_ = false;
        audio_.play(Sfx::MenuBack);
        break;
      }
      if (keyBindable(key)) {
        int& slot = s.keys[setCol_][setRow_];
        for (int kb = 0; kb < 2; ++kb)  // the key was used elsewhere: swap
          for (int a = 0; a < kActionCount; ++a)
            if (s.keys[kb][a] == key && !(kb == setCol_ && a == setRow_)) s.keys[kb][a] = slot;
        slot = key;
        waitingKey_ = false;
        saveSettings();
        audio_.play(Sfx::MenuSelect);
        break;
      }
      key = GetKeyPressed();
    }
    return;
  }

  int before = setRow_ * 10 + setTab_ * 100 + setCol_;
  if (nav.up && setRow_ >= 0) setRow_--;
  if (nav.down && setRow_ < rows - 1) setRow_++;
  if (setTab_ == 3 && setRow_ != kActionCount) setCol_ = std::min(setCol_, 1);  // the 3rd column is the reset row's
  if (nav.left || nav.right) {
    int d = nav.left ? -1 : 1;
    if (setRow_ < 0) {
      setTab_ = (setTab_ + d + 4) % 4;
    } else if (setTab_ == 3 && !padTable && setRow_ == kActionCount) {  // the reset row: 3 buttons
      setCol_ = (setCol_ + d + 3) % 3;
    } else if (setTab_ == 3) {
      setCol_ = 1 - std::min(setCol_, 1);
    } else {
      changeSetting(setRow_, d);
    }
  }
  if (!switched && padTable && nav.prev && setRow_ >= 0 && setRow_ < kPadActionRows) {  // clear a slot
    materializePadFallback();
    const int a = padAction(setRow_);
    auto& slots = s.pad[a];
    if (slots[setCol_] >= 0 && (a == kActSmash || slots[1 - setCol_] >= 0)) {
      slots[setCol_] = -1;
      saveSettings();
      audio_.play(Sfx::MenuBack);
    } else if (slots[setCol_] >= 0) {
      toast(TextFormat("« %s » a besoin d'au moins un bouton", actionName(a)));
    }
  }
  if (switched) {
  } else if (nav.confirm && !waitingKey_ && padTable) {
    if (setRow_ < 0) {
      setRow_ = 0;
    } else if (setRow_ < kPadActionRows) {
      waitingPad_ = true;
    } else if (setCol_ == 1) {
      resetAllControls();
      toast("Touches du clavier et boutons de la manette réinitialisés");
    } else {
      s.resetPad();
      saveSettings();
      toast("Boutons de la manette réinitialisés");
    }
    audio_.play(Sfx::MenuSelect);
  } else if (nav.confirm && !waitingKey_) {
    if (setRow_ < 0) {
      setRow_ = 0;
    } else if (setTab_ == 3) {
      if (setRow_ < kActionCount) {
        waitingKey_ = true;
        while (GetKeyPressed() != 0) {}
      } else if (setCol_ == 2) {
        resetAllControls();
        toast("Touches du clavier et boutons de la manette réinitialisés");
      } else {
        s.resetKeys(setCol_);
        saveSettings();
        toast(setCol_ == 0 ? "Touches du clavier 1 réinitialisées" : "Touches du clavier 2 réinitialisées");
      }
      audio_.play(Sfx::MenuSelect);
    } else {
      changeSetting(setRow_, 1);
    }
  }

  // Mouse
  for (int i = 0; i < 4; ++i)
    if (ui::clicked(tabRect(i))) {
      setTab_ = i;
      setRow_ = -1;
    }
  const bool pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
  if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) dragSlider_ = -1;
  if (setTab_ < 3) {
    for (int r = 0; r < kRowCounts[setTab_]; ++r) {
      if (ui::hovered(rowRect(r)) && ui::mouseMoved() && dragSlider_ < 0) setRow_ = r;
      Widget w = widgetFor(setTab_, r);
      Rectangle wr = widgetRect(r);
      if (w == Widget::Slider && pressed && ui::hovered(ui::grow(wr, 6))) dragSlider_ = r;
      if (w == Widget::Slider && dragSlider_ == r) {
        Rectangle sr = sliderRect(r);
        int v = int(std::lround(clampf((GetMousePosition().x - sr.x) / sr.width, 0.0f, 1.0f) * 100.0f));
        if (v != *volumeFor(r)) {
          *volumeFor(r) = v;
          applyDisplaySettings();
        }
        setRow_ = r;
      } else if (pressed && ui::hovered(wr)) {
        setRow_ = r;
        if (w == Widget::Selector) changeSetting(r, GetMousePosition().x < wr.x + wr.width * 0.5f ? -1 : 1);
        else changeSetting(r, 1);
      }
    }
  } else if (padTable) {  // gamepad bindings
    for (int r = 0; r < kPadActionRows; ++r)
      for (int c = 0; c < 2; ++c) {
        if (ui::hovered(padCellRect(r, c)) && ui::mouseMoved()) setRow_ = r, setCol_ = c;
        if (ui::clicked(padCellRect(r, c))) {
          setRow_ = r;
          setCol_ = c;
          waitingPad_ = true;
          audio_.play(Sfx::MenuSelect);
        }
      }
    if (ui::clicked(padResetRect(0))) {
      setRow_ = kPadActionRows;
      setCol_ = 0;
      s.resetPad();
      saveSettings();
      toast("Boutons de la manette réinitialisés");
    }
    if (ui::clicked(padResetRect(1))) {
      setRow_ = kPadActionRows;
      setCol_ = 1;
      resetAllControls();
      toast("Touches du clavier et boutons de la manette réinitialisés");
    }
  } else {  // keyboard table
    for (int r = 0; r < kActionCount; ++r)
      for (int c = 0; c < 2; ++c) {
        if (ui::hovered(keyRect(r, c)) && ui::mouseMoved()) setRow_ = r, setCol_ = c;
        if (ui::clicked(keyRect(r, c))) {
          setRow_ = r;
          setCol_ = c;
          waitingKey_ = true;
          while (GetKeyPressed() != 0) {}
          audio_.play(Sfx::MenuSelect);
        }
      }
    for (int c = 0; c < 2; ++c)
      if (ui::clicked(resetRect(c))) {
        setRow_ = kActionCount;
        setCol_ = c;
        s.resetKeys(c);
        saveSettings();
        toast(c == 0 ? "Touches du clavier 1 réinitialisées" : "Touches du clavier 2 réinitialisées");
      }
    if (ui::clicked(resetRect(2))) {
      setRow_ = kActionCount;
      setCol_ = 2;
      resetAllControls();
      toast("Touches du clavier et boutons de la manette réinitialisés");
    }
  }
  if (dragSlider_ >= 0 && !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) saveSettings();
  if (setRow_ * 10 + setTab_ * 100 + setCol_ != before) audio_.play(Sfx::MenuMove, 0.6f);
  setRow_ = std::min(setRow_, settingsRowCount() - 1);

  if (nav.back) {
    saveSettings();
    audio_.play(Sfx::MenuBack);
    toast("Paramètres enregistrés");
    goTo(settingsReturn_);
  }
}

void App::drawSettings() {
  const float t = time_;
  const Settings& s = settings();
  const Color accent = kTabColors[setTab_];
  ui::stageBackdrop(0, t, 0.6f);
  ui::header("PARAMÈTRES", "Les réglages sont enregistrés automatiquement.", kTabColors[1], t);

  // Tabs: chunky buttons, the open one gold
  for (int i = 0; i < 4; ++i) {
    const bool sel = i == setTab_;
    Rectangle r = tabRect(i);
    if (sel) r = {r.x, r.y - 6, r.width, r.height + 6};
    ui::button(r, kTabNames[i], sel ? ui::Tone::Gold : ui::Tone::Navy, sel && setRow_ < 0, t, 1.0f, 26.0f);
  }
  ui::card(kPanel, 0.96f);

  const char* help = nullptr;
  if (setTab_ < 3) {
    for (int r = 0; r < kRowCounts[setTab_]; ++r) {
      Rectangle rr = rowRect(r);
      bool foc = r == setRow_;
      if (foc) {
        rowBand(rr);
        help = kRows[setTab_][r].help;
      }
      drawTextOutlined(kRows[setTab_][r].label, rr.x + 28, rr.y + 9, 24, foc ? ui::kCream : ui::kMuted, ui::kInk, 2.0f);
      Rectangle w = widgetRect(r);
      switch (widgetFor(setTab_, r)) {
        case Widget::Slider: {
          int v = *volumeFor(r);
          ui::slider(sliderRect(r), v / 100.0f, foc, accent, t);
          ui::bold(TextFormat("%d %%", v), w.x + w.width - 82, w.y + 2, 24);
          break;
        }
        case Widget::Toggle: {
          bool on = setTab_ == 1 ? (r == 0   ? s.fullscreen
                                    : r == 3 ? s.playerTags
                                    : r == 4 ? s.showFps
                                    : r == 6 ? s.pixelArt
                                             : s.showHitboxes)
                                 : r == 2 ? s.weapons : r == 4 ? s.tapJump : s.items;
          ui::toggle({w.x + w.width - 120, w.y, 120, w.height}, on, foc, t);
          break;
        }
        case Widget::Selector: {
          const char* shakes[3] = {"Aucune", "Légères", "Normales"};
          const char* v = "";
          if (setTab_ == 1 && r == 1) v = s.highQuality ? "Haute (lissage)" : "Normale";
          if (setTab_ == 1 && r == 2) v = shakes[std::clamp(s.screenShake, 0, 2)];
          if (setTab_ == 2 && r == 0) v = TextFormat("%d vie%s", s.stocks, s.stocks > 1 ? "s" : "");
          if (setTab_ == 2 && r == 3) v = TextFormat("Niveau %d", s.cpuLevel);
          ui::selector({w.x + 120, w.y, w.width - 120, w.height}, v, foc, t);
          break;
        }
        case Widget::Button: {
          const Rectangle b{w.x + w.width - 220, w.y - 2, 220, w.height + 4};
          ui::button(b, setTab_ == 0 ? "ÉCOUTER" : "RÉINITIALISER", setTab_ == 0 ? ui::Tone::Blue : ui::Tone::Red, foc, t,
                     1.0f, 20.0f);
          break;
        }
      }
    }
  } else if (padPrompts()) {
    drawPadBindings(promptFamily(), setRow_, setCol_, waitingPad_, t);
  } else {
    // Keyboard bindings table
    drawTextOutlined("ACTION", kPanel.x + 46, kPanel.y + 16, 18, ui::kMuted, ui::kInk, 2.0f);
    for (int c = 0; c < 2; ++c) {
      Rectangle k = keyRect(0, c);
      ui::boldCentered(c == 0 ? "CLAVIER 1" : "CLAVIER 2", k.x + k.width * 0.5f, kPanel.y + 11, 20, portColor(c));
    }
    for (int r = 0; r < kActionCount; ++r) {
      bool rowFoc = r == setRow_;
      Rectangle k0 = keyRect(r, 0);
      if (rowFoc) rowBand({kPanel.x + 22, k0.y - 2, 750, k0.height + 4});
      drawTextOutlined(actionName(r), kPanel.x + 46, k0.y + 2, 18, rowFoc ? ui::kCream : ui::kMuted, ui::kInk, 2.0f);
      for (int c = 0; c < 2; ++c) {
        Rectangle k = keyRect(r, c);
        bool foc = rowFoc && c == setCol_;
        bool wait = foc && waitingKey_;
        cell(k, foc, wait);
        std::string name = wait ? (int(t * 3) % 2 ? "..." : "") : keyName(s.keys[c][r]);
        drawTextCentered(name.c_str(), k.x + k.width * 0.5f, k.y + 2, 16, WHITE, true);
        if (foc && !wait) ui::focusFrame(k, t, 0.4f);
      }
    }
    for (int c = 0; c < 3; ++c) {
      Rectangle b = resetRect(c);
      bool foc = setRow_ == kActionCount && setCol_ == c;
      ui::button(b, c == 2 ? "TOUT RÉINITIALISER (+ MANETTE)" : "RÉINITIALISER", c == 2 ? ui::Tone::Red : ui::Tone::Blue,
                 foc, t, 1.0f, 15.0f);
    }
    drawText("Combos :  Attaque + Spéciale = smash   •   Bas, Bas-avant, Avant + Attaque = smash avant chargé",
             kPanel.x + 46, kPanel.y + 413, 12, ui::kMuted);
    drawText("Bas, Haut + Attaque = smash haut chargé   •   Touche Ultime (U) = ultime",
             kPanel.x + 103, kPanel.y + 426, 12, ui::kMuted);
    // Gamepad reference
    const PadFamily f = promptFamily();
    Rectangle box{kPanel.x + 806, kPanel.y + 16, 338, kPanel.height - 32};
    ui::roundRect(box, 14, withAlpha(ui::kInk, 0.55f), withAlpha(ui::kInk, 0.4f));
    ui::roundLines(box, 14, 1.5f, withAlpha(ui::kSky, 0.3f));
    ui::bold("MANETTE", box.x + 18, box.y + 8, 26, ui::kGold);
    const int pad = shownPad(f);
    for (int i = 0; i < kPadActionRows + 2; ++i) {  // moving, the bindings, pause
      const float y = box.y + 60 + i * 30.0f;
      const int a = i == 0 ? kActLeft : i <= kPadActionRows ? padAction(i - 1) : -1;
      ui::Btn b0 = a < 0 ? ui::Btn::Start : a == kActLeft ? ui::Btn::Move : ui::Btn::None, b1 = ui::Btn::None;
      if (a >= kActJump) {
        const int x0 = boundButton(pad, a, 0), x1 = boundButton(pad, a, 1);
        b0 = x0 >= 0 ? ui::btnFor(x0) : a == kActSmash ? ui::Btn::RStick : ui::Btn::None;
        b1 = x1 >= 0 ? ui::btnFor(x1) : (a == kActSmash && x0 >= 0) ? ui::Btn::RStick : ui::Btn::None;
        if (b0 == ui::Btn::None) std::swap(b0, b1);
      }
      glyphs(b0, b1, box.x + 18, y, 24, f);
      drawText(a < 0 ? "Pause" : a == kActLeft ? "Se déplacer" : actionName(a), box.x + 128, y - 11, 17,
               a == kActUltimate ? ui::kGold : ui::kCream);
    }
    drawText("Clavier 2 : le pavé numérique marche aussi (0 à 7).", box.x + 18, box.y + box.height - 66, 13, ui::kMuted);
  }
  if (help) {
    float y = kPanel.y + kPanel.height - 40;
    DrawCircleV({kPanel.x + 40, y + 12}, 14, ui::kInk);
    DrawCircleV({kPanel.x + 40, y + 12}, 12, ui::kGold);
    drawTextCentered("i", kPanel.x + 40, y + 1, 18, ui::kInk, false);
    drawTextOutlined(help, kPanel.x + 62, y + 2, 18, ui::kCream, ui::kInk, 2.0f);
  }

  if (waitingPad_) {
    DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(ui::kInk, 0.65f));
    Rectangle m{kScreenW * 0.5f - 330, kScreenH * 0.5f - 90, 660, 170};
    ui::card(m);
    drawTextCentered(TextFormat("Nouveau bouton : %s", actionName(padAction(std::clamp(setRow_, 0, kPadActionRows - 1)))),
                     m.x + m.width * 0.5f, m.y + 28, 34, ui::kCream, true, FontKind::Title);
    drawTextCentered(setCol_ == 0 ? "Bouton 1" : "Bouton 2", m.x + m.width * 0.5f, m.y + 76, 20, ui::kGold);
    const char* a = "Appuyez sur un bouton de la manette  —  ";
    const char* b = " pour annuler";
    const PadFamily f = promptFamily();
    const float gw = ui::padGlyphWidth(ui::Btn::Start, 24, f);
    float x = m.x + m.width * 0.5f - (textWidth(a, 19) + 6 + gw + textWidth(b, 19)) * 0.5f;
    drawText(a, x, m.y + 112, 19, ui::kMuted);
    x += textWidth(a, 19) + 3;
    ui::padGlyph(ui::Btn::Start, x, m.y + 110, 24, f);
    drawText(b, x + gw + 3, m.y + 112, 19, ui::kMuted);
  }
  if (waitingKey_) {
    DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(ui::kInk, 0.65f));
    Rectangle m{kScreenW * 0.5f - 330, kScreenH * 0.5f - 90, 660, 170};
    ui::card(m);
    drawTextCentered(TextFormat("Nouvelle touche : %s", actionName(setRow_)), m.x + m.width * 0.5f, m.y + 28, 34,
                     ui::kCream, true, FontKind::Title);
    drawTextCentered(setCol_ == 0 ? "Clavier 1" : "Clavier 2", m.x + m.width * 0.5f, m.y + 76, 20, portColor(setCol_));
    drawTextCentered("Appuyez sur une touche  —  ÉCHAP pour annuler", m.x + m.width * 0.5f, m.y + 112, 19,
                     ui::kMuted, false);
  }
  using ui::Btn;
  if (waitingPad_)
    ui::footer({{"ÉCHAP", Btn::Start, "Annuler"}}, "Appuyez sur le nouveau bouton.");
  else if (waitingKey_)
    ui::footer({{"ÉCHAP", Btn::B, "Annuler"}}, "Appuyez sur la nouvelle touche.");
  else if (setTab_ == 3 && padPrompts() && setRow_ >= 0)
    ui::footer({{"FLÈCHES", Btn::DPad, "Naviguer"}, {"ENTRÉE", Btn::A, "Changer"}, {"K", Btn::X, "Effacer"},
                {"ÉCHAP", Btn::B, "Retour"}},
               setRow_ < kPadActionRows ? "Un bouton déjà utilisé est échangé." : "Boutons d'origine.");
  else if (setRow_ < 0)
    ui::footer({{"GAUCHE/DROITE", Btn::DPadLR, "Onglet"}, {"BAS", Btn::DPadDown, "Réglages"}, {"ÉCHAP", Btn::B, "Retour"}});
  else if (setTab_ == 3)
    ui::footer({{"FLÈCHES", Btn::DPad, "Naviguer"}, {"ENTRÉE", Btn::A, "Changer"}, {"ÉCHAP", Btn::B, "Retour"}},
               setRow_ < kActionCount ? "Une touche déjà utilisée est échangée." : "Touches d'origine.");
  else
    ui::footer({{"HAUT/BAS", Btn::DPadUD, "Choisir"}, {"GAUCHE/DROITE", Btn::DPadLR, "Modifier"}, {"ÉCHAP", Btn::B, "Retour"}});
}
