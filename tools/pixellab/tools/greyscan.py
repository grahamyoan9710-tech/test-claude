"""Scans the imported sheets for dithered grey slabs (2-3 neutral greys mixed, large, lying on the background) that
greyfix.py (one flat grey) misses: python3 greyscan.py [FIGHTER...] -> fighter clip frame area."""
import os, sys
import numpy as np
from PIL import Image
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from greyfix import label

R = '/home/user/test-claude/assets/sprites/'

def slabs2(a, min_area=800, tol=16, open_min=0.25):
    rgb = a[..., :3].astype(int); op = a[..., 3] > 0
    mx, mn = rgb.max(-1), rgb.min(-1)
    grey = op & (mx - mn < tol) & (mx > 95) & (mx < 215)
    lab, n = label(grey)
    out = np.zeros(grey.shape, bool)
    for i in range(1, n + 1):
        m = lab == i
        area = int(m.sum())
        if area < min_area:
            continue
        cols, cnt = np.unique(rgb[m].reshape(-1, 3), axis=0, return_counts=True)
        top = np.sort(cnt)[::-1]
        rim = np.zeros_like(m)
        rim[1:] |= m[:-1]; rim[:-1] |= m[1:]; rim[:, 1:] |= m[:, :-1]; rim[:, :-1] |= m[:, 1:]
        rim &= ~m
        open_rim = (~op[rim]).mean() if rim.any() else 0
        if top[:3].sum() / area > 0.9 and (open_rim > open_min or area > 3000):
            out |= m
    return out

if __name__ == '__main__':
    only = set(sys.argv[1:])
    for l in open(R + 'sheets.txt'):
        p = l.split()
        if len(p) < 11 or p[0] != 'sheet' or (only and p[1] not in only):
            continue
        f, c, n, fw = p[1], p[2], int(p[3]), int(p[4])
        im = np.array(Image.open(R + f'{f}_{c}.png').convert('RGBA'))
        hits = []
        for k in range(n):
            m = slabs2(im[:, k * fw:(k + 1) * fw])
            if m.sum():
                hits.append((k, int(m.sum())))
        if hits:
            print(f, c, hits)
