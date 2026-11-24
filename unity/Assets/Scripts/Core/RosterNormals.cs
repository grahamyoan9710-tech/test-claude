using System;
using System.Collections.Generic;
using static ClashHorizon.Core.MathUtil;
using static ClashHorizon.Core.MoveId;

namespace ClashHorizon.Core
{
    // Port of src/core/roster_normals.cpp: the shared normals, the weapon kits and the hitbox builders.
    public static class RosterNormals
    {
        public static HitboxDef Hb(int start, int end, float ox, float oy, float radius, float dmg, float angle, float bkb,
                                   float kbg, Limb limb, int group = 0, HitFx fx = HitFx.Normal)
        {
            HitboxDef h = HitboxDef.Default;
            h.start = start; h.end = end; h.offset = new Vec2(ox, oy); h.radius = radius;
            h.damage = dmg; h.angle = angle; h.baseKB = bkb; h.growth = kbg; h.limb = limb; h.group = group; h.fx = fx;
            return h;
        }

        public static MoveDef Mv(string name, int duration, List<HitboxDef> boxes, int landingLag = 6) =>
            new MoveDef { name = name, duration = duration, hitboxes = new List<HitboxDef>(boxes), landingLag = landingLag };

        static void Scale(MoveDef m, float S, float P)  // hitboxes only
        {
            for (int i = 0; i < m.hitboxes.Count; i++)
            {
                var h = m.hitboxes[i];
                h.offset = h.offset * S; h.radius *= S; h.damage *= P;
                m.hitboxes[i] = h;
            }
        }

        static HitboxDef Rev(HitboxDef h) { h.reverse = true; return h; }

        // C++ `for (auto& h : v)`: a struct list is edited through a copy written back.
        internal static void Edit(List<HitboxDef> v, Func<HitboxDef, HitboxDef> f)
        {
            for (int i = 0; i < v.Count; i++) v[i] = f(v[i]);
        }

