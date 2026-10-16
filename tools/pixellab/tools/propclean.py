"""Erases scenery PixelLab drew next to a fighter (the rock of a ledge): pixels whose colour is far from every
colour of the fighter's reference clip, within the top part of the frame, then the specks left over.
python3 propclean.py DIR REFDIR [--top 0.5] [--dist 28] [--frames f_08.png,...]  (untouched frames: DIR/orig)"""
import os, sys
import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from greyfix import label  # noqa: E402


def arg(k, d):
    return sys.argv[sys.argv.index(k) + 1] if k in sys.argv else d


d, ref = sys.argv[1], sys.argv[2]
top, dist = float(arg('--top', 0.5)), float(arg('--dist', 28))
names = arg('--frames', None)
pal = []
for f in sorted(os.listdir(ref)):
    if f.startswith('f_') and f.endswith('.png'):
        a = np.asarray(Image.open(os.path.join(ref, f)).convert('RGBA'))
        pal.append(a[a[..., 3] > 0][:, :3])
pal = np.unique(np.concatenate(pal), axis=0).astype(int)
files = names.split(',') if names else [f for f in sorted(os.listdir(d)) if f.startswith('f_') and f.endswith('.png') and f != 'f_00.png']
for f in files:
    p = os.path.join(d, f)
    a = np.array(Image.open(p).convert('RGBA'))
    h = a.shape[0]
    op = a[..., 3] > 0
    ys, xs = np.nonzero(op)
    col = a[ys, xs, :3].astype(int)
    dmin = np.full(len(col), 1e9)
    for i in range(0, len(pal), 64):
        dd = np.sqrt(((col[:, None, :] - pal[None, i:i + 64, :]) ** 2).sum(-1)).min(1)
        dmin = np.minimum(dmin, dd)
    far = (dmin > dist) & (ys < top * h)
    gone = np.zeros(op.shape, bool)
    gone[ys[far], xs[far]] = True
    rest = op & ~gone
    lab, n = label(rest & (np.arange(h)[:, None] < top * h))
    for i in range(1, n + 1):
        m = lab == i
        if m.sum() < 25:
            gone |= m
    keep = os.path.join(d, 'orig', f)
    if not os.path.exists(keep):
        os.makedirs(os.path.dirname(keep), exist_ok=True)
        Image.fromarray(a).save(keep)
    a[gone] = 0
    Image.fromarray(a).save(p)
    print(f, int(gone.sum()))
