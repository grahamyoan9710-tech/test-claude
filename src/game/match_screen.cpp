// The fight itself: inputs -> simulation -> effects -> camera -> drawing.
#include "game/app.hpp"
#include "game/render.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
const char* dummyName(const CpuBrain& b, int level) {
  switch (b.dummy()) {
    case CpuBrain::kDummyStand: return "IMMOBILE";
    case CpuBrain::kDummyJump: return "SAUTE";
    case CpuBrain::kDummyShield: return "BOUCLIER";
    default: return TextFormat("ORDINATEUR (niv. %d)", level);
  }
}
Rectangle pauseItemRect(int i, int n) {
  float h = 52, gap = 12;
  float top = kScreenH * 0.5f - (n * (h + gap)) * 0.5f + 40;
  return {kScreenW * 0.5f - 260, top + i * (h + gap), 520, h};
}
constexpr bool InputState::*kButtons[] = {&InputState::attack, &InputState::special, &InputState::jump,
                                          &InputState::shield, &InputState::grab,    &InputState::smash,
                                          &InputState::ultimate};
// Buttons pressed in the pause menu stay ignored until released (A on "REPRENDRE" must not attack).
void maskHeld(InputState& in, InputState& held) {
  for (bool InputState::*b : kButtons) {
    if (!(in.*b)) held.*b = false;
    else if (held.*b) in.*b = false;
  }
}
}  // namespace

MatchScreen::MatchScreen(const MatchConfig& cfg, const std::array<int, 4>& devices, Audio& audio, bool debug)
    : m_(cfg), devices_(devices), audio_(audio), fx_(&audio), debug_(debug) {
  for (const auto& f : m_.fighters) {
    brains_.emplace_back(f.cpuLevel, cfg.seed + 31u * uint32_t(f.id + 1));
    if (cfg.training && f.cpu) brains_.back().setDummy(CpuBrain::kDummyStand);
  }
  prevPercent_.assign(m_.fighters.size(), 0.0f);
  combo_.assign(m_.fighters.size(), 0);
  comboStart_.assign(m_.fighters.size(), 0.0f);
  comboShow_.assign(m_.fighters.size(), 0);
  updateCamera(true);
  audio_.setMusic(2);
}

std::vector<std::string> MatchScreen::pauseItems() const {
  if (!training()) return {"REPRENDRE", "QUITTER LE COMBAT"};
  int dummy = -1;
  for (const auto& f : m_.fighters)
    if (f.cpu) dummy = f.id;
  std::string mode = dummy >= 0 ? dummyName(brains_[dummy], m_.fighters[dummy].cpuLevel) : "-";
  return {"REPRENDRE", "RÉINITIALISER LES POSITIONS", "MANNEQUIN : " + mode,
          std::string("ZONES DE COUP : ") + (debug_ ? "OUI" : "NON"), "QUITTER L'ENTRAÎNEMENT"};
}

MatchScreen::Result MatchScreen::updatePause() {
  MenuNav nav = readMenuNav();
  const auto items = pauseItems();
  const int n = int(items.size());
  int before = pauseSel_;
  if (nav.up) pauseSel_ = (pauseSel_ + n - 1) % n;
  if (nav.down) pauseSel_ = (pauseSel_ + 1) % n;
  bool click = false;
  for (int i = 0; i < n; ++i) {
    if (!ui::hovered(pauseItemRect(i, n))) continue;
    if (ui::mouseMoved()) pauseSel_ = i;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      pauseSel_ = i;
      click = true;
    }
  }
  if (pauseSel_ != before) audio_.play(Sfx::MenuMove);
  bool unpause = padAnyPressed(kPadB);
  for (const auto& f : m_.fighters)
    if (!f.cpu && devicePausePressed(devices_[f.port])) unpause = true;
  if (unpause) {
    resume();
    audio_.play(Sfx::MenuBack);
    return Result::Running;
  }
  const bool activate = nav.confirm || click;
  const bool cycle = activate || nav.left || nav.right;
  if (training()) {
    if (pauseSel_ == 2 && cycle) {
      int d = nav.left ? -1 : 1;
      for (auto& b : brains_) b.setDummy((b.dummy() + d + CpuBrain::kDummyCount) % CpuBrain::kDummyCount);
      audio_.play(Sfx::MenuMove);
      return Result::Running;
    }
    if (pauseSel_ == 3 && cycle) {
      debug_ = !debug_;
      audio_.play(Sfx::MenuMove);
      return Result::Running;
    }
  }
  if (!activate) return Result::Running;
  audio_.play(Sfx::MenuSelect);
  if (pauseSel_ == 0) {
    resume();
  } else if (pauseSel_ == n - 1) {
    return Result::Quit;
  } else if (training() && pauseSel_ == 1) {
    m_.resetPositions();
    std::fill(combo_.begin(), combo_.end(), 0);
    std::fill(prevPercent_.begin(), prevPercent_.end(), 0.0f);
    resume();
  }
  return Result::Running;
}

