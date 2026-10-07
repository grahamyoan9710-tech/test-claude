#pragma once
#include "raylib.h"

// The two typefaces of the game, embedded in the executable:
//   Title - Luckiest Guy (logo, big titles, damage percents)
//   Ui    - Poppins Bold (everything else)
enum class FontKind { Ui, Title };

void loadFonts();   // after InitWindow
void unloadFonts();
const Font& gameFont(FontKind kind);
