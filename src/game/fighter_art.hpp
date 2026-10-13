#pragma once
// Fighters made from an illustration instead of procedural shapes.
//
// scripts/rig_art.py cuts a picture of the fighter (full body, facing right) into parts — head, torso, the cloth
// of a skirt or a cape, and the limbs — packed in assets/fighters/<name>.png, and writes the joints marked on the
// picture and the measures of its limbs in assets/fighters/<name>.rig. In a fight the head, the torso and the
// cloth are the picture's own (rigid, never stretched); the arms and the legs are redrawn for each pose as lit
// volumes with the picture's widths, patterns and colours. All of it rides the same skeleton as the drawn
// fighters (render_body.cpp), with the picture's proportions. A picture without limb joints is only a portrait.
#include <string>
#include <vector>

#include "game/render.hpp"

// Fighter-local frame (body centre at the origin, feet at y = +height/2, +x the way it faces) to the screen.
struct Xf {
  Vec2 center;
  int facing;
  float scale, rot, squash, sx;
  Vector2 operator()(Vec2 l) const {
    Vec2 v{l.x * facing * scale * sx, l.y * scale * squash};
    return V(center + rotate(v, rot));
  }
};

// Joint positions of a pose, in the fighter-local frame.
struct Rig {
  float hr, tilt;
  Vec2 hips, chest, neck, head, shF, shB, hipF, hipB;
  Vec2 elbowF, elbowB, kneeF, kneeB, handF, handB, footF, footB;
};

// The skeleton of a pose with the drawn build's proportions (render_body.cpp).
Rig buildRig(const CharacterDef& d, const Pose& pose);

namespace art {
bool rigged(Hero h);    // the fighter itself is drawn from its picture
bool portrait(Hero h);  // the portraits are (the fighter may still be drawn by code)
// The skeleton of a pose with the picture's proportions, from the same pose on the drawn build (`built`): each
// limb takes the direction and the extension of the drawn one. False: no picture, the drawn build applies.
bool solve(const CharacterDef& d, const Pose& pose, const Rig& built, Rig& r);
// Rest pose guard: the stance of the picture (hand and foot targets, torso as drawn).
bool stance(const CharacterDef& d, Pose& p, float bob);
// The weapon goes between the two layers: the front fist closes over its grip.
enum class Layers { All, BelowWeapon, WeaponArm };
void drawFighter(const CharacterDef& d, const Pose& pose, const Rig& r, const Xf& X, float alpha, Color flashColor,
                 float flash, Layers layers = Layers::All);
// The picture in its own pose: the whole fighter (`height`: from the feet to the top of the skull), the head
// and shoulders in drawPortrait's frame, the head in a disc.
bool drawWhole(Hero h, Vec2 feet, float height, int facing, float alpha);
// Animations generated from the picture (PixelLab, see scripts/import_sheet.py): frame by frame drawings that
// replace the skeleton for the states and moves they cover. A clip is named after its state ("idle", "run",
// "roll"...), its move ("jab", "fsmash", "nspecial", "ultimate"...) or a move done with a picked-up weapon
// ("w0_jab", "w1_nspecial": weapon slot 0 or 1).
enum SheetAnim : int {
  kSheetIdle, kSheetWalk, kSheetRun, kSheetCrouch, kSheetJumpSquat, kSheetJump, kSheetAirJump, kSheetFall,
  kSheetLand, kSheetShield, kSheetRoll, kSheetSpotDodge, kSheetAirDodge, kSheetHurt, kSheetTumble, kSheetKnockdown,
  kSheetGetUp, kSheetHelpless, kSheetLedge, kSheetClimb, kSheetHold, kSheetGrabbed, kSheetDizzy, kSheetVictory,
  kSheetMove0,                                // + MoveId: one clip per move
  kSheetWeapon0 = kSheetMove0 + MOVE_COUNT,   // + MoveId: the move done with weapon slot 0
  kSheetWeapon1 = kSheetWeapon0 + MOVE_COUNT,  // ... with weapon slot 1
  kSheetCount = kSheetWeapon1 + MOVE_COUNT
};
const char* sheetName(int anim);  // "idle", "jab", "w0_jab"...
// Every PNG of assets/sprites is embedded under its name ("naruto_idle", "weapon_zangetsu"); sheets.txt indexes them.
bool embeddedSprite(const char* name, const unsigned char** data, unsigned* size);
const std::string& spriteIndex();
bool hasSheet(Hero h, int anim);
bool hasClips(Hero h);  // the fighter has at least one generated clip
// How much lower the clip's first frame is drawn (world units; 0 for most clips): the ledge climb starts from the
// hands on the ledge, where the hanging clip holds them, and ends standing on its anchor.
float sheetDrop(const CharacterDef& d, int anim);
// For the hitbox check (--check-hitboxes): the clip frame `pose` shows, and that frame's opacity on the CPU
// (alpha > 0 per pixel, row by row) with the clip's anchor and pixels per fighter height. False: no such clip.
struct SheetMask {
  int frame = -1, w = 0, h = 0;
  float ax = 0, ay = 0, pxPerH = 1;
  std::vector<unsigned char> opaque;
};
bool sheetMask(const CharacterDef& d, const Pose& pose, SheetMask& out);
// The same for frame `frame` of clip `sheet` (whatever the pose); frames: the clip's frame count (0: no clip).
bool sheetMaskAt(const CharacterDef& d, int sheet, int frame, SheetMask& out);
int sheetFrames(const CharacterDef& d, int sheet);
bool drawSheet(const CharacterDef& d, const Pose& pose, const Xf& X, float alpha, Color flashColor, float flash);
bool drawBust(Hero h, Vec2 center, float size, float alpha);
// Effects made with PixelLab (vfx_workbench or animated pictures, scripts/import_fx.py): a strip of frames
// assets/sprites/fx_<name>.png indexed by an "fx" line of sheets.txt. drawFx draws the effect `t` seconds after
// it started (looping or holding its last frame), its anchor at `at`, `size` world units per frame pixel, turned
// `rot` radians, mirrored when `flip`. False: no such effect (the caller draws its own).
struct FxInfo {
  int frames = 0;
  float fw = 0, fh = 0, ax = 0, ay = 0, fps = 15.0f;
  bool loop = false;
  float hit = 0.0f;  // the radius (half-thickness for a beam), in frame pixels, that its hitbox should have; 0: unknown
  float duration() const { return frames / fps; }
};
const FxInfo* fxInfo(const char* name);
bool drawFx(const char* name, float t, Vec2 at, float size, float rot = 0.0f, bool flip = false, float alpha = 1.0f,
            Color tint = WHITE);
bool drawFxFrame(const char* name, int frame, Vec2 at, float size, float rot = 0.0f, bool flip = false,
                 float alpha = 1.0f, Color tint = WHITE);
// A beam: the effect `name` (a horizontal segment that tiles, anchored on its left middle) repeated from `from` along
// `dir` over `len` world units, `size` world units per frame pixel, the last tile cut where the beam ends.
bool drawFxStrip(const char* name, float t, Vec2 from, Vec2 dir, float len, float size, float alpha = 1.0f);
// The same bust (not faded), cut to a box slanted like ui::slanted: the close-ups of the character select.
bool drawBustIn(Hero h, Vec2 center, float size, Rectangle box, float slant, float alpha);
bool drawDisc(Hero h, Vec2 center, float radius, float alpha);
}  // namespace art
