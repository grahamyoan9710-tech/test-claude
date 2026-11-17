# The hit marks of each fighter (yoyo, 17/11: spectacular and unique to each fighter, the bar Guilty Gear Strive and
# Dragon Ball FighterZ set): played once where a blow lands, facing the knockback (+x). Every mark shares a fighting
# game's grammar (frame 0: a sharp white-hot star of uneven spikes; 1-3: the spikes thin and retract, a few broken arcs
# fly out, bits are flung along the blow; then they fade), and each fighter adds its own matter: 01 shadow ink and a
# violet tear, 02 ice shards and glints, 03 tongues of flame and embers, 07 forked red lightning, 08 needles of light
# and arcs of gold, Hakuen a blood-red cut and red sparks. Pure Python, shapes as in aura_geo (1 = 100 px).
# Kinds: q (quick, a light blow), s (slash, a blade's cut), r (rise, a launcher upward), c (crit, a big blow).
import math
import random

from aura_geo import TAU, rib, jag, star, shard, place

N = dict(q=10, s=12, r=12, c=16)
SIZE = dict(q=0.42, s=0.5, r=0.5, c=0.8)  # the star's reach


def burst(out, f, R, n, rng, h=1.0, aim=0.0, spread=math.pi, life=3, fat=0.09, front=True):
    """The white-hot star: n uneven spikes (longer along aim), sharp, gone in `life` frames; never a disc."""
    if f >= life:
        return
    k = f / life
    pts = []
    for j in range(2 * n):
        a = TAU * j / (2 * n) + rng.uniform(-0.08, 0.08)
        if j % 2:
            rr = R * fat
        else:
            along = math.cos(a - aim) if spread < math.pi else 0.0
            rr = R * rng.uniform(0.45, 1.0) * (1.0 + 0.6 * max(0.0, along))
        rr *= (1.0 - 0.55 * k) if j % 2 == 0 else 1.0
        pts.append((rr * math.cos(a), rr * math.sin(a)))
    out.append(dict(k="poly", pts=pts, m="shape", a=1.0 - k ** 2, s=2.6, h=h, y=-0.05, front=front))


def arcs(out, f, rng, R0, R1, n, born=1, life=6, w=0.03, sy=1.0, aim=0.0, span=2.2, h=0.8):
    """A few broken arcs flying out (a shock), facing the blow, never a whole ring."""
    k = (f - born) / life
    if not 0 <= k < 1:
        return
    R = R0 + (R1 - R0) * (1 - (1 - k) ** 2)
    for j in range(n):
        c = aim + (j - (n - 1) / 2) * span / n + rng.uniform(-0.15, 0.15)
        half = span / n * rng.uniform(0.25, 0.4)
        P = [(R * math.cos(c - half + 2 * half * i / 12), R * sy * math.sin(c - half + 2 * half * i / 12)) for i in range(13)]
        out.append(rib(P, [w * (1 - k) * math.sin(math.pi * i / 12) ** 0.7 + 0.003 for i in range(13)], "line",
                       1.0 - k ** 1.5, 2.0, h, -0.04))


def streaks(out, f, rng, n, L, aim=0.0, spread=0.6, born=(0, 1), life=(4, 6), w=0.012, speed=0.09, h=0.95):
    """Speed lines shooting out along the blow, thin and tapered."""
    for i in range(n):
        a, b, lf, ln = aim + rng.uniform(-spread, spread), rng.randint(*born), rng.randint(*life), rng.uniform(*L)
        d0 = rng.uniform(0.05, 0.2)
        k = (f - b) / lf
        if not 0 <= k < 1:
            continue
        d = d0 + speed * (f - b)
        P = [((d - ln * (1 - j / 8)) * math.cos(a), (d - ln * (1 - j / 8)) * math.sin(a)) for j in range(9)]
        out.append(rib(P, [w * (1 - k) * (j / 8) ** 0.6 + 0.002 for j in range(9)], "line", 1.0 - k, 2.0, h, -0.03))


def flung(out, f, rng, n, poly, speed, aim=0.0, spread=0.7, born=(0, 2), life=(6, 10), size=(0.02, 0.04), h=0.8,
          m="shape", grav=0.004, spin=0.3):
    """Bits flung along the blow (every draw made each frame: they keep their course)."""
    for i in range(n):
        a, v, b, lf, sz = (aim + rng.uniform(-spread, spread), speed * rng.uniform(0.5, 1.0), rng.randint(*born),
                           rng.randint(*life), rng.uniform(*size))
        rot, sp = a + rng.uniform(-0.4, 0.4), rng.uniform(-spin, spin)
        k = f - b
        if not 0 <= k < lf:
            continue
        x, z = v * math.cos(a) * k, v * math.sin(a) * k - grav * k * k
        out.append(dict(k="poly", pts=place(poly(sz), (x, z), rot + sp * k, 1.0 - 0.4 * k / lf), m=m,
                        a=1.0 - (k / lf) ** 2, s=1.6, h=h, y=-0.02, front=True))


