#!/usr/bin/env python3
"""Finds the grey dust and smoke PixelLab glues to a fighter (the owner: none at all, or the clip is made again):
python3 dustscan.py FIGHTER [CLIP...] -> per clip, the frames with grey (neutral, mid-light, unsaturated pixels that
are not the fighter's own greys) and those touching the canvas edge. The fighter's own greys are those of its guard."""
import os
import sys

import numpy as np
from PIL import Image

S = os.path.dirname(os.path.abspath(__file__))


def greys(a):
    rgb = a[..., :3].astype(int)
    mx, mn = rgb.max(-1), rgb.min(-1)
    return (a[..., 3] > 0) & (mx >= 85) & (mx <= 215) & ((mx - mn) < 0.10 * np.maximum(mx, 1) + 2)


def scan(fighter, names=None):
    guard = np.asarray(Image.open(os.path.join(S, "base", f"{fighter}_guard256.png")).convert("RGBA"))
    own = int(greys(guard).sum())  # the fighter's own greys (steel, silver hair...) are allowed up to about this much
    root = os.path.join(S, fighter)
    bad = []
    for c in sorted(names or os.listdir(root)):
        d = os.path.join(root, c)
        if "." in c or not os.path.isdir(d):
            continue
        fs = sorted(f for f in os.listdir(d) if f.startswith("f_") and f.endswith(".png"))
        dusty, cut = [], []
        for f in fs[1:]:  # f_00 is the base itself
            a = np.asarray(Image.open(os.path.join(d, f)).convert("RGBA"))
            g = int(greys(a).sum())
            if g > own + 150:
                dusty.append((f[2:4], g))
            al = a[..., 3] > 0
            if al[0].any() or al[-1].any() or al[:, 0].any() or al[:, -1].any():
                cut.append(f[2:4])
        if dusty or cut:
            bad.append(c)
            print(f"{c:14s} grey {dusty}  cut {cut}")
    print(f"{fighter}: own greys {own}, {len(bad)} clip(s) to look at")
    return bad


if __name__ == "__main__":
    scan(sys.argv[1], sys.argv[2:] or None)
