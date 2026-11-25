// Port of src/core/ai.hpp and ai.cpp: the CPU players (and the training dummy).
using System;
using static ClashHorizon.Core.MathUtil;
using static ClashHorizon.Core.MoveId;
using static ClashHorizon.Core.Swing;

namespace ClashHorizon.Core
{
    public sealed class CpuBrain
    {
        // The C++ enum Dummy (an enum would clash with the Dummy() method)
        public const int kDummyCpu = 0, kDummyStand = 1, kDummyJump = 2, kDummyShield = 3, kDummyCount = 4;

        public CpuBrain(int level = 5, uint seed = 1)
        {
            level_ = Math.Clamp(level, 1, Fighter.kMaxCpuLevel);
            skill_ = Math.Clamp(level, 1, 9) / 9.0f;  // level 10 plays like 9 (its double damage is the match's)
            rng_ = new Rng(seed * 2654435761u + 7u);
        }

        // Training dummy: kDummyCpu plays, the others stand, jump or shield
        public void SetDummy(int mode) { dummy_ = mode; }
        public int Dummy() => dummy_;

        int level_;
        float skill_;  // 0..1
        Rng rng_;
        InputState last_;
        int decisionTimer_ = 0;
        int weaponTimer_ = -1;  // frames before the next weapon decision (-1: not drawn yet)
        int shieldHold_ = 0;
        int chargeHold_ = 0;
        Vec2 chargeDir_;
        int waitTimer_ = -1;
        int projCooldown_ = 0;  // frames before firing another projectile special (no spamming)
        int crossTimer_ = 0;    // jumping over a gap toward the target: make for the ledges on that side
        int ultKey_ = -1;       // the opponent's ultimate already seen, and whether to shield it
        bool ultBlock_ = false;
        float crossDir_ = 0.0f;
        int dummy_ = kDummyCpu;

        // A tap is a press on a frame the button was up: written inline as @in.x = !last_.x.
        void TapCStick(ref InputState @in, Vec2 dir) { @in.cstick = last_.cstick.Len() > 0.5f ? new Vec2() : dir; }
        void Cross(ref InputState @in, float dir)  // jump over a gap toward dir; Recover() then makes for the far side
        {
            @in.jump = !last_.jump;
            crossDir_ = dir;
            crossTimer_ = 150;
        }

        // How far in front a move hits (swings: the blade tip over the whole arc).
        static float MoveReach(Fighter f, MoveDef m)
        {
            float r = 0.0f;
            foreach (var h in m.hitboxes)
            {
                if (!h.swing)
                {
                    r = Maxf(r, MathF.Abs(h.offset.x) + h.radius);
                    continue;
                }
                for (int k = 0; k <= 8; ++k)
                {
                    var s = SwingSegment(h, h.a0 + (h.a1 - h.a0) * k / 8.0f, f.Height(), f.BladeLength());
                    r = Maxf(r, Maxf(MathF.Abs(s.a.x), MathF.Abs(s.b.x)) + h.radius);
                }
            }
            return r;
        }

        static bool HasProjectile(CharacterDef d) => d.moves[NSpecial].projectileFrame >= 0;

        static bool SideSpecialTravels(CharacterDef d)
        {
            foreach (var e in d.moves[SSpecial].vel)
                if (e.mode == VelMode.SetX && e.v.x > 8.0f) return true;
            return false;
        }

        // What a special does, for choosing when to use it.
        struct SpecialUse
        {
            public bool projectile, dash, counter, grab, beam, melee;
            public float reach;
            public float landX;  // dashes: distance at which the finishing hit lands
        }

        static SpecialUse SpecialUseOf(Fighter f, int id)
        {
            MoveDef m = f.MoveDef(id);
            var u = new SpecialUse();
            if (!m.Exists() || f.moveCooldown[id] > 0) return u;  // recharging: as if it did not exist
            u.projectile = m.projectileFrame >= 0;
            u.counter = m.counterStart >= 0;
            u.grab = m.isGrab;
            u.beam = m.beam.start >= 0;
            int f0 = -1, fStop = 1000;
            float vx = 0.0f;
            foreach (var e in m.vel)
            {
                if (e.mode == VelMode.SetX && e.v.x > 8.0f && f0 < 0)
                {
                    u.dash = true;
                    f0 = e.frame;
                    vx = e.v.x;
                }
                else if (f0 >= 0 && (e.mode == VelMode.Scale || e.mode == VelMode.Set) && e.frame > f0)
                    fStop = Math.Min(fStop, e.frame);
            }
            bool hasFin = m.hitboxes.Count > 0;
            HitboxDef fin = hasFin ? m.hitboxes[m.hitboxes.Count - 1] : HitboxDef.Default;
            if (u.dash && hasFin) u.landX = vx * (float)Math.Max(0, Math.Min(fin.start, fStop) - f0) + fin.offset.x;
            if (m.teleportForward)
            {
                u.dash = true;
                u.landX = m.teleportDist - (hasFin ? fin.offset.x : 0.0f);
            }
            u.melee = m.hitboxes.Count > 0 && !u.counter && !u.grab;
            u.reach = MoveReach(f, m);
            if (u.beam) u.reach = m.beam.from.x + m.beam.length;
            return u;
        }

