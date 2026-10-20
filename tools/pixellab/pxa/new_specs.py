#!/usr/bin/env python3
"""Writes the clip specs of the new fighters (docs/refonte/FICHES.md) for clips.py:
specs_final/<id>.json (bare hands: guard, the four skills and the ultimate) and wkits/<id>.json (the two weapon
kits: a new look each, so every state is drawn holding the weapon, plus the 3-hit M1, the air M1, four skills and
the ultimate). Run it after changing a prompt: python3 new_specs.py

A clip: [frames, base, pin, loop, direction, motion]. Motions only (PixMiniMax: "describe movement, not
appearance"); the game draws every effect, so no prompt asks for one.
"""
import json
import os

S = os.path.dirname(os.path.abspath(__file__))
SUBJECTS = json.load(open(os.path.join(S, "subjects14.json")))


def clip(frames, motion, direction="forward", base="guard", pin=True):
    return [frames, base, pin, False, direction, motion]


def air(frames, motion, direction="around"):
    return [frames, "jump@last", True, False, direction, motion]


# --- per fighter: the bare-handed skills, then each weapon (held, how it is taken, its moves) ---
F = {}
F["p01"] = {
    "she": False,
    "skills": {
        "nspecial": clip(12, "points two fingers forward at an opponent and holds the pose, his coat swaying"),
        "sspecial": clip(12, "dashes forward very low and fast, turns around on the spot and strikes behind him with a quick elbow, then turns back into the stance", "back"),
        "uspecial": clip(12, "crouches low and gathers himself, then springs straight up with his arms tucked, his coat streaming below him", "up"),
        "dspecial": clip(12, "raises one forearm in a calm parry, then answers with a fast backhand strike", "forward"),
        "ultimate": clip(16, "spreads his arms wide and throws his head back, his coat and hair lifting, then settles into a stronger stance", "none"),
    },
    "weapons": {
        0: {"held": "a long curved black odachi sword with a violet ribbon",
            "take": "reaches over his shoulder and draws a long curved black odachi sword with a violet ribbon, then holds it in both hands, the blade low behind him",
            "moves": {
                "jab": clip(8, "a fast horizontal cut of the odachi from left to right at chest height"),
                "jab2": clip(8, "a rising diagonal cut of the odachi from low in front up over his head"),
                "jab3": clip(12, "a big two-handed downward cut of the odachi from high above to the ground in front"),
                "nair": air(12, "in mid-air, spins once and sweeps the odachi in a full circle around him"),
                "nspecial": clip(12, "swings the odachi in a wide low arc along the ground, sending its edge forward"),
                "sspecial": clip(12, "dashes forward low and draws a lightning-fast horizontal cut, ending far ahead with the blade extended"),
                "uspecial": clip(12, "spins upward with the odachi held out, cutting in a rising spiral", "up"),
                "dspecial": clip(12, "holds the odachi crosswise in front of him to parry, then counters with a quick cut"),
                "ultimate": clip(16, "crouches low with the odachi drawn back, holds perfectly still, then rises and slowly slides the blade home into an invisible sheath at his hip", "none"),
            }},
        1: {"held": "two curved violet daggers, one in each hand",
            "take": "crosses his arms and draws two curved violet daggers from his belt, one in each hand, then crouches in a low ready stance",
            "moves": {
                "jab": clip(8, "a quick slash with the dagger in his front hand"),
                "jab2": clip(8, "a quick slash with the dagger in his back hand"),
                "jab3": clip(12, "stabs both daggers forward together in an X"),
                "nair": air(12, "in mid-air, spins with both daggers out, slashing all around"),
                "nspecial": clip(12, "throws both daggers forward with a snap of the wrists, then catches them as they come back"),
                "sspecial": clip(12, "rushes forward slashing five times with alternating daggers"),
                "uspecial": clip(12, "drills upward spinning fast with both daggers held out", "up"),
                "dspecial": clip(12, "stabs a dagger into the ground in front of him, then pulls it out and stands back up", "down"),
                "ultimate": clip(16, "dashes forward and slashes in every direction with both daggers in a whirl of cuts, ending in a crossed X pose"),
            }},
    },
}
F["p02"] = {
    "she": True,
    "skills": {
        "nspecial": clip(12, "flicks one open hand forward as if throwing a shard, then pulls it back"),
        "sspecial": clip(12, "slides forward low on one leg and kicks with the other, ending in a low crouch"),
        "uspecial": clip(12, "jumps straight up with her arms raised, her hair and coat streaming below her", "up"),
        "dspecial": clip(12, "raises both palms in front of her face as a guard, then pushes them forward"),
        "ultimate": clip(16, "spreads her arms and floats slightly, her hair and coat rising around her, then lands in a stronger stance", "none"),
    },
    "weapons": {
        0: {"held": "a long silver spear with white ribbons",
            "take": "reaches to her side and draws out a long silver spear with white ribbons, then holds it in both hands, the point forward",
            "moves": {
                "jab": clip(8, "a quick straight thrust of the spear forward"),
                "jab2": clip(8, "two quick thrusts of the spear forward"),
                "jab3": clip(12, "a wide circular sweep of the spear around her at waist height", "around"),
                "nair": air(12, "in mid-air, thrusts the spear down in front, then straight ahead"),
                "nspecial": clip(12, "throws the spear forward with a full overhand throw, then catches it as it comes back"),
                "sspecial": clip(12, "charges forward with the spear held out in front, then skids to a stop"),
                "uspecial": clip(12, "spins upward with the spear twirling above her head", "up"),
                "dspecial": clip(12, "stabs the spear point into the ground in front of her, then pulls it back out", "down"),
                "ultimate": clip(16, "plants the spear upright in the ground beside her and holds it with both hands, head bowed, then pulls it out", "none"),
            }},
        1: {"held": "a staff topped by a blue crystal star with blue ribbons",
            "take": "raises one hand and grips a staff topped by a blue crystal star with blue ribbons, then holds it upright beside her",
            "moves": {
                "jab": clip(8, "a short swing of the star staff forward"),
                "jab2": clip(8, "a backhand swing of the star staff"),
                "jab3": clip(12, "raises the star staff and strikes forward with its star head"),
                "nair": air(12, "in mid-air, swings the star staff in a full circle around her"),
                "nspecial": clip(12, "points the star staff forward and up, as if casting a lob"),
                "sspecial": clip(12, "twirls the star staff in front of her, then points it forward"),
                "uspecial": clip(12, "raises the star staff overhead and jumps straight up", "up"),
                "dspecial": clip(12, "plants the star staff in front of her and holds it with both hands as a guard"),
                "ultimate": clip(16, "raises the star staff high to the sky with both hands and holds it there, then lowers it", "none"),
            }},
    },
}
F["p03"] = {
    "she": False,
    "skills": {
        "nspecial": clip(12, "a short powerful straight punch forward, the scarf flaring behind him"),
        "sspecial": clip(12, "charges forward shoulder first, then stops in a wide stance"),
        "uspecial": clip(12, "a jumping uppercut that carries him straight up, spinning once", "up"),
        "dspecial": clip(12, "punches both fists into the ground in front of him, then stands back up", "down"),
        "ultimate": clip(16, "clenches his fists and roars with his head thrown back, arms tense, then drops into a lower stance", "none"),
    },
    "weapons": {
        0: {"held": "a huge jagged black greatsword with glowing red veins",
            "take": "reaches behind his back and swings a huge jagged black greatsword with glowing red veins onto his shoulder",
            "moves": {
                "jab": clip(8, "a heavy downward cut of the greatsword from the shoulder to the front"),
                "jab2": clip(8, "a heavy backhand sweep of the greatsword at chest height"),
                "jab3": clip(12, "lifts the greatsword overhead with both hands and smashes it into the ground in front", "down"),
                "nair": air(12, "in mid-air, swings the greatsword in a full circle around him"),
                "nspecial": clip(12, "swings the greatsword low along the ground in a wide forward arc"),
                "sspecial": clip(12, "lunges forward with a huge horizontal swing of the greatsword"),
                "uspecial": clip(12, "leaps up with a rising cut of the greatsword above his head", "up"),
                "dspecial": clip(12, "drives the greatsword point down into the ground in front of him with both hands", "down"),
                "ultimate": clip(16, "raises the greatsword high overhead with both hands, holds it, then cuts straight down", "down"),
            }},
        1: {"held": "a huge spiked black and red greatsword, larger and more jagged",
            "take": "reaches behind his back and swings a huge spiked black and red greatsword onto his shoulder",
            "moves": {
                "jab": clip(8, "a heavy diagonal cut of the spiked greatsword"),
                "jab2": clip(8, "a crossing cut of the spiked greatsword the other way"),
                "jab3": clip(12, "a heavy overhead blow of the spiked greatsword into the ground", "down"),
                "nair": air(12, "in mid-air, swings the spiked greatsword in a full circle around him"),
                "nspecial": clip(12, "swings the spiked greatsword forward in a big overhead arc and stops it pointing straight ahead"),
                "sspecial": clip(12, "dashes forward dragging the spiked greatsword low behind him, then swings it up"),
                "uspecial": clip(12, "spins upward with the spiked greatsword held out", "up"),
                "dspecial": clip(12, "spins once in place with the spiked greatsword held out at waist height", "around"),
                "ultimate": clip(16, "hurls the spiked greatsword straight up into the sky, then holds his arms up waiting, then catches it", "up"),
            }},
    },
}
F["p07"] = {
    "she": True,
    "skills": {
        "nspecial": clip(12, "thrusts one open hand forward fast, fingers stiff, then pulls it back"),
        "sspecial": clip(12, "dashes forward very low and fast and finishes with a quick flying knee strike, then lands back in the stance"),
        "uspecial": clip(12, "a rising flying kick straight up, her ponytail streaming below her", "up"),
        "dspecial": clip(12, "crouches and slams one palm onto the ground in front of her, then stands back up", "down"),
        "ultimate": clip(16, "raises one hand high to the sky and holds it there, then swings it down", "none"),
    },
    "weapons": {
        0: {"held": "a broad black and red sword with red ribbons",
            "take": "draws a broad black and red sword with red ribbons from her hip and holds it forward in one hand",
            "moves": {
                "jab": clip(8, "a fast horizontal cut of the sword"),
                "jab2": clip(8, "a fast return cut of the sword the other way"),
                "jab3": clip(12, "a lunging straight thrust of the sword far forward"),
                "nair": air(12, "in mid-air, spins and cuts all around her with the sword"),
                "nspecial": clip(12, "swings the sword in a fast forward arc, stopping with the blade extended"),
                "sspecial": clip(12, "dashes forward very low and fast and finishes with the sword extended far ahead"),
                "uspecial": clip(12, "leaps straight up with the sword raised above her head", "up"),
                "dspecial": clip(12, "holds the sword crosswise to parry, then counters with a quick cut"),
                "ultimate": clip(16, "zigzags forward in fast dashes, cutting at each turn, then stops with her back turned and the sword lowered"),
            }},
        1: {"held": "a broad black and red sword, larger and more jagged, with red ribbons",
            "take": "draws a larger jagged black and red sword with red ribbons from her hip and holds it forward in one hand",
            "moves": {
                "jab": clip(8, "a fast cut of the jagged sword"),
                "jab2": clip(8, "a return cut of the jagged sword"),
                "jab3": clip(12, "a lunging thrust of the jagged sword far forward"),
                "nair": air(12, "in mid-air, spins and cuts all around her with the jagged sword"),
                "nspecial": clip(12, "draws back the jagged sword and hurls her free hand forward as if throwing a spear"),
                "sspecial": clip(12, "rushes forward cutting twice with the jagged sword"),
                "uspecial": clip(12, "spins upward with the jagged sword held out", "up"),
                "dspecial": clip(12, "stabs the jagged sword into the ground in front of her, then pulls it out", "down"),
                "ultimate": clip(16, "points the jagged sword to the sky with both hands, holds it, then swings it down at the ground ahead", "down"),
            }},
    },
}
F["p08"] = {
    "she": False,
    "skills": {
        "nspecial": clip(12, "flicks one hand forward as if throwing a feather, then pulls it back"),
        "sspecial": clip(12, "dashes forward low, his coat streaming behind him, then stops"),
        "uspecial": clip(12, "jumps straight up with his arms spread like wings, coat flaring", "up"),
        "dspecial": clip(12, "raises both forearms crossed in front of him as a guard, then opens them"),
        "ultimate": clip(16, "spreads his arms and rises on his toes, his coat and hair lifting, then lands in a calm stance", "none"),
    },
    "weapons": {
        0: {"held": "a white and gold sword with white ribbons",
            "take": "draws a white and gold sword with white ribbons from his side and holds it in one hand, the point down",
            "moves": {
                "jab": clip(8, "a clean horizontal cut of the sword"),
                "jab2": clip(8, "a return cut of the sword the other way"),
                "jab3": clip(12, "two crossing cuts of the sword in an X, ending with the blade forward"),
                "nair": air(12, "in mid-air, spins and cuts all around him with the sword"),
                "nspecial": clip(12, "thrusts the sword straight forward and holds it extended"),
                "sspecial": clip(12, "dashes forward with a fast cut, ending far ahead with the sword extended"),
                "uspecial": clip(12, "spins upward with the sword held out, cutting all around", "up"),
                "dspecial": clip(12, "holds the sword vertical in front of him to parry, then counters with a quick cut"),
                "ultimate": clip(16, "raises the sword to the sky with both hands and holds it, then points it down at the ground ahead", "down"),
            }},
        1: {"held": "a golden halberd",
            "take": "grips a golden halberd that he swings down from over his shoulder, then holds it upright in both hands",
            "moves": {
                "jab": clip(8, "a straight thrust of the halberd forward"),
                "jab2": clip(8, "a wide horizontal sweep of the halberd"),
                "jab3": clip(12, "spins the halberd once and finishes with a long thrust forward"),
                "nair": air(12, "in mid-air, twirls the halberd in a full circle around him"),
                "nspecial": clip(12, "throws the halberd forward like a javelin, then catches it as it comes back"),
                "sspecial": clip(12, "charges forward with the halberd held out in front, then skids to a stop"),
                "uspecial": clip(12, "leaps up with a rising sweep of the halberd above his head", "up"),
                "dspecial": clip(12, "slams the halberd's butt into the ground in front of him, then lifts it", "down"),
                "ultimate": clip(16, "raises the halberd high to the sky with both hands and holds it, then lowers it", "none"),
            }},
    },
}


