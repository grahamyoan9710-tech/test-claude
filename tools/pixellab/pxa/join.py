#!/usr/bin/env python3
"""A blade attack made of two clean parts (owner 17/11: no swing trail in the sprites, the cut is the Blender effect).

    python3 join.py FIGHTER CLIP   -> pxa/<fighter>/<clip>/ = <clip>.a (the slow wind-up) + <clip>.b (from the end
                                      of the cut back to the stance), the cut itself being the jump between them
    python3 join.py FIGHTER CLIP --back -> first makes <clip>.b = <clip>.a played backwards (the Fente écarlate:
                                      the sheathe, the click, then the same blade path out of the scabbard)

PixelLab draws a white trail on every fast swing frame, whatever the prompt. So the attack is never generated in one
go: <clip>.a winds up slowly and holds, <clip>.p moves slowly into the end pose of the cut (only its last frame is
used, as the start of <clip>.b), <clip>.b holds that pose and slowly returns to the stance. Every frame is generated,
none is touched. f_00 stays the input frame (import_clip.py leaves it out).
"""
import json
import os
import shutil
import sys

S = os.path.dirname(os.path.abspath(__file__))


def frames(d):
    return sorted(f for f in os.listdir(d) if f.startswith("f_") and f.endswith(".png"))


def back(fighter, clip):
    """<clip>.b = the frames of <clip>.a in reverse order: f_00 is the last frame of .a, the last is its f_00."""
    a, b = (os.path.join(S, fighter, f"{clip}.{p}") for p in "ab")
    if os.path.exists(b):
        raise SystemExit(f"{b} exists: set it aside first")
    os.makedirs(b)
    fs = frames(a)
    for i, f in enumerate(reversed(fs)):
        shutil.copyfile(os.path.join(a, f), os.path.join(b, f"f_{i:02d}.png"))
    json.dump({"status": "done", "tool": "back", "of": f"{clip}.a"}, open(os.path.join(b, "job.json"), "w"), indent=1)


def join(fighter, clip):
    a, b = (os.path.join(S, fighter, f"{clip}.{p}") for p in "ab")
    for d in (a, b):
        if json.load(open(os.path.join(d, "job.json"))).get("status") != "done":
            raise SystemExit(f"{d} is not done")
    out = os.path.join(S, fighter, clip)
    if os.path.exists(out):
        raise SystemExit(f"{out} exists: set it aside first")
    os.makedirs(out)
    src = [os.path.join(a, f) for f in frames(a)] + [os.path.join(b, f) for f in frames(b)]
    for i, f in enumerate(src):
        shutil.copyfile(f, os.path.join(out, f"f_{i:02d}.png"))
    json.dump({"status": "done", "tool": "join", "parts": [f"{clip}.a", f"{clip}.b"], "cut_after": len(frames(a)) - 1},
              open(os.path.join(out, "job.json"), "w"), indent=1)
    assert frames(out) == [f"f_{i:02d}.png" for i in range(len(src))]
    return len(src)


if __name__ == "__main__":
    if "--back" in sys.argv:
        back(sys.argv[1], sys.argv[2])
    print(sys.argv[2], join(sys.argv[1], sys.argv[2]), "frames")
