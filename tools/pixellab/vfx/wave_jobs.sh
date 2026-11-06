#!/bin/bash
# Submits (and waits for) the burst and fade clips of the waves given: bash wave_jobs.sh 2 3 4
cd "$(dirname "$0")/.."
for n in "$@"; do
  python3 tools/pxcli.py anim vfx/waves_anim_mm/w${n}_burst --src vfx/waves_src/w${n}_seed.png --last vfx/waves_src/w${n}_full.png --frames 12 --desc "a shock wave of white energy bursts up from the ground and grows to its full size: everything rises and spreads out from the ground, small shards fly out. Only the effect is drawn, in the same greyscale pixel art style, on a transparent background." &
  python3 tools/pxcli.py anim vfx/waves_anim_mm/w${n}_fade --src vfx/waves_src/w${n}_full.png --frames 12 --desc "the white energy shock wave keeps moving for a moment, then the whole effect dissipates upward into small sparkles and fades away completely. Only the effect is drawn, in the same greyscale pixel art style, on a transparent background." &
done
wait
