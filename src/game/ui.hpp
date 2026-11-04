#pragma once
// Small immediate-mode UI kit: the visual identity of every menu (Clash Horizon, « Ligne d'horizon »: flat night
// blue panels with bevelled corners, azure for the idle lines and player 1, gold for what is chosen and player 2, the
// horizon line azure -> white -> gold, Saira Condensed type).
#include <initializer_list>
#include <string>
#include <utility>

#include "game/common.hpp"
#include "game/controls.hpp"

namespace ui {

constexpr Color kInk = {6, 10, 24, 255};         // « Nuit »: the deepest blue, outlines, text on gold
constexpr Color kNavy = {20, 29, 64, 255};       // « Panneau »: cards (top)
constexpr Color kNavyDeep = {13, 20, 48, 255};   // « Abysse »: cards (bottom)
constexpr Color kSlate = kNavy;
constexpr Color kBlue = {61, 139, 255, 255};     // « Azur »: idle lines, player 1
constexpr Color kSky = {191, 217, 255, 255};     // light trims and small labels
constexpr Color kStone = {58, 76, 128, 255};     // idle marks
constexpr Color kGold = {255, 181, 71, 255};     // « Or »: what is chosen, player 2
constexpr Color kGoldDeep = {222, 132, 30, 255};
constexpr Color kCream = {238, 243, 255, 255};   // « Éclat »: text
constexpr Color kSoft = {212, 222, 245, 255};    // body text
constexpr Color kMuted = {154, 168, 199, 255};   // « Brume »: secondary text
constexpr Color kDisabled = {130, 144, 179, 255};  // what is not open yet
constexpr Color kTrack = {26, 37, 80, 255};      // slider tracks, idle segments
constexpr Color kUmber = kInk;                   // outline of the big titles
constexpr Color kTeal = kSky;                    // light accents
constexpr Color kRed = {255, 96, 82, 255};       // cancel, danger
constexpr Color kRedDark = {168, 34, 44, 255};
constexpr Color kGreen = {61, 220, 151, 255};    // go

// The tones of the chunky buttons.
enum class Tone : int { Gold, Blue, Navy, Red, Green, Grey };
Color toneTop(Tone t);
Color toneBottom(Tone t);
Color toneInk(Tone t);  // the text that reads on it

// ---- Shapes -------------------------------------------------------------------
// Quad with one colour per corner (a = top-left, b = top-right, c = bottom-right, d = bottom-left).
void quad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color ca, Color cb, Color cc, Color cd);
// Parallelogram leaning right ("/") inside the bounding box r, vertical gradient.
void slanted(Rectangle r, float slant, Color top, Color bottom);
void slantedOutline(Rectangle r, float slant, float thick, Color c);
// A box with its top-left and bottom-right corners cut (the charte's shape), vertical gradient; and its outline.
void bevel(Rectangle r, float cut, Color top, Color bottom);
void bevelLines(Rectangle r, float cut, float thick, Color c);
// The horizon line: azure -> white -> gold, fading at both ends.
void horizonLine(float x0, float x1, float y, float thick, float alpha = 1.0f);
// The white diamond spark of the logo, with its glow.
void eclat(Vector2 c, float r, float alpha = 1.0f, Color col = WHITE);
// Rounded rectangle with a vertical gradient (radius in pixels), and its outline.
void roundRect(Rectangle r, float radius, Color top, Color bottom);
void roundLines(Rectangle r, float radius, float thick, Color c);
// A flat bevelled plate (the old chunky look's name); with a Tone, the idle ones have an azure line underneath.
// radius: the cut of the corners (< 0: from the height).
void chunky(Rectangle r, Color top, Color bottom, float alpha = 1.0f, float radius = -1.0f);
void chunky(Rectangle r, Tone tone, float alpha = 1.0f, float radius = -1.0f);
float chunkyLip(Rectangle r);  // 0 now: the face is the whole plate
// The chosen button's shimmer: a soft white band crossing r every 2.6 s.
void shimmer(Rectangle r, float time, float alpha = 1.0f);
// A button: its plate, the label in the tone's ink; focused: a white rim and a shimmer running over it.
void button(Rectangle r, const char* label, Tone tone, bool focused, float time, float alpha = 1.0f, float size = 0.0f);
// Night blue card with a faint inner line: panels and sheets.
void card(Rectangle r, float alpha = 1.0f, Color top = kNavy, Color bottom = kNavyDeep);
// A bevelled plate centred on cx, its text on it; returns its width.
float ribbon(const char* text, float cx, float y, float size, Tone tone = Tone::Gold, float alpha = 1.0f);
float ribbonWidth(const char* text, float size);
// The white rim of what is focused.
void rim(Rectangle r, float radius, float time);
// Big type: white (or fill) with a thin dark outline and a soft drop under it (title font).
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
// The logo's spark, a four-pointed star: its tips at r, the corners of its waist at (±r·waist, ±r·waist); turned by rot.
void spark(Vector2 c, float r, float rot, Color col, float waist = 0.2f);
// Short L brackets on the corners of a slanted box (slant 0 = rectangle).
void corners(Rectangle r, float slant, float len, float thick, Color col);
// A card in the given colours (the slant is ignored: everything is rounded now).
void plate(Rectangle r, float slant, Color top, Color bottom, Color line);
// Hairline rule with a small diamond at both ends.
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
// Screen title, big at the top left, the horizon line under it, an optional subtitle.
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
// Bottom bar with control hints: {{"ENTRÉE", Btn::A, "Choisir"}, {"ÉCHAP", Btn::B, "Retour"}}; `go`: the gold call to
// action at its right end (e.g. {"ENTRÉE", Btn::Start, "COMBATTRE !"}), dimmed when not `goOn`.
void footer(std::initializer_list<Hint> hints, const char* info = nullptr, const Hint* go = nullptr, bool goOn = true);
Rectangle goRect(const Hint& go);  // where the footer puts it (mouse clicks)

// Small capitals spaced out like the charte's labels (CSS letter-spacing, in em), title font; returns the width.
float tracked(const char* text, float x, float y, float size, float em, Color col);
float trackedWidth(const char* text, float size, float em);
// Draws `text` wrapped to `width`, line after line; returns the y under the last line (draw = false: only measures).
float drawWrapped(const std::string& text, float x, float y, float width, float size, Color col, bool draw = true);
// Title text with a horizontal gradient, left -> middle -> right (the "VS").
void gradientText(const char* text, float x, float y, float size, Color left, Color mid, Color right);

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
