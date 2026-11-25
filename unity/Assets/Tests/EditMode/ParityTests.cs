using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;
using NUnit.Framework;
using ClashHorizon.Core;

namespace ClashHorizon.Tests
{
    // Reads the C++ dumps written by scripts/unity_parity.sh and compares them line by line with tolerance.
    static class Parity
    {
        public static readonly string Dir = Path.Combine(UnityEngine.Application.dataPath, "..", "Logs", "parity");

        public static string[] Expected(string name)
        {
            var path = Path.Combine(Dir, name + ".txt");
            Assert.IsTrue(File.Exists(path), path + " missing: run scripts/unity_parity.sh");
            return File.ReadAllLines(path, Encoding.UTF8);
        }

        public static double Tolerance(string name) => name == "stages" || name == "roster" ? 0.0002 : 0.01;

        // First differing line as a readable message, or null when every line matches.
        public static string FirstDiff(string name, List<string> actual, double tol)
        {
            var exp = Expected(name);
            int n = Math.Max(exp.Length, actual.Count);
            for (int i = 0; i < n; i++)
            {
                string e = i < exp.Length ? exp[i] : "<end>", a = i < actual.Count ? actual[i] : "<end>";
                if (!LineMatches(e, a, tol))
                    return name + ".txt line " + (i + 1) + ":\n  C++: " + e + "\n  C#:  " + a;
            }
            return null;
        }

        static bool LineMatches(string e, string a, double tol)
        {
            if (e == a) return true;
            var et = e.Split(' ');
            var at = a.Split(' ');
            if (et.Length != at.Length) return false;
            for (int i = 0; i < et.Length; i++)
            {
                if (et[i] == at[i]) continue;
                if (et[i].IndexOf('.') < 0 || at[i].IndexOf('.') < 0) return false;
                if (!double.TryParse(et[i], NumberStyles.Float, CultureInfo.InvariantCulture, out var x)) return false;
                if (!double.TryParse(at[i], NumberStyles.Float, CultureInfo.InvariantCulture, out var y)) return false;
                if (Math.Abs(x - y) > tol + 1e-9) return false;
            }
            return true;
        }

        public static void Compare(string name, List<string> actual)
        {
            var diff = FirstDiff(name, actual, Tolerance(name));
            if (diff != null) Assert.Fail(diff);
        }
    }

    // Same text format as namespace parity in tests/sim_test.cpp.
    sealed class Dump
    {
        public readonly List<string> lines = new List<string>();

        public void Line(string key, params object[] v)
        {
            var sb = new StringBuilder(key);
            foreach (var x in v) Val(sb, x);
            lines.Add(sb.ToString());
        }

        static void Val(StringBuilder sb, object o)
        {
            switch (o)
            {
                case null: sb.Append(" -"); break;
                case string s:
                    if (s.Length == 0) { sb.Append(" ''"); break; }
                    sb.Append(' ').Append(s.Replace(' ', '_').Replace('\t', '_').Replace('\n', '_').Replace('\r', '_'));
                    break;
                case int i: sb.Append(' ').Append(i.ToString(CultureInfo.InvariantCulture)); break;
                case bool b: sb.Append(b ? " 1" : " 0"); break;
                case float f: sb.Append(' ').Append(((double)f).ToString("F4", CultureInfo.InvariantCulture)); break;
                case Vec2 v: Val(sb, v.x); Val(sb, v.y); break;
                case Rect r: Val(sb, r.x); Val(sb, r.y); Val(sb, r.w); Val(sb, r.h); break;
                case Color3 c: Val(sb, (int)c.r); Val(sb, (int)c.g); Val(sb, (int)c.b); break;
                case Enum e: Val(sb, Convert.ToInt32(e, CultureInfo.InvariantCulture)); break;
                default: throw new ArgumentException("Dump: no format for " + o.GetType());
            }
        }
    }

