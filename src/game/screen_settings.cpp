// Settings screen (« Prestige », the mockup validated on 13/11): the night raised on the right, cut by the blade, and
// the five tabs on the left as big as the main menu's scoreboard. Audio, Affichage, Graphismes and Jeu are rows as in
// « Règles & mode », the help of the chosen row under them. Commandes: the device first (CLAVIER 1, CLAVIER 2,
// MANETTE), then only its keys; the gamepad drawn in outline (Xbox, PS4 or PS5: the one plugged in, or the one
// chosen), each of its buttons wired to its action.
#include <algorithm>
#include <cmath>
#include <cstring>

#include "game/app.hpp"
#include "game/fighter_art.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"
#ifdef CLASH_HAS_GLFW  // our own raylib build: its GLFW lists the screen's modes
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#endif

namespace {
constexpr FontKind kB = FontKind::Body;
constexpr int kKeysTab = 4;
constexpr Color kRim{214, 226, 255, 255};  // the hairlines: 12 % under the rows, 20 % around the widgets
constexpr Color kWhite{255, 255, 255, 255};
constexpr float kX = 548, kR = 1240;  // the drawer's column
constexpr float kRowsY = 118, kRowH = 53;  // 52 and the hairline under it

// ---- The rows ---------------------------------------------------------------------------------
enum class Kind { Slider, Toggle, Select, Number, Button };
struct RowDef {
  const char* title;
  const char* sub;
  Kind kind;
  const char* help;
  const char* opts[5];  // a switch's choices (nullptr after the last); a button's label
};
const RowDef kRows[4][7] = {
    {{"Volume général", "Tout le jeu", Kind::Slider, "Volume de tout le jeu.", {}},
     {"Musique", "Menus et combats", Kind::Slider, "Volume des musiques des menus et des combats.", {}},
     {"Effets sonores", "Coups, sauts, K.O., la foule", Kind::Slider, "Coups, sauts, K.O., cris de la foule...", {}},
     {"Tester le son", "Un coup puissant et la foule", Kind::Button, "Joue un coup puissant et les cris de la foule.",
      {"ÉCOUTER"}}},
    {{"Mode d'affichage", "Touche F11", Kind::Toggle,
      "Plein écran : le jeu couvre tout l'écran, sans changer la résolution de l'écran (aucune bande noire). Fenêtré : "
      "une fenêtre de la taille choisie.",
      {"PLEIN ÉCRAN", "FENÊTRÉ"}},
     {"Résolution", "Celles de ton écran", Kind::Select,
      "En plein écran, la résolution de rendu du combat : plus basse, le combat est moins fin mais plus rapide. Les "
      "menus et les textes restent toujours nets, à la résolution de l'écran. En fenêtré : la taille de la fenêtre.",
      {}},
     {"Synchronisation verticale", "V-Sync", Kind::Toggle,
      "Les images sont calées sur l'écran, sans déchirure. Sans, la latence baisse un peu mais l'image peut se "
      "déchirer.",
      {"OUI", "NON"}},
     {"Secousses de l'écran", "Gros coups et K.O.", Kind::Toggle,
      "Tremblements de la caméra lors des gros coups et des K.O.", {"NON", "LÉGÈRES", "NORMALES"}},
     {"Noms au-dessus des joueurs", "J1, J2... ou CPU", Kind::Toggle,
      "Affiche J1, J2... ou CPU au-dessus de chaque combattant.", {"OUI", "NON"}},
     {"Compteur d'images", "FPS, en haut à gauche", Kind::Toggle,
      "Affiche le nombre d'images par seconde en haut à gauche.", {"OUI", "NON"}},
     {"Zones de coup", "Hitboxes · touche F1", Kind::Toggle, "Montre les zones d'attaque pendant les combats.",
      {"OUI", "NON"}}},
    {{"Préréglage", "Règle les quatre suivants", Kind::Select,
      "Règle la qualité, le lancer de rayons, la mise à l'échelle et l'anticrénelage d'un coup. Performance : le plus "
      "de FPS. Ultra : tout au maximum. Perso : tes propres choix.",
      {"PERFORMANCE", "ÉQUILIBRÉ", "QUALITÉ", "ULTRA", "PERSO"}},
     {"Qualité graphique", "Halos et persos éclairés", Kind::Toggle,
      "Haute : persos en pixel art, halo des effets et persos éclairés par eux. Moyenne : rendu simple, bien plus "
      "léger.",
      {"HAUTE", "MOYENNE"}},
     {"Lancer de rayons", "L'éclairage tracé", Kind::Toggle,
      "La lumière des effets, de la lune ou du soleil est tracée par des rayons sur la carte graphique, avec de vraies "
      "ombres. Moyen : 4 fois moins de pixels et 2 fois moins de rayons que Haut. Avec FSR 1, il en trace moins lui "
      "aussi. Non : l'éclairage dessiné, le plus léger.",
      {"NON", "MOYEN", "HAUT"}},
     {"Mise à l'échelle", "AMD FSR 1", Kind::Select,
      "AMD FSR 1, le vrai (EASU agrandit l'image en gardant les bords nets, RCAS la rend nette). Le combat est calculé "
      "plus petit puis agrandi : plus de FPS. Les menus et les textes ne sont jamais réduits.",
      {"NATIVE", "FSR 1 · QUALITÉ", "FSR 1 · ÉQUILIBRÉ", "FSR 1 · PERFORMANCE"}},
     {"Anticrénelage", "Bords en escalier", Kind::Toggle,
      "FXAA : léger, presque gratuit. SSAA ×2 : l'image est calculée deux fois plus grande puis réduite, le plus beau "
      "et le plus lourd.",
      {"NON", "FXAA", "SSAA ×2"}},
     {"Netteté", "RCAS de FSR 1", Kind::Slider, "La netteté de FSR 1 (RCAS) : 100 = la plus forte, 0 = la plus douce.",
      {}}},
    {{"Vies par défaut", "Par combattant", Kind::Number, "Nombre de vies de chaque combattant au début d'un match.", {}},
     {"Clash Orb", "La seule source des ultimes", Kind::Toggle,
      "L'orbe qui traverse l'arène. La casser donne l'ultime : c'est la seule façon de l'obtenir.", {"OUI", "NON"}},
     {"Armes", "Elles tombent sur l'arène", Kind::Toggle,
      "Les deux armes de chaque combattant tombent sur l'arène : il prend la sienne en restant dessus, touche Arme "
      "maintenue 1 s.",
      {"OUI", "NON"}},
     {"Niveau des ordinateurs", "Le 10 frappe deux fois plus fort", Kind::Number,
      "Niveau par défaut des combattants contrôlés par l'ordinateur.", {}},
     {"Saut avec Haut", "Stick ou touche", Kind::Toggle,
      "Haut fait sauter. Haut + attaque ou spéciale ensemble reste un coup vers le haut.", {"OUI", "NON"}},
     {"Tout réinitialiser", "Paramètres et touches", Kind::Button,
      "Remet tous les paramètres et toutes les touches par défaut.", {"RÉINITIALISER"}}},
};
const int kRowCount[4] = {4, 7, 6, 6};
const char* kTabNames[5] = {"AUDIO", "AFFICHAGE", "GRAPHISMES", "JEU", "COMMANDES"};
const char* kTabSub[5] = {"Le volume de tout le jeu, de la musique et des effets.",
                          "L'écran, la résolution et ce qui s'affiche pendant les combats.",
                          "La qualité de l'image : l'éclairage, la mise à l'échelle, l'anticrénelage.",
                          "Les règles par défaut des prochains combats.",
                          "Choisis le périphérique : seules ses touches s'affichent."};
int optCount(const RowDef& d) {
  int n = 0;
  while (n < 5 && d.opts[n]) ++n;
  return n;
}

// The graphics presets (Qualité, Lancer de rayons, Mise à l'échelle, Anticrénelage); anything else is « Perso ».
struct Preset {
  bool high;
  int rtx, upscale, aa;
};
constexpr Preset kPresets[4] = {{false, 0, 3, 1}, {true, 0, 1, 1}, {true, 1, 0, 1}, {true, 2, 0, 2}};
int presetOf(const Settings& s) {
  for (int i = 0; i < 4; ++i) {
    const Preset& p = kPresets[i];
    if (s.highQuality == p.high && s.rtx == p.rtx && s.upscale == p.upscale && s.antiAlias == p.aa) return i;
  }
  return 4;
}

// ---- The screen's resolutions --------------------------------------------------------------
struct Res {
  int w, h;
};
// What the « Résolution » selector offers: 0 = the screen's own, then its modes from the largest (at least 1024 x 576).
std::vector<Res> resolutions() {
  std::vector<Res> out{{0, 0}};
  const int m = GetCurrentMonitor(), mw = GetMonitorWidth(m), mh = GetMonitorHeight(m);
  auto add = [&](int w, int h) {
    if (w < 1024 || h < 576 || w > mw || h > mh || (w == mw && h == mh)) return;
    for (const Res& r : out)
      if (r.w == w && r.h == h) return;
    out.push_back({w, h});
  };
#ifdef CLASH_HAS_GLFW
  int nm = 0, nv = 0;
  GLFWmonitor** mons = glfwGetMonitors(&nm);
  const GLFWvidmode* modes = (mons && m >= 0 && m < nm) ? glfwGetVideoModes(mons[m], &nv) : nullptr;
  for (int i = nv - 1; i >= 0; --i) add(modes[i].width, modes[i].height);  // GLFW lists them smallest first
#endif
  if (out.size() == 1)  // no list: the usual 16:9 ones under the screen's size
    for (Res r : {Res{3840, 2160}, Res{2560, 1440}, Res{1920, 1080}, Res{1600, 900}, Res{1366, 768}, Res{1280, 720}}) add(r.w, r.h);
  const Settings& s = settings();
  if (s.resW) add(s.resW, s.resH);  // a saved one the list lost (another screen)
  return out;
}
int resolutionIndex(const std::vector<Res>& list) {
  const Settings& s = settings();
  for (size_t i = 0; i < list.size(); ++i)
    if (list[i].w == s.resW && list[i].h == s.resH) return int(i);
  return 0;
}

// A row's switch: its chosen option; and setting it.
int choiceOf(int key) {
  const Settings& s = settings();
  switch (key) {
    case 10: return s.fullscreen ? 0 : 1;
    case 12: return s.vsync ? 0 : 1;
    case 13: return std::clamp(s.screenShake, 0, 2);
    case 14: return s.playerTags ? 0 : 1;
    case 15: return s.showFps ? 0 : 1;
    case 16: return s.showHitboxes ? 0 : 1;
    case 21: return s.highQuality ? 0 : 1;
    case 22: return std::clamp(s.rtx, 0, 2);
    case 24: return std::clamp(s.antiAlias, 0, 2);
    case 31: return s.items ? 0 : 1;
    case 32: return s.weapons ? 0 : 1;
    case 34: return s.tapJump ? 0 : 1;
    default: return 0;
  }
}
void setChoice(int key, int k) {
  Settings& s = settings();
  switch (key) {
    case 10: s.fullscreen = k == 0; break;
    case 12: s.vsync = k == 0; break;
    case 13: s.screenShake = k; break;
    case 14: s.playerTags = k == 0; break;
    case 15: s.showFps = k == 0; break;
    case 16: s.showHitboxes = k == 0; break;
    case 21: s.highQuality = k == 0; break;
    case 22: s.rtx = k; break;
    case 24: s.antiAlias = k; break;
    case 31: s.items = k == 0; break;
    case 32: s.weapons = k == 0; break;
    case 34: s.tapJump = k == 0; break;
    default: break;
  }
}
int& sliderOf(int key) {
  Settings& s = settings();
  return key == 0 ? s.masterVolume : key == 1 ? s.musicVolume : key == 2 ? s.sfxVolume : s.sharpness;
}
std::string selectText(int key) {
  const Settings& s = settings();
  if (key == 11) {
    if (s.resW) return TextFormat("%d × %d", s.resW, s.resH);
    const int m = GetCurrentMonitor(), mw = GetMonitorWidth(m), mh = GetMonitorHeight(m);
    if (!s.fullscreen) return TextFormat("AUTO · %d × %d", windowW(s, mw), windowH(s, mw, mh));
    return TextFormat("ÉCRAN · %d × %d", mw, mh);
  }
  if (key == 20) return kRows[2][0].opts[presetOf(s)];
  return kRows[2][3].opts[std::clamp(s.upscale, 0, 3)];
}

// ---- Where things are ------------------------------------------------------------------------
// The tabs, as on the main menu: the chosen one big and white, the others hollow; those after it lower.
struct TabBox {
  float top, h, nameX, nameW;
};
TabBox tabBox(int i, int sel) {
  const float em = i == sel ? 76.0f : 46.0f;
  TabBox b;
  b.top = 150.0f + i * 62.0f + (i > sel ? 34.0f : 0.0f);
  b.h = 0.84f * em;
  b.nameX = 56.0f + ui::capsWidth(TextFormat("%02d", i + 1), 12, 0.16f) + 1.92f + 14.0f;
  b.nameW = ui::bigWidth(kTabNames[i], em, ui::Face::Cond);
  return b;
}
Rectangle tabRect(int i, int sel) {
  const TabBox b = tabBox(i, sel);
  return {56, b.top, b.nameX + b.nameW - 56, b.h};
}
Rectangle closeRect() { return {1200, 40, 40, 40}; }
Rectangle rowRect(int row) { return {kX, kRowsY + row * kRowH, kR - kX, kRowH}; }
float rowCy(int row) { return kRowsY + row * kRowH + 26.0f; }

// A switch (its boxes side by side, the chosen one silver), right-aligned on kR and centred on cy.
float segWidth(const char* s) { return std::max(64.0f, ui::capsWidth(s, 10, 0.1f) + 1.0f + 24.0f); }
float togWidth(const char* const* opts, int n) {
  float w = 2.0f;
  for (int i = 0; i < n; ++i) w += segWidth(opts[i]);
  return w;
}
Rectangle togBox(const char* const* opts, int n, float cy, float right = kR) {
  const float w = togWidth(opts, n);
  return {right - w, cy - 17, w, 34};
}
Rectangle togSeg(const char* const* opts, int n, float cy, int k, float right = kR) {
  float x = right - togWidth(opts, n) + 1.0f;
  for (int i = 0; i < k; ++i) x += segWidth(opts[i]);
  return {x, cy - 16, segWidth(opts[k]), 32};
}
// A selector: < its value > in a box at least 260 wide.
Rectangle selBox(const char* text, float cy) {
  const float w = 32.0f + std::max(196.0f, ui::capsWidth(text, 10, 0.12f) + 1.2f) + 32.0f + 2.0f;
  return {kR - w, cy - 17, w, 34};
}
Rectangle selPrev(const char* text, float cy) { const Rectangle b = selBox(text, cy); return {b.x + 1, cy - 16, 32, 32}; }
Rectangle selNext(const char* text, float cy) { const Rectangle b = selBox(text, cy); return {b.x + b.width - 33, cy - 16, 32, 32}; }
// A number: < 3 >; a slider: < ——— > 80
Rectangle numPrev(float cy) { return {kR - 120, cy - 16, 32, 32}; }
Rectangle numNext(float cy) { return {kR - 32, cy - 16, 32, 32}; }
Rectangle sliderPrev(float cy) { return {kR - 362, cy - 16, 32, 32}; }
Rectangle sliderTrack(float cy) { return {kR - 316, cy - 1.5f, 220, 3}; }
Rectangle sliderNext(float cy) { return {kR - 82, cy - 16, 32, 32}; }
// A button (and the one under the controls): its label in a thin box.
Rectangle btnRect(const char* label, float y, float right = kR) {
  const float w = ui::capsWidth(label, 10, 0.12f) + 1.2f + 34.0f;
  return {right - w, y, w, 34};
}

// ---- Drawing helpers --------------------------------------------------------------------------
void hair(Rectangle r, float a) { DrawRectangleLinesEx(r, 1.0f, withAlpha(kRim, 0.2f * a)); }
void ring(Rectangle r) { DrawRectangleLinesEx(ui::grow(r, 3.0f), 2.0f, ui::kBlue); }  // the focus, as on « Règles »
// A shape leaning like the blade (CSS skewX(-12deg) around its centre).
void skewed(Rectangle r, Color c) {
  const float k = r.height * 0.5f * ui::kLean;
  ui::quad({r.x + k, r.y}, {r.x + r.width + k, r.y}, {r.x + r.width - k, r.y + r.height}, {r.x - k, r.y + r.height}, c,
           c, c, c);
}
void chevron(Rectangle r, int dir, float size, float a) {
  const Color c = withAlpha(ui::hovered(r) ? ui::kSilver : ui::kMist, a);
  ui::icon(dir < 0 ? ui::Icon::ChevronL : ui::Icon::ChevronR, {r.x + r.width * 0.5f, r.y + r.height * 0.5f}, size, c, 2.0f);
}
void drawToggle(const char* const* opts, int n, int chosen, float cy, float a, float right = kR) {
  hair(togBox(opts, n, cy, right), a);
  for (int k = 0; k < n; ++k) {
    const Rectangle g = togSeg(opts, n, cy, k, right);
    const bool on = k == chosen;
    if (on) DrawRectangleRec(g, withAlpha(ui::kSilver, a));
    const Color ink = on ? ui::kNight : ui::hovered(g) ? ui::kSilver : ui::kMist;
    ui::caps(opts[k], g.x + (g.width - ui::capsWidth(opts[k], 10, 0.1f) - 1.0f) * 0.5f, cy, 10, 0.1f, withAlpha(ink, a));
  }
}
void drawButton(Rectangle b, const char* label, float a) {
  if (ui::hovered(b)) DrawRectangleRec(b, withAlpha(ui::kSilver, 0.08f * a));
  hair(b, a);
  ui::caps(label, b.x + 17, b.y + b.height * 0.5f, 10, 0.12f, withAlpha(ui::kSilver, a));
}
// ---- The gamepad --------------------------------------------------------------------------------
// Its buttons on each drawing (assets/sprites/ui_pad_*.png, from the mockup): centre and radius, in the drawing's units.
enum Spot { kLT, kLB, kRT, kRB, kLS, kDP, kRS, kBK, kST, kFY, kFX, kFB, kFA, kSpots };
constexpr float kSpot[3][kSpots][3] = {
    {{87, 22, 9}, {84, 40, 8}, {233, 22, 9}, {236, 40, 8}, {88, 86, 22}, {132, 122, 18}, {198, 128, 22},
     {139, 87, 6}, {181, 87, 6}, {234, 70, 9}, {216, 88, 9}, {252, 88, 9}, {234, 106, 9}},
    {{79, 22, 9}, {78, 38, 8}, {241, 22, 9}, {242, 38, 8}, {116, 132, 21}, {68, 90, 18}, {204, 132, 21},
     {101, 61, 7}, {219, 61, 7}, {258, 70, 10}, {238, 90, 10}, {278, 90, 10}, {258, 110, 10}},
    {{79, 24, 9}, {78, 42, 8}, {241, 24, 9}, {242, 42, 8}, {128, 130, 20}, {64, 92, 18}, {192, 130, 20},
     {94, 61, 7}, {226, 61, 7}, {260, 72, 10}, {240, 92, 10}, {280, 92, 10}, {260, 112, 10}}};
const char* kPadPicture[3] = {"ui_pad_xbox", "ui_pad_ps4", "ui_pad_ps5"};
// The drawing on the screen: its (0, 0) and scale; the picture covers x -12..332, y 0..240 of its units.
constexpr float kPadAX = 548, kPadAY = 226, kPadOX = kPadAX + 166, kPadOY = kPadAY + 40, kPadK = 1.125f;
int spotOf(int button) {
  switch (button) {
    case kPadA: return kFA;
    case kPadB: return kFB;
    case kPadX: return kFX;
    case kPadY: return kFY;
    case kPadLB: return kLB;
    case kPadRB: return kRB;
    case kPadLT: return kLT;
    case kPadRT: return kRT;
    case kPadBack: return kBK;
    case kPadStart: return kST;
    case kPadL3: return kLS;
    case kPadR3: return kRS;
    default: return -1;
  }
}
bool spotRight(int spot) { return spot == kRT || spot == kRB || spot == kRS || spot == kST || spot >= kFY; }

// The pad the tab shows: the first connected one of that family (-1: none, the saved bindings).
int shownPad(PadFamily f) {
  for (int p = 0; p < kMaxPads; ++p)
    if (padConnected(p) && padFamily(p) == f) return p;
  return -1;
}
int boundButton(int pad, int action, int slot) {
  return pad >= 0 ? padBinding(pad, action, slot) : settings().pad[action][slot];
}
// The drawing: the one chosen, else the pad's own (a DualSense, another PlayStation pad, the rest as an Xbox one).
int padArt(int pad) {
  const int style = settings().padStyle;
  if (style > 0) return std::min(style - 1, 2);
  if (pad < 0) return promptFamily() == PadFamily::PlayStation ? 1 : 0;  // none plugged: the kind last used
  if (std::strstr(padModel(pad), "DualSense")) return 2;
  return padFamily(pad) == PadFamily::PlayStation ? 1 : 0;
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

// What a button is called on the pad drawn (Nintendo pads are drawn as Xbox ones, with their own letters).
struct Glyph {
  const char* text;  // nullptr: a PlayStation shape or the d-pad's cross
  Color col;
  bool wide;
};
Glyph glyphOf(int spot, bool click, int art, bool nintendo) {
  const bool ps = art > 0;
  switch (spot) {
    case kFA: return ps ? Glyph{nullptr, {127, 167, 255, 255}, false} : nintendo ? Glyph{"B", ui::kSilver, false} : Glyph{"A", {108, 203, 95, 255}, false};
    case kFB: return ps ? Glyph{nullptr, {255, 90, 106, 255}, false} : nintendo ? Glyph{"A", ui::kSilver, false} : Glyph{"B", {229, 72, 77, 255}, false};
    case kFX: return ps ? Glyph{nullptr, {229, 143, 216, 255}, false} : nintendo ? Glyph{"Y", ui::kSilver, false} : Glyph{"X", {79, 155, 255, 255}, false};
    case kFY: return ps ? Glyph{nullptr, {63, 208, 180, 255}, false} : nintendo ? Glyph{"X", ui::kSilver, false} : Glyph{"Y", {242, 201, 76, 255}, false};
    case kLT: return {ps ? "L2" : nintendo ? "ZL" : "LT", ui::kSilver, true};
    case kLB: return {ps ? "L1" : nintendo ? "L" : "LB", ui::kSilver, !nintendo};
    case kRT: return {ps ? "R2" : nintendo ? "ZR" : "RT", ui::kSilver, true};
    case kRB: return {ps ? "R1" : nintendo ? "R" : "RB", ui::kSilver, !nintendo};
    case kBK: return {art == 1 ? "SHARE" : art == 2 ? "CRÉER" : nintendo ? "-" : "VUE", ui::kSilver, !nintendo};
    case kST: return {ps ? "OPTIONS" : nintendo ? "+" : "MENU", ui::kSilver, !nintendo};
    case kLS: return click ? Glyph{ps ? "L3" : "LS", ui::kSilver, true} : Glyph{"L", ui::kSilver, false};
    case kRS: return click ? Glyph{ps ? "R3" : "RS", ui::kSilver, true} : Glyph{"R", ui::kSilver, false};
    default: return {nullptr, ui::kSilver, false};  // the d-pad
  }
}
// Its width with its 1.5 px rim (as the mockup's border-box): a 24 px disc, or a pill 6 px around its text.
float glyphWidth(const Glyph& g) {
  return (g.wide ? std::max(24.0f, ui::capsWidth(g.text, 10, 0.06f) + 0.6f) + 12.0f : 24.0f) + 3.0f;
}
// The glyph: a night disc (a pill when wide) with a silver rim (cyan, haloed, when chosen), its mark inside.
void drawGlyph(const Glyph& g, int spot, int art, float x, float cy, bool on, float a) {
  const float w = glyphWidth(g);
  const Rectangle r{x + 1.5f, cy - 12, w - 3, 24};  // inside the rim (raylib strokes outside the box)
  const float round = g.wide ? 9.0f / 24.0f : 1.0f;   // the pill: 4.5 px corners inside, 6 outside
  if (on) DrawRectangleRounded({x - 3, cy - 16.5f, w + 6, 33}, g.wide ? 18.0f / 33.0f : 1.0f, 12, withAlpha(ui::kBlue, 0.18f * a));
  DrawRectangleRounded(r, round, 12, withAlpha(ui::kNight, 0.85f * a));
  DrawRectangleRoundedLinesEx(r, round, 12, 1.5f, withAlpha(on ? ui::kBlue : ui::kSilver, (on ? 1.0f : 0.7f) * a));
  const Vector2 c{x + w * 0.5f, cy};
  const Color ink = withAlpha(g.col.r == ui::kSilver.r && g.col.g == ui::kSilver.g && on ? ui::kBlue : g.col, a);
  if (g.text) {
    const float size = g.wide ? 10.0f : 13.0f, ls = g.wide ? 0.06f : 0.0f;
    ui::caps(g.text, c.x - (ui::capsWidth(g.text, size, ls) + size * ls) * 0.5f, cy, size, ls, ink);
    return;
  }
  const float t = 1.6f;
  if (spot == kDP) {  // the d-pad: a small cross
    DrawRectangleRec({c.x - 1.6f, c.y - 6, 3.2f, 12}, ink);
    DrawRectangleRec({c.x - 6, c.y - 1.6f, 12, 3.2f}, ink);
  } else if (spot == kFA) {  // ✕
    DrawLineEx({c.x - 4.5f, c.y - 4.5f}, {c.x + 4.5f, c.y + 4.5f}, t, ink);
    DrawLineEx({c.x - 4.5f, c.y + 4.5f}, {c.x + 4.5f, c.y - 4.5f}, t, ink);
  } else if (spot == kFB) {  // ○
    DrawRing(c, 4.6f - t * 0.5f, 4.6f + t * 0.5f, 0, 360, 32, ink);
  } else if (spot == kFX) {  // □
    DrawRectangleLinesEx({c.x - 4.6f, c.y - 4.6f, 9.2f, 9.2f}, t, ink);
  } else if (spot == kFY) {  // △
    const Vector2 p[3] = {{c.x, c.y - 5.6f}, {c.x + 5.4f, c.y + 4.0f}, {c.x - 5.4f, c.y + 4.0f}};
    for (int i = 0; i < 3; ++i) DrawLineEx(p[i], p[(i + 1) % 3], t, ink);
  }
  (void)art;
}

// A label of the drawing: an action (or what never changes) wired to its button, on the left or right of the pad.
struct PadLabel {
  int id;              // action * 2 + slot; 100 the left stick, 101 the d-pad, 102 the right stick, 103 Start
  int action, slot;    // the binding (-1: none)
  int spot;
  bool click;          // a stick pressed in (L3 / R3), not the stick
  const char* name;
  bool right;
  Vector2 b;           // the button's centre on the screen
  float br;            // and its radius
  float y;             // the label's centre line
  Glyph g;
  Rectangle rect;      // what the mouse can point at
};
std::vector<PadLabel> padLabels(int pad, int art, bool nintendo) {
  std::vector<PadLabel> all;
  auto add = [&](int id, int action, int slot, int spot, bool click, const char* name) {
    PadLabel l;
    l.id = id, l.action = action, l.slot = slot, l.spot = spot, l.click = click, l.name = name;
    l.right = spotRight(spot);
    const float* p = kSpot[art][spot];
    l.b = {kPadOX + p[0] * kPadK, kPadOY + p[1] * kPadK};
    l.br = p[2] * kPadK;
    l.g = glyphOf(spot, click, art, nintendo);
    all.push_back(l);
  };
  add(100, -1, -1, kLS, false, "Déplacement");
  add(101, -1, -1, kDP, false, "Déplacement · flèches");
  add(102, kActSmash, -1, kRS, false, "Attaque smash");
  add(103, -1, -1, kST, false, "Pause");
  for (int a = kActJump; a < kActionCount; ++a)
    for (int sl = 0; sl < 2; ++sl) {
      const int b = boundButton(pad, a, sl), spot = spotOf(b);
      if (spot >= 0) add(a * 2 + sl, a, sl, spot, b == kPadL3 || b == kPadR3, actionName(a));
    }
  // Each side: the labels as close as they can to their button's height, never closer than 36 px, inside the frame
  constexpr float kGap = 36.0f, kBottom = kPadAY + 318.0f;
  for (int side = 0; side < 2; ++side) {
    std::vector<PadLabel*> col;
    for (PadLabel& l : all)
      if (l.right == (side == 1)) col.push_back(&l);
    std::stable_sort(col.begin(), col.end(), [](const PadLabel* u, const PadLabel* v) { return u->b.y < v->b.y; });
    float y = -1e9f;
    for (PadLabel* l : col) y = l->y = std::max(l->b.y, y + kGap);
    if (y > kBottom)
      for (PadLabel* l : col) l->y -= y - kBottom;
  }
  for (PadLabel& l : all) {  // where the label sits: right of x 1104, or left of x 684
    const float w = glyphWidth(l.g) + 10.0f + textWidth(l.name, 14, kB);
    l.rect = {l.right ? kX + 556.0f : kX + 136.0f - w, l.y - 15.0f, w, 30.0f};
  }
  std::stable_sort(all.begin(), all.end(), [](const PadLabel& u, const PadLabel& v) {
    return u.right != v.right ? !u.right : u.y < v.y;  // the left column from the top, then the right one
  });
  return all;
}
int labelIndex(const std::vector<PadLabel>& L, int id) {
  for (size_t i = 0; i < L.size(); ++i)
    if (L[i].id == id) return int(i);
  return -1;
}

// What the gamepad's chosen label can do: a fixed one says why it never changes.
const char* fixedNote(int id) {
  if (id == 100 || id == 101) return "Toujours le stick gauche ou la croix : ne se change pas.";
  if (id == 102) return "Le stick droit fait toujours les smashs.";
  return "START met toujours le jeu en pause.";
}
int freeSlot(int action) {
  const auto& P = settings().pad[action];
  return P[0] < 0 ? 0 : P[1] < 0 ? 1 : -1;
}
bool canClear(int action, int slot) {
  const auto& P = settings().pad[action];
  return slot >= 0 && P[slot] >= 0 && (action == kActSmash || P[1 - slot] >= 0);
}
// The hints under the drawing (and what a click on them does): 0 change, 1 add a button, 2 clear it.
struct PadHint {
  const char* key;
  ui::Btn pad;
  const char* label;
  int what;
};
std::vector<PadHint> padHints(const PadLabel& l) {
  std::vector<PadHint> h;
  if (l.slot >= 0) h.push_back({"ENTRÉE", ui::Btn::A, "Changer", 0});
  if (l.action >= 0 && freeSlot(l.action) >= 0) h.push_back({"INSER", ui::Btn::Y, "Ajouter un bouton", 1});
  if (l.action >= 0 && canClear(l.action, l.slot)) h.push_back({"SUPPR", ui::Btn::X, "Effacer", 2});
  return h;
}
float hintKeyWidth(const PadHint& h) { return padPrompts() ? ui::padGlyphWidth(h.pad, 22, promptFamily()) : ui::keyCapWidth(h.key, 10); }
float hintWidth(const PadHint& h) { return hintKeyWidth(h) + 8.0f + textWidth(h.label, 13, kB); }
Rectangle hintRect(const std::vector<PadHint>& hs, int i) {
  float x = kX;
  for (int k = 0; k < i; ++k) x += hintWidth(hs[k]) + 22.0f;
  return {x, 606, hintWidth(hs[i]), 22};
}
}  // namespace

int App::settingsRowCount() const {
  if (setTab_ < kKeysTab) return kRowCount[setTab_];
  return setDevice_ == 2 ? 4 : 8;  // the gamepad: device, style, labels, reset; a keyboard: device, 6 rows, reset
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
  const RowDef& d = kRows[setTab_][row];
  const int key = setTab_ * 10 + row;
  switch (d.kind) {
    case Kind::Slider: {
      int& v = sliderOf(key);
      v = std::clamp(v + delta * 5, 0, 100);
      break;
    }
    case Kind::Toggle: {
      const int n = optCount(d);
      setChoice(key, ((choiceOf(key) + delta) % n + n) % n);
      break;
    }
    case Kind::Select:
      if (key == 11) {
        const std::vector<Res> list = resolutions();
        const int n = int(list.size()), i = ((resolutionIndex(list) + delta) % n + n) % n;
        s.resW = list[i].w;
        s.resH = list[i].h;
      } else if (key == 20) {  // a preset sets the four rows under it; from « Perso », the first or the last
        const int p = presetOf(s), q = p == 4 ? (delta > 0 ? 0 : 3) : ((p + delta) % 4 + 4) % 4;
        s.highQuality = kPresets[q].high;
        s.rtx = kPresets[q].rtx;
        s.upscale = kPresets[q].upscale;
        s.antiAlias = kPresets[q].aa;
      } else {
        s.upscale = ((s.upscale + delta) % 4 + 4) % 4;
      }
      break;
    case Kind::Number:
      if (key == 30) s.stocks = 1 + ((s.stocks - 1 + delta) % 5 + 5) % 5;
      else s.cpuLevel = 1 + ((s.cpuLevel - 1 + delta) % kMaxCpuLevel + kMaxCpuLevel) % kMaxCpuLevel;
      break;
    case Kind::Button:
      if (key == 3) {
        audio_.play(Sfx::HitHeavy, 1.0f);
        audio_.play(Sfx::Crowd, 0.8f);
        soundTestAt_ = frames_;
      } else {
        s = Settings();
        toast("Paramètres par défaut rétablis");
      }
      break;
  }
  applyDisplaySettings();
  saveSettings();
  if (key != 3) audio_.play(Sfx::MenuMove, 0.8f);
}

void App::updateSettings(const MenuNav& nav) {
  Settings& s = settings();
  if (setDevice_ < 0) setDevice_ = padPrompts() ? 2 : 0;
  const bool padView = setTab_ == kKeysTab && setDevice_ == 2;

  if (waitingPad_) {  // a new gamepad button: the next one pressed on any pad (START or Échap cancels)
    if (!padView || IsKeyPressed(KEY_ESCAPE) || padAnyPressed(kPadStart) || bindAction_ < kActJump ||
        bindAction_ >= kActionCount) {
      waitingPad_ = false;
      audio_.play(Sfx::MenuBack);
      return;
    }
    for (int p = 0; p < kMaxPads; ++p)
      for (int b = 0; b < kPadButtonCount; ++b)
        if (padBindable(b) && padPressed(p, b)) {
          waitingPad_ = false;
          const bool ok = bindPadButton(bindAction_, bindSlot_, b);
          if (ok) padFocus_ = bindAction_ * 2 + bindSlot_;  // the label follows its action
          audio_.play(ok ? Sfx::MenuSelect : Sfx::MenuBack);
          return;
        }
    return;
  }
  if (waitingKey_) {  // a new key (Échap, or B on a gamepad, cancels)
    if (setTab_ != kKeysTab || setDevice_ > 1 || padAnyPressed(kPadB) || bindAction_ < 0 || bindAction_ >= kActionCount) {
      waitingKey_ = false;
      audio_.play(Sfx::MenuBack);
      return;
    }
    for (int key = GetKeyPressed(); key != 0; key = GetKeyPressed()) {
      if (key == KEY_ESCAPE) {
        waitingKey_ = false;
        audio_.play(Sfx::MenuBack);
        break;
      }
      if (!keyBindable(key)) continue;
      int& slot = s.keys[setDevice_][bindAction_];
      for (int kb = 0; kb < 2; ++kb)  // the key was used elsewhere: swap
        for (int a = 0; a < kActionCount; ++a)
          if (s.keys[kb][a] == key && !(kb == setDevice_ && a == bindAction_)) s.keys[kb][a] = slot;
      slot = key;
      waitingKey_ = false;
      saveSettings();
      audio_.play(Sfx::MenuSelect);
      break;
    }
    return;
  }

  const int before = setTab_ * 1000 + setRow_ * 10 + setCol_ + (padView ? padFocus_ * 100000 : 0);
  // The tabs: Q / E (A / E on AZERTY: the keys around Up), LB / RB, or a click
  auto openTab = [&](int t) {
    if (t == setTab_) return;
    setTabFrom_ = setTab_;
    setTab_ = t;
    setTabAt_ = screenFrames_;
    setRow_ = 0;
    setCol_ = 0;
  };
  if (IsKeyPressed(KEY_Q) || padAnyPressed(kPadLB)) openTab((setTab_ + 4) % 5);
  if (IsKeyPressed(KEY_E) || padAnyPressed(kPadRB)) openTab((setTab_ + 1) % 5);
  for (int i = 0; i < 5; ++i)
    if (ui::clicked(tabRect(i, setTab_))) openTab(i);
  setRow_ = std::clamp(setRow_, 0, settingsRowCount() - 1);

  if (setTab_ < kKeysTab) {
    const int rows = kRowCount[setTab_];
    if (nav.up) setRow_ = (setRow_ + rows - 1) % rows;
    if (nav.down) setRow_ = (setRow_ + 1) % rows;
    if (nav.left || nav.right) changeSetting(setRow_, nav.left ? -1 : 1);
    if (nav.confirm) changeSetting(setRow_, 1);
    // The mouse: a row under it is chosen; its widget clicked or dragged
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT) && dragSlider_ >= 0) {
      dragSlider_ = -1;
      saveSettings();
    }
    const bool pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    for (int r = 0; r < rows; ++r) {
      const RowDef& d = kRows[setTab_][r];
      const int key = setTab_ * 10 + r;
      const float cy = rowCy(r);
      if (ui::hovered(rowRect(r)) && ui::mouseMoved() && dragSlider_ < 0) setRow_ = r;
      switch (d.kind) {
        case Kind::Slider:
          if (pressed && ui::hovered({sliderTrack(cy).x - 6, cy - 12, sliderTrack(cy).width + 12, 24})) dragSlider_ = r;
          if (dragSlider_ == r) {
            const Rectangle tr = sliderTrack(cy);
            const int v = int(std::lround(clampf((GetMousePosition().x - tr.x) / tr.width, 0.0f, 1.0f) * 100.0f));
            if (v != sliderOf(key)) {
              sliderOf(key) = v;
              applyDisplaySettings();
            }
            setRow_ = r;
          } else if (ui::clicked(sliderPrev(cy)) || ui::clicked(sliderNext(cy))) {
            setRow_ = r;
            changeSetting(r, ui::clicked(sliderPrev(cy)) ? -1 : 1);
          }
          break;
        case Kind::Toggle: {
          const int n = optCount(d);
          for (int k = 0; k < n; ++k)
            if (ui::clicked(togSeg(d.opts, n, cy, k))) {
              setRow_ = r;
              if (k != choiceOf(key)) changeSetting(r, k - choiceOf(key));
            }
          break;
        }
        case Kind::Select: {
          const std::string v = selectText(key);
          if (ui::clicked(selPrev(v.c_str(), cy)) || ui::clicked(selNext(v.c_str(), cy))) {
            setRow_ = r;
            changeSetting(r, ui::clicked(selPrev(v.c_str(), cy)) ? -1 : 1);
          }
          break;
        }
        case Kind::Number:
          if (ui::clicked(numPrev(cy)) || ui::clicked(numNext(cy))) {
            setRow_ = r;
            changeSetting(r, ui::clicked(numPrev(cy)) ? -1 : 1);
          }
          break;
        case Kind::Button:
          if (ui::clicked(btnRect(d.opts[0], cy - 17))) {
            setRow_ = r;
            changeSetting(r, 1);
          }
          break;
      }
    }
  } else {
    // ---- Commandes: the device (row 0), then its keys or its buttons, then its reset (the last row)
    const int rows = settingsRowCount(), reset = rows - 1;
    const int pad = shownPad(promptFamily()), art = padArt(pad);
    const bool nintendo = pad >= 0 && padFamily(pad) == PadFamily::Nintendo && settings().padStyle == 0;
    std::vector<PadLabel> labels;
    if (padView) labels = padLabels(pad, art, nintendo);
    if (padView && labelIndex(labels, padFocus_) < 0 && !labels.empty()) padFocus_ = labels.front().id;
    auto side = [&](bool right, bool last) {  // the first or last label of a column
      int found = -1;
      for (size_t i = 0; i < labels.size(); ++i)
        if (labels[i].right == right && (found < 0 || last)) found = int(i);
      return found;
    };
    auto setDevice = [&](int d) {
      setDevice_ = (d + 3) % 3;
      setCol_ = 0;
    };
    auto resetDevice = [&] {
      if (setDevice_ < 2) {
        s.resetKeys(setDevice_);
        toast(setDevice_ == 0 ? "Touches du clavier 1 réinitialisées" : "Touches du clavier 2 réinitialisées");
      } else {
        s.resetPad();
        toast("Boutons de la manette réinitialisés");
      }
      saveSettings();
      audio_.play(Sfx::MenuSelect);
    };
    auto waitKey = [&](int action) {
      bindAction_ = action;
      waitingKey_ = true;
      while (GetKeyPressed() != 0) {}
      audio_.play(Sfx::MenuSelect);
    };
    // A gamepad label: 0 change its button, 1 add one to its action, 2 clear it
    auto padDo = [&](const PadLabel& l, int what) {
      padFocus_ = l.id;
      setRow_ = 2;
      if (what == 0 && l.slot >= 0) {
        bindAction_ = l.action, bindSlot_ = l.slot, waitingPad_ = true;
      } else if (what == 1 && l.action >= 0 && freeSlot(l.action) >= 0) {
        materializePadFallback();
        bindAction_ = l.action, bindSlot_ = freeSlot(l.action), waitingPad_ = true;
      } else if (what == 2 && l.action >= 0 && canClear(l.action, l.slot)) {
        materializePadFallback();
        s.pad[l.action][l.slot] = -1;
        saveSettings();
        audio_.play(Sfx::MenuBack);
        return;
      } else {
        if (what == 1 && l.action >= 0) toast(TextFormat("« %s » a déjà deux boutons", l.name));
        else if (what == 2 && l.action >= 0) toast(TextFormat("« %s » a besoin d'au moins un bouton", l.name));
        else toast(fixedNote(l.id));
        audio_.play(Sfx::MenuBack);
        return;
      }
      audio_.play(Sfx::MenuSelect);
    };
    const int fi = padView ? labelIndex(labels, padFocus_) : -1;

    if (nav.up) {
      if (padView && setRow_ == 2) {
        int prev = -1;
        for (int i = 0; i < fi; ++i)
          if (labels[i].right == labels[fi].right) prev = i;
        if (prev >= 0) padFocus_ = labels[prev].id;
        else setRow_ = 1;
      } else if (padView && setRow_ == reset && !labels.empty()) {
        setRow_ = 2;
        padFocus_ = labels[side(true, true)].id;
      } else {
        setRow_ = std::max(0, setRow_ - 1);
      }
    } else if (nav.down) {
      if (padView && setRow_ == 2) {
        int next = -1;
        for (int i = int(labels.size()) - 1; i > fi; --i)
          if (labels[i].right == labels[fi].right) next = i;
        if (next >= 0) padFocus_ = labels[next].id;
        else setRow_ = reset;
      } else if (padView && setRow_ == 1 && !labels.empty()) {
        setRow_ = 2;
        padFocus_ = labels[side(true, false)].id;
      } else {
        setRow_ = std::min(reset, setRow_ + 1);
      }
    }
    if (nav.left || nav.right) {
      const int d = nav.left ? -1 : 1;
      if (setRow_ == 0) {
        setDevice(setDevice_ + d);
      } else if (padView && setRow_ == 1) {
        s.padStyle = (s.padStyle + d + 4) % 4;
        saveSettings();
      } else if (padView && setRow_ == 2 && fi >= 0) {  // the other column, the label nearest in height
        int best = -1;
        for (size_t i = 0; i < labels.size(); ++i)
          if (labels[i].right != labels[fi].right &&
              (best < 0 || std::fabs(labels[i].y - labels[fi].y) < std::fabs(labels[best].y - labels[fi].y)))
            best = int(i);
        if (best >= 0) padFocus_ = labels[best].id;
      } else if (!padView && setRow_ > 0 && setRow_ < reset) {
        setCol_ = 1 - setCol_;
      }
    }
    if (nav.confirm) {
      if (setRow_ == 0) setRow_ = 1;
      else if (setRow_ == reset) resetDevice();
      else if (!padView) waitKey((setRow_ - 1) * 2 + setCol_);
      else if (setRow_ == 1) s.padStyle = (s.padStyle + 1) % 4, saveSettings();
      else if (fi >= 0) padDo(labels[fi], labels[fi].slot >= 0 ? 0 : 1);
    }
    if (padView && setRow_ == 2 && fi >= 0 && !waitingPad_) {
      if (IsKeyPressed(KEY_INSERT) || padAnyPressed(kPadY)) padDo(labels[fi], 1);
      else if (IsKeyPressed(KEY_DELETE) || padAnyPressed(kPadX)) padDo(labels[fi], 2);
    }

    // The mouse
    for (int i = 0; i < 3; ++i) {
      const Rectangle r{kX + i * (676.0f / 3.0f + 8.0f), 118, 676.0f / 3.0f, 46};
      if (ui::clicked(r)) setDevice(i), setRow_ = 0;
    }
    if (!waitingPad_ && !waitingKey_) {
      const char* resetLabel = "RÉINITIALISER CE PÉRIPHÉRIQUE";
      if (ui::clicked(btnRect(resetLabel, 600))) setRow_ = reset, resetDevice();
      if (setDevice_ < 2) {
        for (int r = 0; r < 6; ++r)
          for (int c = 0; c < 2; ++c) {
            const Rectangle k{kX + c * 364.0f, 196 + r * 44.0f, 328, 44};
            if (ui::hovered(k) && ui::mouseMoved()) setRow_ = r + 1, setCol_ = c;
            if (ui::clicked(k)) {
              setRow_ = r + 1, setCol_ = c;
              waitKey(r * 2 + c);
            }
          }
      } else {
        static const char* kStyles[4] = {"AUTO", "XBOX", "PS4", "PS5"};
        for (int k = 0; k < 4; ++k)
          if (ui::clicked(togSeg(kStyles, 4, 197, k))) {
            setRow_ = 1;
            if (s.padStyle != k) s.padStyle = k, saveSettings();
          }
        for (const PadLabel& l : labels)  // a click: that button, waiting for its new one (the mouse only points)
          if (ui::clicked(l.rect)) padDo(l, l.slot >= 0 ? 0 : 1);
        const int f = labelIndex(labels, padFocus_);
        if (setRow_ == 2 && f >= 0 && !waitingPad_) {
          const std::vector<PadHint> hs = padHints(labels[f]);
          for (size_t i = 0; i < hs.size(); ++i)
            if (ui::clicked(hintRect(hs, int(i)))) padDo(labels[f], hs[i].what);
        }
      }
    }
  }
  const int after = setTab_ * 1000 + setRow_ * 10 + setCol_ + (setTab_ == kKeysTab && setDevice_ == 2 ? padFocus_ * 100000 : 0);
  if (after != before) audio_.play(Sfx::MenuMove, 0.6f);

  if (nav.back || ui::clicked(closeRect())) {
    saveSettings();
    audio_.play(Sfx::MenuBack);
    toast("Paramètres enregistrés");
    if (match_ && settingsReturn_ == Screen::Match) match_->setHitboxes(s.showHitboxes);  // the paused fight too
    goTo(settingsReturn_);
  }
}

