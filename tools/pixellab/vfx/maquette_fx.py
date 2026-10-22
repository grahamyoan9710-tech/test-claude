"""The owner's effect sheets (docs/refonte/vfx_maquette_*.png, in the style of Rivals of Aether) rebuilt with PixelLab's
vfx_workbench (free). Each drawing is cut out of its sheet and reduced to its pixel grid; the workbench pastes it as
the effect's key picture (an image layer), reveals it, adds its light and debris, and breaks it up.
Sheet 1 keeps the owner's colours (the same for every fighter). The grey sheets (2 to 5) are drawn in ramp shades
('1' darkest to '5'): each fighter's ramp colours them.

    python3 maquette_fx.py cut                -> OUT/cut/<name>.png, the cutouts at 4x (check them)
    python3 maquette_fx.py render [names...]  -> OUT/<name>/sheet.png
    python3 maquette_fx.py export [names...]  -> OUT/<name>.png + .json, for scripts/import_fx.py
"""
import os
import sys

import numpy as np
from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
OUT = os.environ.setdefault("MOVE_FX_OUT", os.path.join(HERE, "maquette"))
SHEET = {k: os.path.join(REPO, "docs", "refonte", f"vfx_maquette_{k}.png") for k in
         ("1_systeme", "2_competences", "3_foudre", "4_foudre_grande", "5_bulle")}

# name: (sheet, box in the sheet's pixels). Sheet 1: the system effects; 2: the skills; 3 and 4: lightning; 5: shield.
CUTS = {
    "m_quick": ("1_systeme", (104, 150, 271, 308)),     # 01 Coup rapide
    "m_slash": ("1_systeme", (377, 82, 690, 369)),      # 02 Coup tranchant
    "m_rise": ("1_systeme", (769, 57, 988, 375)),       # 03 Coup ascendant
    "m_crit": ("1_systeme", (1066, 53, 1373, 369)),     # 04 Coup critique
    "m_ground_big": ("1_systeme", (1395, 110, 1756, 380)),   # 05 Gros impact au sol
    "m_ground_heavy": ("1_systeme", (14, 495, 354, 778)),    # 06 Impact lourd au sol
    "m_jump": ("1_systeme", (378, 478, 642, 772)),      # 07 Effet de saut
    "m_dash": ("1_systeme", (648, 640, 992, 772)),      # 08 Dash au sol
    "m_ground": ("1_systeme", (1004, 590, 1372, 772)),  # 09 Impact au sol
    "m_land": ("1_systeme", (1388, 625, 1760, 772)),    # 10 Atterrissage
}
SKILLS = ["fente", "croissant", "double_entaille", "orbe", "projectile_flamme", "lance", "flammes", "pilier", "vortex",
          "onde_sol", "arc", "impact", "tir_spirale", "vagues_croisees", "deferlante", "fente_descendante", "uppercut",
          "impact_poing", "trainee", "anneau", "tourbillon", "etincelles", "rayon", "salve", "explosion_croix",
          "pics", "ballet", "onde_pression", "flammes_jumelles", "tir_concentre"]
_S2 = [(24, 67, 280, 145), (299, 18, 511, 167), (556, 23, 755, 170), (782, 46, 999, 161), (1030, 37, 1258, 167),
       (1287, 70, 1515, 150), (35, 201, 247, 375), (312, 201, 484, 375), (544, 213, 754, 375), (783, 275, 1018, 370),
       (1049, 210, 1262, 380), (1265, 210, 1505, 380), (30, 431, 285, 577), (287, 431, 511, 577), (533, 414, 760, 577),
       (828, 414, 971, 582), (1049, 414, 1249, 580), (1292, 414, 1499, 583), (30, 625, 254, 771), (285, 617, 518, 775),
       (546, 630, 752, 775), (786, 626, 1006, 774), (1030, 657, 1280, 763), (1297, 619, 1511, 770), (24, 811, 268, 975),
       (288, 811, 513, 968), (552, 816, 748, 968), (789, 819, 1002, 973), (1040, 820, 1255, 973), (1288, 824, 1513, 968)]
CUTS.update({f"s_{n}": ("2_competences", b) for n, b in zip(SKILLS, _S2)})
BOLTS = ["eclair", "eclair_ramifie", "arc_horizontal", "impact_electrique", "anneau_foudre", "frappe_sol",
         "eclairs_croises", "arc_chaine", "colonne", "eclair_rapide"]
_S3 = [(129, 12, 225, 369), (367, 9, 665, 369), (690, 156, 1055, 296), (1090, 34, 1399, 364), (1448, 51, 1755, 363),
       (25, 451, 312, 793), (379, 487, 648, 788), (681, 576, 1085, 742), (1147, 451, 1355, 798), (1436, 495, 1747, 800)]
