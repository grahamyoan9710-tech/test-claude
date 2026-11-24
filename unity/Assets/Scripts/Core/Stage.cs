using System;
using System.Collections.Generic;
using static ClashHorizon.Core.MathUtil;

namespace ClashHorizon.Core
{
    // Port of src/core/stage.hpp and stage.cpp.
    public struct Platform
    {
        public Rect rect;      // current position
        public Rect @base;     // rest position
        public bool solid;     // false = pass-through (land on top only)
        public bool ledges;    // corners can be grabbed
        public Vec2 amplitude; // moving platforms: offset = amplitude * sin(phase)
        public int period;     // frames for a full cycle (0 = static)
        public Vec2 delta;     // displacement during the last frame

        public static readonly Platform Default = new Platform { solid = true };
    }

    public enum StageTheme : byte { Lunar, Frost, Crimson, Kurenai, Clouds }

    public sealed class StageDef
    {
        public string name = "";
        public string series = "";
        public string blurb = "";
        public StageTheme theme = StageTheme.Lunar;
        public List<Platform> platforms = new List<Platform>();
        public Rect blast;   // leaving this rectangle = KO
        public List<Vec2> spawns = new List<Vec2>();
        public Vec2 respawn; // respawn platform position (feet)

        public StageDef Clone()
        {
            var c = (StageDef)MemberwiseClone();
            c.platforms = new List<Platform>(platforms);
            c.spawns = new List<Vec2>(spawns);
            return c;
        }
    }

    public static class Stage
    {
        static Platform Solid(float x0, float x1, float y, float h)
        {
            Platform p = Platform.Default;
            p.rect = p.@base = new Rect(x0, y, x1 - x0, h);
            p.solid = true;
            p.ledges = true;
            return p;
        }

        static Platform Thin(float x0, float x1, float y)
        {
            Platform p = Platform.Default;
            p.rect = p.@base = new Rect(x0, y, x1 - x0, 12);
            p.solid = false;
            return p;
        }

        // Swings around its rest position: offset = amplitude * sin(2 pi frame / period).
        static Platform Moving(Platform p, Vec2 amplitude, int period)
        {
            p.amplitude = amplitude;
            p.period = period;
            return p;
        }

        static StageDef World(string name, string series, StageTheme theme) => new StageDef
        {
            name = name, series = series, theme = theme, blast = new Rect(-1150, -950, 2300, 1600), respawn = new Vec2(0, -360)
        };

        static StageDef AbimeLunaire()
        {
            var s = World("Abîme Lunaire", "Ombre", StageTheme.Lunar);
            s.blurb = "Un seul grand plateau suspendu dans un vide violet, sous une lune immense.";
            s.platforms = new List<Platform> { Solid(-440, 440, 0, 70) };
            s.spawns = new List<Vec2> { new Vec2(-300, 0), new Vec2(300, 0), new Vec2(-110, 0), new Vec2(110, 0) };
            s.respawn = new Vec2(0, -320);
            return s;
        }

        static StageDef CitadelleDeGivre()
        {
            var s = World("Citadelle de Givre", "Givre", StageTheme.Frost);
            s.blurb = "Les remparts enneigés d'une forteresse, sous l'aurore : trois corniches au-dessus du sol.";
            s.platforms = new List<Platform> { Solid(-313, 313, 0, 60), Thin(-300, -120, -150), Thin(120, 300, -150), Thin(-90, 90, -290) };
            s.blast = new Rect(-1100, -950, 2200, 1600);
            s.spawns = new List<Vec2> { new Vec2(-260, 0), new Vec2(260, 0), new Vec2(-210, -150), new Vec2(210, -150) };
            s.respawn = new Vec2(0, -420);
            return s;
        }

        static StageDef CanyonEcarlate()
        {
            var s = World("Canyon Écarlate", "Brasier", StageTheme.Crimson);
            s.blurb = "Deux corniches de grès rouge de part et d'autre d'un gouffre, sous les lanternes.";
            s.platforms = new List<Platform> { Solid(-470, -150, 0, 70), Solid(150, 470, 0, 70), Thin(-80, 80, -115) };
            s.spawns = new List<Vec2> { new Vec2(-330, 0), new Vec2(330, 0), new Vec2(-230, 0), new Vec2(230, 0) };
            s.blast = new Rect(-1200, -950, 2400, 1600);
            return s;
        }

        static StageDef ToitsKurenai()
        {
            var s = World("Toits Kurenai", "Foudre rouge", StageTheme.Kurenai);
            s.blurb = "Les toits d'une vieille ville sous la pluie : un long faîte et deux poutres.";
            s.platforms = new List<Platform> { Solid(-374, 374, 0, 60), Thin(-370, -205, -175), Thin(205, 370, -175) };
            s.spawns = new List<Vec2> { new Vec2(-300, 0), new Vec2(300, 0), new Vec2(-120, 0), new Vec2(120, 0) };
            s.blast = new Rect(-1200, -950, 2400, 1600);
            return s;
        }

        static StageDef MonastereDesNuees()
        {
            var s = World("Monastère des Nuées", "Lumière", StageTheme.Clouds);
            s.blurb = "Des îlots au-dessus d'une mer de nuages, une planche qui va et vient.";
            s.platforms = new List<Platform>
            {
                Solid(-300, 300, 0, 60), Solid(-640, -440, 70, 50), Solid(440, 640, 70, 50),
                Moving(Thin(-100, 100, -170), new Vec2(360, 0), 600)
            };
            s.blast = new Rect(-1300, -950, 2600, 1650);
            s.spawns = new List<Vec2> { new Vec2(-200, 0), new Vec2(200, 0), new Vec2(-540, 70), new Vec2(540, 70) };
            s.respawn = new Vec2(0, -340);
            return s;
        }

        // Shared and read-only by contract: a match must Clone() a StageDef before changing it.
        static readonly List<StageDef> all = Build();

        static List<StageDef> Build()
        {
            var v = new List<StageDef> { AbimeLunaire(), CitadelleDeGivre(), CanyonEcarlate(), ToitsKurenai(), MonastereDesNuees() };
            foreach (var s in v)
            {
                // The sides and the top 20 % closer; the bottom stays.
                float bottom = s.blast.Bottom();
                s.blast.x *= 0.8f;
                s.blast.w *= 0.8f;
                s.blast.y *= 0.8f;
                s.blast.h = bottom - s.blast.y;
            }
            return v;
        }

        public static IReadOnlyList<StageDef> List() => all;

        public static void UpdatePlatforms(List<Platform> plats, int frame)
        {
            for (int i = 0; i < plats.Count; i++)
            {
                Platform p = plats[i];
                if (p.period <= 0)
                {
                    p.delta = new Vec2(0, 0);
                    plats[i] = p;
                    continue;
                }
                float s = MathF.Sin(2.0f * kPi * (float)frame / (float)p.period);
                float nx = p.@base.x + p.amplitude.x * s;
                float ny = p.@base.y + p.amplitude.y * s;
                p.delta = new Vec2(nx - p.rect.x, ny - p.rect.y);
                p.rect.x = nx;
                p.rect.y = ny;
                plats[i] = p;
            }
        }
    }
}
