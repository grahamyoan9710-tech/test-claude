"""Clip specifications for the PixelLab fighters (the new fighters p01, p02...).

Each clip: name -> (frames, base, pin, loop, direction, description)
- base: which picture the clip starts from: 'guard' (the fighting stance, bare-handed), 'w<slot>guard@last' (the
  stance holding a weapon: its armed base picture), or '<clip>@last' (the last frame of another clip: stage B);
- pin: True = the clip must end on its base picture (loops, moves that return to the stance), 'guard' = end on the
  stance, None = free ending;
- direction: where the blow goes (forward, up, down, back, around, none), to find the impact frame;
- the description is the motion only (PixMiniMax: "describe movement, not appearance").
`python3 clips.py FIGHTER STAGE` writes the batch file of that stage (jobs whose base exists).
"""
import json
import os
import re
import sys

S = os.path.dirname(os.path.abspath(__file__))
SUBJECT = {}
POSE = {"guard": "fighting stance facing right, fists raised",
        "air": "in mid-air facing right, knees tucked",
        "ledge": "hanging from his raised hands, arms stretched straight up, legs dangling, empty background above the hands", "lying": "lying on his back on the ground",
        "hold": "holding an invisible opponent in front of him with both hands", "run": "running to the right, leaning forward, arms swept back"}
# The fighters: subjects written for their base (pxa/subjects14.json), the rest from their spec file
# pxa/specs_final/<fighter>.json (written by new_specs.py: their specific clips, guard pose, "she").
SUBJECT.update(json.load(open(os.path.join(S, "subjects14.json"))))
OTHERS = sorted(json.load(open(os.path.join(S, "subjects14.json"))))
STATES = {"idle", "walk", "runstart", "jump", "crouch", "land", "shield", "roll", "spotdodge", "hurt", "knockdown", "dizzy",
          "grabbed", "holdstart", "victory", "run", "fall", "airjump", "airdodge", "helpless", "tumble", "ledgegrab",
          "getup", "hold", "ledge", "climb"}


# The base stance of each fighter (what its guard picture shows), until its spec gives its own "guard_pose".
GUARD14 = {}


# A weapon that changes the look: its w<slot>_ clips start from the guard holding it (pxa/base/<id>_w<slot>guard256.png,
# made by make_bases.py with the owner's weapon as a reference), and the w<slot>guard clip (taking the weapon) ends on
# it. The motion of taking it comes from the weapon kit ("wguard").
WGUARD = {}


# What a fighter holds in its own guard picture (none: the new fighters start bare-handed).
HELD = {}
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
    "idle": (8, "guard", True, True, "none", "idle stance loop: breathes calmly, shoulders rise and fall, knees bounce slightly, hair and clothes sway in the wind, {held} held exactly as in the first frame, the feet stay planted, returns exactly to the first pose"),
    "walk": (8, "guard", True, True, "none", "walks forward to the right in side profile: the back leg steps past the front leg, then the other, the body and head stay turned to the right like the first frame, {held} held exactly as in the first frame, a seamless two-step walk cycle that ends exactly on the first pose"),
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
        if kind == "guard" and g:
            return g
        if kind == "air":
            return "in mid-air facing right, knees tucked" + ("" if sp.get("armed", "none") in ("none", "fists", "magic", "") else ", weapon in hand")
    return POSE[kind]


def spec(fighter):
    p = os.path.join(S, "specs_final", f"{fighter}.json")  # reviewed copies of specs_early
    return json.load(open(p)) if os.path.exists(p) else None


TAIL = (" The character stays at the same place in the frame (the game moves him), always facing right, seen from the side;"
        " keep his look, proportions and colours exactly. Only the character is drawn, with what he holds: no effect, no"
        " motion streak, no speed lines, no trail behind the limbs or the weapon, no glow, no energy, no projectile, no"
        " smoke, no dust, no ground, the whole body inside the picture. Every frame shows the arms, legs and weapon"
        " clearly in a crisp pose, never blurred.")
