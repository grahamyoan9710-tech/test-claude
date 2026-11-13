# Clash Horizon - the fighters' projectiles and techniques in Blender (yoyo, 12/11: every effect of every fighter
# made in Blender).
# One recipe per kind, coloured per fighter: orbe (an orb with its trail), meteore (a burning rock), lance (a javelin
# of energy), eclair (a bolt), etoile (a shooting star), dague (a spinning spectral dagger), plume (a feather of
# light), marque (a sigil left as a trap), pilier (a column out of the ground, played once).
# Flying ones loop on N frames (the game cycles them), flying right; the hit circle (RH px) is centred on the anchor.
# Cast effects (a technique's castFx, played once at the fighter's feet): see MOVES, e.g. p01-a_s.
# Usage: blender -b -P effets.py -- <kind>-<fighter> | <fighter>-<move> <out_dir> [samples]     e.g. orbe-p01
# Writes out_dir/fNN.exr (linear, premultiplied, transparent film; post2.py adds the glow) and prints its fx line.
import math
import os
import random
import sys

import bpy
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else sys.argv[1:]
RECIPE, OUT = argv[0], argv[1]
SAMPLES = int(argv[2]) if len(argv) > 2 else 24
FIRST, SECOND = RECIPE.split("-")
CAST = len(FIRST) == 3 and FIRST[0] == "p" and FIRST[1:].isdigit()  # p01-a_s: a technique's cast effect
STYLE = FIRST if CAST else SECOND
KIND = None if CAST else FIRST
PALETTES = {  # deep, main, hot, core, smoke (None: no smoke)
    "p01": ("#3B1772", "#8A4DFF", "#C9A2FF", "#F7EEFF", "#140A20"),
    "p02": ("#1E5C9E", "#5CC8F0", "#D8F6FF", "#FFFFFF", None),
    "p03": ("#B3170E", "#FF5A1F", "#FFC86E", "#FFF7E2", "#1E0F0B"),
    "p07": ("#7E0A18", "#FF2D3D", "#FF7F8C", "#FFF1F2", None),
    "p08": ("#C98F28", "#FFD45A", "#FFF3BF", "#FFFFFF", None),
    "garde": ("#1E5C9E", "#5CC8F0", "#D8F6FF", "#FFFFFF", None),  # the shield: the counter's dome, as it is
}
rng = random.Random(sum(map(ord, RECIPE)))


def lin(c):
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def rgba(h):
    h = h.lstrip("#")
    return (*(lin(int(h[i:i + 2], 16) / 255) for i in (0, 2, 4)), 1.0)


DEEP, MAIN, HOT, CORE = (rgba(c) for c in PALETTES[STYLE][:4])
SMOKE = rgba(PALETTES[STYLE][4]) if PALETTES[STYLE][4] else None
RAMP = [(0.0, DEEP), (0.35, MAIN), (0.7, HOT), (1.0, CORE)]
sc = None
CLOCK = []  # Value nodes holding the loop time (0..1)


def scene(W, H, AX, AY):
    global sc
    bpy.ops.wm.read_factory_settings(use_empty=True)
    sc = bpy.context.scene
    sc.render.engine = 'CYCLES'
    sc.cycles.device = 'CPU'
    sc.cycles.samples = SAMPLES
    sc.cycles.use_denoising = False
    sc.cycles.max_bounces = 0
    sc.cycles.transparent_max_bounces = 64
    sc.render.film_transparent = True
    sc.render.resolution_x, sc.render.resolution_y, sc.render.resolution_percentage = W, H, 100
    sc.view_settings.view_transform = 'Standard'
    sc.view_settings.look = 'None'
    ims = sc.render.image_settings
    ims.file_format = 'OPEN_EXR'
    ims.color_mode = 'RGBA'
    ims.color_depth = '16'
    ims.exr_codec = 'ZIP'
    world = bpy.data.worlds.new("Noir")
    sc.world = world
    try:
        world.use_nodes = True
    except Exception:
        pass
    bg = world.node_tree.nodes.get('Background') or world.node_tree.nodes.new('ShaderNodeBackground')
    bg.inputs['Strength'].default_value = 0.0
    cam = bpy.data.cameras.new("Camera")
    cam.type = 'ORTHO'
    cam.sensor_fit = 'VERTICAL'
    cam.ortho_scale = H / 100
    co = bpy.data.objects.new("Camera", cam)
    sc.collection.objects.link(co)
    co.location = ((W / 2 - AX) / 100, -10.0, (AY - H / 2) / 100)  # the anchor is the origin, 1 unit = 100 px
    co.rotation_euler = (math.pi / 2, 0.0, 0.0)  # looks along +y: x right, z up
    sc.camera = co


class Nodes:
    """A small helper to wire shader nodes."""

    def __init__(self, name):
        self.mat = bpy.data.materials.new(name)
        try:
            self.mat.use_nodes = True
        except Exception:
            pass
        self.t = self.mat.node_tree
        for n in list(self.t.nodes):
            self.t.nodes.remove(n)

    def new(self, kind, **props):
        n = self.t.nodes.new(kind)
        for k, v in props.items():
            setattr(n, k, v)
        return n

    def put(self, sock, v):
        if isinstance(v, (int, float, tuple)):
            sock.default_value = v
        else:
            self.t.links.new(v, sock)

    def math(self, op, a, b=0.0):
        n = self.new('ShaderNodeMath', operation=op)
        self.put(n.inputs[0], a)
        self.put(n.inputs[1], b)
        return n.outputs[0]

    def add(self, a, b):
        return self.math('ADD', a, b)

    def mul(self, a, b):
        return self.math('MULTIPLY', a, b)

    def ramp(self, x, stops):
        n = self.new('ShaderNodeValToRGB')
        els = n.color_ramp.elements
        els[0].position, els[0].color = stops[0]
        els[1].position, els[1].color = stops[-1]
        for p, c in stops[1:-1]:
            e = els.new(p)
            e.color = c
        self.put(n.inputs[0], x)
        return n.outputs[0]

    def smooth(self, x, a, b):  # 0 below a, 1 above b
        n = self.new('ShaderNodeMapRange', interpolation_type='SMOOTHSTEP', clamp=True)
        self.put(n.inputs[0], x)
        self.put(n.inputs[1], a)
        self.put(n.inputs[2], b)
        n.inputs[3].default_value, n.inputs[4].default_value = 0.0, 1.0
        return n.outputs[0]

    def uv(self):
        s = self.new('ShaderNodeSeparateXYZ')
        self.put(s.inputs[0], self.new('ShaderNodeTexCoord').outputs['UV'])
        return s.outputs[0], s.outputs[1]

    def knobs(self):
        """The object's colour, set per frame: strength, alpha, width; its alpha: the heat."""
        oi = self.new('ShaderNodeObjectInfo')
        s = self.new('ShaderNodeSeparateColor')
        self.put(s.inputs[0], oi.outputs['Color'])
        self.heat = oi.outputs['Alpha']
        return s.outputs[0], s.outputs[1], s.outputs[2]

    def clock(self):
        v = self.new('ShaderNodeValue')
        CLOCK.append(v)
        return v.outputs[0]

    def loop_noise(self, across, along, t, ka, kl, scale, detail=5.0, seed=0.0, rho=0.2):
        """Noise on a cylinder: it scrolls along `along` by kl periods per loop and repeats seamlessly."""
        a = self.mul(self.math('SUBTRACT', self.mul(along, kl), t), 2 * math.pi)
        c = self.new('ShaderNodeCombineXYZ')
        self.put(c.inputs[0], self.mul(self.math('COSINE', a), rho))
        self.put(c.inputs[1], self.mul(self.math('SINE', a), rho))
        self.put(c.inputs[2], self.add(self.mul(across, ka), seed))
        n = self.new('ShaderNodeTexNoise', noise_dimensions='3D')
        self.put(n.inputs['Vector'], c.outputs[0])
        n.inputs['Scale'].default_value = scale
        n.inputs['Detail'].default_value = detail
        n.inputs['Roughness'].default_value = 0.55
        return n.outputs['Fac']

    def surface(self, color, strength, alpha):
        e = self.new('ShaderNodeEmission')
        self.put(e.inputs['Color'], color)
        self.put(e.inputs['Strength'], strength)
        tr = self.new('ShaderNodeBsdfTransparent')
        mx = self.new('ShaderNodeMixShader')
        self.put(mx.inputs[0], alpha)
        self.t.links.new(tr.outputs[0], mx.inputs[1])
        self.t.links.new(e.outputs[0], mx.inputs[2])
        o = self.new('ShaderNodeOutputMaterial')
        self.t.links.new(mx.outputs[0], o.inputs['Surface'])
        return self.mat


# --- materials ---------------------------------------------------------------------------------------------
def m_glow():
    """A disc of light: white-hot in the middle (v: the radius, 0..1)."""
    nd = Nodes("Lueur")
    u, v = nd.uv()
    S, A, Wk = nd.knobs()
    x = nd.math('POWER', nd.math('SUBTRACT', 1.0, v), 1.4)
    return nd.surface(nd.ramp(x, RAMP), nd.mul(S, nd.add(0.4, nd.mul(x, 3.5))), nd.mul(A, nd.smooth(x, 0.0, 0.3)))


def m_dark():
    """A disc of dark smoke (a sigil's heart, a shadow's core)."""
    nd = Nodes("Ombre")
    u, v = nd.uv()
    S, A, Wk = nd.knobs()
    x = nd.math('SUBTRACT', 1.0, v)
    return nd.surface(nd.ramp(x, [(0.0, DEEP), (1.0, SMOKE or DEEP)]), nd.mul(S, 0.25), nd.mul(A, nd.smooth(x, 0.0, 0.5)))


def m_ring(noisy=False):
    """A band at the disc's rim, as wide as the width knob; noisy: it stirs around (the object turns)."""
    nd = Nodes("Anneau")
    u, v = nd.uv()
    S, A, Wk = nd.knobs()
    band = nd.mul(nd.smooth(v, nd.math('SUBTRACT', 1.0, Wk), nd.math('SUBTRACT', 1.0, nd.mul(Wk, 0.35))),
                  nd.math('SUBTRACT', 1.0, nd.smooth(v, 0.95, 1.0)))
    if noisy:
        a = nd.mul(u, 2 * math.pi)
        c = nd.new('ShaderNodeCombineXYZ')
        nd.put(c.inputs[0], nd.math('COSINE', a))
        nd.put(c.inputs[1], nd.math('SINE', a))
        nd.put(c.inputs[2], nd.mul(v, 2.0))
        n = nd.new('ShaderNodeTexNoise', noise_dimensions='3D')
        nd.put(n.inputs['Vector'], c.outputs[0])
        n.inputs['Scale'].default_value = 2.2
        n.inputs['Detail'].default_value = 4.0
        band = nd.mul(band, nd.add(0.35, nd.mul(nd.smooth(n.outputs['Fac'], 0.4, 0.7), 0.9)))
    return nd.surface(nd.ramp(band, RAMP), nd.mul(S, nd.add(0.4, nd.mul(band, 3.0))), nd.mul(A, band))


def m_flame(rho=0.2, ka=8.0, pale=False, smoky=True):
    """A ribbon of flame or energy streaming toward its end (u: along, 0 at its head; v: across); smoky: its end
    turns to the fighter's smoke (if it has one) instead of fading out."""
    smoke = SMOKE if smoky else None
    nd = Nodes("Flamme")
    u, v = nd.uv()
    S, A, Wk = nd.knobs()
    t = nd.clock()
    f1 = nd.loop_noise(v, u, t, ka, 1.0, 2.0, detail=5.0, seed=7.0, rho=rho)
    f2 = nd.loop_noise(v, u, t, ka * 2.2, 2.0, 2.0, detail=3.0, seed=19.0, rho=rho)
    fl = nd.add(nd.mul(f1, 0.7), nd.mul(f2, 0.3))
    mid = nd.math('SUBTRACT', 1.0, nd.math('ABSOLUTE', nd.math('SUBTRACT', nd.mul(v, 2.0), 1.0)))
    burn = nd.add(nd.add(0.14, nd.mul(u, 0.6)), nd.mul(nd.math('SUBTRACT', 1.0, mid), 0.32))
    a = nd.smooth(nd.math('SUBTRACT', fl, burn), -0.03, 0.08)
    heat = nd.mul(nd.mul(nd.math('POWER', nd.math('SUBTRACT', 1.0, u), 1.2), nd.add(0.35, nd.mul(fl, 0.9))),
                  nd.add(0.5, nd.mul(mid, 0.5)))
    stops = [(0.0, smoke or DEEP), (0.3, DEEP), (0.55, MAIN), (0.85, HOT), (1.0, CORE)]
    light = nd.add(nd.mul(nd.math('POWER', heat, 2.0), 2.5), 0.12 if smoke else 0.45)
    if not smoke:  # no smoke: the far end fades to nothing instead of darkening
        a = nd.mul(a, nd.math('SUBTRACT', 1.0, nd.smooth(u, 0.55, 1.0)))
    if pale:
        a = nd.mul(a, 0.55)
    return nd.surface(nd.ramp(heat, stops), nd.mul(S, light), nd.mul(A, a))


def m_solid(strength=5.0):
    """Flat light, its colour from the heat knob (sparks, arcs, lines)."""
    nd = Nodes("Plein")
    S, A, Wk = nd.knobs()
    return nd.surface(nd.ramp(nd.heat, RAMP), nd.mul(S, strength), A)


def m_shape(fill=0.75):
    """A filled shape (fan, v: 0 at its centre, 1 at its outline): bright edges, its middle as hot as the heat."""
    nd = Nodes("Forme")
    u, v = nd.uv()
    S, A, Wk = nd.knobs()
    edge = nd.smooth(v, 0.7, 0.97)
    mid = nd.mul(nd.math('POWER', nd.math('SUBTRACT', 1.0, v), 3.0), nd.heat)
    x = nd.add(0.3, nd.mul(nd.math('MAXIMUM', edge, mid), 0.7))
    return nd.surface(nd.ramp(x, RAMP), nd.mul(S, nd.add(0.5, nd.add(nd.mul(edge, 2.5), nd.mul(mid, 3.0)))),
                      nd.mul(A, nd.add(fill, nd.mul(edge, 1.0 - fill))))


