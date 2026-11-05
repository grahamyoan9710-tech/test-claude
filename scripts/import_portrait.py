#!/usr/bin/env python3
"""Turns a character illustration (one fighter, full body, on a plain background) into a game portrait.

    python3 scripts/import_portrait.py INPUT OUTPUT.png [--box X0 Y0 X1 Y1] [--tol 10] [--height H]

- --box crops the input first (for an image holding several characters side by side),
- the background colour is read on the border of the (cropped) picture, then flood-filled away from the edges,
  together with its darker shades (the soft ground shadow under the feet),
- background pockets enclosed by the character (between an arm and the body) are removed too when they are
  plainly the background colour,
- the pixels on the cut get a partial alpha and lose the background tint, so the outline stays clean on any
  backdrop,
- the result is trimmed to the character and optionally scaled to a given height.

Then mark the joints on the result and cut it into animatable parts with scripts/rig_art.py.
"""
import argparse
from collections import deque

import numpy as np
from PIL import Image

SHADE_MIN, SHADE_MAX = 0.55, 1.12  # background shades counted as background (ground shadow .. light noise)
# (--shade-min raises the darkest one: clothes in a darker shade of the background must stay)


def shade_distance(rgb, bg, shade_min=SHADE_MIN):
    """Distance of each pixel to the segment of background shades, and the closest shade."""
    bgv = bg.astype(np.float32)
    s = (rgb @ bgv) / float(bgv @ bgv)
    s = np.clip(s, shade_min, SHADE_MAX)
    near = s[..., None] * bgv
    return np.linalg.norm(rgb - near, axis=-1), near


def flood(mask, seeds):
    """Pixels of `mask` 4-connected to the seed pixels."""
    h, w = mask.shape
    flat = mask.ravel()
    out = np.zeros(h * w, dtype=bool)
    q = deque()
    for i in seeds:
        if flat[i] and not out[i]:
            out[i] = True
            q.append(i)
    while q:
        i = q.popleft()
        y, x = divmod(i, w)
        for j, ok in ((i - 1, x > 0), (i + 1, x < w - 1), (i - w, y > 0), (i + w, y < h - 1)):
            if ok and flat[j] and not out[j]:
                out[j] = True
                q.append(j)
    return out.reshape(h, w)


def components(mask):
    """Connected components (4-neighbourhood) of a boolean mask, as lists of flat indices."""
    h, w = mask.shape
    flat = mask.ravel()
    seen = np.zeros(h * w, dtype=bool)
    for start in np.flatnonzero(flat):
        if seen[start]:
            continue
        seen[start] = True
        comp, q = [start], deque([start])
        while q:
            i = q.popleft()
            y, x = divmod(i, w)
            for j, ok in ((i - 1, x > 0), (i + 1, x < w - 1), (i - w, y > 0), (i + w, y < h - 1)):
                if ok and flat[j] and not seen[j]:
                    seen[j] = True
                    comp.append(j)
                    q.append(j)
        yield comp


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("input")
    ap.add_argument("output")
    ap.add_argument("--box", type=int, nargs=4, metavar=("X0", "Y0", "X1", "Y1"))
    ap.add_argument("--tol", type=float, default=10.0, help="colour distance still counted as background")
    ap.add_argument("--edge", type=float, default=40.0, help="colour range of the soft edge beyond --tol")
    ap.add_argument("--height", type=int, help="scale the trimmed character to this height")
    ap.add_argument("--shade-min", type=float, default=SHADE_MIN,
                    help="darkest background shade removed (raise it when there is no ground shadow)")
    ap.add_argument("--scale", type=int, default=1,
                    help="enlarge by this whole factor, pixel for pixel (small pixel-art pictures)")
    ap.add_argument("--main", action="store_true",
                    help="drop the pieces touching the left or right side (a neighbour on a sheet of characters)")
    a = ap.parse_args()

    im = Image.open(a.input).convert("RGB")
    if a.box:
        im = im.crop(tuple(a.box))
    rgb = np.asarray(im).astype(np.float32)
    h, w, _ = rgb.shape

    border = np.concatenate([rgb[:3].reshape(-1, 3), rgb[-3:].reshape(-1, 3), rgb[:, :3].reshape(-1, 3),
                             rgb[:, -3:].reshape(-1, 3)])
    bg = np.median(border, axis=0)
    dist, near = shade_distance(rgb, bg, a.shade_min)
    bglike = dist < a.tol

    # 1. The background, from the edges of the picture.
    edge = np.zeros((h, w), dtype=bool)
    edge[0, :] = edge[-1, :] = edge[:, 0] = edge[:, -1] = True
    removed = flood(bglike, np.flatnonzero(edge.ravel()))

    # 2. Enclosed pockets of plain background (not the darker shadow shades).
    plain = (np.linalg.norm(rgb - bg, axis=-1) < a.tol * 0.8) & ~removed
    pockets = 0
    for comp in components(plain):
        if len(comp) >= 150:
            removed.ravel()[comp] = True
            pockets += 1

    # 3. Alpha, soft on the cut: pixels next to the background blend from it to the outline.
    alpha = np.where(removed, 0.0, 1.0).astype(np.float32)
    nb = np.zeros((h, w), dtype=bool)
    nb[1:, :] |= removed[:-1, :]
    nb[:-1, :] |= removed[1:, :]
    nb[:, 1:] |= removed[:, :-1]
    nb[:, :-1] |= removed[:, 1:]
    ring = nb & ~removed
    soft = np.clip((dist - a.tol) / a.edge, 0.0, 1.0)
    alpha[ring] = np.maximum(soft[ring], 0.15)
    out = rgb.copy()
    k = alpha[ring][:, None]
    out[ring] = np.clip((rgb[ring] - (1.0 - k) * near[ring]) / k, 0.0, 255.0)

    # 4. Lone specks of noise left in the background (and, with --main, the neighbours' pieces).
    opaque = alpha > 0.0
    specks = 0
    comps = list(components(opaque))
    biggest = max((len(c) for c in comps), default=0)
    for comp in comps:
        xs_ = np.asarray(comp) % w
        side = a.main and len(comp) < biggest and (xs_.min() == 0 or xs_.max() == w - 1)
        if len(comp) < 24 or side:
            alpha.ravel()[comp] = 0.0
            specks += 1

    rgba = np.dstack([out, alpha * 255.0]).round().astype(np.uint8)
    rgba[alpha == 0.0, :3] = 0
    res = Image.fromarray(rgba, "RGBA")
    ys, xs = np.nonzero(alpha > 0.0)
    m = 2
    res = res.crop((max(0, xs.min() - m), max(0, ys.min() - m), min(w, xs.max() + 1 + m), min(h, ys.max() + 1 + m)))
    if a.height and res.height != a.height:
        sw = max(1, round(res.width * a.height / res.height))
        res = res.convert("RGBa").resize((sw, a.height), Image.LANCZOS).convert("RGBA")
    if a.scale > 1:
        res = res.resize((res.width * a.scale, res.height * a.scale), Image.NEAREST)
    res.save(a.output, optimize=True)
    print(f"{a.output}: {res.width}x{res.height}, background {tuple(int(v) for v in bg)}, "
          f"{pockets} enclosed pocket(s), {specks} speck(s) removed")


if __name__ == "__main__":
    main()
