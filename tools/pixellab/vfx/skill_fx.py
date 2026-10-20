#!/usr/bin/env python3
"""The effects of the fighters' skills (roster.cpp names them "<id>_<x|a|b|t>_<n|s|u|d|ult|aura|burst>"), built
with PixelLab's vfx_workbench in each fighter's colours:

    python3 skill_fx.py render [names...]   -> move/<name>/sheet.png (looked at before export)
    python3 skill_fx.py export [names...]   -> move/<name>.png + .json
    python3 skill_fx.py import [names...]   -> assets/sprites/fx_<name>.png + its line (anchor, loop, hit radius)

Conventions of the game: a projectile is drawn facing right on its centre, sized so that `hit` frame pixels are its
hitbox radius; a cast effect (castFx) is drawn on the fighter's feet for a fighter 170 px tall; a pillar stands on
its bottom; an aura loops behind the fighter, on its feet.
"""
import json
import os
import subprocess
import sys

import move_fx as M
from move_fx import A, K, recipe

S = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(S)))

# Each fighter's colours: main (dark -> light) and glow (light -> white).
COLOURS = {
    "p01": {"main": ["#140a20", "#3a1a5c", "#6a34a8", "#9a62e0", "#d0b0ff"], "glow": ["#8a5ad8", "#c8a8ff", "#f0e6ff"]},
    "p02": {"main": ["#0e2a4a", "#24589a", "#4a90d8", "#8cc8f4", "#dcf2ff"], "glow": ["#9fd8ff", "#e0f6ff", "#ffffff"]},
    "p03": {"main": ["#3a0806", "#8a1a0a", "#d8401a", "#ff8a2a", "#ffd890"], "glow": ["#ffb040", "#ffe8a0", "#ffffff"]},
    "p07": {"main": ["#300408", "#760c16", "#c41c2c", "#ff5a5a", "#ffd0d0"], "glow": ["#ff8080", "#ffd8d8", "#ffffff"]},
    "p08": {"main": ["#5a4a20", "#a08a40", "#e0c870", "#fff0b0", "#ffffff"], "glow": ["#ffe9a0", "#fff8e0", "#ffffff"]},
}
GLOWY = {"p01": False, "p02": True, "p03": True, "p07": True, "p08": True}

FX = {}      # name -> recipe
META = {}    # name -> import options (anchor from the recipe origin, loop, hit radius)


def add(name, rec, hit=0.0, loop=False):
    FX[name] = rec
    META[name] = {"hit": hit, "loop": loop}


def R(f, w, h, ox, oy, frames, layers, loop=False, fps=15):
    return recipe(w, h, ox, oy, frames, layers, loop=loop, fps=fps, ramps=COLOURS[f])


def trail(n=10, speed=90, size=(1, 3), shape="dot", y=0, spread=30):
    return {"id": "trail", "type": "particles", "mode": "travel", "direction": 180, "spread": spread, "speed": speed * 0.6,
            "count": n, "size": list(size), "shape": shape, "lifespan": 0.5, "y": y, "ramp": "glow", "fade_out": 0.6}


# --- projectiles (loops, facing right, on their centre) ---
def orb(f, name, r):
    add(name, R(f, int(r * 5), int(r * 3.2), int(r * 3.2), int(r * 1.6), 8, [
        {"id": "halo", "type": "glow", "radius": {"wave": {"mid": r * 1.5, "amp": 2, "cycles": 1}}, "ramp": "main",
         "intensity": 0.75},
        {"id": "core", "type": "glow", "radius": {"wave": {"mid": r * 0.95, "amp": 1.5, "cycles": 2}}, "shape": "hard",
         "ramp": "glow" if GLOWY[f] else "main", "intensity": 1.0 if GLOWY[f] else 0.15, "z": "front"},
        {"id": "rim", "type": "ring", "radius": r * 1.0, "width": 1.2, "soft": 2, "ramp": "glow", "z": "front"},
        {"id": "swirl", "type": "spiral", "radius": r * 1.3, "arms": 3, "turns": 0.8, "width": 1.5, "spin": A(0, 120, "linear"),
         "ramp": "main"},
        {"id": "motes", "type": "particles", "mode": "orbit", "radius": r * 1.25, "count": 8, "size": [1, 3],
         "shape": "sparkle", "spacing": "even", "ramp": "glow"},
        trail(10, 70, (2, 4), "dot"),
    ], loop=True), hit=r)


def fireball(f, name, r, big=False):
    add(name, R(f, int(r * 5), int(r * 3.4), int(r * 3.4), int(r * 1.7), 8, [
        {"id": "body", "type": "noise", "radius": r * 1.05, "scale": max(3, r / 4), "erosion": 0.35,
         "scroll": [-r * 2, 0], "ramp": "main"},
        {"id": "core", "type": "glow", "radius": {"wave": {"mid": r * 0.7, "amp": 1.5, "cycles": 2}}, "ramp": "glow"},
        {"id": "tail", "type": "puffs", "mode": "column", "direction": 180, "height": r * 2.2, "radius": r * 0.55,
         "count": 6, "x": -r * 0.6, "ramp": "main"},
        trail(14 if big else 10, 90, (2, 4), "dot", spread=40),
    ], loop=True), hit=r)


