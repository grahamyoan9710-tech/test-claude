"""Clip specifications for the PixelLab fighters (Naruto, Sasuke, Kakashi).

Each clip: name -> (frames, base, pin, loop, direction, description)
- base: which picture the clip starts from: 'guard' (the fighting stance), 'sword' (Sasuke, sword drawn), or
  '<clip>@last' (the last frame of another clip: stage B);
- pin: True = the clip must end on its base picture (loops, moves that return to the stance), 'guard' = end on the
  stance, None = free ending;
- direction: where the blow goes (forward, up, down, back, around, none), to find the impact frame;
- the description is the motion only (PixMiniMax: "describe movement, not appearance").
`python3 clips.py FIGHTER STAGE` writes the batch file of that stage (jobs whose base exists).
"""
import json
import os
import sys

S = os.path.dirname(os.path.abspath(__file__))
SUBJECT = {
    "naruto": "Naruto, a young ninja in an orange and black tracksuit with spiky blond hair and a forehead protector with long black tails",
    "sasuke": "Sasuke, a young ninja with spiky dark blue hair, an open white kimono shirt, a thick purple rope belt and dark blue pants",
    "kakashi": "Kakashi, a tall ninja with spiky silver hair, a dark mask over his lower face, a forehead protector slanted over his left eye and a green flak vest over a navy outfit",
}
POSE = {"guard": "fighting stance facing right, fists raised", "sword": "fighting stance facing right, holding a straight sword forward in the right hand",
        "air": "in mid-air facing right, knees tucked", "airsword": "in mid-air facing right, knees tucked, holding a straight sword",
        "ledge": "hanging from his raised hands, arms stretched straight up, legs dangling, empty background above the hands", "lying": "lying on his back on the ground",
        "hold": "holding an invisible opponent in front of him with both hands", "run": "running to the right, leaning forward, arms swept back"}
# The 14 other fighters: subjects written for their base (pxa/subjects14.json), the rest from their spec file
# pxa/specs/<fighter>.json (overrides of the generic moves, their specific clips, guard pose, weapon).
SUBJECT.update(json.load(open(os.path.join(S, "subjects14.json"))))
OTHERS = sorted(json.load(open(os.path.join(S, "subjects14.json"))))
STATES = {"idle", "walk", "runstart", "jump", "crouch", "land", "shield", "roll", "spotdodge", "hurt", "knockdown", "dizzy",
          "grabbed", "holdstart", "victory", "run", "fall", "airjump", "airdodge", "helpless", "tumble", "ledgegrab",
          "getup", "hold", "ledge", "climb"}


# The base stance of each of the 14 (what their guard picture shows), until their spec gives its own "guard_pose".
GUARD14 = {
    "asta": "low ready stance facing right, holding a huge black greatsword forward with both hands",
    "yuno": "calm magic stance facing right, one hand raised forward, a small green grimoire floating behind him",
    "ichigo": "wide stance facing right, holding the huge cleaver sword Zangetsu raised in his right hand",
    "byakuya": "upright fencing stance facing right, a slim katana pointed forward in his right hand",
    "link": "crouched stance facing right, the Master Sword in his left hand, a round Hylian shield on his right arm",
    "ganondorf": "heavy fighting stance facing right, fists clenched at chest height, cape behind him",
    "kratos": "low brawling stance facing right, a round shield on his left forearm, right fist clenched",
    "gojo": "relaxed stance facing right, one hand raised in front with two fingers up",
    "sukuna": "relaxed menacing stance facing right, arms loose, one hand open",
    "kenpachi": "leaning stance facing right, a long jagged katana held low in one hand",
    "ulquiorra": "calm upright stance facing right, one hand in his coat pocket, the other raised loosely",
    "grimmjow": "feral crouched stance facing right, fists raised like claws",
    "starrk": "loose relaxed stance facing right, one hand pointing forward like a pistol",
    "hunter": "heavy low guard facing right, holding an enormous greatsword with both hands",
}


# Picked-up weapons held in the hand that change the fighter's look: its w<slot>_ clips start from a guard holding
# it ("w<slot>guard", a helper clip from the guard picture: the swap itself), not from the base weapon's guard.
WGUARD = {
    "asta": {1: "the huge black greatsword dissolves into black mist and he grips a slim black katana-like sword with a "
                "dark guard (the Demon Dweller Sword) instead, holding it forward in a ready stance"},
    "yuno": {1: "green wind gathers in his raised hand and forms a long spear of swirling green wind, which he grips "
                "with both hands in a ready stance, point forward; his cream-white and gold Golden Dawn robe keeps exactly its "
                "colours the whole time"},
    "ichigo": {1: "Zangetsu condenses in a burst of black spiritual pressure into a slim all-black katana with a short "
                  "chain hanging from the hilt (Tensa Zangetsu), which he holds forward in one hand, a black coat "
                  "fluttering"},
    "kenpachi": {0: "his jagged katana grows into a huge crude cleaver-axe with a wide blade (Nozarashi), which he rests "
                    "on his shoulder with one hand, leaning forward with a wild grin"},
    "grimmjow": {0: "draws his katana Pantera from his waist and holds it low and forward in one hand, crouched",
                 1: "his hands turn into long white claws of a panther, he crouches with the claws spread forward"},
    "link": {1: "puts the sword away and draws a glowing golden bow of light, an arrow of light nocked, aiming forward"},
    "ganondorf": {0: "draws a huge dark curved sword glowing with red malice (the Demon King's sword) and holds it low "
                     "in front with one hand"},
    "kratos": {0: "swaps the shield for the Leviathan Axe, a frosted battle axe with runes, gripped in his right hand",
               1: "draws the Blades of Chaos, two curved runed blades on long chains wrapped around his forearms, one "
                  "in each hand, embers glowing"},
    "hunter": {0: "swaps the greatsword for a charge blade: a sword in the right hand and a shield on the left arm",
               1: "swaps the greatsword for a gunlance: a huge lance with a gun barrel in the right hand and a big "
                  "shield on the left arm"},
}


