# Effets ParticleFX : les 15 prompts (06/11/2026)

Répartition voulue par le propriétaire (06/11) : PixelLab fait les persos et les armes (plus les fumées de saut, de
dash et d'atterrissage) ; ParticleFX fait tous les autres effets : marques de coups, coup critique, bouclier, ultime,
ondes d'impact, compétences, effets d'état. Les formes suivent ses maquettes validées
(`vfx_maquette_1_systeme.png`, `vfx_maquette_2_competences.png`, `ref_effets.png`).

Dans le studio : coller le prompt dans l'agent, puis **Export → GIF, 1x, fond transparent, 15 images/s**, et
enregistrer le GIF sous le nom donné (ex. `pfx_hit_quick.gif`). Le jeu l'importe avec `scripts/import_fx.py`.
Les effets « communs » servent à tous les persos ; ceux en blanc sont teintés par le jeu à l'élément de
l'attaquant. Les effets du 01 sont en blanc, rouge et or.

## Communs

### pfx_hit_quick : coup rapide
Pixel art QUICK HIT spark for a 2D side-view platform fighting game (like Rivals of Aether). Transparent background,
pixel art mode with pixel size 2 (canvas 128x128), plays ONCE in 0.35 s and ends empty, centred on the canvas. Look: a
sharp 4-point star twinkle, the vertical and horizontal points long and thin, gold #ffb428 with a white-hot core and a
1-art-pixel orange #e06a10 rim, plus 4 to 6 tiny gold spark pixels flying out diagonally and 2 small diagonal glints.
Timing: the star pops to full size in 2 frames (ease out), holds 1 frame, then shrinks to a point while the sparks fly
out and fade. No text, no character, no floor, no smoke, no grey, no dithering. Stays inside the canvas, readable at 1x.

### pfx_hit_slash : coup tranchant (blanc, teinté par le jeu)
Pixel art BLADE CUT hit for a 2D side-view fighting game: the mark a sword leaves on the target. Transparent
background, pixel art mode with pixel size 2 (canvas 192x160), plays ONCE in 0.4 s, centred. Look: a thick crescent
slash arc swept to the right, tips pointing left, its leading edge white-hot, its body white with very light blue-grey
#dfe6f5 shading (the game tints it, so keep it white and pale), 3 shorter parallel speed slivers along its outer side
and a few small shard pixels flung right. Timing: the arc wipes on from top to bottom in 2 to 3 frames, flashes, then
thins from the tips and breaks into shards that fade by 0.4 s. No text, no character, no floor, no smoke, no dark
grey, no dithering. Stays inside the canvas.

### pfx_hit_rise : coup ascendant
Pixel art RISING HIT for a 2D side-view fighting game: the blow that launches the target upward. Transparent
background, pixel art mode with pixel size 2 (canvas 128x192), plays ONCE in 0.45 s. The hit point is at x 64, y 150.
Look: a fan of sharp flame-like spikes bursting upward from the hit point, the tallest in the middle reaching near the
top, gold #ffc030 with white-hot cores and orange #e86a14 edges, a thin vertical white light line shooting up the
centre, gold spark pixels rising and slowing. Timing: the spikes shoot up in 2 frames, hold 1, then retract downward
and thin out while the sparks keep rising and fade. No text, no character, no floor, no smoke, no grey, no dithering.
Stays inside the canvas.

### pfx_hit_crit : coup critique
Pixel art CRITICAL HIT spark for a 2D side-view platform fighting game. Transparent background, pixel art mode with
pixel size 2 (canvas 192x192), plays ONCE in 0.5 s, centred. Look: a big sharp 8-point starburst: 4 long main spikes
(horizontal and vertical, reaching 85 % of the half canvas) and 4 shorter diagonal spikes, crimson red #e0202c with a
darker red #8c1020 1-art-pixel rim, a small white-hot core, thin red shard streaks shooting outward and a few red
square pixel chips. Timing: frame 1 a white flash core, the star snaps to full size in 2 frames (ease out), holds 2
frames, then the spikes thin and retract toward the centre while the shards fly out and fade; all gone by 0.45 s. No
text, no character, no floor, no smoke, no grey, no dithering. Bold and readable at 1x.

### pfx_hit_wave : onde d'impact des coups puissants (blanc, teinté par le jeu)
Pixel art IMPACT SHOCKWAVE bursting from the ground under a fighter hit by a powerful blow, 2D side-view fighting
game. Transparent background, pixel art mode with pixel size 2 (canvas 288x176), plays ONCE in 0.7 s. The ground line
is at y 160, centred at x 144 (do not draw the ground). Look: a flat elliptical shock ring on the ground (a squashed
circle, its far half behind, its near half in front) expanding fast to about 260 px wide, and a crown of sharp
vertical light spikes bursting up from the ring, the tallest in the middle (about 120 px), white and very pale blue
#e8eeff with white-hot edges (the game tints it, so keep it white and pale), small white sparks thrown up and falling
back. Timing: the ring and the spikes burst in 2 to 3 frames (ease out), the spikes sink back while the ring keeps
expanding and thins out, the sparks fade by 0.7 s. No rocks, no dust, no smoke, no grey, no text, no character, no
dithering. Stays inside the canvas.

### pfx_shield_hit : coup sur le bouclier
Pixel art SHIELD HIT for a 2D fighting game: a blow lands on a round bubble shield. Transparent background, pixel art
mode with pixel size 2 (canvas 160x160), plays ONCE in 0.35 s, centred on the impact point. Look: a bright white
impact star, two thin curved ripple arcs (parts of a circle, as if on the bubble's surface) in white and pale cyan
#cfefff spreading from it, and 5 to 8 small white sparks deflected away with 4-point twinkles. Timing: flash on frame
1, the ripples spread in 3 to 4 frames and fade, the sparks fade by 0.35 s. No text, no full circle, no character, no
smoke, no grey, no dithering.

### pfx_shield_break : bouclier brisé
Pixel art SHIELD BREAK for a 2D fighting game: a round glass-like bubble shield, about 150 px across, shatters.
Transparent background, pixel art mode with pixel size 2 (canvas 256x256), plays ONCE in 0.9 s, centred. Look: on
frames 1 and 2 the bubble flashes white with a thin dark outline and crack lines, then it breaks into 20 to 30 angular
glass shards (white with pale cyan #cfefff faces and white 1-art-pixel edges) that fly outward, spin, slow down and
fall a little, with a white flash ring expanding and 4-point white twinkles. All gone by 0.9 s. No text, no character,
no floor, no smoke, no dark grey, no dithering. Stays inside the canvas.

### pfx_ult_ready : effet d'état, ultime prêt
Pixel art ULTIMATE READY effect around a fighter in a 2D side-view fighting game: the fighter has just been charged
with their ultimate. Transparent background, pixel art mode with pixel size 2 (canvas 192x256), plays ONCE in 1.0 s.
The fighter (not drawn) stands with feet at x 96, y 240 and is about 170 px tall. Look: a gold #ffcf40 light ring
flashes on the ground (a flat ellipse), a column of vertical gold and white light streaks rushes up around the body,
gold 4-point stars and motes spiral up and twinkle, a brief white-gold flash at chest height. Timing: the ring and the
flash in 3 frames, the streaks rise for 0.4 s, the motes linger and fade by 1.0 s. No text, no character, no smoke,
no grey, no dithering. Stays inside the canvas.

## Le 01 (Hakuen) : blanc, rouge, or

### pfx_h01_crescent : Tranchant du néant (croissant lancé ; aussi les ondes de l'éveil)
Pixel art FLYING CRESCENT SLASH WAVE projectile for a 2D side-view fighting game, travelling to the right.
Transparent background, pixel art mode with pixel size 2 (canvas 256x192), seamless LOOP of 0.5 s; the crescent stays
at the canvas centre (the game moves it). Look: a big crescent-moon blade of energy with its tips pointing left and
its bulge to the right, the leading (right) edge white-hot, the body crimson red #c8242c fading to gold #e8b240 at the
tips, flickering flame tongues along its back edge, a short trail of horizontal red speed streaks and gold embers
behind it. No text, no character, no smoke, no grey, no dithering. Stays inside the canvas.

### pfx_h01_glint : éclat de la Rengaine (l'instant parfait pour dégainer)
Pixel art GLINT on a sheathed katana's guard, the instant a quick-draw is perfectly ready, 2D fighting game.
Transparent background, pixel art mode with pixel size 2 (canvas 96x96), plays ONCE in 0.35 s, centred. Look: a sharp
4-point star glint with a long thin horizontal point, a white-hot core, crimson red #c8242c and gold #e8b240 edges, a
tiny gold ring pulse and 2 small diagonal sparkles. Timing: it snaps on in 1 frame, holds 2, then shrinks to a point.
No text, no sword drawn, no character, no smoke, no grey, no dithering.

### pfx_h01_draw : Coupe-éclair (et Pas-iaï)
Pixel art LIGHTNING-FAST QUICK-DRAW CUT (iaido) for a 2D side-view fighting game: the swordsman dashes through and
leaves one straight cut in the air. Transparent background, pixel art mode with pixel size 2 (canvas 384x96), plays
ONCE in 0.45 s. Look: a long razor-thin horizontal cut line at y 48 from x 16 to x 368, a white-hot core, crimson red
#c8242c glowing edges, thin gold #e8b240 sparks spraying off it and a short white flash at its right end; then the line
splits into two hairlines that drift apart and burn out into red and gold embers. Timing: the line draws from left to
right in 2 frames, flashes, holds 2 frames, then splits and fades by 0.45 s. No text, no character, no sword, no
smoke, no grey, no dithering.

### pfx_h01_spiral : Spirale montante
Pixel art RISING FLAME SPIRAL for a 2D side-view fighting game: a swordsman's rising spinning slash leaves a spiral of
fire. Transparent background, pixel art mode with pixel size 2 (canvas 160x288), plays ONCE in 0.7 s. It starts at
the ground at x 80, y 272 and rises to near the top. Look: a tight helix of two crescent flame ribbons winding upward
around a vertical white-hot core line, like a pillar of flame with a spiral wrapped around it, crimson red #c8242c
with gold #e8b240 edges and white-hot leading edges, embers and gold spark pixels flung out. Timing: it spirals up
from the ground in 0.25 s, flashes at the top, then the ribbons burn out from the bottom up into embers by 0.7 s. No
text, no character, no smoke, no grey, no dithering. Stays inside the canvas.

### pfx_h01_parry : Prescience (chaque parade)
Pixel art PERFECT PARRY flash for a 2D fighting game: a swordsman's blade blocks a blow at the last moment.
Transparent background, pixel art mode with pixel size 2 (canvas 160x160), plays ONCE in 0.4 s, centred on the contact
point. Look: a gold #e8b240 4-point star flash with a white-hot core and one long horizontal anamorphic streak, a
crimson red #c8242c ring and a thin gold ring expanding, a radial spray of short gold and white sparks that stop dead
with drag. Timing: flash on frame 1, the rings expand in 3 to 4 frames, the sparks fade by 0.4 s. No text, no sword,
no character, no smoke, no grey, no dithering.

### pfx_h01_aura : Lame éveillée (aura de l'éveil, en boucle)
Pixel art FLAME AURA LOOP to place behind a fighter sprite in a 2D side-view fighting game (an awakened swordsman).
Transparent background, pixel art mode with pixel size 2 (canvas 192x256), seamless LOOP of 1.0 s. The fighter (not
drawn) stands with feet at x 96, y 240 and is about 170 px tall, so keep the middle (x 60 to 132, y 70 to 240) mostly
open. Look: stepped flame tongues licking up around an open oval around the body and meeting above the head, a
white-hot inner edge, crimson red #c8242c body, gold #e8b240 tips, a flat red and gold ring pulsing on the ground (an
ellipse), rising gold embers and short vertical light streaks. No text, no character, no smoke, no grey, no dithering.
Stays inside the canvas.

### pfx_h01_ult : Forge de l'aube (ultime)
Pixel art ULTIMATE ATTACK for a 2D side-view fighting game, "Dawn Forge": a swordsman holds the opponent and cuts it
nine times at blinding speed, then a final blast of dawn fire. Transparent background, pixel art mode with pixel size 2
(canvas 384x384), plays ONCE in 1.6 s, centred on the target (the canvas centre). Look: phase 1 (0 to 0.8 s) nine fast
straight slash lines crossing the centre at different angles, one every 0.08 s, each a thin white-hot line with
crimson red #c8242c edges that flashes and fades in 3 frames, leaving short gold #e8b240 sparks; phase 2 (from 0.85 s)
a white hero flash, then a big explosion of dawn fire: a gold and white starburst, a red and gold flame ring
expanding, flame tongues bursting upward, embers and gold sparks raining down; it fades by 1.6 s. No text, no
character, no sword, no grey smoke, no dithering. Stays inside the canvas.
