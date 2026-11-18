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
    # weapon B, the long odachi in the manner of Alpha (PGR): its own three cuts and its air cut, the rest as A's
    # with the longer blade
    "b_jab": ((0.3, 0.95), 1.4, -130, 60, 0.85, 0.3),    # rising diagonal, low behind to high ahead
    "b_jab2": ((0.85, 0.85), 2.3, 40, -40, 0, 0.05, "x"),  # the X: two long cuts crossing ahead of her
    "b_jab3": ((0.45, 0.42), 1.7, -170, 5, 0.3, 0.3),    # skimming the floor, far ahead
    "b_nair": ((0.3, 1.2), 1.4, 100, -95, 1.0, 0.3),     # falling in front, in the air
}
for _k in ("ftilt", "utilt", "dtilt", "fair", "fthrow", "uthrow", "ledgeattack", "getupattack"):
    _c, _R, *_r = CUTS["a_" + _k]
    CUTS["b_" + _k] = (_c, _R * 1.15, *_r)


def drop(s, head=0.12, p=0.9):
    """Width along a cut (s: 0 at its tip, 1 at its tail): round at the tip, thinning to nothing."""
    return min(1.0, s / head) ** 0.5 * (1.0 - s) ** p


def arcpts(c, R, a0, a1, sy, n=36):
    return [(c[0] + R * math.cos(math.radians(a0 + (a1 - a0) * i / n)),
             c[1] + R * sy * math.sin(math.radians(a0 + (a1 - a0) * i / n))) for i in range(n + 1)]


def frames(cut, b, f):
    """The shapes of frame f of cut `cut` (weapon B if b)."""
    g, seed = CUTS[cut], sum(map(ord, cut)) * 2 + b
    if g[-1] == "x":  # a cross: two long straight cuts through c, the second 2 frames later (Alpha's X)
        out = []
        for j, (ang, born) in enumerate(((math.radians(g[2]), 0), (math.radians(g[3]), 2))):
            age = f - born
            if 0 <= age < 7:
                q = (1.0, 0.95, 0.8, 0.6, 0.4, 0.22, 0.1)[age]
                grow = (0.6, 1.0, 1.04, 1.07, 1.09, 1.1, 1.1)[age]
                out += slash(g[0], ang, g[1] * grow, g[5] * (1.4 if age == 0 else 1.0) * (1 - 0.1 * age), q,
                             0.48 - 0.04 * age, -0.02 - 0.01 * j)
        return out
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
    w = W * (1.25 if f == 0 else 1.0) * k ** 0.9 * (0.45 if b else 1.0)  # B: a thin neon cut (Alpha's)
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
        heat = (0.38 if b else 0.16) + 0.2 * k  # red bodies (B: neon red), the white is the edge's
        out.append(dict(k="rib", pts=body, w=ws, m="solid", a=k, s=1.0 + (0.5 if f < 2 else 0.0), h=heat, y=-0.02))
        # the razor edge: white-hot
        out.append(dict(k="rib", pts=seg, w=[0.022 * (0.4 + 0.6 * drop((lo + i) / n, 0.05, 0.6)) + 0.004
                                            for i in range(len(seg))],
                        m="line", a=k, s=3.0, h=1.0, y=-0.03))
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
    # weapon B: Alpha's cuts that go on: long straight lines shooting on from the tip, along its travel
    if b:
        ta = math.radians(a1 + (-90 if a1 < a0 else 90))  # the tip's direction of travel
        tip = arcpts(c, R, a1, a1, sy, 1)[0]
        for i, (born, L, off) in enumerate(((0, 2.2, 0.0), (1, 1.6, 0.06), (2, 1.2, -0.07))):
            age = f - born
            if 0 <= age < 4:
                q = (1.0, 0.75, 0.45, 0.2)[age]
                p = (tip[0] + (0.35 + 0.25 * age) * math.cos(ta) - off * math.sin(ta),
                     tip[1] + (0.35 + 0.25 * age) * math.sin(ta) + off * math.cos(ta))
                out += slash(p, ta, L * (0.8 + 0.1 * age), 0.03 * (1.0 - 0.15 * age), q, 0.45, -0.04 - 0.002 * i)
    return out


# --- the Fente écarlate (her weapon A neutral special): cuts opening in the air 200 px ahead -----------------------
# Feel: Vergil's Judgment Cut (feel only): a click, then a knot of thin, very long, very fast cuts crossing on the spot,
# in three bursts (its three hits, 6 frames apart), each cut white-hot then red, gone in a few frames; shards flung
# out. Centred on the projectile (its hit radius RH = 56 px), drawn as it lives (played once).
JN = 22
JRH = 56


def slash(p, ang, L, w, k, heat, y):
    """A straight cut through p: thin at both ends, widest a little ahead of its middle; its razor line on it."""
    n = 16
    pts = [(p[0] + (i / n - 0.5) * L * math.cos(ang), p[1] + (i / n - 0.5) * L * math.sin(ang)) for i in range(n + 1)]
    ws = [w * math.sin(math.pi * i / n) ** 0.7 * (0.6 + 0.4 * i / n) + 0.003 for i in range(n + 1)]
    return [dict(k="rib", pts=pts, w=ws, m="solid", a=k, s=1.1, h=heat, y=y),
            dict(k="rib", pts=pts, w=[0.3 * x + 0.002 for x in ws], m="line", a=k, s=3.0, h=1.0, y=y - 0.001)]


def jugement(f):
    rng = random.Random(77)
    out = []
    for burst, born0 in enumerate((1, 7, 13)):
        for j in range(5 if burst < 2 else 7):
            born = born0 + rng.randint(0, 2)
            ang = rng.uniform(0, math.pi)
            off = (rng.uniform(-0.18, 0.18), rng.uniform(-0.18, 0.18))
            L = rng.uniform(1.1, 1.7) * (1.15 if burst == 2 else 1.0)
            w = rng.uniform(0.035, 0.06)
            age = f - born
            if 0 <= age < 5:
                k = (1.0, 0.9, 0.6, 0.35, 0.15)[age]
                grow = (0.55, 1.0, 1.05, 1.08, 1.1)[age]  # it snaps open in a frame
                out += slash(off, ang, L * grow, w * (1.3 if age == 0 else 1.0) * (1.2 - 0.2 * age / 4), k,
                             0.42 - 0.06 * age, -0.02 - 0.002 * j - 0.02 * burst)
            # its shards: flung out along it from its second frame
            for s in range(2):
                sa = ang + rng.choice((0.0, math.pi)) + rng.uniform(-0.3, 0.3)
                sp, life = rng.uniform(0.05, 0.1), rng.randint(4, 7)
                a2 = f - born - 1
                if 0 <= a2 < life:
                    q = 1.0 - a2 / life
                    x, z = off[0] + (0.3 + sp * a2) * math.cos(sa), off[1] + (0.3 + sp * a2) * math.sin(sa)
                    Ls = rng.uniform(0.06, 0.12) * q
                    out.append(dict(k="poly", pts=place(shard(Ls, Ls * 0.25), (x, z), sa), m="solid", a=q, s=1.6,
                                    h=0.5 + 0.3 * q, y=-0.09))
    if f < 2:  # the click: a hairline flash across the spot
        out += slash((0.0, 0.0), 0.15, 2.0, 0.012, (1.0, 0.5)[f], 1.0, -0.1)
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
