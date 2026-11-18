"""floaters.py DIRS...: detached blobs lying more than 4 px from the body (stray dots, dust) in every frame."""
import sys, os
import numpy as np
from PIL import Image
def comps(a):
    h, w = a.shape; lab = np.zeros(a.shape, int); n = 0; out = []
    for y, x in zip(*np.nonzero(a)):
        if lab[y, x]: continue
        n += 1; st = [(y, x)]; lab[y, x] = n; pts = []
        while st:
            cy, cx = st.pop(); pts.append((cy, cx))
            for dy in (-1, 0, 1):
                for dx in (-1, 0, 1):
                    yy, xx = cy + dy, cx + dx
                    if 0 <= yy < h and 0 <= xx < w and a[yy, xx] and not lab[yy, xx]: lab[yy, xx] = n; st.append((yy, xx))
        out.append(pts)
    return sorted(out, key=len, reverse=True)
bad = 0
for d in sys.argv[1:]:
    for f in sorted(x for x in os.listdir(d) if x.startswith("f_")):
        im = np.array(Image.open(os.path.join(d, f)).convert("RGBA"))
        cs = comps(im[..., 3] > 0)
        body = np.array(cs[0])
        for c in cs[1:]:
            pts = np.array(c)
            dist = min(np.abs(body - p).sum(1).min() for p in pts)
            if dist > 4:
                bad += 1
                y, x = c[0]
                print(f"{d}/{f}: {len(c)} px at ({x},{y}), {dist} px from the body, rgba {tuple(int(v) for v in im[y, x])}")
print("floaters:", bad)
