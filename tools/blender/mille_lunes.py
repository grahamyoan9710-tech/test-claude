# Clash Horizon - 01's odachi ultimate « MILLE LUNES » (yoyo, 12/11: the space around the opponent cut again and
# again, "à la Vergil mais en violet"; an original look: thin crescent-moon cuts inside a violet moon that shatters).
# Two layers the game plays together: back (the rift, behind the fighters) and front (the cuts, the flash, the
# shards, over them). N frames at 30 fps from the teleport (game frame 34): cuts in time with the 7 blows, the
# launcher (frame 24) breaks it all.
# Usage: blender -b -P mille_lunes.py -- <back|front> <out_dir> [samples]
# Writes out_dir/fNN.exr: linear, premultiplied RGBA on a transparent film (post2.py adds the glow).
import math
import os
import random
import sys

import bpy
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else sys.argv[1:]
LAYER, OUT = argv[0], argv[1]
SAMPLES = int(argv[2]) if len(argv) > 2 else 24
N = 40
W = H = 520  # px, the sphere's centre in the middle (1 unit = 100 px)
RS = 1.2  # its radius
HITS = [4.5 + 2.5 * k for k in range(7)]  # the 7 blows (game frames 43, 48 ... 73)
BREAK = 24  # the launcher (game frame 82)
WHITE, HOT, MAIN, DEEP, DARK = "#FFFFFF", "#E2CCFF", "#9A5CF0", "#4A1F9E", "#12081F"
rng = random.Random(1000)


def lin(c):
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def rgba(h):
    h = h.lstrip("#")
    return (*(lin(int(h[i:i + 2], 16) / 255) for i in (0, 2, 4)), 1.0)


# --- scene -----------------------------------------------------------------------------------------------
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
co.location = (0.0, -10.0, 0.0)
co.rotation_euler = (math.pi / 2, 0.0, 0.0)  # looks along +y: x right, z up
sc.camera = co


class Nodes:
    """A small helper to wire shader nodes."""

    def __init__(self, mat):
        try:
            mat.use_nodes = True
        except Exception:
            pass
        self.t = mat.node_tree
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

    def smooth(self, x, a, b):  # 0 below a, 1 above b (a, b numbers or sockets)
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
        """The object's colour, set per frame: (strength, alpha, width), and its alpha: the heat."""
        oi = self.new('ShaderNodeObjectInfo')
        s = self.new('ShaderNodeSeparateColor')
        self.put(s.inputs[0], oi.outputs['Color'])
        self.heat = oi.outputs['Alpha']
        return s.outputs[0], s.outputs[1], s.outputs[2]

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


def mesh(name, verts, faces, uvs, mat, at=(0.0, 0.0, 0.0)):
    me = bpy.data.meshes.new(name)
    me.from_pydata(verts, [], faces)
    uvl = me.uv_layers.new(name="UVMap")
    for poly in me.polygons:
        for li in poly.loop_indices:
            uvl.data[li].uv = uvs[me.loops[li].vertex_index]
    me.materials.append(mat)
    ob = bpy.data.objects.new(name, me)
    ob.location = at
    sc.collection.objects.link(ob)
    return ob


HIDDEN = set()  # the other layer's objects


def knob(ob, s, a, w=1.0, heat=1.0):
    ob.color = (max(0.0, s), max(0.0, min(1.0, a)), max(0.0, min(1.0, w)), max(0.0, min(1.0, heat)))
    ob.hide_render = a <= 0.003 or ob.name in HIDDEN


def disc(name, mat, y, rings=16, segs=96):
    """A unit disc: u = angle / 2pi, v = radius."""
    verts, uvs, faces = [(0.0, y, 0.0)], [(0.0, 0.0)], []
    for r in range(1, rings + 1):
        for s in range(segs + 1):
            a = 2 * math.pi * s / segs
            verts.append((r / rings * math.cos(a), y, r / rings * math.sin(a)))
            uvs.append((s / segs, r / rings))
    idx = lambda r, s: 1 + (r - 1) * (segs + 1) + s
    for s in range(segs):
        faces.append((0, idx(1, s), idx(1, s + 1)))
    for r in range(1, rings):
        for s in range(segs):
            faces.append((idx(r, s), idx(r + 1, s), idx(r + 1, s + 1), idx(r, s + 1)))
    return mesh(name, verts, faces, uvs, mat)


