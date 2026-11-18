# Clash Horizon - Hakuen's sword cuts (yoyo, 17/11): her clips jump from the wind-up straight to the end pose of the
# cut, so the sweep of the blade is this effect, played on the cut's frame. Feel: Guilty Gear Strive and Dragon Ball
# FighterZ slashes, Vergil's cuts (feel only): the whole crescent there at once (a smear), white-hot at its leading
# edge, its tail eaten toward the tip as it cools, breaking into pieces; a thin echo outside it, shards flung along it,
# a glint at the tip. Weapon A (nodachi): blood red. Weapon B (long odachi): crimson lacquer edged with gold, gold
# eight-point stars (its guard's) on the cut.
# Pure Python shapes (units of 100 px, origin at the fighter's feet, x to its front, z up), built by effets.py.
import math
import random

from aura_geo import star, place, shard

TAU = 2 * math.pi
CN = 11  # frames (30 fps): about a third of a second


# yoyo, 18/11: "les traînées des lames / coups rapides : un truc simple, pas d'éclair ni autre, en blanc, plus vers
# l'arme (là elle est trop devant)". So each cut is one white crescent drawn by the blade's tip: its outer edge is the
# tip's path, its body lies on the blade (inward), no lightning, shards or lines. clip: centre (her shoulders or
# hands, the pivot), radius (pivot to tip), start and end angles (degrees, from the wind-up tip to the cut's end
# tip, measured on her clips' frames 6 and 8), vertical squash (a horizontal cut seen from the side), width.
CUTS = {
    "a_jab": ((0.1, 1.05), 1.35, 114, -42, 1.0, 0.34),      # diagonal down, from over the shoulder to low ahead
    "a_jab2": ((-0.3, 1.2), 1.42, -124, 120, 1.0, 0.34),    # rising from low behind, past her front, over her head
    "a_jab3": ((0.05, 1.0), 1.2, 175, -12, 1.0, 0.36),      # over the head down to the knees ahead
    "b_jab": ((0.0, 1.25), 1.56, -126, 55, 1.0, 0.26),      # one hand: rising, low behind to high ahead
    "b_jab2": ((0.14, 1.12), 1.1, -172, 8, 0.3, 0.22),      # wide horizontal at the shoulders, behind to ahead
    "b_jab3": ((0.07, 0.55), 1.17, -172, 2, 0.25, 0.22),    # skimming the floor, behind to ahead
}
for _w in "ab":  # her eclipse cuts are the same clips as the three M1
    for _t, _j in (("ftilt", "jab"), ("utilt", "jab2"), ("dtilt", "jab3")):
        CUTS["%s_%s" % (_w, _t)] = CUTS["%s_%s" % (_w, _j)]
OLD = {  # the other cuts, their old lines pulled in onto the blade (smaller radius, pivot nearer her)
    "fair": ((0.15, 1.3), 1.0, 100, -95, 1.0, 0.32),
    "nair": ((0.0, 1.15), 1.0, 180, -180, 0.5, 0.26),
    "fthrow": ((0.1, 1.12), 1.0, -170, 5, 0.38, 0.3),
    "uthrow": ((0.05, 1.1), 1.0, -30, 95, 1.0, 0.3),
    "ledgeattack": ((0.1, 0.5), 1.0, -170, 5, 0.35, 0.3),
    "getupattack": ((0.1, 0.5), 1.0, -170, 5, 0.35, 0.3, 2),  # twice (its second cut 2 frames later)
}
for _k, _g in OLD.items():
    CUTS["a_" + _k] = _g
    CUTS["b_" + _k] = (_g[0], _g[1] * 1.12, _g[2], _g[3], _g[4], _g[5] * 0.8, *_g[6:])

