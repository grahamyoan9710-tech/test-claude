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
| Effets (lignes `fx`, `assets/sprites/fx_*.png`) | libre | |
| Équilibrage (`src/core/roster*.cpp`) | libre | |
| Version et paquets (README, numéros de version, `dist/`) | libre | |

### Journal (le plus récent en haut)
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
