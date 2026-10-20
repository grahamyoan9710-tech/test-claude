#!/usr/bin/env python3
"""Times every clip's blows on its hitboxes: python3 scripts/fit_impacts.py FIT.tsv [--apply] [--min 0.15]

FIT.tsv comes from `super_clash --fit-impacts FIT.tsv`: for each blow of each clip-drawn move, the share of its
hitboxes each frame of the clip covers. For each clip this picks one frame per blow, in increasing order, so that
the drawing covers the hitboxes best (dynamic programming), and prints it next to the impact frames sheets.txt has.
--apply writes them into assets/sprites/sheets.txt. A blow no frame covers well (best below --min) is reported:
there the hitbox itself is not where the clip draws the blow (move it, or redraw the clip).
"""
import argparse
import csv
import os
from collections import defaultdict


def best_frames(cov):
    """cov[b][j]: coverage of blow b by frame j. Increasing frames, one per blow, maximising the sum."""
    B, F = len(cov), len(cov[0])
    if B > F:
        cov, B = cov[:F], F
    NEG = -1e9
    dp = [[NEG] * F for _ in range(B)]
    back = [[-1] * F for _ in range(B)]
    for j in range(F):
        dp[0][j] = cov[0][j]
    for b in range(1, B):
        best, arg = NEG, -1
        for j in range(F):
            if j - 1 >= 0 and dp[b - 1][j - 1] > best:
                best, arg = dp[b - 1][j - 1], j - 1
            if best > NEG:
                dp[b][j] = best + cov[b][j]
                back[b][j] = arg
    j = max(range(F), key=lambda k: dp[B - 1][k])
    out = [j]
    for b in range(B - 1, 0, -1):
        j = back[b][j]
        out.append(j)
    return out[::-1]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("tsv")
    ap.add_argument("--apply", action="store_true")
    ap.add_argument("--min", type=float, default=0.15)
    a = ap.parse_args()
    clips = defaultdict(list)
    for r in csv.DictReader(open(a.tsv), delimiter="\t"):
        cov = [float(v) for v in r["coverage_per_frame"].split(",")]
        clips[(r["fighter"].lower(), r["clip"])].append((int(r["blow"]), int(r["start"]), int(r["end"]), cov))
    root = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
    index = os.path.join(root, "assets", "sprites", "sheets.txt")
    lines = open(index).read().splitlines()
    current = {(l.split()[1], l.split()[2]): l.split()[10] for l in lines if l.startswith("sheet ")}
    fitted, weak = {}, []
    for key, blows in sorted(clips.items()):
        blows.sort()
        cov = [b[3] for b in blows]
        frames = best_frames(cov)
        bad = [(bi, s, e, j, c[j], max(c)) for (bi, s, e, c), j in zip(blows, frames) if c[j] < a.min]
        if bad:  # the drawing does not reach this hitbox: keep the frames chosen by eye, move the hitbox instead
            weak += [(key,) + b for b in bad]
            continue
        fitted[key] = ",".join(map(str, frames))
    changed = 0
    for key, imp in fitted.items():
        old = current.get(key)
        if old != imp:
            changed += 1
            print(f"{key[0]:<10} {key[1]:<14} impact {old} -> {imp}")
    print(f"{changed} clips retimed of {len(fitted)}")
    if weak:
        print("\nblows no frame covers well (the hitbox is not where the clip draws the blow):")
        for (f, clip), bi, s, e, j, c, mx in weak:
            print(f"  {f:<10} {clip:<14} blow {bi} (frames {s}-{e}): best frame {j} covers {c:.2f}")
    if a.apply:
        out = []
        for l in lines:
            p = l.split()
            if l.startswith("sheet ") and (p[1], p[2]) in fitted:
                p[10] = fitted[(p[1], p[2])]
                l = " ".join(p)
            out.append(l)
        open(index, "w").write("\n".join(out) + "\n")
        print("applied to", index)


if __name__ == "__main__":
    main()
