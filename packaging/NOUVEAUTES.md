## Installer

- **Linux** : télécharger `ClashHorizon-linux-x64.tar.gz`, le décompresser, puis lancer `./clash_horizon` dans le
  dossier `ClashHorizon` (Ubuntu 24.04 ou plus récent).
- **Windows** : télécharger `ClashHorizon-windows-x64.zip`, le décompresser, puis double-cliquer sur
  `clash_horizon.exe` (si Windows affiche « Windows a protégé votre ordinateur » : « Informations complémentaires »,
  puis « Exécuter quand même »).

## Nouveautés

Version 14.0 : **la compétence vers le haut est corrigée, et le jeu est plus léger**.

- Compétence vers le haut (tous les persos) : le perso ne devient plus sombre ni bleuté pendant sa chute, et ne
  « fait plus de squats » en l'air. Il garde la pose de la fin de sa spirale jusqu'au sol, et ne se tient plus
  debout dans le vide à la fin de la montée.
- Lumière des effets sur les persos : les zones les plus sombres gardent leur couleur (plus de teinte olive sous
  une lumière jaune).
- L'écran titre démarre plus vite : il ne charge plus les animations de tous les persos d'un coup.
- RTX : plus de fuite de mémoire de la carte graphique sur les PC qui ne gèrent pas certains formats ; le porteur de
  la Clash Orb coûte beaucoup moins cher à dessiner.
- Corrigé : un perso pouvait rester bloqué en remontant d'un rebord ; l'ultime n'est plus perdu s'il ne peut pas
  partir.
- Contient la 13.9.

Version 13.9 : **en RTX, les effets ne virent plus au blanc**.

