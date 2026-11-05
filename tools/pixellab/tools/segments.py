"""Runs of Naruto's states in a trace: 'T frame state move moveFrame' (state 0 = Idle, 7 = Attack)."""
import sys
rows = [tuple(int(v) for v in l.split()[1:]) for l in open(sys.argv[1]) if l.startswith("T ") and len(l.split()) == 5]
names = {0: 'Jab', 1: 'Jab2', 2: 'Jab3', 3: 'FTilt'}
runs, cur = [], None
for fr, st, mv, mf in rows:
    key = ('idle',) if st == 0 else (('atk', names.get(mv, mv)) if st == 7 else ('other', st))
    if cur and cur[0] == key and fr == cur[2] + 1:
        cur[2] = fr
    else:
        if cur: runs.append(cur)
        cur = [key, fr, fr]
if cur: runs.append(cur)
for key, a, b in runs:
    if key[0] == 'idle' and b - a >= 15 or key[0] == 'atk' and key[1] in ('Jab', 'Jab2', 'FTilt', 'Jab3'):
        print(key, a, b, b - a + 1)
