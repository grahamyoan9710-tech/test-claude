"""python3 sheetstrip.py OUT.png fighter:clip ... : the imported sheets as rows (frame numbers, impact frames boxed)."""
import sys
from PIL import Image, ImageDraw
R = '/home/user/test-claude/assets/sprites/'
info = {}
for l in open(R + 'sheets.txt'):
    p = l.split()
    if len(p) >= 11 and p[0] == 'sheet':
        info[(p[1], p[2])] = p
rows = []
for a in sys.argv[2:]:
    f, c = a.split(':')
    p = info[(f, c)]
    n, fw, fh = int(p[3]), int(p[4]), int(p[5])
    imp = {int(v) for v in p[10].split(',') if v.lstrip('-').isdigit() and int(v) >= 0}
    rows.append((a, n, fw, fh, imp, Image.open(R + f'{f}_{c}.png').convert('RGBA')))
H = 110
W = max(120 + r[1] * int(r[2] * H / r[3] + 6) for r in rows)
g = Image.new('RGB', (W, len(rows) * (H + 16)), (58, 66, 80))
d = ImageDraw.Draw(g)
for i, (a, n, fw, fh, imp, im) in enumerate(rows):
    y = i * (H + 16)
    d.text((4, y + 4), a, fill=(255, 255, 255))
    x = 120
    for k in range(n):
        fr = im.crop((k * fw, 0, (k + 1) * fw, fh)).resize((int(fw * H / fh), H), Image.NEAREST)
        g.paste(fr, (x, y), fr)
        d.text((x + 2, y + H + 2), str(k), fill=(255, 220, 120) if k in imp else (200, 200, 200))
        if k in imp:
            d.rectangle((x, y, x + fr.width, y + H), outline=(230, 60, 60))
        x += fr.width + 6
g.save(sys.argv[1])
print(g.size)
