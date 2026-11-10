#!/usr/bin/env python3
"""Imports a PixelLab effect into the game: python3 scripts/import_fx.py NAME SOURCE [options]

SOURCE is either
- the spritesheet PNG exported by PixelLab's vfx_workbench (its JSON, same name with .json, gives the frames, the
  frame time and the pivot),
- a folder of frames f_00.png, f_01.png... returned by animate_image_pixminimax (f_00 is the input picture: left
  out unless --keep-first), or
- a GIF with a transparent background, such as a ParticleFX Studio export (its frame time gives the fps).
Options: --anchor X Y (the effect's origin in the source frame: where the game puts it; default: the JSON pivot,
else the frame centre), --fps N (default: from the JSON or the GIF, else 15), --loop, --only A-B (frames to keep),
--keep-first, --step N (keep one frame in N: a 30 fps GIF at the game's 15 fps), --scale S (resize the frames:
each effect is one texture, keep its strip under 8192 px), --flip (mirror the frames: the game expects effects
travelling to the right), --track (keep the effect's centre still, its anchor x: for one that travels across its
frames, drawn on a moving projectile), --hit R (the radius, in frame pixels after --scale, the hitbox should have to
match the drawing: the game then sizes the effect on the hitbox), --pixel (pixel art made at its final size, one source
pixel block per fighter pixel: --scale keeps its pixels (nearest) and the game draws it with the fighters' pixel size,
never stretched), --soft (keeps the see-through pixels: otherwise the alpha is all or nothing).

Writes assets/sprites/fx_NAME.png (the frames side by side, cropped to their common box, paletted when it fits)
and its line in assets/sprites/sheets.txt: fx NAME frames frame_w frame_h anchor_x anchor_y fps loop [hit [pixel]].
"""
import argparse
import json
import os

import numpy as np
from PIL import Image, ImageSequence


def paletted(img):
    arr = np.asarray(img)
    if ((arr[..., 3] > 0) & (arr[..., 3] < 255)).any():  # see-through pixels: kept as RGBA
        return img
    opaque = arr[..., 3] > 0
    cols = np.unique(arr[opaque][:, :3], axis=0)
    if len(cols) > 255:
        return img
    key = (arr[..., 0].astype(np.uint32) << 16) | (arr[..., 1].astype(np.uint32) << 8) | arr[..., 2]
    keys = (cols[:, 0].astype(np.uint32) << 16) | (cols[:, 1].astype(np.uint32) << 8) | cols[:, 2]
    idx = (np.searchsorted(keys, key) + 1).astype(np.uint8)
    idx[~opaque] = 0
    p = Image.fromarray(idx, "P")
    pal = [0, 0, 0] + [int(v) for c in cols for v in c]
    p.putpalette(pal + [0] * (768 - len(pal)))
    return p


def load(a):
    """The frames (RGBA, same size), the anchor in them, the fps."""
    if os.path.isdir(a.source):
        names = sorted(f for f in os.listdir(a.source) if f.startswith("f_") and f.endswith(".png"))
        idx = [int(n[2:-4]) for n in names]
        if not a.keep_first:
            idx = [i for i in idx if i > 0] or idx
        frames = [Image.open(os.path.join(a.source, f"f_{i:02d}.png")).convert("RGBA") for i in idx]
        w, h = frames[0].size
        return frames, (w / 2, h / 2), 15.0
    if a.source.lower().endswith(".gif"):
        frames, ms = [], []
        for f in ImageSequence.Iterator(Image.open(a.source)):
            frames.append(f.convert("RGBA"))
            ms.append(f.info.get("duration", 67))
        w, h = frames[0].size
        return frames, (w / 2, h / 2), 1000.0 / max(1.0, sum(ms) / len(ms))
    sheet = Image.open(a.source).convert("RGBA")
    meta = json.load(open(os.path.splitext(a.source)[0] + ".json"))
    fr = meta["frames"]
    fr = fr if isinstance(fr, list) else list(fr.values())
    frames = []
    for f in fr:
        r = f["frame"]
        frames.append(sheet.crop((r["x"], r["y"], r["x"] + r["w"], r["y"] + r["h"])))
    piv = fr[0].get("pivot", {"x": 0.5, "y": 0.5})
    w, h = frames[0].size
    dur = fr[0].get("duration", 67)
    return frames, (piv["x"] * w, piv["y"] * h), 1000.0 / max(1, dur)


