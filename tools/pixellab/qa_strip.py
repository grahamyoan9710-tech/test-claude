"""python3 qa_strip.py OUT.png DIR [DIR...]: one row per clip dir (frames half size, on grey)."""
import os, sys
from PIL import Image
rows = []
for d in sys.argv[2:]:
    fs = sorted(f for f in os.listdir(d) if f.startswith('f_') and f.endswith('.png'))
    ims = [Image.open(os.path.join(d, f)).convert('RGBA') for f in fs]
    w, h = ims[0].size
    row = Image.new('RGBA', (w // 2 * len(ims), h // 2), (60, 60, 80, 255))
    for i, im in enumerate(ims):
        row.alpha_composite(im.resize((w // 2, h // 2)), (i * w // 2, 0))
    rows.append(row)
W = max(r.size[0] for r in rows)
out = Image.new('RGBA', (W, sum(r.size[1] for r in rows)), (60, 60, 80, 255))
y = 0
for r in rows:
    out.alpha_composite(r, (0, y)); y += r.size[1]
out.save(sys.argv[1])
