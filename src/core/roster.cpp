// The seventeen fighters of Super Clash Bros.: fan-made homages to Black Clover, Bleach, Naruto, Jujutsu
// Kaisen, Zelda: Tears of the Kingdom, God of War and Monster Hunter. Each one has its normals, four
// specials, three ultimates (bare-handed and with each of its two weapons) and two weapons that drop
// during a match.
#include <cmath>
#include <iterator>

#include "core/roster_kit.hpp"

namespace {
using namespace kit;

// ---------------------------------------------------------------------------------------------
// Black Clover
// ---------------------------------------------------------------------------------------------
CharacterDef makeAsta() {
  CharacterDef c;
  c.name = "ASTA";
  c.title = "Le briseur de magie";
  c.series = "Black Clover";
  c.hero = Hero::Asta;
  c.style = BodyStyle::Knight;
  c.baseWeapon = WeaponVisual::AstaSword;
  c.palettes = {pal({44, 42, 50}, {96, 92, 100}, {222, 172, 64}, {238, 198, 162}, {226, 226, 214}, {80, 150, 70}),
                pal({236, 232, 220}, {60, 58, 66}, {200, 48, 52}, {238, 198, 162}, {226, 226, 214}, {80, 150, 70})};
  stats(c, 46, 98, 102, 4.3f, 9.2f, 5.3f, 0.66f, 10.5f, 15.5f, 16.0f, 10.5f, 14.5f, 3);
  c.weaponLength = 84;
  const float S = 1.05f, P = 0.9f;
  buildSwordNormals(c, S, P, F::AntiMagic);
  auto& m = c.moves;

  m[NSpecial] = mv("Lame anti-magie", 42, {});
  m[NSpecial].projectileFrame = 14;
  m[NSpecial].projectile = proj({46, -8}, {11, 0}, 0, 34, 22, 8, 30, 62, F::AntiMagic, PV::BlackSlash);
  m[NSpecial].projectile.piercing = true;
  m[NSpecial].pose = PoseHint::Overhead;

  std::vector<HitboxDef> dash;
  multi(dash, 8, 3, 4, 40, -4, 28, 2.5f, L::Weapon, F::AntiMagic);
  dash.push_back(imp(hb(20, 24, 52, -6, 32, 7.5f, 40, 55, 80, L::Weapon, 3, F::AntiMagic)));
  m[SSpecial] = mv("Météore noir", 48, dash);
  m[SSpecial].vel = {ve(8, VelMode::SetX, 15, 0), ve(8, VelMode::SetY, 0, -1.0f, 0, true),
                     ve(26, VelMode::Scale, 0.2f, 1.0f)};
  m[SSpecial].gravityMul = 0.3f;
  m[SSpecial].frictionMul = 0.1f;
  m[SSpecial].pose = PoseHint::Thrust;

  std::vector<HitboxDef> up;
  multi(up, 4, 4, 4, 0, -30, 34, 2.0f, L::Weapon, F::AntiMagic, 0, 85);
  up.push_back(hb(20, 24, 0, -50, 38, 5.5f, 80, 60, 105, L::Weapon, 4, F::AntiMagic));
  m[USpecial] = mv("Ouragan noir", 46, up);
  m[USpecial].vel = {ve(4, VelMode::Set, 1.5f, -16.5f, 3.5f)};
  m[USpecial].gravityMul = 0.5f;
  m[USpecial].helplessAfter = true;
  m[USpecial].pose = PoseHint::Spin;

  m[DSpecial] = mv("Renvoi anti-magie", 52, {imp(hb(26, 30, 60, -8, 46, 12, 361, 60, 95, L::Weapon, 0, F::AntiMagic))});
  m[DSpecial].counterStart = 5;
  m[DSpecial].counterEnd = 26;
  m[DSpecial].reflects = true;
  m[DSpecial].gravityMul = 0.3f;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  c.weapons[0] = weapon("Épée pourfendeuse", "Lame géante : frappe plus fort et plus loin", WeaponVisual::DemonSlayer,
                        F::AntiMagic, 1.3f, 1.12f, 1.25f);
  MoveDef big = m[NSpecial];
  big.name = "Fendeur anti-magie";
  big.projectile.radius *= 1.6f;
  big.projectile.hit.radius = big.projectile.radius;
  big.projectile.hit.damage *= 1.4f;
  big.projectile.velocity.x = 13;
  big.projectile.hit.impact = true;
  c.weapons[0].moves = {{NSpecial, big}};
  c.weapons[1] = weapon("Épée habitante", "Lame rapide : la ruée devient Black Divider", WeaponVisual::DemonDweller,
                        F::AntiMagic, 1.15f, 1.05f, 1.1f);
  MoveDef div = m[SSpecial];
  div.name = "Black Divider";
  div.vel = {ve(6, VelMode::SetX, 19, 0), ve(6, VelMode::SetY, 0, -1.0f, 0, true), ve(28, VelMode::Scale, 0.2f, 1.0f)};
  div.hitboxes.clear();
  multi(div.hitboxes, 6, 5, 4, 46, -4, 32, 2.5f, L::Weapon, F::AntiMagic);
  div.hitboxes.push_back(imp(hb(26, 30, 56, -6, 38, 11, 40, 65, 92, L::Weapon, 5, F::AntiMagic)));
  sc(div, S, P);
  c.weapons[1].moves = {{SSpecial, div}};
  return c;
}

CharacterDef makeYuno() {
  CharacterDef c;
  c.name = "YUNO";
  c.title = "Le prodige du vent";
  c.series = "Black Clover";
  c.hero = Hero::Yuno;
  c.style = BodyStyle::Ninja;
  c.palettes = {pal({236, 232, 220}, {44, 52, 86}, {214, 176, 72}, {240, 206, 176}, {28, 28, 36}, {214, 170, 60}),
                pal({44, 44, 52}, {58, 72, 62}, {120, 210, 150}, {240, 206, 176}, {28, 28, 36}, {214, 170, 60})};
  stats(c, 44, 100, 84, 4.4f, 9.6f, 5.8f, 0.52f, 8.6f, 13.5f, 14.8f, 9.8f, 13.5f, 3);
  c.groundAccel = 1.4f;
  c.airAccel = 0.55f;
  c.airJumps = 2;
  const float S = 0.97f, P = 1.1f;
  buildBrawlerNormals(c, S, P, F::Wind);
  auto& m = c.moves;

  m[NSpecial] = mv("Lames de vent", 34, {});
  m[NSpecial].projectileFrame = 10;
  m[NSpecial].projectile = proj({34, -8}, {12.5f, 0}, 0, 38, 10, 3.6f, 12, 32, F::Wind, PV::WindBlade);
  m[NSpecial].projectile.count = 3;
  m[NSpecial].projectile.spreadDeg = 7;
  m[NSpecial].pose = PoseHint::Cast;

  m[SSpecial] = mv("Tempête tranchante", 46, {});
  m[SSpecial].projectileFrame = 14;
  m[SSpecial].projectile = proj({46, 0}, {4.5f, 0}, 0, 80, 30, 1.8f, 12, 8, F::Wind, PV::Tornado);
  m[SSpecial].projectile.piercing = true;
  m[SSpecial].projectile.rehit = 7;
  m[SSpecial].projectile.pull = 1.0f;
  m[SSpecial].projectile.pullRadius = 90;
  m[SSpecial].pose = PoseHint::Cast;

  m[USpecial] = mv("Envol de Sylphe", 38, {hb(8, 22, 0, 0, 26, 3.5f, 80, 45, 30, L::Body, 0, F::Wind)});
  m[USpecial].vel = {ve(5, VelMode::Set, 0, 0), ve(8, VelMode::StickDir, 20, 0), ve(24, VelMode::Scale, 0.3f, 0.3f)};
  m[USpecial].gravityMul = 0.0f;
  m[USpecial].allowDrift = false;
  m[USpecial].helplessAfter = true;
  m[USpecial].pose = PoseHint::Rush;

  std::vector<HitboxDef> vortex;
  multi(vortex, 8, 4, 5, 0, -6, 52, 2.0f, L::Body, F::Wind);
  vortex.push_back(hb(30, 34, 0, -6, 58, 6, 60, 65, 100, L::Body, 4, F::Wind));
  m[DSpecial] = mv("Tourbillon", 50, vortex);
  m[DSpecial].vel = {ve(8, VelMode::SetY, 0, -3, 0, true)};
  m[DSpecial].gravityMul = 0.4f;
  m[DSpecial].pose = PoseHint::Spin;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  c.weapons[0] = weapon("Grimoire à quatre trèfles", "Cinq lames de vent d'un coup", WeaponVisual::Grimoire, F::Wind,
                        1.2f, 1.1f, 1.0f);
  MoveDef five = m[NSpecial];
  five.name = "Lames de Sylphe";
  five.projectile.count = 5;
  five.projectile.spreadDeg = 6;
  five.projectile.hit.damage *= 1.25f;
  c.weapons[0].moves = {{NSpecial, five}};
  c.weapons[1] = weapon("Lance de l'esprit", "Allonge du vent, la tempête devient une plongée", WeaponVisual::WindSpear,
                        F::Wind, 1.2f, 1.08f, 1.45f);
  std::vector<HitboxDef> dive;
  multi(dive, 8, 3, 4, 50, -4, 30, 2.5f, L::ArmFront, F::Wind);
  dive.push_back(imp(hb(20, 26, 60, -6, 36, 9, 40, 60, 86, L::ArmFront, 3, F::Wind)));
  MoveDef spirit = mv("Plongée de l'esprit", 46, dive);
  spirit.vel = {ve(8, VelMode::SetX, 16, 0), ve(8, VelMode::SetY, 0, -1, 0, true), ve(26, VelMode::Scale, 0.2f, 1.0f)};
  spirit.gravityMul = 0.3f;
  spirit.frictionMul = 0.1f;
  spirit.pose = PoseHint::Thrust;
  sc(spirit, S, P);
  c.weapons[1].moves = {{SSpecial, spirit}};
  return c;
}

// ---------------------------------------------------------------------------------------------
// Bleach
// ---------------------------------------------------------------------------------------------
CharacterDef makeIchigo() {
  CharacterDef c;
  c.name = "ICHIGO";
  c.title = "Le shinigami remplaçant";
  c.series = "Bleach";
  c.hero = Hero::Ichigo;
  c.style = BodyStyle::Knight;
  c.baseWeapon = WeaponVisual::Zangetsu;
  c.palettes = {pal({32, 32, 38}, {236, 236, 240}, {200, 50, 40}, {242, 206, 170}, {240, 128, 40}, {120, 80, 50}),
                pal({240, 240, 244}, {32, 32, 38}, {200, 30, 30}, {232, 228, 226}, {240, 128, 40}, {230, 200, 40})};
  stats(c, 46, 104, 100, 4.1f, 8.9f, 5.1f, 0.64f, 10.2f, 15.0f, 15.4f, 10.2f, 14.0f, 4);
  c.weaponLength = 88;
  const float S = 1.08f, P = 1.06f;  // v6: toned down, he was far too strong
  buildSwordNormals(c, S, P, F::Reiatsu);
  auto& m = c.moves;

  m[NSpecial] = mv("Getsuga Tenshō", 58, {});  // long end lag: no spamming it
  m[NSpecial].projectileFrame = 20;
  m[NSpecial].projectile = proj({52, -10}, {12, 0}, 0, 34, 28, 9.5f, 40, 72, F::Reiatsu, PV::Getsuga);
  m[NSpecial].projectile.piercing = true;
  m[NSpecial].projectile.grow = 0.3f;
  m[NSpecial].pose = PoseHint::Overhead;

  m[SSpecial] = mv("Shunpo", 44, {hb(12, 16, 56, -6, 30, 8.5f, 40, 48, 74, L::Weapon, 0, F::Slash)});
  m[SSpecial].intangibleStart = 4;
  m[SSpecial].intangibleEnd = 12;
  m[SSpecial].vel = {ve(4, VelMode::SetX, 20, 0), ve(4, VelMode::SetY, 0, 0, 0, true), ve(12, VelMode::Scale, 0.15f, 1.0f)};
  m[SSpecial].gravityMul = 0.2f;
  m[SSpecial].frictionMul = 0.05f;
  m[SSpecial].pose = PoseHint::Thrust;

  m[USpecial] = mv("Tsukiyomi", 44, {hb(4, 7, 36, -30, 30, 11, 80, 50, 72, L::Weapon, 0, F::Reiatsu),
                                     hb(7, 14, 20, -64, 26, 6, 85, 40, 60, L::Weapon, 1, F::Reiatsu)});
  m[USpecial].vel = {ve(4, VelMode::Set, 1.5f, -17.5f, 3.0f)};
  m[USpecial].gravityMul = 0.5f;
  m[USpecial].helplessAfter = true;
  m[USpecial].pose = PoseHint::Uppercut;

  BeamDef cero;
  cero.start = 22;
  cero.end = 34;
  cero.from = {36, -14};
  cero.length = 230;
  cero.width = 40;
  cero.fx = F::Blood;
  m[DSpecial] = mv("Cero", 56, beamHits(cero, 7, 30, 46, 74, 3));
  m[DSpecial].beam = cero;
  m[DSpecial].gravityMul = 0.3f;
  m[DSpecial].pose = PoseHint::Cast;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  c.weapons[0] = weapon("Zangetsu (Shikai)", "Getsuga Tenshō géant", WeaponVisual::Zangetsu, F::Reiatsu, 1.15f, 1.06f,
                        1.12f);
  MoveDef huge = m[NSpecial];
  huge.name = "Getsuga Tenshō !";
  huge.projectile.radius *= 1.4f;
  huge.projectile.hit.radius = huge.projectile.radius;
  huge.projectile.hit.damage *= 1.25f;
  huge.projectile.grow = 0.6f;
  huge.projectile.hit.impact = true;
  c.weapons[0].moves = {{NSpecial, huge}};
  c.weapons[1] = weapon("Tensa Zangetsu (Bankai)", "Vitesse du Bankai : Getsuga noir", WeaponVisual::TensaZangetsu,
                        F::Dark, 1.1f, 1.05f, 1.05f);
  MoveDef black = m[NSpecial];
  black.name = "Getsuga noir";
  black.projectile.velocity.x = 16;
  black.projectile.hit.fx = F::Dark;
  black.projectile.hit.damage *= 1.1f;
  black.projectileFrame = 14;
  black.duration = 46;
  MoveDef flash = m[SSpecial];
  flash.name = "Shunpo du Bankai";
  flash.hitboxes[0].damage *= 1.2f;
  flash.hitboxes[0].fx = F::Dark;
  flash.hitboxes[0].impact = true;
  c.weapons[1].moves = {{NSpecial, black}, {SSpecial, flash}};
  return c;
}

CharacterDef makeByakuya() {
  CharacterDef c;
  c.name = "BYAKUYA";
  c.title = "Le noble aux mille lames";
  c.series = "Bleach";
  c.hero = Hero::Byakuya;
  c.style = BodyStyle::Knight;
  c.baseWeapon = WeaponVisual::Katana;
  c.palettes = {pal({236, 236, 240}, {32, 32, 38}, {240, 150, 180}, {240, 210, 184}, {26, 26, 32}, {90, 90, 110}),
                pal({40, 40, 48}, {200, 200, 210}, {200, 120, 220}, {240, 210, 184}, {26, 26, 32}, {90, 90, 110})};
  stats(c, 44, 106, 92, 4.4f, 9.8f, 5.2f, 0.6f, 10.0f, 15.0f, 15.0f, 10.0f, 14.0f, 3);
  c.groundAccel = 1.4f;
  c.weaponLength = 72;
  const float S = 1.02f, P = 1.0f;
  buildSwordNormals(c, S, P, F::Petal);
  auto& m = c.moves;

  m[NSpecial] = mv("Senbonzakura", 46, {});
  m[NSpecial].projectileFrame = 16;
  m[NSpecial].projectile = proj({44, -12}, {5.5f, 0}, 0, 72, 32, 2.0f, 8, 6, F::Petal, PV::Petals);
  m[NSpecial].projectile.piercing = true;
  m[NSpecial].projectile.rehit = 6;
  m[NSpecial].pose = PoseHint::Raise;

  m[SSpecial] = mv("Senka", 40, {imp(hb(14, 18, 50, -8, 30, 11, 50, 55, 80, L::Weapon, 0, F::Slash))});
  m[SSpecial].teleportFrame = 10;
  m[SSpecial].teleportDist = 150;
  m[SSpecial].teleportForward = true;
  m[SSpecial].turnAfterTeleport = true;
  m[SSpecial].intangibleStart = 6;
  m[SSpecial].intangibleEnd = 14;
  m[SSpecial].vel = {ve(2, VelMode::Set, 0, 0)};
  m[SSpecial].gravityMul = 0.2f;
  m[SSpecial].pose = PoseHint::Thrust;

  m[USpecial] = mv("Shunpo céleste", 40, {hb(22, 26, 0, -6, 40, 8, 80, 55, 75, L::Body, 0, F::Petal)});
  m[USpecial].teleportFrame = 20;
  m[USpecial].teleportDist = 260;
  m[USpecial].intangibleStart = 6;
  m[USpecial].intangibleEnd = 22;
  m[USpecial].vel = {ve(2, VelMode::Set, 0, 0)};
  m[USpecial].gravityMul = 0.0f;
  m[USpecial].allowDrift = false;
  m[USpecial].helplessAfter = true;

  m[DSpecial] = mv("Danku", 52, {imp(hb(26, 30, 52, -6, 48, 12, 361, 60, 92, L::Body, 0, F::Petal))});
  m[DSpecial].counterStart = 4;
  m[DSpecial].counterEnd = 26;
  m[DSpecial].reflects = true;
  m[DSpecial].gravityMul = 0.3f;
  m[DSpecial].pose = PoseHint::HandSign;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  c.weapons[0] = weapon("Senbonzakura (Shikai)", "La nuée de pétales grossit", WeaponVisual::Senbonzakura, F::Petal,
                        1.2f, 1.08f, 1.2f);
  MoveDef swarm = m[NSpecial];
  swarm.name = "Chire, Senbonzakura";
  swarm.projectile.radius *= 1.4f;
  swarm.projectile.hit.radius = swarm.projectile.radius;
  swarm.projectile.lifetime = 90;
  swarm.projectile.hit.damage *= 1.25f;
  c.weapons[0].moves = {{NSpecial, swarm}};
  c.weapons[1] = weapon("Kageyoshi (Bankai)", "Tempête de lames autour de lui", WeaponVisual::Kageyoshi, F::Petal,
                        1.25f, 1.12f, 1.3f);
  std::vector<HitboxDef> senkei;
  multi(senkei, 8, 5, 5, 0, -8, 74, 2.2f, L::Body, F::Petal);
  senkei.push_back(imp(hb(34, 38, 0, -8, 84, 9, 65, 65, 100, L::Body, 5, F::Petal)));
  MoveDef bankai = mv("Senkei", 54, senkei);
  bankai.gravityMul = 0.3f;
  bankai.pose = PoseHint::HandSign;
  sc(bankai, S, P);
  c.weapons[1].moves = {{DSpecial, bankai}};
  return c;
}

// ---------------------------------------------------------------------------------------------
// Zelda: Tears of the Kingdom
// ---------------------------------------------------------------------------------------------
CharacterDef makeLink() {
  CharacterDef c;
  c.name = "LINK";
  c.title = "Le héros d'Hyrule";
  c.series = "Zelda: Tears of the Kingdom";
  c.hero = Hero::Link;
  c.style = BodyStyle::Knight;
  c.baseWeapon = WeaponVisual::ShieldAndSword;
  c.palettes = {pal({62, 104, 170}, {120, 88, 58}, {222, 184, 90}, {244, 210, 176}, {214, 164, 92}, {70, 130, 200}),
                pal({70, 128, 70}, {120, 88, 58}, {222, 184, 90}, {244, 210, 176}, {214, 164, 92}, {70, 130, 200})};
  stats(c, 44, 98, 98, 4.2f, 8.9f, 5.1f, 0.62f, 10.2f, 15.0f, 15.2f, 10.0f, 14.2f, 4);
  c.weaponLength = 64;
  const float S = 1.0f, P = 0.92f;
  buildSwordNormals(c, S, P, F::Light);
  auto& m = c.moves;

  m[NSpecial] = mv("Arc", 40, {});
  m[NSpecial].projectileFrame = 20;
  m[NSpecial].projectile = proj({36, -14}, {15, -0.6f}, 0.14f, 60, 8, 6, 15, 50, F::Normal, PV::Arrow);
  m[NSpecial].pose = PoseHint::Bow;

  m[SSpecial] = mv("Bombe Zonaï", 38, {});
  m[SSpecial].projectileFrame = 14;
  m[SSpecial].projectile = proj({28, -26}, {6.5f, -6.5f}, 0.38f, 100, 12, 3, 10, 20, F::Fire, PV::Bomb);
  m[SSpecial].projectile.explodeRadius = 70;
  m[SSpecial].projectile.explode = hb(0, 0, 0, 0, 70, 10, 60, 50, 72, L::None, 0, F::Fire);
  m[SSpecial].pose = PoseHint::Throw;

  m[USpecial] = mv("Ascension", 44, {hb(4, 8, 0, -30, 30, 5, 85, 50, 40, L::Body, 0, F::Light)});
  m[USpecial].vel = {ve(4, VelMode::Set, 1.5f, -18.0f, 3.0f)};
  m[USpecial].intangibleStart = 4;
  m[USpecial].intangibleEnd = 18;
  m[USpecial].gravityMul = 0.45f;
  m[USpecial].helplessAfter = true;
  m[USpecial].pose = PoseHint::Raise;

  m[DSpecial] = mv("Emprise", 44, {hb(10, 18, 96, -8, 34, 0, 0, 0, 0, L::ArmFront)});
  m[DSpecial].isGrab = true;
  m[DSpecial].pose = PoseHint::Cast;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  c.weapons[0] = weapon("Épée de légende", "L'épée tire un rayon de lumière", WeaponVisual::MasterSword, F::Light, 1.25f,
                        1.1f, 1.2f);
  MoveDef beam = mv("Rayon de l'épée", 40, {});
  beam.projectileFrame = 14;
  beam.projectile = proj({50, -10}, {14, 0}, 0, 40, 20, 9, 35, 60, F::Light, PV::SwordBeam);
  beam.projectile.piercing = true;
  beam.pose = PoseHint::Overhead;
  sc(beam, S, P);
  c.weapons[0].moves = {{NSpecial, beam}};
  c.weapons[1] = weapon("Arc de lumière", "Flèches de lumière perçantes", WeaponVisual::BowOfLight, F::Light, 1.15f,
                        1.08f, 1.0f);
  MoveDef light = mv("Flèche de lumière", 38, {});
  light.projectileFrame = 16;
  light.projectile = proj({36, -14}, {20, 0}, 0, 50, 12, 13, 40, 80, F::Light, PV::LightArrow);
  light.projectile.piercing = true;
  light.projectile.hit.impact = true;
  light.pose = PoseHint::Bow;
  sc(light, S, P);
  c.weapons[1].moves = {{NSpecial, light}};
  return c;
}

CharacterDef makeGanondorf() {
  CharacterDef c;
  c.name = "GANONDORF";
  c.title = "Le Roi Démon";
  c.series = "Zelda: Tears of the Kingdom";
  c.hero = Hero::Ganondorf;
  c.style = BodyStyle::Brute;
  c.palettes = {pal({46, 42, 50}, {120, 30, 36}, {216, 176, 80}, {150, 112, 92}, {196, 58, 40}, {236, 200, 80}),
                pal({72, 32, 42}, {30, 24, 30}, {200, 60, 60}, {138, 108, 100}, {150, 40, 40}, {255, 90, 60})};
  stats(c, 64, 128, 132, 3.2f, 7.2f, 4.5f, 0.68f, 11.2f, 15.8f, 15.2f, 10.0f, 13.8f, 6);
  c.groundAccel = 0.95f;
  c.friction = 0.7f;
  c.airAccel = 0.36f;
  const float S = 1.25f, P = 1.56f;
  buildBrawlerNormals(c, S, P, F::Gloom);
  auto& m = c.moves;

  m[NSpecial] = mv("Sphère de malice", 50, {});
  m[NSpecial].projectileFrame = 20;
  m[NSpecial].projectile = proj({44, -12}, {6.5f, 0}, 0, 80, 20, 12, 40, 80, F::Gloom, PV::DarkOrb);
  m[NSpecial].projectile.grow = 0.12f;
  m[NSpecial].pose = PoseHint::Cast;

  m[SSpecial] = mv("Ruée du Roi Démon", 52, {hb(12, 28, 40, -6, 32, 12, 40, 55, 82, L::ArmFront, 0, F::Gloom)});
  m[SSpecial].armorStart = 6;
  m[SSpecial].armorEnd = 28;
  m[SSpecial].vel = {ve(12, VelMode::SetX, 12, 0), ve(30, VelMode::Scale, 0.2f, 1.0f)};
  m[SSpecial].gravityMul = 0.5f;
  m[SSpecial].frictionMul = 0.12f;
  m[SSpecial].pose = PoseHint::Rush;

  std::vector<HitboxDef> rise;
  multi(rise, 4, 3, 5, 24, -30, 30, 2.5f, L::ArmFront, F::Gloom, 0, 85);
  rise.push_back(hb(19, 23, 16, -60, 34, 6, 75, 60, 108, L::ArmFront, 3, F::Gloom));
  m[USpecial] = mv("Ascension maudite", 52, rise);
  m[USpecial].vel = {ve(4, VelMode::Set, 1.5f, -14.5f, 4.5f)};
  m[USpecial].gravityMul = 0.45f;
  m[USpecial].helplessAfter = true;
  m[USpecial].pose = PoseHint::Uppercut;

  m[DSpecial] = mv("Fléau de malice", 60, {hb(14, 60, 0, 34, 34, 11, 270, 25, 80, L::Body, 0, F::Gloom)});
  m[DSpecial].vel = {ve(2, VelMode::Set, 0, -7), ve(14, VelMode::SetY, 0, 18)};
  m[DSpecial].loopFrame = 16;
  m[DSpecial].landMove = Extra1;
  m[DSpecial].allowDrift = false;
  m[DSpecial].pose = PoseHint::Stomp;
  HitboxDef wave = imp(hb(1, 5, 60, 40, 42, 9, 55, 60, 82, L::Body, 0, F::Gloom));
  HitboxDef waveB = wave;
  waveB.offset.x = -60;
  waveB.reverse = true;
  m[Extra1] = mv("Onde de malice", 30, {wave, waveB});
  m[Extra1].pose = PoseHint::Stomp;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial, Extra1}) sc(m[id], S, P);

  c.weapons[0] = weapon("Épée du Roi Démon", "Lame de malice à grande portée", WeaponVisual::DemonKingSword, F::Gloom,
                        1.3f, 1.12f, 1.5f);
  MoveDef blade = mv("Lame de malice", 52, {imp(hb(10, 22, 70, -6, 38, 15, 40, 60, 88, L::ArmFront, 0, F::Gloom))});
  blade.armorStart = 4;
  blade.armorEnd = 22;
  blade.vel = {ve(10, VelMode::SetX, 13, 0), ve(26, VelMode::Scale, 0.2f, 1.0f)};
  blade.gravityMul = 0.5f;
  blade.frictionMul = 0.12f;
  blade.pose = PoseHint::Thrust;
  sc(blade, S, P);
  c.weapons[0].moves = {{SSpecial, blade}};
  c.weapons[1] = weapon("Lances de malice", "Trois lances de malice", WeaponVisual::GloomSpear, F::Gloom, 1.15f, 1.08f,
                        1.2f);
  MoveDef spears = mv("Lances de malice", 46, {});
  spears.projectileFrame = 16;
  spears.projectile = proj({40, -16}, {13, 0}, 0, 45, 12, 7, 30, 60, F::Gloom, PV::GloomSpear);
  spears.projectile.count = 3;
  spears.projectile.spreadDeg = 8;
  spears.pose = PoseHint::Throw;
  sc(spears, S, P);
  c.weapons[1].moves = {{NSpecial, spears}};
  return c;
}

