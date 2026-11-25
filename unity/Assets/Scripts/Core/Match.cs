using System;
using System.Collections.Generic;
using System.Linq;
using static ClashHorizon.Core.MathUtil;
using static ClashHorizon.Core.MoveId;
using static ClashHorizon.Core.Fighter;

namespace ClashHorizon.Core
{
    // src/core/match.hpp + match.cpp. Match is split across Match, FighterUpdate, FighterPhysics, Combat and Weapons
    // like the C++ .cpp files.
    public sealed class MatchConfig
    {
        public struct Slot
        {
            public bool active;
            public int character;
            public bool cpu;
            public int level;  // CPU level 1-10
            public int team;   // team battles: 0 red, 1 blue, 2 green, 3 yellow
            public Slot(bool active, int character, bool cpu, int level, int team = 0)
            {
                this.active = active; this.character = character; this.cpu = cpu; this.level = level; this.team = team;
            }
            public static readonly Slot Default = new Slot(false, 0, false, 5, 0);
        }
        public int stage = 0;
        public int stocks = 3;
        public bool items = true;     // Clash Orbs
        public bool weapons = true;   // weapon drops
        public bool training = false;
        public bool teams = false;    // team battle (2 v 2...): the last team standing wins
        public Slot[] slots = { Slot.Default, Slot.Default, Slot.Default, Slot.Default };
        public uint seed = 12345;
        // C++ copies the config by value.
        public MatchConfig Clone()
        {
            var c = (MatchConfig)MemberwiseClone();
            c.slots = (Slot[])slots.Clone();
            return c;
        }
    }

    public enum EvType : byte
    {
        Hit, ShieldHit, KO, Jump, AirJump, Land, Dash, Grab, Throw, Projectile, ShieldBreak, Respawn, Tech, LedgeGrab,
        Counter, Swing, Clank, Countdown, Go, GameSet, OrbSpawn, OrbHit, OrbBreak, FinalSmash, UltReady,
        WeaponSpawn, WeaponLand,
        WeaponSummon, WeaponVanish,  // taken (or switched to) / lost: a = fighter, b = weapon slot, strength = character
        SuperFlash, Explosion, Reflect, Transform, FormEnd, Capture, Glint
    }

    // A fighter's weapon lying on the stage (or falling on it): its owner takes it by standing on it and holding the
    // weapon button for kPickupFrames in all (a hit pauses the count, it does not reset it).
    public sealed class WeaponItem
    {
        public int character = 0;
        public int slot = 0;
        public Vec2 pos, vel;
        public bool grounded = false;
        public int life = 0;  // frames left before it vanishes (nobody took it)
        public int age = 0;
        public int hold = 0;
    }

    public sealed class Orb
    {
        public bool active = false;
        public Vec2 pos, vel;
        public float hp = 0.0f;
        public float radius = 26.0f;
        public int life = 0;
        public int flash = 0;
        public List<int> hitBy = new List<int>();  // attacker * 4096 + move instance
    }

    // A class: combat writes events.back().impact / .visual in place.
    public sealed class GameEvent
    {
        public EvType type = EvType.Hit;
        public Vec2 pos;
        public Vec2 dir;
        public float strength = 0.0f;
        public int a = -1;  // main fighter (attacker / jumper / KO'd fighter)
        public int b = -1;  // other fighter (victim)
        public HitFx fx = HitFx.Normal;
        public bool impact = false;  // signature hit (impact frames)
        public int visual = -1;
    }

    public sealed class Projectile
    {
        public int owner = -1;
        public Vec2 pos, vel;
        public float gravity = 0.0f;
        public float radius = 10.0f;
        public int life = 0, maxLife = 1;
        public HitboxDef hit = HitboxDef.Default;
        public bool bounces = false;
        public bool piercing = false;
        public ProjVisual visual = ProjVisual.Orb;
        public string fx = null;  // its PixelLab effect (ProjectileDef.fx)
        public bool alive = true;
        public List<int> hitList = new List<int>();
        // Behaviours (see ProjectileDef)
        public int rehit = 0;
        public List<(int id, int frame)> lastHit = new List<(int id, int frame)>();  // fighter id -> frame (rehit)
        public float pull = 0.0f, pullRadius = 0.0f;
        public bool boomerang = false, returning = false;
        public float grow = 0.0f;
        public float explodeRadius = 0.0f;
        public HitboxDef explode = HitboxDef.Default;
        public int age = 0;
        public float spawnRadius = 10.0f;
        public bool follow = false;  // sticks to the first fighter it hits (stuckTo) for the rest of its life
        public int stuckTo = -1;
        public Vec2 stickOffset;     // from the victim's center
        public int armDelay = 0;     // harmless and motionless while age < armDelay
        public bool ignoreGround = false;
        public bool column = false;  // hits everything above it, not only its circle
        public bool ult = false;     // fired by an ultimate: cannot be blocked, countered or reflected
        public bool Armed() => age >= armDelay;
    }

