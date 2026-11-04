"""Imports every finished clip of a PixelLab fighter into the game: python3 import_all.py FIGHTER [CLIP...]

The anchor and scale come from the fighter's base (all its clips share the canvas of that base). The impact frame
of a blow is the frame that reaches furthest in the blow's direction. The ledge clip is anchored on its hands
(the ledge top is 0.75 of the fighter's height above its feet).
"""
import json
import os
import subprocess
import sys

import numpy as np
from PIL import Image

S = os.path.dirname(os.path.abspath(__file__))
REPO = "/home/user/test-claude"
sys.path.insert(0, S)
from clips import clips  # noqa: E402

ANCHOR = {"naruto": (104, 244, 213), "sasuke": (124, 244, 213), "kakashi": (128, 244, 213)}
if os.path.exists(os.path.join(S, "anchors14.json")):  # the 14 others: feet of their guard picture, its scale
    ANCHOR.update({k: tuple(v) for k, v in json.load(open(os.path.join(S, "anchors14.json"))).items()})
# Clips whose generated frames go past what the move shows: (fighter, clip) -> extra import_clip.py arguments.
EXTRA = {("sasuke", "uspecial"): ["--only", "1-8"]}  # flung up: what follows (landing crouched) happens in the air
# Clips with several blows: the frame of each, in the order of the move's hitbox windows (reviewed by eye).
IMPACTS = {("kakashi", "w0_nspecial"): "5", ("kakashi", "w0_ultimate"): "16",  # the eye techniques: the held stare
           ("sasuke", "w1_nspecial"): "5", ("sasuke", "w1_ultimate"): "12",
           ("naruto", "nair"): "3", ("kakashi", "w1_ultimate"): "12"}  # the spin with its chakra rings at their biggest
# Clips generated from their start picture moved down in the canvas (room for a big swing): px lower feet.
SHIFT = {("p03", "w0_nair"): 40, ("p08", "w0_nair"): 48, ("p08", "w1_nair"): 46}
HELPERS = {"runstart", "ledgegrab", "holdstart", "drawsword", "airsword", "w0guard", "w1guard"}  # bases only, not clips of the game
HELPERS |= {f"w{s}_{h}" for s in (0, 1) for h in ("runstart", "ledgegrab", "holdstart")}  # ... of a weapon's look


QA_FIXES = json.load(open(os.path.join(S, "qa_fixes.json"))) if os.path.exists(os.path.join(S, "qa_fixes.json")) else {}


def with_replaced(d, replace, extra):
    """A copy of the clip folder where sheet frame k (as import_clip.py numbers them) is replaced by sheet frame
    replace[k]; the copies are taken from the original frames."""
    import shutil
    names = frames_of(d)
    idx = [int(n[2:-4]) for n in names]
    if extra[:1] == ["--only"]:
        lo, hi = (int(v) for v in extra[1].split("-"))
        idx = [i for i in idx if lo <= i <= hi]
    else:
        idx = [i for i in idx if i > 0] or idx
    out = os.path.join(S, "_fixed", os.path.basename(os.path.dirname(d)) + "_" + os.path.basename(d))
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    for n in names:
        shutil.copy(os.path.join(d, n), os.path.join(out, n))
    for dst, src in replace.items():
        shutil.copy(os.path.join(d, f"f_{idx[int(src)]:02d}.png"), os.path.join(out, f"f_{idx[int(dst)]:02d}.png"))
    return out


