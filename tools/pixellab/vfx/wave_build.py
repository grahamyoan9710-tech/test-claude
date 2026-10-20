#!/usr/bin/env python3
"""Builds a shock wave's frames from its PixelLab clips: python3 wave_build.py N [--burst A-B] [--fade A-B] [--hold K]

waves_anim_mm/wN_burst (the wave bursting from the ground up to its picture) and waves_anim_mm/wN_fade (the picture
dissipating into sparkles) become waves_anim/wN/f_00.png...: the burst frames, the picture held K frames, the fade
frames. What the video model adds around the wave (flat grey patches where the picture has nothing) is cleared,
and the big white areas (the glowing floor, the dome of wave 3) become see-through so the fighter stays visible.
Then: python3 scripts/import_fx.py move_wave_N tools/pixellab/vfx/waves_anim/wN --keep-first --anchor X Y
"""
import argparse
import os

import numpy as np
from PIL import Image

S = os.path.dirname(os.path.abspath(__file__))


def grow(m, it=1):
    for _ in range(it):
        g = m.copy()
        g[1:] |= m[:-1]; g[:-1] |= m[1:]; g[:, 1:] |= m[:, :-1]; g[:, :-1] |= m[:, 1:]
        m = g
    return m


def shrink(m, it=1):
    return ~grow(~m, it)


def regions(m):
    """The 4-connected regions of a mask, as lists of (y, x) arrays."""
    seen = np.zeros_like(m)
    out = []
    for y, x in zip(*np.nonzero(m)):
        if seen[y, x]:
            continue
        stack, pts = [(y, x)], []
        seen[y, x] = True
        while stack:
            cy, cx = stack.pop()
            pts.append((cy, cx))
            for ny, nx in ((cy + 1, cx), (cy - 1, cx), (cy, cx + 1), (cy, cx - 1)):
                if 0 <= ny < m.shape[0] and 0 <= nx < m.shape[1] and m[ny, nx] and not seen[ny, nx]:
                    seen[ny, nx] = True
                    stack.append((ny, nx))
        out.append(tuple(np.array(pts).T))
    return out


def frames(d):
    fs = sorted(f for f in os.listdir(d) if f.startswith("f_") and f.endswith(".png"))
    return [np.asarray(Image.open(os.path.join(d, f)).convert("RGBA")).copy() for f in fs]


def span(text, n):
    a, b = (int(v) for v in text.split("-"))
    return list(range(a, min(b, n - 1) + 1))


def clean(fr, footprint):
    """Clears the flat grey patches the video model paints: big compact areas of one single grey (the picture's own
    greys are thin bands), and any grey area mostly outside the picture."""
    a = fr[:, :, 3] > 0
    cols = np.unique(fr[a][:, :3], axis=0)
    for c in cols:
        if not (abs(int(c[0]) - int(c[2])) < 6 and 90 < c[0] < 200):
            continue
        m = a & (fr[:, :, 0] == c[0]) & (fr[:, :, 1] == c[1]) & (fr[:, :, 2] == c[2])
        for r in regions(m):
            n = len(r[0])
            h, w = r[0].max() - r[0].min() + 1, r[1].max() - r[1].min() + 1
            if (n > 150 and n / (h * w) > 0.38 and min(h, w) >= 8) or (n > 40 and (~footprint[r]).sum() > 0.6 * n):
                fr[r] = 0
    return fr


def see_through(fr, alpha=120):
    """Big areas of plain white (a glowing floor, a dome) are drawn half see-through."""
    white = (fr[:, :, 3] > 0) & (fr[:, :, :3].min(axis=2) > 240)
    for r in regions(white):
        if len(r[0]) > 300:
            m = np.zeros_like(white)
            m[r] = True
            fr[shrink(m, 2), 3] = alpha  # keep a bright rim
    return fr


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("n", type=int)
    ap.add_argument("--burst", default="1-7")
    ap.add_argument("--fade", default="9-12")
    ap.add_argument("--hold", type=int, default=2)
    ap.add_argument("--full", default=None)
    a = ap.parse_args()
    full = np.asarray(Image.open(a.full or os.path.join(S, "waves_src", f"w{a.n}_full.png")).convert("RGBA")).copy()
    footprint = grow(full[:, :, 3] > 0, 3)
    burst = frames(os.path.join(S, "waves_anim_mm", f"w{a.n}_burst"))
    fade = frames(os.path.join(S, "waves_anim_mm", f"w{a.n}_fade"))
    seq = [clean(burst[i], footprint) for i in span(a.burst, len(burst))]
    seq += [full.copy() for _ in range(a.hold)]
    seq += [clean(fade[i], footprint) for i in span(a.fade, len(fade))]
    out = os.path.join(S, "waves_anim", f"w{a.n}")
    os.makedirs(out, exist_ok=True)
    for f in os.listdir(out):
        os.remove(os.path.join(out, f))
    for i, fr in enumerate(seq):
        Image.fromarray(see_through(fr)).save(os.path.join(out, f"f_{i:02d}.png"))
    rows = np.nonzero(full[:, :, 3].any(axis=1))[0]
    print(out, len(seq), "frames; picture rows", rows.min(), "-", rows.max(), "canvas", full.shape[1], "x", full.shape[0])


if __name__ == "__main__":
    main()
