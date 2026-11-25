using System;
using System.Collections.Generic;
using System.Globalization;
using static ClashHorizon.Core.Kit;
using static ClashHorizon.Core.RosterNormals;
using static ClashHorizon.Core.MoveId;
using static ClashHorizon.Core.Moves;

namespace ClashHorizon.Core
{
    // Port of src/core/roster.cpp. Built once, on the first All(): set rawHitboxes before it.
    public static class Roster
    {
        public static bool rawHitboxes = false;
        static List<CharacterDef> all;
        public static IReadOnlyList<CharacterDef> All() => all ??= Build();

        sealed class Builder  // `Fighter` in C++
        {
            public CharacterDef c = new CharacterDef();
            public float S = 1.0f, P = 1.0f;
            public HitFx e = HitFx.Normal;
            public string Fx(string slot, string what) => c.id + "_" + slot + "_" + what;
            public string Pfx(string what) => "pfx_" + c.id + "_" + what;  // its Blender effect
        }
        enum BuildSize { Small, Light, Medium, Heavy, Huge }
        enum KitShape { Blade, Lance, Fist, Gauntlet, Magic }
        const int kSkillCooldown = 135;
        static Color3 C(byte r, byte g, byte b) => new Color3(r, g, b);

        static Builder Start(int number, string element, BuildSize b, HitFx e, Palette main, Palette alt)
        {
            var k = new Builder(); CharacterDef c = k.c;
            string nn = number.ToString("00", CultureInfo.InvariantCulture);
            c.id = "p" + nn; c.name = "N° " + nn; c.title = element; c.series = element;
            c.hero = (Hero)(number - 1); c.element = e; c.palettes = new[] { main, alt };
            switch (b)
            {
                case BuildSize.Small: Stats(c, 40, 86, 82, 4.4f, 9.4f, 5.6f, 0.62f, 10.0f, 15.0f, 16.0f, 10.5f, 15.0f, 3); break;
                case BuildSize.Light: Stats(c, 42, 96, 88, 4.6f, 9.8f, 5.6f, 0.64f, 10.2f, 15.5f, 16.0f, 10.5f, 15.0f, 3); break;
                case BuildSize.Medium: Stats(c, 46, 100, 100, 4.3f, 9.0f, 5.2f, 0.66f, 10.5f, 15.5f, 16.0f, 10.5f, 14.5f, 3); break;
                case BuildSize.Heavy: Stats(c, 50, 104, 112, 4.0f, 8.4f, 4.9f, 0.70f, 11.0f, 16.0f, 16.0f, 10.5f, 14.0f, 4); break;
                case BuildSize.Huge: Stats(c, 54, 108, 122, 3.8f, 8.0f, 4.7f, 0.72f, 11.5f, 16.5f, 15.5f, 10.0f, 13.5f, 4); break;
            }
            c.style = b == BuildSize.Huge || b == BuildSize.Heavy ? BodyStyle.Brute
                    : b == BuildSize.Small || b == BuildSize.Light ? BodyStyle.Speedster : BodyStyle.Brawler;
            k.S = c.height / 100.0f; k.P = 1.0f; k.e = e;
            BuildBrawlerNormals(c, k.S, k.P, e);
            return k;
        }

        // The four skills of the bare hands (slot < 0) or of a weapon, scaled to the fighter.
        static void Skills(Builder k, int slot, MoveDef n, MoveDef s, MoveDef u, MoveDef d)
        {
            foreach (MoveDef m in new[] { n, s, u, d }) Sc(m, k.S, k.P);
            foreach (MoveDef m in new[] { n, s, d }) m.cooldown = Math.Max(m.cooldown, kSkillCooldown);
            if (slot < 0) { k.c.moves[NSpecial] = n; k.c.moves[SSpecial] = s; k.c.moves[USpecial] = u; k.c.moves[DSpecial] = d; return; }
            var w = k.c.weapons[slot].moves;
            w.Add((NSpecial, n)); w.Add((SSpecial, s)); w.Add((USpecial, u)); w.Add((DSpecial, d));
        }

        static void UltimateOf(Builder k, int slot, MoveDef m)
        {
            if (slot < 0) k.c.moves[FinalSmash] = m; else k.c.weapons[slot].moves.Add((FinalSmash, m));
        }

        static WeaponDef Arm(Builder k, int slot, string name, string desc, WeaponVisual vis, KitShape shape, float blade, float dmg,
                             string[] m1, float speed = 1.0f)
        {
            WeaponDef w = k.c.weapons[slot] = Weapon(name, desc, vis, k.e, dmg, 1.05f, 1.0f);
            w.blade = blade * k.S;
            switch (shape)
            {
                case KitShape.Blade: AddBladeKit(w, k.S, k.P, k.e, speed); break;
                case KitShape.Lance: AddLanceKit(w, k.S, k.P, k.e, speed); break;
                case KitShape.Fist: AddFistKit(w, k.S, k.P, k.e, speed); break;
                case KitShape.Gauntlet: AddFistKit(w, k.S, k.P, k.e, speed, 1.25f); break;
                case KitShape.Magic: AddFistKit(w, k.S, k.P, k.e, speed, 1.5f); break;
            }
            for (int i = 0; i < m1.Length; i++)
                foreach (var (id, m) in w.moves)
                    if (id == Jab + i) m.name = m1[i];
            return w;
        }

