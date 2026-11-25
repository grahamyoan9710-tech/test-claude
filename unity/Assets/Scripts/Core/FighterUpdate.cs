using System;
using static ClashHorizon.Core.MathUtil;
using static ClashHorizon.Core.MoveId;
using static ClashHorizon.Core.Fighter;

namespace ClashHorizon.Core
{
    // src/core/fighter_update.cpp — fighter state machine: reads the controller and decides what happens.
    public sealed partial class Match
    {
        const float kAttackStep = 0.5f;  // jabs and tilts: how fast the stick moves the fighter (share of its walk)
        const int kAttackBuffer = 10;    // frames an attack pressed while busy is kept (an M1 right after a jump)

        static bool IsAerial(int m) => m >= NAir && m <= DAir;
        static bool IsSpecialMove(int m) => (m >= NSpecial && m <= DSpecial) || m >= Extra1;

        // Rivals-like flow (the owner, 11/11: an aggressive, fluid game): a normal that hit a fighter can be cut into
        // any action once its last hitbox is over; a missed one, its last quarter. Combos, grabs and specials keep
        // their timing.
        static bool CanCancel(Fighter f, MoveDef m)
        {
            if (f.move > DAir || m.nextMove >= 0 || m.hitboxes.Count == 0 || f.charging) return false;
            int last = 0;
            foreach (var h in m.hitboxes) last = Math.Max(last, h.end);
            if (f.moveFrame < last) return false;
            return f.landedHit || f.moveFrame >= last + (m.duration - last) * 3 / 4;
        }

        static int SmashFor(Fighter f, Vec2 d)
        {
            if (MathF.Abs(d.y) > MathF.Abs(d.x)) return d.y < 0 ? USmash : DSmash;
            if (d.x != 0) f.facing = d.x > 0 ? 1 : -1;
            return FSmash;
        }

        static int TiltFor(Fighter f)
        {
            Vec2 s = f.@in.stick;
            if (s.y < -0.5f && MathF.Abs(s.y) >= MathF.Abs(s.x) * 0.8f) return UTilt;
            if (s.y > 0.5f && MathF.Abs(s.y) >= MathF.Abs(s.x) * 0.8f) return DTilt;
            if (MathF.Abs(s.x) > 0.3f)
            {
                f.facing = s.x > 0 ? 1 : -1;
                return FTilt;
            }
            return Jab;
        }

        static int AerialFor(Fighter f, Vec2 d)
        {
            if (d.Len() < 0.3f) return NAir;
            if (MathF.Abs(d.y) >= MathF.Abs(d.x)) return d.y < 0 ? UAir : DAir;
            return (d.x > 0 ? 1 : -1) == f.facing ? FAir : BAir;
        }

        // Command inputs, read on the stick's history (numpad notation, `face` = 1 facing right): the steps of `seq`
        // in order within the last 18 frames, the last one held now or a frame ago.
        static bool Motion(Fighter f, int[] seq, int face)
        {
            int Mirror(int d) => face > 0 ? d : (d % 3 == 1 ? d + 2 : d % 3 == 0 ? d - 2 : d);
            int n = seq.Length;
            int k = n - 1;
            // The whole motion within 26 frames; its last direction held now, or let go at most 3 frames before the
            // button (a little slack: keyboards and pads alike).
            for (int i = 0; i < 26 && k >= 0; ++i)
            {
                if (f.dirHist[i] == Mirror(seq[k])) --k;
                else if (k == n - 1 && i > 3) return false;
            }
            return k < 0;
        }

        // Tap jump: the stick flicked up (gamepads, the setting on).
        static bool TapJumped(Fighter f) => f.@in.tapJump && f.@in.stick.y < -0.6f && f.FlickY();

        // A + B together: a smash attack (as Smash Bros.' "A+B smash" option).
        static bool AbPressed(Fighter f) =>
            (f.Pressed(static i => i.attack) && f.@in.special) || (f.Pressed(static i => i.special) && f.@in.attack);

        static int SpecialFor(Fighter f)
        {
            Vec2 s = f.@in.stick;
            if (s.y < -0.5f && MathF.Abs(s.y) >= MathF.Abs(s.x) * 0.7f) return USpecial;
            if (s.y > 0.5f && MathF.Abs(s.y) >= MathF.Abs(s.x) * 0.7f) return DSpecial;
            if (MathF.Abs(s.x) > 0.4f)
            {
                if (!f.grounded && f.sideSpecialUsed) return -1;
                f.facing = s.x > 0 ? 1 : -1;
                return SSpecial;
            }
            return NSpecial;
        }

        public void SetState(Fighter f, FState s, int timer = 0)
        {
            f.state = s;
            f.stateFrame = 0;
            f.timer = timer;
            if (s != FState.Attack)
            {
                f.move = -1;
                f.charging = false;
            }
            if (s != FState.Hitstun) f.tumble = false;
        }

