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
  Color wc = winnerChar_ >= 0 ? (winnerCpu_ ? cpuColor() : portColor(winnerPort_)) : Color{180, 184, 176, 255};
  ui::stageBackdrop(1, t, 0.4f);  // the sanctuary at golden hour
  for (int i = 0; i < 90; ++i) {  // confetti
    float x = std::fmod(i * 97.0f + std::sin(t + i) * 30.0f, float(kScreenW));
    float y = std::fmod(i * 53.0f + t * (60.0f + (i % 7) * 15.0f), float(kScreenH + 40)) - 20.0f;
    Color c = i % 3 == 0 ? ui::kGold : portColor(i);
    DrawRectanglePro({x, y, 10, 6}, {5, 3}, t * 200.0f + i * 40.0f, i % 5 == 0 ? ui::kCream : c);
  }

  const Vector2 spot{330, 380};
  ui::glow(spot, 300, withAlpha(mixColor(wc, ui::kCream, 0.4f), 0.4f), 10);
  ui::zonaiRing({spot.x, spot.y + 50}, 205, t, 0.75f);
  ui::stoneDisc({330, 634}, 190, 30, t);
  if (winnerChar_ >= 0) {
    const CharacterDef& d = r[winnerChar_];
    Pose p = restPose(d, t);
    float H = d.height;
    p.handF = {0.42f * H, -0.46f * H};  // arms wide open, away from the face
    p.handB = {-0.32f * H, -0.44f * H};
    p.weaponDir = Vec2{0.35f, -1.0f}.norm();
    if (d.style == BodyStyle::Puff) p.handF = {0.32f * H, -0.2f * H}, p.handB = {-0.32f * H, -0.2f * H};
    float hop = std::fabs(std::sin(t * 4.0f)) * 18.0f;
    DrawEllipse(330, 632, 70.0f - hop, 11.0f, withAlpha(ui::kInk, 0.3f));
    drawCharacter(d, winnerPalette_, p, {330, 630 - hop}, 1, 250.0f / H, 1.0f, WHITE, 0.0f, t);
    ui::drawSprite(ui::Sprite::Victory, 330 - ui::spriteWidth(ui::Sprite::Victory, 92) * 0.5f, 26, 92);
    drawTextCentered(d.name.c_str(), 330, 134, 44, wc, true, FontKind::Title);
    drawTextCentered(winnerCpu_ ? "Ordinateur" : TextFormat("Joueur %d", winnerPort_ + 1), 330, 186, 22, ui::kCream);
  } else {
    ui::drawSprite(ui::Sprite::Draw, 330 - ui::spriteWidth(ui::Sprite::Draw, 80) * 0.5f, 60, 80);
  }

  const float tx = 650, ty = 112;
  const int n = int(results_.size());
  ui::panel({tx - 20, ty - 18, 610, 76.0f + n * 96.0f}, withAlpha({18, 28, 32, 255}, 0.9f), withAlpha(ui::kGold, 0.85f),
            0.06f);
  const char* heads[3] = {"K.O.", "CHUTES", "DÉGÂTS"};
  for (int i = 0; i < 3; ++i) drawTextCentered(heads[i], tx + 330 + i * 100.0f, ty, 18, ui::kMuted, false);
  const Color medals[3] = {ui::kGold, {204, 212, 214, 255}, {206, 138, 86, 255}};
  for (int i = 0; i < n; ++i) {
    const ResultRow& row = results_[i];
    float enter = clampf((screenFrames_ - 20 - i * 8) / 16.0f, 0.0f, 1.0f);
    float y = ty + 36 + i * 96.0f;
    float x = tx + (1.0f - enter) * 700.0f;
    Color pc = row.cpu ? cpuColor() : portColor(row.port);
    const Rectangle rr{x, y, 570, 84};
    ui::slanted(rr, 16, withAlpha(mixColor(pc, ui::kInk, 0.55f), 0.95f), withAlpha(mixColor(pc, ui::kInk, 0.82f), 0.95f));
    ui::slanted({x + 16 - 5, y, 9, 84}, 16, pc, shade(pc, 0.6f));  // player colour edge
    ui::slantedOutline(rr, 16, i == 0 ? 2.5f : 1.5f, i == 0 ? medals[0] : withAlpha(ui::kStone, 0.5f));
    if (i == 0) ui::corners(ui::grow(rr, 4), 16, 16, 2.5f, medals[0]);
    drawTextOutlined(TextFormat("%d", i + 1), x + 26, y + 16, 50, i < 3 ? medals[i] : ui::kCream, ui::kInk, 4.0f,
                     FontKind::Title);
    drawPortrait(r[row.charIndex], row.palette, {x + 104, y + 40}, 64, t + i);
    float ns = 30.0f;  // long names shrink before the K.O. column
    while (ns > 16.0f && textWidth(r[row.charIndex].name.c_str(), ns, FontKind::Title) > 140.0f) ns -= 1.0f;
    drawTextOutlined(r[row.charIndex].name.c_str(), x + 146, y + 12 + (30 - ns) * 0.5f, ns, ui::kCream, ui::kInk, 3.0f,
                     FontKind::Title);
    drawText(row.cpu ? "Ordinateur" : TextFormat("Joueur %d", row.port + 1), x + 148, y + 50, 16, ui::kMuted);
    drawTextCentered(TextFormat("%d", row.kos), x + 330, y + 22, 38, ui::kCream, true, FontKind::Title);
    drawTextCentered(TextFormat("%d", row.falls), x + 430, y + 22, 38, ui::kCream, true, FontKind::Title);
    drawTextCentered(TextFormat("%d%%", int(row.damage)), x + 525, y + 22, 34, ui::kCream, true, FontKind::Title);
  }
  if (screenFrames_ > 40) ui::footer({{"ENTRÉE", ui::Btn::A, "Continuer"}});
}
