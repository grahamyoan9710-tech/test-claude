"""Logo rebuilt from the mock: keep only the letter fills (white/grey, red/orange), the gold crown and the kana,
found inside the logo's brush shape, then draw a clean black outline around them (no brush, no scene fringe)."""
import numpy as np
from PIL import Image, ImageFilter
S = "/tmp/menu_cut/"  # where cut.py wrote logo.png: python3 cut.py 25 10 505 215 90 9 /tmp/menu_cut/logo.png
old = Image.open(S + "logo.png")                      # the first cut: RGB of the mock + the brush mask
rgb = np.asarray(old.convert("RGB"), float)
brush = np.asarray(old)[..., 3] > 128
inner = np.asarray(Image.fromarray((brush * 255).astype(np.uint8)).filter(ImageFilter.MinFilter(9))) > 128
r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
mx, mn = rgb.max(2), rgb.min(2)
light = (mn > 150) & (mx - mn < 70)                    # white / grey letters, kana
warm = (r > 150) & (r - b > 70) & (r - g > 40)         # red / orange letters
gold = (r > 190) & (g > 130) & (b < 120) & (r - b > 90)  # crown
fill = (light | warm | gold) & inner
m = Image.fromarray((fill * 255).astype(np.uint8))
m = m.filter(ImageFilter.MaxFilter(3)).filter(ImageFilter.MinFilter(3))  # close pinholes in the fills
fill = np.asarray(m) > 128
# drop the specks of the scene that passed the colour test: fill pieces under 30 px
from collections import deque
seen = np.zeros_like(fill); H, W = fill.shape
for y in range(H):
    for x in range(W):
        if fill[y, x] and not seen[y, x]:
            comp = []; q = deque([(y, x)]); seen[y, x] = True
            while q:
                cy, cx = q.popleft(); comp.append((cy, cx))
                for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    ny, nx = cy + dy, cx + dx
                    if 0 <= ny < H and 0 <= nx < W and fill[ny, nx] and not seen[ny, nx]:
                        seen[ny, nx] = True; q.append((ny, nx))
            if len(comp) < 30 and min(c[0] for c in comp) < H * 0.74:  # the kana row keeps its small strokes
                ys, xs = zip(*comp); fill[ys, xs] = False
m = Image.fromarray((fill * 255).astype(np.uint8))
outline = np.asarray(m.filter(ImageFilter.MaxFilter(9))) > 128           # 4 px black outline
out = np.zeros(rgb.shape[:2] + (4,), np.uint8)
out[outline] = (14, 10, 16, 255)
out[fill, :3] = rgb[fill].astype(np.uint8)
out[fill, 3] = 255
im = Image.fromarray(out)
a = im.getchannel("A").filter(ImageFilter.GaussianBlur(0.6)); im.putalpha(a)
arr = np.array(im); solid = arr[..., 3] > 40; seen = np.zeros_like(solid)
for y in range(H):  # a lone speck left outside the letters
    for x in range(W):
        if solid[y, x] and not seen[y, x]:
            comp = []; q = deque([(y, x)]); seen[y, x] = True
            while q:
                cy, cx = q.popleft(); comp.append((cy, cx))
                for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    ny, nx = cy + dy, cx + dx
                    if 0 <= ny < H and 0 <= nx < W and solid[ny, nx] and not seen[ny, nx]:
                        seen[ny, nx] = True; q.append((ny, nx))
            if len(comp) < 250:
                ys, xs = zip(*comp); arr[ys, xs, 3] = 0
im = Image.fromarray(arr)
im = im.crop(im.getbbox())
im.save(S + "logo2.png")
for bg, n in (((255, 255, 255, 255), "w"), ((20, 20, 30, 255), "d"), ((230, 90, 120, 255), "p")):
    c = Image.new("RGBA", im.size, bg); c.alpha_composite(im); c.save(S + f"logo2_{n}.png")
o = Image.new("RGBA", (im.width, im.height * 3))
for i, n in enumerate("wdp"): o.paste(Image.open(S + f"logo2_{n}.png"), (0, i * im.height))
o.save(S + "logo2_prev.png"); print(im.size)
