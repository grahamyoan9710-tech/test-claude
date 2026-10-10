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

// The "Clash" emblem: a ring crossed by a lightning bolt.
void emblem(Vector2 c, float r, Color col) {
  DrawRing(c, r * 0.86f, r, 0, 360, 96, col);
  Vector2 p[4] = {{c.x + r * 0.42f, c.y - r * 1.05f}, {c.x - r * 0.18f, c.y - r * 0.02f},
                  {c.x + r * 0.22f, c.y + r * 0.06f}, {c.x - r * 0.46f, c.y + r * 1.08f}};
  float th[3] = {r * 0.2f, r * 0.15f, r * 0.09f};
  for (int i = 0; i < 3; ++i) {
    DrawLineEx(p[i], p[i + 1], th[i], col);
    DrawCircleV(p[i + 1], th[i] * 0.5f, col);
  }
  DrawCircleV(p[0], th[0] * 0.5f, col);
}

void drawGear(Vector2 c, float r, float rot, Color col) {
  for (int i = 0; i < 8; ++i) {
    float a = rot + i * kPi / 4.0f;
    DrawRectanglePro({c.x + std::cos(a) * r * 0.86f, c.y + std::sin(a) * r * 0.86f, r * 0.42f, r * 0.42f},
                     {r * 0.21f, r * 0.21f}, a * 180.0f / kPi, col);
  }
  DrawCircleV(c, r * 0.82f, col);
  DrawCircleV(c, r * 0.36f, withAlpha(BLACK, 0.35f));
}

void drawTarget(Vector2 c, float r, float time) {
  const Color cols[3] = {{255, 255, 255, 255}, {230, 60, 60, 255}, {255, 255, 255, 255}};
  for (int i = 0; i < 3; ++i) DrawCircleV(c, r * (1.0f - i * 0.3f), cols[i]);
  DrawCircleV(c, r * 0.14f, {230, 60, 60, 255});
  float wob = std::sin(time * 3.0f) * 4.0f;
  DrawLineEx({c.x + r * 1.2f + wob, c.y - r * 0.9f - wob}, {c.x + r * 0.1f, c.y - r * 0.05f}, 6, {120, 70, 40, 255});
  DrawTriangle({c.x + r * 0.05f, c.y}, {c.x + r * 0.3f, c.y - r * 0.08f}, {c.x + r * 0.1f, c.y - r * 0.25f},
               {200, 200, 210, 255});
}

void drawExit(Vector2 c, float r) {
  DrawRectangleRounded({c.x - r * 0.75f, c.y - r, r * 1.1f, r * 2.0f}, 0.15f, 6, {245, 235, 255, 255});
  DrawRectangleRounded({c.x - r * 0.6f, c.y - r * 0.85f, r * 0.8f, r * 1.7f}, 0.15f, 6, {90, 60, 130, 255});
  DrawCircleV({c.x + r * 0.05f, c.y + r * 0.05f}, r * 0.09f, {255, 210, 80, 255});
  DrawLineEx({c.x + r * 0.2f, c.y}, {c.x + r * 1.1f, c.y}, r * 0.22f, {255, 210, 80, 255});
  ui::arrowTri({c.x + r * 1.15f, c.y}, r * 0.55f, 1, {255, 210, 80, 255});
}

struct Tile {
  Rectangle r;
  float slant;
  Color top, bottom;
  const char* label;
  const char* desc;
};

