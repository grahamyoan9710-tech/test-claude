#!/usr/bin/env python3
"""Repaints a fighter's pictures in its alternate colours (palette 2, worn by the second copy in a mirror match).

    python3 scripts/recolor.py NN IN.png [IN.png ...] OUT_DIR   repaints fighter NN's pictures (PNG, transparent
                                                                background) into OUT_DIR, same file names
    python3 scripts/recolor.py --test [OUT.png]                   repaints the 10 fighters of the normal sheet and
                                                                compares them with the alternate sheet

PixelLab draws the normal colours only. The alternate ones come from the owner's two sheets of the refonte v2
(docs/refonte/ref_persos_v2.png and ref_persos_v2_alt.png: the same drawings, repainted; fighter NN is cell NN, rows
of 5). For fighter NN, the pixels of the normal drawing are grouped (k-means) by their colour (Lab), the colour around
them and their height in the drawing; each group takes the colour of the alternate drawing at the same pixels (inside
the group's areas, the alternate drawing slid onto the normal one). A pixel is repainted from its nearest groups,
keeping its shade within the group, so the shading and the anti-aliasing survive. The output is RGBA.

shortcut: a colour that turns into two different colours on two parts with the same surroundings (03's white jacket
and white hair, 07's black hair and black coat) can come out mixed on a new pose; check each fighter's first PixelLab
clips with it and add per-fighter fixes if needed.
"""
import os
import sys
from collections import deque

import numpy as np
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
from crop_refs import components, ink, rows_of  # noqa: E402

SHEETS = [os.path.join(ROOT, "docs", "refonte", n) for n in ("ref_persos_v2.png", "ref_persos_v2_alt.png")]
LABEL_TOP = (457, 957)  # the number labels under each row of the sheets start there
GROUPS = 48
SOFT = 7.0  # feature distance over which a pixel blends its nearest groups
CONTEXT = 0.7  # weight of the colour around a pixel
HEIGHT = 60.0  # weight of its height in the drawing (Lab units from the top to the feet)


# ---- colour ---------------------------------------------------------------------------------------------------
def to_lab(rgb):
    c = rgb.astype(np.float64) / 255.0
    c = np.where(c > 0.04045, ((c + 0.055) / 1.055) ** 2.4, c / 12.92)
    xyz = c @ np.array([[0.4124, 0.3576, 0.1805], [0.2126, 0.7152, 0.0722], [0.0193, 0.1192, 0.9505]]).T
    xyz /= np.array([0.95047, 1.0, 1.08883])
    f = np.where(xyz > 216 / 24389, np.cbrt(xyz), (24389 / 27 * xyz + 16) / 116)
    return np.stack([116 * f[..., 1] - 16, 500 * (f[..., 0] - f[..., 1]), 200 * (f[..., 1] - f[..., 2])], -1)


def to_rgb(lab):
    fy = (lab[..., 0] + 16) / 116
    f = np.stack([fy + lab[..., 1] / 500, fy, fy - lab[..., 2] / 200], -1)
    xyz = np.where(f ** 3 > 216 / 24389, f ** 3, (116 * f - 16) / (24389 / 27)) * np.array([0.95047, 1.0, 1.08883])
    c = xyz @ np.array([[3.2406, -1.5372, -0.4986], [-0.9689, 1.8758, 0.0415], [0.0557, -0.2040, 1.0570]]).T
    c = np.clip(c, 0.0, 1.0)
    c = np.where(c > 0.0031308, 1.055 * c ** (1 / 2.4) - 0.055, 12.92 * c)
    return np.clip(np.round(c * 255), 0, 255).astype(np.uint8)


# ---- the sheets -------------------------------------------------------------------------------------------------
def cells(img):
    """The 10 drawings' boxes, numbered 1..10 (rows of 5), cut above the number labels."""
    blobs = [b for b in components(ink(img), 0) if b[3] - b[1] > 150]
    rows = rows_of(blobs, 2)
    if [len(r) for r in rows] != [5, 5]:
        sys.exit(f"expected 2 rows of 5 fighters, found {[len(r) for r in rows]}")
    return [(x0, y0, x1, min(y1, LABEL_TOP[ri])) for ri, r in enumerate(rows) for (x0, y0, x1, y1) in r]


def body(rgb):
    """True on the drawing: everything but the white background reached from the borders."""
    white = rgb.astype(np.int16).min(axis=2) >= 232
    h, w = white.shape
    bg = np.zeros_like(white)
    q = deque((y, x) for y in range(h) for x in (0, w - 1) if white[y, x])
    q.extend((y, x) for x in range(w) for y in (0, h - 1) if white[y, x])
    for y, x in q:
        bg[y, x] = True
    while q:
        y, x = q.popleft()
        for ny, nx in ((y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)):
            if 0 <= ny < h and 0 <= nx < w and white[ny, nx] and not bg[ny, nx]:
                bg[ny, nx] = True
                q.append((ny, nx))
    return ~bg


