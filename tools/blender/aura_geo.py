# The awakened forms' auras and their bursts (yoyo, 12/11: every effect in Blender), after the owner's sheets
# docs/refonte/ref_transformations.png (T01, T02, T03, T08) and ref_effets.png (« ondes de choc »: T01 onde 5, T02
# onde 3, T03 onde 2, T08 onde 1). Pure Python: Blender (effets.py) builds these shapes and the concept sheets draw
# them. Units: 1 = 100 px, the feet at the origin, x ahead (facing right), z up.
# An aura loops on N frames, drawn behind its fighter (a fighter 1.7 tall): every flame lives a whole number of times
# per loop. Flames are born on the outline (silhouettes.json), rise, pinch off at the top (the piece flies up and
# fades) and sink back. A burst plays once (N frames), for a fighter H tall (its strip is drawn 1.6 times bigger).
# Never a whole ring, a ball or a cone: rings are broken tapered arcs, flashes uneven spikes.
# A shape is a dict: k = "rib" (a ribbon along pts, w wide at each point; u runs from its first point, the
# hottest), "poly" (a filled outline), "dot" (a disc: c, r) or "bolt" (lightning along pts); m: its material (flame,
# veil, line, solid, shape), a: alpha, s: strength, h: heat (0 its deep colour .. 1 white-hot), y: depth (smaller is
# nearer), front: drawn over the fighter (bursts only).
import json
import math
import os
import random

HERE = os.path.dirname(os.path.abspath(__file__))
SIL = json.load(open(os.path.join(HERE, "aura", "silhouettes.json")))
TAU = 2 * math.pi
# The forms' colours: deep, main, hot, core, smoke (None: none). T03 redder than 03's other effects (its sheet: a
# crimson cloak), T08 white-blue (its sheet) where 08's other effects are gold, Hakuen's the blood red of his
# ultimate with the embers of his awakened blade.
PAL = {
    "p01": ("#3B1772", "#8A4DFF", "#C9A2FF", "#F7EEFF", "#140A20"),
    "p02": ("#1E5C9E", "#5CC8F0", "#D8F6FF", "#FFFFFF", None),
    "p03": ("#8E0B0B", "#E8261A", "#FF8A3D", "#FFE2B0", "#1E0F0B"),
    "p08": ("#3E6FD8", "#A9D4FF", "#EAF5FF", "#FFFFFF", None),
    "h01": ("#7A0E1A", "#E8343C", "#FF8A4C", "#FFE9D6", "#1A0A0C"),
    "h01b": ("#6E0A16", "#D9332E", "#F2B33D", "#FFF2C4", None),  # her long odachi: crimson lacquer, gold star guard
}


def sm(a, b, x):
    t = min(1.0, max(0.0, (x - a) / (b - a)))
    return t * t * (3 - 2 * t)


def unit(x, z):
    L = math.hypot(x, z) or 1.0
    return x / L, z / L


def frac(x):
    return x - math.floor(x)


def rib(pts, w, m, a, s, h, y, front=False):
    return dict(k="rib", pts=pts, w=w, m=m, a=a, s=s, h=h, y=y, front=front)


def sites(who, n, rng, weight, gap=0.07):
    """n points of the outline (x, z, nx, nz), at least gap apart, drawn by weight(x, z, nx, nz)."""
    pool = [p for p in SIL[who]["edge"] if weight(*p) > 0]
    ws = [weight(*p) for p in pool]
    got = []
    for _ in range(6000):
        if len(got) == n:
            break
        p = rng.choices(pool, ws)[0]
        if all((p[0] - q[0]) ** 2 + (p[1] - q[1]) ** 2 > gap * gap for q in got):
            got.append(p)
    return got


