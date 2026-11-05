// Settings screen (« Ligne d'horizon »): four tabs (audio, display, game, controls). The first three are rows of
// sliders, OUI / NON switches, selectors and buttons, with the help of the chosen row beside them and a live card:
// the real volumes, a preview that follows the display settings, the rules of the next fights. The controls are one
// table: both keyboards and the gamepad's two buttons for each action.
#include <cstring>

#include "game/app.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
constexpr FontKind kT = FontKind::Title, kU = FontKind::Ui;
const char* kTabNames[4] = {"AUDIO", "AFFICHAGE", "JEU", "COMMANDES"};
constexpr Color kSoft{212, 222, 245, 255};   // #D4DEF5
constexpr Color kDim{154, 168, 199, 255};    // #9AA8C7
constexpr Color kLight{255, 200, 112, 255};  // #FFC870
constexpr Color kTrack{26, 37, 80, 255};     // #1A2550
constexpr Color kWell{10, 16, 36, 255};      // #0A1024
constexpr Color kDanger{255, 154, 122, 255};
const Rectangle kPanel{64, 188, 772, 456};       // the rows (audio, display, game)
const Rectangle kKeysPanel{64, 188, 1152, 456};  // the controls: the whole width

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
     {"Clash Orb", "L'orbe qui traverse l'arène. La casser donne l'ultime : c'est la seule façon de l'obtenir."},
     {"Armes", "Les deux armes de chaque combattant tombent sur l'arène : il prend la sienne en restant dessus, touche Arme maintenue 1 s. Avec A et B, la touche alterne."},
     {"Niveau des ordinateurs", "Niveau par défaut des combattants contrôlés par l'ordinateur (1 à 10 : le niveau 10 frappe deux fois plus fort)."},
     {"Saut avec Haut", "Haut (stick ou touche) fait sauter. Haut + attaque ou spéciale ensemble reste un coup vers le haut."},
     {"Tout réinitialiser", "Remet tous les paramètres et toutes les touches par défaut."}},
};
const RowInfo kTabHelp[4] = {{"Audio", "Le volume de tout le jeu, de la musique et des effets."},
                             {"Affichage", "L'écran, la qualité de l'image et ce qui s'affiche pendant les combats."},
                             {"Jeu", "Les règles par défaut des prochains combats."},
                             {"Commandes", ""}};
const int kRowCounts[4] = {4, 7, 6, kActionCount + 1};  // controls: the actions + the reset row