# What the armed fighters hold in their guard picture: PixelLab drops it in jumps, runs and grabs unless told to keep
# it (their states and generic moves say so, their subject names it).
HELD = {
    "asta": "a huge black broken-edged greatsword", "ichigo": "the huge black-bladed cleaver sword Zangetsu",
    "byakuya": "a slim katana", "kenpachi": "a long jagged katana", "hunter": "an enormous greatsword",
    "link": "the Master Sword in one hand and a shield on the other arm", "kratos": "a round shield on his left forearm",
}
ARMED_SHIELD = ("braces to block: holds {held} crosswise in front of his body like a shield and crouches slightly, a "
                "solid defensive guard")
ARMED_LEDGE = {
    "ledgegrab": "jumps up and stretches his free arm straight up above his head, the empty hand closing on nothing near the top "
                 "of the picture but fully inside it, then hangs still from that raised hand with the legs dangling below, his "
                 "other hand holding {held} pointing down beside his legs; empty background above and around the raised hand: "
                 "no log, no bar, no branch, no cliff, no rock, no wall, no platform, no ground",
    "ledge": "hangs from his raised free hand, the arm stretched straight up with the hand closed on nothing near the top of the "
             "picture, legs swaying gently below, his other hand holding {held} pointing down, a seamless loop; empty "
             "background above and around the raised hand: no log, no bar, no branch, no cliff, no rock, no wall, no platform",
}
ARMED_HOLD = ("reaches forward with his free hand and closes it on the collar of an invisible opponent in front of him, "
              "then holds it at arm's length, the other hand still holding {held}. Only the character is drawn: the "
              "opponent is invisible, there is no other person in the frame.")


