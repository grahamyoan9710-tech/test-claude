#!/bin/bash
# Copies the live PixelLab pipeline state (this scratchpad) into the repo's tools/pixellab, without keys or raw frames.
S=/tmp/claude-0/-home-user-test-claude/454a74a0-0344-5f0d-8ed7-e18843be178a/scratchpad
D=/home/user/test-claude/tools/pixellab
cd $S
mkdir -p $D/{tools,pxa,m1,vfx/ult,qa14,m1src}
cp tools/*.py tools/fetch_orphans.sh $D/tools/ 2>/dev/null
cp pxa/{clips,driver,import_all,status,lock_impacts}.py pxa/watch_bases.sh pxa/{qa_prompts,qa_fixes,anchors14,subjects14}.json pxa/approved.txt $D/pxa/
cp -r pxa/specs_final pxa/base pxa/base14 $D/pxa/
cp m1/import_m1.py m1/jobs_m1.json $D/m1/; cp m1src/*.png m1src/anchors.json $D/m1src/
cp vfx/BRIEF.md $D/vfx/; cp vfx/ult/*.json vfx/ult/*.png $D/vfx/ult/ 2>/dev/null
cp qa14/qa_fighter.js $D/qa14/; for f in qa14/*/defects*.json; do mkdir -p $D/$(dirname $f); cp $f $D/$f; done
cp qa_strip.py qa_clips.py qa_prep.sh qa_bases.py sheetstrip.py vfxsheet.py recult.sh fitall.sh sync_repo.sh $D/ 2>/dev/null
for d in pxa/*/; do f=$(basename $d); for c in jump runstart knockdown holdstart ledgegrab w0guard w1guard drawsword airsword; do
  [ -f $d$c/job.json ] && { mkdir -p $D/pxa/$f/$c; cp $d$c/f_*.png $d$c/job.json $D/pxa/$f/$c/ 2>/dev/null; }; done; done
python3 - $D <<'PY'
import glob, json, os, sys
out = {}
for jf in sorted(glob.glob('pxa/*/*/job.json')):
    f, c = jf.split('/')[1:3]
    try: j = json.load(open(jf))
    except Exception: continue
    out.setdefault(f, {})[c] = {'status': j.get('status'), 'job_id': j.get('job_id')}
json.dump(out, open(os.path.join(sys.argv[1], 'pxa', 'job_status.json'), 'w'), indent=1, sort_keys=True)
PY
python3 pxa/status.py > $D/pxa/status_snapshot.txt
find $D -name '.token' -o -name '.key' | grep . && echo "SECRET FOUND - aborting" && exit 1
tail -1 $D/pxa/status_snapshot.txt