// ---------------------------------------------------------------------------------------------
// God of War
// ---------------------------------------------------------------------------------------------
CharacterDef makeKratos() {
  CharacterDef c;
  c.name = "KRATOS";
  c.title = "Le Fantôme de Sparte";
  c.series = "God of War";
  c.hero = Hero::Kratos;
  c.style = BodyStyle::Brute;
  c.baseWeapon = WeaponVisual::GuardianShield;
  c.palettes = {pal({104, 40, 34}, {96, 72, 52}, {196, 40, 36}, {214, 210, 202}, {44, 34, 30}, {200, 160, 80}),
                pal({160, 130, 60}, {80, 60, 44}, {196, 40, 36}, {214, 210, 202}, {196, 196, 196}, {200, 160, 80})};
  stats(c, 56, 114, 118, 3.6f, 8.0f, 4.8f, 0.66f, 11.0f, 15.5f, 15.6f, 10.2f, 14.0f, 5);
  c.groundAccel = 1.1f;
  const float S = 1.14f, P = 1.34f;
  buildBrawlerNormals(c, S, P, F::Fire);
  auto& m = c.moves;

  m[NSpecial] = mv("Rage spartiate", 48, {imp(hb(14, 18, 0, -10, 66, 8, 45, 70, 60, L::Body, 0, F::Fire))});
  m[NSpecial].armorStart = 4;
  m[NSpecial].armorEnd = 18;
  m[NSpecial].pose = PoseHint::Roar;

  m[SSpecial] = mv("Charge du bouclier", 46, {hb(8, 22, 36, -8, 30, 10, 40, 55, 78, L::ArmBack, 0, F::Normal)});
  m[SSpecial].armorStart = 4;
  m[SSpecial].armorEnd = 22;
  m[SSpecial].vel = {ve(8, VelMode::SetX, 12.5f, 0), ve(24, VelMode::Scale, 0.2f, 1.0f)};
  m[SSpecial].gravityMul = 0.45f;
  m[SSpecial].frictionMul = 0.12f;
  m[SSpecial].pose = PoseHint::Rush;

  m[USpecial] = mv("Bond de Sparte", 46, {hb(5, 9, 26, -26, 28, 8, 80, 55, 70, L::ArmFront, 0, F::Fire),
                                          hb(9, 16, 20, -56, 24, 5, 85, 45, 60, L::ArmFront, 1, F::Fire)});
  m[USpecial].vel = {ve(5, VelMode::Set, 1.5f, -16.0f, 3.5f)};
  m[USpecial].gravityMul = 0.5f;
  m[USpecial].helplessAfter = true;
  m[USpecial].pose = PoseHint::Uppercut;

  m[DSpecial] = mv("Parade du bouclier", 48, {imp(hb(22, 26, 48, -8, 40, 12, 361, 60, 95, L::ArmBack, 0, F::Fire))});
  m[DSpecial].counterStart = 4;
  m[DSpecial].counterEnd = 22;
  m[DSpecial].reflects = true;
  m[DSpecial].gravityMul = 0.3f;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  c.weapons[0] = weapon("Hache Léviathan", "La hache gèle et revient dans la main", WeaponVisual::LeviathanAxe, F::Ice,
                        1.25f, 1.12f, 1.35f);
  MoveDef axe = mv("Lancer de Léviathan", 36, {});
  axe.projectileFrame = 14;
  axe.projectile = proj({30, -20}, {15, 0}, 0, 56, 18, 9, 30, 62, F::Ice, PV::Axe);
  axe.projectile.boomerang = true;
  axe.projectile.piercing = true;
  axe.pose = PoseHint::Throw;
  sc(axe, S, P);
  c.weapons[0].moves = {{NSpecial, axe}};
  c.weapons[1] = weapon("Lames du Chaos", "Chaînes de feu : allonge énorme", WeaponVisual::BladesOfChaos, F::Fire, 1.15f,
                        1.08f, 1.6f);
  std::vector<HitboxDef> whirl;
  multi(whirl, 8, 4, 5, 0, -6, 104, 2.5f, L::Weapon, F::Fire);
  whirl.push_back(imp(hb(30, 34, 0, -6, 112, 8, 55, 65, 100, L::Weapon, 4, F::Fire)));
  MoveDef chaos = mv("Fléau du Chaos", 50, whirl);
  chaos.gravityMul = 0.4f;
  chaos.pose = PoseHint::Spin;
  sc(chaos, S, P);
  c.weapons[1].moves = {{NSpecial, chaos}};
  return c;
}

