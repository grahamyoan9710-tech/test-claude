#pragma once
#include <array>
#include <vector>

#include "game/common.hpp"

// ---- Fighters (render_fighter.cpp) ----------------------------------------
// kZonaiGlow (ancient-tech teal-green) lives with the palette in common.hpp.

// Pose in fighter-local pixels, relative to the body center, facing right.
struct Pose {
  float rot = 0.0f;       // whole body rotation (radians, + = forward flip)
  Vec2 shift;             // body center offset
  float lean = 0.0f;      // torso lean (+ = forward)
  float crouch = 0.0f;    // 0..1, lowers the hips
  float squash = 1.0f;    // vertical scale
  float sx = 1.0f;        // horizontal scale (squash & stretch)
  Vec2 handF, handB;      // hand targets
  Vec2 footF, footB;      // foot targets
  bool weapon = false;    // draw the weapon
  Vec2 weaponDir{1, 0};
  bool hurt = false;      // pained face
  bool blink = false;
  float trail = 0.0f;     // swoosh intensity on the attacking limb
  Vec2 trailPos;          // hitbox position for the swoosh
  float trailRadius = 0.0f;
  HitFx trailFx = HitFx::Normal;
  Limb trailLimb = Limb::None;
  Vec2 flow, flowTip;     // drag on scarves, capes and tails (secondary motion, ~[-1.5, 1.5])
  float glow = 0.0f;      // extra glow of the ancient-tech runes (ultimate ready)
  bool ghost = false;     // afterimage: flat tinted silhouette
};

Pose restPose(const CharacterDef& def, float time);
Pose computePose(const Fighter& f, const Match& m, float time);
void drawCharacter(const CharacterDef& def, int palette, const Pose& pose, Vec2 feet, int facing,
                   float scale, float alpha, Color flashColor, float flash, float time);
// Where the rig really puts a limb end (IK reach applied) / the weapon tip, in world space.
Vec2 limbWorld(const CharacterDef& def, const Pose& pose, Vec2 feet, int facing, float scale, Limb limb,
               bool weaponTip = false);
void drawFighter(const Fighter& f, const Match& m, float time, bool debugBoxes);
void drawFighterTags(const Match& m);  // "J1" / "CPU" above everyone, after all the fighters
void drawPortrait(const CharacterDef& def, int palette, Vec2 center, float size, float time);

// ---- Weapons (render_weapons.cpp) ---------------------------------------------
// A weapon held at `grip`, pointing along `dir` (unit), at `scale` (1 = a 100 px fighter).
void drawWeaponVisual(WeaponVisual v, Vec2 grip, Vec2 dir, float scale, float alpha, float time);
float weaponVisualLength(WeaponVisual v);  // grip to tip, at scale 1
// Weapons lying on the stage (render_items.cpp)
void drawItems(const Match& m, float time);

// ---- Big effects (render_vfx.cpp) -------------------------------------------------
void drawBeams(const Match& m, float time);              // beams of the moves being performed (world space)
void drawUltimateBackdrop(const Match& m, float time);   // domain / cinematic behind the fighters (world space)
// The domain's foreground layer (petal storm, slashes, embers...): after the platforms, before the fighters.
void drawUltimateFront(const Match& m, float time);

// Signature element of each hero (auras, cut-ins, domains).
inline HitFx heroElement(Hero h) {
  switch (h) {
    case Hero::Asta: return HitFx::AntiMagic;
    case Hero::Yuno: return HitFx::Wind;
    case Hero::Ichigo: return HitFx::Reiatsu;
    case Hero::Byakuya: return HitFx::Petal;
    case Hero::Link: return HitFx::Light;
    case Hero::Ganondorf: return HitFx::Gloom;
    case Hero::Kratos: return HitFx::Fire;
    case Hero::Gojo: return HitFx::Cursed;
    case Hero::Sukuna: return HitFx::Blood;
    default: return HitFx::Normal;
  }
}

// ---- VFX drawing kit (render_vfx.cpp): soft gradients made of a few big triangles ------
// (software GL pays for every pixel and every tiny triangle: prefer these over many small shapes)
namespace vfx {
float hash01(uint32_t i);                                 // deterministic noise in [0, 1]
void additive(bool on);                                   // additive blending on / back to the frame's blending
void tri(Vec2 a, Vec2 b, Vec2 c, Color ca, Color cb, Color cc);
void quad(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Color ca, Color cb, Color cc, Color cd);
void glow(Vec2 c, float r, Color in, Color out, int seg = 0);   // radial gradient disc
void ring(Vec2 c, float r0, float r1, Color in, Color out, int seg = 0);  // annulus, r0 colour `in` -> r1 `out`
// Soft ring: transparent inside and outside, `col` at its middle radius.
void softRing(Vec2 c, float r, float thick, Color col, int seg = 0);
// Tapered streak from a (width w0) to b (width w1): opaque core, transparent edges.
void streak(Vec2 a, Vec2 b, float w0, float w1, Color c0, Color c1);
// Hard-edged tapered stroke (a filled wedge / quad).
void wedge(Vec2 a, Vec2 b, float w0, float w1, Color c0, Color c1);
// Jagged lightning from a to b; `core` (if not blank) is drawn thinner on top.
void bolt(Vec2 a, Vec2 b, float jag, int segs, uint32_t seed, float w, Color col, Color core = BLANK);
// Crescent (convex side along fwd): outer radius r, thickness at the middle `thick`, half span in radians.
void crescent(Vec2 c, Vec2 fwd, float r, float thick, float span, Color body, Color edge);
// Star burst: `rays` spikes from r0 to r1, `width` radians wide at their base.
void burst(Vec2 c, int rays, float r0, float r1, float width, float rot, Color in, Color out, uint32_t seed = 0);
Rect viewRect(float margin = 0.0f);                       // world rectangle on screen (inside beginWorld)
}  // namespace vfx

// ---- World (render_world.cpp) -----------------------------------------------
void drawBackground(const StageDef& stage, Vec2 camTarget, float camZoom, float time);
void drawPlatforms(const Match& m, float time);
void drawPlatformList(const std::vector<Platform>& platforms, StageTheme theme, float time);
void drawProjectiles(const Match& m, float time);
void drawOrb(const Match& m, float time);
void drawStagePreview(const StageDef& stage, Rectangle area, float time);

// ---- HUD (render_hud.cpp) ---------------------------------------------------
// devices (indexed by port, optional): shows each human's ultimate button once the meter is full.
void drawHud(const Match& m, const std::array<float, 4>& shake, float time, const std::array<int, 4>* devices = nullptr);
void drawOffscreenBubbles(const Match& m, const Camera2D& cam, float time);
void drawCountdown(const Match& m);
