#pragma once
#include <array>
#include <string>

// Keyboard actions that can be rebound.
enum KeyAction : int {
  kActLeft, kActRight, kActUp, kActDown, kActJump, kActAttack, kActSpecial, kActShield, kActGrab, kActSmash,
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
  int screenShake = 2;      // 0 none, 1 light, 2 normal
  bool showFps = false;
  bool showHitboxes = false;
  bool playerTags = true;
  // Rules
  int stocks = 3;
  bool items = true;
  int cpuLevel = 5;
  // Keyboard bindings for "Clavier 1" and "Clavier 2"
  std::array<std::array<int, kActionCount>, 2> keys{};

  Settings();
  void resetKeys(int keyboard);
};

Settings& settings();
void loadSettings();
bool saveSettings();
std::string settingsPath();
float shakeScale();
// Human readable key name, following the keyboard layout when possible (AZERTY shows "Z", not "W").
std::string keyName(int key);
bool keyBindable(int key);
