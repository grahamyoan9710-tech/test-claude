#pragma once
#include <array>
#include <vector>

#include "core/fighter.hpp"
#include "core/stage.hpp"

struct MatchConfig {
  struct Slot {
    bool active = false;
    int character = 0;
    bool cpu = false;
    int level = 5;
  };
  int stage = 0;
  int stocks = 3;
  bool items = true;  // Clash Orb
  bool weapons = true;  // weapons drop from the sky (each one for a given character)
  bool training = false;  // infinite lives, the match never ends
  std::array<Slot, 4> slots;
  uint32_t seed = 12345;
};

enum class EvType : uint8_t {
  Hit, ShieldHit, KO, Jump, AirJump, Land, Dash, Grab, Throw, Projectile, ShieldBreak,
  Respawn, Tech, LedgeGrab, Counter, Swing, Clank, Countdown, Go, GameSet,
  OrbSpawn, OrbHit, OrbBreak, FinalSmash, UltReady,
  WeaponSpawn, WeaponPickup, WeaponBreak, WeaponLand,  // b = weapon slot, strength = character
  SuperFlash,  // a = fighter, b = move id: freeze + cut-in
  Explosion,   // strength = radius
  Reflect
};

// A weapon lying on the stage (or falling on it).
struct WeaponItem {
  int character = 0;  // roster index: only this character can pick it up
  int slot = 0;       // index in CharacterDef::weapons
  Vec2 pos, vel;      // pos = bottom centre
  bool grounded = false;
  int life = 0;       // frames left before it vanishes
  int age = 0;
};

// The Clash Orb: floats around the stage, whoever breaks it gets a final smash.
struct Orb {
  bool active = false;
  Vec2 pos, vel;
  float hp = 0.0f;
  float radius = 26.0f;
  int life = 0;
  int flash = 0;
  std::vector<int> hitBy;  // attacker * 4096 + move instance
};

struct GameEvent {
  EvType type = EvType::Hit;
  Vec2 pos;
  Vec2 dir;
  float strength = 0.0f;  // damage for hits, knockback for KOs...
  int a = -1;             // main fighter (attacker / jumper / KO'd fighter)
  int b = -1;             // other fighter (victim)
  HitFx fx = HitFx::Normal;
  bool impact = false;    // signature hit (impact frames)
};

struct Projectile {
  int owner = -1;
  Vec2 pos, vel;
  float gravity = 0.0f;
  float radius = 10.0f;
  int life = 0, maxLife = 1;
  HitboxDef hit;
  bool bounces = false;
  bool piercing = false;
  ProjVisual visual = ProjVisual::Fireball;
  bool alive = true;
  std::vector<int> hitList;
  // Behaviours (see ProjectileDef)
  int rehit = 0;
  std::vector<std::pair<int, int>> lastHit;  // fighter id -> frame (rehit)
  float pull = 0.0f, pullRadius = 0.0f;
  bool boomerang = false, returning = false;
  float grow = 0.0f;
  float explodeRadius = 0.0f;
  HitboxDef explode;
  int age = 0;
  float spawnRadius = 10.0f;
};

class Match {
 public:
  explicit Match(const MatchConfig& cfg);

  // Advance the simulation by one frame (1/60 s). inputs are indexed by port.
  void step(const std::array<InputState, 4>& inputs);

  const StageDef& stage() const { return *stageDef; }
  bool started() const { return countdown <= 0; }
  int ledgeCount() const { return int(platforms.size()) * 2; }
  bool ledgeValid(int ledge) const;
  Vec2 ledgePoint(int ledge) const;  // ledge = platform * 2 + (0 left / 1 right)
  int ledgeOwner(int ledge) const;
  // Final ranking: fighter ids from 1st to last.
  std::vector<int> ranking() const;
  // Training: put everybody back on their spawn point at 0%.
  void resetPositions();
  // Training / tests: drops one of fighter id's weapons right above it.
  void dropWeaponFor(int id, int slot);

  MatchConfig cfg;
  const StageDef* stageDef = nullptr;
  std::vector<Fighter> fighters;
  std::vector<Platform> platforms;
  std::vector<Projectile> projectiles;
  std::vector<GameEvent> events;
  Orb orb;
  int orbTimer = 0;       // frames until the next orb
  std::vector<WeaponItem> items;
  int itemTimer = 0;      // frames until the next weapon drop
  int superFreeze = 0;    // "super flash": everybody frozen while the move's cut-in plays
  int superOwner = -1;
  int freezeTimer = 0;    // final smash cinematic: everyone but the user is frozen
  int freezeOwner = -1;
  int frame = 0;
  int countdown = 200;
  bool finished = false;
  int winner = -1;
  int finishFrame = 0;
  Rng rng;

  // --- internals (split across several .cpp files) ---
  void emit(EvType t, Vec2 pos, int a = -1, int b = -1, float strength = 0.0f, Vec2 dir = {},
            HitFx fx = HitFx::Normal);
  // fighter_update.cpp
  void updateFighter(Fighter& f);
  void setState(Fighter& f, FState s, int timer = 0);
  void startMove(Fighter& f, int move);
  bool tryGroundActions(Fighter& f);
  bool tryAirActions(Fighter& f);
  void updateMove(Fighter& f);
  void airControl(Fighter& f, float driftMul = 1.0f);
  void applyGravity(Fighter& f, float mul = 1.0f);
  void groundFriction(Fighter& f, float mul = 1.0f);
  void startJump(Fighter& f);
  void airJump(Fighter& f);
  void updateLedge(Fighter& f);
  void updateGrabbed(Fighter& f);
  void releaseGrab(Fighter& f, bool escaped);
  // fighter_physics.cpp
  void moveFighter(Fighter& f);
  void onLand(Fighter& f, float impact);
  void tryLedgeGrab(Fighter& f);
  bool groundBelow(Vec2 feet, float maxDist) const;
  // combat.cpp
  void resolveHits();
  void updateProjectiles();
  void applyHit(Fighter* attacker, Fighter& victim, const HitboxDef& h, Vec2 hitPos, int dirSign,
                float damageMul, bool throwHit);
  void spawnProjectiles(Fighter& f, const ProjectileDef& p);
  bool tryCounter(Fighter& victim, Fighter* attacker);
  // match.cpp
  void checkBlastZones();
  void koFighter(Fighter& f);
  void respawnFighter(Fighter& f);
  void checkEnd();
  void updateOrb();
  void hitOrb(int attacker, float damage, Vec2 from, HitFx fx);
  void startFinalSmash(Fighter& f);
  // weapons.cpp
  void updateItems();
  void spawnWeapon(int character, int slot, Vec2 pos);
  void equipWeapon(Fighter& f, int slot);
  void loseWeapon(Fighter& f, bool broken);
  void explodeProjectile(Projectile& pr);
  void creditProjectile(int owner, Fighter& victim, float damage, bool wasShielding);
  void gainUlt(Fighter& f, float amount);
};

float computeKnockback(float percentAfter, float damage, float weight, float growth, float base);
