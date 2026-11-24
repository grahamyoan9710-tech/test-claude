#!/usr/bin/env bash
# Copies the game's assets into the Unity project (unity/Assets/Resources/{Sprites,Audio}, unity/Assets/Fonts).
# The copies are git-ignored: edit assets/, never the copies. rsync keeps file times, so Unity only reimports
# what changed, and it leaves the .meta files Unity writes next to each copy.
# Usage: scripts/unity_sync_assets.sh
set -euo pipefail
cd "$(dirname "$0")/.."
copy() { mkdir -p "$2" && rsync -a --delete --exclude='*.meta' "$1/" "$2/"; }
copy assets/sprites unity/Assets/Resources/Sprites
copy assets/audio unity/Assets/Resources/Audio
copy assets/fonts unity/Assets/Fonts