        static MoveDef Shot(string name, HitFx e, ProjVisual vis, string fx, float speed, float radius, float dmg, int life = 50)
        {
            MoveDef m = Mv(name, 40, new List<HitboxDef>());
            m.projectileFrame = 14;
            m.projectile = Proj(new Vec2(42, -10), new Vec2(speed, 0), 0, life, radius, dmg, 30, 55, e, vis);
            m.projectile.fx = fx; m.pose = PoseHint.Cast;
            return m;
        }

        static MoveDef Piercing(MoveDef m, int rehit = 0) { m.projectile.piercing = true; m.projectile.rehit = rehit; return m; }
        static MoveDef Boomerang(MoveDef m) { m.projectile.boomerang = true; m.projectile.piercing = true; m.pose = PoseHint.Throw; return m; }

        static MoveDef Lob(MoveDef m, float vy, float gravity)
        {
            m.projectile.velocity.y = vy; m.projectile.gravity = gravity; m.pose = PoseHint.Throw;
            return m;
        }

        static MoveDef Spread(MoveDef m, int count, float deg) { m.projectile.count = count; m.projectile.spreadDeg = deg; return m; }

        static MoveDef Exploding(MoveDef m, float radius, float dmg)
        {
            m.projectile.explodeRadius = radius;
            m.projectile.explode = Imp(Hb(0, 0, 0, 0, radius, dmg, 60, 50, 85, Limb.None, 0, m.projectile.hit.fx));
            return m;
        }

        // Unused by the roster: parity does not cover it.
        static MoveDef Pulling(MoveDef m, float strength, float radius)
        {
            m.projectile.pull = strength; m.projectile.pullRadius = radius; m.projectile.piercing = true; m.projectile.rehit = 10;
            return m;
        }

        static MoveDef Rush(string name, HitFx e, string fx, float speed, int hits, float dmg, Limb limb = Limb.Weapon)
        {
            var v = new List<HitboxDef>();
            Multi(v, 8, hits, 4, 40, -4, 28, 2.5f, limb, e);
            int f = 8 + hits * 4 + 2;
            v.Add(Imp(Hb(f, f + 4, 50, -6, 32, dmg, 40, 55, 80, limb, hits, e)));
            MoveDef m = Mv(name, f + 22, v);
            m.vel = new List<VelEvent> { Ve(6, VelMode.SetX, speed, 0), Ve(6, VelMode.SetY, 0, -1.0f, 0, true), Ve(f + 2, VelMode.Scale, 0.2f, 1.0f) };
            m.gravityMul = 0.3f; m.frictionMul = 0.1f; m.pose = PoseHint.Rush; m.castFx = fx; m.castFxFrame = 6; m.castFxStays = true;
            return m;
        }

        // Vanishes and strikes from behind the opponent.
        static MoveDef Through(string name, HitFx e, string fx, float dmg)
        {
            MoveDef m = Mv(name, 40, new List<HitboxDef> { Imp(Hb(14, 18, 50, -8, 30, dmg, 50, 55, 80, Limb.Weapon, 0, e)) });
            m.teleportFrame = 10; m.teleportDist = 150; m.teleportForward = true; m.turnAfterTeleport = true;
            m.intangibleStart = 6; m.intangibleEnd = 14; m.vel = new List<VelEvent> { Ve(2, VelMode.Set, 0, 0) };
            m.gravityMul = 0.2f; m.pose = PoseHint.Thrust; m.castFx = fx; m.castFxFrame = 6; m.castFxStays = true;
            return m;
        }

        // The recovery: rises with `hits` hits on the way up, helpless after.
        static MoveDef Rise(string name, HitFx e, string fx, float vy, int hits, Limb limb = Limb.Weapon)
        {
            var v = new List<HitboxDef>();
            Multi(v, 4, hits, 4, 0, -30, 34, 2.0f, limb, e, 0, 85);
            int f = 4 + hits * 4 + 2;
            v.Add(Hb(f, f + 4, 0, -50, 38, 5.5f, 80, 60, 105, limb, hits, e));
            MoveDef m = Mv(name, f + 22, v);
            m.vel = new List<VelEvent> { Ve(4, VelMode.Set, 1.5f, -vy, 3.5f) };
            m.gravityMul = 0.5f; m.helplessAfter = true; m.pose = PoseHint.Spin; m.castFx = fx; m.castFxFrame = 4;
            return m;
        }

        static MoveDef Staying(MoveDef m) { m.castFxStays = true; return m; }

