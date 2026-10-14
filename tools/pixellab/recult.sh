#!/bin/bash
# recult.sh NAME FIGHTERS SLOT: records an ultimate (frames 270-500 every 10) and makes NAME_grid.png
S=/tmp/claude-0/-home-user-test-claude/454a74a0-0344-5f0d-8ed7-e18843be178a/scratchpad
n=$1; fi=$2; slot=$3
rm -rf $S/ultrec/$n; mkdir -p $S/ultrec/$n
cd /home/user/test-claude/build
timeout 150 xvfb-run -a ./super_clash --demo --fighters $fi --ult $slot --record 270 500 10 $S/ultrec/$n --quit-after 505 > /dev/null 2>&1
python3 - $S/ultrec/$n <<'PY'
import os, sys
from PIL import Image
d = sys.argv[1]
fs = sorted(os.listdir(d))
ims = [Image.open(os.path.join(d, f)).resize((320, 180)) for f in fs]
cols = 6; rows = (len(ims) + cols - 1) // cols
g = Image.new('RGB', (cols * 320, max(1, rows) * 180))
for i, im in enumerate(ims): g.paste(im, ((i % cols) * 320, (i // cols) * 180))
g.save(d + '_grid.png'); print(d, len(fs))
PY
