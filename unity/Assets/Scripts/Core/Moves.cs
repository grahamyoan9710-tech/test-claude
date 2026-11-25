using System.Collections.Generic;

namespace ClashHorizon.Core
{
    // Port of src/core/moves.hpp. Structs with C++ defaults start from T.Default, never new T() or default.
    public enum Limb : byte { None, ArmFront, ArmBack, LegFront, LegBack, Body, Head, Weapon }

    public enum HitFx : byte
    {
        Normal, Fire, Electric, Slash, Dark, Magic, Shadow, Ice, Star, Water, Rose, Lightning, Light, Wind, Arcane, Sand,
        Blood, Moon, Sun, Nature, Count
    }

    public struct HitboxDef
    {
        public int start, end;  // active on frames [start, end)
        public Vec2 offset;     // from the fighter's center, as if facing right (+y = down)
        public float radius, damage, angle, baseKB, growth;
        public int group;
        public Limb limb;
        public HitFx fx;
        public bool reverse, impact, swing;
        public float a0, a1, reach0, reach1;
        public bool capture, launch;

        public static readonly HitboxDef Default = new HitboxDef
        {
            radius = 20.0f, damage = 5.0f, angle = 361.0f, baseKB = 20.0f, growth = 80.0f, limb = Limb.ArmFront, reach1 = 1.0f
        };
    }

    public static class Moves
    {
        public const float kShoulderX = 0.03f, kShoulderY = -0.17f;
        public const float kGripDist = 0.28f;
    }

    public enum ProjVisual : byte { Orb, Crescent, Wave, Bolt, Star, Arrow, Spear, Blade, Feather, Pillar, Bomb, Beast, Count }

    public enum ProjTarget : byte { Self, Nearest, All }

    public struct ProjectileDef
    {
        public Vec2 spawnOffset, velocity;
        public float gravity;
        public int lifetime;
        public float radius;
        public HitboxDef hit;
        public int count;
        public float spreadDeg;
        public bool bounces, piercing;
        public ProjVisual visual;
        public string fx;
        public int rehit;
        public float pull, pullRadius;
        public bool boomerang;
        public float grow, explodeRadius;
        public HitboxDef explode;
        public ProjTarget target;
        public bool follow;
        public int armDelay;
        public bool ignoreGround, column;
        public Vec2 jitter;

        public static readonly ProjectileDef Default = new ProjectileDef
        {
            lifetime = 60, radius = 12.0f, hit = HitboxDef.Default, count = 1, explode = HitboxDef.Default
        };
    }

    public struct ProjShot
    {
        public int frame;
        public ProjectileDef def;

        public static readonly ProjShot Default = new ProjShot { def = ProjectileDef.Default };
    }

    public struct BeamDef
    {
        public int start, end;  // drawn on frames [start, end)
        public Vec2 from;
        public float angle, length, width;
        public HitFx fx;
        public string sprite;

        public static readonly BeamDef Default = new BeamDef { start = -1, end = -1 };
    }

    public enum PoseHint : byte
    {
        Auto, Cast, Overhead, Thrust, Spin, HandSign, Bow, Throw, Uppercut, Stomp, Roar, Rush, Raise, Point, TwoHanded, Kneel
    }

    public enum UltFx : byte { None, Cinematic, Count }

    public enum Form : byte { None, Awakened, Count }

    public enum VelMode : byte { Set, SetX, SetY, Add, Scale, StickDir }

    public struct VelEvent
    {
        public int frame;
        public VelMode mode;
        public Vec2 v;
        public float stickX;
        public bool airOnly;
    }

    // A plain int enum in C++: ids index arrays and mix with ints.
    public static class MoveId
    {
        public const int Jab = 0, Jab2 = 1, Jab3 = 2, FTilt = 3, UTilt = 4, DTilt = 5, DashAttack = 6, FSmash = 7, USmash = 8,
            DSmash = 9, NAir = 10, FAir = 11, BAir = 12, UAir = 13, DAir = 14, NSpecial = 15, SSpecial = 16, USpecial = 17,
            DSpecial = 18, Grab = 19, Pummel = 20, FThrow = 21, BThrow = 22, UThrow = 23, DThrow = 24, LedgeAttack = 25,
            GetupAttack = 26, Extra1 = 27, Extra2 = 28, Extra3 = 29, FinalSmash = 30, MOVE_COUNT = 31;
    }

    public sealed class MoveDef
    {
        public string name = "";
        public int duration = 0;
        public int landingLag = 6;
        public List<HitboxDef> hitboxes = new List<HitboxDef>();
        public List<VelEvent> vel = new List<VelEvent>();
        public float gravityMul = 1.0f;
        public float frictionMul = 1.0f;
        public bool allowDrift = true;
        public int chargeFrame = -1;
        public int maxCharge = 60;
        public float chargeDamageMul = 0.4f;
        public int projectileFrame = -1;
        public int projectileRepeat = 0;
        public int projectileEvery = 0;
        public ProjectileDef projectile = ProjectileDef.Default;
        public List<ProjShot> shots = new List<ProjShot>();
        public int intangibleStart = -1, intangibleEnd = -1;
        public int armorStart = -1, armorEnd = -1;
        public int counterStart = -1, counterEnd = -1;
        public int teleportFrame = -1;
        public float teleportDist = 0.0f;
        public bool teleportForward = false;
        public bool turnAfterTeleport = false;
        public int cooldown = 0;
        public int charges = 0;
        public int channelEnd = -1;
        public int superFlash = 0;
        public string castFx = null;
        public int castFxFrame = -1;
        public Vec2 castFxAt;
        public bool castFxStays = false;
        public float castFxScale = 1.0f;
        public string sparkFx = null;
        public PoseHint pose = PoseHint.Auto;
        public BeamDef beam = BeamDef.Default;
        public bool reflects = false;
        public bool helplessAfter = false;
        public bool endOnLand = false;
        public int landMove = -1;
        public int loopFrame = -1;
        public int nextMove = -1;
        public int comboStart = 0, comboEnd = 0;
        public int comboAt = 0;
        public bool comboOnSpecial = false;
        public int perfectMove = -1, perfectFrom = 0, perfectTo = 0;
        public int releaseMove = -1;
        public int holdMove = -1;
        public bool fillsWill = false;
        public int parries = 0;
        public UltFx ultFx = UltFx.None;
        public Vec2 holdAt;
        public bool holdInPlace = false;
        public bool teleportToTarget = false;
        public int formIndex = -1;
        public int formFrame = -1;
        public bool isGrab = false;
        public int throwFrame = -1;
        public bool pummel = false;
        public bool airOnly = false;

