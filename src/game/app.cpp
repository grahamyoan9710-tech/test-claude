// Application shell: window, main loop, screen flow and transitions.
#include "game/app.hpp"
#include "game/fighter_art.hpp"

#include "game/pixel.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

extern "C" {
extern const unsigned char icon_png[];  // assets/icon.png, embedded at build time
extern const unsigned int icon_png_size;
}

namespace {

// FXAA (after Timothy Lottes' FXAA 3.11 « console »): only along the edges it finds; flat pixel art is left as it is.
const char* kFxaaFrag = R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec2 texel;
out vec4 finalColor;
float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }
void main() {
  vec2 uv = fragTexCoord;
  vec3 m = texture(texture0, uv).rgb;
  vec3 nw = texture(texture0, uv + vec2(-1.0, -1.0) * texel).rgb;
  vec3 ne = texture(texture0, uv + vec2(1.0, -1.0) * texel).rgb;
  vec3 sw = texture(texture0, uv + vec2(-1.0, 1.0) * texel).rgb;
  vec3 se = texture(texture0, uv + vec2(1.0, 1.0) * texel).rgb;
  float lm = luma(m), lnw = luma(nw), lne = luma(ne), lsw = luma(sw), lse = luma(se);
  float lo = min(lm, min(min(lnw, lne), min(lsw, lse))), hi = max(lm, max(max(lnw, lne), max(lsw, lse)));
  if (hi - lo < max(0.0312, hi * 0.125)) { finalColor = vec4(m, 1.0); return; }
  vec2 dir = vec2(-((lnw + lne) - (lsw + lse)), (lnw + lsw) - (lne + lse));
  float reduce = max((lnw + lne + lsw + lse) * 0.03125, 1.0 / 128.0);
  dir = clamp(dir / (min(abs(dir.x), abs(dir.y)) + reduce), -8.0, 8.0) * texel;
  vec3 a = 0.5 * (texture(texture0, uv - dir / 6.0).rgb + texture(texture0, uv + dir / 6.0).rgb);
  vec3 b = a * 0.5 + 0.25 * (texture(texture0, uv - dir * 0.5).rgb + texture(texture0, uv + dir * 0.5).rgb);
  float lb = luma(b);
  finalColor = vec4((lb < lo || lb > hi) ? a : b, 1.0);
}
)";
// Upscaling as FSR 1 ends it: the frame stretched to the screen, then AMD's RCAS (robust contrast-adaptive
// sharpening, its lobe limited so that no edge rings) on the screen's pixels.
const char* kRcasFrag = R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec2 stepUv;   // one screen pixel, in texture coordinates
uniform float sharp;   // 0..1 (FSR's con.x)
out vec4 finalColor;
void main() {
  vec2 uv = fragTexCoord;
  vec3 e = texture(texture0, uv).rgb;
  vec3 b = texture(texture0, uv - vec2(0.0, stepUv.y)).rgb;
  vec3 d = texture(texture0, uv - vec2(stepUv.x, 0.0)).rgb;
  vec3 f = texture(texture0, uv + vec2(stepUv.x, 0.0)).rgb;
  vec3 h = texture(texture0, uv + vec2(0.0, stepUv.y)).rgb;
  vec3 mn4 = min(min(b, d), min(f, h)), mx4 = max(max(b, d), max(f, h));
  vec3 hitMin = mn4 / (4.0 * mx4 + 1e-5);
  vec3 hitMax = (1.0 - mx4) / (4.0 * mn4 - 4.0 - 1e-5);
  vec3 l3 = max(-hitMin, hitMax);
  float lobe = max(-0.1875, min(max(l3.r, max(l3.g, l3.b)), 0.0)) * sharp;
  finalColor = vec4((lobe * (b + d + f + h) + e) / (4.0 * lobe + 1.0), 1.0);
}
)";

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

bool App::teamBattle() const { return !training_ && (settings().teams || teamsForced_); }