    // Pad, writing the fighters after every step: frame fighter state move moveFrame pos vel kbVel percent stocks.
    sealed class TracePad
    {
        readonly Match m;
        readonly Dump d;
        readonly InputState[] @in = new InputState[4];
        public TracePad(Match m, Dump d) { this.m = m; this.d = d; }
        public void Run(int frames, InputState s)
        {
            for (int i = 0; i < frames; ++i)
            {
                @in[0] = s;
                m.Step(@in);
                Fighters(m, d);
            }
        }
        public void Idle(int frames) => Run(frames, new InputState());
        public static void Fighters(Match m, Dump d)
        {
            for (int k = 0; k < m.fighters.Count; ++k)
            {
                var f = m.fighters[k];
                d.Line(m.frame.ToString(CultureInfo.InvariantCulture), k, f.state, f.move, f.moveFrame, f.pos, f.vel, f.kbVel,
                    f.percent, f.stocks);
            }
        }
    }

    [SetCulture("fr-FR")]
    public class ParityTests
    {
        [Test]
        public void Stages_MatchCpp()
        {
            var d = new Dump();
            var list = Stage.List();
            for (int s = 0; s < list.Count; s++)
            {
                var sd = list[s];
                string p = "s" + s;
                d.Line(p, sd.name, sd.series, sd.blurb, sd.theme, sd.blast, sd.respawn, sd.platforms.Count, sd.spawns.Count);
                for (int j = 0; j < sd.spawns.Count; j++) d.Line(p + "/sp" + j, sd.spawns[j]);
                for (int j = 0; j < sd.platforms.Count; j++)
                {
                    var q = sd.platforms[j];
                    d.Line(p + "/p" + j, q.rect, q.@base, q.solid, q.ledges, q.amplitude, q.period, q.delta);
                }
                var plats = new List<Platform>(sd.platforms);
                foreach (int f in new[] { 0, 37, 90 })
                {
                    Stage.UpdatePlatforms(plats, f);
                    for (int j = 0; j < plats.Count; j++) d.Line(p + "/f" + f + "/p" + j, plats[j].rect, plats[j].delta);
                }
            }
            Parity.Compare("stages", d.lines);
        }

        [Test]
        public void Roster_MatchesCpp() => Parity.Compare("roster", RosterDump().lines);

        static Dump RosterDump()
        {
            var d = new Dump();
            var all = Roster.All();
            for (int c = 0; c < all.Count; c++)
            {
                var r = all[c];
                string p = "c" + c;
                d.Line(p, r.id, r.name, r.title, r.series, r.element, r.hero, r.style, r.width, r.height, r.weight, r.walkSpeed,
                    r.runSpeed, r.groundAccel, r.friction, r.walkStride, r.airSpeed, r.airAccel, r.airFriction, r.gravity,
                    r.maxFall, r.fastFall, r.jumpVel, r.shortHopVel, r.airJumpVel, r.airJumps, r.jumpSquat, r.weaponLength,
                    r.armed, r.willForm, r.forms.Count);
                for (int k = 0; k < r.palettes.Length; k++)
                {
                    var pl = r.palettes[k];
                    d.Line(p + "/pal" + k, pl.primary, pl.secondary, pl.accent, pl.skin, pl.hair, pl.eyes);
                }
                for (int i = 0; i < r.moves.Length; i++) DumpMove(d, p + "/m" + i, r.moves[i]);
                for (int k = 0; k < r.weapons.Length; k++)
                {
                    var w = r.weapons[k];
                    string q = p + "/w" + k;
                    d.Line(q, w.name, w.desc, w.visual, w.fx, w.damageMul, w.kbMul, w.reachMul, w.blade, w.kit, w.moves.Count);
                    foreach (var (id, md) in w.moves) DumpMove(d, q + "/m(" + id + ")", md);
                }
                for (int k = 0; k < r.forms.Count; k++)
                {
                    var fd = r.forms[k];
                    string q = p + "/f" + k;
                    d.Line(q, fd.form, fd.duration, fd.fx, fd.damageMul, fd.kbTakenMul, fd.superArmor, fd.aura, fd.m1Waves,
                        fd.m1Blink, fd.moves.Count);
                    DumpProj(d, q + "/wave", fd.m1Wave);
                    foreach (var (id, md) in fd.moves) DumpMove(d, q + "/m(" + id + ")", md);
                }
            }
            return d;
        }

