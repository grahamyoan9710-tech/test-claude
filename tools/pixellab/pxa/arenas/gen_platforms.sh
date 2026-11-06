#!/bin/bash
# One picture per platform of the five arenas, at the game's size (half resolution: imported x2), in each backdrop's
# style: seen slightly from above as on a Super Smash Bros. stage (its top floor visible in depth), finished at both
# ends. The flat versions (flat_<name>.png) are the reference for the look (scripts/import_art.py imports them).
cd "$(dirname "$0")/../.."
plat() {  # name, width, height, what it is
  python3 tools/pxcli.py image pxa/arenas/$1_depth --model pro --w $2 --h $3 --style pxa/arenas/${1%%_*}_bg/f_00.png \
    --ref "pxa/arenas/flat_$1.png|the platform's look to keep: its materials, colours and details" \
    --desc "one single floating platform for a 2D fighting game, seen from the front and slightly from above like a Super Smash Bros. stage: its flat top floor is clearly visible as a wide band going back in depth, above its front side; $4. Both of its ends are complete and finished, nothing is cut by the edges of the picture; transparent background, nothing else, no characters, no text; detailed anime pixel art" \
    > pxa/arenas/$1_depth.log 2>&1
}
[ -z "$1" ] && {
plat lunaire_solid 448 184 "a long bridge of dark violet stone with a carved rim and a rocky underside with a few hanging roots" &
plat givre_solid 392 208 "a white castle rampart covered in snow, a row of red roof tiles along its front edge, icicles below, on snowy rocks" &
plat givre_thin 112 56 "a short snowy wooden beam with small icicles under it" &
plat ecarlate_solid 176 144 "a red sandstone rock ledge with two small paper lanterns hanging under it" &
plat ecarlate_thin 104 56 "a short wooden plank bridge section, its planks bound with rope at both ends" &
plat kurenai_solid 496 192 "the long dark tiled roof of a Japanese house, its flat top walkway along the ridge, with red paper lanterns hanging under the eaves" &
plat kurenai_thin 104 56 "a short dark wooden beam with iron bands" &
plat nuees_solid 312 192 "a grey stone floor with a red lacquered wooden edge and a low red railing at each end, on a floating rock" &
plat nuees_small 112 120 "a small floating rock island with a flat grey stone top and a red lacquered wooden edge" &
plat nuees_thin 112 88 "a short red lacquered wooden plank on top of a small floating rock" &
wait
}

# Second pass for the pictures that touched the edges of their canvas (cut): a bigger canvas and empty space around.
plat2() {  # name, width, height, what it is
  python3 tools/pxcli.py image pxa/arenas/$1_depth2 --model pro --w $2 --h $3 --style pxa/arenas/${1%%_*}_bg/f_00.png \
    --ref "pxa/arenas/flat_$1.png|the platform's look to keep: its materials, colours and details" \
    --desc "one single floating platform for a 2D fighting game, small in the middle of the picture with empty transparent space all around it (nothing touches any edge of the picture), seen from the front and slightly from above like a Super Smash Bros. stage: its flat top floor is clearly visible as a wide band going back in depth, above its front side; $4. Both of its ends are complete and finished; transparent background, nothing else, no characters, no text; detailed anime pixel art" \
    > pxa/arenas/$1_depth2.log 2>&1
}
if [ "$1" = second ]; then
  plat2 lunaire_solid 480 240 "a long bridge of dark violet stone with a carved rim and a rocky underside with a few short hanging roots" &
  plat2 ecarlate_solid 224 176 "a red sandstone rock ledge with two small paper lanterns hanging under it" &
  plat2 kurenai_solid 576 240 "the long dark tiled roof of a Japanese house, its flat top walkway along the ridge, with red paper lanterns hanging under the eaves" &
  plat2 nuees_small 144 144 "a small floating rock island with a flat grey stone top and a red lacquered wooden edge" &
  plat2 nuees_thin 144 112 "a short red lacquered wooden plank on top of a small floating rock" &
  wait
fi

# Third pass: the moving plank of the Nuées drew a sky behind it, the Lunar bridge came out too small.
plat3() {  # name, width, height, what it is
  python3 tools/pxcli.py image pxa/arenas/$1_depth3 --model pro --w $2 --h $3 --style pxa/arenas/${1%%_*}_bg/f_00.png \
    --ref "pxa/arenas/flat_$1.png|the platform's look to keep: its materials, colours and details" \
    --desc "a game sprite on a fully transparent background, no sky, no clouds, no scenery: one single floating platform for a 2D fighting game, filling most of the picture's width with a few pixels of empty space on every side, seen from the front and slightly from above like a Super Smash Bros. stage: its flat top floor is clearly visible as a wide band going back in depth, above its front side; $4. Both of its ends are complete and finished; detailed anime pixel art" \
    > pxa/arenas/$1_depth3.log 2>&1
}
if [ "$1" = third ]; then
  plat3 lunaire_solid 480 200 "a long bridge of dark violet stone with a carved rim and a rocky underside whose few hanging roots stay short, well above the bottom of the picture" &
  plat3 nuees_thin 144 104 "a short red lacquered wooden plank on top of a small floating rock" &
  wait
fi

# Fourth pass: the third Lunar bridge ran into both edges; ask for clear margins on its sides.
plat4() {  # name, output suffix, width, height, what it is
  python3 tools/pxcli.py image pxa/arenas/$1_$2 --model pro --w $3 --h $4 --style pxa/arenas/${1%%_*}_bg/f_00.png \
    --ref "pxa/arenas/flat_$1.png|the platform's look to keep: its materials, colours and details" \
    --desc "a game sprite on a fully transparent background, no sky, no clouds, no scenery: one single floating platform for a 2D fighting game, centred and about three quarters of the picture's width, with wide empty transparent margins on its left and right so that nothing touches any edge of the picture, seen from the front and slightly from above like a Super Smash Bros. stage: its flat top floor is clearly visible as a wide band going back in depth, above its front side; $5. Both of its ends are complete and finished, with bevelled corners; detailed anime pixel art" \
    > pxa/arenas/$1_$2.log 2>&1
}
if [ "$1" = fourth ]; then
  plat4 lunaire_solid depth4 512 200 "a long bridge of dark violet stone with a carved rim and a rocky underside whose few hanging roots stay short" &
  plat4 lunaire_solid depth4b 448 176 "a long bridge of dark violet stone with a carved rim and a rocky underside whose few hanging roots stay short" &
  wait
fi