def m_line():
    """A ribbon brightest along its middle (v: across), fading toward its end (u: along)."""
    nd = Nodes("Trait")
    u, v = nd.uv()
    S, A, Wk = nd.knobs()
    mid = nd.math('SUBTRACT', 1.0, nd.math('ABSOLUTE', nd.math('SUBTRACT', nd.mul(v, 2.0), 1.0)))
    along = nd.math('POWER', nd.math('SUBTRACT', 1.0, u), Wk)
    x = nd.mul(mid, nd.add(0.4, nd.mul(nd.heat, 0.6)))
    return nd.surface(nd.ramp(x, RAMP), nd.mul(S, nd.add(0.4, nd.mul(nd.math('POWER', mid, 2.0), 4.0))),
                      nd.mul(nd.mul(A, along), nd.smooth(mid, 0.0, 0.5)))


def m_beam(top=0.7):
    """A beam of light: soft edges, a white core, fading toward its end (u: from top on)."""
    nd = Nodes("Faisceau")
    u, v = nd.uv()
    S, A, Wk = nd.knobs()
    mid = nd.math('SUBTRACT', 1.0, nd.math('ABSOLUTE', nd.math('SUBTRACT', nd.mul(v, 2.0), 1.0)))
    t = nd.clock()
    ripple = nd.loop_noise(v, u, t, 3.0, 2.0, 2.0, detail=2.0, seed=3.0, rho=0.3)
    x = nd.mul(nd.math('POWER', mid, 0.8), nd.add(0.75, nd.mul(ripple, 0.4)))
    return nd.surface(nd.ramp(x, RAMP), nd.mul(S, nd.add(0.3, nd.mul(nd.math('POWER', x, 3.0), 4.0))),
                      nd.mul(nd.mul(A, nd.smooth(x, 0.05, 0.5)), nd.math('SUBTRACT', 1.0, nd.smooth(u, top, 1.0))))


def m_bubble():
    """A bubble like glass (a disc, v: the radius): clear in its middle, denser and brighter toward its rim."""
    nd = Nodes("Bulle")
    u, v = nd.uv()
    S, A, Wk = nd.knobs()
    x = nd.math('POWER', v, 4.0)
    rim = nd.math('SUBTRACT', 1.0, nd.smooth(v, 0.96, 1.0))
    return nd.surface(nd.ramp(nd.add(0.6, nd.mul(x, 0.4)), RAMP), nd.mul(S, nd.add(0.2, nd.mul(x, 2.2))),
                      nd.mul(nd.mul(A, nd.mul(x, 0.85)), rim))


# --- geometry ----------------------------------------------------------------------------------------------
def link(ob):
    sc.collection.objects.link(ob)
    return ob


def mesh(name, verts, faces, uvs, mat, at=(0.0, 0.0, 0.0)):
    me = bpy.data.meshes.new(name)
    me.from_pydata(verts, [], faces)
    uvl = me.uv_layers.new(name="UVMap")
    for poly in me.polygons:
        for li in poly.loop_indices:
            uvl.data[li].uv = uvs[me.loops[li].vertex_index]
    me.materials.append(mat)
    ob = link(bpy.data.objects.new(name, me))
    ob.location = at
    return ob


def knob(ob, s, a, w=1.0, heat=1.0):
    ob.color = (max(0.0, s), max(0.0, min(1.0, a)), max(0.0, min(1.0, w)), max(0.0, min(1.0, heat)))
    ob.hide_render = a <= 0.003


def disc(name, r, mat, y=0.0, at=(0.0, 0.0), rings=12, segs=72):
    verts, uvs, faces = [(0.0, y, 0.0)], [(0.0, 0.0)], []
    for i in range(1, rings + 1):
        for s in range(segs + 1):
            a = 2 * math.pi * s / segs
            verts.append((r * i / rings * math.cos(a), y, r * i / rings * math.sin(a)))
            uvs.append((s / segs, i / rings))
    idx = lambda i, s: 1 + (i - 1) * (segs + 1) + s
    faces += [(0, idx(1, s), idx(1, s + 1)) for s in range(segs)]
    faces += [(idx(i, s), idx(i + 1, s), idx(i + 1, s + 1), idx(i, s + 1)) for i in range(1, rings) for s in range(segs)]
    return mesh(name, verts, faces, uvs, mat, (at[0], 0.0, at[1]))


def fan(name, poly, mat, y=0.0, centre=(0.0, 0.0), at=(0.0, 0.0)):
    """A filled polygon seen from its centre (it must see its whole outline)."""
    verts = [(centre[0], y, centre[1])] + [(x, y, z) for x, z in poly]
    uvs = [(0.0, 0.0)] + [(i / len(poly), 1.0) for i in range(len(poly))]
    faces = [(0, 1 + i, 1 + (i + 1) % len(poly)) for i in range(len(poly))]
    return mesh(name, verts, faces, uvs, mat, (at[0], 0.0, at[1]))


def ribbon(name, pts, widths, mat, y=0.0, at=(0.0, 0.0)):
    """A strip along pts (x, z), widths[i] wide at each; u along it, v across."""
    P = [Vector((x, 0.0, z)) for x, z in pts]
    verts, uvs, faces = [], [], []
    for i, p in enumerate(P):
        d = (P[min(i + 1, len(P) - 1)] - P[max(i - 1, 0)]).normalized()
        n = Vector((-d.z, 0.0, d.x))
        w = widths[i] / 2 + 0.0015
        verts += [(p - n * w)[:], (p + n * w)[:]]
        uvs += [(i / (len(P) - 1), 0.0), (i / (len(P) - 1), 1.0)]
        if i:
            faces.append((2 * i - 2, 2 * i, 2 * i + 1, 2 * i - 1))
    verts = [(x, y, z) for x, _, z in verts]
    return mesh(name, verts, faces, uvs, mat, (at[0], 0.0, at[1]))


def curve(name, pts, radius, mat, y=0.0):
    cu = bpy.data.curves.new(name, 'CURVE')
    cu.dimensions = '3D'
    cu.bevel_depth = radius
    cu.bevel_resolution = 1
    sp = cu.splines.new('POLY')
    sp.points.add(len(pts) - 1)
    for i, p in enumerate(pts):
        sp.points[i].co = (p[0], y, p[1], 1)
    cu.materials.append(mat)
    return link(bpy.data.objects.new(name, cu))


def jag(a, b, depth, amp, r):
    """A jagged path from a to b (2D tuples), by midpoint displacement."""
    pts = [Vector(a), Vector(b)]
    for _ in range(depth):
        new = [pts[0]]
        for p, q in zip(pts, pts[1:]):
            d = q - p
            n = Vector((-d.y, d.x)).normalized() if d.length > 1e-6 else Vector((0, 1))
            new += [(p + q) / 2 + n * r.uniform(-amp, amp) * d.length, q]
        pts = new
    return [tuple(p) for p in pts]


def star(n, ro, ri, rot=0.0):
    return [((ro if k % 2 == 0 else ri) * math.cos(rot + math.pi * k / n),
             (ro if k % 2 == 0 else ri) * math.sin(rot + math.pi * k / n)) for k in range(2 * n)]


def kite(name, a, b, mat, y=0.0):
    """A spark: a thin kite pointing +x, widest near its head."""
    return mesh(name, [(-a, y, 0), (a * 0.55, y, -b), (a, y, 0), (a * 0.55, y, b)], [(0, 1, 2, 3)],
                [(0, 0)] * 4, mat)


class Motes:
    """Sparks born around `source()`, flying by `vel`, a whole number of lives per loop (seamless); once: all
    born together (a one-shot effect)."""

    def __init__(self, n, source, vel, size, mat, shape="kite", y=-0.05, lives=(1, 2), once=False):
        self.items = []
        for k in range(n):
            ob = mote(shape, size, mat, y)
            self.items.append((ob, source(), vel(), 0.0 if once else rng.random(), 1 if once else rng.randint(*lives),
                               rng.uniform(0.6, 1.0)))
        self.shape = shape

    def at(self, t, strength=1.0, heat=0.9):
        for ob, p, v, ph, lives, s0 in self.items:
            k = (t * lives + ph) % 1.0
            ob.location = (p.x + v.x * k, ob.location.y, p.y + v.y * k)
            if self.shape == "kite":
                ob.rotation_euler = (0.0, -math.atan2(v.y, v.x), 0.0)
            s = s0 * math.sin(math.pi * k) ** 0.6
            ob.scale = (s, s, s)
            knob(ob, strength, 1.0 - k * 0.6, 1.0, heat)

    def hide(self):
        for ob, *_ in self.items:
            knob(ob, 0.0, 0.0)


def mote(shape, size, mat, y=-0.05):
    name = "%s%d" % (shape, len(sc.collection.objects))
    if shape == "star":
        return fan(name, star(4, size, size * 0.22), mat, y)
    if shape == "dot":
        return disc(name, size, mat, y, rings=2, segs=10)
    if shape == "shard":  # a splinter of ice
        return fan(name, [(size, 0.0), (0.0, size * 0.32), (-size * 0.55, 0.0), (0.0, -size * 0.32)], mat, y)
    return kite(name, size * rng.uniform(0.6, 1.4), size * 0.16, mat, y)


def vec(x, z):
    return Vector((x, z))


# --- recipes -----------------------------------------------------------------------------------------------
def r_orbe():
    """An orb of energy and its trail (01: shadow, 02: frost, 03: a fist of fire)."""
    meta = dict(W=260, H=180, AX=185, AY=90, RH=40, N=12, FPS=30, LOOP=1)
    scene(meta["W"], meta["H"], meta["AX"], meta["AY"])
    core = disc("coeur", 0.33, m_glow(), 0.0)
    shell = disc("coquille", 0.45, m_ring(noisy=True), 0.01)
    m_sw = m_line()
    swirls = []
    for k in range(3):
        r, span, a0 = rng.uniform(0.36, 0.44), rng.uniform(1.6, 2.4), rng.uniform(0, 2 * math.pi)
        pts = [(r * math.cos(a0 - span * i / 20), r * math.sin(a0 - span * i / 20)) for i in range(21)]
        swirls.append((ribbon("volute%d" % k, pts, [0.03 * (1 - i / 20) ** 0.5 for i in range(21)], m_sw, -0.02),
                       rng.choice((1, -1)) * rng.choice((1, 2))))
    frost = STYLE == "p02"
    L = 1.5
    tail = ribbon("traine", [(-0.12 - L * i / 30, 0.0) for i in range(31)],
                  [0.8 * (1 - i / 30) ** 0.7 for i in range(31)], m_flame(rho=0.22, pale=frost), 0.03)
    m_sp = m_solid(5.0)
    motes = Motes(12, lambda: vec(-0.2 * rng.random(), rng.uniform(-0.35, 0.35)),
                  lambda: vec(-rng.uniform(0.6, 1.3), rng.uniform(-0.15, 0.15)), 0.035, m_sp,
                  "star" if frost else ("dot" if STYLE == "p03" else "kite"))
    crystals = []
    if frost:  # ice crystals turning around the orb
        m_cr = m_shape(0.6)
        for k in range(4):
            crystals.append((fan("cristal%d" % k, [(0.09, 0), (0, 0.03), (-0.09, 0), (0, -0.03)], m_cr, -0.03),
                             2 * math.pi * k / 4))

    def anim(f, t):
        knob(core, 1.0 + 0.12 * math.sin(4 * math.pi * t), 1.0)
        shell.rotation_euler = (0.0, -2 * math.pi * t, 0.0)
        knob(shell, 1.0, 1.0, 0.5)
        for ob, turns in swirls:
            ob.rotation_euler = (0.0, -2 * math.pi * t * turns, 0.0)
            knob(ob, 1.2, 0.9, 1.5, 0.8)
        knob(tail, 1.0, 1.0)
        motes.at(t)
        for ob, a in crystals:
            b = a + 2 * math.pi * t
            ob.location = (0.56 * math.cos(b), -0.03, 0.56 * math.sin(b) * 0.55)
            ob.rotation_euler = (0.0, -b * 2, 0.0)
            knob(ob, 1.0, 0.95, 1.0, 0.6)
    return meta, anim


def r_meteore():
    """A burning rock (03's meteor): dark stone cracked with lava, a mane of fire and smoke behind it."""
    meta = dict(W=320, H=200, AX=215, AY=100, RH=40, N=12, FPS=30, LOOP=1)
    scene(meta["W"], meta["H"], meta["AX"], meta["AY"])
    nd = Nodes("Roche")
    u, v = nd.uv()
    S, A, Wk = nd.knobs()
    tc = nd.new('ShaderNodeTexCoord')
    n = nd.new('ShaderNodeTexNoise', noise_dimensions='3D')
    nd.put(n.inputs['Vector'], tc.outputs['Object'])
    n.inputs['Scale'].default_value = 4.5
    n.inputs['Detail'].default_value = 3.0
    crack = nd.math('SUBTRACT', 1.0, nd.smooth(nd.math('ABSOLUTE', nd.math('SUBTRACT', n.outputs['Fac'], 0.5)), 0.0,
                                                0.05))
    rim = nd.smooth(v, 0.75, 1.0)
    x = nd.math('MAXIMUM', crack, nd.mul(rim, 0.8))
    rock = nd.surface(nd.ramp(x, [(0.0, rgba("#24110C")), (0.4, DEEP), (0.7, MAIN), (1.0, HOT)]),
                      nd.mul(S, nd.add(0.15, nd.mul(nd.math('POWER', x, 2.0), 4.0))),
                      nd.mul(A, nd.math('SUBTRACT', 1.0, nd.smooth(v, 0.96, 1.0))))
    stone = fan("roche", [(0.3 * (1 + 0.12 * math.sin(3 * a + 1) + 0.07 * math.sin(5 * a)) * math.cos(a),
                           0.3 * (1 + 0.12 * math.sin(3 * a + 1) + 0.07 * math.sin(5 * a)) * math.sin(a))
                          for a in (2 * math.pi * k / 40 for k in range(40))], rock, 0.0)
    shell = disc("feu", 0.44, m_ring(noisy=True), 0.01)
    L = 2.0
    tail = ribbon("criniere", [(-0.05 - L * i / 30, 0.04 * math.sin(i / 5)) for i in range(31)],
                  [0.95 * (1 - i / 30) ** 0.6 for i in range(31)], m_flame(rho=0.2, ka=7.0), 0.03)
    motes = Motes(16, lambda: vec(-0.25 * rng.random(), rng.uniform(-0.4, 0.4)),
                  lambda: vec(-rng.uniform(0.7, 1.6), rng.uniform(-0.1, 0.35)), 0.022, m_solid(6.0), "dot")

    def anim(f, t):
        stone.rotation_euler = (0.0, 2 * math.pi * t, 0.0)  # tumbling
        knob(stone, 1.0, 1.0)
        shell.rotation_euler = (0.0, -2 * math.pi * t, 0.0)
        knob(shell, 1.1, 1.0, 0.45)
        knob(tail, 1.0, 1.0)
        motes.at(t, 1.0, 0.85)
    return meta, anim


