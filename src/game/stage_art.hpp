#pragma once
// The anime worlds (render_stages.cpp): two from each series, Bleach (Las Noches, Seireitei), Black Clover
// (the Black Bulls' hideout, the Grimoire Tower), Jujutsu Kaisen (Shibuya, Jujutsu High) and Naruto
// (Konoha, the Valley of the End). Same painterly look as the original three stages (render_world.cpp).
#include "core/stage.hpp"
#include "game/paint.hpp"

namespace stageart {
void bake(StageTheme theme, paint::Art& a);
// cam: parallax reference (the clamped camera target); (skyX, skyY): where the sky backdrop goes.
void drawBackground(StageTheme theme, const paint::Art& a, Vec2 cam, float skyX, float skyY, float time);
// One platform of an anime world, in world space; false for the original themes.
bool drawPlatform(const Platform& p, StageTheme theme, float time, int seed);
// Over the whole scene, in screen space (rain, drifting petals...): nothing for most worlds.
void drawForeground(StageTheme theme, Vec2 cam, float time);
}  // namespace stageart