def crescent(out, f, R, born, life, a0, a1, w, h=0.85, m="line", sy=0.55, tilt=0.35, s=2.2):
    """A blade's cut: a crescent swept from a0 to a1 in two frames, then thinning (a tapered drop, not a band)."""
    k = (f - born) / life
    if not 0 <= k < 1:
        return
    sweep = min(1.0, (f - born + 1) / 2)
    c, sn = math.cos(tilt), math.sin(tilt)
    P, W = [], []
    for i in range(25):
        u = i / 24
        a = a0 + (a1 - a0) * u * sweep
        x, z = R * math.cos(a), R * sy * math.sin(a)
        P.append((x * c - z * sn, x * sn + z * c))
        W.append(w * (1 - k) ** 0.8 * (u ** 0.6) * (1 - u) ** 0.25 * 1.6 + 0.003)
    out.append(rib(P, W, m, 1.0 - k ** 1.4, s, h, -0.045))
    out.append(rib(P, [x * 0.35 for x in W], m, 1.0 - k ** 1.4, s * 1.2, 1.0, -0.046))


def bolt(out, a, b, rng, w=0.9, alpha=1.0, h=0.9, depth=3, amp=0.4):
    out.append(dict(k="bolt", pts=jag(a, b, depth, amp, rng), w=w, a=alpha, s=1.0, h=h, y=-0.035))


# --- each fighter's matter --------------------------------------------------------------------------------------
def ink(L):
    """A drop of shadow ink: long, pointed both ends, one side fuller."""
    return [(L * 0.6, 0.0), (L * 0.1, L * 0.22), (-L * 0.45, L * 0.08), (-L * 0.6, 0.0), (-L * 0.3, -L * 0.12),
            (L * 0.15, -L * 0.15)]


def p01(kind, f, rng):
    """Ombre violette: shadow ink splashing out along the blow under a violet star; the crit tears a dark cross."""
    R, out = SIZE[kind], []
    aim = math.pi / 2 if kind == "r" else 0.0
    burst(out, f, R, 7 if kind == "q" else 9, rng, h=0.9, aim=aim, spread=1.0)
    flung(out, f, rng, 7 if kind == "q" else 12, lambda s: ink(s * 3.0), 0.05 if kind == "q" else 0.07, aim=aim,
          h=0.0, m="rock", life=(5, 9))
    if kind == "s":
        crescent(out, f, 0.42, 0, 9, -1.1, 1.3, 0.07, h=0.75)
    if kind == "c":
        for t in (0.8, -0.8):  # the tear: two dark cuts crossing, violet-edged
            P = [(-0.6 * R * math.cos(t) + 1.2 * R * math.cos(t) * i / 12, -0.6 * R * math.sin(t) + 1.2 * R * math.sin(t) * i / 12)
                 for i in range(13)]
            k = (f - 1) / 10
            if 0 <= k < 1:
                W = [0.06 * (1 - k) * math.sin(math.pi * i / 12) ** 0.6 + 0.004 for i in range(13)]
                out.append(rib(P, W, "line", 1.0 - k, 1.6, 0.0, -0.047))
                out.append(rib(P, [w * 0.3 for w in W], "line", 1.0 - k, 2.2, 0.7, -0.048))
        arcs(out, f, rng, 0.2, 0.75, 3, life=8, w=0.035, h=0.7)
    streaks(out, f, rng, 4 if kind == "q" else 7, (0.15, 0.3), aim=aim)
    return out


