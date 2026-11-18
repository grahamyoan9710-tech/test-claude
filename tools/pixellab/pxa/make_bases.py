#!/usr/bin/env python3
"""The bases of the new fighters (the guard pictures every clip starts from), from the owner's reference sheet.

    python3 make_bases.py guard           -> pxa/base/<id>_guard256.png from the chosen pro candidate (PICK)
    python3 make_bases.py armed [ids...]  -> submits the armed guards: the guard + the owner's weapon icon, as
                                             references to the pro model (pxa/base_src/<id>_w<slot>/)
    python3 make_bases.py v2 [ids...]     -> submits the guards of the new fighters (V2) from their cell of the sheet
    python3 make_bases.py armedpick      -> pxa/base/<id>_w<slot>guard256.png from the chosen candidates (APICK),
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


# The new fighters (planche v2): their look as the owner's sheet draws it (docs/refonte/ref_persos_v2.png, cut into
# pxa/base_src/refs_v2/<id>.png by recolor.cells/body) and their martial art's guard (FICHES.md 1.7).
ADULT = (" An adult of at least 20 with adult proportions: longer legs and torso, a slightly smaller head; everything "
         "else exactly as the reference.")
V2 = {
    "h02": ("an adult woman with very long dark brown-black hair falling to her knees, tied in a high ponytail with long "
            "red ribbons and a cluster of red and gold beads, a long front lock, red eyes, a gentle smile, a white "
            "shrine maiden robe with very long wide sleeves with red branch patterns and red cuffs, the sleeves "
            "detached at her bare shoulders, a white kimono collar with red edges crossed over her chest, a black "
            "corset, a wide red belt with a big gold buckle and a thin gold chain, a black front panel with red and "
            "gold trim, a long white skirt panel with red branch patterns over a red and black checkered underskirt, "
            "sheer black stockings with garters, red and black heeled boots with gold buckles",
            "standing in a calm aikido guard, her open hands raised in front of her, half hidden in her long sleeves"),
    "h05": ("a huge muscular adult man with a big wild white mane falling down his back, a short black beard and thick "
            "eyebrows, tanned skin, a bare muscular chest and abs with dark tattoos, a big white fur collar, a long "
            "black and red coat with gold trim and red and orange flame patterns on its lower part, a wide red sash "
            "belt with a big gold buckle with a red gem, a red and black front panel, bronze armoured shoulder and arm "
            "guards, a red rope coiled around his upper left arm, black fingerless gloves with gold studs, baggy black "
            "trousers, brown and bronze armoured boots",
            "standing in a wide low judo guard, his open hands raised in front of his chest"),
    "h08": ("an adult woman with long wavy teal hair worn inside her hood, falling in front of her shoulders, never "
            "coming out through the hood or down her back, green eyes, a white hood with cat ears (yellow inside) and "
            "yellow patterns that covers the back of her head and her hair, a long loose white coat with yellow "
            "patterns, a black crop top with yellow patterns, black shorts with a silver belt, black fingerless gloves, "
            "a grey stocking on one leg and a black garter on the other, chunky white and yellow sneakers, a big dark "
            "khaki backpack with yellow-green straps and a tassel",
            "standing in a kung fu tiger guard, her hands raised in front of her like tiger claws"),
    "h10": ("an adult woman with very long silver-white hair with a faint lavender tint falling to her knees, "
            "violet-pink eyes, a large black witch hat with gold trim, a violet band, long violet ribbons, white and "
            "violet flowers and hanging red pendants, a long white coat with violet patterns, gold trim and a violet "
            "lining, worn off her shoulders, a black top with gold ornaments, a short white and violet pleated skirt, "
            "black fingerless gloves, a pink crystal pendant at her hip, black thigh-high stockings with garters, black "
            "and violet heeled boots",
            "standing in a kickboxing guard, fists raised"),
}
V2_ADULT = {"h08"}  # drawn younger on the sheet: the base makes her adult (planche v2, prompts_10_persos.md)
# A guard asked again in another stance (pxa/base_src/<id>_guard<suffix>/): Fuyuri's idle keeps her hands in her
# sleeves (fiche 02), and a stance clip from the first guard came out blurrier than a pro picture.
V2_REDO = {"h02": ("b", "standing relaxed and upright in a calm aikido stance, one foot a little forward, her hands "
                        "joined in front of her waist and tucked inside her long wide sleeves, the head slightly "
                        "tilted, a mischievous little smile")}
V2_PICK = {"h02": ("b", 2)}  # the chosen guard of a new fighter, as PICK (make_bases.py guard places it too)


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
        for k, (suffix, i) in V2_PICK.items():
            print(k, place(os.path.join(S, "base_src", f"{k}_guard{suffix}", f"f_{i:02d}.png"),
                           os.path.join(S, "base", f"{k}_guard256.png")))
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
    elif cmd == "v2":  # python3 make_bases.py v2 h02 h05 ... -> pro candidates in pxa/base_src/<id>_guard/
        for k in sys.argv[2:] or sorted(V2):
            look, stance = V2[k]
            suffix, stance = V2_REDO.get(k, ("", stance))
            out = os.path.join(S, "base_src", f"{k}_guard{suffix}")
            if os.path.exists(os.path.join(out, "job.json")):
                print(k, "already submitted")
                continue
            ref = os.path.join(S, "base_src", "refs_v2", f"{k}.png")
            her = "her" if look.startswith("an adult woman") else "his"
            desc = (f"{look[0].upper()}{look[1:]}. Full body, side view facing right, {stance}, feet apart, "
                    f"bare-handed. Same character design, hair, face, outfit and colours as the reference. Only this "
                    f"character is drawn." + (ADULT if k in V2_ADULT else ""))
            r = subprocess.run(["python3", os.path.join(os.path.dirname(S), "tools", "pxcli.py"), "image", out,
                                "--model", "pro", "--desc", desc, "--w", "170", "--h", "170", "--style", ref,
                                "--ref", ref + f"|the character to draw: copy {her} hair, face, outfit, colours and "
                                               "proportions exactly", "--no-wait"],
                               capture_output=True, text=True, timeout=300)
            print(k, r.returncode, (r.stdout + r.stderr).strip().splitlines()[-1:])
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
