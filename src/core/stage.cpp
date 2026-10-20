#include "core/stage.hpp"

namespace {
const char* const kZonai = "Terres anciennes";

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

StageDef finalDestination() {
  StageDef s;
  s.name = "Profondeurs";
  s.blurb = "Les abysses sous Hyrule : un seul grand plateau, nulle part où se cacher.";
  s.series = kZonai;
  s.theme = StageTheme::Cosmic;
  s.platforms = {solid(-440, 440, 0, 70)};
  s.blast = {-1150, -950, 2300, 1600};
  s.spawns = {{-300, 0}, {300, 0}, {-110, 0}, {110, 0}};
  s.respawn = {0, -320};
  return s;
}

StageDef battlefield() {
  StageDef s;
  s.name = "Sanctuaire";
  s.blurb = "Un temple zonaï au soleil couchant et ses trois corniches de pierre.";
  s.series = kZonai;
  s.theme = StageTheme::Sunset;
  s.platforms = {solid(-380, 380, 0, 60), thin(-300, -120, -150), thin(120, 300, -150), thin(-90, 90, -290)};
  s.blast = {-1100, -950, 2200, 1600};
  s.spawns = {{-260, 0}, {260, 0}, {-210, -150}, {210, -150}};
  s.respawn = {0, -420};
  return s;
}

StageDef skyIslands() {
  StageDef s;
  s.name = "Îles Célestes";
  s.blurb = "Des îles flottantes sur une mer de nuages, une dalle qui va et vient.";
  s.series = kZonai;
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
// ---- Desert and white city ----
// Hueco Mundo under its eternal crescent moon: the white palace of the Espada and two broken pillars.
StageDef lasNoches() {
  StageDef s = world("Désert de lune", "Désert", StageTheme::LasNoches);
  s.blurb = "Le palais des Espada au Hueco Mundo, sous la lune éternelle.";
  s.platforms = {solid(-430, 430, 0, 60), thin(-290, -125, -140), thin(125, 290, -140)};
  s.spawns = {{-280, 0}, {280, 0}, {-200, -140}, {200, -140}};
  return s;
}
// The Court of Pure Souls: fighting on the white walls and the tiled roofs, the higher roof off-centre.
StageDef seireitei() {
  StageDef s = world("Cité blanche", "Cité", StageTheme::Seireitei);
  s.blurb = "La Cour des âmes pures : on se bat sur les murs blancs et les toits.";
  s.platforms = {solid(-410, 410, 0, 60), thin(-320, -140, -130), thin(60, 270, -225)};
  s.spawns = {{-270, 0}, {270, 0}, {-230, -130}, {165, -225}};
  return s;
}

// ---- Forest ----
// The Black Bulls' hideout: two wooden balconies and a magic broom flying back and forth above them.
StageDef blackBulls() {
  StageDef s = world("Vieux repaire", "Forêt", StageTheme::BlackBulls);
  s.blurb = "Le QG des Taureaux Noirs, et un balai magique qui fait des allers-retours.";
  s.platforms = {solid(-390, 390, 0, 60), thin(-300, -145, -140), thin(145, 300, -140),
                 moving(thin(-80, 80, -275), {230, 0}, 560)};
  s.spawns = {{-250, 0}, {250, 0}, {-220, -140}, {220, -140}};
  s.respawn = {0, -420};
  return s;
}
// At the foot of the Grimoire Tower of Hage: open grimoires floating up and down at twilight.
StageDef grimoireTower() {
  StageDef s = world("Tour du savoir", "Forêt", StageTheme::GrimoireTower);
  s.blurb = "Au pied de la tour de Hage, des grimoires flottent en montant et descendant.";
  s.platforms = {solid(-370, 370, 0, 60), moving(thin(-285, -130, -135), {0, 22}, 260),
                 moving(thin(130, 285, -135), {0, -22}, 260), moving(thin(-75, 75, -260), {0, 16}, 340)};
  s.spawns = {{-240, 0}, {240, 0}, {-130, 0}, {130, 0}};
  s.respawn = {0, -400};
  return s;
}

// ---- Neon crossroads and shadow temple ----
// Shibuya on Halloween night, under the veil: a long overpass and two billboards.
StageDef shibuya() {
  StageDef s = world("Carrefour néon", "Cité", StageTheme::Shibuya);
  s.blurb = "Shibuya le soir d'Halloween, sous le voile : une longue voie rapide.";
  s.platforms = {solid(-480, 480, 0, 60), thin(-370, -205, -175), thin(205, 370, -175)};
  s.spawns = {{-300, 0}, {300, 0}, {-120, 0}, {120, 0}};
  s.blast = {-1200, -950, 2400, 1600};
  return s;
}
// The Tokyo school of jujutsu in autumn: wooden walkways climbing up toward the temple.
StageDef jujutsuHigh() {
  StageDef s = world("Temple des ombres", "Temples", StageTheme::JujutsuHigh);
  s.blurb = "L'école d'exorcisme de Tokyo en automne, des passerelles en escalier.";
  s.platforms = {solid(-390, 390, 0, 60), thin(-320, -160, -115), thin(-80, 80, -200), thin(160, 320, -285)};
  s.spawns = {{-260, 0}, {260, 0}, {-240, -115}, {0, -200}};
  s.respawn = {0, -420};
  return s;
}

// ---- Mountain temples ----
// The Hidden Leaf: a rooftop under the Hokage Rock, branches of the great trees above it.
StageDef konoha() {
  StageDef s = world("Village des cimes", "Temples", StageTheme::Konoha);
  s.blurb = "Le village caché de la Feuille, au pied du mont des Hokage.";
  s.platforms = {solid(-400, 400, 0, 60), thin(-330, -170, -125), thin(170, 330, -125), thin(-80, 80, -240)};
  s.spawns = {{-270, 0}, {270, 0}, {-250, -125}, {250, -125}};
  s.respawn = {0, -400};
  return s;
}
// The Valley of the End: standing on the heads of the two great statues, a waterfall between them.
StageDef valleyOfTheEnd() {
  StageDef s = world("Vallée des Cieux", "Temples", StageTheme::ValleyOfTheEnd);
  s.blurb = "Debout sur les statues de Hashirama et Madara, la cascade entre les deux.";
  s.platforms = {solid(-470, -150, 0, 70), solid(150, 470, 0, 70), thin(-80, 80, -115)};
  s.spawns = {{-330, 0}, {330, 0}, {-230, 0}, {230, 0}};
  s.blast = {-1200, -950, 2400, 1600};
  return s;
}
}  // namespace

const std::vector<StageDef>& stageList() {
  static const std::vector<StageDef> list = {finalDestination(), battlefield(), skyIslands(), lasNoches(),
                                             seireitei(),        blackBulls(),  grimoireTower(), shibuya(),
                                             jujutsuHigh(),      konoha(),      valleyOfTheEnd()};
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
