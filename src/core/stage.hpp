#pragma once
#include <string>
#include <vector>

#include "core/math.hpp"

struct Platform {
  Rect rect;               // current position
  Rect base;               // rest position
  bool solid = true;       // false = pass-through (land on top only)
  bool ledges = false;     // corners can be grabbed
  Vec2 amplitude;          // moving platforms: offset = amplitude * sin(phase)
  int period = 0;          // frames for a full cycle (0 = static)
  Vec2 delta;              // displacement during the last frame
};

// The five arenas painted with PixelLab, one per fighter (render_stages.cpp).
enum class StageTheme : uint8_t { Lunar, Frost, Crimson, Kurenai, Clouds };

struct StageDef {
  std::string name;
  std::string series;      // where the world comes from (stage select)
  std::string blurb;       // a line about it (stage select)
  StageTheme theme = StageTheme::Lunar;
  std::vector<Platform> platforms;
  Rect blast;              // leaving this rectangle = KO
  std::vector<Vec2> spawns;
  Vec2 respawn;            // respawn platform position (feet)
};

const std::vector<StageDef>& stageList();
void updatePlatforms(std::vector<Platform>& plats, int frame);