- Les gros effets (l'ultime de Hakuen surtout) blanchissaient tout l'écran en RTX, avec un grain de poussière
  lumineuse dans le ciel : leur lumière est maintenant plafonnée, dans sa propre couleur.
- Le décor éclairé par un effet garde ses couleurs au lieu de passer au blanc.
- Les petites particules des effets restent telles qu'elles sont dessinées : plus de points blancs.
- Contient la 13.8 (pixel art à la sélection et à la victoire, visages dézoomés).

Version 13.8 : **le pixel art revient, les visages sont dézoomés**.

- Sélection des persos : la fiche de chaque joueur montre de nouveau le perso en pixel art à côté de ses stats.
- Écran de victoire : le gagnant est de nouveau en pixel art.
- Tous les visages ronds (carte de combat à côté du %, petites vies, classement, « Règles & mode ») et la tuile de
  Hakuen sont dézoomés : on voit toute la tête, cheveux compris.
- Le portrait manga reste sur la tuile de la sélection et dans l'ultime.

Version 13.7 : **nouvelle DA « Encre d'horizon » avec les portraits manga**.

- Sélection des persos : grandes tuiles penchées avec le visage de chacun, une fiche par joueur avec son portrait,
  ses stats (puissance, vitesse, poids, saut, aérien) et ses deux armes. Deux joueurs : deux grandes fiches ;
  trois ou quatre : quatre fiches côte à côte.
- Le bouton « Règles & mode » (en haut à droite, touche R ou Y sur la manette) ouvre le mode de jeu (chacun pour
  soi ou en équipes), les vies, l'orbe, les armes, et pour chaque joueur : humain ou IA, sa manette ou son niveau,
  son équipe.
- En combat, les cartes des joueurs sont transparentes façon Smash : le visage du perso, ses vies en petits
  visages, et le pourcentage dans une nouvelle police (Exo 2, libre de droits).
- L'ultime montre le portrait qui sort de la bande, avec une bande des yeux au-dessus.
- L'écran de victoire montre le grand portrait du gagnant dans sa couleur, et les visages dans le classement.
- Seul Hakuen a déjà son portrait ; les autres persos gardent leur image pixel en attendant leur arrivée.

Version 13.6 : **les sols suivent le dessin**.

- Toits Kurenai et Citadelle de Givre : le sol s'arrête au bord du dessin. Les persos ne marchent plus dans le vide
  et ne s'accrochent plus au bord en l'air pour remonter. Le décor ne bouge pas.
- En jeu, F1 montre aussi les zones de sol des plateformes (en bleu).

Version 13.5 : **RTX : plateformes éclairées, ombres au sol**.

- Le dessus des plateformes fines et des petites îles prend la lumière de la lune ou du soleil comme le sol
  principal (elles restaient sombres).
- Chaque perso, l'orbe et les armes posées ont une ombre douce sur le sol ou la plateforme juste sous eux, un peu
  étirée à l'opposé de la lune ou du soleil, plus pâle et plus floue quand ils sautent. Elle est lisse et immobile :
  ni grain, ni rayon noir.
- Les ombres des plateformes sur le sol sont de nouveau plus marquées, toujours lisses.

Version 13.4 : **un nouvel orage pour l'ultime de Hakuen**.

- L'ultime « Forge de l'aube » est un vrai orage : nuages au-dessus (le haut sort de l'écran quand la caméra est
  proche), cercle au sol, gros éclair, onde au sol. Il a le rouge de ses compétences (moins rouge sang qu'avant).
- Les éclairs frappent deux fois plus vite et tombent toutes les 10 images au lieu de 14 ; chacun tombe toujours là
  où est l'adversaire quand il apparaît.
- La zone touchée suit le dessin, un peu plus large (rayon 60 au lieu de 50).

Version 13.1 : **des éjections plus violentes**.

- La traînée d'un perso éjecté est bien plus grosse (bouffées 1,6 fois plus grandes, une à chaque image) : elle
  forme une colonne de fumée continue derrière lui.
- L'explosion au bord de l'écran quand un perso est éjecté (KO) est 1,7 fois plus grande.
- Les effets de mouvement (saut, double saut, atterrissage, dash) sont 5 % plus grands.

Version 13.0 : **RTX : ombres plus douces, couleurs moins chaudes**.

- Plus de faisceaux d'ombre ni de lumière dans l'air : la lune ou le soleil n'y trace plus de rayons.
- Les ombres de la lune ou du soleil sont lisses et immobiles (plus de « poussière » pixelisée), et jamais noires.
- Les persos ne projettent plus de longue ombre en biais sur les sols, et l'ombre d'une plateforme ne traverse plus le
  sol en diagonale.
- Lunes et soleils presque blancs : les arènes chaudes ne virent plus à l'orange, et les zones sans lumière sont un
  peu moins sombres.

Version 12.9 : **Hakuen marche plus calmement**.

- Ses pas sont deux fois moins rapides quand elle marche : ils ne s'agitent plus pour la distance parcourue.
- Son coup de pied vers le haut sera refait dès que PixelLab le permettra (le 13/11).

Version 12.8 : **RTX : la lune ou le soleil de chaque arène, ses ombres, des reflets partout**.

- Avec *RTX : Oui*, chaque arène a sa lumière principale : la grande lune de l'Abîme Lunaire, le soleil couchant à
  gauche de la Citadelle de Givre, le soleil derrière la falaise du Canyon Écarlate, la lune rouge des Toits Kurenai
  et le soleil haut du Monastère des Nuées. Elle éclaire plus fort près d'elle, flotte dans l'air, et les persos et
  les plateformes y projettent leur ombre.
- Plus d'images fantômes ni de « poussière » : la lumière calculée est nettoyée à chaque image, et ce qui bouge ne
  laisse plus de traînée.
- Reflets RTX sur toutes les plateformes, selon leur matière : sol poli, tuiles glacées, planches laquées ou
  mouillées, plus ou moins floues ; le grès reste mat.
- La fumée et la poussière des déplacements n'ont plus de bloom et n'éclairent plus le décor.
- La traînée d'éjection fonce avec les pourcentages du perso et la distance parcourue.
- Plus aucun effet PixelLab : l'apparition au respawn, la fin de transformation et l'orbe brisée sont retirées.

Version 12.7 : **l'ultime de Hakuen, une pluie d'éclairs à esquiver**.

- « Forge de l'aube » fait tomber cinq éclairs rouge sang, l'un après l'autre. Chacun marque le sol sous
  l'adversaire d'un cercle, puis frappe toute la colonne à cet endroit, du ciel jusqu'au sol.
- Les éclairs ne suivent plus l'adversaire : celui qui bouge vite à temps peut les esquiver. Le dernier éjecte.

Version 12.6 : **Hakuen reprend ses couleurs d'origine**.

- Hakuen retrouve sa tenue noire, or et rouge (clips et portrait). Le reste de la 12.4 et de la 12.5 est gardé :
  ultime en éclair rouge sang, effets communs ParticleFX et RTX plus profond.

Version 12.5 : **RTX plus profond, façon path tracing**.

- Avec *RTX : Oui*, le décor est plus sombre et légèrement plus chaud là où aucune lumière n'arrive, un peu plus vers
  les bords de l'écran. Le blanc de la neige et des ciels clairs reste blanc.
- La lumière des effets est bien plus forte : deux fois plus de rayons, elle porte plus loin, rebondit davantage et
  flotte dans l'air autour de l'effet.
- Les reflets des sols brillants sont presque deux fois plus forts (le grès du Canyon Écarlate reste mat).
- Les persos gardent leurs couleurs, sans être salis : à peine plus sombres, avec leur liseré de lumière en aplats.
- Le RTX ne se coupe plus tout seul quand le jeu rame : il est très gourmand pour l'instant, il sera optimisé ensuite.
- Avec le RTX, le halo des effets et les reflets des sols marchent aussi en qualité *Normale*.

Version 12.4 : **Hakuen en blanc, foudre rouge sang, effets communs refaits**.

- Hakuen est repeinte : sa tenue noire et or devient blanche, avec le rouge de son manteau et de ses rubans
  (portrait compris).
- Son ultime à mains nues, « Forge de l'aube » : elle charge, puis un seul grand éclair rouge sang, très foncé, tombe
  du ciel sur l'adversaire.
- Ses coups et ses ondes sont en foudre rouge (plus de disque doré au sol).
- Les effets communs à tous les persos sont refaits avec ParticleFX : saut, double saut, atterrissage, dash, bulle du
  bouclier et explosion au bord de l'écran quand un perso est éjecté (à sa couleur).
- Un perso éjecté fort laisse une traînée de fumée derrière lui (elle ne s'affichait jamais avant).

Version 12.3 : **vrai RTX**.

- Nouveau réglage **Paramètres > Affichage > RTX**. *Non* (par défaut) garde l'éclairage dessiné des versions
  précédentes. *Oui* calcule la lumière des effets en vrai lancer de rayons, sur la carte graphique.
- Avec le RTX, les persos arrêtent la lumière (vraies ombres), sont éclairés du côté tourné vers l'effet, et la
  lumière rebondit sur eux et sur le décor.
- Sur les persos, la lumière reste en aplats, façon pixel art, sans les salir.
- Sur les Toits Kurenai, les lanternes éclairent la scène.
- Les reflets des sols brillants sont gardés.
- Si le jeu rame, le RTX se coupe tout seul.
- Sans RTX : les flaques de lumière et les ombres ne débordent plus des bords des sols.

Version 12.2 : **Hakuen, effets refaits**.

- Le croissant part droit devant, à hauteur du corps, aussi grand qu'elle, avec de la foudre rouge (éveillée, elle
  en lance trois en éventail).
- Nouvelle aura d'éveil (flammes rouge et or, foudre rouge) et nouvelle onde d'éveil rouge.
- Pas-iaï : une coupe nette et des éclairs rouges le long de la ruée.
- Spirale montante : deux tours de lame et de la foudre.
- Ultime « Forge de l'aube » : une colonne de foudre rouge frappe l'adversaire.
- Éveillée, ses blancs ne virent plus au beige.

Version 12.1 : **reflets des sols mieux découpés**.

- Les reflets des sols brillants (Abîme Lunaire, Citadelle de Givre, Toits Kurenai, Monastère des Nuées) s'arrêtent
  au bord du sol : ils ne débordent plus sur les bouts du toit, les rambardes ni le décor derrière.

Version 12.0 : **Hakuen, le nouveau N° 01, arrive en bêta**.

- Hakuen, le sabreur aux cheveux blancs, rejoint les combattants (il s'ajoute aux persos actuels). Il se bat à mains
  nues, avec ses nouvelles animations (garde, marche, course, dash, coups, compétences).
- Ses compétences et son ultime « Forge de l'aube » ont leurs effets ParticleFX, avec du bloom : croissant, Pas-iaï,
  Spirale montante, Prescience (parade) et l'aura de son éveil.
- Nouveaux effets ParticleFX pour tout le monde : coups (rapide, tranchant, montant, critique), onde d'impact,
  bouclier touché et brisé, ultime prêt.
- Ses deux armes (le nodachi et la lame éveillée) ne tombent pas encore : leurs animations arrivent dans une
  prochaine version.

Version 11.9 : **deux sphères d'énergie de ParticleFX**.

- N° 02, mains nues : sa compétence neutre devient la « Sphère de givre », une sphère bleue tourbillonnante lancée
  droit devant.
- N° 03 avec la grande épée éveillée : son ultime « Apocalypse » lance une sphère d'énergie géante et dorée qui
  explose au contact.
- En qualité Haute, le halo des effets ne brûle plus en blanc l'intérieur des formes pleines comme ces sphères : leur
  couleur et leurs détails restent visibles.

Version 11.8 : **lumière façon RTX sur les arènes**.

- Les sols brillants reflètent la scène : le ciel, les persos, leurs coups et les effets s'y mirent, un peu flous et
  de plus en plus pâles vers l'avant. Le sol mouillé des Toits Kurenai reflète aussi ses lanternes rouges, et la pluie
  le fait onduler. La pierre de l'Abîme Lunaire et du Monastère des Nuées est polie, les tuiles gelées de la Citadelle
  de Givre brillent un peu, le grès du Canyon Écarlate reste mat.
- Les effets lumineux éclairent le décor et le sol autour d'eux : leur lumière s'étale sur le sol en profondeur, plus
  visible la nuit qu'en plein jour.
- Chaque perso projette une ombre douce à l'opposé de l'effet qui l'éclaire : courte sous une lumière haute, longue
  quand elle est basse à côté de lui.
- Tout cela avec « Qualité graphique : Haute ». L'ombre sous les pieds est adoucie partout.

Version 11.7 : **étincelles d'ultime et étoiles des persos sonnés**.

- Pendant son ultime, le perso est entouré d'étincelles dorées, blanches et bleues qui scintillent. Les effets de
  l'ultime restent tous là, les étincelles s'ajoutent par-dessus.
- Un perso sonné (bouclier brisé) a maintenant des étoiles qui tournent au-dessus de sa tête.
- Ces deux effets viennent de ParticleFX et brillent avec « Qualité graphique : Haute », mais n'éclairent pas le perso
  qui les porte.

Version 11.6 : **la lumière vient de l'effet**.

- Un perso près d'un effet lumineux est éclairé du côté de l'effet : ce côté prend la couleur de la lumière (les
  parties sombres dans une teinte profonde, les claires vers le blanc), avec un bord net et un liseré de lumière sur
  le contour. L'autre côté reste dans l'ombre.
- Plus l'effet est proche et fort, plus la lumière couvre le corps ; collé à lui, le perso est éclairé en entier. Sous
  la pluie d'épées, la lumière vient d'en haut.
- La lumière suit l'effet image par image : elle part de là où il brille, aussi fort qu'il brille (le clignotement de
  la 11.5 est retiré).
- Toujours avec « Qualité graphique : Haute ».

Version 11.5 : **les effets éclairent les persos**.

- Quand un effet lumineux passe près d'un perso (la pluie d'épées du Jugement céleste, le Croissant du néant), le
  perso prend sa couleur et clignote comme sous un éclair : les ombres se teintent, les parties claires blanchissent.
- Comme le halo, cette lumière est active avec « Qualité graphique : Haute ».

Version 11.4 : **les effets brillent**.

- Le Croissant du néant (N° 01) et la pluie d'épées du Jugement céleste (N° 08) ont maintenant un halo lumineux qui
  déborde autour d'eux, comme une vraie lumière.
- Le halo est actif avec « Qualité graphique : Haute » (Paramètres, Affichage). En « Normale », pour les petites
  machines, il est coupé.

Version 11.3 : **deux nouveaux effets**.

- **Croissant du néant** (N° 01, avec l'odachi) : la compétence neutre lance un croissant d'énergie rose et bleu qui
  file au ras du sol.
- **Jugement céleste** (N° 08, avec l'épée blanche et or) : l'ultime fait tomber une pluie d'épées de lumière sur
  chaque adversaire. Chaque épée qui touche blesse un peu, puis le jugement final l'éjecte.
- Ces deux effets restent affichés même quand les autres effets sont coupés.

Version 11.2 : **nouvelle direction artistique « Ligne d'horizon », 4e et dernière partie**.

- **Nouvel écran des résultats** : le gagnant en grand dans la lumière de la victoire, et le classement à droite
  (KO, chutes, dégâts et orbes brisées), avec la durée du combat et l'arène.
- Des récompenses sous les noms : « Le plus de KO », « Casseur d'orbe » ou « Plus gros coup : N % ».
- **Revanche** : chaque joueur se dit prêt avec sa propre touche (ou un clic sur sa case) ; quand tout le monde l'est,
  le combat repart tout seul. On peut aussi changer de perso, d'arène ou revenir au menu.
- **Nouveaux paramètres** : des onglets avec icônes, l'aide de la ligne choisie, et une carte en direct à côté :
  le volume réel (musique et effets passent par le volume général), un aperçu du combat qui suit les réglages
  d'affichage (noms, FPS, zones de coup, secousses), et les règles des prochains combats.
- **Une seule table des commandes** : clavier 1, clavier 2 et les deux boutons de la manette côte à côte. Les
  directions de la manette restent sur le stick gauche et la croix. Réinitialiser un clavier, la manette ou tout.
- Pour changer un bouton, la manette est dessinée avec le bouton actuel entouré.

Version 11.1 : **nouvelle direction artistique « Ligne d'horizon », 3e partie**.

- **HUD refait** : une carte par joueur en bas de l'écran (visage, numéro, vies en losanges, armes, recharge de la
  compétence). Les dégâts chauffent du blanc à l'or, puis à l'orange et au rouge.
- **Nouvelle Clash Orb** : une sphère mi-ciel, mi-or, coupée par la ligne d'horizon, avec l'éclat du logo au centre ;
  elle se fissure sous les coups. Un bandeau en haut montre ses points de vie et le temps qui lui reste.
- Quand on la brise, son éclat vole jusqu'au gagnant : il s'entoure d'or, porte l'éclat au-dessus de son nom et sa
  carte affiche « ULTIME PRÊT » jusqu'à l'ultime.
- Les dégâts reçus s'affichent au-dessus du joueur (« +14 % ») ; pendant un combo, le compteur « N COUPS ! » prend le
  relais. Bulles des joueurs hors de l'écran refaites, avec leur distance.
- **Nouvelle pause** : le menu à gauche, l'état du combat (vies, dégâts, orbe) et la **liste des coups** de celui qui
  a mis en pause, en trois onglets (mains nues, arme A, arme B), avec ses propres touches. Les options s'ouvrent depuis
  la pause et ramènent au combat. Quitter demande une confirmation.
- À venir : les résultats et les paramètres.

Version 11.0 : **nouvelle direction artistique « Ligne d'horizon », 2e partie**.

- **Choix des combattants** refait : les combattants se tiennent en grand sur la ligne d'horizon, de part et d'autre
  du « VS », chacun avec sa fiche dessous (numéro, art martial, gabarit, transformation, ses deux armes). La grille
  des visages est au-dessus du « VS », les règles en dessous. Les places libres attendent en bas à gauche : J3 et J4
  rejoignent d'une touche.
- **Choix de l'arène** refait : l'arène remplit l'écran, avec le plan de ses plateformes (sols, plateformes fines,
  planche mobile qui bouge).
- **Nouvel écran VS** avant chaque combat : qui contre qui, l'arène, les règles et une astuce (ENTRÉE pour passer).
- Couleurs des joueurs : J1 bleu, J2 or, J3 vert, J4 rose ; un ordinateur prend la couleur de sa place.
- À venir : le HUD, l'orbe et la pause, puis les résultats et les paramètres.

Version 10.9 : **nouvelle direction artistique « Ligne d'horizon », 1re partie**.

- Écran titre refait : un vrai combat entre deux ordinateurs (niveau 9) se joue derrière le menu, sur la terrasse
  face à l'horizon. À chaque K.O., une nouvelle paire.
- Nouveau menu : quatre grands boutons (Jouer, Versus, Entraînement, En ligne), quatre petits, une astuce par bouton.
- Nouvelle charte pour tous les menus : polices Saira, panneaux aux coins en biseau, ligne d'horizon bleu, blanc et
  or, touches dessinées au trait.
- À venir : la sélection, les arènes et l'écran VS, puis le HUD, l'orbe et la pause, puis les résultats et les
  paramètres.

Version 10.8 : **essai des effets tout faits de PixelLab**.

- Chaque coup fait un éclat PixelLab (flash, rayons et anneau), à la couleur de l'élément du coup.
- Un coup puissant envoie une onde de choc au sol sous l'adversaire.
- Aussi : une traînée de fumée derrière un perso éjecté vite.

Version 10.7 : **seulement les bons effets**.

- On garde ceux que tu aimes : la fumée de saut, de dash et d'atterrissage.
- La bulle de bouclier est de nouveau visible (elle avait disparu avec la pause des effets).
- Les coups gardent leurs petits éclats en pixels et leur étoile.
- Retirés : étincelles, gerbes au sol, images d'impact noir et blanc, flashs, auras et traînées dessinées.

Version 10.6 : **les effets reviennent, avec mesure**.

- Étincelles, fumées de saut et d'atterrissage, bulle de bouclier, projectiles : les effets sont de retour, dosés
  pour qu'on voie toujours le combat.
- Chaque coup a son étincelle et ses éclats, selon sa force. La gerbe au sol ne sort que pour les coups forts.
- Les images d'impact noir et blanc et le flash sont gardés pour les coups qui peuvent tuer et pour les KO (avant,
  chaque fin de combo les déclenchait).

Version 10.5 : **des coups qui claquent**.

- **Impacts** : chaque coup fait jaillir de petits éclats en pixels et une étoile, plus grands pour un coup fort. La
  victime tremble pendant l'arrêt sur le coup, dans le sens où elle part. Les secousses sont plus brèves, et les gros
  coups sonnent plus grave. Lames qui se croisent, coups dans le bouclier et chutes au sol ont aussi leurs éclats.
- **Compétences** : 3 secondes de recharge (neutre, côté, bas), pour tous les persos.
- **Ultime** : il ne vient plus que de la Clash Orb.
- **Caméra** un peu plus reculée, et **plus de salto** au double saut.
- **Le 03** : ses M1 avec l'épée éveillée et sa compétence avec la grande épée sont refaits (l'épée bouge vraiment) ;
  il respire plus calmement.
- **Le 02 au sceptre-étoile** se bat au bâton comme Wukong : balayages, moulinets, frappe écrasante, vrille en l'air.
- **Traînées** : chaque coup d'arme laisse sa traînée (lance du 02, épées du 07 et du 08 refaites).
- **Armes au sol** redessinées en pixel art, propres et colorées, d'après tes dessins.

Version 10.4 : **les effets visuels sont en pause**.

- **Aucun effet** : plus d'étincelles, de fumées, d'auras, de traînées, de flashs ni d'éclairs de rayon, en attendant
  de nouveaux effets. Les projectiles et les rayons sont de simples formes blanches, pour voir ce qui touche.
- **Le 03 et le 08** : leur M1 aérien avec arme montait plusieurs coups sans qu'on les voie ; les animations sont
  refaites, on voit maintenant chaque entaille.
- **Calage des coups** : les attaques à plusieurs touches montrent un coup par touche.
- **Le 01** : son coup arrière en l'air donnait un coup de pied vers l'avant ; il frappe maintenant derrière lui.

Version 10.3 : le M1 n'est plus perdu juste après un saut ou à la réception.
