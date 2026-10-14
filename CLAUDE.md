# Super Clash Bros. — instructions pour Claude Code

Jeu de combat façon Smash (fan game) en C++17 avec raylib 5.5 : 17 combattants d'animes et de jeux.
Avant toute chose, lire **REPRISE_PROJET.md** : il donne l'état exact du travail et la suite.

## Règles
- Le propriétaire du projet écrit en français : répondre en français.
- Ne jamais recommencer le projet ni retirer une fonctionnalité existante.
- Ne jamais afficher, committer ou recopier une clé :
  - jeton PixelLab : `tools/pixellab/px/.token` ;
  - clé ElevenLabs : `tools/pixellab/el/.key`.
  Ces dossiers sont ignorés par git, et chacun utilise son propre compte.
- Ne jamais relancer un job PixelLab encore « submitted » ou en cours : chaque job est facturé. Vérifier d'abord avec
  `python3 tools/pixellab/tools/pxcli.py jobs`.
- Lilynette (Starrk) est une enfant : sa tenue reste couverte et rien n'est sexualisé.
- Commits : messages en anglais, clairs. Ne rien fusionner dans `main` sans demande explicite.
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
- Les gros effets (rayons, tornades, explosions) sont dessinés par le jeu (effets `fx`), pas dans les clips du
  personnage. Les prompts PixelLab se terminent par « Only <nom> is drawn ».
