// Main menu, the first screen (« Ligne d'horizon »): a real fight between two CPUs on the title's terrace behind it,
// the logo over four big buttons and four small ones on the left, what the chosen one does under them, a tip at the
// top right, the demo fight's cards at the bottom right.
#include "game/app.hpp"
#include "game/fighter_art.hpp"
#include "game/fx.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
constexpr const char* kVersion = "v14.0";

float easeOut(float t) {
  t = clampf(t, 0.0f, 1.0f);
  return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
}

struct Item {
  const char* label;
  const char* tag;  // the big buttons' note, on their right
  const char* icon;
  const char* desc;
  const char* tip;
  bool ready;  // false: shown, not open yet
};
// Four big buttons, then four small ones in two rows of two.
const Item kItems[] = {
    {"JOUER", "1 JOUEUR", "menu_icon_play", "Un combat contre l'ordinateur : choisis ton combattant et ton arène.",
     "Casse la Clash Orb : c'est la seule façon d'obtenir ton ultime.", true},
    {"VERSUS", "2 À 4", "menu_icon_versus", "De 2 à 4 joueurs sur la même machine, en équipes ou chacun pour soi.",
     "Clavier 1, clavier 2 et manettes : jusqu'à 4 joueurs.", true},
    {"ENTRAÎNEMENT", "LIBRE", "menu_icon_training", "Vies infinies, mannequin réglable en pause.",
     "Zones de coup visibles avec la touche F1.", true},
    {"EN LIGNE", "BIENTÔT", "menu_icon_online", "Le jeu en ligne arrive dans une prochaine version.",
     "Le jeu en ligne arrive plus tard.", false},
    {"PERSONNAGES", "", "menu_icon_fighters", "La galerie des combattants : leur style, leurs armes, leurs compétences.",
     "Les deux armes de chacun tombent sur l'arène pendant le combat.", true},
    {"ARÈNES", "", "menu_icon_arenas", "L'aperçu des arènes et de leurs plateformes ; ENTRÉE pour s'y entraîner.",
     "Chaque arène a ses propres plateformes.", true},
    {"OPTIONS", "", "menu_icon_options", "Son, affichage, règles du jeu, touches et manettes.",
     "Plein écran : touche F11.", true},
    {"QUITTER", "", "menu_icon_quit", "Quitter Clash Horizon.", "À bientôt sur l'horizon.", true},
};
constexpr int kItemCount = int(sizeof kItems / sizeof kItems[0]);
constexpr int kBig = 4;  // the big buttons come first
constexpr int kQuit = kItemCount - 1;

Rectangle itemRect(int i) {
  if (i < kBig) return {48, 262.0f + i * 60.0f, 384, 54};
  const int k = i - kBig;
  return {48.0f + (k % 2) * 195.0f, 510.0f + (k / 2) * 48.0f, 189, 42};
}
// Up and down through the column, the small buttons two to a row.
int itemBelow(int i) { return i < kBig ? i + 1 : i + 2 < kItemCount ? i + 2 : 0; }
int itemAbove(int i) { return i == 0 ? kItemCount - 2 : i <= kBig ? i - 1 : i - 2 < kBig ? kBig - 1 : i - 2; }

// An icon (white on transparent), h pixels high, centred on c.
void icon(const char* name, Vector2 c, float h, Color tint) {
  float w, hh;
  if (!art::pictureSize(name, w, hh)) return;
  const float k = h / hh;
  art::drawPicture(name, {0, 0, w, hh}, {c.x - w * k * 0.5f, c.y - h * 0.5f, w * k, h}, 1.0f, false, tint);
}

