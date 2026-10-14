"""Every 2 minutes, starts `pxcli.py wait` for each submitted job nobody is waiting for (wait never resubmits)."""
import glob, json, os, subprocess, sys, time
S = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(S)
started = {}
while True:
    procs = subprocess.run(['ps', '-eo', 'args'], capture_output=True, text=True).stdout
    for jf in sorted(glob.glob('pxa/*/*/job.json') + glob.glob('m1/*/job.json')):
        try:
            d = json.load(open(jf))
        except ValueError:
            continue
        out = os.path.abspath(jf.rsplit('/', 1)[0])
        if d.get('status') != 'submitted' or (out + ' ') in procs or (out + '\n') in procs:
            continue
        if time.time() - started.get(out, 0) < 600:
            continue
        started[out] = time.time()
        print(time.strftime('%H:%M'), 'rescue', os.path.relpath(out, S), flush=True)
        subprocess.Popen([sys.executable, os.path.join(S, 'tools', 'pxcli.py'), 'wait', out, '--timeout', '1500'],
                         stdout=open(os.path.join(out, 'wait.log'), 'a'), stderr=subprocess.STDOUT)
    time.sleep(120)