bool App::readyToFight() const {
  if (activeSlots() < 2) return false;
  if (!teamBattle()) return true;
  bool seen[2] = {false, false};
  for (int i = 0; i < 4; ++i)
    if (slots_[i].type != 0) seen[slotTeam(i)] = true;
  return seen[0] && seen[1];
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
  // « Prestige »: the blade cuts into the VS, the choc opens the fight, a brush closes it and wipes between the menus.
  wipe_ = s == Screen::Versus ? ui::Wipe::Cut
          : s == Screen::Match && screen_ != Screen::Settings ? ui::Wipe::Flash
          : screen_ == Screen::Match && s == Screen::Results ? ui::Wipe::Game
                                                              : ui::Wipe::Horizon;
  // « Prestige »: the brush in Hakuen's colour between the menus, in the winner's out of a fight; the choc in the colours
  // of the first two fighters
  ui::Flood a = ui::flood(Hero::H01), b = ui::flood(Hero::P01);
  if (match_ && match_->match().fighters.size() >= 2) {
    const Match& m = match_->match();
    if (wipe_ == ui::Wipe::Flash) a = ui::flood(m.fighters[0].def->hero), b = ui::flood(m.fighters[1].def->hero);
    if (wipe_ == ui::Wipe::Game && m.winner >= 0) a = ui::flood(m.fighters[m.cfg.teams ? m.ranking()[0] : m.winner].def->hero);
  }
  ui::wipeColors(a, b);
  if (wipe_ == ui::Wipe::Cut) {  // freeze the screen being left: its halves fly apart over the next one
    if (snap_.id == 0 || snap_.texture.width != target_.texture.width) {
      if (snap_.id != 0) UnloadRenderTexture(snap_);
      snap_ = LoadRenderTexture(target_.texture.width, target_.texture.height);
      SetTextureFilter(snap_.texture, TEXTURE_FILTER_BILINEAR);
    }
    BeginTextureMode(snap_);
    ClearBackground(BLACK);
    const Rectangle src{0, 0, float(target_.texture.width), -float(target_.texture.height)};
    DrawTexturePro(target_.texture, src, {0, 0, float(snap_.texture.width), float(snap_.texture.height)}, {0, 0}, 0.0f, WHITE);
    EndTextureMode();
    enterScreen(s);
  }
}

