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


FX["move_wave_1"] = recipe(256, 200, 128, 160, 16, [
    ground_ring(1, 20, 110, 0.0, 0.8, 4), ground_ring(2, 10, 80, 0.15, 1.0, 3),
    {"id": "glow", "type": "glow", "radius": A(30, 70), "squash": 0.3, "shape": "soft", "ramp": "glow",
     "dissolve": A(0, 1, "in", 0.4, 1)},
    streaks(1, 18, 70, 140, 0.0, 0.9), sparkles(1, 8, 80, 0.2, 1.0, -70),
])
FX["move_wave_2"] = recipe(240, 230, 120, 210, 14, [
    *spikes(1, [(0, 170), (-14, 120), (14, 125), (-28, 85), (28, 90), (-42, 55), (42, 60)], 16, 0.0, 1.0),
    {"id": "base", "type": "glow", "radius": A(20, 60), "squash": 0.35, "ramp": "glow", "dissolve": A(0, 1, "in", 0.3, 1)},
    debris(1, 14, 120, 0.05), sparkles(1, 6, 70, 0.3, 1.0, -110),
])
FX["move_wave_3"] = recipe(272, 210, 136, 170, 16, [
    ground_ring(1, 16, 120, 0.0, 0.7, 4, 0.26), ground_ring(2, 16, 100, 0.2, 0.85, 3, 0.26),
    ground_ring(3, 16, 80, 0.4, 1.0, 3, 0.26),
    {"id": "pillar", "type": "beam", "height": K((0, 0), (0.2, 150), (1, 160)), "width": A(18, 4), "taper": 0.2,
     "soft": 3, "top_fade": 0.6, "ramp": "glow", "dissolve": A(0, 1, "in", 0.5, 1)},
    sparkles(1, 14, 100, 0.1, 1.0, -70),
])
FX["move_wave_4"] = recipe(272, 230, 136, 190, 16, [
    {"id": "swirl1", "type": "arc", "profile": "taper", "radius": 90, "squash": 0.3, "start": A(0, 360, "linear"),
     "sweep": 220, "thickness": 6, "ramp": "glow", "dissolve": A(0, 1, "in", 0.5, 1)},
    {"id": "swirl2", "type": "arc", "profile": "taper", "y": -60, "radius": 100, "squash": 0.25,
     "start": A(180, 540, "linear"), "sweep": 200, "thickness": 5, "ramp": "glow", "life": [0.1, 1.0],
     "dissolve": A(0, 1, "in", 0.5, 1)},
    ground_ring(1, 20, 95, 0.0, 0.9, 3, 0.3), streaks(1, 20, 60, 160, 0.0, 0.95),
])
FX["move_wave_5"] = recipe(300, 250, 150, 206, 16, [
    *spikes(1, [(0, 200), (-10, 160), (10, 165), (-22, 130), (22, 135), (-36, 95), (36, 100), (-52, 60), (52, 64)],
            18, 0.0, 1.0),
    ground_ring(1, 30, 140, 0.0, 0.85, 5, 0.25),
    {"id": "flash", "type": "glow", "radius": K((0, 20), (0.15, 90), (0.5, 60), (1, 20)), "squash": 0.4,
     "ramp": "glow", "dissolve": A(0, 1, "in", 0.3, 0.9)},
    debris(1, 20, 150, 0.05), sparkles(1, 10, 110, 0.25, 1.0, -120),
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
                            json.dumps({"argv": [s0, "render", json.dumps({"recipe": FX[name], "zoom": 2})]}),
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


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    cmd, names = sys.argv[1], sys.argv[2:] or list(FX)
    steps_path = os.path.join(OUT, "steps.json")
    steps = json.load(open(steps_path)) if os.path.exists(steps_path) else {}
    if cmd == "export":
        import time, urllib.request
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
            print(n, base, files[:4])
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
    if cmd == "render":
        for n in names:
            step, warn, err = render(n)
            if step:
                steps[n] = step
            print(n, step, "|", " / ".join(warn)[:300], err[:600])
            json.dump(steps, open(steps_path, "w"), indent=1)
