using System.Collections.Generic;
using static ClashHorizon.Core.Kit;
using static ClashHorizon.Core.RosterNormals;

namespace ClashHorizon.Core
{
    // Port of src/core/roster_ultimates.cpp. Frames 1-30 are the cut-in freeze; the action starts around frame 34.
    // Positions are not scaled by the fighter's size: an ultimate fills the screen.
    public static class Ult
    {
        static ProjectileDef StrikeOnAll(float radius, int warn, int life, float dmg, float bkb, float kbg, HitFx e, ProjVisual vis, string fx)
        {
            ProjectileDef p = Proj(new Vec2(0, 0), new Vec2(0, 0), 0, life, radius, dmg, bkb, kbg, e, vis);
            p.target = ProjTarget.All; p.armDelay = warn; p.piercing = true; p.ignoreGround = true; p.hit.angle = 80; p.fx = fx;
            return p;
        }

        static void Effect(MoveDef m, string fx, int frame) { m.castFx = fx; m.castFxFrame = frame; m.castFxScale = 1.6f; }

        // `Form` here is this method: the enum needs its full name.
        public static int Form(CharacterDef c, string aura, HitFx fx, int seconds, float damage, float kbTaken)
        {
            c.forms.Add(new FormDef
            {
                form = global::ClashHorizon.Core.Form.Awakened, duration = seconds * 60, fx = fx, damageMul = damage, kbTakenMul = kbTaken,
                aura = aura
            });
            return c.forms.Count - 1;
        }

        public static MoveDef Awaken(string name, int formIndex, HitFx e, string fx)
        {
            MoveDef m = Cine(name, 76, new List<HitboxDef> { Imp(Hb(38, 44, 0, -10, 200, 14, 60, 52, 62, Limb.Body, 0, e)) },
                             PoseHint.Roar, UltFx.Cinematic);
            m.formIndex = formIndex; m.formFrame = 34;
            Effect(m, fx, 34);
            return m;
        }

        public static MoveDef Ambush(string name, HitFx e, string fx, Limb limb, int hits)
        {
            const int tp = 34, every = 5;
            var v = new List<HitboxDef> { Cap(Hb(tp, tp + 6, 46, -6, 58, 6, 0, 0, 0, limb, 0, e)) };
            Multi(v, tp + 9, hits, every, 50, -6, 52, 2.6f, limb, e, 1, 0);
            int f = tp + 9 + hits * every + 4;
            v.Add(Lau(Hb(f, f + 6, 50, -20, 90, 22, 55, 94, 108, limb, hits + 1, e)));
            MoveDef m = Cine(name, f + 28, v, PoseHint.Rush, UltFx.Cinematic);
            m.teleportFrame = tp; m.teleportToTarget = true; m.holdAt = new Vec2(40, -10);
            Effect(m, fx, tp);
            return m;
        }

        public static MoveDef Storm(string name, HitFx e, string fx, float r0, float r1)
        {
            MoveDef m = Cine(name, 104, Kit.Storm(38, 8, 5, r0, r1, 3.0f, e, 24, r1 + 40), PoseHint.Raise, UltFx.Cinematic);
            Effect(m, fx, 34);
            return m;
        }

        // Unused by the roster: parity does not cover it.
        public static MoveDef Beam(string name, HitFx e, string fx, float angle, float length, float width)
        {
            BeamDef b = BeamDef.Default;
            b.start = 38; b.end = 88; b.from = angle > 45.0f ? new Vec2(0, -40) : new Vec2(40, -20);
            b.angle = angle; b.length = length; b.width = width; b.fx = e; b.sprite = fx;
            MoveDef m = Cine(name, 100, BeamHits(b, 24, angle > 45.0f ? 85.0f : 30.0f, 95, 110, 6), PoseHint.Cast, UltFx.Cinematic);
            m.beam = b;
            return m;
        }

        public static MoveDef Rain(string name, HitFx e, string fx, ProjVisual vis, int strikes)
        {
            MoveDef m = Cine(name, 112, new List<HitboxDef>(), PoseHint.Raise, UltFx.Cinematic);
            for (int k = 0; k < strikes; ++k)
            {
                ProjectileDef p = StrikeOnAll(54, 14, 26, 4, 20, 10, e, vis, fx);
                p.jitter = new Vec2(90, 0);
                m.shots.Add(new ProjShot { frame = 36 + k * 7, def = p });
            }
            ProjectileDef last = StrikeOnAll(110, 18, 34, 20, 92, 104, e, vis, fx);
            last.hit.impact = true;
            m.shots.Add(new ProjShot { frame = 36 + strikes * 7 + 4, def = last });
            return m;
        }

        public static MoveDef Downpour(string name, HitFx e, string fx, float radius, int hits)
        {
            const int warn = 14, every = 12;
            MoveDef m = Cine(name, 36 + warn + hits * every + 12, new List<HitboxDef>(), PoseHint.Raise, UltFx.Cinematic);
            ProjectileDef p = StrikeOnAll(radius, warn, warn + hits * every, 2.4f, 20, 10, e, ProjVisual.Spear, fx);
            p.rehit = every;
            p.explodeRadius = radius + 30.0f;
            p.explode = Imp(Hb(0, 0, 0, 0, radius + 30.0f, 20, 80, 92, 104, Limb.None, 0, e));
            m.shots.Add(new ProjShot { frame = 36, def = p });
            return m;
        }

        public static MoveDef Thunder(string name, HitFx e, string fx, int warn, int bolts)
        {
            const int every = 10;
            int life = warn + 20;
            MoveDef m = Cine(name, 36 + (bolts - 1) * every + life, new List<HitboxDef>(), PoseHint.Raise, UltFx.Cinematic);
            for (int k = 0; k < bolts; ++k)
            {
                bool last = k == bolts - 1;
                ProjectileDef p = last ? StrikeOnAll(60, warn, life, 16, 92, 104, e, ProjVisual.Spear, fx)
                                       : StrikeOnAll(60, warn, life, 5, 25, 8, e, ProjVisual.Spear, fx);
                p.column = true;
                p.hit.impact = last;
                m.shots.Add(new ProjShot { frame = 36 + k * every, def = p });
            }
            return m;
        }

        public static MoveDef Blast(string name, HitFx e, string fx, ProjVisual vis, float speed, float radius, float explode)
        {
            MoveDef m = Cine(name, 104, new List<HitboxDef>(), PoseHint.Throw, UltFx.Cinematic);
            m.projectileFrame = 54;
            m.projectile = Proj(new Vec2(60, -20), new Vec2(speed, 0), 0, 70, radius, 6, 30, 20, e, vis);
            m.projectile.fx = fx;
            m.projectile.explodeRadius = explode;
            m.projectile.explode = Imp(Hb(0, 0, 0, 0, explode, 24, 70, 94, 108, Limb.None, 0, e));
            return m;
        }
    }
}
