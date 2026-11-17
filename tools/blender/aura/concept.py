# Concept sheets of the auras and bursts (flat cel shapes from aura_geo, no Blender): keyframes over the fighter.
# Usage: concept.py <out_dir>
import math, os, sys
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

R = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../..'))  # the repo
sys.path.insert(0, R + '/tools/blender')
import aura_geo as G

OUT = sys.argv[1]
os.makedirs(OUT, exist_ok=True)
PAL = {k: v[:4] for k, v in G.PAL.items()}
SHEET = {l.split()[1]: l.split() for l in open(R + '/assets/sprites/sheets.txt') if l.startswith('sheet ') and l.split()[2] == 'idle'}


def hexc(h):
    h = h.lstrip('#')
    return np.array([int(h[i:i + 2], 16) for i in (0, 2, 4)], float)


def ramp(who, h):
    stops = [(0.0, 0), (0.35, 1), (0.7, 2), (1.0, 3)]
    P = [hexc(c) for c in PAL[who]]
    for (a, i), (b, j) in zip(stops, stops[1:]):
        if h <= b:
            k = (h - a) / (b - a)
            return P[i] * (1 - k) + P[j] * k
    return P[3]


def ribpoly(pts, ws):
    L, Rr = [], []
    n = len(pts)
    for i, (x, z) in enumerate(pts):
        a, b = pts[max(i - 1, 0)], pts[min(i + 1, n - 1)]
        dx, dz = b[0] - a[0], b[1] - a[1]
        d = math.hypot(dx, dz) or 1e-6
        nx, nz = -dz / d, dx / d
        w = ws[i] / 2
        L.append((x + nx * w, z + nz * w))
        Rr.append((x - nx * w, z - nz * w))
    return L + Rr[::-1]


def draw(who, shapes, size, origin, scale, front=None):
    """Shapes -> premultiplied RGBA float image (supersampled 3x)."""
    SS = 3
    W, H = size
    img = np.zeros((H * SS, W * SS, 4))
    toPx = lambda x, z: ((origin[0] + x * scale) * SS, (origin[1] - z * scale) * SS)
    for s in sorted(shapes, key=lambda s: -s['y']):
        if front is not None and s.get('front', False) != front:
            continue
        a = max(0.0, min(1.0, s['a']))
        if a < 0.01:
            continue
        m = s.get('m', 'flame')
        if s['k'] == 'rib':
            poly = ribpoly(s['pts'], s['w'])
            alpha = a * {'flame': 0.92, 'veil': 0.55, 'line': 0.9}.get(m, 0.9)
        elif s['k'] == 'poly':
            poly = s['pts']
            alpha = a
        elif s['k'] == 'dot':
            (cx, cz), r = s['c'], s['r']
            poly = [(cx + r * math.cos(t), cz + r * math.sin(t)) for t in np.linspace(0, 2 * math.pi, 14)]
            alpha = a
        else:  # bolt
            poly = None
            alpha = a
        if poly is None:
            P = [toPx(*p) for p in s['pts']]
            col = ramp(who, 1.0)
            pad = 3 * SS
        else:
            P = [toPx(*p) for p in poly]
            col = ramp(who, s['h'])
            pad = 2
        xs, ys = [p[0] for p in P], [p[1] for p in P]
        bx0, by0 = max(0, int(min(xs)) - pad), max(0, int(min(ys)) - pad)
        bx1, by1 = min(W * SS, int(max(xs)) + pad + 1), min(H * SS, int(max(ys)) + pad + 1)
        if bx1 <= bx0 or by1 <= by0:
            continue
        mask = Image.new('L', (bx1 - bx0, by1 - by0), 0)
        d = ImageDraw.Draw(mask)
        Q = [(x - bx0, y - by0) for x, y in P]
        if poly is None:  # lightning: a red halo, then its white core
            d.line(Q, fill=255, width=int(6 * SS))
            M = np.asarray(mask.filter(ImageFilter.GaussianBlur(SS)), float)[..., None] / 255 * alpha * 0.8
            sub = img[by0:by1, bx0:bx1]
            c2 = ramp(who, 0.35)
            sub[..., :3] = sub[..., :3] * (1 - M) + c2 / 255 * M
            sub[..., 3:] = sub[..., 3:] * (1 - M) + M
            mask = Image.new('L', mask.size, 0)
            ImageDraw.Draw(mask).line(Q, fill=255, width=int(1.6 * SS))
        else:
            d.polygon(Q, fill=255)
        M = np.asarray(mask, float)[..., None] / 255 * alpha
        sub = img[by0:by1, bx0:bx1]
        sub[..., :3] = sub[..., :3] * (1 - M) + col / 255 * M
        sub[..., 3:] = sub[..., 3:] * (1 - M) + M
    im = Image.fromarray((np.clip(img, 0, 1) * 255).astype(np.uint8), 'RGBA')
    return np.asarray(im.resize((W, H), Image.LANCZOS), float) / 255


