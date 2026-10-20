"""Comparison sheet of the menu palettes: python3 palsheet.py KIND OUT.png (KIND = menu | css)."""
import sys
from PIL import Image, ImageDraw, ImageFont
P = '/tmp/claude-0/-home-user-test-claude/454a74a0-0344-5f0d-8ed7-e18843be178a/scratchpad/pal'
F = '/home/user/test-claude/assets/fonts/'
kind, out = sys.argv[1], sys.argv[2]
title = ImageFont.truetype(F + 'LuckiestGuy-Regular.ttf', 30)
small = ImageFont.truetype(F + 'Poppins-Bold.ttf', 15)
# number, index, name, mood, swatches (main menu tiles, then the two sides of the character select)
pals = [
    (1, 1, 'BRASIER', 'feu et adrénaline : rouge, orange, ambre',
     [(255, 64, 36), (255, 146, 24), (240, 176, 26), (84, 46, 44), (255, 52, 30), (255, 176, 20)]),
    (2, 2, 'NÉON', 'arcade de nuit : magenta, cyan, violet',
     [(255, 40, 150), (0, 210, 228), (140, 80, 255), (255, 214, 30), (255, 30, 150), (0, 210, 255)]),
    (3, 3, 'SHŌNEN', "générique d'anime : orange et bleu en plein jour",
     [(255, 112, 20), (112, 200, 40), (30, 130, 255), (222, 40, 60), (255, 120, 16), (20, 140, 255)]),
    (4, 4, 'ÉLECTRIQUE', 'la foudre : bleu électrique et jaune éclair',
     [(30, 150, 255), (255, 206, 20), (0, 206, 240), (130, 60, 240), (255, 196, 0), (0, 150, 255)]),
    (5, 5, 'POP', 'bonbons acidulés : rose, citron vert, turquoise',
     [(255, 50, 130), (150, 226, 30), (0, 200, 210), (255, 150, 30), (255, 50, 140), (0, 210, 220)]),
    (0, 0, 'CRÉPUSCULE', 'les couleurs actuelles (v8.1), pour comparer',
     [(218, 52, 70), (30, 164, 116), (48, 116, 230), (146, 74, 210), (232, 38, 80), (30, 128, 255)]),
]
cw, ch, band = 640, 360, 70
sheet = Image.new('RGB', (cw * 2, (ch + band) * 3), (14, 22, 26))
d = ImageDraw.Draw(sheet)
for k, (num, idx, name, mood, sw) in enumerate(pals):
    x, y = (k % 2) * cw, (k // 2) * (ch + band)
    label = f'{num} · {name}' if num else name
    d.text((x + 14, y + 8), label, font=title, fill=(246, 236, 212))
    d.text((x + 14, y + 44), mood, font=small, fill=(200, 190, 170))
    for j, c in enumerate(sw):  # swatches, top right
        sx = x + cw - 14 - (len(sw) - j) * 30 - (10 if j < 4 else 0)
        d.rectangle([sx, y + 12, sx + 24, y + 36], fill=c, outline=(232, 196, 110))
    im = Image.open(f'{P}/{kind}_{idx}.png').convert('RGB').resize((cw, ch), Image.LANCZOS)
    sheet.paste(im, (x, y + band))
sheet.save(out)
print(sheet.size)