        public void StartMove(Fighter f, int id)
        {
            WeaponDef w = f.WeaponDef();
            if (w != null && w.kit)
            {  // the weapon's kit: its M1 on the ground and in the air
                FormDef fd = f.FormDef();
                bool blink = fd != null && fd.m1Blink;  // awake: the blink cuts, kept under ids the kit never uses
                if (blink && f.state == FState.Attack && f.MoveDef(f.move).nextMove == id)
                {
                    // the next blink cut of their combo
                }
                else if (id == FTilt || id == UTilt || id == DTilt || id == DashAttack)
                    id = Jab;
                else if (id == FSmash || id == USmash || id == DSmash)  // the heavy attack: the combo's finisher at once
                    id = Jab3;
                else if (id == FAir || id == BAir || id == UAir || id == DAir)
                    id = NAir;
                if (blink) id = id == Jab ? FTilt : id == Jab2 ? UTilt : id == Jab3 ? DTilt : id == NAir ? FAir : id;
            }
            MoveDef m = f.MoveDef(id);
            if (!m.Exists() || id == f.cancelling) return;
            if (m.cooldown > 0)
            {  // still recharging: nothing happens
                if (f.moveCooldown[id] > 0) return;
                if (++f.moveUses[id] >= Math.Max(1, m.charges))
                {  // its last charge: the cooldown starts
                    f.moveUses[id] = 0;
                    f.moveCooldown[id] = m.cooldown;
                }
            }
            f.state = FState.Attack;
            f.stateFrame = 0;
            f.attackBuffer = 0;
            f.move = id;
            f.moveFrame = 0;
            f.charge = 0;
            f.charging = m.chargeFrame >= 0;
            f.comboQueued = false;
            f.counterTriggered = false;
            f.parried = 0;
            f.willGiven = false;
            f.landedHit = false;
            f.moveLeftGround = false;
            f.moveStartedAir = !f.grounded;
            f.hitList.Clear();
            if (m.intangibleStart == 0) f.intangible = Math.Max(f.intangible, 1);  // covers the start frame too
            if (id == SSpecial && !f.grounded) f.sideSpecialUsed = true;
        }

        // A smash attack at full power without charging it (the command inputs): the screen flashes.
        public void StartChargedSmash(Fighter f, int id)
        {
            StartMove(f, id);
            MoveDef m = f.MoveDef(id);
            if (f.move != id) return;
            if (m.chargeFrame >= 0) f.charge = m.maxCharge;
            f.charging = false;
            Emit(EvType.SuperFlash, f.Center(), f.id);
        }

        public bool TryGroundActions(Fighter f)
        {
            InputState @in = f.@in;
            if (f.will >= kWillMax && f.def.willForm >= 0 && f.form < 0)
            {  // full will: one step back, and it awakens
                f.will = 0;
                StartForm(f, f.def.willForm);
                f.vel.x = -6.0f * (float)f.facing;
                return true;
            }
            // The ultimate: its own button only (U on the keyboard), never the special.
            if (f.finalSmash && freezeTimer == 0 && f.Pressed(static i => i.ultimate))
            {
                StartFinalSmash(f);
                return true;
            }
            if (f.Pressed(static i => i.jump))
            {
                SetState(f, FState.JumpSquat, f.def.jumpSquat);
                return true;
            }
            if (f.Pressed(static i => i.grab) || (@in.shield && f.Pressed(static i => i.attack)))
            {
                StartMove(f, Grab);
                return true;
            }
            if (f.Pressed(static i => i.attack))
            {  // command inputs: a smash at full power
                foreach (int face in new[] { f.facing, -f.facing })
                    if (Motion(f, new[] { 2, 3, 6 }, face))
                    {  // quarter circle forward: forward smash
                        f.facing = face;
                        StartChargedSmash(f, FSmash);
                        return true;
                    }
                if (Motion(f, new[] { 2, 8 }, 1))
                {  // down then up: up smash
                    StartChargedSmash(f, USmash);
                    return true;
                }
            }
            if (AbPressed(f))
            {  // attack + special together: a smash attack
                StartMove(f, SmashFor(f, @in.stick.Len() > 0.3f ? @in.stick : new Vec2(f.facing, 0.0f)));
                return true;
            }
            if (f.Pressed(static i => i.special))
            {
                int sp = SpecialFor(f);
                if (sp >= 0)
                {
                    StartMove(f, sp);
                    return true;
                }
            }
            if (f.CstickPressed())
            {
                StartMove(f, SmashFor(f, f.@in.cstick));
                return true;
            }
            if (f.Pressed(static i => i.smash))
            {
                StartMove(f, SmashFor(f, @in.stick.Len() > 0.3f ? @in.stick : new Vec2(f.facing, 0.0f)));
                return true;
            }
            if (f.attackBuffer > 0)
            {  // pressed now, or a little before while busy
                // A flick (direction + attack together) is a smash attack; holding the direction a
                // little first gives a tilt, even if the fighter already started to dash.
                if (f.state == FState.Run && f.stateFrame > 10 && MathF.Abs(@in.stick.x) > 0.5f)
                {
                    StartMove(f, DashAttack);
                }
                else if (f.FlickX() || (f.FlickY() && @in.stick.y < 0) ||
                         (f.FlickY() && @in.stick.y > 0 && (f.state != FState.Crouch || f.stateFrame <= 3)))
                {
                    StartMove(f, SmashFor(f, @in.stick));
                }
                else
                {
                    StartMove(f, TiltFor(f));
                }
                return true;
            }
            if (@in.shield)
            {
                SetState(f, FState.Shield);
                return true;
            }
            if (TapJumped(f))
            {  // on keys, a slightly longer crouch leaves the time to add the attack (up tilt)
                SetState(f, FState.JumpSquat, f.@in.digital ? Math.Max(f.def.jumpSquat, 5) : f.def.jumpSquat);
                return true;
            }
            return false;
        }

