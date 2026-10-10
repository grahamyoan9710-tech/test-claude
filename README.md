# Super Clash Bros.

Un jeu de combat de plateforme en C++ inspiré de **Super Smash Bros.** : on ne vide pas une barre de vie,
on fait monter le **pourcentage de dégâts** de l'adversaire jusqu'à pouvoir l'**éjecter hors de l'écran**.

![Écran titre](docs/titre.jpg)

Les personnages sont dessinés et animés par le code (squelette + cinématique inverse), les sons et la
musique sont synthétisés au lancement. Les seules ressources sont deux polices libres et la base de
manettes SDL, intégrées directement dans l'exécutable : il n'y a aucun fichier à installer à côté.

La direction artistique s'inspire de *Zelda: Tears of the Kingdom* : îles célestes, ruines Zonaï aux
runes turquoise, profondeurs éclairées par des racines lumineuses, personnages en ombrage « peint ».
Tout est dessiné par le code (aucune image ni logo repris du jeu).

| Menu principal | Sélection des combattants |
|---|---|
| ![Menu principal](docs/menu.jpg) | ![Sélection](docs/selection.jpg) |
| **Choix de l'arène** | **Combat** |
| ![Arène](docs/arene.jpg) | ![Combat](docs/combat.jpg) |
| **Attaque finale SUPER CLASH** | **Entraînement** |
| ![Attaque finale](docs/attaque_finale.jpg) | ![Entraînement](docs/entrainement.jpg) |
| **Paramètres** | **Résultats** |
| ![Paramètres](docs/parametres.jpg) | ![Résultats](docs/resultats.jpg) |

## Le jeu

- **Combat** : de 2 à 4 combattants, humains (2 sur le clavier + manettes) et/ou **ordinateurs de
  niveau 1 à 9**, de 1 à 5 vies.
- **Entraînement** : vies infinies face à un **mannequin** (immobile, qui saute, qui se protège, ou
  piloté par l'ordinateur), réinitialisation des positions, **compteur de combos** et dégâts du combo,
  affichage des zones de coup. Tout se règle dans le menu pause.
- **6 combattants** avec chacun ses stats (poids, vitesse, gravité, sauts) et ses 4 attaques spéciales :
  | | Style | Spéciales |
  |---|---|---|
  | **BLAZE** | Polyvalent, poings de feu | Boule de feu rebondissante, Poing comète, Uppercut solaire, Tornade |
  | **VOLT** | Rapide et léger | Éclair, Charge foudre (à charger), Zip (dans 8 directions), Tonnerre |
  | **TITAN** | Poids lourd | Poing titan (à charger), Charge bélier (super armure), Forteresse, Séisme |
  | **ARIA** | Épéiste (pointe de l'épée plus forte) | Brise-garde, Lame dansante (3 coups), Envol du dauphin, Contre |
  | **PUFFO** | Flotteur, 5 sauts en l'air | Étoile, Marteau, Tornade étoilée, Pierre (super armure) |
  | **SHADE** | Ninja | 3 Shurikens, Lame éclipse (intangible), Disparition (téléportation), Contre-ombre |
- **Mécaniques à la Smash** : formule de recul de Smash (dépend du %, du poids et des dégâts), arrêt sur
  image à l'impact (*hitlag*), influence directionnelle (*DI*), petits/grands sauts, chute rapide,
  bouclier qui s'use (et se brise !), roulades, esquives au sol et en l'air, saisies + 4 projections,
  rebords (accroche, remontée, attaque, roulade, saut), retombée en culbute + *tech*, chute libre après
  la remontée, attaques smash à charger, super armure, contres, projectiles, plateformes traversables
  et mobiles.
- **La barre d'ulti** : sous forme de cellules d'énergie Zonaï au-dessus de chaque carte de dégâts, elle
  se remplit **en frappant** (0,45 point par % infligé) **et en prenant des coups** (0,35 point par %
  reçu). Pleine, le combattant s'entoure d'une aura turquoise, la carte affiche « ULTI PRÊTE ! » avec le
  bouton à presser, et la touche **Ultime** déclenche l'attaque finale **SUPER CLASH** (cinématique,
  ondes de choc dévastatrices). Les ordinateurs s'en servent aussi.
- **L'Orbe Clash** (l'équivalent de la Smash Ball) : elle apparaît de temps en temps et flotte au-dessus
  de l'arène. Celui qui la brise **remplit d'un coup sa barre d'ulti**.