# Weapon kits (pxa/wkits/<fighter>.json): with a picked-up weapon, a 3-hit M1 combo (w<slot>_jab, jab2, jab3), an air
# M1 (w<slot>_nair), 4 skills and an ultimate. A slot whose "look" is "new" also gets the states below, so the weapon
# is seen in every animation; their bases are its own (guard -> w<slot>guard@last, jump@last -> w<slot>_jump@last).
WSTATES = {
    "idle": (8, "guard", True, True, "none", "idle stance loop: breathes calmly, shoulders rise and fall, knees bounce slightly, hair and clothes sway in the wind, {held} held ready in front of him, the feet stay planted, returns exactly to the first pose"),
    "walk": (8, "guard", True, True, "none", "walks forward to the right in side profile: the back leg steps past the front leg, then the other, the body and head stay turned to the right like the first frame, {held} held ready in front of him, a seamless two-step walk cycle that ends exactly on the first pose"),
    "runstart": (8, "guard", None, False, "none", "breaks into a fast run to the right: leans the torso far forward, {held} held low and trailing behind him in one hand, the free arm swept back, legs start a long running stride"),
    "run": (8, "runstart@last", True, True, "none", "fast run cycle to the right: leaning far forward, {held} held low and trailing behind him in one hand, long fast strides, a seamless loop that ends exactly on the first pose"),
    "jump": (8, "guard", None, False, "none", "jumps straight up: a quick crouch, springs up, then tucks the knees up in mid-air, {held} held ready in one hand, hair and clothes fly upward"),
    "fall": (8, "jump@last", True, True, "none", "falls through the air: legs reach down, the free arm out for balance, {held} held ready, hair and clothes flutter upward, a seamless loop that ends exactly on the first pose"),
    "airjump": (8, "jump@last", True, False, "none", "double jump in mid-air: kicks off the air and does a full forward somersault holding {held} close to his body, ending tucked in the same pose"),
    "land": (4, "guard", True, False, "none", "lands from a jump: absorbs the impact by bending the knees deeply, {held} held ready, then rises back into the stance"),
    "crouch": (4, "guard", None, False, "none", "crouches down low on the spot and holds it: knees fully bent, hips dropped onto the heels, torso folded forward, head ducked low at about half his standing height, {held} held low and ready in front of him"),
    "shield": (4, "guard", None, False, "none", "braces to block: holds {held} crosswise in front of his body like a shield and crouches slightly, a solid defensive guard"),
    "roll": (8, "guard", True, False, "none", "dodge roll: tucks and rolls in a tight ball along the ground holding {held} close to his body, then springs back up into the same stance"),
    "spotdodge": (8, "guard", True, False, "none", "spot dodge: quickly ducks down low on the spot, bending his knees and leaning his head and torso back so a blow passes over him, {held} held close, then straightens up into the stance. Only the character is drawn: no slash, no effect"),
    "airdodge": (8, "jump@last", True, False, "none", "air dodge: curls up tightly in mid-air in the same place holding {held} close to his body, the body staying in the middle of the picture, then opens up again in the same pose"),
    "hurt": (4, "guard", None, False, "none", "gets hit hard: recoils backwards, head snaps back, the free arm flung up, body bent back, still gripping {held}"),
    "tumble": (8, "jump@last", True, True, "none", "tumbles head over heels through the air after a heavy blow, spinning out of control, still gripping {held}, a seamless loop"),
    "knockdown": (8, "guard", None, False, "none", "is knocked off his feet and falls flat on his back on the ground, ending lying down, still gripping {held}"),
    "getup": (8, "knockdown@last", "guard", False, "none", "gets up from lying on his back: rolls onto his feet and stands back up into the ready stance, {held} in hand"),
    "helpless": (8, "jump@last", True, True, "none", "falls helplessly, exhausted: the free arm hangs limp above his head, {held} dangling loosely from the other hand, head drooping, legs dangling, the body tilting slowly as he drops, a seamless loop that ends exactly on the first pose. Only the character is drawn"),
    "ledgegrab": (8, "jump@last", None, False, "none", ARMED_LEDGE["ledgegrab"]),
    "ledge": (4, "ledgegrab@last", True, True, "none", ARMED_LEDGE["ledge"]),
    "climb": (8, "ledgegrab@last", "guard", False, "none", "from hanging by his raised free hand, he pulls his chest up to the height of that hand, swings one knee up, pushes himself up and stands up in the ready stance, {held} in the other hand; nothing is drawn around him: no log, no bar, no branch, no cliff, no rock, no wall, no platform, no ground"),
    "dizzy": (8, "guard", True, True, "none", "dazed and dizzy after his guard breaks: stands slumped on wobbly bent knees, arms hanging limp, {held} dangling from one loose hand with its tip on the ground, head drooping and rolling slowly in a circle, the body swaying from side to side on the spot, a seamless loop that ends exactly on the first pose. Only the character is drawn"),
    "grabbed": (4, "guard", True, True, "none", "held up by the collar from in front of him: chest pulled forward, head tilted back in pain, his free hand clutching at the collar at his throat, the other hand still gripping {held} pointing down, legs kicking and squirming, a seamless loop. Only the character is drawn: the opponent is invisible, there is no other person in the frame"),
    "holdstart": (4, "guard", None, False, "none", ARMED_HOLD),
    "hold": (4, "holdstart@last", True, True, "none", "holds an invisible opponent by the collar at arm's length with his free hand, the other hand keeping {held} ready, breathing calmly, a seamless loop. Only the character is drawn: the opponent is invisible, there is no other person in the frame"),
    "grab": (8, "guard", True, False, "forward", "lunging grab: reaches forward with his free hand to seize an opponent, the other hand keeping {held} ready, then pulls back"),
    "pummel": (4, "holdstart@last", True, False, "forward", "strikes the invisible opponent he is holding with the pommel of {held}: a quick short jab of the hilt, then back to holding. Only the character is drawn: the opponent is invisible, there is no other person in the frame"),
    "fthrow": (8, "holdstart@last", "guard", False, "forward", "shoves the invisible opponent he is holding forward with his free hand, then slashes straight ahead with {held}, then returns to the ready stance. Only the character is drawn: the opponent is invisible, there is no other person in the frame"),
    "bthrow": (12, "holdstart@last", "guard", False, "back", "swings the invisible opponent he is holding around behind him and hurls it away, turning his body, {held} in the other hand, then returns to the ready stance. Only the character is drawn: the opponent is invisible, there is no other person in the frame"),
    "uthrow": (8, "holdstart@last", "guard", False, "up", "hurls the invisible opponent he is holding straight up with his free hand, then slashes upward over his head with {held}, then returns to the ready stance. Only the character is drawn: the opponent is invisible, there is no other person in the frame"),
    "dthrow": (12, "holdstart@last", "guard", False, "down", "slams the invisible opponent he is holding down onto the ground in front of him with his free hand, then stabs down at it with {held}, then returns to the ready stance. Only the character is drawn: the opponent is invisible, there is no other person in the frame"),
    "ledgeattack": (8, "guard", True, False, "forward", "rises from a low crouch with a quick low horizontal cut of {held} forward at knee height, then back into the ready stance"),
    "getupattack": (8, "knockdown@last", "guard", False, "around", "from lying on his back, swings {held} low across in front of him, then low behind him while springing to his feet, ending in the ready stance"),
}


def wkit(fighter):
    p = os.path.join(S, "wkits", f"{fighter}.json")
    return json.load(open(p)) if os.path.exists(p) else {}


def wbase(slot, base):
    """The base of a clip of a new-look weapon: its own guard and its own stage-B clips."""
    if base == "guard":
        return f"w{slot}guard@last"
    if base.endswith("@last") and not base.startswith("w"):
        return f"w{slot}_{base}"
    return base


def wslot(fighter, name):
    """The kit slot a clip belongs to (its spec), or None."""
    if len(name) > 3 and name[0] == "w" and name[1].isdigit() and name[2] == "_":
        return wkit(fighter).get("slots", {}).get(name[1])
    return None


QA_PROMPTS = json.load(open(os.path.join(S, "qa_prompts.json"))) if os.path.exists(os.path.join(S, "qa_prompts.json")) else {}


def subject_of(fighter):
    return SUBJECT[fighter] + (f", holding {HELD[fighter]}" if fighter in HELD else "")


def pose_of(fighter, kind):
    """The caption of the first frame: the generic one, or this fighter's own stance."""
    if fighter in OTHERS and kind in ("guard", "hold", "air"):
        sp = spec(fighter) or {}
        g = sp.get("guard_pose") or GUARD14.get(fighter)
        if kind == "guard":
            return g
        if kind == "air":
            return "in mid-air facing right, knees tucked" + ("" if sp.get("armed", "none") in ("none", "fists", "magic", "") else ", weapon in hand")
    return POSE[kind]


