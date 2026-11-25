using System.Collections.Generic;
using static ClashHorizon.Core.MathUtil;

namespace ClashHorizon.Core
{
    // Port of src/core/weapons.cpp. Everyone starts bare-handed; now and then one of a fighter's two weapons (A or B, one
    // it does not have) drops on the stage: only its owner can take it, by standing on it and holding the weapon button.
    // The weapon taken is in hand; with both, the button switches A <-> B, with no cooldown, as soon as the fighter is free
    // to act (a press during a move waits for it to end). The weapons stay until a KO; then they drop again later.
    public sealed partial class Match
    {
        const int kMaxItems = 4;  // two fighters, both weapons each
        const int kItemLife = 60 * 20;
        const float kItemGravity = 0.45f;
        const float kItemMaxFall = 9.0f;
        const int kWeaponBuffer = 45;  // frames a press waits for the fighter to be free (a move to end)

        static bool CanAct(Fighter f)
        {
            switch (f.state)
            {
                case FState.Idle: case FState.Walk: case FState.Run: case FState.Crouch: case FState.Air:
                case FState.Landing: case FState.JumpSquat: case FState.Shield:
                    return f.Active();
                default:
                    return false;
            }
        }

        public void SpawnWeapon(int character, int slot, Vec2 pos)
        {
            var it = new WeaponItem();
            it.character = character;
            it.slot = slot & 1;
            it.pos = pos;
            it.vel = new Vec2(0.0f, 1.0f);
            it.life = kItemLife;
            items.Add(it);
            Emit(EvType.WeaponSpawn, pos, -1, it.slot, (float)character);
        }

        public void DropWeaponFor(int id, int slot = 0)
        {
            if (id < 0 || id >= fighters.Count) return;
            Fighter f = fighters[id];
            SpawnWeapon(f.charIndex, slot, new Vec2(f.pos.x + f.facing * 90.0f, f.pos.y - 260.0f));
        }

        public void EquipWeapon(Fighter f, int slot)
        {
            if (f.weapon >= 0) Emit(EvType.WeaponVanish, f.Center(), f.id, f.weapon, (float)f.charIndex);
            f.weapon = slot & 1;
            f.weapons |= 1 << f.weapon;
            Emit(EvType.WeaponSummon, f.Center(), f.id, f.weapon, (float)f.charIndex);
        }

        public void SwitchWeapon(Fighter f)
        {
            if (f.weapons == 3 && f.weapon >= 0) EquipWeapon(f, f.weapon ^ 1);
        }

        public void LoseWeapon(Fighter f)
        {
            f.weapons = 0;
            if (f.weapon < 0) return;
            Emit(EvType.WeaponVanish, f.Center(), f.id, f.weapon, (float)f.charIndex);
            f.weapon = -1;
        }

        public void UpdateItems()
        {
            if (!Started()) return;

            // A new drop now and then: a weapon a fighter does not have, and that is not already waiting on the stage
            if (cfg.weapons && --itemTimer <= 0)
            {
                itemTimer = rng.Irange(60 * 6, 60 * 11);
                var wanting = new List<(int id, int slot)>();  // fighter, weapon slot
                IReadOnlyList<CharacterDef> roster = Roster.All();
                foreach (Fighter f in fighters)
                {
                    if (!f.Active() || !roster[f.charIndex].armed) continue;
                    for (int slot = 0; slot < 2; ++slot)
                    {
                        bool onStage = ((f.weapons >> slot) & 1) != 0;
                        foreach (WeaponItem it in items) onStage = onStage || (it.character == f.charIndex && it.slot == slot);
                        if (!onStage) wanting.Add((f.id, slot));
                    }
                }
                if (wanting.Count > 0 && items.Count < kMaxItems)
                {
                    var (id, slot) = wanting[rng.Irange(0, wanting.Count - 1)];
                    Fighter f = fighters[id];
                    // Above a random platform (the main one more often), inside the camera's usual view
                    var plats = new List<int>();
                    for (int i = 0; i < platforms.Count; ++i)
                    {
                        plats.Add(i);
                        if (platforms[i].solid) plats.Add(i);
                    }
                    Rect r = platforms[plats[rng.Irange(0, plats.Count - 1)]].rect;
                    float x = rng.Range(r.Left() + 30.0f, Maxf(r.Left() + 30.0f, r.Right() - 30.0f));
                    SpawnWeapon(f.charIndex, slot, new Vec2(x, r.Top() - 620.0f));
                }
            }

            Rect blast = stageDef.blast;
            foreach (WeaponItem it in items)
            {
                ++it.age;
                --it.life;
                // Fall, land on the first platform top under it (moving platforms carry it)
                it.grounded = false;
                if (it.vel.y >= 0.0f)
                {
                    foreach (Platform p in platforms)
                    {
                        Rect r = p.rect;
                        if (it.pos.x < r.Left() || it.pos.x > r.Right()) continue;
                        if (it.pos.y <= r.Top() + 6.0f && it.pos.y + it.vel.y + 2.0f >= r.Top())
                        {
                            if (it.vel.y > 3.0f)
                                Emit(EvType.WeaponLand, new Vec2(it.pos.x, r.Top()), -1, it.slot, (float)it.character);
                            it.pos.y = r.Top();
                            it.vel = new Vec2(0.0f, 0.0f);
                            it.grounded = true;
                            break;
                        }
                    }
                }
                if (!it.grounded)
                {
                    it.vel.y = Minf(kItemMaxFall, it.vel.y + kItemGravity);
                    it.pos += it.vel;
                }
                if (it.pos.y > blast.Bottom()) it.life = 0;
            }

            // The weapon button: held over an own weapon it does not have, takes it; with both, a press switches A <-> B
            foreach (Fighter f in fighters)
            {
                WeaponItem under = null;
                Rect hurt = f.Hurtbox();
                foreach (WeaponItem it in items)
                    if (it.life > 0 && it.character == f.charIndex && ((f.weapons >> it.slot) & 1) == 0 &&
                        CircleRect(new Vec2(it.pos.x, it.pos.y - 18.0f), 34.0f, hurt))
                        under = it;
                if (under != null)
                {
                    if (f.@in.weapon && CanAct(f) && ++under.hold >= kPickupFrames)
                    {
                        EquipWeapon(f, under.slot);
                        under.life = 0;
                    }
                    continue;
                }
                if (f.weapons != 3) continue;
                if (f.Pressed(static i => i.weapon)) f.weaponBuffer = kWeaponBuffer;
                if (f.weaponBuffer <= 0) continue;
                --f.weaponBuffer;
                if (!CanAct(f)) continue;
                f.weaponBuffer = 0;
                SwitchWeapon(f);
            }
            items.RemoveAll(i => i.life <= 0);
        }
    }
}