        public static void BuildBrawlerNormals(CharacterDef c, float S, float P, HitFx fx)
        {
            MoveDef[] m = c.moves;
            m[Jab] = Mv("Jab", 18, new List<HitboxDef> { Hb(3, 5, 36, -8, 16, 2.5f, 361, 6, 30, Limb.ArmFront) });
            m[Jab].nextMove = Jab2; m[Jab].comboStart = 3; m[Jab].comboEnd = 16; m[Jab].comboAt = 8;
            m[Jab2] = Mv("Jab 2", 18, new List<HitboxDef> { Hb(3, 5, 38, -6, 17, 2.0f, 361, 6, 30, Limb.ArmBack) });
            m[Jab2].nextMove = Jab3; m[Jab2].comboStart = 3; m[Jab2].comboEnd = 16; m[Jab2].comboAt = 8;
            m[Jab3] = Mv("Jab 3", 30, new List<HitboxDef> { Hb(4, 7, 44, -2, 20, 4.5f, 361, 45, 100, Limb.LegFront, 0, fx) });
            m[FTilt] = Mv("Coup avant", 30, new List<HitboxDef> { Hb(6, 9, 48, -6, 20, 8.5f, 361, 22, 95, Limb.LegFront) });
            m[UTilt] = Mv("Coup haut", 28, new List<HitboxDef>
            {
                Hb(5, 7, 30, -30, 18, 6.5f, 95, 32, 110, Limb.ArmFront), Hb(7, 11, 0, -64, 22, 6.5f, 95, 32, 110, Limb.ArmFront)
            });
            m[DTilt] = Mv("Balayage", 22, new List<HitboxDef> { Hb(5, 8, 46, 38, 17, 6.0f, 75, 30, 70, Limb.LegFront) });
            m[DashAttack] = Mv("Attaque elan", 36, new List<HitboxDef>
            {
                Hb(5, 9, 34, 0, 24, 9.0f, 55, 50, 70, Limb.Body, 0, fx), Hb(9, 16, 34, 0, 20, 6.0f, 55, 40, 60, Limb.Body)
            });
            m[DashAttack].vel = new List<VelEvent> { new VelEvent { frame = 2, mode = VelMode.SetX, v = new Vec2(9.0f, 0) } };
            m[DashAttack].frictionMul = 0.25f;
            m[FSmash] = Mv("Smash avant", 46, new List<HitboxDef> { Hb(14, 17, 54, -10, 26, 17.0f, 361, 30, 98, Limb.ArmFront, 0, fx) });
            m[FSmash].chargeFrame = 6;
            m[USmash] = Mv("Smash haut", 42, new List<HitboxDef> { Hb(11, 15, 8, -72, 26, 15.0f, 88, 32, 98, Limb.Head, 0, fx) });
            m[USmash].chargeFrame = 5;
            m[DSmash] = Mv("Smash bas", 46, new List<HitboxDef>
            {
                Hb(8, 10, 42, 40, 22, 13.0f, 30, 30, 95, Limb.LegFront, 0, fx), Rev(Hb(13, 15, -42, 40, 22, 14.0f, 30, 30, 95, Limb.LegBack, 1, fx))
            });
            m[DSmash].chargeFrame = 4;
            m[NAir] = Mv("Aerien neutre", 40, new List<HitboxDef>
            {
                Hb(4, 8, 0, 0, 34, 9.0f, 361, 25, 90, Limb.Body), Hb(8, 26, 0, 0, 30, 5.0f, 361, 15, 85, Limb.Body)
            }, 8);
            m[FAir] = Mv("Aerien avant", 42, new List<HitboxDef> { Hb(10, 14, 46, -2, 24, 12.0f, 361, 30, 95, Limb.LegFront, 0, fx) }, 12);
            m[BAir] = Mv("Aerien arriere", 32, new List<HitboxDef> { Rev(Hb(6, 10, -48, -2, 22, 11.5f, 361, 28, 98, Limb.LegBack)) }, 9);
            m[UAir] = Mv("Aerien haut", 30, new List<HitboxDef> { Hb(5, 10, 0, -64, 26, 8.0f, 80, 30, 92, Limb.LegFront) }, 8);
            m[DAir] = Mv("Aerien bas", 44, new List<HitboxDef>
            {
                Hb(12, 16, 0, 52, 22, 13.0f, 270, 20, 90, Limb.LegFront, 0, fx), Hb(16, 22, 0, 48, 18, 8.0f, 361, 20, 80, Limb.LegFront)
            }, 16);
            foreach (int id in new[] { Jab, Jab2, Jab3, FTilt, UTilt, DTilt, DashAttack, FSmash, USmash, DSmash, NAir, FAir, BAir, UAir, DAir })
            {
                Scale(m[id], S, P * 1.14f);
                Edit(m[id].hitboxes, h => { h.radius *= 1.04f; return h; });
            }
            BuildCommonMoves(c, S, P);
        }

        // Unused by the roster (only Ult.Beam calls it): parity does not cover it. Casts keep the maths in float32 like C++.
        public static List<HitboxDef> BeamHits(in BeamDef b, float dmg, float angle, float bkb, float kbg, int hits = 1, int firstGroup = 0)
        {
            var output = new List<HitboxDef>();
            float r = (float)(b.width * 0.5f), step = MathF.Max(10.0f, (float)(r * 1.3f));
            Vec2 dir = new Vec2(MathF.Cos(Deg2rad(b.angle)), -MathF.Sin(Deg2rad(b.angle)));
            int span = Math.Max(1, (b.end - b.start) / Math.Max(1, hits));
            for (int k = 0; k < hits; ++k)
            {
                bool last = k == hits - 1;
                int s = b.start + k * span, e = last ? b.end : s + span;
                for (float d = (float)(r * 0.6f); d <= b.length; d = (float)(d + step))
                {
                    HitboxDef h = Hb(s, e, b.from.x + (float)(dir.x * d), b.from.y + (float)(dir.y * d), r, last ? dmg : 2.0f,
                                     last ? angle : 361.0f, last ? bkb : 8.0f, last ? kbg : 0.0f, Limb.None, firstGroup + k, b.fx);
                    h.impact = last;
                    output.Add(h);
                }
            }
            return output;
        }

