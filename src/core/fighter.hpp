#pragma once
#include <vector>

#include "core/input.hpp"
#include "core/moves.hpp"

enum class FState : uint8_t {
  Idle, Walk, Run, Crouch, JumpSquat, Air, Landing, Attack,
  Shield, ShieldStun, Roll, SpotDodge, AirDodge,
  Hitstun, Knockdown, GetUp, Helpless,
  LedgeHang, LedgeClimb, Holding, Grabbed, ShieldBreak,
  Captured,  // held by an opponent's cinematic ultimate (see HitboxDef::capture)
  Dead, Respawn
};

constexpr float kMaxShield = 50.0f;
// Ultimate meter: fills up by dealing and taking damage; once full the ultimate button unleashes the final smash.
constexpr float kUltMax = 100.0f;
constexpr float kUltPerDamageDealt = 0.45f;
constexpr float kUltPerDamageTaken = 0.35f;
constexpr float kUltPerShieldHit = 0.15f;  // attacker, per damage blocked by a shield
// CPU levels go from 1 to 10; level 10 plays like a perfect level 9 and hits twice as hard.
constexpr int kMaxCpuLevel = 10;
constexpr float kLevel10Power = 2.0f;

struct Fighter {
  int id = 0;          // index in Match::fighters
  int port = 0;        // controller slot (0..3)
  int team = 0;        // fighters of one team cannot hurt each other (free for all: its own id)
  bool inTeam = false; // a team battle: the fighter wears its team's colour
  int charIndex = 0;
  int palette = 0;
  const CharacterDef* def = nullptr;
  bool cpu = false;
  int cpuLevel = 5;

  Vec2 pos;            // feet position
  Vec2 vel;            // self-propelled velocity
  Vec2 kbVel;          // knockback velocity (decays on its own)
  int facing = 1;

  FState state = FState::Idle;
  int stateFrame = 0;
  int timer = 0;       // generic per-state duration
  int actionDir = 0;   // roll direction, directional air dodge...

  float percent = 0.0f;
  int stocks = 3;
  bool eliminated = false;

  bool grounded = false;
  int groundPlat = -1;
  int airJumps = 0;
  bool fastFall = false;
  bool airDodgeUsed = false;
  bool sideSpecialUsed = false;
  int moveCooldown[MOVE_COUNT] = {};  // frames left before a move with a cooldown can be used again

  // Current move
  int move = -1;
  int moveFrame = 0;
  int charge = 0;
  bool charging = false;
  bool comboQueued = false;
  bool counterTriggered = false;
  bool moveLeftGround = false;
  bool moveStartedAir = false;
  std::vector<int> hitList;  // victim * 64 + group

  // Combat
  int hitlag = 0;
  int hitstun = 0;
  bool tumble = false;
  float shield = kMaxShield;
  int intangible = 0;
  int invincible = 0;  // respawn invincibility
  int techWindow = 0;  // frames left where a tech input counts
  int techLockout = 0;
  int lastHitBy = -1;
  int lastHitFrame = -10000;
  float lastKnockback = 0.0f;

  // Ledges / grabs / platforms
  int ledge = -1;
  int ledgeCooldown = 0;
  int ledgeAction = 0;  // 0 climb, 1 attack, 2 roll
  Vec2 ledgeFrom, ledgeTo;
  int dropThrough = 0;
  int grabPartner = -1;
  int escapeTimer = 0;
  int respawnTimer = 0;
  float ult = 0.0f;          // ultimate meter (0..kUltMax)
  int weapon = -1;           // weapon picked up (index in def->weapons), -1 = none
  int weaponTimer = 0;       // frames left with it
  int form = -1;             // transformation (index in def->forms), -1 = none
  int formTimer = 0;         // frames left (0 with a form = until the move that started it ends)
  int formMove = -1;         // the move that started a move-long form
  int captor = -1;           // Captured: the fighter whose ultimate holds this one
  Vec2 capturePos;           // Captured with holdInPlace: where it was caught
  bool finalSmash = false;   // ultimate ready (full meter or Clash Orb broken): the ultimate button unleashes it

  // Input
  InputState in, prev;
  Vec2 stickHist[6];
  uint8_t dirHist[30] = {};  // the stick's direction over the last frames, newest first (numpad: 5 = neutral)

  // Stats
  int kos = 0;
  int falls = 0;
  float damageDealt = 0.0f;
  int koFrame = -1;     // frame of elimination (for ranking)

  // Visual helpers (read by the renderer)
  int flash = 0;
  HitFx flashFx = HitFx::Normal;  // element of the blow that made it flash
  int airJumpFrame = -1000;  // frame of the last air jump (for the flip)
  float spin = 0.0f;

  float width() const { return def->width; }
  float height() const { return def->height; }
  Vec2 center() const { return {pos.x, pos.y - def->height * 0.5f}; }
  Rect hurtbox() const {
    float h = def->height;
    if (state == FState::Crouch) h *= 0.7f;
    if (state == FState::Knockdown) h *= 0.4f;
    return {pos.x - def->width * 0.5f, pos.y - h, def->width, h};
  }
  const WeaponDef* weaponDef() const { return weapon >= 0 ? &def->weapons[weapon & 1] : nullptr; }
  const FormDef* formDef() const {
    return form >= 0 && form < int(def->forms.size()) ? &def->forms[form] : nullptr;
  }
  Form formKind() const { return formDef() ? formDef()->form : Form::None; }
  // A move as this fighter performs it: a transformation, then the held weapon may replace it.
  const MoveDef& moveDef(int id) const {
    if (const FormDef* fd = formDef())
      for (const auto& [mid, m] : fd->moves)
        if (mid == id) return m;
    if (const WeaponDef* w = weaponDef())
      for (const auto& [mid, m] : w->moves)
        if (mid == id) return m;
    return def->moves[id];
  }
  // Blade length (grip to tip) of the weapon in hand: swing hitboxes ride it, the renderer draws it.
  float bladeLength() const {
    const WeaponDef* w = weaponDef();
    if (w && w->blade > 0.0f) return w->blade;
    return def->weaponLength * (w ? w->reachMul : 1.0f);
  }
  float damageMul() const {
    float m = 1.0f;
    if (const WeaponDef* w = weaponDef()) m *= w->damageMul;
    if (const FormDef* fd = formDef()) m *= fd->damageMul;
    if (cpu && cpuLevel >= kMaxCpuLevel) m *= kLevel10Power;
    return m;
  }
  const MoveDef* currentMove() const {
    return (state == FState::Attack && move >= 0) ? &moveDef(move) : nullptr;
  }
  bool pressed(bool InputState::*b) const { return in.*b && !(prev.*b); }
  bool cstickPressed() const { return in.cstick.len() > 0.5f && prev.cstick.len() <= 0.5f; }
  // Fast flick of the stick (used for smash attacks, dashes, fast falls, rolls).
  bool flickX() const {
    if (std::fabs(in.stick.x) < 0.8f) return false;
    for (int i = 1; i < 4; ++i)
      if (std::fabs(stickHist[i].x) < 0.3f) return true;
    return false;
  }
  bool flickY() const {
    if (std::fabs(in.stick.y) < 0.8f) return false;
    for (int i = 1; i < 4; ++i)
      if (std::fabs(stickHist[i].y) < 0.3f) return true;
    return false;
  }
  bool isIntangible() const { return intangible > 0 || invincible > 0; }
  bool active() const { return !eliminated && state != FState::Dead; }
};