const Tile kTiles[4] = {
    {{60, 126, 640, 474}, 70, {236, 66, 56, 255}, {130, 16, 46, 255}, "COMBAT",
     "Affrontez jusqu'à 3 adversaires, humains ou ordinateurs."},
    {{728, 126, 500, 142}, 21, {70, 200, 112, 255}, {16, 100, 70, 255}, "ENTRAÎNEMENT",
     "Vies infinies, mannequin, compteur de combos : idéal pour apprendre."},
    {{704, 292, 500, 142}, 21, {72, 132, 246, 255}, {28, 46, 150, 255}, "PARAMÈTRES",
     "Son, affichage, règles du jeu, touches et manettes."},
    {{680, 458, 500, 142}, 21, {150, 96, 196, 255}, {58, 34, 96, 255}, "QUITTER",
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

  DrawRectangleGradientV(0, 0, kScreenW, kScreenH, {12, 6, 36, 255}, {78, 20, 86, 255});
  for (int i = 0; i < 150; ++i) {
    float x = std::fmod(hash(i) * kScreenW + t * (4.0f + 10.0f * hash(i + 9)), float(kScreenW));
    float y = hash(i + 500) * kScreenH * 0.8f;
    float tw = 0.5f + 0.5f * std::sin(t * (1.0f + 3.0f * hash(i + 3)) + i);
    DrawCircleV({x, y}, 0.7f + 1.6f * hash(i + 7), withAlpha(WHITE, 0.25f + 0.6f * tw));
  }
  ui::sunburst(center, 1100, 28, t * 0.07f, withAlpha({255, 214, 150, 255}, 0.13f));
  ui::glow(center, 470, withAlpha({255, 110, 90, 255}, 0.6f), 16);
  float pulse = 1.0f + 0.03f * std::sin(t * 2.0f);
  emblem(center, 215 * pulse, withAlpha(WHITE, 0.12f));

  // Floor and fighters (they run in from the sides)
  DrawRectangleGradientV(0, 560, kScreenW, 160, withAlpha({10, 4, 24, 255}, 0.0f), {8, 4, 20, 255});
  DrawEllipse(kScreenW / 2, 612, 600, 30, {70, 40, 130, 220});
  DrawEllipseLines(kScreenW / 2, 612, 600, 30, withAlpha({180, 150, 255, 255}, 0.7f));
  ui::glow({kScreenW * 0.5f, 612}, 300, withAlpha({160, 120, 255, 255}, 0.35f), 8);
  const auto& r = roster();
  for (int i = 0; i < int(r.size()); ++i) {
    float enter = easeOut((f - 26.0f - i * 5.0f) / 28.0f);
    float side = i < 3 ? -1.0f : 1.0f;
    float x = 205.0f + i * 174.0f + side * (1.0f - enter) * 760.0f;
    float hop = std::max(0.0f, std::sin(t * 2.4f + i * 1.3f)) * 9.0f;
    int facing = i < 3 ? 1 : -1;
    Pose p = restPose(r[i], t + i);
    drawCharacter(r[i], 0, p, {x, 608 - hop}, facing, 1.3f, 1.0f, WHITE, 0.0f, t + i);
    Pose refl = p;
    refl.squash = -0.55f;
    drawCharacter(r[i], 0, refl, {x, 616 + hop * 0.5f}, facing, 1.3f, 0.12f, WHITE, 0.0f, t + i);
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
                     s1, withAlpha(ui::kCream, alpha), withAlpha({130, 20, 40, 255}, alpha), 4.0f * scale,
                     FontKind::Title);
    float size = 150 * scale;
    float w = ui::spriteWidth(ui::Sprite::Logo, size);
    ui::drawSprite(ui::Sprite::Logo, center.x - w * 0.5f + shakeX, 108 - (scale - 1) * 60, size, alpha);
  }
  // Subtitle ribbon
  if (f > 28) {
    float a = clampf((f - 28) / 12.0f, 0.0f, 1.0f);
    Rectangle rib{center.x - 270, 286, 540, 44};
    ui::slanted({rib.x + 6, rib.y + 6, rib.width, rib.height}, 22, withAlpha(BLACK, 0.4f * a), withAlpha(BLACK, 0.4f * a));
    ui::slanted(rib, 22, withAlpha({232, 60, 60, 255}, a), withAlpha({150, 20, 50, 255}, a));
    drawTextCentered("LE CHOC DES COMBATTANTS", center.x, rib.y + 9, 23, withAlpha(ui::kCream, a));
  }
  // Sparkles around the logo
  for (int i = 0; i < 6; ++i) {
    float ph = std::fmod(t * 0.8f + hash(i + 40), 1.0f);
    float s = std::sin(ph * kPi) * 14.0f;
    Vector2 p{center.x - 380 + hash(i + 60) * 760, 100 + hash(i + 80) * 170};
    if (f > 30 && s > 1.0f) ui::star(p, s, s * 0.3f, t, withAlpha(WHITE, 0.85f));
  }
  // Press start
  if (f > 40) {
    float a = 0.55f + 0.45f * std::sin(t * 4.0f);
    const char* txt = "APPUYEZ SUR";
    const float ph = padPrompts() ? 44 : 34;  // a gamepad button is a smaller shape than a key cap
    float total = textWidth(txt, 26) + 14 + ui::promptWidth("ENTRÉE", ui::Btn::Start, ph);
    float x = center.x - total * 0.5f;
    drawTextOutlined(txt, x, 368, 26, withAlpha(WHITE, a), withAlpha(ui::kInk, a), 2.5f);
    ui::prompt("ENTRÉE", ui::Btn::Start, x + textWidth(txt, 26) + 14, 381 - ph * 0.5f, ph, true);
  }
  drawText("v2.0", kScreenW - 54, kScreenH - 26, 15, withAlpha(ui::kMuted, 0.8f));
  drawText("C++ & raylib", 16, kScreenH - 26, 15, withAlpha(ui::kMuted, 0.8f));
  if (f >= 22 && f < 40) DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(WHITE, (40 - f) / 18.0f * 0.7f));
}

