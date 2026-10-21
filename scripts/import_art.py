#!/usr/bin/env python3
"""Imports the PixelLab art of the menus and arenas into assets/sprites (run from the repository root):
- arena backdrops  tools/pixellab/pxa/arenas/<key>_bg/f_00.png  -> arena_<key>_bg.png
- platform kits    tools/pixellab/pxa/arenas/<key>_kit/f_00.png -> arena_<key>_solid.png (left piece), _thin.png (right)
- portraits        tools/pixellab/pxa/portraits/<id>/f_NN.png   -> <id>_portrait.png (the candidate picked in PORTRAITS)
Pixel art is scaled x2 with nearest neighbour (crisp once the game filters it). Prints each platform's walkable top
(pixels below its first row) for render_stages.cpp."""
import os
import sys

import numpy as np
from PIL import Image

PX = "tools/pixellab/pxa"
OUT = "assets/sprites"
ARENAS = ["lunaire", "givre", "ecarlate", "kurenai", "nuees"]
PORTRAITS = {"p01": 0, "p02": 0, "p03": 0, "p07": 0, "p08": 3}  # the candidate kept for each fighter


def x2(im):
    return im.resize((im.width * 2, im.height * 2), Image.NEAREST)


def crop_alpha(im):
    return im.crop(im.getchannel("A").getbbox())


def walk_top(im):
    """The first row covered over 60% of the width: the walkable top (railings and posts above it are sparse)."""
    a = np.array(im.getchannel("A")) > 40
    cover = a.sum(axis=1) / a.shape[1]
    return int(np.argmax(cover >= 0.6))


def split_kit(im):
    """The two pieces of a kit, cut at the widest run of empty columns between them."""
    used = (np.array(im.getchannel("A")) > 40).any(axis=0)
    xs = np.flatnonzero(used)
    best, start = (0, 0), None
    for x in range(xs[0], xs[-1] + 1):
        if not used[x] and start is None:
            start = x
        if used[x] and start is not None:
            best, start = max(best, (x - start, start)), None
    gap, at = best
    assert gap >= 4, "no empty gap between the two platforms"
    return crop_alpha(im.crop((0, 0, at, im.height))), crop_alpha(im.crop((at + gap, 0, im.width, im.height)))


def clean_portrait(im):
    """Clears the light frame line the model sometimes draws along the bottom and left edges."""
    a = np.array(im)
    light = (a[..., 3] > 40) & (a[..., :3].min(axis=2) > 200)  # light pixels inside (hair, highlights) are kept
    for r in range(1, 4):  # bottom rows mostly light: a frame line
        if light[-r].mean() > 0.4:
            a[-r, :, 3] = 0
    for c in range(0, 3):  # left columns mostly light
        if light[:, c].mean() > 0.4:
            a[:, c, 3] = 0
    return Image.fromarray(a)


def main():
    for key in ARENAS:
        bg = f"{PX}/arenas/{key}_bg/f_00.png"
        if os.path.exists(bg):
            x2(Image.open(bg).convert("RGB")).save(f"{OUT}/arena_{key}_bg.png")
        kit = f"{PX}/arenas/{key}_kit/f_00.png"
        if os.path.exists(kit):
            solid, thin = split_kit(Image.open(kit).convert("RGBA"))
            x2(solid).save(f"{OUT}/arena_{key}_solid.png")
            x2(thin).save(f"{OUT}/arena_{key}_thin.png")
            print(f'{key}: solidTop {walk_top(solid) * 2}, thinTop {walk_top(thin) * 2}')
    for pid, pick in PORTRAITS.items():
        src = f"{PX}/portraits/{pid}/f_{pick:02d}.png"
        if os.path.exists(src):
            x2(crop_alpha(clean_portrait(Image.open(src).convert("RGBA")))).save(f"{OUT}/{pid}_portrait.png")
    return 0


if __name__ == "__main__":
    sys.exit(main())
