"""Puts back the impact frames chosen by the clip review (qa_fixes.json "impact") into assets/sprites/sheets.txt,
after scripts/fit_impacts.py --apply retimed the clips: python3 lock_impacts.py

A blade attack joined from two parts (join.py) has its blow on the first frame of its second part, the end pose of
the cut: every blow of the move holds that pose and the frames after it (the second part holds it three frames).
fit_impacts.py would pick the wind-up instead, where the drawn blade meets the start of the swing."""
import json, os
S = os.path.dirname(os.path.abspath(__file__))
idx = "/home/user/test-claude/assets/sprites/sheets.txt"
fixes = json.load(open(os.path.join(S, "qa_fixes.json")))
AS = {"extra1": "rengaine", "extra2": "coupeeclair", "extra3": "coupeeclair"}  # import_all.SHEET_AS, the other way


def joined(fighter, sheet, imp):
    """The impacts of a joined clip: its cut frame (the clip's f_00 is left out of the sheet), then the next ones."""
    clip = sheet[:3] + AS.get(sheet[3:], sheet[3:]) if sheet[:1] == "w" and sheet[2:3] == "_" else AS.get(sheet, sheet)
    job = os.path.join(S, fighter, clip, "job.json")
    if imp == "-1" or not os.path.exists(job):
        return None
    j = json.load(open(job))
    if j.get("tool") != "join":
        return None
    return ",".join(str(j["cut_after"] + i) for i in range(len(imp.split(","))))


lines, n = open(idx).read().splitlines(), 0
for i, l in enumerate(lines):
    p = l.split()
    if len(p) >= 11 and p[0] == "sheet":
        imp = fixes.get(p[1], {}).get(p[2], {}).get("impact") or joined(p[1], p[2], p[10])
        if imp and p[10] != imp:
            p[10] = imp
            lines[i] = " ".join(p)
            n += 1
open(idx, "w").write("\n".join(lines) + "\n")
print(f"locked impacts: {n} restored")
