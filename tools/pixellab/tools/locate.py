"""Finds Naruto (saturated orange clothes, yellow hair) in recorded match frames and crops around him."""
import sys, os
import numpy as np
from PIL import Image

src, dst = sys.argv[1], sys.argv[2]
os.makedirs(dst, exist_ok=True)
files = sorted(f for f in os.listdir(src) if f.endswith('.png'))
rows = []
for f in files:
    a = np.asarray(Image.open(os.path.join(src, f)).convert('RGB')).astype(int)
    r, g, b = a[..., 0], a[..., 1], a[..., 2]
    m = (r > 200) & (g > 95) & (g < 175) & (b < 80) & (r - g > 55)
    m[585:, :] = False  # HUD
    ys, xs = np.nonzero(m)
    if len(xs) < 40:
        rows.append((f, None))
        continue
    cx, cy = int(np.median(xs)), int(np.median(ys))
    rows.append((f, (cx, cy, len(xs))))
with open(os.path.join(dst, 'where.txt'), 'w') as o:
    for f, w in rows:
        o.write(f"{f} {w}\n")
print(len(rows), 'frames')
