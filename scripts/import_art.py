#!/usr/bin/env python3
"""Imports the PixelLab art of the menus and arenas into assets/sprites (run from the repository root):
- arena backdrops  tools/pixellab/pxa/arenas/<key>_bg/f_00.png  -> arena_<key>_bg.png
- platforms        tools/pixellab/pxa/arenas/<key>_<part>/f_00.png -> arena_<key>_<part>.png (solid, thin, small: one
                   picture per platform, generated at the game's size by pxa/arenas/gen_platforms.sh)
- portraits        tools/pixellab/pxa/portraits/<id>/f_NN.png   -> <id>_portrait.png (the candidate picked in PORTRAITS)
- the game's logo  docs/refonte/logo_clash_horizon.png (the owner's, on white) -> menu_logo.png, on transparency
Pixel art is scaled x2 with nearest neighbour (crisp once the game filters it). Prints each platform's walkable top
(pixels below its first row) for render_stages.cpp."""
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

PX = "tools/pixellab/pxa"
OUT = "assets/sprites"
ARENAS = ["lunaire", "givre", "ecarlate", "kurenai", "nuees"]
PARTS = ["solid", "thin", "small"]  # the main ground, the thin platforms, the small islands (Monastère des Nuées)
PLATFORM_PICK = {("givre", "thin"): 3, ("ecarlate", "thin"): 1, ("kurenai", "thin"): 1, ("nuees", "thin"): 1}
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


def cut_out_white(im, band=3):
    """The white around the logo becomes transparent: the near-white area joined to the border (the logo's own whites,
    inside its dark outline, stay), its edge un-blended from the white over a few pixels."""
    a = np.array(im.convert("RGB")).astype(np.float32)
    light = (a.min(axis=2) >= 232) & (a.max(axis=2) - a.min(axis=2) < 24)
    mask = Image.fromarray((light * 255).astype(np.uint8)).copy()  # (an array-backed image ignores floodfill)
    for seed in [(0, 0), (mask.width - 1, 0), (0, mask.height - 1), (mask.width - 1, mask.height - 1)]:
        if mask.getpixel(seed) == 255:
            ImageDraw.floodfill(mask, seed, 128)
    bg = np.array(mask) == 128
    near = np.array(Image.fromarray((bg * 255).astype(np.uint8)).filter(ImageFilter.MaxFilter(2 * band + 1))) > 0
    alpha = np.where(bg, 0.0, 1.0)
    edge = near & ~bg  # colour to alpha against white: the least alpha that explains the pixel
    k = np.clip((255.0 - a[edge].min(axis=1)) / 255.0 * 1.6, 0.0, 1.0)
    alpha[edge] = k
    rgb = a.copy()
    safe = np.maximum(k, 1e-3)[:, None]
    rgb[edge] = np.clip((a[edge] - (1.0 - k)[:, None] * 255.0) / safe, 0.0, 255.0)
    out = np.dstack([rgb, alpha * 255.0]).astype(np.uint8)
    return crop_alpha(Image.fromarray(out, "RGBA"))


def main():
    logo = "docs/refonte/logo_clash_horizon.png"
    if os.path.exists(logo):
        cut = cut_out_white(Image.open(logo))
        cut.resize((1000, round(cut.height * 1000 / cut.width)), Image.LANCZOS).save(f"{OUT}/menu_logo.png", optimize=True)
    for key in ARENAS:
        bg = f"{PX}/arenas/{key}_bg/f_00.png"
        if os.path.exists(bg):
            x2(Image.open(bg).convert("RGB")).save(f"{OUT}/arena_{key}_bg.png")
        for part in PARTS:
            src = f"{PX}/arenas/{key}_{part}/f_{PLATFORM_PICK.get((key, part), 0):02d}.png"
            if os.path.exists(src):
                pic = crop_alpha(Image.open(src).convert("RGBA"))
                x2(pic).save(f"{OUT}/arena_{key}_{part}.png")
                print(f"{key} {part}: walkable top {walk_top(pic) * 2}")
    for pid, pick in PORTRAITS.items():
        src = f"{PX}/portraits/{pid}/f_{pick:02d}.png"
        if os.path.exists(src):
            x2(crop_alpha(clean_portrait(Image.open(src).convert("RGBA")))).save(f"{OUT}/{pid}_portrait.png")
    return 0


if __name__ == "__main__":
    sys.exit(main())