CUTS.update({  # her air cuts, measured on their clips (wind-up frame 6, cut frame 8)
    "a_fair": ((-0.2, 1.3), 1.1, 122, -84, 1.0, 0.3),      # from overhead behind, past her front, down under her
    "a_nair": ((-0.1, 1.5), 1.03, -170, 17, 0.4, 0.24),     # a spin, ending flat ahead at her chest (cut frame 15)
    "b_fair": ((-0.4, 1.35), 1.17, 115, -87, 1.0, 0.24),
    "b_nair": ((-0.55, 1.4), 1.22, 124, -82, 1.0, 0.24),
})


def drop(s, head=0.12, p=0.9):
    """Width along a cut (s: 0 at its tip, 1 at its tail): round at the tip, thinning to nothing."""
    return min(1.0, s / head) ** 0.5 * (1.0 - s) ** p


def arcpts(c, R, a0, a1, sy, n=36):
    return [(c[0] + R * math.cos(math.radians(a0 + (a1 - a0) * i / n)),
             c[1] + R * sy * math.sin(math.radians(a0 + (a1 - a0) * i / n))) for i in range(n + 1)]


def frames(cut, b, f):
    """The shapes of frame f of cut `cut` (weapon B if b)."""
    g = CUTS[cut]
    out = trail(*g[:6], f)
    if len(g) > 6 and f >= 2:  # its second cut, back the other way, a hair lower
        c, R, a0, a1, sy, W = g[:6]
        out += trail((c[0], c[1] - 0.08), R * 0.95, a1, a0, sy, W, f - 2)
    return out


def trail(c, R, a0, a1, sy, W, f):
    """One white crescent, frame f: whole at once, its tail eaten toward the tip as it fades."""
    a0 = a1 - math.copysign(min(abs(a1 - a0), 100), a1 - a0)  # only the swing's end: it stays on the blade
    tip = arcpts(c, R, a1, a1, sy, 1)[0]  # a tighter arc ending on the same tip: it hugs the blade, not ahead of it
    c, R = (tip[0] + (c[0] - tip[0]) * 0.7, tip[1] + (c[1] - tip[1]) * 0.7), R * 0.7
    eat = 0.0 if f < 1 else min(0.95, (f / (CN - 2)) ** 0.8)
    k = 1.0 if f < 1 else max(0.0, 1.0 - f / (CN - 1)) ** 0.8
    if k <= 0:
        return []
    tail = a1 + (a0 - a1) * (1.0 - eat)
    P = arcpts(c, R, a1, tail, sy)  # from the tip back
    n = len(P) - 1
    ws = [W * k ** 0.6 * drop(i / n) + 0.004 for i in range(n + 1)]
    body = []  # the body lies inside the tip's path, on the blade
    for i, (x, z) in enumerate(P):
        dx, dz = x - c[0], (z - c[1]) / max(sy, 0.2)
        L = math.hypot(dx, dz) or 1.0
        body.append((x - dx / L * ws[i] * 0.5, z - dz / L * ws[i] * 0.5 * sy))
    return [dict(k="rib", pts=body, w=ws, m="solid", a=0.8 * k, s=1.0, h=0.7, y=-0.02),
            dict(k="rib", pts=P, w=[0.02 * drop(i / n, 0.05, 0.6) + 0.004 for i in range(n + 1)], m="line", a=k,
                 s=2.5, h=1.0, y=-0.03)]



# --- the Fente écarlate (her weapon A neutral special), v4 (yoyo, 18/11, his GIF of Vergil's skill in PGR: "pas une
# boule", "plus petit", "slashs plus fins"; the Judgment Cut is for her ultimate). What the GIF does: the draw (a glint,
# a hairline), then on the target a flat whirlwind of thin energy ribbons, a disk seen from the side, all turning one
# way, white heads, tails melting off; it thins into fine arcs; again tilted on its side; a last, wider one. Ours in
# red, a few red bolts as each opens. 60 fps, centred on the projectile (RH = 64 px): the game holds it 42 frames,
# hits on 0, 14 and 28 (the last on her resheathe).
JN = 42
JRH = 64
VBORN = (1, 14, 28)


