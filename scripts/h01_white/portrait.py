"""Hakuen's portrait (a smooth upscale, not pixel art): her black cloth to white, outlines kept 2 px thick."""
import sys
import numpy as np
from PIL import Image
from white import hsv, ramp, WHITE, RED

def erode(m, n):
    for _ in range(n):
        p = np.pad(m, 1)
        r = m.copy()
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                r &= p[1 + dy:p.shape[0] - 1 + dy, 1 + dx:p.shape[1] - 1 + dx]
        m = r
    return m

def repaint(img, gold, vmax, grow):
    a = np.asarray(img.convert("RGBA")).astype(float)
    rgb, alpha = a[..., :3], a[..., 3]
    solid = alpha >= 128
    h, s, v = hsv(a)
    dark = solid & (v < vmax) & ((s < 0.6) | (v < 0.15))
    inner = erode(dark, grow)
    out = rgb.copy()
    out[inner] = ramp(WHITE, 0.15 + np.clip(v[inner] / vmax, 0, 1) * 0.85)
    if gold != "keep":
        g = solid & (h >= 12) & (h <= 55) & (s >= 0.42) & (v >= 0.45)
        skinish = (h >= 15) & (h <= 35) & (s < 0.45)
        g &= ~skinish
        out[g] = ramp(RED if gold == "red" else WHITE, (v[g] - 0.45) / 0.55 * 0.9 + 0.1)
    out[90:140, 65:155] = rgb[90:140, 65:155]  # her face stays as it is (eyes)
    return Image.fromarray(np.concatenate([np.clip(out, 0, 255), alpha[..., None]], -1).astype(np.uint8), "RGBA")

if __name__ == "__main__":
    repaint(Image.open(sys.argv[1]), sys.argv[3], float(sys.argv[4]), int(sys.argv[5])).save(sys.argv[2])
