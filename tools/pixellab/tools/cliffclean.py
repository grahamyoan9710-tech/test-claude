"""Erases a cliff PixelLab drew next to a fighter hanging from a ledge. The fighter's colours are those of
reference frames without the cliff (REF frames); a pixel of another colour that belongs to a blob touching the
cliff's grass (green) or bigger than --min goes. python3 cliffclean.py DIR --ref f_00.png,f_01.png [--frames ...]
Untouched frames are kept in DIR/orig."""
import os, sys
import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from greyfix import label  # noqa: E402


def arg(k, d):
    return sys.argv[sys.argv.index(k) + 1] if k in sys.argv else d


d = sys.argv[1]
refs = arg('--ref', 'f_00.png,f_01.png').split(',')
tol = float(arg('--tol', 10))
minarea = int(arg('--min', 150))
pal = np.unique(np.concatenate([
    (lambda a: a[a[..., 3] > 0][:, :3])(np.asarray(Image.open(os.path.join(arg('--refdir', d), r)).convert('RGBA')))
    for r in refs]), axis=0).astype(int)
names = arg('--frames', None)
files = names.split(',') if names else [f for f in sorted(os.listdir(d)) if f.startswith('f_') and f.endswith('.png') and f not in refs]
for f in files:
    p = os.path.join(d, f)
    keep = os.path.join(d, 'orig', f)
    src = keep if os.path.exists(keep) else p
    a = np.array(Image.open(src).convert('RGBA'))
    op = a[..., 3] > 0
    ys, xs = np.nonzero(op)
    col = a[ys, xs, :3].astype(int)
    dmin = np.full(len(col), 1e9)
    for i in range(0, len(pal), 64):
        dmin = np.minimum(dmin, np.abs(col[:, None, :] - pal[None, i:i + 64, :]).max(-1).min(1))
    alien = np.zeros(op.shape, bool)
    alien[ys[dmin > tol], xs[dmin > tol]] = True
    rgb = a[..., :3].astype(int)
    green = op & (rgb[..., 1] > rgb[..., 0] + 15) & (rgb[..., 1] > rgb[..., 2] + 5)
    lab, n = label(alien | green)
    gone = np.zeros(op.shape, bool)
    for i in range(1, n + 1):
        m = lab == i
        if (m & green).any() or m.sum() >= minarea:
            gone |= m
    if not os.path.exists(keep):
        os.makedirs(os.path.dirname(keep), exist_ok=True)
        Image.fromarray(a).save(keep)
    out = a.copy()
    out[gone] = 0
    Image.fromarray(out).save(p)
    print(f, int(gone.sum()), 'of', int(op.sum()))
