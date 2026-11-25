#!/usr/bin/env bash
# Runs the Unity EditMode tests from the command line (assets synced first). Exit 0 when all pass.
# Usage: scripts/unity_test.sh        Unity binary: $UNITY (default below)
set -uo pipefail
cd "$(dirname "$0")/.."
UNITY=${UNITY:-$HOME/Unity/Hub/Editor/6000.6.9f1/Editor/Unity}
scripts/unity_sync_assets.sh || exit 1
scripts/unity_parity.sh > /dev/null || exit 1
mkdir -p unity/Logs
results=$PWD/unity/Logs/test-results.xml log=$PWD/unity/Logs/unity_test.log
rm -f "$results"
"$UNITY" -batchmode -nographics -projectPath "$PWD/unity" -runTests -testPlatform EditMode \
  -testResults "$results" -logFile "$log"
rc=$?
[ -f "$results" ] && python3 - "$results" <<'PY'
import sys, xml.etree.ElementTree as ET
run = ET.parse(sys.argv[1]).getroot()
print(f"tests: {run.get('passed')} passed, {run.get('failed')} failed, {run.get('total')} in all")
for case in run.iter('test-case'):
    if case.get('result') == 'Failed':
        print('FAILED', case.get('fullname') + ':', (case.findtext('failure/message') or '').strip())
PY
if [ "$rc" -ne 0 ]; then grep -m 20 'error CS' "$log"; echo "Unity exited $rc, log: $log"; fi
exit "$rc"
