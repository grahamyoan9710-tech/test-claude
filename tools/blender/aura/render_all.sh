#!/bin/bash
# render_all.sh <recipe>... : Blender render + post (glow, straight alpha) of each recipe, front layers too
S=${SCRATCH:?set SCRATCH: the folder holding bpyenv/ and aura/}
cd "$(dirname "$0")/.."
for r in "$@"; do
  rm -rf $S/aura/bl/$r $S/aura/bl/${r}_front $S/aura/png/$r $S/aura/png/${r}_front
  $S/bpyenv/bin/python effets.py -- $r $S/aura/bl/$r 24 2>&1 | grep "FXLINE" >> $S/aura/fxlines.txt
  for d in $r ${r}_front; do
    [ -d $S/aura/bl/$d ] && python3 post2.py $S/aura/bl/$d $S/aura/png/$d 0.8 0.6 > /dev/null
  done
  echo done $r
done
