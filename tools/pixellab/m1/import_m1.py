"""Imports the finished weapon M1 clips of the rig fighters: python3 import_m1.py [NAME_wS ...]
Each m1/<fighter>_w<slot> folder becomes the clip w<slot>_jab of that fighter, anchored like its first frame
(--export-pose, m1src/anchors.json). Flat grey slabs are erased first (tools/greyfix.py)."""
import json, os, subprocess, sys
import numpy as np
from PIL import Image

S = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(S)
REPO = "/home/user/test-claude"
anchors = json.load(open(os.path.join(ROOT, "m1src", "anchors.json")))
sys.path.insert(0, os.path.join(ROOT, "pxa"))
import import_all as ia  # noqa: E402  (its review-fix helpers)
QA_FIXES = ia.QA_FIXES


def impact(d, ax, pxh):
    """The frame with the most drawn pixels in front of the body: the slash, the shot, the burst."""
    fs = sorted(f for f in os.listdir(d) if f.startswith("f_") and f.endswith(".png"))[1:]
    best, bi = -1, -1
    for i, f in enumerate(fs):
        a = np.asarray(Image.open(os.path.join(d, f)).convert("RGBA"))[..., 3] > 0
        n = int((np.nonzero(a)[1] > ax + 0.22 * pxh).sum())
        if n > best:
            best, bi = n, i
    return bi


only = set(sys.argv[1:])
for key, (ax, ay, pxh) in anchors.items():
    if only and key not in only:
        continue
    d = os.path.join(S, key)
    jf = os.path.join(d, "job.json")
    if not os.path.exists(jf) or json.load(open(jf)).get("status") != "done" or not os.path.exists(os.path.join(d, "f_01.png")):
        print("missing", key)
        continue
    subprocess.run(["python3", os.path.join(ROOT, "tools", "greyfix.py"), d, "--fix"], check=True)
    fighter, slot = key.rsplit("_", 1)
    args = ["python3", os.path.join(REPO, "scripts", "import_clip.py"), fighter, f"{slot}_jab", d,
            "--anchor", str(ax), str(ay), "--pxh", str(pxh), "--impact", str(impact(d, ax, pxh))]
    fx = QA_FIXES.get(fighter, {}).get(f"{slot}_jab", {})  # fixes of the clip review (pxa/qa_fixes.json)
    if fx.get("impact"):
        args[args.index("--impact") + 1] = fx["impact"]
    if fx.get("replace") or fx.get("degrey") or fx.get("trim_right"):
        args[4] = ia.with_replaced(d, fx.get("replace", {}), [])
        if fx.get("trim_right"):
            ia.trim_right(args[4], fx["trim_right"])
        if fx.get("degrey"):
            ia.degrey(args[4], fx["degrey"], [])
    r = subprocess.run(args, capture_output=True, text=True)
    print(r.stdout.strip() or r.stderr.strip()[-300:])
