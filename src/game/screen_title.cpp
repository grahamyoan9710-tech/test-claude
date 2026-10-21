// Main menu, the first screen (docs/refonte/menu_accueil.png): one of the owner's three backgrounds (the next one
// at each visit), the logo, a column of slanted buttons, the featured arena, a tip, the version.
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
    {"QUITTER", "menu_icon_quit", "Quitter Super Clash Bros.", true},
};
constexpr int kItemCount = int(sizeof kItems / sizeof kItems[0]);
using ui::kRed;

Rectangle itemRect(int i, bool sel) {
  const Rectangle r{58, 170.0f + i * 47.0f, 300, 42};
  return sel ? Rectangle{r.x - 14, r.y - 6, r.width + 30, r.height + 12} : r;
}
const Rectangle kCard{26, 590, 452, 104};
const Rectangle kGear{kScreenW - 58, kScreenH - 54, 40, 40};

// An icon cut from the mock (white on transparent), h pixels high, centred on c.
void icon(const char* name, Vector2 c, float h, Color tint) {
  float w, hh;
  if (!art::pictureSize(name, w, hh)) return;
  const float k = h / hh;
  art::drawPicture(name, {0, 0, w, hh}, {c.x - w * k * 0.5f, c.y - h * 0.5f, w * k, h}, 1.0f, false, tint);
}

void button(const Item& it, Rectangle r, bool sel) {
  const float s = r.height * 0.3f;
  ui::menuPlate(r, s, sel);
  const Color ink = !it.ready ? Color{140, 140, 152, 255} : sel ? WHITE : Color{232, 232, 240, 255};
  const float cy = r.y + r.height * 0.5f, size = sel ? 25.0f : 21.0f;
  icon(it.icon, {r.x + s + 28, cy}, r.height * 0.56f, ink);
  drawTextOutlined(it.label, r.x + s + 58, cy - size * 0.55f, size, ink, withAlpha(BLACK, 0.6f), 2.0f, FontKind::Title);
  if (!it.ready) {  // in place of the arrow
    const Rectangle tag{r.x + r.width - s - 62, cy - 8, 54, 16};
    DrawRectangleRounded(tag, 0.5f, 6, withAlpha({70, 70, 84, 255}, 0.9f));
    drawText("BIENTÔT", tag.x + 5, tag.y + 2, 11, {200, 200, 210, 255});
  } else {
    ui::arrowTri({r.x + r.width - s - 18, cy}, sel ? 16.0f : 12.0f, 1, withAlpha(ink, sel ? 1.0f : 0.6f));
  }
}

// One tip per visit, with the key or button of its action.
struct Tip {
  int action;
  const char* text;
};
const Tip kTips[] = {
    {kActWeapon, "Arme : ramasse ton arme, puis alterne A et B"},
    {kActUltimate, "Ultime : quand la jauge est pleine"},
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
  DrawRectangleGradientH(0, 0, 620, kScreenH, withAlpha(BLACK, 0.55f), withAlpha(BLACK, 0.0f));

  // Logo, top left
  float lw, lh;
  if (art::pictureSize("menu_logo", lw, lh)) {
    const float k = 372.0f / lw, in = easeOut(f / 16.0f);
    art::drawPicture("menu_logo", {0, 0, lw, lh}, {18 - (1.0f - in) * 80.0f, 6, lw * k, lh * k}, in);
  }

  // Buttons, sliding in one after the other; the chosen one is red, bigger
  for (int i = 0; i < kItemCount; ++i) {
    if (i == menuSel_) continue;
    const float pop = easeOut((f - i * 2.0f) / 14.0f);
    Rectangle r = itemRect(i, false);
    r.x -= (1.0f - pop) * 420.0f;
    button(kItems[i], r, false);
  }
  {
    const float pop = easeOut((f - menuSel_ * 2.0f) / 14.0f);
    Rectangle r = itemRect(menuSel_, true);
    r.x -= (1.0f - pop) * 420.0f + std::sin(t * 5.0f) * 1.5f;
    button(kItems[menuSel_], r, true);
  }

  // Featured arena (changes with the background)
  const auto& stages = stageList();
  const int fs = featuredStage();
  ui::slanted({kCard.x + 6, kCard.y + 7, kCard.width, kCard.height}, 18, withAlpha(BLACK, 0.45f), withAlpha(BLACK, 0.45f));
  ui::slanted(kCard, 18, withAlpha({28, 28, 38, 255}, 0.94f), withAlpha({12, 12, 18, 255}, 0.94f));
  DrawRectangleGradientH(int(kCard.x + 10), int(kCard.y + kCard.height - 4), int(kCard.width - 10), 4, kRed,
                         withAlpha(kRed, 0.0f));
  const Rectangle thumb{kCard.x + 26, kCard.y + 10, 150, 84};
  drawStageThumb(fs, stages[fs], thumb);
  DrawRectangleLinesEx(thumb, 1.5f, withAlpha(WHITE, 0.5f));
  const float tx = thumb.x + thumb.width + 18;
  drawText("ARÈNE À LA UNE", tx, kCard.y + 14, 13, {200, 200, 212, 255});
  drawTextOutlined(stages[fs].name.c_str(), tx, kCard.y + 31, 26, WHITE, withAlpha(BLACK, 0.6f), 2.0f, FontKind::Title);
  const float ph = padPrompts() ? 26 : 22;
  const float pw = ui::prompt("K", ui::Btn::X, tx, kCard.y + 70 - ph * 0.5f, ph);
  drawText("Découvrir cette arène", tx + pw + 8, kCard.y + 62, 16, {232, 232, 240, 255});

  // Tip, top right
  const Tip& tip = kTips[settings().menuBg % 3];
  const std::string key = keyName(settings().keys[0][tip.action]);
  const float th = 26, tw = ui::promptWidth(key.c_str(), ui::padButtonFor(tip.action), th) + 12 + textWidth(tip.text, 16);
  const Rectangle pill{kScreenW - tw - 52, 16, tw + 32, 40};
  DrawRectangleRounded({pill.x + 3, pill.y + 4, pill.width, pill.height}, 0.5f, 8, withAlpha(BLACK, 0.4f));
  DrawRectangleRounded(pill, 0.5f, 8, withAlpha({16, 16, 24, 255}, 0.88f));
  DrawRectangleRoundedLinesEx(pill, 0.5f, 8, 1.5f, withAlpha({150, 150, 170, 255}, 0.6f));
  const float kx = pill.x + 16 + ui::prompt(key.c_str(), ui::padButtonFor(tip.action), pill.x + 16, pill.y + 7, th);
  drawText(tip.text, kx + 12, pill.y + 11, 16, WHITE);

  // What the chosen button does, the version and the options wheel, bottom right
  drawTextOutlined(kItems[menuSel_].desc, kCard.x + kCard.width + 30, kScreenH - 34, 17, {232, 232, 240, 255},
                   withAlpha(BLACK, 0.7f), 2.0f);
  drawTextOutlined("v9.9", kGear.x - 50, kGear.y + 11, 17, WHITE, withAlpha(BLACK, 0.7f), 2.0f);
  const bool gearHot = ui::hovered(kGear);
  icon("menu_icon_options", {kGear.x + kGear.width * 0.5f, kGear.y + kGear.height * 0.5f}, gearHot ? 34.0f : 30.0f,
       gearHot ? kRed : WHITE);
}
