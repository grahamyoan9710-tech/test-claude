# Making PixelLab effects for Super Clash Bros. ultimates

You make pixel-art VFX with PixelLab's free `vfx_workbench` tool for a Smash-like fan game (C++/raylib). You do NOT
touch the game repository (/home/user/test-claude): you only leave finished effects in
`SCRATCH/vfx/ult/` and a notes file. Another agent integrates them.

SCRATCH = /tmp/claude-0/-home-user-test-claude/454a74a0-0344-5f0d-8ed7-e18843be178a/scratchpad

## Rules
- Never print, cat or echo the PixelLab token (SCRATCH/px/.token) or any file under SCRATCH/px/. The helper below
  reads it for you.
- Only use `vfx_workbench` (free). Do not call any other PixelLab tool (they cost generations).
- Work from SCRATCH (`cd SCRATCH`). Keep your renders under `SCRATCH/vfx/ult/work/<effect>/`.
- Look at every render you make (Read tool on the saved PNG) and judge it like an art director: does it read at 1x,
  does it look like the anime/game it comes from, few clean colours, no muddy blobs, nothing cut by the canvas edge,
  a loop with no jump. Two to five renders per effect is normal; stop when it is good.

## Docs already fetched
The tool's long help texts are cached in `SCRATCH/vfx/ult/work/_d/` (cookbook.txt, recipe.txt, parts.txt, render.txt,
craft.txt, start.txt): read or grep those instead of calling `describe` again.

## Calling the tool
`python3 tools/pxcall.py vfx_workbench '{"argv": [...]}' --save DIR` prints the tool's text and saves its images
(DIR/call_1.png ...). Each call returns `effect step: <id>`; pass the LATEST step id to the next call.
Useful argv forms (read `["describe","recipe"]`, `["describe","parts"]`, `["describe","tool","render"]` once if you
need details; they are long, grep them):
- `["start","--fps","15"]` plus a top-level `"notes": "<what the effect is>"` -> a new effect step.
- `[step,"add_part", "<json>"]` with `{"parts":[{"name":"fire_blast","around":"canvas","size":2,"colour":[...],"core":true}, ...]}`
  (parts list: describe parts; colours are a preset ramp name or "#rrggbb" dark -> light).
- `[step,"render","<json>"]` with `{"recipe": {...full recipe...}, "zoom": 2}` or `{"patch":[ops], "zoom":2,
  "frames":[1,4], "fit_canvas": true}`. Patch ops: `{"op":"set","path":"layers.x.radius","value":40}`,
  `{"op":"add_layer","layer":{...}}`, `{"op":"replace_layer","layer":{...}}`, `{"op":"remove_layer","id":"x"}`,
  `{"op":"set","path":"ramps.name","value":["#..",...]}`.
- `[step,"remove_part","{\"id\":\"<part id>\"}"]`, `[step,"set_part","{\"id\":..., \"colour\":..., ...}"]`.
- `[step,"export","--name","<Nice Name>"]` -> prints `download https://api.pixellab.ai/mcp/pixel-tools/<image id>/:
  <file>.gif, <file>.json, <file>.png` and the anchor. Download the .png AND the .json with
  `curl -sS --retry 4 -o SCRATCH/vfx/ult/<effect>.png "https://api.pixellab.ai/mcp/pixel-tools/<image id>/<file>.png"`
  (same for .json). A connection reset happens: retry. Check both files exist and the PNG opens.

## What worked before (lessons)
- A recipe needs `canvas`, `ramps.main`, layers; `core` must name an EXISTING layer id.
- Layers that share a ramp and a z merge into one shape: give separate ramps to things that must stay apart.
- Removing a layer removes ramps no other layer uses: re-declare a ramp in the same patch if you add a layer using it.
- `fit_canvas: true` shifts every position: read the new positions from the result before patching x/y again.
- `outline` at recipe level `{"mode":"none"}` drops the dark outline (good for glowing energy); per layer `"outline": false`.
- shape layers draw your own outline (points relative to x,y) with fill edge/linear/radial/flat/fire/smoke;
  flame layers make drawn fire tongues; noise layers make ragged masses; particles (rise, travel, converge, burst,
  vortex) make motes/sparks/streaks; bolt makes lightning (keep jag low ~0.05-0.12 for clean bolts); rays, ring,
  glow, arc (slash crescents, profile "slash").
- Effects travelling sideways must travel to the RIGHT (+x); the game mirrors them.

## Conventions for the game (put these in your notes)
- projectile (loop): flies right; ORIGIN = the centre of its hitbox (the head of a spear, the centre of an orb).
- sky strike (loop or one-shot): ORIGIN = the point it hits (the target's body centre); it extends upward.
- area / storm (loop): ORIGIN = the centre of the area.
- explosion (one-shot, 10-14 frames): ORIGIN = its centre.
- beam (loop, a horizontal segment that tiles): ORIGIN = left middle; it must tile seamlessly left-right.
Note for each effect, in px of the exported frame: the origin, and the radius (or half-height for a beam) that the
game's hitbox should have so it matches what is drawn.

## Output
- `SCRATCH/vfx/ult/<effect>.png` + `.json` for every effect (effect = the snake_case name given in your list).
- Append one JSON object per effect to `SCRATCH/vfx/ult/notes_<your group>.json` (a JSON list):
  `{"effect": ..., "kind": "projectile|sky_strike|area|explosion|beam", "loop": bool, "frames": n, "fps": f,
    "frame_w": w, "frame_h": h, "origin": [x, y], "hit_radius_px": r, "pixellab_effect_id": "...", "comment": "..."}`
- Your final message: the list of effects with one line each (good / weak and why).
