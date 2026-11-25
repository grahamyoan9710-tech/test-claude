using System;
using System.Collections.Generic;
using static ClashHorizon.Core.MathUtil;
using static ClashHorizon.Core.MoveId;
using static ClashHorizon.Core.Fighter;
using static ClashHorizon.Core.Swing;

namespace ClashHorizon.Core
{
    // Port of src/core/combat.cpp: hits, knockback, counters, captures and projectiles.
    // The (float) casts keep float32 rounding where a value sets frame timing (Mono computes floats in double).
    public sealed partial class Match
    {
        const float kLaunchScale = 0.15f;  // knockback units -> px/frame
        // Ultimates: a little more damage, less knockback (they KO'd far too early), and they crush shields.
        const float kUltDamageMul = 1.1f;
        const float kUltKnockbackMul = 0.8f;
        const float kUltShieldMul = 2.5f;  // shield cost per damage (an ordinary blow: 1.1)

        // Below 100 % the launches are softened, so that nobody flies out at 50-60 %: the knockback beyond a light hit
        // (combos and jabs keep theirs) shrinks by up to 40 % at 0 %, less and less up to 100 %.
        static float SoftenLowPercent(float kb, float percent)
        {
            const float kLight = 60.0f;
            if (kb <= kLight) return kb;
            float low = Clampf((float)(100.0f - percent) / 100.0f, 0.0f, 1.0f);
            float keep = (float)(1.0f - (float)(0.4f * low));
            return (float)(kLight + (float)((float)(kb - kLight) * keep));
        }

        // Does the hitbox touch the circle / rectangle this frame? hp = contact point (world). Swing hitboxes
        // sweep the blade from its angle on the previous frame to this frame's (no tunnelling through thin
        // targets); the tip is tested first.
        static bool HitboxTouches(Fighter a, in HitboxDef x, Func<Vec2, float, bool> touches, ref Vec2 hp)
        {
            if (!x.swing)
            {
                hp = a.Center() + new Vec2(x.offset.x * a.facing, x.offset.y);
                return touches(hp, x.radius);
            }
            float H = a.Height(), L = a.BladeLength(), t = a.moveFrame;
            float ang1 = SwingAngle(x, t), ang0 = SwingAngle(x, t - 1.0f);
            int sub = MathF.Abs(ang1 - ang0) > 30.0f ? 3 : 2;
            for (int k = sub; k >= 0; --k)
            {
                BladeSeg s = SwingSegment(x, ang0 + (ang1 - ang0) * (float)k / sub, H, L);
                Vec2 p0 = a.Center() + new Vec2(s.a.x * a.facing, s.a.y);
                Vec2 p1 = a.Center() + new Vec2(s.b.x * a.facing, s.b.y);
                int n = Math.Max(1, (int)(float)((p1 - p0).Len() / Maxf(6.0f, x.radius * 0.8f)));
                for (int i = n; i >= 0; --i)
                {
                    Vec2 p = Lerpv(p0, p1, (float)i / n);
                    if (touches(p, x.radius))
                    {
                        hp = p;
                        return true;
                    }
                }
            }
            return false;
        }

        static int HitlagFor(float dmg, float kb)
        {
            int lag = (int)Clampf((float)(dmg * 0.45f) + 4.0f, 4.0f, 18.0f);
            if (kb > 130.0f) lag += 4;  // dramatic freeze on strong hits
            return lag;
        }

        // Super Smash Bros. style knockback formula.
        public static float ComputeKnockback(float p, float d, float w, float g, float b)
        {
            float sum = (float)((float)(p / 10.0f) + (float)((float)(p * d) / 20.0f));
            float scaled = (float)((float)(sum * (float)(200.0f / (float)(w + 100.0f))) * 1.4f);
            return (float)((float)((float)(scaled + 18.0f) * (float)(g / 100.0f)) + b);
        }

