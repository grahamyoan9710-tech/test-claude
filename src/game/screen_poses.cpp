// Development sheets. --screen poses --tab PAGE: every fighter at rest, with each of its weapons, walking,
// running and jumping, to check how the characters stand and hold their weapons. --screen anim --tab N:
// filmstrips of one fighter (N / 6) through its gaits and moves (page N % 6: gaits, jabs and tilts, smashes and
// aerials, specials, with its weapons, ledge and throws), to judge the animation.
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
      const float scale = std::min(1.12f, 112.0f / d.height);  // about the size of a close fight
      drawCharacterSprite(d, 0, p, {x0 + (c + 0.42f) * colW, base - (c == 5 ? 14.0f : 0.0f)}, 1, scale, 1.0f, WHITE,
                          0.0f, time_);
    }
  }
  drawText(TextFormat("page %d/%d", page + 1, pages), 12, kScreenH - 26, 16, ui::kMuted);
}

namespace {
// One filmstrip: `label`, then kCols poses sampled along the action.
struct Strip {
  const char* label;
  int move;      // MoveId, or -1 for a gait
  int gait = 0;  // 0 walk, 1 run, 2 jump arc, 3 ledge (hanging, then climbing up)
  int weapon = -1;  // the picked-up weapon held (0, 1), or none
};
}  // namespace

