"""The fighters' skill effects redrawn by PixelLab after the owner's sheets 2 to 4 (docs/refonte/vfx_maquette_*.png):
each skill takes the sheet's shape that fits what it does, in its fighter's colours (07, the red lightning, takes the
lightning sheets). The cut-out drawing, recoloured, is the init picture of a create_image_pixflux job at the size the
game draws the old effect (skill_meta.json: its frame size, origin and hitbox radius), then animate_image_pixminimax
animates it: a loop for a projectile, a break-up for the rest. Safe to re-run: pxcli never pays twice for a folder.

    python3 skill_gen.py image [names...]    -> gen/<name>/f_00.png
    python3 skill_gen.py anim [names...]     -> gen/<name>_anim/f_NN.png
    python3 skill_gen.py import [names...]   -> assets/sprites/fx_<name>.png and its line in sheets.txt
"""
import json
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
META = json.load(open(os.path.join(HERE, "skill_meta.json")))  # the old effects: frame size, origin, hit radius
ONLY = "Only the effect is drawn: no character, no background, nothing else; the whole effect stays inside the picture."

# Each fighter's colours, dark to light (the grey drawing is mapped onto them), and how to say them.
RAMP = {"p01": ["#140a20", "#3a1a5c", "#6a34a8", "#9a62e0", "#d0b0ff", "#f4ecff"],
        "p02": ["#0e2a4a", "#24589a", "#4a90d8", "#8cc8f4", "#dcf2ff", "#ffffff"],
        "p03": ["#3a0806", "#8a1a0a", "#d8401a", "#ff8a2a", "#ffd890", "#fff8e0"],
        "p07": ["#300408", "#760c16", "#c41c2c", "#ff5a5a", "#ffd0d0", "#ffffff"],
        "p08": ["#5a4a20", "#a08a40", "#e0c870", "#fff0b0", "#fffbe8", "#ffffff"]}
WORDS = {"p01": "dark violet and purple with lavender highlights", "p02": "icy blue and pale cyan with white highlights",
         "p03": "fiery red, orange and yellow with a white-hot core", "p07": "crimson red with pink and white highlights",
         "p08": "gold and pale yellow with white highlights"}

# The sheet's shapes: what each one is.
SHAPE = {
    "s_fente": "a long horizontal energy slash, a thin sharp streak with a bright core and streaks trailing behind",
    "s_croissant": "a crescent-shaped energy wave, a thick curved crescent with a bright core and small streaks",
    "s_double_entaille": "two crossing energy slashes forming an X, sharp tapered blades of light",
    "s_orbe": "an energy orb projectile with a bright glowing head and spiky energy streaks trailing behind",
    "s_projectile_flamme": "a flame projectile, a fireball head with wavy flame tongues trailing behind",
    "s_lance": "a long thin energy lance projectile with a sharp point and thin streaks trailing behind",
    "s_flammes": "rising flames, tall flame tongues bursting up from the ground with small embers",
    "s_pilier": "a pillar of energy shooting up from the ground, a tall bright beam with a ribbon spiralling around it",
    "s_vortex": "a swirling energy vortex, curved spiral arms around a bright centre",
    "s_onde_sol": "a ground shockwave rushing forward, low sharp energy streaks along the ground",
    "s_arc": "a sharp diagonal crescent slash of energy with thin streaks",
    "s_vagues_croisees": "two crossing energy wave slashes forming a big X",
    "s_deferlante": "a big final energy surge, a mass of sharp flame-like spikes bursting up from the ground",
    "s_fente_descendante": "a downward diagonal energy slash, sharp curved blade strokes",
    "s_uppercut": "an upward crescent energy swoosh curving up, a rising slash",
    "s_trainee": "a comet of energy with a spiralling trail behind its round head",
    "s_anneau": "an energy ring, a flat glowing oval ring with sharp spikes on its sides",
    "s_tourbillon": "a whirlwind of curved energy blades spinning around a centre",
    "s_etincelles": "scattered four-pointed sparkle stars of different sizes",
    "s_rayon": "a continuous energy beam with a bright round head and a long straight ray",
    "s_explosion_croix": "a cross-shaped star explosion, four long sharp rays and a bright centre",
    "s_pics": "sharp energy spikes bursting up from the ground in a fan",
    "s_onde_pression": "concentric pressure waves, curved arcs around a bright centre",
    "s_flammes_jumelles": "twin curling flames forming an S shape, like wings of fire",
    "s_tir_concentre": "a concentrated energy shot, a cone of energy narrowing to a sharp point",
    "b_eclair": "a vertical lightning bolt, a zigzag with small branches",
    "b_eclair_ramifie": "a branching lightning bolt with many forks",
    "b_arc_horizontal": "a horizontal arc of lightning, a jagged electric streak",
    "b_impact_electrique": "an electric impact burst, jagged lightning spikes radiating from a bright centre",
    "b_frappe_sol": "a lightning bolt striking the ground, a jagged column with rocks thrown up",
    "b_eclairs_croises": "two crossing lightning bolts forming an X",
    "b_colonne": "a tall column of crackling lightning standing on the ground",
    "b_eclair_rapide": "a fast lightning streak with speed lines",
    "g_ruee": "a lightning rush, jagged electric streaks shooting forward low along the ground",
    "g_arc_ascendant": "a rising crescent of lightning curving upward with crackling branches",
    "g_frappe_verticale": "a huge vertical lightning strike falling from the sky onto the ground",
    "g_spirale": "a lightning spiral, a crackling electric coil winding upward",
    "g_dash_eclate": "a burst dash of jagged lightning shards flying forward",
}

