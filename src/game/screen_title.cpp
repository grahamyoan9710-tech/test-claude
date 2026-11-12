// The title screen and the main menu (« Prestige »). The title: a trailer of hard cuts, one per fighter in his colour,
// then the logo cut open on the blade, two fighters face to face on the horizon, the call to start. The menu: the four
// ways to play as big as a scoreboard (the chosen one huge, the others hollow), the other four as small buttons, the
// newest fighter on his colour on the right.
#include "game/app.hpp"
#include "game/fighter_art.hpp"
#include "game/fx.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
constexpr const char* kVersion = "v15.1";

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

// « Prestige »: the four ways to play stacked from y 146, the chosen one 200 px tall (its 170 px name and its line),
// the others 78; the other four in a row of small buttons at y 598.
Rectangle itemRect(int i, int sel) {
  if (i >= kBig) {
    float x = 56.0f;
    for (int k = kBig; k < i; ++k) x += 54.3f + ui::capsWidth(kItems[k].label, 11, 0.12f) + 8.0f;  // 12 | icon 18 | 9 | label | 14
    return {x, 598, 54.3f + ui::capsWidth(kItems[i].label, 11, 0.12f), 44};
  }
  float y = 146.0f;
  for (int k = 0; k < i; ++k) y += k == sel ? 200.0f : 78.0f;
  return {40, y, 620, i == sel ? 190.0f : 78.0f};
}

const CharacterDef* fighterById(const char* id) {
  for (const auto& c : roster())
    if (c.id == id) return &c;
  return nullptr;
}

// The frame of a looping clip at time t (seconds).
int loopFrame(const CharacterDef& d, int anim, float t) {
  int n = 1;
  float fw, fh, fps = 15.0f;
  if (!art::clipInfo(d, anim, n, fw, fh, fps)) return 0;
  return int(std::floor(t * fps)) % std::max(1, n);
}