        static readonly string[] Traces =
            { "jab_hit", "smash_ko", "projectile", "weapon_pickup_switch", "ult_capture", "hakuen_rift", "moving_platform" };

        [TestCaseSource(nameof(Traces))]
        public void Trace_MatchesCpp(string name) => Parity.Compare(name, Trace(name));

        // Review Focus 5: Mono's double intermediates could make long CPU fights drift. Report only: the first differing line.
        [Test]
        public void CpuVsCpu600_Report()
        {
            var cfg = new MatchConfig { stage = 1, stocks = 3, seed = 777 };
            int[] chars = { 0, 3, 4 };
            for (int i = 0; i < 3; ++i) cfg.slots[i] = new MatchConfig.Slot(true, chars[i], true, 9);
            var m = new Match(cfg);
            var brains = new List<CpuBrain>();
            for (int i = 0; i < 3; ++i) brains.Add(new CpuBrain(9 - i, 777u + (uint)i * 77u));
            var d = new Dump();
            for (int i = 0; i < 600 && !m.finished; ++i)
            {
                var @in = new InputState[4];
                for (int k = 0; k < m.fighters.Count; ++k) @in[m.fighters[k].port] = brains[k].Think(m, k);
                m.Step(@in);
                TracePad.Fighters(m, d);
            }
            string diff = Parity.FirstDiff("cpu_vs_cpu_600", d.lines, Parity.Tolerance("cpu_vs_cpu_600"));
            TestContext.WriteLine("cpu_vs_cpu_600: " + (diff ?? "no difference in " + d.lines.Count + " lines"));
        }

        // Review Focus 2: a match must never change the shared roster. Plays every scenario, then every ultimate,
        // kit and awakening with mashed inputs, and dumps the roster again.
        [Test]
        public void Roster_UnchangedAfterMatch()
        {
            foreach (var name in Traces) Trace(name);
            var all = Roster.All();
            for (int c = 0; c < all.Count; ++c)
                for (int slot = -1; slot <= 1; ++slot)
                {
                    var cfg = Sim.Duel(0, c, c == 0 ? 1 : 0, false, false);
                    cfg.items = false;
                    var m = new Match(cfg);
                    Sim.SkipCountdown(m);
                    var f = m.fighters[0];
                    f.pos = new Vec2(-120, 0); f.facing = 1;
                    m.fighters[1].pos = new Vec2(40, 0);
                    if (slot >= 0) m.EquipWeapon(f, slot);
                    f.ult = Fighter.kUltMax; f.finalSmash = true;
                    var @in = new InputState[4];
                    for (int i = 0; i < 400; ++i)
                    {
                        if (i == 200) f.will = Fighter.kWillMax;
                        var s = Sim.Key(i / 60 % 2 == 0 ? 1 : 0, i / 30 % 3 - 1, i % 8 < 2, i % 50 == 25, false, i % 24 == 12);
                        s.ultimate = i == 0;
                        @in[0] = s;
                        m.Step(@in);
                    }
                }
            Parity.Compare("roster", RosterDump().lines);
        }

