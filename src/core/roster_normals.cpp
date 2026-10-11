// Generic move sets shared by several fighters. Offsets are authored for a
// 100 px tall fighter (center at mid-height) and scaled by S; damage by P.
#include "core/moves.hpp"

HitboxDef hb(int start, int end, float ox, float oy, float radius, float dmg, float angle,
             float bkb, float kbg, Limb limb, int group, HitFx fx) {
  HitboxDef h;
  h.start = start;
  h.end = end;
  h.offset = {ox, oy};
  h.radius = radius;
  h.damage = dmg;
  h.angle = angle;
  h.baseKB = bkb;
  h.growth = kbg;
  h.limb = limb;
  h.group = group;
  h.fx = fx;
  return h;
}

MoveDef mv(const char* name, int duration, std::vector<HitboxDef> boxes, int landingLag) {
  MoveDef m;
  m.name = name;
  m.duration = duration;
  m.hitboxes = std::move(boxes);
  m.landingLag = landingLag;
  return m;
}

namespace {
void scale(MoveDef& m, float S, float P) {
  for (auto& h : m.hitboxes) {
    h.offset = h.offset * S;
    h.radius *= S;
    h.damage *= P;
  }
}
HitboxDef rev(HitboxDef h) {
  h.reverse = true;
  return h;
}
}  // namespace

void buildBrawlerNormals(CharacterDef& c, float S, float P, HitFx fx) {
  using L = Limb;
  auto& m = c.moves;
  m[Jab] = mv("Jab", 18, {hb(3, 5, 36, -8, 16, 2.5f, 361, 6, 30, L::ArmFront)});
  m[Jab].nextMove = Jab2; m[Jab].comboStart = 3; m[Jab].comboEnd = 16; m[Jab].comboAt = 8;
  m[Jab2] = mv("Jab 2", 18, {hb(3, 5, 38, -6, 17, 2.0f, 361, 6, 30, L::ArmBack)});
  m[Jab2].nextMove = Jab3; m[Jab2].comboStart = 3; m[Jab2].comboEnd = 16; m[Jab2].comboAt = 8;
  m[Jab3] = mv("Jab 3", 30, {hb(4, 7, 44, -2, 20, 4.5f, 361, 45, 100, L::LegFront, 0, fx)});

  m[FTilt] = mv("Coup avant", 30, {hb(6, 9, 48, -6, 20, 8.5f, 361, 22, 95, L::LegFront)});
  m[UTilt] = mv("Coup haut", 28, {hb(5, 7, 30, -30, 18, 6.5f, 95, 32, 110, L::ArmFront),
                                  hb(7, 11, 0, -64, 22, 6.5f, 95, 32, 110, L::ArmFront)});
  m[DTilt] = mv("Balayage", 22, {hb(5, 8, 46, 38, 17, 6.0f, 75, 30, 70, L::LegFront)});
  m[DashAttack] = mv("Attaque elan", 36, {hb(5, 9, 34, 0, 24, 9.0f, 55, 50, 70, L::Body, 0, fx),
                                          hb(9, 16, 34, 0, 20, 6.0f, 55, 40, 60, L::Body)});
  m[DashAttack].vel = {{2, VelMode::SetX, {9.0f, 0}}};
  m[DashAttack].frictionMul = 0.25f;

  m[FSmash] = mv("Smash avant", 46, {hb(14, 17, 54, -10, 26, 17.0f, 361, 30, 98, L::ArmFront, 0, fx)});
  m[FSmash].chargeFrame = 6;
  m[USmash] = mv("Smash haut", 42, {hb(11, 15, 8, -72, 26, 15.0f, 88, 32, 98, L::Head, 0, fx)});
  m[USmash].chargeFrame = 5;
  m[DSmash] = mv("Smash bas", 46, {hb(8, 10, 42, 40, 22, 13.0f, 30, 30, 95, L::LegFront, 0, fx),
                                   rev(hb(13, 15, -42, 40, 22, 14.0f, 30, 30, 95, L::LegBack, 1, fx))});
  m[DSmash].chargeFrame = 4;

  m[NAir] = mv("Aerien neutre", 40, {hb(4, 8, 0, 0, 34, 9.0f, 361, 25, 90, L::Body),
                                     hb(8, 26, 0, 0, 30, 5.0f, 361, 15, 85, L::Body)}, 8);
  m[FAir] = mv("Aerien avant", 42, {hb(10, 14, 46, -2, 24, 12.0f, 361, 30, 95, L::LegFront, 0, fx)}, 12);
  m[BAir] = mv("Aerien arriere", 32, {rev(hb(6, 10, -48, -2, 22, 11.5f, 361, 28, 98, L::LegBack))}, 9);
  m[UAir] = mv("Aerien haut", 30, {hb(5, 10, 0, -64, 26, 8.0f, 80, 30, 92, L::LegFront)}, 8);
  m[DAir] = mv("Aerien bas", 44, {hb(12, 16, 0, 52, 22, 13.0f, 270, 20, 90, L::LegFront, 0, fx),
                                  hb(16, 22, 0, 48, 18, 8.0f, 361, 20, 80, L::LegFront)}, 16);

  for (int id : {Jab, Jab2, Jab3, FTilt, UTilt, DTilt, DashAttack, FSmash, USmash, DSmash, NAir,
                 FAir, BAir, UAir, DAir})
    scale(m[id], S, P);
  buildCommonMoves(c, S, P);
}