# --- the rift (back): a dark violet moon, lit on one side, its space stirring inside ---------------------
m_rift = bpy.data.materials.new("Faille")
nd = Nodes(m_rift)
u, v = nd.uv()
S, A, Wd = nd.knobs()
tc = nd.new('ShaderNodeValue')
CLOCK = [tc]
ang = nd.math('MULTIPLY', u, 2 * math.pi)
cx = nd.new('ShaderNodeCombineXYZ')
nd.put(cx.inputs[0], nd.math('MULTIPLY', nd.math('COSINE', ang), nd.math('ADD', v, 0.3)))
nd.put(cx.inputs[1], nd.math('MULTIPLY', nd.math('SINE', ang), nd.math('ADD', v, 0.3)))
nd.put(cx.inputs[2], nd.math('MULTIPLY', tc.outputs[0], 0.35))
noise = nd.new('ShaderNodeTexNoise', noise_dimensions='3D')
nd.put(noise.inputs['Vector'], cx.outputs[0])
noise.inputs['Scale'].default_value = 3.2
noise.inputs['Detail'].default_value = 4.0
noise.inputs['Distortion'].default_value = 1.2
stir = nd.math('POWER', nd.smooth(noise.outputs['Fac'], 0.45, 0.8), 2.0)
rim = nd.math('MULTIPLY', nd.smooth(v, 0.90, 0.975), nd.math('SUBTRACT', 1.0, nd.smooth(v, 0.985, 1.0)))
lit = nd.math('ADD', 0.35, nd.math('MULTIPLY', nd.math('MAXIMUM', nd.math('COSINE', nd.math(
    'SUBTRACT', ang, 2.3)), 0.0), 1.3))  # the moon's lit side, upper left
glow = nd.math('ADD', nd.math('ADD', nd.math('MULTIPLY', nd.math('MULTIPLY', rim, lit), 4.5), nd.math(
    'MULTIPLY', stir, 1.3)), 0.45)
col = nd.ramp(nd.math('ADD', nd.math('ADD', nd.math('MULTIPLY', rim, 0.75), nd.math('MULTIPLY', stir, 0.4)), 0.22),
              [(0.0, rgba(DARK)), (0.35, rgba(DEEP)), (0.7, rgba(MAIN)), (1.0, rgba(HOT))])
body = nd.math('ADD', 0.30, nd.math('MULTIPLY', nd.math('POWER', v, 2.0), 0.35))  # denser toward the rim
alpha = nd.math('MULTIPLY', A, nd.math('MAXIMUM', body, nd.math('MAXIMUM', rim, nd.math('MULTIPLY', stir, 0.8))))
alpha = nd.math('MULTIPLY', alpha, nd.math('SUBTRACT', 1.0, nd.smooth(v, 0.985, 1.0)))
nd.surface(col, nd.math('MULTIPLY', glow, S), alpha)
rift = disc("faille", m_rift, 0.3)

# --- a cut: a thin crescent, white-hot in its middle, violet at its edges ----------------------------------
m_cut = bpy.data.materials.new("Coupe")
nd = Nodes(m_cut)
u, v = nd.uv()
S, A, Wd = nd.knobs()
core = nd.math('SUBTRACT', 1.0, nd.math('ABSOLUTE', nd.math('SUBTRACT', nd.math('MULTIPLY', v, 2.0), 1.0)))
lo = nd.math('SUBTRACT', 1.0, Wd)
vis = nd.smooth(core, lo, nd.math('ADD', lo, 0.35))
col = nd.ramp(nd.math('MULTIPLY', core, nd.math('ADD', 0.42, nd.math('MULTIPLY', nd.heat, 0.58))),
              [(0.0, rgba(DEEP)), (0.35, rgba(MAIN)), (0.7, rgba(HOT)), (0.88, rgba(WHITE)), (1.0, rgba(WHITE))])
nd.surface(col, nd.math('MULTIPLY', S, nd.math('ADD', 0.5, nd.math('MULTIPLY', nd.math('POWER', core, 2.0), 3.5))),
           nd.math('MULTIPLY', nd.math('MULTIPLY', A, vis), nd.math('ADD', 0.3, nd.math('MULTIPLY', core, 0.7))))


