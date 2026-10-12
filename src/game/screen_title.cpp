// Title screen ("appuyez sur ENTRÉE") and main menu.
#include "game/app.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
float hash(int i) {
  uint32_t x = uint32_t(i) * 2246822519u + 0x2545F491u;
  x ^= x >> 15;
  x *= 0x2c1b3c6du;
  x ^= x >> 12;
  return float(x & 0xFFFF) / 65535.0f;
}
float easeOutBack(float t) {
  t = clampf(t, 0.0f, 1.0f);
  const float c1 = 1.70158f, c3 = c1 + 1.0f;
  return 1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
}
float easeOut(float t) {
  t = clampf(t, 0.0f, 1.0f);
  return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
}

void drawGear(Vector2 c, float r, float rot, Color col) {
  for (int i = 0; i < 8; ++i) {
    float a = rot + i * kPi / 4.0f;
    DrawRectanglePro({c.x + std::cos(a) * r * 0.86f, c.y + std::sin(a) * r * 0.86f, r * 0.42f, r * 0.42f},
                     {r * 0.21f, r * 0.21f}, a * 180.0f / kPi, col);
  }
  DrawCircleV(c, r * 0.82f, col);
  DrawCircleV(c, r * 0.36f, withAlpha(ui::kInk, 0.55f));
  DrawRing(c, r * 0.5f, r * 0.58f, 0, 360, 32, withAlpha(ui::kInk, 0.25f));
}

void drawTarget(Vector2 c, float r, float time) {
  const Color cream{240, 230, 204, 255}, terra{184, 76, 56, 255};
  const Color cols[3] = {cream, terra, cream};
  DrawCircleV(c, r + 2.5f, ui::kInk);
  for (int i = 0; i < 3; ++i) DrawCircleV(c, r * (1.0f - i * 0.3f), cols[i]);
  DrawCircleV(c, r * 0.14f, terra);
  float wob = std::sin(time * 3.0f) * 4.0f;
  DrawLineEx({c.x + r * 1.2f + wob, c.y - r * 0.9f - wob}, {c.x + r * 0.1f, c.y - r * 0.05f}, 6, {112, 76, 46, 255});
  DrawTriangle({c.x + r * 0.05f, c.y}, {c.x + r * 0.3f, c.y - r * 0.08f}, {c.x + r * 0.1f, c.y - r * 0.25f},
               {196, 200, 190, 255});
}

void drawExit(Vector2 c, float r) {
  DrawRectangleRounded({c.x - r * 0.75f, c.y - r, r * 1.1f, r * 2.0f}, 0.15f, 6, {238, 228, 204, 255});
  DrawRectangleRounded({c.x - r * 0.6f, c.y - r * 0.85f, r * 0.8f, r * 1.7f}, 0.15f, 6, {56, 42, 62, 255});
  DrawCircleV({c.x + r * 0.05f, c.y + r * 0.05f}, r * 0.09f, ui::kGold);
  DrawLineEx({c.x + r * 0.2f, c.y}, {c.x + r * 1.1f, c.y}, r * 0.22f, ui::kGold);
  ui::arrowTri({c.x + r * 1.15f, c.y}, r * 0.55f, 1, ui::kGold);
}

struct Tile {
  Rectangle r;
  float slant;
  Color top, bottom;
  const char* label;
  const char* desc;
};

// Earthy tiles: terracotta, moss, slate blue, dusk plum.
const Tile kTiles[4] = {
    {{60, 126, 640, 474}, 70, {178, 82, 58, 255}, {86, 30, 32, 255}, "COMBAT",
     "Affrontez jusqu'à 3 adversaires, humains ou ordinateurs."},
    {{728, 126, 500, 142}, 21, {98, 136, 76, 255}, {36, 62, 44, 255}, "ENTRAÎNEMENT",
     "Vies infinies, mannequin, compteur de combos : idéal pour apprendre."},
    {{704, 292, 500, 142}, 21, {66, 112, 132, 255}, {24, 46, 62, 255}, "PARAMÈTRES",
     "Son, affichage, règles du jeu, touches et manettes."},
    {{680, 458, 500, 142}, 21, {112, 86, 110, 255}, {46, 34, 52, 255}, "QUITTER",
     "Quitter Super Clash Bros."},
};
}  // namespace

