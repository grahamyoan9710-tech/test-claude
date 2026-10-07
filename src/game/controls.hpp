#pragma once
#include "core/input.hpp"

// Input devices a human player can use.
enum Device : int { kKeyboard1 = 0, kKeyboard2 = 1, kPad1 = 2, kPad2, kPad3, kPad4, kDeviceCount };

const char* deviceName(int device);
bool deviceAvailable(int device);
InputState readDevice(int device);
bool devicePausePressed(int device);

// Menu navigation merged over every device.
struct MenuNav {
  bool up = false, down = false, left = false, right = false;
  bool confirm = false, back = false, start = false;
};
MenuNav readMenuNav();
