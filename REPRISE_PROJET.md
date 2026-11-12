# Reprise du projet — état au 14/10/2026, ~17 h UTC

Ce document dit où en est le travail et comment le reprendre sans rien refaire. Lire aussi **CLAUDE.md**.

## 0. Travail à deux : qui fait quoi
Deux sessions Claude Code travaillent en même temps sur la branche `claude/sleepy-goodall-s399kv`. Pour éviter les
conflits, chacune a ses propres fichiers.

**Session A**, celle du propriétaire du compte PixelLab. Elle seule a les images brutes générées et les jobs.
- Tout ce qui touche aux **clips des combattants** :
  - génération PixelLab et validation des bases ;
  - import et relecture des clips ;
  - corrections images par image, y compris Ulquiorra et Byakuya ;
  - régénérations.
- Fichiers concernés :
  - `assets/sprites/<perso>_<clip>.png` ;
  - les lignes `sheet` de `assets/sprites/sheets.txt` ;
  - tout `tools/pixellab/` ;
  - `src/core/roster_hitfit.inc`, car elle relance `fitall.sh` après ses imports.

**Session B**, l'ami.
- Les **effets et le code du jeu** :
  - import des rayons d'ultimes (section 4, point 4) ;
  - création de `wyvern_fire` avec vfx_workbench (gratuit) ;
  - vérification des ultimes en jeu ;
  - **équilibrage** (Kakashi 72 %) ;
  - IA, menus, README, préparation de la v10.0.
- Fichiers concernés :
  - `assets/sprites/fx_*.png` et les lignes `fx` de `sheets.txt` ;
  - `src/` (sauf `roster_hitfit.inc`) ;
  - `README.md`.

**Règles communes**
- Avant de commencer et avant chaque push : `git pull origin claude/sleepy-goodall-s399kv`, en fusion simple.
  Jamais de `rebase`, jamais de `push --force`.
- Commits petits et fréquents.
- Pour toucher aux fichiers de l'autre session, l'écrire d'abord dans le journal ci-dessous.
- Conflit sur `sheets.txt` : garder les deux lignes. Le fichier est trié, une ligne par clip ou effet.
- **`fitall.sh` n'est lancé que par la session A**, après ses imports : il réécrit `roster_hitfit.inc` et les
  impacts de `sheets.txt`, et deux passes en même temps se contredisent. La session B qui a besoin d'un recalage le
  demande dans le journal.
- Noter dans le journal ce qui **commence** (« je lance… », « je modifie… »), pas seulement ce qui est fini.

### Verrous
Avant une tâche longue sur un fichier partagé : `git pull`, vérifier que la ligne est libre, mettre son nom et
l'heure, commiter et pousser **avant** de commencer. Remettre « libre » à la fin, dans le même commit que le travail.
Un verrou de plus de 2 h sans commit de son détenteur peut être repris, en le notant dans le journal.

| Ressource | Détenteur | Depuis |
|---|---|---|
| `fitall.sh` (`roster_hitfit.inc`, impacts de `sheets.txt`) | libre | |
| Imports de clips (lignes `sheet`, `assets/sprites/<perso>_*.png`) | libre | |
| Effets (lignes `fx`, `assets/sprites/fx_*.png`) | session claude/project-thread-gzf1un (tous les effets refaits dans Blender) | 12/11 17 h 15 UTC |
| Équilibrage (`src/core/roster*.cpp`) | libre | |
| Version et paquets (README, numéros de version, `dist/`) | libre | |

