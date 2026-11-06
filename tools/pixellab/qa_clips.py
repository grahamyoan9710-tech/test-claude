"""python3 qa_clips.py FIGHTER OUTDIR: the fighter's imported clips (assets/sprites/<fighter>_<clip>.png, as indexed
by sheets.txt) drawn as labelled rows, frame numbers under each frame, the anchor (feet) marked, 11 rows a page:
OUTDIR/clips_<fighter>_<n>.png."""
import os, sys
from PIL import Image, ImageDraw
REPO = "/home/user/test-claude/assets/sprites"
f, out = sys.argv[1], sys.argv[2]
os.makedirs(out, exist_ok=True)
rows = []
for l in open(os.path.join(REPO, "sheets.txt")):
    p = l.split()
    if len(p) < 11 or p[0] != "sheet" or p[1] != f:
        continue
    name, n, fw, fh, ax, ay = p[2], int(p[3]), int(p[4]), int(p[5]), float(p[6]), float(p[7])
    im = Image.open(os.path.join(REPO, f"{f}_{name}.png")).convert("RGBA")
    rows.append((name, n, fw, fh, ax, ay, p[10], im))
rows.sort()
H = 120  # row height for the drawings
pages = [rows[i:i + 11] for i in range(0, len(rows), 11)]
for pi, page in enumerate(pages):
    W = 160 + max(int(r[1] * r[2] * H / max(r[3], 1)) + r[1] * 4 for r in page)
    g = Image.new("RGBA", (min(W, 4000), len(page) * (H + 22)), (58, 66, 80, 255))
    d = ImageDraw.Draw(g)
    for ri, (name, n, fw, fh, ax, ay, imp, im) in enumerate(page):
        y0 = ri * (H + 22)
        d.text((4, y0 + 4), f"{name} ({n}) impact {imp}", fill=(255, 255, 255, 255))
        s = H / fh
        per_row = im.width // fw
        for i in range(n):
            sx, sy = (i % per_row) * fw, (i // per_row) * fh
            fr = im.crop((sx, sy, sx + fw, sy + fh)).resize((max(1, int(fw * s)), H), Image.NEAREST)
            x = 160 + i * (int(fw * s) + 4)
            if x + fr.width > g.width:
                break
            d.rectangle([x, y0 + 2, x + fr.width - 1, y0 + 2 + H - 1], outline=(80, 92, 108, 255))
            g.alpha_composite(fr, (x, y0 + 2))
            fx, fy = x + ax * s, y0 + 2 + ay * s
            d.line([fx - 4, fy, fx + 4, fy], fill=(255, 80, 80, 255))
            d.text((x + 2, y0 + H + 6), str(i), fill=(255, 220, 0, 255))
    g.save(os.path.join(out, f"clips_{f}_{pi}.png"))
print(len(rows), "clips,", len(pages), "pages ->", out)
