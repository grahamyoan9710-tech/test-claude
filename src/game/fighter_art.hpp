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
int sheetLastAir(const CharacterDef& d, int sheet);  // -1: none (see Pose::sheetAir)
bool drawSheet(const CharacterDef& d, const Pose& pose, const Xf& X, float alpha, Color flashColor, float flash);

// « Prestige »: the menus show the fighters in their own pixel art, crisp. A clip's frame count, frame size, rate and
// whether it loops (false: the fighter has no such clip); frame `frame` of it drawn with every pixel `scale` px
// square, the frame's top-left at (x, y), flip mirroring it in its box.
bool clipInfo(const CharacterDef& d, int anim, int& frames, float& fw, float& fh, float& fps);
bool drawClipPx(const CharacterDef& d, int anim, int frame, float x, float y, float scale, bool flip = false,
                float alpha = 1.0f, Color tint = WHITE);
// A still picture of assets/sprites (portraits "<id>_portrait", placeholder bodies "<id>_still", weapon icons
// "<id>_icon_a"...): its size in pixels, false if there is none.
bool pictureSize(const char* name, float& w, float& h);
// Draws `src` (pixels of the picture) into `dst`; flipX mirrors it.
bool drawPicture(const char* name, Rectangle src, Rectangle dst, float alpha = 1.0f, bool flipX = false,
                 Color tint = WHITE);
// Draws the whole picture into dst, fading out over its left `fadeL` and bottom `fadeB` (fractions of dst; a negative
// fadeL fades its right side instead; fadeT its top): pictures melting into the screen.
bool drawPictureFaded(const char* name, Rectangle dst, float fadeL, float fadeB, float alpha = 1.0f, bool flipX = false,
                      float fadeT = 0.0f);
// Draws the convex polygon `dst` (n screen points) textured with the picture's points `src` (pixels): slanted tiles,
// discs, rotated strips.
bool drawPicturePoly(const char* name, const Vector2* src, const Vector2* dst, int n, Color tint = WHITE);

struct FxInfo {
  int frames = 0;
  float fw = 0, fh = 0, ax = 0, ay = 0, fps = 15.0f;
  bool loop = false;
  float hit = 0.0f;  // the radius (half-thickness for a beam), in frame pixels, that its hitbox should have; 0: unknown
  bool pixel = false;  // pixel art made at its final size (import_fx.py --pixel): drawn with the fighters' pixels
  float duration() const { return frames / fps; }
};
const FxInfo* fxInfo(const char* name);
// World units per pixel of the fighter's clips: a pixel-art effect drawn at this size has the same pixels.
float pixelScale(const CharacterDef& d);
// The size to draw effect `name` at on fighter d: a pixel-art one at d's pixel size (the movement puffs, the launch
// trail, the KO blast and Hakuen's ultimate bigger), the others `size` times d's height over 170.
float fxSize(const char* name, const CharacterDef& d, float size = 1.0f);
// "<name>_front", the part of a technique's effect drawn over the fighters (the cuts of an ultimate), if it has one.
const char* fxFront(const char* name);
// Time of an effect shown for as long as something lasts (projectile, beam, aura): its frames cycle.
float fxLoopTime(const char* name, float t);
// castLight false: a decoration on a fighter (its ultimate's sparkles, the stars of a daze) does not light him.
bool drawFx(const char* name, float t, Vec2 at, float size, float rot = 0.0f, bool flip = false, float alpha = 1.0f,
            Color tint = WHITE, bool castLight = true);
// Starts a frame: the light the ParticleFX effects threw two frames ago no longer falls on the fighters.
void newFrame();
unsigned frameId();  // counts the frames newFrame started
// The light of a ParticleFX effect (world units): where its bright pixels are, how far they spread, its colour and
// strength (its frame's, faded with it).
struct Light {
  Vec2 at;
  float r;
  Color c;
  float k;
};
const std::vector<Light>& lastLights();  // the effects of the frame before, all drawn
bool drawFxFrame(const char* name, int frame, Vec2 at, float size, float rot = 0.0f, bool flip = false,
                 float alpha = 1.0f, Color tint = WHITE, bool castLight = true);
bool drawFxStrip(const char* name, float t, Vec2 from, Vec2 dir, float len, float size, float alpha = 1.0f,
                 float fadeIn = 0.0f);
// Everything the game draws, loaded one item at a time (the launch screen shows the progress): 0 a fighter's clips,
// 1 an effect, 2 a picture.
int preloadCount();
int preloadKind(int i);
void preload(int i);
}  // namespace art
