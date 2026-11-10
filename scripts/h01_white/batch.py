"""Repaint all of Hakuen's clips (not her weapon icons); keeps the paletted PNG when the colours fit in 256."""
import sys, glob, os
import numpy as np
from PIL import Image
from white import repaint

src, dst, mode = sys.argv[1], sys.argv[2], sys.argv[3]
for p in sorted(glob.glob(os.path.join(src, "h01_*.png"))):
    if "_icon_" in p: continue
    im = Image.open(p)
    out = repaint(im, gold=mode)
    a = np.asarray(out)
    if im.mode == "P":
        flat = a.reshape(-1, 4).copy()
        flat[flat[:, 3] == 0] = 0  # one transparent colour
        cols, idx = np.unique(flat, axis=0, return_inverse=True)
        if len(cols) <= 256:
            q = Image.fromarray(idx.reshape(a.shape[:2]).astype(np.uint8), "P")
            q.putpalette(cols[:, :3].ravel().tolist())
            q.info["transparency"] = bytes(cols[:, 3].tolist())
            q.save(os.path.join(dst, os.path.basename(p)), transparency=bytes(cols[:, 3].tolist()))
            continue
        print("too many colours, RGBA:", p, len(cols))
    out.save(os.path.join(dst, os.path.basename(p)))
