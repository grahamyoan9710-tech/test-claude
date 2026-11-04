#!/usr/bin/env python3
"""Runs many PixelLab animation/image jobs, at most N at a time (PixelLab Tier 2: 10 background jobs).

    pxbatch.py JOBS.json [--parallel 8]

JOBS.json: a list of {"out": DIR, "kind": "anim"|"image", ...pxcli options...}:
  anim:  {"out", "src", "desc", "frames", "engine", "last", "subject", "pose", "direction", "seed"}
  image: {"out", "desc", "w", "h", "direction", "view", "init", "strength"}
Each job goes through pxcli.py, so a directory that already has a job.json is never submitted twice: running the
batch again only waits for / downloads what is missing. Prints one line per job and a summary.
"""
import json, os, subprocess, sys, threading
from concurrent.futures import ThreadPoolExecutor

CLI = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'pxcli.py')


def run(job):
    out = job['out']
    jf = os.path.join(out, 'job.json')
    if os.path.exists(jf) and json.load(open(jf)).get('status') == 'done' and os.path.exists(os.path.join(out, 'f_00.png')):
        return out, 'done (cached)'
    cmd = [sys.executable, CLI, job.get('kind', 'anim'), out, '--desc', job['desc']]
    if job.get('kind', 'anim') == 'anim':
        cmd += ['--src', job['src'], '--frames', str(job.get('frames', 8)), '--engine', job.get('engine', 'mm')]
        for k in ('last', 'subject', 'pose', 'direction'):
            if job.get(k):
                cmd += ['--' + k, job[k]]
        if job.get('seed') is not None:
            cmd += ['--seed', str(job['seed'])]
    else:
        cmd += ['--w', str(job.get('w', 128)), '--h', str(job.get('h', 64))]
        for k in ('direction', 'view', 'init', 'model', 'outline', 'detail'):
            if job.get(k):
                cmd += ['--' + k, job[k]]
        if job.get('strength'):
            cmd += ['--strength', str(job['strength'])]
    r = subprocess.run(cmd, capture_output=True, text=True)
    tail = (r.stdout.strip().splitlines() or [''])[-1] + (' ERR ' + r.stderr.strip()[-300:] if r.returncode else '')
    return out, tail


def main():
    jobs = json.load(open(sys.argv[1]))
    par = int(sys.argv[sys.argv.index('--parallel') + 1]) if '--parallel' in sys.argv else 8
    lock = threading.Lock()
    done = []
    with ThreadPoolExecutor(max_workers=par) as ex:
        for out, msg in ex.map(run, jobs):
            with lock:
                done.append((out, msg))
                print(f'[{len(done)}/{len(jobs)}] {out}: {msg}', flush=True)
    bad = [o for o, m in done if 'ERR' in m or 'FAILED' in m or 'still processing' in m]
    print('summary:', len(jobs) - len(bad), 'ok,', len(bad), 'to look at:', *bad, sep=' ')


if __name__ == '__main__':
    main()
