#pragma once
// The fighters' pictures: their PixelLab clips (assets/sprites/<id>_<clip>.png, the "sheet" lines of sheets.txt),
// their still pictures (portrait, placeholder body, weapon icons) and the PixelLab effects ("fx" lines).
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

namespace art {
// The generated animations of a fighter, by state and by move.
enum SheetAnim : int {
  kSheetIdle, kSheetWalk, kSheetRun, kSheetCrouch, kSheetJumpSquat, kSheetJump, kSheetAirJump, kSheetFall,
  kSheetLand, kSheetShield, kSheetRoll, kSheetSpotDodge, kSheetAirDodge, kSheetHurt, kSheetTumble, kSheetKnockdown,
  kSheetGetUp, kSheetHelpless, kSheetLedge, kSheetClimb, kSheetHold, kSheetGrabbed, kSheetDizzy, kSheetVictory,
  kSheetDash,
  kSheetMove0,                               // + MoveId: one clip per move
  kSheetWeapon0 = kSheetMove0 + MOVE_COUNT,   // + MoveId: the move done with weapon slot 0
  kSheetWeapon1 = kSheetWeapon0 + MOVE_COUNT,  // ... with weapon slot 1
  kSheetWState0 = kSheetWeapon1 + MOVE_COUNT,  // + state: the state while holding weapon slot 0
  kSheetWState1 = kSheetWState0 + kSheetMove0,  // ... weapon slot 1
  kSheetCount = kSheetWState1 + kSheetMove0
};
const char* sheetName(int anim);  // "idle", "jab", "w0_jab"...
bool embeddedSprite(const char* name, const unsigned char** data, unsigned* size);
const std::string& spriteIndex();  // assets/sprites/sheets.txt
bool hasSheet(Hero h, int anim);
bool hasClips(Hero h);  // the fighter has at least one generated clip
float sheetDrop(const CharacterDef& d, int anim);
struct SheetMask {
  int frame = -1, w = 0, h = 0;
  float ax = 0, ay = 0, pxPerH = 1;
  std::vector<unsigned char> opaque;
};
bool sheetMask(const CharacterDef& d, const Pose& pose, SheetMask& out);
bool sheetMaskAt(const CharacterDef& d, int sheet, int frame, SheetMask& out);
int sheetFrames(const CharacterDef& d, int sheet);
bool drawSheet(const CharacterDef& d, const Pose& pose, const Xf& X, float alpha, Color flashColor, float flash);

// A still picture of assets/sprites (portraits "<id>_portrait", placeholder bodies "<id>_still", weapon icons
// "<id>_icon_a"...): its size in pixels, false if there is none.
bool pictureSize(const char* name, float& w, float& h);
// Draws `src` (pixels of the picture) into `dst`; flipX mirrors it.
bool drawPicture(const char* name, Rectangle src, Rectangle dst, float alpha = 1.0f, bool flipX = false,
                 Color tint = WHITE);

struct FxInfo {
  int frames = 0;
  float fw = 0, fh = 0, ax = 0, ay = 0, fps = 15.0f;
  bool loop = false;
  float hit = 0.0f;  // the radius (half-thickness for a beam), in frame pixels, that its hitbox should have; 0: unknown
  float duration() const { return frames / fps; }
};
const FxInfo* fxInfo(const char* name);
// Time of an effect shown for as long as something lasts (projectile, beam, aura): its frames cycle.
float fxLoopTime(const char* name, float t);
bool drawFx(const char* name, float t, Vec2 at, float size, float rot = 0.0f, bool flip = false, float alpha = 1.0f,
            Color tint = WHITE);
bool drawFxFrame(const char* name, int frame, Vec2 at, float size, float rot = 0.0f, bool flip = false,
                 float alpha = 1.0f, Color tint = WHITE);
bool drawFxStrip(const char* name, float t, Vec2 from, Vec2 dir, float len, float size, float alpha = 1.0f,
                 float fadeIn = 0.0f);
}  // namespace art
