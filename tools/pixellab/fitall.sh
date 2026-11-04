#!/bin/bash
# Full fitting pass after clips were imported: impact frames, then hitboxes, then the check and the balance.
set -e
cd /home/user/test-claude
S=${S:-$(mktemp -d)}  # work files (TSVs, the fitted hitboxes)
cmake --build build -j8 2>&1 | grep -E "error:" || true
./build/sim_test balance | grep "win rate" > $S/bal_raw.txt
timeout 900 xvfb-run -a ./build/super_clash --fit-impacts $S/fit_imp.tsv > /dev/null 2>&1
python3 scripts/fit_impacts.py $S/fit_imp.tsv --apply | grep -E "retimed|applied"
python3 tools/pixellab/pxa/lock_impacts.py
cmake --build build -j8 2>&1 | grep -E "error:" || true
timeout 900 xvfb-run -a ./build/super_clash --fit-hitboxes $S/hitfit.inc 2>&1 | tail -1
cp $S/hitfit.inc src/core/roster_hitfit.inc
cmake --build build -j8 2>&1 | grep -E "error:" || true
timeout 900 xvfb-run -a ./build/super_clash --check-hitboxes $S/hc.tsv > /dev/null 2>&1
python3 scripts/check_hitboxes.py $S/hc.tsv | tail -1
./build/sim_test balance | grep "win rate" > $S/bal_fit.txt
paste $S/bal_raw.txt $S/bal_fit.txt | awk '{print $1, $4, "->", $8}'
(cd build && ctest 2>&1 | grep -E "tests passed|Failed")
