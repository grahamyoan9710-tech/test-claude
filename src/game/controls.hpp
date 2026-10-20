#pragma once
#include <string>

#include "core/input.hpp"

// Input devices a human player can use.
enum Device : int { kKeyboard1 = 0, kKeyboard2 = 1, kPad1 = 2, kPad2, kPad3, kPad4, kDeviceCount };
constexpr int kMaxPads = kDeviceCount - kPad1;

const char* deviceName(int device);
bool deviceAvailable(int device);
InputState readDevice(int device);
bool devicePausePressed(int device);

// ---- Gamepads -------------------------------------------------------------------
// "MANETTE n" is a logical pad: only real gamepads get a number (never a touchpad or the
// motion sensors of a PlayStation pad), and a pad keeps its number while it stays plugged in
// (unplugged and plugged back, it gets it back).
enum class PadFamily { Xbox, PlayStation, Nintendo };
// Buttons by position, Xbox names: A = bottom face, B = right, X = left, Y = top.
enum PadButton : int {
  kPadA, kPadB, kPadX, kPadY, kPadLB, kPadRB, kPadLT, kPadRT, kPadBack, kPadStart,
  kPadL3, kPadR3, kPadUp, kPadDown, kPadLeft, kPadRight, kPadButtonCount
};
void initPads();    // after InitWindow: controller database, hot-plug fix
void updatePads();  // once per frame, before any input is read
bool padConnected(int pad);
bool padDown(int pad, int button);
bool padPressed(int pad, int button);
bool padAnyPressed(int button);  // on any connected pad
PadFamily padFamily(int pad);
bool padHasTriggers(int pad);    // false for SNES-style pads: they shield with RB, ultimate on Back
// Button bound to an action on that pad (Paramètres > Commandes), slot 0 or 1; -1 = none.
int padBinding(int pad, int action, int slot);
const char* padModel(int pad);   // short readable name: "Manette Xbox", "DualSense"...
std::string takePadNotice();     // "MANETTE 2 connectée..." once, for a toast ("" = nothing new)

// ---- Prompts ----------------------------------------------------------------------
// Button hints follow the last used device: keyboard keys, or the buttons of the gamepad
// family last used (or plugged in).
bool padPrompts();
PadFamily promptFamily();
void forcePadPrompts(PadFamily family);  // --pad-glyphs (no gamepad needed)

// Menu navigation merged over every device.
struct MenuNav {
  bool up = false, down = false, left = false, right = false;
  bool confirm = false, back = false, start = false, prev = false;
  int device = -1;  // device behind confirm / start (-1: none)
};
MenuNav readMenuNav();
// One device only (its own key repeat): keyboard 1 = WASD, confirm Space/J, previous K; keyboard 2 = arrows,
// confirm . or numpad 1, previous / or numpad 2; a gamepad = stick/d-pad, A, X. Start: Enter / numpad Enter /
// START. Back (Escape, B) is left to readMenuNav. Call it after readMenuNav (which reads the sticks).
MenuNav readMenuNavFor(int device);
