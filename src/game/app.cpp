// Application shell: window, main loop, title screen and results screen.
#include "game/app.hpp"

#include "game/render.hpp"

App::App(Options opt) : opt_(std::move(opt)) {
  slots_[0] = {1, kKeyboard1, 0, 5};
  slots_[1] = {2, kKeyboard2, 2, 5};
  slots_[2] = {0, kPad1, 3, 5};
  slots_[3] = {0, kPad2, 4, 5};
  if (opt_.stage >= 0) stage_ = opt_.stage % int(stageList().size());
}

int App::activeSlots() const {
  int n = 0;
  for (const auto& s : slots_) n += s.type != 0;
  return n;
}

void App::goTo(Screen s) {
  screen_ = s;
  screenFrames_ = 0;
  if (s != Screen::Match) audio_.setMusic(1);
}

void App::startMatch() {
  MatchConfig cfg;
  cfg.stage = stage_;
  cfg.stocks = stocks_;
  cfg.seed = uint32_t(frames_) * 2654435761u + 12345u;
  std::array<int, 4> devices{};
  for (int i = 0; i < 4; ++i) {
    const SlotSetup& s = slots_[i];
    if (s.type == 0) continue;
    cfg.slots[i] = {true, s.character, s.type == 2, s.level};
    devices[i] = s.device;
  }
  match_ = std::make_unique<MatchScreen>(cfg, devices, audio_, opt_.debug);
  goTo(Screen::Match);
  audio_.setMusic(2);
}

void App::finishMatch() {
  const Match& m = match_->match();
  results_.clear();
  for (int id : m.ranking()) {
    const Fighter& f = m.fighters[id];
    results_.push_back({f.charIndex, f.palette, f.port, f.cpu, f.kos, f.falls, f.damageDealt});
  }
  winnerChar_ = -1;
  if (m.winner >= 0) {
    const Fighter& w = m.fighters[m.winner];
    winnerChar_ = w.charIndex;
    winnerPalette_ = w.palette;
    winnerPort_ = w.port;
    winnerCpu_ = w.cpu;
  }
  match_.reset();
  goTo(Screen::Results);
}

void App::run() {
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
  InitWindow(kScreenW, kScreenH, "Super Clash Bros.");
  SetExitKey(KEY_NULL);
  SetTargetFPS(60);
  audio_.init();
  RenderTexture2D target = LoadRenderTexture(kScreenW, kScreenH);
  SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

  if (opt_.demo) {
    for (int i = 0; i < 4; ++i) slots_[i] = {2, kKeyboard1, (i * 2 + 1) % int(roster().size()), 9 - i};
    if (opt_.stage < 0) stage_ = 1;
    startMatch();
  } else if (opt_.screen == "css") {
    goTo(Screen::CharSelect);
  } else if (opt_.screen == "stage") {
    goTo(Screen::StageSelect);
  } else if (opt_.screen == "results") {
    for (int i = 0; i < 4; ++i) slots_[i] = {2, kKeyboard1, i, 9};
    startMatch();
    while (match_->update() != MatchScreen::Result::Finished) {}
    finishMatch();
  } else {
    goTo(Screen::Title);
  }

  while (!quit_ && !WindowShouldClose()) {
    ++frames_;
    ++screenFrames_;
    time_ += 1.0f / 60.0f;
    if (IsKeyPressed(KEY_M)) audio_.toggleMute();
    if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
    MenuNav nav = screen_ != Screen::Match ? readMenuNav() : MenuNav{};
    switch (screen_) {
      case Screen::Title: updateTitle(nav); break;
      case Screen::CharSelect: updateCss(nav); break;
      case Screen::StageSelect: updateStage(nav); break;
      case Screen::Results: updateResults(nav); break;
      case Screen::Match: {
        MatchScreen::Result r = match_->update();
        if (r == MatchScreen::Result::Finished) finishMatch();
        else if (r == MatchScreen::Result::Quit) {
          match_.reset();
          goTo(Screen::CharSelect);
        }
        break;
      }
    }

    BeginTextureMode(target);
    ClearBackground(BLACK);
    switch (screen_) {
      case Screen::Title: drawTitle(); break;
      case Screen::CharSelect: drawCss(); break;
      case Screen::StageSelect: drawStage(); break;
      case Screen::Results: drawResults(); break;
      case Screen::Match: if (match_) match_->draw(); break;
    }
    if (audio_.muted()) drawTextOutlined("SON COUPE (M)", kScreenW - 170, 8, 16, {255, 200, 200, 255});
    EndTextureMode();

    for (const auto& [frame, path] : opt_.shots) {
      if (frame != frames_) continue;
      Image img = LoadImageFromTexture(target.texture);
      ImageFlipVertical(&img);
      ExportImage(img, path.c_str());
      UnloadImage(img);
    }
    if (opt_.quitAfter > 0 && frames_ >= opt_.quitAfter) quit_ = true;

    BeginDrawing();
    ClearBackground(BLACK);
    float sw = float(GetScreenWidth()), sh = float(GetScreenHeight());
    float sc = std::min(sw / kScreenW, sh / kScreenH);
    Rectangle dst{(sw - kScreenW * sc) * 0.5f, (sh - kScreenH * sc) * 0.5f, kScreenW * sc, kScreenH * sc};
    DrawTexturePro(target.texture, {0, 0, float(kScreenW), -float(kScreenH)}, dst, {0, 0}, 0.0f, WHITE);
    EndDrawing();
    SetMouseOffset(int(-dst.x), int(-dst.y));
    SetMouseScale(1.0f / sc, 1.0f / sc);
  }
  match_.reset();
  UnloadRenderTexture(target);
  audio_.shutdown();
  CloseWindow();
}

