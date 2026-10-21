// The fighters' gallery (PERSONNAGES on the main menu): each fighter's look, fighting style, weapons and skills.
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
Rectangle plateRect(int i) { return {44, 116.0f + i * 82.0f, 310, 68}; }
constexpr Color kSoft{184, 204, 240, 255};  // labels and secondary lines
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
  ui::stageBackdrop(featuredStage(), t, 0.55f);
  ui::header("PERSONNAGES", nullptr, ui::kGold, t);

  // The fighters, one chunky button each: the chosen one is gold
  for (int i = 0; i < int(r.size()); ++i) {
    const bool sel = i == gallerySel_;
    Rectangle p = plateRect(i);
    ui::button(p, "", sel ? ui::Tone::Gold : ui::Tone::Blue, sel, t);
    if (sel) p = ui::grow(p, 3.0f);
    const float lip = ui::chunkyLip(p);
    const Vec2 face{p.x + 44, p.y + (p.height - lip) * 0.5f};
    DrawCircleV(V(face), 30, ui::kInk);
    DrawCircleV(V(face), 27, shade(ui::toneBottom(sel ? ui::Tone::Gold : ui::Tone::Blue), 0.5f));
    drawPortraitDisc(r[i], 0, face, 27, t + i);
    ui::bold(r[i].name.c_str(), p.x + 86, p.y + 8, sel ? 28 : 25);
    drawTextOutlined(r[i].series.c_str(), p.x + 88, p.y + 40, 15, WHITE, ui::kInk, 2.0f);
  }

  // The fighter, live, on a card of its colour
  const Color ac = auraColor(d.hero);
  const Rectangle stage{384, 104, 392, 548};
  ui::roundRect(ui::grow(stage, 4), 26, ui::kInk, ui::kInk);
  ui::roundRect(stage, 22, mixColor(ac, WHITE, 0.2f), shade(ac, 0.3f));
  ui::roundRect({stage.x + 6, stage.y + 4, stage.width - 12, 60}, 18, withAlpha(WHITE, 0.16f), withAlpha(WHITE, 0.0f));
  ui::roundLines(ui::grow(stage, -6), 16, 2.0f, withAlpha(WHITE, 0.35f));
  DrawEllipse(580, 612, 80, 12, withAlpha(BLACK, 0.45f));
  drawShowcase(d, 0, restPose(d, t), {580, 610}, 1, 330.0f, t);

  // Its sheet: style, weapons, transformation, skills
  const Rectangle sheet{804, 104, 444, 548};
  ui::card(sheet, 0.96f);
  const float x = sheet.x + 30;
  float y = sheet.y + 18;
  ui::bold(d.name.c_str(), x, y, 44);
  drawTextOutlined(TextFormat("%s  •  %s", d.series.c_str(), martialArt(d.id)), x + 2, y + 54, 18, kSoft, ui::kInk, 2.0f);
  DrawRectangleGradientH(int(x), int(y + 86), 300, 3, ui::kGold, withAlpha(ui::kGold, 0.0f));
  y += 100;
  drawTextOutlined("ARMES", x, y, 15, ui::kGold, ui::kInk, 2.0f);
  for (int s = 0; s < 2; ++s) {
    const float wy = y + 24 + s * 58;
    DrawCircleV({x + 22, wy + 22}, 25, ui::kInk);
    DrawCircleV({x + 22, wy + 22}, 22, mixColor(ac, ui::kNavyDeep, 0.45f));
    drawWeaponIcon(d, s, {x + 22, wy + 22}, 34.0f);
    ui::bold(TextFormat("%s  %s", s == 0 ? "A" : "B", d.weapons[s].name.c_str()), x + 56, wy, 21);
    drawTextOutlined(d.weapons[s].desc.c_str(), x + 58, wy + 28, 15, kSoft, ui::kInk, 2.0f);
  }
  y += 148;
  drawTextOutlined("TRANSFORMATION", x, y, 15, ui::kGold, ui::kInk, 2.0f);
  drawTextOutlined(d.forms.empty() ? "Aucune" : TextFormat("T%s", d.id.substr(1).c_str()), x + 160, y - 2, 19, WHITE,
                   ui::kInk, 2.0f);
  y += 36;
  drawTextOutlined("COMPÉTENCES", x, y, 15, ui::kGold, ui::kInk, 2.0f);
  const char* dirs[4] = {"Neutre", "Côté", "Haut", "Bas"};
  for (int k = 0; k < 4; ++k) {
    drawTextOutlined(dirs[k], x, y + 24 + k * 27, 16, kSoft, ui::kInk, 2.0f);
    drawTextOutlined(d.moves[NSpecial + k].name.c_str(), x + 80, y + 22 + k * 27, 19, WHITE, ui::kInk, 2.0f);
  }
  drawTextOutlined("Ultime", x, y + 24 + 4 * 27, 16, kSoft, ui::kInk, 2.0f);
  ui::bold(d.moves[FinalSmash].name.c_str(), x + 80, y + 18 + 4 * 27, 22, ui::kGold);

  ui::footer({{"HAUT/BAS", ui::Btn::DPadUD, "Choisir"}, {"ÉCHAP", ui::Btn::B, "Retour"}});
}
