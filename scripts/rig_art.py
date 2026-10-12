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
      "skirt": [[[x, y], ...]],           # polygon(s): cloth from the waist down (hakama, robe, coat tails, kilt)
      "cape": [[[x, y], ...]],            # polygon(s): cloth hanging behind the body
      "order": ["cape", "legB", ...],     # drawing order of the parts (back to front), see below
      "neckCut": [[x, y], ...],           # line between the head and the collar
      "cutToEdges": true,                 # extend that line to both sides of the picture
      "close": 22                         # how far the torso is filled in under the limbs (px)
    }

F is the arm / leg on the side the fighter faces (to the right of the picture), drawn in front of the body;
B is the other one, drawn behind it. A picture without limb joints ("parts": "whole") is only a portrait.

The parts, back to front by default: cape, legB, torso, legF, skirt, armB, head, armF. Each limb takes the
pixels around its bones (tapered capsules); the head takes what hangs from the skull above the neck cut; the
skirt and the cape take their polygons; the torso takes the rest. Under the parts drawn in front of them, the
torso, the skirt and the cape are filled in from their own colours, so a limb that moves away leaves no hole.
In the game the skirt bends between both legs and the hips, the cape follows the torso and trails behind.

The game redraws the arms and the legs itself (volumes lit for each pose): for them the .rig carries the widths
and colours measured along each limb ("limb"), the fists and the shoes ("fist", "shoe", and their pieces of the
picture, "piece"), and each limb segment straightened into a strip of the atlas with the picture's shading taken
out ("strip"). The limb parts stay in the atlas for the whole-picture portraits.
"""
import argparse
import json
import os

import numpy as np
from PIL import Image, ImageDraw

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


def bilinear(img, xs, ys):
    h, w = img.shape[:2]
    x0 = np.floor(xs).astype(int)
    y0 = np.floor(ys).astype(int)
    fx = (xs - x0)[..., None]
    fy = (ys - y0)[..., None]
    xa, xb = np.clip(x0, 0, w - 1), np.clip(x0 + 1, 0, w - 1)
    ya, yb = np.clip(y0, 0, h - 1), np.clip(y0 + 1, 0, h - 1)
    return (img[ya, xa] * (1 - fx) * (1 - fy) + img[ya, xb] * fx * (1 - fy) + img[yb, xa] * (1 - fx) * fy +
            img[yb, xb] * fx * fy)


def limb_strips(img, own, J, profiles, opaque):
    """Each limb segment straightened into a strip (the bone along x, `ext` px past each joint for the round caps),
    with the picture's shading across the limb taken out: the game lights the limb itself, as a volume."""
    h, w = opaque.shape
    strips = {}
    chains = {"armF": ["shF", "elbowF", "handF"], "armB": ["shB", "elbowB", "handB"],
              "legF": ["hipF", "kneeF", "ankleF"], "legB": ["hipB", "kneeB", "ankleB"]}
    lum_w = np.array([0.3, 0.59, 0.11])
    for name, keys in chains.items():
        rows = profiles[name]
        mask = own[name].astype(np.float32)
        for k in range(2):
            a, b = np.array(J[keys[k]], np.float32), np.array(J[keys[k + 1]], np.float32)
            L = float(np.hypot(*(b - a)))
            d = (b - a) / max(L, 1e-3)
            n = np.array([-d[1], d[0]], np.float32)
            hws = [r[1] for r in rows if k <= r[0] < k + 1] or [20.0]
            half = int(np.ceil(max(hws) * 1.12)) + 2
            ext = half
            Wd, Hd = int(np.ceil(L)) + 2 * ext, 2 * half
            xs_, ys_ = np.meshgrid(np.arange(Wd, dtype=np.float32) + 0.5 - ext, np.arange(Hd, dtype=np.float32) + 0.5 - half)
            px = a[0] + d[0] * xs_ + n[0] * ys_
            py = a[1] + d[1] * xs_ + n[1] * ys_
            rgb = bilinear(img[..., :3], px, py)
            m = bilinear(mask[..., None], px, py)[..., 0] > 0.5
            # Inside the measured silhouette: what the picture hides (behind the body) is filled from the limb.
            prof_t = np.array([r[0] - k for r in rows if k <= r[0] < k + 1] or [0.5], np.float32) * L
            prof_w = np.array(hws, np.float32)
            hw_at = np.interp(xs_[0], prof_t, prof_w)
            inside = np.abs(ys_) <= hw_at[None, :] * 1.06 + 1.0
            # The side of the limb towards the body (the torso, or the other leg) picks up the body's colours in the
            # picture: it is rebuilt from the rest of the limb.
            centre = (np.array(J["hips"], np.float32) + np.array(J["neck"], np.float32)) * 0.5
            inner = 1.0 if float(np.dot(centre - (a + b) * 0.5, n)) > 0.0 else -1.0
            u_in = ys_ * inner / np.maximum(hw_at[None, :], 1.0)
            m = m & (u_in <= 0.5)
            if m.sum() >= 12:
                rgb = inpaint(rgb, m, inside & ~m)
            # Shading across the limb (u = -1 .. 1), relative to each row's mean: the light baked in the picture.
            lum = rgb @ lum_w
            u = ys_ / np.maximum(hw_at[None, :], 1.0)
            valid = m & (np.abs(u) < 0.95)
            rowmean = np.array([lum[:, i][valid[:, i]].mean() if valid[:, i].sum() > 2 else np.nan for i in range(Wd)])
            rowmean = np.where(np.isnan(rowmean), np.nanmean(rowmean) if np.isfinite(rowmean).any() else 128.0, rowmean)
            rel = lum / np.maximum(rowmean[None, :], 1.0)
            bins = np.linspace(-1.0, 1.0, 13)
            prof = np.ones(12, np.float32)
            for i in range(12):
                sel = valid & (u >= bins[i]) & (u < bins[i + 1])
                if sel.sum() > 6:
                    prof[i] = float(np.median(rel[sel]))
            prof = np.convolve(np.pad(prof, 1, mode="edge"), [0.25, 0.5, 0.25], mode="valid")
            top = max(1e-3, float(np.percentile(prof, 90)))
            mult = np.clip(prof / top, 0.45, 1.0)
            centers = 0.5 * (bins[:-1] + bins[1:])
            mul = np.interp(np.clip(u, -1, 1), centers, mult)
            lit = np.clip(rgb / mul[..., None], 0, 255)
            alpha = np.where(inside | m, 255.0, 0.0)
            strips[(name, k)] = (np.dstack([lit, alpha]), ext, half, L)
    return strips