        public bool TryCounter(Fighter v, Fighter attacker)
        {
            MoveDef m = v.CurrentMove();
            bool held = m != null && m.parries > 0;  // a held counter keeps parrying until its last parry
            if (m == null || m.counterStart < 0 || (v.counterTriggered && !(held && v.parried < m.parries))) return false;
            if (v.moveFrame < m.counterStart || v.moveFrame >= m.counterEnd) return false;
            v.counterTriggered = true;
            if (held) ++v.parried;
            if (m.fillsWill) v.GainWill(held && v.parried >= m.parries ? kWillMax : 1);
            bool last = !held || v.parried >= m.parries || !v.charging;
            if (last)
            {
                v.charging = false;
                v.moveFrame = m.counterEnd - 1;
            }
            if (held && v.parried >= m.parries) v.intangible = m.duration - m.counterEnd + 20;  // untouchable riposte
            else v.intangible = last ? 20 : 6;  // still on guard: the next blow is parried too
            v.hitlag = 6;
            if (attacker != null)
            {
                v.facing = attacker.pos.x > v.pos.x ? 1 : -1;
                attacker.hitlag = 14;
            }
            Emit(EvType.Counter, v.Center(), v.id, attacker != null ? attacker.id : -1);
            return true;
        }

        public void ResolveHits()
        {
            // h is a copy: a held weapon / a transformation changes its element and knockback.
            // held: the victim is held by this attacker's cinematic ultimate.
            var hits = new List<(int a, int v, HitboxDef h, Vec2 pos, float mul, bool held)>();
            var grabs = new List<(int a, int v)>();
            foreach (Fighter a in fighters)
            {
                MoveDef m = a.CurrentMove();
                if (m == null || !a.Active() || a.hitlag > 0 || a.charging || m.throwFrame >= 0) continue;
                if (m.counterStart >= 0 && !a.counterTriggered) continue;
                float mul = a.DamageMul();
                if (a.move == FinalSmash) mul *= kUltDamageMul / kDamageScale;  // ultimates keep their full power
                if (m.chargeFrame >= 0 && m.parries == 0) mul *= 1.0f + m.chargeDamageMul * (float)a.charge / (float)m.maxCharge;
                if (m.parries > 0) mul *= 1.0f + 0.35f * (float)Math.Max(0, a.parried - 1);  // harder for each parry
                // A held weapon: harder hits; the base moves also reach further and take its element (a
                // transformation's element wins).
                WeaponDef w = a.WeaponDef();
                FormDef fd = a.FormDef();
                bool baseMove = ReferenceEquals(m, a.def.moves[a.move]);
                HitboxDef WorldBox(HitboxDef hb)
                {
                    HitboxDef x = hb;
                    if (w != null)
                    {
                        x.growth *= w.kbMul;
                        if (baseMove)
                        {
                            if (hb.fx == HitFx.Normal || hb.fx == HitFx.Slash) x.fx = w.fx;
                            if (hb.limb == Limb.Weapon || hb.limb == Limb.ArmFront || hb.limb == Limb.ArmBack)
                            {
                                x.offset = x.offset * w.reachMul;
                                x.radius *= MathF.Sqrt(w.reachMul);
                            }
                        }
                    }
                    if (fd != null && fd.fx != HitFx.Normal && baseMove && (hb.fx == HitFx.Normal || hb.fx == HitFx.Slash || w != null))
                        x.fx = fd.fx;
                    return x;
                }
                if (orb.active && !m.isGrab)
                {
                    foreach (HitboxDef h in m.hitboxes)
                    {
                        if (a.moveFrame < h.start || a.moveFrame >= h.end) continue;
                        int key = 63 * 64 + h.group;
                        if (a.hitList.Contains(key)) continue;
                        HitboxDef x = WorldBox(h);
                        Vec2 hp = default;
                        if (!HitboxTouches(a, x, (c, r) => (c - orb.pos).Len() <= r + orb.radius, ref hp)) continue;
                        a.hitList.Add(key);
                        a.hitlag = Math.Max(a.hitlag, 4);
                        HitOrb(a.id, h.damage * mul, a.Center(), h.fx);
                        break;
                    }
                }
                foreach (Fighter v in fighters)
                {
                    if (!Foes(a, v) || !v.Active() || v.state == FState.Respawn || v.IsIntangible()) continue;
                    if (v.state == FState.Grabbed && v.grabPartner == a.id) continue;
                    bool held = v.state == FState.Captured && v.captor == a.id;
                    if (v.state == FState.Captured && !held) continue;  // somebody else's ultimate holds it
                    if (m.isGrab && (v.state == FState.Grabbed || v.state == FState.LedgeHang ||
                                     (!v.grounded && v.pos.y < a.pos.y - 40.0f)))
                        continue;
                    Rect hurt = v.Hurtbox();
                    foreach (HitboxDef h in m.hitboxes)
                    {
                        if (a.moveFrame < h.start || a.moveFrame >= h.end) continue;
                        int key = v.id * 64 + h.group;
                        if (a.hitList.Contains(key)) continue;
                        HitboxDef x = WorldBox(h);
                        Vec2 hp = v.Center();
                        // A held victim is struck by every hitbox of the move, wherever it is.
                        if (!held && !HitboxTouches(a, x, (c, r) => CircleRect(c, r, hurt), ref hp)) continue;
                        if (m.isGrab) grabs.Add((a.id, v.id));
                        else hits.Add((a.id, v.id, x, hp, mul, held));
                        a.hitList.Add(key);
                        break;  // one hitbox per victim per frame (first listed = priority)
                    }
                }
            }
            foreach (var ph in hits)
            {
                Fighter a = fighters[ph.a];
                Fighter v = fighters[ph.v];
                if (!v.Active()) continue;
                bool ult = a.state == FState.Attack && a.move == FinalSmash;
                if (ph.held)
                {
                    if (v.state != FState.Captured || v.captor != a.id) continue;
                    if (!ph.h.launch)
                    {
                        HoldHit(a, v, ph.h, ph.pos, ph.mul);
                        continue;
                    }
                    v.captor = -1;  // released by the launcher
                }
                else if (ph.h.capture && v.state != FState.Shield && v.state != FState.ShieldStun)
                {
                    CaptureFighter(a, v, ph.h, ph.pos, ph.mul);
                    continue;
                }  // (a shield stops the catch: the blow lands on the shield below)
                if (!ult && TryCounter(v, a)) continue;  // ultimates cannot be countered
                int dir = ph.h.reverse ? -a.facing : a.facing;
                ApplyHit(a, v, ph.h, ph.pos, dir, ph.mul, false, ult);
                MoveDef am = a.CurrentMove();
                if (am != null && am.fillsWill && am.counterStart < 0 && !a.willGiven)
                {
                    a.willGiven = true;
                    a.GainWill(1);
                }
            }
            foreach (var (ai, vi) in grabs)
            {
                Fighter a = fighters[ai];
                Fighter v = fighters[vi];
                MoveDef am = a.CurrentMove();
                if (am == null || !am.isGrab) continue;  // got hit meanwhile
                if (v.state == FState.Grabbed || v.state == FState.Captured || !v.Active() || v.IsIntangible()) continue;
                if (v.grabPartner >= 0) ReleaseGrab(v, false);
                SetState(a, FState.Holding);
                a.grabPartner = vi;
                a.vel = new Vec2(0, 0);
                SetState(v, FState.Grabbed);
                v.grabPartner = ai;
                v.escapeTimer = 50 + (int)(float)(v.percent * 0.9f);
                v.vel = v.kbVel = new Vec2(0, 0);
                v.hitstun = 0;
                v.ledge = -1;
                Emit(EvType.Grab, v.Center(), ai, vi);
            }
        }

