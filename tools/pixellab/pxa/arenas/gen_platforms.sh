#!/bin/bash
# One picture per platform of the five arenas, at the game's size (half resolution: imported x2), in each backdrop's
# style: finished at both ends, a flat walkable top across the whole width (scripts/import_art.py imports them).
cd "$(dirname "$0")/../.."
plat() {  # name, width, height, what it is
  python3 tools/pxcli.py image pxa/arenas/$1 --model pro --w $2 --h $3 --style pxa/arenas/${1%%_*}_bg/f_00.png \
    --desc "one single floating platform for a 2D fighting game, seen exactly from the side, on a transparent background: $4. Its top is one perfectly flat, level walkable surface across its whole width; both of its ends are complete and finished, nothing is cut by the edges of the picture; nothing else, no characters, no text; detailed anime pixel art" \
    > pxa/arenas/$1.log 2>&1
}
plat lunaire_solid 448 144 "a long bridge of dark violet stone with a carved rim and a rocky underside with a few hanging roots" &
plat lunaire_thin 112 40 "a short thin slab of dark violet stone with a carved rim" &
plat givre_solid 392 168 "a white castle rampart covered in snow, a row of red roof tiles along its top edge, icicles below, on snowy rocks" &
plat givre_thin 112 44 "a short snowy wooden beam with small icicles under it" &
plat ecarlate_solid 176 112 "a red sandstone rock ledge with two small paper lanterns hanging under it" &
plat ecarlate_thin 104 40 "a short wooden plank bridge section, its planks bound with rope at both ends, no ropes going up" &
plat kurenai_solid 496 152 "the long dark tiled ridge of a Japanese roof, flat on top, with red paper lanterns hanging under the eaves" &
plat kurenai_thin 104 40 "a short dark wooden beam with iron bands" &
plat nuees_solid 312 160 "a grey stone floor with a red lacquered wooden edge and a low red railing at each end, on a floating rock" &
plat nuees_small 112 96 "a small floating rock island with a flat grey stone top and a red lacquered wooden edge" &
plat nuees_thin 112 72 "a short red lacquered wooden plank on top of a small floating rock" &
wait