    public sealed partial class Match
    {
        public const int kPickupFrames = 60;  // 1 s (namespace scope in match.hpp)
        public const int kTrainStocks = 3;

        public MatchConfig cfg;
        public StageDef stageDef;
        public List<Fighter> fighters = new List<Fighter>();
        public List<Platform> platforms;
        public List<Projectile> projectiles = new List<Projectile>();
        public List<GameEvent> events = new List<GameEvent>();
        public Orb orb = new Orb();
        public int orbTimer = 0;      // frames until the next orb
        public List<WeaponItem> items = new List<WeaponItem>();
        public int itemTimer = 0;     // frames until the next weapon drop
        public int superFreeze = 0;   // "super flash": everybody frozen while the move's cut-in plays
        public int superOwner = -1;
        public int freezeTimer = 0;   // final smash cinematic: everyone but the user is frozen
        public int freezeOwner = -1;
        public int frame = 0;
        public int countdown = 200;
        public bool finished = false;
        public int winner = -1;
        public int winnerTeam = -1;   // team battles: the team left standing
        // Training options (the pause menu of Entraînement), changed during the session.
        public bool trainInfiniteUlt = false;  // the ultimate meter fills up again at once
        public bool trainZeroPercent = false;  // every fighter stays at 0 %
        public bool trainFight = false;        // 3 stocks each; when someone runs out, the fight starts over
        public int trainRestart = 0;           // frames before the fight starts over (after the last stock)
        public int finishFrame = 0;
        public Rng rng;

        public StageDef Stage() => stageDef;
        public bool Started() => countdown <= 0;
        public int LedgeCount() => platforms.Count * 2;
        // Can a hurt b (not itself, not a teammate)? `owner` -1: nobody's (the stage, an orb...).
        public bool Foes(Fighter a, Fighter b) => a.id != b.id && a.team != b.team;
        public bool Foes(int owner, Fighter b) => owner < 0 || owner >= fighters.Count || Foes(fighters[owner], b);

        public Match(MatchConfig c)
        {
            cfg = c.Clone();
            rng = new Rng(c.seed);
            var stages = ClashHorizon.Core.Stage.List();
            // C++ holds a const pointer; the clone keeps the shared stage list safe from a match.
            stageDef = stages[Math.Clamp(cfg.stage, 0, stages.Count - 1)].Clone();
            platforms = new List<Platform>(stageDef.platforms);
            var chars = Roster.All();
            var used = new int[chars.Count];  // fighters picked twice alternate their colours
            for (int port = 0; port < 4; ++port)
            {
                var slot = cfg.slots[port];
                if (!slot.active) continue;
                var f = new Fighter();
                f.id = fighters.Count;
                f.port = port;
                f.charIndex = Math.Clamp(slot.character, 0, chars.Count - 1);
                f.def = chars[f.charIndex];
                f.palette = used[f.charIndex]++ % 2;  // mirror match: alternate colours
                f.cpu = slot.cpu;
                f.cpuLevel = slot.level;
                f.team = cfg.teams ? slot.team : f.id;
                f.inTeam = cfg.teams;
                f.stocks = cfg.stocks;
                Vec2 spawn = stageDef.spawns[fighters.Count % stageDef.spawns.Count];
                f.pos = spawn;
                f.facing = spawn.x > 0 ? -1 : 1;
                f.airJumps = f.def.airJumps;
                f.grounded = false;
                fighters.Add(f);
            }
            orbTimer = rng.Irange(60 * 10, 60 * 20);
            itemTimer = rng.Irange(60 * 3, 60 * 6);
        }

        public void Emit(EvType t, Vec2 pos, int a = -1, int b = -1, float strength = 0.0f, Vec2 dir = default,
                         HitFx fx = HitFx.Normal)
        {
            events.Add(new GameEvent { type = t, pos = pos, a = a, b = b, strength = strength, dir = dir, fx = fx });
        }

