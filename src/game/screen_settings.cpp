// Settings screen: audio, display, rules and keyboard bindings.
#include <cstring>

#include "game/app.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
const char* kTabNames[4] = {"AUDIO", "AFFICHAGE", "JEU", "COMMANDES"};
// Earthy tab accents: terracotta, slate blue, moss, ochre.
const Color kTabColors[4] = {{184, 88, 62, 255}, {70, 120, 150, 255}, {96, 140, 78, 255}, {196, 150, 66, 255}};
const Rectangle kPanel{60, 198, 1160, 440};

struct RowInfo {
  const char* label;
  const char* help;
};
const RowInfo kRows[3][6] = {
    {{"Volume général", "Volume de tout le jeu."},
     {"Musique", "Volume des musiques des menus et des combats."},
     {"Effets sonores", "Coups, sauts, K.O., cris de la foule..."},
     {"Tester le son", "Joue un coup puissant et les cris de la foule."}},
    {{"Plein écran", "Bascule aussi avec la touche F11."},
     {"Qualité graphique", "Haute : contours lissés (rendu en double résolution). Normale : pour les petites machines."},
     {"Secousses de l'écran", "Tremblements de la caméra lors des gros coups et des K.O."},
     {"Noms au-dessus des joueurs", "Affiche J1, J2... ou CPU au-dessus de chaque combattant."},
     {"Compteur d'images (FPS)", "Affiche le nombre d'images par seconde en haut à gauche."},
     {"Zones de coup (hitboxes)", "Montre les zones d'attaque pendant les combats (touche F1)."}},
    {{"Vies par défaut", "Nombre de vies de chaque combattant au début d'un match."},
     {"Orbe Clash", "L'orbe flottante qui donne l'attaque finale SUPER CLASH."},
     {"Niveau des ordinateurs", "Niveau par défaut des combattants contrôlés par l'ordinateur (1 à 9)."},
     {"Tout réinitialiser", "Remet tous les paramètres et toutes les touches par défaut."}},
};
const int kRowCounts[4] = {4, 6, 4, kActionCount + 1};  // controls: the actions + the reset buttons

Rectangle tabRect(int i) { return {72.0f + i * 252.0f, 136, 236, 50}; }
Rectangle rowRect(int row) { return {kPanel.x + 20, kPanel.y + 20 + row * 60.0f, kPanel.width - 40, 52}; }
Rectangle widgetRect(int row) {
  Rectangle r = rowRect(row);
  return {r.x + 640, r.y + 9, 440, 34};
}
Rectangle sliderRect(int row) {
  Rectangle w = widgetRect(row);
  return {w.x + 12, w.y + 10, w.width - 110, 14};
}
Rectangle keyRect(int row, int col) { return {kPanel.x + 300 + col * 236.0f, kPanel.y + 48 + row * 30.0f, 216, 26}; }
Rectangle resetRect(int col) { return {kPanel.x + 300 + col * 236.0f, kPanel.y + 56 + kActionCount * 30.0f, 216, 30}; }

enum class Widget { Slider, Toggle, Selector, Button };
Widget widgetFor(int tab, int row) {
  if (tab == 0) return row < 3 ? Widget::Slider : Widget::Button;
  if (tab == 1) return (row == 1 || row == 2) ? Widget::Selector : Widget::Toggle;
  if (row == 1) return Widget::Toggle;
  return row == 3 ? Widget::Button : Widget::Selector;
}
int* volumeFor(int row) {
  Settings& s = settings();
  return row == 0 ? &s.masterVolume : row == 1 ? &s.musicVolume : &s.sfxVolume;
}