void MatchScreen::resume() {
  paused_ = false;
  for (const auto& f : m_.fighters) {
    if (f.cpu) continue;
    const InputState now = readDevice(devices_[f.port]);
    for (bool InputState::*b : kButtons) held_[f.port].*b = now.*b && !(f.in.*b);  // held since before: goes on
  }
}

void MatchScreen::trackCombos() {
  for (const auto& f : m_.fighters) {
    const int i = f.id;
    bool stunned = f.state == FState::Hitstun || f.hitlag > 0 || f.state == FState::Grabbed;
    if (f.percent > prevPercent_[i] + 0.01f) {
      if (combo_[i] > 0) combo_[i]++;
      else {
        combo_[i] = 1;
        comboStart_[i] = prevPercent_[i];
      }
      comboShow_[i] = 120;
    } else if (!stunned && combo_[i] > 0) {
      if (combo_[i] >= 2) {
        lastCombo_ = combo_[i];
        lastComboDamage_ = f.percent - comboStart_[i];
        bestCombo_ = std::max(bestCombo_, combo_[i]);
      }
      combo_[i] = 0;
    }
    if (comboShow_[i] > 0) --comboShow_[i];
    prevPercent_[i] = f.percent;
  }
}

MatchScreen::Result MatchScreen::update() {
  time_ += 1.0f / 60.0f;
  if (IsKeyPressed(KEY_F1)) debug_ = !debug_;
  // A player's gamepad got unplugged: pause until it is back (or until the players resume anyway).
  for (const auto& f : m_.fighters) {
    const int d = devices_[f.port];
    const bool lost = !f.cpu && !f.eliminated && d >= kPad1 && !deviceAvailable(d);
    if (lost && !padLost_[f.port] && !m_.finished && !paused_) {
      paused_ = true;
      pauseSel_ = 0;
      audio_.play(Sfx::MenuBack);
    }
    padLost_[f.port] = lost;
  }
  if (paused_) return updatePause();

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
  if (!anyHuman && (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P) || padAnyPressed(kPadStart))) {
    paused_ = true;
    pauseSel_ = 0;
    return Result::Running;
  }

  std::array<InputState, 4> inputs{};
  for (const auto& f : m_.fighters) {
    if (f.cpu) {
      inputs[f.port] = brains_[f.id].think(m_, f.id);
      continue;
    }
    inputs[f.port] = readDevice(devices_[f.port]);
    maskHeld(inputs[f.port], held_[f.port]);
  }
  m_.step(inputs);
  fx_.onEvents(m_);
  fx_.update(m_);
  if (training()) trackCombos();
  updateCamera(false);
  shake_ = fx_.shakeOffset() * shakeScale();

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
  if (m_.freezeTimer > 0 && m_.freezeOwner >= 0) {  // final smash close-up
    target = m_.fighters[m_.freezeOwner].center();
    zoom = 1.45f;
  }
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
    float speed = m_.freezeTimer > 0 ? 0.2f : 0.08f;
    camPos_ += (target - camPos_) * speed;
    camZoom_ += (zoom - camZoom_) * (m_.freezeTimer > 0 ? 0.15f : 0.05f);
  }
}

Camera2D MatchScreen::camera() {
  Camera2D cam{};
  cam.offset = {kScreenW * 0.5f + shake_.x, kScreenH * 0.5f + shake_.y};
  cam.target = V(camPos_);
  cam.zoom = camZoom_;
  return cam;
}