// The chosen one is gold (the big ones shimmer); the others night blue, the big ones with an azure line under them.
void drawItem(int i, bool sel, float t, float slide) {
  const Item& it = kItems[i];
  const bool big = i < kBig;
  Rectangle r = itemRect(i);
  r.x -= slide;
  const float cut = big ? 14.0f : 10.0f;
  if (sel) {
    ui::bevel(r, cut, ui::kGold, ui::kGold);
    if (big) ui::shimmer(r, t);
  } else {
    const Color fill = it.ready ? withAlpha(ui::kNavyDeep, big ? 0.9f : 0.82f) : Color{10, 16, 36, 225};
    ui::bevel(r, cut, fill, fill);
    if (big && it.ready) DrawRectangleRec({r.x, r.y + r.height - 2, r.width - cut, 2}, withAlpha(ui::kBlue, 0.55f));
  }
  const Color ink = sel ? ui::kInk : !it.ready ? ui::kDisabled : big ? ui::kCream : ui::kSoft;
  const float cy = r.y + r.height * 0.5f, ih = big ? 26.0f : 20.0f, x0 = r.x + (big ? 18.0f : 14.0f);
  icon(it.icon, {x0 + ih * 0.5f, cy}, ih, sel || !it.ready || big ? ink : ui::kSky);
  const float size = big ? 29.0f : 18.0f;
  drawText(it.label, x0 + ih + (big ? 14.0f : 10.0f), capsY(cy, size, FontKind::Title), size, ink, FontKind::Title);
  if (*it.tag) {
    const float ts = 15.0f;
    drawText(it.tag, r.x + r.width - 18 - textWidth(it.tag, ts), capsY(cy, ts, FontKind::Ui), ts, withAlpha(ink, 0.85f));
  }
}

// The demo fight's cards, bottom right: a blinking "DÉMO", then each fighter: portrait, "CPU 9 · N° 01", stock,
// damage (K.O. once out).
void drawDemoHud(const Match& m, float t) {
  constexpr float h = 40.0f, y = 630.0f, cardW = 226.0f, gap = 8.0f;
  const int n = std::min(2, int(m.fighters.size()));
  if (n == 0) return;
  float x = kScreenW - 28.0f - n * cardW - (n - 1) * gap;
  const float tw = textWidth("DÉMO", 14.0f, FontKind::Title), dw = 34.0f + tw;
  const Rectangle chip{x - gap - dw, y, dw, h};
  DrawRectangleRec(chip, withAlpha(ui::kInk, 0.72f));
  DrawCircleV({chip.x + 14, y + h * 0.5f}, 4.0f, withAlpha({255, 122, 82, 255}, std::fmod(t, 1.2f) < 0.6f ? 1.0f : 0.25f));
  drawText("DÉMO", chip.x + 24, capsY(y + h * 0.5f, 14.0f, FontKind::Title), 14.0f, ui::kSoft, FontKind::Title);
  for (int i = 0; i < n; ++i, x += cardW + gap) {
    const Fighter& f = m.fighters[i];
    if (!f.def) continue;
    const Color pc = i == 0 ? ui::kBlue : ui::kGold;
    DrawRectangleRec({x, y, cardW, h}, withAlpha(ui::kInk, 0.72f));
    DrawRectangleRec({x, y, 3, h}, pc);
    const Rectangle pr{x + 3, y, h, h};  // the portrait's top, on the fighter's colour
    DrawRectangleRec(pr, shade(auraColor(f.def->hero), 0.45f));
    const std::string pic = f.def->id + "_portrait";
    float pw, ph;
    if (art::pictureSize(pic.c_str(), pw, ph)) {
      const float s = std::min(pw, ph);
      art::drawPicture(pic.c_str(), {(pw - s) * 0.5f, 0, s, s}, pr);
    }
    const float tx = pr.x + h + 8;
    drawText(TextFormat("CPU %d · %s", f.cpuLevel, f.def->name.c_str()), tx, capsY(y + 13, 14.0f, FontKind::Title), 14.0f,
             pc, FontKind::Title);
    for (int s = 0; s < std::max(1, f.stocks); ++s)
      ui::diamond({tx + 4 + s * 13.0f, y + 29}, 4.5f, withAlpha(pc, f.stocks > s ? 1.0f : 0.3f));
    const bool ko = f.eliminated || f.stocks <= 0;
    const char* pct = ko ? "K.O." : TextFormat("%d %%", int(f.percent));
    const Color heat = ko ? ui::kGold : f.percent < 60 ? ui::kCream : f.percent < 110 ? Color{255, 200, 112, 255} : Color{255, 122, 82, 255};
    const float ps = 26.0f;
    drawText(pct, x + cardW - 10 - textWidth(pct, ps, FontKind::Title), capsY(y + h * 0.5f, ps, FontKind::Title), ps,
             heat, FontKind::Title);
  }
}
}  // namespace

