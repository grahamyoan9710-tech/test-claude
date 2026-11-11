#pragma once
#include "raylib.h"

// The typefaces of the game, embedded in the executable:
//   Title  - Saira Condensed Black (titles, buttons, labels)
//   Ui     - Saira SemiBold (everything else)
//   Damage - Exo 2 Black Italic (the damage percents, Smash-like)
enum class FontKind { Ui, Title, Damage };

void loadFonts();   // after InitWindow
void unloadFonts();
const Font& gameFont(FontKind kind);