// ---- Gamepad controls ------------------------------------------------------------------
// Gamepad button + action, the same for every pad.
struct PadAction {
  ui::Btn btn, btn2;
  const char* label;
};
const PadAction kPadActions[] = {
    {ui::Btn::Move, ui::Btn::None, "Se déplacer"}, {ui::Btn::A, ui::Btn::None, "Attaque"},
    {ui::Btn::B, ui::Btn::None, "Spéciale"},       {ui::Btn::X, ui::Btn::Y, "Saut"},
    {ui::Btn::LT, ui::Btn::RT, "Bouclier"},        {ui::Btn::LB, ui::Btn::None, "Saisie"},
    {ui::Btn::RB, ui::Btn::None, "Ultime"},        {ui::Btn::RStick, ui::Btn::None, "Attaque smash"},
    {ui::Btn::Start, ui::Btn::None, "Pause"}};

// A connected pad of that family without triggers (SNES-style: RB shields, Back = ultimate)?
bool triggerless(PadFamily f) {
  for (int p = 0; p < kMaxPads; ++p)
    if (padConnected(p) && padFamily(p) == f) return !padHasTriggers(p);
  return false;
}
// Button of an action on such a pad.
PadAction forPad(PadAction a, bool noTriggers) {
  if (!noTriggers) return a;
  if (a.btn == ui::Btn::LT) return {ui::Btn::RB, ui::Btn::None, a.label};
  if (a.btn == ui::Btn::RB) return {ui::Btn::Back, ui::Btn::None, a.label};
  return a;
}
bool isUltimate(const char* label) { return std::strcmp(label, "Ultime") == 0; }