CUTS.update({f"b_{n}": ("3_foudre", b) for n, b in zip(BOLTS, _S3)})
BIG = ["ruee", "arc_ascendant", "frappe_verticale", "spirale", "dash_eclate"]
_S4 = [(17, 300, 553, 561), (553, 150, 912, 562), (914, 25, 1180, 562), (1205, 60, 1540, 568), (1540, 60, 1990, 568)]
CUTS.update({f"g_{n}": ("4_foudre_grande", b) for n, b in zip(BIG, _S4)})
CUTS["shield"] = ("5_bulle", (140, 148, 1102, 1112))


HOLLOW = {"s_anneau", "b_anneau_foudre"}


def cutout(name):
    """The drawing on transparency, at the sheet's resolution: the white joined to the crop's border goes."""
    sheet, box = CUTS[name]
    im = Image.open(SHEET[sheet]).convert("RGB").crop(box)
    a = np.asarray(im).astype(int)
    light = (a.min(axis=2) >= 226) & (a.max(axis=2) - a.min(axis=2) < 30)
    red = (a[..., 0] > 150) & (a[..., 1] < 90) & (a[..., 2] < 90)  # the owner's pencil marks, on sheet 1 only
    mask = Image.fromarray((light * 255).astype(np.uint8)).copy()  # (an array-backed image ignores floodfill)
    w, h = mask.size
    for x in range(0, w, 3):
        for y in (0, h - 1):
            if mask.getpixel((x, y)) == 255:
                ImageDraw.floodfill(mask, (x, y), 128)
    for y in range(0, h, 3):
        for x in (0, w - 1):
            if mask.getpixel((x, y)) == 255:
                ImageDraw.floodfill(mask, (x, y), 128)
    if name in HOLLOW:  # a ring: the white inside it is the sheet showing through
        ImageDraw.floodfill(mask, (w // 2, h // 2), 128)
    bg = np.asarray(mask) == 128
    if sheet == "1_systeme" and name != "m_crit":
        bg |= red
    rgba = np.dstack([a, np.where(bg, 0, 255)]).astype(np.uint8)
    return Image.fromarray(rgba, "RGBA")


# Sheet pixels per drawn pixel (the sheets are drawn in big pixels).
GRID = {"1_systeme": 4.7, "2_competences": 2.8, "3_foudre": 4.0, "4_foudre_grande": 2.8, "5_bulle": 13.0}


def grid(name):
    """The cutout reduced to its pixel grid."""
    im = cutout(name)
    im = im.crop(im.getbbox())
    k = 1.0 / GRID[CUTS[name][0]]
    w, h = max(1, round(im.width * k)), max(1, round(im.height * k))
    a = np.asarray(im).astype(np.float32)
    pre = a.copy()
    pre[..., :3] *= a[..., 3:4] / 255.0  # premultiplied: the white around does not bleed in
    small = np.asarray(Image.fromarray(pre.astype(np.uint8), "RGBA").resize((w, h), Image.BOX)).astype(np.float32)
    alpha = small[..., 3]
    rgb = np.where(alpha[..., None] > 0, small[..., :3] * 255.0 / np.maximum(alpha[..., None], 1), 0)
    keep = alpha >= 96
    return np.dstack([np.clip(rgb, 0, 255), np.where(keep, 255, 0)]).astype(np.uint8)


LETTERS = "abcdefghijklmnopqrstuvwxyz"


def palette(g, n=16):
    """The drawing's own colours (sheet 1): at most n, as {letter: #rrggbb}, and its pixels as letters."""
    rgb = Image.fromarray(g[..., :3], "RGB").quantize(n, method=Image.Quantize.MEDIANCUT)
    idx = np.asarray(rgb)
    pal = rgb.getpalette()[:3 * n]
    used = sorted(set(idx[g[..., 3] > 0].tolist()))
    colours = {LETTERS[i]: "#%02x%02x%02x" % tuple(pal[3 * u:3 * u + 3]) for i, u in enumerate(used)}
    letter = {u: LETTERS[i] for i, u in enumerate(used)}
    chars = np.full(idx.shape, ".", dtype="<U1")
    for u, c in letter.items():
        chars[(idx == u) & (g[..., 3] > 0)] = c
    return chars, colours


def shades(g, n=5):
    """A grey drawing as ramp shades '1' (darkest) to str(n) (lightest)."""
    lum = g[..., :3].astype(np.float32) @ np.array([0.3, 0.59, 0.11], np.float32)
    lo, hi = np.percentile(lum[g[..., 3] > 0], [2, 98])
    lvl = np.clip(((lum - lo) / max(1.0, hi - lo) * n).astype(int) + 1, 1, n)
    chars = np.where(g[..., 3] > 0, lvl.astype(str), ".")
    return chars, None


def reveal(chars, masks):
    """One frame per mask (True = shown): the drawing's rows."""
    return [["".join(r) for r in np.where(m, chars, ".")] for m in masks]


def masks_of(shape, how, steps, origin=(0.5, 0.5), angle=0.0):
    """Masks growing over `steps` frames: 'pop' from the origin outward, 'oval' the same but wide and flat (smoke on
    the ground), 'sweep' along `angle` degrees, 'rise' from the bottom up."""
    h, w = shape
    y, x = np.mgrid[0:h, 0:w].astype(np.float32)
    ox, oy = origin[0] * (w - 1), origin[1] * (h - 1)
    if how == "pop":
        d = np.hypot(x - ox, y - oy)
    elif how == "oval":
        d = np.hypot((x - ox) / w, (y - oy) / (0.6 * h))
    elif how == "sweep":
        a = np.radians(angle)
        d = (x - ox) * np.cos(a) + (y - oy) * np.sin(a)
        d -= d.min()
    elif how == "rise":
        d = (h - 1) - y
    else:
        raise ValueError(how)
    top = d.max() + 1e-3
    return [d <= top * (k + 1) / steps for k in range(steps)]


def erosion(shown, steps, seed=1):
    """Masks breaking the drawing up over `steps` frames, from its edges in, in ragged bits (the last one is empty)."""
    depth = np.zeros(shown.shape, np.float32)
    cur = shown.copy()
    k = 0
    while cur.any():  # chessboard distance to the outside, one ring at a time
        k += 1
        depth[cur] = k
        pad = np.pad(cur, 1)
        cur = cur & pad[:-2, 1:-1] & pad[2:, 1:-1] & pad[1:-1, :-2] & pad[1:-1, 2:]
    rng = np.random.default_rng(seed)
    noise = rng.random(shown.shape).astype(np.float32) * 2.5
    level = depth + noise
    top = level[shown].max() + 1e-3 if shown.any() else 1.0
    return [shown & (level > top * (n + 1) / steps) for n in range(steps)]


def tiles(chars_frames, cx, cy, colours, base):
    """Image layers (64 x 64 at most each) pasting the frames with their top-left corner at (cx, cy)."""
    h, w = len(chars_frames[0]), len(chars_frames[0][0])
    nx, ny = -(-w // 64), -(-h // 64)
    xs = [round(i * w / nx) for i in range(nx + 1)]
    ys = [round(j * h / ny) for j in range(ny + 1)]
    out = []
    for j in range(ny):
        for i in range(nx):
            fr = [[row[xs[i]:xs[i + 1]] for row in f[ys[j]:ys[j + 1]]] for f in chars_frames]
            tw, th = xs[i + 1] - xs[i], ys[j + 1] - ys[j]
            layer = dict(base, id=f"{base['id']}{j}{i}", type="image", pixels=fr, facing="none")
            layer["x"] = cx + xs[i] + tw / 2.0
            y0 = cy + ys[j] + th / 2.0
            layer["y"] = {"from": y0, "to": y0 - base["drift"], "ease": "out"} if base.get("drift") else y0
            layer.pop("drift", None)
            if colours:
                layer["colours"] = colours
            out.append(layer)
    return out


def drawing(name, how, reveal_n, hold_n, out_n, origin=(0.5, 0.5), angle=0.0, margin=12, extra=(), ramps=None,
            fps=15, drift=0, seed=1):
    """A one-shot effect from a drawing: revealed by `how` over `reveal_n` frames, held whole `hold_n` frames, then
    broken up over `out_n` frames (16 in all at most). `origin` is the effect's origin in the drawing (0..1): its
    centre, or its foot for a ground effect; the `extra` layers are placed relative to it."""
    g = grid(name)
    chars, colours = palette(g) if CUTS[name][0] == "1_systeme" else shades(g)
    h, w = chars.shape
    shown = chars != "."
    ms = masks_of((h, w), how, reveal_n, origin, angle) + [shown] * hold_n + erosion(shown, out_n, seed)
    W, H = w + 2 * margin, h + 2 * margin
    ox, oy = round(margin + origin[0] * (w - 1)), round(margin + origin[1] * (h - 1))
    base = {"id": "img", "fps": fps, "drift": drift}
    layers = tiles(reveal(chars, ms), margin, margin, colours, base)
    import move_fx
    for l in extra:
        l = dict(l)
        l["x"] = move_fx.shift(l.get("x", 0), ox)
        l["y"] = move_fx.shift(l.get("y", 0), oy)
        if l.get("type") == "particles" and l.get("shape") == "chunk" and "floor" not in l:
            l["floor"] = oy  # debris lands on the ground line (the origin of a ground effect)
        layers.append(l)
    return {"canvas": {"w": W, "h": H}, "origin": [ox, oy], "frames": len(ms), "fps": fps, "loop": False,
            "ramps": {"main": ["#202028", "#5a5a6a", "#9a9aaa", "#d8d8e4", "#ffffff"], **(ramps or {})},
            "layers": layers, "outline": {"mode": "none"}}


def glow(r, end=0.2, ramp="flash"):
    """A flash of light on the origin at the start (the impact)."""
    return {"id": "flash", "type": "glow", "radius": {"from": r, "to": r * 0.4, "ease": "out"}, "life": [0.0, end],
            "ramp": ramp, "shape": "hard", "outline": False}


def sparks(count, speed, ramp, size=(2, 4), shape="sparkle", direction=None, spread=360, gravity=0, end=0.7):
    l = {"id": f"sparks_{ramp}", "type": "particles", "mode": "burst", "count": count, "speed": speed, "spread": spread,
         "size": list(size), "shape": shape, "lifespan": 0.6, "life": [0.0, end], "fade_out": 0.3, "ramp": ramp,
         "gravity": gravity}
    if direction is not None:
        l["direction"] = direction
    return l


def rocks(count, speed, spread=120):
    return {"id": "rocks", "type": "particles", "mode": "burst", "direction": -90, "spread": spread, "speed": speed,
            "count": count, "size": [2, 4], "shape": "chunk", "gravity": 140, "lifespan": 0.9, "life": [0.05, 1.0],
            "ramp": "rock", "outline": True}


# Colours of sheet 1, for the workbench's own sparks and debris
R1 = {"flash": ["#fff4d0", "#ffffff"], "gold": ["#c86410", "#f0a020", "#ffd860", "#fff8d0"],
      "blue": ["#0a5ac8", "#28a0f0", "#8ee0ff", "#ffffff"], "red": ["#a00010", "#e82828", "#ff9a8a", "#ffffff"],
      "rock": ["#2c2c36", "#4e5060", "#7a7e8e", "#a8acba"], "white": ["#9aa0b0", "#d8dce6", "#ffffff"]}
FX = {}
FX["m_quick"] = drawing("m_quick", "pop", 2, 3, 4, extra=[glow(8), sparks(6, 40, "gold")], ramps=R1)
FX["m_slash"] = drawing("m_slash", "sweep", 3, 2, 4, origin=(0.45, 0.5), angle=-20, extra=[sparks(5, 50, "blue", direction=0, spread=90)], ramps=R1)
FX["m_rise"] = drawing("m_rise", "rise", 3, 2, 4, origin=(0.5, 0.75), extra=[sparks(6, 70, "gold", (2, 3), "streak", -90, 50)], ramps=R1)
FX["m_crit"] = drawing("m_crit", "pop", 2, 3, 4, extra=[glow(14), sparks(8, 60, "red")], ramps=R1)
FX["m_ground_big"] = drawing("m_ground_big", "pop", 4, 2, 5, origin=(0.5, 1.0), extra=[rocks(6, 80)], ramps=R1)
FX["m_ground_heavy"] = drawing("m_ground_heavy", "pop", 4, 2, 5, origin=(0.5, 1.0), extra=[rocks(7, 90)], ramps=R1)
FX["m_jump"] = drawing("m_jump", "rise", 3, 2, 4, origin=(0.5, 1.0), extra=[sparks(5, 80, "blue", (2, 3), "streak", -90, 30)], ramps=R1)
FX["m_dash"] = drawing("m_dash", "oval", 4, 3, 5, origin=(0.22, 1.0), drift=3, ramps=R1)
FX["m_ground"] = drawing("m_ground", "oval", 4, 2, 5, origin=(0.5, 0.75), extra=[rocks(5, 60, 140)], ramps=R1)
FX["m_land"] = drawing("m_land", "oval", 4, 3, 5, origin=(0.5, 1.0), drift=3, ramps=R1)


def main():
    cmd = sys.argv[1] if len(sys.argv) > 1 else "cut"
    names = sys.argv[2:] or list(CUTS)
    if cmd == "cut":
        os.makedirs(os.path.join(OUT, "cut"), exist_ok=True)
        for n in names:
            g = grid(n)
            Image.fromarray(g, "RGBA").resize((g.shape[1] * 4, g.shape[0] * 4), Image.NEAREST).save(
                os.path.join(OUT, "cut", f"{n}.png"))
            print(n, f"{g.shape[1]}x{g.shape[0]}")
    else:
        import move_fx
        move_fx.main(FX)


if __name__ == "__main__":
    main()