void App::enterScreen(Screen s) {
  if (s == Screen::CharSelect && screen_ != Screen::StageSelect) {  // a fresh visit
    padLeft_ = {};
    rulesOpen_ = false;
    for (int d = 0; d < kDeviceCount; ++d) cssCur_[d] = {ownedCard(d) >= 0, std::max(0, ownedCard(d)), 1};
    audio_.voice(training_ ? "training" : "choose");
  }
  if (s == Screen::StageSelect && screen_ == Screen::CharSelect) audio_.voice("stage");
  if (s == Screen::Versus) vsTip_ = GetRandomValue(0, 99);
  if (s == Screen::MainMenu) {  // each visit shows the next of the owner's three backgrounds
    settings().menuBg = (settings().menuBg + 1) % 3;
    saveSettings();
  }
  screen_ = s;
  screenFrames_ = 0;
  // The VS screen shows the match about to start; the settings opened from the pause go back to it.
  if (s != Screen::Match && s != Screen::Versus && !(s == Screen::Settings && settingsReturn_ == Screen::Match)) {
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
  // The window: only when the mode or the size changed, so a window the player resized by hand stays as it is.
  if (s.fullscreen != shownFs_ || s.resW != shownW_ || s.resH != shownH_) {
    shownFs_ = s.fullscreen;
    shownW_ = s.resW;
    shownH_ = s.resH;
    if (IsWindowFullscreen()) ToggleFullscreen();  // back to the desktop first: the screen's own size is read there
    const int m = GetCurrentMonitor(), mw = GetMonitorWidth(m), mh = GetMonitorHeight(m);
    if (s.fullscreen) {
      // As games do: the screen stays at its own size (no black bands from a switched screen mode) and a lower
      // resolution is the render's, scaled up to fill it.
      SetWindowSize(mw, mh);
      ToggleFullscreen();
    } else {
      const int w = std::min(s.resW ? s.resW : mw, mw), h = std::min(s.resH ? s.resH : mh, mh);
      SetWindowSize(w, h);
      SetWindowPosition(GetMonitorPosition(m).x + (mw - w) / 2, GetMonitorPosition(m).y + std::max(0, (mh - h) / 2));
    }
  }
  if (s.vsync) SetWindowState(FLAG_VSYNC_HINT);
  else ClearWindowState(FLAG_VSYNC_HINT);
  // The render: the resolution (the screen's by default) fitted to 16:9, times the upscaler's and the SSAA's scale.
  const int m = GetCurrentMonitor();
  const float rw = float(s.resW ? s.resW : GetMonitorWidth(m)), rh = float(s.resH ? s.resH : GetMonitorHeight(m));
  float want = std::min(rw / kScreenW, rh / kScreenH) * kUpscale[std::clamp(s.upscale, 0, 3)] * (s.antiAlias == 2 ? 2.0f : 1.0f);
  want = std::clamp(want > 0 ? want : 1.0f, 0.5f, 4.0f);
  if (std::fabs(want - targetScale_) > 0.001f || target_.id == 0) {
    if (target_.id != 0) UnloadRenderTexture(target_);
    target_ = LoadRenderTexture(int(kScreenW * want), int(kScreenH * want));
    SetTextureFilter(target_.texture, TEXTURE_FILTER_BILINEAR);
    targetScale_ = want;
  }
  g_renderScale = targetScale_;
  audio_.setVolumes(s.masterVolume, s.musicVolume, s.sfxVolume);
}

void App::startMatch() {
  const Settings& st = settings();
  MatchConfig cfg;
  cfg.stage = stageRandom_ ? GetRandomValue(0, int(stageList().size()) - 1) : stage_;
  cfg.stocks = st.stocks;
  cfg.items = training_ ? false : st.items;
  cfg.weapons = st.weapons;
  cfg.training = training_;
  cfg.teams = teamBattle();
  cfg.seed = uint32_t(frames_) * 2654435761u + 12345u;
  std::array<int, 4> devices{};
  for (int i = 0; i < 4; ++i) {
    SlotSetup s = slots_[i];
    if (training_) {  // player 1 against a dummy
      if (i >= 2) continue;
      s.type = i == 0 ? 1 : 2;
    }
    if (s.type == 0) continue;
    const int fighter = s.random ? GetRandomValue(0, int(roster().size()) - 1) : s.character;
    cfg.slots[i] = {true, fighter, s.type == 2, s.level, slotTeam(i)};
    devices[i] = s.device;
  }
  match_ = std::make_unique<MatchScreen>(cfg, devices, audio_, st.showHitboxes || opt_.debug);
  goTo(training_ ? Screen::Match : Screen::Versus);  // a fight shows who fights whom first
}

void App::finishMatch() {
  const Match& m = match_->match();
  results_.clear();
  for (int id : m.ranking()) {
    const Fighter& f = m.fighters[id];
    results_.push_back({f.charIndex, f.palette, f.port, f.cpu, f.kos, f.falls, f.damageDealt, f.inTeam ? f.team : -1});
    ResultRow& r = results_.back();
    r.orbs = match_->orbsBroken(id);
    r.bestHit = match_->bestHit(id);
    r.stocks = f.stocks;
    r.weapon = f.weapon;
    r.level = slots_[f.port & 3].level;
  }
  resultStage_ = m.stage().name;
  resultStocks_ = m.cfg.stocks;
  resultSeconds_ = match_->fightFrames() / 60;
  resSel_ = 0;
  resReady_ = {};
  resGo_ = -1;
  winnerChar_ = -1;
  winnerTeam_ = m.cfg.teams ? m.winnerTeam : -1;
  if (m.winner >= 0) {  // a team battle shows the best of the winning team (ranked first)
    const Fighter& w = m.fighters[m.cfg.teams ? m.ranking()[0] : m.winner];
    winnerChar_ = w.charIndex;
    winnerPalette_ = w.palette;
    winnerPort_ = w.port;
    winnerCpu_ = w.cpu;
  }
  goTo(Screen::Results);
}

void App::run() {
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
  InitWindow(kScreenW, kScreenH, "Clash Horizon");
  SetExitKey(KEY_NULL);
  SetTargetFPS(60);
  fxaa_ = LoadShaderFromMemory(nullptr, kFxaaFrag);
  rcas_ = LoadShaderFromMemory(nullptr, kRcasFrag);
  initPads();
  if (opt_.padGlyphs >= 0) forcePadPrompts(PadFamily(opt_.padGlyphs));
  setWindowIcon();
  rlDisableBackfaceCulling();  // pure 2D: polygons may be wound either way
  loadFonts();
  ui::bakeSprites();
  loadSettings();
  teamsForced_ = opt_.teams;  // --teams: for this run only (never saved)
  audio_.init();
  applyDisplaySettings();
  for (auto& s : slots_) s.level = settings().cpuLevel;

  if (opt_.demo) {
    const int n = int(roster().size());
    for (int i = 0; i < 4; ++i) slots_[i] = {2, kKeyboard1, (i * 2 + 1) % n, 9 - i};
    if (!opt_.fighters.empty())
      for (int i = 0; i < 4; ++i)
        slots_[i] = {i < int(opt_.fighters.size()) ? 2 : 0, kKeyboard1,
                     i < int(opt_.fighters.size()) ? std::clamp(opt_.fighters[i], 0, n - 1) : 0, 9 - i};
    if (opt_.stage < 0) stage_ = 1;
    startMatch();
    if (opt_.ultShow >= -1) match_->showUltimate(opt_.ultShow);
    if (opt_.weaponShow >= 0) match_->showWeapon(opt_.weaponShow);
    if (opt_.skillShow) match_->showSkill(opt_.skillShow);
    transition_ = -1;
    enterScreen(Screen::Match);
  } else if (opt_.screen == "menu") {
    enterScreen(Screen::MainMenu);
  } else if (opt_.screen == "title") {
    enterScreen(Screen::Title);
  } else if (opt_.screen == "css" || opt_.screen == "rules") {  // rules: the « Règles & mode » sheet open
    training_ = opt_.training;
    const int n = int(roster().size());
    for (int i = 0; i < int(std::min<size_t>(4, opt_.fighters.size())); ++i)  // --fighters: who is picked
      slots_[i] = {i == 0 ? 1 : 2, kKeyboard1, std::clamp(opt_.fighters[i], 0, n - 1), 9};
    enterScreen(Screen::CharSelect);
    rulesOpen_ = opt_.screen == "rules";
  } else if (opt_.screen == "stage") {
    enterScreen(Screen::StageSelect);
  } else if (opt_.screen == "versus") {
    const int n = int(roster().size());
    for (int i = 0; i < int(std::min<size_t>(4, opt_.fighters.size())); ++i)  // --fighters: who fights
      slots_[i] = {i == 0 ? 1 : 2, i == 0 ? kKeyboard1 : kKeyboard2, std::clamp(opt_.fighters[i], 0, n - 1), 7};
    startMatch();
    transition_ = -1;
    enterScreen(Screen::Versus);
  } else if (opt_.screen == "gallery") {
    gallerySel_ = std::max(0, opt_.tab);
    enterScreen(Screen::Gallery);
  } else if (opt_.screen == "settings") {
    setTab_ = std::clamp(opt_.tab, 0, 4);
    setRow_ = opt_.tab >= 0 ? 0 : -1;
    enterScreen(Screen::Settings);
  } else if (opt_.screen == "poses") {
    setTab_ = std::max(0, opt_.tab);
    enterScreen(Screen::Poses);
  } else if (opt_.screen == "anim") {
    setTab_ = std::max(0, opt_.tab);
    enterScreen(Screen::Anim);
  } else if (opt_.screen == "training") {
    training_ = true;
    for (int i = 0; i < int(std::min<size_t>(2, opt_.fighters.size())); ++i)  // --fighters: player 1, the dummy
      slots_[i].character = std::clamp(opt_.fighters[i], 0, int(roster().size()) - 1);
    startMatch();
    transition_ = -1;
    enterScreen(Screen::Match);
  } else if (opt_.screen == "results") {
    for (int i = 0; i < 4; ++i)  // J1 a human (it stands still), the others CPUs
      slots_[i] = {i == 0 ? 1 : 2, kKeyboard1, i < int(opt_.fighters.size()) ? std::clamp(opt_.fighters[i], 0, int(roster().size()) - 1) : i, 9};
    startMatch();
    while (match_->update() != MatchScreen::Result::Finished) {}
    finishMatch();
    transition_ = -1;
    enterScreen(Screen::Results);
  } else if (opt_.screen == "loading" || opt_.screen.empty()) {
    enterScreen(Screen::Loading);
  } else {
    enterScreen(Screen::Title);
  }

  while (!quit_ && !WindowShouldClose()) {
    ++frames_;
    ++screenFrames_;
    time_ += 1.0f / 60.0f;
    if (toastTimer_ > 0) --toastTimer_;
    updatePads();
    audio_.update();
    const std::string padNotice = takePadNotice();
    if (!padNotice.empty()) toast(padNotice.c_str());
    if (IsKeyPressed(KEY_F11)) {
      settings().fullscreen = !settings().fullscreen;
      applyDisplaySettings();
      saveSettings();
    }
    const bool blocked = transition_ >= 0;
    if (transition_ >= 0) {
      ++transition_;
      const int len = ui::wipeFrames(wipe_), swap = int(ui::wipeSwap(wipe_) * len);
      if (transition_ == swap && swap > 0) enterScreen(next_);
      if (transition_ >= len) transition_ = -1;
    }
    ui::newFrame();
    MenuNav nav = (screen_ != Screen::Match && !blocked) ? readMenuNav() : MenuNav{};
    if (!blocked) {
      switch (screen_) {
        case Screen::Loading: updateLoading(); break;
        case Screen::Title: updateTitle(nav); break;
        case Screen::MainMenu: updateMainMenu(nav); break;
        case Screen::CharSelect: updateCss(nav); break;
        case Screen::StageSelect: updateStage(nav); break;
        case Screen::Versus: updateVersus(nav); break;
        case Screen::Results: updateResults(nav); break;
        case Screen::Gallery: updateGallery(nav); break;
        case Screen::Settings: updateSettings(nav); break;
        case Screen::Poses: case Screen::Anim:
          if (nav.back) quit_ = true;
          break;
        case Screen::Match: {
          MatchScreen::Result r = match_->update();
          if (r == MatchScreen::Result::Finished) finishMatch();
          else if (r == MatchScreen::Result::Quit) goTo(Screen::CharSelect);
          else if (r == MatchScreen::Result::Options) {  // from the pause: the settings, then back to it
            settingsReturn_ = Screen::Match;
            goTo(Screen::Settings);
          }
          break;
        }
      }
    }

    art::newFrame();
    BeginTextureMode(target_);
    pix::setFrameTarget(&target_);
    rlScalef(g_renderScale, g_renderScale, 1.0f);
    ClearBackground(BLACK);
    // Standard alpha blending for colours, but the frame itself stays opaque: overlapping
    // anti-aliased edges must not punch translucent holes into the off-screen image.
    ui::frameBlend();
    switch (screen_) {
      case Screen::Loading: drawLoading(); break;
      case Screen::Title: drawTitle(); break;
      case Screen::MainMenu: drawMainMenu(); break;
      case Screen::CharSelect: drawCss(); break;
      case Screen::StageSelect: drawStage(); break;
      case Screen::Versus: drawVersus(); break;
      case Screen::Results: drawResults(); break;
      case Screen::Gallery: drawGallery(); break;
      case Screen::Settings: drawSettings(); break;
      case Screen::Poses: drawPoses(); break;
      case Screen::Anim: drawAnimSheet(); break;
      case Screen::Match: if (match_) match_->draw(); break;
    }
    if (toastTimer_ > 0) {
      float a = std::min(1.0f, toastTimer_ / 20.0f);
      float w = textWidth(toast_.c_str(), 20) + 48;
      Rectangle r{(kScreenW - w) * 0.5f, screen_ == Screen::Match ? 22.0f : kScreenH - 110.0f, w, 44};  // clear of the HUD
      ui::chunky(r, ui::Tone::Navy, a, r.height * 0.45f);
      drawTextOutlined(toast_.c_str(), r.x + 24, r.y + 9, 20, withAlpha(ui::kCream, a), withAlpha(ui::kInk, a), 2.0f);
    }
    if (settings().showFps) drawTextOutlined(TextFormat("%d FPS", GetFPS()), 10, 8, 16, {150, 255, 150, 255});
    if (transition_ >= 0) ui::wipe(wipe_, transition_ / float(ui::wipeFrames(wipe_)), &snap_.texture);
    EndBlendMode();
    EndTextureMode();
    pix::setFrameTarget(nullptr);

    auto saveFrame = [&](const std::string& path) {
      Image img = LoadImageFromTexture(target_.texture);
      ImageFlipVertical(&img);
      if (img.width != kScreenW && !opt_.fullRes) ImageResize(&img, kScreenW, kScreenH);
      ExportImage(img, path.c_str());
      UnloadImage(img);
    };
    for (const auto& [frame, path] : opt_.shots)
      if (frame == frames_) saveFrame(path);
    if (!opt_.recordDir.empty() && frames_ >= opt_.recordFrom && frames_ <= opt_.recordTo &&
        (frames_ - opt_.recordFrom) % opt_.recordEvery == 0)
      saveFrame(opt_.recordDir + TextFormat("/f_%05d.png", frames_));
    if (opt_.quitAfter > 0 && frames_ >= opt_.quitAfter) quit_ = true;
    if (opt_.pauseAt == frames_ && match_) match_->forcePause();

    // The frame to the screen: FXAA at the render's size, then stretched to fill it (FSR 1: RCAS when it grows)
    const Settings& gs = settings();
    Texture2D src = target_.texture;
    const float tw = float(src.width), th = float(src.height);
    if (gs.antiAlias == 1 && fxaa_.id != 0) {
      if (aa_.id == 0 || aa_.texture.width != src.width || aa_.texture.height != src.height) {
        if (aa_.id != 0) UnloadRenderTexture(aa_);
        aa_ = LoadRenderTexture(src.width, src.height);
        SetTextureFilter(aa_.texture, TEXTURE_FILTER_BILINEAR);
      }
      static const int texelLoc = GetShaderLocation(fxaa_, "texel");
      const float texel[2] = {1.0f / tw, 1.0f / th};
      BeginTextureMode(aa_);
      BeginShaderMode(fxaa_);
      SetShaderValue(fxaa_, texelLoc, texel, SHADER_UNIFORM_VEC2);
      DrawTexturePro(src, {0, 0, tw, -th}, {0, 0, tw, th}, {0, 0}, 0.0f, WHITE);
      EndShaderMode();
      EndTextureMode();
      src = aa_.texture;
    }
    BeginDrawing();
    ClearBackground(BLACK);
    float sw = float(GetScreenWidth()), sh = float(GetScreenHeight());
    float sc = std::min(sw / kScreenW, sh / kScreenH);
    Rectangle dst{(sw - kScreenW * sc) * 0.5f, (sh - kScreenH * sc) * 0.5f, kScreenW * sc, kScreenH * sc};
    const bool rcas = rcas_.id != 0 && gs.sharpness > 0 && dst.width > tw * 1.01f;
    if (rcas) {
      static const int stepLoc = GetShaderLocation(rcas_, "stepUv"), sharpLoc = GetShaderLocation(rcas_, "sharp");
      const float step[2] = {1.0f / dst.width, 1.0f / dst.height}, sharp = gs.sharpness / 100.0f;
      BeginShaderMode(rcas_);
      SetShaderValue(rcas_, stepLoc, step, SHADER_UNIFORM_VEC2);
      SetShaderValue(rcas_, sharpLoc, &sharp, SHADER_UNIFORM_FLOAT);
    }
    DrawTexturePro(src, {0, 0, tw, -th}, dst, {0, 0}, 0.0f, WHITE);
    if (rcas) EndShaderMode();
    EndDrawing();
    SetMouseOffset(int(-dst.x), int(-dst.y));
    SetMouseScale(1.0f / sc, 1.0f / sc);
  }
  saveSettings();
  match_.reset();
  pix::unload();
  UnloadRenderTexture(target_);
  if (snap_.id != 0) UnloadRenderTexture(snap_);
  if (aa_.id != 0) UnloadRenderTexture(aa_);
  UnloadShader(fxaa_);
  UnloadShader(rcas_);
  ui::unloadSprites();
  unloadFonts();
  audio_.shutdown();
  CloseWindow();
}
