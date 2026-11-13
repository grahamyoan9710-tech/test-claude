// Settings screen (« Prestige »): five tabs (audio, display, graphics, game, controls). The first four are rows of
// sliders, OUI / NON switches, selectors and buttons on the night, with the help of the chosen row beside them and a
// live card: the real volumes, a preview that follows the display settings, the rules of the next fights. The
// controls are one table: both keyboards and the gamepad's two buttons for each action.
#include <cstring>

#include "game/app.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"
#ifdef CLASH_HAS_GLFW  // our own raylib build: its GLFW lists the screen's modes
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#endif

namespace {
constexpr FontKind kT = FontKind::Title, kU = FontKind::Ui, kB = FontKind::Body;
constexpr Color kCard = ui::kNightUp;  // the plates
const char* kTabNames[5] = {"AUDIO", "AFFICHAGE", "GRAPHISMES", "JEU", "COMMANDES"};
constexpr int kKeysTab = 4;
constexpr Color kSoft = ui::kSilver;
constexpr Color kDim = ui::kMist;
constexpr Color kLight = ui::kBlue;
constexpr Color kTrack{20, 28, 62, 255};
constexpr Color kWell = ui::kNight;
constexpr Color kDanger{255, 122, 110, 255};
const Rectangle kPanel{36, 168, 760, 470};       // the rows
const Rectangle kKeysPanel{36, 168, 1208, 470};  // the controls: the whole width

struct RowInfo {
  const char* label;
  const char* help;
};
const RowInfo kRows[4][8] = {
    {{"Volume général", "Volume de tout le jeu."},
     {"Musique", "Volume des musiques des menus et des combats."},
     {"Effets sonores", "Coups, sauts, K.O., cris de la foule..."},
     {"Tester le son", "Joue un coup puissant et les cris de la foule."}},
    {{"Mode d'affichage", "Plein écran : le jeu couvre tout l'écran, sans changer la résolution de l'écran (aucune bande noire). Fenêtré : une fenêtre de la taille choisie. Touche F11."},
     {"Résolution", "Les résolutions de ton écran. En plein écran, c'est la résolution de rendu : plus basse, l'image est un peu moins fine mais le jeu tourne plus vite, et elle remplit toujours tout l'écran. En fenêtré, c'est la taille de la fenêtre."},
     {"Synchronisation verticale", "V-Sync : les images sont calées sur l'écran, sans déchirure. Sans, la latence baisse un peu mais l'image peut se déchirer."},
     {"Secousses de l'écran", "Tremblements de la caméra lors des gros coups et des K.O."},
     {"Noms au-dessus des joueurs", "Affiche J1, J2... ou CPU au-dessus de chaque combattant."},
     {"Compteur d'images (FPS)", "Affiche le nombre d'images par seconde en haut à gauche."},
     {"Zones de coup (hitboxes)", "Montre les zones d'attaque pendant les combats (touche F1)."}},
    {{"Qualité graphique", "Haute : persos en pixel art, halo des effets et persos éclairés par eux. Moyenne : rendu simple, bien plus léger."},
     {"RTX", "Haut : vrai lancer de rayons complet (lune ou soleil, vraies ombres, lumière qui rebondit), très gourmand. Moyen : le même en 4 fois moins de pixels et 2 fois moins de rayons, pour les cartes moyennes (RTX 4060...). Non : l'éclairage dessiné, le plus léger."},
     {"Mise à l'échelle", "AMD FSR 1, le vrai (ses deux passes : EASU agrandit l'image en gardant les bords nets, RCAS la rend nette). Le jeu est calculé plus petit puis agrandi à la taille de l'écran : plus de FPS. Qualité : 67 % de la résolution ; Équilibré : 59 % ; Performance : 50 %. Native : aucune mise à l'échelle."},
     {"Anticrénelage", "Lisse les bords en escalier. FXAA : léger, presque gratuit. SSAA ×2 : l'image est calculée deux fois plus grande puis réduite, le plus beau et le plus lourd. Pas de TAA : sur du pixel art, il laisse des traînées floues derrière les persos qui bougent."},
     {"Netteté", "La netteté de FSR 1 (RCAS), comme dans les jeux qui l'ont : 100 = la plus forte, 0 = la plus douce. Ne sert qu'avec la mise à l'échelle FSR."}},
    {{"Vies par défaut", "Nombre de vies de chaque combattant au début d'un match."},
     {"Clash Orb", "L'orbe qui traverse l'arène. La casser donne l'ultime : c'est la seule façon de l'obtenir."},
     {"Armes", "Les deux armes de chaque combattant tombent sur l'arène : il prend la sienne en restant dessus, touche Arme maintenue 1 s. Avec A et B, la touche alterne."},
     {"Niveau des ordinateurs", "Niveau par défaut des combattants contrôlés par l'ordinateur (1 à 10 : le niveau 10 frappe deux fois plus fort)."},
     {"Saut avec Haut", "Haut (stick ou touche) fait sauter. Haut + attaque ou spéciale ensemble reste un coup vers le haut."},
     {"Tout réinitialiser", "Remet tous les paramètres et toutes les touches par défaut."}},
};
const RowInfo kTabHelp[5] = {{"Audio", "Le volume de tout le jeu, de la musique et des effets."},
                             {"Affichage", "L'écran, la résolution et ce qui s'affiche pendant les combats."},
                             {"Graphismes", "La qualité de l'image : l'éclairage, la mise à l'échelle FSR, l'anticrénelage."},
                             {"Jeu", "Les règles par défaut des prochains combats."},
                             {"Commandes", ""}};
const int kRowCounts[5] = {4, 7, 5, 6, kActionCount + 1};  // controls: the actions + the reset row

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

// ---- Where things are -------------------------------------------------------------------
// The tabs: capitals on one line, the open one on a silver plate leaning like the blade.
float tabX(int i) {
  float x = 36;
  for (int k = 0; k < i; ++k) x += ui::capsWidth(kTabNames[k], 13, 0.22f) + 44;
  return x;
}
Rectangle tabRect(int i) { return {tabX(i), 116, ui::capsWidth(kTabNames[i], 13, 0.22f) + 32, 34}; }
Rectangle backRect() { return {1244 - 140, 30, 140, 30}; }
constexpr float kRowH = 54.0f;
Rectangle rowRect(int row) { return {kPanel.x, kPanel.y + row * kRowH, kPanel.width, kRowH}; }  // 8 rows fit
float rowCy(int row) { return kPanel.y + kRowH * 0.5f + row * kRowH; }
// The widgets, right-aligned on x 780
Rectangle minusRect(int row) { return {448, rowCy(row) - 13, 26, 26}; }
Rectangle trackRect(int row) { return {486, rowCy(row) - 7, 200, 14}; }
Rectangle plusRect(int row) { return {698, rowCy(row) - 13, 26, 26}; }
Rectangle toggleRect(int row) { return {660, rowCy(row) - 15, 120, 30}; }
Rectangle prevRect(int row) { return {500, rowCy(row) - 13, 26, 26}; }
Rectangle nextRect(int row) { return {754, rowCy(row) - 13, 26, 26}; }
Rectangle buttonRect(int row) { return {604, rowCy(row) - 15, 176, 30}; }
// The controls table: an action per row; keyboard 1, keyboard 2, the gamepad's button 1 and button 2
constexpr float kTableY = 222.0f, kKeyH = 27.0f;
constexpr float kColX[4] = {350, 528, 730, 904}, kColW[4] = {170, 170, 166, 190};
Rectangle keyRect(int row, int col) { return {kColX[col], kTableY + row * kKeyH + 1.5f, kColW[col], kKeyH - 3.0f}; }
// The reset row under it: keyboard 1, keyboard 2, the gamepad, everything
Rectangle resetRect(int col) {
  const float y = kTableY + kActionCount * kKeyH + 6.0f;
  if (col == 2) return {kColX[2], y, kColX[3] + kColW[3] - kColX[2], 26};
  if (col == 3) return {1110, y, 104, 26};
  return {kColX[col], y, kColW[col], 26};
}

enum class Widget { Slider, Toggle, Selector, Button };
Widget widgetFor(int tab, int row) {
  if (tab == 0) return row < 3 ? Widget::Slider : Widget::Button;
  if (tab == 1) return row <= 1 || row == 3 ? Widget::Selector : Widget::Toggle;
  if (tab == 2) return row == 4 ? Widget::Slider : Widget::Selector;
  if (row == 1 || row == 2 || row == 4) return Widget::Toggle;
  return row == 5 ? Widget::Button : Widget::Selector;
}
int* sliderFor(int tab, int row) {
  Settings& s = settings();
  if (tab == 2) return &s.sharpness;
  return row == 0 ? &s.masterVolume : row == 1 ? &s.musicVolume : &s.sfxVolume;
}
bool toggleOn(int tab, int row) {
  const Settings& s = settings();
  if (tab == 1) return row == 2 ? s.vsync : row == 4 ? s.playerTags : row == 5 ? s.showFps : s.showHitboxes;
  return row == 1 ? s.items : row == 2 ? s.weapons : s.tapJump;
}
std::string selectorText(int tab, int row) {
  const Settings& s = settings();
  static const char* shakes[3] = {"Aucune", "Légères", "Normales"};
  static const char* rtxs[3] = {"Non", "Moyen", "Haut"};
  static const char* ups[4] = {"Native", "FSR Qualité", "FSR Équilibré", "FSR Performance"};
  static const char* aas[3] = {"Non", "FXAA", "SSAA ×2"};
  if (tab == 1) {
    if (row == 0) return s.fullscreen ? "Plein écran" : "Fenêtré";
    if (row == 1) {
      if (s.resW) return TextFormat("%d × %d", s.resW, s.resH);
      const int m = GetCurrentMonitor(), mw = GetMonitorWidth(m), mh = GetMonitorHeight(m);
      if (!s.fullscreen) return TextFormat("Auto · %d × %d", windowW(s, mw), windowH(s, mw, mh));
      return TextFormat("Écran · %d × %d", mw, mh);
    }
    return shakes[std::clamp(s.screenShake, 0, 2)];
  }
  if (tab == 2)
    return row == 0 ? (s.highQuality ? "Haute" : "Moyenne") : row == 1 ? rtxs[std::clamp(s.rtx, 0, 2)]
           : row == 2 ? ups[std::clamp(s.upscale, 0, 3)] : aas[std::clamp(s.antiAlias, 0, 2)];
  if (row == 0) return TextFormat("%d vie%s", s.stocks, s.stocks > 1 ? "s" : "");
  return TextFormat("Niveau %d", s.cpuLevel);
}

// ---- Drawing helpers --------------------------------------------------------------------
// A plate of the charte: a raised night, a faint silver line.
void plate(Rectangle r, Color fill = kCard) {
  DrawRectangleRec(r, fill);
  DrawRectangleLinesEx(r, 1.0f, withAlpha(ui::kSilver, 0.12f));
}
// A parallelogram leaning like the blade (the tabs, the buttons, the chosen half of a switch).
void slant(Rectangle r, Color c, bool outline = false) {
  const float k = r.height * ui::kLean;
  const Vector2 a{r.x + k, r.y}, b{r.x + r.width, r.y}, d{r.x + r.width - k, r.y + r.height}, e{r.x, r.y + r.height};
  if (!outline) {
    ui::quad(a, b, d, e, c, c, c, c);
    return;
  }
  DrawLineEx(a, b, 1.0f, c);
  DrawLineEx(b, d, 1.0f, c);
  DrawLineEx(d, e, 1.0f, c);
  DrawLineEx(e, a, 1.0f, c);
}
// A small square button (−, +, the selector's arrows): silver line, white on hover.
void smallBox(Rectangle r, const char* label, int arrow = 0) {
  const bool hov = ui::hovered(r);
  DrawRectangleLinesEx(r, 1.0f, withAlpha(ui::kSilver, hov ? 0.6f : 0.18f));
  const Vector2 c{r.x + r.width * 0.5f, r.y + r.height * 0.5f};
  const Color ink = hov ? WHITE : ui::kSilver;
  if (arrow) ui::arrowTri(c, 9, arrow, ink);
  else ui::caps(label, c.x - ui::capsWidth(label, 14, 0) * 0.5f, c.y, 14, 0, ink);
}
// The focus of a cell: a silver frame just outside it.
void focusRing(Rectangle r) { DrawRectangleLinesEx(ui::grow(r, 2.0f), 2.0f, ui::kSilver); }
std::vector<std::string> wrapLines(const std::string& text, float width, float size, FontKind f = kB) {
  std::vector<std::string> out;
  std::string line, word;
  for (size_t i = 0; i <= text.size(); ++i) {
    if (i < text.size() && text[i] != ' ') {
      word += text[i];
      continue;
    }
    const std::string next = line.empty() ? word : line + " " + word;
    if (!line.empty() && textWidth(next.c_str(), size, f) > width) {
      out.push_back(line);
      line = word;
    } else {
      line = next;
    }
    word.clear();
  }
  if (!line.empty()) out.push_back(line);
  return out;
}
// The padlock of a binding that never changes.
void lockIcon(Vector2 o, Color c) {
  DrawRectangleLinesEx({o.x, o.y + 5, 9, 6}, 1.4f, c);
  DrawRing({o.x + 4.5f, o.y + 4.5f}, 2.2f, 3.6f, 180, 360, 10, c);
}


// ---- Gamepad controls ------------------------------------------------------------------
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
    ui::glow(pos[hl], r * 2.2f, withAlpha(ui::kBlue, 0.35f + 0.2f * pulse), 6);
    DrawRing(pos[hl], r + 2 + 2 * pulse, r + 5 + 2 * pulse, 0, 360, 40, ui::kBlue);
  }
  rlPopMatrix();
}


