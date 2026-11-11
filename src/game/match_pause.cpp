// The pause menu (« Ligne d'horizon »): the menu and the fight's state on the left, the move list of whoever paused on
// the right (bare hands, weapon A, weapon B), the fighter standing beside it.
#include <cstdio>
#include <cstring>

#include "game/app.hpp"
#include "game/fighter_art.hpp"
#include "game/fx.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
constexpr FontKind kT = FontKind::Title, kU = FontKind::Ui;
constexpr Color kGoldLight{111, 227, 255, 255};  // « Épure »: the accent is cyan
constexpr Color kDanger{255, 154, 122, 255};

// What the move list says of each move (docs/refonte/FICHES.md; the bare hands' combo: its martial art, §1.7).
struct KitText {
  const char* combo;     // the 3-hit combo
  const char* skill[4];  // neutral, side, up, down
  const char* ultName;   // as the fiche writes it
  const char* ult;
};
struct FighterText {
  const char* id;
  KitText kit[3];  // bare hands, weapon A, weapon B
};
const FighterText kMoveText[] = {
    {"p01",
     {{"Garde basse et fluide : frappes à main ouverte, coudes, balayages.",
       {"Une sphère d'ombre lente.", "Il traverse l'adversaire en fondu et le frappe dans le dos.",
        "Téléportation vers le haut.", "Un contre."},
       "Transformation T01", "Aura de flammes violettes, yeux violets."},
      {"Coupe horizontale, coupe montante, grande coupe descendante.",
       {"Un croissant d'énergie qui file au ras du sol.", "Ruée et coupe éclair.", "Coupe en spirale qui s'élève.",
        "Contre puis coupe."},
       "Mille lunes", "Il disparaît, des dizaines de coupes violettes zèbrent la zone, puis il réapparaît en rengainant."},
      {"Entaille gauche, entaille droite, double estoc en X.",
       {"Deux dagues lancées, qui reviennent.", "Ruée à 5 coups.", "Tourbillon vertical.",
        "Un piège au sol qui explose au contact."},
       "Danse du néant", "L'adversaire est enfermé dans une sphère d'ombre et frappé de tous côtés, final en X."}}},
    {"p02",
     {{"Coups de pied rapides et acrobatiques : circulaire, retourné, tornade, hache.",
       {"Une sphère de givre tourbillonnante lancée droit devant.", "Elle glisse, frappe du pied et laisse du givre.",
        "Elle monte sur un pilier qui jaillit.", "Renvoie les projectiles."},
       "Transformation T02", "Aura de givre bleu, tenue éclaircie de blanc, cristaux flottants."},
      {"Estoc, double estoc, balayage circulaire.",
       {"Lancée, elle revient.", "Ruée lance en avant.", "Elle monte en tournoyant.",
        "Des pics sortent du sol devant elle."},
       "Zéro absolu", "Elle plante la lance, une large zone gèle puis éclate en cristaux."},
      {"Coup de sceptre, revers, l'étoile éclate.",
       {"Un projectile en arc.", "Trois étoiles orbitent, puis partent devant.", "Téléportation en étincelles.",
        "Un bouclier bref qui repousse."},
       "Pluie d'étoiles", "Des étoiles de glace tombent sur toute l'arène."}}},
    {"p03",
     {{"Coudes, genoux, tibias, teep, genou sauté.",
       {"Une boule de feu courte.", "Coup d'épaule enflammé.", "Il monte dans une spirale de feu.",
        "Une explosion au sol autour de lui."},
       "Transformation T03", "Aura de flammes rouges, cornes et yeux d'énergie rouge."},
      {"Taille lourde, revers, frappe au sol.",
       {"Une vague de feu au sol.", "Ruée qui casse les boucliers.", "Une coupe montante qui l'emporte.",
        "Il plante l'épée, une onde de choc part."},
       "Exécution écarlate", "Un coup vertical géant fend l'arène, dans une colonne de flammes."},
      {"Coupe enflammée, coupe en croix, frappe explosive.",
       {"Une boule de feu lancée de la lame.", "Une ruée qui laisse des flammes au sol.",
        "Il monte, entouré d'un oiseau de feu.", "Un anneau de flammes autour de lui."},
       "Apocalypse", "Il lance une sphère d'énergie géante, dorée, qui explose au contact."}}},
    {"p07",
     {{"Direct, gyaku-zuki, mae-geri, yoko-geri, tranchant de la main.",
       {"Un projectile rapide.", "Un pas-éclair à travers l'adversaire, et un coup.",
        "Un coup de pied qui l'emporte vers le haut.", "Une décharge autour d'elle."},
       "Tempête écarlate", "Un orage de foudres rouges."},
      {"Coupe, seconde coupe, estoc.",
       {"Une onde rouge.", "Ruée foudroyante et coupe éclair.", "Un saut chargé de foudre qui frappe en montant.",
        "Un contre."},
       "Cent éclairs", "Ruée en zigzag à travers l'arène."},
      {"Coupe électrique, coupe, estoc foudroyant.",
       {"Une lance de foudre lancée droit devant.", "Ruée et trois coupes rouges.",
        "Une spirale de coupes qui monte.", "La foudre jaillit du sol devant elle."},
       "Jugement rouge", "Une foudre géante s'abat sur l'adversaire."}}},
    {"p08",
     {{"Coups de poing en chaîne, paumes, griffe du tigre, coup de pied de la grue.",
       {"Une plume de lumière lancée.", "Une ruée de vent.", "Des ailes de vent l'emportent vers le haut.",
        "Une barrière qui renvoie les projectiles."},
       "Transformation T08", "Aura blanc-bleu, yeux rouges."},
      {"Coupe, seconde coupe, coupe en croix.",
       {"Une vague de lumière.", "Une ruée.", "Une coupe montante dans la lumière.", "Un contre."},
       "Jugement céleste", "Une pluie d'épées de lumière s'abat sur chaque adversaire."},
      {"Estoc, balayage, estoc tournant.",
       {"Une lance de lumière lancée.", "Ruée lance en avant.", "Il monte dans un rayon de lumière.",
        "Une colonne de lumière devant lui."},
       "Lances divines", "Une pluie de lances dorées."}}},
};

