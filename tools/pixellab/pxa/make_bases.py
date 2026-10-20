#!/usr/bin/env python3
"""The bases of the new fighters (the guard pictures every clip starts from), from the owner's reference sheet.

    python3 make_bases.py guard           -> pxa/base/<id>_guard256.png from the chosen pro candidate (PICK)
    python3 make_bases.py armed [ids...]  -> submits the armed guards: the guard + the owner's weapon icon, as
                                             references to the pro model (pxa/base_src/<id>_w<slot>/)
    python3 make_bases.py armedpick       -> pxa/base/<id>_w<slot>guard256.png from the chosen candidates (APICK),
                                             feet where the guard's are, and pxa/anchors14.json for import_all.py

Every base is a 256x256 canvas, the fighter's feet on y = 244, its body centred on x = 118 (room in front for the
blows). Never resubmits a job: pxcli.py refuses a folder that already has a job.json.
"""
import json
import os
import subprocess
import sys

import numpy as np
from PIL import Image

S = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(S)))
SPRITES = os.path.join(ROOT, "assets", "sprites")

# The chosen candidate (f_NN) of each pro job, after looking at the four of them.
PICK = {"p01": 3, "p02": ("b", 1), "p03": 3, "p07": 2, "p08": 0}  # ("b", i): f_i of a redone job, <id>_guardb
APICK = {("p01", 0): 0, ("p01", 1): 0, ("p02", 0): ("b", 1), ("p02", 1): ("b", 0), ("p03", 0): 0, ("p03", 1): 0,
         ("p07", 0): 2, ("p07", 1): 2, ("p08", 0): 0, ("p08", 1): ("b", 0)}
# A redone armed guard: its own folder (pxa/base_src/<id>_w<slot><suffix>/) and what the prompt adds.
REDO = {("p08", 1): ("b", "His skin stays pale exactly like the first reference: only the halberd is golden"),
        ("p02", 0): ("b", "Her legs stay fully covered like the first reference: knee-length shorts over opaque tights"),
        ("p02", 1): ("b", "Her legs stay fully covered like the first reference: knee-length shorts over opaque tights")}


def guard_src(k):
    suffix, i = PICK[k] if isinstance(PICK[k], tuple) else ("", PICK[k])
    return os.path.join(S, "base_src", f"{k}_guard{suffix}", f"f_{i:02d}.png")

# The weapons in hand, as the owner's sheet draws them (docs/refonte/ref_armes.png), and how they are held.
WEAPONS = {
    ("p01", 0): ("a long curved black odachi sword with a violet ribbon", "holding it in both hands, the blade low behind him"),
    ("p01", 1): ("two curved violet daggers", "one dagger in each hand, held low in a crouched ready stance"),
    ("p02", 0): ("a long silver spear with white ribbons", "holding it in both hands, the point forward"),
    ("p02", 1): ("a staff topped by a blue crystal star with blue ribbons", "holding it upright beside her in one hand"),
    ("p03", 0): ("a huge jagged black greatsword with glowing red veins", "resting it on his shoulder with one hand"),
    ("p03", 1): ("a huge spiked black and red greatsword, larger and more jagged", "resting it on his shoulder with one hand"),
    ("p07", 0): ("a broad black and red sword with red ribbons", "holding it forward in one hand"),
    ("p07", 1): ("a broad black and red sword, larger and more jagged, with red ribbons", "holding it forward in one hand"),
    ("p08", 0): ("a white and gold sword with white ribbons", "holding it in one hand, the point down"),
    ("p08", 1): ("a golden halberd", "holding it upright in both hands"),
}


def feet_x(a):
    """The middle of the feet: the outer edges of the wide runs (boots, not a thin pole) in the bottom 10 rows."""
    ys = np.nonzero(a.any(axis=1))[0]
    cols = a[ys.max() - 9:ys.max() + 1].any(axis=0)
    runs, x = [], 0
    while x < len(cols):
        if cols[x]:
            x0 = x
            while x < len(cols) and cols[x]:
                x += 1
            if x - x0 >= 6:
                runs.append((x0, x))
        x += 1
    return (runs[0][0] + runs[-1][1]) / 2.0 if runs else (np.nonzero(cols)[0].min() + np.nonzero(cols)[0].max()) / 2.0


