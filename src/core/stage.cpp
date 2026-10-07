#include "core/stage.hpp"

namespace {
Platform solid(float x0, float x1, float y, float h) {
  Platform p;
  p.rect = p.base = {x0, y, x1 - x0, h};
  p.solid = true;
  p.ledges = true;
  return p;
}
Platform thin(float x0, float x1, float y) {
  Platform p;
  p.rect = p.base = {x0, y, x1 - x0, 12};
  p.solid = false;
  return p;
}

StageDef finalDestination() {
  StageDef s;
  s.name = "Destination Finale";
  s.theme = StageTheme::Cosmic;
  s.platforms = {solid(-440, 440, 0, 70)};
  s.blast = {-1150, -950, 2300, 1600};
  s.spawns = {{-300, 0}, {300, 0}, {-110, 0}, {110, 0}};
  s.respawn = {0, -320};
  return s;
}

StageDef battlefield() {
  StageDef s;
  s.name = "Champ de Bataille";
  s.theme = StageTheme::Sunset;
  s.platforms = {solid(-380, 380, 0, 60), thin(-300, -120, -150), thin(120, 300, -150), thin(-90, 90, -290)};
  s.blast = {-1100, -950, 2200, 1600};
  s.spawns = {{-260, 0}, {260, 0}, {-210, -150}, {210, -150}};
  s.respawn = {0, -420};
  return s;
}

StageDef skyIslands() {
  StageDef s;
  s.name = "Iles Celestes";
  s.theme = StageTheme::Sky;
  Platform moving = thin(-100, 100, -170);
  moving.amplitude = {360, 0};
  moving.period = 600;
  s.platforms = {solid(-300, 300, 0, 60), solid(-640, -440, 70, 50), solid(440, 640, 70, 50), moving};
  s.blast = {-1300, -950, 2600, 1650};
  s.spawns = {{-200, 0}, {200, 0}, {-540, 70}, {540, 70}};
  s.respawn = {0, -340};
  return s;
}
}  // namespace

const std::vector<StageDef>& stageList() {
  static const std::vector<StageDef> list = {finalDestination(), battlefield(), skyIslands()};
  return list;
}

void updatePlatforms(std::vector<Platform>& plats, int frame) {
  for (auto& p : plats) {
    if (p.period <= 0) {
      p.delta = {0, 0};
      continue;
    }
    float s = std::sin(2.0f * kPi * float(frame) / float(p.period));
    float nx = p.base.x + p.amplitude.x * s;
    float ny = p.base.y + p.amplitude.y * s;
    p.delta = {nx - p.rect.x, ny - p.rect.y};
    p.rect.x = nx;
    p.rect.y = ny;
  }
}
