#pragma once
// Small immediate-mode UI kit: the visual identity of every menu.
#include <initializer_list>
#include <utility>

#include "game/common.hpp"

namespace ui {

constexpr Color kInk = {18, 14, 32, 255};
constexpr Color kGold = {255, 206, 64, 255};
constexpr Color kCream = {255, 246, 228, 255};
constexpr Color kMuted = {170, 172, 205, 255};

// ---- Shapes -------------------------------------------------------------------
// Quad with one colour per corner (a = top-left, b = top-right, c = bottom-right, d = bottom-left).
void quad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color ca, Color cb, Color cc, Color cd);
// Parallelogram leaning right ("/") inside the bounding box r, vertical gradient.
void slanted(Rectangle r, float slant, Color top, Color bottom);
void slantedOutline(Rectangle r, float slant, float thick, Color c);
Rectangle grow(Rectangle r, float px);
void glow(Vector2 c, float radius, Color col, int layers = 10);
void sunburst(Vector2 c, float radius, int rays, float rot, Color col);
void vignette(float strength);
void star(Vector2 c, float rOut, float rIn, float rot, Color col);

// ---- Backgrounds ----------------------------------------------------------------
// Animated menu background: gradient, moving diagonal bands, floating sparks.
void backdrop(float time, Color top, Color bottom, Color bands);

// ---- Text -----------------------------------------------------------------------
// Big extruded title text with a three-tone fill (Title font).
void logoText(const char* text, float x, float y, float size, Color light, Color mid, Color dark, Color outline,
              Color depth, int depthLayers = 8);
float logoWidth(const char* text, float size);
// Pre-rendered big titles (rendering an extruded text is expensive: ~90 draws).
enum class Sprite : int { Logo, Combat, Victory, Pause, Game, Count3, Count2, Count1, Go, Ready, Draw, Count };
void bakeSprites();    // after loadFonts(), outside of any BeginTextureMode
void unloadSprites();
// Draws the sprite like logoText would draw its text at (x, y) with the given size.
void drawSprite(Sprite s, float x, float y, float size, float alpha = 1.0f);
float spriteWidth(Sprite s, float size);
// Blend mode used for the whole frame (keeps the off-screen image opaque).
void frameBlend();
// Screen title on a slanted ribbon (top-left) + optional subtitle.
void header(const char* title, const char* subtitle, Color accent, float time);
// Key cap (e.g. [ENTRÉE]) - returns its width.
float keycap(const char* label, float x, float y, float h, bool lit = false);
// Bottom bar with control hints: {{"ENTRÉE", "Choisir"}, {"ÉCHAP", "Retour"}}.
void footer(std::initializer_list<std::pair<const char*, const char*>> hints, const char* info = nullptr);

// ---- Widgets ----------------------------------------------------------------------
void panel(Rectangle r, Color fill, Color border, float roundness = 0.08f);
void focusFrame(Rectangle r, float time, float roundness = 0.25f);
void slider(Rectangle r, float value01, bool focused, Color fill, float time);
void toggle(Rectangle r, bool on, bool focused, float time);
void selector(Rectangle r, const char* value, bool focused, float time);
void arrowTri(Vector2 c, float size, int dir, Color col);  // dir: -1 left, +1 right

// ---- Input helpers ----------------------------------------------------------------
bool hovered(Rectangle r);
bool clicked(Rectangle r);
bool mouseMoved();  // the pointer really moved this frame (not the initial jump)
void newFrame();    // call once per frame, before the screen updates

// ---- Screen transition (t in [0,1], fully covered at 0.5) -----------------------
void wipe(float t);

}  // namespace ui
