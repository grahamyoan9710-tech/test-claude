// Port of tests/sim_test.cpp (the non-AI tests; the CPU ones come with the AI in task 6).
using System;
using System.Collections.Generic;
using NUnit.Framework;
using ClashHorizon.Core;
using static ClashHorizon.Core.MoveId;
using static ClashHorizon.Core.Fighter;
using static ClashHorizon.Core.Match;
using static ClashHorizon.Tests.Sim;

namespace ClashHorizon.Tests
{
    // Shared helpers for the match tests and the parity traces.
    static class Sim
    {
        public static MatchConfig Duel(int stage, int c0, int c1, bool cpu0, bool cpu1, int stocks = 3)
        {
            var cfg = new MatchConfig { stage = stage, stocks = stocks };
            cfg.slots[0] = new MatchConfig.Slot(true, c0, cpu0, 9);
            cfg.slots[1] = new MatchConfig.Slot(true, c1, cpu1, 9);
            return cfg;
        }
        public static void SkipCountdown(Match m) { var none = new InputState[4]; while (!m.Started()) m.Step(none); }
        public static bool Sane(Match m)
        {
            foreach (var f in m.fighters)
            {
                if (!float.IsFinite(f.pos.x) || !float.IsFinite(f.pos.y) || !float.IsFinite(f.percent)) return false;
                if (MathF.Abs(f.pos.x) > 5000 || MathF.Abs(f.pos.y) > 5000) return false;
            }
            return true;
        }
        public static InputState Key(float x, float y, bool attack = false, bool jump = false, bool shield = false,
                                     bool special = false, bool grab = false, bool smash = false)
            => new InputState { stick = new Vec2(x, y), attack = attack, jump = jump, shield = shield,
                                special = special, grab = grab, smash = smash };
        public static int CharIndex(string name)
        {
            var r = Roster.All();
            for (int i = 0; i < r.Count; ++i) if (r[i].name == name) return i;
            return 0;
        }
    }

    // Drives fighter 0 like a keyboard player.
    sealed class Pad
    {
        readonly Match m;
        public readonly InputState[] @in = new InputState[4];
        public Pad(Match m) { this.m = m; }
        public void Run(int frames, InputState s) { for (int i = 0; i < frames; ++i) { @in[0] = s; m.Step(@in); } }
        public void Idle(int frames) => Run(frames, new InputState());
    }

    public class SimTests
    {
        [Test]
        public void TestKnockback()
        {
            float a = Match.ComputeKnockback(10, 10, 100, 100, 30);
            float b = Match.ComputeKnockback(100, 10, 100, 100, 30);
            float c = Match.ComputeKnockback(100, 10, 130, 100, 30);
            Assert.IsTrue(b > a, $"more percent -> more knockback ({b} vs {a})");
            Assert.IsTrue(c < b, $"heavier -> less knockback ({c} vs {b})");
        }

        [Test]
        public void TestJabHits()
        {
            var m = new Match(Duel(0, 0, 0, false, false));
            SkipCountdown(m);
            m.fighters[0].pos = new Vec2(0, 0); m.fighters[0].facing = 1; m.fighters[1].pos = new Vec2(45, 0);
            var @in = new InputState[4];
            for (int i = 0; i < 20; ++i) m.Step(@in);
            @in[0].attack = true; m.Step(@in); @in[0].attack = false;
            for (int i = 0; i < 20; ++i) m.Step(@in);
            Assert.IsTrue(m.fighters[1].percent > 0.0f, $"victim percent {m.fighters[1].percent}");
            Assert.IsTrue(m.fighters[0].percent == 0.0f, "attacker untouched");
        }

        [Test]
        public void TestCancels()
        {   // a normal that hits can be cut into a jump once its last hitbox is over; a missed one only near its end
            static (int at, int last, int duration) CutAt(float victimX)
            {
                var m = new Match(Duel(0, 0, 0, false, false));
                SkipCountdown(m);
                var f = m.fighters[0];
                f.pos = new Vec2(0, 0); f.facing = 1; m.fighters[1].pos = new Vec2(victimX, 0);
                var @in = new InputState[4];
                for (int i = 0; i < 20; ++i) m.Step(@in);
                m.StartMove(f, FTilt);
                var mv = f.MoveDef(FTilt);
                int last = 0;
                foreach (var h in mv.hitboxes) last = Math.Max(last, h.end);
                for (int frame = 0; frame < 200 && f.state == FState.Attack; ++frame)
                {
                    @in[0].jump = f.moveFrame >= last && frame % 2 == 0;
                    m.Step(@in);
                }
                return (f.state == FState.JumpSquat ? f.moveFrame : -1, last, mv.duration);
            }
            var hit = CutAt(45.0f);
            Assert.IsTrue(hit.at >= 0 && hit.at <= hit.last + 3, $"a forward tilt that hits jumps right after its last hitbox ({hit.at}, last hitbox {hit.last})");
            var miss = CutAt(600.0f);
            Assert.IsTrue(miss.at > hit.at && miss.at < miss.duration, $"a missed one jumps later, before its end ({miss.at} of {miss.duration})");
        }

        [Test]
        public void TestSmashKills()
        {
            var m = new Match(Duel(0, 2, 0, false, false));
            SkipCountdown(m);
            m.fighters[0].pos = new Vec2(200, 0); m.fighters[0].facing = 1;
            m.fighters[1].pos = new Vec2(270, 0); m.fighters[1].percent = 150;
            var @in = new InputState[4];
            for (int i = 0; i < 10; ++i) m.Step(@in);
            @in[0].cstick = new Vec2(1, 0); m.Step(@in); @in[0].cstick = new Vec2(0, 0);
            int stocks = m.fighters[1].stocks;
            for (int i = 0; i < 300; ++i) m.Step(@in);
            var v = m.fighters[1];
            Assert.IsTrue(v.stocks < stocks || v.state == FState.Dead || v.state == FState.Respawn,
                $"Titan fsmash should KO at 150% (pos {v.pos.x:F0},{v.pos.y:F0})");
        }

        static Match FreshKeys()
        {
            var m = new Match(Duel(0, 0, 2, false, false));
            SkipCountdown(m);
            m.fighters[0].pos = new Vec2(-200, 0); m.fighters[1].pos = new Vec2(300, 0);   // no facing set, as in C++
            var none = new InputState[4];
            for (int i = 0; i < 30; ++i) m.Step(none);
            return m;
        }

        static int MoveAfter(params (int n, InputState s)[] script)
        {
            var m = FreshKeys(); var p = new Pad(m);
            foreach (var (n, s) in script) p.Run(n, s);
            return m.fighters[0].state == FState.Attack ? m.fighters[0].move : -1;
        }