// ---- The live cards beside the rows -----------------------------------------------------------
// A card's small title ("VOLUME RÉEL"...), tracked capitals.
void cardTitle(const char* text, float x, float cy, Color c = kDim) { ui::caps(text, x, cy, 11, 0.22f, c); }

// Audio: what each volume really gives (the music and the effects go through the general volume).
float audioCard(float y, bool testing, float t) {
  const Settings& s = settings();
  const Rectangle r{856, y, 360, 287};
  plate(r);
  cardTitle("VOLUME RÉEL", r.x + 18, y + 26);
  const int eff[3] = {s.masterVolume, s.musicVolume * s.masterVolume / 100, s.sfxVolume * s.masterVolume / 100};
  const char* labels[3] = {"Général", "Musique", "Effets"};
  static const float kBar[6] = {0.7f, 1.0f, 0.85f, 0.6f, 0.95f, 0.75f};
  for (int m = 0; m < 3; ++m) {
    const float cx = r.x + 18 + 49.3f + m * 112.7f;
    const Rectangle box{cx - 42, y + 47, 84, 140};
    DrawRectangleRec(box, kWell);
    for (int i = 0; i < 6; ++i) {  // an equaliser dancing at that volume (faster while the test plays)
      const float period = testing ? 0.28f : 0.9f, ph = std::fmod(t + (i * 0.13f + m * 0.07f), period) / period;
      const float k = eff[m] == 0 ? 1.0f : 0.35f + 0.65f * (0.5f - 0.5f * std::cos(ph * 2.0f * kPi));
      const float h = std::max(4.0f, eff[m] * 1.4f * kBar[i]) * k, bx = box.x + 8 + i * 12.0f, by = box.y + box.height;
      ui::quad({bx, by - h}, {bx + 8, by - h}, {bx + 8, by}, {bx, by}, WHITE, WHITE, ui::kBlue, ui::kBlue);
    }
    DrawRectangleRec({box.x, box.y + box.height - eff[m] * 1.4f - 1, box.width, 2}, ui::kBlue);
    const char* v = TextFormat("%d %%", eff[m]);
    drawText(v, cx - textWidth(v, 23.9f, kT) * 0.5f, capsY(y + 205, 23.9f, kT), 23.9f, ui::kSilver, kT);
    drawText(labels[m], cx - textWidth(labels[m], 16.4f, kB) * 0.5f, capsY(y + 228, 16.4f, kB), 16.4f, kDim, kB);
  }
  const char* note = testing ? "Lecture : un coup puissant, puis les cris de la foule."
                             : "Musique et effets sont multipliés par le volume général.";
  drawText(note, r.x + 18, capsY(y + 262, 16.4f, kB), 16.4f, kDim, kB);
  return r.y + r.height;
}