        // Damage of a hit that does not launch (held victims): the percent and the meters go up, it flinches.
        void DamageOnly(Fighter a, Fighter v, float dmg)
        {
            v.percent = Minf(999.0f, v.percent + dmg);
            GainUlt(v, dmg * kUltPerDamageTaken);
            a.damageDealt += dmg;
            GainUlt(a, dmg * kUltPerDamageDealt);
            v.lastHitBy = a.id;
            v.lastHitFrame = frame;
            v.flash = 10;
        }

        public void CaptureFighter(Fighter a, Fighter v, HitboxDef h, Vec2 hitPos, float damageMul)
        {
            if (v.grabPartner >= 0) ReleaseGrab(v, false);
            float dmg = h.damage * damageMul;
            DamageOnly(a, v, dmg);
            SetState(v, FState.Captured);
            v.captor = a.id;
            v.capturePos = v.pos;
            v.vel = v.kbVel = new Vec2(0, 0);
            v.hitstun = 0;
            v.hitlag = 8;
            v.ledge = -1;
            v.fastFall = false;
            v.grounded = false;
            v.groundPlat = -1;
            v.facing = a.pos.x > v.pos.x ? 1 : -1;
            Emit(EvType.Capture, v.Center(), a.id, v.id);
            Emit(EvType.Hit, hitPos, a.id, v.id, 30.0f + dmg * 2.0f, new Vec2(a.facing, 0), h.fx);
            events[events.Count - 1].impact = h.impact;
        }

