"""The owner's effect sheets redrawn by PixelLab: each drawing of docs/refonte/vfx_maquette_*.png (cut by
maquette_fx.py) is the init picture of a create_image_pixflux job (PixelLab draws the effect clean, as close to the
drawing as it can), then animate_image_pixminimax breaks it up. Safe to re-run: pxcli never pays twice for a folder.

    python3 maquette_gen.py image [names...]   -> gen/<name>/f_00.png (the effect at its peak)
    python3 maquette_gen.py anim [names...]    -> gen/<name>_anim/f_NN.png (f_00 = the peak, then it breaks up)
"""
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
os.environ.setdefault("MOVE_FX_OUT", os.path.join(HERE, "maquette"))
import maquette_fx as M  # noqa: E402

GEN = os.path.join(HERE, "gen")
PXCLI = os.path.join(os.path.dirname(HERE), "tools", "pxcli.py")
ONLY = "Only the effect is drawn: no character, no background, nothing else; the whole effect stays inside the picture."
# name: (canvas w, h, foot: the drawing sits on the bottom, what it is, how it breaks up)
SPEC = {
    "m_quick": (80, 80, False, "a small golden four-pointed star spark of a quick hit, white-hot centre, orange edges, "
                "two tiny sparks", "the spark flashes, then shrinks and breaks into tiny golden sparks that fade away"),
    "m_slash": (128, 128, False, "a sword slash hit effect, a bright blue crescent swoosh curving like a C with a "
                "white-hot core, cyan and deep blue edges, small blue shards flying off to the right",
                "the slash flashes, then breaks apart into small blue shards that scatter and fade away"),
    "m_rise": (104, 144, False, "an upward hit burst: golden orange flame spikes bursting up from a point, a thin white "
               "and grey streak shooting straight up, small orange embers",
               "the spikes shoot higher and thin out, the embers rise, everything fades away"),
    "m_crit": (136, 136, False, "a critical hit burst: a big red and white star explosion with long sharp red rays "
               "crossing, a white-hot core, small red sparks",
               "the star flashes white, its rays stretch and thin out, then it breaks into red sparks that fade away"),
    "m_ground_big": (160, 120, True, "a big ground impact: white and light grey shockwave spikes bursting up from the "
                     "ground in a wide fan, dark grey rocks flying, small debris",
                     "the spikes sink back into the ground, the rocks fall down, the dust fades away"),
    "m_ground_heavy": (152, 128, True, "a heavy ground slam: a tall white shockwave spike shooting up in the middle "
                       "with spikes fanning out, big dark grey rock chunks thrown up, dust",
                       "the spikes sink back into the ground, the rocks fall down, the dust fades away"),
    "m_jump": (120, 128, True, "a jump take-off effect: a small white burst of spikes from the ground with thin blue "
               "streaks shooting straight up, a flat white line on the ground",
               "the streaks fly up and vanish, the burst shrinks into the ground and fades away"),
    "m_dash": (152, 56, True, "a dash dust cloud on the ground: round white and light grey cartoon smoke puffs, bigger "
               "on the left, trailing into small puffs and dots to the right",
               "the puffs drift a little, shrink and fade away"),
    "m_ground": (160, 80, True, "a ground impact ring: a flat white oval shockwave ring on the ground with small white "
                 "spikes around its rim and dark grey rocks flying up",
                 "the ring widens and thins out, the rocks fall, everything fades away"),
    "m_land": (160, 64, True, "a landing dust cloud: round white and light grey cartoon smoke puffs spreading on the "
               "ground on both sides, small dust dots",
               "the puffs spread a little wider, shrink and fade away"),
    "shield": (144, 144, False, "a round see-through shield bubble in light blue, a darker blue rim, bright white "
               "shine highlights along its edge, a few small sparkles around it, nothing inside", ""),
}
# Grey drawings recoloured before PixelLab redraws them (the owner asked for a fitting colour): dark to light.
TINT = {"shield": ["#123a78", "#2f7fd0", "#7cc4f4", "#c4ecff", "#ffffff"]}


def tinted(im, ramp):
    """A grey drawing mapped onto a colour ramp by brightness."""
    a = np.asarray(im).astype(np.float32)
    lum = a[..., :3] @ np.array([0.3, 0.59, 0.11], np.float32) / 255.0
    cols = np.array([[int(c[i:i + 2], 16) for i in (1, 3, 5)] for c in ramp], np.float32)
    t = np.clip(lum, 0, 1) * (len(ramp) - 1)
    i0 = np.clip(np.floor(t).astype(int), 0, len(ramp) - 2)
    f = (t - i0)[..., None]
    rgb = cols[i0] * (1 - f) + cols[i0 + 1] * f
    return Image.fromarray(np.dstack([rgb, a[..., 3]]).astype(np.uint8), "RGBA")


