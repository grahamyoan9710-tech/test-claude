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

# clip: centre, radius, start and end angles (degrees, the blade moves from a0 to a1), vertical squash (a flat ellipse:
# a horizontal cut seen from the side), width; poses from /mnt/project-files/hakuen/coupes_h01.md and her clips
H_CUT = ((0.15, 1.12), 1.2, -170, 5, 0.38, 0.32)      # horizontal, behind to front, chest high
CUTS = {
    "a_jab": H_CUT,
    "a_jab2": ((0.25, 1.05), 1.15, -50, 95, 1.0, 0.32),    # rising in front, to straight up
    "a_jab3": ((0.4, 0.85), 1.2, 110, -25, 1.0, 0.36),    # falling over the head to low ahead
    "a_ftilt": ((0.2, 1.0), 1.15, 125, -40, 0.9, 0.32),   # high behind the shoulder to low ahead
    "a_utilt": ((0.15, 1.05), 1.35, -170, 5, 0.42, 0.34),  # a wide horizontal
    "a_dtilt": ((0.2, 1.0), 1.15, -45, 95, 1.0, 0.32),    # rising
    "a_fair": ((0.25, 1.35), 1.15, 100, -95, 1.0, 0.34),  # in the air: over the head to under the feet, in front
    "a_nair": ((0.0, 1.15), 1.15, 180, -180, 0.5, 0.26),  # a full turn round her
    "a_fthrow": H_CUT,
    "a_uthrow": ((0.1, 1.1), 1.1, -30, 95, 1.0, 0.3),     # rising over her
    "a_ledgeattack": ((0.2, 0.5), 1.2, -170, 5, 0.35, 0.3),  # horizontal, knee high
    "a_getupattack": ((0.2, 0.5), 1.2, -170, 5, 0.35, 0.3, 2),  # twice (its second cut 2 frames later)
}


def drop(s, head=0.12, p=0.9):
    """Width along a cut (s: 0 at its tip, 1 at its tail): round at the tip, thinning to nothing."""
    return min(1.0, s / head) ** 0.5 * (1.0 - s) ** p


def arcpts(c, R, a0, a1, sy, n=36):
    return [(c[0] + R * math.cos(math.radians(a0 + (a1 - a0) * i / n)),
             c[1] + R * sy * math.sin(math.radians(a0 + (a1 - a0) * i / n))) for i in range(n + 1)]


def frames(cut, b, f):
    """The shapes of frame f of cut `cut` (weapon B if b)."""
    g, seed = CUTS[cut], sum(map(ord, cut)) * 2 + b
    out = one(*g[:6], b, f, seed)
    if len(g) > 6 and f >= 2:  # its second cut, back the other way, a hair lower
        c, R, a0, a1, sy, W = g[:6]
        out += one((c[0], c[1] - 0.08), R * 0.95, a1, a0, sy, W, b, f - 2, seed + 1)
    return out


