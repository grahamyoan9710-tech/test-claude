// Results screen: the winner celebrates, then the scoreboard.
#include "game/app.hpp"
#include "game/render.hpp"
#include "game/ui.hpp"

void App::updateResults(const MenuNav& nav) {
  if (screenFrames_ > 40 && (nav.confirm || nav.start || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
    audio_.play(Sfx::MenuSelect);
    goTo(Screen::CharSelect);
  }
}

void App::drawResults() {
  const float t = time_;
  const auto& r = roster();
  Color wc = winnerChar_ >= 0 ? (winnerCpu_ ? cpuColor() : portColor(winnerPort_)) : Color{180, 180, 200, 255};
  ui::backdrop(t, {20, 12, 46, 255}, mixColor(wc, BLACK, 0.62f), wc);
  for (int i = 0; i < 90; ++i) {  // confetti
    float x = std::fmod(i * 97.0f + std::sin(t + i) * 30.0f, float(kScreenW));
    float y = std::fmod(i * 53.0f + t * (60.0f + (i % 7) * 15.0f), float(kScreenH + 40)) - 20.0f;
    DrawRectanglePro({x, y, 10, 6}, {5, 3}, t * 200.0f + i * 40.0f, portColor(i));
  }

  const Vector2 spot{330, 380};
  ui::sunburst(spot, 620, 22, t * 0.25f, withAlpha(wc, 0.3f));
  ui::glow(spot, 260, withAlpha(WHITE, 0.35f), 10);
  DrawEllipse(330, 634, 190, 30, {20, 14, 40, 230});
  DrawEllipseLines(330, 634, 190, 30, withAlpha(wc, 0.9f));
  if (winnerChar_ >= 0) {
    const CharacterDef& d = r[winnerChar_];
    Pose p = restPose(d, t);
    float H = d.height;
    p.handF = {0.42f * H, -0.46f * H};  // arms wide open, away from the face
    p.handB = {-0.32f * H, -0.44f * H};
    p.weaponDir = Vec2{0.35f, -1.0f}.norm();
    if (d.style == BodyStyle::Puff) p.handF = {0.32f * H, -0.2f * H}, p.handB = {-0.32f * H, -0.2f * H};
    float hop = std::fabs(std::sin(t * 4.0f)) * 18.0f;
    drawCharacter(d, winnerPalette_, p, {330, 630 - hop}, 1, 250.0f / H, 1.0f, WHITE, 0.0f, t);
    const char* title = "VICTOIRE !";
    float size = 92;
    ui::logoText(title, 330 - ui::logoWidth(title, size) * 0.5f, 26, size, {255, 248, 190, 255}, {255, 206, 64, 255},
                 {242, 124, 34, 255}, {110, 16, 34, 255}, {70, 8, 30, 255}, 7);
    drawTextCentered(d.name.c_str(), 330, 134, 44, wc, true, FontKind::Title);
    drawTextCentered(winnerCpu_ ? "Ordinateur" : TextFormat("Joueur %d", winnerPort_ + 1), 330, 186, 22, ui::kCream);
  } else {
    ui::logoText("MATCH NUL !", 60, 60, 80, WHITE, {230, 230, 240, 255}, {200, 200, 220, 255}, ui::kInk,
                 {40, 30, 60, 255}, 5);
  }

  const float tx = 650, ty = 112;
  const int n = int(results_.size());
  ui::panel({tx - 20, ty - 18, 610, 76.0f + n * 96.0f}, withAlpha({18, 14, 40, 255}, 0.88f), withAlpha(wc, 0.8f),
            0.06f);
  const char* heads[3] = {"K.O.", "CHUTES", "DÉGÂTS"};
  for (int i = 0; i < 3; ++i) drawTextCentered(heads[i], tx + 330 + i * 100.0f, ty, 18, ui::kMuted, false);
  const Color medals[3] = {{255, 206, 64, 255}, {200, 210, 225, 255}, {215, 140, 80, 255}};
  for (int i = 0; i < n; ++i) {
    const ResultRow& row = results_[i];
    float enter = clampf((screenFrames_ - 20 - i * 8) / 16.0f, 0.0f, 1.0f);
    float y = ty + 36 + i * 96.0f;
    float x = tx + (1.0f - enter) * 700.0f;
    Color pc = row.cpu ? cpuColor() : portColor(row.port);
    ui::slanted({x, y, 570, 84}, 16, mixColor(pc, BLACK, 0.35f), mixColor(pc, BLACK, 0.65f));
    ui::slantedOutline({x, y, 570, 84}, 16, 3, i == 0 ? medals[0] : withAlpha(ui::kInk, 0.9f));
    drawTextOutlined(TextFormat("%d", i + 1), x + 26, y + 16, 50, i < 3 ? medals[i] : ui::kCream, ui::kInk, 4.0f,
                     FontKind::Title);
    drawPortrait(r[row.charIndex], row.palette, {x + 104, y + 40}, 64, t + i);
    drawTextOutlined(r[row.charIndex].name.c_str(), x + 146, y + 12, 30, WHITE, ui::kInk, 3.0f, FontKind::Title);
    drawText(row.cpu ? "Ordinateur" : TextFormat("Joueur %d", row.port + 1), x + 148, y + 50, 16, ui::kCream);
    drawTextCentered(TextFormat("%d", row.kos), x + 330, y + 22, 38, WHITE, true, FontKind::Title);
    drawTextCentered(TextFormat("%d", row.falls), x + 430, y + 22, 38, WHITE, true, FontKind::Title);
    drawTextCentered(TextFormat("%d%%", int(row.damage)), x + 525, y + 22, 34, WHITE, true, FontKind::Title);
  }
  if (screenFrames_ > 40) ui::footer({{"ENTRÉE", "Continuer"}});
}