// ---------------------------------------------------------------------------------------------
// Jujutsu Kaisen
// ---------------------------------------------------------------------------------------------
CharacterDef makeGojo() {
  CharacterDef c;
  c.name = "GOJO";
  c.title = "Le plus fort";
  c.series = "Jujutsu Kaisen";
  c.hero = Hero::Gojo;
  c.style = BodyStyle::Ninja;
  c.palettes = {pal({30, 32, 44}, {44, 48, 64}, {90, 170, 255}, {244, 222, 206}, {244, 244, 252}, {120, 210, 255}),
                pal({236, 236, 240}, {44, 48, 64}, {180, 110, 255}, {244, 222, 206}, {244, 244, 252}, {120, 210, 255})};
  stats(c, 44, 114, 90, 4.4f, 9.4f, 5.4f, 0.58f, 9.6f, 14.5f, 15.4f, 10.0f, 14.4f, 3);
  c.groundAccel = 1.4f;
  const float S = 1.04f, P = 1.26f;
  buildBrawlerNormals(c, S, P, F::Cursed);
  auto& m = c.moves;

  m[NSpecial] = mv("Rouge", 40, {});
  m[NSpecial].projectileFrame = 16;
  m[NSpecial].projectile = proj({40, -12}, {13, 0}, 0, 30, 15, 8, 50, 68, F::Cursed, PV::RedOrb);
  m[NSpecial].projectile.explodeRadius = 62;
  m[NSpecial].projectile.explode = hb(0, 0, 0, 0, 62, 7, 40, 60, 75, L::None, 0, F::Cursed);
  m[NSpecial].pose = PoseHint::Cast;

  m[SSpecial] = mv("Bleu", 58, {});  // long end lag: no spamming it
  m[SSpecial].projectileFrame = 16;
  m[SSpecial].projectile = proj({64, -10}, {3.2f, 0}, 0, 64, 24, 1.2f, 6, 5, F::Cursed, PV::BlueOrb);
  m[SSpecial].projectile.piercing = true;
  m[SSpecial].projectile.rehit = 9;
  m[SSpecial].projectile.pull = 1.5f;
  m[SSpecial].projectile.pullRadius = 140;
  m[SSpecial].pose = PoseHint::Raise;

  m[USpecial] = mv("Téléportation", 38, {hb(20, 24, 0, -6, 40, 7, 80, 55, 70, L::Body, 0, F::Cursed)});
  m[USpecial].teleportFrame = 18;
  m[USpecial].teleportDist = 280;
  m[USpecial].intangibleStart = 6;
  m[USpecial].intangibleEnd = 20;
  m[USpecial].vel = {ve(2, VelMode::Set, 0, 0)};
  m[USpecial].gravityMul = 0.0f;
  m[USpecial].allowDrift = false;
  m[USpecial].helplessAfter = true;

  m[DSpecial] = mv("Infini", 54, {imp(hb(30, 34, 40, -6, 52, 10, 361, 70, 90, L::Body, 0, F::Cursed))});
  m[DSpecial].counterStart = 3;
  m[DSpecial].counterEnd = 30;
  m[DSpecial].reflects = true;
  m[DSpecial].gravityMul = 0.2f;
  m[DSpecial].pose = PoseHint::HandSign;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  c.weapons[0] = weapon("Technique : Violet", "Le Rouge devient un Violet qui efface tout", WeaponVisual::HollowPurple,
                        F::Cursed, 1.1f, 1.05f, 1.0f);
  MoveDef purple = mv("Violet", 52, {});
  purple.projectileFrame = 24;
  purple.projectile = proj({50, -12}, {9, 0}, 0, 70, 40, 18, 60, 90, F::Cursed, PV::PurpleOrb);
  purple.projectile.piercing = true;
  purple.projectile.hit.impact = true;
  purple.pose = PoseHint::HandSign;
  sc(purple, S, P);
  c.weapons[0].moves = {{NSpecial, purple}};
  c.weapons[1] = weapon("Six Yeux", "Bandeau retiré : tout frappe plus fort", WeaponVisual::SixEyes, F::Cursed, 1.35f,
                        1.2f, 1.0f, 15);
  return c;
}

