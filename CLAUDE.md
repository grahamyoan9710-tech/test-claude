# Super Clash Bros. — instructions pour Claude Code

Jeu de combat façon Smash en C++17 avec raylib 5.5.
**Refonte en cours (décidée le 20/10/2026)** : 17 combattants originaux remplacent les 17 anciens persos d'animes et
de jeux, sur la branche `claude/refonte-originaux`. Fiches, menus et budget : `docs/refonte/FICHES.md`. Images du
propriétaire : `docs/refonte/*.png`.
Avant toute chose, lire **REPRISE_PROJET.md** : il donne l'état exact du travail et la suite.

## Règles
- Le propriétaire du projet écrit en français : répondre en français.
- Ne jamais recommencer le moteur ni retirer une fonctionnalité existante, **sauf la refonte décidée par le
  propriétaire** :
  - les 17 anciens combattants, leurs sprites, leurs effets et leurs voix sont retirés (droits d'auteur) ;
  - sont gardés : `src/core` (combat, IA, tests), le système d'armes à kits, la chaîne PixelLab (`tools/pixellab`,
    `import_all.py`, `qa_fixes`, `fitall.sh`), les scripts d'import, les paquets et les releases.
- Les persos, armes et transformations n'ont pas encore de nom : ils sont repérés par numéro (perso `NN`, armes
  `NN-A` et `NN-B`, transformation `TNN`).
- Les 3 persos et 3 armes prévus pour la 10.5 ne sont pas à faire maintenant.
- Ne jamais afficher, committer ou recopier une clé :
  - jeton PixelLab : `tools/pixellab/px/.token` ;
  - clé ElevenLabs : `tools/pixellab/el/.key`.
  Ces dossiers sont ignorés par git, et chacun utilise son propre compte.
- Ne jamais relancer un job PixelLab encore « submitted » ou en cours : chaque job est facturé. Vérifier d'abord avec
  `python3 tools/pixellab/tools/pxcli.py jobs`.
- Aucun personnage sexualisé : pas de décolleté, ventre couvert, jupes ou shorts à mi-cuisse au moins, pas de
  jarretelles. Les persos jeunes (04, 11, 14) ont des tenues sages.
- Commits : messages en anglais, clairs. Ne rien fusionner dans `main` sans demande explicite.
- **Versions : +0,1 à chaque mise à jour livrée au propriétaire** (9.1, 9.2, 9.3…), dans
  `src/game/screen_title.cpp`, `scripts/package_release.sh` et le README. Prendre le verrou « Version et paquets »,
  écrire les nouveautés dans `packaging/NOUVEAUTES.md`, committer et pousser : le nouveau numéro de
  `screen_title.cpp` lance GitHub Actions (`.github/workflows/release.yml`), qui teste, fabrique les archives Linux
  et Windows et les met dans la release vX.Y (GitHub crée le tag ; le proxy de session refuse les push de tags).
  Vérifier que le workflow a réussi, puis donner au propriétaire le lien
  https://github.com/grahamyoan9710-tech/test-claude/releases/latest (liens directs dans le README, « Jouer sans
  compiler »).
- Deux sessions travaillent en même temps sur `claude/sleepy-goodall-s399kv` : respecter la répartition de
  REPRISE_PROJET.md (section 0). Faire `git pull` (fusion, jamais de rebase ni de `--force`) avant de commencer et
  avant chaque push, committer et pousser souvent, et noter dans le journal de REPRISE_PROJET.md ce qui commence et
  ce qui a été fait. Prendre le verrou de la section « Verrous » avant une tâche longue sur un fichier partagé.
  `fitall.sh` n'est lancé que par la session A.
- Ne pas annoncer qu'un test ou une compilation passe sans l'avoir lancé.

