#pragma once
// Small immediate-mode UI kit: the visual identity of every menu (TotK-like: slate ink panels,
// gold hairlines, parchment text, Zonai teal glow).
#include <initializer_list>
#include <utility>

#include "game/common.hpp"
#include "game/controls.hpp"

namespace ui {

constexpr Color kInk = {14, 22, 26, 255};       // deep slate / teal ink
constexpr Color kSlate = {26, 40, 44, 255};     // panel fill
constexpr Color kStone = {112, 126, 120, 255};  // idle frames, quiet lines
constexpr Color kGold = {232, 196, 110, 255};   // hairlines, focus, highlights
constexpr Color kCream = {246, 236, 212, 255};  // parchment text
constexpr Color kMuted = {158, 176, 168, 255};  // secondary text
constexpr Color kUmber = {58, 34, 20, 255};     // warm depth of the big titles
constexpr Color kTeal = kZonaiGlow;             // Zonai glow: selection accents

// ---- Shapes -------------------------------------------------------------------
// Quad with one colour per corner (a = top-left, b = top-right, c = bottom-right, d = bottom-left).
void quad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color ca, Color cb, Color cc, Color cd);
// Parallelogram leaning right ("/") inside the bounding box r, vertical gradient.
void slanted(Rectangle r, float slant, Color top, Color bottom);
void slantedOutline(Rectangle r, float slant, float thick, Color c);
// The main menu's look (docs/refonte/menu_accueil.png): crimson for the chosen item, dark ink for the others.
constexpr Color kRed = {236, 40, 74, 255};
constexpr Color kRedDark = {164, 14, 48, 255};
// One of its slanted plates: drop shadow, crimson (chosen) or dark fill, white or grey outline.
void menuPlate(Rectangle r, float slant, bool chosen, float alpha = 1.0f);
Rectangle grow(Rectangle r, float px);
void glow(Vector2 c, float radius, Color col, int layers = 10);
void sunburst(Vector2 c, float radius, int rays, float rot, Color col);
void vignette(float strength);
void star(Vector2 c, float rOut, float rIn, float rot, Color col);
void diamond(Vector2 c, float r, Color col);
// Short L brackets on the corners of a slanted box (slant 0 = rectangle).
void corners(Rectangle r, float slant, float len, float thick, Color col);
// Slanted slate plate: drop shadow, fill, gold hairline and corner ornaments.
void plate(Rectangle r, float slant, Color top, Color bottom, Color line);
// Hairline rule with small diamonds at both ends.
void rule(float x0, float x1, float y, Color col);
// Zonai rune glyph (kind 0 spiral, 1 eye, 2 three dots, 3 compass) inside a circle of radius r.
void rune(Vector2 c, float r, int kind, float rot, Color col, float thick = 2.0f);
// Rotating Zonai ring emblem with rune marks and a teal glow.
void zonaiRing(Vector2 c, float r, float time, float alpha = 1.0f);
// Ancient stone disc (fighters stand on it): mossy rim, glowing runes, teal under-glow.
void stoneDisc(Vector2 c, float rx, float ry, float time, bool rock = true);

// ---- Backgrounds ----------------------------------------------------------------
// Painterly sky: gradient, soft cloud banks, drifting motes (tinted by `motes`).
void backdrop(float time, Color top, Color bottom, Color motes);
// A real stage background, slowly panning, under a teal-ink veil (dim 0..1) and drifting Zonai motes.
void stageBackdrop(int stage, float time, float dim);
// Drifting glowing motes rising across the screen.
void motes(int count, float time, Color col, float alpha = 1.0f);

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
float keycapWidth(const char* label, float h);

// ---- Button prompts ---------------------------------------------------------------
// Gamepad buttons drawn like on a controller of the family (procedural: crisp at any size).
// Face buttons by position: A = bottom, B = right, X = left, Y = top (a Nintendo "B" is A here).
enum class Btn : int { None, A, B, X, Y, LB, RB, LT, RT, Start, LStick, RStick, DPad, DPadUD, DPadLR, DPadDown, Move, Back, L3, R3 };
float padGlyph(Btn b, float x, float y, float h, PadFamily family);  // returns its width
float padGlyphWidth(Btn b, float h, PadFamily family);
// Key cap or gamepad button, following the prompt mode (last used device). Returns its width.
float prompt(const char* key, Btn pad, float x, float y, float h, bool lit = false);
float promptWidth(const char* key, Btn pad, float h);
// Glyph of a PadButton (controls.hpp; -1 -> None).
Btn btnFor(int padButton);
// Button of a KeyAction on the gamepad, following the bindings (kActUltimate -> RB by default...).
Btn padButtonFor(int action);
// The same for one device (a pad without triggers may use other buttons).
Btn deviceButtonFor(int device, int action);
// Prompt of one player's device, whatever the prompt mode: the key cap of the key bound to the action
// on CLAVIER 1/2, or the button of that gamepad (its own family). E.g. the ULTIME button on the HUD:
// actionPrompt(devices[port], kActUltimate, x, y, 24). Returns its width.
float actionPrompt(int device, int action, float x, float y, float h, bool lit = false);
float actionPromptWidth(int device, int action, float h);
// One hint of the bottom bar: keyboard key, gamepad button, what it does.
struct Hint {
  const char* key;
  Btn pad;
  const char* label;
};
// Bottom bar with control hints: {{"ENTRÉE", Btn::A, "Choisir"}, {"ÉCHAP", Btn::B, "Retour"}}.
void footer(std::initializer_list<Hint> hints, const char* info = nullptr);

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