# Bare hands: each fighter fights in a martial art (the owner's list of 10 styles); these motions replace the generic
# brawler normals of clips.py (same frames, base and direction). With a weapon, its kit moves are used instead.
STYLE_OF = {"p01": "Penchak Silat", "p02": "Taekwondo", "p03": "Muay Thai", "p07": "Karate", "p08": "Kung Fu"}
STYLES = {
    "Penchak Silat": {
        "jab": "silat open-hand strike from a low crouched stance: snaps a palm-heel strike straight forward at face height, then back",
        "jab2": "silat elbow strike: steps in low and swings the rear elbow horizontally forward at head height, then back into the low stance",
        "jab3": "silat crescent kick: turns low and whips a sharp heel kick forward in an arc at chest height, then back into the low stance",
        "ftilt": "silat side kick: turns sideways and thrusts the heel straight forward at waist height, then back into the low stance",
        "utilt": "silat rising claw: from a low crouch sweeps one open clawed hand in a high arc up above his head, then back",
        "dtilt": "silat floor sweep: drops into a deep squat and sweeps one straight leg in a wide arc along the ground in front, then rises",
        "dashattack": "silat sliding strike: drops low and slides forward on one bent knee thrusting a palm strike ahead, then rises into the low stance",
        "fsmash": "silat double palm strike: sinks low drawing both hands back to the hip, then explodes forward driving both open palms straight ahead at chest height, then recovers",
        "usmash": "silat rising crescent kick: spins and swings one leg in a high vertical crescent arc over his head, then lands back in the low stance",
        "dsmash": "silat ground sweep: drops onto one hand and spins sweeping his extended leg around, in front and then behind him, then rises",
        "nair": "in mid-air, tucks and spins once with elbows and knees out, then opens up again",
        "fair": "in mid-air, a sharp flying knee straight forward, then pulls the leg back",
        "bair": "in mid-air, looks back and thrusts one heel straight out behind him, then back to the tucked pose",
        "uair": "in mid-air, an upward claw strike sweeping one open hand over his head, then back to the tucked pose",
        "dair": "in mid-air, stomps one heel straight down below him, then pulls the leg back up",
    },
    "Taekwondo": {
        "jab": "taekwondo lead-leg roundhouse kick: snaps the front foot up in a quick roundhouse at waist height, then puts it back down in the stance",
        "jab2": "taekwondo rear-leg roundhouse kick: whips the back leg around in a fast roundhouse at chest height, then back into the stance",
        "jab3": "taekwondo spinning back kick: spins around and thrusts the heel straight forward at chest height, then back into the stance",
        "ftilt": "taekwondo side kick: lifts the lead knee and thrusts the foot sideways straight forward at chest height, then back",
        "utilt": "taekwondo axe kick: swings one straight leg high above her head, then brings it back down into the stance",
        "dtilt": "taekwondo low sweep: crouches and sweeps one extended leg along the ground in front, then rises",
        "dashattack": "taekwondo flying side kick: leaps forward with one leg extended straight ahead, then lands in the stance",
        "fsmash": "taekwondo tornado kick: spins a full turn on one foot and whips the other leg around in a high roundhouse forward at head height, then lands in the stance",
        "usmash": "taekwondo jumping scissor kick: springs straight up kicking both legs high above her head one after the other, then lands",
        "dsmash": "taekwondo spinning sweep: crouches and spins with one leg extended, sweeping in front and then behind her, then rises",
        "nair": "in mid-air, a spinning kick with both legs extended all around, then tucks again",
        "fair": "in mid-air, a flying side kick straight forward, then pulls the leg back",
        "bair": "in mid-air, a back kick thrusting one heel straight behind her, then back to the tucked pose",
        "uair": "in mid-air, a bicycle kick straight up above her head, then back to the tucked pose",
        "dair": "in mid-air, an axe kick: brings one straight leg down hard beneath her, then pulls it back",
    },
    "Muay Thai": {
        "jab": "muay thai jab: a fast straight punch with the lead fist at face height, then back to the high guard",
        "jab2": "muay thai elbow: steps in and swings the rear elbow horizontally across at head height, then back to the high guard",
        "jab3": "muay thai knee: reaches forward with both hands as if clinching an invisible opponent and drives the rear knee up and forward at belly height, then back",
        "ftilt": "muay thai teep: lifts the lead knee and pushes the sole of the foot straight forward at belly height, then back to the guard",
        "utilt": "muay thai upward elbow: drives the rear elbow straight up in front of his face, then back to the high guard",
        "dtilt": "muay thai low kick: swings the rear shin low in front at knee height, then back to the guard",
        "dashattack": "muay thai flying knee: leaps forward driving one knee straight ahead with both arms raised, then lands in the high guard",
        "fsmash": "muay thai roundhouse kick: pivots on the lead foot and swings the rear shin in a wide powerful arc forward at rib height, then back to the high guard",
        "usmash": "muay thai jumping knee: springs straight up driving one knee high above, arms reaching up, then lands in the guard",
        "dsmash": "muay thai double elbow: chops both elbows down in front of him, then turns and strikes a back elbow behind him, then the guard",
        "nair": "in mid-air, knees and elbows tucked tight, spinning once, then opens up",
        "fair": "in mid-air, a flying knee straight forward, then pulls the leg back",
        "bair": "in mid-air, a spinning back elbow behind him, then back to the tucked pose",
        "uair": "in mid-air, an upward elbow strike straight above his head, then back to the tucked pose",
        "dair": "in mid-air, a downward double elbow chop beneath him, then back to the tucked pose",
    },
    "Karate": {
        "jab": "karate lead punch: from a low front stance snaps a sharp straight punch with the lead fist, then pulls it back",
        "jab2": "karate reverse punch: drives the rear fist straight forward with a twist of the hips, the other fist pulled back to the hip, then back",
        "jab3": "karate front kick: snaps the rear foot straight forward at chest height, then back into the stance",
        "ftilt": "karate side thrust kick: turns sideways and thrusts the heel straight forward at chest height, then back",
        "utilt": "karate rising block strike: sweeps the forearm straight up above her head, then back into the stance",
        "dtilt": "karate foot sweep: drops low and sweeps the foot along the ground in front, then rises into the stance",
        "dashattack": "karate lunge punch: lunges forward into a long front stance driving a straight punch ahead, then stops in the stance",
        "fsmash": "karate knife-hand strike: raises one open hand behind her head, then drives a powerful knife-hand chop forward and down, then back into the stance",
        "usmash": "karate jumping front kick: springs up and snaps a kick straight up above her head, then lands in the stance",
        "dsmash": "karate low spinning sweep: drops and sweeps one leg along the ground in front and then behind her, then rises",
        "nair": "in mid-air, spins once with both legs extended, then tucks again",
        "fair": "in mid-air, a flying side kick straight forward, then pulls the leg back",
        "bair": "in mid-air, a back kick thrusting one heel straight behind her, then back to the tucked pose",
        "uair": "in mid-air, a rising knife-hand strike above her head, then back to the tucked pose",
        "dair": "in mid-air, a downward heel kick straight below her, then pulls the leg back",
    },
    "Kung Fu": {
        "jab": "kung fu chain punch: a fast vertical-fist punch straight forward, then back",
        "jab2": "kung fu palm strike: a flowing open-palm strike forward with the other hand, then back",
        "jab3": "kung fu crane kick: lifts one knee high with both arms spread like wings and snaps a kick straight forward at chest height, then back",
        "ftilt": "kung fu tiger claw: lunges forward raking one clawed hand at chest height, then back into the stance",
        "utilt": "kung fu rising palm: sweeps one palm in a circle straight up above his head, then back into the stance",
        "dtilt": "kung fu low sweep: sinks into a low horse stance and sweeps one leg along the ground in front, then rises",
        "dashattack": "kung fu flying kick: leaps forward with one leg extended straight ahead, then lands in the stance",
        "fsmash": "kung fu dragon palms: circles both arms in a flowing motion, then thrusts both palms straight forward at chest height, then back into the stance",
        "usmash": "kung fu jumping crescent kick: springs up and swings one leg in a high arc above his head, then lands in the stance",
        "dsmash": "kung fu sweeping kick: drops low and sweeps one leg around along the ground, in front and then behind him, then rises",
        "nair": "in mid-air, a butterfly twist with both legs extended, then tucks again",
        "fair": "in mid-air, a flying kick straight forward, then pulls the leg back",
        "bair": "in mid-air, a back palm strike behind him, then back to the tucked pose",
        "uair": "in mid-air, a scissor kick straight up above his head, then back to the tucked pose",
        "dair": "in mid-air, pushes both palms straight down beneath him, then back to the tucked pose",
    },
}