def shard(f, name, r, points=None):
    pts = points or [[r * 1.8, 0], [r * 0.2, -r * 0.7], [-r * 1.2, -r * 0.25], [-r * 1.5, 0], [-r * 1.2, r * 0.25],
                     [r * 0.2, r * 0.7]]
    add(name, R(f, int(r * 5.5), int(r * 2.8), int(r * 3), int(r * 1.4), 8, [
        {"id": "glow", "type": "glow", "radius": r * 1.3, "squash": 0.6, "ramp": "main", "intensity": 0.6},
        {"id": "body", "type": "shape", "points": pts, "fill": "edge", "bevel": 2, "ramp": "main"},
        {"id": "edge", "type": "shape", "closed": False, "points": [[-r * 1.3, 0], [r * 1.7, 0]], "width": 1.2,
         "ramp": "glow", "z": "front"},
        trail(10, 100, (3, 7), "streak", spread=10),
    ], loop=True), hit=r)


def star(f, name, r):
    add(name, R(f, int(r * 5), int(r * 3.2), int(r * 3), int(r * 1.6), 8, [
        {"id": "glow", "type": "glow", "radius": {"wave": {"mid": r * 1.4, "amp": 2, "cycles": 1}}, "ramp": "main",
         "intensity": 0.7},
        {"id": "star", "type": "star", "radius": r * 1.05, "points": 5, "step": 2, "rotation": A(0, 72, "linear"),
         "width": 1.6, "soft": 2, "ramp": "glow"},
        {"id": "core", "type": "glow", "radius": r * 0.5, "shape": "hard", "ramp": "glow"},
        {"id": "glints", "type": "particles", "mode": "twinkle", "radius": r * 1.6, "count": 5, "size": [2, 3],
         "shape": "sparkle", "blink": 2, "ramp": "glow"},
        trail(10, 80, (2, 3), "sparkle"),
    ], loop=True), hit=r)


def spear(f, name, r):
    L = r * 5.0
    pts = [[L * 0.55, 0], [L * 0.32, -r * 0.55], [L * 0.3, -r * 0.18], [-L * 0.45, -r * 0.18], [-L * 0.5, 0],
           [-L * 0.45, r * 0.18], [L * 0.3, r * 0.18], [L * 0.32, r * 0.55]]
    add(name, R(f, int(L * 1.6), int(r * 3), int(L * 0.95), int(r * 1.5), 8, [
        {"id": "glow", "type": "glow", "radius": L * 0.5, "squash": 0.22, "ramp": "main", "intensity": 0.6},
        {"id": "body", "type": "shape", "points": pts, "fill": "edge", "bevel": 2, "ramp": "main"},
        {"id": "edge", "type": "shape", "closed": False, "points": [[-L * 0.45, 0], [L * 0.52, 0]], "width": 1.2,
         "ramp": "glow", "z": "front"},
        trail(12, 140, (6, 12), "streak", spread=6),
    ], loop=True), hit=r)


def crescent(f, name, r, ground=False):
    rad = r * 1.7
    add(name, R(f, int(r * 6), int(r * 4.4), int(r * 2.6), int(r * 2.2), 8, [
        {"id": "glow", "type": "glow", "x": r * 0.6, "radius": r * 1.6, "squash": 1.3 if ground else 1.0, "ramp": "main",
         "intensity": 0.55},
        {"id": "moon", "type": "arc", "profile": "crescent", "radius": rad, "start": -75, "sweep": 150,
         "thickness": {"wave": {"mid": r * 0.75, "amp": 1, "cycles": 1}}, "squash": 1.25 if ground else 1.0,
         "ramp": "main"},
        {"id": "rim", "type": "arc", "profile": "crescent", "radius": rad + 1, "start": -70, "sweep": 140,
         "thickness": r * 0.28, "squash": 1.25 if ground else 1.0, "ramp": "glow", "z": "front"},
        trail(12, 110, (4, 9), "streak", spread=50),
    ], loop=True), hit=r)


def spin_blade(f, name, r):
    pts = [[r * 1.6, 0], [r * 0.3, -r * 0.45], [-r * 0.6, -r * 0.2], [-r * 1.2, -r * 0.35], [-r * 1.2, r * 0.35],
           [-r * 0.6, r * 0.2], [r * 0.3, r * 0.45]]
    add(name, R(f, int(r * 5), int(r * 4), int(r * 2.8), int(r * 2), 8, [
        {"id": "glow", "type": "glow", "radius": r * 1.5, "ramp": "main", "intensity": 0.55},
        {"id": "whirl", "type": "arc", "profile": "taper", "radius": r * 1.5, "start": A(0, 360, "linear"), "sweep": 220,
         "thickness": 2, "ramp": "glow"},
        {"id": "blade", "type": "shape", "points": pts, "fill": "edge", "bevel": 2, "rotation": A(0, 360, "linear"),
         "ramp": "main", "z": "front"},
        trail(8, 80, (2, 3), "sparkle"),
    ], loop=True), hit=r)


def bolt(f, name, r):
    add(name, R(f, int(r * 9), int(r * 3.6), int(r * 5), int(r * 1.8), 8, [
        {"id": "glow", "type": "glow", "radius": r * 2.2, "squash": 0.35, "ramp": "main", "intensity": 0.6},
        {"id": "bolt", "type": "bolt", "start": [-r * 3.2, 0], "end": [r * 3.2, 0], "segments": 10, "jag": 0.25,
         "branches": 3, "width": 1.6, "flicker": 2, "ramp": "glow"},
        {"id": "bolt2", "type": "bolt", "start": [-r * 2.4, 0], "end": [r * 3.0, 0], "segments": 8, "jag": 0.3,
         "branches": 1, "width": 1.0, "flicker": 3, "seed": 7, "ramp": "main"},
        {"id": "head", "type": "glow", "x": r * 3, "radius": r * 0.8, "shape": "hard", "ramp": "glow"},
        trail(8, 120, (3, 7), "streak", spread=20),
    ], loop=True), hit=r)


