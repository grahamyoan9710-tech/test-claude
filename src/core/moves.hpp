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
enum class HitFx : uint8_t {
  Normal, Fire, Electric, Slash, Dark, Magic,
  AntiMagic,  // black lightning that cancels magic (Asta)
  Wind,       // green-white gusts (Yuno)
  Reiatsu,    // blue-white spiritual pressure (Ichigo)
  Petal,      // cherry-blossom blades (Byakuya)
  Light,      // golden sacred light (Link)
  Gloom,      // red-black malice (Ganondorf)
  Ice,        // frost (Leviathan axe)
  Cursed,     // blue / purple cursed energy (Gojo)
  Blood,      // crimson slashes (Sukuna)
  Chakra,     // orange / blue chakra swirls (Naruto)
  Lightning,  // crackling blue-white lightning (Chidori, Raikiri, Kirin)
  BlackFlame, // Amaterasu's black flames (Sasuke)
  Hollow,     // green-black hollow energy (Ulquiorra: Cero Oscuras, Luz de la Luna)
  Berserk,    // Kenpachi's wild yellow reiatsu
  Water,      // water jutsu (Kakashi)
  Wolf,       // pale blue ceros and spectral wolves (Starrk)
  Claw,       // cyan claw slashes (Grimmjow)
  Count
};

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
  bool impact = false;     // signature hit: anime "impact frames" when it connects
  // Blade swing: the hitbox rides the weapon instead of sitting at `offset`. The blade turns around
  // the shoulder (kShoulder, scaled by the fighter's height) from angle a0 to a1 (degrees, 0 = forward,
  // 90 = up, as if facing right) during [start, end) and hits along it from reach0 to reach1 (fractions
  // of the blade, measured from the grip). The renderer draws the blade at the very same angle.
  bool swing = false;
  float a0 = 0.0f, a1 = 0.0f;
  float reach0 = 0.0f, reach1 = 1.0f;
  // Cinematic ultimates: `capture` catches the victim instead of launching it. A captured fighter is
  // held by the move (see MoveDef::holdAt); every later hitbox of the move then connects with it, without
  // knockback, until a `launch` hitbox (or the end of the move) releases it with its knockback.
  bool capture = false;
  bool launch = false;
};

// Shoulder pivot of blade swings, as a fraction of the fighter's height (from its center, facing right).
constexpr float kShoulderX = 0.03f, kShoulderY = -0.17f;
constexpr float kGripDist = 0.28f;  // shoulder -> hand distance along the blade, fraction of the height

enum class ProjVisual : uint8_t {
  Fireball, Bolt, Shuriken, Star,
  BlackSlash,   // Asta: black crescent of anti-magic
  WindBlade,    // Yuno: thin green crescents
  Tornado,      // Yuno: travelling whirlwind
  Getsuga,      // Ichigo: huge crescent wave of reiatsu
  Cero,         // Ichigo: red blast from the hollow mask
  Petals,       // Byakuya: swarm of blade petals
  Arrow,        // Link: arrow
  LightArrow,   // Link: arrow of light
  Bomb,         // Link: Zonai bomb
  SwordBeam,    // Link: Master Sword beam
  DarkOrb,      // Ganondorf: sphere of gloom
  GloomSpear,   // Ganondorf: spear of malice
  Axe,          // Kratos: spinning Leviathan axe (comes back)
  ChainBlade,   // Kratos: thrown Blade of Chaos
  BlueOrb,      // Gojo: Lapse Blue (attracts)
  RedOrb,       // Gojo: Reversal Red (repels)
  PurpleOrb,    // Gojo: Hollow Purple (erases)
  Dismantle,    // Sukuna: invisible cutting lines
  FireArrow,    // Sukuna: Fuga
  Lightning,    // Sukuna: Kamutoke
  ReiatsuWave,  // Kenpachi: wild yellow slash wave
  CeroOscuras,  // Ulquiorra: black-green blast
  LuzDeLaLuna,  // Ulquiorra: green light spear
  Lanza,        // Ulquiorra: Lanza del Relampago (huge green lightning javelin)
  BarrelBomb,   // Hunter: large barrel bomb
  Phial,        // Hunter: charge blade phial burst
  GunlanceShell,// Hunter: gunlance shelling (fire burst)
  Rasengan,     // Naruto: spinning chakra sphere
  Rasenshuriken,// Naruto: wind shuriken with a white ring of micro-blades
  ShadowClone,  // Naruto: a running shadow clone
  Bijudama,     // Naruto: tailed beast bomb (black-purple sphere)
  GreatFireball,// Sasuke / Kakashi: Katon great fireball (orange, big)
  ChidoriSpear, // Sasuke: thrown lightning spear
  KirinBolt,    // Sasuke: the lightning dragon diving from the sky
  IndraArrow,   // Sasuke: Susanoo's arrow
  BlackFlames,  // Sasuke: Amaterasu (sticks to its target)
  Kunai,        // Kakashi: thrown kunai
  WaterDragon,  // Kakashi: Suiton water dragon
  KamuiVortex,  // Kakashi: dimensional vortex (sticks to its target)
  KamuiShuriken,// Kakashi: Susanoo's giant shuriken
  GiantGetsuga, // Ichigo: ultimate Getsuga Tensho
  LightPillar,  // Link: arrow of light falling from the sky
  GloomPillar,  // Ganondorf: gloom erupting from the ground
  PetalSphere,  // Byakuya: Gokei, a sphere of blades around the target
  WindRain,     // Yuno: wind blades raining from the sky
  ZephyrSpear,  // Yuno: giant wind spear
  IceAxe,       // Kratos: Leviathan axe wrapped in frost (ultimate)
  MaxBlue,      // Gojo: maximum output blue (huge vortex)
  GetsugaCross, // Ichigo: Getsuga Jujisho, two black crescents crossing (hollow mask)
  Desgarron,    // Grimmjow: five claws of cyan reiatsu raking forward
  PanteraDart,  // Grimmjow: Garra de la Pantera, a forearm blade fired like a missile
  CeroWolf,     // Starrk: a wolf of blue cero running (or leaping on its prey)
  StarrkCero,   // Starrk: a small blue cero (Cero Metralleta)
  Count
};