def crescent(name, mid, theta, length, sag, wmax, y):
    d = Vector((math.cos(theta), 0.0, math.sin(theta)))
    n = Vector((-d.z, 0.0, d.x))
    verts, uvs, faces = [], [], []
    K = 48
    for i in range(K + 1):
        s = i / K
        p = d * ((s - 0.5) * length) + n * (sag * (1 - (2 * s - 1) ** 2))
        t = (d * length + n * (-4 * sag * (2 * s - 1))).normalized()
        nn = Vector((-t.z, 0.0, t.x))
        w = wmax * math.sin(math.pi * s) ** 0.75 + 0.002
        verts += [tuple(p - nn * w / 2 + Vector((0, y, 0))), tuple(p + nn * w / 2 + Vector((0, y, 0)))]
        uvs += [(s, 0.0), (s, 1.0)]
        if i:
            faces.append((2 * i - 2, 2 * i, 2 * i + 1, 2 * i - 1))
    return mesh(name, verts, faces, uvs, m_cut, (mid.x, 0.0, mid.z))


CUTS = []  # (object, birth frame)
k = 0
for h in HITS:
    for j in range(4):
        birth = int(round(h)) + (1 if j >= 3 else 0)
        r = 0.55 * RS * math.sqrt(rng.random())
        a = rng.uniform(0, 2 * math.pi)
        mid = Vector((r * math.cos(a), 0.0, r * math.sin(a)))
        ln = rng.uniform(1.3, 2.1) * RS
        sag = ln * rng.uniform(0.10, 0.18) * rng.choice((-1, 1))
        CUTS.append((crescent("coupe%d" % k, mid, rng.uniform(0, math.pi), ln, sag, rng.uniform(0.07, 0.12),
                              -0.2 - 0.001 * k), birth))
        k += 1
CROSS = [crescent("croix%d" % i, Vector((0.0, 0.0, 0.0)), math.radians(a), 3.0 * RS, 0.25 * s, 0.2, -0.4 - 0.01 * i)
         for i, (a, s) in enumerate(((32, 1), (-36, -1)))]

# --- the opening ring and the breaking flash ---------------------------------------------------------------
m_ring = bpy.data.materials.new("Anneau")
nd = Nodes(m_ring)
u, v = nd.uv()
S, A, Wd = nd.knobs()
band = nd.math('MULTIPLY', nd.smooth(v, nd.math('SUBTRACT', 1.0, Wd), 0.97), nd.math('SUBTRACT', 1.0, nd.smooth(
    v, 0.97, 1.0)))
nd.surface(nd.ramp(band, [(0.0, rgba(MAIN)), (0.6, rgba(HOT)), (1.0, rgba(WHITE))]),
           nd.math('MULTIPLY', S, nd.math('ADD', 0.5, nd.math('MULTIPLY', band, 4.0))), nd.math('MULTIPLY', A, band))
ring = disc("anneau", m_ring, -0.1)

m_flash = bpy.data.materials.new("Eclat")
nd = Nodes(m_flash)
u, v = nd.uv()
S, A, Wd = nd.knobs()
fall = nd.math('POWER', nd.math('SUBTRACT', 1.0, v), 1.6)
nd.surface(nd.ramp(fall, [(0.0, rgba(DEEP)), (0.4, rgba(MAIN)), (0.8, rgba(HOT)), (1.0, rgba(WHITE))]),
           nd.math('MULTIPLY', S, nd.math('ADD', 0.3, nd.math('MULTIPLY', fall, 3.0))), nd.math('MULTIPLY', A, fall))
flash = disc("eclat", m_flash, -0.15)

# --- the shards: the moon broken into Voronoi cells that fly apart, spinning ---------------------------
m_shard = bpy.data.materials.new("Eclat_lune")
nd = Nodes(m_shard)
u, v = nd.uv()
S, A, Wd = nd.knobs()
edgev = nd.smooth(v, 0.78, 0.97)
nd.surface(nd.ramp(nd.math('ADD', nd.math('MULTIPLY', v, 0.4), nd.math('MULTIPLY', edgev, 0.6)),
                   [(0.0, rgba(DARK)), (0.35, rgba(DEEP)), (0.7, rgba(MAIN)), (1.0, rgba(HOT))]),
           nd.math('MULTIPLY', S, nd.math('ADD', 0.35, nd.math('MULTIPLY', edgev, 2.5))),
           nd.math('MULTIPLY', A, nd.math('ADD', 0.6, nd.math('MULTIPLY', edgev, 0.4))))