const char* gameVersion() { return kVersion; }

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
    case 0: case 1:  // JOUER, VERSUS: the same select, where players join
      stageBrowse_ = false;
      training_ = false;
      cssRow_ = 1;
      claimP1(device);
      goTo(Screen::CharSelect);
      break;
    case 2:  // ENTRAÎNEMENT (also from the arenas opened here: a training in the chosen one)
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
  if (nav.up) menuSel_ = itemAbove(menuSel_);
  if (nav.down) menuSel_ = itemBelow(menuSel_);
  if ((nav.left || nav.right) && menuSel_ >= kBig) menuSel_ = kBig + ((menuSel_ - kBig) ^ 1);
  if (nav.back) menuSel_ = kQuit;
  for (int i = 0; i < kItemCount; ++i) {
    if (!ui::hovered(itemRect(i))) continue;
    if (ui::mouseMoved()) menuSel_ = i;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      menuSel_ = i;
      activateMenu(i);
      return;
    }
  }
  if (menuSel_ != before) audio_.play(Sfx::MenuMove, 0.7f);
  if (nav.confirm || nav.start) activateMenu(menuSel_, nav.device);
}

// The demo fight: two CPUs (level 9) drawn among the fighters that have their animations, on the flat arena (the
// terrace has no platforms), without the Clash Orb (no ultimate over the menu); a new pair after each K.O.
void App::updateDemo() {
  if (demo_ && demo_->update() != MatchScreen::Result::Finished) return;
  demo_.reset();
  std::vector<int> pool;
  for (int i = 0; i < int(roster().size()); ++i)
    if (art::hasClips(roster()[i].hero)) pool.push_back(i);
  if (pool.size() < 2) return;
  MatchConfig cfg;
  for (int i = 0; i < int(stageList().size()); ++i)
    if (stageList()[i].theme == StageTheme::Lunar) cfg.stage = i;
  cfg.stocks = 1;
  cfg.items = false;
  cfg.seed = uint32_t(GetRandomValue(1, 1 << 30));
  const int a = GetRandomValue(0, int(pool.size()) - 1);
  int b = GetRandomValue(0, int(pool.size()) - 2);
  if (b >= a) ++b;
  cfg.slots[0] = {true, pool[a], true, 9, 0};
  cfg.slots[1] = {true, pool[b], true, 9, 1};
  demo_ = std::make_unique<MatchScreen>(cfg, std::array<int, 4>{}, silent_, false, true);
}