def r_lance():
    """A javelin of energy (02: a crystal of ice, 07: red lightning, 08: light)."""
    meta = dict(W=330, H=110, AX=250, AY=55, RH=30, N=12, FPS=30, LOOP=1)
    scene(meta["W"], meta["H"], meta["AX"], meta["AY"])
    body = fan("lance", [(0.5, 0.0), (0.06, 0.13), (-0.08, 0.07), (-1.55, 0.0), (-0.08, -0.07), (0.06, -0.13)],
               m_shape(0.7 if STYLE == "p02" else 0.6), 0.0, centre=(0.0, 0.0))
    core = ribbon("ame", [(0.4 - 1.8 * i / 30, 0.0) for i in range(31)], [0.022] * 31, m_line(), -0.01)
    m_tr = m_line()
    trails = [ribbon("sillage%d" % k, [(-0.4 - 1.6 * i / 30, z) for i in range(31)],
                     [0.02 * (1 - i / 30) for i in range(31)], m_tr, 0.02) for k, z in enumerate((-0.06, 0.06))]
    glow = ribbon("aura", [(0.45 - 2.2 * i / 30, 0.0) for i in range(31)], [0.26 * (1 - i / 30) ** 0.5 for i in range(31)],
                  m_flame(rho=0.3, ka=5.0, pale=True), 0.04)
    motes = Motes(10, lambda: vec(rng.uniform(-1.2, 0.2), rng.uniform(-0.08, 0.08)),
                  lambda: vec(-rng.uniform(0.4, 0.9), rng.uniform(-0.2, 0.2)), 0.03 if STYLE != "p08" else 0.045,
                  m_solid(5.0), "star" if STYLE in ("p02", "p08") else "kite")
    m_arc, m_halo = m_solid(14.0), m_solid(4.0)
    arcs = []

    def anim(f, t):
        knob(body, 1.0, 1.0, 1.0, 0.9)
        knob(core, 1.6, 1.0, 0.4, 1.0)
        for k, ob in enumerate(trails):
            knob(ob, 1.0, 0.6 + 0.3 * math.sin(2 * math.pi * (t * 2 + k * 0.5)), 1.5, 0.6)
        knob(glow, 0.9, 1.0)
        motes.at(t)
        for ob in arcs:
            bpy.data.objects.remove(ob)
        arcs.clear()
        if STYLE == "p07":  # arcs crawling along the bolt, new every frame
            r = random.Random(f * 31 + 7)
            for k in range(3):
                x0 = r.uniform(-1.3, 0.2)
                pts = jag((x0, r.uniform(-0.05, 0.05)), (x0 - r.uniform(0.25, 0.6), r.uniform(-0.18, 0.18)), 4, 0.35, r)
                arcs.append(curve("arc%d_%d" % (f, k), pts, 0.004, m_arc, -0.03))
                arcs.append(curve("halo%d_%d" % (f, k), pts, 0.012, m_halo, -0.025))
            for ob in arcs:
                knob(ob, 1.0, 1.0, 1.0, 1.0 if ob.name.startswith("arc") else 0.4)
    return meta, anim


def r_eclair():
    """07's red bolt: a jagged streak of lightning behind a ball of plasma, new every frame."""
    meta = dict(W=320, H=130, AX=250, AY=65, RH=24, N=12, FPS=30, LOOP=1)
    scene(meta["W"], meta["H"], meta["AX"], meta["AY"])
    head = disc("plasma", 0.17, m_glow(), 0.0)
    ring = disc("anneau", 0.22, m_ring(noisy=True), 0.01)
    m_core, m_halo = m_solid(16.0), m_solid(4.5)
    bolts = []

    def anim(f, t):
        knob(head, 1.3, 1.0)
        ring.rotation_euler = (0.0, -2 * math.pi * t * 2, 0.0)
        knob(ring, 1.0, 1.0, 0.5)
        for ob in bolts:
            bpy.data.objects.remove(ob)
        bolts.clear()
        r = random.Random(f * 17 + 3)
        main = jag((0.1, 0.0), (-2.3, r.uniform(-0.12, 0.12)), 5, 0.22, r)
        paths = [main]
        for k in range(3):
            i = r.randrange(4, len(main) - 6)
            a = main[i]
            paths.append(jag(a, (a[0] - r.uniform(0.25, 0.6), a[1] + r.uniform(-0.3, 0.3)), 3, 0.3, r))
        for k, p in enumerate(paths):
            w = 1.0 if k == 0 else 0.6
            c = curve("eclair%d_%d" % (f, k), p, 0.006 * w, m_core, -0.02)
            h = curve("halo%d_%d" % (f, k), p, 0.02 * w, m_halo, -0.015)
            knob(c, 1.0, 1.0, 1.0, 1.0)
            knob(h, 1.0, 0.7, 1.0, 0.45)
            bolts.extend([c, h])
    return meta, anim


def r_etoile():
    """02's shooting star: a four-pointed star of frost turning, a trail of sparkles behind it."""
    meta = dict(W=230, H=160, AX=150, AY=80, RH=34, N=12, FPS=30, LOOP=1)
    scene(meta["W"], meta["H"], meta["AX"], meta["AY"])
    m_st = m_shape(0.8)
    big = fan("etoile", star(4, 0.42, 0.1), m_st, 0.0)
    small = fan("etoile2", star(4, 0.25, 0.07, math.pi / 4), m_st, 0.01)
    core = disc("coeur", 0.16, m_glow(), -0.01)
    tail = ribbon("traine", [(-0.1 - 1.1 * i / 30, 0.0) for i in range(31)], [0.34 * (1 - i / 30) ** 0.8 for i in range(31)],
                  m_flame(rho=0.25, pale=True), 0.03)
    motes = Motes(10, lambda: vec(-rng.uniform(0.0, 0.3), rng.uniform(-0.2, 0.2)),
                  lambda: vec(-rng.uniform(0.6, 1.1), rng.uniform(-0.25, 0.25)), 0.05, m_solid(5.0), "star")

    def anim(f, t):
        big.rotation_euler = (0.0, -math.pi / 2 * t, 0.0)  # a quarter turn per loop: seamless
        small.rotation_euler = (0.0, math.pi / 2 * t, 0.0)
        knob(big, 1.1, 1.0, 1.0, 1.0)
        knob(small, 0.9, 0.85, 1.0, 0.8)
        knob(core, 1.2 + 0.2 * math.sin(4 * math.pi * t), 1.0)
        knob(tail, 1.0, 1.0)
        motes.at(t)
    return meta, anim


def r_dague():
    """01's spectral dagger, spinning (boomerang), two ghosts of it lagging behind."""
    meta = dict(W=200, H=200, AX=100, AY=100, RH=30, N=12, FPS=30, LOOP=1)
    scene(meta["W"], meta["H"], meta["AX"], meta["AY"])
    m_bl = m_shape(0.7)
    parts = []
    for g, (lag, alpha) in enumerate(((0.0, 1.0), (0.45, 0.45), (0.9, 0.2))):
        y = 0.02 * g
        k = 1.5
        blade = fan("lame%d" % g, [(0.36 * k, 0.0), (0.02 * k, 0.06 * k), (-0.05 * k, 0.05 * k), (-0.05 * k, -0.05 * k),
                                   (0.02 * k, -0.06 * k)], m_bl, y, centre=(0.05 * k, 0.0))
        guard = fan("garde%d" % g, [(-0.05 * k, 0.12 * k), (-0.09 * k, 0.12 * k), (-0.09 * k, -0.12 * k),
                                    (-0.05 * k, -0.12 * k)], m_bl, y, centre=(-0.07 * k, 0.0))
        grip = fan("poignee%d" % g, [(-0.09 * k, 0.025 * k), (-0.25 * k, 0.025 * k), (-0.29 * k, 0.0),
                                     (-0.25 * k, -0.025 * k), (-0.09 * k, -0.025 * k)], m_bl, y, centre=(-0.17 * k, 0.0))
        parts.append(([blade, guard, grip], lag, alpha))

    def anim(f, t):
        for obs, lag, alpha in parts:
            a = -2 * math.pi * t - lag
            for ob in obs:
                ob.rotation_euler = (0.0, a, 0.0)
                knob(ob, 1.0 if alpha == 1.0 else 0.8, alpha, 1.0, 0.85 if alpha == 1.0 else 0.4)
    return meta, anim


def r_plume():
    """08's feather of light, swaying a little, a soft trail of light and glints behind it."""
    meta = dict(W=280, H=130, AX=185, AY=65, RH=26, N=12, FPS=30, LOOP=1)
    scene(meta["W"], meta["H"], meta["AX"], meta["AY"])
    m_f = m_shape(0.7)
    vane = []
    for k in range(25):
        s = k / 24
        x = 0.46 - 0.95 * s
        w = 0.14 * math.sin(math.pi * min(1.0, s * 1.1)) ** 0.7
        vane.append((x, w + 0.05 * math.sin(s * math.pi)))
    poly = vane + [(x, -(w - 0.05 * math.sin(k / 24 * math.pi)) * 0.75 + 0.05 * math.sin(k / 24 * math.pi))
                   for k, (x, w) in reversed(list(enumerate(vane))[1:-1])]
    feather = fan("plume", poly, m_f, 0.0, centre=(0.0, 0.0))
    quill = ribbon("tige", [(0.44 - 1.05 * i / 20, 0.045 * math.sin(i / 20 * math.pi)) for i in range(21)],
                   [0.016] * 21, m_line(), -0.01)
    m_b = m_line()
    barbs = []
    for k in range(12):
        x = 0.34 - 0.7 * k / 11
        z = 0.045 * math.sin((0.46 - x) / 0.95 * math.pi)
        for sgn in (1, -1):
            barbs.append(ribbon("barbe%d_%d" % (k, sgn), [(x, z), (x - 0.08, z + 0.11 * sgn)], [0.006, 0.003], m_b,
                                -0.005))
    tail = ribbon("traine", [(-0.3 - 1.0 * i / 30, 0.0) for i in range(31)], [0.2 * (1 - i / 30) for i in range(31)],
                  m_flame(rho=0.3, pale=True), 0.03)
    motes = Motes(9, lambda: vec(rng.uniform(-0.4, 0.2), rng.uniform(-0.1, 0.1)),
                  lambda: vec(-rng.uniform(0.4, 0.9), rng.uniform(-0.2, 0.2)), 0.05, m_solid(5.0), "star")
    group = [feather, quill] + barbs

    def anim(f, t):
        a = math.radians(5) * math.sin(2 * math.pi * t)
        for ob in group:
            ob.rotation_euler = (0.0, -a, 0.0)
        knob(feather, 1.0, 1.0, 1.0, 0.8)
        knob(quill, 1.5, 1.0, 0.3, 1.0)
        for ob in barbs:
            knob(ob, 0.8, 0.6, 1.0, 0.9)
        knob(tail, 1.0, 1.0)
        motes.at(t)
    return meta, anim


def r_marque():
    """01's trap: a violet sigil hanging where it was set, its rings turning, its heart dark."""
    meta = dict(W=190, H=190, AX=95, AY=95, RH=44, N=12, FPS=30, LOOP=1)
    scene(meta["W"], meta["H"], meta["AX"], meta["AY"])
    heart = disc("coeur", 0.3, m_dark(), 0.03)
    outer = disc("cercle", 0.42, m_ring(), 0.0)
    inner = disc("cercle2", 0.31, m_ring(), 0.0)
    m_r = m_shape(0.8)
    runes = [fan("rune%d" % k, [(0.035, 0), (0, 0.02), (-0.035, 0), (0, -0.02)], m_r, -0.01) for k in range(8)]
    m_l = m_line()
    tri = []
    for k in range(3):  # three thin crescents (01's moons), a third of a turn apart
        a0 = 2 * math.pi * k / 3
        pts = [(0.2 * math.cos(a0 + 1.9 * i / 16) + 0.06 * math.cos(a0), 0.2 * math.sin(a0 + 1.9 * i / 16)
                + 0.06 * math.sin(a0)) for i in range(17)]
        tri.append(ribbon("lune%d" % k, pts, [0.03 * math.sin(math.pi * i / 16) ** 0.7 + 0.004 for i in range(17)],
                          m_l, -0.005))
    core = disc("lueur", 0.1, m_glow(), -0.02)

    def anim(f, t):
        pulse = 1.0 + 0.25 * math.sin(2 * math.pi * t)
        knob(heart, 1.0, 0.75)
        knob(outer, pulse, 1.0, 0.12)
        knob(inner, pulse * 0.8, 0.9, 0.08)
        for k, ob in enumerate(runes):
            a = 2 * math.pi * k / 8 + math.pi / 4 * t  # an eighth of a turn per loop
            ob.location = (0.365 * math.cos(a), -0.01, 0.365 * math.sin(a))
            ob.rotation_euler = (0.0, -a, 0.0)
            knob(ob, pulse, 1.0, 1.0, 0.9)
        for ob in tri:
            ob.rotation_euler = (0.0, 2 * math.pi / 3 * t, 0.0)  # a third of a turn per loop
            knob(ob, pulse, 0.9, 0.0, 0.7)
        knob(core, pulse, 0.9)
    return meta, anim