// ---------------------------------------------------------------------------
// Title screen
// ---------------------------------------------------------------------------
void App::updateTitle(const MenuNav& nav) {
  if (nav.up || nav.down) {
    titleSel_ = 1 - titleSel_;
    audio_.play(Sfx::MenuMove);
  }
  for (int i = 0; i < 2; ++i) {
    Rectangle r{kScreenW * 0.5f - 160, 470.0f + i * 56.0f, 320, 46};
    if (CheckCollisionPointRec(GetMousePosition(), r)) {
      if (titleSel_ != i && (GetMouseDelta().x != 0 || GetMouseDelta().y != 0)) titleSel_ = i;
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        titleSel_ = i;
        if (i == 0) {
          audio_.play(Sfx::MenuSelect);
          goTo(Screen::CharSelect);
        } else {
          quit_ = true;
        }
        return;
      }
    }
  }
  if (nav.confirm || nav.start) {
    audio_.play(Sfx::MenuSelect);
    if (titleSel_ == 0) goTo(Screen::CharSelect);
    else quit_ = true;
  }
}

void App::drawTitle() {
  drawBackground(stageList()[0], {std::sin(time_ * 0.2f) * 400.0f, 0}, 1.0f, time_);
  // Lineup of fighters on a glowing platform
  const auto& r = roster();
  float baseY = 400.0f;
  DrawRectangleGradientV(140, int(baseY), kScreenW - 280, 26, {90, 230, 255, 200}, {40, 60, 120, 0});
  DrawRectangle(140, int(baseY), kScreenW - 280, 5, {150, 240, 255, 255});
  for (int i = 0; i < int(r.size()); ++i) {
    float x = 240.0f + i * 160.0f;
    Pose p = restPose(r[i], time_ + i);
    float hop = std::max(0.0f, std::sin(time_ * 2.5f + i * 1.1f)) * 10.0f;
    drawCharacter(r[i], 0, p, {x, baseY - hop}, i < 3 ? 1 : -1, 1.25f, 1.0f, WHITE, 0.0f, time_ + i);
  }
  // Logo
  float bounce = std::sin(time_ * 2.0f) * 6.0f;
  drawTextCentered("SUPER", kScreenW * 0.5f, 58 + bounce, 48, {255, 240, 200, 255});
  const char* logo = "CLASH BROS.";
  float size = 112.0f;
  float w = textWidth(logo, size);
  float x0 = kScreenW * 0.5f - w * 0.5f;
  drawTextOutlined(logo, x0 + 6, 106 + bounce + 6, size, {0, 0, 0, 160}, {0, 0, 0, 0}, 0.0f);
  drawTextOutlined(logo, x0, 106 + bounce, size, {255, 210, 60, 255}, {150, 20, 30, 255}, 5.0f);
  drawTextCentered("Le choc des combattants", kScreenW * 0.5f, 228 + bounce, 26, {200, 230, 255, 255});

  const char* items[2] = {"COMBAT", "QUITTER"};
  for (int i = 0; i < 2; ++i) {
    Rectangle rr{kScreenW * 0.5f - 160, 470.0f + i * 56.0f, 320, 46};
    bool sel = i == titleSel_;
    DrawRectangleRounded(rr, 0.4f, 8, sel ? Color{230, 60, 60, 255} : Color{30, 30, 60, 200});
    DrawRectangleRoundedLinesEx(rr, 0.4f, 8, 3, sel ? Color{255, 220, 80, 255} : Color{120, 120, 160, 255});
    drawTextCentered(items[i], rr.x + rr.width * 0.5f, rr.y + 9, 30, WHITE);
  }
  if (int(time_ * 2) % 2 == 0) drawTextCentered("Appuyez sur ENTREE", kScreenW * 0.5f, 600, 22, {255, 255, 255, 220});
  drawTextCentered("M : son   -   F11 : plein ecran   -   Manettes supportees", kScreenW * 0.5f, 680, 18,
                   {170, 180, 220, 255});
}