// Draws one or two glyphs (vertically centred on cy), returns the width.
float glyphs(ui::Btn a, ui::Btn b, float x, float cy, float h, PadFamily f) {
  float w = ui::padGlyph(a, x, cy - h * 0.5f, h, f);
  if (b != ui::Btn::None) w += 6 + ui::padGlyph(b, x + w + 6, cy - h * 0.5f, h, f);
  return w;
}
float glyphsWidth(ui::Btn a, ui::Btn b, float h, PadFamily f) {
  return ui::padGlyphWidth(a, h, f) + (b != ui::Btn::None ? 6 + ui::padGlyphWidth(b, h, f) : 0.0f);
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

// A labelled controller: every action next to its button (gamepad prompt mode).
void drawPadControls(PadFamily f) {
  const Vector2 c{640, 418};
  const PadLayout L = layoutFor(f);
  auto at = [&](Vector2 v) { return Vector2{c.x + v.x, c.y + v.y}; };
  const Color body = f == PadFamily::PlayStation ? Color{214, 216, 222, 255}
                     : f == PadFamily::Nintendo  ? Color{58, 62, 68, 255}
                                                 : Color{70, 78, 84, 255};
  // Title: the pad in use (or the family)
  const char* model = f == PadFamily::PlayStation ? "Manette PlayStation"
                      : f == PadFamily::Nintendo  ? "Manette Nintendo"
                                                  : "Manette Xbox";
  for (int p = 0; p < kMaxPads; ++p)
    if (padConnected(p) && padFamily(p) == f) {
      model = padModel(p);
      break;
    }
  std::string title = model;
  for (char& ch : title)
    if (ch >= 'a' && ch <= 'z') ch = char(ch - 'a' + 'A');
  drawText(title.c_str(), kPanel.x + 30, kPanel.y + 18, 28, ui::kGold, FontKind::Title);

  // Body: outline, then plastic
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
  DrawCircleV(at(L.select), 9, ui::kInk);

  // Callouts: glyph + action on both sides, a thin line to the button (drawn under the buttons)
  const float d = 27;  // face buttons around the cluster centre
  struct Callout {
    ui::Btn btn, btn2;
    Vector2 to;
    const char* label;
  };
  const bool psSticks = f == PadFamily::PlayStation;  // d-pad above the left stick
  const bool noTrig = triggerless(f);
  const Callout left[4] = {
      noTrig ? Callout{ui::Btn::Back, ui::Btn::None, L.select, "Ultime"}
             : Callout{ui::Btn::LT, ui::Btn::None, {-124, -150}, "Bouclier"},
      {ui::Btn::LB, ui::Btn::None, {-124, -115}, "Saisie"},
      psSticks ? Callout{ui::Btn::DPad, ui::Btn::None, L.dpad, "Se déplacer"}
               : Callout{ui::Btn::LStick, ui::Btn::None, L.lstick, "Se déplacer"},
      psSticks ? Callout{ui::Btn::LStick, ui::Btn::None, L.lstick, "Se déplacer"}
               : Callout{ui::Btn::DPad, ui::Btn::None, L.dpad, "Se déplacer"}};
  const Callout right[6] = {noTrig ? Callout{ui::Btn::RB, ui::Btn::None, {124, -115}, "Bouclier"}
                                   : Callout{ui::Btn::RT, ui::Btn::None, {124, -150}, "Bouclier"},
                            noTrig ? Callout{ui::Btn::None, ui::Btn::None, {}, nullptr}
                                   : Callout{ui::Btn::RB, ui::Btn::None, {124, -115}, "Ultime"},
                            {ui::Btn::Y, ui::Btn::X, {L.face.x, L.face.y - d}, "Saut"},
                            {ui::Btn::B, ui::Btn::None, {L.face.x + d, L.face.y}, "Spéciale"},
                            {ui::Btn::A, ui::Btn::None, {L.face.x, L.face.y + d}, "Attaque"},
                            {ui::Btn::RStick, ui::Btn::None, L.rstick, "Attaque smash"}};
  const float gh = 28;
  const Color line = withAlpha(ui::kGold, 0.55f);
  auto leader = [&](Vector2 from, Vector2 to) {
    DrawLineEx(from, at(to), 2, line);
    DrawCircleV(from, 3, line);
  };
  auto leftY = [&](int i) { return c.y - 140 + i * 58.0f + (i >= 2 ? 22.0f : 0.0f); };
  auto rightY = [&](int i) { return c.y - 140 + i * 44.0f; };
  const float leftX = c.x - 236, rightX = c.x + 236;
  for (int i = 0; i < 4; ++i) leader({leftX + 8, leftY(i)}, left[i].to);
  for (int i = 0; i < 6; ++i)
    if (right[i].label) leader({rightX - 8, rightY(i)}, right[i].to);
  const float sw = ui::padGlyphWidth(ui::Btn::Start, gh, f) + 10 + textWidth("Pause", 21);
  const Vector2 sp{at(L.start).x - sw * 0.5f, c.y - 186};  // Pause, above the middle
  leader({at(L.start).x, sp.y + 18}, L.start);

  // Buttons
  const float sh = 26;
  for (int side = -1; side <= 1; side += 2) {
    const ui::Btn trig = side < 0 ? ui::Btn::LT : ui::Btn::RT, bump = side < 0 ? ui::Btn::LB : ui::Btn::RB;
    if (!noTrig) ui::padGlyph(trig, c.x + side * 124 - ui::padGlyphWidth(trig, sh, f) * 0.5f, c.y - 164, sh, f);
    ui::padGlyph(bump, c.x + side * 124 - ui::padGlyphWidth(bump, sh, f) * 0.5f, c.y - 128, sh, f);
  }
  ui::padGlyph(ui::Btn::Start, at(L.start).x - ui::padGlyphWidth(ui::Btn::Start, 24, f) * 0.5f, at(L.start).y - 12, 24, f);
  ui::padGlyph(ui::Btn::LStick, at(L.lstick).x - 20, at(L.lstick).y - 20, 40, f);
  ui::padGlyph(ui::Btn::RStick, at(L.rstick).x - 20, at(L.rstick).y - 20, 40, f);
  ui::padGlyph(ui::Btn::DPad, at(L.dpad).x - 23, at(L.dpad).y - 23, 46, f);
  const Vector2 fc = at(L.face);
  const float fh = 28;
  ui::padGlyph(ui::Btn::Y, fc.x - fh * 0.5f, fc.y - d - fh * 0.5f, fh, f);
  ui::padGlyph(ui::Btn::X, fc.x - d - fh * 0.5f, fc.y - fh * 0.5f, fh, f);
  ui::padGlyph(ui::Btn::B, fc.x + d - fh * 0.5f, fc.y - fh * 0.5f, fh, f);
  ui::padGlyph(ui::Btn::A, fc.x - fh * 0.5f, fc.y + d - fh * 0.5f, fh, f);

  // Labels
  for (int i = 0; i < 4; ++i) {
    const Callout& co = left[i];
    const float tw = textWidth(co.label, 21);
    drawText(co.label, leftX - tw, leftY(i) - 13, 21, isUltimate(co.label) ? ui::kGold : ui::kCream);
    glyphs(co.btn, co.btn2, leftX - tw - 10 - glyphsWidth(co.btn, co.btn2, gh, f), leftY(i), gh, f);
  }
  for (int i = 0; i < 6; ++i) {
    const Callout& co = right[i];
    if (!co.label) continue;
    const float w = glyphs(co.btn, co.btn2, rightX, rightY(i), gh, f);
    drawText(co.label, rightX + w + 10, rightY(i) - 13, 21, isUltimate(co.label) ? ui::kGold : ui::kCream);
  }
  ui::padGlyph(ui::Btn::Start, sp.x, sp.y - gh * 0.5f, gh, f);
  drawText("Pause", sp.x + ui::padGlyphWidth(ui::Btn::Start, gh, f) + 10, sp.y - 13, 21, ui::kCream);

  // Menus
  float x = kPanel.x + 30, y = kPanel.y + kPanel.height - 34;
  drawText("Menus :", x, y - 11, 18, ui::kMuted);
  x += textWidth("Menus :", 18) + 14;
  const PadAction menu[4] = {{ui::Btn::A, ui::Btn::None, "Valider"}, {ui::Btn::B, ui::Btn::None, "Retour"},
                             {ui::Btn::X, ui::Btn::None, "Valeur précédente"}, {ui::Btn::Start, ui::Btn::None, "Commencer"}};
  for (const PadAction& m : menu) {
    x += glyphs(m.btn, m.btn2, x, y, 24, f) + 8;
    drawText(m.label, x, y - 11, 18, ui::kCream);
    x += textWidth(m.label, 18) + 24;
  }
}
}  // namespace

