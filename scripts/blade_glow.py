# The red glow of Hakuen's long odachi (weapon B; yoyo, 17/11: "la lame brille en rouge", Alpha's in PGR): for each
# w1_* clip, finds the blade on every frame (the long thin part of the outline, its line extended to the guard) and
# paints a neon line on it with its bloom, in a sheet laid out like the clip: assets/sprites/<id>_<clip>_glow.png,
# drawn by the game over the frame, additively.
# Usage: blade_glow.py <fighter> <clip>... [--sheet out.png]   (--sheet: a contact sheet to check the blades by eye)
import sys
from collections import deque
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

R = __file__.rsplit('/scripts/', 1)[0] + '/'
BLADE = 92.0  # px, tip to guard, in the clips' scale (156 px per fighter height): h01_w1_ref


def thin_part(im, r):
    """The longest thin, elongated piece of the outline (what an opening of radius r removes): the blade's free end."""
    a = (np.array(im.getchannel('A')) > 60).astype(np.uint8) * 255
    op = Image.fromarray(a).filter(ImageFilter.MinFilter(2 * r + 1)).filter(ImageFilter.MaxFilter(2 * r + 1))
    thin = (a > 0) & (np.array(op) == 0)
    H, W = thin.shape
    lab = np.zeros((H, W), np.int32)
    best, n = None, 0
    for y, x in zip(*np.nonzero(thin)):
        if lab[y, x]:
            continue
        n += 1
        q, pts = deque([(y, x)]), []
        lab[y, x] = n
        while q:
            cy, cx = q.popleft()
            pts.append((cy, cx))
            for yy in (cy - 1, cy, cy + 1):
                for xx in (cx - 1, cx, cx + 1):
                    if 0 <= yy < H and 0 <= xx < W and thin[yy, xx] and not lab[yy, xx]:
                        lab[yy, xx] = n
                        q.append((yy, xx))
        P = np.array(pts)
        ext = float(np.hypot(*(P.max(0) - P.min(0))))
        if ext >= 25 and len(P) < ext * 6 and (best is None or ext > best[0]):
            best = (ext, n)
    return (lab == best[1]) if best else None


def blade(im):
    """(tip, unit vector toward the guard) of the blade on this frame, or None."""
    best = None
    for r in (3, 4, 5):
        m = thin_part(im, r)
        if m is not None and (best is None or m.sum() > best.sum()):
            best = m
    if best is None:
        return None
    ys, xs = np.nonzero(best)
    P = np.c_[xs, ys].astype(float)
    c = P.mean(0)
    u = np.linalg.svd(P - c)[2][0]
    by, bx = np.nonzero(np.array(im.getchannel('A')) > 60)
    body = np.array([bx.mean(), by.mean()])
    t = (P - c) @ u
    e1, e2 = c + u * t.min(), c + u * t.max()
    tip, inner = (e1, e2) if np.linalg.norm(e1 - body) > np.linalg.norm(e2 - body) else (e2, e1)
    return tip, (inner - tip) / np.linalg.norm(inner - tip)


def glow(size, b):
    """The blade's light: a white-pink core in a neon red line in a soft red halo, thinner at the tip."""
    g = Image.new('RGBA', size, (0, 0, 0, 0))
    if b is None:
        return g
    tip, d = b

    def line(w, col):
        lay = Image.new('RGBA', size, (0, 0, 0, 0))
        dr = ImageDraw.Draw(lay)
        for k in range(24):
            p, q = tip + d * BLADE * k / 24, tip + d * BLADE * (k + 1) / 24
            dr.line([tuple(p), tuple(q)], fill=col, width=max(1, round(w * (0.45 + 0.55 * k / 24))))
        return lay

    for lay in (line(14, (255, 20, 70, 140)).filter(ImageFilter.GaussianBlur(6)),
                line(7, (255, 30, 80, 220)).filter(ImageFilter.GaussianBlur(1.5)),
                line(2, (255, 230, 238, 255))):
        g.alpha_composite(lay)
    return g


def main():
    args = sys.argv[1:]
    check = args[args.index('--sheet') + 1] if '--sheet' in args else None
    if check:
        args = args[:args.index('--sheet')]
    who, clips = args[0], args[1:]
    lines = {l.split()[2]: l.split() for l in open(R + 'assets/sprites/sheets.txt') if l.startswith('sheet ' + who + ' ')}
    rows = []
    for clip in clips:
        L = lines[clip]
        n, fw, fh = int(L[3]), int(float(L[4])), int(float(L[5]))
        sheet = Image.open(R + 'assets/sprites/%s_%s.png' % (who, clip)).convert('RGBA')
        frames = [sheet.crop((i * fw, 0, (i + 1) * fw, fh)) for i in range(n)]
        B = [blade(f) for f in frames]
        for i in range(n):  # a frame where it hides: its nearest neighbour's
            if B[i] is None:
                B[i] = next((B[j] for j in sorted(range(n), key=lambda j: abs(j - i)) if B[j] is not None), None)
        out = Image.new('RGBA', sheet.size, (0, 0, 0, 0))
        for i, b in enumerate(B):
            out.paste(glow((fw, fh), b), (i * fw, 0))
        out.save(R + 'assets/sprites/%s_%s_glow.png' % (who, clip), optimize=True)
        print(clip, sum(b is not None for b in B), '/', n)
        if check:
            row = Image.new('RGBA', sheet.size, (24, 26, 36, 255))
            row.alpha_composite(sheet)
            add = np.array(row).astype(int)
            gl = np.array(out).astype(int)
            add[..., :3] = np.clip(add[..., :3] + gl[..., :3] * gl[..., 3:4] // 255, 0, 255)
            rows.append(Image.fromarray(add.astype(np.uint8)))
    if check:
        W, H = max(r.width for r in rows), sum(r.height for r in rows)
        o = Image.new('RGBA', (W, H), (0, 0, 0, 255))
        y = 0
        for r in rows:
            o.paste(r, (0, y))
            y += r.height
        o.save(check)


if __name__ == '__main__':
    main()