def feather(f, name, r):
    L = r * 3.2
    pts = [[L * 0.5, 0], [L * 0.25, -r * 0.6], [-L * 0.2, -r * 0.5], [-L * 0.5, -r * 0.1], [-L * 0.5, r * 0.1],
           [-L * 0.2, r * 0.5], [L * 0.25, r * 0.6]]
    add(name, R(f, int(L * 1.9), int(r * 3.4), int(L * 1.1), int(r * 1.7), 8, [
        {"id": "glow", "type": "glow", "radius": L * 0.6, "squash": 0.5, "ramp": "main", "intensity": 0.5},
        {"id": "vane", "type": "shape", "points": pts, "smooth": 0.6, "fill": "edge", "bevel": 2,
         "rotation": {"wave": {"mid": 0, "amp": 8, "cycles": 1}}, "ramp": "main"},
        {"id": "shaft", "type": "shape", "closed": False, "points": [[-L * 0.6, 0], [L * 0.5, 0]], "width": 1,
         "ramp": "glow", "z": "front"},
        trail(10, 70, (2, 3), "sparkle"),
    ], loop=True), hit=r)


def trap_sigil(f, name, r):
    add(name, R(f, int(r * 6), int(r * 3), int(r * 3), int(r * 1.5), 10, [
        {"id": "glow", "type": "glow", "radius": r * 2.2, "squash": 0.3, "ramp": "main", "intensity": 0.5},
        {"id": "ring", "type": "ring", "radius": r * 2.2, "width": 1.2, "squash": 0.3, "ramp": "glow"},
        {"id": "runes", "type": "glyphs", "radius": r * 1.7, "count": 8, "size": 4, "squash": 0.3,
         "rotation": A(0, 45, "linear"), "ramp": "glow"},
        {"id": "star", "type": "star", "radius": r * 1.4, "points": 5, "step": 2, "squash": 0.3,
         "rotation": A(0, -72, "linear"), "width": 1, "ramp": "main"},
        {"id": "wisps", "type": "particles", "mode": "rise", "radius": r * 1.6, "squash": 0.3, "speed": r * 2, "count": 6,
         "size": [1, 2], "shape": "dot", "ramp": "glow", "fade_out": 0.5},
    ], loop=True), hit=r)


# --- cast effects (on the fighter's feet) ---
def dash(f, name, kind="slash"):
    layers = [
        *[{"id": f"line{k}", "type": "shape", "closed": False, "width": w, "taper": 1.0,
           "points": [[0, dy * 0.3], [L * 0.5, dy * 0.8], [L, dy]], "smooth": 0.5, "fill": "edge",
           "ramp": "glow" if k == 0 else "main", "reveal": A(0, 1, "out", 0, 0.3), "dissolve": A(0, 1, "in", 0.4, 1)}
          for k, (L, dy, w) in enumerate([(230, -60, 6), (200, -82, 3), (205, -38, 3), (160, -100, 2), (170, -20, 2)])],
    ]
    if kind == "slash":
        layers.append({"id": "slash", "type": "arc", "profile": "slash", "x": 150, "y": -70, "radius": 60, "start": -60,
                       "sweep": 130, "thickness": 9, "taper": 10, "life": [0.1, 0.8], "ramp": "glow"})
    elif kind == "fire":
        layers.append({"id": "flames", "type": "flame", "x": 60, "width": 26, "height": 40, "tongues": 5, "spread": 1.6,
                       "life": [0.0, 0.8], "dissolve": A(0, 1, "in", 0.4, 1), "ramp": "main"})
        layers.append({"id": "embers", "type": "particles", "mode": "burst", "x": 60, "y": -30, "direction": -150,
                       "spread": 80, "speed": 90, "count": 14, "size": [1, 3], "shape": "dot", "cool": 0.5,
                       "ramp": "main"})
    elif kind == "lightning":
        layers.append({"id": "bolt", "type": "bolt", "start": [0, -60], "end": [230, -60], "segments": 12, "jag": 0.3,
                       "branches": 3, "width": 1.6, "flicker": 2, "life": [0.0, 0.7], "ramp": "glow"})
    elif kind == "frost":
        layers.append({"id": "frost", "type": "particles", "mode": "travel", "x": 40, "direction": 0, "spread": 10,
                       "speed": 160, "count": 16, "line": 20, "line_angle": 90, "y": -10, "size": [2, 4],
                       "shape": "chunk", "shrink": 0.5, "ramp": "glow"})
    elif kind == "wind":
        layers.append({"id": "gust", "type": "arc", "profile": "taper", "x": 120, "y": -60, "radius": 50, "start": 180,
                       "sweep": 300, "thickness": 3, "squash": 0.5, "life": [0.0, 0.8], "ramp": "glow"})
    add(name, R(f, 300, 150, 30, 136, 12, layers))