// ---------------------------------------------------------------------------
// Results screen
// ---------------------------------------------------------------------------
void App::updateResults(const MenuNav& nav) {
  if (screenFrames_ > 40 && (nav.confirm || nav.start || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
    audio_.play(Sfx::MenuSelect);
    goTo(Screen::CharSelect);
  }
}

void App::drawResults() {
  DrawRectangleGradientV(0, 0, kScreenW, kScreenH, {30, 20, 60, 255}, {120, 40, 80, 255});
  for (int i = 0; i < 80; ++i) {  // confetti
    float x = std::fmod(i * 97.0f + std::sin(time_ + i) * 30.0f, float(kScreenW));
    float y = std::fmod(i * 53.0f + time_ * (60.0f + (i % 7) * 15.0f), float(kScreenH + 40)) - 20.0f;
    Color c = portColor(i);
    DrawRectanglePro({x, y, 10, 6}, {5, 3}, time_ * 200.0f + i * 40.0f, c);
  }
  const auto& r = roster();
  if (winnerChar_ >= 0) {
    const CharacterDef& d = r[winnerChar_];
    Color pc = winnerCpu_ ? cpuColor() : portColor(winnerPort_);
    for (int k = 0; k < 6; ++k) DrawCircleV({330, 380}, 250.0f - k * 30.0f, withAlpha(pc, 0.05f + k * 0.03f));
    Pose p = restPose(d, time_);
    float H = d.height;
    p.handF = {0.30f * H, -0.42f * H};
    p.handB = {-0.24f * H, -0.38f * H};
    p.weaponDir = Vec2{0.35f, -1.0f}.norm();
    if (d.style == BodyStyle::Puff) p.handF = {0.32f * H, -0.2f * H}, p.handB = {-0.32f * H, -0.2f * H};
    float hop = std::fabs(std::sin(time_ * 4.0f)) * 18.0f;
    float scale = 250.0f / H;
    drawCharacter(d, winnerPalette_, p, {330, 620 - hop}, 1, scale, 1.0f, WHITE, 0.0f, time_);
    drawTextCentered(TextFormat("VICTOIRE DE %s !", d.name.c_str()), 330, 40, 52, {255, 230, 90, 255});
    drawTextCentered(winnerCpu_ ? "(CPU)" : TextFormat("(Joueur %d)", winnerPort_ + 1), 330, 100, 28, pc);
  } else {
    drawTextCentered("MATCH NUL !", 330, 60, 60, WHITE);
  }

  float tx = 640, ty = 150;
  DrawRectangleRounded({tx - 20, ty - 20, 620, 80.0f + results_.size() * 92.0f}, 0.08f, 8, withAlpha(BLACK, 0.45f));
  const char* heads[3] = {"K.O.", "CHUTES", "DEGATS"};
  for (int i = 0; i < 3; ++i) drawTextCentered(heads[i], tx + 310 + i * 105.0f, ty, 18, {200, 200, 230, 255});
  for (size_t i = 0; i < results_.size(); ++i) {
    const ResultRow& row = results_[i];
    float y = ty + 40 + i * 92.0f;
    Color pc = row.cpu ? cpuColor() : portColor(row.port);
    DrawRectangleRounded({tx, y, 580, 80}, 0.25f, 8, withAlpha(shade(pc, 0.5f), 0.9f));
    drawTextOutlined(TextFormat("%d", int(i) + 1), tx + 14, y + 18, 44, i == 0 ? Color{255, 215, 60, 255} : WHITE);
    drawPortrait(r[row.charIndex], row.palette, {tx + 92, y + 40}, 62, time_ + i);
    drawTextOutlined(r[row.charIndex].name.c_str(), tx + 132, y + 14, 24, WHITE);
    drawTextOutlined(row.cpu ? "CPU" : TextFormat("J%d", row.port + 1), tx + 132, y + 44, 18, pc);
    drawTextCentered(TextFormat("%d", row.kos), tx + 310, y + 24, 32, WHITE);
    drawTextCentered(TextFormat("%d", row.falls), tx + 415, y + 24, 32, WHITE);
    drawTextCentered(TextFormat("%d%%", int(row.damage)), tx + 520, y + 24, 30, WHITE);
  }
  if (screenFrames_ > 40 && int(time_ * 2) % 2 == 0)
    drawTextCentered("ENTREE : continuer", 960, 670, 24, WHITE);
}