        public void HoldHit(Fighter a, Fighter v, HitboxDef h, Vec2 hitPos, float damageMul)
        {
            float dmg = h.damage * damageMul;
            DamageOnly(a, v, dmg);
            v.hitlag = Math.Max(v.hitlag, 3);
            Emit(EvType.Hit, hitPos, a.id, v.id, 20.0f + dmg * 3.0f, new Vec2(a.facing, 0), h.fx);
            events[events.Count - 1].impact = h.impact;
        }

        // -1 if nobody can be targeted.
        public int NearestOpponent(Fighter f)
        {
            int best = -1;
            float bd = 1e18f;
            foreach (Fighter o in fighters)
            {
                if (!Foes(f, o) || !o.Active() || o.state == FState.Respawn) continue;
                float d = (o.Center() - f.Center()).Len2();
                if (d < bd)
                {
                    bd = d;
                    best = o.id;
                }
            }
            return best;
        }

        public void ApplyHit(Fighter attacker, Fighter v, HitboxDef h, Vec2 hitPos, int dirSign, float damageMul,
                             bool throwHit, bool ult = false)
        {
            float dmg = h.damage * damageMul;
            bool shielding = (v.state == FState.Shield || v.state == FState.ShieldStun) && !throwHit;
            if (shielding)
            {
                // A shield blocks an ultimate too, but pays dearly: 250 % of the damage of each blow (an ultimate breaks
                // a full shield); an ordinary blow costs 110 % of its damage.
                v.shield -= ult ? (float)(dmg * kUltShieldMul) : (float)(dmg * 1.1f);
                int slag = HitlagFor(dmg, 0.0f);
                v.hitlag = slag;
                if (attacker != null) attacker.hitlag = slag;
                v.vel.x = dirSign * Minf(10.0f, 1.5f + dmg * 0.35f);
                if (attacker != null) GainUlt(attacker, dmg * kUltPerShieldHit);
                Emit(EvType.ShieldHit, hitPos, attacker != null ? attacker.id : -1, v.id, dmg, new Vec2(dirSign, 0), h.fx);
                if (v.shield <= 0.0f)
                {
                    v.shield = 0.0f;
                    SetState(v, FState.ShieldBreak, 220);
                    v.vel = new Vec2(0, -9);
                    v.grounded = false;
                    v.groundPlat = -1;
                    Emit(EvType.ShieldBreak, v.Center(), v.id);
                }
                else
                {
                    SetState(v, FState.ShieldStun, (int)(float)((float)(dmg * 0.6f) + 3.0f));
                }
                return;
            }
            v.percent = Minf(999.0f, v.percent + dmg);
            GainUlt(v, dmg * kUltPerDamageTaken);
            if (attacker != null)
            {
                attacker.damageDealt += dmg;
                GainUlt(attacker, dmg * kUltPerDamageDealt);
                v.lastHitBy = attacker.id;
                v.lastHitFrame = frame;
            }
            FormDef vf = v.FormDef();
            float kb = ComputeKnockback(v.percent, dmg, v.def.weight, h.growth, h.baseKB) * (vf != null ? vf.kbTakenMul : 1.0f);
            MoveDef vm = v.CurrentMove();
            bool moveArmor = vm != null && vm.armorStart >= 0 && v.moveFrame >= vm.armorStart && v.moveFrame < vm.armorEnd;
            bool formArmor = vf != null && vf.superArmor && kb < 95.0f && !ult;  // only big blows (and ultimates) make it flinch
            if (!throwHit && (moveArmor || formArmor))
            {
                int alag = HitlagFor(dmg, 0.0f);
                v.hitlag = alag;
                if (attacker != null) attacker.hitlag = alag;
                v.flash = 8;
                v.flashFx = h.fx;
                Emit(EvType.Hit, hitPos, attacker != null ? attacker.id : -1, v.id, dmg * 0.5f, new Vec2(dirSign, 0), h.fx);
                return;
            }
            if (ult) kb *= kUltKnockbackMul;
            kb = SoftenLowPercent(kb, v.percent);
            float ang = h.angle;
            if (ang > 360.0f) ang = v.grounded ? (kb < 60.0f ? 5.0f : 40.0f) : 45.0f;
            if (v.grounded && ang > 180.0f && ang < 360.0f && !throwHit) ang = 80.0f;  // grounded spike pops up
            Vec2 dir = new Vec2(MathF.Cos(Deg2rad(ang)) * dirSign, -MathF.Sin(Deg2rad(ang)));
            Vec2 s = v.@in.stick;  // Directional Influence
            if (s.Len() > 0.2f)
            {
                Vec2 perp = new Vec2(-dir.y, dir.x);
                float amt = Dot(s.Norm(), perp) * Minf(1.0f, s.Len());
                dir = (dir + perp * (amt * 0.27f)).Norm();
            }
            if (v.grabPartner >= 0 && !throwHit) ReleaseGrab(v, false);
            SetState(v, FState.Hitstun);
            v.kbVel = dir * (kb * kLaunchScale);
            v.vel = new Vec2(0, 0);
            v.hitstun = Math.Max(5, (int)(float)(kb * 0.45f));  // a little longer than Smash: room to chain (the owner, 11/11)
            v.lastKnockback = kb;
            v.tumble = kb > 80.0f;
            v.ledge = -1;
            v.fastFall = false;
            v.spin = 0.0f;
            if (v.kbVel.y < -1.0f || v.tumble)
            {
                v.grounded = false;
                v.groundPlat = -1;
            }
            v.facing = dirSign > 0 ? -1 : 1;
            int lag = HitlagFor(dmg, kb);
            v.hitlag = lag;
            if (attacker != null && !throwHit)
            {
                attacker.hitlag = lag;
                attacker.landedHit = true;
            }
            v.flash = 10;
            v.flashFx = h.fx;
            Emit(EvType.Hit, hitPos, attacker != null ? attacker.id : -1, v.id, kb, dir, h.fx);
            events[events.Count - 1].impact = h.impact;
        }

