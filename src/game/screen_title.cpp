// The title screen and the main menu (« Épure »). The title: the logo (CLASH, the horizon line, H◎RIZON) over a real
// fight between two CPUs, « Appuie sur ENTRÉE ». The menu: eight buttons with their icons down the left, the chosen
// one's big icon, name, what it does and a tip on the right, a fighter's manga portrait melting into the dusk.
#include "game/app.hpp"
#include "game/fighter_art.hpp"
#include "game/fx.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
constexpr const char* kVersion = "v14.2";

float easeOut(float t) {
  t = clampf(t, 0.0f, 1.0f);
  return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
}

struct Item {
  const char* label;
  const char* tag;  // the note on the button's right
  ui::Icon icon;
  const char* desc;
  const char* tip;
  bool ready;  // false: shown, not open yet
};
const Item kItems[] = {
    {"JOUER", "1 JOUEUR", ui::Icon::Play, "Un combat contre l'ordinateur : choisis ton combattant et ton arène.",
     "Casse la Clash Orb : c'est la seule façon d'obtenir ton ultime.", true},
    {"VERSUS", "2 À 4", ui::Icon::Versus, "De 2 à 4 joueurs sur la même machine, en équipes ou chacun pour soi.",
     "Clavier 1, clavier 2 et manettes : jusqu'à 4 joueurs.", true},
    {"ENTRAÎNEMENT", "LIBRE", ui::Icon::Training, "Vies infinies, mannequin réglable en pause.",
     "Zones de coup visibles avec la touche F1.", true},
    {"EN LIGNE", "BIENTÔT", ui::Icon::Online, "Le jeu en ligne arrive dans une prochaine version.",
     "Le jeu en ligne arrive plus tard.", false},
    {"PERSONNAGES", "", ui::Icon::Fighters, "La galerie des combattants : leur style, leurs armes, leurs compétences.",
     "Les deux armes de chacun tombent sur l'arène pendant le combat.", true},
    {"ARÈNES", "", ui::Icon::Arenas, "L'aperçu des arènes et de leurs plateformes ; ENTRÉE pour s'y entraîner.",
     "Chaque arène a ses propres plateformes.", true},
    {"OPTIONS", "", ui::Icon::Options, "Son, affichage, règles du jeu, touches et manettes.",
     "Plein écran : touche F11.", true},
    {"QUITTER", "", ui::Icon::Quit, "Quitter Clash Horizon.", "À bientôt sur l'horizon.", true},
};
constexpr int kItemCount = int(sizeof kItems / sizeof kItems[0]);
constexpr int kBig = 4;  // the four ways to play, then a gap
constexpr int kQuit = kItemCount - 1;

Rectangle itemRect(int i) { return {64, 118.0f + i * 58.0f + (i >= kBig ? 14.0f : 0.0f), 400, 54}; }

// The chosen one is cream with its icon box in ink and an arrow; a closed one is faint.
void drawItem(int i, bool sel, float in) {
  const Item& it = kItems[i];
  Rectangle r = itemRect(i);
  r.x -= 28.0f * (1.0f - in);
  const float a = in * (it.ready ? 1.0f : 0.4f);
  if (sel) ui::roundRect(r, 14, withAlpha(ui::kCream, a), withAlpha(ui::kCream, a));
  else if (ui::hovered(r)) ui::roundRect(r, 14, withAlpha(ui::kCream, 0.06f * a), withAlpha(ui::kCream, 0.06f * a));
  const Rectangle ib{r.x + 8, r.y + 8, 38, 38};
  if (sel) ui::roundRect(ib, 10, withAlpha(ui::kInk, a), withAlpha(ui::kInk, a));
  else ui::roundLines(ib, 10, 1.0f, withAlpha(ui::kCream, 0.16f * a));
  const Color ink = sel ? ui::kInk : ui::kCream;
  ui::icon(it.icon, {ib.x + 19, ib.y + 19}, 20, withAlpha(sel ? ui::kCream : ui::kSoft, a));
  constexpr FontKind D = FontKind::Display;
  drawText(it.label, ib.x + 38 + 16, capsY(r.y + r.height * 0.5f, 16, D), 16, withAlpha(ink, a * (sel ? 1.0f : 0.78f)), D);
  float right = r.x + r.width - 18;
  if (sel) {
    ui::icon(ui::Icon::Arrow, {right - 9, r.y + r.height * 0.5f}, 18, withAlpha(ui::kInk, a), 2.0f);
    right -= 28;
  }
  if (*it.tag) {
    const float tw = ui::labelWidth(it.tag, 11, 0.14f);
    ui::label(it.tag, right - tw, r.y + r.height * 0.5f, 11, 0.14f, withAlpha(sel ? ui::kStone : ui::kMuted, a));
  }
}