// A fighter in his own pixel art, every pixel `scale` square, his frame's bottom on `bottom`, its left on x.
void figure(const char* id, int anim, float t, float x, float bottom, float scale, bool flip, float alpha = 1.0f) {
  const CharacterDef* d = fighterById(id);
  int n;
  float fw, fh, fps;
  if (!d || !art::clipInfo(*d, anim, n, fw, fh, fps)) return;
  art::drawClipPx(*d, anim, loopFrame(*d, anim, t), x, bottom - fh * scale, scale, flip, alpha);
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
    if (!ui::hovered(itemRect(i, menuSel_))) continue;
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

// The title screen: any confirm opens the menu.
void App::updateTitle(const MenuNav& nav) {
  if (nav.confirm || nav.start || ui::clicked({0, 0, kScreenW, kScreenH})) {
    audio_.play(Sfx::MenuSelect);
    goTo(Screen::MainMenu);
  }
  if (nav.back) quit_ = true;
}

namespace {
// The trailer's cuts, one per fighter (0.45 s each from 0.30 s): its colour on one side of the blade (the lean
// alternates), its name huge on the other, its number and martial art, the fighter sliding in, x4.
struct Cut {
  const char* id;
  float figX;  // the left of its x4 frame
};
const Cut kCuts[] = {{"h01", 814}, {"p01", 60}, {"p02", 724}, {"p03", 80}, {"p07", 740}, {"p08", 100}};

std::string numberLabel(const CharacterDef& d) {
  const Fiche fi = fiche(d);
  return "N° " + d.id.substr(1) + (fi.art.empty() ? std::string() : " · " + upperFr(fi.art));
}
std::string shortName(const CharacterDef& d) {
  const std::string n = upperFr(fiche(d).nick);
  return n.substr(0, n.find(' '));
}

// A night plate with a label in small spaced capitals (CSS: padding 5 9, font 14, letter-spacing 0.3 em) at top y;
// right: x is its right end. Returns its width.
float tag(const char* text, float x, float y, float size, float ls, Color col, bool right = false, float alpha = 1.0f) {
  const float w = 18.0f + ui::capsWidth(text, size, ls) + ls * size, h = 10.0f + 1.088f * size;
  const float x0 = right ? x - w : x;
  DrawRectangleRec({x0, y, w, h}, withAlpha(ui::kNight, alpha));
  ui::caps(text, x0 + 9, y + 5 + 0.534f * size, size, ls, withAlpha(col, alpha));
  return w;
}

void drawCut(int i, float u) {  // u: the % of the 9 s loop since it began (0..5)
  const Cut& c = kCuts[i];
  const CharacterDef* d = fighterById(c.id);
  if (!d) return;
  const ui::Flood fl = ui::flood(d->hero);
  const bool nameLeft = i % 2 == 0;  // its colour on the right, its name on the left, the fighter facing left
  if (nameLeft) {
    ui::lean(700, 1600, fl.fill);
    ui::lean(560, 700, fl.deep);
    ui::lean(557, 561, ui::kSilver);
  } else {
    ui::lean(-400, 580, fl.fill, -ui::kLean);
    ui::lean(580, 720, fl.deep, -ui::kLean);
    ui::lean(719, 723, ui::kSilver, -ui::kLean);
  }
  // The fighter: in from its side, then drifting on
  const float in = ui::seg(u, 0.0f, 1.6f), on = ui::seg(u, 1.6f, 5.0f);
  const float dx = u < 1.6f ? 140.0f * (1.0f - in) : -18.0f * on;
  int n;
  float fw, fh, fps;
  if (art::clipInfo(*d, art::kSheetIdle, n, fw, fh, fps))
    art::drawClipPx(*d, art::kSheetIdle, loopFrame(*d, art::kSheetIdle, u * 0.09f), c.figX + (nameLeft ? dx : -dx),
                    720.0f - fh * 4.0f, 4.0f, nameLeft);
  // Its name, landing big then settling
  const std::string name = shortName(*d);
  const float sc = u < 1.3f ? 1.18f - 0.18f * ui::seg(u, 0.0f, 1.3f) : 1.0f - 0.015f * ui::seg(u, 1.3f, 5.0f);
  const float w = ui::bigWidth(name.c_str(), 230, ui::Face::Cond), x = nameLeft ? 56.0f : kScreenW - 56.0f - w;
  ui::scaled(sc, nameLeft ? x : x + w, 437.0f + 0.7f * 0.84f * 230.0f,
             [&] { ui::big(name.c_str(), x, 437, 230, ui::Face::Cond, WHITE); });
  // Its number and art
  const float la = ui::seg(u, 0.0f, 1.6f);
  const std::string lab = numberLabel(*d);
  tag(lab.c_str(), nameLeft ? 62.0f : kScreenW - 62.0f, 410.0f + 10.0f * (1.0f - la), 14, 0.3f, ui::kSilver, !nameLeft, la);
}
}  // namespace

// The title (« Prestige »), on a 9 s loop like the mock-up: the blade (0-0.3 s), six cuts (0.3-3.0 s), then the logo
// cut open (3.2 s) with a jolt, the horizon line, the two fighters, the tagline and the call to start.
void App::drawTitle() {
  const float T = screenFrames_ / 60.0f, tau = std::fmod(T, 9.0f), p = tau / 9.0f * 100.0f;  // p: % of the loop
  constexpr float E = 0.2f, F = 0.8f;  // the hold's easing: cubic-bezier(.2,.8,.2,1)
  auto up = [&](float a, float b) { return ui::seg(p, a, b, E, F, E, 1.0f); };
  DrawRectangle(0, 0, kScreenW, kScreenH, ui::kNight);

  // The hold, shaken as the logo is cut open
  Vector2 sh{0, 0};
  {
    static const float kt[] = {35.6f, 36.0f, 36.5f, 37.0f, 37.6f, 38.3f};
    static const Vector2 kv[] = {{0, 0}, {-8, 5}, {7, -5}, {-5, 3}, {3, -2}, {0, 0}};
    for (int k = 0; k + 1 < 6; ++k)
      if (p >= kt[k] && p < kt[k + 1]) {
        const float u = (p - kt[k]) / (kt[k + 1] - kt[k]);
        sh = {kv[k].x + (kv[k + 1].x - kv[k].x) * u, kv[k].y + (kv[k + 1].y - kv[k].y) * u};
      }
  }
  rlPushMatrix();
  rlTranslatef(sh.x, sh.y, 0.0f);
  if (p >= 36.0f) {  // the horizon line, from its middle
    const float k = up(36.0f, 41.0f);
    DrawRectangleRec({640.0f - 640.0f * k, 652, 1280.0f * k, 1}, withAlpha(ui::kSilver, 0.3f));
  }
  if (p >= 37.0f) {  // the two fighters face to face, x3
    const float a = up(37.0f, 42.0f), dy = 30.0f * (1.0f - a);
    figure("h01", art::kSheetIdle, T, 70, 655 + dy, 3, false, a);
    figure("p01", art::kSheetIdle, T, 838, 655 + dy, 3, true, a);
  }
  if (p >= 35.6f) {
    ui::LogoIn in;
    in.cut = ui::seg(p, 35.6f, 38.4f);
    in.word = up(38.6f, 42.5f);
    in.lines = up(39.0f, 43.5f);
    if (p < 42.5f) in.starScale = 0.0f;
    else if (p < 45.0f) in.starScale = 1.3f * ui::seg(p, 42.5f, 45.0f), in.starRot = -90.0f + 100.0f * ui::seg(p, 42.5f, 45.0f);
    else in.starScale = 1.3f - 0.3f * ui::seg(p, 45.0f, 47.5f), in.starRot = 10.0f * (1.0f - ui::seg(p, 45.0f, 47.5f));
    ui::prestigeLogo(280, 86, 0.6f, T, in);
  }
  if (p >= 40.0f) {  // the tagline
    const float a = up(40.0f, 44.5f);
    const char* tagline = "COMBAT DE PLATEFORME · 1 À 4 JOUEURS";
    ui::caps(tagline, (kScreenW - ui::capsWidth(tagline, 13, 0.34f)) * 0.5f, 362 + 0.534f * 13 + 14 * (1.0f - a), 13, 0.34f,
             withAlpha(ui::kMist, a));
  }
  if (p >= 42.0f) {
    const float a = up(42.0f, 46.5f), dy = 14.0f * (1.0f - a);
    // The call to start: a real button, the blade's lean, its key, an arrow nudging on; a cyan line drawing under it
    {
      constexpr float h = 60.0f, cut = 13.0f;
      const char* word = "COMMENCER";
      const bool pad = padPrompts();
      const float kw = pad ? ui::padGlyphWidth(ui::Btn::A, 30, promptFamily()) : 20.0f + ui::capsWidth("ENTRÉE", 11, 0.14f) + 1.54f;
      const float tw = ui::capsWidth(word, 20, 0.24f) + 4.8f, w = 30.0f + kw + 18.0f + tw + 18.0f + 22.0f + 40.0f;
      const float x = (kScreenW - w) * 0.5f, y = 532.0f + dy;
      ui::quad({x + cut, y}, {x + w, y}, {x + w - cut, y + h}, {x, y + h}, withAlpha(ui::kSilver, a), withAlpha(ui::kSilver, a),
               withAlpha(ui::kSilver, a), withAlpha(ui::kSilver, a));
      float cx = x + 30.0f;
      if (pad) {
        ui::padGlyph(ui::Btn::A, cx, y + (h - 30.0f) * 0.5f, 30, promptFamily());
      } else {
        DrawRectangleRec({cx, y + 16, kw, 28}, withAlpha(ui::kNight, a));
        ui::caps("ENTRÉE", cx + 10, y + 30, 11, 0.14f, withAlpha(ui::kSilver, a));
      }
      cx += kw + 18.0f;
      ui::caps(word, cx, y + 30, 20, 0.24f, withAlpha(ui::kNight, a));
      cx += tw + 18.0f;
      const float nudge = 3.0f - 3.0f * std::cos(T * 2.0f * kPi / 1.6f), k = 22.0f / 24.0f, ax = cx + nudge, ay = y + 30 - 12 * k;
      const Color ink = withAlpha(ui::kNight, a);
      DrawLineEx({ax + 4 * k, ay + 12 * k}, {ax + 19 * k, ay + 12 * k}, 2.4f * k, ink);
      DrawLineEx({ax + 13 * k, ay + 5.5f * k}, {ax + 19.5f * k, ay + 12 * k}, 2.4f * k, ink);
      DrawLineEx({ax + 19.5f * k, ay + 12 * k}, {ax + 13 * k, ay + 18.5f * k}, 2.4f * k, ink);
      if (p >= 44.0f) DrawRectangleRec({x, y + h + 7, (w - cut) * up(44.0f, 49.0f), 4}, withAlpha(ui::kBlue, a));
    }
    constexpr FontKind B = FontKind::Body;
    const std::string ver = std::string("Version ") + (kVersion + 1) + " · bêta";
    drawText(ver.c_str(), 24, 682 + dy, 12, withAlpha(ui::kMist, a), B);
    const float fw = textWidth("Plein écran", 12, B), kw = ui::pkeyWidth("F11", ui::Btn::None, 24);
    drawText("Plein écran", kScreenW - 24 - kw - 8 - fw, 682 + dy, 12, withAlpha(ui::kMist, a), B);
    ui::pkey("F11", ui::Btn::None, kScreenW - 24 - kw, 676 + dy, 24, false, a);
  }
  rlPopMatrix();

  // The trailer: the blade crossing, then the cuts
  if (p < 3.3f) {
    const float k = ui::seg(p, 0.0f, 2.2f, 0.7f, 0.0f, 0.2f, 1.0f);
    DrawRectanglePro({640, 359.5f, 1400 * k, 3}, {700 * k, 1.5f}, -12.0f, ui::kSilver);
  }
  for (int i = 0; i < 6; ++i) {
    const float u = p - (0.30f + 0.45f * i) / 9.0f * 100.0f;
    if (u >= 0.0f && u < 5.0f) drawCut(i, u);
  }
}

void App::drawMainMenu() {
  const float T = screenFrames_ / 60.0f, t = time_;
  constexpr FontKind B = FontKind::Body;
  const float sec = T;
  auto in = [&](float delay, float len, float x1 = 0.2f, float y1 = 0.9f, float x2 = 0.1f) {
    return ui::seg(sec, delay, delay + len, x1, y1, x2, 1.0f);
  };
  DrawRectangle(0, 0, kScreenW, kScreenH, ui::kNight);
  if (menuChanged_ > screenFrames_) menuChanged_ = 0;  // (a former visit's)
  if (markY_ != float(menuSel_)) {  // the chosen one lands again on each change
    markY_ = float(menuSel_);
    menuChanged_ = screenFrames_;
  }
  const float since = (screenFrames_ - menuChanged_) / 60.0f, re = menuChanged_ > 0 ? since : sec;

  // The newest fighter's colour takes the right side, cut by the blade; his name scrolls in it, tone on tone
  const CharacterDef* hk = fighterById("h01");
  const ui::Flood fl = ui::flood(Hero::H01);
  {
    const float ox = 560.0f * (1.0f - in(0.0f, 0.45f));
    ui::lean(960 + ox, 1700 + ox, fl.fill);
    ui::lean(820 + ox, 960 + ox, fl.deep);
    ui::bigClip(820 + ox, false);
    const char* word = "HAKUEN ";
    const float ww = ui::bigWidth(word, 300, ui::Face::Cond);
    for (int row = 0; row < 2; ++row) {
      const float speed = 3.0f * ww / (row == 0 ? 34.0f : 46.0f), off = std::fmod(t * speed, 3.0f * ww);
      const float x0 = row == 0 ? -off : -3.0f * ww + off;
      for (int k = 0; k < 6; ++k)
        ui::bigHollow(word, x0 + k * ww + ox, row == 0 ? 28.0f : 300.0f, 300, ui::Face::Cond, 2.0f, withAlpha(ui::kNight, 0.18f));
    }
    ui::bigClipOff();
    ui::lean(817 + ox, 821 + ox, ui::kSilver);
  }
  // Hakuen in his own pixel art, x4, breathing
  if (hk) {
    const float a = in(0.12f, 0.6f);
    figure("h01", art::kSheetIdle, T, 900 + 90.0f * (1.0f - a), 720, 4, true, a);
  }
  // « Nouveau combattant »
  {
    const float a = in(0.3f, 0.4f, 0.2f, 0.8f, 0.2f), dy = 12.0f * (1.0f - a);
    const float tw = 18.0f + ui::capsWidth("NOUVEAU COMBATTANT", 12, 0.3f) + 3.6f;
    DrawRectangleRec({700, 538 + dy, tw, 10.0f + 1.088f * 12}, withAlpha(fl.fill, a));
    ui::caps("NOUVEAU COMBATTANT", 709, 538 + dy + 5 + 0.534f * 12, 12, 0.3f, withAlpha(ui::kNight, a));
    const float py = 538 + dy + 23.06f + 6, nw = ui::bigWidth("HAKUEN", 96, ui::Face::Cond);
    const char* sub = "Karaté · Lame blanche";
    const float pw = 16 + std::max(nw, textWidth(sub, 15, B)) + 26, ph = 10 + 0.84f * 96 + 6 + 18.15f + 8;
    ui::quad({700, py}, {700 + pw, py}, {700 + pw - 22, py + ph}, {700, py + ph}, withAlpha(ui::kNight, a), withAlpha(ui::kNight, a),
             withAlpha(ui::kNight, a), withAlpha(ui::kNight, a));
    ui::big("HAKUEN", 716, py + 10, 96, ui::Face::Cond, withAlpha(WHITE, a));
    drawText(sub, 716, py + 10 + 0.84f * 96 + 6, 15, withAlpha(ui::kMist, a), B);
  }
  // The logo, small; the tip of the chosen way
  {
    const float a = in(0.0f, 0.4f, 0.2f, 0.8f, 0.2f);
    rlPushMatrix();
    rlTranslatef(0, 12.0f * (1.0f - a), 0);
    ui::prestigeLogo(56, 30, 0.2f, t);
    rlPopMatrix();
    const float b = in(0.4f, 0.4f, 0.2f, 0.8f, 0.2f), dy = 12.0f * (1.0f - b);
    ui::caps("ASTUCE", 306, 50 + dy + 0.534f * 11, 11, 0.22f, withAlpha(ui::kBlue, b));
    drawText(kItems[menuSel_].tip, 306, 50 + dy + 11.97f + 6 + 1.33f, 14, withAlpha(ui::kMist, b), B);
  }

  // The four ways to play, as big as a scoreboard
  const int sel = menuSel_;
  for (int i = 0; i < kBig; ++i) {
    const Item& it = kItems[i];
    const Rectangle r = itemRect(i, sel);
    const float a = in(0.06f + 0.06f * i, 0.34f), ox = -56.0f * (1.0f - a);
    const char* num = TextFormat("%02d", i + 1);
    const bool chosen = i == sel, hover = !chosen && ui::hovered(r);
    const float size = chosen ? 170.0f : 92.0f, box = 0.84f * size, bottom = r.y + box;
    const float numPad = chosen ? 24.0f : 10.0f, tagPad = chosen ? 24.0f : 12.0f;
    const Color dim = it.ready ? ui::kMist : ui::kDim;
    float x = 56.0f + ox;
    ui::caps(num, x, bottom - numPad - 14.14f + 0.534f * 13, 13, 0.16f, withAlpha(chosen ? ui::kBlue : dim, a));
    x += ui::capsWidth(num, 13, 0.16f) + 2.08f + 16.0f;
    const float ww = ui::bigWidth(it.label, size, ui::Face::Cond);
    if (chosen) {
      if (re >= 0.1f) {  // it lands a little big, then settles
        const float u = (re - 0.1f) / 0.5f, k = u < 0.3f ? ui::seg(u, 0.0f, 0.3f) : 1.0f;
        const float sc = u < 0.3f ? 1.12f - 0.12f * k : 1.0f, al = u < 0.3f ? k : 1.0f;
        ui::scaled(sc, x, r.y + 0.8f * box, [&] { ui::big(it.label, x, r.y, size, ui::Face::Cond, withAlpha(WHITE, al * a)); });
      }
      // the cyan mark at its left, growing up
      const float m = ui::seg(re, 0.18f, 0.58f), mh = 126.0f * m, my = r.y + 4 + 126.0f;
      ui::quad({22 + ui::kLean * mh, my - mh}, {32 + ui::kLean * mh, my - mh}, {32, my}, {22, my}, ui::kBlue, ui::kBlue,
               ui::kBlue, ui::kBlue);
      drawText(it.desc, 104 + ox, r.y + 160, 16, withAlpha(ui::kSilver, a), B);
    } else {
      ui::bigHollow(it.label, x, r.y, size, ui::Face::Cond, 1.5f, withAlpha(hover ? ui::kSilver : dim, a));
    }
    x += ww + 16.0f + (chosen ? 6.0f : 0.0f);
    if (*it.tag)
      ui::caps(it.tag, x, bottom - tagPad - 11.97f + 0.534f * 11, 11, 0.18f,
               withAlpha(chosen ? ui::kBlue : it.ready ? ui::kMist : Color{127, 139, 174, 255}, a));
  }
  // The other four, small buttons
  {
    const float a = in(0.36f, 0.4f, 0.2f, 0.8f, 0.2f), dy = 12.0f * (1.0f - a);
    for (int i = kBig; i < kItemCount; ++i) {
      Rectangle r = itemRect(i, sel);
      r.y += dy;
      const bool chosen = i == sel, hover = ui::hovered(r);
      const Color ink = chosen ? ui::kNight : ui::kSilver;
      if (chosen) DrawRectangleRec(r, withAlpha(ui::kSilver, a));
      else {
        if (hover) DrawRectangleRec(r, withAlpha(ui::kSilver, 0.08f * a));
        DrawRectangleRoundedLinesEx(r, 6.0f / 22.0f, 6, 1.0f, withAlpha(Color{214, 226, 255, 255}, 0.2f * a));
      }
      ui::icon(kItems[i].icon, {r.x + 12 + 9, r.y + 22}, 18, withAlpha(ink, a), 1.8f * 18.0f / 24.0f);
      ui::caps(kItems[i].label, r.x + 39, r.y + 22, 11, 0.12f, withAlpha(ink, a));
    }
    if (sel >= kBig) drawText(kItems[sel].desc, 56, 556 + dy, 16, withAlpha(ui::kSilver, a), B);
  }
  // The hints and the version
  {
    const float a = in(0.44f, 0.4f, 0.2f, 0.8f, 0.2f), y = 654.0f + 12.0f * (1.0f - a);
    float x = 58.0f;
    auto hint = [&](const char* key, ui::Btn b, const char* label) {
      x += ui::pkey(key, b, x, y, 22, false, a) + 8.0f;
      drawText(label, x, y + 11 - 0.605f * 12, 12, withAlpha(ui::kMist, a), B);
      x += textWidth(label, 12, B) + 18.0f;
    };
    hint("↑ ↓", ui::Btn::DPadUD, "Choisir");
    hint("ENTRÉE", ui::Btn::A, "Valider");
    hint("ÉCHAP", ui::Btn::B, "Quitter");
    const std::string ver = std::string("Version ") + (kVersion + 1);
    drawText(ver.c_str(), x + 8, y + 11 - 0.605f * 12, 12, withAlpha(ui::kMist, a), B);
  }
}