// Display: a little fight that follows the settings (window, quality, shaking, names, FPS, hitboxes).
float previewCard(float y, int stage, float t, const char* render) {
  const Settings& s = settings();
  const float bar = s.fullscreen ? 0.0f : 18.0f;
  const Rectangle r{856, y, 360, 28 + 8 + bar + 187 + 8 + 34 + 14};
  plate(r);
  cardTitle("APERÇU EN DIRECT", r.x + 14, y + 24);
  const Rectangle frame{r.x + 14, y + 36, 332, bar + 187};
  DrawRectangleRec(frame, kWell);
  DrawRectangleLinesEx(ui::grow(frame, 1), 1.0f, Color{191, 217, 255, 51});
  if (bar > 0) {  // windowed: the window's title bar
    DrawRectangleRec({frame.x, frame.y, frame.width, bar}, kTrack);
    DrawRectangleRec({frame.x + 6, frame.y + 6, 6, 6}, ui::kDisabled);
    DrawRectangleRec({frame.x + 16, frame.y + 6, 6, 6}, ui::kDisabled);
    drawText("Clash Horizon · fenêtre", frame.x + 30, capsY(frame.y + 9, 13.7f, kB), 13.7f, kDim, kB);
  }
  const Rectangle view{frame.x, frame.y + bar, 332, 187};
  // The scene, zoomed on the middle of the arena; it shakes like the camera would
  const float cyc = std::fmod(t, 1.8f) / 1.8f, amp = s.screenShake == 0 ? 0.0f : s.screenShake == 1 ? 2.0f : 6.0f;
  const float fade = cyc < 0.3f ? 1.0f - cyc / 0.3f : 0.0f;
  const Vec2 shake{amp * fade * std::sin(t * 61.0f), amp * fade * 0.6f * std::cos(t * 47.0f)};
  const auto& list = stageList();
  const StageDef& st = list[std::clamp(stage, 0, int(list.size()) - 1)];
  constexpr float kZoom = 2.2f, kWorld = 0.78f;
  const float sc = view.width / kScreenW * kZoom;
  // world (x, y) -> preview: the ground's middle a little under the view's centre
  auto at = [&](float wx, float wy) {
    return Vec2{view.x + view.width * 0.5f + wx * kWorld * sc + shake.x, view.y + view.height * 0.62f + wy * kWorld * sc + shake.y};
  };
  beginClip(view);
  rlPushMatrix();
  rlTranslatef(view.x + view.width * 0.5f + shake.x, view.y + view.height * 0.5f + shake.y, 0.0f);
  rlScalef(sc, sc, 1.0f);
  rlTranslatef(-kScreenW * 0.5f, -kScreenH * 0.55f, 0.0f);
  drawBackground(st, {0.0f, -150.0f}, 1.0f, t);
  rlPopMatrix();
  rlPushMatrix();
  const Vec2 o = at(0, 0);
  rlTranslatef(o.x, o.y, 0.0f);
  rlScalef(kWorld * sc, kWorld * sc, 1.0f);
  std::vector<Platform> plats = st.platforms;
  updatePlatforms(plats, int(t * 60.0f));
  drawPlatformList(plats, st.theme, t);
  rlPopMatrix();
  // J1 (N° 01) and a CPU (N° 07) face to face, J3 (N° 02) in the air
  const auto& R = roster();
  struct Who { int fighter; float x, y; int facing; const char* tag; Color col; };
  const Who who[3] = {{0, -95, 0, 1, "J1", portColor(0)}, {std::min(3, int(R.size()) - 1), 30, 0, -1, "CPU", portColor(1)},
                      {std::min(1, int(R.size()) - 1), 110, -150, -1, "J3", portColor(2)}};
  const float fh = 120.0f * kWorld * sc * 0.55f;
  for (const Who& w : who) {
    const CharacterDef& d = R[w.fighter];
    const Vec2 feet = at(w.x, w.y);
    drawShowcase(d, 0, restPose(d, t), feet, w.facing, fh, t + w.x);
    if (s.showHitboxes) {  // the hurtbox, green
      const Rectangle hb{feet.x - fh * 0.3f, feet.y - fh, fh * 0.6f, fh};
      DrawRectangleRec(hb, Color{61, 220, 151, 56});
      DrawRectangleLinesEx(hb, 1.5f, Color{61, 220, 151, 255});
    }
    if (s.playerTags) {
      const float tw = textWidth(w.tag, 13, kT) + 8, ty = feet.y - fh - 14;
      DrawRectangleRec({feet.x - tw * 0.5f, ty - 7, tw, 14}, w.col);
      drawText(w.tag, feet.x - tw * 0.5f + 4, capsY(ty, 13, kT), 13, ui::kInk, kT);
    }
  }
  if (s.showHitboxes) {  // J1's punch, orange
    const Vec2 p = at(-40, -50);
    const Rectangle hb{p.x - 10, p.y - 6, 20, 12};
    DrawRectangleRec(hb, Color{255, 122, 82, 90});
    DrawRectangleLinesEx(hb, 1.5f, Color{255, 122, 82, 255});
  }
  endClip();
  if (s.showFps) {
    DrawRectangleRec({view.x + 6, view.y + 4, 44, 15}, withAlpha(ui::kInk, 0.75f));
    drawText("60 FPS", view.x + 9, capsY(view.y + 11.5f, 13, kT), 13, Color{61, 220, 151, 255}, kT);
  }
  const char* q = TextFormat("%s%s", s.highQuality ? "Pixel art" : "Rendu simple", s.fullscreen ? " · plein écran" : " · fenêtre");
  const float qw = textWidth(q, 13.7f, kB) + 10;
  DrawRectangleRec({view.x + view.width - 6 - qw, view.y + view.height - 20, qw, 16}, withAlpha(ui::kNight, 0.8f));
  drawText(q, view.x + view.width - 1 - qw, capsY(view.y + view.height - 12, 13.7f, kB), 13.7f, kSoft, kB);
  ui::caps("RENDU", r.x + 14, frame.y + frame.height + 18, 10, 0.24f, kDim);
  drawText(render, r.x + 14 + ui::capsWidth("RENDU", 10, 0.24f) + 10, capsY(frame.y + frame.height + 18, 15, kB), 15, kSoft, kB);
  ui::caps("L'APERÇU SUIT TES RÉGLAGES", r.x + 14, frame.y + frame.height + 40, 10, 0.18f, withAlpha(kDim, 0.8f));
  return r.y + r.height;
}

