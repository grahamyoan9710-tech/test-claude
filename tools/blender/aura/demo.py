# demo.py <png_root> <fighter> <out.mp4> <aura_AX> <aura_AY> <eveil_AX> <eveil_AY> [--no-rim]
# A filmed demo of an awakening as the game will show it: the burst (behind + _front, drawn 1.6x) over the idle clip,
# then the aura looping behind the animated idle with the rim light the game draws (8 crisp copies 2 px out in the
# hot colour, 8 soft ones 5 px out in the main colour, after render_fighter.cpp's orb outline). 2x, 30 fps.
import os, subprocess, sys, tempfile
from PIL import Image, ImageChops, ImageFilter

R = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../..'))
sys.path.insert(0, os.path.join(R, 'tools/blender'))
import aura_geo  # noqa: E402

root, who, out = sys.argv[1], sys.argv[2], sys.argv[3]
aax, aay, eax, eay = (float(v) for v in sys.argv[4:8])
rim = '--no-rim' not in sys.argv
L = [l.split() for l in open(R + '/assets/sprites/sheets.txt') if l.split()[:3] == ['sheet', who, 'idle']][0]
n, fw, fh, ax, ay, pph, fps = int(L[3]), int(float(L[4])), int(float(L[5])), float(L[6]), float(L[7]), float(L[8]), float(L[11])
sheet = Image.open('%s/assets/sprites/%s_idle.png' % (R, who)).convert('RGBA')
k = 170.0 / pph  # strip px per sprite px (the aura: drawn for a fighter 170 px tall)
idle = [sheet.crop((i * fw, 0, (i + 1) * fw, fh)).resize((round(fw * k), round(fh * k)), Image.NEAREST) for i in range(n)]
hexc = lambda h: tuple(int(h[i:i + 2], 16) for i in (1, 3, 5))
deep, main, hot, core, _ = aura_geo.PAL[who]


def frames(d):
    return [Image.open(os.path.join(d, f)).convert('RGBA') for f in sorted(os.listdir(d)) if f.endswith('.png')]


aura = frames(root + '/aura-' + who)
has_ev = os.path.isdir(root + '/eveil-' + who)  # (Hakuen: no burst of his own)
ev, evf = (frames(root + '/eveil-' + who), frames(root + '/eveil-' + who + '_front')) if has_ev else ([], [])
W, H = 520, 420
FX, FY = W // 2, 330  # the feet
bg = Image.new('RGBA', (W, H), (16, 18, 30, 255))
for y in range(H):  # a dim arena: a floor line and a soft horizon
    c = (16 + y // 24, 18 + y // 22, 30 + y // 14, 255)
    bg.paste(c, (0, y, W, y + 1))
bg.paste((46, 50, 72, 255), (0, FY + 2, W, FY + 4))


def outline(spr, col, r, a):
    m = spr.getchannel('A').point(lambda v: 255 if v > 40 else 0)
    acc = Image.new('L', spr.size, 0)
    for dx, dy in ((r, 0), (-r, 0), (0, r), (0, -r), (r, r), (-r, r), (r, -r), (-r, -r)):
        acc = ImageChops.lighter(acc, ImageChops.offset(m, dx, dy))
    acc = acc.point(lambda v: int(v * a))
    lay = Image.new('RGBA', spr.size, col + (0,))
    lay.putalpha(acc)
    return lay


def body(i, t):
    spr = idle[int(t * fps) % n]
    pad = 12
    big = Image.new('RGBA', (spr.width + 2 * pad, spr.height + 2 * pad), (0, 0, 0, 0))
    big.alpha_composite(spr, (pad, pad))
    if rim:
        beat = 0.8 + 0.2 * __import__('math').sin(t * 6.28 / 1.2)
        glow = outline(big, hexc(main), 5, 0.65 * beat).filter(ImageFilter.GaussianBlur(3))
        crisp = outline(big, hexc(hot), 2, 1.0)
        b2 = Image.new('RGBA', big.size, (0, 0, 0, 0))
        b2.alpha_composite(glow)
        b2.alpha_composite(crisp)
        b2.alpha_composite(big)
        big = b2
    return big, (round(FX - ax * k) - pad, round(FY - ay * k) - pad)


def put(img, fx, AX, AY, s=1.0):
    if s != 1.0:
        fx = fx.resize((round(fx.width * s), round(fx.height * s)), Image.BILINEAR)
    img.alpha_composite(fx, (round(FX - AX * s), round(FY - AY * s)))


tmp = tempfile.mkdtemp()
seq = [('idle', i) for i in range(20)] + [('eveil', i) for i in range(len(ev))] + [('aura', i) for i in range(len(aura) * 2)]
for j, (what, i) in enumerate(seq):
    t = j / 30.0
    img = bg.copy()
    on = what != 'idle'
    if what == 'aura' or (what == 'eveil' and i >= 10):
        a = aura[i % len(aura)] if what == 'aura' else aura[0].copy()
        if what == 'eveil':  # the aura fading in under the end of the burst
            a.putalpha(a.getchannel('A').point(lambda v, q=(i - 10) / 14: int(v * q)))
        put(img, a, aax, aay)
    if what == 'eveil':
        put(img, ev[i], eax, eay, 1.6)
    if on:
        spr, at = body(j, t)
    else:
        spr = idle[int(t * fps) % n]
        at = (round(FX - ax * k), round(FY - ay * k))
    img.alpha_composite(spr, at)
    if what == 'eveil':
        put(img, evf[i], eax, eay, 1.6)
    img.convert('RGB').resize((W * 2, H * 2), Image.NEAREST).save('%s/%04d.png' % (tmp, j))
subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-framerate', '30', '-i', tmp + '/%04d.png', '-c:v', 'libx264',
                '-pix_fmt', 'yuv420p', '-crf', '16', out], check=True)
print(out, len(seq), 'frames')