        [Test]
        public void TestKeyboardControls()
        {
            {
                var m = FreshKeys(); var p = new Pad(m);
                float y0 = m.fighters[0].pos.y, minShort = y0, minFull = y0;
                p.Run(2, Key(0, 0, false, true));
                for (int i = 0; i < 60; ++i) { p.Idle(1); minShort = Math.Min(minShort, m.fighters[0].pos.y); }
                for (int i = 0; i < 40; ++i) { p.Run(1, Key(0, 0, false, true)); minFull = Math.Min(minFull, m.fighters[0].pos.y); }
                for (int i = 0; i < 40; ++i) { p.Idle(1); minFull = Math.Min(minFull, m.fighters[0].pos.y); }
                for (int i = 0; i < 40; ++i) p.Run(1, Key(0, 0, false, true));
                Assert.IsTrue(minFull < minShort - 40.0f, $"full hop ({y0 - minFull:F0}) higher than short hop ({y0 - minShort:F0})");
            }
            Assert.AreEqual(FSmash, MoveAfter((1, Key(1, 0, true))), "right+attack together = forward smash");
            Assert.AreEqual(FTilt, MoveAfter((6, Key(1, 0)), (1, Key(1, 0, true))), "hold right then attack = forward tilt");
            Assert.AreEqual(DashAttack, MoveAfter((20, Key(1, 0)), (1, Key(1, 0, true))), "run then attack = dash attack");
            Assert.AreEqual(USmash, MoveAfter((1, Key(0, -1, true))), "up+attack together = up smash");
            Assert.AreEqual(UTilt, MoveAfter((6, Key(0, -1)), (1, Key(0, -1, true))), "hold up then attack = up tilt");
            Assert.AreEqual(NAir, MoveAfter((1, Key(0, 0, true, true)), (8, new InputState())),
                "jump and attack together: the attack waits for the jump and comes out in the air");
            {
                var m = FreshKeys();
                m.EquipWeapon(m.fighters[0], 0);
                var p = new Pad(m);
                InputState up = Key(0, -1), upAttack = Key(0, -1, true);
                up.tapJump = true; upAttack.tapJump = true; up.digital = true; upAttack.digital = true;
                p.Run(1, up); p.Run(1, upAttack); p.Run(8, up);
                var f = m.fighters[0];
                Assert.IsTrue(!f.grounded && f.state == FState.Attack && f.move == NAir,
                    $"with a weapon, up then attack jumps and gives the air M1 (state {(int)f.state}, move {f.move})");
            }
            Assert.AreEqual(DSmash, MoveAfter((1, Key(0, 1, true))), "down+attack together = down smash");
            Assert.AreEqual(DTilt, MoveAfter((8, Key(0, 1)), (1, Key(0, 1, true))), "crouch then attack = down tilt");
            Assert.AreEqual(Jab, MoveAfter((1, Key(0, 0, true))), "attack = jab");
            Assert.AreEqual(FSmash, MoveAfter((1, Key(0, 0, smash: true))), "smash key = forward smash");
            Assert.AreEqual(USmash, MoveAfter((1, Key(0, -1, smash: true))), "smash key + up = up smash");
            Assert.AreEqual(USpecial, MoveAfter((1, Key(0, -1, special: true))), "up+special = up special");
            Assert.AreEqual(SSpecial, MoveAfter((1, Key(1, 0, special: true))), "side+special = side special");
            int Aerial(InputState s) => MoveAfter((1, Key(0, 0, false, true)), (12, Key(0, 0, false, true)), (1, s));
            Assert.AreEqual(NAir, Aerial(Key(0, 0, true)), "neutral air");
            Assert.AreEqual(FAir, Aerial(Key(1, 0, true)), "forward air");
            Assert.AreEqual(BAir, Aerial(Key(-1, 0, true)), "back air");
            Assert.AreEqual(DAir, Aerial(Key(0, 1, true)), "down air");
            Assert.AreEqual(UAir, Aerial(Key(0, -1, true)), "up air");
            {
                var m = FreshKeys(); var p = new Pad(m);
                p.Run(5, Key(0, 0, shield: true));
                Assert.AreEqual(FState.Shield, m.fighters[0].state, "shield held");
                p.Run(1, Key(-1, 0, shield: true));
                Assert.AreEqual(FState.Roll, m.fighters[0].state, "shield + direction = roll");
                p.Idle(40);
                p.Run(5, Key(0, 0, shield: true));
                p.Run(1, Key(0, 1, shield: true));
                Assert.AreEqual(FState.SpotDodge, m.fighters[0].state, "shield + down = spot dodge");
            }
            {
                var m = FreshKeys();
                m.fighters[1].pos = new Vec2(-150, 0);
                var p = new Pad(m);
                p.Idle(5); p.Run(1, Key(0, 0, grab: true)); p.Idle(10);
                Assert.AreEqual(FState.Holding, m.fighters[0].state, "grab connects");
                p.Run(8, Key(0, -1)); p.Idle(40);
                Assert.IsTrue(m.fighters[1].percent > 0.0f, "up throw deals damage");
            }
            {
                var m = FreshKeys(); var f = m.fighters[0];
                f.pos = new Vec2(-470, -40); f.grounded = false; f.state = FState.Air;   // direct, as in C++
                var p = new Pad(m);
                for (int i = 0; i < 60 && f.state != FState.LedgeHang; ++i) p.Idle(1);
                Assert.AreEqual(FState.LedgeHang, f.state, $"ledge grabbed (pos {f.pos.x:F0},{f.pos.y:F0})");
                p.Idle(10); p.Run(1, Key(1, 0)); p.Idle(40);
                Assert.IsTrue(f.grounded && f.groundPlat == 0 && f.pos.x > -440.0f, $"climbed on stage ({f.pos.x:F0},{f.pos.y:F0})");
            }
            {
                var m = new Match(Duel(1, 0, 2, false, false));
                SkipCountdown(m);
                var f = m.fighters[0];
                f.pos = new Vec2(-210, -160); f.grounded = false; f.groundPlat = -1; f.state = FState.Air;
                var p = new Pad(m);
                p.Idle(30);
                Assert.IsTrue(f.grounded && f.groundPlat == 1, "standing on the soft platform");
                p.Run(1, Key(0, 1)); p.Idle(20);
                Assert.IsTrue(f.pos.y > -100.0f, $"dropped through (y={f.pos.y:F0})");
            }
        }

