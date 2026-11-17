"""Ground weapon icons redrawn by PixelLab (pixflux) from the owner's weapon sheets: the old icon (a cut-out of
docs/refonte/ref_armes.png) is the init picture, or for the 10 fighters of the v2 sheet (h01..h10) the weapon cut out of
docs/refonte/ref_armes_v2_normales.png. python3 make_icons.py submit|import [p01_a h02_b ...]"""
import os, subprocess, sys
import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
SPRITES = os.path.join(REPO, "assets", "sprites")
PXCLI = os.path.join(HERE, "..", "tools", "pxcli.py")
SIZE = 128
STYLE = "pixel art game item icon, clean crisp pixels, vibrant saturated colours, transparent background"
DESC = {
    "p01_a": "a long curved odachi katana, dark purple-black blade with a thin pale edge, black handle with gold fittings, long dark purple ribbons hanging",
    "p01_b": "twin curved daggers side by side, black and dark purple blades with glowing violet edges, ornate dark hilts, violet glow",
    "p02_a": "a long silver spear, ornate silver and pale blue spearhead, slim dark shaft with silver fittings, white flowing ribbons",
    "p02_b": "a star scepter staff, a big glowing blue crystal eight-pointed star on top of a slim dark blue staff, light blue flowing ribbons",
    "p03_a": "a huge jagged black greatsword with glowing red veins and red edges, spiked black crossguard",
    "p03_b": "a huge jagged black greatsword with red veins engulfed in red and orange flames",
    "p07_a": "a broad black sword with red edges and spikes, red ribbons hanging from the hilt",
    "p07_b": "a broad black and red sword crackling with red lightning, red ribbons hanging from the hilt",
    "p08_a": "a white and gold longsword, white blade with gold trim, ornate gold crossguard, white flowing ribbons",
    "p08_b": "a golden halberd spear, ornate gold spear tip with side blades, dark shaft, golden flames, cream ribbons",
    # v2: the object only, no effect words (owner, 17/11)
    "h01_a": "a long curved nodachi sword, white steel blade with red patterns, dark red wrapped handle, round gold guard, red tassel",
    "h01_b": "a curved sword with an ornate gold star-shaped guard, white steel blade with red and gold patterns, dark red handle, red tassel with gold beads",
    "h02_a": "a Chinese glaive polearm, curved purple and silver blade on top, long black shaft with gold rings, gold tassel, gold spike at the bottom",
    "h02_b": "a bundle of paper talisman charms, cream paper strips with red patterns hanging from a bronze ring with gold beads and tassels",
    "h03_a": "a big mechanical gauntlet, purple and gold armour plates, dark grey metal fingers",
    "h03_b": "a big mechanical arm cannon, purple and black armour plates with gold rims, round open barrel",
    "h04_a": "an electric guitar shaped like a battle axe, blue and white body, black axe blade edges, silver strings, blue tassel",
    "h04_b": "two small flying speaker cubes, white with blue corners, black speaker cones",
    "h05_a": "a spiked mace, round dark iron head with gold spikes, gold and dark handle with blue runes, white feathers, gold spike at the bottom",
    "h05_b": "a pair of huge stone gauntlets side by side, brown stone fists with gold bands and blue runes",
    "h06_a": "a big rocket hammer, black and white striped head with pink hearts, pink ribbon, long striped pink handle",
    "h06_b": "a pink candy launcher gun with white bunny ears on top, pink barrel, round pink charm hanging",
    "h07_a": "a large scythe, curved purple blade with black spikes, purple orb at the joint, long black shaft, purple ribbons",
    "h07_b": "a chain sickle, a silver and purple curved sickle blade on a long silver chain with purple charms",
    "h08_a": "a pair of clawed gauntlets side by side, black and gold armour with teal gems, three long silver claws each",
    "h08_b": "a small round flying robot cat drone, white body with black cat ears, cute black face screen, yellow and teal fins",
    "h09_a": "twin short swords side by side, dark steel blades with pink edges, gold star-shaped guards, gold pommels, small tassels",
    "h09_b": "twin pistols side by side, dark green and gold engraved handguns",
    "h10_a": "a scepter staff, gold ring at the top with a purple orb inside, purple tassels and gold charms, long purple and gold shaft",
    "h10_b": "an open grimoire spellbook, purple and gold cover, cream pages with purple star circle drawings, purple ribbon bookmark",
}
V2 = os.path.join(REPO, "docs", "refonte", "ref_armes_v2_normales.png")
# Each v2 weapon = the blobs (ink darker than 236) whose top-left corner is listed; 10's staff and book touch, so the
# staff keeps x < 1540 or y < 575 of their blob and the book the rest.
V2_PARTS = {
    "h01_a": [(8, 37)], "h01_b": [(191, 27)], "h02_a": [(362, 10)], "h02_b": [(505, 125)],
    "h03_a": [(692, 139)], "h03_b": [(842, 110)], "h04_a": [(1052, 29)],
    "h04_b": [(1222, 223), (1238, 236), (1267, 123), (1302, 141)], "h05_a": [(1411, 13)],
    "h05_b": [(1504, 160), (1625, 159)], "h06_a": [(25, 464)], "h06_b": [(130, 559)], "h07_a": [(349, 459)],
    "h07_b": [(467, 593)], "h08_a": [(661, 532)], "h08_b": [(846, 560)], "h09_a": [(1041, 490)],
    "h09_b": [(1228, 563), (1266, 631)], "h10_a": [(1416, 450)], "h10_b": [(1416, 450)],
}