        public static HitboxDef Sw(int start, int end, float a0, float a1, float reach0, float reach1, float radius, float dmg,
                                   float angle, float bkb, float kbg, int group = 0, HitFx fx = HitFx.Slash)
        {
            HitboxDef h = Hb(start, end, 0, 0, radius, dmg, angle, bkb, kbg, Limb.Weapon, group, fx);
            h.swing = true; h.a0 = a0; h.a1 = a1; h.reach0 = reach0; h.reach1 = reach1;
            return h;
        }

        public static void AddBladeKit(WeaponDef w, float S, float P, HitFx fx, float speed = 1.0f, bool thrust2 = false, int airCuts = 2)
        {
            const HitFx sl = HitFx.Slash;
            int T(int f) => Math.Max(1, (int)(float)((float)(f * speed) + 0.5f));  // float32 like C++ (ledgered)
            List<HitboxDef> Pair(int s0, int e0, float a0, float a1, float r, float dmg, float ang, float bkb, float kbg, int group, HitFx hfx) =>
                new List<HitboxDef>
                {
                    Sw(T(s0), T(e0), a0, a1, 0.55f, 1.0f, r, dmg, ang, bkb, kbg, group, hfx),
                    Sw(T(s0), T(e0), a0, a1, 0.0f, 0.55f, r, dmg * 0.8f, ang, bkb, kbg, group, hfx)
                };
            MoveDef jab = Mv("Taille", T(24), Pair(5, 8, 110, -40, 18, 4.5f, 361, 10, 25, 0, sl));
            jab.nextMove = Jab2; jab.comboStart = T(4); jab.comboEnd = T(22); jab.comboAt = T(12);
            MoveDef jab2 = thrust2 ? Mv("Estoc éclair", T(26), Pair(5, 8, 4, 4, 17, 4.5f, 361, 12, 28, 0, sl))
                                   : Mv("Revers", T(26), Pair(5, 8, 30, -15, 18, 4.5f, 361, 12, 28, 0, sl));
            jab2.nextMove = Jab3; jab2.comboStart = T(4); jab2.comboEnd = T(24); jab2.comboAt = T(13);
            MoveDef jab3 = Mv("Taille montante", T(36), Pair(9, 14, -50, 130, 22, 9.5f, 55, 50, 98, 0, fx));
            Edit(jab3.hitboxes, h => { h.impact = true; return h; });
            var air = Pair(5, 8, 100, -60, 24, 5.0f, 70, 20, 35, 0, sl);
            if (airCuts > 2) air.AddRange(Pair(11, 14, -60, 80, 24, 4.0f, 80, 20, 35, 1, sl));
            air.AddRange(Pair(airCuts > 2 ? 18 : 13, airCuts > 2 ? 23 : 18, -90, 180, 26, 8.0f, 361, 35, 98, airCuts > 2 ? 2 : 1, fx));
            MoveDef nair = Mv("Taille aérienne", T(airCuts > 2 ? 40 : 38), air, 8);
            int[] ids = { Jab, Jab2, Jab3, NAir };
            MoveDef[] ms = { jab, jab2, jab3, nair };
            for (int i = 0; i < 4; i++)
            {
                Scale(ms[i], S, P * 0.88f);
                w.moves.Add((ids[i], ms[i]));
            }
            w.kit = true;
        }

