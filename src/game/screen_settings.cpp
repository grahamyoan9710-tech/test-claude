// Settings screen: audio, display, rules and keyboard bindings.
#include "game/app.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
const char* kTabNames[4] = {"AUDIO", "AFFICHAGE", "JEU", "COMMANDES"};
const Color kTabColors[4] = {{236, 92, 72, 255}, {72, 132, 246, 255}, {64, 190, 108, 255}, {240, 170, 40, 255}};
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
const int kRowCounts[4] = {4, 6, 4, 11};

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
Rectangle keyRect(int row, int col) { return {kPanel.x + 300 + col * 236.0f, kPanel.y + 52 + row * 34.0f, 216, 30}; }
Rectangle resetRect(int col) { return {kPanel.x + 300 + col * 236.0f, kPanel.y + 52 + 10 * 34.0f + 8, 216, 32}; }

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
}  // namespace

int App::settingsRowCount() const { return kRowCounts[setTab_]; }

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

  if (waitingKey_) {  // rebinding a key
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
  } else {
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
  ui::backdrop(t, {16, 18, 52, 255}, {46, 20, 70, 255}, accent);
  ui::header("PARAMÈTRES", "Les réglages sont enregistrés automatiquement.", {72, 110, 230, 255}, t);

  // Tabs
  for (int i = 0; i < 4; ++i) {
    Rectangle r = tabRect(i);
    bool sel = i == setTab_;
    if (sel) r = {r.x, r.y - 6, r.width, r.height + 6};
    ui::slanted({r.x + 6, r.y + 6, r.width, r.height}, 18, withAlpha(BLACK, 0.35f), withAlpha(BLACK, 0.35f));
    ui::slanted(r, 18, sel ? mixColor(kTabColors[i], WHITE, 0.15f) : Color{44, 40, 84, 255},
                sel ? shade(kTabColors[i], 0.7f) : Color{30, 26, 60, 255});
    if (sel && setRow_ < 0) ui::slantedOutline(ui::grow(r, 3), 18, 4, withAlpha(ui::kGold, 0.7f + 0.3f * std::sin(t * 7)));
    drawTextCentered(kTabNames[i], r.x + r.width * 0.5f, r.y + r.height * 0.5f - 13, 26,
                     sel ? ui::kCream : ui::kMuted, true, FontKind::Title);
  }
  ui::panel(kPanel, withAlpha({20, 16, 46, 255}, 0.93f), withAlpha(accent, 0.9f), 0.04f);

  const char* help = nullptr;
  if (setTab_ < 3) {
    for (int r = 0; r < kRowCounts[setTab_]; ++r) {
      Rectangle rr = rowRect(r);
      bool foc = r == setRow_;
      if (foc) {
        DrawRectangleRounded(rr, 0.35f, 8, withAlpha(accent, 0.22f));
        help = kRows[setTab_][r].help;
      }
      drawText(kRows[setTab_][r].label, rr.x + 26, rr.y + 12, 24, foc ? ui::kCream : Color{200, 200, 225, 255});
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
          DrawRectangleRounded({b.x, b.y + 3, b.width, b.height}, 0.5f, 10, shade(accent, 0.5f));
          DrawRectangleRounded(b, 0.5f, 10, foc ? mixColor(accent, WHITE, 0.2f) : accent);
          drawTextCentered(setTab_ == 0 ? "ÉCOUTER" : "RÉINITIALISER", b.x + b.width * 0.5f, b.y + 7, 19, ui::kCream);
          if (foc) ui::focusFrame(b, t, 0.5f);
          break;
        }
      }
    }
  } else {
    // Keyboard bindings table
    drawText("ACTION", kPanel.x + 46, kPanel.y + 18, 18, ui::kMuted);
    for (int c = 0; c < 2; ++c) {
      Rectangle k = keyRect(0, c);
      drawTextCentered(c == 0 ? "CLAVIER 1" : "CLAVIER 2", k.x + k.width * 0.5f, kPanel.y + 16, 20,
                       portColor(c), true, FontKind::Title);
    }
    for (int r = 0; r < kActionCount; ++r) {
      bool rowFoc = r == setRow_;
      Rectangle k0 = keyRect(r, 0);
      if (rowFoc) DrawRectangleRounded({kPanel.x + 24, k0.y - 2, 750, k0.height + 4}, 0.4f, 8, withAlpha(accent, 0.18f));
      drawText(actionName(r), kPanel.x + 46, k0.y + 4, 19, rowFoc ? ui::kCream : Color{200, 200, 225, 255});
      for (int c = 0; c < 2; ++c) {
        Rectangle k = keyRect(r, c);
        bool foc = rowFoc && c == setCol_;
        bool wait = foc && waitingKey_;
        DrawRectangleRounded({k.x, k.y + 3, k.width, k.height}, 0.4f, 8, {14, 12, 30, 255});
        DrawRectangleRounded(k, 0.4f, 8, wait ? mixColor(ui::kGold, WHITE, 0.3f) : foc ? Color{70, 64, 120, 255}
                                                                                       : Color{48, 44, 88, 255});
        std::string name = wait ? (int(t * 3) % 2 ? "..." : "") : keyName(s.keys[c][r]);
        drawTextCentered(name.c_str(), k.x + k.width * 0.5f, k.y + 5, 17, wait ? ui::kInk : ui::kCream, false);
        if (foc && !wait) ui::focusFrame(k, t, 0.4f);
      }
    }
    for (int c = 0; c < 2; ++c) {
      Rectangle b = resetRect(c);
      bool foc = setRow_ == kActionCount && setCol_ == c;
      DrawRectangleRounded(b, 0.5f, 10, foc ? mixColor(accent, WHITE, 0.2f) : shade(accent, 0.75f));
      drawTextCentered("RÉINITIALISER", b.x + b.width * 0.5f, b.y + 7, 17, ui::kCream);
      if (foc) ui::focusFrame(b, t, 0.5f);
    }
    // Gamepad reference
    Rectangle box{kPanel.x + 810, kPanel.y + 16, 330, kPanel.height - 70};
    DrawRectangleRounded(box, 0.06f, 8, withAlpha(BLACK, 0.3f));
    drawText("MANETTE", box.x + 18, box.y + 12, 26, ui::kGold, FontKind::Title);
    const char* pad[][2] = {{"Stick gauche", "Se déplacer"}, {"A", "Attaque"},          {"B", "Spéciale"},
                            {"X / Y", "Saut"},               {"RB / gâchettes", "Bouclier"},
                            {"LB", "Saisie"},                {"Stick droit", "Attaque smash"},
                            {"Start", "Pause"}};
    for (int i = 0; i < 8; ++i) {
      float y = box.y + 52 + i * 33.0f;
      drawText(pad[i][0], box.x + 18, y, 17, ui::kCream);
      drawText(pad[i][1], box.x + 170, y, 17, ui::kMuted);
    }
    drawText("Clavier 2 : le pavé numérique", box.x + 18, box.y + box.height - 50, 15, ui::kMuted);
    drawText("marche aussi (0 à 5).", box.x + 18, box.y + box.height - 30, 15, ui::kMuted);
  }
  if (help) {
    float y = kPanel.y + kPanel.height - 40;
    DrawCircleV({kPanel.x + 40, y + 12}, 12, accent);
    drawTextCentered("i", kPanel.x + 40, y + 1, 18, ui::kCream, false);
    drawText(help, kPanel.x + 62, y + 2, 18, {215, 215, 235, 255});
  }

  if (waitingKey_) {
    DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(BLACK, 0.55f));
    Rectangle m{kScreenW * 0.5f - 330, kScreenH * 0.5f - 90, 660, 170};
    ui::panel(m, {28, 24, 60, 255}, ui::kGold, 0.12f);
    drawTextCentered(TextFormat("Nouvelle touche : %s", actionName(setRow_)), m.x + m.width * 0.5f, m.y + 28, 34,
                     ui::kCream, true, FontKind::Title);
    drawTextCentered(setCol_ == 0 ? "Clavier 1" : "Clavier 2", m.x + m.width * 0.5f, m.y + 76, 20, portColor(setCol_));
    drawTextCentered("Appuyez sur une touche  —  ÉCHAP pour annuler", m.x + m.width * 0.5f, m.y + 112, 19,
                     ui::kMuted, false);
  }
  if (setRow_ < 0)
    ui::footer({{"GAUCHE/DROITE", "Onglet"}, {"BAS", "Réglages"}, {"ÉCHAP", "Retour"}});
  else if (setTab_ == 3)
    ui::footer({{"FLÈCHES", "Naviguer"}, {"ENTRÉE", "Changer"}, {"ÉCHAP", "Retour"}},
               setRow_ < kActionCount ? "Une touche déjà utilisée est échangée." : "Touches d'origine.");
  else
    ui::footer({{"HAUT/BAS", "Choisir"}, {"GAUCHE/DROITE", "Modifier"}, {"ÉCHAP", "Retour"}});
}
