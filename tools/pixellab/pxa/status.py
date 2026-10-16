"""python3 status.py: for each of the 14 fighters, clips done / planned, and what is waiting (running, not started)."""
import json, os, clips as C
FS = "asta yuno ichigo byakuya kenpachi ulquiorra grimmjow starrk gojo sukuna link ganondorf kratos hunter".split()
S = os.path.dirname(os.path.abspath(__file__))
tot_d = tot = 0
for f in FS:
    cl = C.clips(f)
    done, run, wait, bad = [], [], [], []
    for name in cl:
        jf = os.path.join(S, f, name, 'job.json')
        st = json.load(open(jf)).get('status') if os.path.exists(jf) else None
        (done if st == 'done' else run if st in ('submitted', 'submitting', 'processing') else bad if st in ('failed', 'rejected', 'error') else wait).append(name)
    tot_d += len(done); tot += len(cl)
    print(f"{f:<10} {len(done):>2}/{len(cl):<2} running {len(run):>2}  waiting {len(wait):>2}" + (f"  FAILED {bad}" if bad else ""))
print(f"total {tot_d}/{tot}")