        public static void AddLanceKit(WeaponDef w, float S, float P, HitFx fx, float speed = 1.0f)
        {
            const HitFx sl = HitFx.Slash;
            int T(int f) => Math.Max(1, (int)(float)((float)(f * speed) + 0.5f));  // float32 like C++ (ledgered)
            List<HitboxDef> Pair(int s0, int e0, float a0, float a1, float r, float dmg, float ang, float bkb, float kbg, int group, HitFx hfx) =>
                new List<HitboxDef>
                {
                    Sw(T(s0), T(e0), a0, a1, 0.6f, 1.0f, r, dmg, ang, bkb, kbg, group, hfx),         // the point
                    Sw(T(s0), T(e0), a0, a1, 0.2f, 0.6f, r, dmg * 0.8f, ang, bkb, kbg, group, hfx)   // the shaft
                };
            MoveDef jab = Mv("Coup de lance", T(24), Pair(5, 8, 4, 4, 17, 4.5f, 361, 10, 25, 0, sl));
            jab.nextMove = Jab2; jab.comboStart = T(4); jab.comboEnd = T(22); jab.comboAt = T(12);
            MoveDef jab2 = Mv("Estoc appuyé", T(26), Pair(6, 9, 2, 2, 18, 5.0f, 361, 12, 28, 0, sl));
            jab2.nextMove = Jab3; jab2.comboStart = T(4); jab2.comboEnd = T(24); jab2.comboAt = T(13);
            MoveDef jab3 = Mv("Balayage", T(38), Pair(9, 15, -50, 130, 22, 9.5f, 55, 50, 98, 0, fx));
            Edit(jab3.hitboxes, h => { h.impact = true; return h; });
            var air = Pair(5, 8, -35, -35, 20, 5.0f, 70, 20, 35, 0, sl);
            air.AddRange(Pair(13, 17, 0, 0, 22, 8.0f, 361, 35, 98, 1, fx));
            MoveDef nair = Mv("Estoc aérien", T(38), air, 8);
            int[] ids = { Jab, Jab2, Jab3, NAir };
            MoveDef[] ms = { jab, jab2, jab3, nair };
            for (int i = 0; i < 4; i++)
            {
                Scale(ms[i], S, P * 0.88f);
                w.moves.Add((ids[i], ms[i]));
            }
            w.kit = true;
        }

        public static void AddFistKit(WeaponDef w, float S, float P, HitFx fx, float speed = 1.0f, float reach = 1.0f, bool upper = false)
        {
            int T(int f) => Math.Max(1, (int)(float)((float)(f * speed) + 0.5f));  // float32 like C++ (ledgered)
            MoveDef jab = Mv("Frappe", T(20), new List<HitboxDef> { Hb(T(4), T(7), 38 * reach, -8, 18, 4.0f, 361, 6, 18, Limb.ArmFront, 0, fx) });
            jab.nextMove = Jab2; jab.comboStart = T(3); jab.comboEnd = T(18); jab.comboAt = T(10);
            MoveDef jab2 = Mv("Enchaînement", T(22), new List<HitboxDef> { Hb(T(5), T(8), 42 * reach, -10, 20, 4.5f, 361, 7, 20, Limb.LegFront, 0, fx) });
            jab2.nextMove = Jab3; jab2.comboStart = T(3); jab2.comboEnd = T(20); jab2.comboAt = T(11);
            HitboxDef fin = Hb(T(9), T(13), 46 * reach, upper ? -40.0f : -8.0f, 26, 9.5f, upper ? 80.0f : 50.0f, 50, 98, Limb.ArmFront, 0, fx);
            fin.impact = true;
            MoveDef jab3 = Mv("Coup final", T(34), new List<HitboxDef> { fin });
            MoveDef nair = Mv("Combo aérien", T(36), new List<HitboxDef>
            {
                Hb(T(5), T(8), 30 * reach, -10, 30, 5.0f, 70, 20, 35, Limb.LegFront, 0, fx),
                Hb(T(12), T(17), -26 * reach, -10, 30, 7.5f, 361, 35, 98, Limb.LegBack, 1, fx)
            }, 8);
            int[] ids = { Jab, Jab2, Jab3, NAir };
            MoveDef[] ms = { jab, jab2, jab3, nair };
            for (int i = 0; i < 4; i++)
            {
                Scale(ms[i], S, P * 1.14f);
                w.moves.Add((ids[i], ms[i]));
            }
            w.kit = true;
        }