def p02(kind, f, rng):
    """Givre stellaire: ice shards bursting out, six-point glints, frost dust; the crit a crystal bloom."""
    R, out = SIZE[kind], []
    aim = math.pi / 2 if kind == "r" else 0.0
    burst(out, f, R, 6, rng, h=1.0, aim=aim, spread=1.0, fat=0.12)
    flung(out, f, rng, 8 if kind == "q" else 14, lambda s: shard(s * 3.2, s * 0.9), 0.06, aim=aim, spread=0.9,
          h=0.9, life=(6, 11), spin=0.15)
    for i in range(3 if kind == "q" else 6):  # glints popping round the hit
        b, a, d, sz = rng.randint(1, 4), rng.uniform(0, TAU), rng.uniform(0.1, 0.45), rng.uniform(0.04, 0.07)
        e = (f - b) / 4
        if 0 <= e < 1:
            out.append(dict(k="poly", pts=place(star(3, sz, sz * 0.15), (d * math.cos(a), d * math.sin(a)), 0.3,
                                                math.sin(math.pi * e)), m="shape", a=1.0, s=2.2, h=1.0, y=-0.04,
                            front=True))
    if kind == "s":
        crescent(out, f, 0.42, 0, 9, -1.1, 1.3, 0.06, h=0.9)
    if kind == "c":  # crystals blooming out of the hit and shattering
        for j in range(7):
            a = TAU * j / 7 + rng.uniform(-0.2, 0.2)
            L = rng.uniform(0.35, 0.6)
            k = (f - 1) / 8
            if 0 <= k < 1:
                g = min(1.0, (f) / 2)
                out.append(dict(k="poly", pts=place(shard(L * g, L * 0.28), (0.5 * L * g * math.cos(a), 0.5 * L * g * math.sin(a)),
                                                    a, 1.0), m="shape", a=(1 - k) ** 1.2, s=1.5, h=0.75, y=-0.03,
                                front=True))
        arcs(out, f, rng, 0.25, 0.8, 4, life=7, w=0.03, h=0.95, span=TAU * 0.8)
    streaks(out, f, rng, 4 if kind == "q" else 6, (0.12, 0.25), aim=aim, h=1.0)
    return out


def p03(kind, f, rng):
    """Brasier rouge: tongues of flame bursting along the blow, embers; the crit an explosion of flame petals."""
    from aura_geo import spike
    R, out = SIZE[kind], []
    aim = math.pi / 2 if kind == "r" else 0.0
    burst(out, f, R * 0.9, 7, rng, h=0.95, aim=aim, spread=1.0)
    n = 5 if kind == "q" else 9 if kind != "c" else 14
    for i in range(n):  # tongues: spikes of flame shooting out, curling
        a = aim + rng.uniform(-1.2, 1.2) if kind != "c" else TAU * i / n + rng.uniform(-0.2, 0.2)
        spike(out, (0.0, 0.0), math.pi / 2 - a, rng.uniform(0.6, 1.0) * R * 1.4, rng.uniform(0.08, 0.14) * R * 1.4, f,
              rng.randint(0, 1), 2, rng.randint(6, 9), h=(0.25, 0.6, 0.95), front=True, y=-0.01 - 0.001 * i,
              hook=rng.uniform(-0.4, 0.4), ph=rng.random())
    for i in range(8 if kind == "q" else 14):  # embers
        a, v, b = aim + rng.uniform(-1.0, 1.0), rng.uniform(0.03, 0.07), rng.randint(1, 3)
        k = f - b
        if 0 <= k < 9:
            out.append(dict(k="dot", c=(v * math.cos(a) * k, v * math.sin(a) * k + 0.003 * k * k), r=0.012,
                            m="solid", a=1.0 - k / 9, s=2.0, h=0.85, y=-0.03, front=True))
    if kind == "s":
        crescent(out, f, 0.42, 0, 9, -1.1, 1.3, 0.08, h=0.7, m="flame")
    if kind == "c":
        arcs(out, f, rng, 0.25, 0.8, 4, life=7, w=0.04, h=0.75, span=TAU * 0.8)
    return out


def p07(kind, f, rng):
    """Foudre rouge: forked red lightning cracking out of the hit, sparks; the crit a cross of lightning."""
    R, out = SIZE[kind], []
    aim = math.pi / 2 if kind == "r" else 0.0
    burst(out, f, R * 0.8, 8, rng, h=1.0, aim=aim, spread=1.0, fat=0.07)
    nb = 3 if kind == "q" else 5 if kind != "c" else 8
    for i in range(nb):  # bolts: a new zigzag each frame, crackling for 4 frames
        a, L = aim + rng.uniform(-1.3, 1.3) if kind != "c" else TAU * i / nb, rng.uniform(0.7, 1.2) * R * 1.3
        for age in range(4):
            if f == age + i % 2:
                q = random.Random(i * 31 + f)
                tip = (L * math.cos(a), L * math.sin(a))
                bolt(out, (0.0, 0.0), tip, q, w=1.1 if kind == "c" else 0.9, alpha=(1.0, 0.8, 0.5, 0.25)[age])
                if q.random() < 0.6:
                    m = (tip[0] * 0.55, tip[1] * 0.55)
                    a2 = a + q.choice((-1, 1)) * q.uniform(0.4, 0.9)
                    bolt(out, m, (m[0] + 0.4 * L * math.cos(a2), m[1] + 0.4 * L * math.sin(a2)), q, w=0.7,
                         alpha=(1.0, 0.8, 0.5, 0.25)[age] * 0.8)
    flung(out, f, rng, 8 if kind == "q" else 14, lambda s: shard(s * 2.6, s * 0.35), 0.08, aim=aim, spread=1.1,
          h=0.95, life=(4, 7))
    if kind == "s":
        crescent(out, f, 0.42, 0, 9, -1.1, 1.3, 0.06, h=0.9)
    if kind == "c":
        arcs(out, f, rng, 0.25, 0.85, 4, life=7, w=0.03, h=0.9, span=TAU * 0.8)
    streaks(out, f, rng, 5 if kind == "q" else 8, (0.15, 0.3), aim=aim, h=1.0)
    return out