// ---------------------------------------------------------------------------
// Title screen
// ---------------------------------------------------------------------------
void App::updateTitle(const MenuNav& nav) {
  if (screenFrames_ == 22) audio_.play(Sfx::HitHeavy, 0.9f, 0.8f);
  if (screenFrames_ == 24) audio_.play(Sfx::Crowd, 0.5f, 1.1f);
  bool go = nav.confirm || nav.start || IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
  if (go && screenFrames_ > 20) {
    audio_.play(Sfx::Go, 0.8f);
    goTo(Screen::MainMenu);
  }
}

void App::drawTitle() {
  const float t = time_, f = float(screenFrames_);
  const Vector2 center{kScreenW * 0.5f, 232};

  ui::stageBackdrop(2, t, 0.0f);  // the sky islands, slowly drifting
  DrawRectangleGradientV(0, 0, kScreenW, 420, withAlpha(ui::kInk, 0.5f), withAlpha(ui::kInk, 0.0f));
  float pulse = 1.0f + 0.02f * std::sin(t * 2.0f);
  ui::zonaiRing(center, 226 * pulse, t, clampf(f / 30.0f, 0.0f, 1.0f));

  // Ancient stone disc and fighters (they run in from the sides)
  ui::stoneDisc({kScreenW * 0.5f, 614}, 600, 34, t);
  const auto& r = roster();
  const int n = int(r.size());
  const float gap = std::min(174.0f, 1000.0f / std::max(1, n - 1));
  for (int i = 0; i < n; ++i) {
    float enter = easeOut((f - 26.0f - i * 4.0f) / 28.0f);
    const bool left = i < n / 2;
    float side = left ? -1.0f : 1.0f;
    float x = kScreenW * 0.5f + (i - (n - 1) * 0.5f) * gap + side * (1.0f - enter) * 760.0f;
    float hop = std::max(0.0f, std::sin(t * 2.4f + i * 1.3f)) * 9.0f;
    int facing = left ? 1 : -1;
    DrawEllipse(int(x), 610, 40.0f - hop, 8.0f, withAlpha(ui::kInk, 0.3f));
    Pose p = restPose(r[i], t + i);
    drawCharacterSprite(r[i], 0, p, {x, 608 - hop}, facing, std::min(1.3f, 150.0f / r[i].height * (n > 6 ? 1.0f : 1.15f)),
                  1.0f, WHITE, 0.0f, t + i);
  }

  // Logo: slams in, then breathes
  float k = easeOutBack(f / 24.0f);
  float scale = f < 24 ? lerpf(2.6f, 1.0f, k) : 1.0f + 0.012f * std::sin(t * 2.2f);
  float alpha = clampf(f / 10.0f, 0.0f, 1.0f);
  float shakeX = (f >= 24 && f < 36) ? std::sin(f * 3.1f) * (36 - f) * 0.8f : 0.0f;
  if (alpha > 0.0f) {
    float s1 = 46 * scale;
    const char* super = "SUPER";
    drawTextOutlined(super, center.x - textWidth(super, s1, FontKind::Title) * 0.5f + shakeX, 70 - (scale - 1) * 40,
                     s1, withAlpha(ui::kCream, alpha), withAlpha(ui::kUmber, alpha), 4.0f * scale, FontKind::Title);
    float size = 150 * scale;
    float w = ui::spriteWidth(ui::Sprite::Logo, size);
    ui::drawSprite(ui::Sprite::Logo, center.x - w * 0.5f + shakeX, 108 - (scale - 1) * 60, size, alpha);
  }
  // Subtitle ribbon: teal ink, gold edges, rules on both sides
  if (f > 28) {
    float a = clampf((f - 28) / 12.0f, 0.0f, 1.0f);
    Rectangle rib{center.x - 270, 286, 540, 44};
    ui::slanted({rib.x + 6, rib.y + 6, rib.width, rib.height}, 22, withAlpha(BLACK, 0.35f * a), withAlpha(BLACK, 0.35f * a));
    ui::slanted(rib, 22, withAlpha({34, 56, 60, 255}, 0.95f * a), withAlpha(ui::kInk, 0.95f * a));
    ui::slantedOutline(rib, 22, 1.5f, withAlpha(ui::kGold, a));
    const float my = rib.y + rib.height * 0.5f;
    ui::rule(rib.x - 74, rib.x + 4, my, withAlpha(ui::kGold, a));
    ui::rule(rib.x + rib.width - 4, rib.x + rib.width + 74, my, withAlpha(ui::kGold, a));
    drawTextCentered("LE CHOC DES COMBATTANTS", center.x, rib.y + 9, 23, withAlpha(ui::kCream, a));
  }
  // Glints around the logo
  for (int i = 0; i < 6; ++i) {
    float ph = std::fmod(t * 0.8f + hash(i + 40), 1.0f);
    float s = std::sin(ph * kPi) * 14.0f;
    Vector2 p{center.x - 380 + hash(i + 60) * 760, 100 + hash(i + 80) * 170};
    if (f > 30 && s > 1.0f) ui::star(p, s, s * 0.3f, t, withAlpha({255, 244, 210, 255}, 0.85f));
  }
  // Press start
  if (f > 40) {
    float a = 0.55f + 0.45f * std::sin(t * 4.0f);
    const char* txt = "APPUYEZ SUR";
    const float ph = padPrompts() ? 44 : 34;  // a gamepad button is a smaller shape than a key cap
    float total = textWidth(txt, 26) + 14 + ui::promptWidth("ENTRÉE", ui::Btn::Start, ph);
    float x = center.x - total * 0.5f;
    drawTextOutlined(txt, x, 368, 26, withAlpha(ui::kCream, a), withAlpha(ui::kInk, a), 2.5f);
    ui::prompt("ENTRÉE", ui::Btn::Start, x + textWidth(txt, 26) + 14, 381 - ph * 0.5f, ph, true);
  }
  drawTextOutlined("v7.2", kScreenW - 54, kScreenH - 26, 15, ui::kCream, ui::kInk, 2.0f);
  drawTextOutlined("C++ & raylib", 16, kScreenH - 26, 15, ui::kCream, ui::kInk, 2.0f);
  if (f >= 22 && f < 40) DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha({255, 248, 228, 255}, (40 - f) / 18.0f * 0.7f));
}