        // Advance the simulation by one frame (1/60 s). inputs are indexed by port.
        public void Step(InputState[] inputs)
        {
            events.Clear();
            if (finished)
            {
                ++frame;
                return;
            }
            ClashHorizon.Core.Stage.UpdatePlatforms(platforms, frame);

            if (countdown > 0)
            {
                --countdown;
                if (countdown == 180 || countdown == 120 || countdown == 60)
                    Emit(EvType.Countdown, default, -1, -1, (float)(countdown / 60));
                if (countdown == 0) Emit(EvType.Go, default);
            }

            foreach (var f in fighters)
            {
                f.prev = f.@in;
                f.@in = Started() ? inputs[f.port] : default;
                for (int i = 5; i > 0; --i) f.stickHist[i] = f.stickHist[i - 1];
                f.stickHist[0] = f.@in.stick;
                f.attackHeld = f.@in.attack ? f.attackHeld + 1 : 0;
                for (int i = f.dirHist.Length - 1; i > 0; --i) f.dirHist[i] = f.dirHist[i - 1];
                {
                    int x = f.@in.stick.x > 0.5f ? 1 : f.@in.stick.x < -0.5f ? -1 : 0;
                    int y = f.@in.stick.y > 0.5f ? 1 : f.@in.stick.y < -0.5f ? -1 : 0;
                    f.dirHist[0] = (byte)(5 + x - 3 * y);  // numpad notation: 2 down, 6 right, 3 down-right...
                }
            }
            if (freezeTimer > 0)
            {  // final smash cinematic: only its user moves
                --freezeTimer;
                UpdateFighter(fighters[freezeOwner]);
            }
            else
            {
                foreach (var f in fighters) UpdateFighter(f);
                UpdateProjectiles();
                UpdateOrb();
                UpdateItems();
            }
            ResolveHits();
            CheckBlastZones();
            CheckEnd();
            if (cfg.training) UpdateTraining();
            ++frame;
        }

        public void UpdateTraining()
        {
            foreach (var f in fighters)
            {
                if (trainZeroPercent) f.percent = 0.0f;  // after the hits: each blow only knocks back by its own damage
                bool usingUlt = f.state == FState.Attack && f.move == FinalSmash;
                if (trainInfiniteUlt && f.Active() && !f.eliminated && !f.finalSmash && !usingUlt && freezeTimer == 0)
                    GainUlt(f, kUltMax);
            }
            if (trainRestart > 0 && --trainRestart == 0) RestartTrainingFight();
        }

        public void SetTrainFight(bool on)
        {
            trainFight = on;
            RestartTrainingFight();
        }

        // Everyone back to 3 stocks, 0 % and the starting positions.
        public void RestartTrainingFight()
        {
            trainRestart = 0;
            foreach (var f in fighters)
            {
                f.stocks = kTrainStocks;
                f.eliminated = false;
            }
            ResetPositions();
        }

        public bool LedgeValid(int ledge)
        {
            int p = ledge / 2;
            return p >= 0 && p < platforms.Count && platforms[p].solid && platforms[p].ledges;
        }

        // ledge = platform * 2 + (0 left / 1 right)
        public Vec2 LedgePoint(int ledge)
        {
            Rect r = platforms[ledge / 2].rect;
            return (ledge % 2 == 0) ? new Vec2(r.Left(), r.Top()) : new Vec2(r.Right(), r.Top());
        }

        public int LedgeOwner(int ledge)
        {
            foreach (var f in fighters)
                if ((f.state == FState.LedgeHang || f.state == FState.LedgeClimb) && f.ledge == ledge) return f.id;
            return -1;
        }

        public void CheckBlastZones()
        {
            Rect b = stageDef.blast;
            foreach (var f in fighters)
            {
                if (!f.Active() || f.state == FState.Respawn) continue;
                bool @out = f.pos.x < b.Left() || f.pos.x > b.Right() || f.pos.y > b.Bottom();
                if (f.pos.y - f.Height() < b.Top())
                {
                    if (f.state == FState.Hitstun)
                    {
                        @out = true;
                    }
                    else
                    {
                        // Without hitstun you just bonk on the ceiling of the blast zone.
                        f.pos.y = b.Top() + f.Height();
                        f.vel.y = Maxf(f.vel.y, 0.0f);
                    }
                }
                if (@out) KoFighter(f);
            }
        }

