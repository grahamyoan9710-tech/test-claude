// The fight itself: inputs -> simulation -> effects -> camera -> drawing.
#include "game/app.hpp"
#include "game/fighter_art.hpp"
#include "game/pixel.hpp"
#include "game/render.hpp"
#include "game/rtx.hpp"
#include "game/settings.hpp"
#include "game/ui.hpp"

namespace {
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

// The ultimate's cut-in (« Prestige »): the fight holds and sinks into the night; letterbox bars carry the word
// ULTIME outlined in the fighter's colour; his colour slams across, speed lines drawn in Blender 2D rushing through
// it; he lunges in, in his own pixel art x4; the move's name lands white on a brush stroke of his deep colour.
// c: frames since it began. The fight holds for kCutHold of them, then the cut-in leaves over kCutOut.
constexpr int kCutHold = 66, kCutOut = 14;

void drawUltCutIn(const Match& m, int c, float time) {
  if (c < 0 || c >= kCutHold + kCutOut || m.freezeOwner < 0 || m.freezeOwner >= int(m.fighters.size())) return;
  const Fighter& f = m.fighters[m.freezeOwner];
  const ui::Flood fl = ui::flood(f.def->hero);
  constexpr FontKind kB = FontKind::Body;
  const float s = c / 60.0f, o = ui::seg((c - kCutHold) / 60.0f, 0.0f, kCutOut / 60.0f, 0.7f, 0.0f, 0.2f, 1.0f), keep = 1.0f - o;
  DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(ui::kNight, 0.74f * ui::seg(s, 0.0f, 0.2f, 0.0f, 0.0f, 0.58f, 1.0f) * keep));
  const Vector2 sh = ui::slam(s - 0.6f);
  rlPushMatrix();
  rlTranslatef(sh.x, sh.y, 0.0f);
  // His colour, from 0.14 s; on the way out it carries on to the right
  const float in = ui::seg(s, 0.14f, 0.48f, 0.7f, 0.0f, 0.2f, 1.0f);
  if (in > 0.0f) {
    const float bx = -1420.0f * (1.0f - in) + 1420.0f * o, y = 196.0f;
    DrawRectangleRec({bx, y, 1280, 328}, fl.fill);
    ui::speedFrame(int(s * 24.0f), {bx, y, 1280, 328}, fl.deep);
    DrawRectangleRec({bx, y, 1280, 14}, fl.deep);
    DrawRectangleRec({bx, y + 314, 1280, 14}, fl.deep);
    DrawRectangleRec({bx, y - 4, 1350, 4}, ui::kSilver);
    DrawRectangleRec({bx, y + 328, 1280, 4}, ui::kSilver);
  }
  // The bars, ULTIME running along them hollow (the top one leftwards, the bottom one rightwards)
  const float by = 120.0f * (1.0f - ui::seg(s, 0.0f, 0.28f, 0.7f, 0.0f, 0.2f, 1.0f) * keep);
  const float ww = ui::bigWidth("ULTIME ", 92, ui::Face::Wide), run = std::fmod(time / 9.0f, 1.0f) * 3.0f * ww;
  for (int k = 0; k < 2; ++k) {
    const float y0 = k == 0 ? -by : 600.0f + by;
    DrawRectangleRec({0, y0, float(kScreenW), 120}, ui::kNight);
    beginClip({0, std::max(0.0f, y0), float(kScreenW), 120});
    for (int r = 0; r < 6; ++r)
      ui::bigHollow("ULTIME ", (k == 0 ? -run : run - 3.0f * ww) + r * ww, y0 + 14, 92, ui::Face::Wide, 1.5f, withAlpha(fl.fill, 0.6f), 1.0f);
    endClip();
  }
  // He lunges in from the left (his side special's strike, with the weapon in hand if he has its clip)
  if (const float u = (s - 0.32f) / 0.5f; u > 0.0f) {
    int anim = art::kSheetMove0 + SSpecial;
    if (f.weapon == 0 && art::hasSheet(f.def->hero, art::kSheetWeapon0 + SSpecial)) anim = art::kSheetWeapon0 + SSpecial;
    if (f.weapon == 1 && art::hasSheet(f.def->hero, art::kSheetWeapon1 + SSpecial)) anim = art::kSheetWeapon1 + SSpecial;
    if (!art::hasSheet(f.def->hero, anim)) anim = art::kSheetIdle;
    int n;
    float fw, fh, fps;
    if (art::clipInfo(*f.def, anim, n, fw, fh, fps)) {
      const float k = ui::seg(u, 0.0f, 0.6f), dx = u < 0.6f ? -220.0f + 234.0f * k : 14.0f - 14.0f * ui::seg(u, 0.6f, 1.0f);
      art::drawClipPx(*f.def, anim, std::min(4, n - 1), 20.0f + dx + 1420.0f * o, 690.0f - fh * 4.0f, 4.0f, false, k * keep);
    }
  }
  // Who: the player's tag and « ULTIME · NAME », on the band
  if (const float u = ui::seg(s, 0.52f, 0.88f, 0.2f, 0.8f, 0.2f, 1.0f), al = u * keep; al > 0.0f) {
    const std::string tag = f.cpu ? std::string("CPU") : std::string(TextFormat("J%d", f.port + 1));
    const std::string txt = "ULTIME · " + upperFr(fiche(*f.def).nick);
    const float tw = ui::capsWidth(tag.c_str(), 11, 0.08f) + 0.88f + 14.0f, xw = ui::capsWidth(txt.c_str(), 13, 0.34f) + 4.42f;
    const float x0 = 1218.0f - xw - 12.0f - tw, cy = 299.0f + 12.0f * (1.0f - u);
    DrawRectangleRec({x0, cy - 11, tw, 22}, withAlpha(ui::kNight, al));
    ui::caps(tag.c_str(), x0 + 7, cy, 11, 0.08f, withAlpha(f.cpu ? ui::kCpuTag : slotColor(f), al));
    ui::caps(txt.c_str(), x0 + tw + 12, cy, 13, 0.34f, withAlpha(ui::kNight, al));
  }
  // The move's name on its brush stroke
  if (s >= 0.5f) ui::swooshFrame(int((s - 0.5f) * 24.0f), {370 + 1420.0f * o, 334, 900, 160}, withAlpha(fl.deep, keep));
  if (const float u = ui::seg(s, 0.6f, 1.05f), al = u * keep; al > 0.0f) {
    const std::string nm = upperFr(f.moveDef(FinalSmash).name);
    float em = 96.0f;
    while (em > 48.0f && ui::bigWidth(nm.c_str(), em, ui::Face::Cond) > 860.0f) em -= 2.0f;
    const float w = ui::bigWidth(nm.c_str(), em, ui::Face::Cond);
    ui::big(nm.c_str(), 1224.0f - w + 60.0f * (1.0f - u) + 1420.0f * o, 324.0f + (96.0f - em) * 0.42f, em, ui::Face::Cond,
            withAlpha(WHITE, al));
  }
  if (const float u = ui::seg(s, 0.7f, 1.06f, 0.2f, 0.8f, 0.2f, 1.0f), al = u * keep; al > 0.0f) {
    const bool armed = f.weapon >= 0 && f.weapon < int(f.def->weapons.size());
    const std::string sub = (armed ? f.def->weapons[f.weapon].name : std::string("Mains nues")) + " · gagnée en brisant la Clash Orb";
    drawText(sub.c_str(), 1218.0f - textWidth(sub.c_str(), 15, kB), 420.0f + 12.0f * (1.0f - u), 15, withAlpha(ui::kNight, al), kB);
  }
  rlPopMatrix();
}
}  // namespace

