#pragma once
// Small immediate-mode UI kit: the visual identity of every menu (Clash Horizon: the logo's royal blue and gold,
// chunky rounded buttons with a thick dark outline, a lip and a gloss, deep navy cards, gold ribbons).
#include <initializer_list>
#include <utility>

#include "game/common.hpp"
#include "game/controls.hpp"

namespace ui {

constexpr Color kInk = {10, 14, 36, 255};        // outlines, the deepest navy
constexpr Color kNavy = {30, 54, 118, 255};      // cards (top)
constexpr Color kNavyDeep = {13, 25, 68, 255};   // cards (bottom)
constexpr Color kSlate = kNavy;
constexpr Color kBlue = {72, 156, 255, 255};     // secondary buttons
constexpr Color kSky = {160, 218, 255, 255};     // light trims and highlights
constexpr Color kStone = {104, 124, 172, 255};   // idle lines
constexpr Color kGold = {255, 210, 74, 255};     // primary buttons, focus
constexpr Color kGoldDeep = {238, 140, 24, 255};
constexpr Color kCream = {255, 250, 238, 255};   // text
constexpr Color kMuted = {170, 190, 230, 255};   // secondary text
constexpr Color kUmber = kInk;                   // outline of the big titles
constexpr Color kTeal = kSky;                    // light accents
constexpr Color kRed = {246, 72, 80, 255};       // cancel, danger
constexpr Color kRedDark = {168, 28, 46, 255};
constexpr Color kGreen = {126, 226, 74, 255};    // go

// The tones of the chunky buttons.
enum class Tone : int { Gold, Blue, Navy, Red, Green, Grey };
Color toneTop(Tone t);
Color toneBottom(Tone t);

// ---- Shapes -------------------------------------------------------------------
// Quad with one colour per corner (a = top-left, b = top-right, c = bottom-right, d = bottom-left).
void quad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color ca, Color cb, Color cc, Color cd);
// Parallelogram leaning right ("/") inside the bounding box r, vertical gradient.
void slanted(Rectangle r, float slant, Color top, Color bottom);
void slantedOutline(Rectangle r, float slant, float thick, Color c);
// Rounded rectangle with a vertical gradient (radius in pixels), and its outline.
void roundRect(Rectangle r, float radius, Color top, Color bottom);
void roundLines(Rectangle r, float radius, float thick, Color c);
// The chunky look: drop shadow, thick dark outline, a darker lip under the face, a gloss on top. radius < 0: from
// the height.
void chunky(Rectangle r, Color top, Color bottom, float alpha = 1.0f, float radius = -1.0f);
void chunky(Rectangle r, Tone tone, float alpha = 1.0f, float radius = -1.0f);
float chunkyLip(Rectangle r);  // the lip's height: the face is r without it
// A button: its chunky body, the label in white; focused: lifted, a gold rim, a shine running over it.
void button(Rectangle r, const char* label, Tone tone, bool focused, float time, float alpha = 1.0f, float size = 0.0f);
// Navy card: panels and sheets.
void card(Rectangle r, float alpha = 1.0f, Color top = kNavy, Color bottom = kNavyDeep);
// Ribbon banner with folded tails, centred on cx, its text on it; returns its width.
float ribbon(const char* text, float cx, float y, float size, Tone tone = Tone::Gold, float alpha = 1.0f);
float ribbonWidth(const char* text, float size);
// The gold rim of what is focused.
void rim(Rectangle r, float radius, float time);
// Big type: white (or fill) with a thick dark outline and a dark drop under it (title font).
void bold(const char* text, float x, float y, float size, Color fill = WHITE, float alpha = 1.0f);
void boldCentered(const char* text, float cx, float y, float size, Color fill = WHITE, float alpha = 1.0f);
// The main menu's plate (the old look's name): a chunky button, gold when chosen, navy otherwise.
void menuPlate(Rectangle r, float slant, bool chosen, float alpha = 1.0f);
Rectangle grow(Rectangle r, float px);
void glow(Vector2 c, float radius, Color col, int layers = 10);
void sunburst(Vector2 c, float radius, int rays, float rot, Color col);
void vignette(float strength);
void star(Vector2 c, float rOut, float rIn, float rot, Color col);
void diamond(Vector2 c, float r, Color col);
// Short L brackets on the corners of a slanted box (slant 0 = rectangle).
void corners(Rectangle r, float slant, float len, float thick, Color col);
// A card in the given colours (the slant is ignored: everything is rounded now).
void plate(Rectangle r, float slant, Color top, Color bottom, Color line);
// Hairline rule with a small dot at both ends.
void rule(float x0, float x1, float y, Color col);
// Zonai rune glyph (kind 0 spiral, 1 eye, 2 three dots, 3 compass) inside a circle of radius r.
void rune(Vector2 c, float r, int kind, float rot, Color col, float thick = 2.0f);

// ---- Backgrounds ----------------------------------------------------------------
// Painterly sky: gradient, soft cloud banks, drifting motes (tinted by `motes`).
void backdrop(float time, Color top, Color bottom, Color motes);
// A real stage background, slowly panning, under a navy veil (dim 0..1).
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