        public void KoFighter(Fighter f)
        {
            Rect b = stageDef.blast;
            var p = new Vec2(Clampf(f.pos.x, b.Left(), b.Right()), Clampf(f.Center().y, b.Top(), b.Bottom()));
            Vec2 dir = (p - new Vec2(0, -150)).Norm();
            Emit(EvType.KO, p, f.id, f.lastHitBy, f.lastKnockback, dir);
            if (f.grabPartner >= 0) ReleaseGrab(f, false);
            LoseWeapon(f);
            if (f.form >= 0) EndForm(f);
            f.captor = -1;
            if (f.lastHitBy >= 0 && f.lastHitBy != f.id && frame - f.lastHitFrame < 600)
                fighters[f.lastHitBy].kos++;
            f.falls++;
            if (!cfg.training || trainFight) f.stocks--;
            if (freezeTimer > 0 && freezeOwner == f.id) freezeTimer = 0;  // the ultimate meter itself is kept
            f.state = FState.Dead;
            f.stateFrame = 0;
            f.respawnTimer = 100;
            f.vel = f.kbVel = new Vec2(0, 0);
            f.hitstun = f.hitlag = 0;
            f.move = -1;
            f.ledge = -1;
            if (f.stocks <= 0)
            {
                f.eliminated = true;
                f.koFrame = frame;
                f.finalSmash = false;  // nothing left to unleash it with
                f.ult = 0.0f;
                if (cfg.training && trainFight && trainRestart == 0)
                {  // training fight: "GAME!", then it starts over
                    Emit(EvType.GameSet, default, -1);
                    trainRestart = 150;
                }
            }
        }

        // Fighters KO'd at the same time get side-by-side respawn platforms.
        static readonly float[] kRespawnOffsets = { 0.0f, -130.0f, 130.0f, -260.0f };

        public void RespawnFighter(Fighter f)
        {
            foreach (float dx in kRespawnOffsets)
            {
                f.pos = stageDef.respawn + new Vec2(dx, 0.0f);
                bool taken = false;
                foreach (var o in fighters)
                    taken = taken || (o.id != f.id && o.state == FState.Respawn && MathF.Abs(o.pos.x - f.pos.x) < 100.0f);
                if (!taken) break;
            }
            f.vel = f.kbVel = new Vec2(0, 0);
            f.percent = 0.0f;
            f.shield = kMaxShield;
            f.grounded = false;
            f.groundPlat = -1;
            f.airJumps = f.def.airJumps;
            f.fastFall = f.airDodgeUsed = f.sideSpecialUsed = false;
            Array.Clear(f.moveCooldown, 0, f.moveCooldown.Length);
            Array.Clear(f.moveUses, 0, f.moveUses.Length);
            f.lastHitBy = -1;
            f.facing = 1;
            SetState(f, FState.Respawn);
            Emit(EvType.Respawn, f.pos, f.id);
        }

        public void CheckEnd()
        {
            if (fighters.Count < 2 || cfg.training) return;
            int alive = 0, last = -1;  // teams still standing (a fighter is its own team out of team battles)
            foreach (var f in fighters)
                if (!f.eliminated)
                {
                    bool counted = false;
                    foreach (var g in fighters) counted = counted || (g.id < f.id && !g.eliminated && g.team == f.team);
                    if (!counted) ++alive;
                    last = f.id;
                }
            if (alive <= 1)
            {
                finished = true;
                winner = alive == 1 ? last : -1;
                winnerTeam = alive == 1 ? fighters[last].team : -1;
                finishFrame = frame;
                Emit(EvType.GameSet, default, winner);
            }
        }

        public void UpdateOrb()
        {
            if (!cfg.items || !Started()) return;
            if (!orb.active)
            {
                if (--orbTimer > 0) return;
                foreach (var f in fighters)
                    if (f.state == FState.Attack && f.move == FinalSmash)
                    {
                        orbTimer = 300;  // not during a final smash
                        return;
                    }
                orb = new Orb();
                orb.active = true;
                orb.pos = new Vec2(rng.Range(-350.0f, 350.0f), -520.0f);
                orb.vel = new Vec2(rng.Range(-2.0f, 2.0f), 1.5f);
                orb.hp = 40.0f;
                orb.life = 60 * 18;
                Emit(EvType.OrbSpawn, orb.pos);
                return;
            }
            if (orb.flash > 0) --orb.flash;
            if (--orb.life <= 0)
            {
                orb.active = false;
                orbTimer = rng.Irange(60 * 15, 60 * 30);
                return;
            }
            orb.vel += new Vec2(rng.Range(-0.15f, 0.15f), rng.Range(-0.15f, 0.15f));
            float sp = orb.vel.Len();
            if (sp > 3.2f) orb.vel = orb.vel * Approach(1.0f, 3.2f / sp, 0.05f);
            if (sp < 1.2f) orb.vel = orb.vel.Len() > 0.01f ? orb.vel * (1.2f / sp) : new Vec2(1.2f, 0.0f);
            orb.pos += orb.vel;
            if (orb.pos.x < -550.0f || orb.pos.x > 550.0f)
            {
                orb.pos.x = Clampf(orb.pos.x, -550.0f, 550.0f);
                orb.vel.x = -orb.vel.x;
            }
            if (orb.pos.y < -560.0f || orb.pos.y > -70.0f)
            {
                orb.pos.y = Clampf(orb.pos.y, -560.0f, -70.0f);
                orb.vel.y = -orb.vel.y;
            }
        }