def erase(d, spec, extra):
    """Erases, in the listed frames (sheet numbering), the effect blobs of one colour family drawn over the character:
    spec = {"frames": [...], "hue": "blue|yellow|red|warm|purple|scarlet|all", "min": 40} (the logic of
    tools/fxclean.py: connected blobs of that colour, with the near-white pixels touching them). "scarlet" is only the
    pure bright red of an effect, for a fighter whose own reds (hair, cape) are darker."""
    sys.path.insert(0, os.path.join(os.path.dirname(S), "tools"))
    from greyfix import label
    idx = [int(n[2:-4]) for n in frames_of(d)]
    if extra[:1] == ["--only"]:
        lo, hi = (int(v) for v in extra[1].split("-"))
        idx = [i for i in idx if lo <= i <= hi]
    else:
        idx = [i for i in idx if i > 0] or idx
    for k in spec["frames"]:
        p = os.path.join(d, f"f_{idx[k]:02d}.png")
        a = np.asarray(Image.open(p).convert("RGBA")).copy()
        rgb = a[..., :3].astype(int); op = a[..., 3] > 0
        yellow = op & (rgb[..., 0] > 190) & (rgb[..., 1] > 140) & (rgb[..., 2] < rgb[..., 1] - 60)
        red = op & (rgb[..., 0] > 150) & (rgb[..., 0] > rgb[..., 1] + 70) & (rgb[..., 2] < 100)
        bluish = op & (rgb[..., 2] > rgb[..., 0] + 50) & (rgb[..., 2] > 120)
        purple = op & (rgb[..., 2] > 150) & (rgb[..., 0] > 110) & (rgb[..., 1] < rgb[..., 0] - 40)
        scarlet = op & (rgb[..., 0] > 200) & (rgb[..., 1] < 40) & (rgb[..., 2] < 60)
        col = {"yellow": yellow, "red": red, "warm": yellow | red, "blue": bluish, "purple": purple, "scarlet": scarlet,
               "all": bluish | yellow | red | purple}[spec.get("hue", "all")]
        fx = col | (op & (rgb.min(-1) > 215))
        lab, n = label(fx)
        gone = np.zeros_like(fx)
        for i in range(1, n + 1):
            m = lab == i
            if m.sum() >= spec.get("min", 40) and (col & m).sum() >= 0.25 * m.sum():
                gone |= m
        a[gone] = 0
        Image.fromarray(a).save(p)


def degrey(d, frames, extra):
    """Erases the dithered grey slabs (tools/greyscan.py) of the listed frames (sheet numbering); frames is a list,
    or {"frames": [...], "tol": 30} to also catch taupe smoke ("open": 0 also catches a slab walled in by an effect's
    strokes, "min": 100 a smaller leftover)."""
    tol, open_min, min_area = 16, 0.25, 400
    if isinstance(frames, dict):
        tol, open_min, min_area = frames.get("tol", 16), frames.get("open", 0.25), frames.get("min", 400)
        frames = frames["frames"]
    sys.path.insert(0, os.path.join(os.path.dirname(S), "tools"))
    from greyscan import slabs2
    idx = [int(n[2:-4]) for n in frames_of(d)]
    if extra[:1] == ["--only"]:
        lo, hi = (int(v) for v in extra[1].split("-"))
        idx = [i for i in idx if lo <= i <= hi]
    else:
        idx = [i for i in idx if i > 0] or idx
    for k in frames:
        p = os.path.join(d, f"f_{idx[k]:02d}.png")
        a = np.asarray(Image.open(p).convert("RGBA")).copy()
        a[slabs2(a, min_area, tol, open_min)] = 0
        Image.fromarray(a).save(p)


def trim_right(d, trim):
    """Erases, in the listed frames, what lies right of the reference frame's drawing (an invented opponent's cloth
    cut by the frame edge, beyond the hands): trim = {"ref": k, "frames": [...], "margin": px} in sheet numbering."""
    idx = [int(n[2:-4]) for n in frames_of(d)]
    idx = [i for i in idx if i > 0] or idx
    ref = np.asarray(Image.open(os.path.join(d, f"f_{idx[trim['ref']]:02d}.png")).convert("RGBA"))
    x_max = int(np.nonzero(ref[..., 3] > 0)[1].max()) + int(trim.get("margin", 2))
    for k in trim["frames"]:
        p = os.path.join(d, f"f_{idx[k]:02d}.png")
        a = np.asarray(Image.open(p).convert("RGBA")).copy()
        a[:, x_max + 1:, 3] = 0
        Image.fromarray(a).save(p)