// ---- Where things are -------------------------------------------------------------------
Rectangle tabRect(int i) { return {64.0f + i * 206.0f, 120, 200, 48}; }
Rectangle backRect() {
  const float w = 10 + ui::promptWidth("ÉCHAP", ui::Btn::B, 24) + 10 + ui::trackedWidth("RETOUR", 19.5f, 0.08f) + 16;
  return {1216 - w, 46, w, 40};
}
Rectangle rowRect(int row) { return {kPanel.x, kPanel.y + 14 + row * 58.0f, kPanel.width, 58}; }
float rowCy(int row) { return kPanel.y + 43 + row * 58.0f; }
// The widgets, right-aligned on x 814 like the mockup
Rectangle minusRect(int row) { return {484, rowCy(row) - 15, 30, 30}; }
Rectangle trackRect(int row) { return {522, rowCy(row) - 7, 190, 14}; }
Rectangle plusRect(int row) { return {720, rowCy(row) - 15, 30, 30}; }
Rectangle toggleRect(int row) { return {700, rowCy(row) - 17, 114, 34}; }
Rectangle prevRect(int row) { return {568, rowCy(row) - 15, 30, 30}; }
Rectangle nextRect(int row) { return {784, rowCy(row) - 15, 30, 30}; }
Rectangle buttonRect(int row) { return {644, rowCy(row) - 17, 170, 34}; }
// The controls table: an action per row; keyboard 1, keyboard 2, the gamepad's button 1 and button 2
constexpr float kTableY = 236.0f, kRowH = 27.0f;
constexpr float kColX[4] = {350, 528, 730, 904}, kColW[4] = {170, 170, 166, 190};
Rectangle keyRect(int row, int col) { return {kColX[col], kTableY + row * kRowH + 1.5f, kColW[col], kRowH - 3.0f}; }
// The reset row under it: keyboard 1, keyboard 2, the gamepad, everything
Rectangle resetRect(int col) {
  const float y = kTableY + kActionCount * kRowH + 6.0f;
  if (col == 2) return {kColX[2], y, kColX[3] + kColW[3] - kColX[2], 26};
  if (col == 3) return {1102, y, 96, 26};
  return {kColX[col], y, kColW[col], 26};
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
bool toggleOn(int tab, int row) {
  const Settings& s = settings();
  if (tab == 1) return row == 0 ? s.fullscreen : row == 3 ? s.playerTags : row == 4 ? s.showFps : row == 5 ? s.showHitboxes : s.pixelArt;
  return row == 1 ? s.items : row == 2 ? s.weapons : s.tapJump;
}
std::string selectorText(int tab, int row) {
  const Settings& s = settings();
  static const char* shakes[3] = {"Aucune", "Légères", "Normales"};
  if (tab == 1) return row == 1 ? (s.highQuality ? "Haute (lissage)" : "Normale") : shakes[std::clamp(s.screenShake, 0, 2)];
  if (row == 0) return TextFormat("%d vie%s", s.stocks, s.stocks > 1 ? "s" : "");
  return TextFormat("Niveau %d", s.cpuLevel);
}

// ---- Drawing helpers --------------------------------------------------------------------
// A plate with only its top-left corner cut (tabs, small buttons).
void cutTL(Rectangle r, float cut, Color c) {
  DrawRectangleRec({r.x + cut, r.y, r.width - cut, r.height}, c);
  DrawRectangleRec({r.x, r.y + cut, cut, r.height - cut}, c);
  DrawTriangle({r.x, r.y + cut}, {r.x + cut, r.y + cut}, {r.x + cut, r.y}, c);
}
// The panels: night at 92 %, a faint line, two corners cut.
void panelBox(Rectangle r) {
  ui::bevel(r, 18, Color{13, 20, 48, 235}, Color{13, 20, 48, 235});
  ui::bevelLines(r, 18, 1.0f, Color{191, 217, 255, 36});
}
// A small square button (−, +, the selector's arrows).
void smallBox(Rectangle r, const char* label, int arrow = 0) {
  DrawRectangleRec(r, ui::hovered(r) ? mixColor(ui::kNavy, WHITE, 0.12f) : ui::kNavy);
  const Vector2 c{r.x + r.width * 0.5f, r.y + r.height * 0.5f};
  if (arrow) ui::arrowTri(c, 11, arrow, ui::kCream);
  else drawText(label, c.x - textWidth(label, 21.7f, kT) * 0.5f, capsY(c.y, 21.7f, kT), 21.7f, ui::kCream, kT);
}
std::vector<std::string> wrapLines(const std::string& text, float width, float size) {
  std::vector<std::string> out;
  std::string line, word;
  for (size_t i = 0; i <= text.size(); ++i) {
    if (i < text.size() && text[i] != ' ') {
      word += text[i];
      continue;
    }
    const std::string next = line.empty() ? word : line + " " + word;
    if (!line.empty() && textWidth(next.c_str(), size, kU) > width) {
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
// A line icon (24 x 24 design, like the mockup's), drawn `size` wide at (x, y).
void polyline(std::initializer_list<Vector2> pts, Vector2 o, float k, float th, Color c, bool closed = false) {
  const Vector2* p = pts.begin();
  const int n = int(pts.size());
  for (int i = 0; i + 1 < n + (closed ? 1 : 0); ++i) {
    const Vector2 a = p[i], b = p[(i + 1) % n];
    DrawLineEx({o.x + a.x * k, o.y + a.y * k}, {o.x + b.x * k, o.y + b.y * k}, th, c);
  }
}
void bezier(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Vector2 o, float k, float th, Color col) {
  Vector2 prev = a;
  for (int i = 1; i <= 10; ++i) {
    const float t = i / 10.0f, u = 1.0f - t;
    const Vector2 p{u * u * u * a.x + 3 * u * u * t * b.x + 3 * u * t * t * c.x + t * t * t * d.x,
                    u * u * u * a.y + 3 * u * u * t * b.y + 3 * u * t * t * c.y + t * t * t * d.y};
    DrawLineEx({o.x + prev.x * k, o.y + prev.y * k}, {o.x + p.x * k, o.y + p.y * k}, th, col);
    prev = p;
  }
}
void tabIcon(int tab, Vector2 o, float size, Color c) {
  const float k = size / 24.0f, th = 2.0f * k;
  switch (tab) {
    case 0:  // a speaker and its waves
      polyline({{4, 9}, {8, 9}, {13, 5}, {13, 19}, {8, 15}, {4, 15}}, o, k, th, c, true);
      bezier({16, 9}, {17.5f, 10.5f}, {17.5f, 13.5f}, {16, 15}, o, k, th, c);
      bezier({19, 6}, {22, 9}, {22, 15}, {19, 18}, o, k, th, c);
      break;
    case 1:  // a screen on its foot
      polyline({{3, 5}, {21, 5}, {21, 16}, {3, 16}}, o, k, th, c, true);
      polyline({{9, 20}, {15, 20}}, o, k, th, c);
      polyline({{12, 16}, {12, 20}}, o, k, th, c);
      break;
    case 2:  // two crossed swords
      polyline({{4, 4}, {14, 14}}, o, k, th, c);
      polyline({{12, 16}, {16, 12}}, o, k, th, c);
      polyline({{15, 15}, {19, 19}}, o, k, th, c);
      polyline({{20, 4}, {10, 14}}, o, k, th, c);
      polyline({{8, 12}, {12, 16}}, o, k, th, c);
      polyline({{9, 15}, {5, 19}}, o, k, th, c);
      break;
    default:  // a gamepad
      polyline({{7, 8}, {17, 8}}, o, k, th, c);
      bezier({17, 8}, {20, 8}, {22, 11}, {22, 15}, o, k, th, c);
      bezier({22, 15}, {22, 18}, {20, 19}, {18.5f, 17.5f}, o, k, th, c);
      polyline({{18.5f, 17.5f}, {16, 15}, {8, 15}, {5.5f, 17.5f}}, o, k, th, c);
      bezier({5.5f, 17.5f}, {4, 19}, {2, 18}, {2, 15}, o, k, th, c);
      bezier({2, 15}, {2, 11}, {4, 8}, {7, 8}, o, k, th, c);
      polyline({{7, 10.5f}, {7, 13.5f}}, o, k, th, c);
      polyline({{5.5f, 12}, {8.5f, 12}}, o, k, th, c);
      DrawCircleV({o.x + 16 * k, o.y + 11 * k}, 1.2f * k, c);
      DrawCircleV({o.x + 18 * k, o.y + 13 * k}, 1.2f * k, c);
      break;
  }
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
    ui::glow(pos[hl], r * 2.2f, withAlpha(ui::kGold, 0.35f + 0.2f * pulse), 6);
    DrawRing(pos[hl], r + 2 + 2 * pulse, r + 5 + 2 * pulse, 0, 360, 40, ui::kGold);
  }
  rlPopMatrix();
}


// ---- The live cards beside the rows -----------------------------------------------------------
// A card's small title ("VOLUME RÉEL"...), tracked capitals.
void cardTitle(const char* text, float x, float cy, Color c = kDim) { ui::tracked(text, x, capsY(cy, 13, kT), 13, 0.22f, c); }

// Audio: what each volume really gives (the music and the effects go through the general volume).
float audioCard(float y, bool testing, float t) {
  const Settings& s = settings();
  const Rectangle r{856, y, 360, 287};
  DrawRectangleRec(r, Color{13, 20, 48, 235});
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
    DrawRectangleRec({box.x, box.y + box.height - eff[m] * 1.4f - 1, box.width, 2}, ui::kGold);
    const char* v = TextFormat("%d %%", eff[m]);
    drawText(v, cx - textWidth(v, 23.9f, kT) * 0.5f, capsY(y + 205, 23.9f, kT), 23.9f, ui::kCream, kT);
    drawText(labels[m], cx - textWidth(labels[m], 16.4f, kU) * 0.5f, capsY(y + 228, 16.4f, kU), 16.4f, kDim, kU);
  }
  const char* note = testing ? "Lecture : un coup puissant, puis les cris de la foule."
                             : "Musique et effets sont multipliés par le volume général.";
  drawText(note, r.x + 18, capsY(y + 262, 16.4f, kU), 16.4f, kDim, kU);
  return r.y + r.height;
}

// Display: a little fight that follows the settings (window, quality, shaking, names, FPS, hitboxes).
float previewCard(float y, int stage, float t) {
  const Settings& s = settings();
  const float bar = s.fullscreen ? 0.0f : 18.0f;
  const Rectangle r{856, y, 360, 28 + 8 + bar + 187 + 8 + 34 + 14};
  DrawRectangleRec(r, Color{13, 20, 48, 235});
  cardTitle("APERÇU EN DIRECT", r.x + 14, y + 24);
  const Rectangle frame{r.x + 14, y + 36, 332, bar + 187};
  DrawRectangleRec(frame, kWell);
  DrawRectangleLinesEx(ui::grow(frame, 1), 1.0f, Color{191, 217, 255, 51});
  if (bar > 0) {  // windowed: the window's title bar
    DrawRectangleRec({frame.x, frame.y, frame.width, bar}, kTrack);
    DrawRectangleRec({frame.x + 6, frame.y + 6, 6, 6}, ui::kDisabled);
    DrawRectangleRec({frame.x + 16, frame.y + 6, 6, 6}, ui::kDisabled);
    drawText("Clash Horizon · fenêtre", frame.x + 30, capsY(frame.y + 9, 13.7f, kU), 13.7f, kDim, kU);
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
  const char* q = TextFormat("%s%s", s.highQuality ? "Rendu ×2 · lissé" : "Rendu normal", s.fullscreen ? " · plein écran" : "");
  const float qw = textWidth(q, 13.7f, kU) + 10;
  DrawRectangleRec({view.x + view.width - 6 - qw, view.y + view.height - 20, qw, 16}, withAlpha(ui::kInk, 0.75f));
  drawText(q, view.x + view.width - 1 - qw, capsY(view.y + view.height - 12, 13.7f, kU), 13.7f, kSoft, kU);
  const auto lines = wrapLines("L'aperçu suit tes réglages : essaie les noms, les FPS, les zones de coup ou les secousses.", 332, 16.4f);
  float ly = frame.y + frame.height + 18;
  for (const auto& l : lines) {
    drawText(l.c_str(), r.x + 14, capsY(ly, 16.4f, kU), 16.4f, kDim, kU);
    ly += 17;
  }
  return r.y + r.height;
}

// Game: the rules the next fights will have.
float rulesCard(float y, float t) {
  const Settings& s = settings();
  const float h = 16 + 19 + 10 + 5 * 34 + 4 * 10 + (s.items ? 0 : 28) + 16;
  const Rectangle r{856, y, 360, h};
  DrawRectangleRec(r, Color{13, 20, 48, 235});
  cardTitle("RÈGLES DES PROCHAINS COMBATS", r.x + 18, y + 26);
  float ly = y + 45;
  auto line = [&](const char* label, float indent) {
    DrawRectangleRec({r.x + 18, ly, 324, 34}, kWell);
    drawText(label, r.x + 28 + indent, capsY(ly + 17, 19.2f, kU), 19.2f, kSoft, kU);
    const float cy = ly + 17;
    ly += 44;
    return cy;
  };
  auto value = [&](const char* v, float cy, Color c) {
    drawText(v, r.x + 332 - textWidth(v, 19.5f, kT), capsY(cy, 19.5f, kT), 19.5f, c, kT);
    return r.x + 332 - textWidth(v, 19.5f, kT);
  };
  float cy = line("Vies", 0);
  float vx = value(TextFormat("%d vie%s", s.stocks, s.stocks > 1 ? "s" : ""), cy, ui::kCream) - 14;
  for (int i = 0; i < s.stocks; ++i) ui::diamond({vx - i * 16.0f, cy}, 7.0f, ui::kBlue);
  cy = line("Clash Orb", 24);
  drawClashOrb({r.x + 36, cy}, 8.0f, t, 0);
  value(s.items ? "Activée" : "Désactivée", cy, s.items ? kLight : ui::kDisabled);
  if (!s.items) {
    drawText("Sans orbe, personne ne peut lancer d'ultime.", r.x + 18, capsY(ly + 4, 17.8f, kU), 17.8f, kDanger, kU);
    ly += 28;
  }
  cy = line("Armes", 28);
  if (!roster().empty()) drawWeaponIcon(roster()[0], 0, {r.x + 38, cy}, 20, s.weapons ? 1.0f : 0.35f);
  value(s.weapons ? "Activées" : "Désactivées", cy, s.weapons ? kLight : ui::kDisabled);
  cy = line("Ordinateurs", 0);
  const bool max = s.cpuLevel >= kMaxCpuLevel;
  vx = value(TextFormat("Niveau %d%s", s.cpuLevel, max ? " · ×2" : ""), cy, ui::kCream) - 8;
  for (int i = kMaxCpuLevel - 1; i >= 0; --i) {  // ten slanted segments, lit up to the level
    const float x1 = vx - (kMaxCpuLevel - 1 - i) * 11.0f, x0 = x1 - 8;
    const Color c = i < s.cpuLevel ? (max ? Color{255, 122, 82, 255} : ui::kGold) : kTrack;
    ui::quad({x0 + 2, cy - 6}, {x1, cy - 6}, {x1 - 2, cy + 6}, {x0, cy + 6}, c, c, c, c);
  }
  cy = line("Saut avec Haut", 0);
  value(s.tapJump ? "Oui" : "Non", cy, ui::kCream);
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
        int* v = volumeFor(row);
        *v = std::clamp(*v + delta * 5, 0, 100);
      } else {
        audio_.play(Sfx::HitHeavy, 1.0f);
        audio_.play(Sfx::Crowd, 0.8f);
        soundTestAt_ = frames_;
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
  const int rows = settingsRowCount();
  setRow_ = std::min(setRow_, rows - 1);

  if (waitingPad_) {  // rebinding a gamepad button: the next button pressed on any pad
    if (setTab_ != 3 || IsKeyPressed(KEY_ESCAPE) || padAnyPressed(kPadStart) || setRow_ < kActJump ||
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
    if (setTab_ != 3 || padAnyPressed(kPadB) || setRow_ < 0 || setRow_ >= kActionCount || setCol_ > 1) {
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
    if (setRow_ < 0) setTab_ = (setTab_ + d + 4) % 4;
    else if (setTab_ == 3) setCol_ = (setCol_ + d + 4) % 4;
    else changeSetting(setRow_, d);
  }
  const bool padCell = setTab_ == 3 && setRow_ >= kActJump && setRow_ < kActionCount && setCol_ >= 2;
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
    else if (setTab_ == 3) press(setRow_, setCol_);
    else changeSetting(setRow_, 1);
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
      switch (widgetFor(setTab_, r)) {
        case Widget::Slider:
          if (pressed && ui::hovered(ui::grow(trackRect(r), 6))) dragSlider_ = r;
          if (dragSlider_ == r) {
            const Rectangle tr = trackRect(r);
            const int v = int(std::lround(clampf((GetMousePosition().x - tr.x) / tr.width, 0.0f, 1.0f) * 100.0f));
            if (v != *volumeFor(r)) {
              *volumeFor(r) = v;
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
    goTo(settingsReturn_);
  }
}

void App::drawSettings() {
  const float t = time_;
  const Settings& s = settings();
  ui::stageBackdrop(stage_, t, 0.75f);
  ui::quad({0, 0}, {float(kScreenW), 0}, {float(kScreenW), float(kScreenH)}, {0, float(kScreenH)}, withAlpha(ui::kInk, 0.35f),
           withAlpha(ui::kInk, 0.35f), withAlpha(ui::kInk, 0.6f), withAlpha(ui::kInk, 0.6f));

  // The title, "saved automatically", the way back
  ui::tracked("PARAMÈTRES", 64, capsY(61, 65.1f, kT), 65.1f, 0.02f, ui::kCream);
  polyline({{4, 12}, {10, 18}, {20, 6}}, {64, 95}, 14.0f / 24.0f, 2.0f, ui::kGold);
  drawText("Les réglages sont enregistrés automatiquement.", 86, capsY(102, 17.8f, kU), 17.8f, ui::kSky, kU);
  const Rectangle br = backRect();
  cutTL(br, 10, ui::hovered(br) ? mixColor(ui::kNavy, WHITE, 0.12f) : ui::kNavy);
  const float pw = ui::prompt("ÉCHAP", ui::Btn::B, br.x + 10, br.y + 8, 24);
  ui::tracked("RETOUR", br.x + 20 + pw, capsY(br.y + 20, 19.5f, kT), 19.5f, 0.08f, ui::kCream);

  // The tabs: the open one gold and taller, the others night with an azure underline
  for (int i = 0; i < 4; ++i) {
    const bool sel = i == setTab_;
    Rectangle r = tabRect(i);
    if (!sel) r = {r.x, r.y + 6, r.width, 42};
    cutTL(r, 12, sel ? ui::kGold : ui::hovered(r) ? mixColor(ui::kNavy, WHITE, 0.1f) : ui::kNavy);
    if (!sel) DrawRectangleRec({r.x, r.y + r.height - 3, r.width, 3}, withAlpha(ui::kBlue, 0.5f));
    const Color ink = sel ? ui::kInk : ui::kCream;
    tabIcon(i, {r.x + 16, r.y + r.height * 0.5f - 11}, 22, ink);
    ui::tracked(kTabNames[i], r.x + 48, capsY(r.y + r.height * 0.5f, 21.7f, kT), 21.7f, 0.08f, ink);
    if (sel && setRow_ < 0) ui::rim(r, 12, t);
  }
  ui::horizonLine(64, 1216, 173, 2.0f);

  if (setTab_ < 3) {
    panelBox(kPanel);
    for (int r = 0; r < kRowCounts[setTab_]; ++r) {
      const Rectangle rr = rowRect(r);
      const float cy = rowCy(r);
      const bool foc = r == setRow_;
      if (foc) {
        const Color a0 = withAlpha(ui::kBlue, 0.22f), a1 = withAlpha(ui::kBlue, 0.06f);
        ui::quad({rr.x, rr.y}, {rr.x + rr.width, rr.y}, {rr.x + rr.width, rr.y + rr.height}, {rr.x, rr.y + rr.height}, a0, a1, a1, a0);
        DrawRectangleRec({rr.x, rr.y, 3, rr.height}, ui::kGold);
      } else {
        DrawRectangleRec({rr.x, rr.y + rr.height - 1, rr.width, 1}, Color{191, 217, 255, 15});
      }
      drawText(kRows[setTab_][r].label, rr.x + 28, capsY(cy, 24.7f, kU), 24.7f, foc ? ui::kCream : Color{195, 205, 230, 255}, kU);
      switch (widgetFor(setTab_, r)) {
        case Widget::Slider: {
          const int v = *volumeFor(r);
          smallBox(minusRect(r), "−");
          ui::slider(trackRect(r), v / 100.0f, foc, ui::kBlue, t);
          smallBox(plusRect(r), "+");
          const char* txt = TextFormat("%d %%", v);
          drawText(txt, 814 - textWidth(txt, 23.9f, kT), capsY(cy, 23.9f, kT), 23.9f, ui::kCream, kT);
          break;
        }
        case Widget::Toggle:
          ui::toggle(toggleRect(r), toggleOn(setTab_, r), false, t);
          break;
        case Widget::Selector: {
          smallBox(prevRect(r), "", -1);
          smallBox(nextRect(r), "", 1);
          const std::string v = selectorText(setTab_, r);
          const Color vc = setTab_ == 2 && r == 3 && s.cpuLevel >= kMaxCpuLevel ? Color{255, 122, 82, 255} : ui::kCream;
          drawText(v.c_str(), 691 - textWidth(v.c_str(), 21.7f, kT) * 0.5f, capsY(cy, 21.7f, kT), 21.7f, vc, kT);
          break;
        }
        case Widget::Button: {
          const Rectangle b = buttonRect(r);
          const bool danger = setTab_ == 2;
          cutTL(b, 8, ui::hovered(b) ? mixColor(ui::kNavy, WHITE, 0.12f) : ui::kNavy);
          if (danger) DrawRectangleLinesEx(b, 1.0f, withAlpha(kDanger, 0.5f));
          else DrawRectangleRec({b.x, b.y + b.height - 3, b.width, 3}, ui::kBlue);
          const bool playing = setTab_ == 0 && frames_ - soundTestAt_ < 96;
          const char* label = danger ? "RÉINITIALISER" : playing ? "LECTURE…" : "ÉCOUTER";
          const float lw = ui::trackedWidth(label, 17.4f, 0.08f);
          ui::tracked(label, b.x + (b.width - lw) * 0.5f, capsY(cy, 17.4f, kT), 17.4f, 0.08f, danger ? kDanger : ui::kCream);
          break;
        }
      }
    }

    // Beside: the help of the chosen row, then the tab's live card
    const RowInfo& info = setRow_ >= 0 ? kRows[setTab_][setRow_] : kTabHelp[setTab_];
    const auto lines = wrapLines(info.help, 324, 19.2f);
    const float hh = 16 + 19 + 6 + 23 + 6 + lines.size() * 20.3f + 14;
    const Rectangle help{856, 188, 360, hh};
    DrawRectangleRec(help, Color{13, 20, 48, 235});
    DrawRectangleRec({help.x, help.y, 3, help.height}, ui::kGold);
    cardTitle("AIDE", help.x + 18, help.y + 26, ui::kGold);
    drawText(info.label, help.x + 18, capsY(help.y + 52.5f, 23.9f, kT), 23.9f, ui::kCream, kT);
    for (size_t i = 0; i < lines.size(); ++i)
      drawText(lines[i].c_str(), help.x + 18, capsY(help.y + 80 + i * 20.3f, 19.2f, kU), 19.2f, kSoft, kU);
    const float cy = help.y + hh + 12;
    if (setTab_ == 0) audioCard(cy, frames_ - soundTestAt_ < 96, t);
    else if (setTab_ == 1) previewCard(cy, stage_, t);
    else rulesCard(cy, t);
  } else {
    // The controls: one table for both keyboards and the gamepad
    panelBox(kKeysPanel);
    const PadFamily fam = promptFamily();
    const int pad = shownPad(fam);
    ui::tracked("ACTION", 92, capsY(226, 14.1f, kT), 14.1f, 0.14f, kDim);
    for (int c = 0; c < 2; ++c) {
      const char* h = c == 0 ? "CLAVIER 1" : "CLAVIER 2";
      const float w = ui::trackedWidth(h, 17.4f, 0.14f);
      ui::tracked(h, kColX[c] + (kColW[c] - w) * 0.5f, capsY(223, 17.4f, kT), 17.4f, 0.14f, portColor(c));
    }
    std::string padTitle = pad >= 0 ? padModel(pad)
                           : fam == PadFamily::PlayStation ? "Manette PlayStation"
                           : fam == PadFamily::Nintendo    ? "Manette Nintendo"
                                                           : "Manette Xbox";
    padTitle = upperFr(padTitle);
    const float px0 = kColX[2], px1 = kColX[3] + kColW[3];
    float ps = 17.4f;
    while (ps > 12.0f && ui::trackedWidth(padTitle.c_str(), ps, 0.14f) > px1 - px0) ps -= 0.5f;
    const float tw = ui::trackedWidth(padTitle.c_str(), ps, 0.14f);
    ui::tracked(padTitle.c_str(), (px0 + px1 - tw) * 0.5f, capsY(206, ps, kT), ps, 0.14f, ui::kCream);
    const char* sub = pad >= 0 ? "détectée · bouton 1 et bouton 2" : "pas branchée · bouton 1 et bouton 2";
    drawText(sub, (px0 + px1 - textWidth(sub, 15.1f, kU)) * 0.5f, capsY(227, 15.1f, kU), 15.1f, kDim, kU);

    int hl = -1;
    for (int r = 0; r < kActionCount; ++r) {
      const float y = kTableY + r * kRowH, cy = y + kRowH * 0.5f;
      const bool rowFoc = r == setRow_, ult = r == kActUltimate;
      const Rectangle band{82, y, 1116, kRowH};
      if (ult) {
        const Color g0 = Color{255, 181, 71, 41}, g1 = Color{255, 181, 71, 0};
        ui::quad({band.x, band.y}, {band.x + band.width * 0.7f, band.y}, {band.x + band.width * 0.7f, band.y + band.height},
                 {band.x, band.y + band.height}, g0, g1, g1, g0);
        DrawRectangleRec({band.x, band.y, 3, band.height}, ui::kGold);
      } else if (r % 2) {
        DrawRectangleRec(band, Color{10, 16, 36, 128});
      }
      if (rowFoc) {
        const Color a0 = withAlpha(ui::kBlue, 0.22f), a1 = withAlpha(ui::kBlue, 0.06f);
        ui::quad({band.x, band.y}, {band.x + band.width, band.y}, {band.x + band.width, band.y + band.height},
                 {band.x, band.y + band.height}, a0, a1, a1, a0);
        DrawRectangleRec({band.x, band.y, 3, band.height}, ui::kGold);
      }
      drawText(actionName(r), 92, capsY(cy, 20.6f, kU), 20.6f, ult ? kLight : ui::kCream, kU);
      if (ult)
        drawText("après la Clash Orb", 100 + textWidth(actionName(r), 20.6f, kU), capsY(cy, 15.1f, kU), 15.1f, kLight, kU);
      for (int c = 0; c < 4; ++c) {
        const Rectangle k = keyRect(r, c);
        const bool foc = rowFoc && c == setCol_, wait = foc && (c < 2 ? waitingKey_ : waitingPad_);
        const bool fixed = c >= 2 && r < kActJump;
        const int b = c >= 2 && !fixed ? boundButton(pad, r, c - 2) : -1;
        const float rad = c < 2 ? 5.0f : k.height * 0.5f;
        if (wait) {
          ui::roundRect(k, rad, ui::kGold, ui::kGold);
          drawText("…", k.x + (k.width - textWidth("…", 16.3f, kT)) * 0.5f, capsY(k.y + k.height * 0.5f, 16.3f, kT), 16.3f, ui::kInk, kT);
        } else if (fixed) {  // moving on a pad: always the left stick or the d-pad
          ui::roundLines(k, rad, 1.0f, Color{130, 144, 179, 102});
          const ui::Btn g = c == 2 ? ui::Btn::LStick : ui::Btn::DPad;
          const float gw = ui::padGlyphWidth(g, 18, fam), gx = k.x + (k.width - gw - 14) * 0.5f;
          lockIcon({gx, k.y + 6}, ui::kDisabled);
          ui::padGlyph(g, gx + 14, k.y + 3, 18, fam);
        } else if (c >= 2 && b < 0) {
          ui::roundLines(k, rad, 1.0f, Color{191, 217, 255, 31});
          drawText("—", k.x + (k.width - textWidth("—", 16.3f, kT)) * 0.5f, capsY(k.y + k.height * 0.5f, 16.3f, kT), 16.3f, ui::kDisabled, kT);
        } else {
          ui::roundRect(k, rad, kWell, kWell);
          ui::roundLines(ui::grow(k, -0.75f), rad, 1.5f, Color{212, 222, 245, 179});
          if (c < 2) {
            const std::string name = keyName(s.keys[c][r]);
            float ks = 16.3f;
            while (ks > 11.0f && ui::trackedWidth(name.c_str(), ks, 0.04f) > k.width - 12) ks -= 0.5f;
            const float kw = ui::trackedWidth(name.c_str(), ks, 0.04f);
            ui::tracked(name.c_str(), k.x + (k.width - kw) * 0.5f, capsY(k.y + k.height * 0.5f, ks, kT), ks, 0.04f, ui::kCream);
          } else {
            const ui::Btn g = ui::btnFor(b);
            ui::padGlyph(g, k.x + (k.width - ui::padGlyphWidth(g, 18, fam)) * 0.5f, k.y + 3, 18, fam);
          }
        }
        if (foc) {
          if (c >= 2) hl = b;
          if (!wait) ui::rim(k, rad, t);
        }
      }
    }
    // Reset: each keyboard, the gamepad, everything
    const float ry = resetRect(0).y;
    ui::tracked("RÉINITIALISER", 92, capsY(ry + 13, 14.1f, kT), 14.1f, 0.14f, kDim);
    const char* resets[4] = {"CLAVIER 1", "CLAVIER 2", "MANETTE", "TOUT"};
    for (int c = 0; c < 4; ++c) {
      const Rectangle b = resetRect(c);
      const bool foc = setRow_ == kActionCount && setCol_ == c, all = c == 3;
      cutTL(b, 8, ui::hovered(b) ? mixColor(ui::kNavy, WHITE, 0.12f) : ui::kNavy);
      if (all) DrawRectangleLinesEx(b, 1.0f, withAlpha(kDanger, 0.5f));
      const float w = ui::trackedWidth(resets[c], 15.2f, 0.08f);
      ui::tracked(resets[c], b.x + (b.width - w) * 0.5f, capsY(b.y + 13, 15.2f, kT), 15.2f, 0.08f, all ? kDanger : ui::kCream);
      if (foc) ui::rim(b, 8, t);
    }
    drawText("Attaque + Spéciale = smash · Bas, Bas-avant, Avant + Attaque = smash avant chargé · Bas, Haut + Attaque = "
             "smash haut chargé",
             82, capsY(612, 17.1f, kU), 17.1f, kDim, kU);
    drawText("Manette : le stick droit fait aussi les smashs, START met en pause. Clavier 2 : le pavé numérique marche "
             "aussi (0 à 7).",
             82, capsY(629, 17.1f, kU), 17.1f, kDim, kU);
    (void)hl;
  }

  // Bottom: the version, what the keys do here, a note
  drawText(TextFormat("Clash Horizon · %s", gameVersion()), 64, capsY(679.5f, 16.4f, kU), 16.4f, ui::kDisabled, kU);
  struct Hint { const char* key; ui::Btn pad; const char* label; };
  std::vector<Hint> hints;
  const char* foot = "Changer d'onglet : GAUCHE/DROITE sur la ligne des onglets.";
  if (setRow_ < 0) {
    hints = {{"GAUCHE/DROITE", ui::Btn::DPadLR, "Onglet"}, {"BAS", ui::Btn::DPadDown, "Réglages"}, {"ÉCHAP", ui::Btn::B, "Retour"}};
  } else if (setTab_ == 3) {
    hints = {{"FLÈCHES", ui::Btn::DPad, "Naviguer"}, {"ENTRÉE", ui::Btn::A, "Changer"}};
    if (setCol_ >= 2 && setRow_ >= kActJump && setRow_ < kActionCount) hints.push_back({"K", ui::Btn::X, "Effacer"});
    hints.push_back({"ÉCHAP", ui::Btn::B, "Retour"});
    foot = setRow_ < kActionCount ? "Une touche ou un bouton déjà utilisé est échangé." : "Touches et boutons d'origine.";
  } else {
    hints = {{"HAUT/BAS", ui::Btn::DPadUD, "Choisir"}, {"GAUCHE/DROITE", ui::Btn::DPadLR, "Modifier"}, {"ÉCHAP", ui::Btn::B, "Retour"}};
  }
  float total = 0;
  for (const Hint& h : hints) total += ui::promptWidth(h.key, h.pad, 24) + 6 + textWidth(h.label, 17.8f, kU) + 16;
  float hx = 1216 - (total - 16);
  for (const Hint& h : hints) {
    hx += ui::prompt(h.key, h.pad, hx, 664, 24) + 6;
    drawText(h.label, hx, capsY(676, 17.8f, kU), 17.8f, kSoft, kU);
    hx += textWidth(h.label, 17.8f, kU) + 16;
  }
  drawText(foot, 1216 - textWidth(foot, 16.4f, kU), capsY(701.5f, 16.4f, kU), 16.4f, ui::kDisabled, kU);

  // Waiting for a new key or button
  if (waitingKey_ || waitingPad_) {
    DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(ui::kInk, 0.72f));
    const bool padWait = waitingPad_;
    const Rectangle m{360, padWait ? 160.0f : 270.0f, 560, padWait ? 400.0f : 180.0f};
    ui::bevel(m, 18, ui::kNavyDeep, ui::kNavyDeep);
    ui::bevelLines(ui::grow(m, -1), 17, 2.0f, ui::kGold);
    const int row = std::clamp(setRow_, 0, kActionCount - 1);
    const std::string title = upperFr(std::string(padWait ? "Nouveau bouton : " : "Nouvelle touche : ") + actionName(row));
    float ts = 36.9f;
    while (ts > 22.0f && textWidth(title.c_str(), ts, kT) > m.width - 40) ts -= 1.0f;
    drawText(title.c_str(), 640 - textWidth(title.c_str(), ts, kT) * 0.5f, capsY(m.y + 44, ts, kT), ts, ui::kCream, kT);
    const char* port = padWait ? (setCol_ == 2 ? "Manette · bouton 1" : "Manette · bouton 2") : setCol_ == 0 ? "Clavier 1" : "Clavier 2";
    const Color pc = padWait ? ui::kCream : portColor(setCol_);
    drawText(port, 640 - textWidth(port, 21.7f, kT) * 0.5f, capsY(m.y + 78, 21.7f, kT), 21.7f, pc, kT);
    const float blink = std::fmod(t, 1.0f) < 0.5f ? 1.0f : 0.3f;
    drawText("• • •", 640 - textWidth("• • •", 30.4f, kT) * 0.5f, capsY(m.y + 108, 30.4f, kT), 30.4f, withAlpha(ui::kGold, blink), kT);
    if (padWait) {
      const char* a = "Appuie sur un bouton de la manette  —  ";
      const char* b = " pour annuler";
      const float gw = ui::padGlyphWidth(ui::Btn::Start, 22, promptFamily());
      float x = 640 - (textWidth(a, 20.6f, kU) + gw + textWidth(b, 20.6f, kU)) * 0.5f;
      drawText(a, x, capsY(m.y + 140, 20.6f, kU), 20.6f, kSoft, kU);
      x += textWidth(a, 20.6f, kU);
      ui::padGlyph(ui::Btn::Start, x, m.y + 129, 22, promptFamily());
      drawText(b, x + gw, capsY(m.y + 140, 20.6f, kU), 20.6f, kSoft, kU);
      // The controller, its button of the moment circled
      const PadFamily fam = promptFamily();
      const int b0 = boundButton(shownPad(fam), row, std::clamp(setCol_ - 2, 0, 1));
      drawController(fam, {640, m.y + 275}, 0.5f, b0, t);
    } else {
      const char* h = "Appuie sur une touche  —  ÉCHAP pour annuler";
      drawText(h, 640 - textWidth(h, 20.6f, kU) * 0.5f, capsY(m.y + 140, 20.6f, kU), 20.6f, kSoft, kU);
    }
  }
}
