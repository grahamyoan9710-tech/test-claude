#!/usr/bin/env python3
"""Cuts the owner's reference sheets (docs/refonte) into the fighters' placeholder pictures.

    python3 scripts/crop_refs.py [OUT_DIR]   (default: assets/sprites)

- docs/refonte/ref_persos.png (17 fighters, numbered 1..17 in rows of 6, 6, 5) gives p<NN>_still.png (the whole
  body, white background made transparent) and p<NN>_portrait.png (a square around the head);
- docs/refonte/ref_armes.png (17 pairs of weapons, rows of 5, 5, 5, 2, the layout written below) gives p<NN>_icon_a.png and p<NN>_icon_b.png
  (left and right weapon of pair NN).

The game draws these until the fighter's PixelLab clips are imported (render_body.cpp, render_weapons.cpp). They are
also the starting point of the PixelLab bases.
"""
import os
import sys
from collections import deque

import numpy as np
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CELL = 4  # px per cell of the coarse grid


def ink(img, thresh=228):
    a = np.asarray(img.convert("RGB")).astype(np.int16)
    return a.min(axis=2) < thresh


def components(mask, grow):
    """Connected blobs of `mask` on a coarse grid dilated by `grow` cells: list of (x0, y0, x1, y1) in pixels."""
    h, w = mask.shape
    gh, gw = (h + CELL - 1) // CELL, (w + CELL - 1) // CELL
    pad = np.zeros((gh * CELL, gw * CELL), bool)
    pad[:h, :w] = mask
    grid = pad.reshape(gh, CELL, gw, CELL).any(axis=(1, 3))
    dil = grid.copy()
    for dy in range(-grow, grow + 1):
        for dx in range(-grow, grow + 1):
            dil |= np.roll(np.roll(grid, dy, 0), dx, 1)
    seen = np.zeros_like(dil)
    out = []
    for y in range(gh):
        for x in range(gw):
            if not dil[y, x] or seen[y, x]:
                continue
            q = deque([(y, x)])
            seen[y, x] = True
            x0 = x1 = x
            y0 = y1 = y
            while q:
                cy, cx = q.popleft()
                x0, x1, y0, y1 = min(x0, cx), max(x1, cx), min(y0, cy), max(y1, cy)
                for ny, nx in ((cy + 1, cx), (cy - 1, cx), (cy, cx + 1), (cy, cx - 1)):
                    if 0 <= ny < gh and 0 <= nx < gw and dil[ny, nx] and not seen[ny, nx]:
                        seen[ny, nx] = True
                        q.append((ny, nx))
            out.append(((x0 + grow) * CELL, (y0 + grow) * CELL, (x1 - grow + 1) * CELL, (y1 - grow + 1) * CELL))
    return out


def cutout(img, box, margin=6):
    """The box (plus a margin) with the white background around the drawing made transparent."""
    x0, y0, x1, y1 = box
    x0, y0 = max(0, x0 - margin), max(0, y0 - margin)
    x1, y1 = min(img.width, x1 + margin), min(img.height, y1 + margin)
    crop = img.crop((x0, y0, x1, y1)).convert("RGBA")
    a = np.asarray(crop).copy()
    white = a[:, :, :3].astype(np.int16).min(axis=2) >= 232
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
    a[bg, 3] = 0
    out = Image.fromarray(a)
    bb = out.getbbox()
    return out.crop(bb) if bb else out


def rows_of(blobs, n_rows):
    """Splits blobs into rows by their vertical centre (the n_rows biggest gaps), each row sorted left to right."""
    blobs = sorted(blobs, key=lambda b: (b[1] + b[3]) / 2)
    cy = [(b[1] + b[3]) / 2 for b in blobs]
    gaps = sorted(range(1, len(cy)), key=lambda i: cy[i] - cy[i - 1], reverse=True)[: n_rows - 1]
    rows, start = [], 0
    for cut in sorted(gaps) + [len(blobs)]:
        rows.append(sorted(blobs[start:cut], key=lambda b: b[0]))
        start = cut
    return rows


def portrait(still):
    """A square around the head: the top of the opaque pixels, centred on them."""
    a = np.asarray(still)[:, :, 3] > 0
    h, w = a.shape
    top = a[: max(1, int(h * 0.22))]
    xs = np.nonzero(top.any(axis=0))[0]
    cx = int(xs.mean()) if len(xs) else w // 2
    side = min(w, int(h * 0.36))
    x0 = max(0, min(w - side, cx - side // 2))
    return still.crop((x0, 0, x0 + side, side))


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "assets", "sprites")
    os.makedirs(out_dir, exist_ok=True)
    ref = os.path.join(ROOT, "docs", "refonte")

    # --- fighters ---
    img = Image.open(os.path.join(ref, "ref_persos.png"))
    blobs = [b for b in components(ink(img), 0) if b[3] - b[1] > 150]
    rows = rows_of(blobs, 3)
    fighters = [b for r in rows for b in r]
    if [len(r) for r in rows] != [6, 6, 5]:
        sys.exit(f"ref_persos.png: expected rows of 6, 6, 5 fighters, found {[len(r) for r in rows]}")
    for i, box in enumerate(fighters, 1):
        still = cutout(img, box)
        still.save(os.path.join(out_dir, f"p{i:02d}_still.png"))
        portrait(still).save(os.path.join(out_dir, f"p{i:02d}_portrait.png"))

    # --- weapons: each pair sits above its number, centred between its two weapons (the sheet's layout: the
    # numbers' x per row and the row's bottom). In the pair's cell, a piece left of the number is A, right of it B.
    img = Image.open(os.path.join(ref, "ref_armes.png"))
    layout = [(262, [153, 443, 735, 1037, 1380]), (518, [160, 443, 735, 1037, 1370]),
              (762, [155, 455, 738, 1043, 1370]), (1000, [519, 935])]
    mask = ink(img)
    n, top = 0, 0
    for bottom, xs in layout:
        for li, lx in enumerate(xs):
            n += 1
            xl = 0 if li == 0 else (xs[li - 1] + lx) // 2
            xr = img.width if li == len(xs) - 1 else (xs[li + 1] + lx) // 2
            cell = mask[top:bottom - 16, xl:xr]
            pieces = [p for p in components(cell, 0) if (p[2] - p[0]) * (p[3] - p[1]) > 150]
            for side, keep in (("a", lambda c: c < lx), ("b", lambda c: c >= lx)):
                part = [p for p in pieces if keep(xl + (p[0] + p[2]) / 2)]
                if not part:
                    sys.exit(f"ref_armes.png: pair {n} has no weapon {side.upper()}")
                box = (xl + min(p[0] for p in part), top + min(p[1] for p in part),
                       xl + max(p[2] for p in part), top + max(p[3] for p in part))
                cutout(img, box, margin=2).save(os.path.join(out_dir, f"p{n:02d}_icon_{side}.png"))
        top = bottom
    print(f"{len(fighters)} fighters, {n} weapon pairs -> {out_dir}")


if __name__ == "__main__":
    main()
