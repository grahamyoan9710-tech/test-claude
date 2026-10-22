"""The shared movement effects (the owner's sheet docs/refonte/ref_effets.png), built with PixelLab's vfx_workbench
(free). Grey and white: the game tints them in each fighter's colour. Fighters are ~170 px tall in their clips;
ground effects have their origin on the feet.

    python3 move_fx.py render [names...]   -> OUT/<name>/sheet.png + step id in OUT/steps.json
    python3 move_fx.py export [names...]   -> OUT/<name>.png + .json (spritesheet from PixelLab)
"""
import json, os, subprocess, sys, glob

OUT = os.environ.get("MOVE_FX_OUT", os.path.join(os.path.dirname(os.path.abspath(__file__)), "move"))
PX = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "tools", "pxcall.py")
GREY = ["#3c3c46", "#6e6e7c", "#a6a6b4", "#d6d6e0", "#ffffff"]
GLOW = ["#8a8a98", "#c4c4d0", "#ececf4", "#ffffff"]


def shift(v, o):
    """A layer position written relative to the origin -> the canvas position vfx_workbench wants."""
    if isinstance(v, (int, float)):
        return v + o
    v = dict(v)
    if "from" in v:
        v["from"] += o
        v["to"] += o
    if "keys" in v:
        v["keys"] = [[u, x + o] for u, x in v["keys"]]
    if "wave" in v:
        v["wave"] = dict(v["wave"], mid=v["wave"].get("mid", 0) + o)
    return v


def recipe(w, h, ox, oy, frames, layers, loop=False, fps=15, ramps=None, outline="dark"):
    layers = [dict(l) for l in layers]
    for l in layers:
        if l.get("type") == "bolt":  # no position: its two ends, written relative to the origin
            l["start"] = [l["start"][0] + ox, l["start"][1] + oy]
            l["end"] = [l["end"][0] + ox, l["end"][1] + oy]
            continue
        l["x"] = shift(l.get("x", 0), ox)
        l["y"] = shift(l.get("y", 0), oy)
        if l.get("type") == "particles" and l.get("shape") == "chunk" and "floor" not in l:
            l["floor"] = h - oy - 4  # debris lands on the ground line, inside the frame
    r = {"canvas": {"w": w, "h": h}, "origin": [ox, oy], "frames": frames, "fps": fps, "loop": loop,
         "ramps": {"main": GREY, "glow": GLOW, **(ramps or {})}, "layers": layers, "outline": {"mode": outline},
         "shading": "interior", "shade_depth": 4}
    return r


def A(a, b, ease="out", start=0.0, end=1.0):
    return {"from": a, "to": b, "ease": ease, "start": start, "end": end}


def K(*keys, ease="linear"):
    return {"keys": [list(k) for k in keys], "ease": ease}


FX = {}

# --- Smokes (move / jump / dash) ---
def trail_puffs(direction, squash, n, step, radius, rise):
    out = []
    for k in range(n):
        out.append({"id": f"puff{k}", "type": "puffs", "mode": "poof", "x": -k * step, "y": 0, "direction": direction,
                    "spread": 0.6, "radius": radius * (1.0 - 0.18 * k), "count": 6, "throw": 0.5, "rise": rise,
                    "lump": 1.2, "billow": 0.9, "highlight": 0.7, "thin": 0.45, "squash": squash,
                    "life": [k * 0.12, 1.0]})
    return out


FX["move_smoke_run"] = recipe(220, 120, 186, 104, 13, [
    *trail_puffs(185, 0.85, 3, 34, 24, 0.35),
    {"id": "flecks", "type": "particles", "mode": "burst", "direction": 185, "spread": 40, "speed": 80, "count": 8,
     "size": [1, 3], "shape": "chunk", "gravity": 12, "lifespan": 0.7, "ramp": "main", "intensity": 0.7},
])
STROKES = [(190, 0, 6), (160, -9, 3), (165, 9, 3), (130, -16, 2), (135, 16, 2), (100, -22, 2), (105, 21, 2)]


def speed_strokes(scale, life_end):
    out = []
    for k, (L, dy, w) in enumerate(STROKES):
        out.append({"id": f"stroke{k}", "type": "shape", "closed": False, "width": w * scale ** 0.5, "taper": 1.0,
                    "points": [[0, dy * 0.15], [-L * scale * 0.5, dy * 0.7], [-L * scale, dy]], "smooth": 0.6,
                    "fill": "edge", "ramp": "main" if k else "glow", "reveal": A(0.0, 1.0, "out", 0, 0.35),
                    "dissolve": A(0, 1, "in", 0.4, life_end)})
    return out