void App::drawAnimSheet() {
  constexpr int kCols = 10, kRows = 5;
  const auto& r = roster();
  constexpr int kPages = 6;
  const int ci = std::clamp(setTab_ / kPages, 0, int(r.size()) - 1), page = setTab_ % kPages;
  const CharacterDef& d = r[ci];
  static const Strip pages[kPages][kRows] = {
      {{"Marche", -1, 0}, {"Course", -1, 1}, {"Saut", -1, 2}, {"Attaque course", DashAttack}, {"Tilt avant", FTilt}},
      {{"Jab 1", Jab}, {"Jab 2", Jab2}, {"Jab 3", Jab3}, {"Tilt haut", UTilt}, {"Tilt bas", DTilt}},
      {{"Smash avant", FSmash}, {"Smash haut", USmash}, {"Smash bas", DSmash}, {"Aérien avant", FAir}, {"Aérien arrière", BAir}},
      {{"Spécial", NSpecial}, {"Spécial côté", SSpecial}, {"Spécial haut", USpecial}, {"Spécial bas", DSpecial},
       {"Ultime", FinalSmash}},
      {{"Jab", Jab}, {"M1 arme 1", Jab, 0, 0}, {"M1 arme 2", Jab, 0, 1}, {"Spécial arme 1", NSpecial, 0, 0},
       {"Spécial arme 2", NSpecial, 0, 1}},
      {{"Rebord", -1, 3}, {"Chope", Grab}, {"Frappe (chope)", Pummel}, {"Projection avant", FThrow},
       {"Projection arrière", BThrow}},
  };
  ClearBackground({58, 72, 84, 255});
  drawText(TextFormat("%s  —  page %d/%d", d.name.c_str(), page + 1, kPages), 12, 6, 22, ui::kCream);
  const float x0 = 150.0f, colW = 112.0f, y0 = 34.0f, rowH = 136.0f;
  for (int row = 0; row < kRows; ++row) {
    const Strip& st = pages[page][row];
    const float base = y0 + (row + 1) * rowH - 14.0f;
    if (row % 2 == 0) DrawRectangle(0, int(base - rowH + 14), kScreenW, int(rowH), {64, 79, 92, 255});
    DrawLine(0, int(base), kScreenW, int(base), {40, 50, 60, 255});
    MatchConfig cfg;
    cfg.items = false;
    cfg.weapons = false;
    cfg.slots[0] = {true, ci, true, 5};
    cfg.slots[1] = {true, ci == 0 ? 1 : 0, true, 5};
    Fighter held;  // the move as performed with the weapon in hand
    held.def = &d;
    held.weapon = st.weapon;
    const MoveDef* mvd = st.move >= 0 ? &held.moveDef(st.move) : nullptr;
    const bool exists = !mvd || mvd->duration > 0;
    drawText(st.label, 10, base - 74, 18, ui::kCream);
    if (mvd && exists) drawText(TextFormat("%d images", mvd->duration), 10, base - 50, 15, ui::kMuted);
    if (!exists) {
      drawText("(aucun)", 10, base - 50, 15, ui::kMuted);
      continue;
    }
    for (int c = 0; c < kCols; ++c) {
      Match m(cfg);
      Fighter& f = m.fighters[0];
      if (st.weapon >= 0) m.equipWeapon(f, st.weapon);
      f.facing = 1;
      f.grounded = true;
      float when = time_;
      float lift = 0.0f;
      bool active = false;
      bool onLedge = false;
      if (!mvd) {
        if (st.gait == 3) {  // hanging (4 columns), then climbing up (6), placed as the core does from the corner (0, 0)
          const Vec2 hang{-(d.width * 0.5f + 2.0f), d.height * 0.75f}, top{d.width * 0.7f, 0.0f};
          onLedge = true;
          f.grounded = false;
          if (c < 4) {
            f.state = FState::LedgeHang;
            f.pos = hang;
            when = time_ + c * 0.27f;  // the hanging loop at different moments
          } else {
            f.state = FState::LedgeClimb;
            f.stateFrame = (c - 4) * 18 / 5;
            f.ledgeFrom = hang;
            f.ledgeTo = top;
            const float t = smoothstep(std::min(1.0f, f.stateFrame / 18.0f));
            f.pos = lerpv(hang, top, t);
            f.pos.y -= std::sin(t * kPi) * 20.0f;
          }
        } else if (st.gait == 0) {  // one cycle of each gait (driven by the distance covered)
          f.state = FState::Walk;
          f.pos.x = c / float(kCols) * 0.76f * d.height;
        } else if (st.gait == 1) {
          f.state = FState::Run;
          f.stateFrame = 20;
          f.pos.x = c / float(kCols) * 1.32f * d.height;
        } else if (c < 2) {
          f.state = FState::JumpSquat;
          f.timer = d.jumpSquat;
          f.stateFrame = c == 0 ? 0 : std::max(0, d.jumpSquat - 1);
        } else if (c < 8) {
          f.state = FState::Air;
          f.grounded = false;
          f.vel = {0.0f, -12.0f + (c - 2) * 4.4f};
          lift = 70.0f * std::sin((c - 1) / 7.0f * kPi);
        } else {
          f.state = FState::Landing;
          f.timer = 8;
          f.stateFrame = (c - 8) * 4;
        }
      } else {
        const bool air = st.move >= NAir && st.move <= DAir;
        f.state = FState::Attack;
        f.move = st.move;
        f.moveFrame = int(std::round(c / float(kCols - 1) * float(mvd->duration - 1)));
        f.stateFrame = f.moveFrame;
        f.grounded = !air;
        if (air) lift = 30.0f;
        for (const auto& h : mvd->hitboxes) active = active || (f.moveFrame >= h.start && f.moveFrame < h.end);
      }
      const Pose p = computePose(f, m, when);
      const float scale = std::min(0.92f, 92.0f / d.height) * (onLedge ? 0.62f : 1.0f);
      const float cx = x0 + (c + 0.5f) * colW;
      if (active) DrawRectangle(int(cx - colW * 0.5f + 2), int(base - rowH + 18), int(colW - 4), 4, {230, 80, 70, 255});
      if (onLedge) {  // the stage's corner, the fighter where the core puts it
        const Vec2 corner{cx - 8.0f, base - d.height * 0.75f * scale - 6.0f};
        DrawRectangle(int(corner.x), int(corner.y), int(colW * 0.5f), int(base - corner.y), {92, 104, 116, 255});
        drawCharacterSprite(d, 0, p, corner + f.pos * scale, 1, scale, 1.0f, WHITE, 0.0f, time_);
        continue;
      }
      drawCharacterSprite(d, 0, p, {cx, base - lift}, 1, scale, 1.0f, WHITE, 0.0f, time_);
      if (mvd) drawText(TextFormat("%d", f.moveFrame), cx - colW * 0.45f, base - 16, 13, ui::kMuted);
    }
  }
}