        // A teleport recovery towards the stick.
        static MoveDef Blink(string name, HitFx e, string fx, float dist = 300)
        {
            MoveDef m = Mv(name, 40, new List<HitboxDef> { Hb(22, 26, 0, -6, 40, 7, 80, 55, 75, Limb.Body, 0, e) });
            m.teleportFrame = 20; m.teleportDist = dist; m.intangibleStart = 6; m.intangibleEnd = 22;
            m.vel = new List<VelEvent> { Ve(2, VelMode.Set, 0, 0) };
            m.gravityMul = 0; m.allowDrift = false; m.helplessAfter = true; m.castFx = fx; m.castFxFrame = 6; m.castFxStays = true;
            return m;
        }

        static MoveDef Counter(string name, HitFx e, string fx, bool reflect, Limb limb = Limb.Weapon)
        {
            MoveDef m = Mv(name, 52, new List<HitboxDef> { Imp(Hb(26, 30, 56, -8, 46, 12, 361, 60, 95, limb, 0, e)) });
            m.counterStart = 5; m.counterEnd = 26; m.reflects = reflect; m.gravityMul = 0.3f; m.pose = PoseHint.HandSign;
            m.castFx = fx; m.castFxFrame = 5;
            return m;
        }

        static MoveDef Burst(string name, HitFx e, string fx, float radius, float dmg, int hits = 1)
        {
            var v = new List<HitboxDef>();
            if (hits > 1) Multi(v, 10, hits - 1, 4, 0, -10, radius * 0.8f, 2.0f, Limb.Body, e);
            int f = 10 + (hits - 1) * 4 + 2;
            v.Add(Imp(Hb(f, f + 5, 0, -10, radius, dmg, 70, 50, 85, Limb.Body, hits, e)));
            MoveDef m = Mv(name, f + 26, v);
            m.gravityMul = 0.4f; m.pose = hits > 1 ? PoseHint.Spin : PoseHint.Stomp; m.castFx = fx; m.castFxFrame = 8;
            return m;
        }

        // Unused by the roster: parity does not cover it.
        static MoveDef Trap(string name, HitFx e, string fx, float dmg)
        {
            MoveDef m = Mv(name, 36, new List<HitboxDef>());
            m.projectileFrame = 14;
            m.projectile = Proj(new Vec2(70, 40), new Vec2(0, 0), 0, 300, 22, dmg, 50, 70, e, ProjVisual.Star);
            m.projectile.fx = fx; m.projectile.armDelay = 20; m.projectile.ignoreGround = true; m.projectile.explodeRadius = 70;
            m.projectile.explode = Imp(Hb(0, 0, 0, 0, 70, dmg, 75, 55, 80, Limb.None, 0, e));
            m.pose = PoseHint.Stomp;
            return m;
        }

        static MoveDef Pillar(string name, HitFx e, string fx, float dist, float dmg)
        {
            MoveDef m = Mv(name, 46, new List<HitboxDef>());
            m.projectileFrame = 16;
            m.projectile = Proj(new Vec2(dist, 10), new Vec2(0, 0), 0, 26, 36, dmg, 55, 80, e, ProjVisual.Pillar);
            m.projectile.fx = fx; m.projectile.armDelay = 8; m.projectile.piercing = true; m.projectile.ignoreGround = true;
            m.projectile.hit.angle = 80; m.pose = PoseHint.Stomp;
            return m;
        }

        // Unused by the roster: parity does not cover it.
        static MoveDef Rain(string name, HitFx e, ProjVisual vis, string fx, int n, float dmg)
        {
            MoveDef m = Mv(name, 50, new List<HitboxDef>());
            m.pose = PoseHint.Raise;
            for (int i = 0; i < n; ++i)
            {
                ProjectileDef p = Proj(new Vec2((float)(70.0f + (float)(i * 45.0f)), -220), new Vec2(0, 10), 0, 40, 18, dmg, 30, 60, e, vis);
                p.fx = fx; p.hit.angle = 280;
                m.shots.Add(new ProjShot { frame = 14 + i * 5, def = p });
            }
            return m;
        }

        static Palette Colours(Color3 coat, Color3 trim, Color3 accent, Color3 hair, Color3 eyes) =>
            Pal(coat, trim, accent, C(238, 204, 178), hair, eyes);