def place(src, dst, feet=None):
    """On the 256 canvas, feet on y = 244; the body centred on x = 118, or the feet on x = `feet` (an armed guard
    stands exactly where its bare-handed guard stands, so every clip of the fighter shares one anchor)."""
    im = Image.open(src).convert("RGBA")
    a = np.asarray(im)[:, :, 3] > 0
    ys, xs = np.nonzero(a)
    body = im.crop((xs.min(), ys.min(), xs.max() + 1, ys.max() + 1))
    if body.width > 250 or body.height > 250:
        body.thumbnail((250, 250), Image.NEAREST)
    canvas = Image.new("RGBA", (256, 256), (0, 0, 0, 0))
    x = 118 - body.width // 2 if feet is None else int(round(feet - feet_x(np.asarray(body)[:, :, 3] > 0)))
    x = max(0, min(256 - body.width, x))
    canvas.alpha_composite(body, (x, 244 - body.height))
    canvas.save(dst)
    return body.size


def anchor(k):
    """The fighter's anchor for import_all.py: feet x and y of its guard, and its height in pixels."""
    a = np.asarray(Image.open(os.path.join(S, "base", f"{k}_guard256.png")))[:, :, 3] > 0
    ys = np.nonzero(a.any(axis=1))[0]
    return [round(feet_x(a), 1), 244.0, int(ys.max() - ys.min() + 1)]


def main():
    cmd = sys.argv[1]
    os.makedirs(os.path.join(S, "base"), exist_ok=True)
    subjects = json.load(open(os.path.join(S, "subjects14.json")))
    if cmd == "guard":
        for k in PICK:
            print(k, place(guard_src(k), os.path.join(S, "base", f"{k}_guard256.png")))
    elif cmd == "armed":
        ids = sys.argv[2:] or sorted(PICK)
        for (k, slot), (weapon, hold) in WEAPONS.items():
            if k not in ids:
                continue
            suffix, extra = REDO.get((k, slot), ("", ""))
            out = os.path.join(S, "base_src", f"{k}_w{slot}{suffix}")
            if os.path.exists(os.path.join(out, "job.json")):
                print(k, slot, "already submitted")
                continue
            desc = (f"{subjects[k]}, holding {weapon}: {hold}. Full body, side view facing right, fighting stance. "
                    f"Same character as the first reference and exactly the weapon of the second reference. "
                    f"Only this character and the weapon are drawn." + (f" {extra}." if extra else ""))
            icon = os.path.join(SPRITES, f"{k}_icon_{'ab'[slot]}.png")
            r = subprocess.run(["python3", os.path.join(os.path.dirname(S), "tools", "pxcli.py"), "image", out,
                                "--model", "pro", "--desc", desc, "--w", "170", "--h", "170",
                                "--ref", guard_src(k) +
                                "|the character: keep the same hair, face, outfit and colours",
                                "--ref", icon + "|the weapon held: keep exactly its shape and colours",
                                "--no-wait"], capture_output=True, text=True, timeout=300)
            print(k, slot, r.returncode, (r.stdout + r.stderr).strip().splitlines()[-1:])
    elif cmd == "armedpick":
        for (k, slot), i in APICK.items():
            suffix, i = i if isinstance(i, tuple) else ("", i)
            src = os.path.join(S, "base_src", f"{k}_w{slot}{suffix}", f"f_{i:02d}.png")
            if not os.path.exists(src):
                print(k, slot, "not downloaded yet")
                continue
            print(k, slot, place(src, os.path.join(S, "base", f"{k}_w{slot}guard256.png"), anchor(k)[0]))
        json.dump({k: anchor(k) for k in PICK}, open(os.path.join(S, "anchors14.json"), "w"), indent=1)


if __name__ == "__main__":
    main()