// Inter text wrapped to `width`, 1.5 lines apart; returns the y under it.
float wrapBody(const std::string& text, float x, float y, float width, float size, Color col) {
  constexpr FontKind B = FontKind::Body;
  std::string line, word;
  auto flush = [&] {
    drawText(line.c_str(), x, y, size, col, B);
    y += size * 1.5f;
    line.clear();
  };
  for (size_t i = 0; i <= text.size(); ++i) {
    if (i < text.size() && text[i] != ' ') {
      word += text[i];
      continue;
    }
    const std::string next = line.empty() ? word : line + " " + word;
    if (!line.empty() && textWidth(next.c_str(), size, B) > width) {
      flush();
      line = word;
    } else {
      line = next;
    }
    word.clear();
  }
  if (!line.empty()) flush();
  return y;
}

// The demo fight's cards, top right: a blinking "DÉMO", then each fighter: portrait, "CPU 9 · N° 01", stock,
// damage (K.O. once out).
void drawDemoHud(const Match& m, float t) {
  constexpr float h = 40.0f, y = 24.0f, cardW = 226.0f, gap = 8.0f;
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
  if (nav.up) menuSel_ = (menuSel_ + kItemCount - 1) % kItemCount;
  if (nav.down) menuSel_ = (menuSel_ + 1) % kItemCount;
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

// The title screen: any confirm opens the menu.
void App::updateTitle(const MenuNav& nav) {
  if (nav.confirm || nav.start || ui::clicked({0, 0, kScreenW, kScreenH})) {
    audio_.play(Sfx::MenuSelect);
    goTo(Screen::MainMenu);
  }
  if (nav.back) quit_ = true;
}

namespace {
// The logo « Soleil »: CLASH rising, the horizon line with its travelling spark, H◎RIZON coming down, the O a ring with
// the cyan spark in it. f: frames since the screen opened.
void drawLogo(float f, float t) {
  constexpr FontKind D = FontKind::Display;
  constexpr float size = 108.0f, em = 0.05f * size;
  auto ease = [](float k) { k = clampf(k, 0.0f, 1.0f); return 1.0f - std::pow(1.0f - k, 3.0f); };
  auto wordW = [&](const char* w) {  // with the logo's letter spacing
    return MeasureTextEx(gameFont(D), w, size * fontScale(D), em).x;
  };
  auto word = [&](const char* w, float x, float y) {
    DrawTextEx(gameFont(D), w, {x, capsY(y, size, D)}, size * fontScale(D), em, ui::kCream);
  };
  const float k1 = ease((f - 33) / 54.0f);
  beginClip({0, 240, kScreenW, 112});
  word("CLASH", (kScreenW - wordW("CLASH")) * 0.5f, 296 + 118 * (1.0f - k1));
  endClip();
  // The line draws from its middle; its spark travels every 5 s.
  const float kl = ease(f / 48.0f), x0 = 236, x1 = 1044, cx = (x0 + x1) * 0.5f, hw = (x1 - x0) * 0.5f * kl;
  ui::glow({cx, 361}, 300, withAlpha(ui::kCream, 0.1f * kl));
  DrawRectangleRec({cx - hw, 360, hw * 2, 2}, ui::kCream);
  const float u = std::fmod(std::max(0.0f, t - 1.2f), 5.0f) / 5.0f, e = u < 0.5f ? 2 * u * u : 1 - std::pow(-2 * u + 2, 2) / 2;
  const float da = u < 0.1f ? u / 0.1f : u > 0.9f ? (1.0f - u) / 0.1f : 1.0f;
  if (f > 72) {
    ui::glow({x0 + (x1 - x0) * e, 361}, 30, withAlpha(ui::kBlue, 0.7f * da));
    DrawCircleV({x0 + (x1 - x0) * e, 361}, 6, withAlpha(ui::kBlue, da));
  }
  // H◎RIZON: the ring as wide as an O
  const float ow = 0.83f * size, gap = 0.05f * size, hW = wordW("H"), rW = wordW("RIZON");
  const float total = hW + em + ow + gap + rW, wx = (kScreenW - total) * 0.5f;
  const float k2 = ease((f - 33) / 54.0f), y = 428 - 118 * (1.0f - k2);
  beginClip({0, 372, kScreenW, 112});
  word("H", wx, y);
  const Vector2 oc{wx + hW + em + ow * 0.5f, y};
  const float sk = ow / 90.0f;  // the svg's 90-unit box
  for (int i = 0; i < 64; ++i) {  // the ring: an ellipse 37.5 x 31.5, 15 thick
    const float a0 = i * 2 * kPi / 64, a1 = (i + 1) * 2 * kPi / 64;
    const float ro = 1.0f + 7.5f / 37.5f, ri = 1.0f - 7.5f / 37.5f, ry = 31.5f / 37.5f, R = 37.5f * sk;
    const Vector2 p0{oc.x + std::cos(a0) * R * ro, oc.y + std::sin(a0) * R * (ry + 7.5f / 37.5f)},
        p1{oc.x + std::cos(a1) * R * ro, oc.y + std::sin(a1) * R * (ry + 7.5f / 37.5f)},
        q0{oc.x + std::cos(a0) * R * ri, oc.y + std::sin(a0) * R * (ry - 7.5f / 37.5f)},
        q1{oc.x + std::cos(a1) * R * ri, oc.y + std::sin(a1) * R * (ry - 7.5f / 37.5f)};
    ui::quad(p0, p1, q1, q0, ui::kCream, ui::kCream, ui::kCream, ui::kCream);
  }
  const float sp = 1.0f + 0.12f * std::sin(t * 2.0f * kPi / 2.4f);
  ui::glow(oc, 26 * sk * sp, withAlpha(ui::kBlue, 0.6f));
  ui::spark(oc, 20 * sk * sp, t * 0.4f, ui::kBlue, 0.23f);
  word("RIZON", oc.x + ow * 0.5f + gap, y);
  endClip();
}
}  // namespace

void App::drawTitle() {
  const float t = time_, f = float(screenFrames_);
  constexpr FontKind B = FontKind::Body;
  // The demo fight behind, under the dusk; a darker rim
  if (demo_) demo_->drawAttract();
  ui::dusk(demo_ ? 0.72f : 1.0f);
  ui::vignette(0.75f);
  // Slow rays, the ghost words drifting, the striped sun rising behind the line
  ui::sunburst({640, 360}, 640, 36, t * 2.0f * kPi / 90.0f, withAlpha(ui::kCream, 0.05f));
  {
    const char* ghost = "CLASH HORIZON · CLASH HORIZON · CLASH HORIZON · CLASH HORIZON";
    const float gx = -std::fmod(t * 35.0f, 1400.0f);
    drawText(ghost, gx, capsY(560, 240, FontKind::Display), 240, withAlpha(ui::kCream, 0.025f), FontKind::Display);
  }
  {
    const float k = clampf((f - 18) / 96.0f, 0.0f, 1.0f), e = 1.0f - std::pow(1.0f - k, 3.0f);
    const float lift = 120.0f * (1.0f - e);
    for (float y = 110; y < 361; y += 22) {  // 16 px stripes, 6 px gaps
      beginClip({390, y, 500, std::min(16.0f, 361.0f - y)});
      ui::glow({640, 361 + lift}, 250, withAlpha(ui::kBlue, 0.75f * e));
      endClip();
    }
  }
  for (int i = 0; i < 8; ++i) {  // motes rising
    const float u = std::fmod(t + i * 0.9f, 7.0f) / 7.0f, a = u < 0.15f ? u / 0.15f * 0.7f : 0.7f * (1.0f - u) / 0.85f;
    DrawCircleV({float(180 + i * 132 - (i > 3 ? 20 : 0)), 726 - 640 * u}, 2, withAlpha(ui::kCream, a));
  }
  const float fade = clampf((f - 78) / 42.0f, 0.0f, 1.0f);
  const char* tagline = "COMBAT DE PLATEFORME · 1 À 4 JOUEURS";
  ui::label(tagline, (kScreenW - ui::labelWidth(tagline, 12, 0.34f)) * 0.5f, 203, 12, 0.34f, withAlpha(ui::kMuted, fade));
  drawLogo(f, t);

  // « Appuie sur ENTRÉE pour commencer », breathing
  {
    const float k = clampf((f - 90) / 42.0f, 0.0f, 1.0f) * (f > 132 ? 0.725f + 0.275f * std::cos((t - 2.2f) * 2.0f * kPi / 2.2f) : 1.0f);
    const float kw = ui::promptWidth("ENTRÉE", ui::Btn::A, 26), w1 = textWidth("Appuie sur", 15, B), w2 = textWidth("pour commencer", 15, B);
    float x = (kScreenW - (w1 + 12 + kw + 12 + w2)) * 0.5f;
    drawText("Appuie sur", x, capsY(573, 15, B), 15, withAlpha(ui::kCream, k), B);
    x += w1 + 12;
    if (k > 0.05f) ui::prompt("ENTRÉE", ui::Btn::A, x, 560, 26);
    x += kw + 12;
    drawText("pour commencer", x, capsY(573, 15, B), 15, withAlpha(ui::kCream, k), B);
  }
  // The version, bottom left; full screen, bottom right; the demo's cards over them
  const std::string ver = std::string("Version ") + (kVersion + 1) + " · bêta";
  drawText(ver.c_str(), 48, capsY(675, 12, B), 12, withAlpha(ui::kMuted, fade), B);
  const float fw = textWidth("Plein écran", 12, B), kw = ui::keycapWidth("F11", 24);
  ui::keycap("F11", kScreenW - 48 - fw - 10 - kw, 663, 24);
  drawText("Plein écran", kScreenW - 48 - fw, capsY(675, 12, B), 12, withAlpha(ui::kMuted, fade), B);
  if (demo_) drawDemoHud(demo_->match(), t);
}

void App::drawMainMenu() {
  const float t = time_, f = float(screenFrames_);
  constexpr FontKind D = FontKind::Display, B = FontKind::Body;
  ui::dusk();
  auto ease = [](float k) { k = clampf(k, 0.0f, 1.0f); return 1.0f - std::pow(1.0f - k, 3.0f); };

  // A fighter's manga portrait on the right, melting into the dusk
  float aw, ah;
  if (art::pictureSize("h01_art", aw, ah)) {
    const float k = ease((f - 9) / 60.0f), h = 600.0f, w = h * aw / ah;
    art::drawPictureFaded("h01_art", {kScreenW - 24 - w + 60 * (1.0f - k), kScreenH - 78 - h, w, h}, 0.38f, 0.22f, 0.92f * k);
  }

  // The small logo, top left
  {
    const float lx = 64, cy = 51;
    const float w1 = ui::labelWidth("CLASH", 15, 0.14f);
    DrawTextEx(gameFont(D), "CLASH", {lx, capsY(cy, 15, D)}, 15 * fontScale(D), 0.14f * 15, ui::kCream);
    const float c1 = MeasureTextEx(gameFont(D), "CLASH", 15 * fontScale(D), 0.14f * 15).x;
    (void)w1;
    DrawRectangleRec({lx + c1 + 12, cy - 1, 40, 2}, ui::kCream);
    DrawCircleV({lx + c1 + 52, cy}, 3, ui::kBlue);
    DrawTextEx(gameFont(D), "HORIZON", {lx + c1 + 64, capsY(cy, 15, D)}, 15 * fontScale(D), 0.14f * 15, ui::kCream);
  }

  // The buttons, sliding in one after the other
  for (int i = 0; i < kItemCount; ++i) drawItem(i, i == menuSel_, ease((f - 3 - i * 2.7f) / 33.0f));

  // The chosen one, big on the right: its icon, its name and note, what it does, a tip (fades in on each change)
  if (markY_ != float(menuSel_)) {
    markY_ = float(menuSel_);
    menuChanged_ = screenFrames_;
  }
  const Item& sel = kItems[menuSel_];
  const float k = ease((f - std::max(15.0f, float(menuChanged_))) / 30.0f), sx = 560 + 36 * (1.0f - k);
  ui::icon(sel.icon, {sx + 36, 206}, 72, withAlpha(ui::kCream, k), 0.9f * 24.0f / 72.0f * 3.0f);
  drawText(sel.label, sx, capsY(300, 64, D), 64, withAlpha(ui::kCream, k), D);
  if (*sel.tag) {
    const float lw = textWidth(sel.label, 64, D), tw = ui::labelWidth(sel.tag, 11, 0.14f) + 20;
    const Rectangle pill{sx + lw + 14, 287, tw, 26};
    ui::roundLines(pill, 13, 1.0f, withAlpha(ui::kCream, 0.3f * k));
    ui::label(sel.tag, pill.x + 10, pill.y + 13, 11, 0.14f, withAlpha(ui::kCream, k));
  }
  float y = wrapBody(sel.desc, sx, 340, 330, 19, withAlpha(ui::kSoft, k));
  DrawRectangleRec({sx, y + 22, 40, 1}, withAlpha(ui::kCream, 0.3f * k));
  ui::label("ASTUCE", sx, y + 47, 11, 0.24f, withAlpha(ui::kMuted, k));
  wrapBody(sel.tip, sx, y + 62, 330, 15, withAlpha(ui::kSoft, k));

  // The hints and the version along the bottom
  DrawRectangleRec({0, 640, float(kScreenW), 1}, ui::kHair);
  const float kh = 24.0f, hy = 664.0f, hs = 13.0f;
  float x = 64.0f;
  auto hint = [&](const char* key, ui::Btn b, const char* label) {
    x += ui::prompt(key, b, x, hy, kh) + 8.0f;
    drawText(label, x, capsY(hy + kh * 0.5f, hs, B), hs, ui::kSoft, B);
    x += textWidth(label, hs, B) + 22.0f;
  };
  hint("↑ ↓", ui::Btn::DPadUD, "Choisir");
  hint("ENTRÉE", ui::Btn::A, "Valider");
  hint("ÉCHAP", ui::Btn::B, "Quitter");
  const std::string ver = std::string("Version ") + (kVersion + 1);
  drawText(ver.c_str(), kScreenW - 64 - textWidth(ver.c_str(), 12, B), capsY(hy + kh * 0.5f, 12, B), 12, ui::kMuted, B);
}