CharacterDef makeSukuna() {
  CharacterDef c;
  c.name = "SUKUNA";
  c.title = "Le Roi des Fléaux";
  c.series = "Jujutsu Kaisen";
  c.hero = Hero::Sukuna;
  c.style = BodyStyle::Brawler;
  c.palettes = {pal({236, 232, 226}, {30, 28, 30}, {200, 40, 48}, {242, 212, 190}, {240, 150, 170}, {220, 40, 40}),
                pal({36, 36, 44}, {120, 30, 40}, {220, 60, 60}, {242, 212, 190}, {240, 150, 170}, {220, 40, 40})};
  stats(c, 46, 104, 100, 4.3f, 9.4f, 5.3f, 0.64f, 10.4f, 15.5f, 15.6f, 10.3f, 14.4f, 3);
  c.groundAccel = 1.4f;
  const float S = 1.0f, P = 1.12f;
  buildBrawlerNormals(c, S, P, F::Blood);
  auto& m = c.moves;

  m[NSpecial] = mv("Démantèlement", 30, {});
  m[NSpecial].projectileFrame = 8;
  m[NSpecial].projectile = proj({30, -8}, {16, 0}, 0, 24, 14, 3.4f, 12, 40, F::Blood, PV::Dismantle);
  m[NSpecial].projectile.count = 2;
  m[NSpecial].projectile.spreadDeg = 10;
  m[NSpecial].projectile.piercing = true;
  m[NSpecial].pose = PoseHint::Cast;

  std::vector<HitboxDef> cleave;
  multi(cleave, 6, 3, 4, 40, -4, 30, 2.5f, L::ArmFront, F::Blood);
  cleave.push_back(imp(hb(18, 22, 48, -6, 34, 8, 45, 55, 82, L::ArmFront, 3, F::Blood)));
  m[SSpecial] = mv("Clivage", 44, cleave);
  m[SSpecial].vel = {ve(6, VelMode::SetX, 13, 0), ve(6, VelMode::SetY, 0, -1, 0, true), ve(24, VelMode::Scale, 0.2f, 1.0f)};
  m[SSpecial].gravityMul = 0.3f;
  m[SSpecial].frictionMul = 0.1f;
  m[SSpecial].pose = PoseHint::Thrust;

  m[USpecial] = mv("Envol maudit", 44, {hb(4, 7, 30, -30, 28, 9, 80, 50, 70, L::ArmFront, 0, F::Blood),
                                        hb(7, 14, 18, -62, 24, 5, 85, 40, 60, L::ArmFront, 1, F::Blood)});
  m[USpecial].vel = {ve(4, VelMode::Set, 1.5f, -16.8f, 3.0f)};
  m[USpecial].gravityMul = 0.5f;
  m[USpecial].helplessAfter = true;
  m[USpecial].pose = PoseHint::Uppercut;

  m[DSpecial] = mv("Fûga", 56, {});
  m[DSpecial].projectileFrame = 26;
  m[DSpecial].projectile = proj({36, -12}, {11, 0}, 0, 60, 15, 7, 30, 60, F::Fire, PV::FireArrow);
  m[DSpecial].projectile.explodeRadius = 92;
  m[DSpecial].projectile.explode = hb(0, 0, 0, 0, 92, 12, 50, 60, 85, L::None, 0, F::Fire);
  m[DSpecial].projectile.explode.impact = true;
  m[DSpecial].pose = PoseHint::Bow;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  c.weapons[0] = weapon("Kamutoke", "La foudre remplace le Démantèlement", WeaponVisual::Kamutoke, F::Electric, 1.15f,
                        1.08f, 1.1f);
  MoveDef bolt = mv("Kamutoke", 40, {});
  bolt.projectileFrame = 14;
  bolt.projectile = proj({40, -14}, {18, 0}, 0, 36, 20, 12, 45, 75, F::Electric, PV::Lightning);
  bolt.projectile.piercing = true;
  bolt.projectile.hit.impact = true;
  bolt.pose = PoseHint::Raise;
  sc(bolt, S, P);
  c.weapons[0].moves = {{NSpecial, bolt}};
  c.weapons[1] = weapon("Doigt de Sukuna", "Puissance maudite : tout frappe plus fort", WeaponVisual::CursedFinger,
                        F::Blood, 1.35f, 1.2f, 1.0f, 15);
  return c;
}