        public bool Exists() => duration > 0;

        public MoveDef Clone()
        {
            var c = (MoveDef)MemberwiseClone();
            c.hitboxes = new List<HitboxDef>(hitboxes);
            c.vel = new List<VelEvent>(vel);
            c.shots = new List<ProjShot>(shots);
            return c;
        }

        internal static List<(int first, MoveDef second)> CloneMoves(List<(int first, MoveDef second)> moves)
        {
            var c = new List<(int first, MoveDef second)>(moves.Count);
            foreach (var m in moves) c.Add((m.first, m.second.Clone()));
            return c;
        }
    }

    public enum BodyStyle : byte { Brawler, Speedster, Brute, Knight, Puff, Ninja }

    public struct Palette
    {
        public Color3 primary, secondary, accent, skin, hair, eyes;

        public static readonly Palette Default = new Palette
        {
            primary = Color3.Default, secondary = Color3.Default, accent = Color3.Default, skin = Color3.Default,
            hair = new Color3(40, 30, 30), eyes = new Color3(70, 52, 44)
        };
    }

    public enum Hero : byte { P01, P02, P03, P04, P05, P06, P07, P08, P09, P10, P11, P12, P13, P14, P15, P16, P17, H01, Count }

    public enum WeaponVisual : byte { None, Held, Floating, Count }

    public sealed class WeaponDef
    {
        public string name = "";
        public string desc = "";
        public WeaponVisual visual = WeaponVisual.None;
        public HitFx fx = HitFx.Normal;
        public float damageMul = 1.2f;
        public float kbMul = 1.1f;
        public float reachMul = 1.0f;
        public float blade = 0.0f;
        public List<(int first, MoveDef second)> moves = new List<(int first, MoveDef second)>();
        public bool kit = false;

        public WeaponDef Clone()
        {
            var c = (WeaponDef)MemberwiseClone();
            c.moves = MoveDef.CloneMoves(moves);
            return c;
        }
    }

    public sealed class FormDef
    {
        public Form form = Form.None;
        public int duration = 0;
        public HitFx fx = HitFx.Normal;
        public float damageMul = 1.0f;
        public float kbTakenMul = 1.0f;
        public bool superArmor = false;
        public string aura = null;
        public List<(int first, MoveDef second)> moves = new List<(int first, MoveDef second)>();
        public bool m1Waves = false;
        public bool m1Blink = false;
        public ProjectileDef m1Wave = ProjectileDef.Default;

        public FormDef Clone()
        {
            var c = (FormDef)MemberwiseClone();
            c.moves = MoveDef.CloneMoves(moves);
            return c;
        }
    }

    public sealed class CharacterDef
    {
        public string id = "";
        public string name = "";
        public string title = "";
        public string series = "";
        public HitFx element = HitFx.Normal;
        public Hero hero = Hero.P01;
        public BodyStyle style = BodyStyle.Brawler;
        public Palette[] palettes = { Palette.Default, Palette.Default };
        public float width = 50, height = 100;
        public float weight = 100;
        public float walkSpeed = 4.0f, runSpeed = 8.0f, groundAccel = 1.2f, friction = 0.6f;
        public float walkStride = 0.8f;
        public float airSpeed = 5.0f, airAccel = 0.45f, airFriction = 0.06f;
        public float gravity = 0.55f, maxFall = 9.0f, fastFall = 14.0f;
        public float jumpVel = 14.0f, shortHopVel = 9.5f, airJumpVel = 13.0f;
        public int airJumps = 1;
        public int jumpSquat = 4;
        public float weaponLength = 0.0f;
        public MoveDef[] moves = NewMoves();
        public WeaponDef[] weapons = { new WeaponDef(), new WeaponDef() };
        public List<FormDef> forms = new List<FormDef>();
        public bool armed = true;
        public int willForm = -1;

        static MoveDef[] NewMoves()
        {
            var a = new MoveDef[MoveId.MOVE_COUNT];
            for (int i = 0; i < a.Length; i++) a[i] = new MoveDef();
            return a;
        }

        public CharacterDef Clone()
        {
            var c = (CharacterDef)MemberwiseClone();
            c.palettes = (Palette[])palettes.Clone();
            c.moves = new MoveDef[moves.Length];
            for (int i = 0; i < moves.Length; i++) c.moves[i] = moves[i].Clone();
            c.weapons = new WeaponDef[weapons.Length];
            for (int i = 0; i < weapons.Length; i++) c.weapons[i] = weapons[i].Clone();
            c.forms = new List<FormDef>(forms.Count);
            foreach (var f in forms) c.forms.Add(f.Clone());
            return c;
        }
    }
}
