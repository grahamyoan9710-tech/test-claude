#!/bin/bash
# Arena backdrops and fighter portraits with PixelLab's pro model (pxcli image waits for a free slot, keeps the job).
cd "$(dirname "$0")/../.."
OPEN="the lower third is open empty space with no ground, no floor, no bridge and no walkway, the middle stays empty for the fight; no people, no characters, no text; detailed anime pixel art"
bg() { python3 tools/pxcli.py image pxa/arenas/$1 --model pro --bg --w 688 --h 384 --style pxa/arenas/style_menu.png --desc "side view background for a 2D fighting game arena: $2; $OPEN" > pxa/arenas/$1.log 2>&1; }
pt() { s=$(python3 -c "import json;print(json.load(open('pxa/subjects14.json'))['$1'])")
  python3 tools/pxcli.py image pxa/portraits/$1 --model pro --w 168 --h 168 --ref "pxa/base/$1_guard256.png|character base: same face, hair and outfit" \
    --desc "head and shoulders bust portrait of $s, modest fully covered outfit, three-quarter view facing right, confident look, detailed anime pixel art, the same character as the reference, no border, no frame, nothing touching the edges" > pxa/portraits/$1.log 2>&1; }
bg lunaire_bg "a vast starry violet night sky with a huge pale full moon, distant floating ruined shrines and red torii silhouettes on drifting rocks, violet mist far below" &
bg givre_bg "a snowy mountain fortress with white walls and towers with red roofs under a starry twilight sky with a soft blue aurora, light snowfall, a frozen valley far below" &
bg ecarlate_bg "a deep red sandstone canyon at sunset, glowing paper lanterns strung on ropes across the cliffs, embers drifting in the wind, a deep gorge far below" &
bg kurenai_bg "the tiled rooftops of an old Japanese town at night in light rain, red paper lanterns and crimson shop banners with a white crown emblem, a distant pagoda and a red moon" &
for p in p01 p02 p07 p08; do pt $p & done
wait