FX["move_smoke_dash"] = recipe(230, 80, 212, 40, 10, [
    *speed_strokes(1.0, 1.0),
    {"id": "dust", "type": "puffs", "mode": "poof", "direction": 180, "spread": 0.4, "radius": 10, "count": 5,
     "throw": 0.8, "rise": 0.1, "thin": 0.6, "x": -60, "y": 24, "life": [0.1, 1.0]},
])
FX["move_smoke_jump"] = recipe(180, 180, 90, 164, 13, [
    {"id": "puff", "type": "puffs", "mode": "poof", "direction": -95, "spread": 1.5, "radius": 26, "count": 9,
     "throw": 0.65, "rise": 0.9, "lump": 1.1, "billow": 0.9, "highlight": 0.7, "thin": 0.4},
    {"id": "bits", "type": "particles", "mode": "burst", "direction": -90, "spread": 120, "speed": 80, "count": 10,
     "size": [1, 3], "shape": "chunk", "gravity": 50, "lifespan": 0.75},
])
FX["move_smoke_airjump"] = recipe(170, 130, 85, 112, 11, [
    {"id": "arc1", "type": "arc", "profile": "slash", "x": -10, "y": -10, "radius": 46, "start": 200, "sweep": 120,
     "thickness": 7, "taper": 10, "head": 0.4, "tail": 0.35, "ramp": "glow", "life": [0, 0.85]},
    {"id": "arc2", "type": "arc", "profile": "slash", "x": 22, "y": -34, "radius": 38, "start": 205, "sweep": 115,
     "thickness": 6, "taper": 9, "head": 0.4, "tail": 0.35, "ramp": "glow", "life": [0.12, 1.0]},
    {"id": "bits", "type": "particles", "mode": "burst", "direction": 90, "spread": 200, "speed": 40, "count": 9,
     "size": [1, 2], "shape": "chunk", "gravity": 30, "lifespan": 0.7, "life": [0.1, 1.0]},
])
FX["move_smoke_slide"] = recipe(250, 100, 216, 86, 13, [
    *trail_puffs(184, 0.7, 4, 36, 20, 0.15),
    {"id": "trail", "type": "particles", "mode": "travel", "direction": 182, "spread": 8, "speed": 140, "count": 8,
     "line": 6, "line_angle": 90, "size": [3, 8], "shape": "streak", "lifespan": 0.5, "y": 6},
])

# --- Shock waves (transformations) ---
def ground_ring(i, r0, r1, start, end, width=3, squash=0.28):
    return {"id": f"ring{i}", "type": "ring", "radius": A(r0, r1, "out", 0, 1), "width": A(width * 0.6, 0.5, "linear"),
            "soft": 2, "squash": squash, "life": [start, end], "dissolve": A(0, 1, "in", 0.5, 1), "ramp": "main",
            "intensity": A(1.0, 0.55, "in")}


def streaks(i, count, radius, speed, start=0.0, end=1.0, size=(8, 16)):
    return {"id": f"streaks{i}", "type": "particles", "mode": "rise", "radius": radius, "speed": speed, "count": count,
            "size": list(size), "shape": "streak", "lifespan": 0.6, "life": [start, end], "ramp": "glow",
            "squash": 0.3, "fade_out": 0.3}


def spikes(i, angles_heights, width, start, end, ramp="main"):
    out = []
    for k, (ang, hgt) in enumerate(angles_heights):
        if k == 0:
            out.append({"id": f"spike{i}_core", "type": "beam", "angle": ang, "height": K((0, 0), (0.25, hgt * 0.8), (1, hgt * 0.6)),
                        "width": K((0, width * 0.45), (1, 1)), "taper": 1.0, "soft": 1, "life": [start, end],
                        "dissolve": A(0, 1, "in", 0.5, 1), "ramp": "glow", "z": "front"})
        out.append({"id": f"spike{i}_{k}", "type": "beam", "angle": ang, "height": K((0, 0), (0.25, hgt), (1, hgt * 0.8)),
                    "width": K((0, width), (0.7, width * 0.8), (1, 1)), "taper": 1.0, "soft": 1, "top_fade": 0.2,
                    "life": [start, end], "dissolve": A(0, 1, "in", 0.55, 1), "ramp": ramp})
    return out


def debris(i, count, speed, start=0.0):
    return {"id": f"debris{i}", "type": "particles", "mode": "burst", "direction": -90, "spread": 150, "speed": speed,
            "count": count, "size": [2, 4], "shape": "chunk", "gravity": 90, "lifespan": 0.8, "life": [start, 1.0],
            "ramp": "main"}


def sparkles(i, count, radius, start=0.0, end=1.0, y=-60):
    return {"id": f"sparkles{i}", "type": "particles", "mode": "twinkle", "radius": radius, "count": count,
            "size": [2, 4], "shape": "sparkle", "blink": 2, "life": [start, end], "y": y, "ramp": "glow"}


# The owner's waves 1-5 are also the impact waves of powerful blows: they rise from the ground up.
TONES = {"dk": ["#24242c", "#3e3e4a", "#62626f", "#8c8c9c"], "md": ["#34343e", "#5e5e6c", "#9696a6", "#cacad6"],
         "lt": ["#7e7e8c", "#b8b8c6", "#e4e4ee", "#ffffff"], "star": ["#8a8a98", "#ececf4", "#ffffff"]}
LEAF = ["...111...", ".1233321.", "123444321", ".1233321.", "...111..."]


def tongue(i, angle, h, w, ramp="md", start=0.0, grow=0.26, z="back", bevel=4, curl=0.0):
    """A flame-like spike standing on the ground, turned `angle` degrees (clockwise), growing up from its root."""
    pts = [[-w * 0.5, 0], [-w * 0.36, -h * 0.3], [-w * 0.16 + curl * w, -h * 0.72], [curl * w * 1.8, -h],
           [w * 0.1 + curl * w, -h * 0.7], [w * 0.34, -h * 0.28], [w * 0.5, 0]]
    return {"id": f"tongue{i}", "type": "shape", "points": pts, "smooth": 0.45, "rotation": angle,
            "scale": K((0, 0.04), (grow, 1.06), (grow + 0.1, 1.0), (1, 0.95), ease="out"), "fill": "edge",
            "bevel": bevel, "ramp": ramp, "z": z, "life": [start, 1.0], "dissolve": A(0, 1, "in", 0.58, 1)}