        static Match FreshCombo()
        {
            var m = new Match(Duel(0, 4, 2, false, false));
            SkipCountdown(m);
            m.fighters[0].pos = new Vec2(-200, 0); m.fighters[0].facing = 1; m.fighters[1].pos = new Vec2(300, 0);
            var none = new InputState[4];
            for (int i = 0; i < 30; ++i) m.Step(none);
            return m;
        }

        // The move fighter 0 is in (-1 if none) and the fighter itself (C++ copies it; the match is thrown away here).
        static (int mv, Fighter f) RunCombo(params (int n, InputState s)[] script)
        {
            var m = FreshCombo(); var p = new Pad(m);
            foreach (var (n, s) in script) p.Run(n, s);
            var f = m.fighters[0];
            return (f.state == FState.Attack ? f.move : -1, f);
        }

        static InputState PadY(float y, bool attack = false, bool special = false)
        { var s = Key(0, y, attack, false, false, special); s.tapJump = true; return s; }

        static InputState Keys(float y, bool attack = false, bool special = false)
        { var s = PadY(y, attack, special); s.digital = true; return s; }

        [Test]
        public void TestCombos()
        {
            Assert.AreEqual(FSmash, RunCombo((1, Key(0, 0, true, special: true))).mv, "attack + special together = forward smash");
            Assert.AreEqual(USmash, RunCombo((1, Key(0, -1, true, special: true))).mv, "up + attack + special = up smash");
            Assert.AreEqual(FSmash, RunCombo((1, Key(0, 0, true)), (1, Key(0, 0, true, special: true))).mv, "special a frame after the attack still makes the smash");
            var r = RunCombo((2, Key(0, 1)), (2, Key(1, 1)), (1, Key(1, 0)), (1, Key(1, 0, true)));
            Assert.AreEqual(FSmash, r.mv, "quarter circle forward + attack = forward smash");
            Assert.IsTrue(r.f.charge > 0 && !r.f.charging, $"... at full power without charging (charge {r.f.charge})");
            r = RunCombo((2, Key(0, 1)), (2, Key(-1, 1)), (1, Key(-1, 0)), (1, Key(-1, 0, true)));
            Assert.IsTrue(r.mv == FSmash && r.f.facing < 0, "the quarter circle the other way turns around");
            r = RunCombo((3, Key(0, 1)), (1, Key(0, -1)), (1, Key(0, -1, true)));
            Assert.IsTrue(r.mv == USmash && r.f.charge > 0, "down, up + attack = up smash at full power");
            r = RunCombo((3, Key(0, 1)), (3, Key(1, 1)), (14, Key(1, 0)), (1, Key(1, 0, true)));
            Assert.IsTrue(r.mv == FSmash && r.f.charge > 0, "a slow quarter circle (forward held 14 frames) still charges the smash");
            r = RunCombo((2, Key(0, 1)), (2, Key(1, 1)), (2, Key(1, 0)), (2, Key(0, 0)), (1, Key(0, 0, true)));
            Assert.IsTrue(r.mv == FSmash && r.f.charge > 0, "the stick let go 2 frames before the button still charges the smash");
            Assert.AreEqual(FTilt, RunCombo((8, Key(1, 0)), (1, Key(1, 0, true))).mv, "holding forward then attack stays a tilt");
            InputState up = Key(0, -1); up.tapJump = true;
            Assert.IsFalse(RunCombo((1, up), (12, up)).f.grounded, "the stick flicked up jumps (tap jump)");
            Assert.IsTrue(RunCombo((1, Key(0, -1)), (12, Key(0, -1))).f.grounded, "no tap jump without the setting (keyboards)");
            InputState upTilt = Key(0, -1, true); upTilt.tapJump = true;
            Assert.AreEqual(UTilt, RunCombo((6, Key(0, -0.4f)), (1, upTilt)).mv, "tap jump still allows the up tilt");
            Assert.AreEqual(USmash, RunCombo((1, PadY(-1)), (1, PadY(-1, true))).mv, "stick up then attack in the jump's crouch = up smash");
            Assert.AreEqual(USpecial, RunCombo((1, PadY(-1)), (1, PadY(-1, false, true))).mv, "stick up then special = up special");
            Assert.AreEqual(USmash, RunCombo((1, Keys(-1, true))).mv, "Up + attack together (keys) = up smash");
            Assert.AreEqual(UTilt, RunCombo((1, Keys(-1)), (2, Keys(-1)), (1, Keys(-1, true))).mv, "Up then attack a little after (keys) = up tilt");
            Assert.AreEqual(USpecial, RunCombo((1, Keys(-1)), (1, Keys(-1, false, true))).mv, "Up then special (keys) = up special");
            Assert.IsFalse(RunCombo((1, Keys(-1)), (14, Keys(-1))).f.grounded, "Up alone (keys) jumps");
            Assert.IsFalse(RunCombo((8, Key(0, 0, shield: true)), (1, PadY(-1)), (12, PadY(-1))).f.grounded, "stick up out of the shield jumps");
        }

        static (int move, float dx, Fighter f) JabStep(float x, float sx)
        {
            var m = new Match(Duel(0, 4, 2, false, false));
            SkipCountdown(m);
            var f = m.fighters[0];
            f.pos = new Vec2(x, 0); f.facing = 1; m.fighters[1].pos = new Vec2(x - 700.0f, 0);
            var p = new Pad(m);
            p.Idle(30);
            float x0 = f.pos.x;
            p.Run(1, Key(0, 0, true));
            int move = f.move;
            for (int i = 0; i < 40 && f.state == FState.Attack; ++i) p.Run(1, Key(sx, 0));
            return (move, f.pos.x - x0, f);
        }

        [Test]
        public void TestAttackStep()
        {
            var still = JabStep(-200.0f, 0.0f);
            Assert.IsTrue(still.move == Jab && MathF.Abs(still.dx) < 2.0f, $"a jab without the stick stays in place (moved {still.dx:F1})");
            var fwd = JabStep(-200.0f, 1.0f);
            Assert.IsTrue(fwd.move == Jab && fwd.dx > 8.0f && fwd.f.facing == 1, $"the stick forward moves the jab forward (moved {fwd.dx:F1})");
            var back = JabStep(-200.0f, -1.0f);
            Assert.IsTrue(back.dx < -8.0f && back.f.facing == 1, $"... and back, still facing forward (moved {back.dx:F1}, facing {back.f.facing})");
            var m = new Match(Duel(0, 4, 2, false, false));   // no SkipCountdown, as in C++
            float right = 0.0f;
            foreach (var pl in m.platforms)
                if (pl.solid && pl.rect.Top() == 0.0f) right = MathF.Max(right, pl.rect.Right());
            var edge = JabStep(right - 6.0f, 1.0f);
            Assert.IsTrue(edge.f.grounded && edge.f.pos.x <= right, $"never off the edge (x {edge.f.pos.x:F1}, edge {right:F1})");
        }