        // Damage credit for projectiles and explosions (no attacker in ApplyHit).
        public void CreditProjectile(int owner, Fighter v, float damage, bool wasShielding)
        {
            if (owner < 0 || owner >= fighters.Count) return;
            if (!wasShielding)
            {
                fighters[owner].damageDealt += damage;
                GainUlt(fighters[owner], damage * kUltPerDamageDealt);
                v.lastHitBy = owner;
                v.lastHitFrame = frame;
            }
            else
            {
                GainUlt(fighters[owner], damage * kUltPerShieldHit);
            }
        }

        public void ExplodeProjectile(Projectile pr)
        {
            if (pr.explodeRadius <= 0.0f) return;
            float r = pr.explodeRadius;
            pr.explodeRadius = 0.0f;  // once
            Emit(EvType.Explosion, pr.pos, pr.owner, -1, r, default, pr.explode.fx);
            events[events.Count - 1].visual = (int)pr.visual;
            foreach (Fighter v in fighters)
            {
                if (!Foes(pr.owner, v) || !v.Active() || v.state == FState.Respawn || v.IsIntangible()) continue;
                if (!CircleRect(pr.pos, r, v.Hurtbox())) continue;
                if (v.state == FState.Captured && v.captor != pr.owner) continue;
                if (v.state == FState.Captured) v.captor = -1;  // the explosion launches it out of the hold
                bool wasShielding = !pr.ult && (v.state == FState.Shield || v.state == FState.ShieldStun);
                ApplyHit(null, v, pr.explode, pr.pos, v.Center().x >= pr.pos.x ? 1 : -1, 1.0f, false, pr.ult);
                CreditProjectile(pr.owner, v, pr.explode.damage, wasShielding);
            }
        }

