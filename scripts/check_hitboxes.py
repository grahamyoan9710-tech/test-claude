#!/usr/bin/env python3
"""Sums up the hitbox check: python3 scripts/check_hitboxes.py CHECK.tsv [--fighter NAME] [--all]

CHECK.tsv comes from `super_clash --check-hitboxes CHECK.tsv`: one line per hitbox and move frame of every move drawn
by a clip. A blow (consecutive frames where a hitbox is out) is flagged when its hitbox covers almost no drawn
pixel (coverage < 10% on most of its frames: it hits where nothing is drawn) or lies far from the drawing (gap, in
world units: a fighter is about 100 tall). Prints the flagged blows, worst first (--all: every blow).
"""
import argparse
import csv
from collections import defaultdict


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("tsv")
    ap.add_argument("--fighter", default=None)
    ap.add_argument("--all", action="store_true")
    a = ap.parse_args()
    rows = list(csv.DictReader(open(a.tsv), delimiter="\t"))
    blows = defaultdict(list)  # (fighter, slot, move, box) -> rows
    for r in rows:
        if a.fighter and r["fighter"].lower() != a.fighter.lower():
            continue
        blows[(r["fighter"], int(r["slot"]), r["move"], int(r["box"]))].append(r)
    report = []
    for (f, slot, move, box), rs in blows.items():
        rs.sort(key=lambda r: int(r["frame"]))
        cov = [float(r["coverage"]) for r in rs]
        gaps = [float(r["gap"]) for r in rs]
        empty = sum(c < 0.1 for c in cov) / len(cov)
        clipframes = sorted({int(r["clip_frame"]) for r in rs})
        worst = max(gaps)
        score = empty * (1 + worst / 20.0)
        name = ("" if slot < 0 else f"w{slot}_") + move
        report.append((score, f, name, box, rs[0]["frame"], rs[-1]["frame"], rs[0]["duration"], sum(cov) / len(cov),
                       empty, worst, clipframes, rs[0]["cx"], rs[0]["cy"], rs[0]["radius"]))
    report.sort(key=lambda x: -x[0])
    print("fighter      move              box frames     cover  empty  gap   clip frames      at (cx,cy) r")
    for (score, f, name, box, a0, a1, dur, cov, empty, gap, cf, cx, cy, rad) in report:
        if not a.all and empty < 0.5:
            continue
        print(f"{f:<12} {name:<17} {box:>3} {a0:>3}-{a1:<3}/{dur:<4} {cov:5.2f}  {empty:5.2f} {gap:5.1f}  "
              f"{','.join(map(str, cf)):<15}  ({cx},{cy}) {rad}")
    flagged = sum(1 for r in report if r[8] >= 0.5)
    print(f"{flagged} of {len(report)} hitboxes hit mostly where nothing is drawn")


if __name__ == "__main__":
    main()