        static MatchConfig TeamsCfg()
        {
            var cfg = new MatchConfig { stage = 0, stocks = 2, teams = true };
            cfg.slots[0] = new MatchConfig.Slot(true, 4, false, 9, 0);
            cfg.slots[1] = new MatchConfig.Slot(true, 2, true, 9, 0);
            cfg.slots[2] = new MatchConfig.Slot(true, 3, true, 9, 1);
            cfg.slots[3] = new MatchConfig.Slot(true, 0, true, 9, 1);
            return cfg;
        }

        [Test]
        public void TestTeams()
        {   // a teammate's jab does nothing (the CPUs playing it out comes in task 6)
            var m = new Match(TeamsCfg());
            SkipCountdown(m);
            var a = m.fighters[0]; var mate = m.fighters[1];
            a.pos = new Vec2(0, 0); a.facing = 1; mate.pos = new Vec2(40, 0);
            m.fighters[2].pos = new Vec2(-400, 0); m.fighters[3].pos = new Vec2(-500, 0);
            var @in = new InputState[4];
            for (int i = 0; i < 20; ++i) m.Step(@in);
            float before = mate.percent;
            var p = new Pad(m);
            for (int i = 0; i < 6; ++i) { p.Run(1, Key(0, 0, true)); p.Idle(12); }
            Assert.IsTrue(mate.percent == before, $"no damage to a teammate ({before:F1}% -> {mate.percent:F1}%)");
            m.fighters[2].pos = new Vec2(40, 0); m.fighters[2].vel = new Vec2();
            p.Idle(10);
            float foe = m.fighters[2].percent;
            for (int i = 0; i < 6; ++i) { p.Run(1, Key(0, 0, true)); p.Idle(12); }
            Assert.IsTrue(m.fighters[2].percent > foe, $"the other team takes the hits ({m.fighters[2].percent:F1}%)");
        }

        [Test]
        public void TestFinalSmash()
        {
            var m = new Match(Duel(0, 3, 2, false, false));  // N° 07: its bare-handed ultimate is an attack
            SkipCountdown(m);
            var f = m.fighters[0]; var v = m.fighters[1];
            f.pos = new Vec2(-100, 0); f.facing = 1;
            v.pos = new Vec2(150, 0); v.percent = 60;
            m.orb = new Orb { active = true, hp = 40, life = 10000 };
            var p = new Pad(m);
            for (int i = 0; i < 400 && !f.finalSmash; ++i)
            {
                m.orb.pos = new Vec2(-55, -60); m.orb.vel = new Vec2(0, 0);
                p.Run(1, Key(0, 0, i % 4 == 0));
            }
            Assert.IsTrue(f.finalSmash && f.ult >= kUltMax, $"breaking the orb fills the ultimate meter (orb hp {m.orb.hp:F0}, meter {f.ult:F0})");
            p.Idle(30);
            p.Run(1, Key(0, -1, false, false, false, true));
            Assert.IsTrue(f.move != FinalSmash, "an up special does not waste the ultimate");
            p.Idle(80);
            p.Run(1, Key(0, 0, false, false, false, true));
            Assert.IsTrue(f.move != FinalSmash, "the neutral special with a full meter stays the special (only U unleashes the ultimate)");
            p.Idle(80);
            p.Run(1, new InputState { ultimate = true });
            Assert.IsTrue(f.state == FState.Attack && f.move == FinalSmash, "the ultimate button unleashes the final smash");
            Assert.IsTrue(f.ult == 0.0f && !f.finalSmash, $"the meter is spent ({f.ult:F0})");
            Assert.IsTrue(m.freezeTimer > 0, "cinematic freeze");
            int stocks = v.stocks;
            p.Idle(300);
            Assert.IsTrue(v.stocks < stocks, $"final smash KOs a 60% Titan (percent {v.percent:F0})");
        }

        [Test]
        public void TestUltMeter()
        {
            var m = new Match(Duel(0, 2, 0, false, false));
            m.cfg.items = false;
            SkipCountdown(m);
            var f = m.fighters[0]; var v = m.fighters[1];
            f.pos = new Vec2(-40, 0); f.facing = 1;
            v.pos = new Vec2(10, 0);
            var p = new Pad(m);
            var ult = new InputState { ultimate = true };
            p.Run(1, ult);
            Assert.IsTrue(f.state != FState.Attack, "an empty meter cannot unleash the ultimate");
            p.Idle(10);
            float before = v.percent;
            p.Run(1, Key(0, 0, true));
            p.Idle(30);
            float dealt = v.percent - before;
            Assert.IsTrue(dealt > 0.0f, $"jab connects ({dealt:F1}%)");
            Assert.IsTrue(f.ult == 0.0f && v.ult == 0.0f, $"blows do not fill the meter ({f.ult:F1}, {v.ult:F1}): the Clash Orb gives it");
            int readyEvents = 0;
            m.GainUlt(f, kUltMax);  // what breaking the orb does
            foreach (var e in m.events) readyEvents += e.type == EvType.UltReady && e.a == f.id ? 1 : 0;
            Assert.IsTrue(f.finalSmash && f.ult == kUltMax, $"the orb fills the meter ({f.ult:F0})");
            Assert.IsTrue(readyEvents == 1, $"one UltReady event ({readyEvents})");
            p.Idle(40);
            p.Run(1, ult);
            Assert.IsTrue(f.state == FState.Attack && f.move == FinalSmash, "full meter + ultimate button = final smash");
            p.Idle(200);
            Assert.IsTrue(f.ult == 0.0f && v.ult == 0.0f, $"no meter gained from the final smash ({f.ult:F1}, {v.ult:F1})");
        }

        [Test]
        public void TestUltEdgeCases()
        {
            {
                var m = new Match(Duel(0, 0, 3, false, false));
                m.cfg.items = false;
                SkipCountdown(m);
                var a = m.fighters[0]; var b = m.fighters[1];
                a.ult = kUltMax; a.finalSmash = true;
                foreach (var f in new[] { a, b })  // both fall out of the stage on the same frame
                {
                    f.pos = new Vec2(0, 3000); f.grounded = false; f.groundPlat = -1;
                }
                var p = new Pad(m);
                for (int i = 0; i < 200 && !(a.state == FState.Respawn && b.state == FState.Respawn); ++i) p.Idle(1);
                p.Idle(20);
                Assert.IsTrue(a.state == FState.Respawn && b.state == FState.Respawn, "both respawning");
                Assert.IsTrue(MathF.Abs(a.pos.x - b.pos.x) >= 100.0f, $"side-by-side respawn platforms (x {a.pos.x:F0} / {b.pos.x:F0})");
                Assert.IsTrue(a.finalSmash && a.ult == kUltMax, "a KO keeps a full ultimate meter");
            }
            {
                var m = new Match(Duel(0, 0, 2, false, false));
                m.cfg.items = false;
                SkipCountdown(m);
                var f = m.fighters[0];
                f.ult = kUltMax; f.finalSmash = true;
                var p = new Pad(m);
                p.Run(2, new InputState { ultimate = true });
                Assert.IsTrue(f.move == FinalSmash && f.intangible > 0, $"the final smash user is intangible ({f.intangible})");
            }
        }