def spec(fighter):
    p = os.path.join(S, "specs_final", f"{fighter}.json")  # reviewed copies of specs_early
    return json.load(open(p)) if os.path.exists(p) else None


TAIL = (" The character stays at the same place in the frame (the game moves him), always facing right, seen from the side;"
        " keep his look, proportions and colours exactly.")

# --- the moves every fighter has (bare-handed brawler moves; Sasuke's sword versions are below) ---
COMMON = {
    # name: (frames, base, pin, loop, direction, description)
    "idle": (8, "guard", True, True, "none", "idle fighting stance loop: breathes calmly, shoulders rise and fall, knees bounce slightly, hair and clothes sway in the wind, the feet stay planted, returns exactly to the first pose"),
    "walk": (8, "guard", True, True, "none", "walks forward to the right in side profile: the back leg steps past the front leg, then the other, the body and head stay turned to the right the whole time exactly like the first frame, fists raised, a seamless two-step walk cycle that ends exactly on the first pose"),
    "runstart": (8, "guard", None, False, "none", "breaks into a fast ninja run to the right: leans the torso far forward and sweeps both arms straight back behind him, legs start a long running stride"),
    "jump": (8, "guard", None, False, "none", "jumps straight up: a quick crouch, springs up, then tucks the knees up in mid-air with the arms out for balance, hair and clothes fly upward"),
    "crouch": (4, "guard", None, False, "none", "crouches down as low as he can on the spot and holds it: knees fully bent, hips dropped right down onto the heels, torso folded forward over the knees, head ducked low at about half his standing height, fists raised in front of the face. Only the character is drawn"),
    "land": (4, "guard", True, False, "none", "lands from a jump: absorbs the impact by bending the knees deeply, then rises back into the stance"),
    "shield": (4, "guard", None, False, "none", "braces to block: crosses both forearms in front of the face and crouches slightly, a defensive guard"),
    "roll": (8, "guard", True, False, "none", "dodge roll: tucks and rolls in a tight ball along the ground, then springs back up into the same stance"),
    "spotdodge": (8, "guard", True, False, "none", "spot dodge: quickly ducks down low on the spot, bending his knees and leaning his head and torso back so a blow passes over him, then straightens up into the stance. Only the character is drawn: no slash, no attack, no projectile, no fire, no effect"),
    "hurt": (4, "guard", None, False, "none", "gets hit hard in the face: recoils backwards, head snaps back, arms flung up, body bent back"),
    "knockdown": (8, "guard", None, False, "none", "is knocked off his feet and falls flat on his back on the ground, ending lying down"),
    "dizzy": (8, "guard", True, True, "none", "dazed and dizzy after his guard breaks: stands slumped on wobbly bent knees, both arms hanging limp at his sides, head drooping and rolling slowly in a circle, eyes half closed, the whole body swaying from side to side on the spot, a seamless loop that ends exactly on the first pose. Only the character is drawn"),
    "grabbed": (4, "guard", True, True, "none", "held up by the collar from in front of him: chest pulled forward, head tilted back in pain, hands clutching at the collar at his throat, legs kicking and squirming, body wriggling, a seamless loop. Only the character is drawn: the opponent is invisible, there is no other person in the frame"),
    "holdstart": (4, "guard", None, False, "none", "thrusts his right arm straight forward at shoulder height, the open hand closing on the collar of an invisible opponent in front of him, ending with the arm fully extended forward, holding it at arm's length, staying in side profile facing right the whole time. Only the character is drawn: the opponent is invisible, there is no other person in the frame."),
    "victory": (12, "guard", None, False, "none", "victory pose: straightens up, then strikes a confident cool pose to celebrate the win"),
    "jab": (8, "guard", True, False, "forward", "fast straight jab with the left fist: snaps the fist straight forward at full arm extension, then pulls it back into the stance; no smoke, no grey effects"),
    "jab2": (8, "guard", True, False, "forward", "fast cross punch: throws the rear fist straight forward at full extension with a twist of the hips, then back into the stance"),
    "jab3": (8, "guard", True, False, "forward", "strong front kick: lifts the front knee and kicks straight forward at belly height with a burst of energy, then back into the stance"),
    "ftilt": (8, "guard", True, False, "forward", "stepping side kick: steps forward and kicks straight ahead at chest height, then back into the stance"),
    "utilt": (8, "guard", True, False, "up", "upward swing: bends the knees, then swings the right fist in a big arc from low in front straight up above his head (an uppercut ending with the arm pointing at the sky), then back into the stance"),
    "dtilt": (8, "guard", True, False, "forward", "crouching leg sweep: drops low and sweeps the front leg along the ground in front, then rises back into the stance"),
    "fsmash": (12, "guard", True, False, "forward", "charged power punch: coils far back with the fist pulled behind, then lunges forward and drives the fist straight ahead at full extension with a burst of blue energy, then recovers into the stance"),
    "usmash": (12, "guard", True, False, "up", "upward smash: crouches, then springs up on his toes and strikes straight upward above his head with a burst of energy, then lands back in the stance"),
    "dsmash": (12, "guard", True, False, "around", "low split kicks: drops low and kicks fast along the ground in front, then spins and kicks behind, then back into the stance"),
    "grab": (8, "guard", True, False, "forward", "lunging grab: reaches forward with both hands to seize an opponent, then pulls back"),
    "ledgeattack": (8, "guard", True, False, "forward", "rises from a low crouch with a quick sweeping kick forward along the ground, then back into the fighting stance"),
}
STAGE_B = {
    "run": (8, "runstart@last", True, True, "none", "ninja run cycle to the right: leaning far forward, both arms swept straight back, long fast strides, a seamless loop that ends exactly on the first pose"),
    "dashattack": (12, "runstart@last", "guard", False, "forward", "running tackle: rushes forward shoulder first with a burst of energy, then slides to a stop and stands back up in the fighting stance"),
    "fall": (8, "jump@last", True, True, "none", "falls through the air: legs reach down, arms out for balance, hair and clothes flutter upward, a seamless loop that ends exactly on the first pose"),
    "airjump": (8, "jump@last", True, False, "none", "double jump in mid-air: kicks off the air and does a full forward somersault, ending tucked in the same pose"),
    "airdodge": (8, "jump@last", True, False, "none", "air dodge: curls up tightly in mid-air, vanishing in a quick blur, then opens up again in the same pose"),
    "helpless": (8, "jump@last", True, True, "none", "falls helplessly, exhausted: both arms hang limp and loose straight up above his head, head drooping, eyes half closed, legs dangling loosely below him, the body tilting slowly as he drops, a seamless loop that ends exactly on the first pose. Only the character is drawn"),
    "tumble": (8, "jump@last", True, True, "none", "tumbles head over heels through the air after a heavy blow, spinning out of control, a seamless loop"),
    "ledgegrab": (8, "jump@last", None, False, "none", "jumps up and stretches both arms straight up above his head, the hands closing on nothing near the top of the picture but fully inside it, then hangs still from his raised hands with the legs dangling below; empty background above and around the hands: no log, no bar, no branch, no cliff, no rock, no wall, no platform, no ground"),
    "nair": (12, "jump@last", True, False, "around", "neutral aerial: curls into a spinning ball in mid-air with a swirl of energy around the body, then opens up again"),
    "fair": (12, "jump@last", True, False, "forward", "forward aerial: a powerful flying kick straight forward in mid-air, then pulls the leg back in"),
    "bair": (8, "jump@last", True, False, "back", "back aerial: twists and kicks straight backward behind him with one leg in mid-air, then back to the tucked pose"),
    "uair": (8, "jump@last", True, False, "up", "up aerial: flips backward and kicks straight up above his head in mid-air (a bicycle kick), then back to the tucked pose"),
    "dair": (12, "jump@last", True, False, "down", "down aerial: stomps straight down beneath him with both feet in mid-air with a burst of energy, then pulls the legs back up"),
    "getup": (8, "knockdown@last", "guard", False, "none", "gets up from lying on his back: rolls onto his feet and stands back up into the fighting stance"),
    "getupattack": (8, "knockdown@last", "guard", False, "around", "lying on the back, spins on the floor and sweeps one leg in a wide circle kicking in front and then behind, then springs up into the fighting stance"),
    "hold": (4, "holdstart@last", True, True, "none", "holds an invisible opponent in front of him with both hands, arms extended and fists closed on empty air, breathing calmly, a seamless loop Only the character is drawn: the opponent is invisible, there is no other person in the frame."),
    "pummel": (4, "holdstart@last", True, False, "forward", "knees the invisible opponent he is holding in front of him: a quick knee strike, then back to holding with both hands Only the character is drawn: the opponent is invisible, there is no other person in the frame."),
    "fthrow": (8, "holdstart@last", "guard", False, "forward", "throws the invisible opponent he is holding forward: a strong two-handed heave and push, then returns to the fighting stance Only the character is drawn: the opponent is invisible, there is no other person in the frame."),
    "bthrow": (12, "holdstart@last", "guard", False, "back", "swings the invisible opponent he is holding around behind him and hurls it away, turning his body, then returns to the fighting stance Only the character is drawn: the opponent is invisible, there is no other person in the frame."),
    "uthrow": (8, "holdstart@last", "guard", False, "up", "hoists the invisible opponent he is holding high above his head with both arms and throws it straight up, then returns to the fighting stance Only the character is drawn: the opponent is invisible, there is no other person in the frame."),
    "dthrow": (12, "holdstart@last", "guard", False, "down", "slams the invisible opponent he is holding down onto the ground in front of him with both hands, crouching, then returns to the fighting stance Only the character is drawn: the opponent is invisible, there is no other person in the frame."),
}
STAGE_C = {
    "ledge": (4, "ledgegrab@last", True, True, "none", "hangs from his raised hands, both arms stretched straight up with the hands closed on nothing near the top of the picture, legs swaying gently below, a seamless loop; empty background above and around the hands: no log, no bar, no branch, no cliff, no rock, no wall, no platform"),
    "climb": (8, "ledgegrab@last", "guard", False, "none", "from hanging by his raised hands, he pulls his chest up to the height of his hands, swings one knee up, pushes himself up and stands up in the fighting stance; nothing is drawn around him: no log, no bar, no branch, no cliff, no rock, no wall, no platform, no ground"),
}

