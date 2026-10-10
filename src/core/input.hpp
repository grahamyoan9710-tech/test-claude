#pragma once
#include "core/math.hpp"

// Device-independent controller state for one frame.
// The keyboard, gamepads and the CPU all produce one of these.
struct InputState {
  Vec2 stick;   // movement stick, each axis in [-1, 1], +y = down
  Vec2 cstick;  // "smash stick": instant smash attacks / aerials in a direction
  bool attack = false;
  bool special = false;
  bool jump = false;
  bool shield = false;
  bool grab = false;
  bool smash = false;  // keyboard "smash" key: smash attack / aerial towards the stick (or forward)
  bool ultimate = false;  // unleashes the ultimate (final smash) once the ultimate meter is full
};