        // The CPU block (CPUs take and switch their weapons) comes with the AI in task 6.
        [Test]
        public void TestWeapons()
        {
            int lancer = CharIndex("N° 02"), other = CharIndex("N° 07");
            var cfg = Duel(0, lancer, other, false, false);
            cfg.items = false;
            cfg.weapons = false;  // no random drops: the test drops them itself
            var m = new Match(cfg);
            SkipCountdown(m);
            var f = m.fighters[0]; var v = m.fighters[1];
            f.pos = new Vec2(-200, 0); f.facing = 1;
            v.pos = new Vec2(250, 0);
            var p = new Pad(m);
            p.Idle(20);
            Assert.IsTrue(f.weapon < 0, $"everyone starts bare-handed (weapon {f.weapon})");
            var press = new InputState { weapon = true };
            p.Run(1, press);
            p.Idle(1);
            Assert.IsTrue(f.weapon < 0, $"the button alone does nothing without a weapon (weapon {f.weapon})");
            m.DropWeaponFor(1, 0);  // the other fighter's weapon: it cannot be taken
            m.items[m.items.Count - 1].pos = new Vec2(-150, -300);
            for (int i = 0; i < 120 && m.items.Count > 0 && !m.items[m.items.Count - 1].grounded; ++i) p.Idle(1);
            Assert.IsTrue(m.items.Count > 0 && m.items[m.items.Count - 1].grounded, "a dropped weapon lands on the stage");
            p.Run(30, Key(1, 0));
            p.Run(1, press);
            p.Idle(1);
            Assert.IsTrue(f.weapon < 0 && m.items.Count == 1, $"another character's weapon cannot be taken (weapon {f.weapon})");
            m.items.Clear();
            m.DropWeaponFor(0, 0);  // the silver spear
            m.items[m.items.Count - 1].pos = new Vec2(f.pos.x + 60, -300);
            for (int i = 0; i < 120 && m.items.Count > 0 && !m.items[m.items.Count - 1].grounded; ++i) p.Idle(1);
            p.Run(20, Key(1, 0));
            p.Idle(5);
            Assert.IsTrue(f.weapon < 0 && m.items.Count == 1, $"walking over its weapon does not take it (weapon {f.weapon})");
            int Summons()
            {
                int n = 0;
                foreach (var e in m.events) n += e.type == EvType.WeaponSummon && e.a == f.id ? 1 : 0;
                return n;
            }
            if (m.items.Count > 0) { f.pos.x = m.items[m.items.Count - 1].pos.x; f.vel.x = 0.0f; }  // back on it, standing still
            p.Idle(3);
            p.Run(kPickupFrames / 2, press);
            p.Idle(1);
            Assert.IsTrue(f.weapon < 0, $"a short hold does not take it (weapon {f.weapon})");
            for (int i = 0; i < kPickupFrames && f.weapon < 0; ++i) p.Run(1, press);
            Assert.IsTrue(f.weapon == 0 && m.items.Count == 0 && Summons() == 1,
                          $"standing on it + holding the button takes weapon A (weapon {f.weapon})");
            p.Idle(1);
            p.Run(1, press);
            p.Idle(1);
            Assert.IsTrue(f.weapon == 0 && f.weapons == 1, $"with weapon A only, the button does not switch (weapon {f.weapon})");
            m.DropWeaponFor(0, 1);  // weapon B, its own item
            for (int i = 0; i < 120 && m.items.Count > 0 && !m.items[m.items.Count - 1].grounded; ++i) p.Idle(1);
            if (m.items.Count > 0) f.pos.x = m.items[m.items.Count - 1].pos.x;
            p.Idle(3);
            for (int i = 0; i < kPickupFrames + 5 && f.weapon != 1; ++i) p.Run(1, press);
            Assert.IsTrue(f.weapon == 1 && f.weapons == 3 && m.items.Count == 0, $"holding the button over weapon B takes it too (weapon {f.weapon})");
            p.Idle(1);
            p.Run(1, press);
            Assert.IsTrue(f.weapon == 0, $"with both, the next press switches to weapon A, no cooldown (weapon {f.weapon})");
            p.Idle(1);
            p.Run(1, press);
            Assert.IsTrue(f.weapon == 1, $"and back to B (weapon {f.weapon})");
            p.Idle(1);
            p.Run(30, press);  // held: one switch only
            Assert.IsTrue(f.weapon == 0, $"holding the button switches once (weapon {f.weapon})");
            p.Idle(10);
            // Not in the middle of a move: the press waits for it to end.
            p.Run(1, Key(0, 0, true));
            Assert.IsTrue(f.state == FState.Attack, "the M1 starts");
            int w0 = f.weapon;
            p.Run(1, press);
            Assert.IsTrue(f.weapon == w0 && f.state == FState.Attack, $"no switch during a move (weapon {f.weapon})");
            bool switched = false;
            for (int i = 0; i < 40 && !switched; ++i) { p.Idle(1); switched = f.weapon != w0; }
            Assert.IsTrue(switched, $"the press is kept until the move ends (weapon {f.weapon}, state {(int)f.state})");
            // The weapon's own skills: the silver spear is thrown and comes back.
            if (f.weapon != 0) { p.Idle(10); p.Run(1, press); }
            p.Idle(20);
            f.facing = 1;
            p.Run(1, Key(0, 0, false, false, false, true));
            Assert.IsTrue(f.state == FState.Attack && f.MoveDef(f.move).name == "Lance de givre", "the spear throw replaces the neutral special");
            bool thrown = false, back = false;
            for (int i = 0; i < 200; ++i)
            {
                p.Idle(1);
                if (m.projectiles.Count > 0) thrown = true;
                if (thrown && m.projectiles.Count == 0) { back = true; break; }
            }
            Assert.IsTrue(thrown && back, $"the spear flies and comes back (thrown {thrown} back {back})");
            // A KO: back to bare hands.
            int stocks = f.stocks;
            f.pos = new Vec2(0, m.Stage().blast.Bottom() + 100.0f);
            f.grounded = false; f.groundPlat = -1;
            p.Idle(5);
            Assert.IsTrue(f.stocks < stocks && f.weapon < 0 && f.weapons == 0, $"a KO loses both weapons (stocks {f.stocks}, weapon {f.weapon})");
            p.Idle(400);
            Assert.IsTrue(f.weapon < 0, $"it respawns bare-handed (weapon {f.weapon})");
            {  // both weapons of a fighter drop, one after the other (not one item giving both)
                var c3 = Duel(0, lancer, other, false, false);
                c3.items = false;
                var m3 = new Match(c3);
                bool[] dropped = { false, false };
                var idle = new InputState[4];
                for (int t = 0; t < 60 * 90 && !(dropped[0] && dropped[1]); ++t)
                {
                    m3.Step(idle);
                    foreach (var e in m3.events)
                        if (e.type == EvType.WeaponSpawn && (int)e.strength == lancer) dropped[e.b & 1] = true;
                }
                Assert.IsTrue(dropped[0] && dropped[1], $"weapons A and B both drop on the stage ({dropped[0]} {dropped[1]})");
            }
        }