// Game: the rules the next fights will have.
float rulesCard(float y, float t) {
  const Settings& s = settings();
  const float h = 16 + 19 + 10 + 5 * 34 + 4 * 10 + (s.items ? 0 : 28) + 16;
  const Rectangle r{856, y, 360, h};
  plate(r);
  cardTitle("RÈGLES DES PROCHAINS COMBATS", r.x + 18, y + 26);
  float ly = y + 45;
  auto line = [&](const char* label, float indent) {
    DrawRectangleRec({r.x + 18, ly, 324, 34}, kWell);
    drawText(label, r.x + 28 + indent, capsY(ly + 17, 19.2f, kB), 19.2f, kSoft, kB);
    const float cy = ly + 17;
    ly += 44;
    return cy;
  };
  auto value = [&](const char* v, float cy, Color c) {
    drawText(v, r.x + 332 - textWidth(v, 19.5f, kT), capsY(cy, 19.5f, kT), 19.5f, c, kT);
    return r.x + 332 - textWidth(v, 19.5f, kT);
  };
  float cy = line("Vies", 0);
  float vx = value(TextFormat("%d vie%s", s.stocks, s.stocks > 1 ? "s" : ""), cy, ui::kSilver) - 14;
  for (int i = 0; i < s.stocks; ++i) ui::diamond({vx - i * 16.0f, cy}, 7.0f, ui::kBlue);
  cy = line("Clash Orb", 24);
  drawClashOrb({r.x + 36, cy}, 8.0f, t, 0);
  value(s.items ? "Activée" : "Désactivée", cy, s.items ? kLight : ui::kDisabled);
  if (!s.items) {
    drawText("Sans orbe, personne ne peut lancer d'ultime.", r.x + 18, capsY(ly + 4, 17.8f, kB), 17.8f, kDanger, kB);
    ly += 28;
  }
  cy = line("Armes", 28);
  if (!roster().empty()) drawWeaponIcon(roster()[0], 0, {r.x + 38, cy}, 20, s.weapons ? 1.0f : 0.35f);
  value(s.weapons ? "Activées" : "Désactivées", cy, s.weapons ? kLight : ui::kDisabled);
  cy = line("Ordinateurs", 0);
  const bool max = s.cpuLevel >= kMaxCpuLevel;
  vx = value(TextFormat("Niveau %d%s", s.cpuLevel, max ? " · ×2" : ""), cy, ui::kSilver) - 8;
  for (int i = kMaxCpuLevel - 1; i >= 0; --i) {  // ten slanted segments, lit up to the level
    const float x1 = vx - (kMaxCpuLevel - 1 - i) * 11.0f, x0 = x1 - 8;
    const Color c = i < s.cpuLevel ? (max ? Color{255, 122, 82, 255} : ui::kBlue) : kTrack;
    ui::quad({x0 + 2, cy - 6}, {x1, cy - 6}, {x1 - 2, cy + 6}, {x0, cy + 6}, c, c, c, c);
  }
  cy = line("Saut avec Haut", 0);
  value(s.tapJump ? "Oui" : "Non", cy, ui::kSilver);
  return r.y + r.height;
}
}  // namespace