def end_pieces(img, own, J, R, profiles, opaque):
    """The fists and the shoes, as they are drawn: rigid pieces at the ends of the limbs."""
    h, w = opaque.shape
    py, px = np.mgrid[0:h, 0:w].astype(np.float32)
    pieces = {}
    for f in ("F", "B"):
        hand = np.array(J["hand" + f], np.float32)
        rad = profiles["hand" + f][0] * 1.35 + 4
        m = (np.hypot(px - hand[0], py - hand[1]) <= rad) & own["arm" + f]
        rgba = img.copy()
        rgba[..., 3] = np.where(m, img[..., 3], 0.0)
        pieces["fist" + f] = (rgba, hand)
        ankle, foot, toe = (np.array(J[k + f], np.float32) for k in ("ankle", "foot", "toe"))
        reach = abs(toe[0] - foot[0]) * 1.6 + 30
        m = own["leg" + f] & (py >= ankle[1] - 0.18 * (foot[1] - ankle[1])) & (np.abs(px - foot[0]) <= reach)
        rgba = img.copy()
        rgba[..., 3] = np.where(m, img[..., 3], 0.0)
        pieces["shoe" + f] = (rgba, ankle)
    return pieces


def body_shape(J, R, w, h):
    """The torso without its limbs: from the shoulders down to the hips, as wide as the joints."""
    shF, shB, hipF, hipB = (np.array(J[k], dtype=np.float32) for k in ("shF", "shB", "hipF", "hipB"))
    neck = np.array(J["neck"], dtype=np.float32)
    aF, aB = R["armF"][0] * 0.35, R["armB"][0] * 0.35
    lF, lB = R["legF"][0] * 0.55, R["legB"][0] * 0.55
    right = 1.0 if shF[0] >= shB[0] else -1.0  # the front side
    top = min(shF[1], shB[1]) - 0.6 * abs(shF[0] - shB[0])
    poly = [(shB[0] - right * aB, top), (shF[0] + right * aF, top), (shF[0] + right * aF, shF[1] + aF),
            (hipF[0] + right * lF, hipF[1] + lF), (hipB[0] - right * lB, hipB[1] + lB),
            (shB[0] - right * aB, shB[1] + aB)]
    im = Image.new("L", (w, h), 0)
    ImageDraw.Draw(im).polygon([tuple(map(float, p)) for p in poly], fill=255)
    m = np.asarray(im) > 0
    m[: int(max(0, neck[1] - 4))] |= False
    return m


