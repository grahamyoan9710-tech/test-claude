"""Contact sheet of PixelLab frame folders: python3 strips.py OUT.png DIR [DIR...] [--scale 0.5] [--label]
One row per folder (f_00.png, f_01.png...), on a slate background, with the folder name on the left."""
import os, sys
from PIL import Image, ImageDraw, ImageFont
args = [a for a in sys.argv[1:]]
scale = 0.5
if '--scale' in args:
    i = args.index('--scale'); scale = float(args[i + 1]); del args[i:i + 2]
out, dirs = args[0], args[1:]
rows = []
for d in dirs:
    fs = sorted(f for f in os.listdir(d) if f.startswith('f_') and f.endswith('.png'))
    rows.append((os.path.basename(d.rstrip('/')), [Image.open(os.path.join(d, f)).convert('RGBA') for f in fs]))
fw = max((im.width for _, ims in rows for im in ims), default=256)
fh = max((im.height for _, ims in rows for im in ims), default=256)
n = max((len(ims) for _, ims in rows), default=1)
lw = 150
W, H = int(lw + n * fw * scale), int(len(rows) * fh * scale)
sheet = Image.new('RGBA', (W, H), (58, 66, 80, 255))
d = ImageDraw.Draw(sheet)
try:
    font = ImageFont.truetype('/home/user/test-claude/assets/fonts/Poppins-Bold.ttf', 14)
except Exception:
    font = None
for r, (name, ims) in enumerate(rows):
    y = int(r * fh * scale)
    d.text((6, y + 6), name[:22], fill=(240, 230, 200, 255), font=font)
    for c, im in enumerate(ims):
        t = im.resize((int(im.width * scale), int(im.height * scale)), Image.NEAREST)
        sheet.alpha_composite(t, (int(lw + c * fw * scale), y))
    d.line([(0, y), (W, y)], fill=(30, 34, 40, 255))
sheet.convert('RGB').save(out)
print(out, sheet.size)