# Blows (the owner allows the trails of fast blows and blades): nothing else is drawn but the character.
TAIL_BLOW = (" The character stays at the same place in the frame (the game moves him), always facing right, seen from the"
             " side; keep his look, proportions and colours exactly. Only the character is drawn, with what he holds: no"
             " glow, no energy, no projectile, no smoke, no dust, no ground, the whole body inside the picture.")

# --- the moves every fighter has (bare-handed brawler moves) ---
COMMON = {
    # name: (frames, base, pin, loop, direction, description)
    "idle": (8, "guard", True, True, "none", "idle fighting stance loop: breathes calmly, shoulders rise and fall, knees bounce slightly, hair and clothes sway in the wind, the feet stay planted, returns exactly to the first pose"),
    "walk": (8, "guard", True, True, "none", "walks forward to the right in side profile: the back leg steps past the front leg, then the other, the body and head stay turned to the right the whole time exactly like the first frame, fists raised, a seamless two-step walk cycle that ends exactly on the first pose"),
    "runstart": (8, "guard", None, False, "none", "breaks into a fast run to the right: leans the torso far forward and sweeps both arms straight back behind him, legs start a long running stride"),
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
    "jab3": (8, "guard", True, False, "forward", "strong front kick: lifts the front knee and kicks straight forward at belly height, then back into the stance"),
    "ftilt": (8, "guard", True, False, "forward", "stepping side kick: steps forward and kicks straight ahead at chest height, then back into the stance"),
    "utilt": (8, "guard", True, False, "up", "upward swing: bends the knees, then swings the right fist in a big arc from low in front straight up above his head (an uppercut ending with the arm pointing at the sky), then back into the stance"),
    "dtilt": (8, "guard", True, False, "forward", "crouching leg sweep: drops low and sweeps the front leg along the ground in front, then rises back into the stance"),
    "fsmash": (12, "guard", True, False, "forward", "charged power punch: coils far back with the fist pulled behind, then lunges forward and drives the fist straight ahead at full extension, then recovers into the stance"),
    "usmash": (12, "guard", True, False, "up", "upward smash: crouches, then springs up on his toes and strikes straight upward above his head with both fists, then lands back in the stance"),
    "dsmash": (12, "guard", True, False, "around", "low split kicks: drops low and kicks fast along the ground in front, then spins and kicks behind, then back into the stance"),
    "grab": (8, "guard", True, False, "forward", "lunging grab: reaches forward with both hands to seize an opponent, then pulls back"),
    "ledgeattack": (8, "guard", True, False, "forward", "rises from a low crouch with a quick sweeping kick forward along the ground, then back into the fighting stance"),
}
STAGE_B = {
    "run": (8, "runstart@last", True, True, "none", "fast run cycle to the right: leaning far forward, both arms swept straight back, long fast strides, a seamless loop that ends exactly on the first pose"),
    "dashattack": (12, "runstart@last", "guard", False, "forward", "running tackle: rushes forward shoulder first, then slides to a stop and stands back up in the fighting stance"),
    "fall": (8, "jump@last", True, True, "none", "falls through the air: legs reach down, arms out for balance, hair and clothes flutter upward, a seamless loop that ends exactly on the first pose"),
    "airjump": (8, "jump@last", True, False, "none", "double jump in mid-air: kicks off the air and does a full forward somersault, ending tucked in the same pose"),
    "airdodge": (8, "jump@last", True, False, "none", "air dodge: curls up tightly in mid-air in the same place, the body staying in the middle of the picture, then opens up again in the same pose"),
    "helpless": (8, "jump@last", True, True, "none", "falls helplessly, exhausted: both arms hang limp and loose straight up above his head, head drooping, eyes half closed, legs dangling loosely below him, the body tilting slowly as he drops, a seamless loop that ends exactly on the first pose. Only the character is drawn"),
    "tumble": (8, "jump@last", True, True, "none", "tumbles head over heels through the air after a heavy blow, spinning out of control, a seamless loop"),
    "ledgegrab": (8, "jump@last", None, False, "none", "jumps up and stretches both arms straight up above his head, the hands closing on nothing near the top of the picture but fully inside it, then hangs still from his raised hands with the legs dangling below; empty background above and around the hands: no log, no bar, no branch, no cliff, no rock, no wall, no platform, no ground"),
    "nair": (12, "jump@last", True, False, "around", "neutral aerial: curls into a fast spinning ball in mid-air, then opens up again"),
    "fair": (12, "jump@last", True, False, "forward", "forward aerial: a powerful flying kick straight forward in mid-air, then pulls the leg back in"),
    "bair": (8, "jump@last", True, False, "back", "back aerial in mid-air: looks back over his shoulder and kicks one leg straight out behind him to the left, horizontal and fully extended, then pulls it back into the tucked jump pose"),
    "uair": (8, "jump@last", True, False, "up", "up aerial: flips backward and kicks straight up above his head in mid-air (a bicycle kick), then back to the tucked pose"),
    "dair": (12, "jump@last", True, False, "down", "down aerial: stomps straight down beneath him with both feet in mid-air, then pulls the legs back up"),
    "getup": (8, "knockdown@last", "guard", False, "none", "gets up from lying on his back: rolls onto his feet and stands back up into the fighting stance"),
    "getupattack": (8, "knockdown@last", "guard", False, "around", "lying on his back, spins on the floor and sweeps one leg in a wide circle kicking in front of him and then behind him, then springs up into the fighting stance"),
    "hold": (4, "holdstart@last", True, True, "none", "holds an invisible opponent in front of him with both hands, arms extended and fists closed on empty air, breathing calmly, a seamless loop. Only the character is drawn: the opponent is invisible, there is no other person in the frame."),
    "pummel": (4, "holdstart@last", True, False, "forward", "knees the invisible opponent he is holding in front of him: a quick knee strike, then back to holding with both hands. Only the character is drawn: the opponent is invisible, there is no other person in the frame."),
    "fthrow": (8, "holdstart@last", "guard", False, "forward", "throws the invisible opponent he is holding forward: a strong two-handed heave and push, then returns to the fighting stance. Only the character is drawn: the opponent is invisible, there is no other person in the frame."),
    "bthrow": (12, "holdstart@last", "guard", False, "back", "swings the invisible opponent he is holding around behind him and hurls it away, turning his body, then returns to the fighting stance. Only the character is drawn: the opponent is invisible, there is no other person in the frame."),
    "uthrow": (8, "holdstart@last", "guard", False, "up", "hoists the invisible opponent he is holding high above his head with both arms and throws it straight up, then returns to the fighting stance. Only the character is drawn: the opponent is invisible, there is no other person in the frame."),
    "dthrow": (12, "holdstart@last", "guard", False, "down", "slams the invisible opponent he is holding down onto the ground in front of him with both hands, crouching, then returns to the fighting stance. Only the character is drawn: the opponent is invisible, there is no other person in the frame."),
}
STAGE_C = {
    "ledge": (4, "ledgegrab@last", True, True, "none", "hangs from his raised hands, both arms stretched straight up with the hands closed on nothing near the top of the picture, legs swaying gently below, a seamless loop; empty background above and around the hands: no log, no bar, no branch, no cliff, no rock, no wall, no platform"),
    "climb": (8, "ledgegrab@last", "guard", False, "none", "from hanging by his raised hands, he pulls his chest up to the height of his hands, swings one knee up, pushes himself up and stands up in the fighting stance; nothing is drawn around him: no log, no bar, no branch, no cliff, no rock, no wall, no platform, no ground"),
}