def dust(d, spec):
    """Erases the light warm dust kicked up at the feet: spec = {"frames": [...], "rows": 50}, files numbered like
    f_NN; in the bottom `rows` pixels of each frame's drawing, the light pixels warmer than grey (boots stay)."""
    for k in spec["frames"]:
        p = os.path.join(d, f"f_{k:02d}.png")
        a = np.asarray(Image.open(p).convert("RGBA")).copy()
        rows = np.flatnonzero((a[..., 3] > 0).any(axis=1))
        if not len(rows):
            continue
        rgb = a[..., :3].astype(int)
        warm = (a[..., 3] > 0) & (rgb.min(-1) > 140) & (rgb[..., 0] - rgb[..., 2] > 20)
        warm[: rows[-1] - spec.get("rows", 50)] = False
        a[warm] = 0
        Image.fromarray(a).save(p)


def specks(d, spec, extra):
    """Erases, in the listed frames (sheet numbering), the small bits floating apart from the character (sparks,
    petals, streaks): spec = {"frames": [...], "max": 150}; pieces of fewer pixels go, the biggest piece stays.
    Pixels one apart (diagonals, a one-pixel gap) count as touching, so a thin edge of the character is kept."""
    sys.path.insert(0, os.path.join(os.path.dirname(S), "tools"))
    from greyfix import label
    idx = [int(n[2:-4]) for n in frames_of(d)]
    if extra[:1] == ["--only"]:
        lo, hi = (int(v) for v in extra[1].split("-"))
        idx = [i for i in idx if lo <= i <= hi]
    else:
        idx = [i for i in idx if i > 0] or idx
    for k in spec["frames"]:
        p = os.path.join(d, f"f_{idx[k]:02d}.png")
        a = np.asarray(Image.open(p).convert("RGBA")).copy()
        op = a[..., 3] > 0
        near = op.copy()
        near[1:] |= op[:-1]
        near[:-1] |= op[1:]
        grown = near.copy()
        grown[:, 1:] |= near[:, :-1]
        grown[:, :-1] |= near[:, 1:]
        lab, n = label(grown)
        sizes = np.bincount(lab[op], minlength=n + 1)
        big = int(sizes.argmax())
        small = [i for i in range(1, n + 1) if i != big and sizes[i] < spec.get("max", 150)]
        a[np.isin(lab, small) & op] = 0
        Image.fromarray(a).save(p)


def wslot_name(name):
    """w0_ledge, w1_climb...: a clip of a weapon's look."""
    return len(name) > 3 and name[0] == "w" and name[1].isdigit() and name[2] == "_"


def frames_of(d):
    return sorted(f for f in os.listdir(d) if f.startswith("f_") and f.endswith(".png"))


def impact_frame(d, direction, ax, ay):
    if direction in ("none", None):
        return -1
    fs = frames_of(d)[1:]  # f_00 is the input
    best, bi = -1e9, -1
    for i, f in enumerate(fs):
        a = np.asarray(Image.open(os.path.join(d, f)).convert("RGBA"))[..., 3] > 0
        ys, xs = np.nonzero(a)
        if len(xs) == 0:
            continue
        reach = {"forward": xs.max() - ax, "back": ax - xs.min(), "up": ay - ys.min(), "down": ys.max() - ay,
                 "around": max(xs.max() - ax, ax - xs.min())}[direction]
        if reach > best:
            best, bi = reach, i
    return bi


def area_frame(d, direction, ax, ay, pxh):
    """The frame with the most drawn pixels where the blow goes (in front of the body, above the head...): a
    slash's crescent or a jutsu's burst, where reach alone picks a sword merely held out."""
    fs = frames_of(d)[1:]
    best, bi = -1, -1
    for i, f in enumerate(fs):
        a = np.asarray(Image.open(os.path.join(d, f)).convert("RGBA"))[..., 3] > 0
        ys, xs = np.nonzero(a)
        n = {"forward": (xs > ax + 0.22 * pxh).sum(), "back": (xs < ax - 0.22 * pxh).sum(),
             "up": (ys < ay - 1.0 * pxh).sum(), "down": (ys > ay - 0.2 * pxh).sum(),
             "around": ((xs > ax + 0.22 * pxh) | (xs < ax - 0.22 * pxh)).sum()}.get(direction, -1)
        if n > best:
            best, bi = n, i
    return bi


