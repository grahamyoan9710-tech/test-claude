#!/usr/bin/env python3
"""Sets finished clips aside so the driver makes them again (a new prompt): python3 retire.py TAG FIGHTER CLIP...
pxa/<fighter>/<clip> becomes pxa/<fighter>/<clip>.<TAG>. A clip whose job is still running is left alone (never
resubmitted while PixelLab is working on it)."""
import json
import os
import sys

S = os.path.dirname(os.path.abspath(__file__))
tag, fighter, names = sys.argv[1], sys.argv[2], sys.argv[3:]
for n in names:
    d = os.path.join(S, fighter, n)
    jf = os.path.join(d, "job.json")
    if not os.path.exists(jf):
        print(n, "no job")
        continue
    done = json.load(open(jf)).get("status") in ("done", "rejected") and os.path.exists(os.path.join(d, "f_00.png"))
    if not done:
        print(n, "still running: left alone")
        continue
    dst = f"{d}.{tag}"
    k = 1
    while os.path.exists(dst):
        k += 1
        dst = f"{d}.{tag}{k}"
    os.rename(d, dst)
    print(n, "->", os.path.basename(dst))
