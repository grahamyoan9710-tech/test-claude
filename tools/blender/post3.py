# eclair6.py's two renders of a frame (smoke: denoised; lines: sharp) -> one straight RGBA PNG with the glow baked
# in, like the strips the game draws. The lines get a tight hot glow and a crimson halo; inside the cloud and the
# smoke they are veiled. Usage: post3.py <render_dir> <out_dir>   (render_dir holds fumee/ and trait/)
import os, sys
import numpy as np
from PIL import Image
from post2 import read_exr, gblur, srgb

RED = np.array([0.871, 0.030, 0.069])  # #F0304A, linear


def frame(smoke_path, lines_path):
    Ps, a_s = read_exr(smoke_path)
    Pl, _ = read_exr(lines_path)
    a_s = gblur(np.clip(a_s, 0, 1)[..., None], 1.4)[..., 0]  # the smoke's coverage is not denoised: smooth it
    bright = np.clip(Ps - 0.45, 0, 2.5)  # only what is hotter than the smoke blooms, capped: no white fog
    glow_s = sum(k * gblur(bright, s) for s, k in ((2.5, 0.5), (9, 0.3), (26, 0.18)))
    L = Pl * (1 - 0.45 * a_s[..., None])
    lum = L.max(axis=2, keepdims=True)
    halo = sum(k * gblur(lum, s) for s, k in ((3.5, 0.5), (11, 0.3), (30, 0.16)))
    S = srgb(Ps + L + glow_s + 0.5 * gblur(L, 1.2) + halo * RED)
    al = np.maximum(a_s, S.max(axis=2))  # light covers as much as it shines
    C = np.where(al[..., None] > 1e-4, S / np.maximum(al[..., None], 1e-4), 0)
    return Image.fromarray((np.dstack([np.clip(C, 0, 1), al]) * 255 + 0.5).astype(np.uint8), 'RGBA')


if __name__ == '__main__':
    src, dst = sys.argv[1], sys.argv[2]
    os.makedirs(dst, exist_ok=True)
    for n in sorted(os.listdir(os.path.join(src, 'trait'))):
        if n.endswith('.exr'):
            frame(os.path.join(src, 'fumee', n), os.path.join(src, 'trait', n)).save(os.path.join(dst, n[:-4] + '.png'))
            print(n, flush=True)
