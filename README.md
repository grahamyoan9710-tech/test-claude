# Super Clash Bros.

Un jeu de combat de plateforme en C++ inspiré de **Super Smash Bros.** : on ne vide pas une barre de vie,
on fait monter le **pourcentage de dégâts** de l'adversaire jusqu'à pouvoir l'**éjecter hors de l'écran**.

![Écran titre](docs/titre.jpg)

Les personnages sont dessinés et animés par le code (squelette + cinématique inverse), les sons et la
musique sont synthétisés au lancement. Les seules ressources sont deux polices libres, intégrées
directement dans l'exécutable : il n'y a aucun fichier à installer à côté.

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
- **L'Orbe Clash** (l'équivalent de la Smash Ball) : elle apparaît de temps en temps et flotte au-dessus
  de l'arène. Frappez-la jusqu'à la briser : votre combattant s'illumine et sa prochaine **spéciale**
  déclenche l'attaque finale **SUPER CLASH** (cinématique, ondes de choc dévastatrices).
- **3 arènes** : Destination Finale, Champ de Bataille, Îles Célestes (plateforme mobile).
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
  est échangée), avec un rappel des commandes manette.

Si l'ordinateur n'arrive pas à suivre en qualité *Haute*, le jeu passe tout seul en *Normale*.

## Contrôles

Touches par défaut (modifiables dans **Paramètres → Commandes**) :

| Action | Clavier 1 | Clavier 2 | Manette |
|---|---|---|---|
| Se déplacer / viser | `W` `A` `S` `D` | Flèches | Stick gauche / croix |
| Sauter | `Espace` | `,` ou `Pavé 0` | `Y` / `X` |
| Attaque | `J` | `.` ou `Pavé 1` | `A` |
| Spéciale | `K` | `/` ou `Pavé 2` | `B` |
| Bouclier / esquive | `L` | `Maj droite` ou `Pavé 3` | `RB` / gâchettes |
| Saisie | `I` (ou bouclier + attaque) | `Ctrl droit` ou `Pavé 5` | `LB` |
| Attaque smash | `O` (+ direction) | `;` ou `Pavé 4` | Stick droit |
| Pause | `Échap` / `P` | `Échap` / `P` | `Start` |

Les touches correspondent à des **positions** sur le clavier (celles d'un clavier QWERTY) : sur un
clavier AZERTY, le joueur 1 se déplace donc avec `Z` `Q` `S` `D`, et le joueur 2 utilise `;` (saut),
`:` (attaque), `!` (spéciale) et `M` (smash). L'écran **Commandes** affiche le nom réel des touches de
votre clavier.

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
modifier une valeur et `K` pour revenir à la valeur précédente. La souris marche partout (clic gauche
et clic droit). `F11` : plein écran. `F1` en combat : afficher les zones de coup.

## Jouer sans compiler

Deux archives prêtes à lancer se fabriquent avec `scripts/package_release.sh` (dans `dist/`) :

- **Linux 64 bits** (`SuperClashBros-2.0-linux-x64.tar.gz`, Ubuntu 24.04 ou plus récent) : décompresser,
  puis dans un terminal ouvert dans le dossier : `./super_clash`.
- **Windows 64 bits** (`SuperClashBros-2.0-windows-x64.zip`) : décompresser, puis double-cliquer sur
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
smash/tilt, aériennes, bouclier, roulade, saisie, rebord, plateformes), l'Orbe Clash et l'attaque finale,
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
  match.cpp            déroulement du match, K.O., réapparition, Orbe Clash, entraînement
  ai.cpp               ordinateur (niveaux 1 à 9) et mannequins
src/game/   interface raylib
  app.cpp              boucle principale, rendu suréchantillonné, transitions
  screen_title.cpp     écran titre et menu principal
  screen_settings.cpp  paramètres (audio, affichage, jeu, touches)
  menus.cpp            sélection des combattants et de l'arène
  screen_results.cpp   résultats
  match_screen.cpp     combat, pause, outils d'entraînement
  ui.cpp               kit d'interface : fonds animés, tuiles, curseurs, titres en relief
  settings.cpp         paramètres enregistrés et noms des touches
  fonts.cpp            polices intégrées
  render_body.cpp      dessin des personnages (squelette + IK)
  render_fighter.cpp   poses : chaque attaque anime le membre qui porte la hitbox
  render_world.cpp     décors, plateformes, projectiles, aperçus des arènes
  render_hud.cpp       pourcentages, vies, bulles hors-champ, compte à rebours
  fx.cpp               particules, rayons de K.O., tremblements, sons
  audio.cpp            synthèse des sons et de la musique
assets/fonts/          polices (intégrées à la compilation par cmake/embed.cmake)
tests/sim_test.cpp     tests sans écran
```

La simulation tourne à 60 images/seconde à pas fixe et est entièrement déterministe.

## Crédits

- Police **Luckiest Guy** par Astigmatic — licence Apache 2.0 (`assets/fonts/LuckiestGuy-LICENSE.txt`).
- Police **Poppins** par Indian Type Foundry — licence SIL Open Font License 1.1 (`assets/fonts/Poppins-OFL.txt`).