def slash(p, ang, L, w, k, heat, y):
    """A straight cut through p: thin at both ends, widest a little ahead of its middle; its razor line on it."""
    n = 16
    pts = [(p[0] + (i / n - 0.5) * L * math.cos(ang), p[1] + (i / n - 0.5) * L * math.sin(ang)) for i in range(n + 1)]
    ws = [w * math.sin(math.pi * i / n) ** 0.7 * (0.6 + 0.4 * i / n) + 0.003 for i in range(n + 1)]
    return [dict(k="rib", pts=pts, w=ws, m="solid", a=k, s=1.1, h=heat, y=y),
            dict(k="rib", pts=pts, w=[0.3 * x + 0.002 for x in ws], m="line", a=k, s=3.0, h=1.0, y=y - 0.001)]


def jag(a, b, depth, amp, r):
    pts = [a, b]
    for _ in range(depth):
        new = [pts[0]]
        for p, q in zip(pts, pts[1:]):
            dx, dz = q[0] - p[0], q[1] - p[1]
            L = math.hypot(dx, dz) or 1e-6
            o = r.uniform(-amp, amp) * L
            new += [((p[0] + q[0]) / 2 - dz / L * o, (p[1] + q[1]) / 2 + dx / L * o), q]
        pts = new
    return pts


def bolt(out, p, ang, L, r, a, w=1.0, fork=0.6, y=-0.06):
    """Red lightning from p, L long, zigzagging; often a fork."""
    q = (p[0] + L * math.cos(ang), p[1] + L * math.sin(ang))
    pts = jag(p, q, 4, 0.45, r)
    out.append(dict(k="bolt", pts=pts, w=w, a=a, s=1.0, h=0.9, y=y))
    if r.random() < fork:
        m = pts[r.randrange(4, len(pts) - 2)]
        fa = ang + r.choice((-1, 1)) * r.uniform(0.4, 0.9)
        fl = L * r.uniform(0.3, 0.55)
        out.append(dict(k="bolt", pts=jag(m, (m[0] + fl * math.cos(fa), m[1] + fl * math.sin(fa)), 3, 0.45, r),
                        w=w * 0.65, a=a * 0.8, s=1.0, h=0.9, y=y))