// ---------------------------------------------------------------------------
// Main menu
// ---------------------------------------------------------------------------
// The device that opens the character select plays player 1 (keyboard or gamepad).
void App::claimP1(int device) {
  if (device < 0) return;
  for (int i = 1; i < 4; ++i)
    if (slots_[i].type == 1 && slots_[i].device == device) slots_[i].device = slots_[0].device;
  slots_[0].type = 1;
  slots_[0].device = device;
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
  ui::backdrop(t, {22, 12, 56, 255}, {74, 22, 84, 255}, {255, 120, 170, 255});
  ui::header("MENU PRINCIPAL", nullptr, {230, 60, 70, 255}, t);
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
    // shadow, body, sheen
    ui::slanted({rr.x + 10, rr.y + 12, rr.width, rr.height}, tile.slant, withAlpha(BLACK, 0.4f), withAlpha(BLACK, 0.4f));
    Color top = sel ? mixColor(tile.top, WHITE, 0.12f) : shade(tile.top, 0.8f);
    Color bottom = sel ? tile.bottom : shade(tile.bottom, 0.75f);
    ui::slanted(rr, tile.slant, top, bottom);
    ui::slanted({rr.x + tile.slant * 0.9f, rr.y, rr.width - tile.slant * 0.9f, rr.height * 0.18f}, tile.slant * 0.18f,
                withAlpha(WHITE, 0.22f), withAlpha(WHITE, 0.0f));
    // diagonal stripes inside
    for (int s = 0; s < 5; ++s) {
      float x0 = rr.x + 80 + s * 110 + std::fmod(t * 20.0f, 110.0f);
      if (x0 + 40 > rr.x + rr.width - tile.slant) continue;
      ui::slanted({x0, rr.y + 4, 40, rr.height - 8}, std::min(tile.slant, 40.0f), withAlpha(WHITE, 0.05f),
                  withAlpha(WHITE, 0.02f));
    }
    ui::slantedOutline(rr, tile.slant, sel ? 6.0f : 4.0f, sel ? ui::kGold : withAlpha(ui::kInk, 0.9f));
    if (sel) ui::slantedOutline(ui::grow(rr, 7), tile.slant, 4, withAlpha(ui::kGold, 0.25f + 0.2f * std::sin(t * 7)));

    if (i == 0) {
      ui::glow({rr.x + rr.width * 0.5f, rr.y + rr.height * 0.45f}, 230, withAlpha(WHITE, sel ? 0.35f : 0.2f), 10);
      const int show[4] = {0, 3, 2, 5};
      for (int k = 0; k < 4; ++k) {
        const CharacterDef& d = r[show[k]];
        Pose p = restPose(d, t + k);
        float hop = sel ? std::max(0.0f, std::sin(t * 5.0f + k * 1.6f)) * 10.0f : 0.0f;
        float scale = 170.0f / d.height * (d.style == BodyStyle::Brute ? 1.1f : 1.0f);
        float x = rr.x + 120 + k * 140;
        drawCharacter(d, 0, p, {x, rr.y + rr.height - 132 - hop}, k < 2 ? 1 : -1, scale, 1.0f, WHITE, 0.0f, t + k);
      }
      float bounce = sel ? std::fabs(std::sin(t * 4.0f)) * 6.0f : 0.0f;
      ui::drawSprite(ui::Sprite::Combat, rr.x + 36, rr.y + rr.height - 118 - bounce, 92);
      drawTextOutlined("Jusqu'à 4 joueurs  •  Ordinateurs niveaux 1 à 9", rr.x + 40, rr.y + rr.height - 30, 18,
                       ui::kCream, ui::kInk, 2.0f);
    } else {
      float bounce = sel ? std::fabs(std::sin(t * 4.0f)) * 4.0f : 0.0f;
      drawTextOutlined(tile.label, rr.x + tile.slant + 26, rr.y + 44 - bounce, 42, ui::kCream, ui::kInk, 4.0f,
                       FontKind::Title);
      Vector2 ic{rr.x + rr.width - 78, rr.y + rr.height * 0.5f};
      if (i == 1) drawTarget(ic, 36, t);
      if (i == 2) drawGear(ic, 36, t * (sel ? 1.2f : 0.3f), {235, 240, 255, 255});
      if (i == 3) drawExit({ic.x - 10, ic.y}, 32);
    }
  }
  ui::footer({{"FLÈCHES", ui::Btn::Move, "Naviguer"}, {"ENTRÉE", ui::Btn::A, "Choisir"}, {"ÉCHAP", ui::Btn::B, "Retour"}},
             kTiles[menuSel_].desc);
}