def needle(i, x, h, w, start, ramp="lt", y=0):
    """A light streak drawn from the ground up."""
    return {"id": f"needle{i}", "type": "shape", "closed": False, "x": x, "y": y, "points": [[0, 0], [0, -h * 0.5], [0, -h]],
            "width": w, "taper": 1.0, "fill": "edge", "ramp": ramp, "reveal": A(0, 1, "out", 0, 0.45),
            "life": [start, 1.0], "dissolve": A(0, 1, "in", 0.5, 1)}


def crescent(i, a, b, tilt, y, sweep, turns, thick, ramp="md", start=0.0, phase=0.0, grow=0.3, z="auto"):
    """A shaded ribbon along a tilted ellipse, thickest in its middle, turning round: one outline going along the outer
    edge and back along the inner edge (q runs 0 -> 1 -> 0 along it)."""
    import math
    c, sn = math.cos(math.radians(tilt)), math.sin(math.radians(tilt))
    q = "(1-abs(1-2*s))"
    d = f"({thick}*sin(pi*{q})*(1-2*clamp(floor(2*s),0,1)))"
    th = f"(tau*({sweep}*{q}+{turns}*u+{phase}))"
    ex, ey = f"({a}+{d})*cos{th}", f"({b}+{b / a:.2f}*{d})*sin{th}"
    if abs(tilt) < 0.01:
        fx, fy = ex, ey
    else:
        fx, fy = f"{c:.3f}*{ex}-{sn:.3f}*{ey}", f"{sn:.3f}*{ex}+{c:.3f}*{ey}"
    layer = {"id": f"crescent{i}", "type": "shape", "closed": True, "y": y,
             "points": {"curves": [{"x": fx, "y": fy, "z": f"sin{th}", "n": 72}]},
             "scale": K((0, 0.25), (grow, 1.0), (1, 1.08), ease="out"), "fill": "edge", "bevel": 3, "ramp": ramp,
             "life": [start, 1.0], "dissolve": A(0, 1, "in", 0.55, 1)}
    if z == "auto":
        layer["depth"] = "auto"
    else:
        layer["z"] = z
    return layer


def leaves(i, count, speed, spread=150, start=0.0, x=0, y=-6, gravity=110, direction=-90):
    return {"id": f"leaves{i}", "type": "particles", "mode": "burst", "x": x, "y": y, "direction": direction,
            "spread": spread, "speed": speed, "count": count, "shape": "custom", "pixels": LEAF, "pixel_ramp": 4,
            "orient": True, "shrink": 0, "gravity": gravity, "lifespan": 0.8, "life": [start, 1.0], "fade_out": 0.2,
            "ramp": "md"}


def ground_glow(r0, r1, squash=0.26, intensity=0.55):
    return {"id": "groundglow", "type": "glow", "radius": K((0, r0), (0.2, r1), (1, r1 * 0.8), ease="out"),
            "squash": squash, "ramp": "lt", "intensity": intensity, "dissolve": A(0, 1, "in", 0.3, 1)}


def wave_ring(i, r1, start=0.0, width=2.5, squash=0.24, ramp="md"):
    return {"id": f"gring{i}", "type": "ring", "y": -2, "radius": A(8, r1), "width": A(width, 1), "soft": 1.5,
            "squash": squash, "ramp": ramp, "life": [start, 1.0], "dissolve": A(0, 1, "in", 0.5, 1)}


def wave(w, h, ox, oy, frames, layers):
    return recipe(w, h, ox, oy, frames, layers, ramps=TONES)


# Wave 1: circular energy. A thick ribbon round the feet and a smaller one above, light streaks rising between them.
FX["move_wave_1"] = wave(280, 230, 140, 200, 18, [
    ground_glow(14, 70),
    crescent(1, 108, 30, 0, -6, 0.8, 0.7, 9, "md"),
    crescent(2, 92, 25, 0, -2, 0.6, 0.7, 5, "lt", phase=0.25),
    crescent(3, 76, 20, 0, A(-30, -92, "out"), 0.75, -0.6, 6, "md", start=0.1, phase=0.5),
    wave_ring(1, 70, 0.0, 2, 0.27, "lt"),
    *[needle(k, dx, hgt, w, st) for k, (dx, hgt, w, st) in enumerate(
        [(0, 150, 4, 0.0), (-26, 120, 3, 0.04), (28, 128, 3, 0.06), (-52, 96, 3, 0.1), (54, 102, 3, 0.08),
         (-76, 70, 2, 0.14), (78, 74, 2, 0.12), (-12, 92, 2, 0.18), (14, 106, 2, 0.16)])],
    leaves(1, 6, 70, 70, 0.1, -104, -10, 70, -120), leaves(2, 6, 70, 70, 0.1, 104, -10, 70, -60),
])

