#pragma once
#include "core/match.hpp"

// Computer-controlled player. Produces an InputState every frame, exactly
// like a human controller would, so the CPU plays by the same rules.
class CpuBrain {
 public:
  explicit CpuBrain(int level = 5, uint32_t seed = 1);
  InputState think(const Match& m, int self);

 private:
  void tap(InputState& in, bool InputState::*b) const { in.*b = !(last_.*b); }
  void tapCStick(InputState& in, Vec2 dir) const {
    in.cstick = last_.cstick.len() > 0.5f ? Vec2{} : dir;
  }
  bool recover(const Match& m, const Fighter& f, InputState& in);
  void fight(const Match& m, const Fighter& f, const Fighter& t, InputState& in);

  int level_;
  float skill_;  // 0..1
  Rng rng_;
  InputState last_;
  int decisionTimer_ = 0;
  int shieldHold_ = 0;
  int chargeHold_ = 0;
  Vec2 chargeDir_;
  int waitTimer_ = -1;
};
