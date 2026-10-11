#!/usr/bin/env python3
"""Cuts a fighter illustration into parts the game can animate with its skeleton.

    python3 scripts/rig_art.py art/fighters/naruto.json [--debug DIR]

The JSON file names the cut-out picture (see import_portrait.py) and marks the joints on it, in pixels:

    {
      "image": "naruto.png",              # next to the JSON file
      "output": "assets/fighters/naruto", # writes .png (the parts, packed) and .rig (joints and parts)
      "ground": 930, "top": 55,           # sole line and top of the skull: the fighter's height
      "joints": {"neck": [x, y], "head": [x, y], "hips": [x, y],   # head: centre of the skull
                 "shF": .., "elbowF": .., "handF": .., "shB": .., "elbowB": .., "handB": ..,  # hand: the fist
                 "hipF": .., "kneeF": .., "ankleF": .., "footF": .., "toeF": .., (and the B leg)},
                 # foot: the sole under the ankle, on the ground line (the far foot looks higher: depth)
      "radius": {"armF": [r_shoulder, r_elbow, r_hand], "handF": r, "legF": [r_hip, r_knee, r_ankle],
                 "footF": [r_ankle, r_toe], "heelF": [r_ankle, r_sole], "torso": r, ...},
      "blend": {"neck": px, "shoulder": px, "elbow": px, "hip": px, "knee": px, "ankle": px},  # bend zones
      "neckCut": [[x, y], ...],           # line between the head and the collar
      "close": 22                         # how far the torso is filled in under the limbs (px)
    }

F is the arm / leg on the side the fighter faces (to the right of the picture), drawn in front of the body;
B is the other one, drawn behind it. A picture without limb joints ("parts": "whole") is only a portrait.

The parts: legB, armB (behind the body), torso, legF, head, armF (in front). Each limb takes the pixels around
its bones (tapered capsules); the head takes what hangs from the skull above the neck cut; the torso takes the
rest. Under the parts drawn in front of it, the torso is filled in from its own colours, so a limb that moves
away does not leave a hole.
"""
import argparse
import json
import os

import numpy as np
from PIL import Image

PAD = 12  # transparent margin around each part in the atlas (mipmaps must not bleed between parts)
ATLAS_W = 1024


def capsule_chain(px, py, pts, radii):
    """Pixels within a tapered capsule along the polyline pts (radius interpolated along each segment)."""
    inside = np.zeros(px.shape, dtype=bool)
    edge = np.full(px.shape, -1e9, dtype=np.float32)  # margin to the capsule edge (> 0 inside)
    for (a, b), (ra, rb) in zip(zip(pts, pts[1:]), zip(radii, radii[1:])):
        ax, ay = a
        dx, dy = b[0] - ax, b[1] - ay
        l2 = max(dx * dx + dy * dy, 1e-6)
        t = np.clip(((px - ax) * dx + (py - ay) * dy) / l2, 0.0, 1.0)
        d = np.hypot(px - (ax + t * dx), py - (ay + t * dy))
        m = (ra + t * (rb - ra)) - d
        edge = np.maximum(edge, m)
    inside = edge >= 0.0
    return inside, edge


def disc(px, py, c, r):
    d = r - np.hypot(px - c[0], py - c[1])
    return d >= 0.0, d


def dilate(m, n):
    for _ in range(n):
        g = m.copy()
        g[1:, :] |= m[:-1, :]
        g[:-1, :] |= m[1:, :]
        g[:, 1:] |= m[:, :-1]
        g[:, :-1] |= m[:, 1:]
        if _ % 2:  # alternate 4- and 8-neighbourhoods: an octagon, close to a disc
            g[1:, 1:] |= m[:-1, :-1]
            g[:-1, :-1] |= m[1:, 1:]
            g[1:, :-1] |= m[:-1, 1:]
            g[:-1, 1:] |= m[1:, :-1]
        m = g
    return m


def erode(m, n):
    return ~dilate(~m, n)


def flood(mask, seed):
    from collections import deque
    h, w = mask.shape
    out = np.zeros_like(mask)
    sx, sy = seed
    if not mask[sy, sx]:
        raise SystemExit(f"flood seed {seed} is not on the picture")
    out[sy, sx] = True
    q = deque([(sx, sy)])
    while q:
        x, y = q.popleft()
        for nx, ny in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
            if 0 <= nx < w and 0 <= ny < h and mask[ny, nx] and not out[ny, nx]:
                out[ny, nx] = True
                q.append((nx, ny))
    return out


def raster_line(mask, pts, width=2):
    """Draws a polyline into a boolean mask (a barrier for the flood fill)."""
    h, w = mask.shape
    for a, b in zip(pts, pts[1:]):
        n = int(max(abs(b[0] - a[0]), abs(b[1] - a[1]))) * 2 + 1
        for i in range(n + 1):
            t = i / n
            x, y = a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t
            for oy in range(-width, width + 1):
                for ox in range(-width, width + 1):
                    xi, yi = int(round(x)) + ox, int(round(y)) + oy
                    if 0 <= xi < w and 0 <= yi < h:
                        mask[yi, xi] = True


