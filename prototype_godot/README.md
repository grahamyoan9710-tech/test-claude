# Prototype Godot

Un test rapide pour juger Godot 4 sur le rendu du jeu. **Ce n'est pas le jeu** : le vrai jeu reste celui du dossier
principal (C++ et raylib). Ce prototype ne touche à rien du jeu.

## Ce qu'il montre
- Naruto contre Sasuke, avec les **mêmes animations PixelLab que le jeu**, lues directement dans
  `../assets/sprites` (`sheets.txt` et les planches PNG) : rien n'est copié ni converti.
- La **lueur** de Godot (halo autour des effets, du mode Kurama, du Chidori, du soleil).
- Des **lumières 2D** : la boule de feu éclaire le sol et les persos sur son passage, le Rasengan et le
  Chidori jettent une lumière bleue, le mode Kurama une lumière orange.
- Un décor en plusieurs plans qui défilent à des vitesses différentes, une caméra qui suit et tremble sur les
  gros coups, un arrêt sur image à l'impact.

La logique du vrai jeu (le cœur C++ : coups, hitboxes, IA, équilibrage) n'est **pas** branchée : les persos ont
juste un petit déplacement, un combo de 3 coups et deux techniques chacun.

## L'ouvrir
1. Installer **Godot 4.4** (version standard, pas .NET) : https://godotengine.org/download
2. Dans Godot, « Importer » le fichier `prototype_godot/project.godot` de ce dépôt (le dossier `assets` doit
   être à côté, comme dans le dépôt).
3. Lancer avec F5.

Commandes (Naruto) : **A / D** ou flèches pour courir, **W** ou espace pour sauter, **J** combo, **K** Rasengan,
**L** mode Kurama (activer / couper). Sasuke est joué par une petite IA (combo, Chidori, boule de feu).

En ligne de commande : `godot --path prototype_godot -- --demo` joue un combat scénarisé ;
`-- --demo --record DOSSIER DEBUT FIN PAS` enregistre des images.