std::vector<HitboxDef> beamHits(const BeamDef& b, float dmg, float angle, float bkb, float kbg, int hits,
                                int firstGroup) {
  std::vector<HitboxDef> out;
  const float r = b.width * 0.5f, step = std::max(10.0f, r * 1.3f);
  const Vec2 dir{std::cos(deg2rad(b.angle)), -std::sin(deg2rad(b.angle))};
  const int span = std::max(1, (b.end - b.start) / std::max(1, hits));
  for (int k = 0; k < hits; ++k) {
    const bool last = k == hits - 1;
    const int s = b.start + k * span, e = last ? b.end : s + span;
    for (float d = r * 0.6f; d <= b.length; d += step) {
      HitboxDef h = hb(s, e, b.from.x + dir.x * d, b.from.y + dir.y * d, r, last ? dmg : 2.0f, last ? angle : 361.0f,
                       last ? bkb : 8.0f, last ? kbg : 0.0f, Limb::None, firstGroup + k, b.fx);
      h.impact = last;
      out.push_back(h);
    }
  }
  return out;
}

void buildSwordNormals(CharacterDef& c, float S, float P, HitFx smashFx) {
  using L = Limb;
  const HitFx sl = HitFx::Slash;
  auto& m = c.moves;
  // Each sword swing has a strong "tip" (listed first = priority) and a weaker base.
  m[Jab] = mv("Estoc", 20, {hb(4, 6, 78, -8, 16, 4.0f, 361, 10, 40, L::Weapon, 0, sl),
                            hb(4, 6, 44, -8, 16, 3.0f, 361, 8, 30, L::Weapon, 0, sl)});
  m[Jab].nextMove = Jab2; m[Jab].comboStart = 4; m[Jab].comboEnd = 18; m[Jab].comboAt = 10;
  m[Jab2] = mv("Revers", 30, {hb(4, 7, 76, -14, 18, 5.0f, 361, 40, 90, L::Weapon, 0, sl),
                              hb(4, 7, 42, -14, 18, 4.0f, 361, 35, 80, L::Weapon, 0, sl)});
  m[FTilt] = mv("Taille", 32, {hb(7, 10, 84, -10, 18, 12.0f, 361, 25, 90, L::Weapon, 0, sl),
                               hb(7, 10, 48, -10, 20, 9.0f, 361, 25, 80, L::Weapon, 0, sl)});
  m[UTilt] = mv("Arc céleste", 30, {hb(6, 12, 20, -82, 22, 7.5f, 95, 35, 105, L::Weapon, 0, sl),
                                    hb(6, 12, 48, -50, 20, 7.0f, 95, 35, 100, L::Weapon, 0, sl)});
  m[DTilt] = mv("Fauche", 22, {hb(6, 8, 80, 40, 16, 9.0f, 30, 20, 60, L::Weapon, 0, sl),
                               hb(6, 8, 44, 40, 16, 7.0f, 30, 20, 50, L::Weapon, 0, sl)});
  m[DashAttack] = mv("Charge lame", 38, {hb(8, 13, 70, -4, 24, 11.0f, 55, 50, 75, L::Weapon, 0, sl)});
  m[DashAttack].vel = {{2, VelMode::SetX, {9.5f, 0}}};
  m[DashAttack].frictionMul = 0.25f;

  m[FSmash] = mv("Smash epee", 50, {hb(12, 15, 92, 0, 18, 19.0f, 361, 32, 100, L::Weapon, 0, smashFx),
                                    hb(12, 15, 50, -10, 24, 14.0f, 361, 30, 92, L::Weapon, 0, smashFx)});
  m[FSmash].chargeFrame = 6;
  m[USmash] = mv("Pic céleste", 44, {hb(13, 17, 4, -102, 18, 17.0f, 90, 30, 98, L::Weapon, 0, smashFx),
                                     hb(13, 17, 4, -60, 22, 13.0f, 90, 30, 92, L::Weapon, 0, smashFx)});
  m[USmash].chargeFrame = 6;
  m[DSmash] = mv("Moulinet bas", 48, {hb(6, 8, 74, 42, 18, 12.0f, 25, 30, 90, L::Weapon, 0, sl),
                                      rev(hb(18, 20, -78, 42, 18, 15.0f, 25, 32, 95, L::Weapon, 1, smashFx))});
  m[DSmash].chargeFrame = 3;

  m[NAir] = mv("Double taille", 40, {hb(5, 8, 66, -10, 26, 5.0f, 70, 25, 40, L::Weapon, 0, sl),
                                     hb(14, 18, 66, -4, 28, 9.0f, 361, 30, 95, L::Weapon, 1, sl)}, 8);
  m[FAir] = mv("Croissant", 36, {hb(5, 9, 72, -14, 26, 11.0f, 361, 30, 92, L::Weapon, 0, sl)}, 8);
  m[BAir] = mv("Revers aerien", 36, {rev(hb(7, 10, -72, -6, 26, 12.5f, 361, 30, 98, L::Weapon, 0, sl))}, 10);
  m[UAir] = mv("Lune montante", 36, {hb(5, 10, 0, -80, 30, 11.0f, 85, 30, 95, L::Weapon, 0, sl)}, 8);
  m[DAir] = mv("Lame plongeante", 46, {hb(14, 17, 14, 70, 22, 14.0f, 270, 20, 90, L::Weapon, 0, sl),
                                       hb(14, 17, 46, 40, 22, 11.0f, 361, 30, 80, L::Weapon, 0, sl)}, 16);

  for (int id : {Jab, Jab2, FTilt, UTilt, DTilt, DashAttack, FSmash, USmash, DSmash, NAir, FAir,
                 BAir, UAir, DAir})
    scale(m[id], S, P);
  buildCommonMoves(c, S, P);
}