def spine(base, d0, L, n=14, turn=1.5, g=(0.0, 1.0), lean=0.0, wave=0.0, ph=0.0, k=1.3, hook=0.0):
    """A flame's middle line: it leaves base along d0 and turns toward g (turn: how soon), bent sideways by lean,
    rippled by a wave running along it (ph: its phase, k: waves along it), its tip curled by hook."""
    x, z = base
    P = [(x, z)]
    for i in range(1, n + 1):
        b = min(1.0, turn * i / n)
        dx, dz = unit(d0[0] * (1 - b) + g[0] * b, d0[1] * (1 - b) + g[1] * b)
        x, z = x + dx * L / n, z + dz * L / n
        P.append((x, z))
    out = []
    for i, (x, z) in enumerate(P):
        s = i / n
        a, b = P[max(i - 1, 0)], P[min(i + 1, n)]
        nx, nz = unit(-(b[1] - a[1]), b[0] - a[0])
        o = L * (lean * s * s + wave * s * math.sin(TAU * (ph - k * s)) + hook * max(0.0, s - 0.6) ** 2 * 3.0)
        out.append((x + nx * o, z + nz * o))
    return out


def star(n, ro, ri, rot=0.0):
    return [((ro if k % 2 == 0 else ri) * math.cos(rot + math.pi * k / n),
             (ro if k % 2 == 0 else ri) * math.sin(rot + math.pi * k / n)) for k in range(2 * n)]


def place(poly, at, rot=0.0, k=1.0):
    c, s = math.cos(rot), math.sin(rot)
    return [(at[0] + k * (x * c - z * s), at[1] + k * (x * s + z * c)) for x, z in poly]


def shard(L, w):
    return [(L * 0.5, 0.0), (0.0, w * 0.5), (-L * 0.5, 0.0), (0.0, -w * 0.5)]


# --- the auras ---------------------------------------------------------------------------------------------------
# yoyo, 17/11: no more flames round the body. The burst carries the show; the aura that stays is quiet and sharp:
# slow motes drifting up, tiny lightning crackling on the outline (refs: Dragon Ball FighterZ's Super Saiyan 2 sparks,
# Guilty Gear Strive's install glows, Solo Leveling's violet aura). The rim light hugging the body is the game's
# (it follows the animation): this strip never draws the body's shape.
AN = 72  # 2.4 s a loop: the motes are slow


def motes(who, t, n, rng, rise, size, h, drift=0.05, star_k=0.0, a=1.0, flick=0.0):
    """n motes drifting up from round the body, each once a loop, slowly, swaying; star_k: the share drawn as tiny
    four-point glints (frost, light) instead of dots; flick: an ember's flicker."""
    S = sites(who, n, rng, lambda x, z, nx, nz: 0.25 + max(0.0, nz) + 0.5 * abs(nx), gap=0.025)
    out = []
    for i, (x, z, nx, nz) in enumerate(S):
        ph, r, off, w = rng.random(), rng.uniform(*size), rng.uniform(0.02, 0.16), rng.uniform(0.6, 1.4)
        glint, fp = rng.random() < star_k, rng.random()
        e = frac(t + ph)
        px = x + nx * off + drift * math.sin(TAU * (w * e + ph))
        pz = z + nz * off * 0.5 + rise * e
        k = math.sin(math.pi * e) ** 0.8 * (1.0 - flick * 0.5 * (1 + math.sin(TAU * (9 * t + 3 * fp))))
        if k < 0.02:
            continue
        if glint:
            out.append(dict(k="poly", pts=place(star(4, r * 2.6, r * 0.45), (px, pz), TAU * fp, 0.6 + 0.4 * k),
                            m="shape", a=a * k, s=1.8, h=h, y=-0.02 - 0.0003 * i))
        else:
            out.append(dict(k="dot", c=(px, pz), r=r * (0.7 + 0.3 * k), m="solid", a=a * k, s=1.8, h=h,
                            y=-0.02 - 0.0003 * i))
    return out