# Each skill effect: (the sheet's shape, the drawing turned by degrees clockwise, a projectile that loops).
SKILL = {
    "p01_x_n": ("s_orbe", 0, True), "p01_x_s": ("s_fente", 0, False), "p01_x_u": ("s_etincelles", 0, False),
    "p01_x_d": ("s_onde_pression", 0, False), "p01_t_burst": ("s_deferlante", 0, False),
    "p01_a_n": ("s_onde_sol", 0, True), "p01_a_s": ("s_arc", 0, False), "p01_a_u": ("s_uppercut", 0, False),
    "p01_a_d": ("s_fente_descendante", 0, False), "p01_a_ult": ("s_vagues_croisees", 0, False),
    "p01_b_n": ("s_lance", 0, True), "p01_b_s": ("s_double_entaille", 0, False), "p01_b_u": ("s_tourbillon", 0, False),
    "p01_b_d": ("s_anneau", 0, True), "p01_b_ult": ("s_double_entaille", 0, False),
    "p02_x_n": ("s_tir_concentre", 0, True), "p02_x_s": ("s_fente", 0, False), "p02_x_u": ("s_pilier", 0, False),
    "p02_x_d": ("s_onde_pression", 0, False), "p02_t_burst": ("s_deferlante", 0, False),
    "p02_a_n": ("s_lance", 0, True), "p02_a_s": ("s_arc", 0, False), "p02_a_u": ("s_uppercut", 0, False),
    "p02_a_d": ("s_pics", 0, False), "p02_a_ult": ("s_vortex", 0, False),
    "p02_b_n": ("s_explosion_croix", 0, True), "p02_b_s": ("s_explosion_croix", 0, True),
    "p02_b_u": ("s_etincelles", 0, False), "p02_b_d": ("s_anneau", 0, False), "p02_b_ult": ("s_explosion_croix", 0, False),
    "p03_x_n": ("s_projectile_flamme", 0, True), "p03_x_s": ("s_fente", 0, False), "p03_x_u": ("s_pilier", 0, False),
    "p03_x_d": ("s_flammes", 0, False), "p03_t_burst": ("s_deferlante", 0, False),
    "p03_a_n": ("s_croissant", 0, True), "p03_a_s": ("s_arc", 0, False), "p03_a_u": ("s_uppercut", 0, False),
    "p03_a_d": ("s_pics", 0, False), "p03_a_ult": ("s_pilier", 0, False),
    "p03_b_n": ("s_projectile_flamme", 0, True), "p03_b_s": ("s_fente", 0, False),
    "p03_b_u": ("s_flammes_jumelles", 0, False), "p03_b_d": ("s_flammes", 0, False), "p03_b_ult": ("s_trainee", 0, True),
    "p07_x_n": ("b_arc_horizontal", 0, True), "p07_x_s": ("g_ruee", 0, False), "p07_x_u": ("g_arc_ascendant", 0, False),
    "p07_x_d": ("b_impact_electrique", 0, False), "p07_x_ult": ("b_colonne", 0, False),
    "p07_a_n": ("s_croissant", 0, True), "p07_a_s": ("g_dash_eclate", 0, False), "p07_a_u": ("g_spirale", 0, False),
    "p07_a_d": ("b_eclair", 0, False), "p07_a_ult": ("b_eclairs_croises", 0, False),
    "p07_b_n": ("b_eclair_rapide", 35, True), "p07_b_s": ("b_arc_horizontal", 0, False),
    "p07_b_u": ("b_eclair_ramifie", 0, False), "p07_b_d": ("b_frappe_sol", 0, False),
    "p07_b_ult": ("g_frappe_verticale", 0, False),
    "p08_x_n": ("s_lance", 0, True), "p08_x_s": ("s_fente", 0, False), "p08_x_u": ("s_flammes_jumelles", 0, False),
    "p08_x_d": ("s_anneau", 0, False), "p08_t_burst": ("s_deferlante", 0, False),
    "p08_a_n": ("s_croissant", 0, True), "p08_a_s": ("s_arc", 0, False), "p08_a_u": ("s_uppercut", 0, False),
    "p08_a_d": ("s_fente_descendante", 0, False), "p08_a_ult": ("s_pilier", 0, False),
    "p08_b_n": ("s_tir_concentre", 0, True), "p08_b_s": ("s_double_entaille", 0, False), "p08_b_u": ("s_rayon", -90, False),
    "p08_b_d": ("s_pilier", 0, False), "p08_b_ult": ("s_lance", 90, False),
}


