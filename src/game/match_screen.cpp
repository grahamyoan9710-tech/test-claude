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
  const float h = n > 6 ? 44.0f : 52.0f, gap = n > 6 ? 8.0f : 12.0f;  // the training menu is longer
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

// Anime impact frame: the scene becomes a stark manga panel for a few frames, black and white
// alternating, the fighters involved as flat silhouettes, jagged speed lines bursting from the hit.
void drawImpactFrame(const Match& m, const Fx::Impact& im, const Camera2D& cam, float time) {
  const bool calm = shakeScale() <= 0.0f;  // no screen shake wanted: a single dark frame, no strobe
  if (calm && im.t > 0) return;
  const bool dark = im.t % 2 == 0;
  const Color paper = dark ? Color{8, 6, 10, 255} : Color{246, 242, 232, 255};
  const Color ink = dark ? Color{250, 250, 252, 255} : Color{10, 8, 12, 255};
  DrawRectangle(0, 0, kScreenW, kScreenH, calm ? withAlpha(paper, 0.8f) : paper);
  const Vector2 sp = GetWorldToScreen2D(V(im.pos), cam);
  const Vec2 o{clampf(sp.x, 30.0f, kScreenW - 30.0f), clampf(sp.y, 30.0f, kScreenH - 30.0f)};
  const uint32_t seed = im.seed + uint32_t(im.t) * 7919u;
  for (int i = 0; i < 64; ++i) {  // jagged speed lines
    const float a = (i + vfx::hash01(seed + i * 3u) * 0.8f) * 2.0f * kPi / 64.0f;
    const Vec2 d{std::cos(a), std::sin(a)}, n{-d.y, d.x};
    const float r0 = 80.0f + vfx::hash01(seed + i * 5u) * 170.0f;
    const float w = 2.0f + vfx::hash01(seed + i * 7u) * vfx::hash01(seed + i * 11u) * 30.0f;
    const Vec2 p1 = o + d * (r0 + 140.0f) + n * ((vfx::hash01(seed + i * 13u) - 0.5f) * 26.0f);
    const Vec2 p2 = o + d * (r0 + 420.0f) - n * ((vfx::hash01(seed + i * 17u) - 0.5f) * 30.0f);
    vfx::wedge(o + d * r0, p1, 0.0f, w * 0.4f, ink, ink);
    vfx::wedge(p1, p2, w * 0.4f, w * 0.75f, ink, ink);
    vfx::wedge(p2, o + d * 1700.0f, w * 0.75f, w * 1.3f, ink, ink);
  }
  vfx::burst(o, 14, 26.0f, 170.0f, 0.42f, im.t * 0.4f, ink, ink, seed);  // the blow itself
  vfx::glow(o, 50.0f, ink, ink, 24);
  vfx::glow(o, 24.0f, paper, paper, 16);
  beginWorld(cam);
  for (const auto& f : m.fighters) {
    if (!((im.mask >> f.id) & 1u) || !f.active()) continue;
    drawCharacterSprite(*f.def, f.palette, computePose(f, m, time), f.pos, f.facing, 1.0f, 1.0f, ink, 1.0f, time);
  }
  if (im.attacker >= 0 && im.attacker < int(m.fighters.size())) {  // its projectiles and beam are in the shot
    for (const auto& p : m.projectiles)
      if (p.owner == im.attacker) vfx::glow(p.pos, p.radius * 1.15f, ink, ink);
    const Fighter& f = m.fighters[im.attacker];
    const MoveDef* mv = f.currentMove();
    if (mv && mv->beam.start >= 0 && f.moveFrame >= mv->beam.start && f.moveFrame < mv->beam.end) {
      const BeamDef& b = mv->beam;
      const Vec2 dir{std::cos(deg2rad(b.angle)) * f.facing, -std::sin(deg2rad(b.angle))};
      const Vec2 a = f.center() + Vec2{b.from.x * f.facing, b.from.y};
      vfx::wedge(a, a + dir * b.length, b.width * 0.9f, b.width, ink, ink);
    }
  }
  endWorld();
  DrawRectangleLinesEx({8, 8, kScreenW - 16.0f, kScreenH - 16.0f}, 7, ink);  // panel border
}