def crackles(who, t, n, seed, every=3, w=1.1, h=0.9, reach=(0.02, 0.05), steps=(4, 8)):
    """Tiny lightning: one crackle every `every` frames somewhere on the outline, three frames each (bright, then
    fading, re-jagged every frame so it crackles), a short run just outside the body, often a fork leaping out."""
    f = round(t * AN) % AN
    out = []
    for i in range(n):
        b = (i * every) % AN
        age = (f - b) % AN
        if age > 2:
            continue
        q = random.Random(seed * 1000 + i)  # its own draws: the same bolt on both of its frames
        site = q.choice([p for p in SIL[who]["edge"] if p[1] > 0.12])
        P = crawl(who, site, q.randint(*steps), q.choice((1, -1)))
        P = [(x + nx * o, z + nz * o) for (x, z, nx, nz), o in zip(P, [q.uniform(*reach) for _ in P])]
        if len(P) < 2:
            continue
        j = random.Random(seed * 1000 + i + 77 * age)  # its path kept, its zigzag new each frame
        pts = [P[0]]
        for u, v in zip(P, P[1:]):
            pts += jag(u, v, 2, 0.5, j)[1:]
        a = (1.0, 0.6, 0.3)[age]
        out.append(dict(k="bolt", pts=pts, w=w, a=a, s=1.0, h=h, y=-0.03))
        if q.random() < 0.65:  # a fork leaping off the body
            m = pts[q.randrange(len(pts))]
            x, z, nx, nz = site
            ang = math.atan2(nz, nx) + q.uniform(-0.8, 0.8)
            d = q.uniform(0.06, 0.13)
            out.append(dict(k="bolt", pts=jag(m, (m[0] + d * math.cos(ang), m[1] + d * math.sin(ang)), 3, 0.35, j),
                            w=w * 0.7, a=a * 0.8, s=1.0, h=h, y=-0.03))
    return out


def aura_p01(t):
    """ÉVEIL DU NÉANT: violet motes rising like ash of shadow, violet crackles (Solo Leveling's purple)."""
    r = random.Random(101)
    return motes("p01", t, 30, r, 0.45, (0.016, 0.036), 0.5, a=0.95) + crackles("p01", t, 36, 1, every=2, h=0.85)


def aura_p02(t):
    """GIVRE CÉLESTE: frost glints and motes floating up slowly, ice-blue crackles, sparser."""
    r = random.Random(202)
    return (motes("p02", t, 28, r, 0.35, (0.015, 0.032), 0.7, drift=0.07, star_k=0.45) +
            crackles("p02", t, 18, 2, every=4, h=0.95))


def aura_p03(t):
    """BRASIER DÉMONIAQUE: embers flickering up a little faster, red-orange crackles."""
    r = random.Random(303)
    return (motes("p03", t, 32, r, 0.6, (0.015, 0.032), 0.65, drift=0.04, flick=0.6) +
            crackles("p03", t, 36, 3, every=2, h=0.8))


def aura_p08(t):
    """ANGE BLANC: motes and glints of white light rising very slowly, pale blue crackles."""
    r = random.Random(808)
    return (motes("p08", t, 26, r, 0.3, (0.017, 0.034), 0.8, drift=0.06, star_k=0.35) +
            crackles("p08", t, 18, 8, every=4, h=1.0))


def aura_h01(t):
    """Hakuen awakened: embers of his blade rising, red lightning crackling over him (his ultimate's), denser."""
    r = random.Random(1)
    return (motes("h01", t, 28, r, 0.55, (0.015, 0.032), 0.65, drift=0.04, flick=0.5) +
            crackles("h01", t, 36, 4, every=2, w=1.0, h=0.9))


def crawl(who, p, steps, sgn):
    """A path along the outline from edge point p: each step to the nearest point ahead (the points are some
    0.03 apart, unordered)."""
    E = SIL[who]["edge"]
    path = [p]
    for _ in range(steps):
        tx, tz = -p[3] * sgn, p[2] * sgn
        best, bd = None, 1e9
        for e in E:
            dx, dz = e[0] - p[0], e[1] - p[1]
            d = math.hypot(dx, dz)
            if 0.015 < d < 0.08 and dx * tx + dz * tz > 0.5 * d and d < bd:
                best, bd = e, d
        if best is None:
            break
        path.append(best)
        p = best
    return path


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