        // How far the ultimate reaches: anywhere (homing, teleport), ahead (a shot or a beam) or around.
        struct UltReach
        {
            public float range;
            public bool anywhere, ahead;
        }

        static UltReach UltReachOf(Fighter f)
        {
            MoveDef m = f.MoveDef(FinalSmash);
            var u = new UltReach { range = 320.0f };
            if (m.teleportToTarget) u.anywhere = true;
            if (m.projectileFrame >= 0)
            {
                if (m.projectile.target != ProjTarget.Self) u.anywhere = true;
                else
                {
                    u.ahead = true;
                    u.range = Maxf(u.range, MathF.Abs(m.projectile.velocity.x) * (float)m.projectile.lifetime * 0.8f);
                }
            }
            foreach (var sh in m.shots)
                if (sh.def.target != ProjTarget.Self) u.anywhere = true;
            if (m.beam.start >= 0)
            {
                u.ahead = true;
                u.range = Maxf(u.range, m.beam.from.x + m.beam.length * 0.9f);
            }
            float r = 0.0f;
            foreach (var h in m.hitboxes)
                if (h.limb == Limb.None || h.limb == Limb.Body) r = Maxf(r, MathF.Abs(h.offset.x) + h.radius);
            if (!u.ahead) u.range = Maxf(u.range, r * 0.85f);
            return u;
        }

        // t attacks with hitboxes still to come, and f is within their reach.
        static bool IsThreat(Fighter t, Fighter f)
        {
            MoveDef m = t.CurrentMove();
            if (m == null || m.hitboxes.Count == 0 || m.isGrab) return false;
            int lastEnd = 0;
            foreach (var h in m.hitboxes) lastEnd = Math.Max(lastEnd, h.end);
            if (t.moveFrame >= lastEnd) return false;
            float reach = MoveReach(t, m) + 30.0f;
            Vec2 d = f.Center() - t.Center();
            return MathF.Abs(d.x) < reach && MathF.Abs(d.y) < reach;
        }

        static float NearestPlatformX(Match m, Vec2 p)
        {
            float best = 0.0f, bd = 1e9f;
            foreach (var pl in m.platforms)
            {
                float x = Clampf(p.x, pl.rect.Left() + 20.0f, pl.rect.Right() - 20.0f);
                float d = MathF.Abs(x - p.x) + MathF.Abs(pl.rect.Top() - p.y) * 0.5f;
                if (d < bd)
                {
                    bd = d;
                    best = x;
                }
            }
            return best;
        }

        static int NearestLedge(Match m, Vec2 p)
        {
            int best = -1;
            float bd = 1e9f;
            for (int l = 0; l < m.LedgeCount(); ++l)
            {
                if (!m.LedgeValid(l)) continue;
                float d = (m.LedgePoint(l) - p).Len2();
                if (d < bd)
                {
                    bd = d;
                    best = l;
                }
            }
            return best;
        }

        // The closest ledge on the dir side of p (-1: none).
        static int LedgeToward(Match m, Vec2 p, float dir)
        {
            int best = -1;
            float bd = 1e9f;
            for (int l = 0; l < m.LedgeCount(); ++l)
            {
                if (!m.LedgeValid(l) || (m.LedgePoint(l).x - p.x) * dir <= 0.0f) continue;
                float d = (m.LedgePoint(l) - p).Len2();
                if (d < bd)
                {
                    bd = d;
                    best = l;
                }
            }
            return best;
        }

        // Walking toward dir: 0 ground goes on, 1 a gap with ground beyond (jumpable), -1 the void.
        static int EdgeAhead(Match m, Vec2 p, float dir)
        {
            if (m.GroundBelow(new Vec2(p.x + dir * 40.0f, p.y), 400.0f)) return 0;
            for (float x = 60.0f; x <= 260.0f; x += 20.0f)
                if (m.GroundBelow(new Vec2(p.x + dir * x, p.y - 160.0f), 560.0f)) return 1;
            return -1;
        }

