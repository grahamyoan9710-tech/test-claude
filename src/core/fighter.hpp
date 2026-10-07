#pragma once
#include <vector>

#include "core/input.hpp"
#include "core/moves.hpp"

enum class FState : uint8_t {
  Idle, Walk, Run, Crouch, JumpSquat, Air, Landing, Attack,
  Shield, ShieldStun, Roll, SpotDodge, AirDodge,
  Hitstun, Knockdown, GetUp, Helpless,
  LedgeHang, LedgeClimb, Holding, Grabbed, ShieldBreak,
  Dead, Respawn
};

constexpr float kMaxShield = 50.0f;

struct Fighter {
  int id = 0;          // index in Match::fighters
  int port = 0;        // controller slot (0..3)
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

  // Input
  InputState in, prev;
  Vec2 stickHist[6];

  // Stats
  int kos = 0;
  int falls = 0;
  float damageDealt = 0.0f;
  int koFrame = -1;     // frame of elimination (for ranking)

  // Visual helpers (read by the renderer)
  int flash = 0;
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
  const MoveDef* currentMove() const {
    return (state == FState::Attack && move >= 0) ? &def->moves[move] : nullptr;
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
