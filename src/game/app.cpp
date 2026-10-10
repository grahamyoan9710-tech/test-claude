// Application shell: window, main loop, screen flow and transitions.
#include "game/app.hpp"

#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

extern "C" {
extern const unsigned char icon_png[];  // assets/icon.png, embedded at build time
extern const unsigned int icon_png_size;
}

namespace {
constexpr int kHalfTransition = 14;  // frames to cover the screen, same to reveal it

void setWindowIcon() {
  Image icon = LoadImageFromMemory(".png", icon_png, int(icon_png_size));
  if (icon.data == nullptr) return;
  ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
  SetWindowIcon(icon);
  UnloadImage(icon);
}
}  // namespace

App::App(Options opt) : opt_(std::move(opt)) {
  slots_[0] = {1, kKeyboard1, 0, 5};
  slots_[1] = {2, kKeyboard2, 2, 5};
  slots_[2] = {0, kPad1, 3, 5};
  slots_[3] = {0, kPad2, 4, 5};
  if (opt_.stage >= 0) stage_ = opt_.stage % int(stageList().size());
}

int App::activeSlots() const {
  if (training_) return 2;
  int n = 0;
  for (const auto& s : slots_) n += s.type != 0;
  return n;
}

void App::goTo(Screen s, bool instant) {
  if (instant) {
    enterScreen(s);
    return;
  }
  if (transition_ >= 0) return;
  next_ = s;
  transition_ = 0;
}

void App::enterScreen(Screen s) {
  screen_ = s;
  screenFrames_ = 0;
  if (s != Screen::Match) {
    match_.reset();
    audio_.setMusic(1);
  }
}

void App::toast(const char* text) {
  toast_ = text;
  toastTimer_ = 150;
}

void App::applyDisplaySettings() {
  const Settings& s = settings();
  float want = s.highQuality ? 2.0f : 1.0f;
  if (want != targetScale_ || target_.id == 0) {
    if (target_.id != 0) UnloadRenderTexture(target_);
    target_ = LoadRenderTexture(int(kScreenW * want), int(kScreenH * want));
    SetTextureFilter(target_.texture, TEXTURE_FILTER_BILINEAR);
    targetScale_ = want;
  }
  g_renderScale = targetScale_;
  bool fs = IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
  if (s.fullscreen != fs) ToggleBorderlessWindowed();
  audio_.setVolumes(s.masterVolume, s.musicVolume, s.sfxVolume);
}

void App::startMatch() {
  const Settings& st = settings();
  MatchConfig cfg;
  cfg.stage = stage_;
  cfg.stocks = st.stocks;
  cfg.items = training_ ? false : st.items;
  cfg.training = training_;
  cfg.seed = uint32_t(frames_) * 2654435761u + 12345u;
  std::array<int, 4> devices{};
  for (int i = 0; i < 4; ++i) {
    SlotSetup s = slots_[i];
    if (training_) {  // player 1 against a dummy
      if (i >= 2) continue;
      s.type = i == 0 ? 1 : 2;
    }
    if (s.type == 0) continue;
    cfg.slots[i] = {true, s.character, s.type == 2, s.level};
    devices[i] = s.device;
  }
  match_ = std::make_unique<MatchScreen>(cfg, devices, audio_, st.showHitboxes || opt_.debug);
  goTo(Screen::Match);
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
  goTo(Screen::Results);
}

