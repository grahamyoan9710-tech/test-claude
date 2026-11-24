#!/usr/bin/env bash
# Writes the C++ traces and data dumps the Unity ParityTests compare against: unity/Logs/parity/<name>.txt.
# Builds build-core (no raylib) first when needed. Usage: scripts/unity_parity.sh
set -euo pipefail
cd "$(dirname "$0")/.."
log=build-core/parity_build.log
mkdir -p build-core
[ -f build-core/CMakeCache.txt ] || cmake -S . -B build-core -DCLASH_BUILD_GAME=OFF > "$log" 2>&1 || { tail -30 "$log"; exit 1; }
cmake --build build-core -j --target sim_test > "$log" 2>&1 || { tail -30 "$log"; exit 1; }
out=unity/Logs/parity
mkdir -p "$out"
for name in jab_hit smash_ko projectile weapon_pickup_switch ult_capture hakuen_rift moving_platform cpu_vs_cpu_600 \
            stages roster; do
  ./build-core/sim_test trace "$name" > "$out/$name.txt.tmp" || { echo "trace $name failed"; exit 1; }
  mv "$out/$name.txt.tmp" "$out/$name.txt"
  printf '%-22s %5d lines\n' "$name" "$(wc -l < "$out/$name.txt")"
done