def petals(who, t, n, rng, size, h, top=1.6, a=1.0):
    """n flakes of crimson lacquer and gold leaf falling slowly round the body, each once a loop, tumbling (their
    width beats as they turn), swaying: the opposite way to the awakening's rising motes."""
    out = []
    for i in range(n):
        ph, L, x0 = rng.random(), rng.uniform(*size), rng.uniform(-0.55, 0.45)
        spin, sway, hot = rng.uniform(2.0, 4.0) * rng.choice((1, -1)), rng.uniform(0.03, 0.08), rng.random() < 0.4
        e = frac(t + ph)
        k = math.sin(math.pi * e) ** 0.7
        if k < 0.03:
            continue
        x = x0 + sway * math.sin(TAU * (e * 1.5 + ph))
        z = top + 0.1 * abs(x0) - (top - 0.02) * e
        turn = abs(math.cos(TAU * spin * e))  # its face to us, then its edge
        poly = shard(L, L * (0.12 + 0.45 * turn))
        out.append(dict(k="poly", pts=place(poly, (x, z), TAU * ph + 0.6 * math.sin(TAU * spin * e * 0.5)),
                        m="solid", a=a * k, s=(0.8 if hot else 0.45), h=(0.72 if hot else h) * (0.85 + 0.15 * turn), y=-0.02 - 0.0003 * i))
    return out


def stars(t, n, rng, size, h, R=(0.32, 0.62), z=(0.2, 1.55)):
    """Its signature: gold eight-point stars (its guard's) blinking round it, each twice a loop: a sharp flash
    growing out of nothing, a slow fade, turning a little; most small, a few large."""
    out = []
    for i in range(n):
        ph, r = rng.random(), rng.uniform(*size) * (2.2 if rng.random() < 0.15 else 1.0)
        side = rng.choice((1, -1))  # her back: past her long hair
        x, zz, rot = side * rng.uniform(*R) - (0.25 if side < 0 else 0.0), rng.uniform(*z), rng.random() * TAU
        e = frac(2 * t + ph)
        if e > 0.45:
            continue
        k = (e / 0.06) if e < 0.06 else (1 - (e - 0.06) / 0.39) ** 1.5
        long_ = star(4, r * 3.2 * (0.6 + 0.4 * k), r * 0.32, rot + 0.3 * e)
        short = star(4, r * 1.5 * (0.6 + 0.4 * k), r * 0.3, rot + math.pi / 4 + 0.3 * e)
        y = -0.025 - 0.0003 * i
        out.append(dict(k="poly", pts=place(long_, (x, zz)), m="shape", a=k, s=2.0, h=h, y=y))
        out.append(dict(k="poly", pts=place(short, (x, zz)), m="shape", a=k * 0.8, s=2.0, h=h * 0.9, y=y - 0.0001))
        out.append(dict(k="dot", c=(x, zz), r=r * 0.55 * k, m="solid", a=k, s=2.2, h=1.0, y=y + 0.0002))
    return out


def aura_h01b(t):
    """Hakuen holding her long odachi (weapon B): crimson and gold-leaf flakes falling slowly, gold eight-point stars
    blinking round her, no lightning (that is her awakening's). The rim light: the game's, gold."""
    r = random.Random(11)
    return petals("h01", t, 20, r, (0.07, 0.11), 0.33) + stars(t, 10, r, (0.024, 0.038), 0.92)


AURAS = {k: (v, AN) for k, v in dict(p01=aura_p01, p02=aura_p02, p03=aura_p03, p08=aura_p08, h01=aura_h01,
                                     h01b=aura_h01b).items()}


# --- the bursts (played once as the form begins, behind the fighter; their near halves over it) ----------------
H = 1.0625  # the fighter's height in a burst strip (drawn 1.6 times bigger: 170 px / 1.6)
BN = 24


