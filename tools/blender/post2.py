# Blender EXR frames (linear, premultiplied, transparent film) -> straight RGBA PNGs with the glow baked in, like the
# ParticleFX strips the game draws. Usage: post2.py <in_dir> <out_dir> [bloom] [alpha_blur]
import os, subprocess, sys
import numpy as np
from PIL import Image


def read_exr(path):
    """ffmpeg decodes to planar float32 G, B, R, A: linear values as Blender wrote them (premultiplied)."""
    probe = subprocess.run(['ffprobe', '-v', 'error', '-select_streams', 'v:0', '-show_entries', 'stream=width,height',
                            '-of', 'csv=p=0', path], capture_output=True, text=True, check=True).stdout
    w, h = map(int, probe.strip().split(','))
    raw = subprocess.run(['ffmpeg', '-v', 'error', '-i', path, '-f', 'rawvideo', '-pix_fmt', 'gbrapf32le', '-'],
                         capture_output=True, check=True).stdout
    g, b, r, a = np.frombuffer(raw, np.float32).reshape(4, h, w)
    return np.dstack([r, g, b]).astype(np.float64), a.astype(np.float64)


def gblur(x, s):
    """Gaussian blur (sigma s px) by FFT, padded so nothing wraps around."""
    p = int(3 * s) + 1
    xp = np.pad(x, ((p, p), (p, p), (0, 0)))
    hh, ww = xp.shape[:2]
    fy = np.fft.fftfreq(hh)[:, None]
    fx = np.fft.rfftfreq(ww)[None, :]
    g = np.exp(-2 * (np.pi * s) ** 2 * (fx ** 2 + fy ** 2))
    out = np.fft.irfft2(np.fft.rfft2(xp, axes=(0, 1)) * g[..., None], s=(hh, ww), axes=(0, 1))
    return out[p:-p, p:-p]


def srgb(x):
    x = np.clip(x, 0, 1)
    return np.where(x <= 0.0031308, 12.92 * x, 1.055 * np.power(x, 1 / 2.4) - 0.055)


def frame(path, bloom=1.0, soft=1.4):
    P, a = read_exr(path)
    bright = np.clip(P - 0.45, 0, 2.5)  # only what is hotter than the smoke blooms, capped: no white fog
    glow = sum(k * gblur(bright, s) for s, k in ((2.5, 0.5), (9, 0.3), (26, 0.18)))
    S = srgb(P + bloom * glow)
    a = gblur(np.clip(a, 0, 1)[..., None], soft)[..., 0]  # the smoke's coverage is not denoised by Cycles: smooth it
    al = np.maximum(np.clip(a, 0, 1), S.max(axis=2))  # light covers as much as it shines
    C = np.where(al[..., None] > 1e-4, S / np.maximum(al[..., None], 1e-4), 0)
    return Image.fromarray((np.dstack([np.clip(C, 0, 1), al]) * 255 + 0.5).astype(np.uint8), 'RGBA')


if __name__ == '__main__':
    src, dst = sys.argv[1], sys.argv[2]
    bloom = float(sys.argv[3]) if len(sys.argv) > 3 else 1.0
    soft = float(sys.argv[4]) if len(sys.argv) > 4 else 1.4
    os.makedirs(dst, exist_ok=True)
    for n in sorted(os.listdir(src)):
        if n.endswith('.exr'):
            frame(os.path.join(src, n), bloom, soft).save(os.path.join(dst, n[:-4] + '.png'))
            print(n)