namespace {
// The demo fight of the title screen: where its stage stands on the screen, and how long it lasts after the K.O.
constexpr float kAttractZoom = 1.3f, kAttractX = 720.0f, kAttractGround = 612.0f;
constexpr int kAttractEnd = 100;
}  // namespace

MatchScreen::MatchScreen(const MatchConfig& cfg, const std::array<int, 4>& devices, Audio& audio, bool debug, bool attract)
    : m_(cfg), devices_(devices), audio_(audio), fx_(&audio), debug_(debug), attract_(attract) {
  for (const auto& f : m_.fighters) {
    brains_.emplace_back(f.cpuLevel, cfg.seed + 31u * uint32_t(f.id + 1));
    if (cfg.training && f.cpu) brains_.back().setDummy(CpuBrain::kDummyStand);
  }
  prevPercent_.assign(m_.fighters.size(), 0.0f);
  combo_.assign(m_.fighters.size(), 0);
  comboStart_.assign(m_.fighters.size(), 0.0f);
  comboShow_.assign(m_.fighters.size(), 0);
  if (attract_) {  // no "3, 2, 1": the fighters stand a moment, then fight
    while (m_.countdown > 40) m_.step({});
  } else {
    audio_.setMusic(2, "battle");  // music_battle.mp3, else the synthesized battle theme
  }
  updateCamera(true);
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

void MatchScreen::trackCallouts() {
  const size_t n = m_.fighters.size();
  if (popValue_.size() != n) {
    popValue_.assign(n, 0.0f);
    popAge_.assign(n, 1000);
    seenPercent_.assign(n, 0.0f);
  }
  std::array<float, 4> gained{};  // this frame's damage per fighter (the results' biggest hit)
  for (const auto& f : m_.fighters) {  // the damage taken, summed while the hits keep coming (a combo)
    const int i = f.id;
    const float gain = f.percent - seenPercent_[i];
    if (i < 4) gained[i] = gain;
    seenPercent_[i] = f.percent;
    if (gain > 0.01f && f.active()) {
      popValue_[i] = popAge_[i] < 45 ? popValue_[i] + gain : gain;
      popAge_[i] = 0;
    } else if (popAge_[i] < 1000) {
      ++popAge_[i];
    }
  }
  for (const auto& e : m_.events) {  // the results' stats
    if (e.type == EvType::Go && goFrame_ < 0) goFrame_ = m_.frame;
    if (e.type == EvType::OrbBreak && e.a >= 0 && e.a < 4) ++orbs_[e.a];
    if (e.type == EvType::Hit && e.a >= 0 && e.a < 4 && e.b >= 0 && e.b < 4 && e.a != e.b)
      bestHit_[e.a] = std::max(bestHit_[e.a], gained[e.b]);  // e.strength is the knockback, not the damage
  }
  auto ultReadyFx = [&](int who) {  // its column of light: the owner's ParticleFX one, else the PixelLab one
    if (!fx_.moveFx(m_, who, "pfx_ult_ready", m_.fighters[who].pos)) fx_.moveFx(m_, who, "sys_ult_ready", m_.fighters[who].pos);
  };
  for (const auto& e : m_.events) {
    if (e.type != EvType::UltReady || e.a < 0 || e.a >= int(n)) continue;
    const GameEvent* orb = nullptr;  // won by breaking the Clash Orb: its spark flies to the winner first
    for (const auto& o : m_.events)
      if (o.type == EvType::OrbBreak && o.a == e.a) orb = &o;
    if (orb) {
      starFrom_ = orb->pos;
      starTo_ = e.a;
      starT_ = 0;
    } else {
      ultReadyFx(e.a);
    }
  }
  if (starTo_ >= 0 && ++starT_ >= 36) {  // there: its column of light
    if (m_.fighters[starTo_].active()) ultReadyFx(starTo_);
    starTo_ = -1;
  }
}

void MatchScreen::drawCallouts() {
  if (m_.freezeTimer > 0) return;  // not over an ultimate's cinematic
  for (const auto& f : m_.fighters) {
    if (!f.active() || f.state == FState::Grabbed) continue;
    const int i = f.id;
    if (f.finalSmash && i != starTo_) {  // the Clash Orb's spark turning over whoever holds the ultimate
      const Vec2 c = ultStarPos(m_, i);
      const float rot = time_ * 2.0f * kPi / 9.0f;
      ui::spark(V(c), 15.0f, rot, ui::kInk, 0.24f);
      ui::spark(V(c), 12.0f, rot, WHITE);
    }
    if (i < int(popAge_.size()) && popAge_[i] < 70 && !fx_.comboShown(i)) {  // "+14 %" rising over its tag (a combo has its counter)
      const float t = popAge_[i] / 70.0f, a = popAge_[i] < 50 ? 1.0f : 1.0f - (popAge_[i] - 50) / 20.0f;
      const float pop = popAge_[i] < 8 ? 1.0f + 0.15f * (1.0f - popAge_[i] / 8.0f) : 1.0f;
      const char* txt = TextFormat("+%d %%", std::max(1, int(std::round(popValue_[i]))));
      const float size = 30.0f * pop, w = textWidth(txt, size, FontKind::Title);
      const float y = tagTop(m_, i) - 22.0f - 16.0f * (1.0f - (1.0f - t) * (1.0f - t));
      drawTextOutlined(txt, f.pos.x - w * 0.5f, capsY(y, size, FontKind::Title), size, withAlpha({255, 200, 112, 255}, a),
                       withAlpha(ui::kInk, a), 2.5f, FontKind::Title);
    }
  }
  if (starTo_ >= 0) {  // flying from the broken orb to its winner, on an arc
    const Fighter& f = m_.fighters[starTo_];
    const Vec2 to = ultStarPos(m_, starTo_), mid = (starFrom_ + to) * 0.5f + Vec2{0.0f, -140.0f};
    for (int k = 2; k >= 0; --k) {  // two fading copies behind it
      const float t = smoothstep(std::max(0.0f, starT_ - k * 3.0f) / 36.0f), u = 1.0f - t;
      const Vec2 p = starFrom_ * (u * u) + mid * (2.0f * u * t) + to * (t * t);
      const float r = (k == 0 ? 18.0f : 14.0f - k * 2.0f) * (f.active() ? 1.0f : 0.0f);
      ui::spark(V(p), r, time_ * 6.0f, withAlpha(WHITE, k == 0 ? 1.0f : 0.35f));
    }
  }
}

void MatchScreen::showSkill(char dir) {
  skillShow_ = dir;
  if (brains_.size() > 1) brains_[1].setDummy(CpuBrain::kDummyStand);
}

void MatchScreen::showUltimate(int slot) {
  ultShow_ = std::clamp(slot, -1, 1);
  if (brains_.size() > 1) brains_[1].setDummy(CpuBrain::kDummyStand);
}

MatchScreen::Result MatchScreen::update(bool catchUp) {
  time_ += 1.0f / 60.0f;
  if (!attract_ && !catchUp && IsKeyPressed(KEY_F1)) debug_ = !debug_;
  // A player's gamepad got unplugged: pause until it is back (or until the players resume anyway).
  for (const auto& f : m_.fighters) {
    const int d = devices_[f.port];
    const bool lost = !f.cpu && !f.eliminated && d >= kPad1 && !deviceAvailable(d);
    if (lost && !padLost_[f.port] && !m_.finished && !paused_) {
      openPause(f.port);
      audio_.play(Sfx::MenuBack);
    }
    padLost_[f.port] = lost;
  }
  if (paused_) return updatePause();

  bool anyHuman = false;
  for (const auto& f : m_.fighters) {
    if (f.cpu) continue;
    anyHuman = true;
    if (!catchUp && m_.started() && !m_.finished && devicePausePressed(devices_[f.port])) {
      openPause(f.port);
      audio_.play(Sfx::MenuSelect);
      return Result::Running;
    }
  }
  if (!anyHuman && !attract_ && !catchUp && (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P) || padAnyPressed(kPadStart))) {
    openPause(-1);
    return Result::Running;
  }

  // The ultimate's cut-in holds the fight; GAME plays the last blow in slow motion (a step every third frame)
  const bool held = cutT_ >= 0 && cutT_ < kCutHold, slow = m_.finished && endTimer_ < 36 && endTimer_ % 3 != 0;
  if (!held && !slow) stepFight();
  cutT_ = m_.freezeTimer > 0 ? cutT_ + 1 : -1;
  updateCamera(false);
  shake_ = fx_.shakeOffset() * shakeScale() * (attract_ ? 0.5f : 1.0f);

  if (m_.finished) {  // « GAME », then the winner's brush to the results (1.45 s in)
    if (endTimer_ == 0 && !attract_) audio_.setMusic(0);
    if (++endTimer_ > (attract_ ? kAttractEnd : 87)) return Result::Finished;
  }
  return Result::Running;
}