def ring(out, rng, f, born, life, z, R0, R1, w, sy=0.28, pieces=5, gap=0.35, spin=0.0, h=0.7, s=1.6, tilt=0.0,
         bands=1):
    """A broken ring flying out (thin tapered arcs, gaps between them), its near half over the fighter; bands > 1:
    each arc a band of thinner arcs (a whirl, the planche's onde 1)."""
    q = random.Random(rng.random())  # drawn every frame, so what follows keeps its own randomness
    k = (f - born) / life
    if not 0 <= k < 1:
        return
    R = R0 + (R1 - R0) * (1 - (1 - k) ** 2)
    span = TAU / pieces
    c, sn = math.cos(tilt), math.sin(tilt)
    for j in range(pieces):
        a0 = j * span + q.uniform(0, gap) * span + spin * k
        a1 = a0 + span * (1 - gap) * q.uniform(0.75, 1.0)
        for b in range(bands):
            Rb, lo, hi = R * (1 - 0.07 * b), a0 + 0.1 * b, a1 - 0.05 * b
            for half in (0, 1):  # far half behind, near half in front
                pts, ws = [], []
                for i in range(25):
                    a = lo + (hi - lo) * i / 24
                    if (math.sin(a) >= 0) != (half == 0):
                        continue
                    x, zz = Rb * math.cos(a), Rb * sy * math.sin(a)
                    pts.append((x * c - zz * sn, z + x * sn + zz * c))
                    ws.append(w * (1 - 0.35 * b) * (1 - k) ** 0.7 * math.sin(math.pi * i / 24) ** 0.8 + 0.004)
                if len(pts) > 2:
                    out.append(rib(pts, ws, "line", (1.0 - k ** 1.5) * (1 - 0.25 * b), s, h, -0.03,
                                   front=half == 1))


def spike(out, base, ang, Hs, W, f, born, grow, life, h=(0.3, 0.6, 0.92), m="flame", s=1.4, front=False, y=0.0,
          wave=0.07, hook=0.0, ph=0.0, needle=False):
    """A tongue of energy bursting out of the ground: it shoots up in `grow` frames, bending a little, then thins,
    rises on and frays (a flame, not a cone); needle: thickest low, sharp at both ends (a needle of light)."""
    k = (f - born) / life
    if not 0 <= k < 1:
        return
    g = min(1.0, (f - born + 1) / grow)
    Ls = Hs * (0.3 + 0.7 * g) * (1.0 + 0.3 * k)
    d = (math.sin(ang), math.cos(ang))
    P = spine((base[0], base[1] + 0.25 * Hs * k ** 1.5), d, Ls, 14, 0.6, (0.0, 1.0), 0.0, wave, ph + 0.6 * k, 1.1,
              hook)
    if needle:
        prof = [(u / 0.3 if u < 0.3 else (1 - u) / 0.7) ** 0.8 for u in (i / 14 for i in range(15))]
    else:
        prof = [min(1.0, i / 14 / 0.1) ** 0.5 * (1 - i / 14) ** 1.1 for i in range(15)]
    ws = [W * (1 - k) ** 0.9 * p for p in prof]
    for j, (kw, kl) in enumerate(((1.0, 15), (0.55, 11), (0.25, 7))):
        out.append(rib(P[:kl], [w * kw for w in ws[:kl]], m, (1 - k) ** 0.8, s, h[j], y - 0.002 * j, front))


def streaks(out, rng, f, n, x0, x1, z0, z1, born=(0, 4), life=(5, 8), w=(0.008, 0.016), h=0.9, L=(0.3, 0.7)):
    """Thin lines of light shooting up (speed lines), short-lived."""
    for i in range(n):
        b, lf = rng.randint(*born), rng.randint(*life)
        x, Ln = rng.uniform(x0, x1), rng.uniform(*L) * H
        wd, fr = rng.uniform(*w), rng.random() < 0.4
        k = (f - b) / lf
        if 0 <= k < 1:
            z = z0 + (z1 - z0) * k
            P = [(x, z - Ln * j / 8) for j in range(9)]
            out.append(rib(P, [wd * math.sin(math.pi * (0.1 + 0.9 * j / 8)) ** 0.7 for j in range(9)], "line",
                           1.0 - k, 2.0, h, -0.02, fr))


