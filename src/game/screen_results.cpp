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
  // The main menu's background and look: crimson plates, dark ink, white type
  const int bg = settings().menuBg % 3;
  art::drawPicture(TextFormat("menu_fond_%d", bg + 1), {0, 0, kScreenW, kScreenH}, {0, 0, kScreenW, kScreenH});
  DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha({12, 12, 18, 255}, 0.55f));
  for (int i = 0; i < 90; ++i) {  // confetti
    float x = std::fmod(i * 97.0f + std::sin(t + i) * 30.0f, float(kScreenW));
    float y = std::fmod(i * 53.0f + t * (60.0f + (i % 7) * 15.0f), float(kScreenH + 40)) - 20.0f;
    Color c = i % 3 == 0 ? ui::kGold : portColor(i);
    DrawRectanglePro({x, y, 10, 6}, {5, 3}, t * 200.0f + i * 40.0f, i % 5 == 0 ? ui::kCream : c);
  }

  const Vector2 spot{330, 380};
  const Color ac = winnerChar_ >= 0 ? auraColor(r[winnerChar_].hero) : wc;  // the winner's colour, as on the select
  ui::slanted({spot.x - 230, 236, 460, 420}, 120, withAlpha(ac, 0.55f), withAlpha(shade(ac, 0.35f), 0.55f));
  ui::slanted({spot.x - 230, 236, 460, 420}, 120, withAlpha({12, 12, 18, 255}, 0.2f), withAlpha({12, 12, 18, 255}, 0.7f));
  ui::slantedOutline({spot.x - 230, 236, 460, 420}, 120, 2.0f, withAlpha(mixColor(ac, WHITE, 0.4f), 0.6f));
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
    drawTextOutlined(d.name.c_str(), 330 - textWidth(d.name.c_str(), 44, FontKind::Title) * 0.5f, 134, 44, WHITE,
                     withAlpha(BLACK, 0.7f), 3.0f, FontKind::Title);
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
  ui::slanted({board.x + 6, board.y + 7, board.width, board.height}, 18, withAlpha(BLACK, 0.45f), withAlpha(BLACK, 0.45f));
  ui::slanted(board, 18, withAlpha({28, 28, 38, 255}, 0.94f), withAlpha({12, 12, 18, 255}, 0.94f));
  DrawRectangleGradientH(int(board.x + 10), int(board.y + board.height - 4), int(board.width - 10), 4, ui::kRed,
                         withAlpha(ui::kRed, 0.0f));
  const char* heads[3] = {"K.O.", "CHUTES", "DÉGÂTS"};
  for (int i = 0; i < 3; ++i) drawTextCentered(heads[i], tx + 330 + i * 100.0f, ty, 18, {200, 200, 212, 255}, false);
  const Color medals[3] = {ui::kGold, {204, 212, 214, 255}, {206, 138, 86, 255}};
  for (int i = 0; i < n; ++i) {
    const ResultRow& row = results_[i];
    float enter = clampf((screenFrames_ - 20 - i * 8) / 16.0f, 0.0f, 1.0f);
    float y = ty + 36 + i * 96.0f;
    float x = tx + (1.0f - enter) * 700.0f;
    Color pc = row.team >= 0 ? teamColor(row.team) : row.cpu ? cpuColor() : portColor(row.port);
    const Rectangle rr{x, y, 570, 84};
    ui::menuPlate(rr, 16, i == 0);  // the winner's row is the chosen one
    ui::slanted({x + 16 - 5, y, 9, 84}, 16, pc, shade(pc, 0.6f));  // player colour edge
    drawTextOutlined(TextFormat("%d", i + 1), x + 26, y + 16, 50, i < 3 ? medals[i] : WHITE, withAlpha(BLACK, 0.7f),
                     4.0f, FontKind::Title);
    drawPortrait(r[row.charIndex], row.palette, {x + 104, y + 40}, 64, t + i);
    float ns = 30.0f;  // long names shrink before the K.O. column
    while (ns > 16.0f && textWidth(r[row.charIndex].name.c_str(), ns, FontKind::Title) > 140.0f) ns -= 1.0f;
    drawTextOutlined(r[row.charIndex].name.c_str(), x + 146, y + 12 + (30 - ns) * 0.5f, ns, WHITE, withAlpha(BLACK, 0.7f),
                     3.0f, FontKind::Title);
    drawText(TextFormat("%s%s", row.cpu ? "Ordinateur" : TextFormat("Joueur %d", row.port + 1),
                        row.team < 0 ? "" : row.team == 1 ? "  •  équipe bleue" : "  •  équipe rouge"),
             x + 148, y + 50, 16, {220, 220, 230, 255});
    drawTextCentered(TextFormat("%d", row.kos), x + 330, y + 22, 38, WHITE, true, FontKind::Title);
    drawTextCentered(TextFormat("%d", row.falls), x + 430, y + 22, 38, WHITE, true, FontKind::Title);
    drawTextCentered(TextFormat("%d%%", int(row.damage)), x + 525, y + 22, 34, WHITE, true, FontKind::Title);
  }
  if (screenFrames_ > 40) ui::footer({{"ENTRÉE", ui::Btn::A, "Continuer"}});
}