def rise(f, name, kind="spiral"):
    layers = [
        {"id": "helix", "type": "shape", "closed": False, "depth": "auto", "width": 2.5, "taper": 0.4, "fill": "edge",
         "points": {"curves": [{"x": "32*sin(tau*(2*s + 2*u))", "y": "-190*s", "z": "cos(tau*(2*s + 2*u))", "n": 48}]},
         "reveal": A(0, 1, "out", 0, 0.5), "dissolve": A(0, 1, "in", 0.5, 1), "ramp": "glow"},
        {"id": "helix2", "type": "shape", "closed": False, "depth": "auto", "width": 2, "taper": 0.4, "fill": "edge",
         "points": {"curves": [{"x": "32*sin(tau*(2*s + 2*u + 0.5))", "y": "-190*s",
                                "z": "cos(tau*(2*s + 2*u + 0.5))", "n": 48}]},
         "reveal": A(0, 1, "out", 0.05, 0.55), "dissolve": A(0, 1, "in", 0.5, 1), "ramp": "main"},
        {"id": "streaks", "type": "particles", "mode": "rise", "radius": 26, "squash": 0.4, "speed": 200, "count": 12,
         "size": [6, 14], "shape": "streak", "lifespan": 0.5, "ramp": "glow", "fade_out": 0.4},
        {"id": "ring", "type": "ring", "radius": A(10, 44), "width": A(2.5, 0.5), "squash": 0.28, "ramp": "main",
         "life": [0.0, 0.6], "dissolve": A(0, 1, "in", 0.4, 1)},
    ]
    if kind == "fire":
        layers.append({"id": "flame", "type": "flame", "width": 30, "height": 70, "tongues": 3, "spread": 0.8,
                       "life": [0.0, 0.7], "dissolve": A(0, 1, "in", 0.4, 1), "ramp": "main"})
    elif kind == "lightning":
        layers.append({"id": "bolt", "type": "bolt", "start": [0, 0], "end": [0, -200], "segments": 10, "jag": 0.25,
                       "branches": 2, "width": 1.6, "flicker": 2, "life": [0.0, 0.7], "ramp": "glow"})
    elif kind == "pillar":
        layers.append({"id": "pillar", "type": "beam", "width": K((0, 6), (0.2, 34), (1, 10)), "height": K((0, 0), (0.25, 120), (1, 110)),
                       "taper": 0.6, "soft": 2, "ramp": "main", "dissolve": A(0, 1, "in", 0.5, 1)})
    elif kind == "beam":
        layers.append({"id": "beam", "type": "beam", "width": K((0, 4), (0.2, 26), (1, 6)), "height": 210, "taper": 0.1,
                       "soft": 3, "top_fade": 0.4, "ramp": "glow", "dissolve": A(0, 1, "in", 0.5, 1)})
    add(name, R(f, 150, 240, 75, 224, 12, layers))


def wings(f, name, phoenix=False):
    def wing(sign):
        up = [[0, 0], [sign * 40, -60], [sign * 90, -70], [sign * 110, -40], [sign * 80, -30], [sign * 95, -10],
              [sign * 60, -8], [sign * 70, 10], [sign * 30, 4]]
        down = [[0, 0], [sign * 40, -30], [sign * 90, -10], [sign * 110, 20], [sign * 80, 15], [sign * 90, 35],
                [sign * 55, 25], [sign * 60, 45], [sign * 25, 15]]
        return {"id": f"wing{'l' if sign < 0 else 'r'}", "type": "shape", "y": -120,
                "points": {"keys": [[0, up], [0.5, down], [1, up]], "ease": "in_out"}, "smooth": 0.4,
                "fill": "fire" if phoenix else "edge", "flow": [0, -40], "reach": 10, "bevel": 3, "ramp": "main",
                "scale": K((0, 0.2), (0.25, 1.0), (1, 1.0)), "dissolve": A(0, 1, "in", 0.6, 1)}
    add(name, R(f, 260, 240, 130, 226, 14, [
        wing(-1), wing(1),
        {"id": "feathers", "type": "particles", "mode": "fall", "y": -120, "radius": 90, "speed": 60, "count": 10,
         "size": [2, 3], "shape": "dot" if phoenix else "sparkle", "ramp": "glow", "fade_out": 0.5},
        {"id": "glow", "type": "glow", "y": -120, "radius": K((0, 6), (0.2, 24), (1, 14)), "ramp": "glow",
         "dissolve": A(0, 1, "in", 0.5, 1)},
    ]))


def blink(f, name):
    add(name, R(f, 160, 220, 80, 204, 12, [
        {"id": "ring", "type": "ring", "radius": K((0, 6), (0.25, 44), (1, 48), ease="out"), "width": 1.6, "soft": 2,
         "squash": 0.26, "ramp": "main", "dissolve": A(0, 1, "in", 0.5, 1)},
        *[{"id": f"line{k}", "type": "beam", "x": dx, "height": K((0, 0), (0.3, hgt), (1, hgt)), "width": w, "taper": 0.6,
           "soft": 1, "top_fade": 0.5, "ramp": "glow", "life": [lt, 1.0], "dissolve": A(0, 1, "in", 0.4, 1)}
          for k, (dx, hgt, w, lt) in enumerate([(0, 170, 4, 0.0), (-18, 120, 2, 0.05), (18, 130, 2, 0.08),
                                                (-34, 80, 1, 0.12), (34, 86, 1, 0.1)])],
        {"id": "motes", "type": "particles", "mode": "burst", "y": -80, "count": 18, "speed": 70, "size": [2, 4],
         "shape": "sparkle", "lifespan": 0.7, "ramp": "glow", "fade_out": 0.4},
    ]))


