# The fighters' outlines for the awakened auras (aura_geo.py): each idle clip's frames merged, at the size the game
# draws the aura (170 px for the fighter's height, 1 unit = 100 px, the feet at the origin, facing right).
# Writes aura/silhouettes.json (points on the outline with their outward normal) and aura/halo_<id>.png (the outline
# blurred: the glow behind the body). Usage: python3 silhouettes.py   (run from anywhere; reads assets/sprites)
import json
import os

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
WHO = ("p01", "p02", "p03", "p08", "h01")
PAD = 24  # px around the frame: room for the blur


def gblur(x, s):
    """Gaussian blur (sigma s px) by FFT, padded so nothing wraps around."""
    p = int(3 * s) + 1
    xp = np.pad(x, p)
    fy = np.fft.fftfreq(xp.shape[0])[:, None]
    fx = np.fft.rfftfreq(xp.shape[1])[None, :]
    g = np.exp(-2 * (np.pi * s) ** 2 * (fx ** 2 + fy ** 2))
    return np.fft.irfft2(np.fft.rfft2(xp) * g, s=xp.shape)[p:-p, p:-p]


def fill(S):
    """S with its holes filled (what the border cannot reach)."""
    out = np.zeros_like(S)
    h, w = S.shape
    todo = [(v, u) for v in range(h) for u in (0, w - 1)] + [(v, u) for u in range(w) for v in (0, h - 1)]
    while todo:
        v, u = todo.pop()
        if 0 <= v < h and 0 <= u < w and not S[v, u] and not out[v, u]:
            out[v, u] = True
            todo += [(v + 1, u), (v - 1, u), (v, u + 1), (v, u - 1)]
    return ~out


def sheet(who):
    for line in open(os.path.join(ROOT, "assets/sprites/sheets.txt")):
        f = line.split()
        if f[:3] == ["sheet", who, "idle"]:
            return int(f[3]), int(float(f[4])), int(float(f[5])), float(f[6]), float(f[7]), float(f[8])
    raise SystemExit("no idle clip for " + who)


def outline(who):
    n, fw, fh, ax, ay, pph = sheet(who)
    im = np.asarray(Image.open(os.path.join(ROOT, "assets/sprites/%s_idle.png" % who)).convert("RGBA"))
    cols = im.shape[1] // fw
    a = np.zeros((fh, fw))
    for i in range(n):  # every frame: the outline holds while it breathes
        c, r = i % cols, i // cols
        a = np.maximum(a, im[r * fh:(r + 1) * fh, c * fw:(c + 1) * fw, 3] / 255.0)
    k = 170.0 / pph
    W, H = round(fw * k), round(fh * k)
    big = np.asarray(Image.fromarray((a > 0.5).astype(np.uint8) * 255).resize((W, H), Image.BILINEAR)) / 255.0
    big = np.pad(big, PAD)
    F = gblur(big, 3.0)  # smoothed: no fingers or strands of hair, gaps under the arms closed
    S = fill(F > 0.5)
    edge = S & ~(np.roll(S, 1, 0) & np.roll(S, -1, 0) & np.roll(S, 1, 1) & np.roll(S, -1, 1))
    G = gblur(big, 7.0)
    gy, gx = np.gradient(G)
    ox, oy = ax * k + PAD, ay * k + PAD  # the feet in the padded image
    unit = lambda u, v: (round((u + 0.5 - ox) / 100, 4), round((oy - v - 0.5) / 100, 4))
    pts = []
    for v, u in zip(*np.nonzero(edge)):
        nx, ny = -gx[v, u], -gy[v, u]
        L = (nx * nx + ny * ny) ** 0.5
        if L < 1e-6:
            continue
        x, z = unit(u, v)
        pts.append((x, z, round(nx / L, 3), round(-ny / L, 3)))
    rng = np.random.default_rng(7)
    keep = []
    for i in rng.permutation(len(pts)):  # thinned: one point every 3 px or so
        p = pts[i]
        if all((p[0] - q[0]) ** 2 + (p[1] - q[1]) ** 2 > 0.03 ** 2 for q in keep):
            keep.append(p)
    keep.sort(key=lambda p: (-p[1], p[0]))
    halo = gblur(big, 3.5)  # narrow: hugging the body, not a blob filling the gaps
    halo = np.clip(halo / max(halo.max(), 1e-6), 0, 1)
    Image.fromarray((halo * 255).astype(np.uint8)).save(os.path.join(HERE, "aura", "halo_%s.png" % who))
    zs = [p[1] for p in keep]
    return dict(edge=keep, top=max(zs), halo=dict(file="halo_%s.png" % who, x0=round(-ox / 100, 4),
                                                   z1=round(oy / 100, 4), w=big.shape[1] / 100, h=big.shape[0] / 100))


if __name__ == "__main__":
    os.makedirs(os.path.join(HERE, "aura"), exist_ok=True)
    out = {w: outline(w) for w in WHO}
    with open(os.path.join(HERE, "aura", "silhouettes.json"), "w") as f:
        json.dump(out, f, separators=(",", ":"))
    for w, d in out.items():
        print(w, len(d["edge"]), "points, top %.2f" % d["top"])