        public bool TryAirActions(Fighter f)
        {
            if (f.finalSmash && freezeTimer == 0 && f.Pressed(static i => i.ultimate))
            {
                StartFinalSmash(f);
                return true;
            }
            if (AbPressed(f))
            {  // attack + special together: an aerial
                StartMove(f, AerialFor(f, f.@in.stick.Len() > 0.3f ? f.@in.stick : new Vec2(f.facing, 0.0f)));
                return true;
            }
            if (f.Pressed(static i => i.special))
            {
                int sp = SpecialFor(f);
                if (sp >= 0)
                {
                    StartMove(f, sp);
                    return true;
                }
            }
            if (f.CstickPressed())
            {
                StartMove(f, AerialFor(f, f.@in.cstick));
                return true;
            }
            if (f.Pressed(static i => i.smash))
            {
                StartMove(f, AerialFor(f, f.@in.stick.Len() > 0.3f ? f.@in.stick : new Vec2(f.facing, 0.0f)));
                return true;
            }
            if (f.attackBuffer > 0)
            {  // pressed now, or a little before (in the jump's crouch)
                StartMove(f, AerialFor(f, f.@in.stick));
                return true;
            }
            if ((f.Pressed(static i => i.jump) || TapJumped(f)) && f.airJumps > 0)
            {
                AirJump(f);
                return true;
            }
            if ((f.Pressed(static i => i.shield) || f.Pressed(static i => i.grab)) && !f.airDodgeUsed)
            {
                SetState(f, FState.AirDodge, 34);
                f.airDodgeUsed = true;
                f.fastFall = false;
                if (f.@in.stick.Len() > 0.3f)
                {
                    f.vel = f.@in.stick.Norm() * 10.0f;
                    f.actionDir = 1;
                }
                else
                {
                    f.vel *= 0.5f;
                    f.actionDir = 0;
                }
                return true;
            }
            return false;
        }

        public void AirControl(Fighter f, float mul = 1.0f)
        {
            CharacterDef d = f.def;
            float sx = f.@in.stick.x;
            if (MathF.Abs(sx) > 0.15f)
                f.vel.x = Approach(f.vel.x, sx * d.airSpeed * Maxf(mul, 0.3f), d.airAccel * mul);
            else
                f.vel.x = Approach(f.vel.x, 0.0f, d.airFriction);
            if (!f.fastFall && f.vel.y > -2.0f && f.@in.stick.y > 0.7f && f.FlickY()) f.fastFall = true;
        }

        public void ApplyGravity(Fighter f, float mul = 1.0f)
        {
            if (mul <= 0.0f) return;
            f.vel.y += f.def.gravity * mul;
            if (f.fastFall && f.vel.y > 0.0f)
                f.vel.y = f.def.fastFall;
            else if (f.vel.y > f.def.maxFall)
                f.vel.y = Approach(f.vel.y, f.def.maxFall, 1.0f);
        }

        public void GroundFriction(Fighter f, float mul = 1.0f)
        {
            f.vel.x = Approach(f.vel.x, 0.0f, f.def.friction * mul);
        }

        public void StartJump(Fighter f)
        {
            CharacterDef d = f.def;
            f.vel.y = -(f.@in.jump || (f.@in.tapJump && f.@in.stick.y < -0.6f) ? d.jumpVel : d.shortHopVel);
            f.vel.x = Clampf(f.vel.x + f.@in.stick.x * 1.5f, -d.airSpeed, d.airSpeed);
            f.grounded = false;
            f.groundPlat = -1;
            f.fastFall = false;
            SetState(f, FState.Air);
            Emit(EvType.Jump, f.pos, f.id);
        }

        public void AirJump(Fighter f)
        {
            CharacterDef d = f.def;
            f.vel.y = -d.airJumpVel;
            f.vel.x = f.@in.stick.x * d.airSpeed;
            f.airJumps--;
            f.fastFall = false;
            f.airJumpFrame = frame;
            SetState(f, FState.Air);
            Emit(EvType.AirJump, f.pos, f.id);
        }