def shield(f, name, shape="hex"):
    """A guard in front of the fighter (counters, mirrors, barriers), its centre at chest height."""
    cx, cy = (44, -90) if shape != "dome" else (0, -85)
    layers = [{"id": "glow", "type": "glow", "x": cx, "y": cy, "radius": K((0, 10), (0.2, 50), (1, 40)), "ramp": "main",
               "intensity": 0.55, "dissolve": A(0, 1, "in", 0.6, 1)}]
    if shape == "hex":
        layers += [{"id": "hex", "type": "star", "x": cx, "y": cy, "points": 6, "step": 1, "radius": K((0, 8), (0.2, 44), (1, 46)),
                    "squash": 1.6, "rotation": 30, "width": 1.6, "soft": 2, "ramp": "glow", "dissolve": A(0, 1, "in", 0.6, 1)},
                   {"id": "hex2", "type": "star", "x": cx, "y": cy, "points": 6, "step": 1, "radius": K((0, 6), (0.2, 32), (1, 34)),
                    "squash": 1.6, "rotation": 30, "width": 1, "ramp": "main", "dissolve": A(0, 1, "in", 0.6, 1)}]
    elif shape == "mirror":
        layers += [{"id": "pane", "type": "shape", "x": cx, "y": cy,
                    "points": [[-8, -60], [8, -50], [10, 50], [-6, 60], [-10, 0]], "fill": "edge", "bevel": 3,
                    "scale": K((0, 0.1), (0.2, 1.0), (1, 1.0)), "ramp": "main", "dissolve": A(0, 1, "in", 0.6, 1)},
                   {"id": "shine", "type": "shape", "closed": False, "x": cx, "y": cy, "points": [[-2, -40], [2, 30]],
                    "width": 2, "ramp": "glow", "z": "front", "dissolve": A(0, 1, "in", 0.6, 1)}]
    else:  # dome / barrier: a sphere round the body
        layers += [{"id": "dome", "type": "ring", "x": cx, "y": cy, "radius": K((0, 10), (0.2, 70), (1, 74)), "width": 2,
                    "soft": 3, "squash": 1.15, "ramp": "glow", "dissolve": A(0, 1, "in", 0.6, 1)},
                   {"id": "dome2", "type": "ring", "x": cx, "y": cy, "radius": K((0, 8), (0.2, 62), (1, 66)), "width": 1,
                    "squash": 1.15, "ramp": "main", "dissolve": A(0, 1, "in", 0.6, 1)},
                   {"id": "hexes", "type": "star", "x": cx, "y": cy, "points": 6, "step": 1, "radius": K((0, 8), (0.2, 50), (1, 52)),
                    "rotation": A(0, 30), "width": 0.8, "ramp": "glow", "dissolve": A(0, 1, "in", 0.5, 1)}]
    layers.append({"id": "glints", "type": "particles", "mode": "twinkle", "x": cx, "y": cy, "radius": 50, "count": 6,
                   "size": [2, 4], "shape": "sparkle", "blink": 2, "ramp": "glow"})
    add(name, R(f, 220, 220, 100 if shape != "dome" else 110, 206, 14, layers))


def burst(f, name, r, kind="fire"):
    layers = [{"id": "ring", "type": "ring", "radius": A(10, r), "width": A(4, 1), "soft": 2, "squash": 0.3,
               "ramp": "main", "dissolve": A(0, 1, "in", 0.5, 1)},
              {"id": "flash", "type": "glow", "y": -30, "radius": K((0, 10), (0.15, r * 0.7), (1, 10)), "ramp": "glow",
               "life": [0, 0.5], "dissolve": A(0, 1, "in", 0.3, 1)}]
    if kind == "fire":
        layers += [{"id": f"flame{k}", "type": "flame", "x": dx, "width": 18, "height": hgt, "tongues": 3, "spread": 0.9,
                    "life": [0.03 * k, 0.85], "dissolve": A(0, 1, "in", 0.5, 1), "ramp": "main"}
                   for k, (dx, hgt) in enumerate([(0, 70), (-r * 0.45, 50), (r * 0.45, 52), (-r * 0.8, 34), (r * 0.8, 36)])]
        layers.append({"id": "embers", "type": "particles", "mode": "burst", "y": -20, "direction": -90, "spread": 160,
                       "speed": r * 1.4, "count": 20, "size": [1, 3], "shape": "dot", "cool": 0.5, "gravity": 30,
                       "ramp": "main"})
    elif kind == "lightning":
        layers += [{"id": f"bolt{k}", "type": "bolt", "start": [0, -50], "end": [ex, ey], "segments": 8, "jag": 0.3,
                    "branches": 1, "width": 1.4, "flicker": 2, "seed": k, "life": [0.0, 0.8], "ramp": "glow"}
                   for k, (ex, ey) in enumerate([(-r, -10), (r, -10), (-r * 0.6, -110), (r * 0.6, -110), (0, -140)])]
    elif kind == "quake":
        layers += [{"id": "cracks", "type": "bolt", "plane": "ground", "start": [-r, 0], "end": [r, 0], "segments": 12,
                    "jag": 0.25, "branches": 4, "width": 1.5, "progress": A(0, 1, "out", 0, 0.3), "ramp": "main"},
                   {"id": "rocks", "type": "particles", "mode": "burst", "direction": -90, "spread": 150, "speed": r * 1.3,
                    "count": 16, "size": [2, 5], "shape": "chunk", "shrink": 0, "gravity": 120, "lifespan": 0.8,
                    "floor": 4, "outline": True, "ramp": "main"},
                   {"id": "dust", "type": "puffs", "mode": "poof", "direction": -90, "spread": 3.0, "radius": 14, "count": 8,
                    "throw": 0.9, "rise": 0.3, "thin": 0.5, "squash": 0.6, "ramp": "main"}]
    add(name, R(f, int(r * 2.4 + 40), int(r * 1.6 + 60), int(r * 1.2 + 20), int(r * 1.6 + 40), 14, layers))