const KitText* kitText(const CharacterDef& d, int kit) {
  for (const auto& t : kMoveText)
    if (d.id == t.id) return &t.kit[kit];
  return nullptr;
}

// The move of a kit (0 bare hands, 1 weapon A, 2 weapon B): the weapon's own, else the bare hands'.
const MoveDef& kitMove(const CharacterDef& d, int kit, int id) {
  if (kit > 0)
    for (const auto& [mid, mv] : d.weapons[kit - 1].moves)
      if (mid == id) return mv;
  return d.moves[id];
}

// The lines of `text` that fit in `width`.
std::vector<std::string> wrap(const std::string& text, float width, float size) {
  std::vector<std::string> lines;
  std::string line, word;
  for (size_t i = 0; i <= text.size(); ++i) {
    if (i < text.size() && text[i] != ' ') {
      word += text[i];
      continue;
    }
    const std::string next = line.empty() ? word : line + " " + word;
    if (!line.empty() && textWidth(next.c_str(), size, kU) > width) {
      lines.push_back(line);
      line = word;
    } else {
      line = next;
    }
    word.clear();
  }
  if (!line.empty()) lines.push_back(line);
  return lines;
}

// A direction on a key cap: 0 side (an arrow pointing right), 1 up, 2 down.
float arrowKey(int dir, float x, float y, float h) {
  const float w = ui::keycap("", x, y, h);
  const Vector2 c{x + w * 0.5f, y + h * 0.5f};
  const float s = h * 0.2f;
  const Vec2 d = dir == 0 ? Vec2{1, 0} : dir == 1 ? Vec2{0, -1} : Vec2{0, 1}, n{-d.y, d.x};
  const Vec2 tip = W(c) + d * (s * 1.4f), back = W(c) - d * (s * 1.4f), base = W(c) + d * (s * 0.1f);
  DrawLineEx(V(back), V(base), std::max(2.0f, h * 0.09f), ui::kCream);
  DrawTriangle(V(tip), V(base + n * s), V(base - n * s), ui::kCream);
  DrawTriangle(V(tip), V(base - n * s), V(base + n * s), ui::kCream);
  return w;
}

Rectangle pauseRect(int i, int n) {
  const float h = n > 5 ? 34.0f : 52.0f, gap = n > 5 ? 5.0f : 8.0f;
  return {64, 232 + i * (h + gap), 340, h};
}
constexpr Rectangle kPanel{440, 48, 580, 616};
Rectangle tabRect(int i) { return {kPanel.x + 26 + i * 178.0f, kPanel.y + 98, 172, 48}; }
Rectangle keysRect(int k, const char* label0, const char* label1) {
  const float x0 = 64 + ui::trackedWidth("TOUCHES", 13, 0.18f) + 10;
  const float w0 = textWidth(label0, 15, kT) + 24, w1 = textWidth(label1, 15, kT) + 24;
  return k == 0 ? Rectangle{x0, 674, w0, 30} : Rectangle{x0 + w0 + 2, 674, w1, 30};
}
bool isSetting(MatchScreen::PauseItem k) {
  using P = MatchScreen::PauseItem;
  return k == P::Dummy || k == P::InfiniteUlt || k == P::ZeroPercent || k == P::Fight || k == P::Boxes || k == P::Weapon;
}
}  // namespace

const char* MatchScreen::dummyName(const CpuBrain& b, int level) {
  switch (b.dummy()) {
    case CpuBrain::kDummyStand: return "IMMOBILE";
    case CpuBrain::kDummyJump: return "SAUTE";
    case CpuBrain::kDummyShield: return "BOUCLIER";
    default: return TextFormat("ORDINATEUR (niv. %d)", level);
  }
}

