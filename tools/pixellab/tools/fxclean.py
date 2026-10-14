"""Erases the blue/white energy PixelLab adds to a clip that must not have any (a dodge is not an attack):
connected blobs of bluish (--hue blue), yellow, red, warm (yellow and red) or all of those, with the near-white
pixels touching them. python3 fxclean.py DIR [--min 40] [--hue blue|yellow|red|warm|purple|all]. Untouched frames go to DIR/orig."""
import os, sys
import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from greyfix import label  # noqa: E402

d = sys.argv[1]
minarea = int(sys.argv[sys.argv.index('--min') + 1]) if '--min' in sys.argv else 40
for f in sorted(os.listdir(d)):
    if not (f.startswith('f_') and f.endswith('.png')) or f == 'f_00.png':
        continue
    p = os.path.join(d, f)
    a = np.array(Image.open(p).convert('RGBA'))
    rgb = a[..., :3].astype(int); op = a[..., 3] > 0
    hue = sys.argv[sys.argv.index('--hue') + 1] if '--hue' in sys.argv else 'blue'
    yellow = op & (rgb[..., 0] > 190) & (rgb[..., 1] > 140) & (rgb[..., 2] < rgb[..., 1] - 60)
    red = op & (rgb[..., 0] > 150) & (rgb[..., 0] > rgb[..., 1] + 70) & (rgb[..., 2] < 100)  # red and orange fire
    bluish = op & (rgb[..., 2] > rgb[..., 0] + 50) & (rgb[..., 2] > 120)
    purple = op & (rgb[..., 2] > 150) & (rgb[..., 0] > 110) & (rgb[..., 1] < rgb[..., 0] - 40)  # violet and magenta
    blue = {'yellow': yellow, 'red': red, 'warm': yellow | red, 'blue': bluish, 'purple': purple,
            'all': bluish | yellow | red | purple}[hue]
    white = op & (rgb.min(-1) > 215)
    fx = blue | white
    lab, n = label(fx)
    gone = np.zeros_like(fx)
    for i in range(1, n + 1):
        m = lab == i
        if m.sum() >= minarea and (blue & m).sum() >= 0.25 * m.sum():
            gone |= m
    if gone.any():
        keep = os.path.join(d, 'orig', f)
        if not os.path.exists(keep):
            os.makedirs(os.path.dirname(keep), exist_ok=True)
            Image.fromarray(a).save(keep)
        a[gone] = 0
        Image.fromarray(a).save(p)
        print(f, int(gone.sum()))
