// Drawing a fighter: its PixelLab clip for the pose (fighter_art.cpp), or, while its clips are not imported yet,
// its still picture (assets/sprites/<id>_still.png, from the owner's sheet). Portraits come from
// <id>_portrait.png. Nothing of the fighter is drawn by code.
#include <algorithm>
#include <cmath>
#include <string>

#include "game/fighter_art.hpp"

namespace {
Xf xform(const CharacterDef& d, const Pose& pose, Vec2 feet, int facing, float scale) {
  Xf X;
  X.facing = facing;
  X.scale = scale;
  X.squash = pose.squash;
  X.sx = pose.sx;
  X.rot = pose.rot * facing;
  X.center = feet + Vec2{pose.shift.x * facing * scale, -d.height * 0.5f * scale * X.squash + pose.shift.y * scale};
  return X;
}

std::string pictureOf(const CharacterDef& d, const char* what) { return d.id + "_" + what; }

// The still picture standing on `feet`, `height` tall (its pixels are the body, head to feet).
bool drawStill(const CharacterDef& d, Vec2 feet, float height, int facing, float alpha, Color tint) {
  const std::string name = pictureOf(d, "still");
  float w = 0.0f, h = 0.0f;
  if (!art::pictureSize(name.c_str(), w, h) || h <= 0.0f) return false;
  const float k = height / h;
  const Rectangle dst{feet.x - w * k * 0.5f, feet.y - h * k, w * k, h * k};
  return art::drawPicture(name.c_str(), {0, 0, w, h}, dst, alpha, facing < 0, tint);
}
}  // namespace

void drawCharacter(const CharacterDef& d, int palette, const Pose& pose, Vec2 feet, int facing, float scale,
                   float alpha, Color flashColor, float flash, float time) {
  (void)palette;
  (void)time;
  // A clip is the whole animation: drawn flat on the feet, without the skeleton's bob, squash and spin on top.
  Pose flat = pose;
  flat.rot = 0.0f;
  flat.shift = {0.0f, 0.0f};
  flat.squash = flat.sx = 1.0f;
  if (pose.sheet >= 0 && art::hasSheet(d.hero, pose.sheet) &&
      art::drawSheet(d, pose, xform(d, flat, feet + pose.sheetOffset * scale, facing, scale), alpha, flashColor, flash))
    return;
  // No clip for this pose yet: the fighter's idle clip, else its still picture.
  if (art::hasSheet(d.hero, art::kSheetIdle)) {
    Pose idle = flat;
    idle.sheet = art::kSheetIdle;
    idle.sheetT = time;
    idle.sheetPhase = -1.0f;
    art::drawSheet(d, idle, xform(d, idle, feet, facing, scale), alpha, flashColor, flash);
    return;
  }
  const Color tint = flash > 0.01f ? mixColor(WHITE, flashColor, clampf(flash, 0.0f, 1.0f)) : WHITE;
  drawStill(d, feet + Vec2{pose.shift.x * facing * scale, pose.shift.y * scale}, d.height * scale * 1.08f, facing,
            pose.ghost ? alpha * 0.6f : alpha, tint);
}

void drawCharacterSprite(const CharacterDef& d, int palette, const Pose& pose, Vec2 feet, int facing, float scale,
                         float alpha, Color flashColor, float flash, float time) {
  drawCharacter(d, palette, pose, feet, facing, scale, alpha, flashColor, flash, time);
}

void drawShowcase(const CharacterDef& d, int palette, const Pose& pose, Vec2 feet, int facing, float height,
                  float time) {
  Pose p = pose;
  if (p.sheet < 0 && art::hasSheet(d.hero, art::kSheetIdle)) {  // its generated guard
    p.sheet = art::kSheetIdle;
    p.sheetT = time;
  }
  drawCharacter(d, palette, p, feet, facing, height * 1.1f / d.height, 1.0f, WHITE, 0.0f, time);
}

void drawPortrait(const CharacterDef& d, int palette, Vec2 center, float size, float time) {
  (void)palette;
  (void)time;
  const std::string name = pictureOf(d, "portrait");
  float w = 0.0f, h = 0.0f;
  if (art::pictureSize(name.c_str(), w, h) && w > 0.0f && h > 0.0f) {
    const float k = size / std::max(w, h);
    art::drawPicture(name.c_str(), {0, 0, w, h}, {center.x - w * k * 0.5f, center.y - h * k * 0.5f, w * k, h * k});
    return;
  }
  // No portrait: the top of the still picture.
  const std::string still = pictureOf(d, "still");
  if (art::pictureSize(still.c_str(), w, h) && w > 0.0f && h > 0.0f) {
    const float side = std::min(w, h * 0.32f), k = size / side;
    art::drawPicture(still.c_str(), {(w - side) * 0.5f, 0, side, side},
                     {center.x - size * 0.5f, center.y - size * 0.5f, side * k, side * k});
  }
}