        // How far a weapon's neutral special flies (0: no projectile): the "distance" weapon is the one that reaches further.
        static float WeaponRange(WeaponDef w)
        {
            foreach (var (id, mv) in w.moves)
                if (id == NSpecial && mv.projectileFrame >= 0) return MathF.Abs(mv.projectile.velocity.x) * mv.projectile.lifetime;
            return 0.0f;
        }

        public InputState Think(Match m, int self)
        {
            var @in = new InputState();
            Fighter f = m.fighters[self];
            if (!m.Started() || !f.Active() || m.finished)
            {
                last_ = @in;
                return @in;
            }
            if (dummy_ != kDummyCpu)  // training dummy
            {
                bool free = f.state == FState.Idle || f.state == FState.Walk || f.state == FState.Landing;
                if (f.state == FState.Respawn && f.stateFrame > 60) @in.stick.y = 1.0f;
                if (dummy_ == kDummyJump && free && f.grounded) @in.jump = !last_.jump;
                if (dummy_ == kDummyShield && f.grounded && f.state != FState.Respawn) @in.shield = true;
                last_ = @in;
                return @in;
            }

            switch (f.state)
            {
                case FState.Respawn:
                    if (f.stateFrame > 40 + rng_.Irange(0, 60)) @in.stick.y = 1.0f;
                    last_ = @in;
                    return @in;
                case FState.Hitstun:
                {
                    // DI toward the stage
                    Vec2 toCenter = (new Vec2(0, -200) - f.pos).Norm();
                    @in.stick = toCenter * skill_;
                    if (f.tumble && m.GroundBelow(f.pos, 60.0f) && rng_.Chance(skill_ * 0.5f)) @in.shield = !last_.shield;
                    last_ = @in;
                    return @in;
                }
                case FState.Grabbed:
                    if (rng_.Chance(0.3f + 0.6f * skill_)) @in.attack = !last_.attack;  // mash
                    last_ = @in;
                    return @in;
                case FState.Holding:
                {
                    if (waitTimer_ < 0) waitTimer_ = rng_.Irange(4, 20);
                    if (--waitTimer_ > 0)
                    {
                        if (level_ >= 5 && rng_.Chance(0.08f)) @in.attack = !last_.attack;  // pummel
                        last_ = @in;
                        return @in;
                    }
                    waitTimer_ = -1;
                    Rect main = m.platforms[0].rect;
                    float edgeAhead = f.facing > 0 ? main.Right() - f.pos.x : f.pos.x - main.Left();
                    float edgeBehind = f.facing > 0 ? f.pos.x - main.Left() : main.Right() - f.pos.x;
                    Fighter v = m.fighters[Math.Max(0, f.grabPartner)];
                    if (edgeAhead < 200) @in.stick.x = (float)f.facing;
                    else if (edgeBehind < 200) @in.stick.x = (float)(-f.facing);
                    else if (v.percent > 100) @in.stick.y = -1.0f;
                    else @in.stick = rng_.Chance(0.5f) ? new Vec2(0, 1) : new Vec2((float)f.facing, 0);
                    last_ = @in;
                    return @in;
                }
                case FState.LedgeHang:
                {
                    if (waitTimer_ < 0) waitTimer_ = rng_.Irange(8, 50 - level_ * 3);
                    if (--waitTimer_ <= 0)
                    {
                        waitTimer_ = -1;
                        int toStage = (f.ledge % 2 == 0) ? 1 : -1;
                        float r = rng_.Uniform();
                        if (r < 0.5f) @in.stick.x = (float)toStage;
                        else if (r < 0.7f) @in.jump = !last_.jump;
                        else if (r < 0.85f) @in.attack = !last_.attack;
                        else @in.shield = !last_.shield;
                    }
                    last_ = @in;
                    return @in;
                }
                case FState.Knockdown:
                    if (f.stateFrame > 12 && rng_.Chance(0.1f + 0.2f * skill_))
                    {
                        if (rng_.Chance(0.5f)) @in.attack = !last_.attack;
                        else @in.jump = !last_.jump;
                    }
                    last_ = @in;
                    return @in;
                default:
                    break;
            }
            waitTimer_ = -1;
            if (f.grounded) crossTimer_ = 0;
            else if (crossTimer_ > 0) --crossTimer_;

            if (!f.grounded && Recover(m, f, ref @in))
            {
                last_ = @in;
                return @in;
            }
            ManageWeapon(m, f, ref @in);
            if (m.orb.active && ChaseOrb(m, f, ref @in))
            {
                last_ = @in;
                return @in;
            }
            if (f.weapons != 3 && m.items.Count > 0 && FetchWeapon(m, f, ref @in))
            {
                last_ = @in;
                return @in;
            }

            // Pick the closest opponent.
            Fighter target = null;
            float best = 1e9f;
            foreach (var o in m.fighters)
            {
                if (!m.Foes(f, o) || !o.Active()) continue;
                float d = (o.pos - f.pos).Len() + (o.state == FState.Respawn ? 600.0f : 0.0f);
                if (d < best)
                {
                    best = d;
                    target = o;
                }
            }
            if (target != null) Fight(m, f, target, ref @in);
            last_ = @in;
            return @in;
        }

