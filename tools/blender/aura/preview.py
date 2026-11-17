# preview.py <png_dir> <fighter> <out.png> <AX> <AY> [front_dir] [cast_scale] [frames]
# Each frame (straight RGBA PNGs from post2.py) behind the fighter's idle sprite (and the front strip over it), as the
# game draws it, on a dark and a light ground; 2x, nearest for the sprite.
import os, sys
from PIL import Image

R = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../..'))  # the repo
d, who, out, AX, AY = sys.argv[1], sys.argv[2], sys.argv[3], float(sys.argv[4]), float(sys.argv[5])
front = sys.argv[6] if len(sys.argv) > 6 and sys.argv[6] != '-' else None
cs = float(sys.argv[7]) if len(sys.argv) > 7 else 1.0
pick = [int(x) for x in sys.argv[8].split(',')] if len(sys.argv) > 8 else None
L = [l.split() for l in open(R + '/assets/sprites/sheets.txt') if l.startswith('sheet ') and l.split()[1] == who and l.split()[2] == 'idle'][0]
fw, fh, ax, ay, pph = int(float(L[4])), int(float(L[5])), float(L[6]), float(L[7]), float(L[8])
spr = Image.open('%s/assets/sprites/%s_idle.png' % (R, who)).convert('RGBA').crop((0, 0, fw, fh))
k = 170.0 / pph / cs  # strip px per sprite px
spr = spr.resize((round(fw * k), round(fh * k)), Image.LANCZOS)
files = sorted(f for f in os.listdir(d) if f.endswith('.png'))
if pick:
    files = [files[i] for i in pick if i < len(files)]
Z = 2
tiles = []
for f in files:
    fx = Image.open(os.path.join(d, f)).convert('RGBA')
    W, H = fx.size
    row = []
    for bg in ((20, 22, 36), (150, 190, 230), (226, 205, 170)):
        b = Image.new('RGBA', (W, H), bg + (255,))
        b.alpha_composite(fx)
        b.alpha_composite(spr, (round(AX - ax * k), round(AY - ay * k)))
        if front and os.path.exists(os.path.join(front, f)):
            b.alpha_composite(Image.open(os.path.join(front, f)).convert('RGBA'))
        row.append(b.resize((W * Z, H * Z), Image.NEAREST))
    tiles.append(row)
W, H = tiles[0][0].size
sheet = Image.new('RGB', (W * len(tiles), H * 3))
for i, row in enumerate(tiles):
    for j, t in enumerate(row):
        sheet.paste(t.convert('RGB'), (i * W, j * H))
if sheet.width > 2400:
    sheet = sheet.resize((2400, int(sheet.height * 2400 / sheet.width)), Image.LANCZOS)
sheet.save(out)
print(out, sheet.size)
