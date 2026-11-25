using System;
using static ClashHorizon.Core.MathUtil;
using static ClashHorizon.Core.Moves;

namespace ClashHorizon.Core
{
    // Port of src/core/swing.hpp: blade swing geometry shared by hit detection and rendering.
    public struct BladeSeg
    {
        public Vec2 a, b;
    }

    public static class Swing
    {
        // Blade angle (degrees, facing right, 90 = up) at move frame t: a0 just before the first active frame,
        // a1 on the last, held outside.
        public static float SwingAngle(in HitboxDef h, float t)
        {
            float n = (float)Math.Max(1, h.end - h.start);
            float u = Clampf((t - (float)h.start + 1.0f) / n, 0.0f, 1.0f);
            return h.a0 + (h.a1 - h.a0) * u;
        }

        public static Vec2 SwingDir(float angleDeg) =>
            new Vec2(MathF.Cos(Deg2rad(angleDeg)), -MathF.Sin(Deg2rad(angleDeg)));

        // Where the hand holds the blade (from the fighter's center, facing right).
        public static Vec2 SwingGrip(float angleDeg, float height) =>
            new Vec2(kShoulderX * height, kShoulderY * height) + SwingDir(angleDeg) * (kGripDist * height);

        // The hitting part of the blade: reach0 to reach1 of its length (center-relative, facing right).
        public static BladeSeg SwingSegment(in HitboxDef h, float angleDeg, float height, float bladeLen)
        {
            Vec2 g = SwingGrip(angleDeg, height), d = SwingDir(angleDeg);
            return new BladeSeg { a = g + d * (h.reach0 * bladeLen), b = g + d * (h.reach1 * bladeLen) };
        }
    }
}