// How a projectile picks its spawn point.
enum class ProjTarget : uint8_t {
  Self,     // from the fighter (spawnOffset is facing-relative)
  Nearest,  // on the nearest opponent (spawnOffset relative to its center, not mirrored)
  All       // one on each opponent
};

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
  int rehit = 0;           // > 0: hits the same fighter again every `rehit` frames (swarms, vortices)
  float pull = 0.0f;       // drags fighters within pullRadius towards it (px / frame)
  float pullRadius = 0.0f;
  bool boomerang = false;  // flies back to its owner after half its lifetime
  float grow = 0.0f;       // radius growth per frame
  float explodeRadius = 0; // explodes when it ends (hit, ground, timeout): area damage
  HitboxDef explode;       // damage / knockback of the explosion
  ProjTarget target = ProjTarget::Self;
  bool follow = false;     // stays on its target while it lives (black flames, vortices, petal spheres)
  int armDelay = 0;        // harmless for its first frames (the warning before a strike from the sky)
  bool ignoreGround = false;  // flies through the stage (strikes from the sky, eruptions)
  Vec2 jitter;             // random spawn offset in [-jitter, +jitter] (rains, eruptions)
};

// One more projectile fired by a move at a given frame (cinematic ultimates: a rain, then its finisher).
struct ProjShot {
  int frame = 0;
  ProjectileDef def;
};

// A beam drawn from the fighter during a move (the damage comes from the move's hitboxes, see beamHits).
struct BeamDef {
  int start = -1, end = -1;  // drawn on frames [start, end)
  Vec2 from;                 // from the fighter's center, facing right
  float angle = 0.0f;        // degrees, 0 = forward, 90 = up
  float length = 0.0f, width = 0.0f;
  HitFx fx = HitFx::Normal;
};

// Body language of a special move (the renderer builds the pose from it).
enum class PoseHint : uint8_t {
  Auto,       // the limb follows the hitbox
  Cast,       // both hands thrust forward (blasts, beams)
  Overhead,   // big two-handed overhead swing
  Thrust,     // lunging stab
  Spin,       // whirling body
  HandSign,   // hand sign in front of the face (domains, techniques)
  Bow,        // drawing a bow
  Throw,      // throwing overhand
  Uppercut,   // rising blow
  Stomp,      // slam into the ground
  Roar,       // arms wide, chest out
  Rush,       // body-first dash
  Raise,      // one arm raised to the sky (summon, orb above)
  Point,      // one finger pointed forward (Ulquiorra's cero)
  TwoHanded,  // both hands on the hilt above the head (Kenpachi's kendo, great sword charge)
  Kneel       // crouched low, gathering power
};

