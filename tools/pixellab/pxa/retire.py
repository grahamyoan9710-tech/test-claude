#!/usr/bin/env python3
"""Sets finished clips aside so the driver makes them again (a new prompt): python3 retire.py TAG FIGHTER CLIP...
[--if-desc TEXT]: only a clip whose prompt starts with TEXT (an old prompt), never the remake.
pxa/<fighter>/<clip> becomes pxa/<fighter>/<clip>.<TAG>. A clip whose job is still running is left alone (never
resubmitted while PixelLab is working on it)."""
import json
import os
import sys

S = os.path.dirname(os.path.abspath(__file__))
args = sys.argv[1:]
only = None
if "--if-desc" in args:
    i = args.index("--if-desc")
    only = args[i + 1]
    del args[i:i + 2]
tag, fighter, names = args[0], args[1], args[2:]
for n in names:
    d = os.path.join(S, fighter, n)
    jf = os.path.join(d, "job.json")
    if not os.path.exists(jf):
        print(n, "no job")
        continue
    job = json.load(open(jf))
    if only is not None and not job.get("args", {}).get("description", "").startswith(only):
        print(n, "not the old prompt: left alone")
        continue
    done = job.get("status") in ("done", "rejected") and os.path.exists(os.path.join(d, "f_00.png"))
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