        bool Recover(Match m, Fighter f, ref InputState @in)
        {
            if (m.GroundBelow(f.pos, 1500.0f)) return false;
            int l = crossTimer_ > 0 ? LedgeToward(m, f.pos, crossDir_) : -1;  // flying over a gap: the far side
            if (l < 0) l = NearestLedge(m, f.pos);
            if (l < 0) return false;
            Vec2 L = m.LedgePoint(l);
            int toStage = (l % 2 == 0) ? 1 : -1;
            Vec2 goal = L + new Vec2(toStage * 30.0f, -40.0f);
            @in.stick.x = Signf(goal.x - f.pos.x);
            if (f.state == FState.Helpless || f.state == FState.AirDodge) return true;
            if (f.state == FState.Attack)
            {
                @in.stick = (goal - f.Center()).Norm();  // steer teleports / zips
                MoveDef mv = f.CurrentMove();
                if (mv != null && mv.teleportFrame < 0 && @in.stick.y > 0.5f)  // never fast-fall into the void mid-attack
                    @in.stick = new Vec2(@in.stick.x, 0.5f).Norm() * 0.6f;
                @in.special = last_.special;  // keep charging-type specials short
                return true;
            }
            if (f.state != FState.Air) return true;
            float below = f.pos.y - L.y;  // > 0 : feet under the ledge
            float dist = MathF.Abs(f.pos.x - L.x);
            bool falling = f.vel.y > -1.0f;
            // Under the stage (feet below its underside, body not clear of its side): get out from under it first,
            // or the jump and the special only bump into the underside.
            Rect plat = m.platforms[l / 2].rect;
            float clearance = (L.x - f.pos.x) * toStage - f.def.width * 0.5f;  // > 0: clear of the side
            if (f.pos.y > plat.Bottom() && clearance < 6.0f)
            {
                @in.stick = new Vec2((float)(-toStage), 0.0f);
                if (f.pos.y < plat.Bottom() + 220.0f) return true;  // still time to drift out
                if (falling && f.airJumps > 0)
                {
                    @in.jump = !last_.jump;
                }
                else if (falling)
                {
                    @in.stick = new Vec2((float)(-toStage), -1.0f).Norm();
                    @in.special = !last_.special;
                }
                return true;
            }
            if (falling && below > -80.0f && f.airJumps > 0)
            {
                @in.jump = !last_.jump;
            }
            else if (falling && f.airJumps == 0 && dist > 260.0f && below < 120.0f && !f.sideSpecialUsed &&
                     SideSpecialTravels(f.def))
            {
                @in.stick = new Vec2((float)toStage, 0.0f);
                @in.special = !last_.special;
            }
            else if (falling && f.airJumps == 0 && (below > -20.0f || dist > 200.0f))
            {
                Vec2 aim = (goal - f.Center()).Norm();
                if (aim.y > -0.6f) aim = new Vec2(aim.x * 0.6f, -0.8f).Norm();
                @in.stick = aim;
                @in.special = !last_.special;
            }
            return true;
        }

