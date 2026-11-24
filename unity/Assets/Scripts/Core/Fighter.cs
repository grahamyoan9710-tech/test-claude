using System;
using System.Collections.Generic;

namespace ClashHorizon.Core
{
    // Port of src/core/fighter.hpp.
    public enum FState : byte
    {
        Idle, Walk, Run, Crouch, JumpSquat, Air, Landing, Attack,
        Shield, ShieldStun, Roll, SpotDodge, AirDodge,
        Hitstun, Knockdown, GetUp, Helpless,
        LedgeHang, LedgeClimb, Holding, Grabbed, ShieldBreak,
        Captured,  // held by an opponent's cinematic ultimate (HitboxDef.capture)
        Dead, Respawn
    }

    public sealed class Fighter
    {
        // Free constants in C++; callers use `using static ClashHorizon.Core.Fighter`.
        public const float kMaxShield = 50.0f;
        public const float kUltMax = 100.0f;
        public const float kUltPerDamageDealt = 0.0f;  // the ultimate comes from the Clash Orb only
        public const float kUltPerDamageTaken = 0.0f;
        public const float kUltPerShieldHit = 0.0f;
        public const int kMaxCpuLevel = 10;
        public const float kLevel10Power = 2.0f;
        public const float kDamageScale = 0.8f;
        public const int kWillMax = 3;

        public int id = 0;           // index in Match.fighters
        public int port = 0;         // controller slot (0..3)
        public int team = 0;         // one team cannot hurt itself (free for all: its own id)
        public bool inTeam = false;
        public int charIndex = 0;
        public int palette = 0;
        public CharacterDef def;     // the shared roster entry: never change it
        public bool cpu = false;
        public int cpuLevel = 5;

        public Vec2 pos;             // feet position
        public Vec2 vel;             // self-propelled velocity
        public Vec2 kbVel;           // knockback velocity (decays on its own)
        public int facing = 1;

        public FState state = FState.Idle;
        public int stateFrame = 0;
        public int timer = 0;
        public int actionDir = 0;

        public float percent = 0.0f;
        public int stocks = 3;
        public bool eliminated = false;

        public bool grounded = false;
        public int groundPlat = -1;
        public int airJumps = 0;
        public bool fastFall = false;
        public bool airDodgeUsed = false;
        public bool sideSpecialUsed = false;
        public int[] moveCooldown = new int[MoveId.MOVE_COUNT];
        public int[] moveUses = new int[MoveId.MOVE_COUNT];

        // Current move
        public int move = -1;
        public int moveFrame = 0;
        public int charge = 0;
        public bool charging = false;
        public bool comboQueued = false;
        public bool counterTriggered = false;
        public bool moveLeftGround = false;
        public bool moveStartedAir = false;
        public int parried = 0;
        public bool willGiven = false;
        public bool landedHit = false;
        public int cancelling = -1;
        public List<int> hitList = new List<int>();  // victim * 64 + group

        // Combat
        public int hitlag = 0;
        public int hitstun = 0;
        public bool tumble = false;
        public float shield = kMaxShield;
        public int intangible = 0;
        public int invincible = 0;
        public int techWindow = 0;
        public int techLockout = 0;
        public int lastHitBy = -1;
        public int lastHitFrame = -10000;
        public float lastKnockback = 0.0f;

        // Ledges / grabs / platforms
        public int ledge = -1;
        public int ledgeCooldown = 0;
        public int ledgeAction = 0;  // 0 climb, 1 attack, 2 roll
        public Vec2 ledgeFrom, ledgeTo;
        public int dropThrough = 0;
        public int grabPartner = -1;
        public int escapeTimer = 0;
        public int respawnTimer = 0;
        public float ult = 0.0f;
        public int weapon = -1;
        public int weapons = 0;
        public int weaponBuffer = 0;
        public int attackBuffer = 0;
        public int form = -1;
        public int formTimer = 0;
        public int formMove = -1;
        public int captor = -1;
        public Vec2 capturePos;
        public bool finalSmash = false;
        public int will = 0;
        public int attackHeld = 0;

