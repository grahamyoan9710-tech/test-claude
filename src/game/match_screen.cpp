// The fight itself: inputs -> simulation -> effects -> camera -> drawing.
#include "game/app.hpp"
#include "game/render.hpp"

MatchScreen::MatchScreen(const MatchConfig& cfg, const std::array<int, 4>& devices, Audio& audio, bool debug)
    : m_(cfg), devices_(devices), audio_(audio), fx_(&audio), debug_(debug) {
  for (const auto& f : m_.fighters) brains_.emplace_back(f.cpuLevel, cfg.seed + 31u * uint32_t(f.id + 1));
  updateCamera(true);
  audio_.setMusic(2);
}

MatchScreen::Result MatchScreen::update() {
  time_ += 1.0f / 60.0f;
  if (IsKeyPressed(KEY_F1)) debug_ = !debug_;

  if (paused_) {
    MenuNav nav = readMenuNav();
    if (nav.up || nav.down) {
      pauseSel_ = 1 - pauseSel_;
      audio_.play(Sfx::MenuMove);
    }
    bool resume = false;
    for (const auto& f : m_.fighters)
      if (!f.cpu && devicePausePressed(devices_[f.port])) resume = true;
    if (nav.confirm || resume) {
      audio_.play(Sfx::MenuSelect);
      if (pauseSel_ == 1 && !resume) return Result::Quit;
      paused_ = false;
    }
    return Result::Running;
  }

  bool anyHuman = false;
  for (const auto& f : m_.fighters) {
    if (f.cpu) continue;
    anyHuman = true;
    if (m_.started() && !m_.finished && devicePausePressed(devices_[f.port])) {
      paused_ = true;
      pauseSel_ = 0;
      audio_.play(Sfx::MenuSelect);
      return Result::Running;
    }
  }
  if (!anyHuman && (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P))) {
    paused_ = true;
    pauseSel_ = 0;
    return Result::Running;
  }

  std::array<InputState, 4> inputs{};
  for (const auto& f : m_.fighters)
    inputs[f.port] = f.cpu ? brains_[f.id].think(m_, f.id) : readDevice(devices_[f.port]);
  m_.step(inputs);
  fx_.onEvents(m_);
  fx_.update(m_);
  updateCamera(false);
  shake_ = fx_.shakeOffset();

  if (m_.finished) {
    if (endTimer_ == 0) audio_.setMusic(0);
    if (++endTimer_ > 170) return Result::Finished;
  }
  return Result::Running;
}

void MatchScreen::updateCamera(bool snap) {
  const Rect& blast = m_.stage().blast;
  float minX = -260, maxX = 260, minY = -260, maxY = 60;
  for (const auto& f : m_.fighters) {
    if (!f.active()) continue;
    Vec2 c = f.center();
    c.x = clampf(c.x, blast.left() + 250.0f, blast.right() - 250.0f);
    c.y = clampf(c.y, blast.top() + 250.0f, blast.bottom() - 150.0f);
    minX = std::min(minX, c.x - 160.0f);
    maxX = std::max(maxX, c.x + 160.0f);
    minY = std::min(minY, c.y - 170.0f);
    maxY = std::max(maxY, c.y + 130.0f);
  }
  float w = maxX - minX, h = maxY - minY;
  float zoom = clampf(std::min(kScreenW / w, (kScreenH - 120.0f) / h), 0.42f, 1.15f);
  Vec2 target{(minX + maxX) * 0.5f, (minY + maxY) * 0.5f + 50.0f / zoom};
  if (m_.finished && m_.winner >= 0) {  // zoom on the winner
    const Fighter& w2 = m_.fighters[m_.winner];
    if (w2.active()) {
      target = lerpv(target, w2.center(), std::min(1.0f, endTimer_ / 60.0f));
      zoom = lerpf(zoom, 1.3f, std::min(1.0f, endTimer_ / 60.0f));
    }
  }
  if (snap) {
    camPos_ = target;
    camZoom_ = zoom;
  } else {
    camPos_ += (target - camPos_) * 0.08f;
    camZoom_ += (zoom - camZoom_) * 0.05f;
  }
}

Camera2D MatchScreen::camera() {
  Camera2D cam{};
  cam.offset = {kScreenW * 0.5f + shake_.x, kScreenH * 0.5f + shake_.y};
  cam.target = V(camPos_);
  cam.zoom = camZoom_;
  return cam;
}

void MatchScreen::draw() {
  Camera2D cam = camera();
  drawBackground(m_.stage(), camPos_, camZoom_, time_);
  BeginMode2D(cam);
  drawPlatforms(m_, time_);
  // Grabbed fighters and victims are drawn last so they stay visible.
  for (int pass = 0; pass < 2; ++pass)
    for (const auto& f : m_.fighters) {
      bool late = f.state == FState::Hitstun || f.state == FState::Grabbed;
      if ((pass == 1) == late) drawFighter(f, m_, time_, debug_);
    }
  drawProjectiles(m_, time_);
  fx_.drawWorld();
  if (debug_) {
    const Rect& b = m_.stage().blast;
    DrawRectangleLinesEx({b.x, b.y, b.w, b.h}, 4, RED);
    for (int l = 0; l < m_.ledgeCount(); ++l)
      if (m_.ledgeValid(l)) DrawCircleV(V(m_.ledgePoint(l)), 6, ORANGE);
  }
  EndMode2D();
  fx_.drawScreen(cam);
  drawOffscreenBubbles(m_, cam, time_);
  drawHud(m_, fx_.hudShake, time_);
  drawCountdown(m_);

  if (m_.finished) {
    float k = std::min(1.0f, endTimer_ / 20.0f);
    float size = 190.0f - 60.0f * k;
    drawTextCentered("GAME !", kScreenW * 0.5f, kScreenH * 0.3f - size * 0.5f, size,
                     withAlpha({255, 230, 90, 255}, k));
  }
  if (debug_) {
    DrawFPS(10, 10);
    DrawText("F1: hitboxes", 10, 32, 16, WHITE);
  }
  if (paused_) {
    DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(BLACK, 0.55f));
    drawTextCentered("PAUSE", kScreenW * 0.5f, 200, 80, WHITE);
    const char* items[2] = {"REPRENDRE", "QUITTER LE COMBAT"};
    for (int i = 0; i < 2; ++i) {
      Color c = i == pauseSel_ ? Color{255, 220, 80, 255} : Color{200, 200, 210, 255};
      drawTextCentered(TextFormat(i == pauseSel_ ? "> %s <" : "%s", items[i]), kScreenW * 0.5f, 330.0f + i * 60.0f, 36, c);
    }
  }
}