        public void UpdateFighter(Fighter f)
        {
            if (f.eliminated) return;
            if (f.state == FState.Dead)
            {
                if (--f.respawnTimer <= 0 && f.stocks > 0) RespawnFighter(f);
                return;
            }
            if (f.flash > 0) f.flash--;
            if (f.hitlag > 0)
            {
                f.hitlag--;
                return;
            }
            f.stateFrame++;
            if (f.form >= 0)
            {  // transformations wear off (never in the middle of a move they change)
                bool moveLong = f.formTimer <= 0 && f.formMove >= 0;
                if (!moveLong && f.formTimer > 0) --f.formTimer;
                bool over = moveLong ? !(f.state == FState.Attack && f.move == f.formMove) : f.formTimer <= 0;
                if (over && f.state != FState.Attack) EndForm(f);
            }
            if (f.intangible > 0) f.intangible--;
            if (f.invincible > 0) f.invincible--;
            for (int c = 0; c < f.moveCooldown.Length; ++c)
                if (f.moveCooldown[c] > 0) --f.moveCooldown[c];
            if (f.ledgeCooldown > 0) f.ledgeCooldown--;
            if (f.dropThrough > 0) f.dropThrough--;
            if (f.techWindow > 0) f.techWindow--;
            if (f.techLockout > 0) f.techLockout--;
            if (f.Pressed(static i => i.attack)) f.attackBuffer = kAttackBuffer;
            else if (f.attackBuffer > 0) f.attackBuffer--;
            if (f.Pressed(static i => i.shield) && f.techLockout == 0)
            {
                f.techWindow = 20;
                f.techLockout = 40;
            }
            if (f.state != FState.Shield && f.state != FState.ShieldStun)
                f.shield = Minf(kMaxShield, f.shield + 0.08f);

            CharacterDef d = f.def;
            InputState @in = f.@in;
            switch (f.state)
            {
                case FState.Idle:
                    GroundFriction(f);
                    if (!f.grounded) { SetState(f, FState.Air); break; }
                    if (TryGroundActions(f)) break;
                    if (@in.stick.y > 0.6f)
                    {
                        if (f.groundPlat >= 0 && !platforms[f.groundPlat].solid && f.FlickY())
                        {
                            f.dropThrough = 12;
                            f.grounded = false;
                            f.groundPlat = -1;
                            f.pos.y += 2.0f;
                            SetState(f, FState.Air);
                        }
                        else
                        {
                            SetState(f, FState.Crouch);
                        }
                        break;
                    }
                    if (MathF.Abs(@in.stick.x) > 0.25f)
                    {
                        f.facing = @in.stick.x > 0 ? 1 : -1;
                        if (f.FlickX())
                        {
                            SetState(f, FState.Run);
                            Emit(EvType.Dash, f.pos, f.id);
                        }
                        else
                        {
                            SetState(f, FState.Walk);
                        }
                    }
                    break;
                case FState.Walk:
                    if (!f.grounded) { SetState(f, FState.Air); break; }
                    if (TryGroundActions(f)) break;
                    if (@in.stick.y > 0.6f) { SetState(f, FState.Crouch); break; }
                    if (MathF.Abs(@in.stick.x) < 0.2f) { SetState(f, FState.Idle); break; }
                    f.facing = @in.stick.x > 0 ? 1 : -1;
                    if (f.FlickX()) { SetState(f, FState.Run); Emit(EvType.Dash, f.pos, f.id); break; }
                    f.vel.x = Approach(f.vel.x, @in.stick.x * d.walkSpeed, d.groundAccel);
                    break;
                case FState.Run:
                    if (!f.grounded) { SetState(f, FState.Air); break; }
                    if (TryGroundActions(f)) break;
                    if (@in.stick.y > 0.6f) { SetState(f, FState.Crouch); break; }
                    if (MathF.Abs(@in.stick.x) < 0.2f) { SetState(f, FState.Idle); break; }
                    if ((@in.stick.x > 0 ? 1 : -1) != f.facing)
                    {
                        f.facing = -f.facing;
                        f.vel.x *= 0.25f;
                        Emit(EvType.Dash, f.pos, f.id);
                    }
                    f.vel.x = Approach(f.vel.x, f.facing * d.runSpeed, d.groundAccel);
                    break;
                case FState.Crouch:
                    GroundFriction(f);
                    if (!f.grounded) { SetState(f, FState.Air); break; }
                    if (f.groundPlat >= 0 && !platforms[f.groundPlat].solid && f.FlickY() && @in.stick.y > 0)
                    {
                        f.dropThrough = 12;
                        f.grounded = false;
                        f.groundPlat = -1;
                        f.pos.y += 2.0f;
                        SetState(f, FState.Air);
                        break;
                    }
                    if (TryGroundActions(f)) break;
                    if (@in.stick.y < 0.5f) SetState(f, FState.Idle);
                    break;
                case FState.JumpSquat:
                    GroundFriction(f, 0.5f);
                    // As in Smash Bros., the jump's crouch can still turn into an up attack: up + special is the up
                    // special, up + attack the up smash (on keys, the attack a little after the up key: the up tilt),
                    // the smash key or the smash stick up the up smash. So up jumps and attacks upward on the same
                    // direction.
                    if (@in.stick.y < -0.5f && f.Pressed(static i => i.special))
                    {
                        StartMove(f, USpecial);
                        break;
                    }
                    // A weapon's M1 has no up attack: its press waits for the jump and comes out in the air.
                    if (@in.stick.y < -0.5f &&
                        (f.Pressed(static i => i.smash) ||
                         (f.Pressed(static i => i.attack) && !(f.WeaponDef() != null && f.WeaponDef().kit))))
                    {
                        StartMove(f, @in.digital && !f.Pressed(static i => i.smash) && !@in.special ? UTilt : USmash);
                        break;
                    }
                    if (f.CstickPressed() && @in.cstick.y < -0.5f && MathF.Abs(@in.cstick.y) > MathF.Abs(@in.cstick.x))
                    {
                        StartMove(f, USmash);
                        break;
                    }
                    if (f.stateFrame >= f.timer) StartJump(f);
                    break;
                case FState.Air:
                    AirControl(f);
                    ApplyGravity(f);
                    TryAirActions(f);
                    break;
                case FState.Landing:
                    GroundFriction(f);
                    if (!f.grounded) { AirControl(f); ApplyGravity(f); }
                    if (f.stateFrame >= f.timer) SetState(f, f.grounded ? FState.Idle : FState.Air);
                    break;
                case FState.Attack:
                    UpdateMove(f);
                    break;
                case FState.Shield:
                    GroundFriction(f);
                    if (!f.grounded) { SetState(f, FState.Air); break; }
                    f.shield -= 0.13f;
                    if (f.shield <= 0.0f)
                    {
                        f.shield = 0.0f;
                        SetState(f, FState.ShieldBreak, 220);
                        f.vel = new Vec2(0, -9);
                        f.grounded = false;
                        Emit(EvType.ShieldBreak, f.Center(), f.id);
                        break;
                    }
                    if (f.Pressed(static i => i.jump) || TapJumped(f))
                    {  // out of shield
                        SetState(f, FState.JumpSquat, d.jumpSquat);
                        break;
                    }
                    if (f.Pressed(static i => i.attack) || f.Pressed(static i => i.grab)) { StartMove(f, Grab); break; }
                    if (f.FlickX())
                    {
                        SetState(f, FState.Roll, 28);
                        f.actionDir = @in.stick.x > 0 ? 1 : -1;
                        break;
                    }
                    if (f.FlickY() && @in.stick.y > 0) { SetState(f, FState.SpotDodge, 24); break; }
                    if (!@in.shield) SetState(f, FState.Landing, 5);
                    break;
                case FState.ShieldStun:
                    GroundFriction(f, 0.5f);
                    if (f.stateFrame >= f.timer) SetState(f, @in.shield ? FState.Shield : FState.Idle);
                    break;
                case FState.Roll:
                    if (f.stateFrame == 3) f.intangible = 15;
                    if (f.stateFrame >= 3 && f.stateFrame < 20)
                        f.vel.x = f.actionDir * 7.0f;
                    else
                        GroundFriction(f, 2.0f);
                    if (f.stateFrame >= f.timer)
                    {
                        f.facing = -f.actionDir;
                        SetState(f, FState.Idle);
                    }
                    break;
                case FState.SpotDodge:
                    GroundFriction(f, 2.0f);
                    if (f.stateFrame == 2) f.intangible = 15;
                    if (f.stateFrame >= f.timer) SetState(f, FState.Idle);
                    break;
                case FState.AirDodge:
                    if (f.stateFrame == 2) f.intangible = 18;
                    if (f.actionDir == 1 && f.stateFrame < 20)
                    {
                        f.vel *= 0.9f;
                    }
                    else
                    {
                        AirControl(f, 0.5f);
                        ApplyGravity(f);
                    }
                    if (f.stateFrame >= f.timer) SetState(f, FState.Air);
                    break;
                case FState.Hitstun:
                    ApplyGravity(f);
                    if (f.grounded) GroundFriction(f);
                    if (f.tumble) f.spin += 0.25f * f.facing;
                    if (--f.hitstun <= 0)
                    {
                        bool tumbled = f.tumble;
                        SetState(f, f.grounded ? FState.Idle : FState.Air);
                        f.tumble = tumbled && !f.grounded;
                    }
                    break;
                case FState.Knockdown:
                    GroundFriction(f, 1.5f);
                    if (!f.grounded)
                    {  // slid off an edge: free to act again
                        SetState(f, FState.Air);
                        ApplyGravity(f);
                        break;
                    }
                    if (f.stateFrame > 60)
                    {
                        SetState(f, FState.GetUp, 22);
                        f.intangible = 22;
                    }
                    else if (f.stateFrame > 10)
                    {
                        if (f.Pressed(static i => i.attack))
                        {
                            StartMove(f, GetupAttack);
                        }
                        else if (MathF.Abs(@in.stick.x) > 0.6f)
                        {
                            SetState(f, FState.Roll, 28);
                            f.actionDir = @in.stick.x > 0 ? 1 : -1;
                        }
                        else if (f.Pressed(static i => i.jump) || f.Pressed(static i => i.shield) || @in.stick.y < -0.6f)
                        {
                            SetState(f, FState.GetUp, 22);
                            f.intangible = 22;
                        }
                    }
                    break;
                case FState.GetUp:
                    GroundFriction(f);
                    if (f.stateFrame >= f.timer) SetState(f, FState.Idle);
                    break;
                case FState.Helpless:
                    AirControl(f, 0.7f);
                    ApplyGravity(f);
                    break;
                case FState.LedgeHang:
                case FState.LedgeClimb:
                    UpdateLedge(f);
                    return;  // ledge positions are set directly
                case FState.Holding:
                {
                    GroundFriction(f);
                    Fighter v = f.grabPartner >= 0 ? fighters[f.grabPartner] : null;
                    if (v == null || v.state != FState.Grabbed)
                    {
                        f.grabPartner = -1;
                        SetState(f, FState.Idle);
                        break;
                    }
                    if (f.stateFrame > 4)
                    {
                        Vec2 s = @in.stick;
                        if (f.Pressed(static i => i.attack))
                        {
                            StartMove(f, Pummel);
                        }
                        else if (s.Len() > 0.6f)
                        {
                            int id;
                            if (MathF.Abs(s.y) > MathF.Abs(s.x)) id = s.y < 0 ? UThrow : DThrow;
                            else id = (s.x > 0 ? 1 : -1) == f.facing ? FThrow : BThrow;
                            StartMove(f, id);
                        }
                        else if (f.stateFrame > 200)
                        {
                            ReleaseGrab(f, true);
                        }
                    }
                    break;
                }
                case FState.Grabbed:
                    UpdateGrabbed(f);
                    return;
                case FState.Captured:
                    UpdateCaptured(f);
                    return;
                case FState.ShieldBreak:
                    if (f.grounded)
                    {
                        GroundFriction(f);
                        if (--f.timer <= 0)
                        {
                            f.shield = kMaxShield * 0.6f;
                            SetState(f, FState.Idle);
                        }
                    }
                    else
                    {
                        ApplyGravity(f);
                    }
                    break;
                case FState.Respawn:
                    f.pos.y = stageDef.respawn.y;  // x keeps the side-by-side offset from RespawnFighter
                    f.vel = new Vec2(0, 0);
                    if (f.stateFrame > 300 ||
                        (f.stateFrame > 30 && (@in.stick.Len() > 0.4f || @in.attack || @in.special || @in.jump || @in.shield)))
                    {
                        SetState(f, FState.Air);
                        f.invincible = 120;
                    }
                    return;
                case FState.Dead:
                    return;
            }
            MoveFighter(f);
        }

