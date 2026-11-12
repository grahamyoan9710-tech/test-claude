# Frames fNN.png -> a game strip laid out in rows (as many frames per row as fit under 8192 px).
# Usage: mkgrid.py <png_dir> <out.png> <frames>
import sys
from PIL import Image
src, out, n = sys.argv[1], sys.argv[2], int(sys.argv[3])
frames = [Image.open('%s/f%02d.png' % (src, i)).convert('RGBA') for i in range(n)]
fw, fh = frames[0].size
assert all(f.size == (fw, fh) for f in frames)
cols = min(n, 8191 // fw)  # under 8192 px; no empty columns: the game counts them from the width
sheet = Image.new('RGBA', (cols * fw, -(-n // cols) * fh), (0, 0, 0, 0))
for i, fr in enumerate(frames):
    sheet.paste(fr, ((i % cols) * fw, (i // cols) * fh))
sheet.save(out, optimize=True)
print(out, sheet.size, 'cols', cols)