// ---------------------------------------------------------------------------
// Main menu
// ---------------------------------------------------------------------------
// The device that opens the character select plays player 1 (keyboard or gamepad).
void App::claimP1(int device) {
  padLeft_ = {};
  if (device >= 0) assignDevice(0, device);
}

void App::activateMenu(int item, int device) {
  audio_.play(Sfx::MenuSelect);
  switch (item) {
    case 0:
      training_ = false;
      cssRow_ = 1;
      claimP1(device);
      goTo(Screen::CharSelect);
      break;
    case 1:
      training_ = true;
      cssRow_ = 1;
      cssCol_ = 0;
      claimP1(device);
      goTo(Screen::CharSelect);
      break;
    case 2:
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
  int before = menuSel_;
  if (nav.left && menuSel_ != 0) {
    menuRight_ = menuSel_;
    menuSel_ = 0;
  }
  if (nav.right && menuSel_ == 0) menuSel_ = menuRight_;
  if (nav.up && menuSel_ > 1) menuSel_--;
  if (nav.down && menuSel_ >= 1 && menuSel_ < 3) menuSel_++;
  for (int i = 0; i < 4; ++i) {
    if (!ui::hovered(kTiles[i].r)) continue;
    if (ui::mouseMoved()) menuSel_ = i;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      menuSel_ = i;
      activateMenu(i);
      return;
    }
  }
  if (menuSel_ != 0) menuRight_ = menuSel_;
  if (menuSel_ != before) audio_.play(Sfx::MenuMove, 0.7f);
  if (nav.confirm || nav.start) activateMenu(menuSel_, nav.device);
  else if (nav.back) {
    audio_.play(Sfx::MenuBack);
    goTo(Screen::Title);
  }
}

