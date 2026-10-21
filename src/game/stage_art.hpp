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
}  // namespace stageart