        public void SpawnProjectiles(Fighter f, ProjectileDef p)
        {
            // Where it appears: in front of the fighter, or on its target(s).
            var targets = new List<int>();
            if (p.target == ProjTarget.Nearest)
            {
                targets.Add(NearestOpponent(f));
            }
            else if (p.target == ProjTarget.All)
            {
                foreach (Fighter o in fighters)
                    if (Foes(f, o) && o.Active() && o.state != FState.Respawn) targets.Add(o.id);
                if (targets.Count == 0) targets.Add(-1);
            }
            else
            {
                targets.Add(-2);  // self
            }
            bool ult = f.state == FState.Attack && f.move == FinalSmash;
            float dmgMul = f.DamageMul() * (ult ? kUltDamageMul / kDamageScale : 1.0f);  // ultimates keep their full power
            foreach (int t in targets)
            {
                Vec2 origin;
                int side = f.facing;
                if (t == -2)
                {
                    origin = f.Center() + new Vec2(p.spawnOffset.x * f.facing, p.spawnOffset.y);
                }
                else if (t >= 0)
                {
                    Fighter o = fighters[t];
                    origin = o.Center() + p.spawnOffset;
                    side = o.pos.x >= f.pos.x ? 1 : -1;  // strikes come from the user's side
                    if (p.column)
                    {  // lands on the first floor under them (kept at their height when nothing is under them)
                        float floor = 1e9f;
                        foreach (Platform pl in platforms)
                            if (o.pos.x >= pl.rect.Left() && o.pos.x <= pl.rect.Right() && pl.rect.Top() >= o.pos.y - 1.0f)
                                floor = Minf(floor, pl.rect.Top());
                        if (floor < 1e8f) origin.y = floor + p.spawnOffset.y;
                    }
                }
                else
                {  // nobody to target: a few steps ahead
                    origin = f.Center() + new Vec2(f.facing * 220.0f, 0.0f) + p.spawnOffset;
                }
                for (int i = 0; i < p.count; ++i)
                {
                    float off = p.count > 1 ? ((float)i - (float)(p.count - 1) * 0.5f) * p.spreadDeg : 0.0f;
                    Vec2 v = Rotate(p.velocity, Deg2rad(-off));
                    var pr = new Projectile();
                    pr.owner = f.id;
                    pr.pos = origin;
                    if (p.jitter.x > 0.0f) pr.pos.x += rng.Range(-p.jitter.x, p.jitter.x);
                    if (p.jitter.y > 0.0f) pr.pos.y += rng.Range(-p.jitter.y, p.jitter.y);
                    pr.vel = new Vec2(v.x * side, v.y);
                    pr.gravity = p.gravity;
                    pr.radius = p.radius;
                    pr.life = pr.maxLife = p.lifetime;
                    pr.hit = p.hit;
                    pr.bounces = p.bounces;
                    pr.piercing = p.piercing;
                    pr.visual = p.visual;
                    pr.fx = p.fx;
                    pr.rehit = p.rehit;
                    pr.pull = p.pull;
                    pr.pullRadius = p.pullRadius;
                    pr.boomerang = p.boomerang;
                    pr.grow = p.grow;
                    pr.explodeRadius = p.explodeRadius;
                    pr.explode = p.explode;
                    pr.spawnRadius = p.radius;
                    pr.follow = p.follow;
                    pr.armDelay = p.armDelay;
                    pr.ignoreGround = p.ignoreGround;
                    pr.column = p.column;
                    pr.ult = ult;
                    pr.hit.damage *= dmgMul;
                    pr.explode.damage *= dmgMul;
                    projectiles.Add(pr);
                }
            }
            Emit(EvType.Projectile, f.Center(), f.id, -1, 0.0f, default, p.hit.fx);
        }