void drawPortraitIn(const CharacterDef& d, int palette, Vec2 center, float size, Rectangle box, float slant,
                    float time) {
  beginClip({box.x + slant * 0.5f, box.y, box.width - slant, box.height});
  drawPortrait(d, palette, center, size, time);
  endClip();
}

void drawPortraitDisc(const CharacterDef& d, int palette, Vec2 center, float radius, float time) {
  const float side = radius * 1.41f;  // the square inside the disc
  beginClip({center.x - side * 0.5f, center.y - side * 0.5f, side, side});
  drawPortrait(d, palette, center, radius * 1.9f, time);
  endClip();
}

namespace {
// Where the face and the eyes are on each illustrated portrait, in fractions of its width and height.
struct ArtFace {
  const char* id;
  float fx, fy, eyeY;
};
constexpr ArtFace kArtFaces[] = {{"h01", 0.72f, 0.51f, 0.435f}};

const ArtFace* artFace(const CharacterDef& d, float& w, float& h) {
  for (const ArtFace& a : kArtFaces)
    if (d.id == a.id && art::pictureSize((d.id + "_art").c_str(), w, h)) return &a;
  return nullptr;
}
}  // namespace

bool hasArt(const CharacterDef& d) {
  float w, h;
  return artFace(d, w, h) != nullptr;
}

bool artGeometry(const CharacterDef& d, float& w, float& h, Vec2& face, bool eyes) {
  const ArtFace* a = artFace(d, w, h);
  if (a) face = {a->fx * w, (eyes ? a->eyeY : a->fy) * h};
  return a != nullptr;
}

bool drawArtBust(const CharacterDef& d, float cx, float bottom, float height, Color tint) {
  float w, h;
  if (!artFace(d, w, h)) return false;
  const float k = height / h;
  return art::drawPicture((d.id + "_art").c_str(), {0, 0, w, h}, {cx - w * k * 0.5f, bottom - height, w * k, height},
                          1.0f, false, tint);
}

bool drawArtPoly(const CharacterDef& d, const Vector2* pts, int n, Vec2 at, float scale, bool eyes, Color tint) {
  float w, h;
  Vec2 face;
  if (n > 48 || !artGeometry(d, w, h, face, eyes)) return false;
  Vector2 src[48];
  for (int i = 0; i < n; ++i) src[i] = V(face + (W(pts[i]) - at) / scale);
  return art::drawPicturePoly((d.id + "_art").c_str(), src, pts, n, tint);
}

bool drawArtBox(const CharacterDef& d, Rectangle b, float span, Color tint) {
  float w, h;
  Vec2 face;
  if (!artGeometry(d, w, h, face)) return false;
  const float k = b.width / (span * w);
  const Vector2 pts[4] = {{b.x, b.y}, {b.x + b.width, b.y}, {b.x + b.width, b.y + b.height}, {b.x, b.y + b.height}};
  return drawArtPoly(d, pts, 4, {b.x + b.width * 0.5f, b.y + b.height * 0.5f + 0.08f * span * w * k}, k, false, tint);
}

void drawArtDisc(const CharacterDef& d, int palette, Vec2 center, float radius, float time, float span, Color tint) {
  float w, h;
  if (!artFace(d, w, h)) {
    drawPortraitDisc(d, palette, center, radius, time);
    return;
  }
  constexpr int kSides = 40;
  Vector2 pts[kSides];
  for (int i = 0; i < kSides; ++i) {
    const float a = i * 2.0f * kPi / kSides;
    pts[i] = {center.x + std::cos(a) * radius, center.y + std::sin(a) * radius};
  }
  const float k = 2.0f * radius / (span * w);  // the face a little low in the disc: its hair shows above it
  drawArtPoly(d, pts, kSides, center + Vec2{0.0f, 0.08f * span * w * k}, k, false, tint);
}

// Where a limb is, from the pose's targets (the clips draw the body; effects only need an approximate hand).
Vec2 limbWorld(const CharacterDef& d, const Pose& pose, Vec2 feet, int facing, float scale, Limb limb, bool weaponTip) {
  const Xf X = xform(d, pose, feet, facing, scale);
  const float H = d.height;
  Vec2 l;
  switch (limb) {
    case Limb::ArmBack: l = pose.handB; break;
    case Limb::LegFront: l = pose.footF; break;
    case Limb::LegBack: l = pose.footB; break;
    case Limb::Head: l = {0.0f, -0.38f * H}; break;
    case Limb::Body: case Limb::None: l = {0.0f, 0.0f}; break;
    default: l = pose.handF; break;
  }
  if (weaponTip) l = pose.handF + pose.weaponDir.norm() * std::max(pose.bladeLen, 0.0f);
  return W(X(l));
}
