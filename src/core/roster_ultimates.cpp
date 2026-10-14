// The ultimates: three per fighter (bare-handed, then with each of its two weapons), each with its own
// cinematic (MoveDef::ultFx, drawn by render_vfx.cpp), and the transformations some of them start.
// Frames 1-30 are the cut-in freeze; the action starts around frame 34. Positions are in pixels from the
// fighter's center (facing right) and are not scaled by its size: an ultimate fills the screen.
#include "core/roster_kit.hpp"

namespace {
using namespace kit;
using VM = VelMode;

int addForm(CharacterDef& c, Form form, int seconds, F fx, float damage, float kbTaken, bool armor) {
  FormDef d;
  d.form = form;
  d.duration = seconds * 60;
  d.fx = fx;
  d.damageMul = damage;
  d.kbTakenMul = kbTaken;
  d.superArmor = armor;
  c.forms.push_back(d);
  return int(c.forms.size()) - 1;
}
// A look that only lasts as long as the ultimate that starts it (Susanoo, Vasto Lorde...): no stats.
int addLook(CharacterDef& c, Form form) { return addForm(c, form, 0, F::Normal, 1.0f, 1.0f, false); }

// slot -1: bare-handed, else the ultimate of that weapon.
void setUlt(CharacterDef& c, int slot, MoveDef m) {
  if (slot < 0) c.moves[FinalSmash] = std::move(m);
  else c.weapons[slot].moves.push_back({FinalSmash, std::move(m)});
}

// Appears in front of the nearest opponent at frame `tp` and catches it; `hits` blows while it is held
// in front of the fighter (holdAt), then `finisher` (a launcher) sends it flying.
MoveDef ambush(const char* name, int duration, UltFx fx, PoseHint pose, F e, Limb limb, int tp, Vec2 hold, int hits,
               int every, float hitDmg, HitboxDef finisher) {
  std::vector<HitboxDef> v = {cap(hb(tp, tp + 6, 46, -6, 58, 6, 0, 0, 0, limb, 0, e))};
  multi(v, tp + 9, hits, every, 50, -6, 52, hitDmg, limb, e, 1, 0);
  finisher.group = hits + 1;
  v.push_back(lau(finisher));
  MoveDef m = cine(name, duration, v, pose, fx);
  m.teleportFrame = tp;
  m.teleportToTarget = true;
  m.holdAt = hold;
  return m;
}

// A burst around the fighter when a transformation begins.
MoveDef awaken(const char* name, int form, UltFx fx, PoseHint pose, F e, float radius, float dmg) {
  MoveDef m = cine(name, 76, {imp(hb(38, 44, 0, -10, radius, dmg, 60, 52, 62, L::Body, 0, e))}, pose, fx);
  m.formIndex = form;
  m.formFrame = 34;
  return m;
}

// Strikes on every opponent (pillars of light, lightning, eruptions): a warning, then the hit.
ProjectileDef strike(float radius, int warn, int life, float dmg, float bkb, float kbg, F e, PV vis) {
  ProjectileDef p = proj({0, 0}, {0, 0}, 0, life, radius, dmg, bkb, kbg, e, vis);
  p.target = ProjTarget::All;
  p.armDelay = warn;
  p.piercing = true;
  p.ignoreGround = true;
  p.hit.angle = 80;
  return p;
}

BeamDef beamOf(int start, int end, Vec2 from, float length, float width, F e) {
  BeamDef b;
  b.start = start;
  b.end = end;
  b.from = from;
  b.length = length;
  b.width = width;
  b.fx = e;
  return b;
}

MoveDef beamUlt(const char* name, int duration, const BeamDef& b, float dmg, int hits, PoseHint pose, UltFx fx) {
  MoveDef m = cine(name, duration, beamHits(b, dmg, 30, 95, 110, hits), pose, fx);
  m.beam = b;
  return m;
}

// ------------------------------------------------------------------------------------------------
void asta(CharacterDef& c) {
  const MoveDef bigSlash = c.weapons[0].moves[0].second, divider = c.weapons[1].moves[0].second;
  // Dashes through, catches, cuts, then the black crescent.
  std::vector<HitboxDef> b = {cap(hb(34, 40, 40, -6, 64, 5, 0, 0, 0, L::Weapon, 0, F::AntiMagic))};
  multi(b, 42, 4, 4, 40, -6, 70, 3.0f, L::Weapon, F::AntiMagic, 1, 0);
  MoveDef base = cine("BLACK DIVIDER", 100, b, PoseHint::Thrust, UltFx::BlackDivider);
  base.vel = {ve(1, VM::Set, 0, -2), ve(22, VM::Set, 0, 0), ve(34, VM::SetX, 15, 0), ve(52, VM::Set, 0, 0)};
  base.holdAt = {70, -6};
  base.projectileFrame = 60;
  base.projectile = proj({60, -10}, {17, 0}, 0, 60, 112, 24, 92, 108, F::AntiMagic, PV::BlackSlash);
  base.projectile.piercing = true;
  base.projectile.hit.impact = true;
  setUlt(c, -1, base);

  // Demon Slayer: up into the sky with the victim, then down like a black meteor.
  std::vector<HitboxDef> mb = {cap(hb(32, 37, 40, -10, 60, 6, 0, 0, 0, L::Weapon, 0, F::AntiMagic))};
  multi(mb, 40, 4, 5, 40, -30, 52, 2.5f, L::Weapon, F::AntiMagic, 1, 0);
  mb.push_back(lau(hb(72, 78, 40, 20, 90, 22, 70, 94, 108, L::Weapon, 5, F::AntiMagic)));
  mb.push_back(imp(hb(76, 82, 0, 40, 170, 10, 60, 70, 80, L::Body, 6, F::AntiMagic)));  // shock wave
  MoveDef met = cine("BLACK METEORITE", 104, mb, PoseHint::Overhead, UltFx::BlackMeteorite);
  met.teleportFrame = 31;
  met.teleportToTarget = true;
  met.holdAt = {46, -40};
  met.vel = {ve(1, VM::Set, 0, -2), ve(22, VM::Set, 0, 0), ve(36, VM::Set, 0, -13), ve(58, VM::Set, 0, 0),
             ve(64, VM::Set, 0, 21), ve(78, VM::Set, 0, 0)};
  setUlt(c, 0, met);

  // Demon Dweller: the union with Liebe, black wing and horn, for a while.
  const int u = addForm(c, Form::DemonUnion, 12, F::AntiMagic, 1.3f, 0.8f, false);
  c.forms[u].moves = {{NSpecial, bigSlash}, {SSpecial, divider}};
  setUlt(c, 1, awaken("UNION DÉMONIAQUE", u, UltFx::DemonUnion, PoseHint::Roar, F::AntiMagic, 230, 14));
}

void yuno(CharacterDef& c) {
  setUlt(c, -1, cine("TEMPÊTE DE ZÉPHYR", 100, storm(34, 6, 6, 200, 360, 3, F::Wind, 18, 430, 80), PoseHint::Raise,
                     UltFx::SpiritStorm));

  // Grimoire: wind blades raining on everyone, then one huge blade from the sky.
  MoveDef rain = cine("PLUIE DE LAMES", 104, {}, PoseHint::Raise, UltFx::WindBladeShower);
  rain.projectileFrame = 36;
  rain.projectileRepeat = 11;
  rain.projectileEvery = 4;
  rain.projectile = proj({0, -440}, {0, 17}, 0, 40, 20, 2.4f, 12, 6, F::Wind, PV::WindRain);
  rain.projectile.target = ProjTarget::All;
  rain.projectile.jitter = {110, 30};
  ProjectileDef big = proj({0, -470}, {0, 26}, 0, 30, 66, 18, 92, 104, F::Wind, PV::WindRain);
  big.target = ProjTarget::All;
  big.ignoreGround = true;
  big.piercing = true;
  big.hit.angle = 80;
  big.hit.impact = true;
  rain.shots = {{88, big}};
  setUlt(c, 0, rain);

  // Spirit spear: Sylph shapes a giant spear of wind that drags everything into its path.
  MoveDef spear = cine("LANCE DE ZÉPHYR", 96, {}, PoseHint::Throw, UltFx::ZephyrSpear);
  spear.projectileFrame = 54;
  spear.projectile = proj({50, -16}, {24, 0}, 0, 60, 58, 26, 94, 110, F::Wind, PV::ZephyrSpear);
  spear.projectile.piercing = true;
  spear.projectile.ignoreGround = true;
  spear.projectile.pull = 2.5f;
  spear.projectile.pullRadius = 150;
  spear.projectile.hit.impact = true;
  setUlt(c, 1, spear);
}

void ichigo(CharacterDef& c) {
  // Shikai: Zangetsu grows into a giant blade of reiatsu, then one huge Getsuga Tensho.
  MoveDef gts = cine("GETSUGA TENSHŌ", 96, {}, PoseHint::Overhead, UltFx::GiantGetsuga);
  gts.projectileFrame = 54;
  gts.projectile = proj({70, -14}, {15, 0}, 0, 70, 96, 21, 92, 104, F::Reiatsu, PV::GiantGetsuga);
  gts.projectile.piercing = true;
  gts.projectile.ignoreGround = true;
  gts.projectile.grow = 0.6f;
  gts.projectile.hit.impact = true;
  setUlt(c, -1, gts);

  // Zangetsu: the hollow mask, two Getsuga crossing (Getsuga Jujisho).
  const int mask = addLook(c, Form::HollowMask);
  MoveDef cross = cine("GETSUGA JŪJISHŌ", 100, {}, PoseHint::Overhead, UltFx::GetsugaJujisho);
  cross.formIndex = mask;
  cross.formFrame = 22;
  cross.projectileFrame = 58;
  cross.projectile = proj({70, -14}, {14, 0}, 0, 70, 100, 23, 94, 106, F::Dark, PV::GetsugaCross);
  cross.projectile.piercing = true;
  cross.projectile.ignoreGround = true;
  cross.projectile.grow = 0.5f;
  cross.projectile.hit.impact = true;
  setUlt(c, 0, cross);

  // Tensa Zangetsu: the hollow takes over, catches the opponent and fires a cero point-blank.
  const int vl = addLook(c, Form::VastoLorde);
  std::vector<HitboxDef> vb = {cap(hb(35, 40, 44, -6, 56, 6, 0, 0, 0, L::ArmFront, 0, F::Blood))};
  multi(vb, 42, 2, 6, 44, -6, 50, 4, L::ArmFront, F::Blood, 1, 0);
  const BeamDef cero = beamOf(56, 80, {36, -14}, 900, 172, F::Blood);
  std::vector<HitboxDef> ch = beamHits(cero, 22, 30, 95, 110, 4, 3);
  launchLast(ch);
  vb.insert(vb.end(), ch.begin(), ch.end());
  MoveDef vasto = cine("VASTO LORDE : CERO", 104, vb, PoseHint::Point, UltFx::VastoLordeCero);
  vasto.beam = cero;
  vasto.teleportFrame = 34;
  vasto.teleportToTarget = true;
  vasto.holdAt = {60, -6};
  vasto.formIndex = vl;
  vasto.formFrame = 22;
  setUlt(c, 1, vasto);
}

void byakuya(CharacterDef& c) {
  // Kageyoshi: everyone in the storm is held in place while the petals cut, then thrown out.
  std::vector<HitboxDef> st = storm(34, 9, 5, 300, 380, 2.2f, F::Petal, 16, 400, 70);
  st.front().capture = true;
  st.back().launch = true;
  MoveDef kage = cine("SENBONZAKURA KAGEYOSHI", 104, st, PoseHint::HandSign, UltFx::Kageyoshi);
  kage.holdInPlace = true;
  setUlt(c, -1, kage);

  // Senbonzakura: Gokei, a sphere of blades closes on every opponent, then bursts.
  MoveDef gokei = cine("GŌKEI", 108, {}, PoseHint::HandSign, UltFx::Gokei);
  gokei.projectileFrame = 36;
  gokei.projectile = proj({0, 0}, {0, 0}, 0, 56, 82, 2.0f, 8, 4, F::Petal, PV::PetalSphere);
  ProjectileDef& g = gokei.projectile;
  g.target = ProjTarget::All;
  g.armDelay = 22;
  g.piercing = true;
  g.rehit = 5;
  g.follow = true;
  g.pull = 3.0f;
  g.pullRadius = 160;
  g.ignoreGround = true;
  g.explodeRadius = 120;
  g.explode = imp(hb(0, 0, 0, 0, 120, 18, 75, 92, 104, L::None, 0, F::Petal));
  setUlt(c, 0, gokei);

  // Kageyoshi: Shukei Hakuteiken, the blades become white wings and a single sword of light.
  setUlt(c, 1, ambush("SHŪKEI : HAKUTEIKEN", 100, UltFx::Hakuteiken, PoseHint::Overhead, F::Petal, L::Weapon, 36,
                      {64, -8}, 3, 6, 3.5f, hb(70, 76, 70, -10, 130, 22, 50, 95, 110, L::Weapon, 0, F::Light)));
}

void link(CharacterDef& c) {
  setUlt(c, -1, beamUlt("DRAGON DE LUMIÈRE", 100, beamOf(40, 68, {24, -20}, 1100, 195, F::Light), 19, 5,
                        PoseHint::Overhead, UltFx::LightDragon));

  // Master Sword: a flurry, then the blade's beam at full power.
  MoveDef ms = ambush("ÉPÉE DE LÉGENDE", 100, UltFx::MasterSwordBeam, PoseHint::Overhead, F::Light, L::Weapon, 34,
                      {60, -6}, 4, 4, 3.0f, hb(0, 0, 0, 0, 0, 0, 0, 0, 0, L::Weapon, 0, F::Light));
  ms.hitboxes.pop_back();  // the beam is the finisher
  ms.projectileFrame = 66;
  ms.projectile = proj({50, -10}, {20, 0}, 0, 50, 80, 24, 94, 108, F::Light, PV::SwordBeam);
  ms.projectile.piercing = true;
  ms.projectile.ignoreGround = true;
  ms.projectile.hit.impact = true;
  setUlt(c, 0, ms);

  // Bow of light: pillars of light rain on everyone, then a great one.
  MoveDef rain = cine("PLUIE DE LUMIÈRE", 108, {}, PoseHint::Bow, UltFx::LightArrowRain);
  rain.projectileFrame = 36;
  rain.projectileRepeat = 4;
  rain.projectileEvery = 9;
  rain.projectile = strike(40, 16, 8, 4, 22, 12, F::Light, PV::LightPillar);
  rain.projectile.jitter = {60, 0};
  ProjectileDef big = strike(86, 20, 10, 18, 92, 104, F::Light, PV::LightPillar);
  big.hit.impact = true;
  rain.shots = {{86, big}};
  setUlt(c, 1, rain);
}

void ganondorf(CharacterDef& c) {
  setUlt(c, -1, beamUlt("DRAGON DÉMONIAQUE", 104, beamOf(46, 80, {30, -20}, 1000, 299, F::Gloom), 21, 6, PoseHint::Roar,
                        UltFx::DemonDragon));

  setUlt(c, 0, ambush("RAGE DU ROI DÉMON", 104, UltFx::DemonKingRampage, PoseHint::Overhead, F::Gloom, L::Weapon, 34,
                      {66, -4}, 4, 6, 4.0f, hb(72, 78, 70, -10, 120, 24, 60, 96, 112, L::Weapon, 0, F::Gloom)));

  // Gloom spears: gloom erupts under every opponent, three times.
  MoveDef pil = cine("PILIERS DE MALICE", 112, {}, PoseHint::Stomp, UltFx::GloomPillars);
  pil.projectileFrame = 36;
  pil.projectileRepeat = 2;
  pil.projectileEvery = 18;
  pil.projectile = strike(72, 18, 14, 5, 30, 20, F::Gloom, PV::GloomPillar);
  pil.projectile.hit.angle = 85;
  ProjectileDef big = strike(104, 18, 16, 20, 92, 106, F::Gloom, PV::GloomPillar);
  big.hit.angle = 85;
  big.hit.impact = true;
  pil.shots = {{92, big}};
  setUlt(c, 1, pil);
}

void kratos(CharacterDef& c) {
  // Spartan Rage: a roar, then fists of fire for ten seconds.
  const int r = addForm(c, Form::SpartanRage, 10, F::Fire, 1.35f, 0.75f, true);
  MoveDef blast = c.moves[NSpecial];
  blast.name = "Explosion spartiate";
  for (auto& h : blast.hitboxes) {
    h.radius *= 1.5f;
    h.damage *= 1.3f;
    h.fx = F::Fire;
  }
  c.forms[r].moves = {{NSpecial, blast}};
  setUlt(c, -1, awaken("RAGE DE SPARTE", r, UltFx::SpartanRage, PoseHint::Roar, F::Fire, 210, 12));

  // Leviathan axe: the great winter freezes everyone, then the statues shatter.
  std::vector<HitboxDef> fb = {cap(hb(34, 38, 0, -10, 640, 6, 0, 0, 0, L::None, 0, F::Ice))};
  for (int k = 0; k < 4; ++k) fb.push_back(hb(46 + k * 7, 49 + k * 7, 0, -10, 640, 2.5f, 361, 8, 0, L::None, 1 + k, F::Ice));
  fb.push_back(lau(hb(84, 88, 0, -10, 640, 18, 75, 90, 102, L::None, 5, F::Ice)));
  MoveDef fim = cine("FIMBULWINTER", 104, fb, PoseHint::Throw, UltFx::Fimbulwinter);
  fim.holdInPlace = true;
  setUlt(c, 0, fim);

  // Blades of Chaos: the chains catch from afar and drag the victim into a whirl of fire.
  std::vector<HitboxDef> cb = {cap(hb(36, 44, 200, -6, 120, 6, 0, 0, 0, L::Weapon, 0, F::Fire))};
  multi(cb, 50, 5, 5, 60, -6, 70, 3.0f, L::Weapon, F::Fire, 1, 0);
  cb.push_back(lau(hb(80, 86, 60, -6, 120, 22, 55, 95, 110, L::Weapon, 6, F::Fire)));
  MoveDef whip = cine("FOUET DU CHAOS", 104, cb, PoseHint::Spin, UltFx::ChaosWhip);
  whip.holdAt = {84, -10};
  setUlt(c, 1, whip);
}

void gojo(CharacterDef& c) {
  // Six Eyes: Infinite Void, everyone inside is paralysed by the endless information, then blown away.
  std::vector<HitboxDef> gv = {cap(hb(34, 38, 0, 0, 620, 6, 0, 0, 0, L::None, 0, F::Cursed))};
  multi(gv, 44, 4, 9, 0, 0, 640, 2.5f, L::None, F::Cursed, 1, 0);
  gv.push_back(lau(hb(90, 94, 0, 0, 660, 22, 75, 92, 106, L::None, 5, F::Cursed)));
  MoveDef iv = cine("EXTENSION : SPHÈRE DE L'INFINI", 110, gv, PoseHint::HandSign, UltFx::InfiniteVoid);
  iv.holdInPlace = true;

  // Hollow Purple: red and blue collide into an imaginary mass that erases everything in its path.
  MoveDef ip = cine("VIOLET IMAGINAIRE", 104, {}, PoseHint::HandSign, UltFx::ImaginaryPurple);
  ip.projectileFrame = 64;
  ip.projectile = proj({70, -10}, {11, 0}, 0, 110, 150, 30, 96, 112, F::Cursed, PV::PurpleOrb);
  ip.projectile.piercing = true;
  ip.projectile.ignoreGround = true;
  ip.projectile.hit.impact = true;
  setUlt(c, 0, ip);

  // Bare-handed: maximum output Blue on the nearest opponent, a vortex that swallows the stage.
  MoveDef mb = cine("BLEU : PUISSANCE MAXIMALE", 100, {}, PoseHint::Raise, UltFx::MaxBlue);
  mb.projectileFrame = 36;
  mb.projectile = proj({0, -30}, {0, 0}, 0, 64, 100, 2.2f, 8, 4, F::Cursed, PV::MaxBlue);
  ProjectileDef& b = mb.projectile;
  b.target = ProjTarget::Nearest;
  b.armDelay = 18;
  b.piercing = true;
  b.rehit = 6;
  b.pull = 5.0f;
  b.pullRadius = 420;
  b.ignoreGround = true;
  b.explodeRadius = 170;
  b.explode = imp(hb(0, 0, 0, 0, 170, 18, 80, 92, 106, L::None, 0, F::Cursed));
  setUlt(c, -1, mb);
  // Six Eyes: the blindfold comes off and he opens his domain (iv, above).
  setUlt(c, 1, iv);
}

void sukuna(CharacterDef& c) {
  std::vector<HitboxDef> st = storm(34, 11, 4, 480, 540, 1.8f, F::Blood, 16, 560, 70);
  st.front().capture = true;
  st.back().launch = true;
  st.back().fx = F::Fire;
  MoveDef shrine = cine("SANCTUAIRE DÉMONIAQUE", 110, st, PoseHint::HandSign, UltFx::MalevolentShrine);
  shrine.holdInPlace = true;
  setUlt(c, -1, shrine);

  // Kamutoke: lightning strikes every opponent, four times.
  MoveDef km = cine("KAMUTOKE", 104, {}, PoseHint::Raise, UltFx::KamutokeStrike);
  km.projectileFrame = 36;
  km.projectileRepeat = 3;
  km.projectileEvery = 12;
  km.projectile = strike(64, 14, 6, 5, 28, 20, F::Electric, PV::Lightning);
  ProjectileDef big = strike(96, 16, 8, 18, 92, 104, F::Electric, PV::Lightning);
  big.hit.impact = true;
  km.shots = {{86, big}};
  setUlt(c, 0, km);

  // Sukuna's finger: "Open". A fire arrow, and the whole stage burns.
  MoveDef fg = cine("FÛGA", 100, {}, PoseHint::Bow, UltFx::FugaBlast);
  fg.projectileFrame = 58;
  fg.projectile = proj({40, -12}, {16, 0}, 0, 50, 34, 10, 40, 60, F::Fire, PV::FireArrow);
  fg.projectile.explodeRadius = 260;
  fg.projectile.explode = imp(hb(0, 0, 0, 0, 260, 26, 50, 96, 110, L::None, 0, F::Fire));
  setUlt(c, 1, fg);
}

void kenpachi(CharacterDef& c) {
  const MoveDef cleave = c.weapons[0].moves[0].second;
  // Kendo: both hands on the hilt for once.
  setUlt(c, -1, ambush("KENDŌ", 100, UltFx::Kendo, PoseHint::TwoHanded, F::Berserk, L::Weapon, 34, {70, 0}, 2, 8, 5.0f,
                       hb(72, 78, 84, -20, 150, 26, 45, 96, 112, L::Weapon, 0, F::Berserk)));

  // Nozarashi: one swing that splits the stage.
  MoveDef noz = cine("TAILLE DE NOZARASHI", 96, {}, PoseHint::Overhead, UltFx::NozarashiCleave);
  noz.projectileFrame = 52;
  noz.projectile = proj({60, -10}, {16, 0}, 0, 60, 100, 24, 94, 108, F::Berserk, PV::ReiatsuWave);
  noz.projectile.piercing = true;
  noz.projectile.ignoreGround = true;
  noz.projectile.grow = 0.6f;
  noz.projectile.hit.impact = true;
  setUlt(c, 0, noz);

  // Bankai: the red demon awakens.
  const int rd = addForm(c, Form::RedDemon, 12, F::Blood, 1.4f, 0.7f, true);
  MoveDef red = cleave;
  red.name = "Taille du démon rouge";
  for (auto& h : red.hitboxes) h.fx = F::Blood;
  c.forms[rd].moves = {{NSpecial, red}};
  setUlt(c, 1, awaken("DÉMON ROUGE", rd, UltFx::RedDemon, PoseHint::Roar, F::Blood, 240, 14));
}

void ulquiorra(CharacterDef& c) {
  setUlt(c, -1, beamUlt("CERO OSCURAS", 100, beamOf(46, 80, {30, -14}, 1100, 253, F::Hollow), 22, 6, PoseHint::Point,
                        UltFx::CeroOscuras));

  MoveDef lanza = cine("LANZA DEL RELÁMPAGO", 100, {}, PoseHint::Throw, UltFx::LanzaRelampago);
  lanza.projectileFrame = 60;
  lanza.projectile = proj({40, -30}, {24, 0}, 0, 50, 42, 10, 40, 60, F::Hollow, PV::Lanza);
  lanza.projectile.explodeRadius = 280;
  lanza.projectile.explode = imp(hb(0, 0, 0, 0, 280, 26, 60, 96, 110, L::None, 0, F::Hollow));
  setUlt(c, 0, lanza);

  // Segunda Etapa: black wings and a tail, and his cero becomes a Cero Oscuras.
  const int se = addForm(c, Form::Segunda, 12, F::Hollow, 1.3f, 0.8f, false);
  MoveDef cero = c.moves[NSpecial];
  cero.name = "Cero Oscuras";
  cero.beam.width *= 1.6f;
  for (auto& h : cero.hitboxes) {
    h.radius *= 1.5f;
    h.damage *= 1.4f;
  }
  c.forms[se].moves = {{NSpecial, cero}};
  setUlt(c, 1, awaken("SEGUNDA ETAPA", se, UltFx::SegundaEtapa, PoseHint::Roar, F::Hollow, 220, 14));
}

void hunter(CharacterDef& c) {
  // True charged slash: a shoulder charge catches, two charged cuts, then the true one.
  std::vector<HitboxDef> tc = {cap(hb(36, 46, 50, -6, 64, 6, 0, 0, 0, L::Body, 0, F::Slash))};
  tc.push_back(hb(54, 58, 60, -6, 60, 6, 361, 18, 6, L::Weapon, 1, F::Slash));
  tc.push_back(hb(64, 68, 60, -6, 60, 7, 361, 18, 6, L::Weapon, 2, F::Slash));
  tc.push_back(lau(hb(84, 90, 90, -20, 150, 28, 50, 96, 112, L::Weapon, 3, F::Slash)));
  MoveDef ts = cine("TAILLE CHARGÉE VRAIE", 108, tc, PoseHint::TwoHanded, UltFx::TrueChargedSlash);
  ts.vel = {ve(1, VM::Set, 0, -2), ve(22, VM::Set, 0, 0), ve(36, VM::SetX, 12, 0), ve(46, VM::Set, 0, 0)};
  ts.holdAt = {72, 0};
  setUlt(c, -1, ts);

  // Charge blade: the phials burst one after the other along the ground.
  MoveDef ad = cine("DÉCHARGE AMPLIFIÉE", 100, {imp(hb(46, 50, 70, 10, 70, 8, 60, 40, 40, L::Weapon, 0, F::Electric))},
                    PoseHint::Overhead, UltFx::AmpedDischarge);
  for (int k = 0; k < 5; ++k) {  // four sparks that keep the victim in place, then the big one
    const bool last = k == 4;
    ProjectileDef ph = proj({last ? 230.0f : 100.0f + k * 80.0f, 10}, {0, 0}, 0, 1, 10, 0, 0, 0, F::Electric, PV::Phial);
    ph.ignoreGround = true;
    ph.explodeRadius = last ? 210.0f : 90.0f;
    ph.explode = last ? hb(0, 0, 0, 0, 210, 20, 60, 94, 108, L::None, 0, F::Electric)
                      : hb(0, 0, 0, 0, 90, 5, 80, 25, 5, L::None, 0, F::Electric);
    ph.explode.impact = last;
    ad.shots.push_back({last ? 68 : 50 + k * 4, ph});
  }
  setUlt(c, 0, ad);

  // Gunlance: Wyvern's Fire, a blast of flames that throws the hunter back.
  MoveDef wy = beamUlt("FEU DU WYVERN", 104, beamOf(64, 78, {70, -6}, 560, 241, F::Fire), 26, 2, PoseHint::Thrust,
                       UltFx::WyvernFire);
  wy.vel = {ve(1, VM::Set, 0, -2), ve(22, VM::Set, 0, 0), ve(64, VM::SetX, -5, 0), ve(72, VM::Set, 0, 0)};
  setUlt(c, 1, wy);
}

void naruto(CharacterDef& c) {
  // Shadow clones kick the opponent into the sky, the original finishes it.
  MoveDef uz = ambush("BARRAGE UZUMAKI", 100, UltFx::UzumakiBarrage, PoseHint::Uppercut, F::Chakra, L::LegFront, 34,
                      {40, -90}, 5, 5, 2.6f, hb(72, 78, 40, -40, 90, 22, 60, 94, 108, L::LegFront, 0, F::Chakra));
  uz.hitboxes.front().offset = {40, 20};
  uz.vel = {ve(1, VM::Set, 0, -2), ve(22, VM::Set, 0, 0), ve(40, VM::Set, 0, -9), ve(62, VM::Set, 0, 0),
            ve(80, VM::Set, 0, 6), ve(92, VM::Set, 0, 0)};
  setUlt(c, -1, uz);

  MoveDef rs = cine("RASENSHURIKEN", 100, {}, PoseHint::Throw, UltFx::Rasenshuriken);
  rs.projectileFrame = 56;
  rs.projectile = proj({50, -20}, {13, 0}, 0, 60, 56, 6, 30, 20, F::Wind, PV::Rasenshuriken);
  rs.projectile.explodeRadius = 260;
  rs.projectile.explode = imp(hb(0, 0, 0, 0, 230, 24, 70, 94, 108, L::None, 0, F::Wind));
  setUlt(c, 0, rs);

  // Kurama's chakra: the giant fox around him gathers a tailed beast bomb.
  const int ku = addLook(c, Form::KuramaAvatar);
  MoveDef bj = cine("BIJŪDAMA", 108, {}, PoseHint::Cast, UltFx::KuramaBijudama);
  bj.formIndex = ku;
  bj.formFrame = 24;
  bj.projectileFrame = 66;
  bj.projectile = proj({80, -40}, {12, 0}, 0, 80, 90, 10, 40, 60, F::Chakra, PV::Bijudama);
  bj.projectile.explodeRadius = 300;
  bj.projectile.explode = imp(hb(0, 0, 0, 0, 300, 28, 70, 98, 112, L::None, 0, F::Dark));
  setUlt(c, 1, bj);
}

void sasuke(CharacterDef& c) {
  // Kirin: the storm gathers, then the lightning dragon falls on the nearest opponent.
  MoveDef kr = cine("KIRIN", 104, {}, PoseHint::Raise, UltFx::Kirin);
  kr.projectileFrame = 64;
  kr.projectile = proj({0, 0}, {0, 0}, 0, 10, 130, 28, 96, 110, F::Lightning, PV::KirinBolt);
  kr.projectile.target = ProjTarget::Nearest;
  kr.projectile.armDelay = 12;
  kr.projectile.piercing = true;
  kr.projectile.ignoreGround = true;
  kr.projectile.hit.angle = 80;
  kr.projectile.hit.impact = true;
  setUlt(c, -1, kr);

  const int sus = addLook(c, Form::SusanooPurple);
  MoveDef ia = cine("FLÈCHE D'INDRA", 104, {}, PoseHint::Bow, UltFx::IndraArrow);
  ia.formIndex = sus;
  ia.formFrame = 24;
  ia.projectileFrame = 64;
  ia.projectile = proj({60, -40}, {28, 0}, 0, 50, 58, 28, 96, 110, F::Dark, PV::IndraArrow);
  ia.projectile.piercing = true;
  ia.projectile.ignoreGround = true;
  ia.projectile.hit.impact = true;
  setUlt(c, 0, ia);

  // Mangekyo: black flames on every opponent, they cling and burn, then flare up.
  MoveDef am = cine("AMATERASU", 100, {}, PoseHint::HandSign, UltFx::Amaterasu);
  am.projectileFrame = 40;
  am.projectile = proj({0, 0}, {0, 0}, 0, 80, 62, 2.4f, 8, 4, F::BlackFlame, PV::BlackFlames);
  ProjectileDef& a = am.projectile;
  a.target = ProjTarget::All;
  a.armDelay = 16;
  a.piercing = true;
  a.rehit = 6;
  a.follow = true;
  a.ignoreGround = true;
  a.explodeRadius = 130;
  a.explode = imp(hb(0, 0, 0, 0, 130, 16, 75, 90, 104, L::None, 0, F::BlackFlame));
  setUlt(c, 1, am);
}

void kakashi(CharacterDef& c) {
  setUlt(c, -1, ambush("RAIKIRI", 100, UltFx::Raikiri, PoseHint::Rush, F::Lightning, L::ArmFront, 40, {46, -6}, 4, 4,
                       3.0f, hb(70, 76, 50, -6, 90, 24, 40, 95, 110, L::ArmFront, 0, F::Lightning)));

  // Sharingan: Kamui opens a rift on the nearest opponent and twists it away.
  MoveDef km = cine("KAMUI", 100, {}, PoseHint::Point, UltFx::Kamui);
  km.projectileFrame = 38;
  km.projectile = proj({0, -10}, {0, 0}, 0, 60, 80, 2.6f, 8, 4, F::Dark, PV::KamuiVortex);
  ProjectileDef& k = km.projectile;
  k.target = ProjTarget::Nearest;
  k.armDelay = 20;
  k.piercing = true;
  k.rehit = 6;
  k.pull = 4.5f;
  k.pullRadius = 320;
  k.follow = true;
  k.ignoreGround = true;
  k.explodeRadius = 140;
  k.explode = imp(hb(0, 0, 0, 0, 140, 20, 80, 94, 108, L::None, 0, F::Dark));
  setUlt(c, 0, km);

  // Double Mangekyo: his own Susanoo throws a giant Kamui shuriken that drags its victims along.
  const int sus = addLook(c, Form::SusanooBlue);
  MoveDef ks = cine("SHURIKEN DU SUSANOO", 104, {}, PoseHint::Throw, UltFx::KamuiShuriken);
  ks.formIndex = sus;
  ks.formFrame = 24;
  ks.projectileFrame = 60;
  ks.projectile = proj({70, -30}, {9, 0}, 0, 54, 84, 3, 14, 6, F::Dark, PV::KamuiShuriken);
  ks.projectile.piercing = true;
  ks.projectile.rehit = 5;
  ks.projectile.pull = 2.5f;
  ks.projectile.pullRadius = 180;
  ks.projectile.ignoreGround = true;
  ks.projectile.explodeRadius = 160;
  ks.projectile.explode = imp(hb(0, 0, 0, 0, 160, 22, 60, 94, 108, L::None, 0, F::Dark));
  setUlt(c, 1, ks);
}
void grimmjow(CharacterDef& c) {
  // Gran Rey Cero: his blood in the cero, a blast of blue light across the stage.
  setUlt(c, -1, beamUlt("GRAN REY CERO", 100, beamOf(46, 78, {30, -14}, 1100, 264, F::Claw), 21, 6, PoseHint::Point,
                        UltFx::GranReyCero));

  // Pantera: "Kishire, Pantera". The release (12 s): long hair, ears, tail, claws; the darts and the claws grow.
  const int pt = addForm(c, Form::Pantera, 12, F::Claw, 1.3f, 0.8f, false);
  MoveDef darts = c.moves[DSpecial];
  darts.projectile.count = 7;
  darts.projectile.spreadDeg = 8;
  darts.projectile.radius *= 1.3f;
  darts.projectile.hit.radius = darts.projectile.radius;
  darts.projectile.hit.damage *= 1.25f;
  MoveDef claws = c.weapons[1].moves[0].second;
  claws.name = "Desgarrón";
  c.forms[pt].moves = {{DSpecial, darts}, {SSpecial, claws}};
  setUlt(c, 0, awaken("KISHIRE, PANTERA", pt, UltFx::PanteraRelease, PoseHint::Roar, F::Claw, 230, 14));

  // Claws: Desgarron. He leaps on the nearest opponent, rakes it again and again, then ten claws of light.
  setUlt(c, 1, ambush("DESGARRÓN", 100, UltFx::DesgarronClaws, PoseHint::Throw, F::Claw, L::ArmFront, 34, {60, -6}, 5, 5, 3.0f,
                      hb(72, 78, 70, -10, 130, 22, 45, 95, 110, L::ArmFront, 0, F::Claw)));
}

void starrk(CharacterDef& c) {
  // Cero Metralleta: released for a moment, a thousand ceros from both pistols, then a great one.
  const int gun = addLook(c, Form::LosLobos);
  MoveDef cm = cine("CERO METRALLETA", 104, {}, PoseHint::Cast, UltFx::CeroMetralleta);
  cm.formIndex = gun;
  cm.formFrame = 22;
  cm.projectileFrame = 36;
  cm.projectileRepeat = 23;
  cm.projectileEvery = 2;
  cm.projectile = proj({40, -14}, {19, 0}, 0, 50, 14, 1.4f, 10, 4, F::Wolf, PV::StarrkCero);
  cm.projectile.jitter = {0, 70};
  cm.projectile.piercing = true;
  ProjectileDef big = proj({50, -14}, {16, 0}, 0, 60, 72, 20, 92, 106, F::Wolf, PV::StarrkCero);
  big.piercing = true;
  big.ignoreGround = true;
  big.hit.impact = true;
  cm.shots = {{86, big}};
  setUlt(c, -1, cm);

  // The pack: the wolves of Los Lobos hunt every opponent, bite and explode, then the whole pack at once.
  MoveDef ll = cine("LOS LOBOS", 104, {}, PoseHint::Point, UltFx::LosLobos);
  ll.projectileFrame = 40;
  ll.projectileRepeat = 3;
  ll.projectileEvery = 10;
  ll.projectile = strike(42, 14, 10, 5, 26, 18, F::Wolf, PV::CeroWolf);
  ProjectileDef pack = strike(84, 16, 12, 20, 92, 104, F::Wolf, PV::CeroWolf);
  pack.hit.impact = true;
  ll.shots = {{86, pack}};
  setUlt(c, 0, ll);

  // Lilynette: "Kick 'em, Los Lobos". The release (12 s): fur coat, two pistols; the bursts and the pack grow.
  const int lob = addForm(c, Form::LosLobos, 12, F::Wolf, 1.3f, 0.8f, false);
  MoveDef rapid = c.moves[DSpecial];
  rapid.name = "Cero Metralleta";
  rapid.projectileRepeat = 11;
  rapid.projectileEvery = 2;
  rapid.projectile.hit.damage *= 1.2f;
  MoveDef wolves = c.weapons[0].moves[0].second;
  c.forms[lob].moves = {{DSpecial, rapid}, {SSpecial, wolves}};
  setUlt(c, 1, awaken("RESURRECCIÓN : LOS LOBOS", lob, UltFx::StarrkRelease, PoseHint::Raise, F::Wolf, 220, 14));
}
}  // namespace

void addUltimates(CharacterDef& c) {
  switch (c.hero) {
    case Hero::Asta: asta(c); break;
    case Hero::Yuno: yuno(c); break;
    case Hero::Ichigo: ichigo(c); break;
    case Hero::Byakuya: byakuya(c); break;
    case Hero::Link: link(c); break;
    case Hero::Ganondorf: ganondorf(c); break;
    case Hero::Kratos: kratos(c); break;
    case Hero::Gojo: gojo(c); break;
    case Hero::Sukuna: sukuna(c); break;
    case Hero::Kenpachi: kenpachi(c); break;
    case Hero::Ulquiorra: ulquiorra(c); break;
    case Hero::Hunter: hunter(c); break;
    case Hero::Naruto: naruto(c); break;
    case Hero::Sasuke: sasuke(c); break;
    case Hero::Kakashi: kakashi(c); break;
    case Hero::Grimmjow: grimmjow(c); break;
    case Hero::Starrk: starrk(c); break;
    case Hero::Count: break;
  }
}
