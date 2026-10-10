#include "game/controls.hpp"

#include "game/settings.hpp"
#include "raylib.h"

namespace {
bool anyDown(std::initializer_list<int> keys) {
  for (int k : keys)
    if (IsKeyDown(k)) return true;
  return false;
}
bool anyPressed(std::initializer_list<int> keys) {
  for (int k : keys)
    if (IsKeyPressed(k)) return true;
  return false;
}
float deadzone(float v) { return std::fabs(v) < 0.25f ? 0.0f : v; }
}  // namespace

const char* deviceName(int d) {
  static const char* names[] = {"CLAVIER 1", "CLAVIER 2", "MANETTE 1", "MANETTE 2", "MANETTE 3", "MANETTE 4"};
  return (d >= 0 && d < kDeviceCount) ? names[d] : "?";
}

bool deviceAvailable(int d) {
  if (d == kKeyboard1 || d == kKeyboard2) return true;
  return IsGamepadAvailable(d - kPad1);
}

InputState readDevice(int d) {
  InputState in;
  if (d == kKeyboard1 || d == kKeyboard2) {
    // Rebindable keys (Paramètres > Commandes). Clavier 2 also accepts the numpad.
    const auto& k = settings().keys[d == kKeyboard1 ? 0 : 1];
    const bool pad = d == kKeyboard2;
    auto down = [&](int action, int numpadKey) {
      return IsKeyDown(k[action]) || (pad && numpadKey >= 0 && IsKeyDown(numpadKey));
    };
    in.stick.x = float(down(kActRight, -1)) - float(down(kActLeft, -1));
    in.stick.y = float(down(kActDown, -1)) - float(down(kActUp, -1));
    in.jump = down(kActJump, KEY_KP_0);
    in.attack = down(kActAttack, KEY_KP_1);
    in.special = down(kActSpecial, KEY_KP_2);
    in.shield = down(kActShield, KEY_KP_3);
    in.grab = down(kActGrab, KEY_KP_5);
    in.smash = down(kActSmash, KEY_KP_4);
    in.ultimate = down(kActUltimate, KEY_KP_6);
  } else if (d >= kPad1 && d < kDeviceCount) {
    int g = d - kPad1;
    if (!IsGamepadAvailable(g)) return in;
    in.stick = {deadzone(GetGamepadAxisMovement(g, GAMEPAD_AXIS_LEFT_X)),
                deadzone(GetGamepadAxisMovement(g, GAMEPAD_AXIS_LEFT_Y))};
    if (IsGamepadButtonDown(g, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) in.stick.x = -1;
    if (IsGamepadButtonDown(g, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) in.stick.x = 1;
    if (IsGamepadButtonDown(g, GAMEPAD_BUTTON_LEFT_FACE_UP)) in.stick.y = -1;
    if (IsGamepadButtonDown(g, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) in.stick.y = 1;
    in.cstick = {deadzone(GetGamepadAxisMovement(g, GAMEPAD_AXIS_RIGHT_X)),
                 deadzone(GetGamepadAxisMovement(g, GAMEPAD_AXIS_RIGHT_Y))};
    in.attack = IsGamepadButtonDown(g, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    in.special = IsGamepadButtonDown(g, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
    in.jump = IsGamepadButtonDown(g, GAMEPAD_BUTTON_RIGHT_FACE_UP) ||
              IsGamepadButtonDown(g, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
    in.grab = IsGamepadButtonDown(g, GAMEPAD_BUTTON_LEFT_TRIGGER_1);
    in.shield = IsGamepadButtonDown(g, GAMEPAD_BUTTON_RIGHT_TRIGGER_1) ||
                IsGamepadButtonDown(g, GAMEPAD_BUTTON_LEFT_TRIGGER_2) ||
                IsGamepadButtonDown(g, GAMEPAD_BUTTON_RIGHT_TRIGGER_2) ||
                GetGamepadAxisMovement(g, GAMEPAD_AXIS_LEFT_TRIGGER) > 0.3f ||
                GetGamepadAxisMovement(g, GAMEPAD_AXIS_RIGHT_TRIGGER) > 0.3f;
  }
  return in;
}

bool devicePausePressed(int d) {
  if (d == kKeyboard1 || d == kKeyboard2) return anyPressed({KEY_ESCAPE, KEY_P});
  int g = d - kPad1;
  return IsGamepadAvailable(g) && IsGamepadButtonPressed(g, GAMEPAD_BUTTON_MIDDLE_RIGHT);
}

MenuNav readMenuNav() {
  static int held[4] = {0, 0, 0, 0};
  bool dir[4] = {
      anyDown({KEY_UP, KEY_W}), anyDown({KEY_DOWN, KEY_S}),
      anyDown({KEY_LEFT, KEY_A}), anyDown({KEY_RIGHT, KEY_D})};
  MenuNav nav;
  nav.confirm = anyPressed({KEY_ENTER, KEY_SPACE, KEY_J, KEY_KP_ENTER, KEY_KP_1, KEY_PERIOD});
  nav.back = anyPressed({KEY_ESCAPE, KEY_BACKSPACE, KEY_K, KEY_KP_2, KEY_SLASH});
  nav.start = anyPressed({KEY_ENTER, KEY_KP_ENTER});
  for (int g = 0; g < 4; ++g) {
    if (!IsGamepadAvailable(g)) continue;
    float x = GetGamepadAxisMovement(g, GAMEPAD_AXIS_LEFT_X);
    float y = GetGamepadAxisMovement(g, GAMEPAD_AXIS_LEFT_Y);
    dir[0] = dir[0] || y < -0.6f || IsGamepadButtonDown(g, GAMEPAD_BUTTON_LEFT_FACE_UP);
    dir[1] = dir[1] || y > 0.6f || IsGamepadButtonDown(g, GAMEPAD_BUTTON_LEFT_FACE_DOWN);
    dir[2] = dir[2] || x < -0.6f || IsGamepadButtonDown(g, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
    dir[3] = dir[3] || x > 0.6f || IsGamepadButtonDown(g, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
    nav.confirm = nav.confirm || IsGamepadButtonPressed(g, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    nav.back = nav.back || IsGamepadButtonPressed(g, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
    nav.start = nav.start || IsGamepadButtonPressed(g, GAMEPAD_BUTTON_MIDDLE_RIGHT);
  }
  bool out[4];
  for (int i = 0; i < 4; ++i) {
    held[i] = dir[i] ? held[i] + 1 : 0;
    out[i] = held[i] == 1 || (held[i] > 18 && held[i] % 6 == 0);  // key repeat
  }
  nav.up = out[0];
  nav.down = out[1];
  nav.left = out[2];
  nav.right = out[3];
  return nav;
}