def inpaint(rgb, known, todo):
    """Fills `todo` pixels from the `known` ones, layer by layer (mean of the known neighbours)."""
    rgb = rgb.copy()
    known = known.copy()
    todo = todo & ~known
    while todo.any():
        acc = np.zeros(rgb.shape, dtype=np.float32)
        cnt = np.zeros(known.shape, dtype=np.float32)
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                if dx == 0 and dy == 0:
                    continue
                k = np.roll(np.roll(known, dy, 0), dx, 1)
                c = np.roll(np.roll(rgb, dy, 0), dx, 1)
                acc += c * k[..., None]
                cnt += k
        ring = todo & (cnt > 0)
        if not ring.any():
            break
        rgb[ring] = acc[ring] / cnt[ring][:, None]
        known |= ring
        todo &= ~ring
    return rgb


def bleed(rgba, n=PAD):
    """Spreads the colours of the edge pixels into the transparent ones (alpha stays 0): no dark fringe once
    the texture is filtered or mipmapped."""
    rgb = rgba[..., :3].astype(np.float32)
    known = rgba[..., 3] > 0
    for _ in range(n):
        acc = np.zeros(rgb.shape, dtype=np.float32)
        cnt = np.zeros(known.shape, dtype=np.float32)
        for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            k = np.roll(np.roll(known, dy, 0), dx, 1)
            c = np.roll(np.roll(rgb, dy, 0), dx, 1)
            acc += c * k[..., None]
            cnt += k
        ring = ~known & (cnt > 0)
        if not ring.any():
            break
        rgb[ring] = acc[ring] / cnt[ring][:, None]
        known |= ring
    out = rgba.copy()
    out[..., :3] = np.clip(rgb.round(), 0, 255).astype(np.uint8)
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("spec")
    ap.add_argument("--debug", help="folder for the cut-out parts, to check them")
    a = ap.parse_args()

    spec = json.load(open(a.spec))
    base = os.path.dirname(os.path.abspath(a.spec))
    img = np.asarray(Image.open(os.path.join(base, spec["image"])).convert("RGBA")).astype(np.float32)
    h, w, _ = img.shape
    alpha = img[..., 3] / 255.0
    opaque = alpha > 0.0
    py, px = np.mgrid[0:h, 0:w].astype(np.float32)
    J = {k: tuple(v) for k, v in spec.get("joints", {}).items()}
    R = spec.get("radius", {})

    parts = {}  # name -> (mask, soft alpha factor)
    whole = spec.get("parts") == "whole"
    if whole:
        parts["whole"] = opaque
        order = ["whole"]
    else:
        def limb(chain, rk, end_keys):
            m, e = capsule_chain(px, py, [J[k] for k in chain], R[rk])
            for ek, ck in end_keys:  # hand / foot blobs
                if ek in R:
                    if isinstance(R[ek], list):
                        m2, e2 = capsule_chain(px, py, [J[k] for k in ck], R[ek])
                    else:
                        m2, e2 = disc(px, py, J[ck[0]], R[ek])
                    m |= m2
                    e = np.maximum(e, e2)
            return m & opaque, e

        armF, _ = limb(["shF", "elbowF", "handF"], "armF", [("handF", ["handF"])])
        armB, _ = limb(["shB", "elbowB", "handB"], "armB", [("handB", ["handB"])])
        legF, _ = limb(["hipF", "kneeF", "ankleF"], "legF", [("footF", ["ankleF", "toeF"]), ("heelF", ["ankleF", "footF"])])
        legB, _ = limb(["hipB", "kneeB", "ankleB"], "legB", [("footB", ["ankleB", "toeB"]), ("heelB", ["ankleB", "footB"])])
        legF &= ~armF
        # Head: everything hanging from the skull above the neck cut.
        barrier = np.zeros((h, w), dtype=bool)
        raster_line(barrier, spec["neckCut"])
        head = flood(opaque & ~barrier & ~armF & ~armB & ~legF & ~legB, tuple(int(v) for v in J["head"]))
        head = dilate(head, 2) & opaque & ~armF  # gives back the barrier line
        legF &= ~head
        armB &= ~(armF | head | legF)
        legB &= ~(armF | head | legF | armB)
        torso, _ = capsule_chain(px, py, [J["hips"], J["neck"]], [R.get("torso", 100)] * 2)
        torso &= opaque & ~(armF | head | legF | armB | legB)
        # What no bone claimed (a holster on a thigh, the hem of wide trousers, toes) goes to the nearest part.
        names = ["torso", "legB", "armB", "legF", "head", "armF"]
        label = np.zeros((h, w), dtype=np.int8)
        for i, m in enumerate([torso, legB, armB, legF, head, armF]):
            label[m] = i + 1
        for _ in range(400):
            todo = opaque & (label == 0)
            if not todo.any():
                break
            grown = label.copy()
            for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                n = np.roll(np.roll(label, dy, 0), dx, 1)
                take = todo & (grown == 0) & (n > 0)
                grown[take] = n[take]
            if (grown == label).all():
                break
            label = grown
        torso, legB, armB, legF, head, armF = [(label == i + 1) for i in range(6)]
        front = armF | head | legF
        back = armB | legB
        core = torso
        rc = int(spec.get("close", 22))
        hull = erode(dilate(core, rc), rc)
        torso_keep = core | (hull & back)
        todo = hull & front & opaque
        rgb = inpaint(img[..., :3], torso_keep, todo)
        torso_rgba = np.dstack([rgb, np.where(torso_keep | todo, img[..., 3], 0.0)])
        torso_rgba[todo, 3] = 255.0
        parts = {"legB": legB, "armB": armB, "torso": None, "legF": legF, "head": head, "armF": armF}
        order = ["legB", "armB", "torso", "legF", "head", "armF"]

    layers = {}
    for name in order:
        if name == "torso":
            rgba = torso_rgba
        else:
            m = parts[name]
            rgba = img.copy()
            a_ = np.where(m, img[..., 3], 0.0)
            rgba[..., 3] = a_
        layers[name] = rgba

    # Pack the parts (shelves, tallest first).
    boxes = {}
    for name, rgba in layers.items():
        ys, xs = np.nonzero(rgba[..., 3] > 0.5)
        if len(xs) == 0:
            raise SystemExit(f"part {name} is empty: check the joints")
        boxes[name] = (int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1)
    aw = max(ATLAS_W if len(boxes) > 1 else 0, max(b[2] - b[0] for b in boxes.values()) + 2 * PAD)
    place, x, y, shelf = {}, PAD, PAD, 0
    for name in sorted(boxes, key=lambda n: -(boxes[n][3] - boxes[n][1])):
        bw, bh = boxes[name][2] - boxes[name][0], boxes[name][3] - boxes[name][1]
        if x + bw + PAD > aw:
            x, y, shelf = PAD, y + shelf + 2 * PAD, 0
        place[name] = (x, y)
        x += bw + 2 * PAD
        shelf = max(shelf, bh)
    ah = y + shelf + PAD
    ah = (ah + 3) // 4 * 4
    atlas = np.zeros((ah, aw, 4), dtype=np.float32)
    for name, (ax, ay) in place.items():
        x0, y0, x1, y1 = boxes[name]
        atlas[ay:ay + y1 - y0, ax:ax + x1 - x0] = layers[name][y0:y1, x0:x1]
    atlas8 = np.clip(atlas.round(), 0, 255).astype(np.uint8)
    atlas8[atlas8[..., 3] == 0, :3] = 0
    atlas8 = bleed(atlas8)

    out = spec["output"]
    root = os.path.normpath(os.path.join(base, "..", ".."))
    out_base = os.path.join(root, out)
    os.makedirs(os.path.dirname(out_base), exist_ok=True)
    Image.fromarray(atlas8, "RGBA").save(out_base + ".png", optimize=True)
    with open(out_base + ".rig", "w") as f:
        f.write(f"# Generated by scripts/rig_art.py from {os.path.relpath(a.spec, root)}\n")
        f.write(f"size {w} {h}\nground {spec['ground']}\ntop {spec['top']}\n")
        for k, v in J.items():
            f.write(f"joint {k} {v[0]:g} {v[1]:g}\n")
        for k, v in spec.get("blend", {}).items():
            f.write(f"blend {k} {v:g}\n")
        for name in order:
            x0, y0, x1, y1 = boxes[name]
            ax, ay = place[name]
            f.write(f"part {name} {ax} {ay} {x1 - x0} {y1 - y0} {x0} {y0}\n")
    print(f"{out_base}.png: atlas {aw}x{ah}, parts " + ", ".join(f"{n} {boxes[n][2] - boxes[n][0]}x{boxes[n][3] - boxes[n][1]}" for n in order))

    if a.debug:
        os.makedirs(a.debug, exist_ok=True)
        colors = {"legB": (60, 90, 255), "armB": (0, 200, 255), "torso": (255, 255, 255), "legF": (255, 60, 60),
                  "head": (255, 220, 0), "armF": (60, 255, 60), "whole": (255, 255, 255)}
        vis = np.zeros((h, w, 3), dtype=np.float32)
        for name in order:
            m = layers[name][..., 3] > 127
            vis[m] = vis[m] * 0.3 + np.array(colors[name]) * 0.7
        base_rgb = img[..., :3] * 0.5 + vis * 0.5
        Image.fromarray(np.clip(base_rgb, 0, 255).astype(np.uint8)).save(os.path.join(a.debug, "labels.png"))
        Image.fromarray(atlas8, "RGBA").save(os.path.join(a.debug, "atlas.png"))
        for name in order:
            Image.fromarray(np.clip(layers[name], 0, 255).astype(np.uint8), "RGBA").save(
                os.path.join(a.debug, f"part_{name}.png"))


if __name__ == "__main__":
    main()
