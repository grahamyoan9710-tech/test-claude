#pragma once
// Small immediate-mode UI kit: the visual identity of every menu (Clash Horizon, « Épure », palette « Crépuscule »:
// a night-to-dusk gradient, flat indigo panels with round corners, cream for text and for what is chosen, one cyan
// accent; Archivo Expanded titles, Inter text).
#include <functional>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include "game/common.hpp"
#include "game/controls.hpp"

namespace ui {

constexpr Color kInk = {23, 23, 58, 255};        // the deepest indigo: outlines, text on cream
constexpr Color kNavy = {42, 39, 88, 255};       // panels (top)
constexpr Color kNavyDeep = {33, 31, 72, 255};   // panels (bottom)
constexpr Color kSlate = kNavy;
constexpr Color kBlue = {111, 227, 255, 255};    // the cyan accent: lines, sparks
constexpr Color kSky = {218, 211, 234, 255};     // light trims and small labels
constexpr Color kStone = {91, 86, 128, 255};     // idle marks
constexpr Color kGold = {255, 243, 230, 255};    // what is chosen: cream
constexpr Color kGoldDeep = {218, 211, 234, 255};
constexpr Color kCream = {255, 243, 230, 255};   // text
constexpr Color kSoft = {218, 211, 234, 255};    // body text
constexpr Color kMuted = {171, 166, 204, 255};   // secondary text
constexpr Color kDisabled = {138, 134, 174, 255};  // what is not open yet
constexpr Color kTrack = {52, 49, 98, 255};      // slider tracks, idle segments
constexpr Color kUmber = kInk;                   // outline of the big titles
constexpr Color kTeal = kBlue;                   // light accents
constexpr Color kRed = {255, 96, 82, 255};       // cancel, danger
constexpr Color kRedDark = {168, 34, 44, 255};
constexpr Color kGreen = {61, 220, 151, 255};    // go
// The « Crépuscule » gradient (top, middle at 55 %, bottom) and its flat deep tone.
constexpr Color kDuskTop = {20, 20, 58, 255}, kDuskMid = {35, 30, 77, 255}, kDuskLow = {59, 37, 86, 255};
constexpr Color kDeep = {23, 23, 58, 255};
constexpr Color kHair = {255, 243, 230, 36};     // hairlines (cream at 14 %)

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
void bevel(Rectangle r, float cut, Color top, Color bottom, bool across = false);  // across: top = left
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
// The « Crépuscule » gradient over the whole screen (a: its opacity).
void dusk(float a = 1.0f);
// The menus' ground: the « Crépuscule » gradient (the colours are ignored now).
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

// « Épure » labels: Inter capitals spaced out (em: CSS letter-spacing), their caps centred on cy; returns the width.
float label(const char* text, float x, float cy, float size, float em, Color col);
float labelWidth(const char* text, float size, float em);
// Line icons of the charte (24-unit grid, round caps), centred on c, `size` px wide.
enum class Icon : int { Play, Versus, Training, Online, Fighters, Arenas, Options, Quit, Sliders, Dice, Sword, Trophy,
                        Rematch, Home, Close, ChevronL, ChevronR, Arrow, Person };
void icon(Icon i, Vector2 c, float size, Color col, float stroke = 1.75f);

// Small capitals spaced out like the charte's labels (CSS letter-spacing, in em), title font; returns the width.
float tracked(const char* text, float x, float y, float size, float em, Color col);
float trackedWidth(const char* text, float size, float em);
// `text` cut at its spaces into lines at most `width` px wide (at `size`, in font k).
std::vector<std::string> wrapText(const std::string& text, float width, float size, FontKind k);
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

// ---- « Prestige » (12/11): each screen a match poster ----------------------------------
// The fighter's colour in a flat flood cut by one blade leaning 12°, his name huge, white, on a night plate; his own
// pixel art, crisp; the red / blue split only on 3-2-1-GO, GAME and the % hit; strokes drawn in Blender 2D.
constexpr Color kNight = {3, 5, 15, 255};       // the ground
constexpr Color kNightUp = {7, 11, 34, 255};    // a raised night (bars)
constexpr Color kSilver = {238, 242, 250, 255}; // text, the blade
constexpr Color kMist = {163, 174, 203, 255};   // secondary text
constexpr Color kDim = {93, 106, 142, 255};     // what is not open yet
constexpr Color kSplitR = {255, 79, 79, 255}, kSplitB = {79, 155, 255, 255};  // J1 red, J2 blue
constexpr Color kCpuTag = {201, 205, 212, 255};
constexpr float kLean = 153.0f / 720.0f;  // the blade: 12° from upright, 0.2125 px across per px down
struct Flood {
  Color fill, deep;
};
Flood flood(Hero h);  // the fighter's colour and its deep tone
// A band between two edges leaning like the blade ("/", lean < 0: "\"), through (x0, 0) and (x1, 0).
void lean(float x0, float x1, Color c, float k = kLean, float y0 = 0.0f, float y1 = float(kScreenH));
// CSS easings: cubic-bezier(x1, y1, x2, y2) at t (0..1), and the progress of t through [a, b] eased by it.
float bezier(float t, float x1, float y1, float x2, float y2);
float seg(float t, float a, float b, float x1 = 0.2f, float y1 = 0.9f, float x2 = 0.1f, float y2 = 1.0f);
// Draws `draw` scaled by s around (ox, oy) (CSS scale with its transform-origin).
void scaled(float s, float ox, float oy, const std::function<void()>& draw);
// The jolt as something heavy lands (GO, an ultimate's name, VICTOIRE), s seconds after: 0.45 s, then still.
Vector2 slam(float s);
// Big capitals (CSS sizes: em = font-size; top = the top of its line box, lh its line-height in em; ls the
// letter-spacing in em), from distance fields: crisp at any size. stroke: a CSS text-stroke width in px.
enum class Face : int { Cond, Wide, Exo };  // Archivo 62 %, Archivo 125 %, Exo 2 Black Italic
float bigWidth(const char* s, float em, Face f, float ls = 0.0f);
void big(const char* s, float x, float top, float em, Face f, Color fill, float lh = 0.84f, float ls = 0.0f,
         float stroke = 0.0f, Color strokeCol = kNight);
void bigHollow(const char* s, float x, float top, float em, Face f, float stroke, Color col, float lh = 0.84f,
               float ls = 0.0f);
// The big type is kept on one side of a line leaning like the blade, through (x0, 0): left (keepLeft) or right of it.
void bigClip(float x0, bool keepLeft, float lean = kLean);
void bigClipOff();
// The choc: white capitals with a night outline, their red and blue copies split by `split` px either way.
void choc(const char* s, float cx, float top, float em, float split, float alpha = 1.0f, float stroke = 5.0f,
          Face f = Face::Wide, float lh = 1.0f, float ls = 0.0f);
// Exo 2 Black Italic at a CSS font-size `em`, its baseline at `base` (the damage, the places, the results' numbers).
float exoWidth(const char* s, float em);
void exo(const char* s, float x, float base, float em, Color c);
// Small capitals (Archivo 125 %), caps centred on cy; returns the width.
float caps(const char* s, float x, float cy, float size, float ls, Color col);
float capsWidth(const char* s, float size, float ls);
// A fighter's name in big condensed capitals (em), broken at its spaces when wider than maxW on one line.
std::vector<std::string> nameLines(const std::string& name, float em, float maxW);
// A key cap of the charte (night, a silver rim; filled: silver, night text), or the gamepad's button; its width.
float pkey(const char* key, Btn pad, float x, float y, float h, bool filled = false, float alpha = 1.0f);
float pkeyWidth(const char* key, Btn pad, float h);
// A keyboard key as in the Paramètres mockup: rounded, a night box and a silver rim (alpha `rim`), its name in small
// capitals of `size` (10: at least 12 wide inside, 11: 20), centred on cy, h high in all; its width.
float keyCap(const char* key, float x, float cy, float h, float size, float rim, float alpha = 1.0f);
float keyCapWidth(const char* key, float size);
// The masks drawn in Blender 2D, tinted: the brush that wipes the screen (frame 0..19, 24 a second, frames 8-10
// cover it all), the ultimate's speed lines (frame 0..11, looping) in a band, the brush stroke under a word (0..9).
void brushFrame(int frame, Color col, float alpha = 1.0f);
void speedFrame(int frame, Rectangle band, Color col);
void swooshFrame(int frame, Rectangle dst, Color col);
// The logo: CLASH cut on the blade, H◎RIZON between two thin lines, the cyan star in the O (it pops in, turns and
// breathes); drawn in its 1200 x 500 box at (x, y) scaled by s. in: the parts landing (title), 1 = all there.
struct LogoIn {
  float cut = 1.0f;  // CLASH's halves sliding in along the cut (< 0: not there yet)
  float word = 1.0f, lines = 1.0f;  // H◎RIZON rising in, the lines growing
  float starScale = 1.0f, starRot = 0.0f;  // the star's pop (it overshoots), in degrees
};
void prestigeLogo(float x, float y, float s, float time, const LogoIn& in = {});

// ---- Screen transitions (t in [0,1]) -------------------------------------------
// « Prestige »: Horizon (menus) and Game (out of a fight): the brush drawn in Blender 2D wipes the screen in a
// fighter's colour, its deep tone a frame ahead (covered from frame 8 to 10). Flash (into a fight): the choc, each
// fighter's colour coming in from its side to hit the other's on the blade, then opening on the fight. Cut (into the
// VS): the old screen `old` (a snapshot) splits on the blade, its halves flying apart over the new one.
enum class Wipe : int { Horizon, Flash, Cut, Game };
// The colours of the next transition: the brush's (a), the choc's left (a) and right (b).
void wipeColors(Flood a, Flood b);
float wipeSwap(Wipe w);   // the t at which the next screen comes in
int wipeFrames(Wipe w);   // its length
void wipe(Wipe w, float t, const Texture2D* old = nullptr);

}  // namespace ui