        // The C++ `sim_test trace NAME` scenarios (sim_test.cpp, namespace parity), one fighter line per step.
        static List<string> Trace(string name)
        {
            int stage = 0, c0 = 0, c1 = 0;
            bool weapons = true;
            switch (name)
            {
                case "jab_hit": break;
                case "smash_ko": c0 = 2; break;
                case "projectile": c0 = 1; break;
                case "weapon_pickup_switch": c0 = 1; c1 = 3; weapons = false; break;
                case "ult_capture": c0 = 3; break;
                case "hakuen_rift": c0 = Sim.CharIndex("Hakuen"); c1 = Sim.CharIndex("N° 07"); weapons = false; break;
                case "moving_platform": stage = 4; c1 = 1; break;
                default: throw new ArgumentException("unknown trace " + name);
            }
            var cfg = Sim.Duel(stage, c0, c1, false, false);
            cfg.items = false;
            cfg.weapons = weapons;
            var m = new Match(cfg);
            Sim.SkipCountdown(m);
            var f = m.fighters[0];
            var v = m.fighters[1];
            var d = new Dump();
            var p = new TracePad(m, d);
            switch (name)
            {
                case "jab_hit":  // testJabHits
                    f.pos = new Vec2(0, 0); f.facing = 1;
                    v.pos = new Vec2(45, 0);
                    p.Idle(20);
                    p.Run(1, new InputState { attack = true });
                    p.Idle(20);
                    break;
                case "smash_ko":  // testSmashKills
                    f.pos = new Vec2(200, 0); f.facing = 1;
                    v.pos = new Vec2(270, 0);
                    v.percent = 150.0f;
                    p.Idle(10);
                    p.Run(1, new InputState { cstick = new Vec2(1, 0) });
                    p.Idle(300);
                    break;
                case "projectile":
                    f.pos = new Vec2(0, 0); f.facing = 1;
                    v.pos = new Vec2(300, 0);
                    p.Idle(10);
                    p.Run(1, new InputState { special = true });
                    p.Idle(90);
                    break;
                case "weapon_pickup_switch":  // testWeapons, short: take A, take B, switch back to A
                {
                    f.pos = new Vec2(-200, 0); f.facing = 1;
                    v.pos = new Vec2(250, 0);
                    var press = new InputState { weapon = true };
                    void Landed()
                    {
                        for (int i = 0; i < 120 && m.items.Count > 0 && !m.items[m.items.Count - 1].grounded; ++i) p.Idle(1);
                    }
                    p.Idle(20);
                    m.DropWeaponFor(0, 0);
                    m.items[m.items.Count - 1].pos = new Vec2(f.pos.x + 60, -300);
                    Landed();
                    if (m.items.Count > 0) { f.pos.x = m.items[m.items.Count - 1].pos.x; f.vel.x = 0.0f; }
                    p.Idle(3);
                    for (int i = 0; i < Match.kPickupFrames + 5 && f.weapon != 0; ++i) p.Run(1, press);
                    p.Idle(1);
                    m.DropWeaponFor(0, 1);
                    Landed();
                    if (m.items.Count > 0) f.pos.x = m.items[m.items.Count - 1].pos.x;
                    p.Idle(3);
                    for (int i = 0; i < Match.kPickupFrames + 5 && f.weapon != 1; ++i) p.Run(1, press);
                    p.Idle(1);
                    p.Run(1, press);
                    p.Idle(30);
                    break;
                }
                case "ult_capture":  // testAllUltimates, c = 3, slot 0, first pass
                    f.pos = new Vec2(-120, 0); f.facing = 1;
                    v.pos = new Vec2(40, 0);
                    v.percent = 30.0f;
                    m.EquipWeapon(f, 0);
                    p.Idle(10);
                    f.ult = Fighter.kUltMax;
                    f.finalSmash = true;
                    p.Run(1, new InputState { ultimate = true });
                    p.Idle(360);
                    break;
                case "hakuen_rift":  // testHakuen's nodachi neutral block: fresh(200), spam(240), spam(60)
                {
                    f.pos = new Vec2(0, 0); f.facing = 1;
                    v.pos = new Vec2(200, 0); v.facing = -1;
                    f.weapon = 0;
                    f.weapons = 1;
                    void Spam(int frames)
                    {
                        for (int i = 0; i < frames; ++i) p.Run(1, Sim.Key(0, 0, false, false, false, f.state != FState.Attack && i % 2 == 0));
                    }
                    Spam(240);
                    Spam(60);
                    break;
                }
                case "moving_platform":  // f0 dropped 40 px above the moving plank
                {
                    Assert.IsTrue(m.platforms.Count >= 4 && m.platforms[3].period > 0, "stage 4 has a moving plank at platforms[3]");
                    var r = m.platforms[3].rect;
                    f.pos = new Vec2(r.x + r.w / 2, r.y - 40);
                    f.vel = new Vec2(0, 0);
                    p.Idle(300);
                    break;
                }
            }
            return d.lines;
        }