int App::settingsRowCount() const { return kRowCounts[setTab_]; }

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
        int* v = sliderFor(0, row);
        *v = std::clamp(*v + delta * 5, 0, 100);
      } else {
        audio_.play(Sfx::HitHeavy, 1.0f);
        audio_.play(Sfx::Crowd, 0.8f);
        soundTestAt_ = frames_;
      }
      break;
    case 1:
      if (row == 0) s.fullscreen = !s.fullscreen;
      if (row == 1) {
        const std::vector<Res> list = resolutions();
        const int n = int(list.size()), i = (resolutionIndex(list) + delta + n) % n;
        s.resW = list[i].w;
        s.resH = list[i].h;
      }
      if (row == 2) s.vsync = !s.vsync;
      if (row == 3) s.screenShake = (s.screenShake + delta + 3) % 3;
      if (row == 4) s.playerTags = !s.playerTags;
      if (row == 5) s.showFps = !s.showFps;
      if (row == 6) s.showHitboxes = !s.showHitboxes;
      break;
    case 2:
      if (row == 0) s.highQuality = !s.highQuality;
      if (row == 1) s.rtx = (s.rtx + delta + 3) % 3;
      if (row == 2) s.upscale = (s.upscale + delta + 4) % 4;
      if (row == 3) s.antiAlias = (s.antiAlias + delta + 3) % 3;
      if (row == 4) s.sharpness = std::clamp(s.sharpness + delta * 5, 0, 100);
      break;
    case 3:
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
  const int rows = settingsRowCount();
  setRow_ = std::min(setRow_, rows - 1);

  if (waitingPad_) {  // rebinding a gamepad button: the next button pressed on any pad
    if (setTab_ != kKeysTab || IsKeyPressed(KEY_ESCAPE) || padAnyPressed(kPadStart) || setRow_ < kActJump ||
        setRow_ >= kActionCount || setCol_ < 2) {
      waitingPad_ = false;
      audio_.play(Sfx::MenuBack);
      return;
    }
    for (int p = 0; p < kMaxPads; ++p)
      for (int b = 0; b < kPadButtonCount; ++b)
        if (padBindable(b) && padPressed(p, b)) {
          waitingPad_ = false;
          audio_.play(bindPadButton(setRow_, setCol_ - 2, b) ? Sfx::MenuSelect : Sfx::MenuBack);
          return;
        }
    return;
  }

  if (waitingKey_) {  // rebinding a key
    if (setTab_ != kKeysTab || padAnyPressed(kPadB) || setRow_ < 0 || setRow_ >= kActionCount || setCol_ > 1) {
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

  // A cell of the controls table (or its reset row) pressed: wait for a key / button, or reset
  auto press = [&](int row, int col) {
    setRow_ = row;
    setCol_ = col;
    if (row == kActionCount) {
      if (col < 2) {
        s.resetKeys(col);
        saveSettings();
        toast(col == 0 ? "Touches du clavier 1 réinitialisées" : "Touches du clavier 2 réinitialisées");
      } else if (col == 2) {
        s.resetPad();
        saveSettings();
        toast("Boutons de la manette réinitialisés");
      } else {
        resetAllControls();
        toast("Touches du clavier et boutons de la manette réinitialisés");
      }
    } else if (col < 2) {
      waitingKey_ = true;
      while (GetKeyPressed() != 0) {}
    } else if (row < kActJump) {
      toast("Toujours le stick gauche ou la croix : ne se change pas");
      audio_.play(Sfx::MenuBack);
      return;
    } else {
      waitingPad_ = true;
    }
    audio_.play(Sfx::MenuSelect);
  };

  const int before = setRow_ * 10 + setTab_ * 100 + setCol_;
  if (nav.up && setRow_ >= 0) setRow_--;
  if (nav.down && setRow_ < rows - 1) setRow_++;
  if (nav.left || nav.right) {
    const int d = nav.left ? -1 : 1;
    if (setRow_ < 0) setTab_ = (setTab_ + d + 5) % 5;
    else if (setTab_ == kKeysTab) setCol_ = (setCol_ + d + 4) % 4;
    else changeSetting(setRow_, d);
  }
  const bool padCell = setTab_ == kKeysTab && setRow_ >= kActJump && setRow_ < kActionCount && setCol_ >= 2;
  if (padCell && nav.prev) {  // clear a gamepad slot (an action keeps at least one button)
    materializePadFallback();
    auto& slots = s.pad[setRow_];
    const int sl = setCol_ - 2;
    if (slots[sl] >= 0 && (setRow_ == kActSmash || slots[1 - sl] >= 0)) {
      slots[sl] = -1;
      saveSettings();
      audio_.play(Sfx::MenuBack);
    } else if (slots[sl] >= 0) {
      toast(TextFormat("« %s » a besoin d'au moins un bouton", actionName(setRow_)));
    }
  }
  if (nav.confirm) {
    if (setRow_ < 0) setRow_ = 0;
    else if (setTab_ == kKeysTab) press(setRow_, setCol_);
    else changeSetting(setRow_, 1);
  }

  // Mouse
  for (int i = 0; i < 5; ++i)
    if (ui::clicked(tabRect(i))) {
      setTab_ = i;
      setRow_ = -1;
    }
  const bool pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
  if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) dragSlider_ = -1;
  if (setTab_ < kKeysTab) {
    for (int r = 0; r < kRowCounts[setTab_]; ++r) {
      if (ui::hovered(rowRect(r)) && ui::mouseMoved() && dragSlider_ < 0) setRow_ = r;
      switch (widgetFor(setTab_, r)) {
        case Widget::Slider:
          if (pressed && ui::hovered(ui::grow(trackRect(r), 6))) dragSlider_ = r;
          if (dragSlider_ == r) {
            const Rectangle tr = trackRect(r);
            const int v = int(std::lround(clampf((GetMousePosition().x - tr.x) / tr.width, 0.0f, 1.0f) * 100.0f));
            if (v != *sliderFor(setTab_, r)) {
              *sliderFor(setTab_, r) = v;
              applyDisplaySettings();
            }
            setRow_ = r;
          } else if (ui::clicked(minusRect(r)) || ui::clicked(plusRect(r))) {
            setRow_ = r;
            changeSetting(r, ui::clicked(minusRect(r)) ? -1 : 1);
          }
          break;
        case Widget::Toggle:
          if (ui::clicked(toggleRect(r))) {  // OUI on the left, NON on the right
            setRow_ = r;
            const Rectangle tr = toggleRect(r);
            if ((GetMousePosition().x < tr.x + tr.width * 0.5f) != toggleOn(setTab_, r)) changeSetting(r, 1);
          }
          break;
        case Widget::Selector:
          if (ui::clicked(prevRect(r)) || ui::clicked(nextRect(r))) {
            setRow_ = r;
            changeSetting(r, ui::clicked(prevRect(r)) ? -1 : 1);
          }
          break;
        case Widget::Button:
          if (ui::clicked(buttonRect(r))) {
            setRow_ = r;
            changeSetting(r, 1);
          }
          break;
      }
    }
  } else {
    for (int r = 0; r <= kActionCount; ++r)
      for (int c = 0; c < 4; ++c) {
        const Rectangle k = r < kActionCount ? keyRect(r, c) : resetRect(c);
        if (ui::hovered(k) && ui::mouseMoved()) setRow_ = r, setCol_ = c;
        if (ui::clicked(k)) press(r, c);
      }
  }
  if (dragSlider_ >= 0 && !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) saveSettings();
  if (setRow_ * 10 + setTab_ * 100 + setCol_ != before) audio_.play(Sfx::MenuMove, 0.6f);
  setRow_ = std::min(setRow_, settingsRowCount() - 1);

  if (nav.back || ui::clicked(backRect())) {
    saveSettings();
    audio_.play(Sfx::MenuBack);
    toast("Paramètres enregistrés");
    if (match_ && settingsReturn_ == Screen::Match) match_->setHitboxes(s.showHitboxes);  // the paused fight too
    goTo(settingsReturn_);
  }
}