std::vector<std::pair<MatchScreen::PauseItem, std::string>> MatchScreen::pauseItems() const {
  using P = PauseItem;
  std::vector<std::pair<P, std::string>> v{{P::Resume, "REPRENDRE"}, {P::Moves, "LISTE DES COUPS"}};
  if (training()) {
    int dummy = -1;
    for (const auto& f : m_.fighters)
      if (f.cpu) dummy = f.id;
    auto onOff = [](bool b) { return std::string(b ? "OUI" : "NON"); };
    const Fighter& me = m_.fighters[0];
    v.push_back({P::Reset, "RÉINITIALISER LES POSITIONS"});
    v.push_back({P::Dummy, std::string("MANNEQUIN : ") + (dummy >= 0 ? dummyName(brains_[dummy], m_.fighters[dummy].cpuLevel) : "-")});
    v.push_back({P::InfiniteUlt, "ULTI INFINIE : " + onOff(m_.trainInfiniteUlt)});
    v.push_back({P::ZeroPercent, "RESTER À 0 % : " + onOff(m_.trainZeroPercent)});
    v.push_back({P::Fight, "MODE COMBAT (3 VIES) : " + onOff(m_.trainFight)});
    v.push_back({P::Boxes, "ZONES DE COUP : " + onOff(debug_)});
    v.push_back({P::Weapon, upperFr("ARME " + std::to_string((dropSlot_ & 1) + 1) + "/2 : " + me.def->weapons[dropSlot_ & 1].name)});
  }
  v.push_back({P::Options, "OPTIONS"});
  v.push_back({P::Quit, quitAsk_ ? "VRAIMENT QUITTER ?" : training() ? "QUITTER L'ENTRAÎNEMENT" : "QUITTER LE COMBAT"});
  return v;
}

const Fighter& MatchScreen::listFighter() const {
  for (const auto& f : m_.fighters)
    if (f.port == pausedBy_ && !f.cpu) return f;
  for (const auto& f : m_.fighters)
    if (!f.cpu) return f;
  return m_.fighters[0];
}

void MatchScreen::openPause(int port) {
  paused_ = true;
  pauseSel_ = 0;
  quitAsk_ = false;
  pausedBy_ = port;
  const Fighter& f = listFighter();
  listTab_ = f.weapon + 1;  // the kit in hand
  keysDevice_ = f.cpu ? kKeyboard1 : devices_[f.port & 3];
}

// The two choices of the "TOUCHES" switch: the keys of whoever paused, or the other kind of device.
int MatchScreen::keysChoice(int k) const {
  const Fighter& f = listFighter();
  const int own = f.cpu ? kKeyboard1 : devices_[f.port & 3];
  if (k == 0) return own;
  if (own >= kPad1) return kKeyboard1;
  for (int p = kPad1; p < kDeviceCount; ++p)
    if (deviceAvailable(p)) return p;
  return kPad1;
}