# Wave 2: vertical burst. A sheaf of flame tongues shooting up, leaf shards flying out, a splash on the ground.
FX["move_wave_2"] = wave(260, 250, 130, 228, 16, [
    ground_glow(10, 54),
    {"id": "splash", "type": "rays", "y": -2, "count": 18, "inner": A(10, 34), "length": K((0, 6), (0.25, 36), (1, 26)),
     "width": 4, "jitter": 0.5, "squash": 0.24, "tip_fade": 0.6, "ramp": "dk", "life": [0.0, 0.75],
     "dissolve": A(0, 1, "in", 0.4, 1)},
    *[tongue(k, ang, hh, ww, rp, st, curl=cl) for k, (ang, hh, ww, rp, st, cl) in enumerate(
        [(-64, 60, 22, "dk", 0.06, -0.1), (64, 62, 22, "dk", 0.06, 0.1), (-46, 98, 28, "dk", 0.03, -0.08),
         (46, 102, 28, "dk", 0.03, 0.08), (-28, 138, 30, "md", 0.0, -0.05), (28, 142, 30, "md", 0.0, 0.05)])],
    tongue(10, -13, 150, 30, "md", 0.02, z="front", curl=-0.04), tongue(11, 13, 156, 30, "md", 0.02, z="front", curl=0.04),
    tongue(20, 0, 214, 40, "lt", 0.0, 0.3, z="front"),
    needle(1, -6, 150, 3, 0.05), needle(2, 8, 170, 3, 0.03),
    leaves(1, 12, 150, 150, 0.06, 0, -20, 130),
])

# Wave 3: multiple circles. Rings spreading on the ground, a wide double halo rising, a pillar, streaks and stars.
FX["move_wave_3"] = wave(320, 240, 160, 214, 18, [
    ground_glow(12, 80, 0.22),
    wave_ring(1, 72, 0.0, 3, 0.22, "lt"), wave_ring(2, 102, 0.08, 2.5, 0.22), wave_ring(3, 130, 0.16, 2, 0.22),
    crescent(1, 144, 18, 0, A(-24, -104, "out"), 1.0, 0.0, 3, "md", start=0.08, grow=0.35, z="back"),
    {"id": "halo2", "type": "ring", "y": A(-26, -110, "out"), "radius": A(36, 132, "out"), "width": 1.5, "soft": 1,
     "squash": 0.13, "ramp": "lt", "life": [0.12, 1.0], "dissolve": A(0, 1, "in", 0.55, 1)},
    {"id": "pillar", "type": "beam", "width": K((0, 4), (0.2, 22), (1, 6)), "height": K((0, 0), (0.28, 205), (1, 212)),
     "taper": 0.25, "soft": 3, "top_fade": 0.55, "ramp": "lt", "dissolve": A(0, 1, "in", 0.45, 1)},
    *[needle(k, dx, hgt, w, st) for k, (dx, hgt, w, st) in enumerate(
        [(-30, 150, 3, 0.04), (32, 142, 3, 0.06), (-60, 112, 2, 0.1), (62, 116, 2, 0.08), (-90, 82, 2, 0.14),
         (92, 86, 2, 0.12)])],
    {"id": "stars", "type": "particles", "mode": "twinkle", "y": -100, "radius": 130, "squash": 0.55, "count": 14,
     "size": [3, 6], "shape": "sparkle", "blink": 2, "ramp": "star", "life": [0.18, 1.0], "fade_out": 0.3},
])

# Wave 4: whirlwind energy. Two thick tilted ribbons turning round each other, streaks and debris swept round.
FX["move_wave_4"] = wave(320, 250, 160, 222, 18, [
    ground_glow(12, 74),
    crescent(1, 118, 34, -8, -12, 0.72, 0.9, 11, "md"),
    crescent(2, 100, 28, -8, -14, 0.5, 0.9, 5, "lt", phase=0.2),
    crescent(3, 100, 28, 10, A(-34, -120, "out"), 0.66, -0.8, 9, "md", start=0.06, phase=0.45),
    crescent(4, 86, 23, 10, A(-36, -124, "out"), 0.45, -0.8, 4, "lt", start=0.08, phase=0.6),
    *[needle(k, dx, hgt, w, st) for k, (dx, hgt, w, st) in enumerate(
        [(0, 170, 4, 0.0), (-22, 140, 3, 0.04), (24, 150, 3, 0.05), (-48, 112, 3, 0.08), (50, 118, 3, 0.07),
         (-72, 82, 2, 0.12), (74, 86, 2, 0.1)])],
    {"id": "debris", "type": "particles", "mode": "rise", "radius": 100, "squash": 0.3, "speed": 110, "count": 12,
     "shape": "custom", "pixels": LEAF, "pixel_ramp": 4, "orient": True, "shrink": 0, "vortex": 1.5, "lifespan": 0.7,
     "fade_out": 0.3, "ramp": "md"},
])

# Wave 5: powerful transformation. A wide crown of tongues bursting from a ring on the ground, shards thrown far.
CROWN = [(-80, 50, 24, "dk"), (-68, 82, 28, "md"), (-56, 70, 26, "dk"), (-44, 122, 32, "md"), (-32, 100, 30, "dk"),
         (-20, 166, 36, "md"), (-9, 130, 30, "dk"), (0, 232, 44, "lt"), (9, 136, 30, "dk"), (20, 172, 36, "md"),
         (32, 104, 30, "dk"), (44, 126, 32, "md"), (56, 72, 26, "dk"), (68, 86, 28, "md"), (80, 52, 24, "dk")]