def p08(kind, f, rng):
    """Lumière: long needles of light and arcs of gold; the crit a flare of crossed rays and a halo of broken arcs."""
    R, out = SIZE[kind], []
    aim = math.pi / 2 if kind == "r" else 0.0
    burst(out, f, R * 1.1, 4 if kind != "c" else 8, rng, h=1.0, aim=aim, spread=1.0, fat=0.05, life=4)
    for i in range(5 if kind == "q" else 9):  # needles of light: very thin, very long, flashing
        a = aim + rng.uniform(-1.4, 1.4) if kind != "c" else TAU * i / 9 + rng.uniform(-0.1, 0.1)
        L, b = rng.uniform(0.6, 1.2) * R * 1.4, rng.randint(0, 2)
        k = (f - b) / 5
        if 0 <= k < 1:
            P = [(L * j / 10 * math.cos(a), L * j / 10 * math.sin(a)) for j in range(11)]
            out.append(rib(P, [0.02 * (1 - k) * math.sin(math.pi * (0.05 + 0.9 * j / 10)) ** 0.5 + 0.002 for j in range(11)],
                           "line", 1.0 - k, 2.4, 1.0, -0.04))
    arcs(out, f, rng, 0.15, 0.5 if kind != "c" else 0.85, 3 if kind != "c" else 5, life=6, w=0.025, h=0.9,
         aim=aim, span=2.0 if kind != "c" else TAU * 0.85)
    for i in range(3 if kind == "q" else 6):  # sparkles
        b, a, d, sz = rng.randint(1, 4), rng.uniform(0, TAU), rng.uniform(0.1, 0.5), rng.uniform(0.04, 0.08)
        e = (f - b) / 4
        if 0 <= e < 1:
            out.append(dict(k="poly", pts=place(star(4, sz, sz * 0.14), (d * math.cos(a), d * math.sin(a)), 0.0,
                                                math.sin(math.pi * e)), m="shape", a=1.0, s=2.2, h=1.0, y=-0.04,
                            front=True))
    if kind == "s":
        crescent(out, f, 0.42, 0, 9, -1.1, 1.3, 0.055, h=0.95)
    return out


def h01(kind, f, rng):
    """Hakuen: a blood-red cut (his blade), red sparks crackling off it; the crit two crossing cuts and a shock."""
    R, out = SIZE[kind], []
    aim = math.pi / 2 if kind == "r" else 0.0
    burst(out, f, R * 0.8, 7, rng, h=0.95, aim=aim, spread=1.0, fat=0.07)
    tilt = 1.2 if kind == "r" else 0.35
    crescent(out, f, 0.4 if kind != "q" else 0.3, 0, 8, -1.2, 1.3, 0.07 if kind != "q" else 0.05, h=0.8, tilt=tilt)
    if kind == "c":
        crescent(out, f, 0.55, 2, 9, -1.3, 1.2, 0.08, h=0.85, tilt=-0.9)
        arcs(out, f, rng, 0.25, 0.8, 4, life=7, w=0.03, h=0.85, span=TAU * 0.8)
    for i in range(2 if kind == "q" else 4):
        if f == 1 + i:
            q = random.Random(i * 13 + 5)
            a = aim + q.uniform(-1.2, 1.2)
            L = q.uniform(0.3, 0.55)
            bolt(out, (0.05 * math.cos(a), 0.05 * math.sin(a)), (L * math.cos(a), L * math.sin(a)), q, w=0.8)
    flung(out, f, rng, 6 if kind == "q" else 10, lambda s: shard(s * 2.4, s * 0.4), 0.07, aim=aim, spread=0.8,
          h=0.9, life=(4, 8))
    streaks(out, f, rng, 4 if kind == "q" else 7, (0.15, 0.3), aim=aim)
    return out


HITS = dict(p01=p01, p02=p02, p03=p03, p07=p07, p08=p08, h01=h01)


def frames(who, kind):
    """The mark's shapes, frame by frame (the same draws each frame: nothing jumps)."""
    seed = sum(map(ord, who + kind))
    return [HITS[who](kind, f, random.Random(seed)) for f in range(N[kind])]