MatchScreen::Result MatchScreen::updatePause() {
  using P = PauseItem;
  MenuNav nav = readMenuNav();
  const auto items = pauseItems();
  const int n = int(items.size());
  const int before = pauseSel_;
  if (nav.up) pauseSel_ = (pauseSel_ + n - 1) % n;
  if (nav.down) pauseSel_ = (pauseSel_ + 1) % n;
  bool click = false;
  int clickSide = 0;  // a click on the left (-1) or right (+1) end of a training setting
  for (int i = 0; i < n; ++i) {
    const Rectangle r = pauseRect(i, n);
    if (!ui::hovered(r)) continue;
    if (ui::mouseMoved()) pauseSel_ = i;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      pauseSel_ = i;
      click = true;
      const float mx = GetMousePosition().x;
      if (isSetting(items[i].first)) clickSide = mx < r.x + 50.0f ? -1 : mx > r.x + r.width - 50.0f ? 1 : 0;
    }
  }
  if (pauseSel_ != before) {
    audio_.play(Sfx::MenuMove);
    quitAsk_ = false;
  }
  bool unpause = padAnyPressed(kPadB);
  for (const auto& f : m_.fighters)
    if (!f.cpu && devicePausePressed(devices_[f.port])) unpause = true;
  if (unpause) {
    resume();
    audio_.play(Sfx::MenuBack);
    return Result::Running;
  }
  const P kind = items[pauseSel_].first;
  const bool setting = isSetting(kind);
  // The move list: its tabs with LB / RB, a click, or left / right off the training settings; whose keys it shows
  int tab = listTab_;
  if (padAnyPressed(kPadLB) || (!setting && nav.left)) tab = (tab + 2) % 3;
  if (padAnyPressed(kPadRB) || (!setting && nav.right)) tab = (tab + 1) % 3;
  for (int t = 0; t < 3; ++t)
    if (ui::clicked(tabRect(t))) tab = t;
  if (tab != listTab_) {
    listTab_ = tab;
    audio_.play(Sfx::MenuMove);
  }
  const std::string k0 = deviceName(keysChoice(0)), k1 = deviceName(keysChoice(1));
  for (int k = 0; k < 2; ++k)
    if (ui::clicked(keysRect(k, k0.c_str(), k1.c_str())) && keysDevice_ != keysChoice(k)) {
      keysDevice_ = keysChoice(k);
      audio_.play(Sfx::MenuMove);
    }

  const bool left = nav.left || clickSide < 0, right = nav.right || clickSide > 0;
  const bool activate = nav.confirm || (click && clickSide == 0);
  if (setting && (kind == P::Weapon ? left || right : activate || left || right)) {
    switch (kind) {
      case P::Dummy:
        for (auto& b : brains_) b.setDummy((b.dummy() + (left ? -1 : 1) + CpuBrain::kDummyCount) % CpuBrain::kDummyCount);
        break;
      case P::InfiniteUlt: m_.trainInfiniteUlt = !m_.trainInfiniteUlt; break;
      case P::ZeroPercent: m_.trainZeroPercent = !m_.trainZeroPercent; break;
      case P::Fight:  // a 3-stock fight against the dummy, starting over at each end
        m_.setTrainFight(!m_.trainFight);
        std::fill(combo_.begin(), combo_.end(), 0);
        std::fill(prevPercent_.begin(), prevPercent_.end(), 0.0f);
        break;
      case P::Boxes: debug_ = !debug_; break;
      default: dropSlot_ ^= 1; break;  // the weapon: its arrows (ENTRÉE or a click in the middle drops it)
    }
    audio_.play(Sfx::MenuMove);
    return Result::Running;
  }
  if (!activate) return Result::Running;
  audio_.play(Sfx::MenuSelect);
  switch (kind) {
    case P::Resume: resume(); break;
    case P::Moves: listTab_ = (listTab_ + 1) % 3; break;
    case P::Reset:
      m_.resetPositions();
      std::fill(combo_.begin(), combo_.end(), 0);
      std::fill(prevPercent_.begin(), prevPercent_.end(), 0.0f);
      resume();
      break;
    case P::Weapon:  // the weapon falls in front of the player (one it has: back to bare hands)
      if ((m_.fighters[0].weapons >> (dropSlot_ & 1)) & 1) m_.loseWeapon(m_.fighters[0]);
      m_.dropWeaponFor(0, dropSlot_);
      resume();
      break;
    case P::Options: return Result::Options;
    case P::Quit:
      if (quitAsk_) return Result::Quit;
      quitAsk_ = true;  // asked once more
      break;
    default: break;
  }
  return Result::Running;
}

