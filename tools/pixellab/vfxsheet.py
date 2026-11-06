"""python3 vfxsheet.py OUT.png effect ... : 4 frames of each effect (vfx/ult/<effect>.png + .json) on a dark background,
origin as a yellow cross, the hit radius from the notes as a red circle."""
import json, sys, glob
from PIL import Image, ImageDraw
U = 'vfx/ult/'
notes = {}
for f in glob.glob(U + 'notes_*.json'):
    for e in json.load(open(f)):
        notes[e['effect']] = e
rows = []
for name in sys.argv[2:]:
    j = json.load(open(U + name + '.json'))
    fr = j['frames']
    fr = list(fr.values()) if isinstance(fr, dict) else fr
    im = Image.open(U + name + '.png').convert('RGBA')
    n = len(fr)
    pick = sorted({0, n // 4, n // 2, (3 * n) // 4})
    tiles = [im.crop((f['frame']['x'], f['frame']['y'], f['frame']['x'] + f['frame']['w'], f['frame']['y'] + f['frame']['h'])) for f in (fr[k] for k in pick)]
    rows.append((name, n, tiles, notes.get(name, {})))
pad = 8
W = max(sum(t.width + pad for t in r[2]) + 150 for r in rows)
H = sum(max(t.height for t in r[2]) + 20 for r in rows)
g = Image.new('RGB', (W, H), (34, 38, 48))
d = ImageDraw.Draw(g)
y = 0
for name, n, tiles, nt in rows:
    d.text((4, y + 4), f"{name}\n{n} fr {nt.get('kind','')}\nr={nt.get('hit_radius_px')}", fill=(255, 255, 255))
    x = 150
    for t in tiles:
        g.paste(t, (x, y + 16), t)
        d.rectangle((x, y + 16, x + t.width - 1, y + 16 + t.height - 1), outline=(70, 75, 90))
        o = nt.get('origin')
        if o:
            ox, oy = x + o[0], y + 16 + o[1]
            d.line((ox - 4, oy, ox + 4, oy), fill=(255, 230, 0)); d.line((ox, oy - 4, ox, oy + 4), fill=(255, 230, 0))
            r = nt.get('hit_radius_px') or 0
            if r and nt.get('kind') != 'beam':
                d.ellipse((ox - r, oy - r, ox + r, oy + r), outline=(230, 60, 60))
        x += t.width + pad
    y += max(t.height for t in tiles) + 20
g.save(sys.argv[1]); print(g.size)