## Compiler et tester
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j   # jeu + sim_test
cd build && ctest                                                            # ou ./build/sim_test [ults|weapons|strikes|balance|...]
cmake -S . -B build-core -DCLASH_BUILD_GAME=OFF && cmake --build build-core -j  # cœur seul, sans raylib
```
- Sous Linux, il faut `libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev`.
  raylib est téléchargé par FetchContent.
- Paquets de sortie : `scripts/package_release.sh X.Y` (Linux + Windows, dans `dist/`).
- Captures sans écran :
  `xvfb-run -a ./build/super_clash --screen anim --tab N --shot 20 out.png --quit-after 25`.
  La page N vaut `index_du_perso * 6 + page`, dans l'ordre de `roster()`.
- Démo d'ultime :
  `./build/super_clash --demo --fighters A,B --ult SLOT --record 270 500 10 DIR --quit-after 505`.
  SLOT vaut -1 (mains nues), 0 ou 1 (arme).

## Architecture
- `src/core/` : la simulation, déterministe et sans rendu.
  - `roster*.cpp` : combattants, coups, ultimes.
  - `roster_hitfit.inc` : fichier généré, les hitboxes recalées sur les animations.
  - `combat.cpp`, `match.cpp`, `ai.cpp`.
- `src/game/` : le rendu et les écrans (raylib).
  - `fighter_art.cpp` : charge `assets/sprites/sheets.txt` (lignes `sheet` pour les clips de combattants, lignes
    `fx` pour les effets).
  - `render_body.cpp` : dessin des combattants.
  - `render_world.cpp` : projectiles. La table `kShotFx` associe un projectile à son effet PixelLab.
  - `render_vfx.cpp`, `render_ult.cpp` : rayons et ultimes.
  - `fx.cpp` : effets ponctuels (`playFx`).
  - `pixel.cpp` : shader de distorsion du Kamui.
- Outils de mise au point dans le binaire :
  - `--check-hitboxes`, `--fit-impacts` et `--fit-hitboxes`, avec leurs scripts dans `scripts/` ;
  - `--export-pose`.
- `scripts/` : import des ressources.
  - `import_clip.py` : clip PixelLab → `assets/sprites/<perso>_<clip>.png` + ligne `sheet`.
  - `import_fx.py` : effets.
  - `fit_impacts.py`, `check_hitboxes.py`.
- `tests/sim_test.cpp` : tests sans écran (déterminisme, ultimes, armes, équilibre…).
- `tools/pixellab/` : la chaîne de génération PixelLab. Voir REPRISE_PROJET.md, section « Pipeline ».

## Conventions
- C++17, style du code voisin : commentaires courts en anglais, noms explicites, pas de dépendance nouvelle.
- Un clip regardé vers la droite ; le jeu le retourne. Dans un clip d'attaque, l'image « impact » est celle que le jeu
  tient tant que la hitbox est active.
- Effets : **tous** faits avec PixelLab (`vfx_workbench`), menus compris, et joués par le jeu (effets `fx`). Aucun
  effet dessiné par le code : les anciens effets codés sont supprimés une fois remplacés.
- Aucun effet dans les clips des persos : les prompts PixelLab se terminent par « Only <nom> is drawn ». Seule
  exception : les traînées des coups rapides et des épées sont gardées. Un clip où un autre effet est collé au perso
  est régénéré, pas découpé avec `erase` ou `degrey`. Aucun perso coupé au bord du cadre.
- Sans arme, chaque perso se bat dans un art martial (`docs/refonte/FICHES.md`, §1.7). Les ondes 1 à 5 de la planche
  servent aussi d'ondes d'impact des coups puissants ; elles partent du sol et montent.
- Armes (refonte) : tout le monde commence à mains nues. L'arme d'un perso tombe sur l'arène ; il la prend en se
  tenant dessus et en maintenant la touche Arme 2 s (arme A), puis cette touche alterne A et B, sans délai. L'arme
  reste jusqu'au KO, sans minuteur. Chaque arme B a son propre modèle, exactement comme sur la planche (plan
  complet, validé : le propriétaire autorise toutes les générations nécessaires).