def pillar(f, name, kind="light"):
    """A column erupting from the ground (drawn on its bottom; hit radius 36 for a skill)."""
    if kind == "ice":
        layers = [*[{"id": f"spike{k}", "type": "beam", "x": dx, "angle": ang, "width": w, "taper": 1.0, "soft": 1,
                     "height": K((0, 0), (0.25, h), (1, h * 0.95), ease="out"), "ramp": "main" if k % 2 else "glow",
                     "life": [0.02 * k, 1.0], "dissolve": A(0, 1, "in", 0.6, 1)}
                    for k, (dx, ang, w, h) in enumerate([(0, 0, 24, 150), (-16, -14, 18, 100), (16, 14, 18, 110),
                                                         (-30, -26, 14, 64), (30, 26, 14, 70)])]]
    elif kind == "lightning":
        layers = [{"id": "bolt", "type": "bolt", "start": [0, -230], "end": [0, 0], "segments": 12, "jag": 0.2,
                   "branches": 3, "width": 2.2, "flicker": 2, "progress": A(0, 1, "out", 0, 0.2), "ramp": "glow"},
                  {"id": "bolt2", "type": "bolt", "start": [6, -230], "end": [0, 0], "segments": 10, "jag": 0.3,
                   "branches": 1, "width": 1.2, "flicker": 3, "seed": 9, "progress": A(0, 1, "out", 0, 0.2), "ramp": "main"}]
    elif kind == "fire":
        layers = [{"id": "column", "type": "noise", "shape": "column", "width": 46, "height": K((0, 10), (0.25, 210), (1, 200)),
                   "scale": 8, "erosion": 0.4, "scroll": [0, -60], "ramp": "main", "dissolve": A(0, 1, "in", 0.6, 1)},
                  {"id": "core", "type": "beam", "width": 14, "height": K((0, 0), (0.25, 190), (1, 180)), "taper": 0.2,
                   "soft": 3, "top_fade": 0.4, "ramp": "glow", "dissolve": A(0, 1, "in", 0.6, 1)}]
    else:  # light
        layers = [{"id": "beam", "type": "beam", "width": K((0, 6), (0.2, 44), (1, 30)), "height": 230, "taper": 0.0,
                   "soft": 4, "top_fade": 0.5, "ramp": "main", "dissolve": A(0, 1, "in", 0.6, 1)},
                  {"id": "core", "type": "beam", "width": K((0, 2), (0.2, 14), (1, 8)), "height": 230, "taper": 0.0,
                   "soft": 1, "top_fade": 0.4, "ramp": "glow", "z": "front", "dissolve": A(0, 1, "in", 0.6, 1)}]
    layers += [{"id": "base", "type": "ring", "radius": A(10, 46), "width": A(3, 1), "squash": 0.28, "ramp": "main",
                "dissolve": A(0, 1, "in", 0.5, 1)},
               {"id": "bits", "type": "particles", "mode": "burst", "direction": -90, "spread": 100, "speed": 110,
                "count": 12, "size": [2, 4], "shape": "chunk" if kind == "ice" else "sparkle", "gravity": 60,
                "lifespan": 0.7, "ramp": "glow"}]
    add(name, R(f, 130, 260, 65, 250, 14, layers), hit=36)


def strike(f, name, what="star", hit=54):
    """Falling from the sky onto its point (the ultimates' strikes), then a burst there."""
    if what == "star":
        body = {"id": "body", "type": "star", "y": A(-200, 0, "in", 0, 0.35), "radius": 18, "points": 5, "step": 2,
                "rotation": A(0, 144), "width": 2, "ramp": "glow", "life": [0, 0.4]}
    elif what == "bolt":
        body = {"id": "body", "type": "bolt", "start": [0, -260], "end": [0, 0], "segments": 12, "jag": 0.22,
                "branches": 3, "width": 2.4, "flicker": 2, "progress": A(0, 1, "out", 0, 0.2), "life": [0, 0.7],
                "ramp": "glow"}
    else:  # a spear or a sword of light pointing down
        L = 110 if what == "spear" else 150
        w = 10 if what == "spear" else 22
        pts = [[0, 0], [w, -L * 0.25], [w * 0.4, -L * 0.25], [w * 0.4, -L], [-w * 0.4, -L], [-w * 0.4, -L * 0.25],
               [-w, -L * 0.25]]
        if what == "sword":
            pts = [[0, 0], [w * 0.5, -L * 0.15], [w * 0.5, -L * 0.8], [w * 1.6, -L * 0.8], [w * 1.6, -L * 0.86],
                   [w * 0.3, -L * 0.86], [w * 0.3, -L], [-w * 0.3, -L], [-w * 0.3, -L * 0.86], [-w * 1.6, -L * 0.86],
                   [-w * 1.6, -L * 0.8], [-w * 0.5, -L * 0.8], [-w * 0.5, -L * 0.15]]
        body = {"id": "body", "type": "shape", "points": pts, "fill": "edge", "bevel": 3, "ramp": "main",
                "y": A(-160, 0, "in", 0, 0.3), "life": [0, 0.75], "dissolve": A(0, 1, "in", 0.4, 1)}
    add(name, R(f, 200, 300, 100, 260, 14, [
        body,
        {"id": "warn", "type": "ring", "radius": A(40, 20, "in", 0, 0.3), "width": 1.2, "squash": 0.3, "ramp": "main",
         "life": [0, 0.35]},
        {"id": "flash", "type": "glow", "radius": K((0, 4), (0.35, 6), (0.45, 60), (1, 10)), "ramp": "glow",
         "life": [0.3, 1.0], "dissolve": A(0, 1, "in", 0.4, 1)},
        {"id": "ring", "type": "ring", "radius": A(10, 80), "width": A(4, 1), "squash": 0.3, "ramp": "main",
         "life": [0.33, 1.0], "dissolve": A(0, 1, "in", 0.5, 1)},
        {"id": "bits", "type": "particles", "mode": "burst", "direction": -90, "spread": 160, "speed": 120, "count": 14,
         "size": [2, 4], "shape": "sparkle", "gravity": 60, "lifespan": 0.6, "life": [0.33, 1.0], "ramp": "glow"},
    ]), hit=hit)