        public void UpdateMove(Fighter f)
        {
            // Attack and special a frame or two apart still make an A+B smash: the move just started becomes the smash.
            if (f.grounded && !f.moveStartedAir && f.moveFrame <= 3 && AbPressed(f))
            {
                bool normal = f.move == Jab || f.move == Jab2 || f.move == Jab3 || f.move == FTilt || f.move == UTilt ||
                              f.move == DTilt;
                bool special = f.move == NSpecial || f.move == SSpecial || f.move == USpecial || f.move == DSpecial;
                if (normal || special)
                {
                    StartMove(f, SmashFor(f, f.@in.stick.Len() > 0.3f ? f.@in.stick : new Vec2(f.facing, 0.0f)));
                    return;
                }
            }
            MoveDef m = f.MoveDef(f.move);
            InputState @in = f.@in;
            if (f.charging && f.moveFrame == m.chargeFrame)
            {
                // a draw (the Rengaine, after a held jab) is held with the attack button; a special with its own
                bool held = IsSpecialMove(f.move) && m.releaseMove < 0
                                ? @in.special
                                : (@in.attack || @in.smash || @in.cstick.Len() > 0.5f);
                if (held && f.charge < m.maxCharge)
                {
                    f.charge++;
                    if (m.perfectMove >= 0 && f.charge == m.perfectFrom) Emit(EvType.Glint, f.Center(), f.id, f.move);
                    if (f.grounded) GroundFriction(f);
                    else ApplyGravity(f, 0.5f);
                    return;
                }
                f.charging = false;
                bool perfect = f.charge >= m.perfectFrom && f.charge <= m.perfectTo;
                if (m.perfectMove >= 0 && perfect) { StartMove(f, m.perfectMove); return; }
                if (m.releaseMove >= 0) { StartMove(f, m.releaseMove); return; }
                if (m.parries > 0 && f.counterTriggered) f.moveFrame = m.counterEnd - 1;  // released after a parry: the riposte
            }
            f.moveFrame++;
            int t = f.moveFrame;
            if (!f.grounded) f.moveLeftGround = true;

            if (m.counterStart < 0 && !m.isGrab && m.throwFrame < 0)
            {
                foreach (var h in m.hitboxes)
                    if (h.start == t)
                    {
                        Emit(EvType.Swing, f.Center(), f.id, -1, h.damage, default, h.fx);
                        break;
                    }
            }
            FormDef fd = f.FormDef();
            bool m1 = fd != null && fd.m1Blink ? (f.move >= FTilt && f.move <= DTilt) || f.move == FAir  // the blink cuts
                                               : f.move <= Jab3 || f.move == NAir;
            if (fd != null && fd.m1Waves && m1 && m.hitboxes.Count > 0 && m.hitboxes[0].start == t)
            {
                ProjectileDef w = fd.m1Wave;  // a struct: a copy, the roster's stays as it is
                if (f.weapon == 1)
                {  // B: great crescents of fire
                    w.radius *= 1.5f;
                    w.hit.radius *= 1.5f;
                    w.hit.damage *= 1.3f;
                }
                SpawnProjectiles(f, w);
            }
            foreach (var e in m.vel)
            {
                if (e.frame != t || (e.airOnly && f.grounded)) continue;
                var v = new Vec2(e.v.x * f.facing + @in.stick.x * e.stickX, e.v.y);
                switch (e.mode)
                {
                    case VelMode.Set: f.vel = v; break;
                    case VelMode.SetX: f.vel.x = v.x; break;
                    case VelMode.SetY: f.vel.y = v.y; break;
                    case VelMode.Add: f.vel += v; break;
                    case VelMode.Scale: f.vel.x *= e.v.x; f.vel.y *= e.v.y; break;
                    case VelMode.StickDir:
                    {
                        Vec2 dir = @in.stick.Len() > 0.3f ? @in.stick.Norm() : new Vec2(0, -1);
                        if (MathF.Abs(dir.x) > 0.2f) f.facing = dir.x > 0 ? 1 : -1;
                        f.vel = dir * e.v.x;
                        break;
                    }
                }
                if (f.vel.y < -0.5f) f.grounded = false;
            }
            if (m.projectileFrame >= 0 && t >= m.projectileFrame)
            {
                int k = t - m.projectileFrame;
                if (k == 0 || (m.projectileEvery > 0 && k % m.projectileEvery == 0 && k / m.projectileEvery <= m.projectileRepeat))
                    SpawnProjectiles(f, m.projectile);
            }
            foreach (var sh in m.shots)
                if (sh.frame == t) SpawnProjectiles(f, sh.def);
            if (m.formIndex >= 0 && m.formFrame == t) StartForm(f, m.formIndex);
            if (m.intangibleStart >= 0 && t == Math.Max(1, m.intangibleStart))  // t starts at 1
                f.intangible = m.intangibleEnd - t;
            if (m.teleportFrame == t)
            {
                Vec2 from = f.pos;
                int target = m.teleportToTarget ? NearestOpponent(f) : -1;
                if (target >= 0)
                {  // right in front of the nearest opponent, on the side it came from, facing it
                    Fighter o = fighters[target];
                    int side = f.pos.x <= o.pos.x ? -1 : 1;
                    f.pos = new Vec2(o.pos.x + side * (o.Width() * 0.5f + f.Width() * 0.5f + 14.0f), o.pos.y);
                    f.facing = -side;
                }
                else
                {
                    Vec2 dir = m.teleportForward || m.teleportToTarget ? new Vec2(f.facing, 0.0f)
                             : @in.stick.Len() > 0.3f                ? @in.stick.Norm()
                                                                     : new Vec2(0, -1);
                    if (MathF.Abs(dir.x) > 0.2f) f.facing = dir.x > 0 ? 1 : -1;
                    f.pos += dir * m.teleportDist;
                }
                if (m.turnAfterTeleport) f.facing = -f.facing;
                f.grounded = false;
                f.vel = new Vec2(0, 0);
                foreach (var p in platforms)
                {  // never end up inside the stage: out on top, unless coming from below it
                    if (p.solid && p.rect.Contains(new Vec2(f.pos.x, f.pos.y - 1.0f)))
                    {
                        if (from.y <= p.rect.Bottom() || f.pos.y - p.rect.Top() < p.rect.h * 0.6f) f.pos.y = p.rect.Top();
                        else f.pos.y = p.rect.Bottom() + f.Height() + 1.0f;
                    }
                }
            }
            if (m.throwFrame == t && f.grabPartner >= 0)
            {
                Fighter v = fighters[f.grabPartner];
                HitboxDef h = m.hitboxes[0];
                if (m.pummel)
                {
                    float dmg = h.damage * kDamageScale;
                    v.percent += dmg;
                    v.flash = 6;
                    f.damageDealt += dmg;
                    GainUlt(f, dmg * kUltPerDamageDealt);
                    GainUlt(v, dmg * kUltPerDamageTaken);
                    f.hitlag = v.hitlag = 3;
                    Emit(EvType.Hit, v.Center(), f.id, v.id, dmg, new Vec2(f.facing, 0));
                }
                else
                {
                    f.grabPartner = -1;
                    v.grabPartner = -1;
                    SetState(v, FState.Air);
                    Emit(EvType.Throw, v.Center(), f.id, v.id, h.damage);
                    ApplyHit(f, v, h, v.Center(), f.facing, kDamageScale, true);
                }
            }
            if (m.nextMove >= 0)
            {
                bool p = m.comboOnSpecial ? f.Pressed(static i => i.special) : f.Pressed(static i => i.attack);
                if (p && t >= m.comboStart && t <= m.comboEnd) f.comboQueued = true;
                if (f.comboQueued && t >= m.comboAt)
                {
                    StartMove(f, m.nextMove);
                    return;
                }
            }
            if (m.loopFrame == t && !f.grounded) f.moveFrame--;
            if (CanCancel(f, m))
            {
                int was = f.move;
                f.cancelling = was;
                if (f.grounded) TryGroundActions(f);
                else TryAirActions(f);
                f.cancelling = -1;
                if (f.state != FState.Attack || f.move != was) return;
            }

            if (f.grounded)
            {
                GroundFriction(f, m.frictionMul);
                // Jabs and tilts: the stick shuffles the fighter forward or back while it hits (a little, without
                // turning around), but never off the edge of its platform.
                if (f.move <= DTilt && MathF.Abs(@in.stick.x) > 0.3f && f.groundPlat >= 0)
                {
                    float want = Clampf(@in.stick.x, -1.0f, 1.0f) * f.def.walkSpeed * kAttackStep;
                    float v = f.vel.x;
                    if (want > 0.0f ? v < want : v > want)
                        v = Approach(v, want, f.def.groundAccel * 0.5f + f.def.friction * m.frictionMul);
                    Rect r = platforms[f.groundPlat].rect;  // pushed towards an edge: stops right before it
                    if (want > 0.0f) v = Minf(v, Maxf(0.0f, r.Right() - 2.0f - f.pos.x));
                    else v = Maxf(v, Minf(0.0f, r.Left() + 2.0f - f.pos.x));
                    f.vel.x = v;
                }
            }
            else
            {
                if (m.allowDrift) AirControl(f, (IsAerial(f.move) || f.moveStartedAir) ? 1.0f : 0.6f);
                ApplyGravity(f, m.gravityMul);
            }

            if (f.moveFrame >= m.duration)
            {
                if (m.holdMove >= 0 && f.grounded && f.@in.attack && f.attackHeld > m.duration)
                {
                    StartMove(f, m.holdMove);
                    return;
                }
                if (f.move == Pummel || ((f.move >= FThrow && f.move <= DThrow) && f.grabPartner >= 0))
                {
                    SetState(f, FState.Holding);
                    f.stateFrame = 7;
                    return;
                }
                if (m.helplessAfter && !f.grounded) SetState(f, FState.Helpless);
                else SetState(f, f.grounded ? FState.Idle : FState.Air);
            }
        }