        static CharacterDef Make02()
        {
            Builder k = Start(2, "Givre", BuildSize.Light, HitFx.Ice,
                              Colours(C(34, 32, 42), C(230, 232, 240), C(110, 170, 240), C(214, 218, 232), C(150, 130, 220)),
                              Colours(C(200, 220, 245), C(60, 80, 130), C(110, 170, 240), C(214, 218, 232), C(150, 130, 220)));
            HitFx e = k.e;
            k.P = 1.16f;      // balance (sim_test balance)
            k.c.weight += 4;  // balance
            Skills(k, -1, Shot("Sphère de givre", e, ProjVisual.Orb, k.Pfx("orbe"), 9, 16, 7),
                   Rush("Glissade gelée", e, k.Pfx("x_s"), 13, 0, 9, Limb.LegFront),
                   Staying(Rise("Pilier de glace", e, k.Pfx("x_u"), 15.5f, 3, Limb.Body)),
                   Counter("Miroir de glace", e, k.Pfx("x_d"), true, Limb.ArmFront));
            UltimateOf(k, -1, Ult.Awaken("GIVRE CÉLESTE", Ult.Form(k.c, k.Pfx("aura"), e, 15, 1.15f, 0.9f), e, k.Pfx("eveil")));
            Arm(k, 0, "Lance argentée", "Lance à rubans blancs", WeaponVisual.Held, KitShape.Lance, 110, 1.0f,
                new[] { "Estoc", "Double estoc", "Balayage circulaire" });
            Skills(k, 0, Boomerang(Shot("Lance de givre", e, ProjVisual.Spear, k.Pfx("lance"), 11, 18, 8, 50)),
                   Rush("Charge polaire", e, k.Pfx("a_s"), 15, 2, 9), Rise("Spirale céleste", e, k.Pfx("a_u"), 15.5f, 4),
                   Pillar("Pieux de glace", e, k.Pfx("pilier"), 80, 11));
            UltimateOf(k, 0, Ult.Storm("ZÉRO ABSOLU", e, k.Fx("a", "ult"), 120, 320));
            Arm(k, 1, "Sceptre-étoile", "Bâton à l'étoile de cristal", WeaponVisual.Held, KitShape.Blade, 90, 0.95f,
                new[] { "Coup de sceptre", "Revers", "Éclat d'étoile" });
            Skills(k, 1, Lob(Shot("Étoile filante", e, ProjVisual.Star, k.Pfx("etoile"), 8, 18, 9, 60), -6, 0.3f),
                   Spread(Shot("Constellation", e, ProjVisual.Star, k.Pfx("etoile"), 8, 14, 5, 50), 3, 20),
                   Blink("Saut stellaire", e, k.Pfx("b_u")), Counter("Dôme astral", e, k.Pfx("b_d"), true, Limb.Body));
            UltimateOf(k, 1, Ult.Rain("PLUIE D'ÉTOILES", e, k.Fx("b", "ult"), ProjVisual.Star, 8));
            return k.c;
        }

        static CharacterDef Make03()
        {
            Builder k = Start(3, "Brasier", BuildSize.Heavy, HitFx.Fire,
                              Colours(C(30, 26, 28), C(200, 40, 40), C(240, 110, 40), C(30, 24, 26), C(220, 50, 50)),
                              Colours(C(240, 236, 230), C(200, 40, 40), C(240, 110, 40), C(30, 24, 26), C(220, 50, 50)));
            HitFx e = k.e;
            k.P = 1.08f;
            k.c.weight += 8;
            Skills(k, -1, Shot("Poing ardent", e, ProjVisual.Orb, k.Pfx("orbe"), 8, 20, 9, 30),
                   Rush("Charge brûlante", e, k.Pfx("x_s"), 14, 0, 11, Limb.Body),
                   Staying(Rise("Uppercut volcanique", e, k.Pfx("x_u"), 15, 4, Limb.ArmFront)),
                   Burst("Éruption", e, k.Pfx("x_d"), 70, 12));
            UltimateOf(k, -1, Ult.Awaken("BRASIER DÉMONIAQUE", Ult.Form(k.c, k.Pfx("aura"), e, 15, 1.15f, 0.9f), e, k.Pfx("eveil")));
            Arm(k, 0, "Grande épée noire", "Lame lourde aux veines rouges", WeaponVisual.Held, KitShape.Blade, 100, 1.05f,
                new[] { "Taille lourde", "Revers", "Frappe au sol" }, 1.15f);
            Skills(k, 0, Piercing(Shot("Onde tranchante", e, ProjVisual.Wave, k.Pfx("lame"), 8, 26, 10)),
                   Rush("Brise-garde", e, k.Pfx("a_s"), 13, 0, 13), Rise("Lame ascendante", e, k.Pfx("a_u"), 15, 3),
                   Burst("Séisme", e, k.Pfx("a_d"), 80, 13));
            UltimateOf(k, 0, Ult.Rain("EXÉCUTION ÉCARLATE", e, k.Pfx("pilier"), ProjVisual.Pillar, 2));
            Arm(k, 1, "Grande épée éveillée", "La même lame, embrasée", WeaponVisual.Held, KitShape.Blade, 100, 1.05f,
                new[] { "Coupe enflammée", "Coupe en croix", "Frappe explosive" }, 1.15f);
            Skills(k, 1, Exploding(Shot("Météore", e, ProjVisual.Bomb, k.Pfx("meteore"), 9, 22, 7, 45), 60, 8),
                   Rush("Traînée de feu", e, k.Pfx("b_s"), 15, 3, 9), Rise("Phénix", e, k.Pfx("b_u"), 16, 4),
                   Burst("Cercle de brasier", e, k.Pfx("b_d"), 80, 6, 4));
            UltimateOf(k, 1, Ult.Blast("APOCALYPSE", e, "pfx_giant_sphere", ProjVisual.Orb, 8, 70, 300));
            return k.c;
        }

