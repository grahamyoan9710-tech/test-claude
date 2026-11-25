using System;

namespace ClashHorizon.Core
{
    // src/core/math.hpp
    public struct Vec2
    {
        public float x, y;
        public Vec2(float x_, float y_) { x = x_; y = y_; }
        public static Vec2 operator +(Vec2 a, Vec2 o) => new Vec2(a.x + o.x, a.y + o.y);
        public static Vec2 operator -(Vec2 a, Vec2 o) => new Vec2(a.x - o.x, a.y - o.y);
        public static Vec2 operator *(Vec2 a, float s) => new Vec2(a.x * s, a.y * s);
        public static Vec2 operator /(Vec2 a, float s) => new Vec2(a.x / s, a.y / s);
        public static Vec2 operator -(Vec2 a) => new Vec2(-a.x, -a.y);
        // (float) after each step: Mono keeps float intermediates in double, C++ rounds every step (free under IL2CPP).
        public float Len() => MathF.Sqrt((float)((float)(x * x) + (float)(y * y)));
        public float Len2() => (float)((float)(x * x) + (float)(y * y));
        public Vec2 Norm()
        {
            float l = Len();
            return l > 1e-6f ? new Vec2(x / l, y / l) : new Vec2(0.0f, 0.0f);
        }
    }

    public struct Rect
    {
        public float x, y, w, h;
        public Rect(float x_, float y_, float w_, float h_) { x = x_; y = y_; w = w_; h = h_; }
        public float Left() => x;
        public float Right() => x + w;
        public float Top() => y;
        public float Bottom() => y + h;
        public Vec2 Center() => new Vec2(x + w * 0.5f, y + h * 0.5f);
        public bool Overlaps(Rect o) => x < (float)(o.x + o.w) && o.x < (float)(x + w) && y < (float)(o.y + o.h) && o.y < (float)(y + h);
        public bool Contains(Vec2 p) => p.x >= x && p.x <= (float)(x + w) && p.y >= y && p.y <= (float)(y + h);
    }

    public struct Color3
    {
        public byte r, g, b;
        public Color3(byte r_, byte g_, byte b_) { r = r_; g = g_; b = b_; }
        public static readonly Color3 Default = new Color3(255, 255, 255);
    }

    // Tiny deterministic RNG (xorshift32) so matches are reproducible.
    // A class, so a field holding it is never silently copied; copy with new Rng(other.s).
    public sealed class Rng
    {
        public uint s;
        public Rng(uint seed = 0x9E3779B9u) { s = seed != 0 ? seed : 0x9E3779B9u; }
        public uint Next()
        {
            s ^= s << 13;
            s ^= s >> 17;
            s ^= s << 5;
            return s;
        }
        public float Uniform() => (Next() >> 8) * (1.0f / 16777216.0f);
        public float Range(float a, float b) => (float)(a + (float)((float)(b - a) * Uniform()));
        public int Irange(int a, int b) => a + (int)(Next() % (uint)(b - a + 1));
        public bool Chance(float p) => Uniform() < p;
    }

    public static class MathUtil
    {
        public const float kPi = 3.14159265358979f;

        // std::min / std::max on floats, same argument order and NaN behaviour.
        public static float Minf(float a, float b) => b < a ? b : a;
        public static float Maxf(float a, float b) => a < b ? b : a;

        public static float Deg2rad(float d) => (float)((float)(d * kPi) / 180.0f);
        public static float Rad2deg(float r) => (float)((float)(r * 180.0f) / kPi);
        public static float Clampf(float v, float lo, float hi) => Maxf(lo, Minf(hi, v));
        public static float Lerpf(float a, float b, float t) => (float)(a + (float)((float)(b - a) * t));
        public static float Signf(float v) => v > 0.0f ? 1.0f : (v < 0.0f ? -1.0f : 0.0f);
        public static float Approach(float v, float target, float step)
        {
            if (v < target) return Minf(v + step, target);
            return Maxf(v - step, target);
        }
        public static float Smoothstep(float t)
        {
            t = Clampf(t, 0.0f, 1.0f);
            return (float)((float)(t * t) * (float)(3.0f - (float)(2.0f * t)));
        }

        public static float Dot(Vec2 a, Vec2 b) => (float)((float)(a.x * b.x) + (float)(a.y * b.y));
        public static Vec2 Lerpv(Vec2 a, Vec2 b, float t) => a + (b - a) * t;
        public static Vec2 Rotate(Vec2 v, float rad)
        {
            float c = MathF.Cos(rad), s = MathF.Sin(rad);
            return new Vec2((float)((float)(v.x * c) - (float)(v.y * s)), (float)((float)(v.x * s) + (float)(v.y * c)));
        }

        public static bool CircleRect(Vec2 c, float r, Rect rc)
        {
            float nx = Clampf(c.x, rc.Left(), rc.Right());
            float ny = Clampf(c.y, rc.Top(), rc.Bottom());
            float dx = c.x - nx, dy = c.y - ny;
            return (float)((float)(dx * dx) + (float)(dy * dy)) <= (float)(r * r);
        }
    }
}