// The cinematic of an ultimate (the renderer draws it: domain, summon, transformation...).
enum class UltFx : uint8_t {
  None,
  BlackMeteorite, BlackDivider, DemonUnion,          // Asta
  SpiritStorm, WindBladeShower, ZephyrSpear,         // Yuno
  GetsugaJujisho, GiantGetsuga, VastoLordeCero,      // Ichigo
  Kageyoshi, Gokei, Hakuteiken,                      // Byakuya
  LightDragon, MasterSwordBeam, LightArrowRain,      // Link
  DemonDragon, DemonKingRampage, GloomPillars,       // Ganondorf
  SpartanRage, Fimbulwinter, ChaosWhip,              // Kratos
  MaxBlue, ImaginaryPurple, InfiniteVoid,            // Gojo
  MalevolentShrine, KamutokeStrike, FugaBlast,       // Sukuna
  Kendo, NozarashiCleave, RedDemon,                  // Kenpachi
  CeroOscuras, LanzaRelampago, SegundaEtapa,         // Ulquiorra
  TrueChargedSlash, AmpedDischarge, WyvernFire,      // Hunter
  UzumakiBarrage, Rasenshuriken, KuramaBijudama,     // Naruto
  Kirin, IndraArrow, Amaterasu,                      // Sasuke
  Raikiri, Kamui, KamuiShuriken,                     // Kakashi
  GranReyCero, PanteraRelease, DesgarronClaws,       // Grimmjow
  CeroMetralleta, LosLobos, StarrkRelease,           // Starrk
  Count
};

// Transformations (Spartan Rage, Bankai, Vasto Lorde...): the renderer draws the form, the moves and
// stats change while it lasts.
enum class Form : uint8_t {
  None,
  SpartanRage,   // Kratos: red-hot, glowing tattoos, fists of fire
  DemonUnion,    // Asta: black skin markings, demon horn and black wing
  VastoLorde,    // Ichigo: white hollow body, horns, long orange hair, black sclera
  HollowMask,    // Ichigo: the Visored hollow mask over his face
  RedDemon,      // Kenpachi: bankai, red skin, horns, burning red aura
  Segunda,       // Ulquiorra: Segunda Etapa, black body, huge bat wings, tail
  KuramaAvatar,  // Naruto: golden chakra cloak with the giant fox around him
  SusanooPurple, // Sasuke: purple Susanoo around him
  SusanooBlue,   // Kakashi: blue Susanoo around him
  Pantera,       // Grimmjow: Resurreccion, long hair, panther ears and tail, white armour, claws
  LosLobos,      // Starrk: Resurreccion, fur-lined coat, a pistol in each hand
  Count
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
  FinalSmash,              // the ultimate: full ultimate meter (or Clash Orb) + ultimate button
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
  int projectileRepeat = 0;         // fired again `projectileRepeat` times, every `projectileEvery` frames
  int projectileEvery = 0;
  ProjectileDef projectile;
  std::vector<ProjShot> shots;      // more projectiles, of other kinds
  // Defensive windows
  int intangibleStart = -1, intangibleEnd = -1;
  int armorStart = -1, armorEnd = -1;
  int counterStart = -1, counterEnd = -1;  // hitboxes only fire if countered
  // Teleport (direction from the stick; horizontal-only moves keep their height)
  int teleportFrame = -1;
  float teleportDist = 0.0f;
  bool teleportForward = false;     // straight ahead instead of the stick direction
  bool turnAfterTeleport = false;   // faces back after it (strikes from behind)
  // Use limits
  int cooldown = 0;                 // frames from its start before it can be used again (0: none)
  int channelEnd = -1;              // the technique is held up to this frame: its clip keeps the cast pose
  // Presentation
  int superFlash = 0;               // frames of "super freeze" when the move starts (cut-in with its name)
  // A PixelLab effect the renderer plays behind the fighter from that frame, following it (offset from the feet,
  // facing right): a burst of energy, an aura.
  const char* castFx = nullptr;
  int castFxFrame = -1;
  Vec2 castFxAt;
  bool castFxStays = false;         // stays where it started (a trail) instead of following the fighter
  PoseHint pose = PoseHint::Auto;
  BeamDef beam;
  bool reflects = false;            // a counter that sends projectiles back instead of taking them
  // Flow control
  bool helplessAfter = false;
  bool endOnLand = false;
  int landMove = -1;                // move started when landing (after leaving ground)
  int loopFrame = -1;               // frame held while airborne
  int nextMove = -1;                // combo follow-up
  int comboStart = 0, comboEnd = 0; // window to buffer the follow-up
  int comboAt = 0;                  // frame where the follow-up begins
  bool comboOnSpecial = false;      // follow-up uses special instead of attack
  // Cinematic ultimates
  UltFx ultFx = UltFx::None;        // the cinematic the renderer draws while the move runs
  Vec2 holdAt;                      // captured fighters are pulled there (from the center, facing right)
  bool holdInPlace = false;         // ... or frozen where they were caught (domains)
  bool teleportToTarget = false;    // teleportFrame: appears right in front of the nearest opponent
  int formIndex = -1;               // transformation started at formFrame (index in CharacterDef::forms)
  int formFrame = -1;
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
  Color3 hair{40, 30, 30}, eyes{70, 52, 44};
};