def shift(a, dx, dy, fill):
    """a moved by (dx, dy), the uncovered part filled."""
    out = np.full_like(a, fill)
    h, w = a.shape[:2]
    out[max(0, dy):h + min(0, dy), max(0, dx):w + min(0, dx)] = a[max(0, -dy):h - max(0, dy), max(0, -dx):w - max(0, dx)]
    return out


def pair(n):
    """Fighter n's normal and alternate drawings in one window (the alternate slid onto the normal), and their masks."""
    imgs = [Image.open(p).convert("RGB") for p in SHEETS]
    boxes = [cells(im)[n - 1] for im in imgs]
    m = 24
    x0, y0 = max(0, min(b[0] for b in boxes) - m), max(0, min(b[1] for b in boxes) - m)
    x1 = min(imgs[0].width, max(b[2] for b in boxes) + m)
    y1 = min(LABEL_TOP[0 if n <= 5 else 1], max(b[3] for b in boxes))
    a, b = (np.asarray(im.crop((x0, y0, x1, y1))) for im in imgs)
    ma, mb = body(a), body(b)
    # Slide the alternate drawing onto the normal one: the most overlap of the drawings and of their dark outlines.
    da, db = ma & (to_lab(a)[..., 0] < 30), mb & (to_lab(b)[..., 0] < 30)
    best, at = -1.0, (0, 0)
    for dy in range(-12, 13):
        for dx in range(-12, 13):
            s = (ma & shift(mb, dx, dy, False)).sum() + 2.0 * (da & shift(db, dx, dy, False)).sum()
            if s > best:
                best, at = s, (dx, dy)
    dx, dy = at
    return a, ma, shift(b, dx, dy, 255), shift(mb, dx, dy, False)


# ---- the mapping ------------------------------------------------------------------------------------------------
def kmeans(x, k, iters=25, seed=7):
    rng = np.random.default_rng(seed)
    c = [x[rng.integers(len(x))]]
    for _ in range(1, k):  # k-means++
        d = np.min(((x[:, None, :] - np.array(c)[None]) ** 2).sum(-1), axis=1)
        c.append(x[rng.choice(len(x), p=d / d.sum())])
    c = np.array(c)
    for _ in range(iters):
        lab = np.argmin(((x[:, None, :] - c[None]) ** 2).sum(-1), axis=1)
        c = np.array([x[lab == i].mean(0) if (lab == i).any() else c[i] for i in range(k)])
    return c


def nearest(lab, centres):
    return np.argmin(((lab[..., None, :] - centres) ** 2).sum(-1), axis=-1)


def features(lab, mask):
    """Per pixel of the drawing: its colour, the mean colour around it (outlines left out) and its height in the
    drawing (0 at the top, 1 at the feet), each weighted: two pixels of the same colour on different parts (a dark
    hair and a dark coat, a shaded skin and a bronze armour) differ by what surrounds them or by where they are."""
    ys, xs = np.nonzero(mask)
    top, h = ys.min(), max(1, ys.max() - ys.min())
    r = max(1, int(round(0.02 * h)))
    use = mask & (lab[..., 0] >= 18)
    ctx = np.zeros(lab.shape[:2] + (4,))
    ctx[..., :3], ctx[..., 3] = lab * use[..., None], use
    c = np.pad(ctx, ((r + 1, r), (r + 1, r), (0, 0))).cumsum(0).cumsum(1)
    box = c[2 * r + 1:, 2 * r + 1:] - c[:-2 * r - 1, 2 * r + 1:] - c[2 * r + 1:, :-2 * r - 1] + c[:-2 * r - 1, :-2 * r - 1]
    mean = np.where(box[..., 3:] > 0, box[..., :3] / np.maximum(box[..., 3:], 1), lab)
    y = (np.arange(lab.shape[0])[:, None] - top) / h * np.ones(lab.shape[:2])
    return np.concatenate([lab, CONTEXT * mean, HEIGHT * y[..., None]], -1)


