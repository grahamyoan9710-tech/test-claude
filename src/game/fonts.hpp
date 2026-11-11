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