def size(name):
    """The new frame: the old one's, twice for the small projectiles (more pixels to draw with)."""
    m = META[name]
    k = 2 if max(m["w"], m["h"]) < 110 else 1
    return k, m["w"] * k, m["h"] * k


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
    """The shape's drawing, recoloured, turned and fitted in the effect's frame (left-aligned for a dash)."""
    shape, turn, _ = SKILL[name]
    _, w, h = size(name)
    im = tinted(Image.fromarray(M.grid(shape), "RGBA"), RAMP[name[:3]])
    if turn:
        im = im.rotate(-turn, Image.NEAREST, expand=True)
        im = im.crop(im.getbbox())
    k = min((w - 6) / im.width, (h - 6) / im.height)
    im = im.resize((max(1, round(im.width * k)), max(1, round(im.height * k))), Image.NEAREST)
    c = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    left = META[name]["ax"] < 0.2 * META[name]["w"]  # the old effect started at the fighter and went forward
    c.alpha_composite(im, (3 if left else (w - im.width) // 2, (h - im.height) // 2))
    os.makedirs(GEN, exist_ok=True)
    path = os.path.join(GEN, f"init_{name}.png")
    c.save(path)
    return path


def image(name):
    _, w, h = size(name)
    what = f"{SHAPE[SKILL[name][0]]}, in {WORDS[name[:3]]}"
    return subprocess.run(["python3", PXCLI, "image", os.path.join(GEN, name), "--model", "pixflux", "--w", str(w),
                           "--h", str(h), "--init", init_picture(name), "--strength", "300", "--desc",
                           f"pixel art game VFX: {what}, crisp clean pixel art, transparent background, no character, "
                           "nothing else"], capture_output=True, text=True).stdout[-300:]


def anim(name):
    shape, _, loop = SKILL[name]
    what = f"{SHAPE[shape]}, in {WORDS[name[:3]]}"
    key = os.path.join(GEN, name, "f_00.png")
    how = ("it flickers and pulses in place, its streaks rippling, and comes back to the first frame so that it loops"
           if loop else "it flashes, then breaks apart into small pieces that scatter and fade away, until almost "
           "nothing is left")
    args = ["python3", PXCLI, "anim", os.path.join(GEN, f"{name}_anim"), "--src", key, "--frames", "8", "--engine",
            "mm", "--subject", what, "--desc", f"pixel art effect animation: {how}. {ONLY}"]
    if loop:
        args += ["--last", key]
    return subprocess.run(args, capture_output=True, text=True).stdout[-300:]


def import_fx(name):
    """The animation into the game, at the old effect's origin and hitbox radius (scaled with the frame)."""
    k = size(name)[0]
    m = META[name]
    loop = SKILL[name][2]
    d = os.path.join(GEN, f"{name}_anim")
    tmp = os.path.join(GEN, "_import", name)
    os.makedirs(tmp, exist_ok=True)
    for f in os.listdir(tmp):
        os.remove(os.path.join(tmp, f))
    for i in range(8):  # f_08 comes back to the start (a loop) or is all but empty (a break-up)
        Image.open(os.path.join(d, f"f_{i:02d}.png")).convert("RGBA").save(os.path.join(tmp, f"f_{i:02d}.png"))
    args = ["python3", os.path.join(M.REPO, "scripts", "import_fx.py"), name, tmp, "--anchor", str(m["ax"] * k),
            str(m["ay"] * k), "--fps", "15", "--keep-first"]
    if loop:
        args.append("--loop")
    if m["hit"]:
        args += ["--hit", str(m["hit"] * k)]
    return subprocess.run(args, capture_output=True, text=True, cwd=M.REPO).stdout


def main():
    cmd, names = sys.argv[1], sys.argv[2:] or list(SKILL)
    if cmd == "import":
        for n in names:
            print(n, import_fx(n).strip())
        return
    fn = {"image": image, "anim": anim}[cmd]
    with ThreadPoolExecutor(8) as pool:
        for n, out in zip(names, pool.map(fn, names)):
            print(n, out.strip().splitlines()[-1] if out.strip() else "?")


if __name__ == "__main__":
    main()
