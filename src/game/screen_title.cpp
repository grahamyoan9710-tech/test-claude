// Main menu, the first screen: one of the owner's three backgrounds (the next one at each visit), the Clash Horizon
// logo over a column of chunky buttons, a tip and the options button at the top right, the featured arena below.
#include "game/app.hpp"
#include "game/fighter_art.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
float easeOut(float t) {
  t = clampf(t, 0.0f, 1.0f);
  return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
}

struct Item {
  const char* label;
  const char* icon;
  const char* desc;
  bool ready;  // false: shown, not open yet
};
const Item kItems[] = {
    {"JOUER", "menu_icon_play", "Combat contre l'ordinateur.", true},
    {"ENTRAÎNEMENT", "menu_icon_training", "Vies infinies, mannequin réglable en pause.", true},
    {"VERSUS", "menu_icon_versus", "De 2 à 4 joueurs sur la même machine.", true},
    {"EN LIGNE", "menu_icon_online", "Bientôt.", false},
    {"PERSONNAGES", "menu_icon_fighters", "La galerie des combattants : style, armes, compétences.", true},
    {"ARÈNES", "menu_icon_arenas", "L'aperçu des arènes ; ENTRÉE pour s'y entraîner.", true},
    {"OPTIONS", "menu_icon_options", "Son, affichage, règles du jeu, touches et manettes.", true},
    {"QUITTER", "menu_icon_quit", "Quitter Clash Horizon.", true},
};
constexpr int kItemCount = int(sizeof kItems / sizeof kItems[0]);

// JOUER first, big and gold; the others blue (QUITTER red, what is not open yet grey).
Rectangle itemRect(int i, bool sel) {
  const float y = i == 0 ? 252.0f : 316.0f + (i - 1) * 50.0f;
  const Rectangle r{46, y, i == 0 ? 352.0f : 320.0f, i == 0 ? 56.0f : 42.0f};
  return sel ? Rectangle{r.x, r.y - 2, r.width + 12, r.height + 4} : r;
}
ui::Tone toneOf(int i) {
  if (!kItems[i].ready) return ui::Tone::Grey;
  return i == 0 ? ui::Tone::Gold : i == kItemCount - 1 ? ui::Tone::Red : ui::Tone::Blue;
}
const Rectangle kCard{846, 596, 410, 98};
const Rectangle kGear{kScreenW - 62, 14, 46, 46};

// An icon (white on transparent), h pixels high, centred on c.
void icon(const char* name, Vector2 c, float h, Color tint) {
  float w, hh;
  if (!art::pictureSize(name, w, hh)) return;
  const float k = h / hh;
  art::drawPicture(name, {0, 0, w, hh}, {c.x - w * k * 0.5f, c.y - h * 0.5f, w * k, h}, 1.0f, false, tint);
}

void button(int i, Rectangle r, bool sel, float t) {
  const Item& it = kItems[i];
  ui::button(r, "", toneOf(i), sel, t);
  if (sel) r = ui::grow(r, 3.0f);
  const float lip = ui::chunkyLip(r), cy = r.y + (r.height - lip) * 0.5f, sr = (r.height - lip) * 0.36f;
  const Vector2 sock{r.x + sr + 12, cy};  // the icon in a dark socket
  DrawCircleV(sock, sr + 2, ui::kInk);
  DrawCircleV(sock, sr, withAlpha(shade(ui::toneBottom(toneOf(i)), 0.55f), 0.95f));
  icon(it.icon, sock, sr * 1.25f, WHITE);
  const float size = i == 0 ? 32.0f : 23.0f;
  ui::bold(it.label, sock.x + sr + 14, cy - size * 0.5f + size * 0.06f, size, it.ready ? WHITE : Color{214, 220, 232, 255});
  if (!it.ready) {  // in place of the arrow
    const Rectangle tag{r.x + r.width - 92, cy - 11, 80, 22};
    ui::roundRect(ui::grow(tag, 2), 11, ui::kInk, ui::kInk);
    ui::roundRect(tag, 9, ui::toneTop(ui::Tone::Red), ui::toneBottom(ui::Tone::Red));
    drawTextCentered("BIENTÔT", tag.x + tag.width * 0.5f, tag.y + 3, 14, WHITE, true);
  } else if (sel) {
    ui::arrowTri({r.x + r.width - 22, cy}, 18.0f, 1, ui::kInk);
    ui::arrowTri({r.x + r.width - 24, cy - 1}, 14.0f, 1, WHITE);
  }
}