// ---------------------------------------------------------------------------------------------
// Bleach (2)
// ---------------------------------------------------------------------------------------------
CharacterDef makeKenpachi() {
  CharacterDef c;
  c.name = "KENPACHI";
  c.title = "Le démon du combat";
  c.series = "Bleach";
  c.hero = Hero::Kenpachi;
  c.style = BodyStyle::Brute;
  c.baseWeapon = WeaponVisual::KenpachiSword;
  c.palettes = {pal({236, 236, 240}, {32, 32, 38}, {230, 200, 60}, {222, 188, 156}, {30, 30, 36}, {60, 50, 40}),
                pal({40, 40, 48}, {200, 200, 210}, {220, 70, 50}, {222, 188, 156}, {30, 30, 36}, {60, 50, 40})};
  stats(c, 54, 124, 128, 3.7f, 8.4f, 4.8f, 0.68f, 11.0f, 15.6f, 15.4f, 10.2f, 13.8f, 5);
  c.groundAccel = 1.0f;
  c.weaponLength = 96;
  const float S = 1.18f, P = 1.05f;
  buildSwordNormals(c, S, P, F::Berserk);
  auto& m = c.moves;

  m[NSpecial] = mv("Taille du démon", 56, {imp(hb(22, 26, 70, -16, 40, 15, 361, 40, 100, L::Weapon, 0, F::Berserk))});
  m[NSpecial].chargeFrame = 10;
  m[NSpecial].maxCharge = 60;
  m[NSpecial].chargeDamageMul = 0.8f;
  m[NSpecial].pose = PoseHint::TwoHanded;

  std::vector<HitboxDef> rush;
  multi(rush, 10, 3, 5, 44, -6, 30, 3.0f, L::Weapon, F::Berserk);
  rush.push_back(imp(hb(26, 30, 56, -8, 36, 9, 40, 60, 84, L::Weapon, 3, F::Berserk)));
  m[SSpecial] = mv("Assaut sauvage", 54, rush);
  m[SSpecial].armorStart = 6;
  m[SSpecial].armorEnd = 30;
  m[SSpecial].vel = {ve(10, VelMode::SetX, 11.5f, 0), ve(32, VelMode::Scale, 0.2f, 1.0f)};
  m[SSpecial].gravityMul = 0.5f;
  m[SSpecial].frictionMul = 0.12f;
  m[SSpecial].pose = PoseHint::Rush;

  m[USpecial] = mv("Saut du démon", 48, {hb(5, 9, 30, -30, 30, 9, 80, 55, 72, L::Weapon, 0, F::Berserk),
                                         hb(9, 16, 20, -66, 28, 6, 85, 45, 64, L::Weapon, 1, F::Berserk)});
  m[USpecial].vel = {ve(5, VelMode::Set, 1.5f, -15.8f, 3.2f)};
  m[USpecial].gravityMul = 0.5f;
  m[USpecial].helplessAfter = true;
  m[USpecial].pose = PoseHint::Uppercut;

  m[DSpecial] = mv("Garde ouverte", 52, {imp(hb(24, 28, 64, -10, 50, 15, 361, 65, 100, L::Weapon, 0, F::Berserk))});
  m[DSpecial].counterStart = 4;
  m[DSpecial].counterEnd = 24;
  m[DSpecial].gravityMul = 0.3f;
  m[DSpecial].pose = PoseHint::TwoHanded;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  c.weapons[0] = weapon("Nozarashi (Shikai)", "Couperet géant : tranche tout", WeaponVisual::Nozarashi, F::Berserk, 1.3f,
                        1.12f, 1.4f);
  MoveDef cleave = m[NSpecial];
  cleave.name = "Nozarashi : Taille";
  for (auto& h : cleave.hitboxes) {
    h.radius *= 1.3f;
    h.offset.x *= 1.25f;
    h.damage *= 1.2f;
  }
  c.weapons[0].moves = {{NSpecial, cleave}};
  c.weapons[1] = weapon("Bankai", "Le démon rouge se réveille", WeaponVisual::RedBankai, F::Blood, 1.35f, 1.15f, 1.3f, 15);
  MoveDef red = m[SSpecial];
  red.name = "Ruée du démon rouge";
  for (auto& h : red.hitboxes) h.fx = F::Blood;
  red.vel = {ve(8, VelMode::SetX, 14.0f, 0), ve(30, VelMode::Scale, 0.2f, 1.0f)};
  c.weapons[1].moves = {{SSpecial, red}};
  return c;
}

CharacterDef makeUlquiorra() {
  CharacterDef c;
  c.name = "ULQUIORRA";
  c.title = "Le 4e Espada";
  c.series = "Bleach";
  c.hero = Hero::Ulquiorra;
  c.style = BodyStyle::Ninja;
  c.palettes = {pal({236, 236, 232}, {40, 40, 44}, {90, 200, 120}, {232, 232, 228}, {24, 24, 30}, {60, 200, 110}),
                pal({40, 40, 46}, {200, 200, 196}, {120, 230, 160}, {232, 232, 228}, {24, 24, 30}, {60, 200, 110})};
  stats(c, 44, 106, 86, 4.2f, 9.0f, 5.4f, 0.5f, 8.8f, 13.6f, 14.6f, 9.6f, 13.2f, 3);
  c.airJumps = 2;
  c.airAccel = 0.5f;
  const float S = 1.0f, P = 1.39f;
  buildBrawlerNormals(c, S, P, F::Hollow);
  auto& m = c.moves;

  BeamDef cero;
  cero.start = 18;
  cero.end = 30;
  cero.from = {32, -14};
  cero.length = 270;
  cero.width = 34;
  cero.fx = F::Hollow;
  m[NSpecial] = mv("Cero", 48, beamHits(cero, 9, 25, 45, 80, 2));
  m[NSpecial].beam = cero;
  m[NSpecial].gravityMul = 0.3f;
  m[NSpecial].pose = PoseHint::Point;

  m[SSpecial] = mv("Sonído", 40, {imp(hb(14, 18, 46, -8, 30, 10, 45, 50, 78, L::ArmFront, 0, F::Hollow))});
  m[SSpecial].teleportFrame = 8;
  m[SSpecial].teleportDist = 150;
  m[SSpecial].teleportForward = true;
  m[SSpecial].intangibleStart = 4;
  m[SSpecial].intangibleEnd = 12;
  m[SSpecial].vel = {ve(2, VelMode::Set, 0, 0)};
  m[SSpecial].gravityMul = 0.2f;
  m[SSpecial].pose = PoseHint::Thrust;

  m[USpecial] = mv("Sonído céleste", 38, {hb(20, 24, 0, -6, 38, 7, 80, 55, 70, L::Body, 0, F::Hollow)});
  m[USpecial].teleportFrame = 18;
  m[USpecial].teleportDist = 270;
  m[USpecial].intangibleStart = 6;
  m[USpecial].intangibleEnd = 20;
  m[USpecial].vel = {ve(2, VelMode::Set, 0, 0)};
  m[USpecial].gravityMul = 0.0f;
  m[USpecial].allowDrift = false;
  m[USpecial].helplessAfter = true;

  m[DSpecial] = mv("Bala", 30, {});
  m[DSpecial].projectileFrame = 8;
  m[DSpecial].projectile = proj({34, -10}, {17, 0}, 0, 26, 10, 3.5f, 14, 30, F::Hollow, PV::CeroOscuras);
  m[DSpecial].projectile.count = 2;
  m[DSpecial].projectile.spreadDeg = 6;
  m[DSpecial].pose = PoseHint::Point;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  c.weapons[0] = weapon("Murciélago (Resurrección)", "Lances de lumière verte", WeaponVisual::LuzDeLaLuna, F::Hollow,
                        1.2f, 1.08f, 1.15f);
  MoveDef luz = mv("Luz de la Luna", 40, {});
  luz.projectileFrame = 14;
  luz.projectile = proj({40, -14}, {16, 0}, 0, 40, 14, 10, 40, 74, F::Hollow, PV::LuzDeLaLuna);
  luz.projectile.piercing = true;
  luz.pose = PoseHint::Throw;
  sc(luz, S, P);
  c.weapons[0].moves = {{NSpecial, luz}};
  c.weapons[1] = weapon("Segunda Etapa", "La forme ultime de l'Espada", WeaponVisual::SegundaCore, F::Hollow, 1.35f, 1.18f,
                        1.2f, 15);
  return c;
}

// Grimmjow: a brawler with claws of reiatsu; his katana Pantera stays at his side until he picks it up.
CharacterDef makeGrimmjow() {
  CharacterDef c;
  c.name = "GRIMMJOW";
  c.title = "Le 6e Espada";
  c.series = "Bleach";
  c.hero = Hero::Grimmjow;
  c.style = BodyStyle::Brawler;
  c.baseWeapon = WeaponVisual::Pantera;
  c.palettes = {pal({240, 238, 232}, {34, 34, 40}, {60, 196, 220}, {236, 200, 172}, {104, 196, 232}, {50, 170, 230}),
                pal({40, 40, 48}, {206, 204, 212}, {230, 70, 70}, {236, 200, 172}, {104, 196, 232}, {50, 170, 230})};
  stats(c, 46, 106, 102, 4.5f, 10.0f, 5.6f, 0.66f, 10.6f, 15.8f, 15.8f, 10.4f, 14.6f, 3);
  c.groundAccel = 1.5f;
  c.weaponLength = 72;
  const float S = 1.04f, P = 1.20f;  // v10: hits harder, he fell to 33% after the clip refit
  buildBrawlerNormals(c, S, P, F::Claw);
  auto& m = c.moves;

  m[NSpecial] = mv("Cero", 44, {});
  m[NSpecial].projectileFrame = 16;
  m[NSpecial].projectile = proj({40, -12}, {12, 0}, 0, 36, 18, 9, 40, 70, F::Blood, PV::Cero);
  m[NSpecial].pose = PoseHint::Point;

  m[SSpecial] = mv("Desgarrón", 46, {});
  m[SSpecial].projectileFrame = 12;
  m[SSpecial].projectile = proj({40, -8}, {9, 0}, 0, 22, 30, 2.6f, 14, 10, F::Claw, PV::Desgarron);
  m[SSpecial].projectile.piercing = true;
  m[SSpecial].projectile.rehit = 5;
  m[SSpecial].projectile.explodeRadius = 52;
  m[SSpecial].projectile.explode = hb(0, 0, 0, 0, 52, 7, 45, 50, 80, L::None, 0, F::Claw);
  m[SSpecial].pose = PoseHint::Throw;

  m[USpecial] = mv("Sonído griffu", 44, {hb(4, 7, 30, -30, 30, 9, 80, 50, 72, L::ArmFront, 0, F::Claw),
                                         hb(7, 14, 18, -62, 26, 6, 85, 40, 60, L::ArmFront, 1, F::Claw)});
  m[USpecial].vel = {ve(4, VelMode::Set, 1.5f, -17.4f, 3.2f)};
  m[USpecial].gravityMul = 0.5f;
  m[USpecial].helplessAfter = true;
  m[USpecial].pose = PoseHint::Uppercut;

  m[DSpecial] = mv("Garra de la Pantera", 48, {});
  m[DSpecial].projectileFrame = 18;
  m[DSpecial].projectile = proj({34, -10}, {15, 0}, 0, 26, 9, 3.4f, 16, 34, F::Claw, PV::PanteraDart);
  m[DSpecial].projectile.count = 5;
  m[DSpecial].projectile.spreadDeg = 9;
  m[DSpecial].pose = PoseHint::Cast;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  c.weapons[0] = weapon("Pantera (zanpakutō)", "Il dégaine : l'ultime libère la panthère", WeaponVisual::Pantera, F::Claw,
                        1.2f, 1.08f, 1.15f);
  MoveDef rush = mv("Charge de Pantera", 46, {imp(hb(10, 22, 44, -6, 30, 11, 40, 55, 82, L::Weapon, 0, F::Claw))});
  rush.vel = {ve(10, VelMode::SetX, 14, 0), ve(10, VelMode::SetY, 0, -1, 0, true), ve(24, VelMode::Scale, 0.2f, 1.0f)};
  rush.gravityMul = 0.3f;
  rush.frictionMul = 0.1f;
  rush.pose = PoseHint::Thrust;
  sc(rush, S, P);
  c.weapons[0].moves = {{SSpecial, rush}};
  c.weapons[1] = weapon("Griffes de Pantera", "Desgarrón : cinq griffes géantes", WeaponVisual::PanteraClaws, F::Claw, 1.25f,
                        1.12f, 1.2f, 15);
  MoveDef big = m[SSpecial];
  big.name = "Desgarrón géant";
  big.projectile.radius *= 1.6f;
  big.projectile.hit.radius = big.projectile.radius;
  big.projectile.hit.damage *= 1.3f;
  big.projectile.lifetime = 30;
  big.projectile.explodeRadius *= 1.4f;
  big.projectile.explode.radius = big.projectile.explodeRadius;
  big.projectile.explode.damage *= 1.3f;
  big.projectile.explode.impact = true;
  c.weapons[1].moves = {{SSpecial, big}};
  return c;
}