void MatchScreen::drawTrainingPanel() {
  int dummy = -1;
  for (const auto& f : m_.fighters)
    if (f.cpu) dummy = f.id;
  Rectangle p{18, 18, 330, 172};
  ui::panel(p, withAlpha(ui::kInk, 0.8f), withAlpha(ui::kGold, 0.75f), 0.12f);
  ui::rune({p.x + p.width - 30, p.y + 24}, 12, 3, time_ * 0.5f, withAlpha(ui::kTeal, 0.7f), 2.0f);
  drawText("ENTRAÎNEMENT", p.x + 16, p.y + 10, 22, {156, 206, 128, 255}, FontKind::Title);
  int live = dummy >= 0 ? combo_[dummy] : 0;
  int shown = live >= 2 ? live : lastCombo_;
  Color cc = live >= 2 ? ui::kGold : ui::kCream;
  drawTextOutlined(TextFormat("%d", shown), p.x + 16, p.y + 40, 54, cc, ui::kInk, 3.0f, FontKind::Title);
  drawText("COUPS", p.x + 22 + textWidth(TextFormat("%d", shown), 54, FontKind::Title), p.y + 52, 18, ui::kMuted);
  float dmg = live >= 2 && dummy >= 0 ? m_.fighters[dummy].percent - comboStart_[dummy] : lastComboDamage_;
  drawText(TextFormat("Dégâts du combo : %d %%", int(dmg)), p.x + 16, p.y + 98, 16, ui::kCream);
  drawText(TextFormat("Meilleur combo : %d", bestCombo_), p.x + 16, p.y + 120, 14, ui::kMuted);
  const bool pad = padPrompts();  // options hint, right-aligned
  const char* opt = pad ? ": options" : "ÉCHAP : options";
  const float gw = pad ? ui::padGlyphWidth(ui::Btn::Start, 20, promptFamily()) + 4 : 0.0f;
  float ox = p.x + p.width - 16 - textWidth(opt, 14) - gw;
  if (pad) ox += ui::padGlyph(ui::Btn::Start, ox, p.y + 117, 20, promptFamily()) + 4;
  drawText(opt, ox, p.y + 120, 14, ui::kMuted);
  if (dummy >= 0) {
    std::string mode = std::string("Mannequin : ") + dummyName(brains_[dummy], m_.fighters[dummy].cpuLevel);
    drawText(mode.c_str(), p.x + 16, p.y + 142, 15, withAlpha(ui::kCream, 0.85f));
  }
}

void MatchScreen::drawPause() {
  DrawRectangleGradientV(0, 0, kScreenW, kScreenH, withAlpha(ui::kInk, 0.62f), withAlpha(ui::kInk, 0.78f));
  ui::vignette(0.4f);
  const auto items = pauseItems();
  const int n = int(items.size());
  Rectangle first = pauseItemRect(0, n);
  const float pw = ui::spriteWidth(ui::Sprite::Pause, 96);
  ui::zonaiRing({kScreenW * 0.5f, first.y - 70}, 120, time_, 0.55f);
  ui::drawSprite(ui::Sprite::Pause, kScreenW * 0.5f - pw * 0.5f, first.y - 128, 96);
  ui::rule(kScreenW * 0.5f - 230, kScreenW * 0.5f - pw * 0.5f - 24, first.y - 62, ui::kGold);
  ui::rule(kScreenW * 0.5f + pw * 0.5f + 24, kScreenW * 0.5f + 230, first.y - 62, ui::kGold);
  const float pulse = 0.5f + 0.5f * std::sin(time_ * 6.0f);
  for (int i = 0; i < n; ++i) {
    Rectangle r = pauseItemRect(i, n);
    bool sel = i == pauseSel_;
    if (sel) r = ui::grow(r, 4);
    ui::slanted({r.x + 6, r.y + 7, r.width, r.height}, 18, withAlpha(BLACK, 0.4f), withAlpha(BLACK, 0.4f));
    ui::slanted(r, 18, sel ? Color{44, 96, 90, 245} : Color{32, 46, 50, 240}, sel ? Color{20, 50, 52, 245} : Color{18, 28, 32, 240});
    ui::slantedOutline(r, 18, sel ? 2.5f : 1.5f, sel ? ui::kGold : withAlpha(ui::kStone, 0.5f));
    if (sel) {
      ui::slantedOutline(ui::grow(r, 5), 18, 4, withAlpha(ui::kTeal, 0.18f + 0.16f * pulse));
      ui::corners(ui::grow(r, 6 + pulse * 2), 18, 14, 3.0f, ui::kGold);
    }
    drawTextCentered(items[i].c_str(), r.x + r.width * 0.5f, r.y + r.height * 0.5f - 12, 24,
                     sel ? ui::kCream : ui::kMuted, true);
    if (sel && training() && (i == 2 || i == 3)) {
      ui::arrowTri({r.x + 30, r.y + r.height * 0.5f}, 16, -1, ui::kGold);
      ui::arrowTri({r.x + r.width - 30, r.y + r.height * 0.5f}, 16, 1, ui::kGold);
    }
  }
  // Unplugged gamepads
  int lostCount = 0, lostPort = 0;
  for (const auto& f : m_.fighters)
    if (padLost_[f.port]) ++lostCount, lostPort = f.port;
  if (lostCount > 0) {
    Rectangle b{kScreenW * 0.5f - 340, kScreenH - 146.0f, 680, 82};
    ui::panel(b, {58, 24, 26, 245}, {226, 118, 92, 255}, 0.25f);
    std::string title = lostCount > 1 ? std::string("MANETTES DÉBRANCHÉES")
                                      : std::string(deviceName(devices_[lostPort])) + " DÉBRANCHÉE (JOUEUR " +
                                            std::to_string(lostPort + 1) + ")";
    drawTextCentered(title.c_str(), b.x + b.width * 0.5f, b.y + 12, 26, ui::kCream, true, FontKind::Title);
    drawTextCentered(lostCount > 1 ? "Rebranchez-les : chacune retrouve son joueur. Ou reprenez le combat sans elles."
                                   : "Rebranchez-la : elle retrouve son joueur. Ou reprenez le combat sans elle.",
                     b.x + b.width * 0.5f, b.y + 48, 17, {250, 218, 200, 255}, false);
  }
  ui::footer({{"HAUT/BAS", ui::Btn::DPadUD, "Choisir"}, {"ENTRÉE", ui::Btn::A, "Valider"},
              {"ÉCHAP", ui::Btn::Start, "Reprendre"}});
}