def clip(poly, p, nrm):
    """Sutherland-Hodgman: the part of convex poly where (x - p) . nrm <= 0."""
    out = []
    for i in range(len(poly)):
        a, b = poly[i], poly[(i + 1) % len(poly)]
        da, db = (a - p).dot(nrm), (b - p).dot(nrm)
        if da <= 0:
            out.append(a)
        if da * db < 0:
            out.append(a + (b - a) * (da / (da - db)))
    return out


seeds = []
while len(seeds) < 22:
    q = Vector((rng.uniform(-RS, RS), 0.0, rng.uniform(-RS, RS)))
    if q.length < RS * 0.95 and all((q - s).length > 0.22 for s in seeds):
        seeds.append(q)
circle = [Vector((RS * math.cos(2 * math.pi * i / 64), 0.0, RS * math.sin(2 * math.pi * i / 64))) for i in range(64)]
SHARDS = []
for i, s in enumerate(seeds):
    poly = circle
    for j, o in enumerate(seeds):
        if j != i:
            poly = clip(poly, (s + o) / 2, o - s)
    if len(poly) < 3:
        continue
    c = sum(poly, Vector()) / len(poly)
    far = max((q - c).length for q in poly)
    verts = [(0.0, 0.0, 0.0)] + [tuple(q - c) for q in poly]
    uvs = [(0.0, 0.0)] + [(0.0, 1.0)] * len(poly)
    faces = [(0, 1 + t, 1 + (t + 1) % len(poly)) for t in range(len(poly))]
    ob = mesh("eclat%d" % i, verts, faces, uvs, m_shard, (c.x, -0.05, c.z))
    out = c.normalized() if c.length > 1e-3 else Vector((0.0, 0.0, 1.0))
    SHARDS.append((ob, c, out, rng.uniform(0.07, 0.13) * (0.6 + c.length / RS), rng.uniform(-0.14, 0.14), far))

# --- motes thrown out by the break ------------------------------------------------------------------------
m_mote = bpy.data.materials.new("Poussiere")
nd = Nodes(m_mote)
S, A, Wd = nd.knobs()
nd.surface(rgba(HOT), nd.math('MULTIPLY', S, 6.0), A)
MOTES = []
for i in range(36):
    me = bpy.data.meshes.new("m%d" % i)
    a, b = rng.uniform(0.012, 0.03), 0.006
    me.from_pydata([(-a, 0, 0), (0, 0, -b), (a, 0, 0), (0, 0, b)], [], [(0, 1, 2, 3)])
    me.materials.append(m_mote)
    ob = bpy.data.objects.new("poussiere%d" % i, me)
    sc.collection.objects.link(ob)
    r0 = RS * math.sqrt(rng.random())
    ang = rng.uniform(0, 2 * math.pi)
    MOTES.append((ob, Vector((r0 * math.cos(ang), -0.5, r0 * math.sin(ang))), ang, rng.uniform(0.08, 0.2),
                  rng.uniform(8, 15)))

BACK = [rift]
FRONT = [ob for ob, _ in CUTS] + CROSS + [ring, flash] + [s[0] for s in SHARDS] + [m[0] for m in MOTES]
for ob in (FRONT if LAYER == "back" else BACK):
    HIDDEN.add(ob.name)
    ob.hide_render = True


def cut_state(f, b):
    """(strength, alpha, width, heat) of a cut born on frame b, on frame f: white-hot, then a violet scar."""
    e = f - b
    if e < 0 or f > BREAK + 2:
        return 0.0, 0.0, 0.0, 0.0
    if e == 0:
        s, a, w, h = 3.0, 1.0, 1.0, 1.0
    elif e == 1:
        s, a, w, h = 1.8, 1.0, 0.75, 0.7
    elif e == 2:
        s, a, w, h = 1.1, 0.9, 0.55, 0.35
    else:
        k = math.exp(-(e - 2) / 1.5)
        s, a, w, h = 0.35 + 0.75 * k, 0.5 + 0.4 * k, 0.32 + 0.23 * k, 0.3 * k
    if BREAK - 3 < f < BREAK:  # the scars light up before the break
        g = f - BREAK + 3
        s, w, h = s * (1 + 0.5 * g), w + 0.05 * g, h + 0.2 * g
    elif f == BREAK:
        s, a, w, h = 2.2, 1.0, 0.6, 0.8
    elif f > BREAK:
        s, a, w, h = 1.0, 0.55 if f == BREAK + 1 else 0.22, 0.45, 0.3
    return s, a, w, h


