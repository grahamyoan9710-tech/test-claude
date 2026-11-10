#pragma once
// The five arenas painted with PixelLab (render_stages.cpp): a backdrop picture and platform pictures.
#include "core/stage.hpp"

namespace stageart {
// cam: parallax reference (the clamped camera target); (skyX, skyY): where the backdrop goes.
void drawBackground(StageTheme theme, Vec2 cam, float skyX, float skyY, float time);
// One platform of an arena, in world space.
void drawPlatform(const Platform& p, StageTheme theme, float time);
// Over the whole scene, in screen space: nothing for now.
void drawForeground(StageTheme theme, Vec2 cam, float time);
// The top floor of a platform's picture, seen in depth, in world units: its back edge, the walk line (the platform's
// top) and its front edge; how much it mirrors (0: matt) and whether rain ripples it; its left and right ends at the
// back and at the front edge (world x: the floor is a trapezoid, the picture's ends are rails, roof ridges...). False
// without a picture.
struct Floor {
  float back, walk, front, gloss;
  bool rain;
  float leftBack, rightBack, leftFront, rightFront;
};
bool floorOf(const Platform& p, StageTheme theme, Floor& out);
// How much the effects' light shows on the arena (1: as is): more at night than in daylight.
float lightGain(StageTheme theme);
}  // namespace stageart
