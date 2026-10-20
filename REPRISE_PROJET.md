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