- **3 arènes** : **Profondeurs** (abîme, racines lumineuses et cristaux), **Sanctuaire** (temple Zonaï en
  ruine au soleil couchant) et **Îles Célestes** (îles flottantes, cascades, plateforme mobile).
- **Animations** : transitions fluides entre les poses, élan et accompagnement des coups, traînées des
  attaques, étirements au saut et tassements à l'atterrissage, capes, écharpes et cheveux qui suivent
  le mouvement, respiration et clignements d'yeux.
- Effets : étincelles, traînées d'éjection, rayon de K.O., tremblement d'écran, bulles quand un
  combattant est hors-champ, caméra dynamique, compte à rebours, « GAME ! », écran de résultats,
  transitions animées entre les écrans.

## Paramètres

Le menu **Paramètres** a quatre onglets, et tout est enregistré automatiquement dans le fichier
`super_clash.cfg`, à côté de l'exécutable :

- **Audio** : volume général, musique, effets sonores (et un bouton pour tester).
- **Affichage** : plein écran, qualité graphique (*Haute* = contours lissés, rendu en double
  résolution ; *Normale* pour les petites machines), secousses de l'écran, noms au-dessus des joueurs,
  compteur d'images (FPS), zones de coup.
- **Jeu** : nombre de vies, Orbe Clash, niveau des ordinateurs par défaut, tout réinitialiser.
- **Commandes** : **toutes les touches des deux claviers sont réassignables** (une touche déjà prise
  est échangée) ; quand une manette est utilisée, l'onglet montre le schéma de la manette.

Si l'ordinateur n'arrive pas à suivre en qualité *Haute*, le jeu passe tout seul en *Normale*.

## Contrôles

Touches par défaut (modifiables dans **Paramètres → Commandes**) :

| Action | Clavier 1 | Clavier 2 | Manette (Xbox / PlayStation / Switch) |
|---|---|---|---|
| Se déplacer / viser | `W` `A` `S` `D` | Flèches | Stick gauche / croix |
| Sauter | `Espace` | `,` ou `Pavé 0` | `X` / `Y` (□ / △) |
| Attaque | `J` | `.` ou `Pavé 1` | `A` (✕, `B` sur Switch) |
| Spéciale | `K` | `/` ou `Pavé 2` | `B` (○, `A` sur Switch) |
| Bouclier / esquive | `L` | `Maj droite` ou `Pavé 3` | `LT` / `RT` (`L2` / `R2`, `ZL` / `ZR`) |
| Saisie | `I` (ou bouclier + attaque) | `Ctrl droit` ou `Pavé 5` | `LB` (`L1`, `L`) |
| Attaque smash | `O` (+ direction) | `;` ou `Pavé 4` | Stick droit |
| **Ultime** (barre pleine) | `U` | `'` ou `Pavé 6` | `RB` (`R1`, `R`) |
| Pause | `Échap` / `P` | `Échap` / `P` | `Start` (`Options`, `+`) |

