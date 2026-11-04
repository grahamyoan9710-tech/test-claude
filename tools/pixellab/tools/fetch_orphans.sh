#!/bin/bash
# Downloads every submitted job whose waiting process is gone (pxcli wait never resubmits).
S=$(cd "$(dirname "$0")/.." && pwd)
cd "$S"
python3 - <<'PY' | while read d; do python3 tools/pxcli.py wait "$d" --timeout 1500 > "$d/wait.log" 2>&1 & done; wait
import json,glob,subprocess
procs=subprocess.run(['ps','-eo','args'],capture_output=True,text=True).stdout
for jf in sorted(glob.glob('pxa/*/*/job.json')+glob.glob('m1/*/job.json')):
    d=json.load(open(jf))
    out=jf.rsplit('/',1)[0]
    if d.get('status')=='submitted' and (out+' ') not in procs:
        print(out)
PY
