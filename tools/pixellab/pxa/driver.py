"""Keeps the clips of the PixelLab fighters coming: python3 driver.py FIGHTER [FIGHTER...]
Every minute, submits (through pxbatch.py) the clips whose base is ready and approved, until none is left.
A base clip (jump, runstart, drawsword...) feeds others only once listed in approved.txt ("fighter/clip"): a bad
base must not spoil every clip made from it. The guard picture is always approved."""
import importlib, json, os, subprocess, sys, time

S = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, S)
import clips as C  # noqa: E402

BATCH = os.path.join(os.path.dirname(S), "tools", "pxbatch.py")


def approved():
    p = os.path.join(S, "approved.txt")
    return {l.strip() for l in open(p)} if os.path.exists(p) else set()


def ready(fighter):
    importlib.reload(C)  # clips.py may have changed (a better prompt) while the driver runs
    ok = approved()
    out = []
    for j in C.jobs(fighter):
        name = os.path.basename(j["out"])
        base = C.clips(fighter)[name][1]
        root = base.split("@")[0]
        if root == "sword":
            root = "drawsword"
        if root != "guard" and f"{fighter}/{root}" not in ok:
            continue
        jf = os.path.join(j["out"], "job.json")
        if os.path.exists(jf) and json.load(open(jf)).get("status") != "rejected":
            continue
        out.append(j)
    first = ("drawsword", "jump", "runstart", "knockdown", "holdstart", "ledgegrab", "airsword")
    return sorted(out, key=lambda j: os.path.basename(j["out"]) not in first)  # the bases first: others wait on them


def main():
    fighters = sys.argv[1:]
    n, launched, procs = 0, set(), []
    while True:
        todo = [j for f in fighters for j in ready(f) if j["out"] not in launched]
        procs = [p for p in procs if p.poll() is None]
        if todo:  # each batch runs on its own: a new base does not wait for the clips before it
            n += 1
            path = os.path.join(S, f"jobs_driver_{n}.json")
            json.dump(todo, open(path, "w"), indent=1)
            launched |= {j["out"] for j in todo}
            print(time.strftime("%H:%M"), "batch", n, [os.path.relpath(j["out"], S) for j in todo], flush=True)
            procs.append(subprocess.Popen([sys.executable, BATCH, path, "--parallel", "6"],
                                          stdout=open(path[:-5] + ".log", "w"), stderr=subprocess.STDOUT))
        elif not procs:  # a rejected job comes back to ready() once its batch is over
            launched = {o for o in launched if not rejected(o)}
        time.sleep(60)


def rejected(out):
    jf = os.path.join(out, "job.json")
    return os.path.exists(jf) and json.load(open(jf)).get("status") == "rejected"


if __name__ == "__main__":
    main()
