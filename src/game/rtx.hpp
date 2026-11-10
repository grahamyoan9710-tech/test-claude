#pragma once
// The real "RTX" (setting "RTX : Oui"): the light is ray traced in 2D on the GPU. The match draws the scene a second
// time: what blocks the light (the fighters, the platforms' bodies) and what gives it (the ParticleFX effects, and the
// lanterns of the night arenas, read off their pictures). A distance field (jump flooding) lets 16 rays per pixel
// march through it: the light stops at the fighters (real shadows), bounces off what it hits (the frame before's
// light, so more over time), and is added to the frame, in flat bands on the pixel-art sprites. "RTX : Non" keeps
// the drawn light of render_light.cpp.
#include <functional>

#include "game/common.hpp"

namespace rtx {
bool on();  // between beginFrame and resolve, with the setting on: the hooks below feed the ray tracer
void beginFrame();
// Draws `draw` (current transform) again into the traced scene: as something that blocks the light, or as light
// (`core`: the effect's own pixels, without its halo, kept as drawn).
void solid(const std::function<void()>& draw);
void light(const std::function<void()>& draw, const std::function<void()>& core);
void platforms(const Match& m);  // after the platforms: their bodies block the light, their floors catch it
void resolve(const Match& m, const Camera2D& cam);  // before the reflections: traces the light, adds it to the frame
}  // namespace rtx