NARUTO = {
    "bair": (8, "jump@last", True, False, "back", "back aerial in mid-air: looks back over his shoulder and kicks one leg straight out behind him to the left, horizontal and fully extended, then pulls it back into the tucked jump pose"),
    "nspecial": (12, "guard", True, False, "forward", "Rasengan: gathers a swirling blue chakra sphere in his right palm with the left hand cupped over it, then lunges forward and thrusts the glowing sphere straight ahead, then returns to the stance"),
    "sspecial": (12, "guard", True, False, "none", "shadow clone jutsu: forms a cross-shaped hand sign in front of his chest, a puff of white smoke bursts beside him, then returns to the stance"),
    "uspecial": (12, "guard", None, False, "up", "crouches low, then jumps straight up as if flung upward, one fist raised above his head and the legs tucked under him, rising the whole time"),
    "dspecial": (12, "guard", True, False, "back", "substitution jutsu: forms a hand sign, vanishes in a puff of smoke leaving a log, then reappears and spins into a backward kick, then returns to the stance"),
    "ultimate": (28, "guard", True, False, "forward", "Uzumaki barrage: makes the cross-shaped clone hand sign, crouches, dashes forward with a fast flurry of punches and kicks, then a powerful rising kick upward, lands and returns to the stance. Only Naruto is drawn: no clones, no smoke, no effect"),
    "w0_jab": (8, "guard", True, False, "forward", "sage mode palm strike: orange pigment around his eyes, he thrusts an open palm straight forward with a burst of yellow nature chakra, then back into the stance"),
    "w1_jab": (8, "guard", True, False, "forward", "Kurama chakra strike: wrapped in a blazing golden chakra cloak, he swipes forward with a giant glowing golden chakra claw, then back into the stance"),
    "w0_nspecial": (12, "guard", True, False, "forward", "Sage mode, orange pigment around his eyes: pushes both open palms straight forward at chest height as if driving something huge in front of him, holds the push, then returns to the stance. Only Naruto is drawn: no sphere in his hands, no effect"),
    "w1_sspecial": (12, "guard", True, False, "forward", "wrapped in a golden chakra cloak, he swings both arms forward and two golden chakra clones dash out ahead of him, then returns to the stance"),
    "w0_ultimate": (28, "guard", True, False, "forward", "Sage mode, the exact orange and black outfit the whole time, orange pigment around his eyes: raises his open right hand high above his head and holds it there, then throws forward with a big overhand motion, then returns to the stance. Only Naruto is drawn: no shuriken in his hand, no effect"),
    "w1_ultimate": (28, "guard", True, False, "forward", "Tailed beast bomb: glowing in a golden fox chakra cloak, gathers a giant dark sphere of chakra in front of his mouth and hands, then fires it forward, then returns to the stance"),
    "getupattack": (8, "knockdown@last", "guard", False, "around", "lying on his back, spins on the floor and sweeps one leg in a wide circle kicking in front of him and then behind him, then springs up into the fighting stance"),
}
KAKASHI = {
    "nspecial": (12, "guard", True, False, "forward", "water dragon jutsu: makes a fast series of hand signs, then thrusts both hands forward as a dragon made of water surges out ahead of him, then returns to the stance"),
    "sspecial": (16, "guard", True, False, "forward", "Raikiri dash: his right hand open with the fingers spread like a claw, he crouches low, then dashes and thrusts that hand straight forward, then returns to the stance. Only Kakashi is drawn, in his exact colours: nothing in his hand, no lightning, no effect"),
    "uspecial": (12, "guard", True, False, "none", "body flicker: makes a single hand sign and vanishes in a whirl of leaves and smoke, then reappears in the same place in the stance"),
    "dspecial": (12, "guard", True, False, "forward", "summoning: kneels and slams his palm on the ground, ninja dogs burst out of the ground in front of him and bite, then he stands back up in the stance"),
    "ultimate": (28, "guard", True, False, "forward", "Raikiri barrage: his right hand open with the fingers spread like a claw, he lunges and stabs forward with that hand, steps back, lunges and stabs again three times, then a final powerful stab, then returns to the stance. Only Kakashi is drawn, in his exact colours: nothing in his hand, no lightning, no effect"),
    "w0_jab": (8, "guard", True, False, "forward", "Sharingan strike: his left eye glows red with the Sharingan, he throws a fast straight punch forward with a red flash, then back into the stance"),
    "w1_jab": (8, "guard", True, False, "forward", "Mangekyo strike: both eyes glow red, he spins and sweeps his right arm forward in a wide backhand strike, then back into the stance. Only Kakashi is drawn: no Susanoo arm, no effect"),
    "w0_nspecial": (12, "guard", True, False, "forward", "Kamui: brings two fingers of his right hand up beside his face and stares hard straight forward, his left eye glowing red with the Sharingan, he holds that focused pose without moving, then returns to the stance. Only Kakashi is drawn: no vortex, no swirl, no effect in front of him"),
    "w1_nspecial": (12, "guard", True, False, "forward", "throws a large blue chakra shuriken forward with an overhand throw, then returns to the stance"),
    "w0_ultimate": (28, "guard", True, False, "forward", "Kamui at full power: raises his left hand to his forehead, his left eye glowing red with the Mangekyo Sharingan, a trickle of blood runs from that eye, he stretches his right arm forward and stares at a far point straight ahead, straining and holding that pose for a long time, then lowers his arms and returns to the stance. Only Kakashi is drawn: no vortex, no swirl, no effect in front of him"),
    "w1_ultimate": (28, "guard", True, False, "forward", "Susanoo: both eyes glow red with the Mangekyo Sharingan, he spreads his arms wide and holds, then swings his right arm forward as if hurling something huge, then returns to the stance. Only Kakashi is drawn: no Susanoo, no shuriken, no effect"),
    "getupattack": (8, "knockdown@last", "guard", False, "around", "lying on his back, spins on the floor and sweeps one leg in a wide circle kicking in front of him and then behind him, then springs up into the fighting stance"),
}
SASUKE = {
    # sword normals: from the sword base
    "jab": (8, "sword", True, False, "forward", "fast one-hand thrust: lunges deep forward and stabs the straight sword straight ahead at full arm extension, the blade level and pointing right, then pulls back to the ready stance"),
    "jab2": (8, "sword", True, False, "forward", "backhand diagonal slash with the straight sword, from high on the left down to low in front, then back to the ready stance"),
    "ftilt": (8, "sword", True, False, "forward", "big forward cut with the straight sword: from above the head down to low in front, then back to the ready stance"),
    "utilt": (8, "sword", True, False, "up", "arcing slash with the straight sword from in front, over the head, to behind, then back to the ready stance"),
    "dtilt": (8, "sword", True, False, "forward", "crouching sweep cut with the straight sword along the ground in front, then rises back to the ready stance"),
    "fsmash": (12, "sword", True, False, "forward", "lightning-charged sword slash: raises the straight sword behind his head crackling with blue lightning, then cuts down powerfully to the front, then back to the ready stance"),
    "usmash": (12, "sword", True, False, "up", "rising sword slash with lightning: sweeps the straight sword in a big arc straight up over his head, then back to the ready stance"),
    "dsmash": (12, "sword", True, False, "around", "low spinning sword cut: slashes low in front, spins around and slashes low behind, crackling with lightning, then back to the ready stance"),
    "dashattack": (12, "sword", True, False, "forward", "running slash: dashes forward and slashes across with the straight sword, then stops in the ready stance"),
    "fair": (12, "airsword@last", True, False, "forward", "crescent cut in mid-air: swings the straight sword in a big arc from above down to the front, then back to the tucked pose"),
    "nair": (12, "airsword@last", True, False, "around", "spinning double cut in mid-air: slashes down in front, then up, the straight sword making a full circle around him"),
    "bair": (12, "airsword@last", True, False, "back", "backward sweep in mid-air: twists and slashes the straight sword behind him, then back to the tucked pose"),
    "uair": (12, "airsword@last", True, False, "up", "mid-air: slashes the straight sword in a big rising arc from in front of him up over his head and behind him, the blade visible the whole time, then back to the tucked pose"),
    "dair": (12, "airsword@last", True, False, "down", "downward stab in mid-air: points the straight sword straight down beneath him and plunges, then back to the tucked pose"),
    "nspecial": (12, "guard", True, False, "forward", "fire style: makes quick hand signs, raises his right hand to his mouth with two fingers up and blows hard forward, cheeks puffed, holds it, then returns to the stance. Only Sasuke is drawn: no fire, no effect"),
    "sspecial": (16, "guard", True, False, "forward", "Chidori: crouches low with a crackling ball of blue lightning chirping in his left hand, dashes forward and thrusts the lightning hand straight ahead, then returns to the stance"),
    "uspecial": (12, "guard", None, False, "up", "raises one arm high as a great hawk swoops down, grabs his raised arm and carries him straight up"),
    "dspecial": (12, "guard", True, False, "around", "Chidori Nagashi: flings both arms wide and bursts with blue lightning all around his body, then returns to the stance"),
    "ultimate": (28, "guard", True, False, "up", "Kirin: raises his right arm straight up to the sky and holds it there for a long time, then swings the arm down to point forward, then returns to the stance. Only Sasuke is drawn: no lightning, no clouds, no effect"),
    "w0_jab": (8, "guard", True, False, "forward", "snaps up a purple chakra bow in his left hand, draws and looses a single purple chakra arrow straight forward in one fast motion, the bow stays visible"),
    "w1_jab": (8, "sword", True, False, "forward", "Mangekyo sword slash: his eyes glow red with the Mangekyo, he slashes forward with the straight sword wreathed in black flames, then back to the ready stance"),
    "w0_nspecial": (12, "guard", True, False, "forward", "draws a bow made of purple Susanoo chakra and looses a blazing purple arrow forward, then returns to the stance"),
    "w1_nspecial": (12, "guard", True, False, "forward", "Amaterasu: brings one hand up beside his left eye, the Mangekyo Sharingan glares straight forward and a tear of blood runs down his cheek, he holds that stare, then lowers the hand and returns to the stance. Only Sasuke is drawn: no flames, no effect in front of him"),
    "w0_ultimate": (28, "guard", True, False, "forward", "Indra's arrow: holds a purple chakra bow in his left hand, draws an arrow back to his cheek with the right hand, holds the aim for a long time, then releases, lowers the bow and returns to the stance. The bow is his size; no giant Susanoo, no other effect"),
    "w1_ultimate": (28, "guard", True, False, "forward", "Amaterasu at full power: covers his left eye with one hand, then pulls the hand away and glares straight forward with the bleeding Mangekyo Sharingan, a line of blood running down his cheek, he stretches the other arm forward and holds that glare for a long time, then returns to the stance. Only Sasuke is drawn: no flames, no effect in front of him"),
    "airdodge": (8, "jump@last", True, False, "none", "air dodge: curls up tightly in mid-air in the same place, the body staying in the middle of the picture, then opens up again in the same pose"),
    "getupattack": (8, "knockdown@last", "guard", False, "around", "lying on his back, spins on the floor and sweeps one leg in a wide circle kicking in front of him and then behind him, then springs up into the fighting stance"),
}
SASUKE_PREP = {
    "drawsword": (8, "guard", None, False, "none", "reaches over his shoulder and draws a straight thin chokuto sword, holding it forward in his right hand in a ready stance"),
}
SASUKE_PREP_B = {
    "airsword": (8, "sword", None, False, "none", "jumps straight up holding the straight sword, tucks the knees up in mid-air, the sword held ready"),
}

