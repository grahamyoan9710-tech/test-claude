#!/usr/bin/env bash
# Builds the Unity player (assets synced first) into unity/Builds/Linux or unity/Builds/Windows.
# Usage: scripts/unity_build.sh linux|windows        Unity binary: $UNITY (default below)
set -uo pipefail
cd "$(dirname "$0")/.."
UNITY=${UNITY:-$HOME/Unity/Hub/Editor/6000.6.9f1/Editor/Unity}
case "${1:-}" in
  linux) target=Linux ;;
  windows) target=Windows ;;
  *) echo "usage: $0 linux|windows" >&2; exit 2 ;;
esac
scripts/unity_sync_assets.sh || exit 1
mkdir -p unity/Logs
log=$PWD/unity/Logs/unity_build_$1.log
"$UNITY" -batchmode -nographics -quit -projectPath "$PWD/unity" -executeMethod "ClashHorizon.Editor.Build.$target" \
  -logFile "$log"
rc=$?
if [ "$rc" -eq 0 ]; then echo "built: unity/Builds/$target"; else
  grep -m 20 -E 'error CS|executeMethod|Build Finished, Result' "$log"; echo "Unity exited $rc, log: $log"; fi
exit "$rc"