def r_pilier():
    """A column out of the ground, once (02: ice spikes, 03: fire, 07: red lightning, 08: light). Anchor: its foot."""
    meta = dict(W=240, H=460, AX=120, AY=440, RH=60, N=16, FPS=30, LOOP=0)
    scene(meta["W"], meta["H"], meta["AX"], meta["AY"])
    TOP = 3.8
    mark = disc("marque", 0.5, m_glow(), 0.05)
    mark.scale = (1.0, 1.0, 0.16)
    burst = disc("onde", 0.6, m_ring(), 0.04)
    debris = Motes(14, lambda: vec(rng.uniform(-0.3, 0.3), 0.02),
                   lambda: vec(rng.uniform(-0.9, 0.9), rng.uniform(0.4, 1.4)), 0.03, m_solid(5.0),
                   "star" if STYLE in ("p02", "p08") else "dot", once=True)
    cols, spikes, bolts = [], [], []
    if STYLE == "p02":  # ice: a cluster of crystal spikes
        m_sp = m_shape(0.75)
        for k in range(7):
            h = rng.uniform(1.4, 3.2) * (1.0 if k == 0 else 0.75)
            w = h * rng.uniform(0.09, 0.13)
            x0 = 0.0 if k == 0 else rng.uniform(-0.45, 0.45)
            ob = fan("pic%d" % k, [(0.0, h), (w, h * 0.55), (w * 0.8, 0.0), (-w * 0.8, 0.0), (-w, h * 0.5)], m_sp,
                     -0.01 * k, centre=(0.0, h * 0.4), at=(x0, 0.0))
            ob.rotation_euler = (0.0, -math.radians(rng.uniform(-22, 22) if k else 0.0), 0.0)
            spikes.append(ob)
        mist = ribbon("brume", [(0.0, 0.0), (0.0, 1.2)], [1.2, 0.6], m_flame(rho=0.3, pale=True), 0.06)
        cols.append(mist)
    elif STYLE == "p07":
        cols.append(ribbon("colonne", [(0.0, TOP * i / 20) for i in range(21)], [0.32] * 21,
                           m_flame(rho=0.35, ka=4.0, pale=True), 0.06))
    elif STYLE == "p03":  # a column of flame
        cols.append(ribbon("colonne", [(0.0, TOP * i / 30) for i in range(31)],
                           [1.05 * (1 - 0.3 * i / 30) for i in range(31)], m_flame(rho=0.18, ka=5.0), 0.02))
        cols.append(ribbon("colonne2", [(0.0, TOP * 0.8 * i / 30) for i in range(31)],
                           [0.6 * (1 - 0.3 * i / 30) for i in range(31)], m_flame(rho=0.22, ka=7.0), 0.01))
    else:  # a column of light
        cols.append(ribbon("colonne", [(0.0, TOP * i / 30) for i in range(31)], [0.8] * 31, m_beam(), 0.02))
    m_core, m_halo = m_solid(16.0), m_solid(4.5)
    ERUPT = 4

    def anim(f, t):
        e = f - ERUPT
        # the warning: a glow on the ground, growing; it flashes as the column comes out
        g = min(1.0, (f + 1) / (ERUPT + 1))
        mark.scale = (0.6 + 0.6 * g, 1.0, 0.16)
        knob(mark, 0.6 + 0.8 * g + (1.5 if e == 0 else 0.0), max(0.0, 0.8 if e < 3 else 0.8 - 0.15 * (e - 2)))
        if e < 0:
            for ob in cols + spikes:
                knob(ob, 0.0, 0.0)
            knob(burst, 0.0, 0.0)
            for ob, *_ in debris.items:
                knob(ob, 0.0, 0.0)
            return
        burst.scale = (0.5 + 0.35 * e, 1.0, 0.2 + 0.07 * e)
        knob(burst, 2.0 - 0.3 * e, max(0.0, 1.0 - 0.22 * e), 0.18)
        rise = min(1.0, (e + 1) / 2.0)  # up in two frames
        fade = 1.0 if e < 6 else max(0.0, 1.0 - (e - 5) / 6.0)
        for ob in cols:
            ob.scale = (fade ** 0.5 if STYLE != "p02" else 1.0, 1.0, rise)
            knob(ob, 1.0 + (0.6 if e == 0 else 0.0), fade, 1.5 if ob.name == "coeur" else 1.0, 1.0)
        for k, ob in enumerate(spikes):
            ob.scale = (1.0, 1.0, rise if e < 8 else rise * (1.0 - 0.06 * (e - 7)))
            knob(ob, 1.0 + (0.8 if e == 0 else 0.0), fade, 1.0, 0.7)
        debris.at(min(0.999, e / 9.0), 1.0, 0.8)
        for ob in bolts:
            bpy.data.objects.remove(ob)
        bolts.clear()
        if STYLE == "p07" and e < 8:
            r = random.Random(f * 13 + 5)
            for k in range(2 if e < 5 else 1):
                p = jag((r.uniform(-0.1, 0.1), TOP * rise), (r.uniform(-0.15, 0.15), 0.0), 5, 0.18, r)
                for i in range(2):
                    q = r.randrange(6, len(p) - 4)
                    a = p[q]
                    p2 = jag(a, (a[0] + r.uniform(-0.6, 0.6), a[1] - r.uniform(0.2, 0.5)), 3, 0.3, r)
                    bolts.append(curve("br%d_%d_%d" % (f, k, i), p2, 0.004, m_core, -0.03))
                    bolts.append(curve("brh%d_%d_%d" % (f, k, i), p2, 0.013, m_halo, -0.025))
                bolts.append(curve("b%d_%d" % (f, k), p, 0.007, m_core, -0.03))
                bolts.append(curve("bh%d_%d" % (f, k), p, 0.024, m_halo, -0.025))
            for ob in bolts:
                knob(ob, fade, 1.0, 1.0, 1.0 if "h" not in ob.name else 0.45)
    return meta, anim


# --- cast effects ------------------------------------------------------------------------------------------
# A technique's castFx: drawn behind its fighter, at its feet, for a fighter 170 px tall (1.7 units), facing right.
# Played once (LOOP 0) at 30 fps from the move's castFxFrame: strip frame i is the move's game frame 2i after it.
# Read like anime and game effects, not diagrams: no glowing balls, whole rings or cones; a blow flashes as uneven
# spikes for a frame or two, its shock is thin broken arcs, trails are teardrops, flames and wisps.
ELEMENT = {"p01": "ombre", "p02": "givre", "p03": "feu", "p07": "foudre", "p08": "lumiere", "garde": "verre"}[STYLE]
MOTE = {"p01": "kite", "p02": "star", "p03": "dot", "p07": "kite", "p08": "star", "garde": "star"}[STYLE]
MOTE_SIZE = {"star": 0.05, "dot": 0.024, "kite": 0.04}[MOTE]
DYN = []  # objects rebuilt every frame (lightning, arcs)
M = {}  # materials shared by the rebuilt objects


def fresh():
    for ob in DYN:
        bpy.data.objects.remove(ob)
    DYN.clear()


def dyn(ob):
    DYN.append(ob)
    return ob


def cast_scene(W, H, AX, AY, N):
    meta = dict(W=int(W), H=int(H), AX=AX, AY=AY, RH=0, N=N, FPS=30, LOOP=0)
    scene(meta["W"], meta["H"], meta["AX"], meta["AY"])
    pale = ELEMENT in ("givre", "lumiere", "foudre")
    M.update(core=m_solid(16.0), halo=m_solid(4.5), line=m_line(), spark=m_solid(5.0), flash=m_shape(0.85),
             glow=m_glow(), flame=m_flame(rho=0.25, ka=6.0, pale=pale),
             clean=m_flame(rho=0.25, ka=6.0, pale=pale, smoky=False))  # (clean: no smoke at its end)
    return meta


def drop(s, head=0.15, p=0.8):
    """A teardrop's width along it (s: 0 at its head, 1 at its tail): round at its head, thinning to nothing."""
    return min(1.0, s / head) ** 0.5 * (1.0 - s) ** p


def bolt(name, a, b, depth, amp, r, w=1.0, alpha=1.0, y=-0.03):
    """A jagged arc of lightning from a to b, for this frame only: a white-hot core in a red halo (thick enough to
    read in the game, where a fighter is often half the 170 px it is drawn for)."""
    p = jag(a, b, depth, amp, r)
    for tag, rad, mat, s, al, heat, dy in (("", 0.01, "core", 1.0, 1.0, 0.9, 0.0),
                                           ("h", 0.03, "halo", 1.0, 0.75, 0.3, 0.005),
                                           ("g", 0.085, "halo", 0.45, 0.3, 0.1, 0.01)):
        knob(dyn(curve(name + tag, p, rad * w, M[mat], y + dy)), s, alpha * al, 1.0, heat)
    return p


def tongue(name, mat, x, h, w, lean=0.0, y=0.0, n=12):
    """A flame standing on the ground at x (u: 0 at its foot, the hottest); scale.z makes it grow."""
    pts = [(lean * (i / n) ** 2, h * i / n) for i in range(n + 1)]
    return ribbon(name, pts, [w * (1 - i / n) ** 0.6 + 0.01 for i in range(n + 1)], mat, y, at=(x, 0.0))


def streak(name, mat, w, y=0.0, n=10):
    """A line from its head (0, 0) back to (-1, 0), thinning: placed by location, stretched by scale.x."""
    return ribbon(name, [(-i / n, 0.0) for i in range(n + 1)], [w * (1 - i / n) ** 0.5 + 0.002 for i in range(n + 1)],
                  mat, y)


def line(name, p, q, w, mat, y=0.0, n=12):
    """A straight stroke from p to q, w thick in its middle, sharp at both ends."""
    return ribbon(name, [(p[0] + (q[0] - p[0]) * i / n, p[1] + (q[1] - p[1]) * i / n) for i in range(n + 1)],
                  [w * math.sin(math.pi * (0.03 + 0.94 * i / n)) ** 0.5 + 0.003 for i in range(n + 1)], mat, y)


def arc(name, mat, c, R, a0, a1, w, y=0.0, n=32, power=0.8, sy=1.0, bulge=0.0, prof=None):
    """A crescent along a circle (centre c, radius R, flattened by sy) from angle a0 to a1 (radians), w thick in its
    middle; bulge pushes its middle out (0.5 w': along the outer edge of a crescent w' thick); prof(s): its width
    along it instead (s: 0 at a0, 1 at a1)."""
    pts, ws = [], []
    for i in range(n + 1):
        a, s = a0 + (a1 - a0) * i / n, prof(i / n) if prof else math.sin(math.pi * i / n) ** power
        pts.append((c[0] + (R + bulge * s) * math.cos(a), c[1] + (R + bulge * s) * sy * math.sin(a)))
        ws.append(w * s + 0.004)
    return ribbon(name, pts, ws, mat, y)


def shocks(tag, f, c, arcs, R0, R1, life, w, sy=1.0, s=1.6, heat=0.8):
    """The shock of a blow on frame f: thin broken arcs flying out from c. arcs: (born, middle, span), angles in
    degrees; each grows from R0 to R1 in `life` frames, thinning and fading."""
    for j, (born, mid, span) in enumerate(arcs):
        k = (f - born) / life
        if 0 <= k < 1:
            ob = dyn(arc("%s%d_%d" % (tag, f, j), M["line"], c, R0 + (R1 - R0) * (1 - (1 - k) ** 2),
                         math.radians(mid - span / 2), math.radians(mid + span / 2), w * (1 - k) ** 0.7, -0.035,
                         sy=sy))
            knob(ob, s, 1.0 - k ** 1.5, 0.0, heat)


