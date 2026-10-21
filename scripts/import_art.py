#!/usr/bin/env python3
"""Imports the PixelLab art of the menus and arenas into assets/sprites (run from the repository root):
- arena backdrops  tools/pixellab/pxa/arenas/<key>_bg/f_00.png  -> arena_<key>_bg.png
- platform kits    tools/pixellab/pxa/arenas/<key>_kit/f_00.png -> arena_<key>_solid.png (left piece), _thin.png (right)
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
MIRRORED = {"kurenai", "ecarlate"}  # the main platform cut at the kit's left edge: its right half, mirrored, makes the left
TOP_CUT = {"ecarlate"}  # the thin platform's ropes run out of the picture: only the deck is kept
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
        kit = f"{PX}/arenas/{key}_kit/f_00.png"
        if os.path.exists(kit):
            solid, thin = split_kit(Image.open(kit).convert("RGBA"))
            if key in MIRRORED:
                half = solid.crop((solid.width // 2, 0, solid.width, solid.height))
                solid = Image.new("RGBA", (half.width * 2, solid.height))
                solid.paste(half.transpose(Image.FLIP_LEFT_RIGHT), (0, 0))
                solid.paste(half, (half.width, 0))
            if key in TOP_CUT:
                thin = crop_alpha(thin.crop((0, max(0, walk_top(thin) - 3), thin.width, thin.height)))
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
