#pragma once
#include <array>
#include <string>

// Keyboard actions that can be rebound.
enum KeyAction : int {
  kActLeft, kActRight, kActUp, kActDown, kActJump, kActAttack, kActSpecial, kActShield, kActGrab, kActSmash, kActUltimate, kActWeapon,
  kActionCount
};
const char* actionName(int action);

struct Settings {
  // Audio (0..100)
  int masterVolume = 80;
  int musicVolume = 60;
  int sfxVolume = 90;
  // Display
  bool fullscreen = false;  // the whole screen at its own size; the game renders at resW x resH and scales up
  int resW = 0, resH = 0;   // the resolution (fullscreen: the render's; windowed: the window's); 0 = the screen's
  int upscale = 0;          // 0 native, 1 quality (67 %), 2 balanced (59 %), 3 performance (50 %): FSR 1 style
  int antiAlias = 1;        // 0 none, 1 FXAA, 2 SSAA x2 (renders twice as large)
  int sharpness = 50;       // 0..100: FSR 1's RCAS sharpening of an upscaled frame
  bool vsync = true;
  bool highQuality = true;  // "Haute": 2x supersampling, pixel-art sprite look, halos; "Moyenne": none of them
  int rtx = 0;              // 0 drawn light (render_light.cpp), 1 medium, 2 high ray tracing (rtx.hpp)
  int screenShake = 2;      // 0 none, 1 light, 2 normal
  bool showFps = false;
  bool showHitboxes = false;
  bool playerTags = true;
  // Rules
  int stocks = 3;
  bool items = true;
  bool weapons = true;  // the fighters' weapons drop during matches
  bool teams = false;   // team battle (2 v 2): red against blue
  int cpuLevel = 5;
  bool tapJump = true;  // Up (the stick or the key) jumps too, as in Smash Bros.
  int menuBg = 0;  // the main menu's background (menu_fond_1..3), the next one at each visit
  // Keyboard bindings for "Clavier 1" and "Clavier 2"
  std::array<std::array<int, kActionCount>, 2> keys{};
  // Gamepad bindings, shared by every pad: up to two buttons per action (PadButton values, -1 = none).
  // Only the actions from kActJump on: moving (stick / d-pad), the smash stick and Start stay fixed.
  std::array<std::array<int, 2>, kActionCount> pad{};

  Settings();
  void resetKeys(int keyboard);
  void resetPad();
  bool padIsDefault() const;
};

// The scale of the render for each `upscale` (FSR 1's presets).
constexpr float kUpscale[4] = {1.0f, 1.0f / 1.5f, 1.0f / 1.7f, 0.5f};

Settings& settings();
void loadSettings();
bool saveSettings();
std::string settingsPath();
float shakeScale();
// The window in windowed mode (screen mw x mh): the chosen resolution, or by default 1280 x 720 (16:9, under the screen).
int windowW(const Settings& s, int mw);
int windowH(const Settings& s, int mw, int mh);
// Human readable key name, following the keyboard layout when possible (AZERTY shows "Z", not "W").
std::string keyName(int key);
bool keyBindable(int key);
bool padBindable(int button);  // face buttons, shoulders, triggers, Back and stick clicks (not Start / d-pad)
