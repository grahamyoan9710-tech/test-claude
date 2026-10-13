#!/usr/bin/env bash
# Builds ready-to-run archives of the game in dist/:
#   SuperClashBros-<version>-linux-x64.tar.gz   (needs cmake, g++ and the X11/GL dev packages)
#   SuperClashBros-<version>-windows-x64.zip    (cross-compiled, needs the mingw-w64 package)
# Usage: scripts/package_release.sh [version]
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DIST="$ROOT/dist"
VERSION="${1:-8.1}"
rm -rf "$DIST"
mkdir -p "$DIST"

build() {  # build dir, extra cmake arguments...
  local dir="$1"
  shift
  cmake -S "$ROOT" -B "$dir" -DCMAKE_BUILD_TYPE=Release "$@" > /dev/null
  cmake --build "$dir" -j"$(nproc)" --target super_clash
}

stage() {  # platform, executable, readme
  local out="$DIST/$1/SuperClashBros"
  mkdir -p "$out/licences"
  cp "$2" "$out/"
  cp "$3" "$out/LISEZ-MOI.txt"
  cp "$ROOT/assets/fonts/LuckiestGuy-LICENSE.txt" "$ROOT/assets/fonts/Poppins-OFL.txt" \
    "$ROOT/assets/gamecontrollerdb-LICENSE.txt" "$out/licences/"
}

# Linux: static C++ runtime, X11/OpenGL/audio libraries are loaded at run time.
build "$ROOT/build-release-linux" -DCLASH_PORTABLE=ON
strip "$ROOT/build-release-linux/super_clash"
stage linux "$ROOT/build-release-linux/super_clash" "$ROOT/packaging/LISEZ-MOI-linux.txt"
tar -C "$DIST/linux" -czf "$DIST/SuperClashBros-$VERSION-linux-x64.tar.gz" SuperClashBros

# Windows: one self-contained .exe.
if command -v x86_64-w64-mingw32-g++ > /dev/null; then
  build "$ROOT/build-release-windows" -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/mingw-w64.cmake"
  x86_64-w64-mingw32-strip "$ROOT/build-release-windows/super_clash.exe"
  stage windows "$ROOT/build-release-windows/super_clash.exe" "$ROOT/packaging/LISEZ-MOI-windows.txt"
  sed -i 's/$/\r/' "$DIST/windows/SuperClashBros/LISEZ-MOI.txt"  # Windows line endings
  (cd "$DIST/windows" && python3 -c "
import shutil, sys
shutil.make_archive(sys.argv[1], 'zip', '.', 'SuperClashBros')" "$DIST/SuperClashBros-$VERSION-windows-x64")
else
  echo "mingw-w64 not found: Windows archive skipped" >&2
fi
ls -la "$DIST"