        [Test]
        public void TestWeaponStrikes()
        {
            var r = Roster.All();
            for (int c = 0; c < r.Count; ++c)
                for (int slot = 0; slot < 2; ++slot)
                {
                    var cfg = Duel(0, c, c == 0 ? 1 : 0, false, false);
                    cfg.items = false;
                    var m = new Match(cfg);
                    SkipCountdown(m);
                    var f = m.fighters[0];
                    f.pos = new Vec2(-60, 0); f.facing = 1;
                    m.fighters[1].pos = new Vec2(20, 0);
                    m.EquipWeapon(f, slot);
                    var p = new Pad(m);
                    p.Run(1, Key(0, 0, true));
                    string name = f.state == FState.Attack ? f.MoveDef(f.move).name : "-";
                    bool strike = f.state == FState.Attack && f.move == Jab &&
                                  (f.MoveDef(Jab).nextMove < 0 || f.WeaponDef().kit) &&  // a kit's jab starts its combo
                                  f.MoveDef(Jab).name != r[c].moves[Jab].name;
                    bool hit = false;
                    for (int i = 0; i < 60 && !hit; ++i) { p.Idle(1); hit = m.fighters[1].percent > 0.0f; }
                    Assert.IsTrue(strike && hit, $"{r[c].name} weapon {slot}: the jab becomes the weapon's strike and hits (move \"{name}\", hit {hit})");
                }
        }

        // A weapon kit: every ground attack is its 3-hit M1 combo, a smash its finisher at once (the heavy attack),
        // every aerial its air M1, the specials its own skills; bare-handed the flick is still a smash.
        [Test]
        public void TestWeaponKits()
        {
            var r = Roster.All();
            for (int c = 0; c < r.Count; ++c)
                for (int slot = -1; slot < 2; ++slot)
                {
                    if (slot >= 0 && !r[c].weapons[slot].kit) continue;
                    if (slot < 0 && !r[c].weapons[0].kit && !r[c].weapons[1].kit) continue;
                    var cfg = Duel(0, c, c == 0 ? 1 : 0, false, false);
                    cfg.items = false;
                    var m = new Match(cfg);
                    SkipCountdown(m);
                    var f = m.fighters[0]; var v = m.fighters[1];
                    f.pos = new Vec2(-60, 0); f.facing = 1;
                    v.pos = new Vec2(10, 0);
                    if (slot >= 0) m.EquipWeapon(f, slot);
                    var p = new Pad(m);
                    p.Idle(10);
                    if (slot < 0)
                    {
                        p.Run(1, Key(1, 0, true));  // a flick forward + attack
                        Assert.IsTrue(f.move == FSmash, $"{r[c].name} bare-handed: the flick is a smash (move {f.move})");
                        continue;
                    }
                    p.Run(1, Key(0, 0, true));  // a plain attack
                    Assert.IsTrue(f.state == FState.Attack && f.move == Jab, $"{r[c].name} weapon {slot}: the attack starts the M1 (move {f.move})");
                    int seen = 1, hits = 0;
                    float pct = v.percent;
                    for (int i = 0; i < 120; ++i)
                    {
                        p.Run(1, Key(0, 0, i % 4 < 2));  // mashing the attack button
                        if (f.state == FState.Attack && f.move == Jab + seen && seen < 3) ++seen;
                        if (v.percent > pct) { ++hits; pct = v.percent; }
                    }
                    Assert.IsTrue(seen == 3 && hits >= 3, $"{r[c].name} weapon {slot}: the M1 chains its 3 hits ({seen} moves, {hits} hits)");
                    for (int i = 0; i < 120 && f.state != FState.Idle; ++i) p.Idle(1);
                    p.Run(1, Key(1, 0, true));  // a flick forward + attack
                    Assert.IsTrue(f.state == FState.Attack && f.move == Jab3, $"{r[c].name} weapon {slot}: the flick is the heavy attack, the finisher (move {f.move})");
                    f.pos = new Vec2(-60, -200);
                    f.grounded = false;
                    m.SetState(f, FState.Air);
                    p.Idle(1);  // the attack button let go
                    p.Run(1, Key(0, -1, true));  // up + attack in the air
                    Assert.IsTrue(f.state == FState.Attack && f.move == NAir, $"{r[c].name} weapon {slot}: every aerial is the air M1 (move {f.move})");
                    foreach (int id in new[] { NSpecial, SSpecial, USpecial, DSpecial })
                        Assert.IsTrue(f.MoveDef(id).Exists() && f.MoveDef(id).name != r[c].moves[id].name, $"{r[c].name} weapon {slot}: skill {id} is its own");
                }
        }