        static void DumpHit(Dump d, string p, in HitboxDef h) =>
            d.Line(p, h.start, h.end, h.offset, h.radius, h.damage, h.angle, h.baseKB, h.growth, h.group, h.limb, h.fx, h.reverse,
                h.impact, h.swing, h.a0, h.a1, h.reach0, h.reach1, h.capture, h.launch);

        static void DumpProj(Dump d, string p, in ProjectileDef x)
        {
            d.Line(p, x.spawnOffset, x.velocity, x.gravity, x.lifetime, x.radius, x.count, x.spreadDeg, x.bounces, x.piercing,
                x.visual, x.fx, x.rehit, x.pull, x.pullRadius, x.boomerang, x.grow, x.explodeRadius, x.target, x.follow,
                x.armDelay, x.ignoreGround, x.column, x.jitter);
            DumpHit(d, p + "/hit", x.hit);
            DumpHit(d, p + "/exp", x.explode);
        }

        static void DumpMove(Dump d, string p, MoveDef md)
        {
            d.Line(p, md.name, md.duration, md.landingLag, md.gravityMul, md.frictionMul, md.allowDrift, md.chargeFrame,
                md.maxCharge, md.chargeDamageMul, md.projectileFrame, md.projectileRepeat, md.projectileEvery,
                md.intangibleStart, md.intangibleEnd, md.armorStart, md.armorEnd, md.counterStart, md.counterEnd,
                md.teleportFrame, md.teleportDist, md.teleportForward, md.turnAfterTeleport, md.cooldown, md.charges,
                md.channelEnd, md.superFlash, md.castFx, md.castFxFrame, md.castFxAt, md.castFxStays, md.castFxScale,
                md.sparkFx, md.pose, md.reflects, md.helplessAfter, md.endOnLand, md.landMove, md.loopFrame, md.nextMove,
                md.comboStart, md.comboEnd, md.comboAt, md.comboOnSpecial, md.perfectMove, md.perfectFrom, md.perfectTo,
                md.releaseMove, md.holdMove, md.fillsWill, md.parries, md.ultFx, md.holdAt, md.holdInPlace,
                md.teleportToTarget, md.formIndex, md.formFrame, md.isGrab, md.throwFrame, md.pummel, md.airOnly,
                md.hitboxes.Count, md.vel.Count, md.shots.Count);
            for (int j = 0; j < md.hitboxes.Count; j++) DumpHit(d, p + "/h" + j, md.hitboxes[j]);
            for (int j = 0; j < md.vel.Count; j++)
            {
                var e = md.vel[j];
                d.Line(p + "/v" + j, e.frame, e.mode, e.v, e.stickX, e.airOnly);
            }
            DumpProj(d, p + "/proj", md.projectile);
            for (int j = 0; j < md.shots.Count; j++)
            {
                d.Line(p + "/s" + j, md.shots[j].frame);
                DumpProj(d, p + "/s" + j + "/proj", md.shots[j].def);
            }
            var b = md.beam;
            d.Line(p + "/beam", b.start, b.end, b.from, b.angle, b.length, b.width, b.fx, b.sprite);
        }
    }
}
