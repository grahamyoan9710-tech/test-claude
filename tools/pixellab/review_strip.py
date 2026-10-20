"""Review strip: python3 review_strip.py OUT.png SCALE DIR [DIR...]: one row per clip folder, the frames cropped to
their common box and numbered; CUT marks a frame touching the edge of its canvas."""
import os, sys
import numpy as np
from PIL import Image, ImageDraw
out, scale, dirs = sys.argv[1], float(sys.argv[2]), sys.argv[3:]
rows = []
for d in dirs:
    fs = sorted(f for f in os.listdir(d) if f.startswith('f_') and f.endswith('.png'))
    ims = [Image.open(os.path.join(d, f)).convert('RGBA') for f in fs]
    a = np.zeros(ims[0].size[::-1], bool)
    for im in ims:
        a |= np.asarray(im)[:, :, 3] > 0
    ys, xs = np.nonzero(a)
    box = (max(0, xs.min() - 2), max(0, ys.min() - 2), min(a.shape[1], xs.max() + 3), min(a.shape[0], ys.max() + 3))
    cw, ch = box[2] - box[0], box[3] - box[1]
    W, H = int(cw * scale), int(ch * scale)
    row = Image.new('RGBA', ((W + 4) * len(ims), H + 14), (70, 70, 84, 255))
    dr = ImageDraw.Draw(row)
    for i, im in enumerate(ims):
        c = im.crop(box).resize((W, H), Image.NEAREST)
        row.alpha_composite(c, (i * (W + 4), 14))
        edge = (np.asarray(im)[:, :, 3] > 0)
        cut = edge[0].any() or edge[-1].any() or edge[:, 0].any() or edge[:, -1].any()
        dr.text((i * (W + 4) + 2, 1), f"{i}" + (" CUT" if cut else ""), fill=(255, 80, 80, 255) if cut else (255, 255, 0, 255))
    dr.text((row.width - 120, 1), os.path.basename(d.rstrip('/')), fill=(120, 255, 120, 255))
    rows.append(row)
W = max(r.width for r in rows)
o = Image.new('RGBA', (W, sum(r.height + 4 for r in rows)), (20, 20, 24, 255))
y = 0
for r in rows:
    o.alpha_composite(r, (0, y)); y += r.height + 4
o.save(out); print(o.size)
