"""Hakuen repainted mostly white with red touches (owner, 10/11): her black cloth turns white (lilac shading like
her sleeves), her gold and orange turn red; outlines, skin, eyes, hair and the reds stay."""
import sys
import numpy as np
from comp import components
from PIL import Image

def hsv(a):
    r, g, b = [a[..., i] / 255.0 for i in range(3)]
    mx, mn = np.max(a[..., :3], -1) / 255.0, np.min(a[..., :3], -1) / 255.0
    d = mx - mn + 1e-9
    h = np.where(mx == r, (g - b) / d % 6, np.where(mx == g, (b - r) / d + 2, (r - g) / d + 4)) * 60
    return h, np.where(mx > 0, (mx - mn) / (mx + 1e-9), 0), mx

# the white of her sleeves, darkest to lightest
WHITE = np.array([(128, 92, 134), (159, 116, 159), (186, 153, 183), (223, 188, 208), (234, 212, 226), (246, 236, 239)], float)
RED = np.array([(61, 6, 9), (134, 20, 39), (183, 23, 43), (210, 27, 42), (240, 32, 38), (250, 96, 90)], float)

def ramp(pal, t):  # t in [0,1] -> colour of the palette
    t = np.clip(t, 0, 1) * (len(pal) - 1)
    i = np.minimum(t.astype(int), len(pal) - 2)
    f = (t - i)[..., None]
    return pal[i] * (1 - f) + pal[i + 1] * f

def repaint(img, gold="red"):
    a = np.asarray(img.convert("RGBA")).astype(float)
    rgb, alpha = a[..., :3], a[..., 3]
    solid = alpha >= 128
    h, s, v = hsv(a)
    dark = solid & (v < 0.30) & ((s < 0.75) | (v < 0.12))   # black cloth (and the outlines)
    # an outline: a dark pixel touching the background or a non-dark colour
    p = np.pad(dark, 1)
    inner = dark.copy()
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            inner &= p[1 + dy:p.shape[0] - 1 + dy, 1 + dx:p.shape[1] - 1 + dx]
    out = rgb.copy()
    # black cloth: its shade (0..0.30) onto the white ramp, kept a little darker than the sleeves
    out[inner] = ramp(WHITE, 0.15 + v[inner] / 0.30 * 0.85)[...]
    # her skin shares its orange shadows with the gold flames: a warm patch that is mostly peach is skin
    warm = solid & (h >= 5) & (h <= 50) & (s >= 0.2) & (v >= 0.4)
    peach = solid & (h >= 20) & (h <= 34) & (s >= 0.25) & (s <= 0.42) & (v >= 0.93)
    lab, n = components(warm)
    frac = np.bincount(lab.ravel(), peach.ravel(), n + 1) / np.maximum(1, np.bincount(lab.ravel(), None, n + 1))
    skin = (lab > 0) & (frac[lab] >= 0.4)
    if gold != "keep":  # gold and orange -> red (or white), by their brightness
        g = solid & ~skin & (h >= 12) & (h <= 55) & (s >= 0.42) & (v >= 0.45)
        out[g] = ramp(RED if gold == "red" else WHITE, (v[g] - 0.45) / 0.55 * 0.9 + 0.1)
    if "--skin" in sys.argv: out[skin] = (0, 255, 0)
    o = np.concatenate([np.clip(out, 0, 255), alpha[..., None]], -1).astype(np.uint8)
    return Image.fromarray(o, "RGBA")

if __name__ == "__main__":
    src, dst = sys.argv[1], sys.argv[2]
    repaint(Image.open(src), gold=sys.argv[3] if len(sys.argv) > 3 else "red").save(dst)