class Palette:
    """Fighter n's groups of normal pixels (colour, surroundings, height) and the alternate colour of each."""

    def __init__(self, n, part=None):
        a, ma, b, mb = pair(n)
        if part is not None:  # learn on part of the drawing only (the test)
            ma = ma & part
        la, lb = to_lab(a), to_lab(b)
        fa = features(la, ma)
        px = fa[ma]
        rng = np.random.default_rng(3)
        self.src = kmeans(px[rng.choice(len(px), min(len(px), 20000), replace=False)], GROUPS)
        g = np.where(ma, nearest(fa, self.src), -1)
        # The middle of each group's areas (3 x 3 the same group), where the alternate drawing is also drawn.
        same = np.ones_like(ma)
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                same &= shift(g, dx, dy, -2) == g
        self.dst = self.src[:, :3].copy()
        for i in range(len(self.src)):
            core = same & (g == i) & mb
            if core.sum() < 12:
                core = (g == i) & mb
            if core.sum() == 0:
                continue
            alt = lb[core]
            self.dst[i] = np.median(alt, axis=0)
            if len(alt) >= 24 and np.median(np.linalg.norm(alt - self.dst[i], axis=-1)) > 10:
                # Two colours there: the commoner one, not a blend of both that is nowhere on the sheet.
                two = kmeans(alt, 2, iters=10)
                k = nearest(alt, two)
                self.dst[i] = np.median(alt[k == np.bincount(k, minlength=2).argmax()], axis=0)
        ca, cb = np.hypot(*self.src[:, 1:3].T), np.hypot(*self.dst[:, 1:].T)
        self.chroma = np.clip((cb + 4.0) / (ca + 4.0), 0.2, 1.5)  # scales a pixel's colour shift within its group

    def apply(self, rgb, mask):
        """rgb: an H x W x 3 picture, mask: where the fighter is drawn. Returns the repainted colours of the mask."""
        lab = to_lab(rgb)
        f = features(lab, mask)[mask]
        lab = lab[mask]
        d2 = ((f[:, None, :] - self.src) ** 2).sum(-1)  # (pixels, groups)
        w = np.exp(-(d2 - d2.min(-1, keepdims=True)) / (2 * SOFT * SOFT))
        w[w < 1e-3] = 0.0
        dev = lab[:, None, :] - self.src[:, :3]
        dev[..., 1:] *= self.chroma[:, None]
        out = (w[..., None] * (self.dst + dev)).sum(-2) / w.sum(-1, keepdims=True)
        out[..., 0] = np.clip(out[..., 0], 0.0, 100.0)
        return to_rgb(out)


def recolor_file(pal, src, dst):
    im = Image.open(src).convert("RGBA")
    a = np.asarray(im).copy()
    on = a[..., 3] > 0
    a[..., :3][on] = pal.apply(a[..., :3], on)
    Image.fromarray(a).save(dst)


# ---- the test ---------------------------------------------------------------------------------------------------
def test(out):
    from PIL import ImageDraw
    x = np.random.default_rng(0).integers(0, 256, (1000, 3)).astype(np.uint8)
    assert np.abs(to_rgb(to_lab(x)).astype(int) - x).max() <= 1, "Lab round trip"
    rows, scores = [], []
    for n in range(1, 11):
        pal = Palette(n)
        a, ma, b, mb = pair(n)
        r = a.copy()
        r[ma] = pal.apply(a, ma)
        both = ma & mb
        de = np.linalg.norm(to_lab(r)[both] - to_lab(b)[both], axis=-1)
        scores.append((n, float(np.median(de)), float((de < 12).mean())))
        h = 360
        tiles = [Image.fromarray(x).resize((int(x.shape[1] * h / x.shape[0]), h), Image.NEAREST) for x in (a, r, b)]
        row = Image.new("RGB", (sum(t.width for t in tiles) + 20, h + 22), "white")
        dr, x = ImageDraw.Draw(row), 0
        for t, lab in zip(tiles, ("normal", "repeint", "planche")):
            row.paste(t, (x, 22))
            dr.text((x + 4, 4), f"{n:02d} {lab}", fill="black")
            x += t.width + 10
        rows.append(row)
    W = max(r.width for r in rows)
    sheet = Image.new("RGB", (2 * W + 20, 5 * rows[0].height), "white")
    for i, r in enumerate(rows):
        sheet.paste(r, ((i // 5) * (W + 20), (i % 5) * r.height))
    sheet.save(out)
    for n, med, ok in scores:
        print(f"{n:02d}: median ΔE {med:5.1f}, {100 * ok:4.1f} % of pixels within ΔE 12 of the sheet")
    print(f"-> {out}")


def main():
    if len(sys.argv) >= 2 and sys.argv[1] == "--test":
        test(sys.argv[2] if len(sys.argv) > 2 else "recolor_test.png")
        return
    if len(sys.argv) < 4:
        sys.exit(__doc__)
    pal = Palette(int(sys.argv[1]))
    out_dir = sys.argv[-1]
    os.makedirs(out_dir, exist_ok=True)
    for src in sys.argv[2:-1]:
        recolor_file(pal, src, os.path.join(out_dir, os.path.basename(src)))
        print(f"{src} -> {out_dir}")


if __name__ == "__main__":
    main()
