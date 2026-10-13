#pragma once
// Fighters made from an illustration instead of procedural shapes.
//
// scripts/rig_art.py cuts a picture of the fighter (full body, facing right) into parts — head, torso, the cloth
// of a skirt or a cape, and the limbs — packed in assets/fighters/<name>.png, and writes the joints marked on the
// picture and the measures of its limbs in assets/fighters/<name>.rig. In a fight the head, the torso and the
// cloth are the picture's own (rigid, never stretched); the arms and the legs are redrawn for each pose as lit
// volumes with the picture's widths, patterns and colours. All of it rides the same skeleton as the drawn
// fighters (render_body.cpp), with the picture's proportions. A picture without limb joints is only a portrait.
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
// Animations generated from the picture (SpriteCook, see scripts/import_sheet.py): frame by frame drawings that
// replace the skeleton for the poses they cover.
enum SheetAnim { kSheetIdle, kSheetAttack, kSheetCount };
bool hasSheet(Hero h, SheetAnim a);
bool drawSheet(const CharacterDef& d, const Pose& pose, const Xf& X, float alpha, Color flashColor, float flash);
bool drawBust(Hero h, Vec2 center, float size, float alpha);
// The same bust (not faded), cut to a box slanted like ui::slanted: the close-ups of the character select.
bool drawBustIn(Hero h, Vec2 center, float size, Rectangle box, float slant, float alpha);
bool drawDisc(Hero h, Vec2 center, float radius, float alpha);
}  // namespace art