Les touches correspondent à des **positions** sur le clavier (celles d'un clavier QWERTY) : sur un
clavier AZERTY, le joueur 1 se déplace donc avec `Z` `Q` `S` `D`, et le joueur 2 utilise `;` (saut),
`:` (attaque), `!` (spéciale), `M` (smash) et `ù` (ultime). L'écran **Commandes** affiche le nom réel
des touches de votre clavier.

Les boutons de la manette sont désignés par leur **position** (comme sur une manette Xbox) : sur une
manette Switch, l'attaque est donc le bouton du bas (`B`) et la spéciale celui de droite (`A`).

**Manettes** : branchez-les quand vous voulez, elles sont reconnues automatiquement (Xbox, PlayStation,
Switch Pro, 8BitDo, Logitech... grâce à la base [SDL_GameControllerDB](https://github.com/mdqinc/SDL_GameControllerDB)
intégrée au jeu, licence zlib : `assets/gamecontrollerdb-LICENSE.txt`). Le pavé tactile et les capteurs de
mouvement des manettes PlayStation, que Linux présente comme des manettes à part, sont ignorés.
- La manette (ou le clavier) qui ouvre **Combat** ou **Entraînement** devient celle du joueur 1.
- Dans la sélection des combattants, une autre manette **rejoint la partie en appuyant sur `A` (ou
  `Start`)** : elle prend la première place libre (ou celle d'un ordinateur). On peut toujours choisir
  l'appareil de chaque joueur à la main, en bas de sa carte (« MANETTE 2 », avec le modèle reconnu).
- Si la manette d'un joueur se débranche pendant un combat, le jeu se met en **pause** ; rebranchée,
  elle retrouve son numéro et son joueur.
- Dès qu'une manette est branchée ou utilisée, l'écran montre **ses boutons** (symboles Xbox,
  PlayStation ou Nintendo) ; une touche du clavier fait revenir les touches du clavier. L'onglet
  **Commandes** montre alors le schéma de la manette avec chaque action.

Combinaisons :
- **Attaque smash** : direction + attaque *en même temps* (ou la touche smash). Maintenir pour charger.
- **Attaques normales (tilts)** : maintenir la direction un court instant, *puis* attaquer.
- **Attaque en course** : courir puis attaquer.
- **Aériennes** : attaque + direction en l'air (avant, arrière, haut, bas, neutre).
- **Spéciales** : spéciale + direction (neutre, côté, haut = remontée, bas).
- **Petit saut** : appui bref sur saut. **Chute rapide** : bas pendant la descente.
- **Bouclier + gauche/droite** = roulade, **bouclier + bas** = esquive, **bouclier en l'air** = esquive aérienne.
- **Saisie puis direction** = projection, **saisie puis attaque** = coup de genou.
- **Bas sur une plateforme fine** = la traverser. Au **rebord** : vers la scène pour remonter, saut,
  attaque ou bouclier.

Dans les menus : flèches (ou `WASD`), `Entrée` pour valider, `Échap` pour revenir, `Espace`/`J` pour
modifier une valeur et `K` pour revenir à la valeur précédente. À la manette : stick gauche ou croix,
`A` pour valider ou modifier, `B` pour revenir, `X` pour la valeur précédente, `Start` pour lancer le
combat. La souris marche partout (clic gauche et clic droit). `F11` : plein écran. `F1` en combat :
afficher les zones de coup. Pour voir les boutons d'une manette sans en avoir une :
`super_clash --pad-glyphs xbox` (ou `ps`, `switch`).

## Jouer sans compiler

Deux archives prêtes à lancer se fabriquent avec `scripts/package_release.sh` (dans `dist/`) :

- **Linux 64 bits** (`SuperClashBros-3.0-linux-x64.tar.gz`, Ubuntu 24.04 ou plus récent) : décompresser,
  puis dans un terminal ouvert dans le dossier : `./super_clash`.
- **Windows 64 bits** (`SuperClashBros-3.0-windows-x64.zip`) : décompresser, puis double-cliquer sur
  `super_clash.exe` (si Windows affiche « Windows a protégé votre ordinateur » : « Informations
  complémentaires » puis « Exécuter quand même »).

Chaque archive contient un seul exécutable autonome (polices, sons et icône intégrés), une notice
`LISEZ-MOI.txt` et les licences des polices. La version Windows est compilée depuis Linux avec MinGW-w64
(`cmake/mingw-w64.cmake`).

## Compilation

Prérequis : un compilateur C++17 et CMake ≥ 3.16. [raylib](https://www.raylib.com/) 5.5 est téléchargée
automatiquement depuis GitHub par CMake si elle n'est pas installée.

**Linux** (Debian/Ubuntu) :
```bash
sudo apt install build-essential cmake git libx11-dev libxrandr-dev libxinerama-dev \
                 libxcursor-dev libxi-dev libgl1-mesa-dev libasound2-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/super_clash
```

**macOS** : `xcode-select --install` et `brew install cmake`, puis les trois mêmes commandes `cmake`.

**Windows** : avec Visual Studio 2022 (« Développement Desktop en C++ »), dans l'invite de commandes
développeur :
```bat
cmake -S . -B build
cmake --build build --config Release
build\Release\super_clash.exe
```

**Sans accès à GitHub depuis le terminal** (réseau d'école ou d'entreprise) : téléchargez raylib
avec le navigateur (`https://github.com/raysan5/raylib/archive/refs/tags/5.5.zip`), décompressez-la,
puis indiquez son dossier à CMake :
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/chemin/vers/raylib-5.5
```

### Options de lancement

```
super_clash --demo            # match de démonstration entre 4 ordinateurs
super_clash --screen menu     # démarrer sur un écran : menu, settings, css, stage, training...
super_clash --stage 2         # arène par défaut (0, 1 ou 2)
super_clash --debug           # zones de coup visibles
super_clash --export-audio d  # écrit tous les sons et musiques en .wav dans le dossier d
```

## Tests

La simulation est indépendante du rendu : elle se compile seule et se teste sans écran.

```bash
cmake -S . -B build-core -DCLASH_BUILD_GAME=OFF
cmake --build build-core -j
./build-core/sim_test
```

Les tests vérifient la formule de recul, les coups, les K.O., chaque commande clavier (petit/grand saut,
smash/tilt, aériennes, bouclier, roulade, saisie, rebord, plateformes), la barre d'ulti, l'Orbe Clash et
l'attaque finale,
le mode entraînement, la remontée de chaque personnage depuis le vide, des dizaines de matchs complets
entre ordinateurs, l'équilibrage et le déterminisme.

## Architecture

```
src/core/   simulation pure (aucune dépendance)
  moves.hpp            données : hitboxes, projectiles, attaques, personnages
  roster*.cpp          les 6 combattants et leurs attaques
  stage.cpp            les arènes
  fighter_update.cpp   machine à états des combattants (contrôles)
  fighter_physics.cpp  déplacements, collisions, atterrissages, rebords
  combat.cpp           détection des coups, recul, bouclier, contres, saisies, projectiles
  match.cpp            déroulement du match, K.O., réapparition, barre d'ulti, Orbe Clash, entraînement
  ai.cpp               ordinateur (niveaux 1 à 9) et mannequins
src/game/   interface raylib
  app.cpp              boucle principale, rendu suréchantillonné, transitions
  screen_title.cpp     écran titre et menu principal
  screen_settings.cpp  paramètres (audio, affichage, jeu, touches)
  menus.cpp            sélection des combattants et de l'arène
  screen_results.cpp   résultats
  match_screen.cpp     combat, pause, outils d'entraînement
  controls.cpp         claviers, manettes (numérotation, branchement à chaud), navigation des menus
  ui.cpp               kit d'interface : fonds animés, tuiles, curseurs, titres en relief, boutons de manette
  settings.cpp         paramètres enregistrés et noms des touches
  fonts.cpp            polices intégrées
  render_body.cpp      dessin des personnages (squelette + IK)
  render_fighter.cpp   poses : chaque attaque anime le membre qui porte la hitbox
  render_world.cpp     décors, plateformes, projectiles, aperçus des arènes
  render_hud.cpp       pourcentages, vies, barre d'ulti, bulles hors-champ, compte à rebours
  fx.cpp               particules, rayons de K.O., tremblements, sons
  audio.cpp            synthèse des sons et de la musique
assets/fonts/          polices (intégrées à la compilation par cmake/embed.cmake)
assets/gamecontrollerdb.txt  base des manettes SDL (intégrée elle aussi)
tests/sim_test.cpp     tests sans écran
```

La simulation tourne à 60 images/seconde à pas fixe et est entièrement déterministe.

## Crédits

- Police **Luckiest Guy** par Astigmatic — licence Apache 2.0 (`assets/fonts/LuckiestGuy-LICENSE.txt`).
- Police **Poppins** par Indian Type Foundry — licence SIL Open Font License 1.1 (`assets/fonts/Poppins-OFL.txt`).
- Base de manettes **SDL_GameControllerDB** — licence zlib (`assets/gamecontrollerdb-LICENSE.txt`).
- Direction artistique inspirée de *The Legend of Zelda: Tears of the Kingdom* (Nintendo) ; aucun élément
  du jeu n'est repris, tout est dessiné par le code.
