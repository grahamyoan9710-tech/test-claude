# Prompts PixelLab des 10 persos de la planche v2 (06/11/2026)

Prêts à lancer, **rien n'est lancé sans l'accord de yoyo, perso par perso**. Noms, histoires et caractères : Claude Doc
« Clash Horizon : fiches des 10 nouveaux persos » (https://claude.ai/artifact/BaYrfTvNWcwMHCK2mvWWtz).

Références : `docs/refonte/ref_persos_v2.png` (couleurs normales), `ref_persos_v2_alt.png` (couleurs alternatives, même
dessin repeint), `ref_armes_v2.png` (armes A et B). La case NN de la planche (5 colonnes, 2 rangées, de gauche à droite)
sert de `reference_images` à l'image de base.

## Coût et ordre

| Étape | Outil | Coût par perso | 10 persos |
|---|---|---|---|
| Image de base (de profil, en garde, mains nues) | `create_image_pro`, 170 × 170, 4 essais | 15 | 150 |
| Idle | `animate_image_pixminimax`, 8 images, du premier au premier | 1 | 10 |
| Marche | idem | 1 | 10 |

Il restait 172,9 générations jusqu'au 13/11 (`pxcli.py balance`, 06/11) : les 10 bases, leurs idle et leurs marches
tiennent dedans. Avant chaque lancement : `pxcli.py jobs` (aucun job en cours) et `pxcli.py balance`.

Pour chaque perso : base, comparaison à sa case de la planche (cheveux, visage, couleurs exactes, coupe et longueur des
vêtements, accessoires, chaussures), choix du meilleur des 4 essais (`PICK` de `tools/pixellab/pxa/make_bases.py`), mise
au format 256 × 256 (pieds sur y = 244), puis idle et marche avec la base en première et dernière image.

Les expressions ne coûtent rien de plus : elles sont écrites dans le prompt de chaque clip (le visage de repos dans
l'idle et la marche, concentré dans les coups, la douleur dans les coups reçus, la victoire, la provocation).

## Textes communs

- **Fin de la base** : `Full body, side view facing right, standing in a fighting stance, fists raised, feet apart,
  bare-handed. Same character design, hair, face, outfit and colours as the reference. Only this character is drawn.`
- **Fin de l'idle** : `The character stays at the same place in the frame (the game moves him), always facing right,
  seen from the side; keep his look, proportions and colours exactly. Only the character is drawn, with what he holds:
  no effect, no energy, no projectile, no smoke, no dust, no ground, the whole body inside the picture.`
- **Fin de la marche** : `a seamless two-step walk cycle that ends exactly on the first pose. The character stays at
  the same place in the frame (the game moves him), always facing right, seen from the side; keep his look,
  proportions and colours exactly. Only the character is drawn, with what he holds: no effect, no motion streak, no
  speed lines, no trail behind the limbs or the weapon, no glow, no energy, no projectile, no smoke, no dust, no
  ground, the whole body inside the picture. Every frame shows the arms, legs and weapon clearly in a crisp pose,
  never blurred.`
- **Base** de chaque perso : son `subject` + la fin de la base.
- `his` devient `her` pour les persos féminins.
- `initial_pose` : `fighting stance facing right, fists raised`.
- **Adulte** (03, 06, 08, à valider par yoyo) : ajouter à la base `An adult of at least 20 with adult proportions:
  longer legs and torso, a slightly smaller head; everything else exactly as the reference.`

## Les 10 persos

### 01 · Hakuen (homme)
- **subject** : `an adult man with very long silver-white hair in a high ponytail tied with red ribbons and red and
  gold beads, red eyes, a long white off-shoulder coat with red and gold flame patterns and a red lining, a black
  sleeveless high-collared top, a red and gold belt with a red buckle, baggy black trousers, black fingerless gloves,
  red and black boots with gold buckles`
- **idle** : `idle fighting stance loop: a calm karate guard, the weight on the back leg, slow breathing, he closes his
  eyes for a moment then opens them with a cold calm look, the ponytail and the red ribbons float, the feet stay
  planted, returns exactly to the first pose.` + fin de l'idle.
- **marche** : `walks forward to the right in side profile with slow measured steps, upright chest, hands relaxed, a
  calm cold look: the back leg steps past the front leg, then the other, the body and head stay turned to the right
  the whole time exactly like the first frame,` + fin de la marche.

### 02 · Fuyuri (femme)
- **subject** : `an adult woman with very long black hair in a high ponytail tied with red ribbons and gold beads, red
  eyes, a white and red shrine maiden robe with long wide sleeves and red patterns, bare shoulders, a black corset
  top with a red and gold belt, a red and black checkered underskirt, black stockings with garters, red and black
  heeled boots`
- **idle** : `idle fighting stance loop: an aikido guard with her hands half hidden in her sleeves, the head slightly
  tilted, a mischievous little smile, she blows a strand of hair away, the sleeves sway, the feet stay planted,
  returns exactly to the first pose.` + fin de l'idle.
- **marche** : `walks forward to the right in side profile with small gliding steps, the wide sleeves wave, a
  mischievous smile: the back leg steps past the front leg, then the other, the body and head stay turned to the
  right the whole time exactly like the first frame,` + fin de la marche.

### 03 · Shiden (homme, adulte à valider)
- **subject** : `an adult man with short spiky lavender-white hair, two small violet horns, violet eyes, a short white
  hooded jacket with violet trim and gold studs, a violet crop top, a violet belt with straps, baggy dark violet cargo
  trousers with pockets, violet fingerless gloves, big violet and white boots`
- **idle** : `idle fighting stance loop: he bounces on his toes in a boxing guard, rolls his shoulders and cracks his
  neck, a cocky grin, returns exactly to the first pose.` + fin de l'idle.
- **marche** : `walks forward to the right in side profile with a swaggering gait, the shoulders rolling, fists
  loosely raised, a cocky grin: the back leg steps past the front leg, then the other, the body and head stay turned
  to the right the whole time exactly like the first frame,` + fin de la marche.

### 04 · Sorane (femme)
- **subject** : `an adult woman with very long silver twin tails, blue headphones, blue eyes, a long white coat with
  blue patterns and blue cuffs, a white top with a black belt, white shorts, black fingerless gloves, a black
  thigh-high stocking on one leg and a white patterned stocking on the other with thigh straps, black heeled boots`
- **idle** : `idle fighting stance loop: a taekwondo guard, she nods her head to a beat and taps her front foot, one
  hand touching her headphones, a stage smile, the twin tails sway, returns exactly to the first pose.` + fin de
  l'idle.
- **marche** : `walks forward to the right in side profile with small dancing steps on the beat, a stage smile: the
  back leg steps past the front leg, then the other, the body and head stay turned to the right the whole time
  exactly like the first frame,` + fin de la marche.

### 05 · Gōzan (homme)
- **subject** : `a huge muscular adult man with a big white mane and a black beard, tanned skin, a bare muscular
  tattooed chest, a white fur collar, a long red and black coat with gold and orange flame patterns, a red sash belt
  with a gold buckle, bronze armoured arm guards, a red rope coiled on one arm, black fingerless gloves, black
  trousers, brown and bronze boots`
- **idle** : `idle fighting stance loop: a wide judo stance, a deep breath lifts his huge shoulders, he cracks his
  knuckles, a fierce grin, the mane sways, the feet stay planted, returns exactly to the first pose.` + fin de l'idle.
- **marche** : `walks forward to the right in side profile with heavy slow steps, every step weighs, fists low, a
  confident grin: the back leg steps past the front leg, then the other, the body and head stay turned to the right
  the whole time exactly like the first frame,` + fin de la marche.

### 06 · Kururi (femme, adulte à valider)
- **subject** : `an adult woman with pink twin tails, white rabbit ears and aviator goggles on her head, pink eyes, a
  short white and pink jacket with red patterns, a black crop top, a pink and white plaid miniskirt, black fingerless
  gloves, red tassels hanging at her side, one black thigh-high stocking and one white fishnet stocking with garters,
  big pink and white sneakers`
- **idle** : `idle fighting stance loop: a savate guard, she hops lightly on the spot, the rabbit ears and twin tails
  bounce, a big bright smile, returns exactly to the first pose.` + fin de l'idle.
- **marche** : `walks forward to the right in side profile with small springy hops, a big smile: the back leg steps
  past the front leg, then the other, the body and head stay turned to the right the whole time exactly like the
  first frame,` + fin de la marche.

### 07 · Yakō (homme)
- **subject** : `an adult man with messy black hair, violet horns and a long violet streamer trailing behind, violet
  eyes, a big white fur collar, a long black coat with violet panels, a violet lining and gold patterns, a violet belt
  with a gold and red buckle, white gloves, baggy black trousers, black boots`
- **idle** : `idle fighting stance loop: a low fluid pencak silat guard, he adjusts his fur collar with one hand, a
  bored half smile, the coat and the violet streamer sway, the feet stay planted, returns exactly to the first
  pose.` + fin de l'idle.
- **marche** : `walks forward to the right in side profile, slow and elegant, the long coat waves, a bored half smile:
  the back leg steps past the front leg, then the other, the body and head stay turned to the right the whole time
  exactly like the first frame,` + fin de la marche.

### 08 · Kotora (femme, adulte à valider)
- **subject** : `an adult woman with long wavy teal hair, green eyes, a white hood with cat ears (yellow inside), a
  long white coat with yellow patterns, a black crop top with yellow patterns, black shorts with a silver belt, black
  fingerless gloves, a grey stocking on one leg and a black garter on the other, white and yellow sneakers, a big
  dark khaki backpack with yellow-green straps and a tassel`
- **idle** : `idle fighting stance loop: a kung fu tiger guard, she sways from one foot to the other and tugs at the
  edge of her hood, a sly look, returns exactly to the first pose.` + fin de l'idle.
- **marche** : `walks forward to the right in side profile with soft padded steps, leaning slightly forward like a
  cat, a sly look: the back leg steps past the front leg, then the other, the body and head stay turned to the right
  the whole time exactly like the first frame,` + fin de la marche.

### 09 · Jinrai (homme)
- **subject** : `an adult man with long red hair in a high ponytail with gold beads, red eyes, a white fur collar, a
  long black coat with gold and orange patterns and armoured shoulders, a red sash belt with a gold buckle, a red and
  orange front panel, black fingerless gloves, black trousers, black and bronze armoured boots`
- **idle** : `idle fighting stance loop: a muay thai guard, the weight on one leg, he blows a strand of hair away, a
  bored sarcastic look, the ponytail sways, returns exactly to the first pose.` + fin de l'idle.
- **marche** : `walks forward to the right in side profile, confident and relaxed, a bored look: the back leg steps
  past the front leg, then the other, the body and head stay turned to the right the whole time exactly like the
  first frame,` + fin de la marche.

### 10 · Hoshiyo (femme)
- **subject** : `an adult woman with very long silver hair, violet-pink eyes, a large black witch hat with violet
  ribbons, white and violet flowers, gold trim and red pendants, a long white coat with violet patterns and a violet
  lining, a black top with gold ornaments, a white and violet pleated skirt, black fingerless gloves, black
  thigh-high stockings, black heeled boots`
- **idle** : `idle fighting stance loop: a kickboxing guard, she adjusts her hat, glances up at the sky, a dreamy
  half-closed look, the hair and ribbons float, returns exactly to the first pose.` + fin de l'idle.
- **marche** : `walks forward to the right in side profile, strolling with her nose in the air, a dreamy look: the
  back leg steps past the front leg, then the other, the body and head stay turned to the right the whole time
  exactly like the first frame,` + fin de la marche.
