# dechirure.py frames (all light, white) -> straight RGBA PNGs with the glow baked in. The game tints them.
# Usage: post_ko.py <exr_dir> <out_dir>
import os, sys
import numpy as np
from PIL import Image
from post2 import read_exr, gblur, srgb


def frame(path):
    P, a = read_exr(path)
    lum = P.max(axis=2, keepdims=True)
    glow = sum(k * gblur(lum, s) for s, k in ((2.0, 0.45), (7, 0.3), (22, 0.2)))
    S = srgb(P + glow * 0.85)
    a = gblur(np.clip(a, 0, 1)[..., None], 0.6)[..., 0]
    al = np.maximum(a, S.max(axis=2))  # the dark gap covers, light covers as much as it shines
    C = np.where(al[..., None] > 1e-4, S / np.maximum(al[..., None], 1e-4), 0)
    return Image.fromarray((np.dstack([np.clip(C, 0, 1), al]) * 255 + 0.5).astype(np.uint8), 'RGBA')


if __name__ == '__main__':
    src, dst = sys.argv[1], sys.argv[2]
    os.makedirs(dst, exist_ok=True)
    for n in sorted(os.listdir(src)):
        if n.endswith('.exr'):
            frame(os.path.join(src, n)).save(os.path.join(dst, n[:-4] + '.png'))
            print(n, flush=True)