        static CharacterDef Make07()
        {
            Builder k = Start(7, "Foudre rouge", BuildSize.Light, HitFx.Lightning,
                              Colours(C(28, 24, 28), C(200, 30, 40), C(255, 80, 80), C(26, 22, 26), C(220, 50, 50)),
                              Colours(C(236, 232, 230), C(200, 30, 40), C(255, 80, 80), C(26, 22, 26), C(220, 50, 50)));
            HitFx e = k.e;
            k.P = 0.71f;
            k.c.weight -= 6;
            Skills(k, -1, Piercing(Shot("Éclair rouge", e, ProjVisual.Bolt, k.Pfx("eclair"), 15, 12, 6, 30)),
                   Through("Pas-éclair", e, k.Pfx("x_s"), 8),
                   Rise("Coup de pied foudre", e, k.Pfx("x_u"), 16.5f, 3, Limb.LegFront),
                   Burst("Décharge", e, k.Pfx("x_d"), 70, 4, 4));
            UltimateOf(k, -1, Ult.Rain("TEMPÊTE ÉCARLATE", e, k.Pfx("pilier"), ProjVisual.Pillar, 7));
            Arm(k, 0, "Épée large", "Lame à rubans rouges", WeaponVisual.Held, KitShape.Blade, 85, 1.0f,
                new[] { "Coupe", "Seconde coupe", "Estoc" }, 0.9f);
            Skills(k, 0, Piercing(Shot("Lame-éclair", e, ProjVisual.Crescent, k.Pfx("lame"), 12, 18, 8)),
                   Rush("Iaï foudroyant", e, k.Pfx("a_s"), 19, 0, 11), Rise("Saut-foudre", e, k.Pfx("a_u"), 16, 3),
                   Counter("Riposte", e, k.Pfx("a_d"), false));
            UltimateOf(k, 0, Ult.Ambush("CENT ÉCLAIRS", e, k.Fx("a", "ult"), Limb.Weapon, 9));
            Arm(k, 1, "Épée éveillée", "La même épée, éclairs rouges", WeaponVisual.Held, KitShape.Blade, 85, 1.0f,
                new[] { "Coupe électrique", "Coupe", "Estoc foudroyant" }, 0.9f);
            Skills(k, 1, Piercing(Shot("Lance de foudre", e, ProjVisual.Spear, k.Pfx("lance"), 14, 16, 9, 35)),
                   Rush("Tranchant écarlate", e, k.Pfx("b_s"), 16, 2, 9), Rise("Spirale rouge", e, k.Pfx("b_u"), 16, 5),
                   Pillar("Foudre au sol", e, k.Pfx("pilier"), 90, 11));
            UltimateOf(k, 1, Ult.Rain("JUGEMENT ROUGE", e, k.Fx("b", "ult"), ProjVisual.Bolt, 4));
            return k.c;
        }

        static CharacterDef Make08()
        {
            Builder k = Start(8, "Lumière", BuildSize.Medium, HitFx.Light,
                              Colours(C(236, 236, 240), C(40, 40, 46), C(230, 196, 90), C(232, 232, 238), C(110, 170, 220)),
                              Colours(C(40, 40, 46), C(236, 236, 240), C(230, 196, 90), C(232, 232, 238), C(110, 170, 220)));
            HitFx e = k.e;
            k.P = 1.31f;
            k.c.weight += 18;
            Skills(k, -1, Shot("Plume de lumière", e, ProjVisual.Feather, k.Pfx("plume"), 11, 14, 7),
                   Rush("Rafale", e, k.Pfx("x_s"), 15, 0, 10, Limb.Body),
                   Rise("Ailes de vent", e, k.Pfx("x_u"), 16.5f, 2, Limb.Body),
                   Counter("Barrière de lumière", e, k.Pfx("x_d"), true, Limb.Body));
            UltimateOf(k, -1, Ult.Awaken("ANGE BLANC", Ult.Form(k.c, k.Pfx("aura"), e, 15, 1.15f, 0.9f), e, k.Pfx("eveil")));
            Arm(k, 0, "Épée blanche et or", "Lame blanche à rubans", WeaponVisual.Held, KitShape.Blade, 85, 1.0f,
                new[] { "Coupe", "Seconde coupe", "Coupe en croix" });
            Skills(k, 0, Piercing(Shot("Lame de lumière", e, ProjVisual.Crescent, k.Pfx("lame"), 11, 20, 9)),
                   Rush("Éclair blanc", e, k.Pfx("a_s"), 18, 0, 11), Rise("Ascension sacrée", e, k.Pfx("a_u"), 16, 4),
                   Counter("Contre sacré", e, k.Pfx("a_d"), false));
            UltimateOf(k, 0, Ult.Downpour("JUGEMENT CÉLESTE", e, "pfx_sword_rain", 80, 5));
            Arm(k, 1, "Lance dorée", "Hallebarde d'or", WeaponVisual.Held, KitShape.Lance, 115, 1.0f,
                new[] { "Estoc", "Balayage", "Estoc tournant" });
            Skills(k, 1, Piercing(Shot("Lance de lumière", e, ProjVisual.Spear, k.Pfx("lance"), 13, 16, 9, 40)),
                   Rush("Charge du paladin", e, k.Pfx("b_s"), 15, 2, 10), Staying(Rise("Rayon montant", e, k.Pfx("b_u"), 16, 3)),
                   Pillar("Pilier sacré", e, k.Pfx("pilier"), 85, 12));
            UltimateOf(k, 1, Ult.Rain("LANCES DIVINES", e, k.Fx("b", "ult"), ProjVisual.Spear, 9));
            return k.c;
        }

