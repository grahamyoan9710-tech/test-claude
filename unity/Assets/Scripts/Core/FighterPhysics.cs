using System;
using static ClashHorizon.Core.MathUtil;
using static ClashHorizon.Core.MoveId;

namespace ClashHorizon.Core
{
    // src/core/fighter_physics.cpp — movement, platform collisions, landings and ledge grabs.
    public sealed partial class Match
    {
        const float kKnockbackDecay = 0.26f;

        public void MoveFighter(Fighter f)
        {
            switch (f.state)
            {
                case FState.LedgeHang:
                case FState.LedgeClimb:
                case FState.Grabbed:
                case FState.Respawn:
                case FState.Dead:
                    return;
                default:
                    break;
            }
            float w = f.Width(), h = f.Height();

            float kl = f.kbVel.Len();
            if (kl > 0.0f) f.kbVel = f.kbVel * (Maxf(0.0f, kl - kKnockbackDecay) / kl);

            if (f.grounded && f.groundPlat >= 0) f.pos += platforms[f.groundPlat].delta;
            Vec2 total = f.vel + f.kbVel;
            float prevFeet = f.pos.y;

            // Horizontal movement + walls (sides of solid platforms)
            f.pos.x += total.x;
            foreach (var p in platforms)
            {
                if (!p.solid) continue;
                var body = new Rect(f.pos.x - w * 0.5f, f.pos.y - h, w, h);
                if (!body.Overlaps(p.rect) || f.pos.y <= p.rect.Top() + 1.0f) continue;
                f.pos.x = (f.pos.x < p.rect.Center().x) ? p.rect.Left() - w * 0.5f - 0.01f
                                                        : p.rect.Right() + w * 0.5f + 0.01f;
                if (f.state == FState.Hitstun && MathF.Abs(f.kbVel.x) > 6.0f) f.kbVel.x *= -0.5f;
                else f.kbVel.x = 0.0f;
                f.vel.x = 0.0f;
            }

            // Vertical movement
            if (f.grounded)
            {
                Platform p = platforms[f.groundPlat];
                if (total.y < -0.5f)
                {
                    f.grounded = false;
                    f.groundPlat = -1;
                    f.pos.y += total.y;
                }
                else if (f.pos.x < p.rect.Left() || f.pos.x > p.rect.Right())
                {
                    f.grounded = false;  // walked off the edge
                    f.groundPlat = -1;
                    f.vel.y = 0.0f;
                }
                else
                {
                    f.pos.y = p.rect.Top();
                    f.vel.y = 0.0f;
                    if (f.kbVel.y > 0.0f) f.kbVel.y = 0.0f;
                }
            }
            else
            {
                float newY = f.pos.y + total.y;
                int landOn = -1;
                float landTop = 0.0f;
                if (total.y >= 0.0f)
                {
                    for (int i = 0; i < platforms.Count; ++i)
                    {
                        Platform p = platforms[i];
                        if (f.pos.x < p.rect.Left() || f.pos.x > p.rect.Right()) continue;
                        if (!p.solid && f.dropThrough > 0) continue;
                        float top = p.rect.Top();
                        float prevTop = top - p.delta.y;
                        if (prevFeet <= prevTop + 0.5f && newY >= top && (landOn < 0 || top < landTop))
                        {
                            landOn = i;
                            landTop = top;
                        }
                    }
                }
                if (landOn >= 0)
                {
                    f.pos.y = landTop;
                    f.groundPlat = landOn;
                    f.vel.y = 0.0f;
                    OnLand(f, total.y);
                }
                else
                {
                    f.pos.y = newY;
                    if (total.y < 0.0f)
                    {
                        foreach (var p in platforms)
                        {
                            if (!p.solid) continue;
                            if (f.pos.x < p.rect.Left() - w * 0.4f || f.pos.x > p.rect.Right() + w * 0.4f) continue;
                            float headPrev = prevFeet - h, headNew = f.pos.y - h;
                            if (headPrev >= p.rect.Bottom() - 1.0f && headNew < p.rect.Bottom())
                            {
                                f.pos.y = p.rect.Bottom() + h;
                                f.vel.y = 0.0f;
                                if (f.kbVel.y < 0.0f) f.kbVel.y *= -0.4f;
                            }
                        }
                    }
                    // Diagonal entry into a solid block: push out sideways.
                    foreach (var p in platforms)
                    {
                        if (!p.solid) continue;
                        var body = new Rect(f.pos.x - w * 0.5f, f.pos.y - h, w, h);
                        if (!body.Overlaps(p.rect) || f.pos.y <= p.rect.Top() + 1.0f) continue;
                        f.pos.x = (f.pos.x < p.rect.Center().x) ? p.rect.Left() - w * 0.5f - 0.01f
                                                                : p.rect.Right() + w * 0.5f + 0.01f;
                        f.vel.x = 0.0f;
                        f.kbVel.x = 0.0f;
                    }
                }
            }
            if (!f.grounded) TryLedgeGrab(f);
        }