// Starrk: the lazy Primera, ceros and wolves of cero; Lilynette lets him release Los Lobos.
CharacterDef makeStarrk() {
  CharacterDef c;
  c.name = "STARRK";
  c.title = "Le 1er Espada";
  c.series = "Bleach";
  c.hero = Hero::Starrk;
  c.style = BodyStyle::Ninja;
  c.palettes = {pal({238, 236, 230}, {36, 36, 42}, {120, 180, 255}, {232, 198, 170}, {86, 62, 48}, {120, 140, 170}),
                pal({70, 72, 82}, {230, 228, 220}, {170, 210, 255}, {232, 198, 170}, {86, 62, 48}, {120, 140, 170})};
  stats(c, 46, 112, 106, 4.0f, 8.9f, 5.3f, 0.62f, 10.0f, 14.8f, 15.4f, 10.0f, 13.8f, 4);
  c.airJumps = 2;
  c.airAccel = 0.5f;
  const float S = 1.08f, P = 1.2f;
  buildBrawlerNormals(c, S, P, F::Wolf);
  auto& m = c.moves;

  BeamDef cero;
  cero.start = 16;
  cero.end = 28;
  cero.from = {34, -14};
  cero.length = 300;
  cero.width = 46;
  cero.fx = F::Wolf;
  m[NSpecial] = mv("Cero", 46, beamHits(cero, 11, 30, 50, 86, 2));
  m[NSpecial].beam = cero;
  m[NSpecial].gravityMul = 0.3f;
  m[NSpecial].pose = PoseHint::Point;

  m[SSpecial] = mv("Los Lobos", 46, {});
  m[SSpecial].projectileFrame = 14;
  m[SSpecial].projectileRepeat = 1;
  m[SSpecial].projectileEvery = 9;
  m[SSpecial].projectile = proj({30, 30}, {8.5f, 0}, 0, 60, 16, 3, 10, 10, F::Wolf, PV::CeroWolf);
  m[SSpecial].projectile.explodeRadius = 48;
  m[SSpecial].projectile.explode = hb(0, 0, 0, 0, 48, 9, 45, 48, 76, L::None, 0, F::Wolf);
  m[SSpecial].pose = PoseHint::Point;

  m[USpecial] = mv("Sonído", 38, {hb(20, 24, 0, -6, 38, 7, 80, 55, 70, L::Body, 0, F::Wolf)});
  m[USpecial].teleportFrame = 18;
  m[USpecial].teleportDist = 270;
  m[USpecial].intangibleStart = 6;
  m[USpecial].intangibleEnd = 20;
  m[USpecial].vel = {ve(2, VelMode::Set, 0, 0)};
  m[USpecial].gravityMul = 0.0f;
  m[USpecial].allowDrift = false;
  m[USpecial].helplessAfter = true;

  m[DSpecial] = mv("Rafale de ceros", 52, {});
  m[DSpecial].projectileFrame = 14;
  m[DSpecial].projectileRepeat = 6;
  m[DSpecial].projectileEvery = 3;
  m[DSpecial].projectile = proj({36, -12}, {17, 0}, 0, 24, 9, 1.7f, 8, 14, F::Wolf, PV::StarrkCero);
  m[DSpecial].projectile.jitter = {0, 14};
  m[DSpecial].gravityMul = 0.4f;
  m[DSpecial].pose = PoseHint::Cast;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  c.weapons[0] = weapon("Los Lobos (la meute)", "Trois loups de cero à la fois", WeaponVisual::LobosPack, F::Wolf, 1.2f, 1.08f,
                        1.0f, 15);
  MoveDef pack = m[SSpecial];
  pack.name = "La meute";
  pack.projectileRepeat = 2;
  pack.projectileEvery = 7;
  pack.projectile.explodeRadius *= 1.3f;
  pack.projectile.explode.radius = pack.projectile.explodeRadius;
  pack.projectile.explode.damage *= 1.2f;
  c.weapons[0].moves = {{SSpecial, pack}};
  c.weapons[1] = weapon("Lilynette", "L'autre moitié de son âme : l'ultime le libère", WeaponVisual::Lilynette, F::Wolf, 1.25f,
                        1.1f, 1.0f, 15);
  MoveDef twin = m[NSpecial];
  twin.name = "Cero de Lilynette";
  twin.beam.width *= 1.4f;
  for (auto& h : twin.hitboxes) {
    h.radius *= 1.3f;
    h.damage *= 1.2f;
  }
  c.weapons[1].moves = {{NSpecial, twin}};
  return c;
}

// ---------------------------------------------------------------------------------------------
// Naruto
// ---------------------------------------------------------------------------------------------
CharacterDef makeNaruto() {
  CharacterDef c;
  c.name = "NARUTO";
  c.title = "Le ninja de Konoha";
  c.series = "Naruto";
  c.hero = Hero::Naruto;
  c.style = BodyStyle::Speedster;
  c.palettes = {pal({240, 128, 30}, {30, 34, 46}, {60, 100, 200}, {244, 208, 170}, {250, 210, 70}, {70, 140, 230}),
                pal({30, 34, 46}, {240, 128, 30}, {200, 60, 40}, {244, 208, 170}, {250, 210, 70}, {70, 140, 230})};
  stats(c, 44, 96, 92, 4.5f, 9.8f, 5.6f, 0.6f, 10.0f, 15.0f, 15.6f, 10.4f, 14.6f, 3);
  c.groundAccel = 1.5f;
  c.airJumps = 2;
  const float S = 0.98f, P = 1.07f;
  buildBrawlerNormals(c, S, P, F::Chakra);
  auto& m = c.moves;

  m[NSpecial] = mv("Rasengan", 46, {imp(hb(18, 23, 44, -6, 28, 11, 40, 50, 86, L::ArmFront, 0, F::Chakra))});
  m[NSpecial].chargeFrame = 10;
  m[NSpecial].maxCharge = 50;
  m[NSpecial].chargeDamageMul = 0.7f;
  m[NSpecial].vel = {ve(16, VelMode::SetX, 6.0f, 0)};
  m[NSpecial].pose = PoseHint::Cast;

  m[SSpecial] = mv("Multi-clonage", 40, {});
  m[SSpecial].projectileFrame = 10;
  m[SSpecial].projectile = proj({30, 0}, {9, 0}, 0, 40, 24, 7, 40, 60, F::Chakra, PV::ShadowClone);
  m[SSpecial].pose = PoseHint::HandSign;

  m[USpecial] = mv("Clone tremplin", 44, {hb(6, 12, 0, 40, 30, 6, 80, 50, 60, L::Body, 0, F::Chakra)});
  m[USpecial].vel = {ve(6, VelMode::Set, 1.5f, -17.8f, 3.5f)};
  m[USpecial].gravityMul = 0.5f;
  m[USpecial].helplessAfter = true;
  m[USpecial].pose = PoseHint::Uppercut;

  m[DSpecial] = mv("Substitution", 50, {imp(hb(30, 34, 40, -6, 34, 10, 50, 55, 85, L::LegFront, 0, F::Chakra))});
  m[DSpecial].counterStart = 3;
  m[DSpecial].counterEnd = 22;
  m[DSpecial].teleportFrame = 23;
  m[DSpecial].teleportDist = 110;
  m[DSpecial].teleportForward = true;
  m[DSpecial].turnAfterTeleport = true;
  m[DSpecial].gravityMul = 0.3f;
  m[DSpecial].pose = PoseHint::HandSign;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  c.weapons[0] = weapon("Mode Ermite", "Rasengan géant (Ōdama)", WeaponVisual::SageScroll, F::Chakra, 1.25f, 1.1f, 1.1f);
  MoveDef odama = m[NSpecial];
  odama.name = "Ōdama Rasengan";
  for (auto& h : odama.hitboxes) {
    h.radius *= 1.5f;
    h.offset.x *= 1.2f;
    h.damage *= 1.3f;
  }
  c.weapons[0].moves = {{NSpecial, odama}};
  c.weapons[1] = weapon("Chakra de Kurama", "Le manteau de chakra du renard", WeaponVisual::KuramaChakra, F::Chakra, 1.3f,
                        1.15f, 1.2f, 15);
  MoveDef clones = m[SSpecial];
  clones.name = "Clones de Kurama";
  clones.projectile.count = 2;
  clones.projectile.spreadDeg = 8;
  clones.projectile.hit.damage *= 1.25f;
  c.weapons[1].moves = {{SSpecial, clones}};
  return c;
}