void MatchScreen::draw() {
  Camera2D cam = camera();
  drawBackground(m_.stage(), camPos_, camZoom_, time_);
  beginWorld(cam);
  drawPlatforms(m_, time_);
  drawOrb(m_, time_);
  // Grabbed fighters and victims are drawn last so they stay visible;
  // during a final smash cinematic the scene darkens behind its user.
  const int star = m_.freezeTimer > 0 ? m_.freezeOwner : -1;
  for (int pass = 0; pass < 2; ++pass)
    for (const auto& f : m_.fighters) {
      bool late = f.state == FState::Hitstun || f.state == FState::Grabbed;
      if ((pass == 1) == late && f.id != star) drawFighter(f, m_, time_, debug_);
    }
  if (star >= 0) {
    float k = std::min(1.0f, (30 - m_.freezeTimer) / 8.0f) * std::min(1.0f, m_.freezeTimer / 6.0f);
    DrawRectangle(-5000, -5000, 10000, 10000, withAlpha(BLACK, 0.6f * k));
    drawFighter(m_.fighters[star], m_, time_, debug_);
  }
  drawProjectiles(m_, time_);
  fx_.drawWorld();
  if (debug_) {
    const Rect& b = m_.stage().blast;
    DrawRectangleLinesEx({b.x, b.y, b.w, b.h}, 4, RED);
    for (int l = 0; l < m_.ledgeCount(); ++l)
      if (m_.ledgeValid(l)) DrawCircleV(V(m_.ledgePoint(l)), 6, ORANGE);
  }
  endWorld();
  fx_.drawScreen(cam);
  drawOffscreenBubbles(m_, cam, time_);
  drawHud(m_, fx_.hudShake, time_, &devices_);
  if (!paused_) drawCountdown(m_);  // (an unplugged gamepad can pause before "GO !")
  if (training()) drawTrainingPanel();

  if (m_.finished) {
    float k = std::min(1.0f, endTimer_ / 14.0f);
    float size = lerpf(260.0f, 170.0f, k);
    float w = ui::spriteWidth(ui::Sprite::Game, size);
    ui::drawSprite(ui::Sprite::Game, kScreenW * 0.5f - w * 0.5f, kScreenH * 0.32f - size * 0.5f, size);
  }
  if (debug_) drawTextOutlined("F1 : zones de coup", 10, kScreenH - 150, 16, WHITE);
  if (paused_) drawPause();
}
