# Clash Horizon - the fighters' projectiles in Blender (yoyo, 12/11: every effect of every fighter made in Blender).
# One recipe per kind, coloured per fighter: orbe (an orb with its trail), meteore (a burning rock), lance (a javelin
# of energy), eclair (a bolt), etoile (a shooting star), dague (a spinning spectral dagger), plume (a feather of
# light), marque (a sigil left as a trap), pilier (a column out of the ground, played once).
# Flying ones loop on N frames (the game cycles them), flying right; the hit circle (RH px) is centred on the anchor.
# Usage: blender -b -P effets.py -- <kind>-<fighter> <out_dir> [samples]     e.g. orbe-p01
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
KIND, STYLE = RECIPE.split("-")
PALETTES = {  # deep, main, hot, core, smoke (None: no smoke)
    "p01": ("#3B1772", "#8A4DFF", "#C9A2FF", "#F7EEFF", "#140A20"),
    "p02": ("#1E5C9E", "#5CC8F0", "#D8F6FF", "#FFFFFF", None),
    "p03": ("#B3170E", "#FF5A1F", "#FFC86E", "#FFF7E2", "#1E0F0B"),
    "p07": ("#7E0A18", "#FF2D3D", "#FF7F8C", "#FFF1F2", None),
    "p08": ("#C98F28", "#FFD45A", "#FFF3BF", "#FFFFFF", None),
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


def m_flame(rho=0.2, ka=8.0, pale=False):
    """A ribbon of flame or energy streaming toward its end (u: along, 0 at its head; v: across)."""
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
    stops = [(0.0, SMOKE or DEEP), (0.3, DEEP), (0.55, MAIN), (0.85, HOT), (1.0, CORE)]
    light = nd.add(nd.mul(nd.math('POWER', heat, 2.0), 2.5), 0.12 if SMOKE else 0.45)
    if not SMOKE:  # no smoke: the far end fades to nothing instead of darkening
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
            name = "%s%d" % (shape, len(sc.collection.objects))
            if shape == "star":
                ob = fan(name, star(4, size, size * 0.22), mat, y)
            elif shape == "dot":
                ob = disc(name, size, mat, y, rings=2, segs=10)
            else:
                ob = kite(name, size * rng.uniform(0.6, 1.4), size * 0.16, mat, y)
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
    else:  # a column of light: soft edges, a white core, fading at its top
        nd = Nodes("Faisceau")
        u, v = nd.uv()
        S, A, Wk = nd.knobs()
        mid = nd.math('SUBTRACT', 1.0, nd.math('ABSOLUTE', nd.math('SUBTRACT', nd.mul(v, 2.0), 1.0)))
        t = nd.clock()
        ripple = nd.loop_noise(v, u, t, 3.0, 2.0, 2.0, detail=2.0, seed=3.0, rho=0.3)
        x = nd.mul(nd.math('POWER', mid, 0.8), nd.add(0.75, nd.mul(ripple, 0.4)))
        beam = nd.surface(nd.ramp(x, RAMP), nd.mul(S, nd.add(0.3, nd.mul(nd.math('POWER', x, 3.0), 4.0))),
                          nd.mul(nd.mul(A, nd.smooth(x, 0.05, 0.5)), nd.math('SUBTRACT', 1.0, nd.smooth(u, 0.7, 1.0))))
        cols.append(ribbon("colonne", [(0.0, TOP * i / 30) for i in range(31)], [0.8] * 31, beam, 0.02))
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


RECIPES = dict(orbe=r_orbe, meteore=r_meteore, lance=r_lance, eclair=r_eclair, etoile=r_etoile, dague=r_dague,
               plume=r_plume, marque=r_marque, pilier=r_pilier)
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
print("FXLINE fx pfx_%s_%s %d %d %d %.1f %.1f %d %d %d 0 1" % (STYLE, KIND, meta["N"], meta["W"], meta["H"], meta["AX"],
                                                             meta["AY"], meta["FPS"], meta["LOOP"], meta["RH"]))
print("FIN", OUT)