void MatchScreen::stepFight() {
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
  if (skillShow_ && m_.started() && m_.fighters.size() >= 2) {  // the demo's special, once its cooldown is over
    Fighter& f = m_.fighters[0];
    inputs[f.port] = InputState{};
    if (m_.frame > 260 && m_.frame % 150 == 0 && f.grounded && f.state != FState::Attack) {
      f.pos = {-170.0f, 0.0f};
      f.facing = 1;
      m_.fighters[1].pos = {130.0f, 0.0f};
      inputs[f.port].special = true;
      inputs[f.port].stick = skillShow_ == 's' ? Vec2{1, 0} : skillShow_ == 'u' ? Vec2{0, -1} : skillShow_ == 'd' ? Vec2{0, 1} : Vec2{};
    }
  }
  m_.step(inputs);
  fx_.onEvents(m_);
  fx_.update(m_);
  if (!attract_) trackCallouts();
  if (training()) trackCombos();
}

void MatchScreen::updateCamera(bool snap) {
  if (attract_) {  // the stage's middle right of the title's menu, its floor on the terrace's line
    camZoom_ = kAttractZoom;
    camPos_ = {(kScreenW * 0.5f - kAttractX) / kAttractZoom, (kScreenH * 0.5f - kAttractGround) / kAttractZoom};
    return;
  }
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
  // 1.0: the camera close on the fight again (owner, 12/11: it stood too far back).
  float zoom = clampf(1.0f * std::min(kScreenW / w, (kScreenH - 120.0f) / h), 0.42f, 1.35f);
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
  if (m_.finished) {  // « GAME »: the camera pushes in x1.5 in 0.62 s, onto the winner, as in Smash
    if (endTimer_ == 0) finishZoom_ = camZoom_;
    const float u = std::min(1.0f, endTimer_ / 37.0f);
    const float k = u < 0.7f ? 0.84f * ui::seg(u, 0.0f, 0.7f, 0.12f, 0.8f, 0.2f, 1.0f)
                             : 0.84f + 0.16f * ui::seg(u, 0.7f, 1.0f, 0.12f, 0.8f, 0.2f, 1.0f);
    if (m_.winner >= 0 && m_.fighters[m_.winner].active()) target = lerpv(target, m_.fighters[m_.winner].center(), k);
    zoom = finishZoom_ * (1.0f + 0.5f * k);
  }
  if (snap) {
    camPos_ = target;
    camZoom_ = zoom;
  } else {
    const float speed = m_.freezeTimer > 0 || m_.finished ? 0.2f : ultUser >= 0 ? 0.12f : 0.08f;  // keeps up with an ultimate
    camPos_ += (target - camPos_) * speed;
    camZoom_ = m_.finished ? zoom : camZoom_ + (zoom - camZoom_) * (m_.freezeTimer > 0 ? 0.15f : ultUser >= 0 ? 0.08f : 0.05f);
  }
}

