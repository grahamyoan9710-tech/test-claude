#!/bin/bash
# Downloads the clips PixelLab finished whose waiting process was stopped (job.json says submitted, no frames,
# and the job is no longer in the active list). Never resubmits anything.
cd "$(dirname "$0")/.."
python3 tools/pxcli.py jobs > /tmp/pxjobs_now.txt 2>&1
for d in pxa/p[0-9][0-9]/*/; do
  case $d in *.*/|*_test*) continue;; esac
  [ -f "$d/job.json" ] && [ ! -f "$d/f_00.png" ] || continue
  id=$(python3 -c "import json;print(json.load(open('$d/job.json')).get('job_id'))")
  [ "$id" = "None" ] && continue
  grep -q "$id" /tmp/pxjobs_now.txt && continue
  python3 tools/pxcli.py wait "$d" --timeout 60 2>&1 | tail -1 | cut -c1-90
done