        // Walks to one of its own weapons it does not have when it is close and nobody is in its face, and takes it
        // (holding the weapon button).
        bool FetchWeapon(Match m, Fighter f, ref InputState @in)
        {
            WeaponItem best = null;
            float bd = 650.0f;
            foreach (var it in m.items)
            {
                if (it.character != f.charIndex || !it.grounded || ((f.weapons >> it.slot) & 1) != 0) continue;
                float d = MathF.Abs(it.pos.x - f.pos.x) + MathF.Abs(it.pos.y - f.pos.y);
                if (d < bd)
                {
                    bd = d;
                    best = it;
                }
            }
            if (best == null || !f.grounded) return false;
            float dx = best.pos.x - f.pos.x, dy = best.pos.y - f.pos.y;
            bool onIt = MathF.Abs(dx) < 24.0f && MathF.Abs(dy) < 30.0f;
            foreach (var o in m.fighters)  // once on it, it keeps holding: the take needs kPickupFrames
                if (!onIt && m.Foes(f, o) && o.Active() && (o.pos - f.pos).Len() < 110.0f) return false;  // busy fighting
            @in.stick.x = MathF.Abs(dx) > 12.0f ? Signf(dx) : 0.0f;
            if (@in.stick.x != 0.0f && dy > -20.0f)  // never into the void: across a gap it jumps, else it leaves the weapon
            {
                int e = EdgeAhead(m, f.pos, @in.stick.x);
                if (e < 0) return false;
                if (e > 0) Cross(ref @in, @in.stick.x);
            }
            if (onIt) @in.weapon = true;  // on it: holds the button to take it
            if (dy < -50.0f && MathF.Abs(dx) < 160.0f && f.state != FState.JumpSquat) @in.jump = !last_.jump;
            else if (dy > 30.0f && MathF.Abs(dx) < 30.0f) @in.stick.y = 1.0f;  // drop through a thin platform
            return true;
        }

        // With both weapons, the weapon button switches A <-> B: the distance weapon when the opponent is far, the other
        // one up close; it thinks it over every few seconds.
        void ManageWeapon(Match m, Fighter f, ref InputState @in)
        {
            if (f.weapons != 3) return;  // it goes and takes the weapons it lacks (FetchWeapon)
            if (weaponTimer_ < 0) weaponTimer_ = rng_.Irange(20, Math.Max(30, 150 - level_ * 12));
            if (weaponTimer_ > 0)
            {
                --weaponTimer_;
                return;
            }
            if (f.state == FState.Respawn || f.state == FState.Attack || f.state == FState.Hitstun) return;
            float dist = 1e9f;
            foreach (var o in m.fighters)
                if (m.Foes(f, o) && o.Active()) dist = Minf(dist, MathF.Abs(o.pos.x - f.pos.x));
            int ranged = WeaponRange(f.def.weapons[1]) > WeaponRange(f.def.weapons[0]) ? 1 : 0;
            int want = dist > 380.0f ? ranged : dist < 200.0f ? 1 - ranged : Math.Max(0, f.weapon);
            if (want != f.weapon) @in.weapon = !last_.weapon;
            weaponTimer_ = rng_.Irange(90, 240);
        }

        bool ChaseOrb(Match m, Fighter f, ref InputState @in)
        {
            Vec2 d = m.orb.pos - f.Center();
            if (d.Len() > 520.0f || f.finalSmash) return false;
            foreach (var o in m.fighters)
                if (m.Foes(f, o) && o.Active() && (o.pos - f.pos).Len() < 120.0f) return false;  // busy fighting
            if (!f.grounded && !m.GroundBelow(new Vec2(f.pos.x + f.vel.x * 12.0f, f.pos.y), 1500.0f)) return false;
            @in.stick.x = MathF.Abs(d.x) > 30.0f ? Signf(d.x) : 0.0f;
            if (f.grounded && @in.stick.x != 0.0f)  // same at an edge: jump the gap, or leave the orb (it froze there)
            {
                int e = EdgeAhead(m, f.pos, @in.stick.x);
                if (e < 0) return false;
                if (e > 0) Cross(ref @in, @in.stick.x);
            }
            if (--decisionTimer_ > 0) return true;
            decisionTimer_ = Math.Max(3, 14 - level_);
            bool canAct = f.state == FState.Idle || f.state == FState.Walk || f.state == FState.Run ||
                          f.state == FState.Crouch || f.state == FState.Air;
            if (!canAct) return true;
            if (d.Len() < 90.0f)
            {
                Vec2 dir = MathF.Abs(d.x) > MathF.Abs(d.y) ? new Vec2(Signf(d.x), 0.0f) : new Vec2(0.0f, Signf(d.y));
                TapCStick(ref @in, dir);
            }
            else if (f.grounded && d.y < -60.0f && MathF.Abs(d.x) < 140.0f)
            {
                @in.jump = !last_.jump;
            }
            else if (!f.grounded && f.airJumps > 0 && d.y < -80.0f && f.vel.y > 0.0f)
            {
                @in.jump = !last_.jump;
            }
            return true;
        }