        // Every ultimate (bare-handed and with each weapon) hits, lets its victim go, and can KO at high percent.
        [Test]
        public void TestAllUltimates()
        {
            var r = Roster.All();
            for (int c = 0; c < r.Count; ++c)
                for (int slot = -1; slot <= 1; ++slot)
                    for (int pass = 0; pass < 2; ++pass)  // 0: at 30%, how much damage; 1: at 130%, does it KO
                    {
                        var cfg = Duel(0, c, c == 0 ? 1 : 0, false, false);
                        cfg.items = false;
                        var m = new Match(cfg);
                        SkipCountdown(m);
                        var f = m.fighters[0]; var v = m.fighters[1];
                        f.pos = new Vec2(-120, 0); f.facing = 1;
                        v.pos = new Vec2(40, 0);
                        v.percent = pass == 0 ? 30.0f : 130.0f;
                        if (slot >= 0) m.EquipWeapon(f, slot);
                        var p = new Pad(m);
                        p.Idle(10);
                        f.ult = kUltMax; f.finalSmash = true;
                        p.Run(1, new InputState { ultimate = true });
                        string name = f.MoveDef(FinalSmash).name;
                        Assert.IsTrue(f.move == FinalSmash, $"{r[c].name}/{slot}: ultimate starts");
                        float before = v.percent;
                        int stocks = v.stocks, captured = 0, maxCaptured = 0;
                        bool sawForm = false;
                        float peak = before;
                        for (int i = 0; i < 360; ++i)
                        {
                            p.Idle(1);
                            if (v.stocks == stocks) peak = MathUtil.Maxf(peak, v.percent);
                            captured = v.state == FState.Captured ? captured + 1 : 0;
                            maxCaptured = Math.Max(maxCaptured, captured);
                            sawForm = sawForm || f.form >= 0;
                            if (!Sane(m)) break;
                        }
                        Assert.IsTrue(Sane(m), $"{name}: sane");
                        Assert.IsTrue(v.state != FState.Captured, $"{name}: the victim is released (state {(int)v.state})");
                        Assert.IsTrue(maxCaptured < 140, $"{name}: held {maxCaptured} frames");
                        if (pass == 0)
                        {
                            float dealt = peak - before;
                            Assert.IsTrue(dealt >= 10.0f || v.stocks < stocks, $"{name}: deals damage ({dealt:F1}%)");
                            TestContext.WriteLine($"  {r[c].name,-10} {name,-32} {dealt,5:F1}%{(v.stocks < stocks ? "  KO at 30%" : "")}{(maxCaptured > 0 ? "  (capture)" : "")}{(sawForm ? "  (transformation)" : "")}");
                        }
                        else
                            Assert.IsTrue(v.stocks < stocks || sawForm, $"{name}: KOs at 130% ({v.percent:F0}%, pos {v.pos.x:F0},{v.pos.y:F0})");
                    }
        }

        static string TextFormatShield(float shield) => $"shield left {100.0f * shield / kMaxShield,2:F0}%";

        // Ultimates can be blocked: a fighter holding its shield is never caught, takes no damage while the shield
        // holds, and the shield pays for it (it is left low, or breaks under the heaviest ones).
        [Test]
        public void TestUltimatesBlocked()
        {
            var r = Roster.All();
            int broken = 0, total = 0;
            for (int c = 0; c < r.Count; ++c)
                for (int slot = -1; slot <= 1; ++slot)
                {
                    var cfg = Duel(0, c, c == 0 ? 1 : 0, false, false);
                    cfg.items = false;
                    var m = new Match(cfg);
                    SkipCountdown(m);
                    var f = m.fighters[0]; var v = m.fighters[1];
                    f.pos = new Vec2(-120, 0); f.facing = 1;
                    v.pos = new Vec2(40, 0);
                    v.percent = 30.0f;
                    if (slot >= 0) m.EquipWeapon(f, slot);
                    var @in = new InputState[4];
                    @in[1].shield = true;
                    for (int i = 0; i < 10; ++i) m.Step(@in);
                    f.ult = kUltMax; f.finalSmash = true;
                    @in[0].ultimate = true;
                    m.Step(@in);
                    @in[0].ultimate = false;
                    string name = f.MoveDef(FinalSmash).name;
                    bool caught = false, broke = false;
                    float hurtWhileShielded = 0.0f, last = v.percent;
                    for (int i = 0; i < 360 && f.state == FState.Attack && f.move == FinalSmash; ++i)
                    {
                        m.Step(@in);
                        caught = caught || (v.state == FState.Captured && !broke);
                        broke = broke || v.state == FState.ShieldBreak || v.shield <= 0.0f;  // (worn out and hit in the same frame)
                        if (!broke) hurtWhileShielded += MathUtil.Maxf(0.0f, v.percent - last);
                        last = v.percent;
                    }
                    ++total;
                    if (broke) ++broken;
                    Assert.IsTrue(!caught, $"{name}: a shield stops the catch");
                    Assert.IsTrue(hurtWhileShielded < 0.01f, $"{name}: no damage through the shield ({hurtWhileShielded:F1}%)");
                    TestContext.WriteLine($"  {r[c].name,-10} {name,-32} {(broke ? "breaks the shield" : TextFormatShield(v.shield))}");
                }
            TestContext.WriteLine($"  {total - broken}/{total} ultimates blocked without breaking the shield");
        }

