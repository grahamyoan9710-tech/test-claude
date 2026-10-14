"""Animated GIF of recorded frames A..B, cropped on a box that follows Naruto (orange) smoothly."""
import sys, os
import numpy as np
from PIL import Image

src, a, b, out = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
size = int(sys.argv[5]) if len(sys.argv) > 5 else 420
scale = float(sys.argv[6]) if len(sys.argv) > 6 else 1.0
frames, pts = [], []
for f in range(a, b + 1):
    p = os.path.join(src, f"f_{f:05d}.png")
    im = Image.open(p).convert('RGB')
    arr = np.asarray(im).astype(int)
    r, g, bb = arr[..., 0], arr[..., 1], arr[..., 2]
    m = (r > 200) & (g > 95) & (g < 175) & (bb < 80) & (r - g > 55)
    m[585:, :] = False
    ys, xs = np.nonzero(m)
    pts.append((np.median(xs), np.median(ys)) if len(xs) > 40 else None)
    frames.append(im)
known = [q for q in pts if q is not None]
cx = float(np.median([q[0] for q in known])); cy = float(np.median([q[1] for q in known]))
W, H = frames[0].size
x0 = int(min(max(0, cx - size / 2), W - size)); y0 = int(min(max(0, cy - size * 0.55), H - size))
out_frames = [im.crop((x0, y0, x0 + size, y0 + size)) for im in frames]
if scale != 1.0:
    out_frames = [im.resize((int(size * scale), int(size * scale)), Image.LANCZOS) for im in out_frames]
pal = [im.convert('P', palette=Image.ADAPTIVE, colors=255) for im in out_frames]
pal[0].save(out, save_all=True, append_images=pal[1:], duration=20, loop=0, optimize=False, disposal=1)
print(out, len(pal), 'frames, crop', (x0, y0, size))
