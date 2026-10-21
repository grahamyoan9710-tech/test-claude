// The fighters' gallery (PERSONNAGES on the main menu): each fighter's look, fighting style, weapons and skills, in
// the main menu's style (docs/refonte/menu_accueil.png).
#include "game/app.hpp"
#include "game/fighter_art.hpp"
#include "game/fx.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
// Bare-handed, each fighter fights in a martial art (docs/refonte/FICHES.md, §1.7).
const char* martialArt(const std::string& id) {
  if (id == "p01") return "Penchak Silat";
  if (id == "p02") return "Taekwondo";
  if (id == "p03") return "Muay Thaï";
  if (id == "p07") return "Karaté";
  if (id == "p08") return "Kung-fu";
  return "Combat libre";
}
Rectangle plateRect(int i) { return {40, 160.0f + i * 78.0f, 310, 66}; }
constexpr Color kNight{12, 12, 18, 255};
constexpr Color kSoft{200, 200, 212, 255};  // labels and secondary lines
}  // namespace

void App::updateGallery(const MenuNav& nav) {
  const int n = int(roster().size()), before = gallerySel_;
  if (nav.up) gallerySel_ = (gallerySel_ + n - 1) % n;
  if (nav.down) gallerySel_ = (gallerySel_ + 1) % n;
  for (int i = 0; i < n; ++i)
    if (ui::hovered(plateRect(i)) && (ui::mouseMoved() || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) gallerySel_ = i;
  if (gallerySel_ != before) audio_.play(Sfx::MenuMove);
  if (nav.back) {
    audio_.play(Sfx::MenuBack);
    goTo(Screen::MainMenu);
  }
}

void App::drawGallery() {
  const float t = time_;
  const auto& r = roster();
  gallerySel_ = std::clamp(gallerySel_, 0, int(r.size()) - 1);
  const CharacterDef& d = r[gallerySel_];
  art::drawPicture(TextFormat("menu_fond_%d", settings().menuBg % 3 + 1), {0, 0, kScreenW, kScreenH},
                   {0, 0, kScreenW, kScreenH});
  DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(kNight, 0.6f));

  // Logo and title, top left
  float lw, lh;
  if (art::pictureSize("menu_logo", lw, lh)) {
    const float k = 230.0f / lw;
    art::drawPicture("menu_logo", {0, 0, lw, lh}, {30, 4, lw * k, lh * k});
  }
  const Rectangle tb{44, 112, 230, 30};
  ui::slanted(tb, 10, withAlpha({16, 16, 24, 255}, 0.92f), withAlpha({16, 16, 24, 255}, 0.92f));
  ui::slantedOutline(tb, 10, 1.5f, withAlpha(WHITE, 0.7f));
  drawTextCentered("PERSONNAGES", tb.x + tb.width * 0.5f, tb.y + 5, 19, WHITE, true, FontKind::Title);

  // The fighters, one plate each: the chosen one is crimson
  for (int i = 0; i < int(r.size()); ++i) {
    const bool sel = i == gallerySel_;
    const Rectangle p = sel ? ui::grow(plateRect(i), 4) : plateRect(i);
    ui::menuPlate(p, 20, sel);
    drawPortrait(r[i], 0, {p.x + 62, p.y + p.height * 0.5f}, p.height - 8, t + i);
    drawTextOutlined(r[i].name.c_str(), p.x + 104, p.y + 10, sel ? 27 : 24, WHITE, withAlpha(BLACK, 0.6f), 2.0f,
                     FontKind::Title);
    drawText(r[i].series.c_str(), p.x + 106, p.y + 42, 15, sel ? WHITE : kSoft);
  }

  // The fighter, live, on a slanted panel of its colour (as on the select)
  const Color ac = auraColor(d.hero);
  const Rectangle stage{380, 110, 400, 540};
  ui::slanted(stage, 110, withAlpha(ac, 0.55f), withAlpha(shade(ac, 0.35f), 0.55f));
  ui::slanted(stage, 110, withAlpha(kNight, 0.2f), withAlpha(kNight, 0.7f));
  ui::slantedOutline(stage, 110, 2.0f, withAlpha(mixColor(ac, WHITE, 0.4f), 0.6f));
  DrawEllipse(580, 612, 80, 12, withAlpha(BLACK, 0.45f));
  drawShowcase(d, 0, restPose(d, t), {580, 610}, 1, 330.0f, t);

  // Its sheet: style, weapons, transformation, skills
  const Rectangle sheet{800, 110, 450, 540};
  ui::slanted({sheet.x + 6, sheet.y + 7, sheet.width, sheet.height}, 18, withAlpha(BLACK, 0.45f), withAlpha(BLACK, 0.45f));
  ui::slanted(sheet, 18, withAlpha({28, 28, 38, 255}, 0.94f), withAlpha({12, 12, 18, 255}, 0.94f));
  DrawRectangleGradientH(int(sheet.x + 10), int(sheet.y + sheet.height - 4), int(sheet.width - 10), 4, ui::kRed,
                         withAlpha(ui::kRed, 0.0f));
  const float x = sheet.x + 34;
  float y = sheet.y + 18;
  drawTextOutlined(d.name.c_str(), x, y, 44, WHITE, withAlpha(BLACK, 0.6f), 3.0f, FontKind::Title);
  drawText(TextFormat("%s  •  %s", d.series.c_str(), martialArt(d.id)), x + 2, y + 52, 18, kSoft);
  DrawRectangleGradientH(int(x), int(y + 84), 300, 3, ui::kRed, withAlpha(ui::kRed, 0.0f));
  y += 100;
  drawText("ARMES", x, y, 15, kSoft);
  for (int s = 0; s < 2; ++s) {
    const float wy = y + 24 + s * 58;
    DrawCircleV({x + 22, wy + 22}, 22, withAlpha(kNight, 0.8f));
    DrawRing({x + 22, wy + 22}, 20, 22, 0, 360, 32, withAlpha(ac, 0.9f));
    drawWeaponIcon(d, s, {x + 22, wy + 22}, 34.0f);
    drawTextOutlined(TextFormat("%s  %s", s == 0 ? "A" : "B", d.weapons[s].name.c_str()), x + 56, wy + 2, 21, WHITE,
                     withAlpha(BLACK, 0.6f), 2.0f, FontKind::Title);
    drawText(d.weapons[s].desc.c_str(), x + 58, wy + 28, 15, kSoft);
  }
  y += 148;
  drawText("TRANSFORMATION", x, y, 15, kSoft);
  drawText(d.forms.empty() ? "Aucune" : TextFormat("T%s", d.id.substr(1).c_str()), x + 160, y - 2, 19, WHITE);
  y += 36;
  drawText("COMPÉTENCES", x, y, 15, kSoft);
  const char* dirs[4] = {"Neutre", "Côté", "Haut", "Bas"};
  for (int k = 0; k < 4; ++k) {
    drawText(dirs[k], x, y + 24 + k * 27, 16, kSoft);
    drawText(d.moves[NSpecial + k].name.c_str(), x + 80, y + 22 + k * 27, 19, WHITE);
  }
  drawText("Ultime", x, y + 24 + 4 * 27, 16, kSoft);
  drawTextOutlined(d.moves[FinalSmash].name.c_str(), x + 80, y + 20 + 4 * 27, 21, ui::kRed, withAlpha(BLACK, 0.7f), 2.0f,
                   FontKind::Title);

  ui::footer({{"HAUT/BAS", ui::Btn::DPadUD, "Choisir"}, {"ÉCHAP", ui::Btn::B, "Retour"}});
}
