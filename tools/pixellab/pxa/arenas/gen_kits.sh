#!/bin/bash
# Platform kits of the five arenas: a long main platform (left) and a short thin one (right), in each backdrop's style.
cd "$(dirname "$0")/../.."
kit() { python3 tools/pxcli.py image pxa/arenas/$1_kit --model pro --w 480 --h 160 --style pxa/arenas/$1_bg/f_00.png \
  --desc "two separate floating platforms for a 2D fighting game, side view, on a transparent background, with empty space between them: on the left a long wide platform, $2, with a perfectly flat walkable top surface; on the right a short thin floating platform, $3; nothing else, no characters, no text; detailed anime pixel art" > pxa/arenas/$1_kit.log 2>&1; }
kit lunaire "dark violet stone with a carved rim and a rocky underside" "a short thin violet stone slab" &
kit givre "white castle stone covered in snow with a row of red roof tiles along its front edge and icicles below" "a short snowy wooden beam" &
kit ecarlate "a red sandstone rock ledge with a few small paper lanterns hanging under it" "a short wooden plank held by ropes" &
kit kurenai "the dark tiled ridge of a Japanese roof with a flat top and red lanterns hanging under the eaves" "a short dark wooden beam" &
kit nuees "grey stone with a red lacquered wooden edge and a low red railing, on a floating rock" "a short red lacquered wooden plank" &
wait
