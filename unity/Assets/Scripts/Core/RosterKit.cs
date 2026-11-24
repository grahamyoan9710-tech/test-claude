using System;
using System.Collections.Generic;
using static ClashHorizon.Core.RosterNormals;

namespace ClashHorizon.Core
{
    // Port of src/core/roster_kit.hpp (namespace kit): the small builders the roster files share.
    public static class Kit
    {
        public static void Sc(MoveDef m, float S, float P)
        {
            for (int i = 0; i < m.hitboxes.Count; i++)
            {
                var h = m.hitboxes[i];
                h.offset = h.offset * S; h.radius *= S; h.damage *= P;
                m.hitboxes[i] = h;
            }
            m.projectile.spawnOffset = m.projectile.spawnOffset * S;
            m.projectile.hit.damage *= P;
            m.projectile.explode.damage *= P;
            m.beam.from = m.beam.from * S;
            m.beam.length *= S;
            m.beam.width *= S;
        }

        public static VelEvent Ve(int frame, VelMode mode, float x, float y, float stickX = 0.0f, bool airOnly = false) =>
            new VelEvent { frame = frame, mode = mode, v = new Vec2(x, y), stickX = stickX, airOnly = airOnly };

        public static ProjectileDef Proj(Vec2 spawn, Vec2 vel, float grav, int life, float radius, float dmg, float bkb, float kbg,
                                         HitFx fx, ProjVisual vis)
        {
            ProjectileDef p = ProjectileDef.Default;
            p.spawnOffset = spawn; p.velocity = vel; p.gravity = grav; p.lifetime = life; p.radius = radius;
            p.hit = Hb(0, 0, 0, 0, radius, dmg, 361, bkb, kbg, Limb.None, 0, fx);
            p.visual = vis;
            return p;
        }

        public static Palette Pal(Color3 a, Color3 b, Color3 c, Color3 skin, Color3 hair, Color3 eyes)
        {
            Palette p = Palette.Default;
            p.primary = a; p.secondary = b; p.accent = c; p.skin = skin; p.hair = hair; p.eyes = eyes;
            return p;
        }

        public static HitboxDef Imp(HitboxDef h) { h.impact = true; return h; }
        public static HitboxDef Cap(HitboxDef h) { h.capture = true; return h; }
        public static HitboxDef Lau(HitboxDef h) { h.launch = true; h.impact = true; return h; }

        public static void Multi(List<HitboxDef> v, int start, int n, int every, float ox, float oy, float r, float dmg, Limb limb,
                                 HitFx fx, int g0 = 0, float angle = 361.0f)
        {
            for (int k = 0; k < n; ++k)
                v.Add(Hb(start + k * every, start + k * every + 3, ox, oy, r, dmg, angle, 18, 6, limb, g0 + k, fx));
        }

        public static MoveDef Ultimate(string name, int duration, List<HitboxDef> boxes, PoseHint pose)
        {
            MoveDef m = Mv(name, duration, boxes);
            m.intangibleStart = 0; m.intangibleEnd = duration;
            m.vel = new List<VelEvent> { Ve(1, VelMode.Set, 0.0f, -3.0f), Ve(22, VelMode.Set, 0.0f, 0.0f) };
            m.gravityMul = 0.0f; m.allowDrift = false; m.pose = pose;
            return m;
        }

        // A storm of small hits that grows from r0 to r1, then one launching blast.
        public static List<HitboxDef> Storm(int start, int hits, int every, float r0, float r1, float dmg, HitFx fx, float finalDmg,
                                            float finalR, float angle = 75.0f)
        {
            var v = new List<HitboxDef>();
            for (int k = 0; k < hits; ++k)
            {
                float r = r0 + (r1 - r0) * (float)k / Math.Max(1, hits - 1);
                v.Add(Hb(start + k * every, start + k * every + 3, 0, -10, r, dmg, 361, 10, 0, Limb.None, k, fx));
            }
            int f = start + hits * every + 2;
            v.Add(Imp(Hb(f, f + 6, 0, -10, finalR, finalDmg, angle, 92, 106, Limb.None, hits, fx)));
            return v;
        }

        public static WeaponDef Weapon(string name, string desc, WeaponVisual vis, HitFx fx, float dmg, float kb, float reach) =>
            new WeaponDef { name = name, desc = desc, visual = vis, fx = fx, damageMul = dmg, kbMul = kb, reachMul = reach };

        public static void Stats(CharacterDef c, float w, float h, float weight, float walk, float run, float air, float grav, float maxFall,
                                 float fastFall, float jump, float shortHop, float airJump, int squat)
        {
            c.width = w; c.height = h; c.weight = weight; c.walkSpeed = walk; c.runSpeed = run; c.airSpeed = air; c.gravity = grav;
            c.maxFall = maxFall; c.fastFall = fastFall; c.jumpVel = jump; c.shortHopVel = shortHop; c.airJumpVel = airJump;
            c.jumpSquat = squat;
        }

        // Unused by the roster: parity does not cover it.
        public static void LaunchLast(List<HitboxDef> v)
        {
            int g = 0;
            foreach (var h in v) g = Math.Max(g, h.group);
            for (int i = 0; i < v.Count; i++)
                if (v[i].group == g) { var h = v[i]; h.launch = true; v[i] = h; }
        }

        // A cinematic ultimate: rises during the cut-in freeze, then comes back down.
        public static MoveDef Cine(string name, int duration, List<HitboxDef> boxes, PoseHint pose, UltFx fx)
        {
            MoveDef m = Ultimate(name, duration, boxes, pose);
            m.ultFx = fx;
            m.vel = new List<VelEvent>
            {
                Ve(1, VelMode.Set, 0.0f, -3.0f), Ve(22, VelMode.Set, 0.0f, 0.0f), Ve(30, VelMode.Set, 0.0f, 3.0f), Ve(51, VelMode.Set, 0.0f, 0.0f)
            };
            return m;
        }
    }
}