def v2_cutout(key):
    from greyfix import label
    img = np.asarray(Image.open(V2).convert("RGBA")).copy()
    lab, n = label(img[..., :3].min(-1) < 236)
    keep = np.zeros(lab.shape, bool)
    for i in range(1, n + 1):
        ys, xs = np.nonzero(lab == i)
        if len(xs) >= 800 and (int(xs.min()), int(ys.min())) in V2_PARTS[key]:
            keep |= lab == i
    assert keep.any(), key
    yy, xx = np.mgrid[:lab.shape[0], :lab.shape[1]]
    if key == "h10_a":
        keep &= (xx < 1540) | (yy < 575)
    elif key == "h10_b":
        keep &= (xx >= 1540) & (yy >= 575)
    # the white inside the weapon (blades, paper, speaker bodies) is above the ink threshold: fill the holes back in
    ys, xs = np.nonzero(keep)
    y0, y1, x0, x1 = ys.min() - 1, ys.max() + 2, xs.min() - 1, xs.max() + 2
    k = keep[y0:y1, x0:x1]
    out = np.zeros_like(k)
    h, w = k.shape
    stack = [(y, x) for y in range(h) for x in (0, w - 1)] + [(y, x) for x in range(w) for y in (0, h - 1)]
    while stack:
        y, x = stack.pop()
        if 0 <= y < h and 0 <= x < w and not k[y, x] and not out[y, x]:
            out[y, x] = True
            stack += [(y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)]
    keep[y0:y1, x0:x1] = ~out
    img[~keep, 3] = 0
    return img


def init_picture(key, path):
    """The old icon without its stray bits (pieces of the neighbour cells, the sheet's white left in a hole), fitted
    in the SIZE square."""
    sys.path.insert(0, os.path.join(HERE, "..", "tools"))
    from greyfix import label
    if key in V2_PARTS:
        a = v2_cutout(key)
    else:
        a = np.asarray(Image.open(os.path.join(SPRITES, key.replace("_", "_icon_") + ".png")).convert("RGBA")).copy()
    a[(a[..., :3] >= 248).all(-1), 3] = 0
    lab, n = label(a[..., 3] > 40)
    areas = [(lab == i).sum() for i in range(1, n + 1)]
    edge = set(np.unique(np.concatenate([lab[0], lab[-1], lab[:, 0], lab[:, -1]])))  # a neighbour cut by the crop
    for i, ar in enumerate(areas, 1):
        if ar < 0.15 * max(areas) or (i in edge and ar < 0.5 * max(areas)):
            a[lab == i, 3] = 0
    im = Image.fromarray(a)
    im = im.crop(im.getbbox())
    k = (SIZE - 8) / max(im.size)
    im = im.resize((max(1, round(im.width * k)), max(1, round(im.height * k))), Image.LANCZOS)
    out = Image.new("RGBA", (SIZE, SIZE))
    out.alpha_composite(im, ((SIZE - im.width) // 2, (SIZE - im.height) // 2))
    out.save(path)


def main():
    cmd, keys = sys.argv[1], sys.argv[2:] or list(DESC)
    for key in keys:
        d = os.path.join(HERE, key)
        if cmd == "submit":
            os.makedirs(d, exist_ok=True)
            init = os.path.join(d, "init.png")
            init_picture(key, init)
            subprocess.run(["python3", PXCLI, "image", d, "--model", "pixflux", "--w", str(SIZE), "--h", str(SIZE),
                            "--init", init, "--strength", "400", "--outline", "single color black outline",
                            "--detail", "highly detailed", "--desc", f"{STYLE}, {DESC[key]}", "--no-wait"], check=True)
        elif cmd == "import":  # trimmed to its drawing
            im = Image.open(os.path.join(d, "f_00.png")).convert("RGBA")
            im.crop(im.getbbox()).save(os.path.join(SPRITES, key.replace("_", "_icon_") + ".png"))
            print("imported", key)


if __name__ == "__main__":
    main()
