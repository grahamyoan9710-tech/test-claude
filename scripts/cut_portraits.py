# Cut the 10 portraits out of the owner's sheet (art/portraits/planche_10_persos.png): crop each panel minus its
# number plate, flood the white background from the panel's edges, soften the ink edge.
# Usage: python3 scripts/cut_portraits.py art/portraits/planche_10_persos.png art/portraits
# The game draws <id>_art (assets/sprites/h01_art.png = portrait_01.png); its face centre is in render_body.cpp.
import sys
import numpy as np
from PIL import Image

XS = [(11, 299), (317, 606), (622, 913), (930, 1220), (1236, 1526)]
YS = [(11, 503), (519, 1009)]
LABEL = 50  # the "01" plate at the bottom left
FADE = 14   # px


def dilate(m):
    o = m.copy()
    o[1:] |= m[:-1]; o[:-1] |= m[1:]; o[:, 1:] |= m[:, :-1]; o[:, :-1] |= m[:, 1:]
    return o


def cut(p):
    p = p.astype(np.float32)
    lo, hi = p.min(2), p.max(2)
    white = (lo > 232) & (hi - lo < 14)
    seed = np.zeros_like(white)
    seed[0, :] = seed[:, 0] = seed[:, -1] = True  # not the bottom: the bust is cut there
    bg = seed & white
    while True:
        nb = dilate(bg) & white
        if nb.sum() == bg.sum():
            break
        bg = nb
    alpha = np.where(bg, 0.0, 1.0)
    ring = dilate(bg) & ~bg  # the line pixels touching the background: blend them out of the white
    ring2 = dilate(ring | bg) & ~bg & ~ring
    for r, k in ((ring, 1.0), (ring2, 0.6)):
        a = np.clip((255.0 - lo) / 200.0, 0.0, 1.0)
        a = 1.0 - (1.0 - a) * k
        alpha = np.where(r, a, alpha)
    # The art cut by the panel's left, top and right borders (hair) fades out instead of stopping on a straight line
    hgt, wid = alpha.shape
    ramp = np.clip(np.arange(max(hgt, wid)) / float(FADE), 0.0, 1.0)
    alpha = alpha * ramp[:wid][None, :] * ramp[:wid][::-1][None, :] * ramp[:hgt][:, None]
    a3 = np.maximum(alpha, 1e-3)[..., None]
    rgb = np.clip((p - (1.0 - a3) * 255.0) / a3, 0, 255)
    return np.dstack([rgb, alpha * 255.0]).astype(np.uint8)


def main():
    sheet = np.asarray(Image.open(sys.argv[1]).convert('RGB'))
    out = sys.argv[2]
    n = 0
    for y0, y1 in YS:
        for x0, x1 in XS:
            n += 1
            p = sheet[y0 + 3:y1 - 2 - LABEL, x0 + 3:x1 - 2]
            Image.fromarray(cut(p), 'RGBA').save(f'{out}/portrait_{n:02d}.png')


if __name__ == '__main__':
    main()