FX["move_wave_5"] = wave(360, 270, 180, 246, 18, [
    ground_glow(18, 110, 0.22),
    {"id": "flash", "type": "glow", "y": -40, "radius": K((0, 10), (0.12, 80), (0.4, 40), (1, 10)), "squash": 0.7,
     "ramp": "lt", "intensity": 0.7, "life": [0.0, 0.5], "dissolve": A(0, 1, "in", 0.3, 1)},
    wave_ring(1, 160, 0.0, 4, 0.2),
    *[tongue(k, ang, hh, ww, rp, 0.02 * (abs(ang) // 20), z="front" if rp == "lt" else "back",
             curl=-0.06 if ang < 0 else 0.06 if ang > 0 else 0.0) for k, (ang, hh, ww, rp) in enumerate(CROWN)],
    *[needle(k, dx, hgt, 3, st) for k, (dx, hgt, st) in enumerate([(-10, 170, 0.04), (12, 190, 0.03), (-40, 120, 0.08),
                                                                  (42, 126, 0.07)])],
    leaves(1, 18, 190, 170, 0.06, 0, -24, 150),
])

# --- Other effects (jump, dash...) ---
FX["move_jump"] = recipe(130, 130, 65, 112, 10, [
    *spikes(1, [(0, 70), (-20, 50), (20, 52), (-40, 30), (40, 32)], 10, 0.0, 1.0),
    ground_ring(1, 8, 40, 0.0, 0.8, 2, 0.3),
])
FX["move_jump_air"] = recipe(130, 110, 65, 30, 11, [
    {"id": "puff1", "type": "puffs", "mode": "poof", "direction": 95, "spread": 1.2, "radius": 15, "count": 5,
     "throw": 0.9, "rise": 0.0, "lump": 1.0, "billow": 0.9, "highlight": 0.7, "thin": 0.5},
    {"id": "puff2", "type": "puffs", "mode": "poof", "x": -26, "y": 40, "direction": 110, "spread": 0.6, "radius": 10,
     "count": 3, "throw": 0.7, "rise": 0.0, "thin": 0.5, "life": [0.15, 1.0]},
])
FX["move_land"] = recipe(180, 120, 90, 108, 10, [
    *spikes(1, [(0, 60), (-16, 45), (16, 46), (-32, 30), (32, 31), (-50, 18), (50, 18)], 10, 0.0, 0.8),
    {"id": "dustl", "type": "puffs", "mode": "poof", "direction": 185, "spread": 0.4, "radius": 10, "count": 5,
     "throw": 0.8, "rise": 0.2, "thin": 0.5, "squash": 0.8},
    {"id": "dustr", "type": "puffs", "mode": "poof", "direction": -5, "spread": 0.4, "radius": 10, "count": 5,
     "throw": 0.8, "rise": 0.2, "thin": 0.5, "squash": 0.8},
    debris(1, 8, 70),
])
FX["move_dash"] = recipe(190, 70, 176, 35, 9, [*speed_strokes(0.75, 1.0)])
FX["move_dash_fast"] = recipe(280, 80, 266, 40, 10, [
    *speed_strokes(1.3, 1.0),
    {"id": "lines", "type": "particles", "mode": "travel", "direction": 180, "spread": 4, "speed": 200, "count": 12,
     "line": 36, "line_angle": 90, "size": [8, 16], "shape": "streak", "lifespan": 0.6, "ramp": "main", "x": -30},
])
FX["move_hover"] = recipe(150, 200, 75, 176, 12, [
    {"id": "ring", "type": "ring", "radius": {"wave": {"mid": 34, "amp": 4, "cycles": 1}}, "width": 1.2, "soft": 2,
     "squash": 0.26, "ramp": "main"},
    {"id": "ring2", "type": "ring", "radius": {"wave": {"mid": 22, "amp": 3, "cycles": 1, "phase": 0.5}}, "width": 1,
     "soft": 2, "squash": 0.26, "ramp": "glow"},
    {"id": "streaks", "type": "particles", "mode": "rise", "radius": 30, "speed": 90, "count": 8, "size": [8, 16],
     "shape": "streak", "lifespan": 0.6, "squash": 0.3, "ramp": "glow", "fade_out": 0.3},
    {"id": "sparkles", "type": "particles", "mode": "twinkle", "radius": 50, "count": 6, "size": [2, 4],
     "shape": "sparkle", "blink": 1, "y": -60, "ramp": "glow"},
], loop=True)
FX["move_appear"] = recipe(160, 200, 80, 176, 12, [
    {"id": "ring", "type": "ring", "radius": K((0, 6), (0.25, 42), (1, 46), ease="out"), "width": 1.5, "soft": 2,
     "squash": 0.26, "ramp": "main", "dissolve": A(0, 1, "in", 0.6, 1)},
    {"id": "ring_in", "type": "ring", "radius": K((0, 4), (0.25, 28), (1, 30), ease="out"), "width": 1, "soft": 1,
     "squash": 0.26, "ramp": "glow", "dissolve": A(0, 1, "in", 0.5, 1)},
    *[{"id": f"line{k}", "type": "beam", "x": dx, "height": K((0, 0), (0.3, hgt), (1, hgt)), "width": w, "taper": 0.6,
       "soft": 1, "top_fade": 0.5, "ramp": "glow", "life": [lt, 1.0], "dissolve": A(0, 1, "in", 0.5, 1)}
      for k, (dx, hgt, w, lt) in enumerate([(0, 150, 3, 0.0), (-16, 110, 2, 0.05), (16, 115, 2, 0.08),
                                            (-30, 70, 1, 0.12), (30, 75, 1, 0.1)])],
    {"id": "sparkles", "type": "particles", "mode": "twinkle", "radius": 46, "count": 7, "size": [2, 4],
     "shape": "sparkle", "blink": 2, "y": -70, "ramp": "glow", "life": [0.15, 1.0]},
])
FX["move_vanish"] = recipe(160, 200, 80, 176, 11, [
    {"id": "ring", "type": "ring", "radius": A(40, 24, "in"), "width": 1.2, "soft": 2, "squash": 0.26, "ramp": "main",
     "dissolve": A(0, 1, "in", 0.4, 1)},
    *[{"id": f"line{k}", "type": "beam", "x": dx, "y": A(0, -60, "in"), "height": A(hgt, hgt * 0.6), "width": w,
       "taper": 0.7, "soft": 1, "top_fade": 0.6, "ramp": "glow", "dissolve": A(0, 1, "in", 0.2, 1)}
      for k, (dx, hgt, w) in enumerate([(0, 90, 2), (-18, 60, 1), (18, 64, 1)])],
    {"id": "sparkles", "type": "particles", "mode": "rise", "radius": 30, "speed": 80, "count": 9, "size": [2, 3],
     "shape": "sparkle", "lifespan": 0.7, "y": -40, "ramp": "glow"},
])


# --- System effects (shields, KO, explosions, weapons, ultimates...): grey, the game tints them ---
def burst_rays(i, count, r0, r1, width, start=0.0, end=0.6, ramp="glow", spin=0.0):
    return {"id": f"rays{i}", "type": "rays", "count": count, "inner": A(r0 * 0.3, r0), "length": K((0, 4), (0.3, r1), (1, r1 * 0.6)),
            "width": width, "jitter": 0.45, "alternate": 0.45, "tip_fade": 0.4, "spin": spin, "ramp": ramp,
            "life": [start, end], "dissolve": A(0, 1, "in", 0.4, 1)}


def flash(i, r, end=0.35, y=0):
    return {"id": f"flash{i}", "type": "glow", "y": y, "radius": K((0, r * 0.3), (0.25, r), (1, r * 0.4)), "ramp": "glow",
            "life": [0.0, end], "dissolve": A(0, 1, "in", 0.3, 1)}


def shock(i, r0, r1, width=2.0, start=0.0, end=1.0, squash=1.0, ramp="main"):
    return {"id": f"shock{i}", "type": "ring", "radius": A(r0, r1), "width": A(width, 0.5), "soft": 2, "squash": squash,
            "ramp": ramp, "life": [start, end], "dissolve": A(0, 1, "in", 0.5, 1)}


def shards(i, count, speed, gravity=60, size=(2, 5), start=0.0):
    return {"id": f"shards{i}", "type": "particles", "mode": "burst", "count": count, "speed": speed, "spread": 360,
            "size": list(size), "shape": "chunk", "shrink": 0, "gravity": gravity, "lifespan": 0.8, "life": [start, 1.0],
            "fade_out": 0.3, "outline": True}


def glints(i, count, speed, start=0.0, size=(2, 4), ramp="glow"):
    return {"id": f"glints{i}", "type": "particles", "mode": "burst", "count": count, "speed": speed, "size": list(size),
            "shape": "sparkle", "lifespan": 0.7, "life": [start, 1.0], "fade_out": 0.3, "ramp": ramp}


HEX = {"id": "hex", "type": "star", "points": 6, "step": 1, "radius": A(14, 34), "rotation": A(0, 20), "width": 1.2,
       "soft": 2, "ramp": "glow", "dissolve": A(0, 1, "in", 0.4, 1)}
FX["sys_shield_hit"] = recipe(110, 110, 55, 55, 8, [HEX, shock(1, 8, 44, 2), glints(1, 6, 50)])
FX["sys_shield_break"] = recipe(220, 220, 110, 110, 14, [
    flash(1, 60), dict(HEX, radius=A(30, 44), dissolve=A(0, 1, "in", 0.1, 0.5), breakup="blocks"),
    shock(1, 20, 100, 3), shards(1, 26, 140, 70, (3, 6)), glints(1, 10, 110, 0.05)])
FX["sys_ko"] = recipe(256, 256, 128, 128, 16, [
    flash(1, 90, 0.4), burst_rays(1, 18, 30, 120, 6, 0.0, 0.7), shock(1, 20, 124, 4), shock(2, 10, 90, 2, 0.12),
    glints(1, 14, 140, 0.05, (3, 5))])
FX["sys_explosion"] = recipe(220, 220, 110, 120, 14, [
    flash(1, 50, 0.3),
    {"id": "boom", "type": "puffs", "mode": "burst", "radius": 26, "count": 9, "throw": 0.7, "rise": 0.5, "lump": 1.1,
     "billow": 0.9, "highlight": 0.7, "thin": 0.4},
    shock(1, 14, 96, 3, 0.0, 0.8), shards(1, 14, 120, 120, (2, 5), 0.05)])
FX["sys_weapon_pillar"] = recipe(90, 320, 45, 316, 12, [
    {"id": "beam", "type": "beam", "width": {"wave": {"mid": 16, "amp": 3, "cycles": 1}}, "height": 300, "taper": 0.0,
     "soft": 4, "top_fade": 0.7, "ramp": "glow"},
    {"id": "core", "type": "beam", "width": {"wave": {"mid": 5, "amp": 1, "cycles": 2}}, "height": 300, "taper": 0.0,
     "soft": 1, "top_fade": 0.5, "ramp": "glow", "z": "front"},
    {"id": "motes", "type": "particles", "mode": "fall", "radius": 10, "speed": 220, "count": 10, "size": [2, 3],
     "shape": "sparkle", "lifespan": 0.6, "y": -280, "ramp": "glow"},
    {"id": "base", "type": "ring", "radius": {"wave": {"mid": 20, "amp": 3, "cycles": 1}}, "width": 1.5, "soft": 2,
     "squash": 0.26, "ramp": "main"},
], loop=True, outline="none")
# The manga impact frame: jagged speed lines bursting from the blow (drawn in the screen's ink colour).
FX["sys_impact"] = recipe(256, 256, 128, 128, 4, [
    {"id": "lines", "type": "rays", "count": 56, "inner": 34, "length": 96, "width": 5, "jitter": 0.6, "alternate": 0.6,
     "tip_fade": 0.0, "spin": A(0, 12, "linear"), "ramp": "glow"},
    {"id": "spikes", "type": "rays", "count": 16, "inner": 10, "length": 44, "width": 9, "jitter": 0.4, "alternate": 0.5,
     "tip_fade": 0.0, "spin": A(5, 20, "linear"), "ramp": "glow"},
    {"id": "core", "type": "glow", "radius": 16, "shape": "hard", "ramp": "glow"},
], outline="none")
FX["sys_aura"] = recipe(180, 240, 90, 226, 10, [
    {"id": "back", "type": "flame", "width": 46, "height": 150, "tongues": 5, "spread": 1.0, "taper": 0.5, "base": 0.9,
     "sway": 0.3, "flicker": 0.6, "cycles": 2, "ramp": "main", "intensity": 0.8},
    {"id": "inner", "type": "flame", "width": 30, "height": 110, "tongues": 3, "spread": 0.8, "base": 0.9,
     "sway": 0.25, "flicker": 0.5, "cycles": 2, "ramp": "glow", "intensity": 0.7},
    {"id": "sparks", "type": "particles", "mode": "rise", "radius": 40, "squash": 0.4, "speed": 160, "count": 10,
     "size": [5, 10], "shape": "streak", "lifespan": 0.6, "ramp": "glow", "fade_out": 0.4},
], loop=True)
FX["sys_weapon_beacon"] = recipe(130, 170, 65, 150, 12, [
    {"id": "ring", "type": "ring", "radius": {"wave": {"mid": 34, "amp": 4, "cycles": 1}}, "width": 1.5, "soft": 2,
     "squash": 0.26, "ramp": "main"},
    {"id": "ring2", "type": "ring", "radius": {"wave": {"mid": 24, "amp": 3, "cycles": 1, "phase": 0.5}}, "width": 1,
     "soft": 2, "squash": 0.26, "ramp": "glow"},
    {"id": "glow", "type": "glow", "radius": 30, "squash": 0.3, "ramp": "glow", "intensity": 0.5},
    {"id": "rise", "type": "particles", "mode": "rise", "radius": 30, "squash": 0.3, "speed": 110, "count": 9,
     "size": [2, 3], "shape": "sparkle", "lifespan": 0.8, "ramp": "glow", "fade_out": 0.4},
    {"id": "streaks", "type": "particles", "mode": "rise", "radius": 26, "squash": 0.3, "speed": 120, "count": 5,
     "size": [6, 12], "shape": "streak", "lifespan": 0.6, "ramp": "glow", "fade_out": 0.4},
], loop=True)
FX["sys_weapon_land"] = recipe(160, 110, 80, 92, 10, [
    shock(1, 8, 64, 3, squash=0.26), shock(2, 4, 44, 2, 0.1, squash=0.26, ramp="glow"),
    {"id": "rise", "type": "particles", "mode": "rise", "radius": 40, "squash": 0.3, "speed": 70, "count": 10,
     "size": [2, 4], "shape": "sparkle", "lifespan": 0.7, "ramp": "glow", "fade_out": 0.3},
    {"id": "dust", "type": "puffs", "mode": "poof", "direction": -90, "spread": 2.8, "radius": 10, "count": 6,
     "throw": 0.9, "rise": 0.2, "thin": 0.5, "squash": 0.6}])
FX["sys_ult_ready"] = recipe(170, 220, 85, 200, 14, [
    {"id": "gather", "type": "particles", "mode": "converge", "y": -80, "radius": 80, "count": 16, "size": [2, 4],
     "shape": "sparkle", "lifespan": 0.6, "ramp": "glow"},
    shock(1, 50, 12, 2, 0.0, 0.7, 0.26),
    *[needle(30 + k, dx, hgt, 2, st, "lt") for k, (dx, hgt, st) in enumerate([(0, 150, 0.4), (-20, 110, 0.45), (22, 120, 0.42)])],
    flash(1, 50, 1.0, -80)], ramps=TONES)
FX["sys_final_smash"] = recipe(256, 256, 128, 128, 16, [
    flash(1, 110, 0.5), burst_rays(1, 24, 20, 126, 5, 0.0, 0.8), shock(1, 10, 120, 5), shock(2, 6, 90, 3, 0.15),
    glints(1, 18, 150, 0.0, (3, 6))])
FX["sys_orb_break"] = recipe(220, 220, 110, 110, 14, [
    flash(1, 70), burst_rays(1, 14, 20, 100, 5), shock(1, 14, 100, 3), shards(1, 20, 130, 50, (3, 6)), glints(1, 12, 120)])
FX["sys_orb_hit"] = recipe(110, 110, 55, 55, 7, [flash(1, 26, 0.6), shock(1, 6, 44, 2), glints(1, 6, 60)])
FX["sys_reflect"] = recipe(130, 130, 65, 65, 9, [dict(HEX, radius=A(20, 52)), dict(HEX, id="hex2", radius=A(10, 36),
                                                                                  rotation=A(30, 10)), flash(1, 30, 0.4)])
FX["sys_tech"] = recipe(120, 70, 60, 54, 8, [shock(1, 6, 50, 2, squash=0.3), glints(1, 6, 50)])
FX["sys_clank"] = recipe(100, 100, 50, 50, 7, [burst_rays(1, 8, 8, 44, 3, 0.0, 0.9), flash(1, 22, 0.5), glints(1, 6, 70)])
FX["sys_form_end"] = recipe(170, 220, 85, 200, 14, [
    shock(1, 20, 60, 2, 0.0, 0.8, 0.26),
    {"id": "rise", "type": "particles", "mode": "rise", "radius": 40, "squash": 0.4, "speed": 120, "count": 16,
     "size": [2, 4], "shape": "sparkle", "lifespan": 0.7, "ramp": "glow", "fade_out": 0.3, "y": -60}])


# Full ultimate meter, a smash charging: motes of light rising round the fighter (the game tints them in its colour).
FX["sys_ult_motes"] = recipe(120, 200, 60, 188, 16, [
    {"id": "motes", "type": "particles", "mode": "rise", "radius": 42, "squash": 0.3, "speed": 160, "count": 24,
     "size": [1, 3], "shape": "dot", "lifespan": 0.75, "ramp": "glow", "fade_out": 0.4},
    {"id": "glints", "type": "particles", "mode": "rise", "radius": 46, "squash": 0.35, "speed": 110, "count": 10,
     "size": [2, 4], "shape": "sparkle", "lifespan": 0.65, "ramp": "glow", "fade_out": 0.4, "y": -30}],
    loop=True, outline="none")


def call(argv):
    r = subprocess.run(["python3", PX, "vfx_workbench", json.dumps({"argv": argv})], capture_output=True, text=True,
                       timeout=300)
    return r.stdout + r.stderr


def step_of(text):
    for line in text.splitlines():
        if line.startswith("effect step:"):
            return line.split()[2]
    return None


def render(name):
    from PIL import Image
    out = os.path.join(OUT, name)
    os.makedirs(out, exist_ok=True)
    for f in glob.glob(os.path.join(out, "call_*.png")):
        os.remove(f)
    import time
    for attempt in range(6):
        s0 = step_of(call(["start", "--fps", "15"]))
        if not s0:
            time.sleep(5)
            continue
        r = subprocess.run(["python3", PX, "vfx_workbench",
                            json.dumps({"argv": [s0, "render", json.dumps({"recipe": FX[name], "zoom": 2, "fit_canvas": True})]}),
                            "--save", out], capture_output=True, text=True, timeout=300)
        text = r.stdout + r.stderr
        if "busy" not in text:
            break
        time.sleep(8)
    step = step_of(text)
    warn = [l for l in text.splitlines() if l.startswith(("warnings", "render", "error", "Error"))]
    ims = [Image.open(f).convert("RGBA") for f in sorted(glob.glob(os.path.join(out, "call_*.png")))]
    if ims:
        W = max(i.width for i in ims); H = sum(i.height for i in ims)
        sh = Image.new("RGBA", (W, H), (30, 30, 36, 255)); y = 0
        for i in ims:
            sh.alpha_composite(i, (0, y)); y += i.height
        sh.save(os.path.join(out, "sheet.png"))
    return step, warn, text if not step else ""


def export_all(names, steps):
    import time
    for n in names:
        text = ""
        for attempt in range(5):
            text = call([steps[n], "export", "--name", n])
            if "download" in text:
                break
            time.sleep(6)
        links = [w for w in text.split() if w.startswith("https://api.pixellab.ai/mcp/pixel-tools/")]
        base = links[0].rstrip(":/") if links else None
        files = [w.strip(",") for w in text.replace(":", " ").split() if w.endswith((".png", ".json", ".png,", ".json,"))]
        print(n, "exported" if base else "export failed")
        if not base:
            print(text[:800])
            continue
        for f in files:
            ext = f.rsplit(".", 1)[1].strip(",")
            if ext not in ("png", "json"):
                continue
            dst = os.path.join(OUT, f"{n}.{ext}")
            for attempt in range(4):
                r = subprocess.run(["curl", "-sS", "--retry", "4", "-o", dst, f"{base}/{f.strip(',')}"])
                if r.returncode == 0 and os.path.getsize(dst) > 0:
                    break


def main(fx=None):
    if fx:
        FX.update(fx)
    os.makedirs(OUT, exist_ok=True)
    cmd, names = sys.argv[1], sys.argv[2:] or list(fx or FX)
    steps_path = os.path.join(OUT, "steps.json")
    steps = json.load(open(steps_path)) if os.path.exists(steps_path) else {}
    if cmd == "render":
        for n in names:
            step, warn, err = render(n)
            if step:
                steps[n] = step
            print(n, step, "|", " / ".join(warn)[:300], err[:600])
            json.dump(steps, open(steps_path, "w"), indent=1)
    if cmd == "export":
        export_all(names, steps)


if __name__ == "__main__":
    main()