def style_overrides(f):
    """The bare-handed normals in the fighter's martial art: the generic clip with the style's motion."""
    import clips as C
    base = dict(C.COMMON)
    base.update(C.STAGE_B)
    return {k: [*base[k][:5], motion] for k, motion in STYLES[STYLE_OF[f]].items()}


def main():
    os.makedirs(os.path.join(S, "specs_final"), exist_ok=True)
    os.makedirs(os.path.join(S, "wkits"), exist_ok=True)
    for f, d in F.items():
        json.dump({"guard_pose": "fighting stance facing right, fists raised", "armed": "none", "she": d["she"],
                   "specific": d["skills"], "style": STYLE_OF[f], "overrides": style_overrides(f)}, open(os.path.join(S, "specs_final", f"{f}.json"), "w"), indent=1)
        slots = {}
        for slot, w in d["weapons"].items():
            moves = {f"w{slot}_{k}": v for k, v in w["moves"].items()}
            # No "wguard" swap clip: the weapon clips start from the armed base (make_bases.py armedpick), and the
            # game shows taking the weapon with its effects; "take" stays for a pick-up clip later.
            slots[str(slot)] = {
                "look": "new", "held": w["held"],
                "subject": f"{SUBJECTS[f]}, holding {w['held']}",
                "guard_pose": f"fighting stance facing right, holding {w['held']}",
                "air_pose": f"in mid-air facing right, knees tucked, holding {w['held']}",
                "clips": moves,
            }
        json.dump({"slots": slots}, open(os.path.join(S, "wkits", f"{f}.json"), "w"), indent=1)
        print(f, len(d["skills"]), "skills,", sum(len(w["moves"]) for w in d["weapons"].values()), "weapon moves")


if __name__ == "__main__":
    main()