        public void HitOrb(int attacker, float damage, Vec2 from, HitFx fx)
        {
            orb.hp -= damage;
            orb.flash = 8;
            Vec2 d = (orb.pos - from).Norm();
            orb.vel += d * (2.0f + damage * 0.25f);
            if (orb.vel.Len() > 9.0f) orb.vel = orb.vel.Norm() * 9.0f;
            Emit(EvType.OrbHit, orb.pos, attacker, -1, damage, d, fx);
            if (orb.hp > 0.0f) return;
            orb.active = false;
            orbTimer = rng.Irange(60 * 25, 60 * 45);
            Emit(EvType.OrbBreak, orb.pos, attacker);
            if (attacker >= 0) GainUlt(fighters[attacker], kUltMax);  // the orb fills the ultimate meter
        }

        public void GainUlt(Fighter f, float amount)
        {
            if (f.eliminated || f.finalSmash || amount <= 0.0f || (f.state == FState.Attack && f.move == FinalSmash)) return;
            f.ult = Minf(kUltMax, f.ult + amount);
            if (f.ult < kUltMax) return;
            f.finalSmash = true;
            Emit(EvType.UltReady, f.Center(), f.id);
        }

        public void StartFinalSmash(Fighter f)
        {
            StartMove(f, FinalSmash);
            if (f.state != FState.Attack || f.move != FinalSmash) return;  // it did not start: the ultimate is kept
            f.finalSmash = false;
            f.ult = 0.0f;
            freezeTimer = 30;
            freezeOwner = f.id;
            MoveDef fs = f.MoveDef(FinalSmash);
            HitFx fx = fs.hitboxes.Count == 0 ? fs.projectile.hit.fx : fs.hitboxes[0].fx;
            Emit(EvType.FinalSmash, f.Center(), f.id, -1, 0.0f, default, fx);
        }

        public void StartForm(Fighter f, int form)
        {
            if (form < 0 || form >= f.def.forms.Count) return;
            f.form = form;
            f.formTimer = f.def.forms[form].duration;
            f.formMove = f.formTimer > 0 ? -1 : f.move;
            Emit(EvType.Transform, f.Center(), f.id, form, 0.0f, default, f.def.forms[form].fx);
        }

        public void EndForm(Fighter f)
        {
            if (f.form < 0) return;
            f.form = -1;
            f.formTimer = 0;
            f.formMove = -1;
            Emit(EvType.FormEnd, f.Center(), f.id);
        }

        public void ResetPositions()
        {
            projectiles.Clear();
            items.Clear();
            foreach (var f in fighters)
            {
                if (f.eliminated) continue;
                if (f.grabPartner >= 0) ReleaseGrab(f, false);
                Vec2 spawn = stageDef.spawns[f.id % stageDef.spawns.Count];
                f.pos = spawn;
                f.vel = f.kbVel = new Vec2(0, 0);
                f.facing = spawn.x > 0 ? -1 : 1;
                f.percent = 0.0f;
                f.shield = kMaxShield;
                f.hitlag = f.hitstun = 0;
                f.intangible = f.invincible = 0;
                f.grounded = false;
                f.groundPlat = -1;
                f.ledge = -1;
                f.airJumps = f.def.airJumps;
                f.fastFall = f.airDodgeUsed = f.sideSpecialUsed = false;
                f.finalSmash = false;
                f.ult = 0.0f;
                f.captor = -1;
                LoseWeapon(f);
                if (f.form >= 0) EndForm(f);
                SetState(f, FState.Air);
            }
            freezeTimer = 0;
        }

        public List<int> Ranking()
        {
            bool Less(int a, int b)
            {
                Fighter fa = fighters[a];
                Fighter fb = fighters[b];
                if (cfg.teams && winnerTeam >= 0 && (fa.team == winnerTeam) != (fb.team == winnerTeam))
                    return fa.team == winnerTeam;  // the winning team first
                if (fa.eliminated != fb.eliminated) return !fa.eliminated;
                if (fa.eliminated) return fa.koFrame > fb.koFrame;
                if (fa.stocks != fb.stocks) return fa.stocks > fb.stocks;
                return fa.percent < fb.percent;
            }
            // OrderBy is stable, like std::stable_sort.
            return fighters.Select(f => f.id)
                .OrderBy(i => i, Comparer<int>.Create((a, b) => Less(a, b) ? -1 : Less(b, a) ? 1 : 0)).ToList();
        }
    }
}