SPECIFIC = {}


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
        add_kits(fighter, c)
        for k, desc in QA_PROMPTS.get(fighter, {}).items():  # prompts rewritten after the clip review (qa14)
            if k in c:
                n, base, pin, loop, direction, _ = c[k]
                c[k] = (n, base, pin, loop, direction, desc)
        add_guards(fighter, c)
        return c
    c.update(SPECIFIC.get(fighter, {}))
    add_kits(fighter, c)
    for k, desc in QA_PROMPTS.get(fighter, {}).items():  # prompts rewritten after the clip review (qa14)
        if k in c:
            n, base, pin, loop, direction, _ = c[k]
            c[k] = (n, base, pin, loop, direction, desc)
    add_guards(fighter, c)
    return c


def add_kits(fighter, c):
    """The clips of the fighter's weapon kits (pxa/wkits): a new look gets every state too, holding the weapon
    (WSTATES), or, for a power with nothing in hand (a mode, a cloak), the fighter's own state prompts."""
    for slot, ks in wkit(fighter).get("slots", {}).items():
        new = ks.get("look") == "new"
        if new:
            for k, (n, base, pin, loop, direction, desc) in WSTATES.items():
                if ks.get("held"):
                    desc = desc.format(held=ks["held"]) + ". The whole weapon stays inside the picture."
                elif k in c:
                    n, base, pin, loop, direction, desc = c[k]
                else:
                    continue
                c[f"w{slot}_{k}"] = (n, wbase(slot, base), pin, loop, direction, desc)
        for k, (n, base, pin, loop, direction, desc) in ks.get("clips", {}).items():
            c[k] = (n, wbase(slot, base) if new else base, pin, loop, direction, desc)