def kick(x=0.0, n=10, born=0, sides=(1, -1), reach=1.2, life=6):
    """Grit kicked along the ground by a push or a blow (not a ring): streaks flying low and out to its sides."""
    return Sparks([(vec(x + s * rng.uniform(0.05, 0.25), 0.03), vec(s * rng.uniform(0.4, 1.0) * reach,
                                                                    rng.uniform(0.05, 0.4)), born + rng.randint(0, 1))
                   for s in sides for _ in range(n // len(sides))], life=life, size=0.08, shape="kite")


def spikes(name, at, size, n=7, y=-0.04):
    """The flash of a blow: uneven spikes around a thin core, shown a frame or two (not a symmetric star)."""
    pts, rot = [], rng.uniform(0.0, math.pi)
    for k in range(2 * n):
        a = rot + math.pi * k / n + rng.uniform(-0.12, 0.12)
        r = size * 0.08 if k % 2 else size * rng.uniform(0.35, 1.0)
        pts.append((r * math.cos(a), r * math.sin(a)))
    return fan(name, pts, M["flash"], y, at=at)


def flare(name, at, length, y=-0.045):
    """A thin streak of light across a flash, white in its middle."""
    ob = disc(name, length, M["glow"], y, at=at, rings=6, segs=36)
    ob.scale = (1.0, 1.0, 0.05)
    return ob


class Sparks:
    """One spark per spot, born on its frame, thrown by its velocity, gone after `life` frames."""

    def __init__(self, spots, life=7, size=None, shape=None, mat=None):
        self.shape = shape or MOTE
        self.items = [(mote(self.shape, (size or MOTE_SIZE) * rng.uniform(0.7, 1.2), mat or M["spark"], -0.04), p, v,
                       born) for p, v, born in spots]
        self.life = life

    def at(self, f, strength=1.0, heat=0.9):
        for ob, p, v, born in self.items:
            k = (f - born) / self.life
            if k < 0 or k >= 1:
                knob(ob, 0.0, 0.0)
                continue
            ob.location = (p.x + v.x * k, ob.location.y, p.y + v.y * k)
            if self.shape in ("kite", "shard"):  # along its flight; a splinter tumbles
                ob.rotation_euler = (0.0, -math.atan2(v.y, v.x) - (4.0 * k if self.shape == "shard" else 0.0), 0.0)
            s = math.sin(math.pi * min(1.0, 0.25 + k)) ** 0.6
            ob.scale = (s, s, s)
            knob(ob, strength, 1.0 - k * 0.7, 1.0, heat)


def hide(*obs):
    for ob in obs:
        knob(ob, 0.0, 0.0)


def c_ruee(path):
    """A dash (left where it began): grit kicked back at its start, a teardrop of its element and speed lines going with
    the fighter, and its element left along the way. path: where the fighter is (px ahead of its start) on each
    strip frame."""
    L = path[-1] / 100
    X = [p / 100 for p in path]
    stop = next((i for i in range(1, len(path)) if path[i] - path[i - 1] < 15), len(path) - 1)  # it has slowed down
    N = len(path) + 7
    X += [L] * 7
    meta = cast_scene(path[-1] + 230, 250, 90, 215, N)
    gleam = flare("depart", (0.15, 0.85), 0.8)
    grit = kick(0.0, 8, 0, (-1,), 1.1)
    dust = Motes(10, lambda: vec(rng.uniform(-0.05, 0.1), rng.uniform(0.02, 0.4)),
                 lambda: vec(-rng.uniform(0.5, 1.2), rng.uniform(0.05, 0.5)), MOTE_SIZE, M["spark"], MOTE, once=True)
    heights = sorted(rng.uniform(0.2, 1.6) for _ in range(5))
    lines = [(streak("vitesse%d" % k, M["line"], rng.uniform(0.008, 0.025), -0.02), z, rng.uniform(0.0, 0.4),
              rng.uniform(0.5, 2.0), rng.uniform(0.5, 1.0)) for k, z in enumerate(heights)]
    spots, x = [], 0.08
    while x < L:
        spots.append((x, next(i for i, v in enumerate(X) if v >= x), rng.uniform(0.6, 1.0)))
        x += rng.uniform(0.13, 0.2)
    tongues, shards = [], []
    if ELEMENT in ("ombre", "feu", "givre"):  # flames (frost: mist) left burning where it ran, leaning back
        tall = {"ombre": 0.42, "feu": 0.42, "givre": 0.2}[ELEMENT]
        for k, (x, born, s) in enumerate(spots):
            h = tall * s * rng.uniform(0.5, 1.3)
            tongues.append((tongue("trace%d" % k, M["flame"], x, h, 0.08 + 0.3 * h, -rng.uniform(0.05, 0.25),
                                   0.01 + 0.001 * k), born))
    if ELEMENT == "givre":  # a few crystals of ice left standing, leaning back
        m_ic = m_shape(0.6)
        for k, (x, born, s) in enumerate(spots[1::3]):
            for j in range(rng.randint(1, 3)):
                h = rng.uniform(0.12, 0.35)
                w = h * rng.uniform(0.18, 0.26)
                ob = fan("glace%d_%d" % (k, j), [(0.0, h), (w, h * 0.55), (w * 0.75, 0.0), (-w * 0.75, 0.0),
                                                 (-w, h * 0.45)], m_ic, -0.005 - 0.001 * j, centre=(0.0, h * 0.4),
                         at=(x + rng.uniform(-0.06, 0.06), 0.0))
                ob.rotation_euler = (0.0, -math.radians(rng.uniform(5, 40)), 0.0)
                shards.append((ob, born + j // 2))
    sparks = Sparks([(vec(x, rng.uniform(0.05, 0.9)), vec(-rng.uniform(0.15, 0.5), rng.uniform(0.15, 0.5)), born)
                     for x, born, s in spots])

    def anim(f, t):
        fresh()
        x = X[min(f, len(X) - 1)]
        gone = 1.0 if f <= stop else max(0.0, 1.0 - (f - stop) / 5.0)  # the tails catch up once it stops
        grit.at(f, 1.2, 0.8)
        knob(gleam, 2.0, (1.0, 0.45)[f] if f < 2 else 0.0)
        dust.at(min(0.999, f / 9.0), 1.0, 0.8)
        for ob, z, lag, ln, s in lines:
            head = max(0.0, x - lag)
            ob.location = (head, ob.location.y, z)
            ob.scale = (max(1e-3, min(ln, head) * gone), 1.0, 1.0)
            knob(ob, 1.3 * s, gone * s * (1.0 if head > 0.05 else 0.0), 1.2, 0.8)
        tail = min(x, 1.6) * gone
        if tail > 0.08:  # a teardrop of its element behind the fighter, at its body's height
            ob = dyn(ribbon("comete%d" % f, [(x - tail * i / 16, 0.85) for i in range(17)],
                            [0.42 * drop(i / 16) + 0.01 for i in range(17)], M["flame"], 0.02))
            knob(ob, 1.0, 0.65 * gone)
        for ob, born in tongues:
            age = f - born
            if age < 0:
                hide(ob)
                continue
            ob.location = (ob.location.x, ob.location.y, 0.03 * age)  # the flames rise as they die
            ob.scale = (1.0, 1.0, max(0.05, min(1.0, (age + 1) / 2.0) * max(0.0, 1.0 - age / 9.0) ** 0.5))
            knob(ob, 1.0 + (0.5 if age == 0 else 0.0), max(0.0, 1.0 - age / 9.0), 1.0)
        for ob, born in shards:
            age = f - born
            if age < 0:
                hide(ob)
                continue
            ob.scale = (1.0, 1.0, min(1.0, (age + 1) / 2.0))
            knob(ob, 0.8 + (0.6 if age == 0 else 0.0), max(0.0, 1.0 - max(0, age - 6) / 6.0), 1.0, 0.5)
        if ELEMENT == "givre" and x > 0.05:  # a streak of frost along the ground
            ob = dyn(ribbon("givre%d" % f, [(x * i / 12, 0.02) for i in range(13)], [0.03] * 13, M["line"], -0.01))
            knob(ob, 1.2, max(0.0, 1.0 - max(0, f - stop) / 8.0), 0.0, 0.7)
        sparks.at(f)
        if ELEMENT == "foudre" and gone > 0 and x > 0.05:  # a crackling line where it ran, arcs leaping off it
            r = random.Random(f * 29 + 11)
            bolt("sol%d" % f, (0.0, 0.06), (x, 0.12), 5, 0.12, r, 0.9, gone)
            for j in range(3):
                a = r.uniform(0.0, x)
                bolt("arc%d_%d" % (f, j), (a, 0.06), (a - r.uniform(0.1, 0.35), r.uniform(0.25, 0.8)), 3, 0.3, r, 0.6,
                     gone)
    return meta, anim


def c_passage(D=255):
    """A step through the opponent (left where it began), read like a flash step in anime: the fighter vanishes
    (frame 2) and a long thin cut appears across its path, white in its element's glow, with two after-cuts, speed
    streaks and wisps of its element; then the cut breaks up and fades. D px: where it comes out."""
    TP, N, Z, Du = 2, 11, 0.85, D / 100
    meta = cast_scene(D + 250, 270, 110, 225, N)
    P, Q = (-0.35, Z + 0.1), (Du + 0.45, Z - 0.1)  # the cut: a little past both ends, slightly down
    aura = tongue("aura", M["flame"], 0.0, 1.4, 0.4, 0.0, 0.03)
    pull = [(rng.uniform(0, 2 * math.pi), rng.uniform(0.45, 1.0), rng.randint(0, 1)) for _ in range(7)]
    inward = Sparks([(vec(d * math.cos(a), Z + 0.8 * d * math.sin(a)), vec(-0.85 * d * math.cos(a),
                                                                          -0.7 * d * math.sin(a)), b)
                     for a, d, b in pull], life=3, shape="kite")
    out0 = Sparks([(vec(0.0, Z + rng.uniform(-0.6, 0.6)), vec(rng.uniform(-1.0, 0.2), rng.uniform(-0.4, 0.5)), TP)
                   for _ in range(10)], life=7)
    out1 = Sparks([(vec(Du, Z + rng.uniform(-0.6, 0.6)), vec(rng.uniform(0.0, 1.0), rng.uniform(-0.4, 0.5)), TP)
                   for _ in range(10)], life=7)
    flash = spikes("eclat", (Du, Z), 0.55)
    wisps = []
    for k in range(7):  # wisps of its element along the way, streaming back
        x, z, ln = rng.uniform(0.1, Du), Z + rng.uniform(-0.45, 0.45), rng.uniform(0.35, 0.8)
        dz = rng.uniform(-0.12, 0.12)
        wisps.append((ribbon("volute%d" % k, [(x - ln * i / 10, z + dz * (i / 10) ** 2) for i in range(11)],
                             [0.16 * drop(i / 10, 0.2) + 0.005 for i in range(11)], M["flame"], 0.01 + 0.002 * k),
                      TP + k % 2))
    speed = []
    for k in range(6):  # speed streaks along the path
        x = rng.uniform(0.3, Du + 0.2)
        speed.append((streak("vitesse%d" % k, M["line"], rng.uniform(0.008, 0.02), -0.025), x,
                      Z + rng.uniform(-0.6, 0.6), rng.uniform(0.4, 1.2)))

    def cut(tag, p, q, w, mat, y, pieces, gap, s, a, wk=0.0, heat=1.0):
        for j in range(pieces):  # the stroke broken in `pieces`, `gap` (a share of it) between them
            t0 = j / pieces + (gap / 2 if j else 0.0)
            t1 = (j + 1) / pieces - (gap / 2 if j < pieces - 1 else 0.0)
            knob(dyn(line("%s%d" % (tag, j), (p[0] + (q[0] - p[0]) * t0, p[1] + (q[1] - p[1]) * t0),
                          (p[0] + (q[0] - p[0]) * t1, p[1] + (q[1] - p[1]) * t1), w, mat, y)), s, a, wk, heat)

    def anim(f, t):
        fresh()
        e = f - TP
        knob(aura, 1.0, 0.2 + 0.15 * f if e < 0 else 0.0)  # it gathers itself, gone when it steps
        inward.at(f)
        out0.at(f)
        out1.at(f)
        knob(flash, 2.2, (1.0, 0.4)[e] if 0 <= e < 2 else 0.0, 1.0, 1.0)
        for ob, born in wisps:
            age = f - born
            if 0 <= age < 6:
                ob.location = (-0.06 * age, ob.location.y, 0.03 * age)
                knob(ob, 1.0, 0.85 * (1 - age / 6.0))
            else:
                hide(ob)
        for ob, x, z, ln in speed:
            if 0 <= e < 4:
                ob.location = (x, ob.location.y, z)
                ob.scale = (ln * (1.0 - 0.2 * e), 1.0, 1.0)
                knob(ob, 1.3, 1.0 - e / 4.0, 1.2, 0.8)
            else:
                hide(ob)
        if e < 0:
            return
        k = max(0.0, 1.0 - e / 7.0)
        pieces, gap = (1, 0.0) if e < 2 else (3, 0.05) if e < 4 else (5, 0.1)
        if ELEMENT == "foudre" and e < 5:  # the cut is a bolt
            r = random.Random(f * 7 + 1)
            bolt("coupe%d" % f, P, Q, 5, 0.08, r, 1.2, k)
            for j in range(2):
                a = r.uniform(0.2, Du - 0.2)
                bolt("br%d_%d" % (f, j), (a, Z), (a + r.uniform(-0.3, 0.3), Z + r.uniform(-0.45, 0.45)), 3, 0.3, r,
                     0.6, k)
        else:
            cut("coupe%d_" % f, P, Q, 0.04 * k ** 0.5, M["line"], -0.02, pieces, gap, 2.6 if e < 2 else 1.8, k)
        cut("lueur%d_" % f, P, Q, 0.2 * (0.7 + 0.5 * (1 - k)), M["clean"], -0.01, pieces, gap, 1.0, 0.7 * k)
        if 1 <= e < 6:  # two after-cuts, a little above and below
            for j, dz in enumerate((0.13, -0.14)):
                cut("apres%d_%d_" % (f, j), (P[0] + 0.15 * Du, P[1] + dz), (Q[0] - 0.2 * Du, Q[1] + dz), 0.02,
                    M["line"], -0.015, 1, 0.0, 1.5, 0.6 * k)
        shocks("arrivee", f, (Du, Z), [(TP, 0, 50), (TP, 25, 30), (TP + 1, -20, 40)], 0.3, 0.9, 4, 0.045)
    return meta, anim


def c_clin():
    """A teleport (left where it began): a pool of its light on the ground and light gathering around the fighter;
    at frame 7 the light narrows to a line and shoots up as the fighter vanishes."""
    TP, N = 7, 13
    meta = cast_scene(320, 420, 160, 370, N)
    pool = disc("source", 0.8, M["glow"], 0.06)
    column = ribbon("colonne", [(0.0, 1.9 * i / 20) for i in range(21)],
                    [0.62 * (0.85 + 0.15 * math.sin(math.pi * i / 20)) for i in range(21)], M["flame"], 0.05)
    rising = Motes(14, lambda: vec(rng.uniform(-0.6, 0.6), rng.uniform(0.0, 0.2)),
                   lambda: vec(rng.uniform(-0.1, 0.1), rng.uniform(0.9, 1.7)), MOTE_SIZE, M["spark"], MOTE, lives=(1, 2))
    beam = streak("trait", M["line"], 0.07, -0.02)
    beam.rotation_euler = (0.0, -math.pi / 2, 0.0)  # pointing up, its head on top
    shards = Motes(14, lambda: vec(0.0, 0.9 + rng.uniform(-0.4, 0.5)),
                   lambda: vec(rng.uniform(-1.0, 1.0), rng.uniform(-0.5, 1.1)), MOTE_SIZE * 1.1, M["spark"], MOTE,
                   once=True)

    def anim(f, t):
        fresh()
        e = f - TP
        g = min(1.0, (f + 1) / 3.0)  # the pool spreads
        fade = 1.0 if e < 1 else max(0.0, 1.0 - e / 4.0)
        pool.scale = (g, 1.0, 0.14 * g)
        knob(pool, 0.8 + 0.5 * min(f, TP) / TP + (1.0 if e == 0 else 0.0), 0.8 * fade)
        if e < 0:  # the light gathering until it vanishes
            column.scale = (0.5 + 0.5 * f / TP, 1.0, min(1.0, (f + 1) / 4.0))
            knob(column, 0.6 + 0.6 * f / TP, 0.5 + 0.4 * f / TP)
            rising.at(f / 10.0)
            hide(beam)
            shards.hide()
            return
        column.scale = ((0.3, 0.08)[e] if e < 2 else 0.05, 1.0, 1.0)  # it narrows to a line
        knob(column, 2.0, 1.0 if e < 2 else 0.0)
        if e < 2:
            rising.at(f / 10.0, 1.0 - e / 2.0)
        else:
            rising.hide()
        head, ln = ((1.4, 1.3), (2.6, 1.9), (3.1, 1.4), (3.3, 0.8))[min(e, 3)]
        beam.location = (0.0, beam.location.y, head)
        beam.scale = (ln, 1.0, 1.0)
        knob(beam, 2.4, 1.0 if e < 2 else max(0.0, 1.0 - (e - 1) / 3.0), 0.5, 1.0)
        shards.at(min(0.999, e / 6.0), 1.0, 0.85)
    return meta, anim


def c_montee(variant):
    """A rising technique (it goes up with the fighter): grit kicked at its push off, a slash trail streaming down from
    its feet and, around it, a whirlwind turning up (spirale), a crescent sweeping up (croissant), great wings
    (ailes) or lightning (foudre)."""
    N = 16
    meta = cast_scene(400, 620, 200, 340, N)
    trail = ribbon("traine", [(0.0, -2.6 * i / 30) for i in range(31)],
                   [(0.3 if ELEMENT == "foudre" else 0.38) * drop(i / 30, 0.12) + 0.01 for i in range(31)],
                   M["flame"], 0.04, at=(0.0, 0.35))
    slash = ribbon("trait", [(0.0, -2.0 * i / 20) for i in range(21)], [0.035 * drop(i / 20, 0.1) + 0.003 for i in range(21)],
                   M["line"], 0.03, at=(0.0, 0.3))
    falling = Motes(12, lambda: vec(rng.uniform(-0.3, 0.3), rng.uniform(-0.2, 0.6)),
                    lambda: vec(rng.uniform(-0.4, 0.4), -rng.uniform(0.8, 1.8)), MOTE_SIZE, M["spark"], MOTE)
    flash = spikes("eclat", (0.0, 0.05), 0.55)
    grit = kick(0.0, 10, 0, (1, -1), 1.0)
    parts = []
    if variant == "ailes":  # seven feathers a side, fanned from over its shoulders
        light = ELEMENT == "lumiere"
        m_w = m_shape(0.75) if light else M["clean"]
        for side in (1, -1):
            for k, ln in enumerate((1.0, 1.3, 1.5, 1.55, 1.45, 1.25, 1.0)):
                if light:  # a leaf of light: pointed, widest past its middle
                    ob = fan("plume%d_%d" % (side, k), [(0.0, 0.0), (0.3 * ln, 0.077), (0.68 * ln, 0.12), (ln, 0.0),
                                                       (0.68 * ln, -0.072), (0.3 * ln, -0.048)], m_w,
                             0.02 + 0.002 * k, centre=(0.45 * ln, 0.0), at=(0.1 * side, 1.2))
                else:  # a feather of fire streaming out (long: a flame shows only its first half)
                    ob = ribbon("plume%d_%d" % (side, k), [(1.6 * ln * i / 12, 0.0) for i in range(13)],
                                [0.4 * math.sin(math.pi * min(1.0, 0.15 + i / 12)) ** 0.6 + 0.01 for i in range(13)],
                                m_w, 0.02 + 0.002 * k, at=(0.1 * side, 1.2))
                parts.append((ob, side, k))
    gusts = []
    if variant == "spirale":  # streaks of a whirlwind: (height, span, phase), uneven
        for j, zc in enumerate((0.1, 0.45, 0.72, 1.08, 1.36, 1.75)):
            gusts.append((zc, math.radians(rng.uniform(120, 170)), rng.uniform(0, 2 * math.pi), j // 2))

    def anim(f, t):
        fresh()
        grow = min(1.0, (f + 1) / 3.0)
        fade = 1.0 if f < 9 else max(0.0, 1.0 - (f - 8) / 6.0)
        trail.scale = (fade ** 0.5, 1.0, grow * (1.0 if f < 8 else max(0.1, 1.0 - (f - 7) / 7.0)))
        knob(trail, 1.0 + (0.4 if f < 2 else 0.0), fade * 0.85)
        slash.scale = (1.0, 1.0, grow * (1.0 if f < 6 else max(0.05, 1.0 - (f - 5) / 5.0)))
        knob(slash, 2.0, max(0.0, 1.0 - f / 9.0), 1.5, 1.0)
        falling.at(f / 8.0, 1.0 if f < 11 else max(0.0, 1.0 - (f - 10) / 4.0))
        grit.at(f, 1.2, 0.8)
        knob(flash, 2.2, (1.0, 0.4)[f] if f < 2 else 0.0, 1.0, 1.0)
        r = random.Random(f * 19 + 5)
        if ELEMENT == "foudre":  # bolts streaming down from its feet
            for j in range(3 if f < 4 else 2 if f < 9 else 1):
                if fade > 0:
                    bolt("bas%d_%d" % (f, j), (r.uniform(-0.1, 0.1), 0.25), (r.uniform(-0.5, 0.5), -2.4 * grow * fade), 5,
                         0.15, r, 1.0, fade)
        if variant == "spirale":  # a whirlwind: streaks turning up a cone around it, each a part of a turn
            for j, (zc, span0, ph0, born) in enumerate(gusts):
                age = f - born
                if age < 0 or fade <= 0:
                    continue
                rad = 0.3 + 0.28 * zc  # wider as it goes up
                ph, span, n = ph0 + 0.8 * f, span0 * min(1.0, (age + 1) / 2.0), 20
                pts, ws = [], []
                for i in range(n + 1):
                    a = ph - span * i / n
                    pts.append((rad * math.cos(a), zc + 0.24 * rad * math.sin(a) - 0.14 * i / n))
                    ws.append(0.1 * drop(i / n, 0.2, 0.9) * (0.7 - 0.3 * math.sin(a)) + 0.004)
                knob(dyn(ribbon("vent%d_%d" % (f, j), pts, ws, M["flame"], -0.02 - 0.002 * j)), 1.2, fade * 0.85)
                knob(dyn(ribbon("fil%d_%d" % (f, j), pts, [w * 0.25 + 0.003 for w in ws], M["line"], -0.03 - 0.002 * j)),
                     1.5, fade, 0.0, 0.9)
        elif variant == "croissant":  # a slash sweeping up in front, from low ahead to over the head: a trail thick
            # at the blade's tip, thinning to its tail, which then catches up with it as it fades
            sweep = min(1.0, (f + 1) / 3.0)
            a0 = math.radians(-60)
            a1 = a0 + math.radians(175) * sweep
            tail = min(a1 - math.radians(25), a0 + math.radians(175) * max(0.0, (f - 2) / 6.0))
            k = 1.0 if f < 3 else max(0.0, 1.0 - (f - 2) / 7.0)
            trail_w = lambda s: drop(s, 0.06, 1.0)
            if k > 0:
                R, w = 0.95 + 0.04 * f, 0.34 * k ** 0.5
                knob(dyn(arc("croissant%d" % f, M["clean"], (0.05, 0.85), R, a1, tail, w, -0.02, prof=trail_w)),
                     1.4 + (0.6 if f < 3 else 0.0), k)
                knob(dyn(arc("fil%d" % f, M["line"], (0.05, 0.85), R, a1, tail, 0.035, -0.025, bulge=0.5 * w,
                             prof=trail_w)), 1.6, k, 1.0, 1.0)
                if f >= 1 and a1 - 0.35 > tail + 0.2:  # its ghost, a frame behind
                    knob(dyn(arc("ombre%d" % f, M["line"], (0.05, 0.85), R - 0.12, a1 - 0.35, tail, 0.05, -0.015,
                                 prof=trail_w)), 1.0, 0.45 * k, 1.0, 0.6)
            if ELEMENT == "foudre" and f < 7:
                for j in range(2):
                    a = math.radians(r.uniform(-50, 100))
                    p = (0.05 + 1.0 * math.cos(a), 0.85 + 1.0 * math.sin(a))
                    bolt("arc%d_%d" % (f, j), p, (p[0] + r.uniform(-0.4, 0.4), p[1] + r.uniform(-0.4, 0.4)), 3, 0.3, r,
                         0.6)
        elif variant == "ailes":  # the wings open in three frames, beat once, then fade quickly (not dimming)
            spread = min(1.0, (f + 1) / 3.0) - (0.3 * math.sin(math.pi * (f - 5) / 6) if 5 <= f <= 11 else 0.0)
            wf = 1.0 if f < 10 else max(0.0, 1.0 - (f - 9) / 3.0)
            for ob, side, k in parts:
                a = math.radians(90 - (8 + (12 + 15 * k) * spread))  # from +x, toward its side; mirrored on the left
                ob.rotation_euler = (0.0, -side * a, 0.0)
                ob.scale = ((0.55 + 0.45 * spread) * side, 1.0, 1.0)
                knob(ob, (1.2 if ELEMENT == "lumiere" else 1.5) + (0.5 if f < 3 else 0.0), wf, 1.0, 0.8)
        elif variant == "foudre":  # bolts shooting up past its head as it springs, arcs crawling around its body
            if f < 6:
                for j in range(2):
                    x0 = r.uniform(-0.2, 0.2)
                    bolt("haut%d_%d" % (f, j), (x0, 0.4), (x0 + r.uniform(-0.35, 0.35), r.uniform(2.0, 2.6)), 4, 0.15,
                         r, 1.2 - 0.4 * j, 1.0 - f / 6.0)
            for j in range(3 if f < 10 else 0):
                p = (r.uniform(-0.3, 0.3), r.uniform(0.2, 1.4))
                bolt("corps%d_%d" % (f, j), p, (p[0] + r.uniform(-0.5, 0.5), p[1] + r.uniform(-0.4, 0.6)), 3, 0.3, r,
                     0.7, fade)
    return meta, anim


def c_colonne(path):
    """A rising technique leaving a column behind (left where it began): it grows from the ground to the fighter's
    feet as it rises (path: px up, per strip frame), then breaks: a pillar of ice (givre), an eruption of fire (feu)
    or a beam of light (lumiere)."""
    S = len(path) - 1  # the rise is over: the column breaks
    N = S + 8
    TOP = [p / 100 for p in path] + [path[-1] / 100] * 8
    H = path[-1] + 230
    meta = cast_scene(360, H, 180, H - 70, N)
    base = Motes(14, lambda: vec(rng.uniform(-0.25, 0.25), 0.03),
                 lambda: vec(rng.uniform(-1.0, 1.0), rng.uniform(0.2, 0.9)), MOTE_SIZE, M["spark"], MOTE, once=True)
    flash = spikes("eclat", (0.0, 0.12), 0.7)
    grit = kick(0.0, 10, 0, (1, -1), 1.0)
    rising = Motes(12, lambda: vec(rng.uniform(-0.25, 0.25), rng.uniform(0.0, 1.0)),
                   lambda: vec(rng.uniform(-0.1, 0.1), rng.uniform(1.0, 2.5)), MOTE_SIZE, M["spark"], MOTE)
    spurs, bits, splash = [], None, []
    if ELEMENT == "givre":
        m_ice = m_shape(0.45)
        for k in range(6):  # spikes around its foot, leaning out
            side = 1 if k % 2 else -1
            h = rng.uniform(0.35, 0.9)
            w = h * rng.uniform(0.16, 0.22)
            ob = fan("pic%d" % k, [(0.0, h), (w, h * 0.55), (w * 0.8, 0.0), (-w * 0.8, 0.0), (-w, h * 0.5)], m_ice,
                     -0.01 - 0.002 * k, centre=(0.0, h * 0.4), at=(side * rng.uniform(0.15, 0.4), 0.0))
            ob.rotation_euler = (0.0, side * math.radians(rng.uniform(15, 40)), 0.0)
            spurs.append((ob, k // 2))
        bits = Sparks([(vec(rng.uniform(-0.25, 0.25), rng.uniform(0.1, 1.0) * TOP[S]),
                        vec(rng.choice((1, -1)) * rng.uniform(0.4, 1.1), rng.uniform(-0.7, 0.3)), S + rng.randint(0, 1))
                       for _ in range(20)], life=7, size=0.15, shape="shard", mat=m_ice)
        burst = spikes("bris", (0.0, TOP[S] * 0.5), 0.6)  # the flash of its break
    elif ELEMENT == "feu":
        col = []  # a pillar of several flames (not a cone): (x, lean, width, height share)
        for j, (dx, lean, w, hs) in enumerate(((0.0, 0.0, 0.8, 1.0), (-0.2, -0.18, 0.5, 0.72), (0.22, 0.2, 0.5, 0.8),
                                               (-0.08, 0.12, 0.42, 0.9), (0.1, -0.1, 0.42, 0.62))):
            col.append((ribbon("colonne%d" % j, [(dx + lean * (i / 30) ** 2, i / 30) for i in range(31)],
                               [w * (1 - 0.3 * i / 30) for i in range(31)], M["flame"], 0.03 + 0.002 * j), hs))
        core = ribbon("coeur", [(0.0, i / 30) for i in range(31)], [0.38 * (1 - 0.3 * i / 30) for i in range(31)],
                      m_flame(rho=0.2, ka=8.0), 0.02)
        for k in range(6):  # lava splashing out at its foot
            splash.append(tongue("gerbe%d" % k, M["flame"], 0.0, rng.uniform(0.3, 0.6), 0.2,
                                 (0.5 if k % 2 else -0.5) * rng.uniform(0.4, 1.0), 0.01 + 0.002 * k))
        crack = curve("faille", jag((-0.9, 0.02), (0.9, 0.02), 5, 0.05, random.Random(3)), 0.012, M["core"], -0.02)
    else:
        beam = ribbon("rayon", [(0.0, i / 20) for i in range(21)], [0.8] * 21, m_beam(0.45), 0.03)  # it fades out up
        core = ribbon("ame", [(0.0, i / 20) for i in range(21)], [0.1] * 21, M["line"], 0.02)
        pool = disc("source", 0.7, M["glow"], 0.04)

    def anim(f, t):
        fresh()
        top = TOP[min(f, S)]
        k = 1.0 if f < S else max(0.0, 1.0 - (f - S) / 6.0)
        base.at(min(0.999, f / 9.0), 1.0, 0.85)
        knob(flash, 2.2, (1.0, 0.4)[f] if f < 2 else 0.0, 1.0, 1.0)
        grit.at(f, 1.2, 0.8)
        if ELEMENT == "givre":
            if f < S:
                knob(dyn(fan("pilier%d" % f, [(0.0, top + 0.2), (0.27, top), (0.3, 0.0), (-0.3, 0.0), (-0.27, top)],
                             m_ice, 0.0, centre=(0.0, top * 0.5 + 0.05))), 1.0 + (0.8 if f == 0 else 0.0), 1.0, 1.0,
                     0.6)
                for j, x0 in enumerate((-0.1, 0.12)):  # its facets
                    knob(dyn(ribbon("facette%d_%d" % (f, j), [(x0, 0.0), (x0 * 0.8, top + 0.05)], [0.02, 0.012],
                                    M["line"], -0.01)), 1.2, 0.6, 0.0, 0.9)
                z = (0.5 * f) % max(0.5, top)  # a glint running up the ice
                knob(dyn(ribbon("reflet%d" % f, [(-0.27, z), (0.27, z + 0.3)], [0.03, 0.03], M["line"], -0.02)), 1.8,
                     0.9, 0.0, 1.0)
            knob(burst, 2.0, (1.0, 0.4)[f - S] if 0 <= f - S < 2 else 0.0, 1.0, 1.0)
            for ob, born in spurs:
                age = f - born
                if age < 0:
                    hide(ob)
                    continue
                ob.scale = (1.0, 1.0, min(1.0, (age + 1) / 2.0) if f < S else max(0.05, k))
                knob(ob, 1.0 + (0.8 if age == 0 else 0.0), k, 1.0, 0.6)
            bits.at(f, 1.0, 0.7)
        elif ELEMENT == "feu":
            for ob, hs in col:
                ob.scale = (k ** 0.5, 1.0, max(0.05, (top + 0.35) * hs))
                knob(ob, 1.0 + (0.5 if f < 2 else 0.0), k)
            core.scale = (k, 1.0, max(0.05, top))
            knob(core, 1.4, k)
            for ob in splash:
                ob.scale = (1.0, 1.0, max(0.05, min(1.0, (f + 1) / 2.0) * max(0.0, 1.0 - f / 9.0)))
                knob(ob, 1.2, max(0.0, 1.0 - f / 9.0))
            knob(crack, 1.0, max(0.0, 1.0 - f / 12.0), 1.0, max(0.2, 1.0 - f / 9.0))
            rising.at(f / 10.0, k)
        else:
            beam.scale = (k ** 0.7, 1.0, top + 1.2)
            knob(beam, 1.0 + (0.6 if f < 2 else 0.0), k)
            core.scale = (k, 1.0, top + 1.0)
            knob(core, 1.6, k, 0.6, 1.0)
            pool.scale = (1.0 + 0.1 * min(f, 4), 1.0, 0.14)
            knob(pool, 1.2, 0.8 * k)
            rising.at(f / 10.0, k)
    return meta, anim


def c_contre(variant):
    """A counter's guard (it stays on the fighter for its window, frames 0-10): a gleaming crescent in front (garde),
    a wall of ice shards (miroir) or a bubble like glass (dome); it opens with a glint and a shock, then fades (the
    ice shatters)."""
    N = 14
    meta = cast_scene(380, 320, 190, 265, N)
    ping = fan("eclat", star(4, 0.2, 0.03), M["flash"], -0.03, at=(0.8, 0.95))
    motes = Motes(8, lambda: vec(rng.uniform(-0.6, 0.6), rng.uniform(0.0, 0.5)),
                  lambda: vec(rng.uniform(-0.2, 0.2), rng.uniform(0.6, 1.2)), MOTE_SIZE, M["spark"], MOTE)
    if variant == "garde":
        guard = arc("garde", m_shape(0.5), (0.05, 0.85), 0.75, math.radians(70), math.radians(-70), 0.16)
        edge = arc("fil", M["line"], (0.05, 0.85), 0.75, math.radians(70), math.radians(-70), 0.03, bulge=0.08)
    elif variant == "miroir":  # a wall of ice shards in front
        m_ice = m_shape(0.4)
        shards = []
        for k, h in enumerate((1.1, 1.5, 1.8, 1.6, 1.25, 0.9)):
            w = rng.uniform(0.12, 0.17)
            ob = fan("glace%d" % k, [(0.0, h), (w, h * 0.8), (w * 0.85, 0.0), (-w * 0.85, 0.0), (-w, h * 0.75)], m_ice,
                     -0.01 - 0.002 * k, centre=(0.0, h * 0.45), at=(0.36 + 0.075 * k + rng.uniform(-0.02, 0.02), 0.0))
            ob.rotation_euler = (0.0, math.radians(rng.uniform(-8, 8)), 0.0)
            shards.append((ob, k // 2))
        glint = ribbon("reflet", [(0.28, 0.0), (0.8, 0.35)], [0.03, 0.03], M["line"], -0.03)
        burst = spikes("bris", (0.55, 0.9), 0.6)  # the flash as it shatters
        bits = Sparks([(vec(rng.uniform(0.3, 0.8), rng.uniform(0.1, 1.6)), vec(rng.uniform(0.2, 1.0),
                                                                                 rng.uniform(-0.8, 0.5)),
                        10 + rng.randint(0, 1)) for _ in range(16)], life=5, size=0.13, shape="shard", mat=m_ice)
    else:  # a bubble like glass: clear in its middle, its rim bright, a shine at its top left, glints drifting on it
        rim = disc("dome", 1.05, m_bubble(), 0.01, at=(0.0, 0.85))
        stars = [(fan("astre%d" % k, star(4, 0.06, 0.015), m_shape(0.8), -0.02), rng.uniform(0, 2 * math.pi),
                  rng.uniform(0.85, 1.0), rng.uniform(-0.15, 0.15)) for k in range(5)]
        burst = Sparks([(vec(1.05 * math.cos(a), 0.85 + 1.05 * math.sin(a)),
                         vec(rng.uniform(0.3, 0.7) * math.cos(a), rng.uniform(0.3, 0.7) * math.sin(a) - 0.3),
                         10 + rng.randint(0, 1)) for a in (rng.uniform(0, 2 * math.pi) for _ in range(12))],
                       life=5, shape="shard" if ELEMENT == "givre" else None)

    def anim(f, t):
        fresh()
        open_ = min(1.0, (f + 1) / 2.0)
        fade = 1.0 if f < 10 else max(0.0, 1.0 - (f - 9) / 4.0)
        shimmer = 1.0 + 0.15 * math.sin(f * 1.7)
        ping.scale = (0.6 + 0.6 * f, 1.0, 0.6 + 0.6 * f)
        ping.rotation_euler = (0.0, -0.3 * f, 0.0)
        knob(ping, 2.2, (1.0, 0.6, 0.25)[f] if f < 3 else 0.0, 1.0, 1.0)
        shocks("onde", f, (0.05, 0.85), [(0, 0, 80), (1, 0, 60)], 0.5, 1.15, 4, 0.04)
        motes.at(f / 10.0, fade)
        if variant == "garde":
            knob(guard, (2.0 if f < 1 else 1.0) * shimmer, fade * 0.9, 0.35, 0.5)
            knob(edge, (2.0 if f < 1 else 1.0) * shimmer, fade, 1.0, 0.5)
            if f < 6:  # a gleam running up its edge
                a = math.radians(-70 + 140 * f / 5)
                knob(dyn(arc("lueur%d" % f, M["line"], (0.05, 0.85), 0.83, a - 0.25, a + 0.25, 0.05, -0.03)), 2.4,
                     1.0, 0.0, 1.0)
        elif variant == "miroir":
            for ob, born in shards:
                age = f - born
                if age < 0 or f >= 10:
                    hide(ob)
                    continue
                ob.scale = (1.0, 1.0, min(1.0, (age + 1) / 2.0))
                knob(ob, (1.0 + (0.8 if age == 0 else 0.0)) * shimmer, 1.0, 1.0, 0.6)
            glint.location = (0.0, glint.location.y, 0.15 * f - 0.2)
            knob(glint, 1.8, 0.9 if 0 <= 0.15 * f - 0.2 < 1.4 and f < 10 else 0.0, 0.0, 1.0)
            bits.at(f, 1.0, 0.7)
            knob(burst, 2.0, (1.0, 0.4)[f - 10] if 0 <= f - 10 < 2 else 0.0, 1.0, 1.0)
        else:
            s = (0.3 + 0.7 * open_) * (1.0 + 0.03 * max(0, f - 9))
            rim.scale = (s, 1.0, s)
            knob(rim, (1.6 if f < 1 else 0.9) * shimmer, fade)
            knob(dyn(arc("reflet%d" % f, M["line"], (0.0, 0.85), 0.86 * s, math.radians(165), math.radians(105), 0.045,
                         -0.03)), 1.6, 0.8 * fade, 0.0, 1.0)
            for j, (ob, a0, d, turn) in enumerate(stars):  # each glint drifts on the rim and twinkles
                a = a0 + turn * f
                ob.location = (d * s * math.cos(a), -0.02, 0.85 + d * s * math.sin(a))
                tw = 0.5 + 0.5 * math.sin(1.9 * f + 2.0 * j)
                knob(ob, 0.3 + 1.2 * tw, fade * (0.4 + 0.6 * tw), 1.0, 0.9)
            burst.at(f)
        if ELEMENT == "foudre" and f < 11:
            r = random.Random(f * 23 + 3)
            for j in range(2):
                a = math.radians(r.uniform(-65, 65))
                p = (0.05 + 0.8 * math.cos(a), 0.85 + 0.8 * math.sin(a))
                bolt("arc%d_%d" % (f, j), p, (p[0] + r.uniform(-0.1, 0.4), p[1] + r.uniform(-0.35, 0.35)), 3, 0.3, r,
                     0.6, fade)
    return meta, anim


def c_eruption(variant, R=120, HIT=2):
    """A burst around the fighter, R px wide, striking on strip frame HIT: fire bursting up from the ground
    (eruption), a wave of flames running out along a crack (seisme), a whirl of fire turning around it then
    bursting out (cercle), or an electric discharge (decharge). Before HIT, a whirl flares on its light hits (strip
    frames 1, 3, 5)."""
    N = HIT + 14
    Ru, C = R / 100, (0.0, 1.0)
    meta = cast_scene(2 * R + 240, 440, R + 120, 360, N)
    low = variant in ("eruption", "seisme")  # it comes out of the ground
    O = (0.0, 0.15) if low else C
    core = disc("eclat", 0.25 * Ru, M["glow"], 0.0, at=O)
    star_ = spikes("etoile", O, 0.8 * Ru)
    rays = []
    if variant == "eruption":  # fire bursting up from the ground: tall in the middle, lower to the sides
        for k in range(14):
            up = k < 7
            a = math.radians(90 + (rng.uniform(-30, 30) if up else rng.choice((-1, 1)) * rng.uniform(35, 70)))
            ln = Ru * (rng.uniform(1.0, 1.5) if up else rng.uniform(0.55, 0.85))
            w = rng.uniform(0.32, 0.48)
            ob = ribbon("jet%d" % k, [(ln * i / 16, 0.0) for i in range(17)],
                        [w * drop(i / 16, 0.1, 0.6) + 0.02 for i in range(17)], M["flame"], 0.01 + 0.002 * k,
                        at=(rng.uniform(-0.15, 0.15), 0.05))
            ob.rotation_euler = (0.0, -a, 0.0)
            rays.append(ob)
    embers = Motes(20, lambda: vec(rng.uniform(-0.2, 0.2), O[1] + rng.uniform(-0.1, 0.3)),
                   lambda: vec(rng.uniform(-1.6, 1.6) * Ru, rng.uniform(0.2, 1.8) * Ru), MOTE_SIZE, M["spark"], MOTE,
                   once=True)
    smoke = []
    if SMOKE and low:  # smoke rising after the fire
        for k in range(5):
            smoke.append(tongue("fumee%d" % k, M["flame"], rng.uniform(-0.6, 0.6) * Ru, rng.uniform(0.8, 1.3), 0.35,
                                rng.uniform(-0.2, 0.2), 0.05 + 0.002 * k))
    walls, crack = [], None
    if variant == "seisme":  # great flames bursting one after the other along the crack, outward (a flame shows
        for side in (1, -1):  # only its first half: they are drawn twice as tall as they look)
            for k in range(4):
                h = (1.9 - 0.3 * k) * rng.uniform(0.8, 1.15)
                walls.append((tongue("mur%d_%d" % (side, k), M["flame"], side * (0.25 + 0.3 * k) * Ru, h,
                                     0.36 + 0.12 * h, -0.2 * side * rng.uniform(0.5, 1.2), 0.01 + 0.002 * k), k))
        crack = curve("faille", jag((-1.3 * Ru, 0.02), (1.3 * Ru, 0.02), 6, 0.04, random.Random(5)), 0.01, M["core"],
                      -0.02)
        rocks = Sparks([(vec(rng.uniform(-1.0, 1.0) * Ru, 0.05), vec(rng.uniform(-0.5, 0.5), rng.uniform(0.6, 1.4)),
                         HIT + rng.randint(0, 3)) for _ in range(14)], life=8, size=0.06, shape="shard",
                       mat=m_dark())
    elif variant == "cercle":  # flames left burning at its feet when the whirl bursts, outward
        for side in (1, -1):
            for k in range(3):
                h = (1.1 - 0.2 * k) * rng.uniform(0.85, 1.15)
                walls.append((tongue("mur%d_%d" % (side, k), M["flame"], side * (0.15 + 0.3 * k) * Ru, h,
                                     0.3 + 0.1 * h, -0.15 * side, 0.01 + 0.002 * k), k))
    air = [(HIT, rng.uniform(0, 360), rng.uniform(25, 45)) for _ in range(3)]  # broken short, not rings
    air += [(HIT + 1, rng.uniform(0, 360), rng.uniform(20, 35)) for _ in range(2)]
    grit = kick(0.0, 12, HIT, (1, -1), 1.3 * Ru)

    def whirl(f, g, rad, alpha, boost, span=None):
        """Four flames running around it at four heights, each a long streak with a bright thread."""
        for j, zc in enumerate((0.2, 0.6, 1.0, 1.4)):
            ph, span, n = 1.3 * f + 1.6 * j, span or math.radians(150 + 60 * g), 24
            pts, ws = [], []
            for i in range(n + 1):
                a = ph - span * i / n
                pts.append((rad * math.cos(a), zc + 0.22 * rad * math.sin(a)))
                ws.append((0.22 + 0.16 * g) * boost * drop(i / n, 0.12, 0.9) * (0.75 - 0.25 * math.sin(a)) + 0.008)
            knob(dyn(ribbon("tourbillon%d_%d" % (f, j), pts, ws, M["clean"], -0.02 - 0.002 * j)),
                 1.2 + 0.6 * (boost - 1.0), alpha)
            knob(dyn(ribbon("fil%d_%d" % (f, j), pts, [w * 0.3 + 0.005 for w in ws], M["line"], -0.03 - 0.002 * j)),
                 1.5, alpha, 1.0, 0.9)

    def anim(f, t):
        fresh()
        e = f - HIT
        r = random.Random(f * 37 + 9)
        grit.at(f, 1.2, 0.8)
        light = HIT > 2 and f in (1, 3, 5)  # a light hit of the whirl
        if e < 0:  # the gathering: a whirl of fire turning around it, or arcs crawling in
            hide(core, star_, *rays, *[w for w, _ in walls], *smoke)
            embers.hide()
            if crack:
                hide(crack)
            if variant == "cercle":
                g = (f + 1) / HIT
                whirl(f, g, (0.6 + 0.25 * g) * Ru, min(1.0, 0.5 + g), 1.3 if light else 1.0)
            if variant == "decharge":
                for j in range(5 if light else 3):
                    a = r.uniform(0, 2 * math.pi)
                    d = Ru * r.uniform(0.6, 1.0)
                    bolt("arc%d_%d" % (f, j), (C[0] + d * math.cos(a), C[1] + d * math.sin(a)),
                         (C[0] + 0.1 * math.cos(a), C[1] + 0.1 * math.sin(a)), 4, 0.25, r, 0.8 if light else 0.6)
            return
        k = max(0.0, 1.0 - e / 10.0)
        knob(core, 2.0, 1.0 if e == 0 else 0.0)
        star_.scale = (1.0 + 0.3 * e, 1.0, 1.0 + 0.3 * e)
        knob(star_, 2.4, (1.0, 0.4)[e] if e < 2 else 0.0, 1.0, 1.0)
        for ob in rays:
            ob.scale = (0.55 if e == 0 else 1.0 + 0.05 * (e - 1), 1.0, 1.0 + 0.06 * e)
            knob(ob, 1.0 + (0.6 if e == 0 else 0.0), k)
        embers.at(min(0.999, e / 11.0), 1.0, 0.85)
        for j, ob in enumerate(smoke):
            age = e - 2 - j % 3
            if age < 0:
                hide(ob)
                continue
            ob.location = (ob.location.x, ob.location.y, 0.08 * age)
            ob.scale = (1.0 + 0.1 * age, 1.0, min(1.0, (age + 1) / 3.0))
            knob(ob, 0.5, 0.7 * max(0.0, 1.0 - age / 8.0))
        if variant == "cercle" and e < 4:  # the whirl flung out, its flames shortening as they fly
            whirl(f, 1.0, (0.85 + 0.25 * e) * Ru, 1.0 - e / 4.0, max(0.3, 1.0 - 0.2 * e), math.radians(170 - 30 * e))
        if variant == "decharge":
            shocks("air", f, C, air, 0.35 * Ru, 1.1 * Ru, 4, 0.05)
        if crack:
            crack.scale = (min(1.0, (e + 1) / 3.0), 1.0, 1.0)
            knob(crack, 1.0, max(0.0, 1.0 - e / 12.0), 1.0, max(0.2, 1.0 - e / 8.0))
            rocks.at(f, 1.0, 0.3)
        for ob, j in walls:  # each pillar bursts a frame after the one nearer to it, then dies down
            age = e - j
            if age < 0:
                hide(ob)
                continue
            ob.scale = (1.0, 1.0, max(0.05, min(1.0, (age + 1) / 2.0) * max(0.0, 1.0 - age / 9.0) ** 0.6))
            knob(ob, 1.2 + (0.6 if age == 0 else 0.0), max(0.0, 1.0 - age / 9.0))
        if variant == "decharge" and e < 6:
            for j in range(10 if e < 2 else 4):
                a = r.uniform(0, 2 * math.pi)
                d = Ru * r.uniform(0.8, 1.15)
                bolt("eclair%d_%d" % (f, j), C, (C[0] + d * math.cos(a), C[1] + d * math.sin(a)), 5, 0.2, r, 1.0, k)
    return meta, anim


# Where each dash takes its fighter (px ahead of its start, one value per strip frame), measured in the game.
PATHS = {
    "p01_a_s": [0, 57, 114, 148, 159, 169, 179, 189, 198, 207, 215, 223, 231],
    "p01_b_s": [0, 47, 94, 140, 186, 231, 276, 321, 365, 409, 453, 478, 486, 494, 501, 508, 515, 521, 527, 532, 537],
    "p02_x_s": [0, 46, 91, 117, 126, 134, 141, 148, 155, 162, 167, 173, 178],
    "p02_a_s": [0, 53, 105, 157, 208, 259, 309, 339, 349, 358, 366, 375, 382, 390, 397, 403, 409],
    "p03_x_s": [0, 45, 90, 117, 125, 133, 141, 148, 155, 162, 168, 173, 179],
    "p03_a_s": [0, 42, 84, 108, 116, 123, 130, 137, 143, 149, 155, 160, 164],
    "p03_b_s": [0, 49, 97, 144, 192, 239, 285, 332, 377, 405, 413, 421, 429, 437, 444, 450, 457, 463, 468],
    "p07_a_s": [0, 67, 133, 173, 185, 198, 209, 221, 232, 242, 252, 262, 271],
    "p07_b_s": [0, 56, 112, 167, 222, 276, 330, 363, 373, 383, 392, 401, 409, 417, 425, 432, 439],
    "p08_x_s": [0, 50, 101, 130, 140, 149, 157, 166, 173, 181, 188, 194, 201],
    "p08_a_s": [0, 61, 121, 157, 168, 179, 190, 200, 210, 220, 229, 237, 246],
    "p08_b_s": [0, 50, 101, 150, 200, 248, 297, 326, 335, 343, 352, 360, 367, 374, 381, 387, 393],
    # a column's rise: how high the fighter is (px above its start)
    "p02_x_u": [0, 52, 101, 147, 191, 232, 271, 307, 340, 371, 399, 425, 448, 468, 486, 501, 513, 523],
    "p03_x_u": [0, 46, 89, 129, 167, 202, 235, 265, 292, 317, 339, 358, 375, 389, 401, 410, 416, 420, 421],
    "p08_b_u": [0, 51, 100, 146, 189, 230, 268, 304, 337, 368, 396, 421, 444, 464, 482, 497, 509, 519],
}
CASTS = dict(ruee=c_ruee, passage=c_passage, clin=c_clin, montee=c_montee, colonne=c_colonne, contre=c_contre,
             eruption=c_eruption)
# Each technique (fighter, x / a / b for bare hands / its weapons, s / u / d for its side, up, down skill): its recipe.
# A dash's path is where the fighter is on each strip frame (px ahead), measured in the game.
MOVES = {
    ("p01", "x_s"): ("passage", {}), ("p01", "x_u"): ("clin", {}), ("p01", "x_d"): ("contre", dict(variant="garde")),
    ("p01", "a_s"): ("ruee", dict(path=PATHS["p01_a_s"])), ("p01", "a_u"): ("montee", dict(variant="croissant")),
    ("p01", "a_d"): ("contre", dict(variant="garde")),
    ("p01", "b_s"): ("ruee", dict(path=PATHS["p01_b_s"])), ("p01", "b_u"): ("montee", dict(variant="spirale")),
    ("p02", "x_s"): ("ruee", dict(path=PATHS["p02_x_s"])), ("p02", "x_u"): ("colonne", dict(path=PATHS["p02_x_u"])),
    ("p02", "x_d"): ("contre", dict(variant="miroir")),
    ("p02", "a_s"): ("ruee", dict(path=PATHS["p02_a_s"])), ("p02", "a_u"): ("montee", dict(variant="spirale")),
    ("p02", "b_u"): ("clin", {}), ("p02", "b_d"): ("contre", dict(variant="dome")),
    ("p03", "x_s"): ("ruee", dict(path=PATHS["p03_x_s"])), ("p03", "x_u"): ("colonne", dict(path=PATHS["p03_x_u"])),
    ("p03", "x_d"): ("eruption", dict(variant="eruption", R=119, HIT=2)),
    ("p03", "a_s"): ("ruee", dict(path=PATHS["p03_a_s"])), ("p03", "a_u"): ("montee", dict(variant="croissant")),
    ("p03", "a_d"): ("eruption", dict(variant="seisme", R=136, HIT=2)),
    ("p03", "b_s"): ("ruee", dict(path=PATHS["p03_b_s"])), ("p03", "b_u"): ("montee", dict(variant="ailes")),
    ("p03", "b_d"): ("eruption", dict(variant="cercle", R=136, HIT=8)),
    ("p07", "x_s"): ("passage", dict(D=266)), ("p07", "x_u"): ("montee", dict(variant="foudre")),
    ("p07", "x_d"): ("eruption", dict(variant="decharge", R=119, HIT=8)),
    ("p07", "a_s"): ("ruee", dict(path=PATHS["p07_a_s"])), ("p07", "a_u"): ("montee", dict(variant="croissant")),
    ("p07", "a_d"): ("contre", dict(variant="garde")),
    ("p07", "b_s"): ("ruee", dict(path=PATHS["p07_b_s"])), ("p07", "b_u"): ("montee", dict(variant="spirale")),
    ("p08", "x_s"): ("ruee", dict(path=PATHS["p08_x_s"])), ("p08", "x_u"): ("montee", dict(variant="ailes")),
    ("p08", "x_d"): ("contre", dict(variant="dome")),
    ("p08", "a_s"): ("ruee", dict(path=PATHS["p08_a_s"])), ("p08", "a_u"): ("montee", dict(variant="croissant")),
    ("p08", "a_d"): ("contre", dict(variant="garde")),
    ("p08", "b_s"): ("ruee", dict(path=PATHS["p08_b_s"])), ("p08", "b_u"): ("colonne", dict(path=PATHS["p08_b_u"])),
}


def r_bulle():
    """The shield (yoyo, 13/11: the counter's glass bubble instead of the old ParticleFX one), held as long as the
    fighter guards: the « dome » of c_contre, looping. Centred on the anchor, its rim 100 px out (the game scales it)."""
    meta = cast_scene(280, 280, 140.0, 140.0, 24)
    meta.update(FPS=20, LOOP=1)
    rim = disc("dome", 1.0, m_bubble(), 0.01)
    stars = [(fan("astre%d" % k, star(4, 0.06, 0.015), m_shape(0.8), -0.02), rng.uniform(0, 2 * math.pi),
              rng.uniform(0.15, 0.6), rng.choice((-1, 1))) for k in range(6)]

    def anim(f, t):
        fresh()
        w = 2 * math.pi * t
        s = 1.0 + 0.012 * math.sin(2 * w)  # it breathes
        rim.scale = (s, 1.0, s)
        knob(rim, 0.9 * (1.0 + 0.1 * math.sin(3 * w)), 1.0)
        knob(dyn(arc("reflet", M["line"], (0.0, 0.0), 0.78 * s, math.radians(165), math.radians(105), 0.045, -0.03)),
             1.6, 0.75 + 0.1 * math.sin(w), 0.0, 1.0)
        for k in range(2):  # two gleams running round the rim, thin and broken
            a = w * (1 if k == 0 else -1) + math.pi * k
            knob(dyn(arc("lueur%d" % k, M["line"], (0.0, 0.0), 0.99 * s, a - 0.3, a + 0.3, 0.02, -0.03)), 1.4, 0.5,
                 0.0, 1.0)
        for j, (ob, a0, d, turn) in enumerate(stars):  # glints drifting inside it, twinkling (whole turns: a loop)
            a = a0 + turn * w
            ob.location = (d * s * math.cos(a), -0.02, d * s * math.sin(a))
            tw = 0.5 + 0.5 * math.sin(2 * w * (1 + j % 2) + 2.0 * j)
            knob(ob, 0.3 + 1.2 * tw, 0.4 + 0.6 * tw, 1.0, 0.9)

    return meta, anim


RECIPES = dict(bulle=r_bulle, orbe=r_orbe, meteore=r_meteore, lance=r_lance, eclair=r_eclair, etoile=r_etoile, dague=r_dague,
               plume=r_plume, marque=r_marque, pilier=r_pilier)
if CAST:
    kind, opts = MOVES[(STYLE, SECOND)]
    meta, anim = CASTS[kind](**opts)
else:
    meta, anim = RECIPES[KIND]()
os.makedirs(OUT, exist_ok=True)
FRAMES = [int(x) for x in os.environ.get("CH_FRAMES", "").split(",") if x.strip()] or range(meta["N"])
for f in FRAMES:
    t = f / meta["N"]
    for c in CLOCK:
        c.outputs[0].default_value = t
    anim(f, t)
    sc.render.filepath = os.path.join(OUT, "f%02d.exr" % f)
    bpy.ops.render.render(write_still=True)
    print("IMAGE", f, flush=True)
print("FXLINE fx pfx_%s_%s %d %d %d %.1f %.1f %d %d %d 0 1" % (STYLE, SECOND if CAST else KIND, meta["N"], meta["W"],
                                                             meta["H"], meta["AX"],
                                                             meta["AY"], meta["FPS"], meta["LOOP"], meta["RH"]))
print("FIN", OUT)