### Journal (le plus récent en haut)
- 12/11 22 h UTC, session claude/project-thread-03jksh : version 15.4 (15.3 réservée au fil des effets, gzf1un).
  Réglage « Résolution » (`Settings::resolution`, `kResolutions` dans `settings.hpp`, ligne 8 de l'onglet Affichage) ;
  plein écran = vrai plein écran (`ToggleFullscreen` à la résolution choisie, `App::applyDisplaySettings`) au lieu
  du fenêtré sans bordure, qui laissait des bandes noires chez yoyo. Repli automatique RTX/qualité retiré (yoyo :
  « t'as déjà vu un jeu qui te fait ça ? »). Verrou « Version et paquets » pris et libéré dans ce commit.
- 12/11 19 h 10 UTC, session claude/project-thread-gzf1un : version 15.2 = tous les projectiles dans Blender (`tools/blender/effets.py`, une recette par forme, colorée par perso : orbe, météore, lance, éclair, étoile, dague, plume, marque, pilier). 16 bandes `fx_pfx_<perso>_<forme>.png`, 18 techniques rebranchées sur `k.pfx(...)` dans `roster.cpp` (dont les ultimes EXÉCUTION ÉCARLATE et TEMPÊTE ÉCARLATE en piliers) ; `pfx_spiral_sphere` (ParticleFX) retirée. `render_world.cpp` : le pied d'une grosse colonne reste au sol (décalage plafonné à 60). Verrou « Version et paquets » libéré ; « Effets » gardé (suite : effets de lancement, auras, ultimes).
- 12/11 18 h 35 UTC, session claude/project-thread-gzf1un : version 15.1. Projections de lame dans Blender (`tools/blender/lame.py`, boucles de 12 images, un style par perso) pour 01 (odachi), 03, 07, 08 et tous les croissants de Hakuen (`k.pfx("lame")`, `pfx_h01_crescent`) ; MILLE LUNES (`tools/blender/mille_lunes.py`) en deux calques : `pfx_p01_a_ult` derrière les persos, `pfx_p01_a_ult_front` devant (`art::fxFront` : tout effet de technique `X` joue aussi `X_front` s'il existe). Ligne `fx` : 11e champ = bloom déjà dans la bande. Option de démo `--skill n|s|u|d`. Verrou « Version et paquets » libéré ; « Effets » gardé (suite : compétences, auras et ultimes de chaque perso).
- 12/11 17 h 30 UTC, session claude/project-thread-gzf1un : version 15.0 = premiers effets Blender. Ultime de Hakuen : éclair fin et net en diagonale, du côté de Hakuen jusqu'au sol sous la cible (`combat.cpp` : une colonne part du sol ; `render_world.cpp` : elle penche loin de son lanceur), une frappe par éclair ; KO : l'écran se fissure et se déchire au bord quitté (`fx.cpp`, taille fixe à l'écran, sans halo du jeu + 2e passe additive blanche). Scripts dans `tools/blender/`, CLAUDE.md : les effets se font dans Blender. Verrou « Version et paquets » libéré ; « Effets » gardé (suite : tous les effets de tous les persos en Blender).
- 12/11 17 h 15 UTC, session claude/project-thread-gzf1un : yoyo abandonne ParticleFX pour Blender (« t'es trop chaud avec Blender ») et veut les effets de tout le monde refaits dans Blender, les projections de lame proches des tranchants d'énergie de Vergil ou des grands croissants d'énergie, et le jeu poussé sur GitHub. Début de la version 15.0 (éclair de Hakuen v6 et KO « déchirure » faits dans Blender) ; verrous « Effets » et « Version et paquets » pris.
- 12/11 11 h 55 UTC, session claude/project-thread-gzf1un : version 14.9 = DA « Prestige » dans tout le jeu (validée par yoyo le 12/11 : maquette https://claude.ai/artifact/4m5h9zcVRzzmjeLvByeV5h). Titre (bouton COMMENCER, variante A choisie), menu, sélection (`menus.cpp` : deux côtés ou quatre cartes penchées, face-à-face des stats, tiroir Règles & mode), arène, VS, HUD (`render_hud.cpp`), 3-2-1-GO, cut-in d'ultime tenu 66 images (`match_screen.cpp`), ralenti + zoom avant GAME, résultats. Verrou « Version et paquets » libéré.
- 12/11 11 h 45 UTC, session claude/project-thread-gzf1un : début de la version 14.9 (DA « Prestige » dans tout le jeu), verrou « Version et paquets » pris.
- 12/11 01 h UTC, session claude/project-thread-gzf1un : version 14.6 = glitch du % (render_hud.cpp), caméra plus près (zoom max 1,35), sorties de terrain ×0,8 (stageList), dégâts ×0,8 (kDamageScale, ultimes exclus), dash de Hakuen +25 %, rayons retirés (ui::sunburst supprimé).
- 11/11 22 h 35 UTC, session claude/project-thread-gzf1un : version 14.5 = cartes de combat transparentes façon Smash (render_hud.cpp fighterCard : carré penché, buste ou visage, bande du nom), demande de yoyo.
- 11/11 22 h 15 UTC, session claude/project-thread-gzf1un : version 14.4 = Règles & mode en volet à droite, panneau d'entraînement Épure, rotation des moitiés dans la Coupe (ui::snapQuad prend un angle).
- 11/11 22 h UTC, session claude/project-thread-gzf1un : version 14.3 = fin de la DA « Épure ». Cartes de combat
  façon Smash (plaque à la couleur du joueur, `render_hud.cpp`), cartes qui montent au début du combat, arrêt sur image
  noir et blanc du GAME (`pix::desaturate`, `pixel.cpp`), Horizon recalé (54 images), animations d'entrée (tuiles de
  la sélection, fiches 3-4 joueurs, Règles & mode via `rulesAnim_`, arènes, pause via `pauseAnim_`, galerie,
  options) ; sélection 3-4 joueurs, pause, options et galerie restylées. Reste à l'ancienne : panneau d'entraînement.
- 11/11 21 h 30 UTC, session claude/project-thread-gzf1un (partie de 14.1, h4e20u da84e804) : version 14.2 = DA
  « Épure », palette Crépuscule validée par yoyo (maquettes Claude Design dans le fil « Refaire la DA en version
  épurée »). Polices : Archivo ExtraCondensed (Title), Archivo Expanded (Display, nouveau), Inter (Ui et Body) ; Saira
  retirée. Palette dans `ui.hpp` (kDusk*, accent cyan kBlue, kGold = crème). Nouveaux : écran titre séparé du menu
  principal (`Screen::Title`, `--screen title`), `ui::icon`, `ui::label`, `ui::dusk`, transitions `ui::Wipe`
  (Horizon, Flash avant le combat, Cut avant le VS avec une capture de l'écran, Game vers les résultats),
  `art::drawPictureFaded`. Refaits : sélection (cartes + fiches larges ; 3-4 joueurs encore à l'ancienne mise en
  page), Règles & mode, VS, cartes de combat, GAME, cut-in d'ultime, victoire, choix de l'arène. Illustrations
  ChatGPT abandonnées (« ça fait trop IA ») : portraits manga `<id>_art` + pixel art.
- 11/11 13 h 45 UTC, session claude/project-thread-ab9ctl : version 13.9 (fusion de claude/project-thread-owhcqo,
  13.8). yoyo : en RTX, des effets « trop blancs et lumineux », « poussière lumineuse ». Cause : la lumière tracée
  des effets dépassait 1 (ultime de Hakuen : plus de 2 sur tout l'écran), multipliée dans le décor jusqu'au blanc,
  son grain visible dans le ciel. Composite (`render_rtx.cpp`) : `tame` plie la lumière des effets vers un plafond
  (KNEE 0,6) dans sa couleur, gain du décor plafonné à 1,2 dans sa couleur, épaule douce (0,8) sur la couleur finale,
  pixels pâles des effets gardés tels que dessinés. Verrou « Version et paquets » pris et libéré dans ce commit.
- 11/11 13 h UTC, fil « nouvelle DA avec les portraits » (claude/project-thread-owhcqo) : version 13.8, à la demande de
  yoyo : pixel art (drawShowcase) de nouveau dans les fiches de la sélection et à la victoire ; visages ronds et cases
  dézoomés (drawArtDisc / drawArtBox : span 1.1 par défaut, toute la tête) ; tuile du portrait cadrée sur toute la
  tête. Verrou « Version et paquets » pris et libéré dans ce commit.
- 11/11 06 h 20 UTC, fil « nouvelle DA avec les portraits » (claude/project-thread-owhcqo) : version 13.7, DA
  « Encre d'horizon » validée par yoyo (maquette Claude Design https://claude.ai/artifact/Ss21FmbFf2vpLPgrw1nWaX, %
  en Exo 2 Black Italic, OFL). Portraits manga : `scripts/cut_portraits.py` découpe la planche de yoyo
  (`art/portraits/`) ; `assets/sprites/<id>_art.png` + `kArtFaces` (`render_body.cpp`, centre du visage et des yeux)
  pour chaque perso illustré (seul h01 pour l'instant). Cartes de combat façon Smash (`render_hud.cpp`), cut-in
  (`match_screen.cpp`), victoire (`screen_results.cpp`), sélection réécrite (`menus.cpp` : tuiles penchées, fiches
  avec stats, feuille « Règles & mode » ouverte par R / Y, `--screen rules` pour la capturer). Fusion de
  claude/project-thread-xcscba (13.6). Verrou « Version et paquets » pris et libéré dans ce commit.
- 11/11 01 h 50 UTC, session claude/project-thread-ab9ctl : version 13.5 (après la 13.4 de Hakuen, fusionnée ;
  le numéro 13.2 prévu est abandonné).
  yoyo (13.1) : plateformes sombres alors que la lumière tombe dessus ; « presque plus d'ombre » : la demande était
  de retirer les rayons d'ombre, pas les ombres des persos, de l'orbe et des objets sur le sol. Le dessus des
  plateformes fines et des petites îles (lignes `Top` de `render_stages.cpp`) est un sol dans le masque RTX (canal
  vert, `drawPlatformPart`), leur corps reste solide ; `stageart::topOf` donne ce dessus. `light::groundShadows`
  (`render_light.cpp`, RTX seulement) : un ovale doux sous chaque perso, l'orbe et les armes posées, cœur sombre et
  bord flou, étiré à l'opposé de la lune ou du soleil, plus pâle et plus large en l'air, le même à chaque image.
  Ombre minimale de la lune 0,2 (0,45 en 13.0). Verrou « Version et paquets » pris et libéré dans ce commit.
- 11/11 01 h 50 UTC, session cloud « Hakuen (01) » (claude/project-thread-58u7pf) : version 13.4. Ultime de Hakuen =
  le modèle Thunder God Strike envoyé par yoyo (export ParticleFX, 84 images 640x768, gardé hors git), repeint au
  rouge de ses compétences (teinte -15° → -5°, saturation ×(1 + 0,6 × valeur), voile < 8/255 et bords du cadre
  effacés) et réduit à 80 % : `art/particlefx/pfx_h01_ult.png` + `.json` (images 14 à 62 sur 2). Joué à 30 i/s
  (double vitesse), frappe à sa 15e image = `warn` 28 ; 5 éclairs toutes les 10 images, rayon 60 (`ult::thunder`).
  Taille : `kGrow` 1,05 / 0,8 (yoyo : « que 5 % plus large » que le modèle) ; `shotSprite` passe maintenant par
  `art::fxSize`. `import_fx.py` met les images sur plusieurs rangées quand une bande passerait 8192 px (le jeu le lit
  via `cellOf`). Ce modèle garde son propre bloom (`Fx::ownGlow`) : le halo du jeu par-dessus le rendait blanc.
  13.2 et 13.3 sont réservées aux fils RTX et hitbox (pas encore sorties). Verrou « Version et paquets » pris et
  libéré dans ce commit.
- 11/11 00 h 30 UTC, session claude/project-thread-ab9ctl : version 13.1. yoyo veut un jeu « bien dynamique et
  agressif » : traînée d'éjection et explosion de KO plus grosses, effets de mouvement +5 %. Tableau `kGrow` de
  `art::fxSize` (`fighter_art.cpp`) : pfx_jump / pfx_airjump / pfx_land / pfx_dash ×1,05, pfx_trail ×1,6, pfx_ko ×1,7
  (au-dessus des pixels des persos) ; une bouffée de traînée à chaque image au lieu d'une sur deux (`Fx::update`).
  Verrou « Version et paquets » pris et libéré dans ce commit.
- 11/11 00 h UTC, session claude/project-thread-ab9ctl : version 13.0, **ombres RTX adoucies**. yoyo (12.8) : arènes
  trop chaudes, faisceaux de lumière trop gros, ombrage « comme de la poussière pixelisée », ombres si marquées qu'on
  voit des faisceaux d'ombre. `keyLight` (`render_rtx.cpp`) : ni ombre ni voile de la lune dans l'air ; ombres douces
  par le champ de distance, les mêmes à chaque image (le point tiré au hasard sur le disque faisait la poussière :
  ne plus jamais tirer les ombres au hasard), jamais sous 0,3 ; les persos (canal rouge du masque) ne projettent plus
  l'ombre de la lune ; un sol vu en profondeur prend la lumière de son bord arrière, colonne par colonne (sinon une
  ombre le traverse en biais : un faisceau). Lunes et soleils presque blancs (`Key sky`, `render_stages.cpp`), teinte
  non éclairée ×0,9, gains air 0,25 / sol 0,6 / solide 0,9 / perso 0,6. Verrou « Version et paquets » pris et libéré
  dans ce commit.
- 10/11 21 h 50 UTC, session cloud « Hakuen (01) » (claude/project-thread-58u7pf) : version 12.9 (après la 12.8 RTX,
  fusionnée). yoyo : la marche de Hakuen trop rapide → `CharacterDef::walkStride` (0.8 par défaut, 1.6 pour h01 : ses
  pas en 3/4 sont courts). Son coup de pied vers le haut (usmash, « rising crescent kick ») n'a pas l'air naturel : un
  grand écart figé, le corps raide. PixelLab est à 0 génération jusqu'au 13/11 : à régénérer ce jour-là avec un prompt
  plus précis (buste penché en arrière, bras en contrepoids, jambe en arc continu sans pause en haut, genou d'appui
  plié).
- 10/11 soir UTC, session claude/project-thread-ab9ctl : version 12.8, **RTX : lune ou soleil de chaque arène**.
  yoyo (5070) voyait des images fantômes et de la « poussière » : passe `kResolveFrag` (`render_rtx.cpp`), filtre 5x5
  qui ne traverse pas les bords des solides + historique borné à la moyenne ± écart ; la fumée des déplacements
  (`smoke()`, `fighter_art.cpp`) n'a plus ni bloom ni lumière. Reflets RTX sur toutes les plateformes selon leur
  matière (`stageart::mirrorOf`, rangées `Top` et `floorRough` dans `render_stages.cpp`). Lumière principale par
  arène (`Key sky`, `stageart::keyLight`) : un rayon d'ombre par pixel vers un point du disque de la lune ou du
  soleil (canal alpha de la lumière tracée), ombre qui s'efface sur 260 px tracés, plus faible loin de la source ;
  la teinte du décor non éclairé passe à ×0,8 sous une lune ou un soleil (ombres plus marquées).
  Plus aucun effet PixelLab avec `kVfx = false` (`moveFx` ne force plus `move`) ; traînée d'éjection plus foncée
  selon les % et le temps de vol. Verrou « Version et paquets » pris et libéré dans ce commit.
- 10/11 17 h 10 UTC, session cloud « Hakuen (01) » (claude/project-thread-58u7pf) : version 12.7. Ultime de Hakuen
  façon sort de foudre : 5 éclairs (`ult::thunder(name, e, fx, warn 31, bolts 5)`, un toutes les 14 images), chacun
  posé sur le sol sous l'adversaire à son apparition et frappant toute la colonne (`ProjectileDef::column`), sans
  `follow` (l'éclair suivait le perso touché). `fx_pfx_h01_ult.png` : 8 premières images coupées (24 au lieu de 32 ;
  une réimportation doit passer `--only 8-31`). Test : `sim_test hakuen` (esquive). Le test des ultimes contre un
  bouclier compte un bouclier usé à 0 comme brisé.
- 10/11 16 h 40 UTC, session cloud « Hakuen (01) » (claude/project-thread-58u7pf) : version 12.6. yoyo revient sur sa
  demande : Hakuen reprend ses couleurs d'origine (repeinture en blanc de la 12.4 annulée, `scripts/h01_white/`
  retiré). Branche RTX ab9ctl (12.5) fusionnée avant. Verrou « Version et paquets » pris et libéré dans ce commit.
- 10/11 16 h UTC, session claude/project-thread-ab9ctl : version 12.5, **RTX plus profond** (yoyo veut des zones un
  peu plus sombres aux couleurs légèrement chaudes, des lumières et des reflets bien forts façon path tracing, même si
  ça rame).
  `render_rtx.cpp` : la composition relit une copie de la cible (`g_copy`) et la réécrit : décor × teinte chaude
  (0,7 / 0,65 / 0,6, les hautes lumières gardent leur blanc, vignette légère) + lumière tracée + brume de lumière ;
  persos (canal rouge du masque) à 0,9 neutre + liseré en bandes ; pixels des effets (canal bleu, `rtx::light(draw,
  core)`) et lanternes gardés tels quels. 32 rayons, 64 pas, portée 640, rebond 0,6. Reflets ×1,8 en RTX
  (`g_mirrorGain`, `render_light.cpp`), et sans la qualité Haute. `app.cpp` : plus de coupure auto du RTX (yoyo
  optimisera plus tard). Verrou « Version et paquets » pris et libéré dans ce commit.
  Correctif avant l'annonce à yoyo (archives 12.5 refaites à la main, le tag v12.5 reste sur 2e44c39b) : le centre
  pâle des lanternes de Kurenai devenait une tache brune (gardé tel quel aussi : test « chaud », jamais sur un perso).
- 10/11 fin d'après-midi UTC, session cloud « Hakuen (01) » (claude/project-thread-58u7pf) : version 12.4. Ultime à
  mains nues = un seul grand éclair rouge sang foncé sur l'adversaire (`ult::thunder`, PV::Spear, effet ParticleFX
  f20d9da6, `pfx_h01_ult`) ; élément de Hakuen = Lightning (fin du disque doré). Effets communs refaits en
  ParticleFX (`pfx_jump`, `pfx_land`, `pfx_dash`, `pfx_airjump`, `pfx_trail`, `pfx_ko`, `pfx_shield` ; voir
  `art/particlefx/`), la traînée d'éjection lisait `vel` au lieu de `kbVel` (jamais affichée) ; `import_fx.py --soft`.
  Hakuen repeinte en blanc (variante 3 : noir et or en blanc, `scripts/h01_white/`, le propriétaire peut encore
  choisir la 2 : or en rouge, ou la 1 : l'or reste). Branche RTX (claude/project-thread-ab9ctl, 12.3) fusionnée.
  Verrou « Version et paquets » pris et libéré dans ce commit.
- 10/11 12 h UTC, session claude/project-thread-ab9ctl : version 12.3, **vrai RTX** (réglage Affichage > RTX, `rtx`
  dans `clash_horizon.cfg`, Non par défaut). Oui : `render_rtx.cpp` (`rtx.hpp`) lance la lumière en 2D sur la carte
  graphique : masque des persos et des corps des plateformes, lumière des effets (`rtx::light` dans `fighter_art.cpp`,
  effets pixel art compris) et lanternes lues sur le décor (`lamps` par arène, seulement Kurenai 0,6), champ de
  distance par jump flooding en 640x360, 16 rayons par pixel, un rebond (lumière de l'image d'avant), accumulation
  dans le temps. Sur les persos, lumière par case de pixel art, en bandes, plafonnée (pas d'olive). Non : la lumière
  dessinée de `render_light.cpp`, dont les flaques et les ombres suivent maintenant les bouts du sol (`ends`). Si ça
  rame, `app.cpp` coupe le RTX d'abord, puis la qualité Haute. Branche à fusionner dans
  `claude/project-thread-58u7pf` avant la prochaine version, sinon la « latest » perd le RTX. Verrou « Version et
  paquets » pris et libéré dans ce commit.
- 10/11 après-midi UTC, session cloud « Hakuen (01) » : version 12.2, effets de Hakuen refaits avec les favoris
  ParticleFX du propriétaire. Croissant (`pfx_h01_crescent`, effet 61f74d8d…, base Twin Fang X) droit devant (plus
  `along()`), aura d'éveil (7c8680d4…, base Chakra Energy Charge), onde d'éveil `pfx_h01_wave` (324642af…, base
  Shockwave Ring ; l'onde PixelLab `move_wave_1` faisait un tourbillon brun), Pas-iaï (8ea1869a…, Dash Wind Burst),
  Spirale montante (2ef882da…, Rising Uppercut), ultime (6aa8fcd2…, Thunder God Strike en rouge, joué sur
  l'adversaire : `onFoe`). Foudre rouge partout (Alpha de PGR). Teinte d'éveil 0.22 → 0.08. Pour capturer un coup en
  démo, un bout de code temporaire dans `match_screen.cpp` (non commité) forçait la touche spéciale.
- 10/11 09 h 00 UTC, session cloud « Hakuen (01) » : arme A (nodachi) de Hakuen : garde armée placée
  (`pxa/base/h01_w0guard256.png`) et 15 clips générés (toutes les générations PixelLab utilisées). Bons : idle, walk,
  jump, shield, hurt, dspecial, ultimate, rengaine. Refaits le 13/11 (mis de côté en `.qa1`) : dash (traits gris),
  jab, jab2, jab3, nspecial, sspecial, uspecial (traînées énormes, gris, poussière). Prompts du kit corrigés (traînée
  fine). Rien n'est importé avant que l'arme A soit complète ; restent aussi ses 11 autres clips.
- 10/11 09 h 50 UTC, même session : yoyo voit des effets « RTX » mal découpés. Les reflets des sols couvraient tout le
  rectangle de la plateforme (bouts du toit Kurenai, décor à côté de la dalle lunaire) : ils suivent maintenant le
  trapèze du sol (`insetBack`/`insetFront` par arène, `render_stages.cpp`). Version 12.1.
- 10/11 09 h 30 UTC, session claude/project-thread-58u7pf : yoyo demande Hakuen dans le jeu sans ses 2 armes. Ses armes
  ne tombent plus (`CharacterDef::armed = false` dans makeH01, à retirer quand les clips du nodachi sont validés).
  Version 12.0 publiée par le workflow lancé à la main sur cette branche. Verrou « Version et paquets » pris et
  libéré dans ce commit.
- 10/11 05 h 45 UTC, session cloud « Hakuen (01) » : **les 15 effets ParticleFX sont dans le jeu** (pixel art, bloom,
  `import_fx.py --pixel`) : coups rapide, coupe, montant, critique, onde au sol, coup et bris du bouclier, ultime prêt
  (pour tous) ; croissant, éclat, dégainé, parade, spirale, aura et ultime « Forge de l'aube » de Hakuen
  (`pfx_h01_*`, ses tirs utilisent `k.pfx("crescent")`). 76 builds sur 150 utilisés. `pfxmcp.py` lit les flux SSE ;
  `build_effect` plante avec le paramètre `name`. Pas encore validé en jeu par yoyo.
- 06/11 16 h 00 UTC, session cloud « Hakuen (01) » : yoyo a **acheté le pack de 150 builds ParticleFX** : Claude
  fait les effets par MCP (3 builds dépensés). Portée (yoyo) : coups, coup critique, onde, bouclier et « ultime
  prêt » pour tous les persos ; compétences, éveil et ultime pour Hakuen seulement. Les effets pixel art
  (`import_fx.py --pixel`, 10e champ des lignes `fx`) sont dessinés à la taille des pixels du perso
  (`art::pixelScale`), jamais étirés ni tournés, sans halo ni lumière sur les persos (ils les rendaient olive).
  Premier fait : `pfx_hit_quick` (coup rapide, environ la moitié du perso, source `art/particlefx/pfx_hit_quick.png`) ;
  quand il joue, les éclats codés (`pixelImpact`) ne sont plus dessinés. L'ultime du 01 avait un fourreau inventé :
  refait sans. Les 14 autres effets attendent l'accord de yoyo sur la capture du coup rapide.
- 06/11 15 h 05 UTC, session cloud « Hakuen (01) » : le nouveau 01 (`h01`, Hakuen) en bêta à côté des anciens,
  kit codé (Rengaine → Coupe-éclair ou Coupe, volonté, éveil, Prescience à 3 parades ; test `sim_test hakuen`).
  Clips PixelLab dans `tools/pixellab/pxa/h01/` (les PNG y sont maintenant suivis par git) : 21 clips refaits
  parce que le moteur avait collé flammes, traits ou fumée (yoyo : « aucune poussière grise, sinon tu refais ») ;
  `dustscan.py` les repère. **Répartition des outils (yoyo, 06/11 14 h)** : PixelLab = persos et armes (et fumées
  de déplacement) ; ParticleFX = coups, coup critique, bouclier, ultimes, ondes d'impact, compétences, états. Son
  plan Starter ne paie pas les appels MCP (packs seulement) et il ne veut rien payer de plus : il colle lui-même
  les 15 prompts de `docs/refonte/prompts_vfx_particlefx.md` dans le studio et met les GIF dans son dossier VFX.
- 06/11 12 h 50 UTC, session cloud « Refaire 10 persos » : **11.9**, deux GIF ParticleFX du PC de yoyo
  (`Documents/Projets/Clash Horizon/Art/VFX`) pas encore en jeu : `pfx_spiral_sphere` (neutre mains nues du 02,
  « Sphère de givre ») et `pfx_giant_sphere` (ultime « Apocalypse » du 03-B). Le halo des `pfx_*` (`glowOf`) baisse
  à l'intérieur des formes pleines, qui brûlaient en blanc. Tout validé par yoyo dans le Claude Doc (12 h 18), mais
  **rien n'est généré** sur PixelLab ni ParticleFX avant qu'il prenne ses abonnements.
- 05/11 12 h 00 UTC, session cloud « VFX ParticleFX » : **décision de yoyo : les nouveaux effets viennent de
  ParticleFX Studio, PixelLab reste pour les persos** (CLAUDE.md mis à jour). Licence lue sur leur page des prix :
  jeu vendu permis même en compte gratuit, sans crédit, pas de revente en pack ; en gratuit, les effets enregistrés
  sont publics dans leur galerie. Le texte complet des CGU (particlefx.studio/terms) n'a pas pu être lu (page
  rendue en JavaScript).
- 05/11 11 h 35 UTC, session cloud « VFX ParticleFX » : **v11.3 publiée** (workflow_dispatch sur
  `claude/project-thread-y4vuco`, archives Linux et Windows). Verrous « Effets » et « Version et paquets » libérés.
  Les persos (01 et 08) ont été choisis par Claude : à changer si yoyo le veut. Licence commerciale de ParticleFX
  Studio encore à vérifier avant la vente.
- 06/11 12 h 15 UTC, même session : yoyo confirme que les 10 nouveaux persos remplacent les persos actuels, et que
  de PixelLab on ne garde que les effets de déplacement ; le reste vient de ParticleFX, que Claude pilote maintenant
  par MCP (`tools/pixellab/tools/pfxmcp.py`, hôte mcp.particlefx.studio autorisé dans l'environnement cloud). Rien
  n'est généré avant que tout soit prêt et validé.
- 06/11 12 h 10 UTC, même session : compétences et ultimes des 10 écrits dans le Claude Doc des fiches (01 d'après
  Alpha de PGR, demandé par yoyo ; 02 à 10 proposés). Armes de la planche v2 repeintes aux couleurs de chaque perso :
  `docs/refonte/ref_armes_v2_normales.png` et `ref_armes_v2_alt.png` (formes inchangées ; 01, 02, 05, 06 et 09
  étaient dessinées aux couleurs alternatives, 03 et 04 à aucune des deux). À valider par yoyo.
- 06/11 04 h 35 UTC, même session : `scripts/recolor.py` (gratuit) repeint les images d'un perso NN dans ses
  couleurs alternatives (palette 2 des matchs miroirs), apprises sur les deux planches v2 (`ref_persos_v2.png` et
  `ref_persos_v2_alt.png`) : chaque pixel est rangé par sa couleur, la couleur autour de lui et sa hauteur dans le
  dessin, et garde son ombrage. `--test` repeint les 10 persos de la planche normale et les compare à la planche
  alternative (rendu envoyé à yoyo : `/mnt/project-files/refonte_v2/palette2_test.png`). Le jeu ne lit pas encore la
  palette 2 (`drawCharacter` ignore `palette`) : à brancher quand les clips des nouveaux persos existeront. Limite :
  03 (veste et cheveux blancs) et 07 (cheveux et manteau noirs) sont à vérifier sur leurs vrais clips.
- 06/11 04 h 20 UTC, même session : **11.8 publiée** (workflow_dispatch, archives Linux et Windows), lumière façon RTX en shaders, sans dépendance
  (`src/game/render_light.cpp`, « Qualité graphique : Haute » seulement). `light::backdrop` et `light::arena` : les
  lumières des effets `pfx_*` (`art::lastLights`) éclairent le fond et les plateformes (mélange dst × (1 + lumière)),
  en ellipse sur le sol vu en profondeur ; ombres douces des persos à l'opposé de chaque lumière, coupées à la bande du
  sol. `light::reflections` (après les persos et les effets, avant les noms) : reflet du fond (copie de l'image avant
  les persos) et des persos et effets (copie de l'image finie) sur les sols brillants, en mélange « écran », jamais
  sur un pied ou un effet posé sur le sol. Réglages par arène dans `render_stages.cpp` (`Arena` : bords du sol,
  brillance, pluie, gain de lumière : nuit 1,6, jour 0,5) et `stageart::floorOf` / `lightGain`. Ombre sous les pieds
  adoucie (dégradé). Verrou « Version et paquets » pris et libéré dans ce commit.
- 06/11 03 h 35 UTC, même session : **11.7 publiée**. Fiches des 10 nouveaux persos de la planche ChatGPT (noms,
  histoires, idle, marche, expressions, armes A et B) dans le Claude Doc « Clash Horizon : fiches des 10 nouveaux
  persos », à valider par yoyo. Le jeton PixelLab de cette session cloud est dans la variable `PIXELLAB_TOKEN`
  (`pxcli.py` la lit si `px/.token` manque) : 172,9 générations jusqu'au 13/11, aucun job en cours, rien lancé
  (accord de yoyo par perso). Commence : lumière façon RTX en shaders (11.8).
- 06/11 03 h 30 UTC, même session : **11.7**, deux GIF ParticleFX envoyés par yoyo (sources `art/particlefx/sparkle.gif`
  et `stun_stars.gif`) : `pfx_sparkle` (`import_fx.py --step 3 --loop` : 20 images de 326 px, sous 8192 px) autour du
  perso pendant son ultime, en plus de ses effets, et `pfx_stun_stars` (`--loop`, 30 images) au-dessus de la tête d'un
  perso sonné (`ShieldBreak` au sol), à la place des étoiles dessinées par le code (supprimées). `drawFx` et
  `drawFxFrame` prennent `castLight` : ces décorations ont leur halo mais n'éclairent pas leur perso. 11.6 publiée.
- 06/11 03 h 05 UTC, même session : **11.6**, lumière directionnelle (yoyo : la lumière doit venir de la direction
  de l'effet, et l'ombre à l'opposé). Chaque image d'un effet `pfx_*` éclaire depuis le centre de ses pixels clairs,
  sur leur étendue, avec une force selon leur quantité (`Fx::beams`, calculés au chargement) ; plus de clignotement
  fixe. Le shader de teinte reçoit la direction de la lumière dans la texture (rotation et miroirs défaits) et le
  corps (`body` : centre, hauteur) : la partie tournée vers l'effet prend sa couleur (dégradé des sombres vers le
  blanc) avec un bord net, plus large quand la lumière est forte (tout le corps si elle vient de partout), le reste
  s'assombrit, et le contour face à la lumière prend un liseré. Portée : une hauteur de perso au-delà de l'effet.
  11.4 et 11.5 publiées (workflow_dispatch, archives Linux et Windows).
- 06/11 02 h 37 UTC, même session : **11.5**, les effets `pfx_*` éclairent les persos proches (demande de yoyo, d'après
  une planche où le perso devient cyan sous un éclair). `drawFxFrame` note chaque effet dessiné (centre, rayon,
  couleur moyenne saturée) ; `drawSheet` lit les lumières de l'image courante et de la précédente (`art::newFrame`,
  appelé par `App` avant chaque image) et passe `lit` au shader de teinte : couleur de l'effet, parties claires vers le
  blanc, clignotement 2 images sur 3. Les armes tenues ne sont pas éclairées.
- 06/11 02 h 28 UTC, session cloud « Refaire 10 persos » (branche `claude/project-thread-58u7pf`, partie de la 11.3) :
  **11.4**, halo lumineux des effets ParticleFX. Leurs GIF n'ont qu'une transparence tout ou rien : la lumière du site
  (particules additives, bloom) est recréée au chargement (`glowOf`, `fighter_art.cpp` : chaque image complétée de 32 px,
  prémultipliée, réduite par 4, floutée) et dessinée en additif par-dessus l'effet, deux fois, avec « Qualité : Haute ».
  Seuls les effets `pfx_*` l'ont. Les ombres au sol des persos existaient déjà (`drawGroundShadow`).
- 05/11 11 h 05 UTC, session cloud « VFX ParticleFX » (branche `claude/project-thread-y4vuco`, partie de la 11.2) :
  **11.3**, deux effets ParticleFX Studio choisis par le propriétaire (GIF gratuits, sources dans `art/particlefx/`) :
  - `pfx_crescent` sur la compétence neutre du 01 avec l'odachi (« Croissant du néant ») : le croissant est recentré
    image par image (`import_fx.py --track`) et joué une fois sur le vol du projectile.
  - `pfx_sword_rain` sur l'ultime du 08 avec l'épée (« Jugement céleste ») : nouvelle forme `ult::downpour`, une zone
    par adversaire qui frappe toutes les 12 images (5 fois), puis une explosion qui éjecte (40,5 % à 30 %, KO à 130 %).
  - `scripts/import_fx.py` lit les GIF (`--step`, `--scale`, `--track`). Les effets `pfx_*` s'affichent même avec
    `kVfx = false` (`chosenFx`, `render_world.cpp`). Les anciens `p01_a_n` et `p08_a_ult` ne servent plus.
  - Imports : `pfx_crescent art/particlefx/crescent.gif --only 4-13 --track --anchor 0 344 --hit 105 --fps 12` (l'image
    3 du GIF est une silhouette noire qui clignote en jeu) et `pfx_sword_rain art/particlefx/sword_rain.gif --step 2
    --scale 0.5 --loop --anchor 286 476 --hit 69`.
  Verrous « Effets » et « Version et paquets » pris.
- 05/11 03 h 05 UTC, session cloud « DA v2 » : **v11.2 publiée** (workflow_dispatch sur `claude/project-thread-dj46dj`,
  archives Linux et Windows). Verrou « Version et paquets » libéré. Les 4 livraisons de la DA « Ligne d'horizon » sont
  faites ; suite selon les retours de yoyo.
- 05/11 02 h 50 UTC, session cloud « DA v2 » : **11.2** (4e et dernière livraison de la DA) : résultats
  (`screen_results.cpp` : lumière de victoire, classement avec KO, chutes, dégâts et orbes, récompenses, revanche quand
  chaque joueur est prêt ; stats suivies dans `trackCallouts`), paramètres (`screen_settings.cpp` : onglets avec
  icônes, aide de la ligne choisie, cartes en direct « volume réel », « aperçu en direct » et « règles des prochains
  combats », une seule table des commandes : clavier 1, clavier 2 et les deux boutons de la manette).
  Verrou « Version et paquets » pris.
- 04/11 23 h 30 UTC, session cloud « DA v2 » : **v11.1 publiée** (workflow_dispatch sur `claude/project-thread-dj46dj`,
  archives Linux et Windows). Verrou « Version et paquets » libéré. Suite : résultats et paramètres (11.2).
- 04/11 23 h 15 UTC, session cloud « DA v2 » : **11.1** (3e livraison de la DA) : HUD (`render_hud.cpp` : cartes des
  joueurs, bandeau de l'orbe, badge « ULTIME PRÊT », bulles hors écran), Clash Orb redessinée (`drawClashOrb` dans
  `render_world.cpp`, aussi en icône), éclat qui vole de l'orbe au gagnant et dégâts « +N % » (`trackCallouts` /
  `drawCallouts` dans `match_screen.cpp`), pause et liste des coups (`match_pause.cpp`, textes `kMoveText` pour 01, 02,
  03, 07, 08), options depuis la pause. Verrou « Version et paquets » pris.
- 04/11 22 h 45 UTC, session cloud « DA v2 » : **v11.0 publiée** (workflow_dispatch sur `claude/project-thread-dj46dj`,
  archives Linux et Windows). Verrou « Version et paquets » libéré. Suite : HUD, Clash Orb, pause et liste des coups.
- 04/11 22 h 30 UTC, session cloud « DA v2 » : **11.0** (2e livraison de la DA) : choix des combattants
  (`menus.cpp` : combattants sur la ligne d'horizon, fiches dessous, places libres en puces « Rejoindre »), choix de
  l'arène (plan des plateformes), nouvel écran VS (`screen_versus.cpp`, entre l'arène et le combat, sauf en
  entraînement), bouton doré « COMBATTRE ! » dans `ui::footer`, couleurs des joueurs (J1 azur, J2 or, J3 vert,
  J4 rose ; un CPU garde la couleur de sa place). Fiches des persos (titre, art martial, style, gabarit) :
  `fiche()` dans `screen_gallery.cpp`. Verrou « Version et paquets » pris.
- 04/11 22 h UTC, session cloud « DA v2 » : **v10.9 publiée** (release faite à la main, workflow_dispatch sur
  `claude/project-thread-dj46dj`). Verrou « Version et paquets » libéré. Suite : sélection, arènes et écran VS.
- 04/11 ~22 h UTC, session cloud « DA v2 » (branche `claude/project-thread-dj46dj`, partie de la 10.8) : le
  propriétaire a validé la nouvelle DA « Ligne d'horizon » (canevas v2, voir la mémoire du projet). Codée en 4
  livraisons. **10.9** : la charte dans `ui.cpp` (polices Saira Condensed et Saira, panneaux en biseau, ligne
  d'horizon ; `fontScale`/`fontRaise` de `common.hpp` gardent les mises en page faites pour les anciennes polices),
  l'écran titre v2 et son combat de démo (mode « attract » de `MatchScreen` : CPU niveau 9 sur l'Abîme Lunaire, sans
  son, caméra fixe sur la terrasse de `menu_horizon.png`, nouvelle paire à chaque K.O.). Ensuite : sélection, arènes
  et VS ; HUD, orbe, pause et liste des coups ; résultats et paramètres.
- 28/10 ~23 h UTC : **v10.7 publiée**, verrou libre. **Le propriétaire met le projet en pause** (« on fera un autre
  jeu »). Après la 10.7, poussé mais pas publié : l'éclat d'impact (l'éclat du saut, tourné selon le coup, bord foncé
  de la couleur du coup) et la traînée de fumée derrière un perso éjecté vite (`Fx::update`). La traînée n'a pas pu
  être vue nettement sur capture. Essai « façon Rivals » (persos de 48 px, décor à pixels fins) refusé : il voulait
  parler de la dynamique des coups et des effets, pas du style.
- 28/10 ~23 h UTC : **v10.7** : `kVfx = false` à nouveau ; restent les fumées de mouvement (`SpriteFx::move`, tracées
  par `Fx::drawImpacts`), la bulle de bouclier (toujours) et les éclats en pixels. Le propriétaire a choisi le plan
  « perso témoin » (un perso et une arène au niveau Rivals of Aether : même taille de pixel, palette fixe, clips relus,
  effets et sons soignés) avant de refaire les autres.
- 28/10 ~22 h 30 UTC : **v10.6 publiée** (run réussi), verrou de version libre.
- 28/10 ~22 h 15 UTC, session cloud : v10.5 publiée (run réussi). **v10.6 : effets remis** à la demande du
  propriétaire (« utilise les vfx de manière intelligente ») : `kVfx = true`. Images d'impact noir et blanc et flash
  seulement si le coup a l'air mortel (`lethal` : finisher à 140 de recul ou plus, ou 200 et plus) et aux KO ; gerbe
  au sol (taille 0,5) à 140 de recul ou plus ; bruit sourd sous les coups lourds non mortels ; l'étoile en traits
  des éclats ne sort que si les effets sont coupés (l'étincelle PixelLab la remplace).
- 28/10, session cloud (branche `claude/wonderful-davinci-gr7v02`) : **v10.5, des coups qui claquent.**
  - Impacts légers en pixels dessinés par le code (`Fx::pixelImpact`, `Fx::drawImpacts`), visibles malgré
    `kVfx = false` : éclats carrés à contour sombre et étoile de traits, selon la force ; aussi sur les heurts
    (clank, bouclier, chute au sol). La victime tremble pendant le hit-stop, le long de son éjection.
  - Secousse plus brève (0,8 par image) ; coups lourds plus graves, avec un bruit sourd.
  - Compétences N, côté et bas : 3 s de recharge minimum (`kSkillCooldown`). Ultime : seulement la Clash Orb.
  - Caméra reculée (×0,87, zoom max 1,15). Double saut : le clip de saut est rejoué (les `airjump` sont des saltos).
  - 03 : M1 de l'arme B et compétence N de l'arme A régénérés, idle à 9 i/s.
  - 02-B, catégorie bâton (règle du propriétaire : façon Wukong ; ni faux ni lances) : M1 1-3 et M1 aérien régénérés.
  - Traînées (règle : à mains nues, coups rapides seulement ; avec une arme, tous les coups) : 02 `w0_jab`,
    07 `w0_jab2`, `w0_jab3`, `w0_nair`, `w1_jab3`, 08 `w0_jab2` régénérés avec traînée.
  - Armes au sol des 5 persos redessinées par PixelLab (pixflux, `tools/pixellab/icons/make_icons.py`).
  - Reste :
    - Traînées des smashs à mains nues : 9 régénérations « sans traînée » (01 usmash, 02 fsmash/dsmash, 03
      fsmash/dsmash, 07 fsmash, 08 fsmash/usmash/dsmash) en ont toutes remis une (et parfois éclats et poussière) :
      jetées, les anciens clips restent. Tilts, aériens et attaque en course gardent la leur (coups rapides).
    - Clip `p03_dspecial` (mains nues) avec des gerbes jaunes collées.
    - Équilibrage (simulation) : 07 à 37 % (42 % sans la recharge des compétences), les autres entre 51 et 54 %.
- 22/10 ~0 h 25 UTC, session cloud : **v10.4 publiée, effets en pause** (demande du propriétaire, déçu des effets) :
  `kVfx = false` dans `src/game/render.hpp` coupe étincelles, fumées, auras, traînées, flashs, cadres d'impact et
  balise d'arme ; projectiles et rayons en formes blanches. Les effets refaits d'après ses maquettes restent garés
  (`tools/pixellab/vfx/maquette_*.py`, `skill_gen.py`, `gen/`). M1 aérien avec arme du 03 (A) et du 08 (A, B)
  régénérés (entailles visibles), fitall repassé. Verrou de version libre. **Ne plus générer d'effets PixelLab.**
- 21/10 ~21 h UTC, session cloud : **v10.3 publiée** (M1 gardé 8 images pendant l'élan du saut et la réception ;
  avec une arme, Haut + M1 dans l'élan donne le M1 aérien). Verrou de version libre. Maquettes d'effets du
  propriétaire (style Rivals of Aether) dans `docs/refonte/vfx_maquette_*.png` : planche 1 (coups, sol, saut, dash,
  atterrissage) aux couleurs fixes pour tous ; planches 2 à 4 (compétences, foudre) et bulle de bouclier à
  recolorer. Pour la mise à jour suivante (10.4).
- 21/10 ~20 h 40 UTC, session cloud : **v10.2 publiée** (workflow lancé à la main, réussi). Verrou de version libre.
  Ensuite : M1 sans animation selon l'arme (signalé par le propriétaire), au sol et en l'air.
- 21/10 ~20 h 30 UTC, session cloud : **plateformes en profondeur, sans coupure** (façon Smash, dessus visible).
  Une image PixelLab par plateforme (`pxa/arenas/gen_platforms.sh`, passes 1 à 4) ; `scripts/import_art.py` refuse
  toute image qui touche le bord de son cadre (coupée). Ligne de marche au milieu du dessus (`render_stages.cpp`).
  Armes : la touche et la barre s'affichent aussi sur la 2e arme ; prise en 1 s (`kPickupFrames = 60`).
- 21/10 ~20 h UTC, session cloud : **v10.1 publiée** (workflow lancé à la main, réussi ; archives ClashHorizon-*).
  Verrou de version libre. Ensuite : plateformes avec un effet de profondeur (dessus visible, façon Smash).
- 21/10 ~20 h UTC, session cloud : **v10.1, le jeu devient Clash Horizon** (demandes du propriétaire).
  - Nom et logo (`docs/refonte/logo_clash_horizon.png`, détouré par `scripts/import_art.py`) ; paquets
    `ClashHorizon-*`, exécutable `clash_horizon` dans l'archive, réglages dans `clash_horizon.cfg`.
  - Nouvelle DA des menus (Brawlhalla / Clash of Clans, bleu roi et or) : `ui.cpp` (boutons épais, cartes, rubans,
    widgets) et tous les écrans. L'ancienne DA « TotK » est abandonnée.
  - Deux armes par perso : A et B tombent séparément ; l'alternance n'existe qu'avec les deux (`Fighter::weapons`).
  - Plateformes des arènes : une image PixelLab par plateforme à la taille du jeu (`pxa/arenas/gen_platforms.sh`).
  - Effets visuels : le propriétaire enverra une maquette (style Rivals of Aether 1) ; ne pas les refaire avant.
- 21/10 ~18 h UTC, session cloud : **bugs signalés par le propriétaire après la v10.0** (branche `cool-sagan`).
  - Effets PixelLab de déplacement et d'impact invisibles : `Fx::drawWorld` sortait avant de les dessiner quand
    aucune particule lumineuse n'était en vie. Corrigé (ils s'affichent toujours).
  - Effets figés sur leur dernière image (tous importés « sans boucle ») : projectiles (l'orbe du 01), rayons et
    auras bouclent avec `art::fxLoopTime` ; les colonnes (`PV::Pillar`) jouent une fois depuis leur apparition.
  - Clips déformés par l'ancien squelette (rebond en course, roulade qui tourne, chute au sol couchée de travers) :
    `drawCharacter` dessine les clips « à plat » comme avant la refonte, avec le décalage de la montée au rebord.
  - Anneaux d'ulti dessinés par le code retirés (l'aura PixelLab s'affiche déjà) ; teintes de coup, de charge et de
    contre adoucies (le perso devenait une silhouette unie).
  - Arme au sol : portrait du propriétaire (il était découpé en coordonnées monde), touche Arme du joueur et barre
    du maintien de 2 s (`drawItemPrompts`). Nom de l'arme et compteur de combo au-dessus des étiquettes J1/CPU.
- 21/10 ~17 h 05 UTC, session cloud : **v10.0 publiée et vérifiée** (workflow lancé à la main sur
  `claude/cool-sagan-8wiysw`, absente des branches qui le déclenchent ; réussi ; l'archive Linux téléchargée a la
  somme SHA-256 de la release et contient v10.0). Verrou de version libre.
- 21/10 ~17 h UTC, session cloud : **règle des tenues remplacée** (demande du propriétaire). Tous les persos sont
  adultes ; chaque nouveau perso reproduit exactement sa référence PixelLab, tenue comprise, sans rien couvrir ni
  changer (`CLAUDE.md`, `FICHES.md` §1.5 ; les « Tenue à corriger » des fiches à venir sont retirées). Les 5 persos
  faits restent tels quels. Aucun perso généré ni modifié.
- 21/10 ~17 h UTC, session cloud (ponytail) : **v10.0** (demande du propriétaire).
  - Ramasser son arme : se tenir dessus et **maintenir la touche Arme 2 s** (`kPickupFrames`, compte sur
    `WeaponItem::hold`, un coup le met en pause). L'IA continue de tenir une fois dessus (`fetchWeapon`).
  - **Barre d'ulti retirée** du HUD (`render_hud.cpp`) ; « ULTI PRÊTE ! » reste.
  - `sim_test balance` : 40 duels par paire sur toutes les arènes (les 3 duels sur l'arène 0 donnaient 33 à 67 % au
    hasard) ; `k.P` recalés, victoires 48-52 %. Le test des armes des ordinateurs dure 90 s (médiane ~35 s pour s'armer).
  - Notices `LISEZ-MOI` : ancien roster et ancien ramassage retirés. Version 10.0, verrou « Version et paquets » pris.
- 21/10 ~16 h UTC, session cloud (ponytail) : **les 5 persos sont complets, importés, recalés et équilibrés.**
  - Tous les clips relus ; 7 clips du 08 qui gardaient poussière et éclats après 3-4 essais sont **nettoyés et gardés**
    (décision du propriétaire) : `pxa/qa_fixes.json` (`specks`, `erase`, `degrey`, nouveau `dust`, `replace`).
  - Les reprises d'un clip déjà réécrit reçoivent la fin de prompt stricte (`clips.py`, sans traînée ni effet).
  - `import_all.py` sur les 5, `fitall.sh` (corrigé : ne dépend plus d'un ancien scratchpad) : 84 impacts recalés,
    73 hitboxes ; `ctest` passe. Équilibrage : `k.P` et `k.c.weight` par perso dans `roster.cpp`, victoires 46-54 %.
  - Dash et chute : 3 clips chacun par perso, chargés ; le jeu les joue (vu en démo, pas de capture image par image).
  - **Ensuite** : release v10.0 avec l'accord du propriétaire ; écran OPTIONS encore dans l'ancien style.
- 21/10 ~10 h UTC, session cloud (ponytail), branche `claude/cool-sagan-8wiysw` : **arènes et portraits faits avec
  PixelLab ; relecture de 07 et 08.** (Le conteneur avait redémarré : driver relancé, `fetch_orphans.sh` a récupéré 6
  jobs du 08, rien renvoyé.)
  - **Les 5 arènes remplacent les 11 anciennes** (Zonai et mondes d'animes) : Abîme Lunaire (01, disposition d'un seul
    sol), Citadelle de Givre (02, trois corniches), Canyon Écarlate (03, deux sols et un gouffre), Toits Kurenai (07,
    long sol et deux poutres), Monastère des Nuées (08, îlots et planche mobile). `src/core/stage.cpp` (`stageList`),
    thèmes `Lunar`…`Clouds`. Fond `create_image_pro` 688×384 (style : `pxa/arenas/style_menu.png`, coin des torii de
    `menu_fond_1`), « kit » de plateformes 480×160 dans le style du fond, importés par `scripts/import_art.py`
    (`assets/sprites/arena_<clé>_bg|solid|thin.png`, ×2 au plus proche voisin ; il affiche le haut marchable de chaque
    plateforme, à reporter dans `render_stages.cpp`). Rendu : `render_stages.cpp` (≈70 lignes au lieu des 1 900 des
    mondes d'animes), plateformes étirées en 3 parties (`art::drawPicture3`). Le kit de Kurenai est encore en file.
  - **Portraits** : `create_image_pro` 168×168 avec la garde du jeu en référence (4 candidats, 15 générations) ; choix
    dans `scripts/import_art.py` (`PORTRAITS`), liseré blanc du bas effacé. Tenues du jeu, aucune sexualisée.
  - Relu : toutes les bases de 07 et 08 validées ; ~35 clips écartés (énergie, poussière, éclats, traînées coupées) et
    refaits avec des prompts calmes (`qa_prompts.json`) ; prompts communs corrigés (roulade, balayages, attaque depuis
    le rebord : plus de « along the ground »).
  - **À faire** : importer le kit de Kurenai (`python3 scripts/import_art.py`, puis reporter ses hauts), retirer le code
    mort des anciennes arènes (`render_world.cpp` : `bakeDepths/Sanctuary/Sky`, `drawSolid/Thin`, thèmes `Cosmic`…),
    import des clips (`import_all.py`), `fitall.sh`, équilibrage.
- 21/10 ~1 h 20 UTC, session cloud (ponytail), branche `claude/cool-sagan-8wiysw` : **reprise après la passation.**
  - Jeton remis dans `px/.token`. Compilation et `sim_test` : « all tests passed ». Codesign connecté ; Asset Generator
    (blueprint-studio) et Icons8 demandent une autorisation dans les connecteurs de claude.ai.
  - `fetch_orphans.sh` : 10 jobs du 03 (w0_bthrow, dthrow, fthrow, sspecial, ultimate, uspecial, uthrow, w1_idle,
    runstart, walk) « Generation stalled and was automatically failed » chez PixelLab, absents de la galerie :
    passés en `rejected`. 6 jobs encore en cours (01 bthrow, w1_dspecial ; 02 w0_dthrow ; 03 w1_crouch, land, jump) :
    relancer `fetch_orphans.sh` plus tard.
  - Lancé : `driver.py p01 p02 p03 p07 p08`. Les 25 clips `p02/w1_*` (bâton) étaient refusés avant l'envoi
    (description du perso à 302 caractères, limite 300) : « with blue ribbons » → « with ribbons »
    (`new_specs.py`, `wkits/p02.json`), renvoyés à part (`pxa/jobs_p02w1_fix.json`).
  - **Bloqué** : portraits peints et arènes. Le compte IMG.LY (Codesign `asset_generate`) n'a plus de crédits ;
    Asset Generator et Icons8 ne sont pas autorisés (connecteurs de claude.ai) ; SpriteCook à 0.
  - **En attendant** : `pXX_still.png` et `pXX_portrait.png` des 5 refaits à partir des gardes du jeu
    (`pxa/base/<id>_guard256.png`, ×2 au plus proche voisin) : les anciens venaient de la planche (02 et 07 en
    brassière, culotte et jarretelles). Pause, résultats, pied de page et HUD du combat dans le style du menu
    principal (`ui::menuPlate`, `ui::kRed`) : plus d'anneau Zonai, de disque de pierre ni de filets dorés.
  - **Arènes (décision du propriétaire, 21/10)** : seulement 5 pour l'instant, une par perso, fonds peints dans le
    style des menus, dispositions des plateformes gardées : Abîme Lunaire (01), Citadelle de Givre (02), Canyon
    Écarlate (03), Toits Kurenai (07), Monastère des Nuées (08). À faire dès que la génération d'images marche.
  - Relecture : bases validées 03 w0_knockdown, w1_jump ; 02 w1_jump, w1_runstart, w1_knockdown, w1_holdstart
    (refaite : le bras partait en arrière). Écartés (`qa3`, prompts dans `qa_prompts.json`) : 03 w0_fthrow,
    w0_getup, w1_crouch, w1_land ; 01 w1_dspecial ; 02 w0_dthrow. 03 w1_runstart bloqué de nouveau chez PixelLab :
    renvoyé à part (`pxa/jobs_p03_runfix.json`). 03 dair gardé (geste faible, mais propre).
  - **Propriétaire (21/10)** : « j'autorise tout sauf dépenser de l'argent pour des crédits ». Portraits et arènes
    faits avec PixelLab (générations de l'abonnement, 1 519 restantes) : `create_image_pro` (`pxcli.py image --model pro`,
    `--bg` pour un décor, `--style` pour le style des menus `pxa/arenas/style_menu.png`). Essais : portrait du 03
    (`pxa/portraits/p03`), fond du Monastère des Nuées (`pxa/arenas/nuees_bg`).
  - Prompts communs calmés dans `clips.py` (défauts revenus chez plusieurs persos) : projection vers le bas armée,
    double saut (« kicks off the air »), fin des coups (`TAIL_BLOW` : arme et traînée dans l'image, sans éclair),
    coups pendant une prise (« strike »).
- 21/10 ~2 h UTC, session cloud (ponytail), branche `claude/cool-sagan-8wiysw` : **PASSATION à une nouvelle session**
  (pour les plugins Codesign, Asset Generator et Icons8, connectés seulement au démarrage d'une session).
  - **Arrêté proprement** : driver et lots arrêtés, tout est poussé. 16 jobs ont un numéro (`"submitted"`), dont 6
    encore en cours chez PixelLab : **d'abord** `bash pxa/fetch_orphans.sh` (relancer plus tard pour ceux encore en
    cours), **puis** `nohup python3 pxa/driver.py p01 p02 p03 p07 p08 >> pxa/driver_all.log 2>&1 &`. Les 29 clips
    `"rejected"` sont repris par le driver (prompts calmes). Aucun job sans numéro.
  - **Jeton** : dans l'environnement, `PIXELLAB_TOKEN` → l'écrire dans `tools/pixellab/px/.token` (ignoré par git).
    Clé IMG.LY pour Codesign : `IMGLY_GATEWAY_API_KEY` (génération d'images sans navigateur). Asset Generator et
    Icons8 : autorisés dans les connecteurs de claude.ai.
  - **État** : 01 137/141, 02 97/141, 03 76/141, 07 et 08 1/141 (leurs clips sont en file). 1 665 générations.
  - **Leçons de cette session** :
    - Un clip écarté (`retire.py`) pendant que le driver tourne reçoit un `job.json` `{"status": "rejected"}`, sinon
      ce driver ne le refait pas. Un lot urgent (bases bloquantes) part à part : `pxa/jobs_*_fix.json` + `pxbatch.py`.
    - **Panne PixelLab** (`QueuePool limit`, base injoignable) : un job peut être marqué `failed` alors qu'il tourne
      encore. Avant de le remettre en file, comparer à `pxcli.py jobs` ; un envoi coupé (`"submitting"`, sans
      numéro) se cherche dans la galerie (`list_images` avec `search`) avant d'être renvoyé ; un clip fini se
      récupère par `gallery_frames`.
    - Prompts communs rendus calmes (`clips.py`) : `fall` (une pose tenue, plus de « squat »), `dash` (nouveau),
      `jump` (finit en l'air : sa dernière image est la base des aériens), `knockdown`, `land`, prises et lancers,
      `ledgeattack`. Les mots d'action (« hurls », « slams », « impact », « along the ground ») font dessiner des
      effets.
  - **Fait aussi** : effets des compétences (79, importés), dash avant la course (`kSheetDash`), menu principal et
    sélection dans le style des maquettes, logo refait (`tools/menu/`).
  - **Ensuite** : relire les clips à mesure, import (`import_all.py`) puis `fitall.sh` et l'équilibrage quand les 5
    sont complets ; portraits peints et nouvelles arènes originales (Asset Generator / Codesign) ; HUD, pause et
    résultats dans le style des menus.
- 20/10 19 h 15 UTC, session cloud (ponytail), branche `claude/cool-sagan-8wiysw` (= `claude/refonte-originaux` +
  ses commits) : **reprise des clips et des effets.** Jeton remis dans `px/.token` (ignoré par git).
  - `fetch_orphans.sh` : rien à récupérer. 01 w1_uspecial et w1_dspecial absents de la galerie (`list_images`
    `search`) : passés en `rejected`, le driver les refait.
  - Lancés : `driver.py p01 p02 p03 p07 p08` (348 clips prêts au départ, ~420 générations sur 1 960) et
    `skill_fx.py render` (les 79 effets, gratuit).
  - **Effets des compétences faits** : les 79 rendus, relus, exportés et importés (`assets/sprites/fx_p0*.png`,
    lignes `fx`). Rendu avec `fit_canvas` (`move_fx.py`) : 40 effets touchaient le bord de leur image ; l'ancre
    exportée suit l'agrandissement. Rayons de `t_burst` un peu raccourcis. Vérifié : compilation, `sim_test`
    « all tests passed », démo 07 contre 08 (ultime à l'arme A) où l'effet est joué. `shotPlaceholder` reste comme
    repli quand un sprite manque.
  - **Demandes du propriétaire (20 h 40)** :
    - Chute (`fall`, `w0_fall`, `w1_fall`) : plus de « squat » en l'air (elle partait des genoux repliés du saut,
      tendait les jambes et revenait). Nouveau prompt dans `clips.py` : une seule pose tenue, seuls les cheveux et
      les vêtements flottent vers le haut. Les 4 clips faits (01 ×3, 02) sont écartés (`.qafall`) et renvoyés à
      part (`pxa/jobs_fall_fix.json`) ; les autres persos auront le nouveau prompt directement.
    - **Dash** : un double appui fait d'abord un dash, puis la course. Nouveau clip `dash` (mains nues et armes,
      `clips.py`), joué sur les 12 premières images de `FState::Run` (`pickClip`, `kSheetDash`) ; sans clip, la
      course est jouée comme avant. Les 15 clips partent avec le driver.
  - **Menu principal refait** (`screen_title.cpp`, comme `menu_accueil.png`) : il remplace l'écran titre et
    l'ancien menu à 4 tuiles. Fond `menu_fond_1..3` (un par visite, `Settings::menuBg`, `menu_bg`), logo et
    icônes détourés de la maquette (`assets/sprites/menu_logo.png`, `menu_icon_*.png`), 8 boutons inclinés
    (EN LIGNE, PERSONNAGES et ARÈNES marqués « BIENTÔT » : pas encore d'écran), carte « Arène à la une »
    (K / X : entraînement dans cette arène), astuce avec sa touche, version et roue des options. Prochaine étape :
    la sélection (`menu_selection.png`), puis les autres écrans dans le même style.
  - Clips relus à mesure qu'ils arrivent ; un clip écarté pendant que le driver tourne reçoit un `job.json`
    `{"status": "rejected"}`, sinon ce driver ne le refait pas (il le garde dans sa liste des clips lancés).
  - Relu avant le jeton (clips déjà téléchargés). **Bons** : 01 nair, bair (quelques points blancs sous les pieds),
    usmash, dsmash, dashattack, pummel, uthrow, w0_fthrow, w0_bthrow, tout le kit w1 présent ; 02 idle, walk,
    runstart, land, crouch, shield, holdstart, roll, spotdodge. **Tache grise dans la traînée** (gardés, à ton avis) :
    01 fair, sspecial (déjà refait), w0_uthrow.
  - **Écartés (`qa2`), le driver les refera** : 01 uair, dair, fsmash, grab, fthrow, bthrow, dthrow (énergie,
    poussière, boule de feu ; nouveaux prompts sans mots d'action dans `pxa/qa_prompts.json`), 01 w1_jump (sa
    dernière image, base des aériens, est debout au sol), 02 knockdown (traits bleus).
  - **Bases validées** : 01 w1_runstart, w1_holdstart, w1_knockdown ; 02 runstart, holdstart.
  - Toujours à vérifier dans la galerie avant tout renvoi : 01 w1_uspecial, w1_dspecial (« submitting », sans job).
- 20/10 19 h 30 UTC, session A (cloud), branche `claude/refonte-originaux` : **PASSATION à une session sur le PC du
  propriétaire (plugin ponytail, absent des sessions cloud).** Tout est poussé, y compris les images brutes PixelLab
  et les fiches de jobs (`pxa/p0*/`, `pxa/base_src/`) : rien n'est à repayer. Aucun job PixelLab n'est en cours.
  - **Reprendre les clips** (dans `tools/pixellab`, jeton dans `px/.token`, jamais commité) :
    `bash pxa/fetch_orphans.sh` (télécharge ce qui est fini, ne renvoie rien), puis
    `nohup python3 pxa/driver.py p01 p02 p03 p07 p08 > pxa/driver_all.log 2>&1 &`. Le driver envoie ce dont la
    base est prête et validée (`pxa/approved.txt`). Ne pas tuer `pxbatch.py` : ses jobs en cours ne seraient plus
    téléchargés (il faudrait relancer `fetch_orphans.sh`). `pxcli.py wait` sait maintenant récupérer un vieux job
    par son numéro de galerie.
  - **État** : 01 112/138 clips, 02 11/138, 03, 07 et 08 0/138 (690 en tout, environ 1,2 génération par clip,
    10 jobs à la fois, 6 min par clip). Il reste 1 960 générations (recharge le 13/11).
  - **Bloqués, à vérifier dans la galerie PixelLab avant tout renvoi** : `pxa/p01/w1_dspecial` et
    `pxa/p01/w1_uspecial` (envoi coupé par un redémarrage, sans numéro de job). S'ils n'y sont pas, passer leur
    `job.json` en `"status": "rejected"` : le driver les refera.
  - **À refaire automatiquement** (écartés, le driver les relance) : `p01/sspecial` (prompt dans
    `pxa/qa_prompts.json`), `p02/knockdown`, `p02/spotdodge` (ancien prompt).
  - **Bases validées** : 01 jump, runstart, holdstart, ledgegrab, knockdown, w0_jump, w0_runstart, w0_knockdown,
    w0_holdstart ; 02 jump. À valider dès leur arrivée : les autres bases (runstart, knockdown, holdstart,
    ledgegrab, w0_*/w1_* de chacun). Une base validée s'ajoute à `pxa/approved.txt` (« p02/runstart »).
  - **Relu et bon (01)** : états de base, coups Penchak Silat (jab à dtilt), nspecial, uspecial, dspecial, ultime,
    kit de l'odachi (w0_*), états w0. **Pas encore relu** : aériens, smashs, dashattack, prises et lancers, w0
    lancers, kit des dagues (w1_*), clips du 02. Outil : `python3 review_strip.py OUT.png 0.8 pxa/p01/<clip>...`.
  - **Leçons de relecture** : un prompt avec des mots d'action (« hit », « dodge », « knocked ») fait dessiner des
    effets ; les prompts communs ont été réécrits. Un clip raté s'écarte avec `python3 pxa/retire.py qa1 p01 <clip>`
    (avec `--if-desc <début de l'ancien prompt>` pour ne jamais écarter la nouvelle version), un nouveau prompt se met
    dans `pxa/qa_prompts.json`. Traînées : gardées sur les coups et les épées.
  - **Import** : `python3 pxa/import_all.py p01` (déjà fait pour les clips du 01 présents à 17 h). Puis
    `fitall.sh` (session A seulement) quand les 5 sont importés, puis l'équilibrage (provisoire : 01 et 07 79 %,
    03 42 %, 02 33 %, 08 17 %).
  - **Effets** : ondes 1 à 5 faites et importées (impacts des coups puissants, `vfx/wave_build.py`). Effets système
    `sys_*` et `sys_aura` faits. Effets des compétences : générateur `vfx/skill_fx.py` (79 effets), gratuit,
    rendu interrompu à 67/79 et non exporté : relancer `python3 skill_fx.py render`, relire, puis `export` et
    `import`. Recette `sys_impact` écrite (lignes de vitesse des impact frames) : à rendre et à brancher dans
    `drawImpactFrame` (`src/game/match_screen.cpp`) à la place des lignes dessinées.
  - **Reste dessiné par le code** : lignes des impact frames, `KoBeam` (code mort à retirer), replis de `fx.cpp`
    quand un sprite manque, `shotPlaceholder` (disparaît avec les effets des compétences), menus.
  - **Ensuite** : refonte des menus (fonds `menu_fond_1/2/3`, disposition de `menu_accueil`), démo regardée,
    release v10.0. Question ouverte pour le propriétaire : les décors d'arènes viennent encore des anciennes séries.
- 20/10 15 h 30 UTC, session A, branche `claude/refonte-originaux` : **styles de combat, traînées, ondes d'impact.**
  - Le propriétaire précise : sans arme, chaque perso se bat dans un art martial (01 Penchak Silat, 02 Taekwondo,
    03 Muay Thaï, 07 Karaté, 08 Kung Fu ; FICHES §1.7). Prompts dans `pxa/new_specs.py` (`STYLES`) ; les 8 coups
    génériques du 01 déjà faits sont écartés (`pxa/<id>/<clip>.generic`, `pxa/retire.py`) et refaits.
  - Traînées : gardées pour les coups rapides et les épées, interdites ailleurs (`TAIL_BLOW` / `TAIL` dans
    `clips.py`).
  - Ondes 1 à 5 : ce sont aussi les ondes d'impact des coups puissants (elles partent du sol). Refaites au plus
    près de la planche : image pro PixelLab avec le dessin en référence (`vfx/waves_src`), puis deux clips
    PixMiniMax, le jaillissement et la dissipation (`vfx/waves_anim_mm`), assemblés par `vfx/wave_build.py`
    (taches grises du modèle retirées, sol lumineux à demi transparent). Onde 1 importée ; 2 à 4 en cours ; onde 5
    refaite plus large (`w5d`, la première touchait les bords). Jouées à 1,5 fois la taille, teintées : sur un
    coup puissant, l'onde de l'attaquant monte sous la victime (`Fx::waveOf`).
  - Lots de clips : `driver.py p01 p03 p07 p08` et `driver.py p02` tournent (10 jobs à la fois chez PixelLab,
    environ 6 min par clip : 6 à 7 h pour les 690 clips).
- 20/10 14 h 45 UTC, session A, branche `claude/refonte-originaux` : **5 persos jouables pour l'instant, effets de
  déplacement faits, bases PixelLab prêtes.**
  - Le propriétaire veut d'abord 5 persos complets (armes et compétences inspirées des anciens persos, belles
    animations PixelLab, ses effets de déplacement et d'impact) : **01, 02, 03, 07, 08**. `roster()` ne rend
    plus qu'eux ; les kits des 12 autres sont dans l'historique (commit c349427). Tests adaptés.
  - Banque d'effets de déplacement de sa planche (`fx_move_*`, 18 effets, `vfx_workbench`, recettes dans
    `tools/pixellab/vfx/move_fx.py`) : sauts, atterrissage, course, dash, apparition, disparition, ondes 1 à 5 des
    transformations. Branchés dans `fx.cpp` (`Fx::moveFx`), teintés à la couleur du perso.
  - Bases : `pxa/base/<id>_guard256.png` (mains nues) et `<id>_w<slot>guard256.png` (arme en main, modèle de sa
    planche donné en référence), pieds alignés sur ceux de la garde ; `pxa/make_bases.py`, `pxa/anchors14.json`.
    Base du 08 à la hallebarde refaite (peau dorée) ; base du 02 refaite (short aux genoux sur collants opaques :
    l'ancienne montrait la cuisse et des lignes façon jarretelles).
  - Clips : `pxa/new_specs.py` écrit `specs_final/` et `wkits/` (prompts de mouvement seulement) ; `clips.py`
    ajoute partout « Only the character is drawn… », passe les prompts au féminin pour 02 et 07, et part de l'image
    armée exacte pour les clips d'arme. 138 clips par perso, 690 en tout. Essai de 8 clips sur le 01 en cours.
  - Équilibre provisoire (`sim_test balance`) : 01 79 %, 07 79 %, 03 42 %, 02 33 %, 08 17 %. À refaire après
    l'import des clips et `fitall.sh`.
  - Vérifié : compilation, `sim_test` « all tests passed ».
- 20/10 11 h 30 UTC, session A, branche `claude/refonte-originaux` : **étape 4 faite, nouveau système d'armes.**
  - Les armes tombent toujours sur l'arène, seul leur propriétaire peut les prendre. On les prend en se tenant
    dessus et en appuyant sur la touche **Arme** (arme A) ; marcher dessus ne suffit plus. Ensuite, la même touche
    alterne A et B, sans délai, jamais au milieu d'un coup (la pression attend 45 images). L'arme reste jusqu'au KO,
    sans minuteur.
  - Touches : `E` (clavier 1), `M` ou 7 du pavé (clavier 2), `Y` (manette, le saut reste sur X). Les anciens
    réglages où Y sautait sont migrés.
  - L'IA va chercher son arme, la prend avec la touche, puis alterne arme de distance et arme de mêlée.
  - Effets : prise et perte jouent les effets partagés `move_appear` et `move_vanish`, teintés à la couleur de
    l'arme ; un repli codé reste en attendant.
  - Le propriétaire autorise toutes les générations : **plan complet**, chaque arme B a son propre modèle.
  - Vérifié : compilation complète, `sim_test` « all tests passed ».
- 20/10 10 h 30 UTC, session A, branche `claude/refonte-originaux` : **étape 3 faite, les anciens persos sont
  retirés.** Le propriétaire a tout validé : plan recommandé, torses nus gardés, « EN LIGNE » grisé.
  - Cœur : 17 persos numérotés (P01 à P17, identifiants `p01`…`p17`) d'après `docs/refonte/FICHES.md` (mains
    nues, 2 kits d'armes et 9 transformations). Éléments, visuels de projectiles, ultimes et formes sont
    génériques. Chaque coup nomme son futur effet PixelLab `pNN_<x|a|b|t>_<n|s|u|d|ult|aura|burst>`.
  - Rendu : plus aucun dessin procédural des persos ni des armes. Un perso est dessiné par ses clips, sinon par son
    image provisoire `pNN_still.png`. Ses portraits viennent de `pNN_portrait.png`, ses icônes d'armes de
    `pNN_icon_a/b.png` (`scripts/crop_refs.py` découpe les planches du propriétaire).
  - Retirés : 1 520 images (clips, effets propres aux anciens persos, armes dessinées), `assets/fighters`,
    17 voix, `render_weapons` et cinématiques codées, shader du Kamui, données PixelLab des anciens persos.
    `qa_fixes.json`, `qa_prompts.json`, `subjects14.json` et `approved.txt` sont remis à zéro. Gardés : 15 effets
    génériques (`hit_*`, `explosion_*`, fumée, téléportation, contre).
  - Arènes : noms et textes neutres (plus de Konoha, Las Noches, etc.). **Leurs décors dessinés restent ceux des
    anciennes séries** : à refaire, question posée au propriétaire.
  - Vérifié : `cmake --build build`, `ctest` 1/1, `sim_test` « all tests passed ». Équilibre provisoire de 29 % à
    77 %, à régler après les clips. Captures sans écran du combat et de la sélection : correctes.
- 20/10 10 h 30 UTC, session A, branche `claude/refonte-originaux` : **étape 2 de la refonte faite.**
  - Fiches des 17 persos, de leurs 34 armes et des 9 transformations, plus les menus et le budget :
    `docs/refonte/FICHES.md`. Les images du propriétaire sont dans `docs/refonte/`.
  - CLAUDE.md est mis à jour : refonte, armes, effets, tenues.
  - **En attente de validation du propriétaire.** Aucune génération PixelLab, et pas de jeton PixelLab dans ce
    conteneur.
  - Suite après validation : étape 3 (retirer les anciens persos) et étape 4 (nouvelles armes).
- 20/10 09 h 45 UTC, session A (propriétaire) : **refonte complète décidée par le propriétaire** (droits
  d'auteur) : les 17 combattants actuels seront remplacés par 17 persos originaux, avec des armes invoquées par une
  touche, et tous les effets et menus seront refaits avec PixelLab. Ce travail se fait sur la branche
  `claude/refonte-originaux`, créée depuis celle-ci. Rien n'est changé ici ni dans `main`. Les fiches à valider
  sont dans `docs/refonte/FICHES.md` sur cette branche. Aucune génération PixelLab avant validation.
- 16/10 19 h 25 UTC, session B : **v9.9 publiée et vérifiée** (workflow réussi ; l'archive Linux téléchargée par
  l'API a la même somme SHA-256 que la release, affiche v9.9, et Link y a son Arc de lumière en match). Verrou de
  version libre. Tous les verrous sont libres.
- 16/10 19 h 05 UTC, session B : **lot 3 terminé, v9.9 en cours de publication** (kits de Link et de Gojo).
  `fitall.sh` : 28 clips retimés, 204 hitboxes recalées. Équilibre : Link 1.06 (36 % à 0.92), Gojo 1.26, Kratos
  1.10, chasseur 1.20 ; tous entre 45 et 55 %, 3 % des K.O. sous 70 %. Tests passés. Démos regardées (les 4 kits et
  les ultimes de Link et de Gojo) : armes visibles, flèches et orbes en place, aura des Six Yeux visible avec
  l'arme. Verrous imports, effets, équilibrage et `fitall.sh` libérés ; je prends « Version et paquets ».
  Prochain : les 8 combattants restants (Asta, Yuno, Byakuya, Kenpachi, Ulquiorra, Grimmjow, Starrk, Ganondorf).
- 16/10 18 h 40 UTC, session B : lot 3 généré et relu (Link 45 clips, Gojo 45 clips ; le tir à l'arc, la projection
  arrière et la pose saisie de Link refaits dans le nouveau look ; retouches dans `qa_fixes.json`, `degrey` sait
  maintenant prendre une fumée enfermée par les traits d'un effet : `"open": 0`, `"min"`). Clips importés. Je prends
  `fitall.sh` (session A inactive depuis le 14/10) : recalage, équilibre, puis v9.9.
- 16/10 17 h 50 UTC, session B :
  - **v9.8 publiée et vérifiée** (workflow réussi ; l'archive Linux téléchargée par l'API a la même somme SHA-256
    que la release, affiche v9.8, et les persos y sont nets en match). Verrou de version libre.
  - Lot 3 : code des kits de Link et Gojo poussé (e3a1f85), tests passés. Redémarrage du worker vers 17 h 40 :
    9 jobs de Link « submitted » seulement téléchargés (aucun « submitting »), chaînes relancées (Link : 4 clips
    restants ; Gojo : tout, garde des Six Yeux faite).
- 16/10 14 h 50 UTC, session B :
  - **Netteté des persos** (4fe2888) : la passe « pixel art » (`pix::sprite`, faite pour les persos dessinés par
    le code) réduisait chaque clip PixelLab à ~87 px de haut, écrasait ses couleurs et le recontournait : les
    persos étaient flous et pixelisés. Une pose tirée d'un clip est maintenant dessinée directement (match et
    menus), avec un biais de mipmap de -0,6. Je publie la **v9.8** pour ça seule.
  - **Lot 3 commencé (Link, Gojo)** : `pxa/wkits/link.json`, `gojo.json`. La garde de Gojo manquait dans `base/` :
    reprise de l'image 0 de son clip `idle` (galerie, gratuit). Chaînes : `chainl3.py` (saute
    `gojo/w1_ultimate`, déjà dans le jeu). Godot laissé de côté à la demande du propriétaire
    (`prototype_godot/` reste en l'état).
- 16/10 13 h 00 UTC, session B : **prototype Godot** demandé par le propriétaire, dans `prototype_godot/` (ne touche
  pas au jeu). Godot 4.4, rendu Compatibility : Naruto contre Sasuke avec les clips du jeu lus dans
  `assets/sprites`, lueur (WorldEnvironment), lumières 2D (boule de feu, Rasengan, Chidori, Kurama), décor en
  parallaxe. La logique C++ n'y est pas branchée. Vérifié sous xvfb (`--demo --record`), aucune erreur de script.
  Aucune génération PixelLab.
- 16/10 12 h 30 UTC, session B : **v9.7 publiée** (lot 2 : kits de Kratos, du chasseur et de Sukuna, 14 effets
  PixelLab, rebords des armes à nouvelle apparence recalés sur les mains). Démos de contrôle regardées (les 6 kits,
  2 ultimes pour les auras). Workflow « Release » réussi ; l'archive Linux téléchargée par l'API a la même somme
  SHA-256 que la release, démarre et affiche v9.7. Tous les verrous libres. Suite : attendre le propriétaire pour
  les 10 autres (Asta, Yuno, Byakuya, Kenpachi, Ulquiorra, Grimmjow, Starrk, Gojo, Link, Ganondorf).
- 16/10 12 h 00 UTC, session B, lot 2 : `fitall.sh` relancé (38 clips retimés, 197 hitboxes recalées), tous les
  tests passent. Équilibre : 45-55 % pour tous (chasseur 1,15 ; les autres inchangés), KO sous 70 % : 3 %.
  Verrou `fitall.sh` libre. Reste : démos de contrôle, puis v9.7.
- 16/10 11 h 35 UTC, session B, lot 2 : les 218 clips sont générés et importés (~260 générations, il en reste
  2 640), 14 effets PixelLab importés. Relecture : 38 retouches dans `qa_fixes.json` (fumée grise, effets dans les
  esquives, éclairs parasites) ; 3 clips refaits (silhouette noire dans la projection haute des Lames du Chaos,
  bouclier posé au sol dans la projection basse, tête de Sukuna devenue rouge). **Bug corrigé dans
  `import_all.py`** : les clips `w*_ledge` et `w*_climb` d'une arme à nouvelle apparence étaient ancrés aux pieds,
  pas aux mains comme `ledge` et `climb` (Ichigo, Naruto et Sasuke réimportés). Je prends `fitall.sh`.
- 16/10 08 h 55 UTC, session B, lot 2 en cours :
  - Code des 6 kits poussé (11f88b7), tous les tests passent. Nouveau `addLanceKit` (M1 de la lance-canon).
  - Les 22 clips des Lames du Chaos ont été **refusés sans être facturés** : description du perso au-delà de
    300 caractères (limite de PixelLab). Raccourcie, ils repartent.
  - Le bouclier de Kratos disparaissait au départ de course et quand il est saisi : prompts corrigés,
    `w0_runstart` et `w0_grabbed` refaits (anciens dans `kratos/old_*`). `hunter/airjump` et `tumble` refaits
    (l'épée sortait du cadre ; anciens dans `hunter/old_*`).
  - **Redémarrage du conteneur** vers 08 h 45 : scripts arrêtés. Vérifié chez PixelLab (jobs actifs et récents) :
    aucun job inconnu, donc les 5 dossiers restés « submitting » n'avaient jamais été envoyés ; leur `job.json`
    est effacé et ils repartent. Les 12 jobs « submitted » sont seulement téléchargés (`jobs_l2_resume.json`).
- 16/10 08 h 25 UTC, session B : **lot 2 commencé (Kratos, Hunter, Sukuna)**, comme le lot 1. Fiches des kits dans
  `pxa/wkits/kratos.json`, `hunter.json`, `sukuna.json` : 209 clips à générer (les 4 armes de Kratos et du
  chasseur changent l'apparence, Kamutoke aussi ; le doigt de Sukuna est un pouvoir). Je lance d'abord la garde de
  Kamutoke (`sukuna/w0guard`), puis les vagues avec `chain.py`. Les anciens clips `sukuna/w0_nspecial` (il
  rangeait le bâton) sont mis de côté dans `sukuna/old_w0_nspecial`.
- 16/10 08 h 00 UTC, session B : **v9.6 publiée** (lot 1 : kits d'armes de Naruto, Sasuke et Kakashi, effets
  PixelLab). Démos de contrôle regardées (les 6 kits et les 3 ultimes) : armes visibles, effets bien placés.
  Workflow « Release » réussi ; l'archive Linux téléchargée par l'API a la même somme SHA-256 que la release,
  démarre et affiche v9.6. Tous les verrous libres. Suite : attendre le propriétaire pour les 13 autres.
- 16/10 07 h 46 UTC, session B, lot 1 : génération finie (~200 générations au total, il en reste 2 896), tout importé
  (Naruto 77 clips, Sasuke 44, Kakashi 12 ; 18 effets). `fitall.sh` relancé : 34 clips retimés, 193 hitboxes
  recalées. Le combo M1 à mains nues poussait trop (le 3e coup ratait après recalage) : les 2 premiers coups
  poussent moins. Équilibre réglé : 45-56 % pour tous (Yuno 1,13 ; Ichigo 1,13 ; Byakuya 0,98 ; Kratos 1,13 ;
  Sukuna 1,05 ; Ulquiorra 1,40 ; Grimmjow 1,18 ; Hunter 1,14 ; kits de Naruto ×1,28, de Sasuke 1,36), KO sous
  70 % : 3 %. Tous les tests passent. L'Ōdama est dessiné par le jeu (`odama_ball`) sur la paume mesurée dans le
  clip : PixelLab refusait de dessiner la boule. Verrou `fitall.sh` libre.
- 16/10 06 h 34 UTC, session B, lot 1 en cours : ~134 générations faites, ~100 clips importés (états du mode Ermite et du
  Kurama de Naruto, arc de Sasuke, kits de Kakashi et du Mangekyō de Sasuke), 17 effets PixelLab importés.
  - **Incident évité** : mon script d'enchaînement a construit une vague avec tous les clips de base de Kakashi
    (`jobs(f, [])` traitait une liste vide comme « tous les clips »). Arrêté avant qu'il n'atteigne Kakashi :
    **aucun job en trop n'a été soumis** (vérifié : aucun `job.json` nouveau dans `pxa/kakashi/`). Corrigé dans
    `clips.py` (`only is not None`) et dans le script.
  - Refaits : la contre-attaque du mode Ermite (le prompt « posture de crapaud » transformait Naruto en crapaud)
    et l'Ōdama (la boule n'était pas dessinée) ; le Kagutsuchi de Sasuke (lame verticale).
- 16/10 05 h 36 UTC, session B, **lot 1 du grand chantier : Naruto, Kakashi, Sasuke** (demande du propriétaire), en cours.
  - Kits dans le code (poussés) : `addFistKit` (M1 poing / pied / finisseur, M1 aérien) ; Naruto mode Ermite
    (Kawazu Kumite, bond, contre du crapaud, Ōdama) et Kurama (Rasengan de Kurama, clones, éclair doré, bras de
    chakra qui attrape) ; Sasuke arc du Susanoo (M1 en flèches : rapide, double, chargée ; éventail ; envol ; garde
    du Susanoo qui renvoie) et Mangekyō (M1 d'épée aux flammes noires, Amaterasu, Kagutsuchi, envol d'Enton,
    barrière d'Enton) ; Kakashi Sharingan (taijutsu, Kamui, chien de foudre, téléportation Kamui, contre) et double
    Mangekyō (Kamui Raikiri, shuriken, bond, garde du Susanoo). Nouveau projectile `LightningHound`.
  - Prompts : `pxa/wkits/naruto.json`, `sasuke.json`, `kakashi.json` ; `clips.py` applique maintenant les kits aux
    trois (avant : seulement aux 14 autres). Naruto (mode Ermite : manteau rouge à flammes ; Kurama : corps doré) et
    l'arc de Sasuke changent l'apparence : tous leurs états sont générés.
  - Génération PixelLab en cours (~110 générations prévues) ; effets PixelLab par un agent (vfx_workbench).
  - Équilibre provisoire (avant recalage) : Naruto 42 %, Sasuke 45 % (26 % sans réglage : ses kits étaient à sa
    puissance de base 0,86).
  - Reprise automatique programmée (message `send_later`) si la session est coupée.
- 16/10 01 h 50 UTC, session B : release **v9.5** publiée et vérifiée (workflow réussi ; l'archive Linux, retéléchargée
  par l'API, a la même somme SHA-256 que la release, démarre et affiche v9.5). Verrou de version libre.
- 16/10 01 h 40 UTC, session B : **v9.5, pilote Ichigo terminé**, ce push lance sa publication.
  - Kits : 51 clips PixelLab (Shikai : M1 3 coups, M1 aérien, 3 compétences ; Bankai : tous les états, prises,
    projections, M1, compétences). Un smash avec une arme à kit sort le 3e coup du combo (coup lourd) : sans lui,
    Ichigo tombait à 29 % (plus de coup pour éjecter), avec lui 48 %.
  - 9 effets PixelLab (agent vfx_workbench, gratuits) : `hit_slash`, `hit_reiatsu`, `hit_dark`, `counter_flash`,
    `getsuga_black`, `bankai_burst`, `shunpo_streak`, `reiatsu_aura`, `bankai_aura`. Ils remplacent les
    étincelles, la parade, le Getsuga, l'anneau d'ultime prêt et le pilier d'ultime dessinés par le code ; les
    clips PixelLab n'ont plus la traînée blanche du code (tous les combattants) ; les ultimes d'Ichigo ne
    redessinent plus par-dessus leurs clips la lame géante, la croix et la charge du cero.
  - `fitall.sh` relancé (22 clips retimés, 181 hitboxes recalées) ; équilibre 43-55 % ; tous les tests passent.
  - Démos filmées et relues : kit Shikai, kit Bankai, les 3 ultimes. Option de démo `--weapon SLOT`.
  - Coût PixelLab du pilote : environ 75 générations (dont 10 de clips refaits : bug corrigé dans `clips.py`,
    les clips d'une arme à nouvelle apparence finissant « en garde » finissaient sur la garde de base).
  - Suite, après l'avis du propriétaire : les 16 autres combattants (`pxa/wkits/<perso>.json`, même modèle).
    Verrous imports, effets et équilibrage gardés pour ce chantier ; `fitall.sh` libre.
- 15/10 18 h 21 UTC, session B, pilote Ichigo en cours :
  - **fait et poussé (48a5a76)** : kits d'armes dans le code (`WeaponDef::kit` : avec l'arme, tous les coups au sol
    deviennent le combo M1 jab → jab2 → jab3, tous les aériens le M1 aérien `nair`, les spéciales ses 4
    compétences) ; feuilles d'états d'une arme qui change l'apparence (`w1_idle`, `w1_run`…, `kSheetWState0/1`) ;
    `MoveDef::castFx` (effet PixelLab joué derrière le combattant, qui le suit) ; effets PixelLab des impacts
    `hit_slash`/`hit_reiatsu`/`hit_dark`, de la parade `counter_flash` et du Getsuga (`getsuga`, `getsuga_black`)
    branchés dès qu'ils sont importés. Prompts des kits : `pxa/wkits/ichigo.json` et `WSTATES` dans `clips.py`.
  - générés (~60 générations) et en partie importés : 49 clips du kit (Shikai : jab2, jab3, nair, 3 compétences ;
    Bankai : tous les états, prises, M1, compétences). `w0_jab`, `w1_jab`, `w1_sspecial`, `w1_ledge`, `w1_climb` en
    cours. Le Shunpo de base n'a plus d'image vide, le Cero de base ne dessine plus d'explosion rouge sur Ichigo
    (le rayon du jeu s'en charge) : `qa_fixes.json`.
  - je prends le verrou `fitall.sh` (la session A n'a rien poussé depuis le 14/10) pour recaler les coups du kit.
- 15/10 17 h 41 UTC, session B, **grand chantier validé par le propriétaire** : (1) seule la touche Ultime (U) lance
  l'ultime (fait) ; (2) chaque arme ramassée se voit dans tous les états (garde, repos, marche, course, saut, chute)
  avec un combo M1 de 3 coups, 4 compétences et un ultime propres, tout en clips PixelLab ; (3) les effets de combat
  dessinés par le code sont remplacés par des effets PixelLab accrochés aux mains et aux armes des clips ; (4) les
  clips coupés au bord du cadre sont refaits ; (5) contrôle des bugs. Résolution gardée (PixelLab plafonne à 256 px).
  Pilote : Ichigo (Zangetsu Shikai et Tensa Zangetsu), montré au propriétaire avant les 16 autres. Verrous imports,
  effets et équilibrage pris pour Ichigo. Session A : ne pas toucher aux clips et coups d'Ichigo sans passer par le
  journal.
- 15/10 16 h 53 UTC, session B : release **v9.4** publiée et vérifiée (workflow réussi ; l'archive Linux,
  retéléchargée par l'API, a la même somme SHA-256 que la release, démarre et affiche v9.4).
- 15/10 16 h 40 UTC, session B : **v9.4**, ce push lance sa publication. Amaterasu : délai 3 s. Commandes des smashs
  chargés : `motion()` regarde 26 images (`dirHist` en garde 30) et le stick peut quitter sa dernière direction
  jusqu'à 3 images avant le bouton (2 tests qui échouent sur l'ancien code). Revue des hitboxes : `fitall.sh` relancé
  sur les clips actuels (22 clips retimés, 181 hitboxes recalées), équilibre 43-55 %, KO sous 70 % : 3 %. Le contrôle
  compte 435 hitboxes sur 1 058 « là où rien n'est dessiné », mais 239 sont des ultimes et 77 des Cero (dessinés par
  le jeu) ; restent 119, dont 46 à plus de 10 unités du dessin : surtout des coups d'épée dont l'arc balaie pendant
  que le clip tient la pose finale, et une vingtaine d'arcs qui passent où le clip ne dessine aucune lame (jab2
  d'Ichigo, bair de Hunter, utilt de Kenpachi…). Essayé puis abandonné : tourner ces arcs vers la lame dessinée dans
  `--fit-hitboxes` (la recherche de la lame prenait les jambes, des jabs frappaient vers le sol, Sasuke passait de 46
  à 57 %) ; le recaleur est revenu à l'original. Verrous libres.
- 15/10 16 h 20 UTC, session B, demande du propriétaire : délai de l'Amaterasu à 3 s ; un peu plus de temps pour les
  commandes des smash chargés (quart de cercle, bas-haut) ; revue des hitboxes. Je modifie `src/core/` (roster,
  détection des commandes), `src/game/check_hitboxes.cpp` (le recalage des coups d'épée dont l'arc passe où aucune
  lame n'est dessinée) et je relance `fitall.sh` : verrous « Équilibrage » et `fitall.sh` pris.
- 15/10 15 h 08 UTC, session B : release **v9.3** publiée et vérifiée (workflow réussi ; l'archive Linux,
  retéléchargée par l'API, a la même somme SHA-256 que la release, démarre et affiche v9.3).
- 15/10 14 h 57 UTC, session B : **v9.3**, ce push lance sa publication. Fait : délai entre deux utilisations
  (`MoveDef::cooldown`, `Fighter::moveCooldown`, remis à zéro au retour ; l'IA ignore une spéciale qui recharge).
  L'Amaterasu de Sasuke recharge 5 s et dure jusqu'à la fin des flammes (96 images) : Sasuke s'arrête net, ne dérive
  pas, ne peut ni bouger ni attaquer ; son clip garde la pose (`MoveDef::channelEnd`) et le badge de l'arme dans le
  HUD affiche les secondes restantes (vérifié en capture). Ultimes (`combat.cpp`) : dégâts ×1,1, recul ×0,8, bouclier
  250 % des dégâts (49 ultimes sur 51 cassent un bouclier plein). Recul adouci sous 100 % (`softenLowPercent` :
  au-delà d'un petit coup, jusqu'à 40 % de moins à 0 %) : KO sous 70 % de 9 % à 3 % dans `sim_test balance`, qui
  affiche maintenant les pourcentages de KO par perso. Équilibrage refait (Yuno, Naruto, Ichigo, Kenpachi) : 43 à 56
  %. Vérifié : compilation, ctest, test `amaterasu`, les 51 ultimes en démo. Verrous libres.
- 15/10 14 h 04 UTC, session B, demande du propriétaire : l'Amaterasu de Sasuke reçoit un délai entre deux
  utilisations et Sasuke reste immobile, sans attaquer, pendant la technique ; les ultimes éjectent moins mais font
  quelques % de dégâts en plus et beaucoup plus de dégâts aux boucliers ; moins de recul à bas pourcentage (éjections
  trop faciles à 50-60 %). Je modifie `src/core/` (moves, combat, roster) et `tests/sim_test.cpp` ; verrou «
  Équilibrage » pris. Ensuite la v9.3.
- 15/10 13 h 20 UTC, session B : release **v9.2** publiée et vérifiée (workflow réussi en 9 min ; archives Linux et
  Windows de 46 Mo ; l'archive Linux, retéléchargée par l'API, a la même somme SHA-256 que la release, démarre et
  affiche v9.2). Le dépôt est **privé** : les liens de téléchargement ne marchent que pour un compte GitHub qui a
  accès au dépôt.
- 15/10 13 h 02 UTC, session B : **v9.2** (écran titre, `package_release.sh`, README, `packaging/NOUVEAUTES.md`) ; ce
  push lance sa publication par GitHub Actions. `fitall.sh` : 93 clips sur 325 recalés, 183 hitboxes sur 638 recalées
  ; restent 435 hitboxes sur 1 058 qui frappent surtout là où rien n'est dessiné (même proportion qu'avant, surtout
  des hitboxes secondaires d'épée et des rayons ; ex. le tilt avant de Hunter frappe au-dessus de sa lame).
  Équilibrage refait après le recalage (`sim_test balance` de 45 % à 58 %, le détail est dans le commit). Vérifié :
  compilation, ctest, les 51 ultimes en démo sans plantage, `sheets.txt` cohérent avec les images. Tous les verrous
  sont libres.
- 15/10 12 h 39 UTC, session B : **bug trouvé et corrigé** pendant la recherche de bugs. L'op `erase` de
  `import_all.py` efface aussi les pixels presque blancs qui touchent l'effet, donc un vêtement blanc collé à l'effet
  : le manteau de Starrk (`dashattack`) et le haori de Byakuya (`nspecial`, `w0_nspecial`) étaient troués. Starrk est
  réimporté. Pour Byakuya (ses images brutes sont chez la session A), les images abîmées sont remplacées dans les
  planches par une image intacte du même clip, et `qa_fixes.json` porte la même règle (`replace`), pour que ton
  prochain import donne le même résultat. Session A : attention à `erase` sur un perso habillé de blanc.
- 15/10 12 h 18 UTC, session B : tous les clips des six persos sont importés et relus (compilation, tests et
  ultimes de Hunter et Ganondorf en démo : OK). Je lance `fitall.sh` (verrou déjà pris), puis l'équilibrage, la
  recherche de bugs et la **v9.2** (verrou « Version et paquets » pris).
- 15/10 12 h 05 UTC, session B : Hunter importé (54 clips) et relu. Corrigés dans `qa_fixes.json` : l'esquive sur
  place (traînées de feu et explosions inventées), le dégât (étincelle dessinée), le saut, le lancer arrière, la
  bombe-tonneau (le tonneau du clip disparaît quand le jeu lance le sien), les fumées grises (`fair`, `usmash`,
  `nspecial`, `ultimate`), la Décharge de la Charge Blade et le tir de la lance-canon (leurs explosions sont des
  projectiles du jeu), le Feu du Wyvern (le jeu dessine le rayon). `airdodge` (il devenait une boule) et `dthrow` (son
  épée disparaissait) sont régénérés avec de nouveaux prompts (`qa_prompts.json`, les anciens dans `*.qa1`). Gardés
  tels quels : les traînées de lame (rouges pour les coups chargés). Relus aussi : le `bthrow` régénéré de Starrk,
  le `hold` de Kratos, les deux clips d'épée de Ganondorf (`w0_sspecial` corrigé). Les clips `w0_jab`/`w1_jab` de
  Hunter datent de la mise à jour du 13/10 et n'ont pas été touchés.
- 15/10 11 h 55 UTC, session B : **génération PixelLab terminée** pour les six persos (Starrk 58/58, Sukuna 57/57,
  Link 58/58, Ganondorf 60/60, Kratos 60/60, Hunter 59/59 ; 3 169 générations restantes). Relecture de Ganondorf :
  `dspecial` régénéré (sa plongée était une silhouette toute rouge, nouveau prompt dans `qa_prompts.json`, l'ancien
  dans `pxa/ganondorf/dspecial.qa1`) ; `ultimate`, `w1_ultimate`, `sspecial`, `nspecial`, `extra1` corrigés dans
  `qa_fixes.json` (le rayon, les flammes et les éruptions dessinés dans le clip, que le jeu dessine déjà, sont
  remplacés par les poses propres du même clip). `import_all.py` sait deux corrections de plus : `specks` (efface
  les petits morceaux détachés du perso : étincelles, pétales, traînées) et `erase` avec `"hue": "scarlet"` (le
  rouge vif d'un effet, pour un perso dont les rouges sont plus sombres). Je lance l'import de Hunter (59 clips),
  puis sa relecture, puis `fitall.sh`, l'équilibrage et la v9.2.
- 15/10 08 h 55 UTC, session B : le propriétaire a validé la génération de Link, Kratos et Hunter (et du reste de
  Ganondorf et Sukuna), environ 250 générations. `driver.py link kratos hunter` et `rescue_loop.py` tournent dans la
  session B. Les poses de garde `pxa/base/<perso>_guard256.png` (ignorées par git) sont la dernière image des jobs
  `gen14/<perso>_toguard`. **`tools/pxcli.py` modifié** : `wait()` prend les images par la galerie
  (`pixelart_workbench inspect gallery:<id>` puis `frames.png` sur api.pixellab.ai), car les liens Backblaze sont
  bloqués dans les sessions cloud. Session A : recopier ce `pxcli.py` dans ton scratchpad avant `sync_repo.sh`.
- 15/10 08 h 45 UTC, session B, **à la demande du propriétaire** (la session A n'a rien poussé depuis le 14/10
  19 h 52) : je récupère les clips déjà générés et payés de Starrk (59), Sukuna (49) et Ganondorf (26) avec leurs
  `job_id` de `job_status.json` (`pxcli.py wait`, aucune nouvelle génération), je les importe avec `import_all.py`,
  puis `fitall.sh`. Session A : `git pull` avant de toucher à ces trois persos ou de lancer `fitall.sh`.
- 15/10 08 h 15 UTC, session B : les versions se publient sur GitHub. `.github/workflows/release.yml` (sur un tag
  `vX.Y`) teste, fabrique les archives Linux et Windows et les met dans la release, avec les notes de
  `packaging/NOUVEAUTES.md`. Liens directs dans le README (« Jouer sans compiler »). `package_release.sh`
  recompresse aussi les PNG sans perte quand zopflipng est installé (archives ~8 % plus petites). Le workflow part
  quand le numéro de version de `screen_title.cpp` est poussé (le proxy de session refuse les tags).
- 15/10 05 h 45 UTC, session B : **v9.1** (écran titre, script de paquets, README). Règle du propriétaire : +0,1 à
  chaque mise à jour livrée, avec l'archive Linux envoyée. Paquet Linux 9.1 fabriqué (`package_release.sh 9.1`,
  7 min), testé (démarre, affiche v9.1, aucune bibliothèque manquante) et envoyé en 2 morceaux (limite de 30 Mo).
  Pas d'archive Windows : mingw-w64 n'est pas installé ici, et le propriétaire est sous Linux.
- 14/10 21 h 45 UTC, session B : recherche de régressions. `sheets.txt` cohérent avec les images (720 lignes, rien
  de manquant ni d'orphelin, impacts dans les clips). Les 51 ultimes (17 persos × 3) joués en démo sans plantage.
  Corrigé : l'avertissement de compilation de `check_hitboxes.cpp`. Restent, connus : 282 hitboxes sur 692 qui
  frappent là où rien n'est dessiné (surtout des hitboxes secondaires d'épée, à revoir avec les clips),
  `wyvern_fire` à faire, `crouch`/`dizzy`/`helpless` d'Ulquiorra à régénérer, clips de Grimmjow pas encore relus.
- 14/10 21 h 20 UTC, session B : règles du travail à deux ajoutées (section « Verrous », `fitall.sh` réservé à la
  session A, noter ce qui commence). `.gitignore` laisse passer `tools/pixellab/vfx/ult/*.png` : les planches des
  rayons sont maintenant dans git.
  - **Proposition pour la session A** (son dossier, et son `sync_repo.sh` écraserait un changement fait ici) : lire
    le dossier de travail dans une variable au lieu du chemin absolu, par exemple
    `S=${PIXELLAB_WS:-$(git -C "$(dirname "$0")" rev-parse --show-toplevel)/tools/pixellab}` dans les `.sh` et
    `os.environ.get("PIXELLAB_WS", ...)` dans les `.py`. Le faire d'abord dans son scratchpad, puis synchroniser :
    plus de `sed` ni de lien sur une autre machine.
- 14/10 21 h 10 UTC, session B : équilibrage. Kakashi : puissance 1,26 → 1,08 et Raikiri 14 → 12 (72 % → 54 %).
  Grimmjow : puissance 1,08 → 1,20 (33 % → 44 %). Seuls les dégâts changent, pas l'échelle S : le recalage des
  hitboxes reste valable. Le test balance bouge de ±5 points d'un réglage à l'autre (96 matchs par perso). Encore
  hauts : Asta 60, Ganondorf 61, Kratos 60, Byakuya 57 ; encore bas : Ulquiorra 41, Yuno 42, Sasuke 42, Link 42.
- 14/10 21 h 00 UTC, session B : `fitall.sh` terminé (13 min, ctest réussi). 86 clips sur 205 ont un nouvel
  impact, 17 impacts de la revue remis, 139 hitboxes sur 442 recalées. Le contrôle signale encore 282 hitboxes sur
  692 qui frappent surtout là où rien n'est dessiné. Équilibre après recalage : Asta 60, Yuno 42, Ichigo 43,
  Byakuya 57, Kenpachi 45, Ulquiorra 43, Grimmjow 33, Starrk 47, Naruto 50, Sasuke 40, **Kakashi 72**, Gojo 52,
  Sukuna 57, Link 41, Ganondorf 61, Kratos 59, Hunter 48 (%). Équilibrage en cours (session B).
- 14/10 20 h 50 UTC, session B : **lance `fitall.sh`** à la demande du propriétaire. Il réécrit
  `src/core/roster_hitfit.inc` et la colonne impact des lignes `sheet` de `sheets.txt` (fichiers de la session A).
  Session A : faire `git pull` avant ton prochain `fitall.sh` ou ton prochain import.
- 14/10 20 h 45 UTC, session B : rayons d'ultimes du groupe e importés (`gran_rey_cero`, `cero_oscuras`,
  `vasto_cero`, `light_dragon`, `demon_dragon`, chacun avec son `_head`). Leurs PNG n'étaient pas dans git
  (`*.png` ignoré dans `tools/`) : récupérés en réexportant les effets enregistrés (vfx_workbench, gratuit, mêmes
  noms, disposition identique aux JSON). Le départ du rayon apparaît en fondu, avec une lueur à la main
  (`drawFxStrip`, `drawSpriteBeam`). Vérifié en jeu sur les 5 ultimes. `wyvern_fire` reste à faire : seul son JSON
  existe, sans image ni notes.
  - Relecture Ulquiorra et Byakuya : déjà faite par la session A (186136b, 6cb7ccf), rien refait.
  - Chemins de `tools/pixellab` : pas de `sed` (il toucherait 86 fichiers que `sync_repo.sh` réécrit). Sur une
    machine dont le dépôt est dans `/home/user/test-claude`, un lien suffit :
    `ln -sfn /home/user/test-claude/tools/pixellab <ancien chemin du scratchpad>`.
- 14/10 19 h 30 UTC, session A : génération PixelLab relancée sur le compte du propriétaire (~3 560 générations
  restantes). Les régénérations Naruto/Sasuke/Kakashi sont en cours. Ce document reste à jour à chaque push.

- Branche de travail : `claude/sleepy-goodall-s399kv`. Une copie identique est poussée sur `reprise-claude`.
- PR ouverte : grahamyoan9710-tech/test-claude#1, base `claude/admiring-lamport-jijwx9`.
- Version affichée dans le jeu : **v9.1** (15/10). Le propriétaire veut **+0,1 à chaque mise à jour** (9.2, 9.3…),
  publiée sur la page Releases de GitHub dès que le nouveau numéro est poussé (voir CLAUDE.md).

## 1. Vérifié au moment de la passation (réellement exécuté)
- `cmake --build build` : compile sans erreur ni avertissement.
- `ctest` : 1/1 réussi. `./build/sim_test` : « all tests passed », déterminisme compris.
- Taux de victoire (`./build/sim_test balance`) :

  | Combattant | Taux | Combattant | Taux | Combattant | Taux |
  |---|---|---|---|---|---|
  | Asta | 55 % | Grimmjow | 40 % | Sukuna | 57 % |
  | Yuno | 48 % | Starrk | 51 % | Link | 39 % |
  | Ichigo | 42 % | Naruto | 52 % | Ganondorf | 54 % |
  | Byakuya | 51 % | Sasuke | 42 % | Kratos | 59 % |
  | Kenpachi | 50 % | **Kakashi** | **72 %** (trop fort) | Hunter | 44 % |
  | Ulquiorra | 51 % | Gojo | 44 % | | |

- Aucun test en jeu (écrans, matchs) n'a été relancé après les derniers imports de Kenpachi et d'Ulquiorra.

## 2. La demande en cours (du propriétaire, à respecter)
1. Revérifier Naruto, Sasuke et Kakashi.
2. Refaire avec PixelLab :
   - les lames du Chaos de Kratos ;
   - la lance d'Ulquiorra ;
   - les flammes noires (Amaterasu) ;
   - Lilynette ;
   - le Kamui, qui doit **aspirer** et non faire une tornade ;
   - le Fuga de Sukuna, une **flèche de feu** et non un arc.
3. Refaire proprement les **ultimes** : plus gros, fidèles aux œuvres, effets PixelLab.
4. Refaire **toutes les animations de tous les personnages** avec PixelLab, comme pour Naruto, Sasuke et Kakashi.
5. **Recaler les hitboxes** sur les animations.

## 3. Ce qui est fait
- **Point 2 (redesigns) : fait.** Effets `amaterasu`, `fuga_arrow`, `fuga_blast`, `lanza`, `lanza_blast` et
  `lilynette` (tenue couverte), plus l'arme `weapon_chaos_blade`. Le Kamui est une distorsion du décor :
  `pix::swirl` dans `pixel.cpp` et `shotKamuiVortex` dans `render_world.cpp`.
- **Effets d'ultimes (PixelLab vfx_workbench).** 32 lignes `fx` dans `assets/sprites/sheets.txt`, branchées :
  - projectiles : table `kShotFx` dans `render_world.cpp` ;
  - tempêtes et explosions : `ultSprite` dans `render_vfx.cpp` ;
  - effets ponctuels : `fx.cpp`.

  Vérifiés en jeu (enregistrements de démos) : Hollow Purple, Bleu max, Sanctuaire, Kamutoke, Nozarashi,
  Rasenshuriken, Bijūdama, Kirin, Indra, shuriken du Susanoo, ceros et loup de Starrk, Getsuga, Black Divider,
  Kageyoshi.
- **Ultimes agrandis.** Rayons modifiés dans `src/core/roster_ultimates.cpp`. Leur effet sur l'équilibre n'a pas
  été étudié à part.
- **Clips PixelLab importés dans le jeu (524 lignes `sheet`).** Naruto, Sasuke, Kakashi, Asta, Yuno, Ichigo,
  Byakuya, Kenpachi et Ulquiorra sont complets. Les M1 d'armes existent pour tous.
- **Revue image par image et corrections appliquées.** Images remplacées, impacts recalés, plaques grises
  effacées, tissu parasite rogné. Asta, Naruto, Sasuke, Kakashi, Yuno, Ichigo et Kenpachi sont traités.
  - Les corrections sont dans `tools/pixellab/pxa/qa_fixes.json`.
  - Les prompts réécrits sont dans `tools/pixellab/pxa/qa_prompts.json`.
- **Bug corrigé :** Yuno avait deux grimoires en mode clip (`render_body.cpp`).
- **Hitboxes recalées** pour une partie des coups (`src/core/roster_hitfit.inc`, généré). La dernière passe
  complète date d'avant l'import des 6 nouveaux personnages.

## 4. Ce qui reste
1. **Relecture des clips**, page par page avec `qa_prep.sh` et les planches `clips_*.png` :
   - **Ulquiorra** : relecture en cours. Repérés mais pas encore corrigés :
     - `airdodge`, image 3 vide ;
     - `uspecial`, image 1 quasi vide ;
     - `sspecial`, image 0 toute verte et image 1 grise ;
     - `jab3` et `w1_ultimate` (22), plaques grises ;
     - `hurt`, image 0 avec un éclat jaune ;
     - `crouch`, `dizzy` et `helpless` sont à régénérer (pas de vraie pose).
   - **Byakuya** : relecture pas faite. `tools/greyscan.py byakuya` signale des plaques grises.
   - **Grimmjow, Starrk, Gojo, Sukuna** : à importer (`import_all.py <perso>`) puis à relire.
2. **Génération PixelLab restante : 567/817 clips faits.**
   - Link : 0/58. Kratos : 0/60. Hunter : 0/59. Ganondorf : 26/60. Sukuna : 28/57. Starrk : 51/58.
     Grimmjow : 57/60.
   - La liste exacte est dans `tools/pixellab/pxa/status_snapshot.txt` et `job_status.json`.
3. **Régénérations demandées par la revue.** Leurs dossiers d'origine ont été renommés `*.qa1`, et `driver.py`
   les resoumet tout seul avec les nouveaux prompts. Concernés :
   - crouch, dizzy, helpless, spotdodge et getupattack pour Yuno, Ichigo et Kenpachi ;
   - Ulquiorra holdstart et Starrk holdstart.

   Déjà revenus mais pas encore importés :
   - Naruto : crouch, dizzy, usmash, dsmash et helpless (bons à l'œil) ;
   - jobs N/S/K du lot `jobs_nsk_qa1.json`.
4. **Rayons d'ultimes (groupe e).** Gran Rey Cero, Cero Oscuras, Vasto Lorde, dragon de lumière, dragon démon et
   feu de wyvern :
   - les PNG et JSON sont dans `tools/pixellab/vfx/ult/` ;
   - leurs notes sont dans `notes_e.json` ;
   - **importés le 14/10 à 20 h 45** (session B), sauf **wyvern_fire, pas encore fait**.

   Le code les attend déjà : `beamSprite` dans `render_vfx.cpp` dessine `<nom>` plus `<nom>_head` quand ces effets
   existent. Import :
   `python3 scripts/import_fx.py NOM tools/pixellab/vfx/ult/NOM.png --anchor X Y --hit R --loop`,
   avec X, Y et R tirés du fichier de notes.
5. **Repasse des hitboxes et impacts** après chaque lot d'imports : `tools/pixellab/fitall.sh`. Il enchaîne
   `--fit-impacts`, `lock_impacts.py` (garde les impacts choisis à la main), `--fit-hitboxes`, le contrôle, la
   balance et ctest.
6. **Équilibrage :** Kakashi ramené à 54 % et Grimmjow remonté à 44 % (14/10, session B). Restent un peu
   faibles : Ulquiorra, Yuno, Sasuke et Link (41-42 %) ; un peu forts : Asta, Ganondorf et Kratos (60-61 %).
7. **Chaque mise à jour livrée (prochaine : v9.2) :**
   - version +0,1 dans `src/game/screen_title.cpp`, `scripts/package_release.sh` et le README ;
   - nouveautés dans `packaging/NOUVEAUTES.md`, puis pousser : GitHub Actions publie la release vX.Y ;
   - mise à jour de la description de la PR.

## 5. Pipeline PixelLab (`tools/pixellab/`)
C'est une copie de l'espace de travail de la session précédente. **Les chemins absolus y sont restés** :
`/tmp/claude-0/-home-user-test-claude/454a74a0-0344-5f0d-8ed7-e18843be178a/scratchpad` et `/home/user/test-claude`.
Pour les adapter à ton PC, depuis la racine du dépôt :
```
grep -rl "/tmp/claude-0/-home-user-test-claude/454a74a0-0344-5f0d-8ed7-e18843be178a/scratchpad" tools/pixellab \
  | xargs sed -i "s#/tmp/claude-0/-home-user-test-claude/454a74a0-0344-5f0d-8ed7-e18843be178a/scratchpad#$(pwd)/tools/pixellab#g"
grep -rl "/home/user/test-claude" tools/pixellab | xargs sed -i "s#/home/user/test-claude#$(pwd)#g"
```

### Ton jeton PixelLab
Il n'est **pas** dans le dépôt. Mets le tien dans `tools/pixellab/px/.token` : ce dossier est ignoré par git.

### Les scripts
- `tools/pxcli.py` : anim, image, `wait`, `jobs`, `balance`. Il ne fait jamais apparaître le jeton.
- `tools/pxbatch.py` : soumet des lots.
- `pxa/clips.py` : le prompt de chaque clip. Il lit `specs_final/`, `qa_prompts.json` et `subjects14.json`.
- `pxa/driver.py <persos>` : toutes les minutes, soumet les clips dont la « base » est prête et validée.
  - Les bases sont jump, runstart, knockdown, holdstart, ledgegrab, w0guard et w1guard.
  - Une base n'est validée qu'une fois listée dans `approved.txt`.
- `pxa/watch_bases.sh` : signale les bases finies à valider.
- `pxa/import_all.py <perso> [clips]` : importe les clips dans `assets/sprites`.
  - Applique `qa_fixes.json` : `replace`, `impact`, `degrey`, `trim_right`.
- `m1/import_m1.py` : importe les M1 d'armes.
- `tools/rescue_loop.py` : récupère les jobs finis.
- Outils de relecture :
  - `qa_prep.sh <perso> <index roster>` : prépare `qa14/<perso>/` ;
  - `qa14/qa_fighter.js` : workflow de revue ;
  - `sheetstrip.py`, `qa_strip.py`, `vfxsheet.py`, `tools/greyscan.py`, `tools/fxclean.py`.
- Recalage : `recult.sh` (enregistre un ultime) et `fitall.sh`.

### Ce qui n'a pas été copié
- Les images brutes générées (environ 270 Mo) n'ont **pas** été copiées.
- Seules les bases (leurs images) et l'état des jobs ont été copiés.
- Les clips déjà importés sont dans `assets/sprites/` : c'est la référence.
- Pour corriger un clip déjà importé sans ses images brutes : le régénérer, ou retoucher directement
  `assets/sprites/<perso>_<clip>.png`.

### Jobs soumis avec l'ancien compte
Ces jobs sont **liés au compte PixelLab du propriétaire** :
- Grimmjow : w0_ultimate, w1_sspecial, w1_ultimate ;
- Sasuke : fsmash, helpless ;
- Sukuna : 12 clips.

Leurs identifiants sont dans `job_status.json`. Un autre compte ne peut pas les récupérer. Sur ton compte,
`pxcli.py jobs` montre seulement tes propres jobs.

Si le propriétaire ne les récupère pas avec son jeton (`python3 tools/pxcli.py wait <dossier>`), il faudra les
régénérer. Avant toute nouvelle génération :
1. lancer `pxcli.py jobs` et `pxcli.py balance` ;
2. ne pas soumettre deux fois le même clip.

## 6. Décisions techniques à garder
- Les clips sont dessinés regardant vers la droite. Le jeu tient l'image « impact » tant que la hitbox est active.
- Pas de gros effets dans les clips du personnage : le jeu dessine les effets `fx`. Un clip dont les effets
  cachent le personnage est corrigé, dans cet ordre :
  1. remplacer l'image par une image propre du même clip ;
  2. sinon, effacer l'effet par couleur ;
  3. sinon, régénérer.
- `lock_impacts.py` doit passer après `fit_impacts.py --apply` : sinon les impacts choisis à la revue sont écrasés.
- Les hitboxes ne sont recalées que par décalage et rayon, jamais par rotation des coups d'arme : la rotation a
  été essayée et cassait l'équilibre.
- Kamui : distorsion (shader), pas de tornade. Fuga : flèche de feu. Lilynette : tenue couverte.

## 7. Ne PAS refaire
- Ne pas régénérer les clips déjà importés et relus : Naruto, Sasuke, Kakashi, Asta, Yuno, Ichigo, Kenpachi.
  Ne régénérer que ce qui est listé ci-dessus.
- Ne pas refaire les effets qui sont déjà dans `sheets.txt`.
- Ne pas réécrire le moteur d'animation, le rendu ou le système de hitboxes : ils marchent.