Camera2D MatchScreen::camera() {
  Camera2D cam{};
  const Vector2 go = goShake(m_), cut = cutT_ >= 0 ? ui::slam(cutT_ / 60.0f - 0.6f) : Vector2{0, 0};
  cam.offset = {kScreenW * 0.5f + shake_.x + go.x + cut.x, kScreenH * 0.5f + shake_.y + go.y + cut.y};
  Vec2 target = camPos_;
  float zoom = camZoom_;
  if (const float k = fx_.zoomPunch(); k > 0.0f && !attract_) {  // impact: the camera punches in on the blow
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
  constexpr FontKind kB = FontKind::Body, kDsp = FontKind::Display;
  int dummy = -1;
  for (const auto& f : m_.fighters)
    if (f.cpu) dummy = f.id;
  const Rectangle p{18, 18, 330, 172};
  ui::roundRect({p.x + 4, p.y + 8, p.width, p.height}, 18, withAlpha(BLACK, 0.25f), withAlpha(BLACK, 0.25f));
  ui::roundRect(p, 18, withAlpha(ui::kNavy, 0.92f), withAlpha(ui::kNavyDeep, 0.92f));
  ui::roundLines(p, 18, 1.0f, withAlpha(ui::kCream, 0.12f));
  // Header: cyan dot, label, and the options hint right-aligned
  DrawCircleV({p.x + 22, p.y + 24}, 4, ui::kBlue);
  ui::label("ENTRAÎNEMENT", p.x + 34, p.y + 24, 11, 0.16f, ui::kSoft);
  const bool pad = padPrompts();
  const char* opt = pad ? "options" : "ÉCHAP : options";
  const float gw = pad ? ui::padGlyphWidth(ui::Btn::Start, 18, promptFamily()) + 4 : 0.0f;
  float ox = p.x + p.width - 18 - textWidth(opt, 11, kB) - gw;
  if (pad) ox += ui::padGlyph(ui::Btn::Start, ox, p.y + 15, 18, promptFamily()) + 4;
  drawText(opt, ox, capsY(p.y + 24, 11, kB), 11, ui::kMuted, kB);
  DrawRectangleRec({p.x + 18, p.y + 42, p.width - 36, 1}, withAlpha(ui::kCream, 0.08f));
  // The live combo, big
  const int live = dummy >= 0 ? combo_[dummy] : 0, shown = live >= 2 ? live : lastCombo_;
  const char* num = TextFormat("%d", shown);
  drawText(num, p.x + 18, capsY(p.y + 76, 40, kDsp), 40, live >= 2 ? ui::kBlue : ui::kCream, kDsp);
  ui::label("COUPS", p.x + 26 + textWidth(num, 40, kDsp), p.y + 84, 11, 0.16f, ui::kMuted);
  // Stats rows: name left, value right
  const float dmg = live >= 2 && dummy >= 0 ? m_.fighters[dummy].percent - comboStart_[dummy] : lastComboDamage_;
  auto row = [&](float cy, const char* k, const char* v) {
    ui::label(k, p.x + 18, cy, 10, 0.14f, ui::kMuted);
    drawText(v, p.x + p.width - 18 - textWidth(v, 13, kB), capsY(cy, 13, kB), 13, ui::kCream, kB);
  };
  row(p.y + 112, "DÉGÂTS DU COMBO", TextFormat("%d %%", int(dmg)));
  row(p.y + 134, "MEILLEUR COMBO", TextFormat("%d", bestCombo_));
  if (dummy >= 0) row(p.y + 156, "MANNEQUIN", dummyName(brains_[dummy], m_.fighters[dummy].cpuLevel));
  // The options on: cyan chips under the panel
  float cx = p.x;
  for (const auto& [on, name] : {std::pair{m_.trainFight, "Combat 3 vies"}, std::pair{m_.trainInfiniteUlt, "Ulti infinie"},
                                 std::pair{m_.trainZeroPercent, "0 %"}}) {
    if (!on) continue;
    const float w = textWidth(name, 12, kB) + 24;
    ui::roundRect({cx, p.y + p.height + 10, w, 26}, 13, ui::kBlue, ui::kBlue);
    drawText(name, cx + 12, capsY(p.y + p.height + 23, 12, kB), 12, ui::kInk, kB);
    cx += w + 8;
  }
  if (m_.trainRestart > 0) {  // the fight is over: it starts over
    const float a = clampf(m_.trainRestart / 30.0f, 0.0f, 1.0f);
    const char* msg = "LE COMBAT RECOMMENCE";
    const float w = textWidth(msg, 26, kDsp);
    drawText(msg, (kScreenW - w) * 0.5f, capsY(kScreenH * 0.5f + 70, 26, kDsp), 26, withAlpha(ui::kCream, a), kDsp);
  }
}

void MatchScreen::drawAttract() {
  float w, h;
  const bool bg = art::pictureSize("menu_horizon", w, h);
  if (bg) art::drawPicture("menu_horizon", {0, 0, w, h}, {0, 0, kScreenW, kScreenH});
  beginWorld(camera());
  drawItems(m_, time_);
  for (int pass = 0; pass < 2; ++pass)
    for (const auto& f : m_.fighters) {
      const bool late = f.state == FState::Hitstun || f.state == FState::Grabbed;
      if ((pass == 1) == late) drawFighter(f, m_, time_, false);
    }
  drawBeams(m_, time_);
  drawProjectiles(m_, time_);
  if (kVfx) fx_.drawWorld();
  else fx_.drawImpacts();
  endWorld();
  // Fading in and out: the empty terrace drawn over them.
  const float fade = std::max(1.0f - time_ * 2.0f, clampf((endTimer_ - (kAttractEnd - 40)) / 40.0f, 0.0f, 1.0f));
  if (bg && fade > 0.0f) art::drawPicture("menu_horizon", {0, 0, w, h}, {0, 0, kScreenW, kScreenH}, fade);
}

void MatchScreen::draw() {
  drawWorld();
  drawOverlay();
}

void MatchScreen::drawWorld() {
  ++shown_;
  Camera2D cam = camera();
  drawBackground(m_.stage(), W(cam.target), cam.zoom, time_);
  beginWorld(cam);
  rtx::beginFrame();
  light::backdrop(m_, cam);
  if (kVfx) drawUltimateBackdrop(m_, time_);  // an ultimate's domain replaces the sky; the stage stays readable
  drawPlatforms(m_, time_);
  rtx::platforms(m_);
  light::arena(m_, cam, time_);  // the effects' light on the platforms, the shadows
  drawUltimateFront(m_, time_);
  drawOrb(m_, time_);
  drawItems(m_, time_);
  fx_.drawBack(m_, time_);  // ki auras, pillars of light, the techniques' effects: behind the fighters
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
  drawProjectiles(m_, time_);
  if (kVfx) fx_.drawWorld();
  else fx_.drawImpacts();  // the movement smoke and the pixel impacts stay while the other effects are off
  rtx::resolve(m_, cam);               // RTX: the traced light, before the floors mirror it
  light::reflections(m_, cam, time_);  // the floors mirror the scene (not the name tags)
  if (debug_) {
    const Rect& b = m_.stage().blast;
    DrawRectangleLinesEx({b.x, b.y, b.w, b.h}, 4, RED);
    for (const auto& p : m_.platforms) DrawRectangleLinesEx({p.rect.x, p.rect.y, p.rect.w, p.rect.h}, 2, SKYBLUE);
    for (int l = 0; l < m_.ledgeCount(); ++l)
      if (m_.ledgeValid(l)) DrawCircleV(V(m_.ledgePoint(l)), 6, ORANGE);
  }
  endWorld();
  drawStageForeground(m_.stage(), W(cam.target), time_);
  fx_.drawScreen(cam);
  if (const Fx::Impact* im = fx_.impact(); kVfx && im) drawImpactFrame(m_, *im, cam, time_);
}

// The name tags, the HUD, the menus: drawn at the screen's own resolution, whatever the fight's render resolution.
void MatchScreen::drawOverlay() {
  Camera2D cam = camera();
  beginWorld(cam);
  drawFighterTags(m_);
  drawCallouts();  // over the effects: the orb's spark flies out of its own burst
  endWorld();
  drawOffscreenBubbles(m_, cam, time_);
  drawItemPrompts(m_, cam, devices_, time_);
  drawHud(m_, fx_.hudHit, time_, &devices_);
  if (!paused_) drawCountdown(m_, shown_ / 60.0f);  // (an unplugged gamepad can pause before "GO !")
  if (training()) drawTrainingPanel();
  drawUltCutIn(m_, cutT_, time_);

  if (m_.finished && !attract_) {  // « GAME » (2.33 s, cut at 1.45 s by the brush): letterbox bars, the fight sinks into
    // the night, GAME torn red / blue over a silver line
    const float u = endTimer_ / 60.0f / 2.33f, bar = 76.0f * ui::seg(u, 0.0f, 0.1f);
    DrawRectangle(0, 0, kScreenW, kScreenH, withAlpha(ui::kNight, 0.82f * clampf((u - 0.25f) / 0.02f, 0.0f, 1.0f)));
    DrawRectangleRec({0, 0, float(kScreenW), bar}, ui::kNight);
    DrawRectangleRec({0, kScreenH - bar, float(kScreenW), bar}, ui::kNight);
    if (u >= 0.26f) {
      const float sc = u < 0.32f ? 1.7f - 0.73f * ui::seg(u, 0.26f, 0.32f) : u < 0.37f ? 0.97f + 0.04f * ui::seg(u, 0.32f, 0.37f)
                       : 1.01f - 0.01f * ui::seg(u, 0.37f, 0.46f);
      const float split = u < 0.32f ? 44.0f - 28.0f * ui::seg(u, 0.26f, 0.32f) : 16.0f - 6.0f * ui::seg(u, 0.32f, 0.46f);
      ui::scaled(sc, 640, 350, [&] { ui::choc("GAME", 640, 250, 200, split, ui::seg(u, 0.26f, 0.32f), 5.0f, ui::Face::Wide, 1.0f, 0.04f); });
      const float l = 700.0f * ui::seg(u, 0.32f, 0.46f);
      DrawRectangleRec({640 - l * 0.5f, 470, l, 4}, ui::kSilver);
    }
  }
  if (debug_) drawTextOutlined("F1 : zones de coup", 10, kScreenH - 150, 16, WHITE);
  if (paused_) drawPause();
}
