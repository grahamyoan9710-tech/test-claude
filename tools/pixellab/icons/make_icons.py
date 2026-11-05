"""Ground weapon icons redrawn by PixelLab (pixflux) from the owner's weapon sheet: the old icon (a cut-out of
docs/refonte/ref_armes.png) is the init picture. python3 make_icons.py submit|import [p01_a p02_b ...]"""
import os, subprocess, sys
import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
SPRITES = os.path.join(REPO, "assets", "sprites")
PXCLI = os.path.join(HERE, "..", "tools", "pxcli.py")
SIZE = 128
STYLE = "pixel art game item icon, single weapon, clean crisp pixels, vibrant saturated colours, transparent background"
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
}


def init_picture(key, path):
    """The old icon without its stray bits (pieces of the neighbour cells, the sheet's white left in a hole), fitted
    in the SIZE square."""
    a = np.asarray(Image.open(os.path.join(SPRITES, key.replace("_", "_icon_") + ".png")).convert("RGBA")).copy()
    a[(a[..., :3] >= 248).all(-1), 3] = 0
    sys.path.insert(0, os.path.join(HERE, "..", "tools"))
    from greyfix import label
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
