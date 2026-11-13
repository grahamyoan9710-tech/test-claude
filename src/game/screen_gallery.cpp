// The fighters' gallery (PERSONNAGES on the main menu): each fighter's look, fighting style, weapons and skills.
#include "game/app.hpp"
#include "game/fighter_art.hpp"
#include "game/fx.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

// docs/refonte/FICHES.md: the fiche's title, the martial art of the bare hands (§1.7), the style in a word, the build.
Fiche fiche(const CharacterDef& d) {
  if (d.id == "p01") return {"OMBRE VIOLETTE", "Penchak Silat", "Polyvalent", 3};
  if (d.id == "p02") return {"GIVRE STELLAIRE", "Taekwondo", "Rapide en l'air", 2};
  if (d.id == "p03") return {"BRASIER ROUGE", "Muay Thaï", "Puissant", 4};
  if (d.id == "p07") return {"FOUDRE ROUGE", "Karaté", "Très rapide", 2};
  if (d.id == "p08") return {"LUMIÈRE BLANCHE", "Kung-fu", "Équilibré", 3};
  if (d.id == "h01") return {"HAKUEN", "Karaté", "Lame blanche", 3};
  return {upperFr(d.series), "Combat libre", "", 3};
}

namespace {
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
  const Fiche fi = fiche(d);
  constexpr FontKind D = FontKind::Display, B = FontKind::Body;
  static int shownSel = -1, changedAt = 0;  // the card and the sheet slide in again when the fighter changes
  if (gallerySel_ != shownSel) shownSel = gallerySel_, changedAt = screenFrames_;
  auto ease = [&](float from, float delay, float len) {
    const float u = clampf((screenFrames_ - from - delay) / len, 0.0f, 1.0f);
    return 1.0f - (1.0f - u) * (1.0f - u) * (1.0f - u);
  };
  ui::stageBackdrop(featuredStage(), t, 0.55f);
  ui::dusk(0.78f);
  ui::header("PERSONNAGES", nullptr, ui::kGold, t);

  // The fighters: a rounded plate each, the chosen one cream
  for (int i = 0; i < int(r.size()); ++i) {
    const bool sel = i == gallerySel_;
    const float in = ease(0, 3 + i * 2.4f, 27);
    Rectangle p = plateRect(i);
    p.x -= 28 * (1 - in);
    const Color f = sel ? ui::kCream : ui::hovered(p) ? Color{46, 43, 94, 255} : Color{36, 34, 79, 235};
    ui::roundRect(p, 16, f, f);
    if (!sel) ui::roundLines(p, 16, 1.0f, withAlpha(ui::kCream, 0.1f));
    const Vec2 face{p.x + 38, p.y + p.height * 0.5f};
    DrawCircleV(V(face), 26, sel ? ui::kInk : withAlpha(ui::kCream, 0.25f));
    DrawCircleV(V(face), 24, mixColor(auraColor(r[i].hero), ui::kNavy, 0.55f));
    drawArtDisc(r[i], 0, face, 24, t + i);
    const Fiche fr = fiche(r[i]);
    const std::string nick = upperFr(fr.nick);
    float ns = 14;
    while (ns > 9 && textWidth(nick.c_str(), ns, D) > p.width - 90) ns -= 0.5f;
    const Color ink = sel ? ui::kInk : ui::kCream;
    drawText(nick.c_str(), p.x + 76, capsY(p.y + 26, ns, D), ns, ink, D);
    const std::string sub = "N° " + r[i].id.substr(1) + " · " + fr.art;
    drawText(sub.c_str(), p.x + 76, capsY(p.y + 46, 12, B), 12, sel ? withAlpha(ui::kInk, 0.7f) : ui::kMuted, B);
    if (sel) ui::rim(p, 16, t);
  }