def centre_x(f):
    """The x of the centre of the drawn pixels, None for an empty frame."""
    alpha = np.asarray(f.getchannel("A"), dtype=np.float64).sum(axis=0)
    return float((alpha * np.arange(len(alpha))).sum() / alpha.sum()) if alpha.sum() else None


def tracked(frames):
    """The frames moved sideways so that their centre (of the drawn pixels) stays at one x, and that x."""
    cx = [centre_x(f) for f in frames]
    seen = [c for c in cx if c is not None]
    mid = float(np.median(seen))
    pad = int(np.ceil(max(abs(mid - c) for c in seen)))
    out = []
    for f, c in zip(frames, cx):
        g = Image.new("RGBA", (f.width + 2 * pad, f.height), (0, 0, 0, 0))
        g.alpha_composite(f, (pad + (round(mid - c) if c is not None else 0), 0))
        assert c is None or abs(centre_x(g) - (mid + pad)) <= 1.0
        out.append(g)
    return out, mid + pad


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("name")
    ap.add_argument("source")
    ap.add_argument("--anchor", type=float, nargs=2, default=None)
    ap.add_argument("--fps", type=float, default=None)
    ap.add_argument("--loop", action="store_true")
    ap.add_argument("--only", default=None)
    ap.add_argument("--keep-first", action="store_true")
    ap.add_argument("--flip", action="store_true")
    ap.add_argument("--hit", type=float, default=0.0)
    ap.add_argument("--step", type=int, default=1)
    ap.add_argument("--scale", type=float, default=1.0)
    ap.add_argument("--track", action="store_true")
    ap.add_argument("--pixel", action="store_true")
    ap.add_argument("--soft", action="store_true")
    a = ap.parse_args()

    frames, (ax, ay), fps = load(a)
    if a.only:
        lo, hi = (int(v) for v in a.only.split("-"))
        frames = frames[lo:hi + 1]
    frames, fps = frames[::a.step], fps / a.step
    if a.anchor:
        ax, ay = a.anchor
    if a.fps:
        fps = a.fps
    if a.scale != 1.0:
        size = (round(frames[0].width * a.scale), round(frames[0].height * a.scale))
        frames = [f.resize(size, Image.Resampling.NEAREST if a.pixel else Image.Resampling.LANCZOS) for f in frames]
        ax, ay = ax * a.scale, ay * a.scale
    w, h = frames[0].size
    if a.flip:
        frames = [f.transpose(Image.Transpose.FLIP_LEFT_RIGHT) for f in frames]
        ax = w - ax
    if a.track:
        frames, ax = tracked(frames)
        w = frames[0].width
    for f in frames if not a.soft else []:  # pixel art: hard alpha (--soft: a see-through effect, like a bubble)
        f.putalpha(f.getchannel("A").point(lambda v: 255 if v >= 128 else 0))
    boxes = [f.getbbox() for f in frames if f.getbbox()]
    x0 = max(0, min(b[0] for b in boxes) - 1)
    y0 = max(0, min(b[1] for b in boxes) - 1)
    x1 = min(w, max(b[2] for b in boxes) + 1)
    y1 = min(h, max(b[3] for b in boxes) + 1)
    fw, fh = x1 - x0, y1 - y0
    strip = Image.new("RGBA", (fw * len(frames), fh), (0, 0, 0, 0))
    for k, f in enumerate(frames):
        strip.alpha_composite(f.crop((x0, y0, x1, y1)), (k * fw, 0))
    root = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
    out = os.path.join(root, "assets", "sprites", f"fx_{a.name}.png")
    p = paletted(strip)
    if p.mode == "P":
        p.save(out, optimize=True, transparency=0)
    else:
        p.save(out, optimize=True)
    index = os.path.join(root, "assets", "sprites", "sheets.txt")
    lines = open(index).read().splitlines() if os.path.exists(index) else []
    header = [l for l in lines if l.startswith("#")]
    body = [l for l in lines if l.strip() and not l.startswith("#") and not l.split()[:2] == ["fx", a.name]]
    line = f"fx {a.name} {len(frames)} {fw} {fh} {ax - x0:.1f} {ay - y0:.1f} {fps:g} {1 if a.loop else 0}"
    body.append(line + (f" {a.hit:g}" if a.hit > 0 or a.pixel else "") + (" 1" if a.pixel else ""))
    open(index, "w").write("\n".join(header + sorted(body)) + "\n")
    print(f"fx {a.name}: {len(frames)} frames {fw}x{fh} @ {fps:g} fps, {os.path.getsize(out) // 1024} KB")


if __name__ == "__main__":
    main()
