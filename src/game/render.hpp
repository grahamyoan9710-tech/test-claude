#pragma once
#include <array>
#include <vector>

#include "game/common.hpp"

// ---- Fighters (render_fighter.cpp) ----------------------------------------
// Pose in fighter-local pixels, relative to the body center, facing right.
struct Pose {
  float rot = 0.0f;       // whole body rotation (radians, + = forward flip)
  Vec2 shift;             // body center offset
  float lean = 0.0f;      // torso lean (+ = forward)
  float crouch = 0.0f;    // 0..1, lowers the hips
  float squash = 1.0f;    // vertical scale
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
};

Pose restPose(const CharacterDef& def, float time);
Pose computePose(const Fighter& f, const Match& m, float time);
void drawCharacter(const CharacterDef& def, int palette, const Pose& pose, Vec2 feet, int facing,
                   float scale, float alpha, Color flashColor, float flash, float time);
void drawFighter(const Fighter& f, const Match& m, float time, bool debugBoxes);
void drawPortrait(const CharacterDef& def, int palette, Vec2 center, float size, float time);

// ---- World (render_world.cpp) -----------------------------------------------
void drawBackground(const StageDef& stage, Vec2 camTarget, float camZoom, float time);
void drawPlatforms(const Match& m, float time);
void drawPlatformList(const std::vector<Platform>& platforms, StageTheme theme, float time);
void drawProjectiles(const Match& m, float time);
void drawOrb(const Match& m, float time);
void drawStagePreview(const StageDef& stage, Rectangle area, float time);

// ---- HUD (render_hud.cpp) ---------------------------------------------------
void drawHud(const Match& m, const std::array<float, 4>& shake, float time);
void drawOffscreenBubbles(const Match& m, const Camera2D& cam, float time);
void drawCountdown(const Match& m);