SPECIFIC = {"naruto": NARUTO, "sasuke": SASUKE, "kakashi": KAKASHI}


def clips(fighter):
    c = dict(COMMON)
    c.update(STAGE_B)
    c.update(STAGE_C)
    if fighter in OTHERS:  # its attacks wait for its spec (an armed fighter swings its weapon, not its fists)
        sp = spec(fighter)
        if not sp:
            sp = {"pending": True}
            c = {k: v for k, v in c.items() if k in STATES}
        for part in ("overrides", "specific") if "pending" not in sp else ():
            for k, v in sp.get(part, {}).items():
                if k in ("w0_jab", "w1_jab"):
                    continue  # the weapon M1 clips exist already
                c[k] = tuple(v)
        for k in sp.get("skip", []):  # moves this fighter does not have
            c.pop(k, None)
        if fighter in HELD:
            own = set(sp.get("overrides", {})) | set(sp.get("specific", {}))
            held = HELD[fighter]
            for k, (n, base, pin, loop, direction, desc) in list(c.items()):
                if k in own:
                    continue
                if k in ARMED_LEDGE:  # one hand up, the weapon in the other
                    desc = ARMED_LEDGE[k].format(held=held)
                if k == "shield":
                    desc = ARMED_SHIELD.format(held=held)
                elif k == "holdstart":
                    desc = ARMED_HOLD.format(held=held)
                c[k] = (n, base, pin, loop, direction,
                        desc + f" He keeps holding {held} the whole time; the weapon stays visible.")
        for slot, ks in wkit(fighter).get("slots", {}).items():
            new = ks.get("look") == "new"
            if new:
                for k, (n, base, pin, loop, direction, desc) in WSTATES.items():
                    c[f"w{slot}_{k}"] = (n, wbase(slot, base), pin, loop, direction,
                                         desc.format(held=ks["held"]) + ". The whole weapon stays inside the picture.")
            for k, (n, base, pin, loop, direction, desc) in ks.get("clips", {}).items():
                c[k] = (n, wbase(slot, base) if new else base, pin, loop, direction, desc)
        for k, desc in QA_PROMPTS.get(fighter, {}).items():  # prompts rewritten after the clip review (qa14)
            if k in c:
                n, base, pin, loop, direction, _ = c[k]
                c[k] = (n, base, pin, loop, direction, desc)
        for slot, swap in WGUARD.get(fighter, {}).items():
            c[f"w{slot}guard"] = (8, "guard", None, False, "none",
                                  swap + ". Ends standing still in that stance, the weapon fully inside the picture")
            for k, (n, base, pin, loop, direction, desc) in list(c.items()):
                if k.startswith(f"w{slot}_") and base == "guard":
                    c[k] = (n, f"w{slot}guard@last", True if pin is True else pin, loop, direction, desc)
        return c
    if fighter == "sasuke":
        c.pop("jab3", None)
        c.update(SASUKE_PREP)
        c.update(SASUKE_PREP_B)
    c.update(SPECIFIC.get(fighter, {}))
    for k, desc in QA_PROMPTS.get(fighter, {}).items():  # prompts rewritten after the clip review (qa14)
        if k in c:
            n, base, pin, loop, direction, _ = c[k]
            c[k] = (n, base, pin, loop, direction, desc)
    return c


