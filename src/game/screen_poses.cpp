// Development sheet (--screen poses --tab PAGE): every fighter at rest, with each of its weapons,
// walking, running and jumping, to check how the characters stand and hold their weapons.
#include <algorithm>
#include <cmath>

#include "core/match.hpp"
#include "game/app.hpp"
#include "game/render.hpp"
#include "game/ui.hpp"

void App::drawPoses() {
  constexpr int kPerPage = 5;
  const auto& r = roster();
  const int n = int(r.size());
  const int pages = (n + kPerPage - 1) / kPerPage;
  const int page = std::clamp(setTab_, 0, pages - 1);
  const char* heads[] = {"Repos", "Arme 1", "Arme 2", "Marche", "Course", "Saut"};
  constexpr int kCols = 6;
  const float x0 = 150.0f, colW = 188.0f, y0 = 40.0f, rowH = 136.0f;
  ClearBackground({58, 72, 84, 255});
  for (int c = 0; c < kCols; ++c) {
    if (c % 2 == 0) DrawRectangle(int(x0 + c * colW), 0, int(colW), kScreenH, {64, 79, 92, 255});
    drawTextCentered(heads[c], x0 + (c + 0.5f) * colW, 8, 20, ui::kCream, false);
  }
  for (int row = 0; row < kPerPage; ++row) {
    const int ci = page * kPerPage + row;
    if (ci >= n) break;
    const CharacterDef& d = r[ci];
    const float base = y0 + (row + 1) * rowH - 12.0f;
    DrawLine(0, int(base), kScreenW, int(base), {40, 50, 60, 255});
    drawText(d.name.c_str(), 12, base - 60, 20, ui::kCream);
    MatchConfig cfg;
    cfg.items = false;
    cfg.weapons = false;
    cfg.slots[0] = {true, ci, true, 5};
    cfg.slots[1] = {true, ci == 0 ? 1 : 0, true, 5};
    for (int c = 0; c < kCols; ++c) {
      Match m(cfg);
      Fighter& f = m.fighters[0];
      if (c == 1 || c == 2) m.equipWeapon(f, c - 1);
      f.facing = 1;
      f.grounded = true;
      f.state = FState::Idle;
      f.stateFrame = int(time_ * 60.0f);
      if (c == 3) f.state = FState::Walk;
      if (c == 4) f.state = FState::Run;
      if (c == 5) {
        f.state = FState::Air;
        f.grounded = false;
        f.vel = {0.0f, -2.0f};
      }
      const Pose p = computePose(f, m, time_);
      const float scale = std::min(1.0f, 100.0f / d.height);
      drawCharacterSprite(d, 0, p, {x0 + (c + 0.42f) * colW, base - (c == 5 ? 14.0f : 0.0f)}, 1, scale, 1.0f, WHITE,
                          0.0f, time_);
    }
  }
  drawText(TextFormat("page %d/%d", page + 1, pages), 12, kScreenH - 26, 16, ui::kMuted);
}