def compose(layers, bg, glow=1.0):
    """layers: list of (premult RGBA float, kind) in order; kind 'fx' glows."""
    H, W = layers[0][0].shape[:2]
    out = np.ones((H, W, 3)) * np.array(bg, float) / 255
    for L, kind in layers:
        if kind == 'fx':
            g = Image.fromarray((np.clip(L[..., :3] * L[..., 3:], 0, 1) * 255).astype(np.uint8))
            bl = sum(k * np.asarray(g.filter(ImageFilter.GaussianBlur(s)), float) / 255 for s, k in ((3, 0.6), (10, 0.45), (24, 0.3)))
            out = out * (1 - L[..., 3:]) + L[..., :3] * L[..., 3:]
            out = 1 - (1 - out) * (1 - np.clip(bl * glow, 0, 1))  # screen
        else:
            out = out * (1 - L[..., 3:]) + L[..., :3] * L[..., 3:]
    return Image.fromarray((np.clip(out, 0, 1) * 255).astype(np.uint8))


def sprite_layer(who, size, origin, scale, kscale=1.0):
    f = SHEET[who]
    fw, fh, ax, ay, pph = int(float(f[4])), int(float(f[5])), float(f[6]), float(f[7]), float(f[8])
    im = Image.open('%s/assets/sprites/%s_idle.png' % (R, who)).convert('RGBA').crop((0, 0, fw, fh))
    k = 1.7 / pph * kscale * scale  # px per sprite px
    im = im.resize((round(fw * k), round(fh * k)), Image.NEAREST)
    L = Image.new('RGBA', size, (0, 0, 0, 0))
    L.paste(im, (round(origin[0] - ax * k), round(origin[1] - ay * k)))
    return np.asarray(L, float) / 255


def aura_sheet(who, frames, scale=200):
    fn, N = G.AURAS[who]
    allb = [G.bounds(fn(i / N)) for i in range(0, N, 3)]
    x0, z0 = min(b[0] for b in allb) - 0.12, min(b[1] for b in allb) - 0.08
    x1, z1 = max(b[2] for b in allb) + 0.12, max(b[3] for b in allb) + 0.12
    size = (round((x1 - x0) * scale), round((z1 - z0) * scale))
    origin = (-x0 * scale, z1 * scale)
    tiles = []
    spr = sprite_layer(who, size, origin, scale)
    for fi in frames:
        fx = draw(who, fn(fi / N), size, origin, scale)
        row = [compose([(fx, 'fx'), (spr, 'spr')], bg) for bg in ((20, 22, 36), (205, 214, 228))]
        tiles.append(row)
    return tiles, size


def burst_sheet(who, frames, scale=200):
    fn = G.EVEILS[who]
    allb = [G.bounds(fn(f)) for f in range(G.BN)]
    x0, z0 = min(b[0] for b in allb) - 0.1, min(b[1] for b in allb) - 0.1
    x1, z1 = max(b[2] for b in allb) + 0.1, max(b[3] for b in allb) + 0.1
    size = (round((x1 - x0) * scale), round((z1 - z0) * scale))
    origin = (-x0 * scale, z1 * scale)
    spr = sprite_layer(who, size, origin, scale, kscale=G.H / 1.7)
    tiles = []
    for f in frames:
        sh = fn(f)
        back = draw(who, sh, size, origin, scale, front=False)
        fr = draw(who, sh, size, origin, scale, front=True)
        tiles.append([compose([(back, 'fx'), (spr, 'spr'), (fr, 'fx')], bg) for bg in ((20, 22, 36), (205, 214, 228))])
    return tiles, size


def strip(tiles, path, label=None):
    w, h = tiles[0][0].size
    out = Image.new('RGB', (w * len(tiles), h * 2), (0, 0, 0))
    for i, row in enumerate(tiles):
        for j, t in enumerate(row):
            out.paste(t, (i * w, j * h))
    out.save(path)
    return out.size


if __name__ == '__main__':
    which = sys.argv[2].split(',') if len(sys.argv) > 2 else list(G.AURAS)
    for who in which:
        N = G.AURAS[who][1]
        tiles, size = aura_sheet(who, [0, N // 4, N // 2, 3 * N // 4])
        print(who, 'aura', strip(tiles, '%s/aura_%s.png' % (OUT, who)))
        if who in G.EVEILS:
            tiles, size = burst_sheet(who, [0, 2, 4, 7, 11, 16])
            print(who, 'eveil', strip(tiles, '%s/eveil_%s.png' % (OUT, who)))