def init_picture(name):
    """The drawing at twice its pixel grid (the fighters' pixel size), on the job's canvas."""
    w, h, foot = SPEC[name][:3]
    im = Image.fromarray(M.grid(name), "RGBA")
    if name in TINT:
        im = tinted(im, TINT[name])
    k = min(2.0, (w - 8) / im.width, (h - 8) / im.height)
    im = im.resize((round(im.width * k), round(im.height * k)), Image.NEAREST)
    c = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    c.alpha_composite(im, ((w - im.width) // 2, h - 4 - im.height if foot else (h - im.height) // 2))
    os.makedirs(GEN, exist_ok=True)
    path = os.path.join(GEN, f"init_{name}.png")
    c.save(path)
    return path


def image(name):
    w, h, _, what, _ = SPEC[name]
    return subprocess.run(["python3", PXCLI, "image", os.path.join(GEN, name), "--model", "pixflux", "--w", str(w),
                           "--h", str(h), "--init", init_picture(name), "--strength", "300", "--desc",
                           f"pixel art game VFX: {what}, crisp clean pixel art, transparent background, no character, "
                           "nothing else"], capture_output=True, text=True).stdout[-300:]


def anim(name):
    what, how = SPEC[name][3:]
    return subprocess.run(["python3", PXCLI, "anim", os.path.join(GEN, f"{name}_anim"), "--src",
                           os.path.join(GEN, name, "f_00.png"), "--frames", "8", "--engine", "mm", "--subject", what,
                           "--desc", f"pixel art effect animation: {how}, until almost nothing is left. {ONLY}"],
                          capture_output=True, text=True).stdout[-300:]


# The frames kept from each clip (PixelLab now and then draws a broken frame): the growth clip (<name>_rise, its
# last frame is the peak again) then the break-up clip (<name>_anim, f_00 = the peak).
KEEP = {"m_quick": (None, range(0, 7)), "m_slash": (None, range(0, 8)), "m_rise": (None, range(0, 7)),
        "m_crit": (None, range(0, 5)), "m_ground_big": (None, range(0, 9)), "m_ground_heavy": (None, range(0, 9)),
        "m_jump": (None, range(0, 6)), "m_dash": (range(1, 4), range(0, 7)), "m_ground": (None, [0, 2, 3, 4, 5, 6]),
        "m_land": (range(1, 4), range(0, 7))}
ORIGIN_X = {"m_dash": 0.22}  # where the dust starts, in the drawing (the fighter's feet when it dashes)


def anchor(name):
    """The effect's origin in its frames: the centre, or the foot of a ground effect."""
    w, h, foot = SPEC[name][:3]
    g = M.grid(name)
    k = min(2.0, (w - 8) / g.shape[1], (h - 8) / g.shape[0])
    dw = round(g.shape[1] * k)
    x = (w - dw) / 2 + ORIGIN_X.get(name, 0.5) * dw
    return (x, h - 4) if foot else (w / 2, h / 2)


def import_fx(name):
    """The kept frames, one after the other, into the game (scripts/import_fx.py)."""
    rise, out = KEEP[name]
    frames = [os.path.join(GEN, f"{name}_rise", f"f_{i:02d}.png") for i in (rise or [])]
    frames += [os.path.join(GEN, f"{name}_anim", f"f_{i:02d}.png") for i in out]
    tmp = os.path.join(GEN, "_import", name)
    os.makedirs(tmp, exist_ok=True)
    for f in os.listdir(tmp):
        os.remove(os.path.join(tmp, f))
    for i, f in enumerate(frames):
        Image.open(f).convert("RGBA").save(os.path.join(tmp, f"f_{i:02d}.png"))
    x, y = anchor(name)
    return subprocess.run(["python3", os.path.join(M.REPO, "scripts", "import_fx.py"), name, tmp, "--anchor", str(x),
                           str(y), "--fps", "15", "--keep-first"], capture_output=True, text=True, cwd=M.REPO).stdout


def import_shield():
    """The shield bubble, see-through inside (the owner's sheet has it on white), at twice its pixels."""
    im = Image.open(os.path.join(GEN, "shield", "f_00.png")).convert("RGBA")
    a = np.asarray(im).astype(np.float32)
    h, w = a.shape[:2]
    x0, y0, x1, y1 = im.getbbox()
    cx, cy, r = (x0 + x1) / 2, (y0 + y1) / 2, max(x1 - x0, y1 - y0) / 2
    y, x = np.mgrid[0:h, 0:w].astype(np.float32)
    d = np.hypot(x - cx, y - cy) / r
    lum = a[..., :3] @ np.array([0.3, 0.59, 0.11], np.float32) / 255.0
    inside = np.clip((d - 0.55) / 0.33, 0.0, 1.0)  # 0 in the middle, 1 at the rim
    see = np.where(lum > 0.78, 0.18 + 0.82 * inside, 1.0)  # the light inside lets the fighter through
    a[..., 3] = np.round(a[..., 3] * see / 17.0) * 17.0  # few alpha steps, pixel-art clean
    out = Image.fromarray(a.astype(np.uint8), "RGBA").crop(im.getbbox())
    out = out.resize((out.width * 2, out.height * 2), Image.NEAREST)
    out.save(os.path.join(M.REPO, "assets", "sprites", "sys_shield_bubble.png"), optimize=True)
    return f"{out.width}x{out.height}"


def main():
    cmd, names = sys.argv[1], sys.argv[2:] or list(SPEC)
    if cmd == "import":
        for n in names:
            print(n, import_shield() if n == "shield" else import_fx(n).strip())
        return
    fn = {"image": image, "anim": anim}[cmd]
    with ThreadPoolExecutor(5) as pool:
        for n, out in zip(names, pool.map(fn, names)):
            print(n, out.strip().splitlines()[-1] if out.strip() else "?")


if __name__ == "__main__":
    main()