def flash(out, rng, f, at, size, n=7, life=3, h=1.0):
    """The first instant: thin rays of light, broken (not a ball)."""
    q = random.Random(rng.random())  # drawn every frame (see ring)
    if f < life:
        k = f / life
        pts, rot = [], q.uniform(0, math.pi)
        for j in range(2 * n):
            a = rot + math.pi * j / n + q.uniform(-0.12, 0.12)
            rr = size * (0.06 if j % 2 else q.uniform(0.35, 1.0)) * (0.6 + 0.6 * k)
            pts.append((at[0] + rr * math.cos(a), at[1] + rr * math.sin(a)))
        out.append(dict(k="poly", pts=pts, m="shape", a=1.0 - k ** 2, s=2.4, h=h, y=-0.05, front=True))


def chunks(out, rng, f, n, at, speed, born=(1, 3), life=(10, 16), size=(0.025, 0.05), grav=0.012, h=0.3,
           shape="rock", m="shape", lift=(0.15, 1.45)):
    """Bits flung out to both sides (oval stones of energy, the planche's, or shards), falling back and gone into
    the ground; every random draw made each frame (the bits keep their course)."""
    for i in range(n):
        a = rng.uniform(*lift)
        a = a if i % 2 else math.pi - a
        v = speed * rng.uniform(0.5, 1.0)
        b, lf, sz = rng.randint(*born), rng.randint(*life), rng.uniform(*size)
        jit, spin, fr = [rng.uniform(0.7, 1.0) for _ in range(7)], rng.uniform(-0.35, 0.35), rng.random() < 0.5
        rot = a + rng.uniform(-0.3, 0.3)
        k = f - b
        if not 0 <= k < lf:
            continue
        x = at[0] + v * math.cos(a) * k
        z = at[1] + v * math.sin(a) * k - grav * k * k
        if z < 0.0:
            continue
        if shape == "rock":
            poly = [(sz * 1.7 * math.cos(j * TAU / 7) * jit[j], sz * math.sin(j * TAU / 7) * jit[j]) for j in range(7)]
        else:
            poly = shard(sz * 2.2, sz * 0.7)
        out.append(dict(k="poly", pts=place(poly, (x, z), rot + spin * k, 1.0), m=m, a=1.0 - (k / lf) ** 2, s=1.2,
                        h=h, y=-0.01, front=fr))


def eveil_p01(f):
    """Onde 5, « transformation puissante »: a crown of violet flame tongues bursting from the ground all round him
    (taller at his sides), the ground's broken rings, stones of dark energy flung out, speed lines rising."""
    r = random.Random(51)
    out = []
    flash(out, r, f, (0.0, 0.5 * H), 1.0 * H, n=11, life=2, h=0.75)
    for i in range(26):
        a = TAU * i / 26 + r.uniform(-0.08, 0.08)
        R = r.uniform(0.6, 1.15) * H
        x, z = R * math.cos(a), 0.22 * R * math.sin(a)
        side = abs(math.cos(a))
        tall = r.uniform(0.5, 1.0) * H * (0.65 + 0.75 * side) * (1.1 if math.sin(a) > 0 else 0.85)
        spike(out, (x, z), 0.45 * math.cos(a) + r.uniform(-0.12, 0.12), tall, r.uniform(0.13, 0.22) * H, f,
              r.randint(1, 3), 3, r.randint(9, 13), front=math.sin(a) < -0.2, y=0.001 * i,
              hook=r.uniform(-0.35, 0.35), ph=r.random())
    ring(out, r, f, 1, 10, 0.0, 0.3 * H, 1.9 * H, 0.05 * H, sy=0.24, pieces=5)
    ring(out, r, f, 3, 10, 0.0, 0.25 * H, 1.5 * H, 0.035 * H, sy=0.24, pieces=4)
    chunks(out, r, f, 8, (0.0, 0.15 * H), 0.12 * H, born=(2, 4), life=(12, 16), size=(0.035, 0.06), grav=0.006,
           h=0.8, m="rock", lift=(0.35, 1.2))
    streaks(out, r, f, 9, -0.6 * H, 0.6 * H, 0.3 * H, 2.2 * H, L=(0.25, 0.5))
    return out


