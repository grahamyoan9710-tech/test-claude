#!/usr/bin/env python3
"""The two pictures PixelLab animates a wave from: python3 wave_prep.py N SRC_DIR [W H]
waves_src/wN_full.png (the picture standing on the ground of a W x H canvas) and waves_src/wN_seed.png (the same
picture squashed flat on the ground: where the wave bursts from)."""
import sys

import numpy as np
from PIL import Image

n, src = int(sys.argv[1]), sys.argv[2]
W, H = (int(sys.argv[3]), int(sys.argv[4])) if len(sys.argv) > 4 else (224, 160)
im = Image.open(f"waves_src/{src}/f_00.png").convert("RGBA")
full = Image.new("RGBA", (W, H), (0, 0, 0, 0))
full.alpha_composite(im, ((W - im.width) // 2, H - im.height - 4))
full.save(f"waves_src/w{n}_full.png")
a = np.asarray(full)
rows = np.nonzero(a[:, :, 3].any(axis=1))[0]
top, bottom = rows.min(), rows.max()
seed = full.crop((0, top, W, bottom + 1)).resize((int(W * 0.7), max(4, (bottom - top) // 5)), Image.NEAREST)
s = Image.new("RGBA", (W, H), (0, 0, 0, 0))
s.alpha_composite(seed, ((W - seed.width) // 2, bottom + 1 - seed.height))
s.save(f"waves_src/w{n}_seed.png")
print(n, full.size)