CharacterDef makeSasuke() {
  CharacterDef c;
  c.name = "SASUKE";
  c.title = "Le dernier Uchiha";
  c.series = "Naruto";
  c.hero = Hero::Sasuke;
  c.style = BodyStyle::Ninja;
  c.baseWeapon = WeaponVisual::Kusanagi;
  c.palettes = {pal({228, 228, 236}, {60, 50, 100}, {130, 90, 200}, {240, 214, 190}, {30, 30, 44}, {200, 30, 40}),
                pal({30, 30, 40}, {200, 196, 210}, {90, 140, 230}, {240, 214, 190}, {30, 30, 44}, {200, 30, 40})};
  stats(c, 44, 102, 90, 4.4f, 9.7f, 5.4f, 0.6f, 9.8f, 14.8f, 15.2f, 10.0f, 14.2f, 3);
  c.groundAccel = 1.45f;
  c.weaponLength = 70;
  const float S = 1.0f, P = 0.82f;
  buildSwordNormals(c, S, P, F::Lightning);
  auto& m = c.moves;

  m[NSpecial] = mv("Katon : Boule de feu", 48, {});
  m[NSpecial].projectileFrame = 18;
  m[NSpecial].projectile = proj({40, -16}, {7.5f, 0}, 0, 64, 24, 10, 40, 72, F::Fire, PV::GreatFireball);
  m[NSpecial].projectile.grow = 0.15f;
  m[NSpecial].pose = PoseHint::HandSign;

  std::vector<HitboxDef> chidori;
  multi(chidori, 10, 3, 4, 40, -6, 28, 2.5f, L::ArmBack, F::Lightning);
  chidori.push_back(imp(hb(22, 26, 46, -6, 32, 9, 40, 58, 84, L::ArmBack, 3, F::Lightning)));
  m[SSpecial] = mv("Chidori", 48, chidori);
  m[SSpecial].vel = {ve(10, VelMode::SetX, 15, 0), ve(10, VelMode::SetY, 0, -1, 0, true), ve(26, VelMode::Scale, 0.2f, 1.0f)};
  m[SSpecial].gravityMul = 0.3f;
  m[SSpecial].frictionMul = 0.1f;
  m[SSpecial].pose = PoseHint::Rush;

  std::vector<HitboxDef> hawk;
  multi(hawk, 6, 3, 5, 0, -40, 32, 2.5f, L::Body, F::Wind, 0, 85);
  hawk.push_back(hb(21, 25, 0, -50, 36, 5, 80, 55, 90, L::Body, 3, F::Wind));
  m[USpecial] = mv("Garuda", 48, hawk);
  m[USpecial].vel = {ve(4, VelMode::Set, 1.5f, -16.5f, 4.0f)};
  m[USpecial].gravityMul = 0.45f;
  m[USpecial].helplessAfter = true;
  m[USpecial].pose = PoseHint::Raise;

  std::vector<HitboxDef> flow;
  multi(flow, 8, 3, 5, 0, -6, 54, 2.5f, L::Body, F::Lightning);
  flow.push_back(imp(hb(25, 29, 0, -6, 62, 7, 70, 60, 92, L::Body, 3, F::Lightning)));
  m[DSpecial] = mv("Chidori Nagashi", 46, flow);
  m[DSpecial].armorStart = 6;
  m[DSpecial].armorEnd = 28;
  m[DSpecial].gravityMul = 0.3f;
  m[DSpecial].pose = PoseHint::Roar;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  c.weapons[0] = weapon("Arc de Susanoo", "Flèches de flammes violettes", WeaponVisual::SusanooBow, F::Dark, 1.2f, 1.08f,
                        1.0f);
  MoveDef arrow = mv("Flèche de Susanoo", 40, {});
  arrow.projectileFrame = 18;
  arrow.projectile = proj({36, -14}, {19, 0}, 0, 48, 14, 12, 40, 78, F::Dark, PV::IndraArrow);
  arrow.projectile.piercing = true;
  arrow.pose = PoseHint::Bow;
  sc(arrow, S, P);
  c.weapons[0].moves = {{NSpecial, arrow}};
  c.weapons[1] = weapon("Mangekyō Sharingan", "Amaterasu : des flammes noires sur l'ennemi", WeaponVisual::Mangekyo,
                        F::BlackFlame, 1.25f, 1.1f, 1.0f, 15);
  MoveDef ama = mv("Amaterasu", 50, {});
  ama.projectileFrame = 20;
  ama.projectile = proj({0, 0}, {0, 0}, 0, 70, 30, 2.2f, 8, 6, F::BlackFlame, PV::BlackFlames);
  ama.projectile.target = ProjTarget::Nearest;
  ama.projectile.armDelay = 22;
  ama.projectile.piercing = true;
  ama.projectile.rehit = 8;
  ama.projectile.ignoreGround = true;
  ama.pose = PoseHint::HandSign;
  c.weapons[1].moves = {{NSpecial, ama}};
  return c;
}

CharacterDef makeKakashi() {
  CharacterDef c;
  c.name = "KAKASHI";
  c.title = "Le ninja copieur";
  c.series = "Naruto";
  c.hero = Hero::Kakashi;
  c.style = BodyStyle::Ninja;
  c.baseWeapon = WeaponVisual::KakashiKunai;
  c.palettes = {pal({70, 100, 70}, {40, 48, 70}, {200, 200, 210}, {236, 212, 188}, {214, 216, 222}, {40, 40, 44}),
                pal({40, 48, 70}, {90, 90, 96}, {120, 170, 230}, {236, 212, 188}, {214, 216, 222}, {40, 40, 44})};
  stats(c, 44, 104, 96, 4.3f, 9.5f, 5.3f, 0.6f, 10.0f, 15.0f, 15.2f, 10.0f, 14.2f, 3);
  c.groundAccel = 1.4f;
  const float S = 1.02f, P = 1.08f;  // v10: toned down, he won 72% of the balance duels
  buildBrawlerNormals(c, S, P, F::Lightning);
  auto& m = c.moves;

  m[NSpecial] = mv("Suiton : Dragon aqueux", 50, {});
  m[NSpecial].projectileFrame = 22;
  m[NSpecial].projectile = proj({44, -10}, {7.5f, 0}, 0, 56, 28, 2.6f, 10, 8, F::Water, PV::WaterDragon);
  m[NSpecial].projectile.piercing = true;
  m[NSpecial].projectile.rehit = 8;
  m[NSpecial].pose = PoseHint::HandSign;

  m[SSpecial] = mv("Raikiri", 52, {imp(hb(22, 28, 50, -6, 30, 12, 40, 60, 90, L::ArmFront, 0, F::Lightning))});
  m[SSpecial].vel = {ve(14, VelMode::SetX, 17, 0), ve(14, VelMode::SetY, 0, -1, 0, true), ve(30, VelMode::Scale, 0.2f, 1.0f)};
  m[SSpecial].gravityMul = 0.3f;
  m[SSpecial].frictionMul = 0.1f;
  m[SSpecial].pose = PoseHint::Rush;

  m[USpecial] = mv("Shunshin", 38, {hb(20, 24, 0, -6, 38, 7, 80, 55, 70, L::Body, 0, F::Wind)});
  m[USpecial].teleportFrame = 18;
  m[USpecial].teleportDist = 260;
  m[USpecial].intangibleStart = 6;
  m[USpecial].intangibleEnd = 20;
  m[USpecial].vel = {ve(2, VelMode::Set, 0, 0)};
  m[USpecial].gravityMul = 0.0f;
  m[USpecial].allowDrift = false;
  m[USpecial].helplessAfter = true;
  m[USpecial].pose = PoseHint::HandSign;

  m[DSpecial] = mv("Ninken : Crocs", 46, {hb(12, 20, 96, 30, 34, 0, 0, 0, 0, L::ArmFront)});
  m[DSpecial].isGrab = true;
  m[DSpecial].pose = PoseHint::Kneel;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  c.weapons[0] = weapon("Sharingan (Kamui)", "Kamui : un vortex sur l'ennemi", WeaponVisual::SharinganEye, F::Dark, 1.2f,
                        1.08f, 1.0f);
  MoveDef kamui = mv("Kamui", 52, {});
  kamui.projectileFrame = 22;
  kamui.projectile = proj({0, 0}, {0, 0}, 0, 50, 34, 2.4f, 8, 6, F::Dark, PV::KamuiVortex);
  kamui.projectile.target = ProjTarget::Nearest;
  kamui.projectile.armDelay = 20;
  kamui.projectile.piercing = true;
  kamui.projectile.rehit = 8;
  kamui.projectile.pull = 1.6f;
  kamui.projectile.pullRadius = 110;
  kamui.projectile.ignoreGround = true;
  kamui.pose = PoseHint::Point;
  c.weapons[0].moves = {{NSpecial, kamui}};
  c.weapons[1] = weapon("Mangekyō double", "Le Susanoo de Kakashi", WeaponVisual::DoubleMangekyo, F::Dark, 1.3f, 1.12f,
                        1.1f, 15);
  MoveDef shuri = mv("Shuriken Kamui", 44, {});
  shuri.projectileFrame = 16;
  shuri.projectile = proj({40, -14}, {13, 0}, 0, 44, 22, 3, 12, 20, F::Dark, PV::KamuiShuriken);
  shuri.projectile.piercing = true;
  shuri.projectile.rehit = 7;
  shuri.pose = PoseHint::Throw;
  sc(shuri, S, P);
  c.weapons[1].moves = {{NSpecial, shuri}};
  return c;
}