void MatchScreen::drawMoveList(const Fighter& f) {
  const CharacterDef& d = *f.def;
  const Rectangle P = kPanel;
  const Color aura = auraColor(d.hero);
  ui::bevel(P, 22, withAlpha(ui::kNavyDeep, 0.95f), withAlpha(ui::kNavyDeep, 0.95f));
  ui::bevelLines(ui::grow(P, -0.5f), 22, 1.0f, withAlpha(ui::kSky, 0.16f));

  // Who: the face, "LISTE DES COUPS", its number and fiche
  const float x = P.x + 26, y = P.y + 22, w = P.width - 52;
  const Rectangle face{x, y, 64, 64};
  ui::bevel(face, 10, mixColor(aura, ui::kInk, 0.45f), mixColor(aura, ui::kInk, 0.7f));
  drawPortraitIn(d, f.palette, {face.x + 32, face.y + 38}, 82.0f, face, 0.0f, time_);
  DrawTriangle({face.x, face.y}, {face.x, face.y + 10}, {face.x + 10, face.y}, withAlpha(ui::kNavyDeep, 0.95f));
  DrawRectangleRec({face.x, face.y + face.height - 3, face.width, 3}, aura);
  const Fiche fi = fiche(d);
  ui::tracked("LISTE DES COUPS", x + 80, capsY(y + 9, 13, kT), 13, 0.22f, ui::kGold);
  const std::string who = "N° " + d.id.substr(1) + " · " + fi.nick;
  float ns = 33.0f;
  while (ns > 22.0f && textWidth(who.c_str(), ns, kT) > w - 80) ns -= 1.0f;
  drawText(who.c_str(), x + 80, capsY(y + 31, ns, kT), ns, ui::kCream, kT);
  std::string meta = fi.art + (fi.style.empty() ? "" : " · " + fi.style);
  if (!d.forms.empty()) meta += " · Transformation T" + d.id.substr(1);
  drawText(meta.c_str(), x + 80, capsY(y + 54, 18, kU), 18, ui::kSoft, kU);

  // The tabs: bare hands, weapon A, weapon B ("EN MAIN": the kit it fights with now)
  for (int t = 0; t < 3; ++t) {
    const Rectangle r = tabRect(t);
    const bool on = t == listTab_, hot = ui::hovered(r);
    DrawRectangleRec(r, on ? Color{27, 39, 84, 255} : hot ? Color{18, 26, 56, 255} : Color{10, 16, 36, 255});
    DrawRectangleRec({r.x, r.y + r.height - 3, r.width, 3}, on ? ui::kGold : withAlpha(ui::kSky, 0.1f));
    const Vector2 ic{r.x + 25, r.y + 23};
    if (t == 0) ui::diamond(ic, 7.0f, WHITE);
    else drawWeaponIcon(d, t - 1, W(ic), 30.0f);
    const char* sub = t == 0 ? "Sans arme" : t == 1 ? "Arme A" : "Arme B";
    drawText(sub, r.x + 46, capsY(r.y + 15, 15, kU), 15, on ? kGoldLight : ui::kDisabled, kU);
    std::string label = t == 0 ? "MAINS NUES" : upperFr(d.weapons[t - 1].name);
    float ls = 18.5f;
    while (ls > 13.0f && ui::trackedWidth(label.c_str(), ls, 0.04f) > r.width - 54) ls -= 0.5f;
    ui::tracked(label.c_str(), r.x + 46, capsY(r.y + 32, ls, kT), ls, 0.04f, on ? ui::kCream : ui::kMuted);
    if (t == f.weapon + 1) {
      const float cw = ui::trackedWidth("EN MAIN", 11, 0.08f) + 10;
      DrawRectangleRec({r.x + r.width - 6 - cw, r.y + 5, cw, 15}, ui::kBlue);
      ui::tracked("EN MAIN", r.x + r.width - 1 - cw, capsY(r.y + 12.5f, 11, kT), 11, 0.08f, ui::kInk);
    }
  }

  // What the kit is, then its moves: the combo, the four skills, the ultimate (from the Clash Orb only)
  const int kit = listTab_;
  const KitText* tx = kitText(d, kit);
  std::string caption = kit == 0 ? "Sans arme : " + fi.art + "." : d.weapons[kit - 1].desc + ".";
  if (kit == 0 && !d.forms.empty()) caption += " L'ultime déclenche la transformation.";
  drawText(caption.c_str(), x, capsY(y + 146, 18, kU), 18, ui::kMuted, kU);

  const int dev = keysDevice_ >= 0 ? keysDevice_ : kKeyboard1;
  const bool pad = dev >= kPad1;
  float ry = y + 164;
  for (int row = 0; row < 6; ++row) {
    const bool ult = row == 5;
    const int id = row == 0 ? Jab : row == 5 ? FinalSmash : NSpecial + row - 1;
    const MoveDef& mv = kitMove(d, kit, id);
    std::string name = row == 0 ? "Enchaînement" : ult && tx ? tx->ultName : mv.name;
    if (ult && !tx) name = mv.name;
    const char* desc = !tx ? "" : row == 0 ? tx->combo : ult ? tx->ult : tx->skill[row - 1];
    const auto lines = wrap(desc, w - 24 - 128 - 14, 18);
    const float h = std::max(56.0f, 12 + 26 + float(lines.size()) * 18.5f + 6);
    const Rectangle r{x, ry, w, h};
    if (ult) {
      ui::quad({r.x, r.y}, {r.x + r.width * 0.8f, r.y}, {r.x + r.width * 0.8f, r.y + r.height}, {r.x, r.y + r.height},
               withAlpha(ui::kGold, 0.18f), withAlpha(ui::kGold, 0.0f), withAlpha(ui::kGold, 0.0f), withAlpha(ui::kGold, 0.18f));
      DrawRectangleRec({r.x, r.y, 3, r.height}, ui::kGold);
    } else {
      DrawRectangleRec({r.x, r.y + r.height - 1, r.width, 1}, withAlpha(ui::kSky, 0.08f));
    }
    // The keys: a direction, "+", the button (three times for the combo)
    float kx = r.x + 12;
    const float kh = 30, ky = r.y + (r.height - kh) * 0.5f;
    if (row >= 2 && row <= 4) {
      kx += arrowKey(row - 2, kx, ky, kh) + 4;
      drawText("+", kx + 2, capsY(ky + kh * 0.5f, 18, kT), 18, ui::kDisabled, kT);
      kx += textWidth("+", 18, kT) + 8;
    }
    const int action = row == 0 ? kActAttack : ult ? kActUltimate : kActSpecial;
    kx += ui::actionPrompt(dev, action, kx, ky, kh, ult);
    if (row == 0) drawText("×3", kx + 6, capsY(ky + kh * 0.5f, 17, kT), 17, kGoldLight, kT);
    // Its name and what it does
    const float nx = r.x + 12 + 128 + 14;
    float ny = r.y + 6;
    drawText(name.c_str(), nx, capsY(ny + 13, 23, kU), 23, ult ? kGoldLight : ui::kCream, kU);
    if (ult) {  // the ultimate comes from the Clash Orb only
      const float cx = nx + textWidth(name.c_str(), 23, kU) + 10, cw = 20 + ui::trackedWidth("APRÈS LA CLASH ORB", 11, 0.1f) + 8;
      const Rectangle chip{cx, ny + 4, cw, 18};
      DrawRectangleLinesEx(chip, 1.0f, withAlpha(ui::kGold, 0.6f));
      drawClashOrb({chip.x + 10, chip.y + 9}, 5.0f, time_, 0);
      ui::tracked("APRÈS LA CLASH ORB", chip.x + 20, capsY(chip.y + 9, 11, kT), 11, 0.1f, kGoldLight);
    }
    ny += 30;
    for (const auto& l : lines) {
      drawText(l.c_str(), nx, capsY(ny + 7, 18, kU), 18, ui::kMuted, kU);
      ny += 18.5f;
    }
    ry += h;
  }

  // How to aim and how to take a weapon
  const float by = P.y + P.height - 22 - 8;
  const char* dirs = pad ? "Directions : stick gauche · côté = gauche ou droite"
                         : TextFormat("Directions : %s %s %s %s · côté = gauche ou droite",
                                      keyName(settings().keys[dev][kActUp]).c_str(), keyName(settings().keys[dev][kActLeft]).c_str(),
                                      keyName(settings().keys[dev][kActDown]).c_str(), keyName(settings().keys[dev][kActRight]).c_str());
  drawText(dirs, x, capsY(by, 17, kU), 17, ui::kMuted, kU);
  const float gw = ui::actionPromptWidth(dev, kActWeapon, 22);
  const char* take = "Prendre son arme :";
  const char* hold = "maintenue 1 s";
  const float hw = textWidth(hold, 17, kU), tw = textWidth(take, 17, kU);
  float hx = x + w - hw;
  drawText(hold, hx, capsY(by, 17, kU), 17, ui::kMuted, kU);
  hx -= gw + 6;
  ui::actionPrompt(dev, kActWeapon, hx, by - 11, 22);
  drawText(take, hx - 6 - tw, capsY(by, 17, kU), 17, ui::kMuted, kU);
}