def eveil_p02(f):
    """Onde 3, « cercles multiples »: broken rings of frost at several heights flying out one after the other, the
    widest high (a bowl, as on the planche, not a cone), a thin column of light, needles of light rising round
    him, sparkles and shards of ice."""
    r = random.Random(52)
    out = []
    flash(out, r, f, (0.0, 0.55 * H), 0.7 * H, h=1.0)
    for i in range(5):  # the column: thin lines of light shooting up through him, not a bar
        b, x, wd = r.randint(0, 2), r.uniform(-0.1, 0.1) * H, r.uniform(0.012, 0.026) * H
        k = (f - b) / r.randint(6, 9)
        if 0 <= k < 1:
            z1 = 2.2 * H * min(1.0, (f - b + 1) / 2)
            P = [(x, z1 * j / 12) for j in range(13)]
            out.append(rib(P, [wd * (1 - k) * math.sin(math.pi * (0.05 + 0.9 * j / 12)) ** 0.6 for j in range(13)],
                           "line", 1.0 - k, 2.2, 0.85, -0.01))
    for j, (z, R1, born) in enumerate(((0.05, 1.15, 0), (0.3, 1.45, 2), (0.62, 1.9, 3), (0.95, 1.55, 5))):
        ring(out, r, f, born, 12, z * H, 0.3 * H, R1 * H, 0.07 * H, sy=0.28, pieces=4, spin=0.6 * (-1) ** j, h=0.55)
    for i in range(10):  # needles of light rising round him
        spike(out, (r.uniform(-1.0, 1.0) * H, r.uniform(0.1, 0.5) * H), 0.0, r.uniform(0.35, 0.8) * H,
              r.uniform(0.03, 0.05) * H, f, r.randint(2, 6), 2, r.randint(6, 9), h=(0.6, 0.85, 1.0), m="line", s=2.0,
              front=r.random() < 0.4, y=0.001 * i, wave=0.0, needle=True)
    for i in range(18):
        b, a, d = r.randint(2, 14), r.uniform(0, TAU), r.uniform(0.3, 1.5) * H
        e = (f - b) / 6
        if 0 <= e < 1:
            k = math.sin(math.pi * e)
            at = (d * math.cos(a), 0.7 * H + 0.6 * d * math.sin(a))
            out.append(dict(k="poly", pts=place(star(4, 0.09 * H, 0.014 * H), at, 0.0, k), m="shape", a=1.0, s=2.0,
                            h=1.0, y=-0.04, front=True))
    chunks(out, r, f, 12, (0.0, 0.4 * H), 0.09 * H, shape="shard", size=(0.03, 0.055), h=0.95)
    return out