void App::drawMainMenu() {
  const float t = time_;
  ui::stageBackdrop(2, t, 0.58f);
  ui::header("MENU PRINCIPAL", nullptr, kTiles[0].top, t);
  // small logo, top right
  ui::drawSprite(ui::Sprite::Logo, kScreenW - 360, 30, 52);

  const auto& r = roster();
  for (int i = 0; i < 4; ++i) {
    const Tile& tile = kTiles[i];
    bool sel = i == menuSel_;
    float popIn = easeOut((screenFrames_ - i * 4.0f) / 18.0f);
    Rectangle rr = tile.r;
    rr.x += (1.0f - popIn) * (i == 0 ? -500.0f : 500.0f);
    if (sel) rr = ui::grow(rr, 6.0f + std::sin(t * 6.0f) * 1.5f);
    const float pulse = 0.5f + 0.5f * std::sin(t * 6.0f);
    // shadow, body, sheen
    ui::slanted({rr.x + 10, rr.y + 12, rr.width, rr.height}, tile.slant, withAlpha(BLACK, 0.4f), withAlpha(BLACK, 0.4f));
    Color top = sel ? mixColor(tile.top, ui::kCream, 0.08f) : shade(tile.top, 0.72f);
    Color bottom = sel ? tile.bottom : shade(tile.bottom, 0.7f);
    ui::slanted(rr, tile.slant, withAlpha(top, 0.96f), withAlpha(bottom, 0.96f));
    ui::slanted({rr.x + tile.slant * 0.9f, rr.y, rr.width - tile.slant * 0.9f, rr.height * 0.18f}, tile.slant * 0.18f,
                withAlpha(ui::kCream, 0.14f), withAlpha(ui::kCream, 0.0f));
    // Zonai rune carved in the tile (instead of stripes)
    const bool big = i == 0;
    const Vector2 rc = big ? Vector2{rr.x + rr.width * 0.56f, rr.y + rr.height * 0.4f}
                           : Vector2{rr.x + rr.width - 78, rr.y + rr.height * 0.5f};
    ui::rune(rc, big ? 190.0f : 62.0f, i == 0 ? 0 : i == 1 ? 3 : i == 2 ? 2 : 1, t * (sel ? 0.5f : 0.1f),
             withAlpha(ui::kCream, sel ? 0.1f : 0.06f), big ? 7.0f : 4.0f);
    // inner hairline, frame, corner ornaments
    const Rectangle in = ui::grow(rr, -8);
    ui::slantedOutline(in, tile.slant * in.height / rr.height, 1.0f, withAlpha(ui::kGold, sel ? 0.5f : 0.25f));
    ui::slantedOutline(rr, tile.slant, sel ? 3.0f : 2.5f, sel ? ui::kGold : withAlpha(ui::kInk, 0.9f));
    ui::corners(rr, tile.slant, big ? 30.0f : 18.0f, 3.0f, sel ? ui::kGold : withAlpha(ui::kGold, 0.45f));
    if (sel) {
      ui::slantedOutline(ui::grow(rr, 7), tile.slant, 4, withAlpha(ui::kTeal, 0.18f + 0.16f * pulse));
      ui::corners(ui::grow(rr, 9 + pulse * 3), tile.slant, big ? 34.0f : 20.0f, 3.0f, ui::kGold);
    }

    if (i == 0) {
      ui::glow({rr.x + rr.width * 0.5f, rr.y + rr.height * 0.45f}, 230, withAlpha({255, 226, 170, 255}, sel ? 0.3f : 0.16f), 10);
      const int show[4] = {0, 2, 6, 7};
      for (int k = 0; k < 4; ++k) {
        const CharacterDef& d = r[show[k] % int(r.size())];
        Pose p = restPose(d, t + k);
        float hop = sel ? std::max(0.0f, std::sin(t * 5.0f + k * 1.6f)) * 10.0f : 0.0f;
        float scale = 170.0f / d.height * (d.style == BodyStyle::Brute ? 1.1f : 1.0f);
        float x = rr.x + 120 + k * 140;
        drawCharacterSprite(d, 0, p, {x, rr.y + rr.height - 132 - hop}, k < 2 ? 1 : -1, scale, 1.0f, WHITE, 0.0f, t + k);
      }
      float bounce = sel ? std::fabs(std::sin(t * 4.0f)) * 6.0f : 0.0f;
      ui::drawSprite(ui::Sprite::Combat, rr.x + 36, rr.y + rr.height - 118 - bounce, 92);
      drawTextOutlined("Jusqu'à 4 joueurs  •  Ordinateurs niveaux 1 à 10", rr.x + 40, rr.y + rr.height - 30, 18,
                       ui::kCream, ui::kInk, 2.0f);
    } else {
      float bounce = sel ? std::fabs(std::sin(t * 4.0f)) * 4.0f : 0.0f;
      drawTextOutlined(tile.label, rr.x + tile.slant + 26, rr.y + 44 - bounce, 42, sel ? ui::kCream : Color{226, 218, 198, 255},
                       ui::kInk, 4.0f, FontKind::Title);
      Vector2 ic{rr.x + rr.width - 78, rr.y + rr.height * 0.5f};
      if (i == 1) drawTarget(ic, 36, t);
      if (i == 2) drawGear(ic, 36, t * (sel ? 1.2f : 0.3f), {236, 226, 200, 255});
      if (i == 3) drawExit({ic.x - 10, ic.y}, 32);
    }
  }
  ui::footer({{"FLÈCHES", ui::Btn::Move, "Naviguer"}, {"ENTRÉE", ui::Btn::A, "Choisir"}, {"ÉCHAP", ui::Btn::B, "Retour"}},
             kTiles[menuSel_].desc);
}