def ult_slashes(f, name, cross_bolts=False):
    """A storm of cuts round the held opponent (in front of the fighter, at its chest height)."""
    import random
    rnd = random.Random(name)
    layers = []
    for k in range(9):
        st = 0.06 * k
        layers.append({"id": f"cut{k}", "type": "arc", "profile": "slash", "x": 60 + rnd.uniform(-25, 25),
                       "y": -80 + rnd.uniform(-30, 30), "radius": rnd.uniform(40, 70), "start": rnd.uniform(-180, 180),
                       "sweep": 140, "thickness": 7, "taper": 9, "life": [st, min(1.0, st + 0.3)],
                       "ramp": "glow" if k % 2 else "main"})
    layers.append({"id": "final", "type": "arc", "profile": "slash", "x": 60, "y": -80, "radius": 90, "start": -140,
                   "sweep": 200, "thickness": 12, "taper": 14, "life": [0.6, 1.0], "ramp": "glow"})
    if cross_bolts:
        layers.append({"id": "bolts", "type": "bolt", "start": [0, -80], "end": [140, -80], "segments": 12, "jag": 0.35,
                       "branches": 3, "width": 1.4, "flicker": 2, "life": [0.0, 0.7], "ramp": "glow"})
    layers.append({"id": "orb", "type": "glow", "x": 60, "y": -80, "radius": K((0, 10), (0.1, 70), (0.7, 60), (1, 10)),
                   "ramp": "main", "intensity": 0.45, "dissolve": A(0, 1, "in", 0.7, 1)})
    add(name, R(f, 260, 240, 70, 220, 20, layers))


def ult_storm(f, name):
    add(name, R(f, 300, 220, 150, 200, 20, [
        {"id": "frost", "type": "ring", "radius": A(20, 140), "width": A(6, 2), "soft": 3, "squash": 0.26, "ramp": "main",
         "dissolve": A(0, 1, "in", 0.7, 1)},
        {"id": "floor", "type": "glow", "radius": A(20, 140), "squash": 0.26, "ramp": "glow", "intensity": 0.5,
         "dissolve": A(0, 1, "in", 0.6, 1)},
        *[{"id": f"crystal{k}", "type": "beam", "x": dx, "angle": ang, "width": w, "taper": 1.0, "soft": 1,
           "height": K((0, 0), (0.3, h), (1, h * 0.95), ease="out"), "ramp": "glow" if k % 2 else "main",
           "life": [0.1 + 0.03 * k, 1.0], "dissolve": A(0, 1, "in", 0.7, 1)}
          for k, (dx, ang, w, h) in enumerate([(-110, -20, 14, 60), (110, 20, 14, 64), (-70, -10, 18, 90), (70, 12, 18, 96),
                                               (-30, -4, 20, 120), (30, 5, 20, 126)])],
        {"id": "blizzard", "type": "particles", "mode": "orbit", "y": -60, "radius": 120, "squash": 0.35, "count": 30,
         "size": [2, 4], "shape": "sparkle", "vortex": 1.0, "ramp": "glow"},
    ]))


def comet(f, name):
    add(name, R(f, 240, 150, 160, 75, 10, [
        {"id": "body", "type": "noise", "radius": 46, "scale": 10, "erosion": 0.3, "scroll": [-90, 0], "ramp": "main"},
        {"id": "core", "type": "glow", "radius": {"wave": {"mid": 30, "amp": 3, "cycles": 2}}, "ramp": "glow"},
        {"id": "tail", "type": "puffs", "mode": "column", "direction": 180, "height": 140, "radius": 26, "count": 8,
         "x": -30, "ramp": "main"},
        trail(20, 130, (2, 5), "dot", spread=40),
    ], loop=True), hit=46)


def aura(f, name):
    add(name, R(f, 180, 240, 90, 226, 10, [
        {"id": "back", "type": "flame", "width": 46, "height": 150, "tongues": 5, "spread": 1.0, "taper": 0.5, "base": 0.9,
         "sway": 0.3, "flicker": 0.6, "cycles": 2, "ramp": "main", "intensity": 0.8},
        {"id": "inner", "type": "flame", "width": 30, "height": 110, "tongues": 3, "spread": 0.8, "base": 0.9,
         "sway": 0.25, "flicker": 0.5, "cycles": 2, "ramp": "glow", "intensity": 0.7},
        {"id": "sparks", "type": "particles", "mode": "rise", "radius": 40, "squash": 0.4, "speed": 160, "count": 10,
         "size": [5, 10], "shape": "streak", "lifespan": 0.6, "ramp": "glow", "fade_out": 0.4},
    ], loop=True))


