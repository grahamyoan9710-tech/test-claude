#pragma once
// Data model for fighters: frame data, hitboxes, projectiles and character stats.
#include <array>
#include <string>
#include <vector>

#include "core/math.hpp"

// Which body part "performs" a hitbox. Used by the renderer to animate the
// limb towards the hitbox, so every move gets an animation for free.
enum class Limb : uint8_t { None, ArmFront, ArmBack, LegFront, LegBack, Body, Head, Weapon };

// Visual / audio flavour of a hit.
enum class HitFx : uint8_t { Normal, Fire, Electric, Slash, Dark, Magic };

struct HitboxDef {
  int start = 0, end = 0;  // active on frames [start, end)
  Vec2 offset;             // from the fighter's center, as if facing right (+y = down)
  float radius = 20.0f;
  float damage = 5.0f;
  float angle = 361.0f;    // degrees: 0 = forward, 90 = up, 270 = down, 361 = auto
  float baseKB = 20.0f;    // base knockback
  float growth = 80.0f;    // knockback growth
  int group = 0;           // a victim is hit at most once per group per move
  Limb limb = Limb::ArmFront;
  HitFx fx = HitFx::Normal;
  bool reverse = false;    // launches behind the attacker (back hits)
};

enum class ProjVisual : uint8_t { Fireball, Bolt, Shuriken, Star };

struct ProjectileDef {
  Vec2 spawnOffset;        // from center, facing right
  Vec2 velocity;           // facing right
  float gravity = 0.0f;
  int lifetime = 60;
  float radius = 12.0f;
  HitboxDef hit;           // damage / knockback (offset & frames ignored)
  int count = 1;
  float spreadDeg = 0.0f;
  bool bounces = false;
  bool piercing = false;   // keeps flying after a hit
  ProjVisual visual = ProjVisual::Fireball;
};

enum class VelMode : uint8_t { Set, SetX, SetY, Add, Scale, StickDir };

// Scripted velocity change during a move (recoveries, dashes, slams...).
struct VelEvent {
  int frame = 0;
  VelMode mode = VelMode::Set;
  Vec2 v;                 // x is facing-relative (except for Scale); StickDir uses v.x as speed
  float stickX = 0.0f;    // extra horizontal speed from the stick (absolute direction)
  bool airOnly = false;   // only applies while airborne
};

enum MoveId : int {
  Jab, Jab2, Jab3, FTilt, UTilt, DTilt, DashAttack,
  FSmash, USmash, DSmash,
  NAir, FAir, BAir, UAir, DAir,
  NSpecial, SSpecial, USpecial, DSpecial,
  Grab, Pummel, FThrow, BThrow, UThrow, DThrow,
  LedgeAttack, GetupAttack,
  Extra1, Extra2, Extra3,  // follow-ups (combos, landing shockwaves...)
  FinalSmash,              // granted by breaking the Clash Orb
  MOVE_COUNT
};

struct MoveDef {
  std::string name;
  int duration = 0;                 // 0 = move does not exist
  int landingLag = 6;               // when an aerial lands
  std::vector<HitboxDef> hitboxes;
  std::vector<VelEvent> vel;
  float gravityMul = 1.0f;
  float frictionMul = 1.0f;
  bool allowDrift = true;
  // Charging (smash attacks, some specials): frame freezes while held.
  int chargeFrame = -1;
  int maxCharge = 60;
  float chargeDamageMul = 0.4f;     // damage * (1 + mul * chargeRatio)
  // Projectile
  int projectileFrame = -1;
  ProjectileDef projectile;
  // Defensive windows
  int intangibleStart = -1, intangibleEnd = -1;
  int armorStart = -1, armorEnd = -1;
  int counterStart = -1, counterEnd = -1;  // hitboxes only fire if countered
  // Teleport (direction from the stick)
  int teleportFrame = -1;
  float teleportDist = 0.0f;
  // Flow control
  bool helplessAfter = false;
  bool endOnLand = false;
  int landMove = -1;                // move started when landing (after leaving ground)
  int loopFrame = -1;               // frame held while airborne
  int nextMove = -1;                // combo follow-up
  int comboStart = 0, comboEnd = 0; // window to buffer the follow-up
  int comboAt = 0;                  // frame where the follow-up begins
  bool comboOnSpecial = false;      // follow-up uses special instead of attack
  // Grabs / throws
  bool isGrab = false;              // hitboxes are grab boxes
  int throwFrame = -1;              // apply hitboxes[0] to the held fighter
  bool pummel = false;              // throwFrame hit does not release
  bool airOnly = false;

  bool exists() const { return duration > 0; }
};

enum class BodyStyle : uint8_t { Brawler, Speedster, Brute, Knight, Puff, Ninja };

struct Palette {
  Color3 primary, secondary, accent, skin;
};

struct CharacterDef {
  std::string name;
  std::string title;
  BodyStyle style = BodyStyle::Brawler;
  std::array<Palette, 2> palettes;
  float width = 50, height = 100;
  float weight = 100;
  float walkSpeed = 4.0f, runSpeed = 8.0f, groundAccel = 1.2f, friction = 0.6f;
  float airSpeed = 5.0f, airAccel = 0.45f, airFriction = 0.06f;
  float gravity = 0.55f, maxFall = 9.0f, fastFall = 14.0f;
  float jumpVel = 14.0f, shortHopVel = 9.5f, airJumpVel = 13.0f;
  int airJumps = 1;
  int jumpSquat = 4;
  float weaponLength = 0.0f;  // drawn weapon (sword, hammer, kunai)
  std::array<MoveDef, MOVE_COUNT> moves;
};

const std::vector<CharacterDef>& roster();

// --- helpers used to author the roster (roster_normals.cpp) ---------------
HitboxDef hb(int start, int end, float ox, float oy, float radius, float dmg, float angle,
             float bkb, float kbg, Limb limb, int group = 0, HitFx fx = HitFx::Normal);
MoveDef mv(const char* name, int duration, std::vector<HitboxDef> boxes, int landingLag = 6);
// Generic normal attacks, scaled by body size (S) and power (P).
void buildBrawlerNormals(CharacterDef& c, float S, float P, HitFx fx);
void buildSwordNormals(CharacterDef& c, float S, float P);
void buildCommonMoves(CharacterDef& c, float S, float P);  // grabs, throws, ledge/getup
