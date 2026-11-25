using System.Collections.Generic;
using static ClashHorizon.Core.Swing;

namespace ClashHorizon.Core
{
    // Port of src/core/hitshapes.hpp: active hitboxes in world space, a circle (a == b) or a capsule on a blade.
    public struct HitShape
    {
        public Vec2 a, b;
        public float radius;
        public int index;  // in MoveDef.hitboxes
    }

    // Named HitShapesUtil: a C# method cannot share its class's name.
    public static class HitShapesUtil
    {
        public static List<HitShape> HitShapes(MoveDef md, int frame, Vec2 center, int facing, float height, float bladeLength)
        {
            var output = new List<HitShape>();
            for (int i = 0; i < md.hitboxes.Count; ++i)
            {
                HitboxDef x = md.hitboxes[i];
                if (frame < x.start || frame >= x.end) continue;
                if (!x.swing)
                {
                    Vec2 p = center + new Vec2(x.offset.x * facing, x.offset.y);
                    output.Add(new HitShape { a = p, b = p, radius = x.radius, index = i });
                    continue;
                }
                BladeSeg s = SwingSegment(x, SwingAngle(x, (float)frame), height, bladeLength);
                output.Add(new HitShape
                {
                    a = center + new Vec2(s.a.x * facing, s.a.y), b = center + new Vec2(s.b.x * facing, s.b.y),
                    radius = x.radius, index = i
                });
            }
            return output;
        }

        public static List<HitShape> HitShapes(Fighter f)
        {
            MoveDef md = f.CurrentMove();
            if (md == null) return new List<HitShape>();
            return HitShapes(md, f.moveFrame, f.Center(), f.facing, f.Height(), f.BladeLength());
        }
    }
}