// Ultimate cut-in (during the final smash freeze): the screen darkens, a diagonal band slashes
// across with the fighter's portrait and the move's name, speed lines in the fighter's colours.
void drawUltCutIn(const Match& m, float time) {
  if (m.freezeTimer <= 0 || m.freezeOwner < 0 || m.freezeOwner >= int(m.fighters.size())) return;
  const Fighter& f = m.fighters[m.freezeOwner];
  const int t = 30 - m.freezeTimer;  // 0 .. 29
  const HitFx el = heroElement(f.def->hero);
  const bool black = el == HitFx::Dark;
  const Color c = black ? Color{60, 58, 70, 255} : hitFxColor(el);
  const Color g = black ? Color{235, 235, 245, 255} : hitFxGlow(el);
  const Color deep = black ? Color{6, 6, 10, 255} : shade(hitFxColor(el), 0.3f);
  const float tin = std::min(1.0f, (t + 1) / 6.0f), tout = clampf((t - 23) / 6.0f, 0.0f, 1.0f);
  const float ein = 1.0f - (1.0f - tin) * (1.0f - tin) * (1.0f - tin), eout = tout * tout;
  const float env = ein * (1.0f - eout);
  const float slide = (1.0f - ein) * -1800.0f + eout * 1800.0f;
  constexpr float kAngle = -9.0f, kBand = 240.0f;

  DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(BLACK, 0.55f * env));
  rlPushMatrix();
  rlTranslatef(kScreenW * 0.5f, kScreenH * 0.5f, 0.0f);
  rlRotatef(kAngle, 0.0f, 0.0f, 1.0f);
  vfx::additive(!black);
  for (int i = 0; i < 48; ++i) {  // speed lines racing along the band
    const float y = (vfx::hash01(i * 7 + 1) - 0.5f) * 980.0f;
    const float ph = std::fmod(time * (2.2f + 2.4f * vfx::hash01(i * 7 + 2)) + vfx::hash01(i * 7 + 3), 1.0f);
    const float x = -1200.0f + ph * 2400.0f, len = 200.0f + 380.0f * vfx::hash01(i * 7 + 4);
    const float w = 2.0f + 6.0f * vfx::hash01(i * 7 + 5);
    const Color col = i % 3 == 0 ? WHITE : i % 3 == 1 ? c : g;
    vfx::streak({x - len, y}, {x, y}, w * 0.2f, w, withAlpha(col, 0.0f), withAlpha(col, 0.75f * env));
  }
  vfx::additive(false);
  const float L = -1100.0f + slide, R = 1100.0f + slide, top = -kBand * 0.5f, bot = kBand * 0.5f;
  vfx::quad({L, top}, {R, top}, {R, bot}, {L, bot}, withAlpha(c, 0.97f), withAlpha(deep, 0.97f), withAlpha(deep, 0.97f),
            withAlpha(c, 0.97f));
  vfx::additive(true);
  for (int i = 0; i < 10; ++i) {  // light streaming inside the band
    const float y = top + 12.0f + vfx::hash01(i * 13 + 5) * (kBand - 24.0f);
    const float ph = std::fmod(time * 4.0f + vfx::hash01(i * 13 + 6), 1.0f);
    const float x = L + ph * (R - L);
    vfx::streak({x - 420.0f, y}, {x, y}, 1.0f, 5.0f, withAlpha(g, 0.0f), withAlpha(g, 0.5f));
  }
  vfx::glow({-380.0f + slide, 0.0f}, 300.0f, withAlpha(g, 0.7f), withAlpha(g, 0.0f), 32);  // light behind the portrait
  vfx::burst({-380.0f + slide, 0.0f}, 16, 40.0f, 420.0f, 0.16f, time * 1.5f, withAlpha(g, 0.45f), withAlpha(g, 0.0f));
  vfx::additive(false);
  DrawRectangleRec({L, top - 10.0f, R - L, 7.0f}, WHITE);  // hard white edges
  DrawRectangleRec({L, bot + 3.0f, R - L, 7.0f}, WHITE);
  DrawRectangleRec({L, top - 19.0f, R - L, 3.0f}, withAlpha(g, 0.9f));
  DrawRectangleRec({L, bot + 16.0f, R - L, 3.0f}, withAlpha(g, 0.9f));
  rlPopMatrix();

  // The portrait breaks out of the band
  const float rad = deg2rad(kAngle);
  const Vec2 pc = Vec2{kScreenW * 0.5f, kScreenH * 0.5f} + rotate({-380.0f + slide, 0.0f}, rad);
  drawPortrait(*f.def, f.palette, pc + Vec2{0.0f, 20.0f}, 360.0f, time);

  // The move's name, slammed in
  if (t >= 2) {
    const std::string& name = f.moveDef(FinalSmash).name;
    const float slam = 1.0f + 0.6f * std::max(0.0f, 1.0f - (t - 2) / 4.0f);
    float size = 96.0f;
    const float w0 = textWidth(name.c_str(), size, FontKind::Title);
    if (w0 > 640.0f) size *= 640.0f / w0;
    size *= slam;
    const float w = textWidth(name.c_str(), size, FontKind::Title);
    rlPushMatrix();
    rlTranslatef(kScreenW * 0.5f, kScreenH * 0.5f, 0.0f);
    rlRotatef(kAngle, 0.0f, 0.0f, 1.0f);
    const float cx = 250.0f + slide;
    drawTextOutlined(f.def->name.c_str(), cx - w * 0.5f + 6.0f, top + 14.0f, 30, withAlpha(g, env), withAlpha(BLACK, env), 3.0f,
                     FontKind::Title);
    drawText(name.c_str(), cx - w * 0.5f + 7.0f, -size * 0.42f + 8.0f, size, withAlpha(BLACK, 0.6f * env), FontKind::Title);
    drawTextOutlined(name.c_str(), cx - w * 0.5f, -size * 0.42f, size, withAlpha(WHITE, env), withAlpha(deep, env),
                     std::max(3.5f, size * 0.06f), FontKind::Title);
    rlPopMatrix();
  }
  if (t < 3) DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(WHITE, 0.55f * (3 - t) / 3.0f));  // opening flash
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
  audio_.setMusic(2, "battle");  // music_battle.mp3, else the synthesized battle theme
}

