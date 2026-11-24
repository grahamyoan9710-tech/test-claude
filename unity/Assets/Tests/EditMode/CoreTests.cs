using System;
using NUnit.Framework;
using ClashHorizon.Core;
using static ClashHorizon.Core.MathUtil;

namespace ClashHorizon.Tests
{
    public class CoreTests
    {
        static uint Bits(float f) => (uint)BitConverter.SingleToInt32Bits(f);

        // Expected values printed by a C++ program built on src/core/math.hpp.
        [Test]
        public void Rng_MatchesCpp()
        {
            var a = new Rng(12345);
            Assert.AreEqual(3336926330u, a.Next());
            Assert.AreEqual(1697253807u, a.Next());
            Assert.AreEqual(2816511904u, a.Next());
            Assert.AreEqual(1359758873u, new Rng(0).Next());
            Assert.AreEqual(1359758873u, new Rng().Next());
            var b = new Rng(42);
            var ir = new int[6];
            for (int i = 0; i < 6; i++) ir[i] = b.Irange(0, 9);
            CollectionAssert.AreEqual(new[] { 2, 8, 9, 6, 6, 4 }, ir);
            var c = new Rng(7);
            Assert.AreEqual(0x39e70000u, Bits(c.Uniform()));
            Assert.AreEqual(0xc007ecdcu, Bits(c.Range(-3f, 5f)));
            var e = new Rng(99);
            int n = 0;
            for (int i = 0; i < 1000; i++) if (e.Chance(0.25f)) n++;
            Assert.AreEqual(244, n);
            var f = new Rng(5);
            Assert.AreEqual(-4, f.Irange(-4, 4));
            Assert.AreEqual(-3, f.Irange(-4, 4));
            Assert.AreEqual(-2, f.Irange(-4, 4));
        }

        [Test]
        public void Vec2_Ops()
        {
            var a = new Vec2(3, 4);
            var b = new Vec2(1, -2);
            Assert.AreEqual(new Vec2(4, 2), a + b);
            Assert.AreEqual(new Vec2(2, 6), a - b);
            Assert.AreEqual(new Vec2(6, 8), a * 2f);
            Assert.AreEqual(new Vec2(1.5f, 2f), a / 2f);
            Assert.AreEqual(new Vec2(-3, -4), -a);
            Assert.AreEqual(5f, a.Len());
            Assert.AreEqual(25f, a.Len2());
            Assert.AreEqual(2.23606801f, new Vec2(1, 2).Len());
            Assert.AreEqual(new Vec2(0.6f, 0.8f), a.Norm());
            Assert.AreEqual(new Vec2(0, 0), new Vec2(1e-7f, 0).Norm());
            Assert.AreEqual(-5f, Dot(a, b));
            Assert.AreEqual(new Vec2(2, 1), Lerpv(a, b, 0.5f));
            var r = Rotate(a, 0.7f);
            Assert.AreEqual(-0.282344103f, r.x, 1e-5f);
            Assert.AreEqual(4.99202156f, r.y, 1e-5f);
            var s = a;
            s += b;
            s *= 2f;
            Assert.AreEqual(new Vec2(8, 4), s);
            Assert.AreEqual(new Vec2(3, 4), a);
        }

        [Test]
        public void CircleRect()
        {
            var rc = new Rect(0, 0, 10, 10);
            Assert.IsTrue(MathUtil.CircleRect(new Vec2(5, 5), 1, rc));
            Assert.IsTrue(MathUtil.CircleRect(new Vec2(13, 5), 3, rc));
            Assert.IsFalse(MathUtil.CircleRect(new Vec2(13.01f, 5), 3, rc));
            Assert.IsTrue(MathUtil.CircleRect(new Vec2(13, 14), 5, rc));
            Assert.IsFalse(MathUtil.CircleRect(new Vec2(13, 14), 4.99f, rc));
            Assert.AreEqual(new Vec2(5, 5), rc.Center());
            Assert.IsTrue(rc.Overlaps(new Rect(9, 9, 5, 5)));
            Assert.IsFalse(rc.Overlaps(new Rect(10, 0, 5, 5)));
            Assert.IsTrue(rc.Contains(new Vec2(10, 10)));
        }

        // Unity's Mono keeps float intermediates in double precision (16777216 + 1 - 16777216 gives 1), where the
        // C++ build rounds every operation to float; the parity tolerances absorb the drift (P1 ledger, task 2).
        // Not const, so the compiler and the JIT cannot fold the sum.
        static float big = 16777216f, one = 1f;

        [Test]
        public void Float32_Probe()
        {
            float a = big, b = one;
            Assert.AreEqual(1f, a + b - a, "float maths now round like the C++ build: tighten the parity tolerances");
        }

        // C++ copies these by value; the C# classes must deep-copy in Clone() so a copy never edits the original.
        [Test]
        public void Clone_IsDeep()
        {
            var m = new MoveDef { name = "a", duration = 10 };
            m.hitboxes.Add(HitboxDef.Default); m.vel.Add(new VelEvent { frame = 1 }); m.shots.Add(ProjShot.Default);
            var c = m.Clone();
            var h = c.hitboxes[0]; h.damage = 99; c.hitboxes[0] = h;
            c.hitboxes.Add(HitboxDef.Default); c.vel.Clear(); c.shots.Clear();
            c.projectile.lifetime = 1; c.beam.start = 5;
            Assert.AreEqual(1, m.hitboxes.Count); Assert.AreEqual(5.0f, m.hitboxes[0].damage);
            Assert.AreEqual(1, m.vel.Count); Assert.AreEqual(1, m.shots.Count);
            Assert.AreEqual(60, m.projectile.lifetime); Assert.AreEqual(-1, m.beam.start);

            var w = new WeaponDef(); w.moves.Add((MoveId.Jab, m));
            var wc = w.Clone(); wc.moves[0].second.duration = 3; wc.moves.Add((MoveId.FTilt, new MoveDef()));
            Assert.AreEqual(10, m.duration); Assert.AreEqual(1, w.moves.Count);

            var f = new FormDef(); f.moves.Add((MoveId.Jab, m));
            var fc = f.Clone(); fc.moves[0].second.duration = 4; fc.m1Wave.lifetime = 2;
            Assert.AreEqual(10, m.duration); Assert.AreEqual(60, f.m1Wave.lifetime);

            var cd = new CharacterDef(); cd.forms.Add(f);
            var cc = cd.Clone();
            cc.moves[MoveId.Jab].duration = 7; cc.weapons[0].name = "x";
            cc.weapons[1].moves.Add((MoveId.Jab, new MoveDef())); cc.forms[0].duration = 9; cc.forms.Add(new FormDef());
            cc.palettes[0].hair = new Color3(1, 2, 3);
            Assert.AreEqual(0, cd.moves[MoveId.Jab].duration); Assert.AreEqual("", cd.weapons[0].name);
            Assert.AreEqual(0, cd.weapons[1].moves.Count); Assert.AreEqual(0, cd.forms[0].duration);
            Assert.AreEqual(1, cd.forms.Count); Assert.AreEqual(40, cd.palettes[0].hair.r);

            var s = new StageDef(); s.platforms.Add(Platform.Default); s.spawns.Add(new Vec2(1, 2));
            var sc = s.Clone(); var p = sc.platforms[0]; p.period = 5; sc.platforms[0] = p; sc.spawns.Clear();
            Assert.AreEqual(0, s.platforms[0].period); Assert.AreEqual(1, s.spawns.Count);
        }
    }
}