def base_file(fighter, base):
    if base in ("guard", "sword"):
        if base == "guard":
            return os.path.join(S, "base", f"{fighter}_guard256.png")
        return os.path.join(S, fighter, "drawsword", "f_08.png")
    clip = base.split("@")[0]
    d = os.path.join(S, fighter, clip)
    frames = sorted(f for f in os.listdir(d) if f.startswith("f_")) if os.path.isdir(d) else []
    return os.path.join(d, frames[-1]) if frames else None


def jobs(fighter, only=None):
    out = []
    for name, (n, base, pin, loop, direction, desc) in clips(fighter).items():
        if only and name not in only:
            continue
        src = base_file(fighter, base)
        if not src or not os.path.exists(src):
            continue
        last = None
        if pin is True:
            last = src
        elif pin == "guard":  # a weapon that changes the look ends on its own guard
            ks = wslot(fighter, name)
            last = base_file(fighter, wbase(name[1], "guard") if ks and ks.get("look") == "new" else "guard")
        b = base[3:] if wslot(fighter, base) is not None else base  # w1_jump@last: a jump of the weapon kit
        kind = "sword" if b == "sword" else "air" if b.startswith("jump") else \
            "airsword" if b.startswith("airsword") else "ledge" if b.startswith("ledge") else \
            "lying" if b.startswith("knockdown") else "hold" if b.startswith("holdstart") else \
            "run" if b.startswith("runstart") else "guard"
        pose = pose_of(fighter, kind)
        subject = subject_of(fighter)
        ks = wslot(fighter, name)
        if ks and ks.get("look") == "new":  # the weapon's own look
            subject = ks.get("subject", subject)
            pose = ks.get(f"{kind}_pose", pose) if kind in ("guard", "air") else pose
        j = {"out": os.path.join(S, fighter, name), "src": src, "frames": n, "desc": desc + TAIL,
             "subject": subject, "pose": pose}
        if last:
            j["last"] = last
        out.append(j)
    return out


if __name__ == "__main__":
    fighter = sys.argv[1]
    only = sys.argv[3].split(",") if len(sys.argv) > 3 else None
    js = jobs(fighter, only)
    path = os.path.join(S, f"jobs_{fighter}_{sys.argv[2]}.json")
    json.dump(js, open(path, "w"), indent=1)
    print(len(js), "jobs ->", path)