        // Her draw: the attack held after an M1 sheathes (Rengaine); released on the glint, the Coupe-éclair, else a plain cut.
        static void DrawCut(Builder k, int slot, Limb limb, float dmg)
        {
            HitFx e = k.e;
            MoveDef sheathe = Mv("Rengaine", 14, new List<HitboxDef>());
            sheathe.chargeFrame = 12; sheathe.maxCharge = 75; sheathe.perfectMove = Extra2; sheathe.perfectFrom = 18;
            sheathe.perfectTo = 32; sheathe.releaseMove = Extra3; sheathe.sparkFx = k.Pfx("glint");
            MoveDef flash = Rush("Coupe-éclair", e, k.Pfx("draw"), 22, 0, dmg, limb);
            flash.intangibleStart = 4; flash.intangibleEnd = 18; flash.fillsWill = true;
            flash.hitboxes = new List<HitboxDef> { Imp(Hb(6, 12, 20, -6, 40, dmg, 40, 55, 80, limb, 0, e)) };  // cuts all along the dash
            MoveDef plain = Mv("Coupe", 28, new List<HitboxDef> { Hb(4, 8, 46, -8, 26, dmg * 0.6f, 361, 40, 85, limb, 0, e) });
            foreach (MoveDef m in new[] { sheathe, flash, plain }) Sc(m, k.S, k.P);
            if (slot < 0)
            {
                k.c.moves[Extra1] = sheathe; k.c.moves[Extra2] = flash; k.c.moves[Extra3] = plain;
                foreach (int id in new[] { Jab, Jab2, Jab3 }) k.c.moves[id].holdMove = Extra1;
                return;
            }
            var w = k.c.weapons[slot].moves;
            foreach (var (id, m) in w)
                if (id == Jab || id == Jab2 || id == Jab3) m.holdMove = Extra1;
            w.Add((Extra1, sheathe)); w.Add((Extra2, flash)); w.Add((Extra3, plain));
        }

        // Iaijutsu: drawn on frame 8, held to 24; three hits at `dist`; four charges, then a long recharge.
        static MoveDef FarCut(string name, HitFx e, string fx, float dist, float dmg)
        {
            MoveDef m = Mv(name, 52, new List<HitboxDef>());
            m.channelEnd = 24; m.projectileFrame = 8;
            m.projectile = Proj(new Vec2(dist, -10), new Vec2(0, 0), 0, 42, 56, dmg, 25, 30, e, ProjVisual.Crescent);
            m.projectile.fx = fx; m.projectile.piercing = true; m.projectile.rehit = 14;
            m.castFx = "pfx_h01_degaine"; m.castFxFrame = m.projectileFrame; m.castFxStays = true;
            m.projectile.ignoreGround = true; m.charges = 4; m.cooldown = 8 * 60; m.pose = PoseHint.Cast;
            return m;
        }

        // The awakened M1: the weapon's M1 again, each cut begun by a vanish and a short step (ids FTilt/UTilt/DTilt/FAir).
        static void BlinkCuts(Builder k, int slot)
        {
            var w = k.c.weapons[slot].moves;
            var cuts = new List<(int, MoveDef)>();
            foreach (var (id, m) in w)
            {
                int to = id == Jab ? FTilt : id == Jab2 ? UTilt : id == Jab3 ? DTilt : id == NAir ? FAir : -1;
                if (to < 0) continue;
                MoveDef b = m.Clone();  // a C++ by-value copy
                b.teleportFrame = 3; b.teleportDist = 60 * k.S; b.teleportForward = true; b.intangibleStart = 1; b.intangibleEnd = 5;
                b.nextMove = m.nextMove == Jab2 ? UTilt : m.nextMove == Jab3 ? DTilt : -1;
                cuts.Add((to, b));
            }
            w.AddRange(cuts);
        }

        // Prescience: the guard held parries up to three blows; the riposte on release.
        static MoveDef Prescience(MoveDef m, string spark)
        {
            m.sparkFx = spark; m.chargeFrame = 8; m.maxCharge = 120; m.parries = 3; m.fillsWill = true;
            return m;
        }

        static MoveDef Far(MoveDef m) { m.teleportDist *= 1.25f; return m; }  // the electric dash goes 25 % further

