#!/bin/bash
# qa_prep.sh FIGHTER ROSTER_INDEX: qa14/FIGHTER/ with prompts.json, the clip sheets (clips_*.png) and the six in-game pages.
S=/tmp/claude-0/-home-user-test-claude/454a74a0-0344-5f0d-8ed7-e18843be178a/scratchpad
f=$1; ci=$2; out=$S/qa14/$f
mkdir -p $out; rm -f $out/clips_*.png $out/page_*.png
python3 - "$f" "$out" <<'PY'
import json, os, sys
S = "/tmp/claude-0/-home-user-test-claude/454a74a0-0344-5f0d-8ed7-e18843be178a/scratchpad/pxa"
sys.path.insert(0, S)
from clips import clips
f, out = sys.argv[1], sys.argv[2]
d = {}
for name, (n, base, pin, loop, direction, desc) in clips(f).items():
    jf = os.path.join(S, f, name, "job.json")
    p = desc
    if os.path.exists(jf):
        p = json.load(open(jf)).get("args", {}).get("description", desc)
    d[name] = {"frames": n, "base": base, "direction": direction, "prompt": p}
json.dump(d, open(os.path.join(out, "prompts.json"), "w"), indent=1)
PY
python3 $S/qa_clips.py $f $out
cd /home/user/test-claude/build
for p in 0 1 2 3 4 5; do timeout 90 xvfb-run -a ./super_clash --screen anim --tab $((ci*6+p)) --shot 20 $out/page_$p.png --quit-after 25 >/dev/null 2>&1; done
ls $out