def state(f):
    for c in CLOCK:
        c.outputs[0].default_value = f / N
    # the rift: opens with a pop, swells a little with each blow, tightens, then breaks and fades
    pulse = max((max(0.0, 1 - abs(f - round(h)) / 1.5) for h in HITS), default=0.0)
    if f < 4:
        sc_, a = (0.25, 0.75, 1.08, 1.0)[f], (0.5, 0.85, 1.0, 1.0)[f]
    elif f < BREAK - 2:
        sc_, a = 1.0 + 0.025 * pulse, 1.0
    elif f < BREAK:
        sc_, a = 0.97 - 0.03 * (f - BREAK + 2), 1.0
    else:
        e = f - BREAK
        sc_, a = 1.1 + 0.03 * e, max(0.0, 0.6 - 0.12 * e)
    rift.scale = (RS * sc_,) * 3
    knob(rift, 1.0 + 0.6 * pulse + (1.5 if f == BREAK else 0.0), a)
    for ob, b in CUTS:
        knob(ob, *cut_state(f, b))
    e = f - BREAK
    for ob in CROSS:
        if e < 0 or e > 6:
            knob(ob, 0.0, 0.0)
        else:
            knob(ob, (4.0, 2.6, 1.6, 0.9, 0.5, 0.25, 0.1)[e], 1.0 if e < 4 else 0.7 - 0.15 * (e - 4),
                 (1.0, 0.85, 0.7, 0.55, 0.45, 0.4, 0.35)[e])
    if f <= 4:  # the opening ring
        ring.scale = (RS * (0.25 + 0.95 * (1 - (1 - f / 4) ** 2)),) * 3
        knob(ring, 2.5 - 0.4 * f, 1.0 - 0.2 * f, 0.10)
    else:
        knob(ring, 0.0, 0.0)
    if 0 <= e <= 4:  # the flash of the break
        flash.scale = (RS * (0.6 + 0.18 * e),) * 3
        knob(flash, (2.6, 1.6, 0.9, 0.4, 0.15)[e], (0.85, 0.7, 0.5, 0.3, 0.12)[e])  # (not a white screen)
    else:
        knob(flash, 0.0, 0.0)
    for ob, c, out, sp, w, far in SHARDS:
        if e < 0 or e > 15:
            knob(ob, 0.0, 0.0)
            continue
        go = sp * 6 * (1 - math.exp(-e / 6))
        ob.location = (c.x + out.x * go, -0.05, c.z + out.z * go)
        ob.rotation_euler = (0.0, w * 8 * (1 - math.exp(-e / 8)), 0.0)
        ob.scale = (max(0.2, 1 - 0.03 * e),) * 3
        knob(ob, 3.0 if e == 0 else 1.6 if e == 1 else 1.0, (1 - e / 16) ** 1.3)
    for ob, p, ang, sp, life in MOTES:
        if e < 0 or e > life:
            knob(ob, 0.0, 0.0)
            continue
        go = sp * 6 * (1 - math.exp(-e / 5))
        ob.location = (p.x + math.cos(ang) * go, -0.5, p.z + math.sin(ang) * go)
        ob.rotation_euler = (0.0, -ang, 0.0)
        knob(ob, 1.0, 1 - e / life)


os.makedirs(OUT, exist_ok=True)
FRAMES = [int(x) for x in os.environ.get("CH_FRAMES", "").split(",") if x.strip()] or range(N)
for f in FRAMES:
    state(f)
    sc.render.filepath = os.path.join(OUT, "f%02d.exr" % f)
    bpy.ops.render.render(write_still=True)
    print("IMAGE", f, flush=True)
print("FIN", OUT)
