#pragma once
#include <array>
#include <string>

// Keyboard actions that can be rebound.
enum KeyAction : int {
  kActLeft, kActRight, kActUp, kActDown, kActJump, kActAttack, kActSpecial, kActShield, kActGrab, kActSmash, kActUltimate,
  kActionCount
};
const char* actionName(int action);

struct Settings {
  // Audio (0..100)
  int masterVolume = 80;
  int musicVolume = 60;
  int sfxVolume = 90;
  // Display
  bool fullscreen = false;
  bool highQuality = true;  // 2x supersampling (smooth edges)
  bool pixelArt = true;     // the characters are drawn as pixel-art sprites
  int screenShake = 2;      // 0 none, 1 light, 2 normal
  bool showFps = false;
  bool showHitboxes = false;
  bool playerTags = true;
  // Rules
  int stocks = 3;
  bool items = true;
  bool weapons = true;  // weapons drop during matches
  bool teams = false;   // team battle (2 v 2): red against blue
  int cpuLevel = 5;
  bool tapJump = true;  // Up (the stick or the key) jumps too, as in Smash Bros.
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

Settings& settings();
void loadSettings();
bool saveSettings();
std::string settingsPath();
float shakeScale();
// Human readable key name, following the keyboard layout when possible (AZERTY shows "Z", not "W").
std::string keyName(int key);
bool keyBindable(int key);
bool padBindable(int button);  // face buttons, shoulders, triggers, Back and stick clicks (not Start / d-pad)