int App::settingsRowCount() const {
  if (setTab_ == 3 && padPrompts()) return 0;  // the controller layout: nothing to select
  return kRowCounts[setTab_];
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
      break;
    case 2:
      if (row == 0) s.stocks = 1 + (s.stocks - 1 + delta + 5) % 5;
      if (row == 1) s.items = !s.items;
      if (row == 2) s.cpuLevel = 1 + (s.cpuLevel - 1 + delta + 9) % 9;
      if (row == 3) {
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
  const int rows = settingsRowCount();
  setRow_ = std::min(setRow_, rows - 1);  // the gamepad layout of the Commandes tab has no rows

  if (waitingKey_) {  // rebinding a key
    if (padAnyPressed(kPadB) || setRow_ < 0 || setRow_ >= kActionCount) {
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
  if (nav.left || nav.right) {
    int d = nav.left ? -1 : 1;
    if (setRow_ < 0) {
      setTab_ = (setTab_ + d + 4) % 4;
    } else if (setTab_ == 3) {
      setCol_ = 1 - setCol_;
    } else {
      changeSetting(setRow_, d);
    }
  }
  if (nav.confirm && !waitingKey_) {
    if (setRow_ < 0) {
      setRow_ = 0;
    } else if (setTab_ == 3) {
      if (setRow_ < kActionCount) {
        waitingKey_ = true;
        while (GetKeyPressed() != 0) {}
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
  } else if (!padPrompts()) {  // keyboard table (the gamepad view has nothing to click)
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

  // Tabs
  for (int i = 0; i < 4; ++i) {
    Rectangle r = tabRect(i);
    bool sel = i == setTab_;
    if (sel) r = {r.x, r.y - 6, r.width, r.height + 6};
    ui::slanted({r.x + 6, r.y + 6, r.width, r.height}, 18, withAlpha(BLACK, 0.35f), withAlpha(BLACK, 0.35f));
    ui::slanted(r, 18, sel ? mixColor(kTabColors[i], ui::kInk, 0.2f) : withAlpha({34, 50, 54, 255}, 0.94f),
                sel ? mixColor(kTabColors[i], ui::kInk, 0.6f) : withAlpha({20, 30, 34, 255}, 0.94f));
    if (sel) ui::slanted({r.x + 18, r.y, r.width - 18, r.height * 0.3f}, 18 * 0.3f, withAlpha(ui::kCream, 0.12f),
                         withAlpha(ui::kCream, 0.0f));
    ui::slantedOutline(r, 18, sel ? 2.0f : 1.5f, sel ? ui::kGold : withAlpha(ui::kStone, 0.5f));
    if (sel) ui::corners(r, 18, 12, 2.5f, ui::kGold);
    if (sel && setRow_ < 0) {
      const float pulse = 0.5f + 0.5f * std::sin(t * 6.0f);
      ui::slantedOutline(ui::grow(r, 5), 18, 4, withAlpha(ui::kTeal, 0.2f + 0.16f * pulse));
      ui::corners(ui::grow(r, 6 + pulse * 2), 18, 14, 3.0f, ui::kGold);
    }
    drawTextCentered(kTabNames[i], r.x + r.width * 0.5f, r.y + r.height * 0.5f - 13, 26,
                     sel ? ui::kCream : ui::kMuted, true, FontKind::Title);
  }
  ui::panel(kPanel, withAlpha({18, 28, 32, 255}, 0.93f), withAlpha(ui::kGold, 0.85f), 0.04f);
  const Rectangle tr = tabRect(setTab_);  // the selected tab's colour runs along the panel top
  DrawRectangleGradientH(int(kPanel.x + 20), int(kPanel.y - 1), int(tr.x - kPanel.x), 3, withAlpha(accent, 0.0f), accent);
  DrawRectangle(int(tr.x), int(kPanel.y - 1), int(tr.width - 18), 3, accent);
  DrawRectangleGradientH(int(tr.x + tr.width - 18), int(kPanel.y - 1), int(kPanel.x + kPanel.width - 20 - tr.x - tr.width + 18), 3,
                         accent, withAlpha(accent, 0.0f));

  const char* help = nullptr;
  if (setTab_ < 3) {
    for (int r = 0; r < kRowCounts[setTab_]; ++r) {
      Rectangle rr = rowRect(r);
      bool foc = r == setRow_;
      if (foc) {
        DrawRectangleRounded(rr, 0.35f, 8, withAlpha(mixColor(accent, ui::kTeal, 0.25f), 0.2f));
        DrawRectangleRoundedLinesEx(rr, 0.35f, 8, 1.0f, withAlpha(ui::kGold, 0.35f));
        ui::diamond({rr.x + 13, rr.y + rr.height * 0.5f}, 5, ui::kGold);
        help = kRows[setTab_][r].help;
      }
      drawText(kRows[setTab_][r].label, rr.x + 26, rr.y + 12, 24, foc ? ui::kCream : Color{196, 206, 198, 255});
      Rectangle w = widgetRect(r);
      switch (widgetFor(setTab_, r)) {
        case Widget::Slider: {
          int v = *volumeFor(r);
          ui::slider(sliderRect(r), v / 100.0f, foc, accent, t);
          drawText(TextFormat("%d %%", v), w.x + w.width - 82, w.y + 4, 22, ui::kCream);
          break;
        }
        case Widget::Toggle: {
          bool on = setTab_ == 1 ? (r == 0 ? s.fullscreen : r == 3 ? s.playerTags : r == 4 ? s.showFps : s.showHitboxes)
                                 : s.items;
          ui::toggle({w.x + w.width - 120, w.y, 120, w.height}, on, foc, t);
          break;
        }
        case Widget::Selector: {
          const char* shakes[3] = {"Aucune", "Légères", "Normales"};
          const char* v = "";
          if (setTab_ == 1 && r == 1) v = s.highQuality ? "Haute (lissage)" : "Normale";
          if (setTab_ == 1 && r == 2) v = shakes[std::clamp(s.screenShake, 0, 2)];
          if (setTab_ == 2 && r == 0) v = TextFormat("%d vie%s", s.stocks, s.stocks > 1 ? "s" : "");
          if (setTab_ == 2 && r == 2) v = TextFormat("Niveau %d", s.cpuLevel);
          ui::selector({w.x + 120, w.y, w.width - 120, w.height}, v, foc, t);
          break;
        }
        case Widget::Button: {
          Rectangle b{w.x + w.width - 220, w.y, 220, w.height};
          DrawRectangleRounded({b.x, b.y + 3, b.width, b.height}, 0.5f, 10, shade(accent, 0.4f));
          DrawRectangleRounded(b, 0.5f, 10, foc ? mixColor(accent, ui::kCream, 0.12f) : shade(accent, 0.8f));
          DrawRectangleRoundedLinesEx(ui::grow(b, -2), 0.5f, 10, 1.0f, withAlpha(ui::kGold, 0.6f));
          drawTextCentered(setTab_ == 0 ? "ÉCOUTER" : "RÉINITIALISER", b.x + b.width * 0.5f, b.y + 7, 19, ui::kCream);
          if (foc) ui::focusFrame(b, t, 0.5f);
          break;
        }
      }
    }
  } else if (padPrompts()) {
    drawPadControls(promptFamily());
  } else {
    // Keyboard bindings table
    drawText("ACTION", kPanel.x + 46, kPanel.y + 16, 18, ui::kMuted);
    for (int c = 0; c < 2; ++c) {
      Rectangle k = keyRect(0, c);
      drawTextCentered(c == 0 ? "CLAVIER 1" : "CLAVIER 2", k.x + k.width * 0.5f, kPanel.y + 13, 20,
                       portColor(c), true, FontKind::Title);
    }
    for (int r = 0; r < kActionCount; ++r) {
      bool rowFoc = r == setRow_;
      Rectangle k0 = keyRect(r, 0);
      if (rowFoc) {
        DrawRectangleRounded({kPanel.x + 24, k0.y - 2, 750, k0.height + 4}, 0.4f, 8,
                             withAlpha(mixColor(accent, ui::kTeal, 0.25f), 0.18f));
        ui::diamond({kPanel.x + 34, k0.y + k0.height * 0.5f}, 4, ui::kGold);
      }
      drawText(actionName(r), kPanel.x + 46, k0.y + 2, 18, rowFoc ? ui::kCream : Color{196, 206, 198, 255});
      for (int c = 0; c < 2; ++c) {
        Rectangle k = keyRect(r, c);
        bool foc = rowFoc && c == setCol_;
        bool wait = foc && waitingKey_;
        DrawRectangleRounded({k.x, k.y + 3, k.width, k.height}, 0.4f, 8, {8, 12, 14, 255});
        DrawRectangleRounded(k, 0.4f, 8, wait ? mixColor(ui::kGold, WHITE, 0.3f) : foc ? Color{52, 80, 82, 255}
                                                                                       : Color{34, 50, 54, 255});
        if (!wait) DrawRectangleRoundedLinesEx(ui::grow(k, -1), 0.4f, 8, 1.0f, withAlpha(ui::kStone, foc ? 0.6f : 0.35f));
        std::string name = wait ? (int(t * 3) % 2 ? "..." : "") : keyName(s.keys[c][r]);
        drawTextCentered(name.c_str(), k.x + k.width * 0.5f, k.y + 4, 16, wait ? ui::kInk : ui::kCream, false);
        if (foc && !wait) ui::focusFrame(k, t, 0.4f);
      }
    }
    for (int c = 0; c < 2; ++c) {
      Rectangle b = resetRect(c);
      bool foc = setRow_ == kActionCount && setCol_ == c;
      DrawRectangleRounded(b, 0.5f, 10, foc ? mixColor(accent, ui::kCream, 0.12f) : shade(accent, 0.7f));
      DrawRectangleRoundedLinesEx(ui::grow(b, -2), 0.5f, 10, 1.0f, withAlpha(ui::kGold, 0.55f));
      drawTextCentered("RÉINITIALISER", b.x + b.width * 0.5f, b.y + 6, 17, ui::kCream);
      if (foc) ui::focusFrame(b, t, 0.5f);
    }
    // Gamepad reference
    const PadFamily f = promptFamily();
    Rectangle box{kPanel.x + 806, kPanel.y + 16, 338, kPanel.height - 32};
    DrawRectangleRounded(box, 0.06f, 8, withAlpha(ui::kInk, 0.5f));
    DrawRectangleRoundedLinesEx(box, 0.06f, 8, 1.0f, withAlpha(ui::kGold, 0.3f));
    drawText("MANETTE", box.x + 18, box.y + 10, 26, ui::kGold, FontKind::Title);
    const bool noTrig = triggerless(f);
    for (int i = 0; i < int(sizeof(kPadActions) / sizeof(kPadActions[0])); ++i) {
      const PadAction a = forPad(kPadActions[i], noTrig);
      const float y = box.y + 62 + i * 33.0f;
      glyphs(a.btn, a.btn2, box.x + 18, y, 24, f);
      drawText(a.label, box.x + 128, y - 11, 17, isUltimate(a.label) ? ui::kGold : ui::kCream);
    }
    drawText("Clavier 2 : le pavé numérique marche", box.x + 18, box.y + box.height - 46, 15, ui::kMuted);
    drawText("aussi (0 à 6).", box.x + 18, box.y + box.height - 27, 15, ui::kMuted);
  }
  if (help) {
    float y = kPanel.y + kPanel.height - 40;
    DrawCircleV({kPanel.x + 40, y + 12}, 13, ui::kGold);
    DrawCircleV({kPanel.x + 40, y + 12}, 11, mixColor(accent, ui::kInk, 0.3f));
    drawTextCentered("i", kPanel.x + 40, y + 1, 18, ui::kCream, false);
    drawText(help, kPanel.x + 62, y + 2, 18, {214, 220, 208, 255});
  }

  if (waitingKey_) {
    DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(ui::kInk, 0.65f));
    Rectangle m{kScreenW * 0.5f - 330, kScreenH * 0.5f - 90, 660, 170};
    ui::panel(m, {24, 38, 42, 255}, ui::kGold, 0.12f);
    drawTextCentered(TextFormat("Nouvelle touche : %s", actionName(setRow_)), m.x + m.width * 0.5f, m.y + 28, 34,
                     ui::kCream, true, FontKind::Title);
    drawTextCentered(setCol_ == 0 ? "Clavier 1" : "Clavier 2", m.x + m.width * 0.5f, m.y + 76, 20, portColor(setCol_));
    drawTextCentered("Appuyez sur une touche  —  ÉCHAP pour annuler", m.x + m.width * 0.5f, m.y + 112, 19,
                     ui::kMuted, false);
  }
  using ui::Btn;
  if (setTab_ == 3 && padPrompts())
    ui::footer({{"GAUCHE/DROITE", Btn::DPadLR, "Onglet"}, {"ÉCHAP", Btn::B, "Retour"}},
               "Touches du clavier : appuyez sur une touche.");
  else if (setRow_ < 0)
    ui::footer({{"GAUCHE/DROITE", Btn::DPadLR, "Onglet"}, {"BAS", Btn::DPadDown, "Réglages"}, {"ÉCHAP", Btn::B, "Retour"}});
  else if (setTab_ == 3)
    ui::footer({{"FLÈCHES", Btn::DPad, "Naviguer"}, {"ENTRÉE", Btn::A, "Changer"}, {"ÉCHAP", Btn::B, "Retour"}},
               setRow_ < kActionCount ? "Une touche déjà utilisée est échangée." : "Touches d'origine.");
  else
    ui::footer({{"HAUT/BAS", Btn::DPadUD, "Choisir"}, {"GAUCHE/DROITE", Btn::DPadLR, "Modifier"}, {"ÉCHAP", Btn::B, "Retour"}});
}
