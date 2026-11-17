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


def tongue(site, L, W, ph, lives, t, lean=0.0, hook=0.0, out=0.6, turn=1.6, wave=0.12, g=(0.0, 1.0), m="flame",
           a=1.0, s=1.0, y=0.0, inset=0.03, heats=(0.3, 0.62, 0.95), core=True, wisp_m=None):
    """One flame on the outline: it rises (a quarter of its life), sways, pinches off at 55 % of its height (the top
    piece flies off along g and fades) and sinks back into the body. g = (0, -1): it falls (frost)."""
    x, z, nx, nz = site
    e = frac(t * lives + ph)
    grow = sm(0.0, 0.3, e)
    base = (x - nx * inset, z - nz * inset)
    d0 = unit(nx * out + g[0], nz * out + g[1])
    n = 16
    P = spine(base, d0, L * (0.3 + 0.7 * grow), n, turn, g, lean, wave, e + ph, 1.3, hook)
    neck = 0.55
    pinch = sm(0.48, 0.68, e)
    shapes = []

    def prof(i, end=1.0):
        q = i / n / end
        return min(1.0, q / 0.12) ** 0.5 * max(0.0, 1.0 - q) ** 0.85

    def layers(pts, ws, alpha, deep_only=False):
        for j, (kw, kl) in enumerate(((1.0, 1.0), (0.6, 0.8), (0.3, 0.5))):
            if (j == 2 and not core) or (deep_only and j == 2):
                continue
            cut = max(2, round((len(pts) - 1) * kl)) + 1
            shapes.append(rib(pts[:cut], [w * kw for w in ws[:cut]], m if j == 0 or not wisp_m else m, alpha * a,
                              s, heats[j], y - 0.002 * j))

    wmul = W * (0.55 + 0.45 * grow)
    if e < 0.68:
        ws = [wmul * prof(i) * (1.0 - pinch * math.exp(-((i / n - neck) / 0.07) ** 2)) for i in range(n + 1)]
        layers(P, ws, 1.0)
        return shapes
    f = (e - 0.68) / 0.32
    cut = round(n * neck)
    end = max(0.08, neck * (1.0 - f) ** 0.7)  # the root sinks back
    low = P[:max(2, round(n * end)) + 1]
    layers(low, [wmul * prof(i, end) for i in range(len(low))], 1.0)
    up = P[cut:]
    dz = L * 0.9 * f ** 1.1  # the top piece flies on
    cx, cz = up[len(up) // 2]
    k = 1.0 - 0.55 * f
    up = [(cx + (px - cx) * k + g[0] * dz, cz + (pz - cz) * k + g[1] * dz) for px, pz in up]
    m2 = len(up) - 1
    ws = [wmul * k * min(1.0, (i / m2) / 0.3) ** 0.6 * (1.0 - i / m2) ** 0.9 for i in range(m2 + 1)]
    for j, kw in enumerate((1.0, 0.55)):
        shapes.append(rib(up, [w * kw for w in ws], wisp_m or m, a * (1.0 - f) ** 1.3, s, heats[j], y - 0.002 * j))
    return shapes


def ground(who, n, rng):
    """Sites at the soles of the fighter's feet: flames licking up round them (not in a row between them)."""
    pool = [p for p in SIL[who]["edge"] if p[1] < 0.12]
    got = []
    for _ in range(2000):
        if len(got) == n or not pool:
            break
        p = rng.choice(pool)
        if all(abs(p[0] - q[0]) > 0.07 for q in got):
            got.append((p[0], 0.0, 0.0, 1.0))
    return got


def flames(who, t, n, lives, rng, L, W, lean, out=0.6, turn=1.6, wave=0.12, g=(0.0, 1.0), feet=6, a=1.0, s=1.0,
           heats=(0.3, 0.62, 0.95), weight=None, hooks=0.45, core=True, major=4, avoid=None, kmaj=1.8):
    """n flames on the outline (and `feet` on the ground): the `major` ones high on the body half as big again, the
    rest varied (never a comb of equal tongues); avoid(x, z): keep clear (a horn's place)."""
    up = g[1] > 0
    weight = weight or (lambda x, z, nx, nz: (0.0 if (nz < -0.35 if up else nz > 0.35) or z < 0.08 or
                                              (avoid and avoid(x, z)) else
                                              0.35 + max(0.0, nz if up else -nz) + 0.6 * abs(nx)))
    S = sites(who, n, rng, weight) + (ground(who, feet, rng) if up else [])
    top = SIL[who]["top"]
    high = [i for i, p in enumerate(S) if p[1] > 0.55 * top]
    big = set(rng.sample(high, min(major, len(high))))
    shapes = []
    for i, site in enumerate(S):
        hi = sm(0.2, 1.0, site[1] / top)  # taller up the body; on the ground low and wide
        low = site[1] == 0.0
        kL = 0.5 if low else (0.75 + 0.45 * hi) * (kmaj if i in big else rng.uniform(0.7, 1.05))
        kW = 1.4 if low else (1.3 if i in big else 1.0)
        shapes += tongue(site, rng.uniform(*L) * kL, rng.uniform(*W) * kW, rng.random(), rng.choice(lives),
                         t, lean * rng.uniform(0.6, 1.3), rng.uniform(-hooks, hooks), out, turn, wave, g, "flame",
                         a, s, 0.0005 * i, heats=tuple(h * 0.7 for h in heats) if low else heats, core=core)
    return shapes


def embers(who, t, n, rng, rise=0.5, size=(0.012, 0.022), lives=(1, 2), h=0.95, drift=0.06, a=1.0):
    S = sites(who, n, rng, lambda x, z, nx, nz: 0.2 + max(0.0, nz), gap=0.03)
    out = []
    for i, (x, z, nx, nz) in enumerate(S):
        ph, lv, r, w = rng.random(), rng.choice(lives), rng.uniform(*size), rng.uniform(1.5, 3.5)
        e = frac(t * lv + ph)
        px = x + nx * 0.05 + drift * math.sin(TAU * (w * e + ph))
        out.append(dict(k="dot", c=(px, z + nz * 0.05 + rise * e), r=r * (1.0 - 0.6 * e), m="solid",
                        a=a * math.sin(math.pi * e) ** 0.7, s=1.6, h=h, y=-0.02 - 0.0003 * i))
    return out


def star(n, ro, ri, rot=0.0):
    return [((ro if k % 2 == 0 else ri) * math.cos(rot + math.pi * k / n),
             (ro if k % 2 == 0 else ri) * math.sin(rot + math.pi * k / n)) for k in range(2 * n)]


def place(poly, at, rot=0.0, k=1.0):
    c, s = math.cos(rot), math.sin(rot)
    return [(at[0] + k * (x * c - z * s), at[1] + k * (x * s + z * c)) for x, z in poly]


def twinkles(who, t, n, rng, size=(0.04, 0.07), lives=(2, 3), h=1.0, reach=0.18):
    """Four-point sparkles flashing near the outline (not a halo of dots)."""
    S = sites(who, n, rng, lambda x, z, nx, nz: 0.3 + max(0.0, nz), gap=0.1)
    out = []
    for i, (x, z, nx, nz) in enumerate(S):
        ph, lv, r, d = rng.random(), rng.choice(lives), rng.uniform(*size), rng.uniform(0.04, reach)
        e = frac(t * lv + ph)
        k = sm(0.0, 0.15, e) * (1.0 - sm(0.15, 0.45, e))
        if k > 0.01:
            out.append(dict(k="poly", pts=place(star(4, r, r * 0.16), (x + nx * d, z + nz * d), 0.0, k),
                            m="shape", a=1.0, s=1.8, h=h, y=-0.03 - 0.0003 * i))
    return out


def shard(L, w):
    return [(L * 0.5, 0.0), (0.0, w * 0.5), (-L * 0.5, 0.0), (0.0, -w * 0.5)]


# --- the auras ---------------------------------------------------------------------------------------------------
def aura_p01(t):
    """ÉVEIL DU NÉANT (Ombre violette): violet flames hugging him, rising and curling, their tips going to dark
    smoke (ref. T01: tongues from the shoulders, arms and coat tails)."""
    r = random.Random(101)
    return flames("p01", t, 36, (2,), r, (0.26, 0.68), (0.055, 0.12), lean=-0.1, out=1.0, feet=6, a=0.95,
                  heats=(0.22, 0.55, 0.9), hooks=0.6, major=5)


def aura_p02(t):
    """GIVRE CÉLESTE (Givre stellaire): veils of frost falling from the shoulders, the arms and the hair down to the
    ground, crystals of ice floating up round the head, sparkles (ref. T02)."""
    r = random.Random(202)
    out = []
    S = sites("p02", 20, r, lambda x, z, nx, nz: 0.0 if z < 0.5 or nz < -0.3 else 0.4 + abs(nx) + max(0.0, nz),
              gap=0.06)
    for i, site in enumerate(S):
        x, z, nx, nz = site
        L = min(r.uniform(0.45, 0.95), 0.9 * (z - 0.04))
        ph = r.random()
        e = frac(t + ph)
        W = r.uniform(0.08, 0.16) * (0.85 + 0.15 * math.sin(TAU * (t + ph)))
        P = spine((x - nx * 0.03, z - nz * 0.03), unit(nx * 0.9, -0.6), L, 16, 1.1, (0.0, -1.0), r.uniform(-0.1, 0.1),
                  0.1, -e, 1.0, 0.0)
        ws = [W * math.sin(0.5 * math.pi * min(1.0, j / 16 / 0.3)) ** 0.7 * (1.0 - j / 16) ** 0.6 for j in range(17)]
        out.append(rib(P, ws, "veil", 0.8, 1.0, 0.4, 0.0005 * i))
        out.append(rib(P[:13], [w * 0.45 for w in ws[:13]], "veil", 0.85, 1.2, 0.7, 0.0005 * i - 0.002))
        out.append(rib(P[:9], [w * 0.15 for w in ws[:9]], "veil", 0.9, 1.4, 0.85, 0.0005 * i - 0.004))
        k = frac(2 * t + ph)  # a drop of frost falls from its end
        tx, tz = P[-1]
        out.append(dict(k="poly", pts=place(shard(0.05, 0.016), (tx, tz - 0.22 * k), -math.pi / 2, 1.0 - 0.5 * k),
                        m="shape", a=math.sin(math.pi * k) * 0.9, s=1.4, h=0.8, y=-0.01))
    for i in range(10):  # crystals of ice floating up round the head and shoulders, turning slowly
        ph, x0, z0 = r.random(), r.choice((-1, 1)) * r.uniform(0.25, 0.5), r.uniform(0.95, 1.55)
        e = frac(t + ph)
        L = r.uniform(0.08, 0.14)
        out.append(dict(k="poly", pts=place(shard(L, L * 0.38), (x0 + 0.04 * math.sin(TAU * (e + ph)), z0 + 0.32 * e),
                                            TAU * (ph + 0.25 * e) * r.choice((1, -1)), 1.0), m="shape",
                        a=math.sin(math.pi * e) ** 0.6, s=1.5, h=0.75, y=-0.03 - 0.0003 * i))
    for i in range(5):  # cold mist sinking at the feet, creeping out
        ph, side = r.random(), r.choice((1, -1))
        e = frac(t + ph)
        x0 = side * (0.1 + 0.25 * e)
        P = [(x0 + side * 0.35 * j / 10, 0.03 + 0.03 * math.sin(math.pi * j / 10) + 0.01 * math.sin(TAU * (e + j / 10)))
             for j in range(11)]
        out.append(rib(P, [0.07 * math.sin(math.pi * j / 10) ** 0.8 for j in range(11)], "veil",
                       0.38 * math.sin(math.pi * e), 0.8, 0.35, 0.004))
    return out + twinkles("p02", t, 10, r, size=(0.05, 0.09), h=1.0)


def aura_p03(t):
    """BRASIER DÉMONIAQUE (Brasier rouge): a cloak of red flames streaming back like a cape, two horns of fire on his
    head, embers (ref. T03)."""
    r = random.Random(303)
    hx = [p for p in SIL["p03"]["edge"] if p[1] > SIL["p03"]["top"] - 0.1]
    cx, cz = sum(p[0] for p in hx) / len(hx), SIL["p03"]["top"] - 0.08
    out = flames("p03", t, 32, (3,), r, (0.22, 0.62), (0.06, 0.13), lean=-0.32, out=0.5, turn=2.0, feet=7,
                 heats=(0.12, 0.4, 0.78), major=5, avoid=lambda x, z: z > cz - 0.2 and abs(x - cx) < 0.2)
    for i, (dx, d0, hook) in enumerate(((0.05, (0.35, 1.0), -0.55), (-0.07, (-0.25, 1.0), -0.35))):
        fl = 1.0 + 0.08 * math.sin(TAU * (3 * t + i * 0.3)) + 0.05 * math.sin(TAU * (7 * t + i))
        P = spine((cx + dx, cz), unit(*d0), 0.34 * fl, 14, 1.2, (-0.6, 1.0), 0.0, 0.04, 2 * t + i * 0.5, 1.0, hook)
        ws = [0.09 * min(1.0, j / 14 / 0.15) ** 0.5 * (1.0 - j / 14) ** 1.2 for j in range(15)]
        for j, (kw, kl) in enumerate(((1.0, 15), (0.5, 11), (0.22, 7))):
            out.append(rib(P[:kl], [w * kw for w in ws[:kl]], "flame", 1.0, 1.0, (0.2, 0.5, 0.82)[j],
                           -0.006 - 0.002 * j))
    return out + embers("p03", t, 10, r, rise=0.55, size=(0.008, 0.015), lives=(1, 2, 3), h=0.75, a=0.8)


def aura_p08(t):
    """ANGE BLANC (Lumière blanche): veils of white light rising slowly, swaying, their tips drifting off like
    feathers, a few sparkles (ref. T08: white-blue)."""
    r = random.Random(808)
    out = flames("p08", t, 22, (1,), r, (0.25, 0.6), (0.08, 0.16), lean=0.0, out=1.0, turn=1.0, wave=0.2,
                 feet=4, a=0.75, heats=(0.18, 0.55, 0.9), hooks=0.3, major=4, kmaj=1.3)
    return out + twinkles("p08", t, 7, r, size=(0.045, 0.08), lives=(1, 2))


def aura_h01(t):
    """Hakuen awakened (Volonté de la lame): sharp flames of his awakened blade and red lightning crackling over
    him (his ultimate's): each bolt crawls along his outline, just outside it, with a small fork."""
    r = random.Random(1)
    out = flames("h01", t, 28, (2,), r, (0.2, 0.55), (0.05, 0.1), lean=-0.15, out=0.55, turn=1.8, wave=0.08,
                 feet=5, hooks=0.4, heats=(0.2, 0.5, 0.85), major=4, kmaj=1.5)
    S = sites("h01", 12, r, lambda x, z, nx, nz: 0.0 if z < 0.15 else 1.0, gap=0.12)
    N = 36
    f = round(t * N) % N
    for i, site in enumerate(S):  # each bolt flashes for two frames once a loop
        b = (i * 3) % N
        if (f - b) % N < 2:
            q = random.Random(i * 7 + (f - b) % N)
            P = crawl("h01", site, q.randint(8, 14), q.choice((1, -1)))
            P = [(x + nx * o, z + nz * o) for (x, z, nx, nz), o in zip(P, [q.uniform(0.02, 0.05) for _ in P])]
            pts = [P[0]]
            for u, v in zip(P, P[1:]):
                pts += jag(u, v, 2, 0.35, q)[1:]
            a = 1.0 if (f - b) % N == 0 else 0.55
            out.append(dict(k="bolt", pts=pts, w=0.6, a=a, s=1.0, h=0.9, y=-0.03))
            m = pts[len(pts) // 2]  # a small fork, outward
            x, z, nx, nz = site
            ang = math.atan2(nz, nx) + q.uniform(-0.7, 0.7)
            d = q.uniform(0.07, 0.13)
            out.append(dict(k="bolt", pts=jag(m, (m[0] + d * math.cos(ang), m[1] + d * math.sin(ang)), 3, 0.3, q),
                            w=0.45, a=a * 0.8, s=1.0, h=0.9, y=-0.03))
    return out


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


AURAS = dict(p01=(aura_p01, 36), p02=(aura_p02, 48), p03=(aura_p03, 36), p08=(aura_p08, 48), h01=(aura_h01, 36))


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
           h=0.55, m="rock", lift=(0.35, 1.2))
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
           h=0.6, m="rock", lift=(0.35, 1.25))
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