def ramp(img, m):
    """Shadow, middle and light tones of the dominant colour of mask m, as the picture paints them."""
    pix = img[m][:, :3]
    if len(pix) == 0:
        g = np.array([128.0, 128.0, 128.0])
        return np.stack([g * 0.6, g, g * 1.2])
    lum = pix @ np.array([0.3, 0.59, 0.11])
    body = pix[lum > 28] if (lum > 28).sum() >= max(3, len(pix) // 4) else pix
    mid = np.median(body, axis=0)
    chroma = lambda c: c - c.mean(axis=-1, keepdims=True)
    same = body[np.linalg.norm(chroma(body) - chroma(mid), axis=-1) < 40]
    if len(same) < 4:
        same = body
    ls = same @ np.array([0.3, 0.59, 0.11])
    order = np.argsort(ls)
    dark = same[order[int(len(order) * 0.15)]]
    light = same[order[min(len(order) - 1, int(len(order) * 0.88))]]
    return np.stack([dark, mid, light])


def median_color(img, m):
    """Median colour of the pixels of mask m, the dark ink of the outlines left out when there is more."""
    pix = img[m][:, :3]
    if len(pix) == 0:
        return None
    lum = pix @ np.array([0.3, 0.59, 0.11])
    keep = pix[lum > 28] if (lum > 28).sum() >= max(3, len(pix) // 4) else pix
    return np.median(keep, axis=0)


def measure_limbs(img, own, J, R, opaque, samples=8):
    """Width and colour along each limb, the fists and the shoes: what the game needs to draw the limbs itself.

    Widths are read on the outer edge of the limb (towards the background); where a limb lies against the body the
    picture does not show its inner edge, so the outer one is mirrored."""
    h, w = opaque.shape
    out = {}
    body = own["torso"] | own["head"] | own["skirt"] | own["cape"]
    centre = np.array(J["hips"], dtype=np.float32) * 0.5 + np.array(J["neck"], dtype=np.float32) * 0.5

    def scan(p, n, mask, rmax):
        # Distance from p along n to the first pixel off `mask`; True when that pixel is background.
        for r in range(1, int(rmax)):
            x, y = int(round(p[0] + n[0] * r)), int(round(p[1] + n[1] * r))
            if x < 0 or y < 0 or x >= w or y >= h:
                return r, True
            if not mask[y, x]:
                return r, not opaque[y, x]
        return rmax, False

    def chain(name, keys, radii, side):
        mask = own[name]
        pts = [np.array(J[k], dtype=np.float32) for k in keys]
        rows = []
        for k in range(len(pts) - 1):
            a, b = pts[k], pts[k + 1]
            d = b - a
            L = float(np.hypot(*d))
            if L < 1e-3:
                continue
            d /= L
            n = np.array([-d[1], d[0]])
            ra, rb = radii[k], radii[min(k + 1, len(radii) - 1)]
            for i in range(samples):
                t = (i + 0.5) / samples
                p = a + d * (t * L)
                rcap = ra + (rb - ra) * t
                w1, bg1 = scan(p, n, mask, rcap * 2.2)
                w2, bg2 = scan(p, -n, mask, rcap * 2.2)
                if bg1 and not bg2:
                    hw = w1
                elif bg2 and not bg1:
                    hw = w2
                elif bg1 and bg2:
                    hw = 0.5 * (w1 + w2)
                else:  # lying over the body: its edges are not visible
                    hw = rcap * 0.8
                hw = float(np.clip(hw, rcap * 0.45, rcap * 1.6))
                # Colour across the middle of the limb, a few pixels along it.
                yy, xx = np.mgrid[-5:6, -int(hw * 0.85):int(hw * 0.85) + 1]
                qx = np.clip(np.round(p[0] + d[0] * yy + n[0] * xx).astype(int), 0, w - 1)
                qy = np.clip(np.round(p[1] + d[1] * yy + n[1] * xx).astype(int), 0, h - 1)
                sel = np.zeros((h, w), dtype=bool)
                sel[qy, qx] = True
                pick = sel & mask
                if pick.sum() < 6:  # hidden (under a skirt): the cloth over it
                    pick = sel & opaque
                rows.append((k + t, hw, ramp(img, pick)))
        out[name] = rows

    chain("armF", ["shF", "elbowF", "handF"], R["armF"], 1)
    chain("armB", ["shB", "elbowB", "handB"], R["armB"], -1)
    chain("legF", ["hipF", "kneeF", "ankleF"], R["legF"], 1)
    chain("legB", ["hipB", "kneeB", "ankleB"], R["legB"], -1)
    py, px = np.mgrid[0:h, 0:w].astype(np.float32)
    for f in ("F", "B"):
        hand = np.array(J["hand" + f], dtype=np.float32)
        rad = R.get("hand" + f, 28)
        near = (np.hypot(px - hand[0], py - hand[1]) <= rad) & own["arm" + f]
        core = (np.hypot(px - hand[0], py - hand[1]) <= rad * 0.55) & own["arm" + f]
        area = max(1.0, float(near.sum()))
        out["hand" + f] = (float(np.sqrt(area / np.pi)), ramp(img, core if core.sum() >= 6 else near))
        ankle, foot, toe = (np.array(J[k + f], dtype=np.float32) for k in ("ankle", "foot", "toe"))
        box = own["leg" + f] & (py >= ankle[1] - 4) & (np.abs(px - foot[0]) <= abs(toe[0] - foot[0]) * 1.6 + 30)
        ys, xs = np.nonzero(box)
        if len(xs):
            fwd = 1.0 if toe[0] >= foot[0] else -1.0
            toe_x = float(xs.max() if fwd > 0 else xs.min())
            heel_x = float(xs.min() if fwd > 0 else xs.max())
            low = box & (py >= foot[1] - 0.4 * (foot[1] - ankle[1]))  # the shoe itself, under the leg wraps
            out["foot" + f] = (heel_x, toe_x, float(min(ys.max(), foot[1] + 3)), float(ankle[1]),
                               ramp(img, low if low.sum() >= 6 else box))
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

    parts = {}  # name -> mask
    filled = {}
    profiles = None
    strips, pieces = {}, {}
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

        def polygon(key):
            m = np.zeros((h, w), dtype=bool)
            for poly in spec.get(key, []):
                im = Image.new("L", (w, h), 0)
                ImageDraw.Draw(im).polygon([tuple(p) for p in poly], fill=255)
                m |= np.asarray(im) > 0
            return m & opaque

        armF, _ = limb(["shF", "elbowF", "handF"], "armF", [("handF", ["handF"])])
        armB, _ = limb(["shB", "elbowB", "handB"], "armB", [("handB", ["handB"])])
        armB &= ~armF
        legF, _ = limb(["hipF", "kneeF", "ankleF"], "legF", [("footF", ["ankleF", "toeF"]), ("heelF", ["ankleF", "footF"])])
        legB, _ = limb(["hipB", "kneeB", "ankleB"], "legB", [("footB", ["ankleB", "toeB"]), ("heelB", ["ankleB", "footB"])])
        # Head: everything hanging from the skull above the neck cut.
        barrier = np.zeros((h, w), dtype=bool)
        cut = [list(p) for p in spec["neckCut"]]
        if spec.get("cutToEdges"):  # the cut runs across the whole picture (hair or a collar on the shoulders)
            cut = [[0, cut[0][1]]] + cut + [[w - 1, cut[-1][1]]]
        raster_line(barrier, cut)
        head = flood(opaque & ~barrier & ~armF & ~armB & ~legF & ~legB, tuple(int(v) for v in J["head"]))
        head = dilate(head, 2) & opaque & ~armF & ~armB  # gives back the barrier line
        # Cloth: a skirt (hakama, robe, coat tails, kilt, tunic) bends between the legs; a cape hangs behind.
        skirtZone = polygon("skirt")
        capeZone = polygon("cape")
        skirt = skirtZone & ~(armF | armB | head)
        legF &= ~(armF | armB | head | skirt)
        legB &= ~(armF | armB | head | skirt | legF)
        cape = capeZone & ~(armF | armB | head | skirt | legF | legB)
        torso, _ = capsule_chain(px, py, [J["hips"], J["neck"]], [R.get("torso", 100)] * 2)
        body = body_shape(J, R, w, h)
        torso &= opaque & body & ~(armF | armB | head | skirt | legF | legB | cape)
        # What no bone claimed (a holster on a thigh, the hem of wide trousers, toes) goes to the nearest part.
        names = ["torso", "legB", "armB", "legF", "head", "armF", "skirt", "cape"]
        masks = [torso, legB, armB, legF, head, armF, skirt, cape]
        label = np.zeros((h, w), dtype=np.int8)
        for i, m in enumerate(masks):
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
        own = {name: label == i + 1 for i, name in enumerate(names)}
        # The game draws the limbs itself: what grew into the torso outside the body and lies along a limb is that
        # limb's (left out of the torso), the rest (a coat flaring past the hips) stays with the torso.
        stray = own["torso"] & ~body
        if stray.any():
            pyy, pxx = np.mgrid[0:h, 0:w].astype(np.float32)
            best = np.full((h, w), np.inf, np.float32)
            who = np.full((h, w), "", dtype=object)
            for nm, keys in (("armF", ["shF", "elbowF", "handF"]), ("armB", ["shB", "elbowB", "handB"]),
                             ("legF", ["hipF", "kneeF", "ankleF"]), ("legB", ["hipB", "kneeB", "ankleB"])):
                _, edge = capsule_chain(pxx, pyy, [J[k] for k in keys], [x * 1.7 for x in R[nm]])
                d = -edge  # < 0 inside the widened capsule
                closer = d < best
                best = np.where(closer, d, best)
                who = np.where(closer, nm, who)
            near = stray & (best <= 0.0)
            for nm in ("armF", "armB", "legF", "legB"):
                give = near & (who == nm)
                own[nm] |= give
                own["torso"] &= ~give
        order = [n for n in spec.get("order", ["cape", "legB", "torso", "legF", "skirt", "armB", "head", "armF"])
                 if own[n].any()]
        # A torso, a skirt or a cape is filled in from its own colours under the parts drawn over it (a limb that
        # moves away leaves no hole); under the parts drawn before it, it keeps the picture as it is.
        rc = int(spec.get("close", 22))
        zones = {"torso": erode(dilate(own["torso"], rc), rc), "skirt": skirtZone | own["skirt"],
                 "cape": capeZone | own["cape"]}
        filled = {}
        for name in ("torso", "skirt", "cape"):
            if name not in order:
                continue
            k = order.index(name)
            before = np.zeros((h, w), dtype=bool)
            after = np.zeros((h, w), dtype=bool)
            for other in order:
                if other != name:
                    if order.index(other) < k:
                        before |= own[other]
                    else:
                        after |= own[other]
            limbs = own["armF"] | own["armB"] | own["legF"] | own["legB"]
            keep = own[name] | (zones[name] & before & ~limbs)
            todo = zones[name] & (after | limbs) & opaque & ~keep
            rgb = inpaint(img[..., :3], keep, todo)
            rgba = np.dstack([rgb, np.where(keep | todo, img[..., 3], 0.0)])
            rgba[todo, 3] = 255.0
            filled[name] = rgba
        parts = {n: own[n] for n in names}
        profiles = measure_limbs(img, own, J, R, opaque)
        strips = limb_strips(img, own, J, profiles, opaque)
        pieces = end_pieces(img, own, J, R, profiles, opaque)

    layers = {}
    for name in order:
        if name in filled:
            rgba = filled[name]
        else:
            m = parts[name]
            rgba = img.copy()
            a_ = np.where(m, img[..., 3], 0.0)
            rgba[..., 3] = a_
        layers[name] = rgba

    # The strips and the end pieces go into the atlas too (not drawn as picture parts).
    extra = {}
    for (name, k), (rgba, ext, half, L) in strips.items():
        layers[f"strip_{name}_{k}"] = rgba
        extra[f"strip_{name}_{k}"] = ("strip", name, k, ext, half, L)
    for key, (rgba, anchor) in pieces.items():
        layers[key] = rgba
        extra[key] = ("piece", key, anchor)

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
        for key, info in extra.items():
            x0, y0, x1, y1 = boxes[key]
            ax, ay = place[key]
            if info[0] == "strip":  # the strip's own frame: x along the bone from -ext, y across from -half
                _, name, k, ext, half, L = info
                f.write(f"strip {name} {k} {ax - x0} {ay - y0} {ext} {half} {L:.1f}\n")
            else:  # a piece in picture coordinates: atlas offset, box, anchor joint
                _, pk, anchor = info
                f.write(f"piece {pk} {ax} {ay} {x1 - x0} {y1 - y0} {x0} {y0} {anchor[0]:.1f} {anchor[1]:.1f}\n")
        for name, rows in (profiles or {}).items():
            if name.startswith(("arm", "leg")):
                for t, hw, c in rows:
                    tones = " ".join(f"{int(v)}" for v in np.clip(c, 0, 255).reshape(-1))
                    f.write(f"limb {name} {t:.3f} {hw:.1f} {tones}\n")
            elif name.startswith("hand"):
                rad, c = rows
                tones = " ".join(f"{int(v)}" for v in np.clip(c, 0, 255).reshape(-1))
                f.write(f"fist {name[-1]} {rad:.1f} {tones}\n")
            elif name.startswith("foot"):
                heel, toe, sole, top, c = rows
                tones = " ".join(f"{int(v)}" for v in np.clip(c, 0, 255).reshape(-1))
                f.write(f"shoe {name[-1]} {heel:.0f} {toe:.0f} {sole:.0f} {top:.0f} {tones}\n")
    print(f"{out_base}.png: atlas {aw}x{ah}, parts " + ", ".join(f"{n} {boxes[n][2] - boxes[n][0]}x{boxes[n][3] - boxes[n][1]}" for n in order))

    if a.debug:
        os.makedirs(a.debug, exist_ok=True)
        colors = {"legB": (60, 90, 255), "armB": (0, 200, 255), "torso": (255, 255, 255), "legF": (255, 60, 60),
                  "head": (255, 220, 0), "armF": (60, 255, 60), "whole": (255, 255, 255), "skirt": (255, 120, 220),
                  "cape": (150, 90, 40)}
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