        void Fight(Match m, Fighter f, Fighter t, ref InputState @in)
        {
            CharacterDef d = f.def;
            if (projCooldown_ > 0) --projCooldown_;
            if (f.finalSmash && t.state != FState.Respawn && t.state != FState.Captured &&
                (f.state == FState.Idle || f.state == FState.Walk || f.state == FState.Run || f.state == FState.Air))
            {
                UltReach u = UltReachOf(f);
                Vec2 rel = t.Center() - f.Center();
                bool facing = (rel.x >= 0.0f ? 1 : -1) == f.facing;
                bool go = u.anywhere || rel.Len() < u.range;
                if (u.ahead && !u.anywhere) go = go && facing && MathF.Abs(rel.y) < 140.0f && MathF.Abs(rel.x) < u.range;
                if (go)
                {
                    @in.ultimate = !last_.ultimate;
                    return;
                }
                if (u.ahead && !facing && f.grounded) @in.stick.x = Signf(rel.x);  // turn round first
            }
            float dx = t.pos.x - f.pos.x;
            float dy = t.Center().y - f.Center().y;
            float adx = MathF.Abs(dx);
            int toward = dx >= 0 ? 1 : -1;

            // Charging a smash attack: keep holding.
            if (chargeHold_ > 0)
            {
                --chargeHold_;
                @in.cstick = chargeDir_;
                return;
            }
            // Shielding: keep it up, punish when the threat is over (never in the middle of an ultimate).
            if (shieldHold_ > 0)
            {
                --shieldHold_;
                @in.shield = true;
                if (!IsThreat(t, f) && adx < 70.0f && t.state == FState.Attack && t.move != FinalSmash && rng_.Chance(skill_))
                {
                    @in.attack = !last_.attack;  // shield + attack = grab out of shield
                    shieldHold_ = 0;
                }
                return;
            }
            // An opponent unleashes an ultimate: shield it or not, decided once (the better the CPU, the more often).
            if (f.grounded && (f.state == FState.Idle || f.state == FState.Walk || f.state == FState.Run ||
                               f.state == FState.Crouch || f.state == FState.Landing))
            {
                foreach (var o in m.fighters)
                {
                    if (!m.Foes(f, o) || !o.Active() || o.state != FState.Attack || o.move != FinalSmash) continue;
                    int key = o.id * 1000003 + (m.frame - o.moveFrame);  // this ultimate (its first frame)
                    if (key != ultKey_)
                    {
                        ultKey_ = key;
                        ultBlock_ = rng_.Chance(0.2f + 0.5f * skill_);
                    }
                    if (ultBlock_ && m.freezeTimer == 0 && (IsThreat(o, f) || (o.Center() - f.Center()).Len() < 420.0f))
                    {
                        shieldHold_ = rng_.Irange(36, 64);
                        @in.shield = true;
                        return;
                    }
                }
            }
            if (f.grounded && (f.state == FState.Idle || f.state == FState.Walk || f.state == FState.Run ||
                               f.state == FState.Crouch))
            {
                bool threat = IsThreat(t, f);
                foreach (var p in m.projectiles)
                {
                    if (!m.Foes(p.owner, f)) continue;
                    Vec2 rel = f.Center() - p.pos;
                    if (rel.Len() < 160.0f && Dot(rel, p.vel) > 0.0f) threat = true;
                }
                if (threat && rng_.Chance(0.15f * skill_ + 0.03f))
                {
                    if (SpecialUseOf(f, DSpecial).counter && rng_.Chance(0.35f + 0.3f * skill_))
                    {
                        @in.stick = new Vec2(0, 1);
                        @in.special = !last_.special;
                        return;
                    }
                    if (rng_.Chance(0.75f))
                    {
                        shieldHold_ = rng_.Irange(8, 20);
                        @in.shield = true;
                    }
                    else
                    {
                        @in.stick = new Vec2(0, 1);  // will spot dodge from shield next frames
                        @in.shield = true;
                        shieldHold_ = 2;
                    }
                    return;
                }
            }

            // Movement intent (continuous)
            Rect home = m.platforms[0].rect;
            bool targetOffstage = !m.GroundBelow(t.pos, 1500.0f);
            float goalX = t.pos.x;
            if (targetOffstage)  // edge guard: wait at the edge
                goalX = Clampf(t.pos.x, home.Left() + 40.0f, home.Right() - 40.0f);
            float gdx = goalX - f.pos.x;
            if (MathF.Abs(gdx) > 40.0f) @in.stick.x = Signf(gdx);
            bool targetGrounded = t.grounded && !targetOffstage;
            if (!f.grounded)
            {
                // never drift into the void: head for the target's platform or the closest one
                if (!m.GroundBelow(new Vec2(f.pos.x + f.vel.x * 12.0f, f.pos.y), 1500.0f))
                {
                    float safeX = targetGrounded ? t.pos.x : NearestPlatformX(m, f.pos);
                    @in.stick.x = Signf(safeX - f.pos.x);
                }
                if (f.vel.y > 0 && dy > 40.0f && m.GroundBelow(f.pos, 1500.0f) &&
                    m.GroundBelow(new Vec2(f.pos.x + f.vel.x * 20.0f, f.pos.y), 1500.0f) && rng_.Chance(0.3f * skill_))
                    @in.stick.y = 1.0f;  // fast fall (never over the void, nor drifting into it)
            }
            else
            {
                // stop at the void, unless the target waits across a gap: then jump over it
                int e = @in.stick.x != 0.0f ? EdgeAhead(m, f.pos, @in.stick.x) : 0;
                if (e != 0)
                {
                    if (targetGrounded && Signf(dx) == @in.stick.x && (e > 0 || adx < 600.0f)) Cross(ref @in, @in.stick.x);
                    else @in.stick.x = 0.0f;
                }
            }

            if (--decisionTimer_ > 0) return;
            decisionTimer_ = Math.Max(2, 18 - level_ * 2) + rng_.Irange(0, 5);  // quick to strike (an aggressive game)
            if (rng_.Chance(0.15f * (1.0f - skill_))) return;  // low levels hesitate

            bool killRange = t.percent > 110.0f - level_ * 3.0f;
            // Spacing depends on the fighter's reach (swords hit from further away).
            float close = Maxf(70.0f, MoveReach(f, f.MoveDef(FTilt)) * 0.75f);
            float mid = close + 60.0f;
            bool tShield = t.state == FState.Shield || t.state == FState.ShieldStun;

            if (!f.grounded)
            {
                if (f.state != FState.Air) return;
                bool overGround =
                    m.GroundBelow(f.pos, 1500.0f) && m.GroundBelow(new Vec2(f.pos.x + f.vel.x * 20.0f, f.pos.y), 1500.0f);
                if (adx < 110.0f && MathF.Abs(dy) < 110.0f && (overGround || f.airJumps > 0))
                {
                    Vec2 dir = new Vec2(0, 0);
                    if (dy < -40.0f && adx < 60.0f) dir = new Vec2(0, -1);
                    else if (dy > 40.0f && adx < 60.0f) dir = overGround ? new Vec2(0, 1) : new Vec2((float)toward, 0);
                    else if (adx > 30.0f) dir = new Vec2((float)toward, 0);
                    if (dir.Len() < 0.1f) @in.attack = !last_.attack;
                    else TapCStick(ref @in, dir);
                }
                else if (dy < -80.0f && f.airJumps > 0 && f.vel.y > 0 && rng_.Chance(0.4f))
                {
                    @in.jump = !last_.jump;
                }
                return;
            }

            if (f.state != FState.Idle && f.state != FState.Walk && f.state != FState.Run && f.state != FState.Crouch)
                return;

            if (targetOffstage)
            {
                if (adx < 140.0f && MathF.Abs(dy) < 120.0f)
                {
                    chargeDir_ = new Vec2((float)toward, 0);
                    TapCStick(ref @in, chargeDir_);
                    chargeHold_ = rng_.Irange(0, 6);
                }
                return;
            }

            // Specials: dashes, beams, ranged grabs, area attacks and the odd projectile
            bool ownShot = false;  // one of its projectiles still flying: no new one (no spamming)
            foreach (var p in m.projectiles) ownShot = ownShot || p.owner == f.id;
            bool canShoot = projCooldown_ == 0 && !ownShot;
            if (MathF.Abs(dy) < 70.0f && rng_.Chance(0.16f + 0.1f * skill_))
            {
                SpecialUse n = SpecialUseOf(f, NSpecial), s = SpecialUseOf(f, SSpecial), dn = SpecialUseOf(f, DSpecial);
                // The C++ lambdas use and shoot; a local function cannot capture the ref parameter, so it is passed.
                void Use(ref InputState i, Vec2 stick)
                {
                    i.stick = stick;
                    if (f.facing != toward && stick.x == 0.0f) i.stick.x = 0.3f * toward;
                    i.special = !last_.special;
                }
                void Shoot(ref InputState i, Vec2 stick)
                {
                    projCooldown_ = 70 + rng_.Irange(0, 70) - level_ * 3;
                    Use(ref i, stick);
                }
                if (adx < close && n.melee) { Use(ref @in, new Vec2(0, 0)); return; }
                if (adx < close + 20.0f && dn.melee && !dn.projectile) { Use(ref @in, new Vec2(0, 1)); return; }
                if (s.dash && MathF.Abs(adx - s.landX) < 45.0f) { Use(ref @in, new Vec2((float)toward, 0)); return; }
                if (dn.grab && adx < dn.reach + 10.0f && adx > close * 0.6f) { Use(ref @in, new Vec2(0, 1)); return; }
                if (dn.beam && adx > close * 0.8f && adx < dn.reach && f.facing == toward && rng_.Chance(0.6f))
                {
                    Use(ref @in, new Vec2(0, 1));
                    return;
                }
                if (n.beam && adx > close && adx < n.reach - 20.0f && f.facing == toward && rng_.Chance(0.6f))
                {
                    Use(ref @in, new Vec2(0, 0));
                    return;
                }
                if (s.beam && adx > close && adx < s.reach - 20.0f && f.facing == toward && rng_.Chance(0.6f))
                {
                    Use(ref @in, new Vec2((float)toward, 0));
                    return;
                }
                if (adx > mid && adx < 520.0f && canShoot)
                {
                    if (s.projectile && rng_.Chance(0.5f)) { Shoot(ref @in, new Vec2((float)toward, 0)); return; }
                    if (dn.projectile && rng_.Chance(0.5f)) { Shoot(ref @in, new Vec2(0, 1)); return; }
                }
            }

            if (MathF.Abs(dy) < 60.0f)
            {
                if (adx < close)
                {
                    if (tShield && rng_.Chance(0.6f + 0.3f * skill_))
                    {
                        @in.stick.x = (float)toward;
                        @in.grab = !last_.grab;
                    }
                    else if (killRange && rng_.Chance(0.6f))
                    {
                        chargeDir_ = rng_.Chance(0.7f) ? new Vec2((float)toward, 0) : new Vec2(0, 1);
                        TapCStick(ref @in, chargeDir_);
                        chargeHold_ = rng_.Irange(0, 4 + level_ * 2);
                    }
                    else
                    {
                        float r = rng_.Uniform();
                        if (r < 0.3f)
                        {
                            @in.stick = new Vec2(0, 0);
                            @in.attack = !last_.attack;
                        }
                        else if (r < 0.55f)
                        {
                            @in.stick = new Vec2(0.6f * toward, 0);
                            @in.attack = !last_.attack;
                        }
                        else if (r < 0.7f)
                        {
                            @in.stick = new Vec2(0, 1);
                            @in.attack = !last_.attack;
                        }
                        else if (r < 0.85f)
                        {
                            @in.stick.x = (float)toward;
                            @in.grab = !last_.grab;
                        }
                        else
                        {
                            @in.jump = !last_.jump;
                        }
                    }
                }
                else if (adx < mid)
                {
                    if (killRange && rng_.Chance(0.5f))
                    {
                        chargeDir_ = new Vec2((float)toward, 0);
                        TapCStick(ref @in, chargeDir_);
                        chargeHold_ = rng_.Irange(0, 10);
                    }
                    else if (f.state == FState.Run && rng_.Chance(0.4f))
                    {
                        @in.attack = !last_.attack;  // dash attack
                    }
                    else if (rng_.Chance(0.5f))
                    {
                        @in.jump = !last_.jump;
                    }
                }
                else if (adx > 350.0f && HasProjectile(d) && canShoot && rng_.Chance(0.35f))
                {
                    @in.stick = new Vec2(0, 0);
                    if (f.facing != toward) @in.stick.x = 0.3f * toward;
                    projCooldown_ = 70 + rng_.Irange(0, 70) - level_ * 3;
                    @in.special = !last_.special;
                }
                else if (adx < 230.0f && rng_.Chance(0.25f))
                {
                    @in.jump = !last_.jump;
                }
            }
            else if (dy < -60.0f)
            {
                if (adx < 60.0f && dy > -170.0f)
                {
                    if (killRange || rng_.Chance(0.4f))
                    {
                        chargeDir_ = new Vec2(0, -1);
                        TapCStick(ref @in, chargeDir_);
                        chargeHold_ = rng_.Irange(0, 8);
                    }
                    else
                    {
                        @in.stick = new Vec2(0, -1);
                        @in.attack = !last_.attack;
                    }
                }
                else if (adx < 200.0f && rng_.Chance(0.5f))
                {
                    @in.jump = !last_.jump;
                }
            }
            else if (dy > 80.0f && f.groundPlat >= 0 && !m.platforms[f.groundPlat].solid)
            {
                @in.stick = new Vec2(0, 1);  // drop through the platform
                if (last_.stick.y > 0.5f) @in.stick = new Vec2(0, 0);
            }
        }
    }
}