def main():
    fighter = sys.argv[1]
    only = set(sys.argv[2:])
    ax, ay, pxh = ANCHOR[fighter]
    spec = clips(fighter)
    for name, (n, base, pin, loop, direction, desc) in spec.items():
        if name in HELPERS or (only and name not in only):
            continue
        d = os.path.join(S, fighter, name)
        job = os.path.join(d, "job.json")
        if not os.path.exists(job) or json.load(open(job)).get("status") != "done" or not os.path.exists(os.path.join(d, "f_01.png")):
            print("missing", name)
            continue
        subprocess.run(["python3", os.path.join(os.path.dirname(S), "tools", "greyfix.py"), d, "--fix"], check=True)
        cax, cay = ax, ay + SHIFT.get((fighter, name), 0)
        state = name[3:] if wslot_name(name) else name  # a weapon's look: w1_ledge is anchored like the ledge
        if state == "ledge":  # the hands on the ledge
            a = np.asarray(Image.open(os.path.join(d, "f_01.png")).convert("RGBA"))[..., 3] > 0
            cay = float(np.nonzero(a.any(axis=1))[0].min()) + 0.75 * pxh
        args = ["python3", os.path.join(REPO, "scripts", "import_clip.py"), fighter, name, d,
                "--anchor", str(cax), str(cay), "--pxh", str(pxh)]
        if loop:
            args.append("--loop")
        extra = list(EXTRA.get((fighter, name), []))
        if state == "climb":  # starts hanging (hands on the ledge, as in the ledge clip), ends standing on the anchor
            a0 = np.asarray(Image.open(os.path.join(d, "f_00.png")).convert("RGBA"))[..., 3] > 0
            hands = float(np.nonzero(a0.any(axis=1))[0].min())
            extra += ["--drop", f"{ay - (hands + 0.75 * pxh):.1f}"]
        big = fighter == "sasuke" or "special" in name or "ultimate" in name or name.startswith("w")
        imp = area_frame(d, direction, ax, ay, pxh) if big and direction not in ("none", None) else \
            impact_frame(d, direction, ax, ay)
        if extra[:1] == ["--only"]:  # the impact counts from the first frame kept
            lo, hi = (int(v) for v in extra[1].split("-"))
            imp = min(max(imp - (lo - 1), 0), hi - lo) if imp >= 0 else imp
        if (fighter, name) in IMPACTS:
            args += ["--impact", IMPACTS[(fighter, name)]]
        elif imp >= 0 and not loop:
            args += ["--impact", str(imp)]
        args += extra
        fx = QA_FIXES.get(fighter, {}).get(name, {})
        if fx.get("impact"):
            args = [a for i, a in enumerate(args) if a != "--impact" and (i == 0 or args[i - 1] != "--impact")]
            args += ["--impact", fx["impact"]]
        if any(fx.get(k) for k in ("replace", "trim_right", "degrey", "erase", "dust", "specks")):  # review fixes, in a copy
            args[4] = with_replaced(d, fx.get("replace", {}), extra)
            if fx.get("erase"):  # one spec, or a list of them (several colour families)
                for spec in fx["erase"] if isinstance(fx["erase"], list) else [fx["erase"]]:
                    erase(args[4], spec, extra)
            if fx.get("trim_right"):
                trim_right(args[4], fx["trim_right"])
            if fx.get("degrey"):
                degrey(args[4], fx["degrey"], extra)
            if fx.get("dust"):
                dust(args[4], fx["dust"])
            if fx.get("specks"):
                specks(args[4], fx["specks"], extra)
        r = subprocess.run(args, capture_output=True, text=True)
        print((r.stdout.strip() or r.stderr.strip()[-300:]))


if __name__ == "__main__":
    main()