void App::drawMainMenu() {
  const float t = time_, f = float(screenFrames_);
  float w, h;
  if (demo_) demo_->drawAttract();
  else if (art::pictureSize("menu_horizon", w, h)) art::drawPicture("menu_horizon", {0, 0, w, h}, {0, 0, kScreenW, kScreenH});
  // Night on the left under the menu, and along the bottom under the hints.
  const Color night{4, 7, 18, 255};
  DrawRectangleGradientH(0, 0, 358, kScreenH, withAlpha(night, 0.92f), withAlpha(night, 0.78f));
  DrawRectangleGradientH(358, 0, 232, kScreenH, withAlpha(night, 0.78f), withAlpha(night, 0.0f));
  DrawRectangleGradientV(0, kScreenH - 90, kScreenW, 90, withAlpha(night, 0.0f), withAlpha(night, 0.85f));
  // Two sparks breathing in the sky
  for (int i = 0; i < 2; ++i) {
    const float p = 0.5f + 0.5f * std::sin((t - i * 1.1f) * 2.0f * kPi / 2.4f);
    ui::eclat(i == 0 ? Vector2{212, 69} : Vector2{1010, 50}, (i == 0 ? 6.0f : 5.0f) * (0.85f + 0.3f * p), 0.5f + 0.5f * p,
              i == 0 ? WHITE : Color{255, 226, 181, 255});
  }

  // Logo, top left, sliding down into place
  float lw, lh;
  if (art::pictureSize("menu_logo", lw, lh)) {
    const float in = easeOut(f / 18.0f), lwd = 360.0f;
    art::drawPicture("menu_logo", {0, 0, lw, lh}, {36, 20 - (1.0f - in) * 12.0f, lwd, lwd * lh / lw}, in);
  }

  // Buttons, sliding in one after the other; the spark beside the chosen big one glides to it
  for (int i = 0; i < kItemCount; ++i) drawItem(i, i == menuSel_, t, (1.0f - easeOut((f - i * 2.0f) / 14.0f)) * 440.0f);
  if (menuSel_ < kBig) {
    const float to = itemRect(menuSel_).y + 27.0f;
    markY_ = markY_ < 0.0f ? to : markY_ + (to - markY_) * 0.35f;  // ~120 ms
    ui::eclat({35, markY_}, 7.0f, easeOut(f / 20.0f));
  } else {
    markY_ = -1.0f;
  }

  // What the chosen one does, under the buttons
  const Item& sel = kItems[menuSel_];
  drawText(sel.label, 48, 612, 14.0f, ui::kGold, FontKind::Title);
  ui::drawWrapped(sel.desc, 48, 630, 384, 17.0f, ui::kSoft);

  // Its tip, top right
  {
    const float ts = 18.0f, tagS = 13.0f, maxW = 470.0f;
    const float tagW = textWidth("ASTUCE", tagS, FontKind::Title) + 16.0f, textW = textWidth(sel.tip, ts);
    const float avail = maxW - 40.0f - tagW;
    const int lines = textW <= avail ? 1 : 2;  // every tip fits on two lines
    const float pw = lines == 1 ? textW + tagW + 40.0f : maxW, ph = 20.0f + lines * ts * 1.3f;
    const Rectangle pill{kScreenW - 28 - pw, 24, pw, ph};
    ui::bevel(pill, 10.0f, withAlpha(ui::kInk, 0.78f), withAlpha(ui::kInk, 0.78f));
    ui::bevelLines(ui::grow(pill, -1.0f), 9.6f, 1.0f, withAlpha(ui::kSky, 0.16f));
    const Rectangle tag{pill.x + 12, pill.y + (ph - 22.0f) * 0.5f, tagW, 22.0f};
    DrawRectangleRec(tag, ui::kGold);
    drawText("ASTUCE", tag.x + 8, capsY(tag.y + 11.0f, tagS, FontKind::Title), tagS, ui::kInk, FontKind::Title);
    ui::drawWrapped(sel.tip, tag.x + tagW + 12, pill.y + 10 + ts * 0.15f, avail, ts, ui::kCream);
  }

  // The demo fight's cards, bottom right
  if (demo_) drawDemoHud(demo_->match(), t);

  // Hints, bottom left; the version, bottom right
  const float kh = 26.0f, hy = kScreenH - 16.0f - kh, hs = 18.0f;
  float x = 48.0f;
  auto hint = [&](const char* key, ui::Btn b, const char* label, Color col) {
    x += ui::prompt(key, b, x, hy, kh) + 8.0f;
    drawText(label, x, capsY(hy + kh * 0.5f, hs, FontKind::Ui), hs, col);
    x += textWidth(label, hs) + 22.0f;
  };
  hint("↑ ↓", ui::Btn::DPadUD, "Choisir", ui::kSoft);
  hint("ENTRÉE", ui::Btn::A, "Valider", ui::kGold);
  hint("ÉCHAP", ui::Btn::B, "Quitter", ui::kSoft);
  const float vs = 15.0f;
  drawText(kVersion, kScreenW - 28 - textWidth(kVersion, vs, FontKind::Title), capsY(hy + kh * 0.5f, vs, FontKind::Title),
           vs, ui::kMuted, FontKind::Title);
}
