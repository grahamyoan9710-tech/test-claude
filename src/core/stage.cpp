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
// A platform swinging around its rest position: offset = amplitude * sin(2 pi frame / period).
Platform moving(Platform p, Vec2 amplitude, int period) {
  p.amplitude = amplitude;
  p.period = period;
  return p;
}
StageDef world(const char* name, const char* series, StageTheme theme) {
  StageDef s;
  s.name = name;
  s.series = series;
  s.theme = theme;
  s.blast = {-1150, -950, 2300, 1600};
  s.respawn = {0, -360};
  return s;
}

// ---- The five arenas, one per fighter (painted with PixelLab: render_stages.cpp) ----
// The series is the fighter's element: the arena select tints its chip with that fighter's colour.
StageDef abimeLunaire() {
  StageDef s = world("Abîme Lunaire", "Ombre", StageTheme::Lunar);
  s.blurb = "Un seul grand plateau suspendu dans un vide violet, sous une lune immense.";
  s.platforms = {solid(-440, 440, 0, 70)};
  s.spawns = {{-300, 0}, {300, 0}, {-110, 0}, {110, 0}};
  s.respawn = {0, -320};
  return s;
}
StageDef citadelleDeGivre() {
  StageDef s = world("Citadelle de Givre", "Givre", StageTheme::Frost);
  s.blurb = "Les remparts enneigés d'une forteresse, sous l'aurore : trois corniches au-dessus du sol.";
  s.platforms = {solid(-380, 380, 0, 60), thin(-300, -120, -150), thin(120, 300, -150), thin(-90, 90, -290)};
  s.blast = {-1100, -950, 2200, 1600};
  s.spawns = {{-260, 0}, {260, 0}, {-210, -150}, {210, -150}};
  s.respawn = {0, -420};
  return s;
}
StageDef canyonEcarlate() {
  StageDef s = world("Canyon Écarlate", "Brasier", StageTheme::Crimson);
  s.blurb = "Deux corniches de grès rouge de part et d'autre d'un gouffre, sous les lanternes.";
  s.platforms = {solid(-470, -150, 0, 70), solid(150, 470, 0, 70), thin(-80, 80, -115)};
  s.spawns = {{-330, 0}, {330, 0}, {-230, 0}, {230, 0}};
  s.blast = {-1200, -950, 2400, 1600};
  return s;
}
StageDef toitsKurenai() {
  StageDef s = world("Toits Kurenai", "Foudre rouge", StageTheme::Kurenai);
  s.blurb = "Les toits d'une vieille ville sous la pluie : un long faîte et deux poutres.";
  s.platforms = {solid(-480, 480, 0, 60), thin(-370, -205, -175), thin(205, 370, -175)};
  s.spawns = {{-300, 0}, {300, 0}, {-120, 0}, {120, 0}};
  s.blast = {-1200, -950, 2400, 1600};
  return s;
}
StageDef monastereDesNuees() {
  StageDef s = world("Monastère des Nuées", "Lumière", StageTheme::Clouds);
  s.blurb = "Des îlots au-dessus d'une mer de nuages, une planche qui va et vient.";
  s.platforms = {solid(-300, 300, 0, 60), solid(-640, -440, 70, 50), solid(440, 640, 70, 50),
                 moving(thin(-100, 100, -170), {360, 0}, 600)};
  s.blast = {-1300, -950, 2600, 1650};
  s.spawns = {{-200, 0}, {200, 0}, {-540, 70}, {540, 70}};
  s.respawn = {0, -340};
  return s;
}
}  // namespace

const std::vector<StageDef>& stageList() {
  static const std::vector<StageDef> list = {abimeLunaire(), citadelleDeGivre(), canyonEcarlate(), toitsKurenai(),
                                             monastereDesNuees()};
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