void MatchScreen::drawPause() {
  // The fight, dimmed: darkest on the left, where the menu stands
  DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(ui::kInk, 0.55f));
  const Color a0 = withAlpha(ui::kInk, 0.94f), a1 = withAlpha(ui::kInk, 0.72f), a2 = withAlpha(ui::kInk, 0.38f);
  const float x1 = kScreenW * 0.34f;
  ui::quad({0, 0}, {x1, 0}, {x1, kScreenH}, {0, kScreenH}, a0, a1, a1, a0);
  ui::quad({x1, 0}, {kScreenW, 0}, {kScreenW, kScreenH}, {x1, kScreenH}, a1, a2, a2, a1);

  const Fighter& lf = listFighter();
  const Color aura = auraColor(lf.def->hero);
  const float beat = 0.55f + 0.45f * (0.5f - 0.5f * std::cos(time_ * 2.0f * kPi / 3.6f));
  vfx::glow({1105, 470}, 140, withAlpha(aura, 0.42f * beat), withAlpha(aura, 0.12f * beat));
  vfx::ring({1105, 470}, 140, 235, withAlpha(aura, 0.12f * beat), withAlpha(aura, 0.0f));

  // Who paused, "PAUSE", where and with which rules
  const auto items = pauseItems();
  const int n = int(items.size());
  ui::quad({66, 51}, {72, 51}, {70, 69}, {64, 69}, ui::kBlue, ui::kBlue, ui::kBlue, ui::kBlue);
  ui::quad({76, 51}, {82, 51}, {80, 69}, {74, 69}, ui::kGold, ui::kGold, ui::kGold, ui::kGold);
  float hx = 92;
  const Fighter* by = nullptr;
  for (const auto& f : m_.fighters)
    if (f.port == pausedBy_ && !f.cpu) by = &f;
  if (by) {
    const char* tag = TextFormat("J%d", by->port + 1);
    const float cw = textWidth(tag, 16, kT) + 16;
    DrawRectangleRec({hx, 49, cw, 22}, slotColor(*by));
    drawText(tag, hx + 8, capsY(60, 16, kT), 16, ui::kInk, kT);
    hx += cw + 10;
  }
  drawText(padLost_[std::max(0, pausedBy_) & 3] && by ? "manette débranchée : le combat attend"
           : by ? "a mis le combat en pause" : training() ? "Entraînement" : "Combat en pause",
           hx, capsY(60, 18, kU), 18, ui::kSky, kU);
  drawText("PAUSE", 62, capsY(119, 113, kT), 113, ui::kCream, kT);
  ui::quad({64, 177}, {234, 177}, {234, 180}, {64, 180}, ui::kBlue, WHITE, WHITE, ui::kBlue);
  ui::quad({234, 177}, {404, 177}, {404, 180}, {234, 180}, WHITE, ui::kGold, ui::kGold, WHITE);
  ui::eclat({234.5f, 178.5f}, 9.0f);
  std::string rules = m_.stage().name;
  if (training()) {
    rules += " · Entraînement";
  } else {
    rules += TextFormat(" · %d vie%s", m_.cfg.stocks, m_.cfg.stocks > 1 ? "s" : "");
    rules += m_.cfg.items && m_.cfg.weapons ? " · Orbe et armes" : m_.cfg.items ? " · Orbe" : m_.cfg.weapons ? " · Armes" : " · Sans orbe ni armes";
  }
  drawText(rules.c_str(), 64, capsY(202, 18, kU), 18, ui::kMuted, kU);

  // The menu: night plates, the chosen one gold with its shimmer; a training setting shows its arrows
  for (int i = 0; i < n; ++i) {
    const Rectangle r = pauseRect(i, n);
    const bool sel = i == pauseSel_;
    const PauseItem kind = items[i].first;
    const bool danger = kind == PauseItem::Quit;
    const Color fill = sel ? (danger && quitAsk_ ? ui::kRed : ui::kGold) : ui::kNavy;
    ui::bevel(r, r.height > 40 ? 14.0f : 10.0f, fill, fill);
    if (sel) ui::shimmer(r, time_, 0.8f);
    const bool arrows = sel && isSetting(kind);
    const char* note = kind == PauseItem::Options ? "Son, image, touches"
                       : kind == PauseItem::Quit ? (quitAsk_ ? "Valider pour quitter" : "Confirmation demandée") : nullptr;
    // A note beside a long label folds onto two lines at its last space
    const char* cut = note ? std::strrchr(note, ' ') : nullptr;
    const std::string n1 = cut ? std::string(note, cut) : "", n2 = cut ? cut + 1 : "";
    const float noteW = note ? textWidth(note, 16, kU) : 0.0f;
    const float foldW = cut ? std::max(textWidth(n1.c_str(), 15, kU), textWidth(n2.c_str(), 15, kU)) : noteW;
    float size = r.height > 40 ? 27.0f : 21.0f;
    const float room = r.width - 36 - (arrows ? 52 : 0) - (note && !arrows ? foldW + 12 : 0);
    while (size > 14.0f && ui::trackedWidth(items[i].second.c_str(), size, 0.06f) > room) size -= 0.5f;
    const float lx = r.x + 18 + (arrows ? 26 : 0), cy = r.y + r.height * 0.5f;
    const float lw = ui::tracked(items[i].second.c_str(), lx, capsY(cy, size, kT), size, 0.06f,
                                 sel ? ui::kInk : danger ? kDanger : ui::kCream);
    if (arrows) {
      ui::arrowTri({r.x + 22, cy}, 14, -1, ui::kInk);
      ui::arrowTri({r.x + r.width - 22, cy}, 14, 1, ui::kInk);
    }
    if (note && !arrows) {
      const float right = r.x + r.width - 18;
      const Color nc = sel ? Color{58, 38, 8, 255} : ui::kDisabled;
      if (!cut || lx + lw + 12 + noteW <= right) {
        drawText(note, right - noteW, capsY(cy, 16, kU), 16, nc, kU);
      } else {
        drawText(n1.c_str(), right - textWidth(n1.c_str(), 15, kU), capsY(cy - 8, 15, kU), 15, nc, kU);
        drawText(n2.c_str(), right - textWidth(n2.c_str(), 15, kU), capsY(cy + 8, 15, kU), 15, nc, kU);
      }
    }
  }

  // The fight's state: each fighter's stocks and damage, the Clash Orb
  if (!training()) {
    float sy = 486;
    ui::tracked("EN JEU", 64, capsY(sy + 7, 13, kT), 13, 0.22f, ui::kMuted);
    sy += 20;
    for (const auto& f : m_.fighters) {
      const Color pc = slotColor(f);
      DrawRectangleRec({64, sy, 340, 28}, withAlpha(ui::kNavyDeep, 0.7f));
      DrawRectangleRec({64, sy, 3, 28}, pc);
      const Rectangle pr{75, sy + 1, 26, 26};
      DrawRectangleRec(pr, mixColor(auraColor(f.def->hero), ui::kInk, 0.45f));
      drawPortraitIn(*f.def, f.palette, {pr.x + 13, pr.y + 16}, 34.0f, pr, 0.0f, time_);
      const char* tag = f.cpu ? "CPU" : TextFormat("J%d", f.port + 1);
      drawText(tag, 109, capsY(sy + 14, 16, kT), 16, pc, kT);
      float dx = 159;
      if (f.eliminated) {
        drawText("K.O.", dx, capsY(sy + 14, 15, kT), 15, ui::kDisabled, kT);
      } else {
        for (int s = 0; s < std::min(std::max(m_.cfg.stocks, f.stocks), 5); ++s) {
          ui::diamond({dx + 5, sy + 14}, 5.5f, pc);
          if (s >= f.stocks) ui::diamond({dx + 5, sy + 14}, 3.8f, ui::kNavyDeep);
          dx += 13;
        }
        if (f.finalSmash) {
          const float cw = ui::trackedWidth("ULTIME PRÊT", 12, 0.08f) + 10;
          DrawRectangleRec({dx + 6, sy + 7, cw, 15}, ui::kGold);
          ui::tracked("ULTIME PRÊT", dx + 11, capsY(sy + 14.5f, 12, kT), 12, 0.08f, ui::kInk);
        }
      }
      if (!f.eliminated) {
        const char* pct = TextFormat("%d %%", int(f.percent));
        drawText(pct, 394 - textWidth(pct, 20.5f, kT), capsY(sy + 14, 20.5f, kT), 20.5f, percentColor(f.percent), kT);
      }
      sy += 32;
    }
    DrawRectangleRec({64, sy, 340, 28}, withAlpha(ui::kNavyDeep, 0.7f));
    drawClashOrb({82, sy + 14}, 7.0f, time_, 0);
    const char* orb = !m_.cfg.items ? "Clash Orb désactivée" : m_.orb.active ? "Clash Orb en jeu" : "Clash Orb : bientôt de retour";
    drawText(orb, 98, capsY(sy + 14, 18, kU), 18, ui::kSoft, kU);
    if (m_.cfg.items && m_.orb.active) {
      const char* hp = TextFormat("%d / 40 PV", int(std::ceil(std::max(0.0f, m_.orb.hp))));
      drawText(hp, 394 - textWidth(hp, 16, kT), capsY(sy + 14, 16, kT), 16, ui::kCream, kT);
    }
  }

  // The fighter, behind the move list so that a wide pose never hides its text
  DrawEllipse(1105, 652, 95, 9, withAlpha(BLACK, 0.55f));
  drawShowcase(*lf.def, lf.palette, restPose(*lf.def, time_), {1105, 650}, -1, 300.0f, time_);
  drawMoveList(lf);

  // An unplugged gamepad
  int lostCount = 0, lostPort = 0;
  for (const auto& f : m_.fighters)
    if (padLost_[f.port]) ++lostCount, lostPort = f.port;
  if (lostCount > 0) {
    const Rectangle b{kPanel.x + 20, 572, kPanel.width - 40, 72};
    ui::bevel(b, 12, {150, 34, 52, 250}, {84, 14, 30, 250});
    const std::string title = lostCount > 1 ? std::string("MANETTES DÉBRANCHÉES")
                                            : std::string(deviceName(devices_[lostPort])) + " DÉBRANCHÉE (JOUEUR " +
                                                  std::to_string(lostPort + 1) + ")";
    drawText(title.c_str(), b.x + 18, capsY(b.y + 22, 22, kT), 22, ui::kCream, kT);
    drawText(lostCount > 1 ? "Rebranchez-les : chacune retrouve son joueur. Ou reprenez sans elles."
                           : "Rebranchez-la : elle retrouve son joueur. Ou reprenez le combat sans elle.",
             b.x + 18, capsY(b.y + 50, 17, kU), 17, {250, 218, 200, 255}, kU);
  }

  // Whose keys the list shows; what the buttons do here
  const std::string k0 = deviceName(keysChoice(0)), k1 = deviceName(keysChoice(1));
  ui::tracked("TOUCHES", 64, capsY(689, 13, kT), 13, 0.18f, ui::kMuted);
  for (int k = 0; k < 2; ++k) {
    const Rectangle r = keysRect(k, k0.c_str(), k1.c_str());
    const bool on = keysDevice_ == keysChoice(k);
    DrawRectangleRec(r, on ? ui::kCream : ui::hovered(r) ? Color{30, 42, 88, 255} : ui::kNavy);
    drawText(k == 0 ? k0.c_str() : k1.c_str(), r.x + 12, capsY(r.y + 15, 15, kT), 15, on ? ui::kInk : ui::kMuted, kT);
  }
  const bool padUi = padPrompts();
  const PadFamily fam = promptFamily();
  float rx = kScreenW - 40;
  auto hint = [&](const char* label, auto keys) {
    rx -= textWidth(label, 17, kU);
    drawText(label, rx, capsY(689, 17, kU), 17, ui::kSoft, kU);
    rx -= 6;
    keys();
    rx -= 18;
  };
  hint("Reprendre", [&] {
    if (padUi) rx -= ui::padGlyph(ui::Btn::Start, rx - ui::padGlyphWidth(ui::Btn::Start, 22, fam), 678, 22, fam);
    else rx -= ui::keycap("Échap", rx - ui::keycapWidth("Échap", 22), 678, 22);
  });
  hint("Onglet", [&] {
    if (padUi) {
      rx -= ui::padGlyph(ui::Btn::RB, rx - ui::padGlyphWidth(ui::Btn::RB, 22, fam), 678, 22, fam) + 4;
      rx -= ui::padGlyph(ui::Btn::LB, rx - ui::padGlyphWidth(ui::Btn::LB, 22, fam), 678, 22, fam);
    } else {
      const std::string l = keyName(settings().keys[kKeyboard1][kActLeft]), r = keyName(settings().keys[kKeyboard1][kActRight]);
      rx -= ui::keycap(r.c_str(), rx - ui::keycapWidth(r.c_str(), 22), 678, 22) + 4;
      rx -= ui::keycap(l.c_str(), rx - ui::keycapWidth(l.c_str(), 22), 678, 22);
    }
  });
}