// One tip per visit, with the key or button of its action.
struct Tip {
  int action;
  const char* text;
};
const Tip kTips[] = {
    {kActWeapon, "Arme : maintiens-la 1 s sur ton arme pour la prendre ; avec A et B, elle alterne"},
    {kActUltimate, "Ultime : quand « ULTI PRÊTE ! » s'affiche"},
    {kActGrab, "Prise : attrape, puis lance dans une direction"},
};
}  // namespace

// ---------------------------------------------------------------------------
// Main menu
// ---------------------------------------------------------------------------
// The device that opens the character select plays player 1 (keyboard or gamepad).
void App::claimP1(int device) {
  padLeft_ = {};
  if (device >= 0) assignDevice(0, device);
}

int App::featuredStage() const { return settings().menuBg % int(stageList().size()); }

void App::activateMenu(int item, int device) {
  if (!kItems[item].ready) {
    audio_.play(Sfx::MenuBack);
    return;
  }
  audio_.play(Sfx::MenuSelect);
  switch (item) {
    case 0: case 2:  // JOUER, VERSUS: the same select, where players join
      stageBrowse_ = false;
      training_ = false;
      cssRow_ = 1;
      claimP1(device);
      goTo(Screen::CharSelect);
      break;
    case 1:
      stageBrowse_ = false;
      training_ = true;
      cssRow_ = 1;
      cssCol_ = 0;
      claimP1(device);
      goTo(Screen::CharSelect);
      break;
    case 4:
      goTo(Screen::Gallery);
      break;
    case 5:
      stageBrowse_ = true;
      goTo(Screen::StageSelect);
      break;
    case 6:
      settingsReturn_ = Screen::MainMenu;
      setTab_ = 0;
      setRow_ = -1;
      goTo(Screen::Settings);
      break;
    default:
      quit_ = true;
      break;
  }
}

void App::updateMainMenu(const MenuNav& nav) {
  const int before = menuSel_;
  if (nav.up) menuSel_ = (menuSel_ + kItemCount - 1) % kItemCount;
  if (nav.down) menuSel_ = (menuSel_ + 1) % kItemCount;
  for (int i = 0; i < kItemCount; ++i) {
    if (!ui::hovered(itemRect(i, i == menuSel_))) continue;
    if (ui::mouseMoved()) menuSel_ = i;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      menuSel_ = i;
      activateMenu(i);
      return;
    }
  }
  if (menuSel_ != before) audio_.play(Sfx::MenuMove, 0.7f);
  if (ui::clicked(kGear)) {
    menuSel_ = 6;
    activateMenu(6);
  } else if (nav.prev || ui::clicked(kCard)) {  // discover the featured arena: a training there
    stage_ = featuredStage();
    stageRandom_ = false;
    activateMenu(1, nav.device);
  } else if (nav.confirm || nav.start) {
    activateMenu(menuSel_, nav.device);
  }
}