def add_guards(fighter, c):
    """The w<slot>guard helper clips (the swap to a weapon that changes the look) and the kit clips based on them."""
    swaps = dict(WGUARD.get(fighter, {}))
    swaps.update({int(slot): ks["wguard"] for slot, ks in wkit(fighter).get("slots", {}).items() if ks.get("wguard")})
    for slot, swap in swaps.items():
        c[f"w{slot}guard"] = (8, "guard", None, False, "none",
                              swap + ". Ends standing still in that stance, the weapon fully inside the picture")
        for k, (n, base, pin, loop, direction, desc) in list(c.items()):
            if k.startswith(f"w{slot}_") and base == "guard":
                c[k] = (n, f"w{slot}guard@last", True if pin is True else pin, loop, direction, desc)


def armed_base(fighter, slot):
    """The fighter's guard holding the weapon of that slot (make_bases.py armedpick), or None."""
    p = os.path.join(S, "base", f"{fighter}_w{slot}guard256.png")
    return p if os.path.exists(p) else None


def base_file(fighter, base):
    if base == "guard":
        return os.path.join(S, "base", f"{fighter}_guard256.png")
    m = re.fullmatch(r"w(\d)guard@last", base)
    if m and armed_base(fighter, m.group(1)):  # the exact armed picture, not the end of the swap clip
        return armed_base(fighter, m.group(1))
    clip = base.split("@")[0]
    d = os.path.join(S, fighter, clip)
    frames = sorted(f for f in os.listdir(d) if f.startswith("f_")) if os.path.isdir(d) else []
    return os.path.join(d, frames[-1]) if frames else None


SHE = [(r"\bhe\b", "she"), (r"\bHe\b", "She"), (r"\bhim\b", "her"), (r"\bhis\b", "her"), (r"\bHis\b", "Her"),
       (r"\bhimself\b", "herself")]


def feminine(text):
    for a, b in SHE:
        text = re.sub(a, b, text)
    return text


def jobs(fighter, only=None):
    out = []
    she = (spec(fighter) or {}).get("she", False)
    for name, (n, base, pin, loop, direction, desc) in clips(fighter).items():
        if only is not None and name not in only:  # an empty list is no clip at all, never every clip
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
        m = re.fullmatch(r"w(\d)guard", name)
        if m and armed_base(fighter, m.group(1)):  # taking the weapon ends on the armed picture
            last = armed_base(fighter, m.group(1))
        b = base[3:] if wslot(fighter, base) is not None else base  # w1_jump@last: a jump of the weapon kit
        kind = "air" if b.startswith("jump") else "ledge" if b.startswith("ledge") else \
            "lying" if b.startswith("knockdown") else "hold" if b.startswith("holdstart") else \
            "run" if b.startswith("runstart") else "guard"
        pose = pose_of(fighter, kind)
        subject = subject_of(fighter)
        ks = wslot(fighter, name)
        if ks and ks.get("look") == "new":  # the weapon's own look
            subject = ks.get("subject", subject)
            pose = ks.get(f"{kind}_pose", pose) if kind in ("guard", "air") else pose
        desc = desc.rstrip(" .") + "." + (TAIL_BLOW if direction not in ("none", None) else TAIL)
        if she:
            desc, pose = feminine(desc), feminine(pose)
        j = {"out": os.path.join(S, fighter, name), "src": src, "frames": n, "desc": desc,
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