        // Input
        public InputState @in, prev;
        public Vec2[] stickHist = new Vec2[6];
        public byte[] dirHist = new byte[30];  // stick direction, newest first (numpad: 5 = neutral)

        // Stats
        public int kos = 0;
        public int falls = 0;
        public float damageDealt = 0.0f;
        public int koFrame = -1;

        // Visual helpers (read by the renderer)
        public int flash = 0;
        public HitFx flashFx = HitFx.Normal;
        public int airJumpFrame = -1000;
        public float spin = 0.0f;

        public float Width() => def.width;
        public float Height() => def.height;
        public Vec2 Center() => new Vec2(pos.x, pos.y - def.height * 0.5f);

        public Rect Hurtbox()
        {
            float h = def.height;
            if (state == FState.Crouch) h *= 0.7f;
            if (state == FState.Knockdown) h *= 0.4f;
            return new Rect(pos.x - def.width * 0.5f, pos.y - h, def.width, h);
        }

        public WeaponDef WeaponDef() => weapon >= 0 ? def.weapons[weapon & 1] : null;
        public FormDef FormDef() => form >= 0 && form < def.forms.Count ? def.forms[form] : null;

        public Form FormKind()
        {
            FormDef fd = FormDef();
            return fd != null ? fd.form : Form.None;
        }

        // A move as this fighter performs it: a transformation, then the held weapon may replace it.
        public MoveDef MoveDef(int id)
        {
            FormDef fd = FormDef();
            if (fd != null)
                foreach (var (mid, m) in fd.moves)
                    if (mid == id) return m;
            WeaponDef w = WeaponDef();
            if (w != null)
                foreach (var (mid, m) in w.moves)
                    if (mid == id) return m;
            return def.moves[id];
        }

        // Blade length (grip to tip) of the weapon in hand.
        public float BladeLength()
        {
            WeaponDef w = WeaponDef();
            if (w != null && w.blade > 0.0f) return w.blade;
            return def.weaponLength * (w != null ? w.reachMul : 1.0f);
        }

        public float DamageMul()
        {
            float m = kDamageScale;
            WeaponDef w = WeaponDef();
            if (w != null) m *= w.damageMul;
            FormDef fd = FormDef();
            if (fd != null) m *= fd.damageMul;
            if (cpu && cpuLevel >= kMaxCpuLevel) m *= kLevel10Power;
            return m;
        }

        public MoveDef CurrentMove() => state == FState.Attack && move >= 0 ? MoveDef(move) : null;

        // C++ takes a member pointer; callers pass a static lambda such as `static i => i.jump`.
        public bool Pressed(Func<InputState, bool> b) => b(@in) && !b(prev);
        public bool CstickPressed() => @in.cstick.Len() > 0.5f && prev.cstick.Len() <= 0.5f;

        // Fast flick of the stick (smash attacks, dashes, fast falls, rolls).
        public bool FlickX()
        {
            if (MathF.Abs(@in.stick.x) < 0.8f) return false;
            for (int i = 1; i < 4; ++i)
                if (MathF.Abs(stickHist[i].x) < 0.3f) return true;
            return false;
        }

        public bool FlickY()
        {
            if (MathF.Abs(@in.stick.y) < 0.8f) return false;
            for (int i = 1; i < 4; ++i)
                if (MathF.Abs(stickHist[i].y) < 0.3f) return true;
            return false;
        }

        public bool IsIntangible() => intangible > 0 || invincible > 0;

        public void GainWill(int n)
        {
            if (def.willForm >= 0 && weapon >= 0) will = will + n > kWillMax ? kWillMax : will + n;  // armed only
        }

        public bool Active() => !eliminated && state != FState.Dead;
    }
}