        // Unused by the five fighters: parity does not cover it.
        public static void BuildSwordNormals(CharacterDef c, float S, float P, HitFx smashFx = HitFx.Slash)
        {
            const HitFx sl = HitFx.Slash;
            MoveDef[] m = c.moves;
            List<HitboxDef> Pair(int s0, int e0, float a0, float a1, float rTip, float dTip, float rBase, float dBase, float ang,
                                 float bkb, float kbg, int group, HitFx fx, float tipBkb = -1.0f, float tipKbg = -1.0f) =>
                new List<HitboxDef>
                {
                    Sw(s0, e0, a0, a1, 0.55f, 1.0f, rTip, dTip, ang, tipBkb < 0 ? bkb : tipBkb, tipKbg < 0 ? kbg : tipKbg, group, fx),
                    Sw(s0, e0, a0, a1, 0.0f, 0.55f, rBase, dBase, ang, bkb, kbg, group, fx)
                };
            List<HitboxDef> Join(List<HitboxDef> a, List<HitboxDef> b) { a.AddRange(b); return a; }
            m[Jab] = Mv("Estoc", 20, Pair(4, 7, 12, -4, 16, 4.0f, 16, 3.0f, 361, 8, 35, 0, sl, 10, 40));
            m[Jab].nextMove = Jab2; m[Jab].comboStart = 4; m[Jab].comboEnd = 18; m[Jab].comboAt = 10;
            m[Jab2] = Mv("Revers", 30, Pair(4, 7, 70, -25, 18, 5.0f, 18, 4.0f, 361, 35, 80, 0, sl, 40, 90));
            m[FTilt] = Mv("Taille", 32, Pair(7, 10, 105, -35, 18, 12.0f, 20, 9.0f, 361, 25, 80, 0, sl, 25, 90));
            m[UTilt] = Mv("Arc céleste", 30, Pair(6, 12, 15, 165, 22, 7.5f, 20, 7.0f, 95, 35, 100, 0, sl, 35, 105));
            m[DTilt] = Mv("Fauche", 22, Pair(6, 9, -75, -25, 16, 9.0f, 16, 7.0f, 30, 20, 50, 0, sl, 20, 60));
            m[DashAttack] = Mv("Charge lame", 38, Pair(8, 13, 25, -5, 22, 11.0f, 22, 9.0f, 55, 45, 70, 0, sl, 50, 75));
            m[DashAttack].vel = new List<VelEvent> { new VelEvent { frame = 2, mode = VelMode.SetX, v = new Vec2(9.5f, 0) } };
            m[DashAttack].frictionMul = 0.25f;
            m[FSmash] = Mv("Smash épée", 50, Pair(12, 16, 125, -45, 18, 19.0f, 24, 14.0f, 361, 30, 92, 0, smashFx, 32, 100));
            m[FSmash].chargeFrame = 6;
            m[USmash] = Mv("Pic céleste", 44, Pair(13, 18, 40, 140, 18, 17.0f, 22, 13.0f, 90, 30, 92, 0, smashFx, 30, 98));
            m[USmash].chargeFrame = 6;
            var back = Pair(18, 21, -150, -125, 18, 15.0f, 18, 12.0f, 25, 32, 95, 1, smashFx);
            Edit(back, h => { h.reverse = true; return h; });
            m[DSmash] = Mv("Moulinet bas", 48, Join(Pair(6, 9, -20, -55, 18, 12.0f, 18, 10.0f, 25, 30, 90, 0, sl), back));
            m[DSmash].chargeFrame = 3;
            m[NAir] = Mv("Double taille", 40, Join(Pair(5, 8, 95, -30, 24, 5.0f, 24, 4.0f, 70, 25, 40, 0, sl),
                                                   Pair(14, 18, -60, 75, 26, 9.0f, 26, 7.0f, 361, 30, 95, 1, sl)), 8);
            m[FAir] = Mv("Croissant", 36, Pair(5, 9, 115, -50, 24, 11.0f, 24, 9.0f, 361, 30, 92, 0, sl), 8);
            var bair = Pair(7, 10, 145, 215, 24, 12.5f, 24, 10.0f, 361, 30, 98, 0, sl);
            Edit(bair, h => { h.reverse = true; return h; });
            m[BAir] = Mv("Revers aérien", 36, bair, 10);
            m[UAir] = Mv("Lune montante", 36, Pair(5, 10, 165, 15, 28, 11.0f, 26, 9.0f, 85, 30, 95, 0, sl), 8);
            m[DAir] = Mv("Lame plongeante", 46, Pair(14, 17, -55, -95, 22, 14.0f, 22, 11.0f, 270, 20, 80, 0, sl, 20, 90), 16);
            Edit(m[DAir].hitboxes, h => { h.reach1 = MathF.Min(h.reach1, 0.85f); return h; });
            foreach (int id in new[] { Jab, Jab2, FTilt, UTilt, DTilt, DashAttack, FSmash, USmash, DSmash, NAir, FAir, BAir, UAir, DAir })  // no Jab3
            {
                Scale(m[id], S, P * 0.88f);
                Edit(m[id].hitboxes, h => { h.radius *= 0.95f; h.baseKB *= 0.92f; h.growth *= 0.94f; return h; });
            }
            BuildCommonMoves(c, S, P);
        }