// ---------------------------------------------------------------------------------------------
// Monster Hunter
// ---------------------------------------------------------------------------------------------
CharacterDef makeHunter() {
  CharacterDef c;
  c.name = "HUNTER";
  c.title = "Le chasseur de monstres";
  c.series = "Monster Hunter";
  c.hero = Hero::Hunter;
  c.style = BodyStyle::Knight;
  c.baseWeapon = WeaponVisual::GreatSword;
  c.palettes = {pal({176, 60, 44}, {84, 70, 60}, {226, 190, 96}, {236, 200, 170}, {120, 80, 50}, {70, 110, 70}),
                pal({70, 110, 150}, {84, 70, 60}, {200, 220, 230}, {236, 200, 170}, {50, 40, 34}, {70, 110, 70})};
  stats(c, 52, 108, 120, 3.6f, 8.2f, 4.8f, 0.66f, 10.8f, 15.4f, 15.2f, 10.0f, 13.8f, 5);
  c.groundAccel = 1.0f;
  c.weaponLength = 100;
  const float S = 1.08f, P = 1.11f;
  buildSwordNormals(c, S, P, F::Slash);
  auto& m = c.moves;

  m[NSpecial] = mv("Taille chargée", 60, {imp(hb(26, 30, 72, -10, 42, 15, 361, 42, 100, L::Weapon, 0, F::Slash))});
  m[NSpecial].chargeFrame = 12;
  m[NSpecial].maxCharge = 70;
  m[NSpecial].chargeDamageMul = 0.9f;
  m[NSpecial].pose = PoseHint::TwoHanded;

  m[SSpecial] = mv("Coup d'épaule", 44, {hb(8, 18, 36, -8, 30, 9, 40, 60, 70, L::Body, 0, F::Normal)});
  m[SSpecial].armorStart = 4;
  m[SSpecial].armorEnd = 18;
  m[SSpecial].vel = {ve(8, VelMode::SetX, 11, 0), ve(20, VelMode::Scale, 0.2f, 1.0f)};
  m[SSpecial].gravityMul = 0.5f;
  m[SSpecial].frictionMul = 0.12f;
  m[SSpecial].pose = PoseHint::Rush;

  m[USpecial] = mv("Griffe-grappin", 46, {hb(8, 16, 20, -60, 28, 7, 85, 50, 66, L::ArmFront, 0, F::Normal)});
  m[USpecial].vel = {ve(6, VelMode::Set, 2.0f, -17.0f, 3.5f)};
  m[USpecial].gravityMul = 0.5f;
  m[USpecial].helplessAfter = true;
  m[USpecial].pose = PoseHint::Raise;

  m[DSpecial] = mv("Bombe-tonneau G", 40, {});
  m[DSpecial].projectileFrame = 14;
  m[DSpecial].projectile = proj({34, 20}, {1.0f, -2.0f}, 0.5f, 120, 18, 2, 10, 10, F::Fire, PV::BarrelBomb);
  m[DSpecial].projectile.explodeRadius = 105;
  m[DSpecial].projectile.explode = hb(0, 0, 0, 0, 105, 16, 60, 60, 92, L::None, 0, F::Fire);
  m[DSpecial].projectile.explode.impact = true;
  m[DSpecial].pose = PoseHint::Kneel;
  for (int id : {NSpecial, SSpecial, USpecial, DSpecial}) sc(m[id], S, P);

  c.weapons[0] = weapon("Lame chargée", "Décharge élémentaire explosive", WeaponVisual::ChargeBlade, F::Electric, 1.2f,
                        1.1f, 1.0f);
  MoveDef ued = mv("Décharge élémentaire", 54, {imp(hb(22, 26, 60, 10, 40, 12, 60, 50, 85, L::Weapon, 0, F::Electric))});
  ued.projectileFrame = 24;
  ued.projectile = proj({90, 30}, {0, 0}, 0, 2, 10, 0, 0, 0, F::Electric, PV::Phial);
  ued.projectile.explodeRadius = 90;
  ued.projectile.explode = hb(0, 0, 0, 0, 90, 10, 60, 55, 85, L::None, 0, F::Electric);
  ued.projectile.ignoreGround = true;
  ued.pose = PoseHint::Overhead;
  sc(ued, S, P);
  c.weapons[0].moves = {{NSpecial, ued}};
  c.weapons[1] = weapon("Lancecanon", "Tir explosif à bout portant", WeaponVisual::Gunlance, F::Fire, 1.2f, 1.1f, 1.05f);
  MoveDef shell = mv("Tir de lancecanon", 40, {});
  shell.projectileFrame = 14;
  shell.projectile = proj({70, -6}, {4, 0}, 0, 6, 24, 0, 0, 0, F::Fire, PV::GunlanceShell);
  shell.projectile.explodeRadius = 80;
  shell.projectile.explode = hb(0, 0, 0, 0, 80, 11, 45, 55, 82, L::None, 0, F::Fire);
  shell.pose = PoseHint::Thrust;
  sc(shell, S, P);
  c.weapons[1].moves = {{NSpecial, shell}};
  return c;
}
// The basic attack (jab) with a picked-up weapon: one big strike, drawn by its own clip (w0_jab / w1_jab). A blade
// slashes from above down to the front, a spear or a lance thrusts, a bow looses a quick arrow, a power strikes with
// a burst of its energy. No chain: pressing again strikes again.
float pickupReach(WeaponVisual v) {  // weapon units, a 100-unit fighter (render_weapons.cpp weaponVisualLength)
  switch (v) {
    case WeaponVisual::DemonSlayer: return 100;
    case WeaponVisual::DemonDweller: return 80;
    case WeaponVisual::Zangetsu: return 90;
    case WeaponVisual::TensaZangetsu: return 82;
    case WeaponVisual::Senbonzakura: return 74;
    case WeaponVisual::Kageyoshi: return 80;
    case WeaponVisual::MasterSword: return 79;
    case WeaponVisual::DemonKingSword: return 96;
    case WeaponVisual::LeviathanAxe: return 72;
    case WeaponVisual::BladesOfChaos: return 86;
    case WeaponVisual::Nozarashi: return 110;
    case WeaponVisual::RedBankai: return 113;
    case WeaponVisual::Pantera: return 74;
    case WeaponVisual::ChargeBlade: return 82;
    default: return 0;
  }
}
bool pickupThrusts(WeaponVisual v) {
  return v == WeaponVisual::WindSpear || v == WeaponVisual::GloomSpear || v == WeaponVisual::LuzDeLaLuna ||
         v == WeaponVisual::Gunlance || v == WeaponVisual::Kamutoke;
}
float thrustReach(WeaponVisual v) {
  switch (v) {
    case WeaponVisual::WindSpear: return 98;
    case WeaponVisual::GloomSpear: return 105;
    case WeaponVisual::LuzDeLaLuna: return 104;
    case WeaponVisual::Gunlance: return 108;
    default: return 94;  // Kamutoke
  }
}

MoveDef weaponStrike(const CharacterDef& c, const WeaponDef& w) {
  const float S = c.height / 100.0f;
  const F fx = w.fx;
  const WeaponVisual v = w.visual;
  if (v == WeaponVisual::BowOfLight || v == WeaponVisual::SusanooBow) {  // a quick arrow, like the special's
    for (const auto& [id, special] : w.moves)
      if (id == NSpecial && special.projectileFrame >= 0) {
        MoveDef m = mv("Tir rapide", 32, {});
        m.projectileFrame = 11;
        m.projectile = special.projectile;
        m.projectile.hit.damage *= 0.5f;
        m.projectile.explodeRadius = 0.0f;
        m.pose = PoseHint::Bow;
        return m;
      }
  }
  MoveDef m;
  if (const float R = pickupReach(v) * S; R > 0.0f) {  // the slash: high behind the head, down to the front
    m = mv("Frappe de l'arme", 30, {hb(8, 10, R * 0.35f, -R * 0.55f, 20 * S, 7.0f, 45, 40, 85, L::Weapon, 0, fx),
                                    hb(9, 12, R * 0.7f, -R * 0.12f, 28 * S, 9.0f, 40, 40, 85, L::Weapon, 0, fx),
                                    hb(9, 12, R * 0.35f, -R * 0.05f, 22 * S, 9.0f, 40, 40, 85, L::Weapon, 0, fx)});
  } else if (pickupThrusts(v)) {  // the thrust: straight ahead, far
    const float R = thrustReach(v) * S;
    m = mv("Estoc de l'arme", 30, {hb(8, 11, R * 0.5f, -10 * S, 20 * S, 7.5f, 38, 38, 85, L::Weapon, 0, fx),
                                   hb(9, 12, R * 0.95f, -10 * S, 24 * S, 8.5f, 38, 38, 85, L::Weapon, 0, fx)});
  } else {  // a power: a burst of its energy in front of the hands
    m = mv("Frappe d'énergie", 30, {hb(8, 12, 52 * S, -10 * S, 34 * S, 8.5f, 45, 40, 80, L::ArmFront, 0, fx)});
  }
  return m;
}

void addWeaponStrikes(CharacterDef& c) {
  for (auto& w : c.weapons) w.moves.push_back({Jab, weaponStrike(c, w)});
}
}  // namespace

bool g_rawHitboxes = false;

namespace {
// One hitbox moved onto the drawing of its clip:
// fighter, weapon slot (-1: bare-handed), move, index in the move's hitboxes, the radius it was written with (the
// entry is dropped if that changed: the roster moved on since the fit), its fitted offset and radius, and for a
// blade swing its fitted angles and reach (its offset is unused).
struct HitFit {
  const char* fighter;
  int slot, move, index;
  float was, x, y, r, a0, a1, reach1;
};
constexpr HitFit kHitFit[] = {
#include "core/roster_hitfit.inc"
    {nullptr, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

MoveDef* moveOf(CharacterDef& c, int slot, int move) {
  if (slot < 0) return move >= 0 && move < MOVE_COUNT ? &c.moves[move] : nullptr;
  if (slot >= int(std::size(c.weapons))) return nullptr;
  for (auto& [id, m] : c.weapons[slot].moves)
    if (id == move) return &m;
  return nullptr;
}

void applyHitFit(std::vector<CharacterDef>& v) {
  for (const HitFit& h : kHitFit) {
    if (!h.fighter) break;
    for (auto& c : v) {
      if (c.name != h.fighter) continue;
      MoveDef* m = moveOf(c, h.slot, h.move);
      if (!m || h.index < 0 || h.index >= int(m->hitboxes.size())) continue;
      HitboxDef& x = m->hitboxes[h.index];
      if (std::fabs(x.radius - h.was) > 0.01f) continue;
      x.radius = h.r;
      if (x.swing) {
        x.a0 = h.a0;
        x.a1 = h.a1;
        x.reach1 = h.reach1;
      } else {
        x.offset = {h.x, h.y};
      }
    }
  }
}
}  // namespace

const std::vector<CharacterDef>& roster() {
  static const std::vector<CharacterDef> r = [] {
    std::vector<CharacterDef> v = {makeAsta(),      makeYuno(),     makeIchigo(),   makeByakuya(),   makeKenpachi(),
                                   makeUlquiorra(), makeGrimmjow(), makeStarrk(),   makeNaruto(),    makeSasuke(),
                                   makeKakashi(),   makeGojo(),     makeSukuna(),   makeLink(),      makeGanondorf(),
                                   makeKratos(),    makeHunter()};
    for (auto& c : v) {
      addWeaponStrikes(c);  // before the ultimates: the bows copy their special's arrow
      addUltimates(c);      // roster_ultimates.cpp
    }
    if (!g_rawHitboxes) applyHitFit(v);
    return v;
  }();
  return r;
}
