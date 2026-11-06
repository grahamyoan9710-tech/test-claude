#pragma once
// Colour palettes of the menus (main menu, character select). The shapes and fonts stay: only the colours change.
// The first one is the game's (Clash Horizon: royal blue and gold).
#include "game/common.hpp"

struct MenuTheme {
  const char* name;
  // Main menu
  float dim;             // veil over the stage behind
  Color skyTop, skyLow;  // colour grade over it (their alpha is the strength)
  Color sun;             // glow rising from the bottom (alpha = strength)
  Color tiles[4][2];     // COMBAT, ENTRAÎNEMENT, PARAMÈTRES, QUITTER: top, bottom
  Color glow;            // pulse around the selection
  Color streaks;         // speed streaks behind the tiles (alpha 0 = none)
  // Character select
  float cssDim;
  Color night;           // the ground behind the fighters and under the grid
  Color left, right;     // the two sides of the "VS"
  Color rays;            // the rays behind the "VS"
};

inline const MenuTheme kMenuThemes[] = {
    {"HORIZON (v10.1)", 0.58f, {14, 30, 90, 153}, {40, 100, 220, 87}, {255, 200, 90, 31},
     {{{255, 214, 80, 255}, {240, 140, 24, 255}}, {{90, 170, 255, 255}, {30, 96, 222, 255}},
      {{52, 84, 164, 255}, {20, 38, 98, 255}}, {{255, 104, 104, 255}, {198, 36, 56, 255}}},
     {255, 210, 74, 255}, {0, 0, 0, 0},
     0.7f, {12, 22, 62, 255}, {236, 64, 72, 255}, {64, 140, 255, 255}, {255, 210, 74, 255}},
    {"BRASIER", 0.62f, {70, 0, 12, 217}, {255, 96, 0, 158}, {255, 200, 60, 90},
     {{{255, 64, 36, 255}, {128, 8, 18, 255}}, {{255, 146, 24, 255}, {150, 46, 0, 255}},
      {{240, 176, 26, 255}, {150, 78, 0, 255}}, {{84, 46, 44, 255}, {24, 10, 12, 255}}},
     {255, 222, 70, 255}, {255, 200, 90, 77},
     0.8f, {36, 6, 10, 255}, {255, 52, 30, 255}, {255, 176, 20, 255}, {255, 214, 120, 255}},
    {"NÉON", 0.62f, {18, 4, 48, 190}, {200, 0, 140, 95}, {0, 230, 255, 40},
     {{{255, 40, 150, 255}, {110, 0, 80, 255}}, {{0, 210, 228, 255}, {0, 66, 108, 255}},
      {{140, 80, 255, 255}, {40, 10, 130, 255}}, {{255, 214, 30, 255}, {170, 90, 0, 255}}},
     {0, 240, 255, 255}, {255, 60, 200, 77},
     0.84f, {16, 4, 40, 255}, {255, 30, 150, 255}, {0, 210, 255, 255}, {150, 230, 255, 255}},
    {"SHŌNEN", 0.3f, {10, 110, 255, 140}, {255, 150, 40, 128}, {255, 240, 150, 90},
     {{{255, 112, 20, 255}, {170, 36, 0, 255}}, {{112, 200, 40, 255}, {24, 96, 20, 255}},
      {{30, 130, 255, 255}, {8, 44, 150, 255}}, {{222, 40, 60, 255}, {100, 8, 24, 255}}},
     {255, 236, 90, 255}, {255, 255, 255, 77},
     0.45f, {20, 56, 150, 255}, {255, 120, 16, 255}, {20, 140, 255, 255}, {255, 250, 210, 255}},
    {"ÉLECTRIQUE", 0.6f, {6, 12, 60, 190}, {0, 110, 255, 105}, {255, 230, 60, 45},
     {{{30, 150, 255, 255}, {6, 40, 150, 255}}, {{255, 206, 20, 255}, {180, 96, 0, 255}},
      {{0, 206, 240, 255}, {0, 70, 140, 255}}, {{130, 60, 240, 255}, {40, 10, 120, 255}}},
     {255, 240, 60, 255}, {140, 210, 255, 77},
     0.84f, {6, 14, 56, 255}, {255, 196, 0, 255}, {0, 150, 255, 255}, {255, 240, 140, 255}},
    {"POP", 0.4f, {90, 20, 200, 140}, {255, 70, 170, 105}, {255, 240, 120, 55},
     {{{255, 50, 130, 255}, {140, 0, 70, 255}}, {{150, 226, 30, 255}, {46, 116, 0, 255}},
      {{0, 200, 210, 255}, {0, 88, 120, 255}}, {{255, 150, 30, 255}, {170, 60, 0, 255}}},
     {190, 255, 60, 255}, {255, 255, 255, 77},
     0.7f, {50, 14, 110, 255}, {255, 50, 140, 255}, {0, 210, 220, 255}, {255, 240, 220, 255}},
};
inline int g_menuTheme = 0;
inline int menuThemeCount() { return int(sizeof(kMenuThemes) / sizeof(kMenuThemes[0])); }
inline const MenuTheme& menuTheme() { return kMenuThemes[g_menuTheme < 0 || g_menuTheme >= menuThemeCount() ? 0 : g_menuTheme]; }