def form_burst(f, name):
    add(name, R(f, 260, 260, 130, 240, 14, [
        {"id": "flash", "type": "glow", "y": -90, "radius": K((0, 20), (0.15, 110), (1, 20)), "ramp": "glow",
         "dissolve": A(0, 1, "in", 0.4, 1)},
        M.burst_rays(1, 20, 30, 120, 6, 0.0, 0.7),
        {"id": "rays_pos", "type": "glow", "y": -90, "radius": 1, "ramp": "glow", "intensity": 0.0},
        M.shock(1, 20, 120, 4, squash=0.3),
    ]))


# --- the five fighters (roster.cpp) ---
def build():
    # 01 · violet shadow
    orb("p01", "p01_x_n", 16); dash("p01", "p01_x_s", "slash"); blink("p01", "p01_x_u"); shield("p01", "p01_x_d")
    aura("p01", "p01_t_aura"); form_burst("p01", "p01_t_burst")
    crescent("p01", "p01_a_n", 18, ground=True); dash("p01", "p01_a_s", "slash"); rise("p01", "p01_a_u")
    shield("p01", "p01_a_d"); ult_slashes("p01", "p01_a_ult")
    spin_blade("p01", "p01_b_n", 10); dash("p01", "p01_b_s", "slash"); rise("p01", "p01_b_u")
    trap_sigil("p01", "p01_b_d", 12); ult_slashes("p01", "p01_b_ult")
    # 02 · stellar frost
    shard("p02", "p02_x_n", 12); dash("p02", "p02_x_s", "frost"); rise("p02", "p02_x_u", "pillar")
    shield("p02", "p02_x_d", "mirror"); aura("p02", "p02_t_aura"); form_burst("p02", "p02_t_burst")
    spear("p02", "p02_a_n", 12); dash("p02", "p02_a_s", "frost"); rise("p02", "p02_a_u"); pillar("p02", "p02_a_d", "ice")
    ult_storm("p02", "p02_a_ult")
    star("p02", "p02_b_n", 13); star("p02", "p02_b_s", 10); blink("p02", "p02_b_u"); shield("p02", "p02_b_d", "dome")
    strike("p02", "p02_b_ult", "star")
    # 03 · red blaze
    fireball("p03", "p03_x_n", 15); dash("p03", "p03_x_s", "fire"); rise("p03", "p03_x_u", "fire")
    burst("p03", "p03_x_d", 70, "fire"); aura("p03", "p03_t_aura"); form_burst("p03", "p03_t_burst")
    crescent("p03", "p03_a_n", 19, ground=True); dash("p03", "p03_a_s", "slash"); rise("p03", "p03_a_u")
    burst("p03", "p03_a_d", 80, "quake"); pillar("p03", "p03_a_ult", "fire")
    fireball("p03", "p03_b_n", 16, big=True); dash("p03", "p03_b_s", "fire"); wings("p03", "p03_b_u", phoenix=True)
    burst("p03", "p03_b_d", 80, "fire"); comet("p03", "p03_b_ult")
    # 07 · red lightning
    bolt("p07", "p07_x_n", 9); dash("p07", "p07_x_s", "lightning"); rise("p07", "p07_x_u", "lightning")
    burst("p07", "p07_x_d", 70, "lightning"); pillar("p07", "p07_x_ult", "lightning")
    crescent("p07", "p07_a_n", 14); dash("p07", "p07_a_s", "lightning"); rise("p07", "p07_a_u", "lightning")
    shield("p07", "p07_a_d"); ult_slashes("p07", "p07_a_ult", cross_bolts=True)
    spear("p07", "p07_b_n", 12); dash("p07", "p07_b_s", "lightning"); rise("p07", "p07_b_u", "lightning")
    pillar("p07", "p07_b_d", "lightning"); strike("p07", "p07_b_ult", "bolt")
    # 08 · white light
    feather("p08", "p08_x_n", 11); dash("p08", "p08_x_s", "wind"); wings("p08", "p08_x_u")
    shield("p08", "p08_x_d", "dome"); aura("p08", "p08_t_aura"); form_burst("p08", "p08_t_burst")
    crescent("p08", "p08_a_n", 15); dash("p08", "p08_a_s", "slash"); rise("p08", "p08_a_u")
    shield("p08", "p08_a_d"); pillar("p08", "p08_a_ult", "light")
    spear("p08", "p08_b_n", 12); dash("p08", "p08_b_s", "slash"); rise("p08", "p08_b_u", "beam")
    pillar("p08", "p08_b_d", "light"); strike("p08", "p08_b_ult", "spear")


def import_all(names):
    for n in names:
        src = os.path.join(M.OUT, f"{n}.png")
        if not os.path.exists(src):
            print(n, "not exported")
            continue
        cmd = ["python3", os.path.join(ROOT, "scripts", "import_fx.py"), n, src]
        if META[n]["loop"]:
            cmd.append("--loop")
        if META[n]["hit"]:
            cmd += ["--hit", str(META[n]["hit"])]
        r = subprocess.run(cmd, capture_output=True, text=True)
        print((r.stdout + r.stderr).strip().splitlines()[-1:])


if __name__ == "__main__":
    build()
    if sys.argv[1] == "import":
        import_all(sys.argv[2:] or list(FX))
    elif sys.argv[1] == "list":
        print(len(FX), sorted(FX))
    else:
        M.main(FX)