        static CharacterDef MakeH01()
        {
            Builder k = Start(1, "Lame blanche", BuildSize.Medium, HitFx.Lightning,
                              Colours(C(236, 232, 228), C(200, 36, 44), C(232, 178, 64), C(224, 222, 236), C(214, 40, 48)),
                              Colours(C(236, 232, 228), C(120, 64, 180), C(232, 178, 64), C(224, 222, 236), C(150, 90, 230)));
            CharacterDef c = k.c;
            c.id = "h01"; c.name = "Hakuen"; c.hero = Hero.H01; c.walkStride = 1.6f;
            HitFx e = k.e;
            Skills(k, -1, Piercing(Shot("Tranchant du néant", e, ProjVisual.Crescent, k.Pfx("crescent"), 10, 30, 9)),
                   Far(Through("Pas-iaï", e, k.Pfx("draw"), 9)), Rise("Spirale montante", e, k.Pfx("spiral"), 15.5f, 4, Limb.ArmFront),
                   Prescience(Counter("Prescience", e, null, false, Limb.ArmFront), k.Pfx("parry")));
            UltimateOf(k, -1, Ult.Thunder("FORGE DE L'AUBE", e, k.Pfx("ult"), 28, 5));
            DrawCut(k, -1, Limb.ArmFront, 11);

            Arm(k, 0, "Nodachi", "Longue lame courbe", WeaponVisual.Held, KitShape.Blade, 100, 1.0f,
                new[] { "Coupe diagonale", "Coupe montante", "Coupe descendante" });
            Skills(k, 0, FarCut("Fente écarlate", e, k.Pfx("jugement"), 200, 3),
                   Far(Through("Ruée-iaï", e, k.Pfx("draw"), 11)), Rise("Coupe en spirale", e, k.Pfx("spiral"), 15.5f, 4),
                   Prescience(Counter("Prescience de la lame", e, null, false), k.Pfx("parry")));
            UltimateOf(k, 0, Ult.Ambush("CRÉPUSCULE ÉCARLATE", e, k.Pfx("a_ult"), Limb.Weapon, 7));
            DrawCut(k, 0, Limb.Weapon, 14);

            Arm(k, 1, "Lame éveillée", "Très long odachi, garde en étoile dorée", WeaponVisual.Held, KitShape.Blade, 100, 1.05f,
                new[] { "Fente montante", "Croix pourpre", "Coupe rasante" });
            Skills(k, 1, Piercing(Shot("Trait final", e, ProjVisual.Crescent, k.Pfx("b_trait"), 11, 28, 10)),
                   Far(Through("Ruée ardente", e, k.Pfx("draw"), 12)), Rise("Spirale de feu", e, k.Pfx("spiral"), 16, 5),
                   Prescience(Counter("Prescience ardente", e, null, false), k.Pfx("parry")));
            UltimateOf(k, 1, Ult.Ambush("SENTENCE POURPRE", e, k.Pfx("b_ult"), Limb.Weapon, 9));
            DrawCut(k, 1, Limb.Weapon, 15);
            BlinkCuts(k, 0);  // after DrawCut: the blink cuts keep the hold that sheathes
            BlinkCuts(k, 1);

            // Her swings follow the drawn blade: arc a0 -> a1 and the tip's distance from the shoulder, in heights.
            var fits = new (int slot, int move, int startAt, float a0, float a1, float tip)[]
            {
                (0, Jab, 0, 120, -45, 0.95f), (0, Jab2, 0, -45, 50, 0.85f), (0, Jab3, 0, 115, -30, 0.9f),
                (0, NAir, 0, 40, 25, 0.6f), (0, FTilt, 0, -30, -46, 0.8f), (0, UTilt, 0, 112, 130, 0.95f),
                (0, DTilt, 0, -8, -20, 0.72f), (0, FAir, 0, -95, -112, 0.6f),
                (1, Jab, 0, -60, 45, 1.0f), (1, Jab2, 0, 15, -5, 0.8f), (1, Jab3, 0, -100, -37, 0.95f),
                (1, NAir, 0, -95, -108, 0.62f), (1, FTilt, 0, 45, 60, 0.95f), (1, UTilt, 0, 12, 2, 0.68f),
                (1, DTilt, 0, -14, -28, 0.78f), (1, FAir, 0, -90, -115, 0.62f)
            };
            foreach (var f in fits)
            {
                WeaponDef w = c.weapons[f.slot];
                float r = (float)((float)(f.tip - kGripDist) * c.height) / w.blade;  // tip as a fraction of the blade
                foreach (var (id, m) in w.moves)
                    if (id == f.move)
                        for (int i = f.startAt; i < m.hitboxes.Count; ++i)
                        {
                            HitboxDef h = m.hitboxes[i];
                            if (!h.swing) continue;
                            bool tip = h.reach0 > 0.0f;  // the kit pairs a tip box (0.55..1) with a base box (0..0.55)
                            h.a0 = f.a0; h.a1 = f.a1;
                            h.reach0 = tip ? 0.55f * r : 0.0f;
                            h.reach1 = tip ? r : 0.55f * r;
                            m.hitboxes[i] = h;
                        }
            }
            // Each plain blade move's cut is a Blender effect pfx_h01_<a|b>_<clip>, played on its first hitbox.
            var clips = new (int id, string clip)[]
            {
                (FTilt, "ftilt"), (UTilt, "utilt"), (DTilt, "dtilt"), (NAir, "nair"), (FAir, "fair"), (FThrow, "fthrow"),
                (UThrow, "uthrow"), (LedgeAttack, "ledgeattack"), (GetupAttack, "getupattack")
            };
            foreach (int slot in new[] { 0, 1 })
                foreach (var (id, m) in c.weapons[slot].moves)
                    foreach (var (cid, clip) in clips)
                        if (id == cid && m.castFx == null && m.hitboxes.Count > 0)
                        {
                            m.castFx = k.Pfx((slot != 0 ? "b_" : "a_") + clip);
                            m.castFxFrame = m.hitboxes[0].start;
                            foreach (var h in m.hitboxes) m.castFxFrame = Math.Min(m.castFxFrame, h.start);
                        }
            // Weapon B: its third cut skims the floor and sends a wave ahead, low.
            foreach (var (id, m) in c.weapons[1].moves)
                if (id == Jab3 && m.hitboxes.Count > 0)
                {
                    m.projectileFrame = m.hitboxes[0].start;
                    foreach (var h in m.hitboxes) m.projectileFrame = Math.Min(m.projectileFrame, h.start);
                    m.projectile = Proj(new Vec2(36 * k.S, 28 * k.S), new Vec2(11, 0), 0, 18, 20, 3.5f * k.P, 20, 40, e, ProjVisual.Crescent);
                    m.projectile.fx = k.Pfx("b_wave");
                    m.projectile.piercing = true;
                }
            // Volonté de la lame (armed only): 3 charges, then awake 8 s: blink cuts, waves, a fan of three crescents.
            int awake = Ult.Form(c, k.Pfx("aura"), e, 8, 1.1f, 1.0f);
            FormDef aw = c.forms[awake];
            aw.m1Waves = true; aw.m1Blink = true;
            aw.m1Wave = Proj(new Vec2(30, -6), new Vec2(11, 0), 0, 26, 16, 3.5f, 20, 40, e, ProjVisual.Crescent);
            aw.m1Wave.fx = k.Pfx("crescent"); aw.m1Wave.piercing = true;
            MoveDef fan = Spread(Piercing(Shot("Trois croissants", e, ProjVisual.Crescent, k.Pfx("crescent"), 10, 22, 7)), 3, 14);
            Sc(fan, k.S, k.P);
            fan.cooldown = kSkillCooldown;
            aw.moves.Add((NSpecial, fan));
            c.willForm = awake;
            return c;
        }

