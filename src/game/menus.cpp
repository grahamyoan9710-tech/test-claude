// Character select and stage select screens.
#include "game/app.hpp"
#include "game/render.hpp"

namespace {
constexpr float kCardW = 290, kCardH = 360, kCardGap = 16, kCardY = 290;

float cardX(int col) { return (kScreenW - (4 * kCardW + 3 * kCardGap)) * 0.5f + col * (kCardW + kCardGap); }

Rectangle fieldRect(int col, int row) {
  if (row < 0) return {kScreenW * 0.5f - 110, 74, 220, 36};
  float x = cardX(col);
  switch (row) {
    case 0: return {x + 20, kCardY + 48, kCardW - 40, 34};
    case 1: return {x + 10, kCardY + 90, kCardW - 20, 220};
    default: return {x + 20, kCardY + 316, kCardW - 40, 34};
  }
}

void drawField(Rectangle r, const char* text, bool focused, bool enabled, float time) {
  DrawRectangleRounded(r, 0.35f, 8, enabled ? Color{20, 22, 45, 220} : Color{40, 40, 50, 160});
  if (focused) {
    float a = 0.6f + 0.4f * std::sin(time * 8.0f);
    DrawRectangleRoundedLinesEx(r, 0.35f, 8, 4, withAlpha({255, 220, 70, 255}, a));
  }
  if (text) {
    drawTextCentered(text, r.x + r.width * 0.5f, r.y + r.height * 0.5f - 11, 22, enabled ? WHITE : GRAY);
    if (focused && r.height < 60) {
      drawTextOutlined("<", r.x + 8, r.y + r.height * 0.5f - 11, 22, {255, 220, 70, 255});
      drawTextOutlined(">", r.x + r.width - 22, r.y + r.height * 0.5f - 11, 22, {255, 220, 70, 255});
    }
  }
}
}  // namespace

void App::changeField(int col, int row, int delta) {
  audio_.play(Sfx::MenuMove);
  if (row < 0) {
    stocks_ = 1 + (stocks_ - 1 + delta + 5) % 5;
    return;
  }
  SlotSetup& s = slots_[col];
  const int nChars = int(roster().size());
  if (s.type == 0 && row > 0) row = 0;  // pressing on an empty card joins it
  switch (row) {
    case 0: {
      s.type = (s.type + delta + 3) % 3;
      if (s.type == 1) {  // pick a free input device
        for (int d = 0; d < kDeviceCount; ++d) {
          bool used = false;
          for (int j = 0; j < 4; ++j)
            if (j != col && slots_[j].type == 1 && slots_[j].device == d) used = true;
          if (!used) {
            s.device = d;
            break;
          }
        }
      }
      break;
    }
    case 1:
      if (s.type != 0) s.character = (s.character + delta + nChars) % nChars;
      break;
    default:
      if (s.type == 1) s.device = (s.device + delta + kDeviceCount) % kDeviceCount;
      else if (s.type == 2) s.level = 1 + (s.level - 1 + delta + 9) % 9;
      break;
  }
}

void App::updateCss(const MenuNav& nav) {
  if (nav.up) cssRow_ = std::max(-1, cssRow_ - 1);
  if (nav.down) cssRow_ = std::min(2, cssRow_ + 1);
  if (cssRow_ >= 0) {
    if (nav.left) cssCol_ = (cssCol_ + 3) % 4;
    if (nav.right) cssCol_ = (cssCol_ + 1) % 4;
  } else if (nav.left || nav.right) {
    changeField(0, -1, nav.left ? -1 : 1);
  }
  if (nav.up || nav.down || ((nav.left || nav.right) && cssRow_ >= 0)) audio_.play(Sfx::MenuMove, 0.6f);

  bool prev = IsKeyPressed(KEY_K) || IsKeyPressed(KEY_KP_2) || IsKeyPressed(KEY_SLASH);
  for (int g = 0; g < 4; ++g)
    if (IsGamepadAvailable(g) && IsGamepadButtonPressed(g, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) prev = true;
  bool back = nav.back && !prev;
  if (nav.confirm && !nav.start) changeField(cssCol_, cssRow_, 1);
  if (prev) changeField(cssCol_, cssRow_, -1);

  Vector2 mouse = GetMousePosition();
  bool moved = GetMouseDelta().x != 0 || GetMouseDelta().y != 0;
  for (int row = -1; row <= 2; ++row)
    for (int col = 0; col < (row < 0 ? 1 : 4); ++col) {
      if (!CheckCollisionPointRec(mouse, fieldRect(col, row))) continue;
      if (moved) {
        cssRow_ = row;
        cssCol_ = row < 0 ? cssCol_ : col;
      }
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) changeField(col, row, 1);
      if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) changeField(col, row, -1);
    }
  Rectangle go{kScreenW - 250.0f, 664, 220, 44};
  bool clickGo = CheckCollisionPointRec(mouse, go) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

  if (nav.start || clickGo) {
    if (activeSlots() >= 2) {
      audio_.play(Sfx::MenuSelect);
      goTo(Screen::StageSelect);
    } else {
      audio_.play(Sfx::MenuBack);
    }
  } else if (back) {
    audio_.play(Sfx::MenuBack);
    goTo(Screen::Title);
  }
}

