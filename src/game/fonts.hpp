#pragma once
#include "raylib.h"

// The typefaces of the game, embedded in the executable:
//   Title  - Archivo ExtraCondensed ExtraBold (titles, buttons, labels; as wide as the Saira Condensed it replaced)
//   Ui     - Inter Medium (everything else; sized like the Saira it replaced)
//   Damage - Exo 2 Black Italic (the damage percents, Smash-like)
//   Display - Archivo Expanded ExtraBold (« Épure » titles and buttons)
//   Body    - Inter Medium at its CSS size (« Épure » texts)
enum class FontKind { Ui, Title, Damage, Display, Body };

void loadFonts();   // after InitWindow
void unloadFonts();
const Font& gameFont(FontKind kind);
// « Prestige »: the big capitals of Title, Display and Damage as distance fields, crisp at any size. Drawn between
// beginBig and endBig: `stroke` is half an outline's width in field units (0: none), centred on the glyphs' edge like
// a CSS text-stroke, in `strokeCol` under the fill; hollow: only that outline, in the text's colour.
const Font& bigFont(FontKind kind);
float bigFieldPerPx(float size);  // field units in one pixel of text drawn at raylib size `size`
void beginBig(float stroke = 0.0f, Color strokeCol = BLANK, bool hollow = false);
void endBig();
// Until turned off, the big type keeps to where x * a + y * b <= c (screen units; the frame `targetH` pixels high,
// `scale` pixels per unit).
void setBigClip(bool on, float a = 0.0f, float b = 0.0f, float c = 0.0f, float targetH = 720.0f, float scale = 1.0f);