        public void UpdateLedge(Fighter f)
        {
            Vec2 L = LedgePoint(f.ledge);
            int toStage = (f.ledge % 2 == 0) ? 1 : -1;
            InputState @in = f.@in;
            if (f.state == FState.LedgeHang)
            {
                f.facing = toStage;
                f.pos = new Vec2(L.x - toStage * (f.Width() * 0.5f + 2.0f), L.y + f.Height() * 0.75f);
                f.vel = f.kbVel = new Vec2(0, 0);
                if (f.stateFrame < 8) return;
                float toward = @in.stick.x * toStage;
                int action = -1;
                if (f.Pressed(static i => i.jump) || (@in.stick.y < -0.6f && f.FlickY()))
                {
                    SetState(f, FState.Air);
                    f.pos = new Vec2(L.x - toStage * f.Width() * 0.25f, L.y - 2.0f);
                    f.vel = new Vec2(toStage * 2.0f, -f.def.jumpVel);
                    f.intangible = 6;
                    f.ledgeCooldown = 30;
                    f.ledge = -1;
                    Emit(EvType.Jump, f.pos, f.id);
                    return;
                }
                else if (f.Pressed(static i => i.attack) || f.Pressed(static i => i.special))
                {
                    action = 1;
                }
                else if (f.Pressed(static i => i.shield))
                {
                    action = 2;
                }
                else if (toward > 0.5f || @in.stick.y < -0.6f)
                {
                    action = 0;
                }
                else if (toward < -0.5f || @in.stick.y > 0.6f || f.stateFrame > 360)
                {
                    SetState(f, FState.Air);
                    f.pos.x -= toStage * 6.0f;
                    f.ledgeCooldown = 40;
                    f.ledge = -1;
                    return;
                }
                if (action >= 0)
                {
                    f.ledgeFrom = f.pos;
                    f.ledgeTo = new Vec2(L.x + toStage * f.Width() * 0.7f, L.y);
                    f.ledgeAction = action;
                    SetState(f, FState.LedgeClimb);
                    f.intangible = 24;
                }
                return;
            }
            // Climbing up
            float t = Smoothstep(f.stateFrame / 18.0f);
            f.pos = Lerpv(f.ledgeFrom, f.ledgeTo, t);
            f.pos.y -= MathF.Sin(t * kPi) * 20.0f;
            if (f.stateFrame >= 18)
            {
                f.pos = f.ledgeTo;
                f.grounded = true;
                f.groundPlat = f.ledge / 2;
                f.ledge = -1;
                f.ledgeCooldown = 20;
                f.facing = toStage;
                if (f.ledgeAction == 1)
                {
                    StartMove(f, LedgeAttack);
                    if (f.state != FState.Attack) SetState(f, FState.Idle);  // no ledge attack (or recharging): not stuck climbing
                }
                else if (f.ledgeAction == 2)
                {
                    SetState(f, FState.Roll, 28);
                    f.actionDir = toStage;
                }
                else
                {
                    SetState(f, FState.Idle);
                }
            }
        }

