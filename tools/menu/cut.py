"""Cuts an element of docs/refonte/menu_accueil.png out of its background by difference with menu_fond_1.png:
cut.py X0 Y0 X1 Y1 THRESH GROW OUT.png -> RGBA crop (holes inside the element are filled)."""
import sys
import numpy as np
from PIL import Image, ImageDraw, ImageFilter
x0, y0, x1, y1, th, grow, out = *map(int, sys.argv[1:7]), sys.argv[7]
R = "/home/user/test-claude/docs/refonte/"
a = Image.open(R + "menu_accueil.png").convert("RGB").crop((x0, y0, x1, y1))
b = Image.open(R + "menu_fond_1.png").convert("RGB").crop((x0, y0, x1, y1))
d = np.abs(np.asarray(a, int) - np.asarray(b, int)).sum(2)
m = Image.fromarray(((d > th) * 255).astype(np.uint8))
m = m.filter(ImageFilter.MaxFilter(grow)).filter(ImageFilter.MinFilter(grow))  # close small gaps
# holes: whatever the outside cannot reach
pad = Image.new("L", (m.width + 2, m.height + 2), 0); pad.paste(m, (1, 1))
ImageDraw.floodfill(pad, (0, 0), 128)
p = np.asarray(pad)[1:-1, 1:-1]
mask = (p != 128).astype(np.uint8) * 255
alpha = Image.fromarray(mask).filter(ImageFilter.MinFilter(3)).filter(ImageFilter.GaussianBlur(0.7))
rgba = a.convert("RGBA"); rgba.putalpha(alpha)
rgba.save(out)
# preview on a checkerboard
chk = Image.new("RGBA", rgba.size, (60, 200, 90, 255)); chk.alpha_composite(rgba); chk.save(out[:-4] + "_prev.png")
print(out, rgba.size)
