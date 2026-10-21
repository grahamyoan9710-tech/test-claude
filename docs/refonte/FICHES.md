# Refonte : fiches des 17 persos originaux (à valider)

Les persos, leurs armes et leurs transformations n'ont pas encore de nom : tout est relié par **numéro**.
- Perso `NN` : le n° de ta planche `ref_persos.png`.
- Armes `NN-A` et `NN-B` : la paire n° NN de `ref_armes.png`, image de gauche puis de droite.
- Transformation `TNN` : la planche `ref_transformations.png`. Neuf persos en ont une : 01, 02, 03, 06, 08, 10,
  12, 14 et 16.

Les noms des coups sont provisoires : ce sont des descriptions, à changer librement.
Le dossier « 10.5 update » (3 persos et 3 armes) n'est pas traité : il est prévu pour plus tard.

---

## 1. Règles communes

### 1.1 Armes : nouveau fonctionnement (précisé par le propriétaire le 20/10)
- Tout le monde commence le combat **à mains nues**, et réapparaît à mains nues après un KO.
- De temps en temps, l'arme d'un perso **tombe sur l'arène** (son icône, prise sur ta planche). Seul son
  propriétaire peut la prendre.
- Pour la prendre, il faut **se tenir dessus et appuyer sur la touche Arme**. Marcher dessus ne suffit pas.
- On a alors l'arme A. Ensuite, la même touche passe de A à B, puis de B à A, etc. :
  - aucun délai de recharge ;
  - le changement se fait dès que le perso peut agir, pas au milieu d'un coup (la pression attend la fin du coup).
- L'arme reste jusqu'au KO : il n'y a plus de minuteur. Après un KO, une nouvelle arme tombera plus tard.
- Touches (modifiables dans les options) :

  | Contrôle | Touche Arme |
  |---|---|
  | Clavier J1 | `E` |
  | Clavier J2 | `M` (ou 7 sur le pavé numérique) |
  | Manette | `Y` (le saut reste sur `X`) |

- L'IA :
  - va chercher son arme et la prend avec la touche ;
  - prend l'arme de distance quand l'adversaire est loin et celle de mêlée quand il est près ;
  - change parfois d'arme en plein combat.
- Tests : touche sans arme, prise sur place, pas de prise en marchant, A puis B puis A, aucun délai, pas de
  changement pendant un coup, retour à mains nues au KO, l'IA qui prend et change d'arme.

### 1.2 Les états d'une arme (communs aux 34)
| État | Ce qu'on voit |
|---|---|
| Icône | L'image de l'arme seule, prise sur ta planche (HUD, sélection, galerie). |
| Au sol | L'icône de l'arme posée sur l'arène, avec son halo. |
| Prise ou changement | Effet « Apparition » teinté de la couleur de l'arme, et clip du perso qui la saisit. |
| En main | L'arme est tenue dans **tous** les clips du perso : repos, marche, course, saut, chute, bouclier, esquives, dégâts, rebord, saisies et projections. Chaque fiche donne sa pose de repos. |
| Attaque | Traînée du coup : effet PixelLab joué par le jeu, de la couleur indiquée. |
| Éveil (ultime) | Aura de l'arme : effet PixelLab en boucle, d'après l'image de droite de ta planche. |
| Transformé (9 persos) | La même arme, avec l'aura de la transformation par-dessus. |
| Changement ou KO | Effet « Disparition » teinté, puis l'autre arme apparaît (ou rien, au KO). |

**Paires où l'image de droite est celle de gauche enflammée** (03, 05, 06, 07, 09, 10, 11, 13, 14, 15 et 17) :
- l'arme B est la version **éveillée** de l'arme A : même objet, mais un autre jeu de coups, élémentaire et plus
  lourd ;
- ses flammes ou son aura sont un effet joué par le jeu, jamais dessiné dans les clips.

**Armes qui flottent** (04-B le médaillon, 11-A et 11-B les grimoires) :
- le jeu les dessine à côté du perso, comme un familier ;
- le perso garde donc ses clips à mains nues pour ses états, et seuls ses coups sont des clips à part.

### 1.3 Ce que contient chaque kit
- **Mains nues** :
  - les coups normaux de Smash (combo, inclinaisons, smashs, aériens, saisies) avec l'élément du perso ;
  - 4 compétences et un ultime. Pour les 9 persos à transformation, l'ultime à mains nues **est** la
    transformation.