        [Test]
        public void TestHakuen()
        {
            int h = CharIndex("Hakuen"), o = CharIndex("N° 07");
            Assert.IsTrue(Roster.All()[h].name == "Hakuen", "Hakuen is in the roster");
            Match Fresh(float gap)
            {
                var cfg = Duel(0, h, o, false, false);
                cfg.items = false;
                cfg.weapons = false;
                var mm = new Match(cfg);
                SkipCountdown(mm);
                mm.fighters[0].pos = new Vec2(0, 0);
                mm.fighters[0].facing = 1;
                mm.fighters[1].pos = new Vec2(gap, 0);
                mm.fighters[1].facing = -1;
                mm.fighters[0].weapon = 0;  // the nodachi in hand (its drops wait for the clips)
                mm.fighters[0].weapons = 1;
                return mm;
            }
            var @in = new InputState[4];
            void Sheathe(Match mm)  // hold the jab until it sheathes
            {
                @in[0] = Key(0, 0, true);
                for (int i = 0; i < 120 && mm.fighters[0].move != Extra1; ++i) mm.Step(@in);
                Assert.IsTrue(mm.fighters[0].move == Extra1, $"holding the jab sheathes (move {mm.fighters[0].move})");
            }
            {  // let go in the window: the Coupe-éclair, whose hit gives will
                var m = Fresh(70);
                var f = m.fighters[0];
                Sheathe(m);
                bool glint = false;
                for (int i = 0; i < 200 && f.charge < f.MoveDef(Extra1).perfectFrom + 2; ++i)
                {
                    m.Step(@in);
                    foreach (var e in m.events) glint |= e.type == EvType.Glint;
                }
                Assert.IsTrue(glint, "the Glint shows the perfect window");
                @in[0] = new InputState();
                m.Step(@in);
                Assert.IsTrue(f.move == Extra2, $"let go in the window: the Coupe-éclair (move {f.move})");
                for (int i = 0; i < 60 && f.state == FState.Attack; ++i) m.Step(@in);
                Assert.IsTrue(m.fighters[1].percent > 0.0f && f.will == 1, $"it hits and gives one will ({m.fighters[1].percent:F0}%, will {f.will})");
                f.will = kWillMax;  // full will: she awakens as soon as she can act
                for (int i = 0; i < 5 && f.form < 0; ++i) m.Step(@in);
                Assert.IsTrue(f.form == f.def.willForm && f.will == 0, $"full will awakens her (form {f.form}, will {f.will})");
                for (int i = 0; i < 120 && f.state != FState.Idle; ++i) m.Step(@in);
                m.fighters[1].pos = new Vec2(400, 0);
                int shots = m.projectiles.Count;
                float x0 = f.pos.x;
                bool hidden = false;
                @in[0] = Key(0, 0, true);
                for (int i = 0; i < 12; ++i)
                {
                    m.Step(@in);
                    @in[0] = new InputState();
                    hidden |= f.move == FTilt && f.intangible > 0 && f.moveFrame < f.MoveDef(FTilt).teleportFrame;
                }
                Assert.IsTrue(f.move == FTilt && hidden, $"awake, her jab is the blink cut and she vanishes (move {f.move})");
                Assert.IsTrue(f.pos.x - x0 > 40.0f, $"she reappears ahead ({f.pos.x - x0:F0} px)");
                Assert.IsTrue(m.projectiles.Count > shots, $"awake, her jab throws a crescent ({shots} -> {m.projectiles.Count})");
                @in[0] = Key(0, 0, true);
                for (int i = 0; i < 40 && f.move != UTilt; ++i)
                {
                    m.Step(@in);
                    @in[0] = i % 2 != 0 ? Key(0, 0, true) : new InputState();
                }
                Assert.IsTrue(f.move == UTilt, $"the second blink cut follows (move {f.move})");
            }
            {  // bare-handed: the Coupe-éclair gives no will
                var m = Fresh(70);
                var f = m.fighters[0];
                f.weapon = -1;
                f.weapons = 0;
                Sheathe(m);
                for (int i = 0; i < 200 && f.charge < f.MoveDef(Extra1).perfectFrom + 2; ++i) m.Step(@in);
                @in[0] = new InputState();
                m.Step(@in);
                for (int i = 0; i < 60 && f.state == FState.Attack; ++i) m.Step(@in);
                Assert.IsTrue(m.fighters[1].percent > 0.0f && f.will == 0, $"bare-handed, its hit gives no will ({m.fighters[1].percent:F0}%, will {f.will})");
            }
            {  // the nodachi's neutral: cuts open ahead, four in a row, then the long recharge
                var m = Fresh(200);
                var f = m.fighters[0];
                var v = m.fighters[1];
                int casts = 0;
                void Spam(int frames)
                {
                    for (int i = 0; i < frames; ++i)
                    {
                        @in[0] = Key(0, 0, false, false, false, f.state != FState.Attack && i % 2 == 0);
                        m.Step(@in);
                        if (f.state == FState.Attack && f.move == NSpecial && f.moveFrame == 1) ++casts;
                    }
                }
                Spam(240);  // four 52-frame cuts
                Assert.IsTrue(casts == 4, $"four cuts in a row ({casts})");
                Assert.IsTrue(f.moveCooldown[NSpecial] > 6 * 60, $"then a long recharge ({f.moveCooldown[NSpecial]} frames)");
                Spam(60);
                Assert.IsTrue(casts == 4, $"no fifth while it recharges ({casts})");
                Assert.IsTrue(v.percent > 0.0f, $"the cuts open on the opponent 200 px ahead ({v.percent:F0}%)");
                @in[0] = new InputState();
            }
            {  // let go too early: the plain cut
                var m = Fresh(70);
                Sheathe(m);
                @in[0] = new InputState();
                for (int i = 0; i < 40 && m.fighters[0].move == Extra1; ++i) m.Step(@in);
                Assert.IsTrue(m.fighters[0].move == Extra3, $"let go too early: the plain cut (move {m.fighters[0].move})");
            }
            {  // the Prescience held: the opponent's blows are parried, the third fills the will and he ripostes
                var m = Fresh(40);
                var f = m.fighters[0];
                var v = m.fighters[1];
                for (int i = 0; i < 260 && f.parried < 3; ++i)
                {
                    @in[0] = Key(0, 1, false, false, false, true);
                    @in[1] = Key(0, 0, i >= 12 && i % 4 < 2);  // once he is in the parry window
                    m.Step(@in);
                }
                Assert.IsTrue(f.parried == 3, $"three blows parried ({f.parried})");
                Assert.IsTrue(f.will == kWillMax, $"the third parry fills the will ({f.will})");
                @in[1] = new InputState();
                float before = v.percent;
                for (int i = 0; i < 40; ++i) m.Step(@in);
                Assert.IsTrue(v.percent > before, $"the riposte hits ({before:F0}% -> {v.percent:F0}%)");
            }
            {  // each blade has its own ultimate (the cuts); the thunder stays bare-handed
                var m = Fresh(200);
                var d = m.fighters[0].def;
                foreach (int s in new[] { 0, 1 })
                {
                    string a = "none";
                    foreach (var (id, md) in d.weapons[s].moves)
                        if (id == FinalSmash) a = md.name;
                    Assert.IsTrue(a != "none" && a != d.moves[FinalSmash].name, $"weapon {(char)('A' + s)} has its own ultimate ({a})");
                }
            }
            {  // her ultimate: each bolt strikes where the opponent stood when it appeared; stepping away dodges it
                var m = Fresh(200);
                var f = m.fighters[0];
                var v = m.fighters[1];
                f.weapon = -1;  // bare-handed: the blades cut instead
                f.weapons = 0;
                f.ult = kUltMax;
                f.finalSmash = true;
                @in[0] = new InputState { ultimate = true };
                m.Step(@in);
                @in[0] = new InputState();
                for (int i = 0; i < 120 && m.projectiles.Count == 0; ++i) m.Step(@in);
                Assert.IsTrue(m.projectiles.Count > 0, "the first bolt appears");
                float spot = m.projectiles[0].pos.x, before = v.percent;
                v.pos.x = spot - 300.0f;  // steps away before it strikes
                for (int i = 0; i < 32; ++i) m.Step(@in);  // (it strikes after 28 frames, the next one 10 frames later)
                Assert.IsTrue(m.projectiles[0].pos.x == spot && v.percent == before, $"the bolt stays on its spot and misses ({v.percent - before:F0}%)");
                for (int i = 0; i < 30; ++i) m.Step(@in);
                Assert.IsTrue(v.percent > before, $"the next bolt falls where he stands now ({v.percent - before:F0}%)");
            }
        }
    }
}