void App::run() {
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
  InitWindow(kScreenW, kScreenH, "Super Clash Bros.");
  SetExitKey(KEY_NULL);
  SetTargetFPS(60);
  initPads();
  if (opt_.padGlyphs >= 0) forcePadPrompts(PadFamily(opt_.padGlyphs));
  setWindowIcon();
  rlDisableBackfaceCulling();  // pure 2D: polygons may be wound either way
  loadFonts();
  ui::bakeSprites();
  loadSettings();
  audio_.init();
  applyDisplaySettings();
  for (auto& s : slots_) s.level = settings().cpuLevel;

  if (opt_.demo) {
    for (int i = 0; i < 4; ++i) slots_[i] = {2, kKeyboard1, (i * 2 + 1) % int(roster().size()), 9 - i};
    if (opt_.stage < 0) stage_ = 1;
    startMatch();
    transition_ = -1;
    enterScreen(Screen::Match);
  } else if (opt_.screen == "menu") {
    enterScreen(Screen::MainMenu);
  } else if (opt_.screen == "css") {
    training_ = opt_.training;
    enterScreen(Screen::CharSelect);
  } else if (opt_.screen == "stage") {
    enterScreen(Screen::StageSelect);
  } else if (opt_.screen == "settings") {
    setTab_ = std::clamp(opt_.tab, 0, 3);
    setRow_ = opt_.tab >= 0 ? 0 : -1;
    enterScreen(Screen::Settings);
  } else if (opt_.screen == "training") {
    training_ = true;
    startMatch();
    transition_ = -1;
    enterScreen(Screen::Match);
  } else if (opt_.screen == "results") {
    for (int i = 0; i < 4; ++i) slots_[i] = {2, kKeyboard1, i, 9};
    startMatch();
    while (match_->update() != MatchScreen::Result::Finished) {}
    finishMatch();
    transition_ = -1;
    enterScreen(Screen::Results);
  } else {
    enterScreen(Screen::Title);
  }

  while (!quit_ && !WindowShouldClose()) {
    ++frames_;
    ++screenFrames_;
    time_ += 1.0f / 60.0f;
    if (toastTimer_ > 0) --toastTimer_;
    updatePads();
    const std::string padNotice = takePadNotice();
    if (!padNotice.empty()) toast(padNotice.c_str());
    // Weak machine: if the smooth (supersampled) rendering cannot keep up, fall back once.
    const bool testRun = !opt_.shots.empty() || opt_.quitAfter > 0;
    if (!testRun && settings().highQuality && !autoQualityDone_ && frames_ > 120) {
      const float dt = GetFrameTime();
      slowTime_ = dt > 1.0f / 45.0f ? slowTime_ + dt : std::max(0.0f, slowTime_ - dt * 0.5f);
      if (slowTime_ > 3.0f) {
        settings().highQuality = false;
        applyDisplaySettings();
        saveSettings();
        autoQualityDone_ = true;
        toast("Qualité graphique réglée sur « Normale » pour plus de fluidité");
      }
    }
    if (IsKeyPressed(KEY_F11)) {
      settings().fullscreen = !settings().fullscreen;
      applyDisplaySettings();
      saveSettings();
    }
    const bool blocked = transition_ >= 0;
    if (transition_ >= 0) {
      ++transition_;
      if (transition_ == kHalfTransition) enterScreen(next_);
      if (transition_ >= kHalfTransition * 2) transition_ = -1;
    }
    ui::newFrame();
    MenuNav nav = (screen_ != Screen::Match && !blocked) ? readMenuNav() : MenuNav{};
    if (!blocked) {
      switch (screen_) {
        case Screen::Title: updateTitle(nav); break;
        case Screen::MainMenu: updateMainMenu(nav); break;
        case Screen::CharSelect: updateCss(nav); break;
        case Screen::StageSelect: updateStage(nav); break;
        case Screen::Results: updateResults(nav); break;
        case Screen::Settings: updateSettings(nav); break;
        case Screen::Match: {
          MatchScreen::Result r = match_->update();
          if (r == MatchScreen::Result::Finished) finishMatch();
          else if (r == MatchScreen::Result::Quit) goTo(Screen::CharSelect);
          break;
        }
      }
    }

    BeginTextureMode(target_);
    rlScalef(g_renderScale, g_renderScale, 1.0f);
    ClearBackground(BLACK);
    // Standard alpha blending for colours, but the frame itself stays opaque: overlapping
    // anti-aliased edges must not punch translucent holes into the off-screen image.
    ui::frameBlend();
    switch (screen_) {
      case Screen::Title: drawTitle(); break;
      case Screen::MainMenu: drawMainMenu(); break;
      case Screen::CharSelect: drawCss(); break;
      case Screen::StageSelect: drawStage(); break;
      case Screen::Results: drawResults(); break;
      case Screen::Settings: drawSettings(); break;
      case Screen::Match: if (match_) match_->draw(); break;
    }
    if (toastTimer_ > 0) {
      float a = std::min(1.0f, toastTimer_ / 20.0f);
      float w = textWidth(toast_.c_str(), 20) + 48;
      Rectangle r{(kScreenW - w) * 0.5f, screen_ == Screen::Match ? 22.0f : kScreenH - 110.0f, w, 44};  // clear of the HUD
      DrawRectangleRounded(r, 0.5f, 10, withAlpha(ui::kInk, 0.9f * a));
      DrawRectangleRoundedLinesEx(r, 0.5f, 10, 2, withAlpha(ui::kGold, a));
      drawText(toast_.c_str(), r.x + 24, r.y + 10, 20, withAlpha(ui::kCream, a));
    }
    if (settings().showFps) drawTextOutlined(TextFormat("%d FPS", GetFPS()), 10, 8, 16, {150, 255, 150, 255});
    if (transition_ >= 0) ui::wipe(transition_ / float(kHalfTransition * 2));
    EndBlendMode();
    EndTextureMode();

    for (const auto& [frame, path] : opt_.shots) {
      if (frame != frames_) continue;
      Image img = LoadImageFromTexture(target_.texture);
      ImageFlipVertical(&img);
      if (img.width != kScreenW) ImageResize(&img, kScreenW, kScreenH);
      ExportImage(img, path.c_str());
      UnloadImage(img);
    }
    if (opt_.quitAfter > 0 && frames_ >= opt_.quitAfter) quit_ = true;
    if (opt_.pauseAt == frames_ && match_) match_->forcePause();

    BeginDrawing();
    ClearBackground(BLACK);
    float sw = float(GetScreenWidth()), sh = float(GetScreenHeight());
    float sc = std::min(sw / kScreenW, sh / kScreenH);
    Rectangle dst{(sw - kScreenW * sc) * 0.5f, (sh - kScreenH * sc) * 0.5f, kScreenW * sc, kScreenH * sc};
    DrawTexturePro(target_.texture, {0, 0, float(target_.texture.width), -float(target_.texture.height)}, dst,
                   {0, 0}, 0.0f, WHITE);
    EndDrawing();
    SetMouseOffset(int(-dst.x), int(-dst.y));
    SetMouseScale(1.0f / sc, 1.0f / sc);
  }
  saveSettings();
  match_.reset();
  UnloadRenderTexture(target_);
  ui::unloadSprites();
  unloadFonts();
  audio_.shutdown();
  CloseWindow();
}