std::vector<std::string> MatchScreen::pauseItems() const {
  if (!training()) return {"REPRENDRE", "QUITTER LE COMBAT"};
  int dummy = -1;
  for (const auto& f : m_.fighters)
    if (f.cpu) dummy = f.id;
  std::string mode = dummy >= 0 ? dummyName(brains_[dummy], m_.fighters[dummy].cpuLevel) : "-";
  const Fighter& me = m_.fighters[0];
  std::string weapon = upperFr("ARME " + std::to_string((dropSlot_ & 1) + 1) + "/2 : " + me.def->weapons[dropSlot_ & 1].name);
  auto onOff = [](bool b) { return std::string(b ? "OUI" : "NON"); };
  return {"REPRENDRE",
          "RÉINITIALISER LES POSITIONS",
          "MANNEQUIN : " + mode,
          "ULTI INFINIE : " + onOff(m_.trainInfiniteUlt),
          "RESTER À 0 % : " + onOff(m_.trainZeroPercent),
          "MODE COMBAT (3 VIES) : " + onOff(m_.trainFight),
          "ZONES DE COUP : " + onOff(debug_),
          weapon,
          "QUITTER L'ENTRAÎNEMENT"};
}

MatchScreen::Result MatchScreen::updatePause() {
  MenuNav nav = readMenuNav();
  const auto items = pauseItems();
  const int n = int(items.size());
  int before = pauseSel_;
  if (nav.up) pauseSel_ = (pauseSel_ + n - 1) % n;
  if (nav.down) pauseSel_ = (pauseSel_ + 1) % n;
  bool click = false;
  int clickSide = 0;  // a click on the left (-1) or right (+1) arrow of a training setting
  for (int i = 0; i < n; ++i) {
    const Rectangle r = pauseItemRect(i, n);
    if (!ui::hovered(r)) continue;
    if (ui::mouseMoved()) pauseSel_ = i;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      pauseSel_ = i;
      click = true;
      const float mx = GetMousePosition().x;
      if (training() && i >= 2 && i <= 7) clickSide = mx < r.x + 70.0f ? -1 : mx > r.x + r.width - 70.0f ? 1 : 0;
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
  const bool left = nav.left || clickSide < 0, right = nav.right || clickSide > 0;
  const bool activate = nav.confirm || (click && clickSide == 0);
  const bool cycle = activate || left || right;
  if (training()) {
    if (pauseSel_ == 2 && cycle) {
      int d = left ? -1 : 1;
      for (auto& b : brains_) b.setDummy((b.dummy() + d + CpuBrain::kDummyCount) % CpuBrain::kDummyCount);
      audio_.play(Sfx::MenuMove);
      return Result::Running;
    }
    if (pauseSel_ == 3 && cycle) {
      m_.trainInfiniteUlt = !m_.trainInfiniteUlt;
      audio_.play(Sfx::MenuMove);
      return Result::Running;
    }
    if (pauseSel_ == 4 && cycle) {
      m_.trainZeroPercent = !m_.trainZeroPercent;
      audio_.play(Sfx::MenuMove);
      return Result::Running;
    }
    if (pauseSel_ == 5 && cycle) {  // a 3-stock fight against the dummy, starting over at each end
      m_.setTrainFight(!m_.trainFight);
      std::fill(combo_.begin(), combo_.end(), 0);
      std::fill(prevPercent_.begin(), prevPercent_.end(), 0.0f);
      audio_.play(Sfx::MenuMove);
      return Result::Running;
    }
    if (pauseSel_ == 6 && cycle) {
      debug_ = !debug_;
      audio_.play(Sfx::MenuMove);
      return Result::Running;
    }
    if (pauseSel_ == 7 && (left || right)) {  // which weapon: its arrows (ENTRÉE or a click in the middle drops it)
      dropSlot_ ^= 1;
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
  } else if (training() && pauseSel_ == 7) {  // the weapon falls in front of the player (one it has: back to bare hands)
    if ((m_.fighters[0].weapons >> (dropSlot_ & 1)) & 1) m_.loseWeapon(m_.fighters[0]);
    m_.dropWeaponFor(0, dropSlot_);
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

void MatchScreen::showUltimate(int slot) {
  ultShow_ = std::clamp(slot, -1, 1);
  if (brains_.size() > 1) brains_[1].setDummy(CpuBrain::kDummyStand);
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
  if (weaponShow_ >= 0 && m_.started() && !m_.fighters.empty()) {  // the demo's weapon kit: never lost
    Fighter& f = m_.fighters[0];
    if (f.weapon != weaponShow_ && f.active() && f.state != FState::Attack) m_.equipWeapon(f, weaponShow_);
  }
  if (ultShow_ >= -1 && !ultFired_ && m_.started() && m_.fighters.size() >= 2) {
    Fighter& f = m_.fighters[0];
    Fighter& v = m_.fighters[1];
    if (m_.frame > 260 && m_.frame % 30 == 0 && f.grounded && v.grounded) {  // after "GO!", both standing
      if (ultShow_ >= 0 && f.weapon != ultShow_) m_.equipWeapon(f, ultShow_);
      f.pos = {-170.0f, 0.0f};
      f.facing = 1;
      v.pos = {30.0f, 0.0f};
      v.percent = 60.0f;
      f.ult = kUltMax;
      f.finalSmash = true;
      inputs[f.port] = InputState{};
      inputs[f.port].ultimate = true;
      ultFired_ = true;
    } else if (f.cpu) {
      inputs[f.port] = InputState{};
    }
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
  // 0.87: the camera stands a little further back (the owner's wish), on its own and at its closest.
  float zoom = clampf(0.87f * std::min(kScreenW / w, (kScreenH - 120.0f) / h), 0.42f, 1.15f);
  Vec2 target{(minX + maxX) * 0.5f, (minY + maxY) * 0.5f + 50.0f / zoom};
  // An ultimate: the camera tightens on its user and on the fighters it is dealing with.
  int ultUser = -1;
  for (const auto& f : m_.fighters)
    if (f.active() && f.state == FState::Attack && f.move == FinalSmash) ultUser = f.id;
  if (ultUser >= 0) {
    const Fighter& u = m_.fighters[ultUser];
    auto clampIn = [&](Vec2 c) {
      return Vec2{clampf(c.x, blast.left() + 250.0f, blast.right() - 250.0f), clampf(c.y, blast.top() + 250.0f, blast.bottom() - 150.0f)};
    };
    const Vec2 uc = clampIn(u.center());
    float x0 = uc.x - 150.0f, x1 = uc.x + 150.0f, y0 = uc.y - 160.0f, y1 = uc.y + 120.0f;
    for (const auto& o : m_.fighters) {
      if (o.id == ultUser || !o.active()) continue;
      const float d = (o.center() - u.center()).len();
      const bool involved = (o.state == FState::Captured && o.captor == ultUser) ||
                            (o.state == FState::Hitstun && o.lastHitBy == ultUser && d < 760.0f) || d < 380.0f;
      if (!involved) continue;
      const Vec2 c = clampIn(o.center());
      x0 = std::min(x0, c.x - 130.0f);
      x1 = std::max(x1, c.x + 130.0f);
      y0 = std::min(y0, c.y - 150.0f);
      y1 = std::max(y1, c.y + 110.0f);
    }
    const float uz = clampf(std::min(kScreenW / (x1 - x0), (kScreenH - 140.0f) / (y1 - y0)), 0.62f, 1.4f);
    const Vec2 ut{(x0 + x1) * 0.5f, (y0 + y1) * 0.5f + 30.0f / uz};
    const float dur = float(std::max(1, u.moveDef(FinalSmash).duration)), t = float(u.moveFrame);
    const float k = smoothstep(t / 18.0f) * (1.0f - smoothstep((t - (dur - 24.0f)) / 24.0f));
    target = lerpv(target, ut, k);
    zoom = lerpf(zoom, std::max(zoom, uz), k);
  }
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
    const float speed = m_.freezeTimer > 0 ? 0.2f : ultUser >= 0 ? 0.12f : 0.08f;  // keeps up with an ultimate
    camPos_ += (target - camPos_) * speed;
    camZoom_ += (zoom - camZoom_) * (m_.freezeTimer > 0 ? 0.15f : ultUser >= 0 ? 0.08f : 0.05f);
  }
}

Camera2D MatchScreen::camera() {
  Camera2D cam{};
  cam.offset = {kScreenW * 0.5f + shake_.x, kScreenH * 0.5f + shake_.y};
  Vec2 target = camPos_;
  float zoom = camZoom_;
  if (const float k = fx_.zoomPunch(); k > 0.0f) {  // impact: the camera punches in on the blow
    Vec2 to = fx_.punchFocus() - camPos_;
    if (to.len() > 260.0f) to = to.norm() * 260.0f;
    target = camPos_ + to * (k / (1.0f + k) + 0.3f * k);
    zoom *= 1.0f + k;
  }
  cam.target = V(target);
  cam.zoom = zoom;
  return cam;
}

void MatchScreen::drawTrainingPanel() {
  int dummy = -1;
  for (const auto& f : m_.fighters)
    if (f.cpu) dummy = f.id;
  Rectangle p{18, 18, 330, 172};
  ui::card(p, 0.92f);
  ui::bold("ENTRAÎNEMENT", p.x + 16, p.y + 10, 22, ui::kGold);
  int live = dummy >= 0 ? combo_[dummy] : 0;
  int shown = live >= 2 ? live : lastCombo_;
  Color cc = live >= 2 ? ui::kGold : ui::kCream;
  ui::bold(TextFormat("%d", shown), p.x + 16, p.y + 40, 54, cc);
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
  // The options on, under the panel
  std::string on;
  if (m_.trainFight) on += "Combat 3 vies";
  if (m_.trainInfiniteUlt) on += std::string(on.empty() ? "" : "  •  ") + "Ulti infinie";
  if (m_.trainZeroPercent) on += std::string(on.empty() ? "" : "  •  ") + "0 %";
  if (!on.empty()) drawTextOutlined(on.c_str(), p.x + 4, p.y + p.height + 10, 16, ui::kGreen, ui::kInk, 2.0f);
  if (m_.trainRestart > 0) {  // the fight is over: it starts over
    const float a = clampf(m_.trainRestart / 30.0f, 0.0f, 1.0f);
    ui::boldCentered("Le combat recommence...", kScreenW * 0.5f, kScreenH * 0.5f + 70, 34, WHITE, a);
  }
}

void MatchScreen::drawPause() {
  // The menus' look: a navy veil, chunky buttons (the chosen one gold).
  DrawRectangleGradientV(0, 0, kScreenW, kScreenH, withAlpha(ui::kNavyDeep, 0.6f), withAlpha(ui::kInk, 0.85f));
  ui::vignette(0.4f);
  const auto items = pauseItems();
  const int n = int(items.size());
  Rectangle first = pauseItemRect(0, n);
  const float pw = ui::spriteWidth(ui::Sprite::Pause, 96);
  ui::drawSprite(ui::Sprite::Pause, kScreenW * 0.5f - pw * 0.5f, first.y - 128, 96);
  for (int i = 0; i < n; ++i) {
    const Rectangle r = pauseItemRect(i, n);
    const bool sel = i == pauseSel_;
    ui::button(r, items[i].c_str(), sel ? ui::Tone::Gold : ui::Tone::Navy, sel, time_, 1.0f, sel ? 25.0f : 22.0f);
    if (sel && training() && i >= 2 && i <= 7) {
      const float cy = r.y + (r.height - ui::chunkyLip(r)) * 0.5f;
      ui::arrowTri({r.x + 30, cy}, 18, -1, ui::kInk);
      ui::arrowTri({r.x + r.width - 30, cy}, 18, 1, ui::kInk);
    }
  }
  // Unplugged gamepads
  int lostCount = 0, lostPort = 0;
  for (const auto& f : m_.fighters)
    if (padLost_[f.port]) ++lostCount, lostPort = f.port;
  if (lostCount > 0) {
    Rectangle b{kScreenW * 0.5f - 340, kScreenH - 146.0f, 680, 82};
    ui::card(b, 1.0f, {150, 34, 52, 250}, {84, 14, 30, 250});
    std::string title = lostCount > 1 ? std::string("MANETTES DÉBRANCHÉES")
                                      : std::string(deviceName(devices_[lostPort])) + " DÉBRANCHÉE (JOUEUR " +
                                            std::to_string(lostPort + 1) + ")";
    ui::boldCentered(title.c_str(), b.x + b.width * 0.5f, b.y + 12, 26);
    drawTextCentered(lostCount > 1 ? "Rebranchez-les : chacune retrouve son joueur. Ou reprenez le combat sans elles."
                                   : "Rebranchez-la : elle retrouve son joueur. Ou reprenez le combat sans elle.",
                     b.x + b.width * 0.5f, b.y + 48, 17, {250, 218, 200, 255}, false);
  }
  if (training())
    ui::footer({{"HAUT/BAS", ui::Btn::DPadUD, "Choisir"}, {"GAUCHE/DROITE", ui::Btn::DPadLR, "Changer"},
                {"ENTRÉE", ui::Btn::A, "Valider"}, {"ÉCHAP", ui::Btn::Start, "Reprendre"}});
  else
    ui::footer({{"HAUT/BAS", ui::Btn::DPadUD, "Choisir"}, {"ENTRÉE", ui::Btn::A, "Valider"},
                {"ÉCHAP", ui::Btn::Start, "Reprendre"}});
}

void MatchScreen::draw() {
  Camera2D cam = camera();
  drawBackground(m_.stage(), W(cam.target), cam.zoom, time_);
  beginWorld(cam);
  if (kVfx) drawUltimateBackdrop(m_, time_);  // an ultimate's domain replaces the sky; the stage stays readable
  drawPlatforms(m_, time_);
  drawUltimateFront(m_, time_);
  drawOrb(m_, time_);
  drawItems(m_, time_);
  if (kVfx) fx_.drawBack(m_, time_);  // ki auras, pillars of light: behind the fighters
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
  drawBeams(m_, time_);
  drawFighterTags(m_);
  drawProjectiles(m_, time_);
  if (kVfx) fx_.drawWorld();
  else fx_.drawImpacts();  // the light pixel impacts stay while the other effects are paused
  if (debug_) {
    const Rect& b = m_.stage().blast;
    DrawRectangleLinesEx({b.x, b.y, b.w, b.h}, 4, RED);
    for (int l = 0; l < m_.ledgeCount(); ++l)
      if (m_.ledgeValid(l)) DrawCircleV(V(m_.ledgePoint(l)), 6, ORANGE);
  }
  endWorld();
  drawStageForeground(m_.stage(), W(cam.target), time_);
  fx_.drawScreen(cam);
  if (const Fx::Impact* im = fx_.impact(); kVfx && im) drawImpactFrame(m_, *im, cam, time_);
  drawOffscreenBubbles(m_, cam, time_);
  drawItemPrompts(m_, cam, devices_, time_);
  drawHud(m_, fx_.hudShake, time_, &devices_);
  if (!paused_) drawCountdown(m_);  // (an unplugged gamepad can pause before "GO !")
  if (training()) drawTrainingPanel();
  drawUltCutIn(m_, time_);

  if (m_.finished) {
    float k = std::min(1.0f, endTimer_ / 14.0f);
    float size = lerpf(260.0f, 170.0f, k);
    float w = ui::spriteWidth(ui::Sprite::Game, size);
    ui::drawSprite(ui::Sprite::Game, kScreenW * 0.5f - w * 0.5f, kScreenH * 0.32f - size * 0.5f, size);
  }
  if (debug_) drawTextOutlined("F1 : zones de coup", 10, kScreenH - 150, 16, WHITE);
  if (paused_) drawPause();
}