        public void UpdateGrabbed(Fighter f)
        {
            Fighter g = f.grabPartner >= 0 ? fighters[f.grabPartner] : null;
            bool held = g != null && g.grabPartner == f.id &&
                        (g.state == FState.Holding || (g.state == FState.Attack && g.move >= Pummel && g.move <= DThrow));
            if (!held)
            {
                f.grabPartner = -1;
                SetState(f, f.grounded ? FState.Idle : FState.Air);
                return;
            }
            f.pos = new Vec2(g.pos.x + g.facing * (g.Width() * 0.5f + f.Width() * 0.5f - 4.0f), g.pos.y);
            f.vel = f.kbVel = new Vec2(0, 0);
            f.grounded = g.grounded;
            f.groundPlat = g.groundPlat;
            f.facing = -g.facing;
            bool mash = f.Pressed(static i => i.attack) || f.Pressed(static i => i.special) || f.Pressed(static i => i.jump) ||
                        f.Pressed(static i => i.shield) || f.FlickX() || f.FlickY();
            f.escapeTimer -= mash ? 6 : 1;
            if (f.escapeTimer <= 0 && g.state == FState.Holding) ReleaseGrab(f, true);
        }

        // Held by an opponent's cinematic ultimate: pulled to the hold point (or frozen where it was caught).
        public void UpdateCaptured(Fighter f)
        {
            Fighter c = f.captor >= 0 && f.captor < fighters.Count ? fighters[f.captor] : null;
            MoveDef m = c != null ? c.CurrentMove() : null;
            if (c == null || !c.Active() || m == null || c.move != FinalSmash)
            {
                f.captor = -1;
                SetState(f, f.grounded ? FState.Idle : FState.Air);
                return;
            }
            f.vel = f.kbVel = new Vec2(0, 0);
            if (m.holdInPlace)
            {
                f.pos = f.capturePos;
            }
            else
            {
                Vec2 hold = c.Center() + new Vec2(m.holdAt.x * c.facing, m.holdAt.y);
                Vec2 feet = hold + new Vec2(0.0f, f.Height() * 0.5f);
                f.pos += (feet - f.pos) * 0.35f;
                f.facing = c.pos.x > f.pos.x ? 1 : -1;
            }
            f.grounded = false;
            f.groundPlat = -1;
        }

        public void ReleaseGrab(Fighter f, bool escaped)
        {
            if (f.grabPartner < 0) return;
            Fighter o = fighters[f.grabPartner];
            Fighter victim = (f.state == FState.Grabbed) ? f : o;
            Fighter grabber = (f.state == FState.Grabbed) ? o : f;
            victim.grabPartner = grabber.grabPartner = -1;
            if (victim.state == FState.Grabbed)
            {
                SetState(victim, victim.grounded ? FState.Landing : FState.Air, 12);
                victim.vel.x = grabber.facing * 6.0f;
            }
            if (grabber.state == FState.Holding || grabber.state == FState.Attack)
            {
                SetState(grabber, grabber.grounded ? FState.Landing : FState.Air, escaped ? 18 : 6);
                grabber.vel.x = -grabber.facing * 5.0f;
            }
        }
    }
}