  // The fighter, live, on a card of its colour, its number huge and faint behind
  const Color ac = auraColor(d.hero);
  const float cin = ease(float(changedAt), 0, 30);
  const Rectangle stage{384 + 30 * (1 - cin), 104, 392, 548};
  ui::roundRect(stage, 22, mixColor(ac, ui::kDeep, 0.45f), ui::kDeep);
  ui::roundLines(stage, 22, 1.0f, withAlpha(ui::kCream, 0.14f));
  beginClip(stage);
  const std::string num = d.id.substr(1);
  drawText(num.c_str(), stage.x + stage.width - 20 - textWidth(num.c_str(), 220, D), capsY(stage.y + 110, 220, D), 220,
           withAlpha(ac, 0.14f * cin), D);
  ui::glow({stage.x + 196, 460}, 230, withAlpha(ac, 0.35f * cin));
  endClip();
  DrawEllipse(int(stage.x + 196), 612, 80, 12, withAlpha(BLACK, 0.45f));
  drawShowcase(d, 0, restPose(d, t), {stage.x + 196, 610}, 1, 330.0f, t);

  // Its sheet: style, weapons, transformation, skills
  const float sin_ = ease(float(changedAt), 4, 30);
  const Rectangle sheet{804 + 30 * (1 - sin_), 104, 432, 548};
  const Color card{33, 31, 72, 235};
  ui::roundRect(sheet, 20, card, card);
  ui::roundLines(sheet, 20, 1.0f, withAlpha(ui::kCream, 0.1f));
  const float x = sheet.x + 28;
  float y = sheet.y + 26;
  const std::string name = upperFr(fi.nick);
  float ns = 30;
  while (ns > 16 && textWidth(name.c_str(), ns, D) > sheet.width - 56) ns -= 1;
  drawText(name.c_str(), x, capsY(y + 18, ns, D), ns, ui::kCream, D);
  const std::string sub = "N° " + num + " · " + fi.art + (fi.style.empty() ? std::string() : " · " + fi.style);
  drawText(sub.c_str(), x, capsY(y + 52, 14, B), 14, ui::kSoft, B);
  DrawRectangleRec({x, y + 72, 40, 2}, ui::kCream);
  DrawCircleV({x + 44, y + 73}, 3, ui::kBlue);
  y += 100;
  ui::label("ARMES", x, y, 11, 0.16f, ui::kBlue);
  for (int s = 0; s < 2; ++s) {
    const float wy = y + 18 + s * 56;
    DrawCircleV({x + 22, wy + 22}, 23, withAlpha(ui::kCream, 0.16f));
    DrawCircleV({x + 22, wy + 22}, 22, mixColor(ac, ui::kNavyDeep, 0.6f));
    drawWeaponIcon(d, s, {x + 22, wy + 22}, 32.0f);
    drawText(TextFormat("%s · %s", s == 0 ? "A" : "B", upperFr(d.weapons[s].name).c_str()), x + 58, capsY(wy + 14, 13, D), 13,
             ui::kCream, D);
    drawText(d.weapons[s].desc.c_str(), x + 58, capsY(wy + 34, 13, B), 13, ui::kMuted, B);
  }
  y += 140;
  ui::label("TRANSFORMATION", x, y, 11, 0.16f, ui::kBlue);
  drawText(d.forms.empty() ? "Aucune" : TextFormat("T%s", num.c_str()), x + 150, capsY(y, 14, B), 14, ui::kCream, B);
  y += 34;
  ui::label("COMPÉTENCES", x, y, 11, 0.16f, ui::kBlue);
  const char* dirs[4] = {"Neutre", "Côté", "Haut", "Bas"};
  for (int k = 0; k < 4; ++k) {
    const float ry = y + 26 + k * 27;
    drawText(dirs[k], x, capsY(ry, 13, B), 13, ui::kMuted, B);
    drawText(d.moves[NSpecial + k].name.c_str(), x + 80, capsY(ry, 15, B), 15, ui::kCream, B);
  }
  const float uy = y + 26 + 4 * 27 + 4;
  drawText("Ultime", x, capsY(uy, 13, B), 13, ui::kMuted, B);
  drawText(upperFr(d.moves[FinalSmash].name).c_str(), x + 80, capsY(uy, 14, D), 14, ui::kBlue, D);

  ui::footer({{"HAUT/BAS", ui::Btn::DPadUD, "Choisir"}, {"ÉCHAP", ui::Btn::B, "Retour"}});
}
