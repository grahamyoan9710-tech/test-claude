#pragma once
// The five arenas painted with PixelLab (render_stages.cpp): a backdrop picture and platform pictures.
#include "core/stage.hpp"
#include "game/paint.hpp"

namespace stageart {
void bake(StageTheme theme, paint::Art& a);
// cam: parallax reference (the clamped camera target); (skyX, skyY): where the sky backdrop goes.
void drawBackground(StageTheme theme, const paint::Art& a, Vec2 cam, float skyX, float skyY, float time);
// One platform of an arena, in world space; false for the older themes (render_world.cpp).
bool drawPlatform(const Platform& p, StageTheme theme, float time, int seed);
// Over the whole scene, in screen space: nothing for now.
void drawForeground(StageTheme theme, Vec2 cam, float time);
}  // namespace stageart
