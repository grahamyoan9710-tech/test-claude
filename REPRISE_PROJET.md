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
| Imports de clips (lignes `sheet`, `assets/sprites/<perso>_*.png`) | session B (pilote Ichigo : armes, clips coupés) | 15/10 17 h 41 UTC |
| Effets (lignes `fx`, `assets/sprites/fx_*.png`) | session B (effets PixelLab d'Ichigo) | 15/10 17 h 41 UTC |
| Équilibrage (`src/core/roster*.cpp`) | session B (coups d'armes d'Ichigo) | 15/10 17 h 41 UTC |
| Version et paquets (README, numéros de version, `dist/`) | session B (v9.5) | 16/10 01 h 40 UTC |

### Journal (le plus récent en haut)
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