        public static void BuildCommonMoves(CharacterDef c, float S, float P)
        {
            MoveDef[] m = c.moves;
            m[Grab] = Mv("Saisie", 30, new List<HitboxDef> { Hb(6, 9, 40, -6, 22, 0, 0, 0, 0, Limb.ArmFront) });
            m[Grab].isGrab = true;
            m[Pummel] = Mv("Coup de genou", 16, new List<HitboxDef> { Hb(0, 0, 0, 0, 0, 1.5f, 0, 0, 0, Limb.LegFront) });
            m[Pummel].throwFrame = 5; m[Pummel].pummel = true;
            m[FThrow] = Mv("Projection avant", 30, new List<HitboxDef> { Hb(0, 0, 0, 0, 0, 8.0f, 40, 60, 70, Limb.ArmFront) });
            m[FThrow].throwFrame = 10;
            m[BThrow] = Mv("Projection arriere", 36, new List<HitboxDef> { Hb(0, 0, 0, 0, 0, 10.0f, 140, 60, 72, Limb.ArmBack) });
            m[BThrow].throwFrame = 16;
            m[UThrow] = Mv("Projection haute", 32, new List<HitboxDef> { Hb(0, 0, 0, 0, 0, 7.0f, 90, 70, 65, Limb.ArmFront) });
            m[UThrow].throwFrame = 12;
            m[DThrow] = Mv("Projection basse", 34, new List<HitboxDef> { Hb(0, 0, 0, 0, 0, 6.0f, 72, 70, 35, Limb.LegFront) });
            m[DThrow].throwFrame = 14;
            m[LedgeAttack] = Mv("Attaque bord", 32, new List<HitboxDef> { Hb(12, 16, 42, 10, 26, 9.0f, 361, 45, 25, Limb.LegFront) });
            m[LedgeAttack].intangibleStart = 0; m[LedgeAttack].intangibleEnd = 12;
            m[GetupAttack] = Mv("Relevé offensif", 32, new List<HitboxDef>
            {
                Hb(12, 15, 44, 34, 22, 7.0f, 361, 50, 30, Limb.LegFront), Rev(Hb(17, 20, -44, 34, 22, 7.0f, 361, 50, 30, Limb.LegBack, 1))
            });
            m[GetupAttack].intangibleStart = 0; m[GetupAttack].intangibleEnd = 15;
            foreach (int id in new[] { Grab, Pummel, FThrow, BThrow, UThrow, DThrow, LedgeAttack, GetupAttack }) Scale(m[id], S, P);
            // Final smash: a growing shock wave, the last ring launches very hard.
            m[FinalSmash] = Mv("SUPER CLASH", 110, new List<HitboxDef>
            {
                Hb(34, 38, 0, 0, 130, 8, 80, 40, 20, Limb.Body, 0, HitFx.Magic),
                Hb(46, 50, 0, 0, 250, 8, 80, 40, 20, Limb.Body, 1, HitFx.Magic),
                Hb(60, 66, 0, 0, 420, 22, 50, 95, 105, Limb.Body, 2, HitFx.Magic)
            });
            m[FinalSmash].intangibleStart = 0; m[FinalSmash].intangibleEnd = 110;
            m[FinalSmash].vel = new List<VelEvent>
            {
                new VelEvent { frame = 1, mode = VelMode.Set, v = new Vec2(0.0f, -3.0f) },
                new VelEvent { frame = 22, mode = VelMode.Set, v = new Vec2(0.0f, 0.0f) }
            };
            m[FinalSmash].gravityMul = 0.0f; m[FinalSmash].allowDrift = false;
        }
    }
}
