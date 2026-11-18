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


# --- the Fente écarlate (her weapon A neutral special), v3 (yoyo, 18/11: "pas assez d'effet, juste plein de
# traits", his GIF of Vergil in PGR; "01 fait de la foudre rouge") -------------------------------------------------
# What the GIF does, frame by frame: a glint and a hairline at the draw; then, on the target, a white flash and a
# great sphere of thick energy ribbons whirling round it (orbits seen at every tilt, white-hot leading edges, tails
# melting into dark smoke), the inside darkened, sparks flung out; it fades in thin arcs; again; a last bigger one.
# Ours, in red with red lightning (Hakuen's own): three vortices on the projectile's three hits, the last one
# exploding: long cuts through it, its smoke torn into dark shards rimmed with fire, lightning leaping out.
# Never a ball, a whole ring or a cone: the ribbons are broken arcs, the flash uneven rays. 60 fps, centred on the
# projectile (RH = 56 px): the game holds it 42 frames, hits on 0, 14 and 28 (the last on her resheathe).
JN = 42
JRH = 56
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


def vortex(out, f, born, seed, R, big=False):
    a = f - born
    life = 13 if big else 15
    if not 0 <= a < life:
        return
    rng = random.Random(seed)
    fade = 1.0 if a < life - 7 else (life - a) / 7.0
    grow = (0.55 + 0.45 * min(1.0, a / 4.0) ** 0.5) * (1.0 + 0.012 * a) * (1.0 + (0.05 * max(0, a - 3) if big else 0))
    # the flash: uneven white rays, three frames
    if a < 3:
        g = (1.0, 0.75, 0.4)[a]
        q = random.Random(seed * 3 + a)
        pts, rot = [], q.uniform(0, math.pi)
        for j in range(18):
            an = rot + math.pi * j / 9 + q.uniform(-0.1, 0.1)
            rr = R * (0.1 if j % 2 else q.uniform(0.3, 0.7)) * (0.7 + 0.5 * a) * (1.3 if big else 1.0)
            pts.append((rr * math.cos(an), rr * math.sin(an)))
        out.append(dict(k="poly", pts=pts, m="shape", a=g, s=3.2, h=1.0, y=-0.12))
    # its heart: dark, smoky, jagged (it darkens what it swallows)
    if 1 <= a < life - 2:
        q = random.Random(seed * 7 + a // 2)
        rr = R * 0.78 * grow
        n = 30
        pts = [(rr * (1.0 + q.uniform(-0.14, 0.14)) * math.cos(TAU * i / n),
                rr * (0.9 + q.uniform(-0.14, 0.14)) * math.sin(TAU * i / n)) for i in range(n)]
        out.append(dict(k="poly", pts=pts, m="void", a=0.85 * fade * min(1.0, a / 2.0), s=0.3, h=0.0, y=0.08))
    # the whirl: thick ribbons on orbits of every tilt, white-hot heads, smoky tails
    nr = 12 if big else 10
    for j in range(nr):
        tilt = rng.uniform(0, math.pi)
        sy = rng.uniform(0.18, 0.62)
        Rj = R * rng.uniform(0.75, 1.08) * grow
        om = rng.uniform(0.32, 0.46) * (1 if j % 3 else -1)
        h0 = rng.uniform(0, TAU) + om * a
        span = rng.uniform(1.7, 2.6) * (0.55 + 0.45 * min(1.0, a / 3.0))
        W = rng.uniform(0.2, 0.34) * (1.2 if big else 1.0)
        n = 22
        ct, st = math.cos(tilt), math.sin(tilt)
        pts, ws = [], []
        for i in range(n + 1):
            u = i / n
            an = h0 - math.copysign(span * u, om)
            x, z = Rj * math.cos(an), Rj * sy * math.sin(an)
            pts.append((x * ct - z * st, x * st + z * ct))
            ws.append(W * fade ** 0.5 * (min(1.0, u / 0.08) ** 0.5) * (1.0 - u) ** 0.7 + 0.004)
        k = fade * (1.0 if a > 0 else 0.6)
        out.append(dict(k="rib", pts=pts, w=ws, m="veil", a=0.85 * k, s=1.6, h=0.5, y=-0.02 - 0.003 * j))
        out.append(dict(k="rib", pts=pts, w=[x * 1.6 for x in ws], m="swirl", a=0.7 * k, s=1.2, h=1.0,
                        y=-0.015 - 0.003 * j))  # its smoky, burning fringe
        hot = pts[: int(n * 0.55)]
        out.append(dict(k="rib", pts=hot, w=[x * 0.3 for x in ws[: len(hot)]], m="veil", a=k, s=2.4, h=0.9,
                        y=-0.03 - 0.003 * j))
        head = pts[: n // 2]  # its razor edge, white-hot
        out.append(dict(k="rib", pts=head, w=[0.02 * (1.0 - i / len(head)) + 0.003 for i in range(len(head))],
                        m="line", a=k, s=3.0, h=1.0, y=-0.05 - 0.003 * j))
    # cuts flickering in it, two a frame for its first frames
    for i in range(6 if big else 4):
        b = rng.randint(0, 5)
        ang, off = rng.uniform(0, math.pi), (rng.uniform(-0.4, 0.4) * R, rng.uniform(-0.35, 0.35) * R)
        L, w = rng.uniform(1.2, 2.0) * R, rng.uniform(0.025, 0.04)
        ag = a - b
        if 0 <= ag < 3:
            out += slash(off, ang, L * (0.7, 1.0, 1.05)[ag], w * (1.3, 1.0, 0.6)[ag], (1.0, 0.8, 0.45)[ag],
                         (0.8, 0.55, 0.35)[ag], -0.08 - 0.001 * i)
    # red lightning crackling in it and leaping out: new paths every frame
    q = random.Random(seed * 13 + a)
    for i in range(int((4 if big else 3) * fade + 0.5)):  # from the whirl's edge, leaping out
        an = q.uniform(0, TAU)
        p0 = (0.7 * R * grow * math.cos(an), 0.6 * R * grow * math.sin(an))
        bolt(out, p0, an + q.uniform(-0.6, 0.6), R * q.uniform(0.5, 0.9), q, 0.95 * fade, 1.2 if big else 1.0)
    # sparks flung off the whirl
    for i in range(16):
        an = rng.uniform(0, TAU)
        b = rng.randint(1, 6)
        sp = rng.uniform(0.05, 0.12)
        lf = rng.randint(5, 9)
        ag = a - b
        if 0 <= ag < lf:
            k = 1.0 - ag / lf
            d = R * 0.8 + sp * ag
            c = (d * math.cos(an), d * math.sin(an) - 0.003 * ag * ag)
            Ls = rng.uniform(0.06, 0.12) * k
            out.append(dict(k="poly", pts=place(shard(Ls, Ls * 0.25), c, an + 1.3), m="solid", a=k, s=2.2,
                            h=0.6 + 0.4 * k, y=-0.1))


def jugement(f):
    out = []
    vortex(out, f, VBORN[0], 101, 1.05)
    vortex(out, f, VBORN[1], 202, 1.15)
    vortex(out, f, VBORN[2], 303, 1.3, big=True)
    a = f - VBORN[2]
    q = random.Random(11)
    if 0 <= a < 13:  # the last one bursts: long cuts through it, white then breaking apart
        for i in range(3):
            ang = (0.6, 2.5, 1.55)[i] + q.uniform(-0.1, 0.1)
            L = q.uniform(2.8, 3.4)
            if a < 3:
                out += slash((0.0, 0.0), ang, L * (0.8, 1.0, 1.05)[a], (0.09, 0.065, 0.045)[a], (1.0, 0.95, 0.8)[a],
                             (1.0, 0.85, 0.65)[a], -0.14 - 0.001 * i)
            else:
                kk = max(0.0, 1.0 - (a - 3) / 8.0)
                if kk <= 0:
                    continue
                for side in (-1, 1):
                    for pc in range(3):
                        c0 = side * (0.2 + pc * 0.45 + 0.04 * (a - 3))
                        mid = (c0 * L / 2.6 * math.cos(ang), c0 * L / 2.2 * math.sin(ang))
                        out += slash(mid, ang, L / 4.2 * kk ** 0.5, 0.03 * kk, kk, 0.35 + 0.25 * kk, -0.14 - 0.001 * i)
    if 0 <= a < 14:  # its smoke torn into dark shards rimmed with fire, lightning leaping far
        for i in range(16):
            ang = q.uniform(0, TAU)
            sp = q.uniform(0.07, 0.15)
            life = q.randint(9, 13)
            if a < life:
                k = 1.0 - a / life
                d = 0.5 + sp * a * (1.0 - a / 30.0)
                c = (d * math.cos(ang), d * math.sin(ang) - 0.004 * a * a)
                L = q.uniform(0.14, 0.28)
                out.append(dict(k="poly", pts=place(shard(L, L * q.uniform(0.35, 0.6)), c, ang + a * q.uniform(-0.3, 0.3)),
                                m="rock", a=k, s=1.4, h=0.55 + 0.4 * k, y=-0.13))
        if a < 5:
            r = random.Random(500 + a)
            for i in range(7):
                an = TAU * i / 7 + r.uniform(-0.3, 0.3)
                bolt(out, (0.3 * math.cos(an), 0.3 * math.sin(an)), an, r.uniform(1.1, 1.6), r,
                     (1.0, 0.9, 0.7, 0.45, 0.2)[a], 1.4, 0.8, -0.15)
    if f < 3:  # the click: a hairline flash across the spot as the draw arrives
        out += slash((0.0, 0.0), -0.12, 3.0, 0.016, (1.0, 0.7, 0.3)[f], 1.0, -0.16)
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