void App::drawCss() {
  drawBackground(stageList()[1], {std::sin(time_ * 0.15f) * 300.0f, 0}, 1.0f, time_);
  DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(BLACK, 0.35f));
  drawTextCentered("CHOISISSEZ VOS COMBATTANTS", kScreenW * 0.5f, 18, 42, {255, 230, 120, 255});
  drawField(fieldRect(0, -1), TextFormat("VIES : %d", stocks_), cssRow_ < 0, true, time_);

  // Roster strip
  const auto& r = roster();
  const float bw = 150, bh = 150, gap = 14;
  float x0 = (kScreenW - (r.size() * bw + (r.size() - 1) * gap)) * 0.5f;
  for (int i = 0; i < int(r.size()); ++i) {
    float x = x0 + i * (bw + gap), y = 122;
    DrawRectangleRounded({x, y, bw, bh}, 0.15f, 8, withAlpha({25, 25, 50, 255}, 0.85f));
    drawPortrait(r[i], 0, {x + bw * 0.5f, y + 66}, 100, time_ + i);
    drawTextCentered(r[i].name.c_str(), x + bw * 0.5f, y + bh - 28, 20, WHITE);
    int tags = 0;
    for (int s = 0; s < 4; ++s) {
      if (slots_[s].type == 0 || slots_[s].character != i) continue;
      Color pc = slots_[s].type == 2 ? cpuColor() : portColor(s);
      DrawRectangleRoundedLinesEx({x, y, bw, bh}, 0.15f, 8, 3, pc);
      DrawRectangleRounded({x + 4 + tags * 36.0f, y + 4, 34, 22}, 0.4f, 6, pc);
      drawTextCentered(slots_[s].type == 2 ? "CPU" : TextFormat("J%d", s + 1), x + 21 + tags * 36.0f, y + 6, 16, WHITE);
      ++tags;
    }
  }

  // Player cards
  for (int col = 0; col < 4; ++col) {
    const SlotSetup& s = slots_[col];
    float x = cardX(col);
    Color pc = s.type == 1 ? portColor(col) : s.type == 2 ? cpuColor() : Color{70, 70, 80, 255};
    DrawRectangleRounded({x + 5, kCardY + 6, kCardW, kCardH}, 0.08f, 8, withAlpha(BLACK, 0.4f));
    DrawRectangleRounded({x, kCardY, kCardW, kCardH}, 0.08f, 8, withAlpha(shade(pc, 0.45f), 0.92f));
    DrawRectangleRounded({x, kCardY, kCardW, 40}, 0.3f, 8, pc);
    drawTextOutlined(TextFormat("JOUEUR %d", col + 1), x + 14, kCardY + 9, 22, WHITE);
    const char* type = s.type == 1 ? "HUMAIN" : s.type == 2 ? "ORDI" : "-- VIDE --";
    drawField(fieldRect(col, 0), type, cssRow_ == 0 && cssCol_ == col, true, time_);
    Rectangle fr = fieldRect(col, 1);
    drawField(fr, nullptr, cssRow_ == 1 && cssCol_ == col, s.type != 0, time_);
    if (s.type != 0) {
      const CharacterDef& d = r[s.character];
      int pal = 0;
      for (int j = 0; j < col; ++j)
        if (slots_[j].type != 0 && slots_[j].character == s.character) pal = 1 - pal;
      Pose p = restPose(d, time_ + col);
      float scale = 140.0f / d.height;
      drawCharacter(d, pal, p, {fr.x + fr.width * 0.5f, fr.y + 168}, 1, scale, 1.0f, WHITE, 0.0f, time_ + col);
      drawTextCentered(d.name.c_str(), fr.x + fr.width * 0.5f, fr.y + 174, 26, WHITE);
      drawTextCentered(d.title.c_str(), fr.x + fr.width * 0.5f, fr.y + 200, 16, {210, 210, 230, 255});
      if (cssRow_ == 1 && cssCol_ == col) {
        drawTextOutlined("<", fr.x + 8, fr.y + 90, 30, {255, 220, 70, 255});
        drawTextOutlined(">", fr.x + fr.width - 24, fr.y + 90, 30, {255, 220, 70, 255});
      }
    } else {
      drawTextCentered("ESPACE pour", fr.x + fr.width * 0.5f, fr.y + 80, 20, GRAY);
      drawTextCentered("rejoindre", fr.x + fr.width * 0.5f, fr.y + 104, 20, GRAY);
    }
    const char* opt = s.type == 1 ? deviceName(s.device) : s.type == 2 ? TextFormat("NIVEAU %d", s.level) : nullptr;
    drawField(fieldRect(col, 2), opt, cssRow_ == 2 && cssCol_ == col, s.type != 0, time_);
    if (s.type == 1 && !deviceAvailable(s.device))
      drawTextCentered("(non branchee)", x + kCardW * 0.5f, kCardY + kCardH - 10, 14, {255, 150, 150, 255});
  }

  Rectangle go{kScreenW - 250.0f, 664, 220, 44};
  bool ok = activeSlots() >= 2;
  DrawRectangleRounded(go, 0.4f, 8, ok ? Color{220, 50, 50, 255} : Color{80, 80, 90, 255});
  drawTextCentered("ENTREE : ARENE >", go.x + go.width * 0.5f, go.y + 12, 20, WHITE);
  drawTextOutlined("Fleches : naviguer   Espace/J : modifier   K : precedent   Echap : retour", 30, 678, 18,
                   {200, 205, 230, 255});
  if (!ok) drawTextCentered("Il faut au moins 2 combattants !", kScreenW * 0.5f, 650, 18, {255, 150, 150, 255});
}

