#pragma once
// Pixel-art characters: a fighter is drawn into a low-resolution layer (one art pixel = artPixel() virtual
// pixels), then a shader gives it the look of a hand-made sprite (crisp dark outline, flat colour bands,
// no anti-aliasing) and it is put back into the frame with nearest-neighbour scaling. The stages, the
// effects and the menus stay smooth: only the characters are pixel art.
#include <functional>

#include "game/common.hpp"

namespace pix {
// Virtual pixels per art pixel: 1.5 with the smooth (supersampled) rendering, so that one art pixel is exactly 3x3
// pixels of the frame (1280x720 -> an 854x480 sprite layer: detailed sprites, still pixel art); 2 otherwise.
float artPixel();

bool enabled();  // settings: "Personnages en pixel art"
// The frame's render texture (set by the app before drawing a frame): sprites draw back into it.
void setFrameTarget(const RenderTexture2D* target);
// Draws `draw` (in the current coordinate space: world with a camera, or screen) as a pixel-art sprite.
// `bounds` (same space) must contain everything it draws. `draw` gets the alpha to draw with: 1 in pixel
// art (the whole sprite is faded by `alpha` afterwards), `alpha` itself in smooth HD.
void sprite(Rect bounds, float alpha, const std::function<void(float)>& draw);
// Inside a sprite's draw: the size of one art pixel in drawing units (0 outside a pixel-art sprite).
// The character renderer sizes its ink lines with it.
float artUnit();
constexpr Color kInk = {18, 13, 20, 255};  // sprite outline
void unload();
}  // namespace pix
