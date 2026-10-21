// Results screen: the winner celebrates, then the scoreboard.
#include "game/app.hpp"
#include "game/fighter_art.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

void App::updateResults(const MenuNav& nav) {
  if (screenFrames_ == 25) audio_.voice("winner");  // "The winner is..." then the name
  if (screenFrames_ == 105 && winnerTeam_ >= 0) audio_.voice(winnerTeam_ == 1 ? "blueteam" : "redteam");
  else if (screenFrames_ == 105 && winnerChar_ >= 0) audio_.voice(voiceKey(roster()[winnerChar_].name).c_str());
  if (screenFrames_ > 40 && (nav.confirm || nav.start || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
    audio_.play(Sfx::MenuSelect);
    goTo(Screen::CharSelect);
  }
}

void App::drawResults() {
  const float t = time_;
  const auto& r = roster();
  Color wc = winnerTeam_ >= 0 ? teamColor(winnerTeam_)
             : winnerChar_ >= 0 ? (winnerCpu_ ? cpuColor() : portColor(winnerPort_)) : Color{180, 184, 176, 255};
  // The main menu's background under a navy veil, chunky cards and plates
  const int bg = settings().menuBg % 3;
  art::drawPicture(TextFormat("menu_fond_%d", bg + 1), {0, 0, kScreenW, kScreenH}, {0, 0, kScreenW, kScreenH});
  DrawRectangleGradientV(0, 0, kScreenW, kScreenH, withAlpha(ui::kNavyDeep, 0.6f), withAlpha(ui::kInk, 0.8f));
  for (int i = 0; i < 90; ++i) {  // confetti
    float x = std::fmod(i * 97.0f + std::sin(t + i) * 30.0f, float(kScreenW));
    float y = std::fmod(i * 53.0f + t * (60.0f + (i % 7) * 15.0f), float(kScreenH + 40)) - 20.0f;
    Color c = i % 3 == 0 ? ui::kGold : portColor(i);
    DrawRectanglePro({x, y, 10, 6}, {5, 3}, t * 200.0f + i * 40.0f, i % 5 == 0 ? ui::kCream : c);
  }

  const Vector2 spot{330, 380};
  const Color ac = winnerChar_ >= 0 ? auraColor(r[winnerChar_].hero) : wc;  // the winner's colour, as on the select
  const Rectangle pod{spot.x - 190, 232, 380, 430};
  ui::roundRect(ui::grow(pod, 4), 26, ui::kInk, ui::kInk);
  ui::roundRect(pod, 22, mixColor(ac, WHITE, 0.2f), shade(ac, 0.3f));
  ui::roundLines(ui::grow(pod, -6), 16, 2.0f, withAlpha(WHITE, 0.35f));
  if (winnerChar_ >= 0) {
    const CharacterDef& d = r[winnerChar_];
    Pose p = restPose(d, t);
    float H = d.height;
    p.handF = {0.42f * H, -0.46f * H};  // arms wide open, away from the face
    p.handB = {-0.32f * H, -0.44f * H};
    p.weaponDir = Vec2{0.35f, -1.0f}.norm();
    if (d.style == BodyStyle::Puff) p.handF = {0.32f * H, -0.2f * H}, p.handB = {-0.32f * H, -0.2f * H};
    if (art::hasSheet(d.hero, art::kSheetVictory)) {  // its own victory pose, played once then held
      p.sheet = art::kSheetVictory;
      p.sheetT = clampf(screenFrames_ / 90.0f, 0.0f, 1.0f);
    }
    float hop = std::fabs(std::sin(t * 4.0f)) * 18.0f;
    DrawEllipse(330, 632, 70.0f - hop, 11.0f, withAlpha(BLACK, 0.4f));
    drawShowcase(d, winnerPalette_, p, {330, 630 - hop}, 1, 250.0f, t);
    ui::drawSprite(ui::Sprite::Victory, 330 - ui::spriteWidth(ui::Sprite::Victory, 92) * 0.5f, 26, 92);
    ui::boldCentered(d.name.c_str(), 330, 134, 44);
    if (winnerTeam_ >= 0)
      drawTextCentered(TextFormat("Victoire de l'équipe %s !", winnerTeam_ == 1 ? "bleue" : "rouge"), 330, 186, 24, wc,
                       true);
    else
      drawTextCentered(winnerCpu_ ? "Ordinateur" : TextFormat("Joueur %d", winnerPort_ + 1), 330, 186, 22, WHITE);
  } else {
    ui::drawSprite(ui::Sprite::Draw, 330 - ui::spriteWidth(ui::Sprite::Draw, 80) * 0.5f, 60, 80);
  }

  const float tx = 650, ty = 112;
  const int n = int(results_.size());
  const Rectangle board{tx - 34, ty - 18, 640, 76.0f + n * 96.0f};
  ui::card(board, 0.95f);
  const char* heads[3] = {"K.O.", "CHUTES", "DÉGÂTS"};
  for (int i = 0; i < 3; ++i) ui::boldCentered(heads[i], tx + 330 + i * 100.0f, ty - 4, 18, ui::kGold);
  const Color medals[3] = {ui::kGold, {204, 212, 214, 255}, {206, 138, 86, 255}};
  for (int i = 0; i < n; ++i) {
    const ResultRow& row = results_[i];
    float enter = clampf((screenFrames_ - 20 - i * 8) / 16.0f, 0.0f, 1.0f);
    float y = ty + 36 + i * 96.0f;
    float x = tx + (1.0f - enter) * 700.0f;
    Color pc = row.team >= 0 ? teamColor(row.team) : row.cpu ? cpuColor() : portColor(row.port);
    const Rectangle rr{x, y, 570, 84};
    ui::chunky(rr, i == 0 ? ui::Tone::Gold : ui::Tone::Navy);  // the winner's row is gold
    ui::roundRect({x + 6, y + 8, 10, 62}, 5, mixColor(pc, WHITE, 0.25f), shade(pc, 0.7f));  // player colour edge
    ui::bold(TextFormat("%d", i + 1), x + 28, y + 14, 50, i < 3 ? medals[i] : WHITE);
    drawPortrait(r[row.charIndex], row.palette, {x + 104, y + 40}, 64, t + i);
    float ns = 30.0f;  // long names shrink before the K.O. column
    while (ns > 16.0f && textWidth(r[row.charIndex].name.c_str(), ns, FontKind::Title) > 140.0f) ns -= 1.0f;
    ui::bold(r[row.charIndex].name.c_str(), x + 146, y + 10 + (30 - ns) * 0.5f, ns);
    drawTextOutlined(TextFormat("%s%s", row.cpu ? "Ordinateur" : TextFormat("Joueur %d", row.port + 1),
                                row.team < 0 ? "" : row.team == 1 ? "  •  équipe bleue" : "  •  équipe rouge"),
                     x + 148, y + 48, 16, WHITE, ui::kInk, 2.0f);
    ui::boldCentered(TextFormat("%d", row.kos), x + 330, y + 18, 38);
    ui::boldCentered(TextFormat("%d", row.falls), x + 430, y + 18, 38);
    ui::boldCentered(TextFormat("%d%%", int(row.damage)), x + 525, y + 18, 34);
  }
  if (screenFrames_ > 40) ui::footer({{"ENTRÉE", ui::Btn::A, "Continuer"}});
}