// ---------------------------------------------------------------------------
// Stage select
// ---------------------------------------------------------------------------
void App::updateStage(const MenuNav& nav) {
  const int n = int(stageList().size());
  if (nav.left) { stage_ = (stage_ + n - 1) % n; audio_.play(Sfx::MenuMove); }
  if (nav.right) { stage_ = (stage_ + 1) % n; audio_.play(Sfx::MenuMove); }
  bool go = nav.confirm || nav.start;
  for (int i = 0; i < n; ++i) {
    Rectangle r{40.0f + i * 410.0f, 210, 380, 250};
    if (!CheckCollisionPointRec(GetMousePosition(), r)) continue;
    if (GetMouseDelta().x != 0 || GetMouseDelta().y != 0) stage_ = i;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      stage_ = i;
      go = true;
    }
  }
  if (go) {
    audio_.play(Sfx::MenuSelect);
    startMatch();
  } else if (nav.back) {
    audio_.play(Sfx::MenuBack);
    goTo(Screen::CharSelect);
  }
}

void App::drawStage() {
  const auto& stages = stageList();
  drawBackground(stages[stage_], {std::sin(time_ * 0.3f) * 200.0f, 0}, 1.0f, time_);
  DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(BLACK, 0.3f));
  drawTextCentered("CHOISISSEZ L'ARENE", kScreenW * 0.5f, 60, 48, {255, 230, 120, 255});
  for (int i = 0; i < int(stages.size()); ++i) {
    bool sel = i == stage_;
    float grow = sel ? 10.0f + std::sin(time_ * 5.0f) * 3.0f : 0.0f;
    Rectangle r{40.0f + i * 410.0f - grow, 210 - grow, 380 + grow * 2, 250 + grow * 2};
    DrawRectangleRounded({r.x + 6, r.y + 8, r.width, r.height}, 0.06f, 8, withAlpha(BLACK, 0.5f));
    drawStagePreview(stages[i], r, time_);
    DrawRectangleRoundedLinesEx(r, 0.06f, 8, sel ? 6 : 3, sel ? Color{255, 220, 70, 255} : Color{220, 220, 240, 255});
    drawTextCentered(stages[i].name.c_str(), r.x + r.width * 0.5f, r.y + r.height + 18, 28, sel ? Color{255, 220, 70, 255} : WHITE);
  }
  drawTextCentered(TextFormat("%d combattants  -  %d vies", activeSlots(), stocks_), kScreenW * 0.5f, 560, 24, WHITE);
  drawTextCentered("Gauche/Droite : choisir   Entree : combattre !   Echap : retour", kScreenW * 0.5f, 640, 20,
                   {210, 215, 240, 255});
}