void App::drawSettings() {
  const Settings& s = settings();
  const float sec = screenFrames_ / 60.0f;
  int since = screenFrames_ - setTabAt_;  // the tab's own time: its rows come in again when it changes
  if (since < 0 || since > screenFrames_) since = screenFrames_;
  const float ct = since / 60.0f;
  auto inL = [&](float delay) { return ui::seg(sec, delay, delay + 0.34f); };  // .ps-left
  auto inR = [&](float delay) { return ui::seg(ct, delay, delay + 0.34f); };   // .ps-in
  ClearBackground(ui::kNight);

  // The drawer: the night raised, cut at the 12° blade, sliding in
  {
    const float dx = 620.0f * (1.0f - ui::seg(sec, 0.0f, 0.45f));
    ui::lean(500 + dx, 1500 + dx, ui::kNightUp);
    ui::lean(496 + dx, 500 + dx, ui::kSilver);
  }

  // Left: the brand and the tabs, as on the main menu
  {
    float a = inL(0.0f);
    ui::caps("CLASH HORIZON", 58 - 48.0f * (1.0f - a), 45.9f, 11, 0.3f, withAlpha(ui::kMist, a));
    a = inL(0.05f);
    ui::caps("PARAMÈTRES", 58 - 48.0f * (1.0f - a), 65.9f, 11, 0.3f, withAlpha(ui::kBlue, a));
  }
  {  // the cyan mark slides to the chosen tab
    const float k = ui::seg(ct, 0.0f, 0.25f);
    const float y = 154.0f + 62.0f * (setTabFrom_ + (setTab_ - setTabFrom_) * k);
    skewed({24, y, 9, 66}, withAlpha(ui::kBlue, inL(0.04f * setTab_)));
  }
  for (int i = 0; i < 5; ++i) {
    const float a = inL(0.04f * i), dx = -48.0f * (1.0f - a);
    const TabBox b = tabBox(i, setTab_);
    const bool on = i == setTab_;
    const char* num = TextFormat("%02d", i + 1);
    ui::caps(num, 56 + dx, b.top + b.h - (on ? 12.0f : 6.0f) - 6.65f, 12, 0.16f, withAlpha(on ? ui::kBlue : ui::kMist, a));
    if (on) {  // it lands a little big, then settles
      const float u = std::min(ct, sec) / 0.45f, k = u < 0.35f ? ui::seg(u, 0.0f, 0.35f) : 1.0f;
      ui::scaled(1.1f - 0.1f * k, b.nameX + dx, b.top + 0.8f * b.h,
                 [&] { ui::big(kTabNames[i], b.nameX + dx, b.top, 76, ui::Face::Cond, withAlpha(kWhite, k * a)); });
    } else {
      const bool hov = ui::hovered(tabRect(i, setTab_));
      ui::bigHollow(kTabNames[i], b.nameX + dx, b.top, 46, ui::Face::Cond, 1.3f, withAlpha(hov ? ui::kSilver : ui::kMist, a));
    }
  }
  {  // the keys of this screen, two by two
    const float a = inL(0.3f), dx = -48.0f * (1.0f - a);
    const bool pad = padPrompts();
    struct Cell {
      const char* k1;
      const char* k2;
      ui::Btn b1, b2;
      const char* label;
    };
    const std::string q = upperFr(keyName(KEY_Q)), e = upperFr(keyName(KEY_E));
    const Cell cells[4] = {{q.c_str(), e.c_str(), ui::Btn::LB, ui::Btn::RB, "Onglet"},
                           {"← →", nullptr, ui::Btn::DPadLR, ui::Btn::None, "Changer"},
                           {"↑ ↓", nullptr, ui::Btn::DPadUD, ui::Btn::None, "Choisir"},
                           {"ÉCHAP", nullptr, ui::Btn::B, ui::Btn::None, "Retour"}};
    auto cellW = [&](const Cell& c) {
      float w = pad ? ui::padGlyphWidth(c.b1, 22, promptFamily()) : ui::keyCapWidth(c.k1, 10);
      if (c.k2) w += 8.0f + (pad ? ui::padGlyphWidth(c.b2, 22, promptFamily()) : ui::keyCapWidth(c.k2, 10));
      return w + 8.0f + textWidth(c.label, 12, kB);
    };
    const float col2 = 58 + std::max(cellW(cells[0]), cellW(cells[1])) + 22.0f;
    for (int i = 0; i < 4; ++i) {  // the grid fills row by row: Onglet | Choisir, Changer | Retour
      const Cell& c = cells[i == 1 ? 2 : i == 2 ? 1 : i];
      float x = (i % 2 ? col2 : 58.0f) + dx;
      const float cy = 622 + 12 + (i / 2) * 34.0f;
      for (int k = 0; k < (c.k2 ? 2 : 1); ++k) {
        const char* key = k ? c.k2 : c.k1;
        const ui::Btn btn = k ? c.b2 : c.b1;
        if (pad) x += ui::padGlyph(btn, x, cy - 11, 22, promptFamily()) + 8.0f;
        else x += ui::keyCap(key, x, cy, 24, 10, 0.4f, a) + 8.0f;
      }
      drawText(c.label, x, capsY(cy, 12, kB), 12, withAlpha(ui::kMist, a), kB);
    }
  }

  // The drawer's head: the tab's number, what it holds, the way back
  {
    float a = inR(0.12f);
    ui::caps(TextFormat("%02d / 05", setTab_ + 1), kX + 40.0f * (1.0f - a), 49.9f, 11, 0.3f, withAlpha(ui::kBlue, a));
    a = inR(0.16f);
    drawText(kTabSub[setTab_], kX + 40.0f * (1.0f - a), 64, 16, withAlpha(ui::kSilver, a), kB);
    const Rectangle cl = closeRect();
    if (ui::hovered(cl)) DrawRectangleRec(cl, withAlpha(ui::kSilver, 0.1f));
    hair(cl, 1.0f);
    ui::icon(ui::Icon::Close, {cl.x + 20, cl.y + 20}, 18, ui::kSilver, 1.8f);
  }

  if (setTab_ < kKeysTab) {
    // ---- The rows, as in « Règles & mode »
    const float a0 = inR(0.18f), dx = 40.0f * (1.0f - a0);
    rlPushMatrix();
    rlTranslatef(dx, 0, 0);
    for (int r = 0; r < kRowCount[setTab_]; ++r) {
      const RowDef& d = kRows[setTab_][r];
      const int key = setTab_ * 10 + r;
      const Rectangle rr = rowRect(r);
      const float cy = rowCy(r);
      const bool on = r == setRow_, dim = key == 25 && s.upscale == 0;  // the sharpness needs FSR 1
      const float a = a0 * (dim ? 0.38f : 1.0f);
      if (on) {
        DrawRectangleRec({kX - 12, rr.y, rr.width + 24, rr.height - 1}, withAlpha(Color{238, 242, 250, 255}, 0.045f * a0));
        skewed({kX - 26, rr.y + 9, 6, 34}, withAlpha(ui::kBlue, a0));
      }
      DrawRectangleRec({rr.x, rr.y + rr.height - 1, rr.width, 1}, withAlpha(kRim, 0.12f * a0));
      ui::big(upperFr(d.title).c_str(), rr.x, rr.y + 6.65f, 24, ui::Face::Cond, withAlpha(on ? kWhite : ui::kMist, a), 0.84f, 0.04f);
      const char* sub = key == 25 && s.upscale == 0 ? "Seulement avec FSR 1" : d.sub;
      drawText(sub, rr.x, rr.y + 30.81f, 12, withAlpha(on ? ui::kMist : ui::kDim, a), kB);
      switch (d.kind) {
        case Kind::Slider: {  // < the track, its cyan fill and its silver head > the value
          const int v = sliderOf(key);
          chevron(sliderPrev(cy), -1, 16, a);
          chevron(sliderNext(cy), 1, 16, a);
          const Rectangle tr = sliderTrack(cy);
          DrawRectangleRec(tr, withAlpha(kRim, 0.16f * a));
          DrawRectangleRec({tr.x, tr.y, tr.width * v / 100.0f, tr.height}, withAlpha(ui::kBlue, a));
          skewed({tr.x + tr.width * v / 100.0f - 2, tr.y - 7, 4, 17}, withAlpha(ui::kSilver, a));
          const char* txt = TextFormat("%d", v);
          ui::exo(txt, kR - ui::exoWidth(txt, 26), cy + 10.37f, 26, withAlpha(ui::kSilver, a));
          break;
        }
        case Kind::Toggle:
          drawToggle(d.opts, optCount(d), choiceOf(key), cy, a);
          break;
        case Kind::Select: {
          const std::string v = selectText(key);
          hair(selBox(v.c_str(), cy), a);
          chevron(selPrev(v.c_str(), cy), -1, 14, a);
          chevron(selNext(v.c_str(), cy), 1, 14, a);
          const Rectangle b = selBox(v.c_str(), cy);
          ui::caps(v.c_str(), b.x + (b.width - ui::capsWidth(v.c_str(), 10, 0.12f) - 1.2f) * 0.5f, cy, 10, 0.12f,
                   withAlpha(ui::kSilver, a));
          break;
        }
        case Kind::Number: {
          chevron(numPrev(cy), -1, 16, a);
          chevron(numNext(cy), 1, 16, a);
          const char* txt = TextFormat("%d", key == 30 ? s.stocks : s.cpuLevel);
          ui::exo(txt, kR - 60 - ui::exoWidth(txt, 26) * 0.5f, cy + 10.37f, 26, withAlpha(ui::kSilver, a));
          break;
        }
        case Kind::Button: {
          const bool playing = key == 3 && frames_ - soundTestAt_ < 96;
          drawButton(btnRect(d.opts[0], cy - 17), playing ? "LECTURE…" : d.opts[0], a);
          break;
        }
      }
    }
    rlPopMatrix();

    // The help of the chosen row (and, for the display and the graphics, what the fight is rendered at)
    const float a = inR(0.24f), hx = kX + 40.0f * (1.0f - a);
    const RowDef& d = kRows[setTab_][std::clamp(setRow_, 0, kRowCount[setTab_] - 1)];
    DrawRectangleRec({hx, 560, kR - kX, 1}, withAlpha(kRim, 0.2f * a));
    ui::big(upperFr(d.title).c_str(), hx, 577, 22, ui::Face::Cond, withAlpha(ui::kSilver, a), 0.84f, 0.04f);
    if (setTab_ == 1 || setTab_ == 2) {  // RENDU 1280 × 720 → ÉCRAN 1920 × 1080, right-aligned on the title's baseline
      const float base = 577 + 0.754f * 22;
      const char* render = TextFormat("%d × %d", target_.texture.width, target_.texture.height);
      const std::string screen = TextFormat("%d × %d", GetRenderWidth(), GetRenderHeight());
      const char* to = s.fullscreen ? "→ ÉCRAN" : "→ FENÊTRE";
      float x = kR + 40.0f * (1.0f - a) - ui::exoWidth(screen.c_str(), 17);
      ui::exo(screen.c_str(), x, base, 17, withAlpha(ui::kSilver, a));
      x -= 8 + ui::capsWidth(to, 10, 0.24f) + 2.4f;
      ui::caps(to, x, base - 3.43f, 10, 0.24f, withAlpha(ui::kMist, a));
      x -= 8 + ui::exoWidth(render, 17);
      ui::exo(render, x, base, 17, withAlpha(ui::kSilver, a));
      x -= 8 + ui::capsWidth("RENDU", 10, 0.24f) + 2.4f;
      ui::caps("RENDU", x, base - 3.43f, 10, 0.24f, withAlpha(ui::kBlue, a));
    }
    float y = 605.0f;
    for (const std::string& line : ui::wrapText(d.help, 680, 14, kB)) {
      drawText(line.c_str(), hx, y + 1.68f, 14, withAlpha(ui::kMist, a), kB);
      y += 20.3f;
    }
    return;
  }

  // ---- Commandes: the device first, then only its keys
  const int dev = std::clamp(setDevice_ < 0 ? (padPrompts() ? 2 : 0) : setDevice_, 0, 2);
  const int reset = dev == 2 ? 3 : 7;
  {
    const float a = inR(0.18f), dx = 40.0f * (1.0f - a);
    const char* names[3] = {"CLAVIER 1", "CLAVIER 2", "MANETTE"};
    for (int i = 0; i < 3; ++i) {
      const Rectangle r{kX + dx + i * (676.0f / 3.0f + 8.0f), 118, 676.0f / 3.0f, 46};
      const bool on = i == dev;
      if (on) DrawRectangleRec(r, withAlpha(ui::kSilver, a));
      else hair(r, a);
      const Color ink = on ? ui::kNight : ui::hovered(r) ? ui::kSilver : ui::kMist;
      ui::caps(names[i], r.x + (r.width - ui::capsWidth(names[i], 11, 0.12f) - 1.32f) * 0.5f, r.y + 23, 11, 0.12f,
               withAlpha(ink, a));
      if (on && setRow_ == 0 && !waitingKey_ && !waitingPad_) ring(r);
    }
  }
  if (dev < 2) {
    // A keyboard: its twelve actions on two columns, each with its key
    const float a = inR(0.22f), dx = 40.0f * (1.0f - a);
    for (int act = 0; act < kActionCount; ++act) {
      const int r = act / 2, c = act % 2;
      const Rectangle k{kX + dx + c * 364.0f, 196 + r * 44.0f, 328, 44};
      const bool on = setRow_ == r + 1 && setCol_ == c, wait = waitingKey_ && bindAction_ == act;
      if (on || wait) {
        DrawRectangleRec(k, withAlpha(Color{238, 242, 250, 255}, 0.045f * a));
        DrawRectangleRec({k.x, k.y + k.height - 1, k.width, 1}, withAlpha(ui::kBlue, a));
      } else {
        DrawRectangleRec({k.x, k.y + k.height - 1, k.width, 1}, withAlpha(kRim, 0.12f * a));
      }
      const float cy = k.y + 22;
      drawText(actionName(act), k.x, capsY(cy, 15, kB), 15, withAlpha(on || wait ? kWhite : ui::kMist, a), kB);
      const std::string name = wait ? "…" : upperFr(keyName(s.keys[dev][act]));
      const float w = ui::keyCapWidth(name.c_str(), 11);
      ui::keyCap(name.c_str(), k.x + k.width - w, cy, 30, 11, 0.55f, a);
    }
    const float b = inR(0.26f);
    float y = 476;
    std::string note = "Attaque + Spéciale = smash · Bas, Bas-avant, Avant + Attaque = smash avant chargé · Bas, Haut + "
                       "Attaque = smash haut chargé.";
    if (dev == 1) note += " Le pavé numérique marche aussi (0 à 7).";
    for (const std::string& line : ui::wrapText(note, kR - kX, 13, kB)) {
      drawText(line.c_str(), kX + 40.0f * (1.0f - b), y, 13, withAlpha(ui::kDim, b), kB);
      y += 19.0f;
    }
  } else {
    // The gamepad: detected or forced, its drawing, each button wired to its action
    const int pad = shownPad(promptFamily()), art = padArt(pad);
    const bool nintendo = pad >= 0 && padFamily(pad) == PadFamily::Nintendo && s.padStyle == 0;
    {
      const float a = inR(0.2f), dx = 40.0f * (1.0f - a);
      static const char* kStyles[4] = {"AUTO", "XBOX", "PS4", "PS5"};
      static const char* kForced[3] = {"Manette Xbox", "DualShock 4 · PS4", "DualSense · PS5"};
      const char* tag = s.padStyle > 0 ? "AFFICHAGE FORCÉ" : pad >= 0 ? "DÉTECTÉE" : "AUCUNE MANETTE";
      std::string name = s.padStyle > 0 ? kForced[art] : pad >= 0 ? padModel(pad) : "Branche-la : le jeu la reconnaît";
      if (s.padStyle == 0 && pad >= 0 && art > 0) name += art == 2 ? " · PS5" : " · PS4";
      ui::caps(tag, kX + dx, 197, 10, 0.24f, withAlpha(pad >= 0 || s.padStyle > 0 ? ui::kBlue : ui::kMist, a));
      drawText(name.c_str(), kX + dx + ui::capsWidth(tag, 10, 0.24f) + 2.4f + 10, capsY(197, 14, kB), 14,
               withAlpha(ui::kSilver, a), kB);
      rlPushMatrix();
      rlTranslatef(dx, 0, 0);
      drawToggle(kStyles, 4, s.padStyle, 197, a);
      if (setRow_ == 1 && !waitingPad_) ring(togBox(kStyles, 4, 197));
      rlPopMatrix();
    }
    const float a = inR(0.24f), dx = 40.0f * (1.0f - a);
    const std::vector<PadLabel> labels = padLabels(pad, art, nintendo);
    const int focus = setRow_ == 2 || waitingPad_ ? padFocus_ : -1;
    rlPushMatrix();
    rlTranslatef(dx, 0, 0);
    for (const PadLabel& l : labels) {  // the wires: along to the button's column, then up or down to its edge
      const bool on = l.id == focus;
      const Color c = on ? withAlpha(ui::kBlue, a) : withAlpha(kRim, 0.42f * a);
      const float th = on ? 1.5f : 1.0f, lx = l.right ? kX + 548.0f : kX + 144.0f;
      if (std::fabs(l.y - l.b.y) < 3.0f) {
        DrawLineEx({lx, l.y}, {l.right ? l.b.x + l.br + 3 : l.b.x - l.br - 3, l.y}, th, c);
      } else {
        const float ex = l.b.x + (l.right ? 1.0f : -1.0f) * std::min(l.br * 0.35f, 6.0f);
        const float ey = l.y < l.b.y ? l.b.y - l.br - 3 : l.b.y + l.br + 3;
        DrawLineEx({lx, l.y}, {ex, l.y}, th, c);
        DrawLineEx({ex, l.y}, {ex, ey}, th, c);
      }
    }
    art::drawPicture(kPadPicture[art], {0, 0, 1376, 960}, {kPadOX - 12 * kPadK, kPadOY, 344 * kPadK, 240 * kPadK}, a);
    for (const PadLabel& l : labels) {
      if (l.id != focus) continue;  // the chosen button lit on the drawing
      const float r = l.br + 2 * kPadK;
      DrawCircleV(l.b, r, withAlpha(ui::kBlue, 0.35f * a));
      DrawRing(l.b, r - 1.1f, r + 1.1f, 0, 360, 48, withAlpha(ui::kBlue, a));
    }
    for (const PadLabel& l : labels) {
      const bool on = l.id == focus, hov = ui::hovered(l.rect) && !on;
      const Color ink = withAlpha(on ? ui::kBlue : hov ? kWhite : ui::kSilver, a);
      const float gw = glyphWidth(l.g);
      if (l.right) {
        drawGlyph(l.g, l.spot, art, l.rect.x, l.y, on, a);
        drawText(l.name, l.rect.x + gw + 10, capsY(l.y, 14, kB), 14, ink, kB);
      } else {
        drawText(l.name, l.rect.x, capsY(l.y, 14, kB), 14, ink, kB);
        drawGlyph(l.g, l.spot, art, l.rect.x + l.rect.width - gw, l.y, on, a);
      }
    }
    rlPopMatrix();
  }

  // Under it: what to do (or the key or button being waited for), and the reset of this device
  if (waitingKey_ || waitingPad_) {
    const float x0 = kX, y0 = 596;
    ui::quad({x0 + 10, y0}, {kR, y0}, {kR - 10, y0 + 44}, {x0, y0 + 44}, ui::kSilver, ui::kSilver, ui::kSilver, ui::kSilver);
    const std::string what = upperFr(actionName(std::clamp(bindAction_, 0, kActionCount - 1)));
    float x = x0 + 22;
    x += ui::caps(what.c_str(), x, y0 + 22, 11, 0.2f, ui::kNight) + 12;
    const char* msg = waitingPad_ ? "Appuie sur un bouton de la manette · START annule · déjà pris : les deux actions "
                                    "s'échangent"
                                  : "Appuie sur une touche du clavier · ÉCHAP annule · déjà prise : les deux actions "
                                    "s'échangent";
    const std::vector<std::string> lines = ui::wrapText(msg, kR - 22 - x, 13, kB);  // two lines for a long name
    float ty = y0 + 22 - lines.size() * 15.74f * 0.5f;
    for (const std::string& line : lines) {
      drawText(line.c_str(), x, ty, 13, ui::kNight, kB);
      ty += 15.74f;
    }
    return;
  }
  const float a = inR(0.3f), dx = 40.0f * (1.0f - a);
  const char* resetLabel = "RÉINITIALISER CE PÉRIPHÉRIQUE";
  Rectangle rb = btnRect(resetLabel, 600);
  rb.x += dx;
  drawButton(rb, resetLabel, a);
  if (setRow_ == reset) ring(rb);
  const int pad = shownPad(promptFamily());
  std::vector<PadLabel> labels;
  if (dev == 2) labels = padLabels(pad, padArt(pad), pad >= 0 && padFamily(pad) == PadFamily::Nintendo && s.padStyle == 0);
  const int f = dev == 2 && setRow_ == 2 ? labelIndex(labels, padFocus_) : -1;
  if (f < 0) {
    const char* tip = dev == 2 ? "Choisis une action, puis appuie sur son nouveau bouton."
                               : "Choisis une action, puis appuie sur sa nouvelle touche.";
    drawText(tip, kX + dx, capsY(617, 13, kB), 13, withAlpha(ui::kMist, a), kB);
    return;
  }
  const std::vector<PadHint> hs = padHints(labels[f]);  // the chosen button: what it can do, clickable
  for (size_t i = 0; i < hs.size(); ++i) {
    const Rectangle r = hintRect(hs, int(i));
    if (padPrompts()) ui::padGlyph(hs[i].pad, r.x + dx, r.y, 22, promptFamily());
    else ui::keyCap(hs[i].key, r.x + dx, r.y + 11, 24, 10, 0.4f, a);
    const float x = r.x + dx + hintKeyWidth(hs[i]) + 8.0f;
    drawText(hs[i].label, x, capsY(617, 13, kB), 13, withAlpha(ui::hovered(r) ? kWhite : ui::kSilver, a), kB);
  }
  if (hs.empty() || labels[f].slot < 0) {
    const float x = hs.empty() ? kX : hintRect(hs, int(hs.size()) - 1).x + hintWidth(hs.back()) + 22.0f;
    drawText(fixedNote(labels[f].id), x + dx, capsY(617, 13, kB), 13, withAlpha(ui::kMist, a), kB);
  }
}