def eveil_p03(f):
    """Onde 2, « éclatement vertical »: a great spike of fire shooting straight up out of the ground, a fan of
    flames around it, molten stones and embers thrown out, the ground's broken ring."""
    r = random.Random(53)
    out = []
    flash(out, r, f, (0.0, 0.3 * H), 0.6 * H, life=2, h=0.8)
    hot = (0.12, 0.42, 0.8)
    for i, (dx, ang, tall, wd) in enumerate(((0.0, 0.0, 2.5, 0.42), (-0.12, -0.12, 1.9, 0.3), (0.12, 0.14, 1.8, 0.3),
                                             (-0.22, -0.28, 1.4, 0.24), (0.24, 0.3, 1.3, 0.24), (0.05, 0.05, 2.1, 0.2))):
        spike(out, (dx * H, 0.0), ang, tall * H, wd * H, f, i // 3, 3, 13 - i // 2, h=hot, y=0.002 + 0.001 * i,
              hook=r.uniform(-0.2, 0.2), ph=r.random())
    for i in range(12):
        side = 1 if i % 2 else -1
        ang = side * r.uniform(0.25, 1.1)
        x = side * r.uniform(0.1, 0.55) * H
        spike(out, (x, r.uniform(-0.05, 0.05) * H), ang, r.uniform(0.5, 1.2) * H, r.uniform(0.15, 0.24) * H, f,
              r.randint(1, 3), 3, r.randint(8, 12), h=hot, front=r.random() < 0.4, y=0.01 + 0.001 * i,
              hook=r.uniform(-0.3, 0.3), ph=r.random())
    ring(out, r, f, 1, 9, 0.0, 0.3 * H, 1.6 * H, 0.05 * H, sy=0.24, pieces=4)
    chunks(out, r, f, 12, (0.0, 0.1 * H), 0.13 * H, born=(1, 3), life=(12, 18), size=(0.04, 0.07), grav=0.006,
           h=0.85, m="rock", lift=(0.35, 1.25))
    for i in range(16):  # embers
        b, a, v = r.randint(2, 8), r.uniform(0.3, math.pi - 0.3), r.uniform(0.04, 0.09) * H
        fr = r.random() < 0.5
        k = f - b
        if 0 <= k < 14:
            out.append(dict(k="dot", c=(v * math.cos(a) * k, 0.3 * H + v * math.sin(a) * k * 1.3), r=0.014 * H,
                            m="solid", a=1.0 - k / 14, s=2.0, h=0.85, y=-0.03, front=fr))
    return out


def eveil_p08(f):
    """Onde 1, « énergie circulaire »: two broken rings of light, tilted, whirling out round him (bands of thin
    arcs), needles of light rising inside them, shards thrown out."""
    r = random.Random(54)
    out = []
    flash(out, r, f, (0.0, 0.55 * H), 0.8 * H, h=1.0)
    ring(out, r, f, 0, 13, 0.6 * H, 0.5 * H, 1.55 * H, 0.08 * H, sy=0.3, pieces=6, spin=0.5, tilt=0.12, h=0.6,
         bands=3)
    ring(out, r, f, 1, 13, 0.03 * H, 0.4 * H, 1.85 * H, 0.08 * H, sy=0.25, pieces=5, spin=-0.4, h=0.6, bands=3)
    for i in range(10):
        x = r.uniform(-0.75, 0.75) * H
        spike(out, (x, r.uniform(-0.1, 0.1) * H), r.uniform(-0.08, 0.08), r.uniform(0.6, 1.5) * H,
              r.uniform(0.05, 0.08) * H, f, r.randint(0, 3), 2, r.randint(6, 9), m="line", s=2.0,
              front=r.random() < 0.4, y=0.001 * i, wave=0.0, needle=True)
    chunks(out, r, f, 10, (0.0, 0.1 * H), 0.09 * H, shape="shard", size=(0.03, 0.05), h=0.95)
    return out


EVEILS = dict(p01=eveil_p01, p02=eveil_p02, p03=eveil_p03, p08=eveil_p08)


def bounds(shapes):
    xs, zs = [], []
    for s in shapes:
        if s["k"] == "rib":
            for (x, z), w in zip(s["pts"], s["w"]):
                xs += [x - w, x + w]
                zs += [z - w, z + w]
        elif s["k"] == "dot":
            xs += [s["c"][0] - s["r"], s["c"][0] + s["r"]]
            zs += [s["c"][1] - s["r"], s["c"][1] + s["r"]]
        else:
            xs += [p[0] for p in s["pts"]]
            zs += [p[1] for p in s["pts"]]
    return (min(xs), min(zs), max(xs), max(zs)) if xs else (0, 0, 0, 0)
