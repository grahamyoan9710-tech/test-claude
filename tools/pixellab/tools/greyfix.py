"""Finds (and with --fix erases) the flat grey slabs PixelLab sometimes paints behind fast effects:
large connected regions of one neutral grey. python3 greyfix.py DIR [DIR...] [--fix]"""
import os, sys
import numpy as np
from PIL import Image


def label(mask):
    """4-connected components (no scipy here)."""
    lab = np.zeros(mask.shape, int); n = 0; h, w = mask.shape
    for y0, x0 in zip(*np.nonzero(mask)):
        if lab[y0, x0]:
            continue
        n += 1; stack = [(y0, x0)]; lab[y0, x0] = n
        while stack:
            y, x = stack.pop()
            for yy, xx in ((y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)):
                if 0 <= yy < h and 0 <= xx < w and mask[yy, xx] and not lab[yy, xx]:
                    lab[yy, xx] = n; stack.append((yy, xx))
    return lab, n

def slabs(a):
    rgb = a[..., :3].astype(int); op = a[..., 3] > 0
    mx, mn = rgb.max(-1), rgb.min(-1)
    grey = op & (mx - mn < 14) & (mx > 100) & (mx < 215)
    lab, n = label(grey)
    out = np.zeros(grey.shape, bool)
    for i in range(1, n + 1):
        m = lab == i
        area = m.sum()
        if area < 500:
            continue
        cols, cnt = np.unique(rgb[m].reshape(-1, 3), axis=0, return_counts=True)
        # A slab lies on the background (its rim is mostly transparent); a shade inside smoke or cloth is enclosed.
        rim = np.zeros_like(m)
        rim[1:] |= m[:-1]; rim[:-1] |= m[1:]; rim[:, 1:] |= m[:, :-1]; rim[:, :-1] |= m[:, 1:]
        rim &= ~m
        open_rim = (~op[rim]).mean() if rim.any() else 0
        if cnt.max() / area > 0.85 and (open_rim > 0.3 or area > 3000):
            out |= m
    return out

def main():
    fix = '--fix' in sys.argv
    for d in [x for x in sys.argv[1:] if x != '--fix']:
        for f in sorted(os.listdir(d)):
            if not (f.startswith('f_') and f.endswith('.png')) or f == 'f_00.png':
                continue
            p = os.path.join(d, f)
            a = np.array(Image.open(p).convert('RGBA'))
            m = slabs(a)
            if m.sum():
                print(d.rstrip('/').split('/')[-1], f, int(m.sum()))
                if fix:
                    keep = os.path.join(d, 'orig', f)  # the untouched frame, out of the f_*.png the importers read
                    if not os.path.exists(keep):
                        os.makedirs(os.path.dirname(keep), exist_ok=True)
                        Image.fromarray(a).save(keep)
                    a[m] = 0
                    Image.fromarray(a).save(p)


if __name__ == "__main__":
    main()