// Who the fighter is (drives the procedural drawing: silhouette, outfit, hair, accessories).
enum class Hero : uint8_t {
  Asta, Yuno, Ichigo, Byakuya, Link, Ganondorf, Kratos, Gojo, Sukuna,
  Kenpachi, Ulquiorra, Hunter, Naruto, Sasuke, Kakashi,
  Starrk, Grimmjow,
  Count
};

// Drawn weapons: the fighters' own blades and the weapons that drop during a match.
enum class WeaponVisual : uint8_t {
  None,
  AstaSword,       // Asta's notched black great sword
  DemonSlayer,     // huge black slab of a sword
  DemonDweller,    // slender black katana
  Grimoire,        // floating four-leaf grimoire
  WindSpear,       // spear of green wind
  Zangetsu,        // giant cleaver with a cloth-wrapped grip
  TensaZangetsu,   // black bankai katana with a chain
  Katana,          // Byakuya's sword
  Senbonzakura,    // blade dissolving into pink petals
  Kageyoshi,       // bankai: petal storm swords
  MasterSword,     // blade of evil's bane
  BowOfLight,      // golden bow
  DemonKingSword,  // Ganondorf's gloom sword
  GloomSpear,      // spear of malice
  LeviathanAxe,    // frost axe with runes
  BladesOfChaos,   // two chained fire blades
  HollowPurple,    // purple orb above the hand
  SixEyes,         // blindfold removed: shining blue eyes
  Kamutoke,        // lightning bow-spear
  CursedFinger,    // Sukuna's finger (power-up)
  ShieldAndSword,  // Link's base kit (sword + shield)
  GuardianShield,  // Kratos's base kit
  KenpachiSword,   // Kenpachi: long notched, chipped katana with a bare grip
  Nozarashi,       // Kenpachi's shikai: giant cleaver-axe
  RedBankai,       // Kenpachi's bankai: huge crimson cleaver dripping red reiatsu
  LuzDeLaLuna,     // Ulquiorra: green light spear (Murcielago, resurreccion)
  SegundaCore,     // Ulquiorra: segunda etapa (black-green crystal of power)
  GreatSword,      // Hunter: huge bone-and-steel great sword
  ChargeBlade,     // Hunter: charge blade (sword + shield with glowing phials)
  Gunlance,        // Hunter: gunlance (lance with a shell chamber + big shield)
  SageScroll,      // Naruto: toad sage scroll (sage mode)
  KuramaChakra,    // Naruto: Kurama's golden chakra
  Kusanagi,        // Sasuke: straight Kusanagi sword
  SusanooBow,      // Sasuke: purple flame bow of the Susanoo
  Mangekyo,        // Sasuke: eternal Mangekyo Sharingan (amaterasu)
  KakashiKunai,    // Kakashi: kunai
  SharinganEye,    // Kakashi: Sharingan (kamui)
  DoubleMangekyo,  // Kakashi: double Mangekyo (Susanoo)
  Pantera,         // Grimmjow: his katana, teal grip
  PanteraClaws,    // Grimmjow: claws of cyan reiatsu on both hands (power-up)
  LobosPack,       // Starrk: spectral wolves running round him (power-up)
  Lilynette,       // Starrk: Lilynette, the other half of his soul, floating at his side (power-up)
  LobosPistol,     // Starrk: a pistol of Los Lobos (drawn in both hands while released)
  Count
};