        public void OnLand(Fighter f, float impact)
        {
            f.grounded = true;
            f.fastFall = false;
            f.airJumps = f.def.airJumps;
            f.airDodgeUsed = false;
            f.sideSpecialUsed = false;
            switch (f.state)
            {
                case FState.Air:
                    SetState(f, FState.Landing, 3);
                    Emit(EvType.Land, f.pos, f.id, -1, impact);
                    break;
                case FState.Helpless:
                    SetState(f, FState.Landing, 12);
                    Emit(EvType.Land, f.pos, f.id, -1, impact);
                    break;
                case FState.AirDodge:
                    SetState(f, FState.Landing, 7);
                    Emit(EvType.Land, f.pos, f.id, -1, impact);
                    break;
                case FState.Attack:
                {
                    MoveDef m = f.MoveDef(f.move);
                    if (IsAerial(f.move))
                    {
                        SetState(f, FState.Landing, m.landingLag);
                        Emit(EvType.Land, f.pos, f.id, -1, impact);
                    }
                    else if (f.moveLeftGround || f.moveStartedAir)
                    {
                        if (m.landMove >= 0) StartMove(f, m.landMove);
                        else if (m.endOnLand) SetState(f, FState.Landing, m.landingLag);
                    }
                    break;
                }
                case FState.Hitstun:
                    if (f.tumble || impact > 8.0f)
                    {
                        if (f.techWindow > 0)
                        {
                            SetState(f, FState.GetUp, 14);
                            f.intangible = 20;
                            f.kbVel = new Vec2(0, 0);
                            f.vel.x = 0.0f;
                            Emit(EvType.Tech, f.pos, f.id);
                        }
                        else if (f.kbVel.y > 9.0f)
                        {
                            f.grounded = false;
                            f.groundPlat = -1;
                            f.kbVel.y = -f.kbVel.y * 0.6f;
                            f.pos.y -= 1.0f;
                            Emit(EvType.Land, f.pos, f.id, -1, impact);
                        }
                        else
                        {
                            SetState(f, FState.Knockdown);
                            f.kbVel = new Vec2(f.kbVel.x * 0.5f, 0.0f);
                            Emit(EvType.Land, f.pos, f.id, -1, impact);
                        }
                    }
                    break;
                default:
                    break;
            }
        }

        public void TryLedgeGrab(Fighter f)
        {
            if (f.grounded || f.ledgeCooldown > 0) return;
            bool ok = f.state == FState.Air || f.state == FState.Helpless ||
                      (f.state == FState.Attack && f.move == USpecial && f.moveFrame > 8);
            if (!ok) return;
            if (f.state == FState.Air && f.vel.y + f.kbVel.y < 0.0f) return;
            float w = f.Width(), h = f.Height();
            for (int l = 0; l < LedgeCount(); ++l)
            {
                if (!LedgeValid(l) || LedgeOwner(l) >= 0) continue;
                Vec2 L = LedgePoint(l);
                int toStage = (l % 2 == 0) ? 1 : -1;
                float dx = (f.pos.x - L.x) * toStage;  // negative = outside the stage
                if (dx > 8.0f || dx < -(w * 0.5f + 60.0f)) continue;
                float hand = f.pos.y - h * 0.8f;
                if (hand < L.y - 45.0f || hand > L.y + 50.0f) continue;
                SetState(f, FState.LedgeHang);
                f.ledge = l;
                f.intangible = 40;
                f.airJumps = f.def.airJumps;
                f.fastFall = false;
                f.airDodgeUsed = false;
                f.sideSpecialUsed = false;
                f.vel = f.kbVel = new Vec2(0, 0);
                Emit(EvType.LedgeGrab, L, f.id);
                return;
            }
        }

        public bool GroundBelow(Vec2 feet, float maxDist)
        {
            foreach (var p in platforms)
            {
                if (feet.x < p.rect.Left() || feet.x > p.rect.Right()) continue;
                float d = p.rect.Top() - feet.y;
                if (d >= -1.0f && d <= maxDist) return true;
            }
            return false;
        }
    }
}