        static MoveDef MoveOf(CharacterDef c, int slot, int move)
        {
            if (slot < 0) return move >= 0 && move < MOVE_COUNT ? c.moves[move] : null;
            if (slot >= c.weapons.Length) return null;
            foreach (var (id, m) in c.weapons[slot].moves)
                if (id == move) return m;
            return null;
        }

        // Each fit moves one hitbox onto its clip's drawing; dropped if the radius changed since the fit.
        static void ApplyHitFit(List<CharacterDef> v)
        {
            foreach (HitFit h in RosterHitfit.entries)
                foreach (CharacterDef c in v)
                {
                    if (c.id != h.fighter) continue;
                    MoveDef m = MoveOf(c, h.slot, h.move);
                    if (m == null || h.index < 0 || h.index >= m.hitboxes.Count) continue;
                    HitboxDef x = m.hitboxes[h.index];
                    if (MathF.Abs(x.radius - h.was) > 0.01f) continue;
                    x.radius = h.r;
                    if (x.swing) { x.a0 = h.a0; x.a1 = h.a1; x.reach1 = h.reach1; }
                    else x.offset = new Vec2(h.x, h.y);
                    m.hitboxes[h.index] = x;
                }
        }

        // The owner's pace (Rivals of Aether as a model): faster runs and falls, the same jump heights, lighter landings.
        static void ApplyTempo(List<CharacterDef> v)
        {
            const float kGravity = 1.15f;
            float jump = MathF.Sqrt(kGravity);
            static void Land(MoveDef m) =>
                m.landingLag = Math.Max(3, (int)MathF.Round((float)(m.landingLag * 0.6f), MidpointRounding.AwayFromZero));
            foreach (CharacterDef c in v)
            {
                c.walkSpeed *= 1.08f; c.runSpeed *= 1.12f; c.groundAccel *= 1.25f;
                c.airSpeed *= 1.08f; c.airAccel *= 1.15f;
                c.gravity *= kGravity; c.maxFall *= 1.08f; c.fastFall *= 1.1f;
                c.jumpVel *= jump; c.shortHopVel *= jump; c.airJumpVel *= jump;
                c.jumpSquat = Math.Max(3, c.jumpSquat - 1);
                foreach (MoveDef m in c.moves) Land(m);
                foreach (WeaponDef w in c.weapons) foreach (var (_, m) in w.moves) Land(m);
                foreach (FormDef f in c.forms) foreach (var (_, m) in f.moves) Land(m);
            }
        }

        static List<CharacterDef> Build()
        {
            var v = new List<CharacterDef> { MakeH01(), Make02(), Make03(), Make07(), Make08() };  // Hakuen took the old 01's place
            if (!rawHitboxes) ApplyHitFit(v);
            ApplyTempo(v);
            return v;
        }
    }
}