        public void UpdateProjectiles()
        {
            Rect blast = stageDef.blast;
            for (int pi = 0; pi < projectiles.Count; ++pi)  // an index loop: a List throws if a hit ever adds one
            {
                Projectile pr = projectiles[pi];
                if (!pr.alive) continue;
                ++pr.age;
                if (!pr.Armed()) continue;  // the warning before it strikes
                // Stuck on its victim (black flames, vortices): rides along, dies with it.
                if (pr.stuckTo >= 0)
                {
                    Fighter t = fighters[pr.stuckTo];
                    if (!t.Active() || t.state == FState.Respawn)
                    {
                        pr.alive = false;
                        continue;
                    }
                    pr.pos = t.Center() + pr.stickOffset;
                    pr.vel = new Vec2(0, 0);
                }
                pr.radius += pr.grow;
                pr.hit.radius = pr.radius;
                // Boomerang: after half its flight it homes back to its owner's hand.
                if (pr.boomerang && pr.owner >= 0)
                {
                    Fighter o = fighters[pr.owner];
                    if (!pr.returning && pr.age >= pr.maxLife / 2)
                    {
                        pr.returning = true;
                        pr.hitList.Clear();
                    }
                    if (pr.returning)
                    {
                        Vec2 d = o.Center() - pr.pos;
                        if (d.Len() < 34.0f || !o.Active() || pr.age > pr.maxLife * 3)
                        {
                            pr.alive = false;
                            continue;
                        }
                        pr.vel = d.Norm() * Maxf(12.0f, pr.vel.Len());
                        pr.life = Math.Max(pr.life, 2);
                    }
                }
                if (pr.stuckTo < 0) pr.vel.y += pr.gravity;
                Vec2 prev = pr.pos;
                pr.pos += pr.vel;
                if (!blast.Contains(pr.pos))
                {
                    pr.alive = false;
                    continue;
                }
                if (--pr.life <= 0)
                {
                    pr.alive = false;
                    ExplodeProjectile(pr);
                    continue;
                }
                if (!pr.boomerang && !pr.ignoreGround && pr.stuckTo < 0)
                {
                    foreach (Platform p in platforms)
                    {
                        if (!CircleRect(pr.pos, pr.spawnRadius * 0.6f, p.rect)) continue;
                        bool fromAbove = prev.y + pr.spawnRadius * 0.6f <= p.rect.Top() + 2.0f && pr.vel.y >= 0.0f;
                        if (pr.bounces && fromAbove)
                        {
                            pr.pos.y = p.rect.Top() - pr.spawnRadius * 0.6f;
                            pr.vel.y = -6.5f;
                        }
                        else if (p.solid || (pr.explodeRadius > 0.0f && fromAbove))
                        {
                            pr.alive = false;
                        }
                    }
                }
                if (!pr.alive)
                {
                    ExplodeProjectile(pr);
                    continue;
                }
                // Vortex: drags the fighters around it
                if (pr.pull > 0.0f)
                {
                    foreach (Fighter v in fighters)
                    {
                        if (!Foes(pr.owner, v) || !v.Active() || v.state == FState.Respawn || v.IsIntangible() ||
                            v.state == FState.Grabbed || v.state == FState.LedgeHang || v.state == FState.Captured)
                            continue;
                        Vec2 d = pr.pos - v.Center();
                        float l = d.Len();
                        if (l > pr.pullRadius || l < 4.0f) continue;
                        Vec2 a = d * (pr.pull * 0.25f / l);
                        v.vel.x += a.x;
                        if (!v.grounded) v.vel.y += a.y;
                    }
                }
                if (orb.active && (pr.pos - orb.pos).Len() < pr.radius + orb.radius && !pr.hitList.Contains(63))
                {
                    pr.hitList.Add(63);
                    HitOrb(pr.owner, pr.hit.damage, pr.pos - pr.vel, pr.hit.fx);
                    if (!pr.piercing)
                    {
                        pr.alive = false;
                        ExplodeProjectile(pr);
                        continue;
                    }
                }
                foreach (Fighter v in fighters)
                {
                    if (!Foes(pr.owner, v) || !v.Active() || v.state == FState.Respawn || v.IsIntangible()) continue;
                    if (v.state == FState.Captured && v.captor != pr.owner) continue;
                    if (pr.stuckTo >= 0 && v.id != pr.stuckTo) continue;
                    if (pr.rehit > 0)
                    {
                        int li = pr.lastHit.FindIndex(e => e.id == v.id);
                        if (li >= 0 && frame - pr.lastHit[li].frame < pr.rehit) continue;
                    }
                    else if (pr.hitList.Contains(v.id))
                    {
                        continue;
                    }
                    Rect sky = new Rect(pr.pos.x - pr.radius, pr.pos.y - 4000.0f, 2.0f * pr.radius, 4000.0f + pr.radius);
                    if (pr.column ? !sky.Overlaps(v.Hurtbox()) : !CircleRect(pr.pos, pr.radius, v.Hurtbox())) continue;
                    if (pr.rehit > 0)
                    {
                        int li = pr.lastHit.FindIndex(e => e.id == v.id);
                        if (li >= 0) pr.lastHit[li] = (v.id, frame);
                        else pr.lastHit.Add((v.id, frame));
                    }
                    else
                    {
                        pr.hitList.Add(v.id);
                    }
                    if (!pr.ult && TryCounter(v, null))
                    {
                        MoveDef cm = v.CurrentMove();
                        if (cm != null && cm.reflects)
                        {  // sent back to the sender, faster
                            pr.owner = v.id;
                            pr.vel = pr.vel * -1.15f;
                            pr.life = pr.maxLife;
                            pr.age = 0;
                            pr.boomerang = pr.returning = false;
                            pr.hitList.Clear();
                            pr.lastHit.Clear();
                            Emit(EvType.Reflect, pr.pos, v.id, -1, 0.0f, pr.vel.Norm(), pr.hit.fx);
                        }
                        else
                        {
                            pr.alive = false;
                        }
                        break;
                    }
                    int dir = pr.vel.x >= 0.0f ? 1 : -1;
                    if (pr.stuckTo >= 0 || MathF.Abs(pr.vel.x) < 0.5f) dir = v.Center().x >= pr.pos.x ? 1 : -1;
                    bool wasShielding = v.state == FState.Shield || v.state == FState.ShieldStun;
                    if (v.state == FState.Captured) v.captor = -1;  // the user's own shot launches it out of the hold
                    ApplyHit(null, v, pr.hit, pr.pos, dir, 1.0f, false, pr.ult);
                    CreditProjectile(pr.owner, v, pr.hit.damage, wasShielding);
                    if (pr.follow && pr.stuckTo < 0 && v.Active())
                    {  // latches on
                        pr.stuckTo = v.id;
                        pr.stickOffset = (pr.pos - v.Center()) * 0.25f;
                    }
                    if (!pr.piercing)
                    {
                        pr.alive = false;
                        ExplodeProjectile(pr);
                        break;
                    }
                }
            }
            projectiles.RemoveAll(x => !x.alive);
        }
    }
}