void buildCommonMoves(CharacterDef& c, float S, float P) {
  using L = Limb;
  auto& m = c.moves;
  m[Grab] = mv("Saisie", 30, {hb(6, 9, 40, -6, 22, 0, 0, 0, 0, L::ArmFront)});
  m[Grab].isGrab = true;
  m[Pummel] = mv("Coup de genou", 16, {hb(0, 0, 0, 0, 0, 1.5f, 0, 0, 0, L::LegFront)});
  m[Pummel].throwFrame = 5;
  m[Pummel].pummel = true;
  m[FThrow] = mv("Projection avant", 30, {hb(0, 0, 0, 0, 0, 8.0f, 40, 60, 70, L::ArmFront)});
  m[FThrow].throwFrame = 10;
  m[BThrow] = mv("Projection arriere", 36, {hb(0, 0, 0, 0, 0, 10.0f, 140, 60, 72, L::ArmBack)});
  m[BThrow].throwFrame = 16;
  m[UThrow] = mv("Projection haute", 32, {hb(0, 0, 0, 0, 0, 7.0f, 90, 70, 65, L::ArmFront)});
  m[UThrow].throwFrame = 12;
  m[DThrow] = mv("Projection basse", 34, {hb(0, 0, 0, 0, 0, 6.0f, 72, 70, 35, L::LegFront)});
  m[DThrow].throwFrame = 14;

  m[LedgeAttack] = mv("Attaque bord", 32, {hb(12, 16, 42, 10, 26, 9.0f, 361, 45, 25, L::LegFront)});
  m[LedgeAttack].intangibleStart = 0;
  m[LedgeAttack].intangibleEnd = 12;
  m[GetupAttack] = mv("Relevé offensif", 32, {hb(12, 15, 44, 34, 22, 7.0f, 361, 50, 30, L::LegFront),
                                              rev(hb(17, 20, -44, 34, 22, 7.0f, 361, 50, 30, L::LegBack, 1))});
  m[GetupAttack].intangibleStart = 0;
  m[GetupAttack].intangibleEnd = 15;

  for (int id : {Grab, Pummel, FThrow, BThrow, UThrow, DThrow, LedgeAttack, GetupAttack})
    scale(m[id], S, P);

  // Final smash: a growing shock wave, the last ring launches very hard.
  m[FinalSmash] = mv("SUPER CLASH", 110, {hb(34, 38, 0, 0, 130, 8, 80, 40, 20, L::Body, 0, HitFx::Magic),
                                          hb(46, 50, 0, 0, 250, 8, 80, 40, 20, L::Body, 1, HitFx::Magic),
                                          hb(60, 66, 0, 0, 420, 22, 50, 95, 105, L::Body, 2, HitFx::Magic)});
  m[FinalSmash].intangibleStart = 0;
  m[FinalSmash].intangibleEnd = 110;
  m[FinalSmash].vel = {{1, VelMode::Set, {0.0f, -3.0f}}, {22, VelMode::Set, {0.0f, 0.0f}}};
  m[FinalSmash].gravityMul = 0.0f;
  m[FinalSmash].allowDrift = false;
}
