#pragma once
#include "raylib.h"

// The two typefaces of the game, embedded in the executable:
//   Title - Saira Condensed Black (titles, buttons, labels, damage percents)
//   Ui    - Saira SemiBold (everything else)
enum class FontKind { Ui, Title };

void loadFonts();   // after InitWindow
void unloadFonts();
const Font& gameFont(FontKind kind);