void App::drawSettings() {
  const float t = time_, sec = screenFrames_ / 60.0f;
  const Settings& s = settings();
  auto up = [&](float delay) { return ui::seg(sec, delay, delay + 0.4f, 0.2f, 0.8f, 0.2f); };
  // « Prestige »: the arena far behind the night, the blade on the right
  ui::stageBackdrop(stage_, t, 0.0f);
  DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(ui::kNight, 0.9f));
  ui::lean(1262, 1266, withAlpha(ui::kSilver, 0.16f));

  // The title, "saved automatically", the way back
  {
    const float a = up(0.0f), dy = 12.0f * (1.0f - a);
    ui::caps("CLASH HORIZON", 36, 36 + dy, 11, 0.3f, withAlpha(ui::kBlue, a));
    ui::big("PARAMÈTRES", 36, 50 + dy, 52, ui::Face::Cond, withAlpha(WHITE, a));
    const char* saved = "ENREGISTRÉS AUTOMATIQUEMENT";
    ui::caps(saved, 36 + ui::bigWidth("PARAMÈTRES", 52, ui::Face::Cond) + 20, 84 + dy, 10, 0.24f, withAlpha(ui::kMist, a));
    const Rectangle br = backRect();
    const float kw = ui::pkeyWidth("ÉCHAP", ui::Btn::B, 22), lw = ui::capsWidth("RETOUR", 12, 0.22f);
    const float bx = br.x + br.width - kw - 10 - lw;
    ui::pkey("ÉCHAP", ui::Btn::B, bx, br.y + 4, 22, false, a);
    ui::caps("RETOUR", bx + kw + 10, br.y + 15, 12, 0.22f, withAlpha(ui::hovered(br) ? WHITE : ui::kSilver, a));
  }

  // The tabs: the open one on a silver plate leaning like the blade, the others mist
  for (int i = 0; i < 5; ++i) {
    const float a = up(0.08f + 0.03f * i);
    const bool sel = i == setTab_;
    const Rectangle r = tabRect(i);
    if (sel) slant(r, withAlpha(ui::kSilver, a));
    else if (ui::hovered(r)) slant(r, withAlpha(ui::kSilver, 0.35f * a), true);
    const Color ink = sel ? ui::kNight : ui::hovered(r) ? WHITE : ui::kMist;
    ui::caps(kTabNames[i], r.x + 16, r.y + r.height * 0.5f, 13, 0.22f, withAlpha(ink, a));
    if (sel && setRow_ < 0) DrawRectangleRec({r.x + 16, r.y + r.height + 6, r.width - 32, 2}, ui::kBlue);
  }
  DrawRectangleRec({36, 158, 1208, 1}, withAlpha(ui::kSilver, 0.12f));

  if (setTab_ < kKeysTab) {
    const float a = up(0.2f);
    for (int r = 0; r < kRowCounts[setTab_]; ++r) {
      const Rectangle rr = rowRect(r);
      const float cy = rowCy(r);
      const bool foc = r == setRow_;
      if (foc) {  // the chosen row: a raised night, a cyan edge
        DrawRectangleRec({rr.x, rr.y + 4, rr.width, rr.height - 8}, kCard);
        DrawRectangleRec({rr.x, rr.y + 4, 3, rr.height - 8}, ui::kBlue);
      }
      DrawRectangleRec({rr.x, rr.y + rr.height - 1, rr.width, 1}, withAlpha(ui::kSilver, 0.07f));
      drawText(kRows[setTab_][r].label, rr.x + 20, capsY(cy, 17, kB), 17, withAlpha(foc ? WHITE : ui::kMist, a), kB);
      switch (widgetFor(setTab_, r)) {
        case Widget::Slider: {  // a thin track, the cyan fill and its bright head (like the launch bar)
          const int v = *sliderFor(setTab_, r);
          smallBox(minusRect(r), "-");
          const Rectangle tr = trackRect(r);
          const float fx = tr.x + tr.width * v / 100.0f;
          DrawRectangleRec({tr.x, cy - 1.5f, tr.width, 3}, withAlpha(ui::kSilver, 0.12f));
          DrawRectangleRec({tr.x, cy - 1.5f, fx - tr.x, 3}, ui::kBlue);
          DrawRectangleRec({fx - 2, cy - 6, 4, 12}, foc ? WHITE : ui::kSilver);
          smallBox(plusRect(r), "+");
          const char* txt = TextFormat("%d", v);
          ui::exo(txt, 780 - ui::exoWidth(txt, 22), cy + 8, 22, ui::kSilver);
          break;
        }
        case Widget::Toggle: {  // OUI | NON, the chosen half on a silver plate
          const Rectangle tr = toggleRect(r);
          const bool on = toggleOn(setTab_, r);
          const Rectangle half[2] = {{tr.x, tr.y, tr.width * 0.5f + 3, tr.height}, {tr.x + tr.width * 0.5f - 3, tr.y, tr.width * 0.5f + 3, tr.height}};
          slant(tr, withAlpha(ui::kSilver, 0.18f), true);
          slant(half[on ? 0 : 1], ui::kSilver);
          for (int h = 0; h < 2; ++h) {
            const char* w = h == 0 ? "OUI" : "NON";
            const bool lit = (h == 0) == on;
            ui::caps(w, half[h].x + (half[h].width - ui::capsWidth(w, 12, 0.2f)) * 0.5f, cy, 12, 0.2f, lit ? ui::kNight : ui::kMist);
          }
          break;
        }
        case Widget::Selector: {
          smallBox(prevRect(r), "", -1);
          smallBox(nextRect(r), "", 1);
          const std::string v = upperFr(selectorText(setTab_, r));
          const Color vc = setTab_ == 3 && r == 3 && s.cpuLevel >= kMaxCpuLevel ? Color{255, 122, 82, 255} : ui::kSilver;
          float fs = 14;
          while (fs > 10 && ui::capsWidth(v.c_str(), fs, 0.16f) > nextRect(r).x - prevRect(r).x - 44) fs -= 0.5f;
          const float mx = (prevRect(r).x + nextRect(r).x + 26) * 0.5f;
          ui::caps(v.c_str(), mx - ui::capsWidth(v.c_str(), fs, 0.16f) * 0.5f, cy, fs, 0.16f, vc);
          break;
        }
        case Widget::Button: {
          const Rectangle b = buttonRect(r);
          const bool danger = setTab_ == 3;
          const bool hov = ui::hovered(b);
          if (hov) slant(b, withAlpha(danger ? kDanger : ui::kSilver, 0.18f));
          slant(b, danger ? withAlpha(kDanger, 0.7f) : withAlpha(ui::kSilver, 0.5f), true);
          const bool playing = setTab_ == 0 && frames_ - soundTestAt_ < 96;
          const char* label = danger ? "RÉINITIALISER" : playing ? "LECTURE…" : "ÉCOUTER";
          ui::caps(label, b.x + (b.width - ui::capsWidth(label, 12, 0.2f)) * 0.5f, cy, 12, 0.2f, danger ? kDanger : ui::kSilver);
          break;
        }
      }
    }

    // Beside: the help of the chosen row, then the tab's live card
    const RowInfo& info = setRow_ >= 0 ? kRows[setTab_][setRow_] : kTabHelp[setTab_];
    const float hx = 856, hw = 360;
    const float textBottom = ui::drawWrapped(info.help, hx + 18, 0, hw - 36, 16, ui::kMist, false);
    const Rectangle help{hx, 168, hw, 64 + textBottom + 14};
    plate(help);
    DrawRectangleRec({help.x, help.y, 3, help.height}, ui::kBlue);
    ui::caps("AIDE", help.x + 18, help.y + 20, 10, 0.3f, ui::kBlue);
    ui::caps(upperFr(info.label).c_str(), help.x + 18, help.y + 42, 14, 0.12f, ui::kSilver);
    ui::drawWrapped(info.help, help.x + 18, help.y + 60, hw - 36, 16, ui::kMist);
    const float cy = help.y + help.height + 12;
    if (setTab_ == 0) {
      audioCard(cy, frames_ - soundTestAt_ < 96, t);
    } else if (setTab_ <= 2) {
      const char* render = TextFormat("%d × %d  →  %s %d × %d", target_.texture.width, target_.texture.height,
                                      IsWindowFullscreen() ? "écran" : "fenêtre", GetScreenWidth(), GetScreenHeight());
      previewCard(cy, stage_, t, render);
    } else {
      rulesCard(cy, t);
    }
  } else {
    // The controls: one table for both keyboards and the gamepad
    const PadFamily fam = promptFamily();
    const int pad = shownPad(fam);
    ui::caps("ACTION", 56, 206, 11, 0.24f, kDim);
    for (int c = 0; c < 2; ++c) {
      const char* h = c == 0 ? "CLAVIER 1" : "CLAVIER 2";
      const float w = ui::capsWidth(h, 13, 0.22f);
      ui::caps(h, kColX[c] + (kColW[c] - w) * 0.5f, 204, 13, 0.22f, portColor(c));
    }
    std::string padTitle = pad >= 0 ? padModel(pad)
                           : fam == PadFamily::PlayStation ? "Manette PlayStation"
                           : fam == PadFamily::Nintendo    ? "Manette Nintendo"
                                                           : "Manette Xbox";
    padTitle = upperFr(padTitle);
    const float px0 = kColX[2], px1 = kColX[3] + kColW[3];
    float ps = 13.0f;
    while (ps > 9.0f && ui::capsWidth(padTitle.c_str(), ps, 0.22f) > px1 - px0) ps -= 0.5f;
    const float tw = ui::capsWidth(padTitle.c_str(), ps, 0.22f);
    ui::caps(padTitle.c_str(), (px0 + px1 - tw) * 0.5f, 184, ps, 0.22f, ui::kSilver);
    const char* sub = pad >= 0 ? "détectée · bouton 1 et bouton 2" : "pas branchée · bouton 1 et bouton 2";
    drawText(sub, (px0 + px1 - textWidth(sub, 13, kB)) * 0.5f, capsY(205, 13, kB), 13, kDim, kB);

    for (int r = 0; r < kActionCount; ++r) {
      const float y = kTableY + r * kKeyH, cy = y + kKeyH * 0.5f;
      const bool rowFoc = r == setRow_, ult = r == kActUltimate;
      const Rectangle band{46, y, 1188, kKeyH};
      if (rowFoc) {
        DrawRectangleRec(band, kCard);
        DrawRectangleRec({band.x, band.y, 3, band.height}, ui::kBlue);
      } else if (ult) {
        const Color g0 = withAlpha(ui::kBlue, 0.16f), g1 = withAlpha(ui::kBlue, 0.0f);
        ui::quad({band.x, band.y}, {band.x + band.width * 0.7f, band.y}, {band.x + band.width * 0.7f, band.y + band.height},
                 {band.x, band.y + band.height}, g0, g1, g1, g0);
      } else if (r % 2) {
        DrawRectangleRec(band, withAlpha(ui::kNightUp, 0.5f));
      }
      drawText(actionName(r), 56, capsY(cy, 16, kB), 16, ult ? kLight : rowFoc ? WHITE : ui::kSilver, kB);
      if (ult) drawText("après la Clash Orb", 64 + textWidth(actionName(r), 16, kB), capsY(cy, 13, kB), 13, kLight, kB);
      for (int c = 0; c < 4; ++c) {
        const Rectangle k = keyRect(r, c);
        const bool foc = rowFoc && c == setCol_, wait = foc && (c < 2 ? waitingKey_ : waitingPad_);
        const bool fixed = c >= 2 && r < kActJump;
        const int b = c >= 2 && !fixed ? boundButton(pad, r, c - 2) : -1;
        if (wait) {
          DrawRectangleRec(k, ui::kSilver);
          drawText("…", k.x + (k.width - textWidth("…", 16.3f, kT)) * 0.5f, capsY(k.y + k.height * 0.5f, 16.3f, kT), 16.3f, ui::kNight, kT);
        } else if (fixed) {  // moving on a pad: always the left stick or the d-pad
          DrawRectangleLinesEx(k, 1.0f, withAlpha(ui::kSilver, 0.12f));
          const ui::Btn g = c == 2 ? ui::Btn::LStick : ui::Btn::DPad;
          const float gw = ui::padGlyphWidth(g, 18, fam), gx = k.x + (k.width - gw - 14) * 0.5f;
          lockIcon({gx, k.y + 6}, ui::kDim);
          ui::padGlyph(g, gx + 14, k.y + 3, 18, fam);
        } else if (c >= 2 && b < 0) {
          DrawRectangleLinesEx(k, 1.0f, withAlpha(ui::kSilver, 0.1f));
          drawText("—", k.x + (k.width - textWidth("—", 16.3f, kT)) * 0.5f, capsY(k.y + k.height * 0.5f, 16.3f, kT), 16.3f, ui::kDim, kT);
        } else {
          DrawRectangleRec(k, kWell);
          DrawRectangleLinesEx(k, 1.0f, withAlpha(ui::kSilver, 0.5f));
          if (c < 2) {
            const std::string name = keyName(s.keys[c][r]);
            float ks = 13.0f;
            while (ks > 9.0f && ui::capsWidth(name.c_str(), ks, 0.06f) > k.width - 12) ks -= 0.5f;
            const float kw = ui::capsWidth(name.c_str(), ks, 0.06f);
            ui::caps(name.c_str(), k.x + (k.width - kw) * 0.5f, k.y + k.height * 0.5f, ks, 0.06f, ui::kSilver);
          } else {
            const ui::Btn g = ui::btnFor(b);
            ui::padGlyph(g, k.x + (k.width - ui::padGlyphWidth(g, 18, fam)) * 0.5f, k.y + 3, 18, fam);
          }
        }
        if (foc && !wait) focusRing(k);
      }
    }
    // Reset: each keyboard, the gamepad, everything
    const float ry = resetRect(0).y;
    ui::caps("RÉINITIALISER", 56, ry + 13, 11, 0.24f, kDim);
    const char* resets[4] = {"CLAVIER 1", "CLAVIER 2", "MANETTE", "TOUT"};
    for (int c = 0; c < 4; ++c) {
      const Rectangle b = resetRect(c);
      const bool foc = setRow_ == kActionCount && setCol_ == c, all = c == 3;
      if (ui::hovered(b)) slant(b, withAlpha(all ? kDanger : ui::kSilver, 0.18f));
      slant(b, all ? withAlpha(kDanger, 0.7f) : withAlpha(ui::kSilver, 0.4f), true);
      const float w = ui::capsWidth(resets[c], 11, 0.18f);
      ui::caps(resets[c], b.x + (b.width - w) * 0.5f, b.y + 13, 11, 0.18f, all ? kDanger : ui::kSilver);
      if (foc) focusRing(b);
    }
    drawText("Attaque + Spéciale = smash · Bas, Bas-avant, Avant + Attaque = smash avant chargé · Bas, Haut + Attaque = "
             "smash haut chargé",
             46, capsY(612, 13, kB), 13, kDim, kB);
    drawText("Manette : le stick droit fait aussi les smashs, START met en pause. Clavier 2 : le pavé numérique marche "
             "aussi (0 à 7).",
             46, capsY(630, 13, kB), 13, kDim, kB);
  }

  // Bottom: the version, what the keys do here, a note
  DrawRectangleRec({36, 650, 1208, 1}, withAlpha(ui::kSilver, 0.08f));
  ui::caps(TextFormat("CLASH HORIZON  %s", gameVersion()), 36, 680, 10, 0.24f, ui::kDim);
  struct Hint { const char* key; ui::Btn pad; const char* label; };
  std::vector<Hint> hints;
  const char* foot = "Changer d'onglet : GAUCHE/DROITE sur la ligne des onglets.";
  if (setRow_ < 0) {
    hints = {{"GAUCHE/DROITE", ui::Btn::DPadLR, "Onglet"}, {"BAS", ui::Btn::DPadDown, "Réglages"}};
  } else if (setTab_ == kKeysTab) {
    hints = {{"FLÈCHES", ui::Btn::DPad, "Naviguer"}, {"ENTRÉE", ui::Btn::A, "Changer"}};
    if (setCol_ >= 2 && setRow_ >= kActJump && setRow_ < kActionCount) hints.push_back({"K", ui::Btn::X, "Effacer"});
    foot = setRow_ < kActionCount ? "Une touche ou un bouton déjà utilisé est échangé." : "Touches et boutons d'origine.";
  } else {
    hints = {{"HAUT/BAS", ui::Btn::DPadUD, "Choisir"}, {"GAUCHE/DROITE", ui::Btn::DPadLR, "Modifier"}};
  }
  float total = 0;
  for (const Hint& h : hints) total += ui::pkeyWidth(h.key, h.pad, 22) + 8 + textWidth(h.label, 12, kB) + 28;
  float hx = 1244 - (total - 28);
  for (const Hint& h : hints) {
    hx += ui::pkey(h.key, h.pad, hx, 662, 22) + 8;
    drawText(h.label, hx, 662 + (22 - 14.52f) * 0.5f, 12, ui::kMist, kB);
    hx += textWidth(h.label, 12, kB) + 28;
  }
  drawText(foot, 1244 - textWidth(foot, 12, kB), 692, 12, ui::kDim, kB);

  // Waiting for a new key or button
  if (waitingKey_ || waitingPad_) {
    DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(ui::kNight, 0.8f));
    const bool padWait = waitingPad_;
    const Rectangle m{340, padWait ? 160.0f : 270.0f, 600, padWait ? 400.0f : 180.0f};
    plate(m);
    DrawRectangleRec({m.x, m.y, 3, m.height}, ui::kBlue);
    const int row = std::clamp(setRow_, 0, kActionCount - 1);
    const std::string title = upperFr(std::string(padWait ? "Nouveau bouton : " : "Nouvelle touche : ") + actionName(row));
    float ts = 34.0f;
    while (ts > 14.0f && ui::bigWidth(title.c_str(), ts, ui::Face::Cond) > m.width - 40) ts -= 1.0f;
    ui::big(title.c_str(), 640 - ui::bigWidth(title.c_str(), ts, ui::Face::Cond) * 0.5f, m.y + 26, ts, ui::Face::Cond, WHITE);
    const char* port = padWait ? (setCol_ == 2 ? "MANETTE · BOUTON 1" : "MANETTE · BOUTON 2") : setCol_ == 0 ? "CLAVIER 1" : "CLAVIER 2";
    const Color pc = padWait ? ui::kSilver : portColor(setCol_);
    ui::caps(port, 640 - ui::capsWidth(port, 12, 0.24f) * 0.5f, m.y + 78, 12, 0.24f, pc);
    const float blink = std::fmod(t, 1.0f) < 0.5f ? 1.0f : 0.3f;
    DrawRectangleRec({610, m.y + 104, 60, 3}, withAlpha(ui::kBlue, blink));
    if (padWait) {
      const char* a = "Appuie sur un bouton de la manette  —  ";
      const char* b = " pour annuler";
      const float gw = ui::padGlyphWidth(ui::Btn::Start, 22, promptFamily());
      float x = 640 - (textWidth(a, 15, kB) + gw + textWidth(b, 15, kB)) * 0.5f;
      drawText(a, x, capsY(m.y + 140, 15, kB), 15, kSoft, kB);
      x += textWidth(a, 15, kB);
      ui::padGlyph(ui::Btn::Start, x, m.y + 129, 22, promptFamily());
      drawText(b, x + gw, capsY(m.y + 140, 15, kB), 15, kSoft, kB);
      // The controller, its button of the moment circled
      const PadFamily fam = promptFamily();
      const int b0 = boundButton(shownPad(fam), row, std::clamp(setCol_ - 2, 0, 1));
      drawController(fam, {640, m.y + 275}, 0.5f, b0, t);
    } else {
      const char* h = "Appuie sur une touche  —  ÉCHAP pour annuler";
      drawText(h, 640 - textWidth(h, 15, kB) * 0.5f, capsY(m.y + 140, 15, kB), 15, kSoft, kB);
    }
  }
}