def one(c, R, a0, a1, sy, W, b, f, seed):
    """One cut, frame f."""
    rng = random.Random(seed)
    out = []
    # the tail eats toward the tip from frame 2; the whole cut thins and cools
    eat = 0.0 if f < 2 else min(0.92, ((f - 1) / (CN - 2)) ** 0.8)
    k = 1.0 if f < 2 else max(0.0, 1.0 - (f - 1) / (CN - 1)) ** 0.7
    if k <= 0:
        return out
    tail = a1 + (a0 - a1) * (1.0 - eat)  # from the tip (a1) back to here
    grow = 1.0 + 0.04 * f
    P = arcpts(c, R * grow, a1, tail, sy)  # from the tip back
    n = len(P) - 1
    w = W * (1.25 if f == 0 else 1.0) * k ** 0.9
    pieces = 1 if f < 4 else 2 if f < 7 else 3  # it breaks as it fades
    for j in range(pieces):
        lo, hi = j * n // pieces, (j + 1) * n // pieces
        if pieces > 1:
            hi -= max(1, n // (pieces * 5))  # a gap
        seg = P[lo:hi + 1]
        if len(seg) < 2:
            continue
        ws = [w * drop((lo + i) / n) for i in range(len(seg))]
        # the body sits inside the edge: shift each point inward by half its width
        body = []
        for i, (x, z) in enumerate(seg):
            dx, dz = x - c[0], (z - c[1]) / max(sy, 0.2)
            L = math.hypot(dx, dz) or 1.0
            body.append((x - dx / L * ws[i] * 0.45, z - dz / L * ws[i] * 0.45 * sy))
        heat = (0.3 if b else 0.16) + 0.2 * k  # red bodies: the white (A) or gold (B) is the edge's
        out.append(dict(k="rib", pts=body, w=ws, m="solid", a=k, s=1.0 + (0.5 if f < 2 else 0.0), h=heat, y=-0.02))
        # the razor edge: white-hot on A, gold on B
        out.append(dict(k="rib", pts=seg, w=[0.022 * (0.4 + 0.6 * drop((lo + i) / n, 0.05, 0.6)) + 0.004
                                            for i in range(len(seg))],
                        m="line", a=k, s=3.0 if not b else 2.4, h=1.0 if not b else 0.78, y=-0.03))
    # its echo, a hair outside, a frame late, thin
    if 1 <= f <= 6:
        E = arcpts(c, R * grow + 0.09, a1 - (a1 - a0) * 0.05, tail + (a1 - tail) * 0.25, sy)
        out.append(dict(k="rib", pts=E, w=[0.03 * drop(i / (len(E) - 1), 0.1, 1.0) + 0.003 for i in range(len(E))],
                        m="line", a=0.7 * k, s=1.4, h=0.7 if b else 0.6, y=-0.025))
    # the glint at the tip, frames 0-2: a long thin four-point star
    if f < 3:
        g = (1.0, 0.75, 0.35)[f]
        tip = P[0]
        out.append(dict(k="poly", pts=place(star(4, 0.32 * g, 0.018), tip, math.radians(a1 + 90 * (f % 2) * 0.1)),
                        m="shape", a=g, s=2.6, h=1.0, y=-0.035))
        out.append(dict(k="dot", c=tip, r=0.05 * g, m="solid", a=g, s=3.0, h=1.0, y=-0.034))
    # shards flung along the cut, outward, from frame 1
    for i in range(14):
        born = 1 + rng.randint(0, 2)
        life = rng.randint(4, 7)
        s0 = rng.uniform(0.0, 0.85)
        ang = a1 + (a0 - a1) * s0
        sp = rng.uniform(0.05, 0.11)
        age = f - born
        if not (0 <= age < life):
            continue
        q = 1.0 - age / life
        dirn = math.radians(ang + (90 if a1 < a0 else -90) * 0.6)  # out and along the blade's travel
        x = c[0] + R * math.cos(math.radians(ang)) + sp * age * math.cos(dirn)
        z = c[1] + R * sy * math.sin(math.radians(ang)) + sp * age * math.sin(dirn) - 0.004 * age * age
        L = rng.uniform(0.07, 0.15) * q
        out.append(dict(k="poly", pts=place(shard(L, L * 0.22), (x, z), dirn), m="solid", a=q,
                        s=1.6, h=0.75 if (b and i % 2) else 0.55 + 0.3 * q, y=-0.028))
    # weapon B: gold eight-point stars blinking on the cut
    if b:
        for i, (s0, born) in enumerate(((0.15, 1), (0.45, 2), (0.75, 3))):
            age = f - born
            if 0 <= age < 5:
                q = (1.0, 0.85, 0.6, 0.35, 0.15)[age]
                ang = math.radians(a1 + (a0 - a1) * s0)
                p = (c[0] + (R * grow + 0.05) * math.cos(ang), c[1] + (R * grow + 0.05) * sy * math.sin(ang))
                r = 0.05 * (1.0 + 0.3 * (i == 1))
                out.append(dict(k="poly", pts=place(star(4, r * 3 * q, r * 0.3), p, 0.2 * age), m="shape", a=q,
                                s=2.2, h=0.85, y=-0.036))
                out.append(dict(k="poly", pts=place(star(4, r * 1.5 * q, r * 0.3), p, math.pi / 4 + 0.2 * age),
                                m="shape", a=q * 0.8, s=2.2, h=0.8, y=-0.0361))
    return out