def whirl(out, f, born, seed, R, roll, life=16):
    """yoyo's GIF of Vergil (PGR), 18/11: "pas une boule". A flat whirlwind, a disk seen from the side, tilted by
    `roll`: thin ribbons all turning the same way in the same plane, curling inward, white heads and red tails; it
    widens a little, then thins into fine arcs and is gone. No dark heart, no orbits of every tilt (a ball)."""
    a = f - born
    if not 0 <= a < life:
        return
    rng = random.Random(seed)
    fade = 1.0 if a < life - 8 else (life - a) / 8.0
    grow = (0.6 + 0.4 * min(1.0, a / 4.0) ** 0.5) * (1.0 + 0.015 * a)
    cr, sr = math.cos(roll), math.sin(roll)
    for j in range(9):
        sy = rng.uniform(0.26, 0.4)
        tl = rng.uniform(-0.12, 0.12)  # a hair off the disk's plane
        Rj = R * rng.uniform(0.55, 1.0) * grow
        om = rng.uniform(0.34, 0.44)
        h0 = rng.uniform(0, TAU) + om * a
        span = rng.uniform(1.6, 2.6) * (0.5 + 0.5 * min(1.0, a / 3.0))
        W = rng.uniform(0.07, 0.13)
        n = 24
        pts, ws = [], []
        for i in range(n + 1):
            u = i / n
            an = h0 - span * u
            r = Rj * (1.0 - 0.22 * u)
            x, z = r * math.cos(an), r * sy * math.sin(an) + tl * r * math.cos(an)
            pts.append((x * cr - z * sr, x * sr + z * cr))
            ws.append(W * fade * min(1.0, u / 0.08) ** 0.5 * (1.0 - u) ** 0.8 + 0.003)
        k = fade ** 0.5 * (1.0 if a > 0 else 0.6)
        out.append(dict(k="rib", pts=pts, w=[x * 1.8 for x in ws], m="veil", a=0.6 * k, s=1.3, h=0.45,
                        y=-0.02 - 0.003 * j))  # its red glow
        out.append(dict(k="rib", pts=pts, w=ws, m="veil", a=0.9 * k, s=2.0, h=0.8, y=-0.025 - 0.003 * j))
        head = pts[: n // 2]  # its white edge
        out.append(dict(k="rib", pts=head, w=[0.016 * (1.0 - i / len(head)) + 0.003 for i in range(len(head))],
                        m="line", a=k, s=3.0, h=1.0, y=-0.05 - 0.003 * j))
    if a < 3:  # a few short red bolts as it opens
        q = random.Random(seed * 13 + a)
        for i in range(2):
            an = q.uniform(0, TAU)
            bolt(out, (0.6 * R * math.cos(an), 0.3 * R * math.sin(an)), an, R * q.uniform(0.4, 0.6), q,
                 (1.0, 0.8, 0.5)[a], 0.9, 0.5, -0.1)
    for i in range(10):  # sparks flung off it
        an = rng.uniform(0, TAU)
        b, sp, lf = rng.randint(1, 6), rng.uniform(0.05, 0.1), rng.randint(5, 8)
        ag = a - b
        if 0 <= ag < lf:
            k = 1.0 - ag / lf
            d = R * 0.8 + sp * ag
            c = (d * math.cos(an), 0.45 * d * math.sin(an) - 0.003 * ag * ag)
            Ls = rng.uniform(0.05, 0.1) * k
            out.append(dict(k="poly", pts=place(shard(Ls, Ls * 0.25), c, an + 1.3), m="solid", a=k, s=2.2,
                            h=0.6 + 0.4 * k, y=-0.1))


def jugement(f):
    out = []
    whirl(out, f, VBORN[0], 101, 0.8, -0.15)  # flat, like the GIF's first
    whirl(out, f, VBORN[1], 202, 0.85, 0.9)   # tilted on its side, its second
    whirl(out, f, VBORN[2], 303, 1.0, 0.2, life=14)  # the last, wider
    if f < 3:  # the click: a hairline flash across the spot as the draw arrives
        out += slash((0.0, 0.0), -0.12, 2.0, 0.014, (1.0, 0.7, 0.3)[f], 1.0, -0.16)
    return out


# --- the draw (pfx_h01_degaine, castFx on the projectile's frame, left where it began): a razor line from her hip to
# the spot 200 px ahead, its glint at the hip, speed lines along it. Feet at the origin, facing right, 60 fps.
DN = 16


def degaine(f):
    out = []
    hip = (0.25, 0.92)
    end = (2.75, 0.84)
    ang = math.atan2(end[1] - hip[1], end[0] - hip[0])
    L = math.hypot(end[0] - hip[0], end[1] - hip[1])
    mid = ((hip[0] + end[0]) / 2, (hip[1] + end[1]) / 2)
    if f < 8:
        k = (1.0, 1.0, 0.85, 0.65, 0.45, 0.3, 0.18, 0.08)[f]
        out += slash(mid, ang, L * (1.0 + 0.02 * f), (0.07, 0.05, 0.04, 0.03, 0.025, 0.02, 0.016, 0.012)[f], k,
                     0.6 + 0.4 * k, -0.02)
    if f < 4:  # the glint at her hip
        g = (1.0, 0.8, 0.5, 0.25)[f]
        out.append(dict(k="poly", pts=place(star(4, 0.45 * g + 0.1, 0.02), hip, 0.1), m="shape", a=g, s=3.0, h=1.0,
                        y=-0.04))
    if f < 6:  # red lightning racing along the draw
        r = random.Random(40 + f)
        for i in range(3):
            s0 = r.uniform(0.0, 0.7)
            p0 = (hip[0] + s0 * L * math.cos(ang), hip[1] + s0 * L * math.sin(ang))
            bolt(out, p0, ang + r.uniform(-0.35, 0.35), r.uniform(0.5, 1.1), r, (1.0, 0.9, 0.7, 0.5, 0.3, 0.15)[f], 0.9,
                 0.6, -0.05)
    q = random.Random(3)
    for i in range(10):  # speed lines along the draw, rushing ahead
        born = q.randint(0, 3)
        age = f - born
        life = q.randint(5, 8)
        if 0 <= age < life:
            k = 1.0 - age / life
            off = q.uniform(-0.18, 0.18)
            s0 = q.uniform(0.0, 0.6) + 0.06 * age
            Ls = q.uniform(0.5, 1.0)
            p0 = (hip[0] + s0 * L * math.cos(ang) - off * math.sin(ang), hip[1] + s0 * L * math.sin(ang) + off * math.cos(ang))
            pts = [(p0[0] + Ls * j / 8 * math.cos(ang), p0[1] + Ls * j / 8 * math.sin(ang)) for j in range(9)]
            out.append(dict(k="rib", pts=pts, w=[0.012 * (j / 8) * k + 0.002 for j in range(9)], m="line", a=k, s=2.2,
                            h=0.8, y=-0.03))
    return out


# --- weapon B's flying cuts (projectiles, looped, drawn along their course: x ahead) -------------------------------
# Feel: Alpha's (PGR) cuts that go on and Vergil's flying crescents (feel only): a thin neon-red crescent bowed ahead,
# white razor edge, short speed lines and shards trailing behind. b_wave: the Coupe rasante's low wave (RH 20);
# b_trait: the Trait final, taller and with a second crescent nested behind (RH 28).
PN = 8
PROJ = {"b_wave": (0.36, 20), "b_trait": (0.62, 28)}  # half height (units of 100 px), hit radius in px


def flying(kind, f):
    hh, _ = PROJ[kind]
    rng = random.Random(sum(map(ord, kind)))
    fl = 1.0 + 0.06 * math.sin(TAU * f / PN)  # a slow flicker, looped
    out = []
    for j, (dx, sc, k) in enumerate(((0.0, 1.0, 1.0), (-0.16, 0.8, 0.55)) if kind == "b_trait" else ((0.0, 1.0, 1.0),)):
        R = hh * sc
        P = arcpts((dx - R * 0.55, 0.0), R, 80, -80, 1.0, 30)  # bowed ahead, top to bottom
        n = len(P) - 1
        ws = [0.11 * sc * fl * math.sin(math.pi * i / n) ** 0.8 + 0.004 for i in range(n + 1)]
        body = [(x - w * 0.4, z) for (x, z), w in zip(P, ws)]
        out.append(dict(k="rib", pts=body, w=ws, m="solid", a=k, s=1.2, h=0.5, y=-0.02 - 0.005 * j))
        out.append(dict(k="rib", pts=P, w=[0.25 * w + 0.003 for w in ws], m="line", a=k, s=3.0, h=1.0, y=-0.03 - 0.005 * j))
    # speed lines streaming behind, each crossing the strip once per loop
    for i in range(5):
        z = rng.uniform(-0.7, 0.7) * hh
        ph = (f / PN + i / 5) % 1.0
        L = rng.uniform(0.35, 0.7) * (hh / 0.36) ** 0.5
        q = math.sin(math.pi * ph)
        out += slash((-0.2 - 0.9 * ph * L, z), 0.0, L, 0.014, q * 0.8, 0.45, -0.04 - 0.002 * i)
    # shards shed behind
    for i in range(6):
        ph = (f / PN + i / 6) % 1.0
        z = rng.uniform(-0.8, 0.8) * hh
        q = 1.0 - ph
        Ls = rng.uniform(0.04, 0.08) * q
        out.append(dict(k="poly", pts=place(shard(Ls, Ls * 0.25), (-0.1 - 0.6 * ph, z - 0.1 * ph * ph), math.pi),
                        m="solid", a=q, s=1.6, h=0.6, y=-0.06))
    return out