- **Chaque arme** : un combo **M1 de 3 coups**, un M1 aérien (un coup tournant, pas détaillé ici), 4 compétences
  et un ultime. Les compétences sont toujours rangées ainsi :
  - **Neutre** : projectile ou zone ;
  - **Côté** : attaque en avançant ;
  - **Haut** : remontée (elle sert à revenir sur l'arène) ;
  - **Bas** : contre, piège ou zone.
- Avec une arme, tous les coups au sol sont son M1, tous les coups en l'air son M1 aérien, et ses compétences
  remplacent celles des mains nues. C'est le système de kits actuel, inchangé.

### 1.4 Transformations (01, 02, 03, 06, 08, 10, 12, 14, 16)
- Elle se déclenche avec l'ultime à mains nues, quand la jauge est pleine.
- Elle dure 15 s : +15 % de dégâts, un peu moins de recul subi, et les armes restent utilisables.
- Son apparence :
  - une **aura** PixelLab en boucle autour du perso ;
  - une **recoloration** de ses clips (yeux, mèches, liserés), faite à l'import et sans génération ;
  - à l'activation, une **onde de choc** de ta planche d'effets et un clip « transformation ».
- Ce qui n'est pas repris de ta planche : les cheveux longs de T08. Il faudrait refaire ses ~55 clips. Je propose
  l'aura blanche et les yeux rouges, avec les cheveux inchangés.

### 1.5 Tenues : fidèles à la référence (décision du propriétaire, 21/10/2026)
- Tous les persos sont des adultes (18 ans ou plus). Chaque perso reproduit **fidèlement** sa référence
  (`docs/refonte/ref_persos.png` et son image PixelLab de base) : cheveux, visage, couleurs exactes, coupe et longueur
  des vêtements, accessoires, motifs, chaussures, proportions. Ne jamais couvrir, rallonger, raccourcir, remplacer ni
  inventer un vêtement ou un accessoire ; seules les adaptations techniques (animation, cadrage, optimisation) sont
  permises, sans changer l'apparence. Avant de valider un perso : comparer sa base et ses clips à la référence et
  corriger les écarts. Si la référence manque, la demander au propriétaire (ne pas prétendre avoir comparé).
  Les 5 persos déjà faits (01, 02, 03, 07, 08) restent tels quels, sauf demande explicite.

### 1.6 Effets (tous faits avec PixelLab, `vfx_workbench`)
Les effets génériques viennent de ta planche `ref_effets.png`, teintés de la couleur du perso.

| Effet de ta planche | Où il sert |
|---|---|
| Fumée 1 | Départ de course |
| Fumée 2 | Dash |
| Fumée 3 | Saut |
| Fumée 4 | Double saut |
| Fumée 5 | Freinage, glissade |
| Saut 1, Saut 2 | Décollage, impulsion en l'air |
| Atterrissage | Réception d'un saut |
| Dash 1, Dash 2 | Ruées des compétences « Côté » |
| Sustentation | Rebord, vol plané |
| Apparition | Invocation d'arme, réapparition après un KO |
| Disparition | Changement d'arme, KO |

Ondes de choc des transformations :

| Onde | Transformations |
|---|---|
| 1 | T08, T14 |
| 2 | T03, T16 |
| 3 | T02 |
| 4 | T06, T10 |
| 5 | T01, T12 |

Les ondes sont aussi les **ondes d'impact des coups puissants** (précisé le 20/10) : elles partent du sol et
montent. Chaque perso a la sienne, celle de sa transformation (07, sans transformation : onde 4).

Les autres effets sont propres à chaque coup (projectiles, traînées, ultimes, auras) et figurent sur les fiches.
Les effets des menus sont aussi faits avec PixelLab : pétales, étincelles, éclat de sélection, transition.

Règles des effets :
- Aucun effet n'est dessiné par le code. Les anciens effets codés sont supprimés dès qu'ils sont remplacés.
- Aucun effet dans les clips des persos : chaque prompt se termine par « Only <perso> is drawn ». Exception
  (précisée le 20/10) : les traînées des coups rapides et des épées sont gardées ; partout ailleurs, aucune.
- Un clip où un effet est collé au perso est **régénéré**, jamais découpé avec `erase` ou `degrey`.
- Aucun perso coupé au bord du cadre.

### 1.7 Styles de combat à mains nues (demandé le 20/10)
Sans arme, chaque perso se bat dans un art martial de la liste du propriétaire : ses coups (jab, combo, tilts,
smashs, aériens, ruée) en reprennent les gestes. Avec une arme, ce sont les coups du kit de l'arme.

| Perso | Style | Ce qu'on voit |
|---|---|---|
| 01 | Penchak Silat | Garde basse et fluide, frappes à main ouverte, coudes, balayages au sol |
| 02 | Taekwondo | Coups de pied rapides et acrobatiques : circulaires, retourné, tornade, hache |
| 03 | Muay Thaï | Coudes, genoux, tibias, teep, genou sauté |
| 07 | Karaté | Direct, gyaku-zuki, mae-geri, yoko-geri, tranchant de la main |
| 08 | Kung Fu | Coups de poing en chaîne, paumes, griffe du tigre, coup de pied de la grue |

Pour les 12 suivants : Boxe anglaise, Savate, Judo, Kickboxing et Aïkido, puis les styles déjà pris.

---

## 2. Les 17 fiches

### 01 · ombre violette · transformation T01
- **Modèle :** cheveux noirs en bataille, long manteau noir à doublure violette effilochée, pantalon et bottes
  noirs. Gabarit moyen, polyvalent.
- **Effets :** violet (ombre). Tenue : rien à changer.
- **Mains nues :**
  - Neutre « Orbe d'ombre » : une sphère lente ;
  - Côté « Pas de l'ombre » : il traverse l'adversaire en fondu et le frappe dans le dos ;
  - Haut « Ascension spectrale » : téléportation vers le haut ;
  - Bas « Riposte d'ombre » : un contre ;
  - Ultime : **T01**.
- **T01 :** aura de flammes violettes, yeux violets, onde 5.
- **01-A, odachi** (longue lame courbe, ruban violet) :
  - Repos : à deux mains, lame basse en arrière. Traînée violette. Éveil : la lame fume d'ombre.
  - M1 : coupe horizontale → coupe montante → grande coupe descendante.
  - Neutre « Croissant du néant » : une vague d'ombre au sol.
  - Côté « Iaï » : ruée et coupe éclair.
  - Haut « Lune montante » : coupe en spirale qui s'élève.
  - Bas « Garde-lame » : contre puis coupe.
  - Ultime « Mille lunes » : il disparaît, des dizaines de coupes violettes zèbrent la zone, puis il réapparaît
    en rengainant.
- **01-B, dagues jumelles** (lueur violette) :
  - Repos : une dague par main, garde basse. Traînées violettes doubles. Éveil : flammes violettes.
  - M1 : entaille gauche → entaille droite → double estoc en X.
  - Neutre « Dagues spectrales » : deux dagues lancées, qui reviennent.
  - Côté « Assaut fantôme » : ruée à 5 coups.
  - Haut « Vrille » : tourbillon vertical.
  - Bas « Marque d'ombre » : un piège au sol qui explose au contact.
  - Ultime « Danse du néant » : l'adversaire est enfermé dans une sphère d'ombre et frappé de tous côtés, final
    en X.

### 02 · givre stellaire · transformation T02
- **Modèle :** très longs cheveux argentés, ruban bleu, tenue noire et blanche, manteau noir. Gabarit léger,
  rapide en l'air.
- **Effets :** bleu glacé.
- **Tenue :** celle du jeu actuel est gardée (décision du propriétaire, 21/10).
- **Mains nues :**
  - Neutre « Éclat de givre » ;
  - Côté « Glissade gelée » : elle glisse, frappe du pied et laisse du givre ;
  - Haut « Pilier de glace » : elle monte sur un pilier qui jaillit ;
  - Bas « Miroir de glace » : renvoie les projectiles ;
  - Ultime : **T02**.
- **T02 :** aura de givre bleu, tenue éclaircie de blanc, cristaux flottants, onde 3.
- **02-A, lance argentée** (rubans blancs) :
  - Repos : en diagonale, pointe en avant. Traînée bleu pâle. Éveil : la pointe givre.
  - M1 : estoc → double estoc → balayage circulaire.
  - Neutre « Lance de givre » : lancée, elle revient.
  - Côté « Charge polaire » : ruée lance en avant.
  - Haut « Spirale céleste » : elle monte en tournoyant.
  - Bas « Pieux de glace » : des pics sortent du sol devant elle.
  - Ultime « Zéro absolu » : elle plante la lance, une large zone gèle puis éclate en cristaux.
- **02-B, sceptre-étoile** (étoile de cristal bleu, rubans) :
  - Repos : tenu vertical à côté d'elle. Traînée d'étincelles bleues. Éveil : l'étoile brille.
  - M1 : coup de sceptre → revers → l'étoile éclate.
  - Neutre « Étoile filante » : un projectile en arc.
  - Côté « Constellation » : trois étoiles orbitent, puis partent devant.
  - Haut « Saut stellaire » : téléportation en étincelles.
  - Bas « Dôme astral » : un bouclier bref qui repousse.
  - Ultime « Pluie d'étoiles » : des étoiles de glace tombent sur toute l'arène.

### 03 · brasier rouge · transformation T03
- **Modèle :** cheveux noirs à mèches rouges, grande écharpe rouge, manteau noir et rouge ouvert, pantalon noir.
  Gabarit lourd, puissant.
- **Effets :** rouge et orange (feu).
- **Mains nues :**
  - Neutre « Poing ardent » : une boule de feu courte ;
  - Côté « Charge brûlante » : coup d'épaule enflammé ;
  - Haut « Uppercut volcanique » : il monte dans une spirale de feu ;
  - Bas « Éruption » : une explosion au sol autour de lui ;
  - Ultime : **T03**.
- **T03 :** aura de flammes rouges, cornes et yeux d'énergie rouge (effet), onde 2.
- **03-A, grande épée noire** (veines rouges) :
  - Repos : sur l'épaule. Traînée rouge sombre.
  - M1 : taille lourde → revers → frappe au sol.
  - Neutre « Onde tranchante » : une vague de feu au sol.
  - Côté « Brise-garde » : ruée qui casse les boucliers.
  - Haut « Lame ascendante » : une coupe montante qui l'emporte.
  - Bas « Séisme » : il plante l'épée, une onde de choc part.
  - Ultime « Exécution écarlate » : un coup vertical géant fend l'arène, dans une colonne de flammes.
- **03-B, grande épée éveillée** (la même, embrasée) :
  - M1 : coupe enflammée → coupe en croix → frappe explosive.
  - Neutre « Météore » : une boule de feu lancée de la lame.
  - Côté « Traînée de feu » : une ruée qui laisse des flammes au sol.
  - Haut « Phénix » : il monte, entouré d'un oiseau de feu.
  - Bas « Cercle de brasier » : un anneau de flammes autour de lui.
  - Ultime « Apocalypse » : il lance l'épée au ciel, elle retombe en comète, explosion géante.

### 04 · astrale · pas de transformation
- **Modèle :** longues couettes argentées, nœuds noirs, robe blanche et noire. Petit gabarit, très mobile.
- **Effets :** violet et doré (étoiles).
- **Tenue :** celle de la référence, sans changement.
- **Mains nues :**
  - Neutre « Étincelle » ;
  - Côté « Pirouette » : coup de pied tournant ;
  - Haut « Saut d'étoile » : double impulsion ;
  - Bas « Bulle » : une barrière qui renvoie ;
  - Ultime « Constellation » : un réseau d'étoiles piège l'adversaire, puis explose.
- **04-A, anneau-chakram** (quatre pointes, cristaux pendants) :
  - Repos : tenu d'une main, levé. Traînée violette circulaire.
  - M1 : lancer court qui revient → revers → double rotation.
  - Neutre « Boomerang » : l'anneau va et revient.
  - Côté « Roue tranchante » : l'anneau roule au sol.
  - Haut « Anneau porteur » : elle monte, accrochée à l'anneau qui tourne.
  - Bas « Orbite » : l'anneau tourne autour d'elle.
  - Ultime « Anneaux célestes » : six anneaux tournoient autour de l'adversaire et se resserrent.
- **04-B, médaillon-orbe** (roue dorée, orbe violet), qui **flotte** au-dessus de son épaule :
  - Traînée : rayons violets.
  - M1 : rayon court → second rayon → éclat de l'orbe.
  - Neutre « Rayon arcane » : un laser horizontal.
  - Côté « Sceau » : un glyphe qui avance.
  - Haut « Lévitation » : elle s'élève.
  - Bas « Gravité » : attire les adversaires proches.
  - Ultime « Œil céleste » : l'orbe grandit au-dessus de l'arène et tire un rayon géant.

### 05 · flamme bleue · pas de transformation
- **Modèle :** cheveux bleu nuit, long manteau noir à pans bleu cyan, gants. Gabarit moyen, grande allonge.
- **Effets :** bleu (eau, flamme bleue).
- **Mains nues :**
  - Neutre « Flamme bleue » ;
  - Côté « Ruée de l'eau » ;
  - Haut « Geyser » ;
  - Bas « Vague » : une onde autour de lui ;
  - Ultime « Raz-de-marée » : une vague géante traverse l'arène.
- **05-A, lance sombre** (rubans bleus) :
  - Repos : à deux mains, pointe basse. Traînée bleue.
  - M1 : estoc → estoc → balayage.
  - Neutre « Javelot » : il lance une lance d'eau.
  - Côté « Percée » : une ruée.
  - Haut « Saut du dragon » : un bond, lance vers le ciel.
  - Bas « Tourbillon » : la lance tournoie devant lui.
  - Ultime « Dragon des abysses » : un dragon d'eau sort de la lance et traverse l'arène.
- **05-B, lance éveillée** (flammes bleues) :
  - M1 : estoc enflammé → double estoc → balayage de flammes bleues.
  - Neutre « Comète bleue ».
  - Côté « Charge flamboyante ».
  - Haut « Pilier de flammes ».
  - Bas « Pluie de lances » : trois lances tombent devant lui.
  - Ultime « Brasier azur » : il plante la lance, une colonne de flammes bleues s'élève et une onde part.

### 06 · démon rose · transformation T06
- **Modèle :** très longs cheveux roses, cornes noires, robe noire et rouge, bottes rouges. Gabarit moyen,
  technique.
- **Effets :** rose et magenta.
- **Mains nues :**
  - Neutre « Orbe cramoisi » ;
  - Côté « Griffe » : une ruée griffe ;
  - Haut « Ailes d'ombre » : ailes en effet ;
  - Bas « Malédiction » : une marque qui ralentit ;
  - Ultime : **T06**.
- **T06 :** aura magenta, mèches et yeux plus vifs, onde 4.
- **06-A, faux sombre** :
  - Repos : sur l'épaule, lame derrière. Traînée rose.
  - M1 : fauche → revers → fauche circulaire.
  - Neutre « Croissant rose » : une lame d'énergie lancée.
  - Côté « Moisson » : une ruée fauchante.
  - Haut « Rotor » : la faux tournoie et l'élève.
  - Bas « Crochet » : attire l'adversaire.
  - Ultime « Moisson écarlate » : une fauche géante circulaire balaie l'arène.
- **06-B, faux éveillée** (aura rose) :
  - M1 : fauche → double fauche → coup de manche.
  - Neutre « Lames jumelles » : deux croissants.
  - Côté « Chaîne de ronces ».
  - Haut « Envol démoniaque ».
  - Bas « Cercle maudit » : une zone qui blesse.
  - Ultime « Tempête de pétales » : une pluie de lames roses.

### 07 · foudre rouge · pas de transformation
- **Modèle :** longue queue de cheval noire et rouge, tenue noire et rouge, manches larges, bottes. Gabarit léger,
  très rapide.
- **Effets :** rouge (foudre).
- **Tenue :** celle du jeu actuel est gardée (décision du propriétaire, 21/10).
- **Mains nues :**
  - Neutre « Éclair rouge » : un projectile rapide ;
  - Côté « Pas-éclair » ;
  - Haut « Coup de pied foudre » ;
  - Bas « Décharge » : une zone autour d'elle ;
  - Ultime « Tempête écarlate » : un orage de foudres rouges.
- **07-A, épée large** (rubans rouges) :
  - Repos : d'une main, lame en avant. Traînée rouge.
  - M1 : coupe → coupe → estoc.
  - Neutre « Lame-éclair » : une onde rouge.
  - Côté « Iaï foudroyant ».
  - Haut « Saut-foudre ».
  - Bas « Riposte » : un contre.
  - Ultime « Cent éclairs » : ruée en zigzag à travers l'arène.
- **07-B, épée éveillée** (éclairs rouges) :
  - M1 : coupe électrique → coupe → estoc foudroyant.
  - Neutre « Lance de foudre ».
  - Côté « Tranchant écarlate ».
  - Haut « Spirale rouge ».
  - Bas « Foudre au sol ».
  - Ultime « Jugement rouge » : une foudre géante s'abat sur l'adversaire.

### 08 · lumière blanche · transformation T08
- **Modèle :** cheveux blancs en bataille, long manteau blanc à pans gris, pantalon noir, bottes. Gabarit moyen,
  équilibré.
- **Effets :** blanc et or (lumière, vent).
- **Mains nues :**
  - Neutre « Plume de lumière » ;
  - Côté « Rafale » : une ruée de vent ;
  - Haut « Ailes de vent » ;
  - Bas « Barrière de lumière » ;
  - Ultime : **T08**.
- **T08 :** aura blanc-bleu, yeux rouges, onde 1. Voir la remarque sur les cheveux au §1.4.
- **08-A, épée blanche et or** (rubans) :
  - Repos : d'une main, pointe vers le bas. Traînée blanche.
  - M1 : coupe → coupe → coupe en croix.
  - Neutre « Lame de lumière » : une vague.
  - Côté « Éclair blanc » : une ruée.
  - Haut « Ascension sacrée ».
  - Bas « Contre sacré ».
  - Ultime « Jugement céleste » : une épée géante de lumière tombe du ciel.
- **08-B, lance dorée** (hallebarde) :
  - Repos : à deux mains, verticale. Traînée dorée.
  - M1 : estoc → balayage → estoc tournant.
  - Neutre « Lance de lumière » : lancée.
  - Côté « Charge du paladin ».
  - Haut « Rayon montant ».
  - Bas « Pilier sacré » : une colonne de lumière devant lui.
  - Ultime « Lances divines » : une pluie de lances dorées.

### 09 · glace marine · pas de transformation
- **Modèle :** longs cheveux bleu ciel, nœuds bleus, tenue blanche et bleue, manteau bleu étoilé. Gabarit moyen,
  combat rapproché.
- **Effets :** bleu ciel (eau, glace).
- **Mains nues :**
  - Neutre « Bulle » ;
  - Côté « Glissade » ;
  - Haut « Jet d'eau » : une propulsion ;
  - Bas « Flaque gelée » : fait glisser ;
  - Ultime « Vague géante ».
- **09-A, gantelets** (noirs, liserés bleus) :
  - Repos : poings levés. Traînée bleue courte.
  - M1 : direct → crochet → uppercut.
  - Neutre « Poing-canon » : une onde de choc.
  - Côté « Ruée de poings ».
  - Haut « Uppercut glacé ».
  - Bas « Frappe au sol » : des pics de glace.
  - Ultime « Mille poings » : une rafale, puis un direct final.
- **09-B, gantelets éveillés** (aura bleue) :
  - M1 : direct lourd → crochet glacé → double marteau.
  - Neutre « Poing de glace » : un poing géant lancé.
  - Côté « Bélier ».
  - Haut « Fusée ».
  - Bas « Iceberg » : un bloc de glace qui surgit.
  - Ultime « Glacier » : l'adversaire est enfermé dans la glace, puis brisé d'un coup énorme.

### 10 · tempête de jade · transformation T10
- **Modèle :** cheveux sombres vert-bleu, écharpe et manteau vert sombre, pantalon gris. Gabarit moyen, zoneur.
- **Effets :** vert jade et cyan (vent, éclairs).
- **Mains nues :**
  - Neutre « Lame de vent » ;
  - Côté « Rafale » ;
  - Haut « Tornade » ;
  - Bas « Contre-vent » : repousse ;
  - Ultime : **T10**.
- **T10 :** aura cyan et éclairs, onde 4.
- **10-A, lance de jade** (rubans) :
  - Repos : sur l'épaule. Traînée verte.
  - M1 : estoc → estoc → balayage en tourbillon.
  - Neutre « Lance-vent ».
  - Côté « Percée du vent ».
  - Haut « Saut de la grue ».
  - Bas « Moulinet ».
  - Ultime « Ouragan de jade » : une tornade géante.
- **10-B, lance éveillée** (flammes vertes) :
  - M1 : estoc électrique → estoc → balayage foudroyant.
  - Neutre « Javelot-éclair ».
  - Côté « Ruée du tonnerre ».
  - Haut « Éclair ascendant ».
  - Bas « Champ électrique ».
  - Ultime « Dragon de tempête » : un dragon d'éclairs verts traverse l'arène.

### 11 · arcane violette · pas de transformation
- **Modèle :** longs cheveux blancs, nœuds violets, robe noire et violette. Petit gabarit, magicienne à distance.
- **Effets :** violet.
- **Tenue :** celle de la référence, sans changement.
- **Mains nues :**
  - Neutre « Sphère magique » ;
  - Côté « Glyphe » : il avance ;
  - Haut « Lévitation » ;
  - Bas « Sceau » : un piège ;
  - Ultime « Grand sortilège » : un cercle magique géant, puis une explosion.
- **11-A, grimoire violet**, qui **flotte** devant elle, ouvert :
  - Traînée : des pages.
  - M1 : trois traits magiques.
  - Neutre « Flèches arcaniques ».
  - Côté « Pages tranchantes ».
  - Haut « Portail » : téléportation.
  - Bas « Cercle de protection ».
  - Ultime « Bibliothèque interdite » : une pluie de pages-lames.
- **11-B, grimoire éveillé** (aura violette, l'œil s'ouvre), qui **flotte** aussi :
  - M1 : trois rayons courts.
  - Neutre « Œil du grimoire » : un rayon.
  - Côté « Chaînes magiques » : immobilise.
  - Haut « Ascension runique ».
  - Bas « Trou noir » : attire.
  - Ultime « Invocation » : une main d'ombre géante sort du livre et écrase.

### 12 · soleil d'or · transformation T12
- **Modèle :** très longs cheveux noirs à reflets dorés, tenue noire, blanche et or. Gabarit moyen, épéiste.
- **Effets :** or (sable).
- **Mains nues :**
  - Neutre « Sable doré » ;
  - Côté « Pas de sable » ;
  - Haut « Tourbillon de sable » ;
  - Bas « Sables mouvants » ;
  - Ultime : **T12**.
- **T12 :** aura dorée, tenue rehaussée d'or, onde 5.
- **12-A, katana** (pompon blanc) :
  - Repos : rengainé, la main sur la poignée (posture iaï). Traînée dorée fine.
  - M1 : coupe → coupe → iaï.
  - Neutre « Vague dorée ».
  - Côté « Iaï ».
  - Haut « Coupe ascendante ».
  - Bas « Contre ».
  - Ultime « Lame du crépuscule » : une coupe horizontale géante balaie l'arène.
- **12-B, grande lame dorée** (rubans) :
  - Repos : sur l'épaule. Traînée dorée large.
  - M1 : coupe lourde → revers → fendoir.
  - Neutre « Lame d'or » : un boomerang.
  - Côté « Charge du lion ».
  - Haut « Saut-fendoir ».
  - Bas « Pilier de sable ».
  - Ultime « Soleil couchant » : une lame dorée gigantesque s'abat.

### 13 · corbeau · pas de transformation
- **Modèle :** cheveux blancs, torse nu musclé, grand manteau de plumes noires doublé de rouge, pantalon noir.
  Gabarit très lourd.
- **Effets :** noir et rouge (plumes).
- **Mains nues :**
  - Neutre « Plumes noires » ;
  - Côté « Charge du corbeau » ;
  - Haut « Ailes de corbeau » : un vol bref ;
  - Bas « Écrasement » ;
  - Ultime « Nuée de corbeaux » : un essaim traverse l'arène.
- **13-A, hache de guerre** :
  - Repos : sur l'épaule. Traînée rouge sombre.
  - M1 : taille → revers → frappe au sol.
  - Neutre « Hache lancée » : elle revient.
  - Côté « Charge broyeuse ».
  - Haut « Moulinet montant ».
  - Bas « Séisme ».
  - Ultime « Exécuteur » : il saisit l'adversaire et abat la hache trois fois.
- **13-B, hache éveillée** (en feu) :
  - M1 : taille enflammée → revers → frappe explosive.
  - Neutre « Comète de feu ».
  - Côté « Tourbillon de feu ».
  - Haut « Envol ardent ».
  - Bas « Fissure de lave ».
  - Ultime « Guillotine » : une hache géante tombe du ciel.

### 14 · rose cramoisie · transformation T14
- **Modèle :** couettes roses, nœuds noirs, robe noire et blanche, bottes blanches. Petit gabarit, rapide.
- **Effets :** rose.
- **Tenue :** celle de la référence, sans changement.
- **Mains nues :**
  - Neutre « Pétale » ;
  - Côté « Pirouette » ;
  - Haut « Ruban » : elle s'accroche et monte ;
  - Bas « Ronces » : un piège ;
  - Ultime : **T14**.
- **T14 :** aura rose vif, robe rehaussée de magenta, onde 1.
- **14-A, faucilles jumelles à chaîne** :
  - Repos : une faucille par main, chaîne pendante. Traînée rose.
  - M1 : entaille → entaille → double lancer.
  - Neutre « Chaîne » : attrape et attire.
  - Côté « Toupie » : une roue de lames.
  - Haut « Grappin » : elle monte sur la chaîne.
  - Bas « Ronces d'acier » : la chaîne au sol.
  - Ultime « Valse des lames » : elle enchaîne l'adversaire et le fait tournoyer.
- **14-B, faucilles éveillées** (aura rose) :
  - M1 : entaille → croix → lancer tournoyant.
  - Neutre « Croissants jumeaux ».
  - Côté « Tempête de pétales ».
  - Haut « Spirale rose ».
  - Bas « Jardin de lames ».
  - Ultime « Floraison » : une fleur géante de lames s'ouvre.

### 15 · nuit lunaire · pas de transformation
- **Modèle :** longs cheveux noir-violet, cornes noires, tenue noire et violette, manteau à pans violets. Gabarit
  moyen, contrôle de zone.
- **Effets :** violet nuit (lune, gravité).
- **Mains nues :**
  - Neutre « Orbe lunaire » ;
  - Côté « Ombre glissante » ;
  - Haut « Ailes de nuit » ;
  - Bas « Gravité » ;
  - Ultime « Nuit éternelle » : l'arène s'assombrit, une lune tombe et explose.
- **15-A, faux de nuit** :
  - Repos : tenue haute derrière elle. Traînée violette.
  - M1 : fauche → fauche → crochet.
  - Neutre « Croissant de lune ».
  - Côté « Fauche-ombre ».
  - Haut « Hélice ».
  - Bas « Puits » : attire.
  - Ultime « Éclipse lunaire » : une lune noire tranche toute l'arène.
- **15-B, faux éveillée** (aura violette) :
  - M1 : fauche → double fauche → coup de pointe.
  - Neutre « Lames du vide ».
  - Côté « Faille » : téléportation, puis coupe.
  - Haut « Spirale du vide ».
  - Bas « Singularité ».
  - Ultime « Trou noir » : aspire tout, puis explose.

### 16 · soleil levant · transformation T16
- **Modèle :** cheveux châtain doré, long manteau noir et or ouvert, pantalon noir. Gabarit lourd, paladin.
- **Effets :** or et flamme.
- **Mains nues :**
  - Neutre « Éclat solaire » ;
  - Côté « Charge » ;
  - Haut « Uppercut lumineux » ;
  - Bas « Éruption solaire » ;
  - Ultime : **T16**.
- **T16 :** aura de flammes dorées, onde 2.
- **16-A, épée noire et or** (ruban) :
  - Repos : d'une main, pointe en avant. Traînée dorée.
  - M1 : coupe → coupe → estoc.
  - Neutre « Rayon solaire » : une vague.
  - Côté « Lame de l'aube ».
  - Haut « Ascension ».
  - Bas « Garde solaire ».
  - Ultime « Zénith » : il lève l'épée, un rayon de soleil vertical géant tombe.
- **16-B, bouclier-soleil** (disque à pointes, lames pendantes) :
  - Repos : au bras, devant lui. Traînée dorée.
  - M1 : coup de bouclier → revers → charge.
  - Neutre « Disque lancé » : un boomerang.
  - Côté « Bélier solaire ».
  - Haut « Disque porteur ».
  - Bas « Rempart » : renvoie les projectiles.
  - Ultime « Supernova » : le bouclier devient un soleil et explose.

### 17 · émeraude · pas de transformation
- **Modèle :** longs cheveux vert menthe, nœud vert, robe noire et verte. Gabarit léger, tireuse.
- **Effets :** vert émeraude (nature, vent).
- **Mains nues :**
  - Neutre « Graine » ;
  - Côté « Liane » : un fouet ;
  - Haut « Feuille planante » ;
  - Bas « Racines » : un piège ;
  - Ultime « Forêt » : des arbres géants surgissent du sol.
- **17-A, arc à plumes vertes** :
  - Repos : tenu bas de la main gauche. Traînée verte.
  - M1 : coup d'arc → revers → tir à bout portant.
  - Neutre « Flèche chargée ».
  - Côté « Tir en roulade ».
  - Haut « Flèche-grappin ».
  - Bas « Flèche-piège ».
  - Ultime « Pluie de flèches ».
- **17-B, arc éveillé** (ailes de lumière, étoile) :
  - M1 : coup d'arc → revers → tir d'étoile.
  - Neutre « Flèche-étoile » : perçante.
  - Côté « Triple tir ».
  - Haut « Envol du faucon ».
  - Bas « Flèche plantée » : une zone de lumière.
  - Ultime « Flèche céleste » : un rayon émeraude traverse l'arène.

---

## 3. Menus

Tous les menus sont refaits dans le style de `menu_accueil.png` et `menu_selection.png`.

**Écran titre**
- Fond : tes images 1, 2 et 3 (`menu_fond_*.png`), une par visite : chaque lancement ou retour au menu passe à
  la suivante (1, 2, 3, 1…), retenue dans les réglages (`menu_bg`).
- En haut à gauche : le logo.
- Une colonne de boutons inclinés, avec leurs icônes : le bouton choisi est rouge, les autres sont sombres.
- En bas à gauche : la carte « Arène à la une ».
- En haut à droite : une astuce avec sa touche (par exemple la touche Arme).
- En bas à droite : la version et la roue des options.

| Bouton | Mène à |
|---|---|
| JOUER | Combat contre l'ordinateur |
| ENTRAÎNEMENT | Entraînement |
| VERSUS | 2 à 4 joueurs en local |
| EN LIGNE | Voir les questions |
| PERSONNAGES | Galerie des 17 : armes, transformation, coups |
| ARÈNES | Aperçu des arènes |
| OPTIONS | Options |
| QUITTER | Quitter |

**Sélection** (comme `menu_selection.png`)
- Le logo.
- J1 : un grand portrait à gauche et son perso animé ; l'adversaire à droite.
- Une grille de 9 + 9 cases : les 17 persos et « ? » pour un perso aléatoire.
- Les cases J1 à J4.
- L'aide des touches en bas : Confirmer, Retour, Apparences, Couleurs, Paramètres combat, Commencer.
- Tant qu'ils n'ont pas de nom, les persos s'affichent « N° 03 ».

**Les autres écrans** (arène, options, pause, résultats) prennent le même style.

**Images**
- Le logo et les icônes sont détourés de `menu_accueil.png`, par différence avec `menu_fond_1.png`, qui a le même
  décor.
- Les portraits sont tirés de ta planche des persos.
- Les panneaux et les boutons sont dessinés par le jeu. Ce sont des éléments d'interface, pas des effets.
- Tous les effets de menu sont faits avec PixelLab.

---

## 4. Clips, budget PixelLab et lots

Clips par perso, d'après la chaîne actuelle :

| Élément | Clips | Contenu |
|---|---|---|
| Mains nues | ~55 | 50 états et coups normaux, 4 compétences, l'ultime ou la transformation |
| Arme tenue en main | 44 | La prise, 34 états avec l'arme, puis M1 ×3, M1 aérien, 4 compétences et l'ultime |
| Arme éveillée (plan recommandé, remplacé) | 10 | La prise et ses 9 coups ; elle reprend les états de son arme A |
| Arme flottante | 10 | La prise et ses 9 coups ; le jeu la dessine à côté du perso |

Une transformation ne demande aucun clip en plus : c'est le clip d'ultime, plus la recoloration et l'aura.

| Plan | Clips | Générations (≈ 1,25 par clip) |
|---|---|---|
| Recommandé (abandonné) : les armes éveillées reprennent les états de leur arme A | ~1 990 | ~2 550 |
| **Complet (choisi le 20/10)** : chaque arme B a son propre modèle, exactement comme sur la planche, dans ses 34 états | ~2 330 | **~3 000** |

En plus :
- les 17 images de base (le perso de profil, en garde, d'après ta planche, tenue corrigée) : ~70 générations ;
- les effets (`vfx_workbench`) ne coûtent rien, dont la banque commune des 18 effets de déplacement (§1.6).

Lots du plan complet :

| Lot | Persos | Clips | Générations |
|---|---|---|---|
| 1 | 03, 02, 01 (ceux de tes menus) | 429 | ~550 |
| 2 | 07, 08, 17 | 429 | ~540 |
| 3 | 04, 05, 06 | 395 | ~500 |
| 4 | 09, 10, 11 | 361 | ~450 |
| 5 | 12, 13, 14 | 429 | ~540 |
| 6 | 15, 16 | 286 | ~360 |

Il reste 2 510 générations jusqu'au 13/11, puis le compteur repart à 5 000. Les lots 1 à 4 passent avant le 13/11,
les lots 5 et 6 après la recharge. J'annonce le compte exact avant chaque lot, après `pxcli.py jobs` et
`pxcli.py balance`.