// A weapon that drops during the match: only its owner (character) can pick it up.
struct WeaponDef {
  std::string name;           // shown when it is picked up
  std::string desc;           // one short French line
  WeaponVisual visual = WeaponVisual::None;
  HitFx fx = HitFx::Normal;   // element of the fighter's attacks while held
  float damageMul = 1.2f;     // every hit
  float kbMul = 1.1f;         // knockback growth
  float reachMul = 1.0f;      // hand / weapon hitboxes reach further (and the blade is that much longer)
  float blade = 0.0f;         // > 0: blade length (grip to tip) while held, else the fighter's blade * reachMul
  int duration = 60 * 18;     // frames before it vanishes
  std::vector<std::pair<int, MoveDef>> moves;  // moves replaced while held (its ultimate included)
  // A full kit: every ground attack is its M1 combo (jab, jab2, jab3), every aerial its air M1 (nair); its specials
  // are its 4 skills.
  bool kit = false;
};

// A transformation: started by a move (MoveDef::formIndex), lasts `duration` frames (0 = until that move ends).
struct FormDef {
  Form form = Form::None;
  int duration = 0;
  HitFx fx = HitFx::Normal;   // element of the base attacks while transformed (Normal = unchanged)
  float damageMul = 1.0f;     // every hit
  float kbTakenMul = 1.0f;    // knockback received
  bool superArmor = false;    // weak hits do not make it flinch
  std::vector<std::pair<int, MoveDef>> moves;  // moves replaced while transformed
};

struct CharacterDef {
  std::string name;
  std::string title;
  std::string series;             // universe the fighter comes from
  Hero hero = Hero::Asta;
  BodyStyle style = BodyStyle::Brawler;
  WeaponVisual baseWeapon = WeaponVisual::None;  // drawn in hand for Limb::Weapon moves
  std::array<Palette, 2> palettes;
  float width = 50, height = 100;
  float weight = 100;
  float walkSpeed = 4.0f, runSpeed = 8.0f, groundAccel = 1.2f, friction = 0.6f;
  float airSpeed = 5.0f, airAccel = 0.45f, airFriction = 0.06f;
  float gravity = 0.55f, maxFall = 9.0f, fastFall = 14.0f;
  float jumpVel = 14.0f, shortHopVel = 9.5f, airJumpVel = 13.0f;
  int airJumps = 1;
  int jumpSquat = 4;
  float weaponLength = 0.0f;  // blade from grip to tip (sword, cleaver, kunai): swing hitboxes ride it
  std::array<MoveDef, MOVE_COUNT> moves;
  std::array<WeaponDef, 2> weapons;
  std::vector<FormDef> forms;
};

const std::vector<CharacterDef>& roster();
// The hitboxes of the moves drawn by generated clips are moved onto what the clips draw (core/roster_hitfit.inc,
// written by `super_clash --fit-hitboxes`). That tool sets this before the first roster() call to get the hitboxes
// as written in the roster, the fit not applied.
extern bool g_rawHitboxes;

// --- helpers used to author the roster (roster_normals.cpp) ---------------
HitboxDef hb(int start, int end, float ox, float oy, float radius, float dmg, float angle,
             float bkb, float kbg, Limb limb, int group = 0, HitFx fx = HitFx::Normal);
MoveDef mv(const char* name, int duration, std::vector<HitboxDef> boxes, int landingLag = 6);
// Generic normal attacks, scaled by body size (S) and power (P).
void buildBrawlerNormals(CharacterDef& c, float S, float P, HitFx fx);
void buildSwordNormals(CharacterDef& c, float S, float P, HitFx smashFx = HitFx::Slash);
// A weapon's kit M1 with a blade (WeaponDef::kit): jab, jab2, jab3 and the air M1 (nair).
void addBladeKit(WeaponDef& w, float S, float P, HitFx fx, float speed = 1.0f, bool thrust2 = false, int airCuts = 2);
// A blade swing hitbox: sweeps from a0 to a1 degrees, hits from reach0 to reach1 of the blade.
HitboxDef sw(int start, int end, float a0, float a1, float reach0, float reach1, float radius, float dmg, float angle,
             float bkb, float kbg, int group = 0, HitFx fx = HitFx::Slash);
void buildCommonMoves(CharacterDef& c, float S, float P);  // grabs, throws, ledge/getup
// Hitboxes along a beam (circles of radius width/2), all in one group per `hits` (multi-hit beams).
std::vector<HitboxDef> beamHits(const BeamDef& b, float dmg, float angle, float bkb, float kbg, int hits = 1,
                                int firstGroup = 0);
