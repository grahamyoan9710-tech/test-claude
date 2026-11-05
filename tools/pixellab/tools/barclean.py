"""Erases a bar (log, branch, ledge) PixelLab drew across the top of a hanging fighter: python3 barclean.py DIR
[--x0 X --x1 X] [--arms W] [--from N (first frame with the bar)]. Within columns x0..x1, the bar is the run of rows from the top where the drawing is at
least --arms px wide (wider than the arms holding it);
in the bar's rows, only what is above the arms (the hands gripping it) stays. Originals are kept in DIR/orig."""
import os, shutil, sys
import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from greyfix import label  # noqa: E402


def arg(k, d):
    return sys.argv[sys.argv.index(k) + 1] if k in sys.argv else d


d = sys.argv[1]
arms = int(arg('--arms', 60))
os.makedirs(os.path.join(d, 'orig'), exist_ok=True)
first = int(arg('--from', 0))
for f in sorted(x for x in os.listdir(d) if x.startswith('f_') and x.endswith('.png') and int(x[2:-4]) >= first):
    p = os.path.join(d, f)
    if not os.path.exists(os.path.join(d, 'orig', f)):
        shutil.copy(p, os.path.join(d, 'orig', f))
    a = np.array(Image.open(os.path.join(d, 'orig', f)).convert('RGBA'))
    x0, x1 = int(arg('--x0', 0)), int(arg('--x1', a.shape[1]))
    op = a[..., 3] > 0
    win = op[:, x0:x1]
    width = np.array([(np.ptp(np.where(r)[0]) + 1) if r.any() else 0 for r in win])
    rows = np.where(win.any(1))[0]
    if not len(rows):
        continue
    top = rows[0]
    limit = top + (rows[-1] - top) * 0.4
    wide = [y for y in rows if y <= limit and width[y] >= arms]
    if not wide:
        Image.fromarray(a).save(p)
        continue
    y0 = y1 = wide[0]
    while y1 + 1 < len(width) and width[y1 + 1] >= arms:
        y1 += 1
    below = np.where(win[min(y1 + 3, win.shape[0] - 1)])[0] + x0  # the arms
    al, ar = (below.min(), below.max()) if len(below) else (x1, x0)
    under = (a[y1 + 1:y1 + 9, :, 3] > 0).any(0)  # the arms' columns just under the bar, widened a little
    cols = under.copy()
    for k in range(1, 4):
        cols[k:] |= under[:-k]
        cols[:-k] |= under[k:]
    band = a[y0:y1 + 1]
    drop = (band[..., 3] > 0) & ~cols[None, :]
    drop[:, :x0] = False
    drop[:, x1:] = False
    band[drop] = 0
    left = band[..., 3] > 0  # specks of the bar in skin-like colours, cut off from the arms
    lab, n = label(left)
    for i in range(1, n + 1):
        m = lab == i
        if m.sum() < 12 and not m[-1].any():
            band[m] = 0
    a[y0:y1 + 1] = band
    Image.fromarray(a).save(p)
    print(f, 'bar rows', y0, y1, 'arms', al, ar, 'erased', int(drop.sum()))