void App::drawMainMenu() {
  const float t = time_, f = float(screenFrames_);
  const int bg = settings().menuBg % 3;
  art::drawPicture(TextFormat("menu_fond_%d", bg + 1), {0, 0, kScreenW, kScreenH}, {0, 0, kScreenW, kScreenH});
  // A royal blue grade on the left, under the logo and the buttons
  DrawRectangleGradientH(0, 0, 640, kScreenH, withAlpha(ui::kNavyDeep, 0.88f), withAlpha(ui::kNavyDeep, 0.0f));
  DrawRectangleGradientV(0, kScreenH - 160, kScreenW, 160, withAlpha(ui::kInk, 0.0f), withAlpha(ui::kInk, 0.7f));

  // Logo, top left, popping in
  float lw, lh;
  if (art::pictureSize("menu_logo", lw, lh)) {
    const float in = easeOut(f / 18.0f), pop = 1.0f + 0.08f * std::sin(std::min(1.0f, f / 18.0f) * kPi);
    const float w = 410.0f * pop, h = w * lh / lw;
    art::drawPicture("menu_logo", {0, 0, lw, lh}, {226 - w * 0.5f, 126 - h * 0.5f, w, h}, in);
  }

  // Buttons, sliding in one after the other; the chosen one lifts, with a gold rim
  for (int i = 0; i < kItemCount; ++i) {
    if (i == menuSel_) continue;
    const float pop = easeOut((f - i * 2.0f) / 14.0f);
    Rectangle r = itemRect(i, false);
    r.x -= (1.0f - pop) * 440.0f;
    button(i, r, false, t);
  }
  {
    const float pop = easeOut((f - menuSel_ * 2.0f) / 14.0f);
    Rectangle r = itemRect(menuSel_, true);
    r.x -= (1.0f - pop) * 440.0f;
    button(menuSel_, r, true, t);
  }
  // What the chosen button does, under the column
  drawTextOutlined(kItems[menuSel_].desc, 50, kScreenH - 46, 18, ui::kCream, ui::kInk, 2.5f);

  // Featured arena (changes with the background), bottom right
  const auto& stages = stageList();
  const int fs = featuredStage();
  ui::card(kCard, 0.96f);
  const Rectangle thumb{kCard.x + 12, kCard.y + 12, 132, kCard.height - 24};
  ui::roundRect(ui::grow(thumb, 3), 10, ui::kInk, ui::kInk);
  drawStageThumb(fs, stages[fs], thumb);
  const float tx = thumb.x + thumb.width + 16;
  drawTextOutlined("ARÈNE À LA UNE", tx, kCard.y + 12, 14, ui::kGold, ui::kInk, 2.0f);
  ui::bold(stages[fs].name.c_str(), tx, kCard.y + 30, 27);
  const float ph = padPrompts() ? 26 : 24;
  const float pw = ui::prompt("K", ui::Btn::X, tx, kCard.y + 72 - ph * 0.5f, ph);
  drawTextOutlined("Découvrir cette arène", tx + pw + 8, kCard.y + 62, 16, ui::kCream, ui::kInk, 2.0f);

  // Options button and version, top right; the tip left of them
  const bool gearHot = ui::hovered(kGear);
  ui::chunky(gearHot ? ui::grow(kGear, 2) : kGear, gearHot ? ui::Tone::Gold : ui::Tone::Blue, 1.0f, 14.0f);
  icon("menu_icon_options", {kGear.x + kGear.width * 0.5f, kGear.y + kGear.height * 0.45f}, 28.0f, WHITE);
  drawTextOutlined("v10.5", kGear.x + 4, kGear.y + kGear.height + 8, 16, ui::kCream, ui::kInk, 2.0f);
  const Tip& tip = kTips[settings().menuBg % 3];
  const std::string key = keyName(settings().keys[0][tip.action]);
  const float th = 26, tw = ui::promptWidth(key.c_str(), ui::padButtonFor(tip.action), th) + 12 + textWidth(tip.text, 16);
  const Rectangle pill{kGear.x - tw - 52, kGear.y + 2, tw + 32, 42};
  ui::card(pill, 0.94f);
  const float kx = pill.x + 16 + ui::prompt(key.c_str(), ui::padButtonFor(tip.action), pill.x + 16, pill.y + 8, th);
  drawTextOutlined(tip.text, kx + 12, pill.y + 12, 16, ui::kCream, ui::kInk, 2.0f);
}
