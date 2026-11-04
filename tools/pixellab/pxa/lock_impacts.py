"""Puts back the impact frames chosen by the clip review (qa_fixes.json "impact") into assets/sprites/sheets.txt,
after scripts/fit_impacts.py --apply retimed the clips: python3 lock_impacts.py"""
import json, os
S = os.path.dirname(os.path.abspath(__file__))
idx = "/home/user/test-claude/assets/sprites/sheets.txt"
fixes = json.load(open(os.path.join(S, "qa_fixes.json")))
lines, n = open(idx).read().splitlines(), 0
for i, l in enumerate(lines):
    p = l.split()
    if len(p) >= 11 and p[0] == "sheet":
        imp = fixes.get(p[1], {}).get(p[2], {}).get("impact")
        if imp and p[10] != imp:
            p[10] = imp
            lines[i] = " ".join(p)
            n += 1
open(idx, "w").write("\n".join(lines) + "\n")
print(f"locked impacts: {n} restored")
