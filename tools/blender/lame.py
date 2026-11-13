# Clash Horizon - a flying blade of energy (yoyo, 12/11: "proche des slashes de Vergil ou des grands croissants
# d'énergie", original look): a crescent with a razor, white-hot leading edge, a body of energy that tears into
# flames streaming backward, two trails from its tips, speed lines and sparks; styles per fighter (arcs for the
# lightning ones, dark smoke for the shadow, embers for the fire, glints for the light).
# A seamless loop of N frames (the game cycles it while the blade flies), flying right; the hit circle (radius RH
# px) is centred on the anchor (AX, AY).
# Usage: blender -b -P lame.py -- <style> <out_dir> [samples]   (or python with bpy installed)
# Writes out_dir/fNN.exr: linear, premultiplied RGBA on a transparent film (post_lame.py adds the glow).
import math
import os
import random
import sys

import bpy
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else sys.argv[1:]
STYLE, OUT = argv[0], argv[1]
SAMPLES = int(argv[2]) if len(argv) > 2 else 32
N = 12
W, H, AX, AY, RH = 330, 360, 240, 180, 70

# core, hot, main, deep, smoke (None: no smoke, the body just fades out), arcs, sparks kind, blade thickness
STYLES = {
    "h01": ("#FFF4F6", "#FF8FA3", "#F0304A", "#8E0C22", None, 3, "spark", 0.15),
    "p01": ("#F7EEFF", "#C9A2FF", "#8A4DFF", "#3B1772", "#140A20", 0, "spark", 0.15),
    "p03": ("#FFF7E2", "#FFC86E", "#FF5A1F", "#B3170E", "#1E0F0B", 0, "ember", 0.17),
    "p07": ("#FFF1F2", "#FF7F8C", "#FF2D3D", "#7E0A18", None, 6, "spark", 0.10),
    "p08": ("#FFFFFF", "#FFF8D6", "#FFE07A", "#D9A441", None, 0, "glint", 0.14),
}
BODY = {"p08": 0.6}.get(STYLE, 1.0)  # how solid the body is (the light one stays airy, not a yellow moon)
C_CORE, C_HOT, C_MAIN, C_DEEP, C_SMOKE, ARCS, SPARKS, THICK = STYLES[STYLE]
rng = random.Random(sum(map(ord, STYLE)))

# --- geometry, in units (100 px each; the anchor is the origin, x to the front, z up) -------------------------
R = 1.30                # radius of the leading edge
CX = 0.20 - R           # its centre: the edge passes 20 px ahead of the anchor
TH = math.radians(72)   # half the arc


def lin(c):
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def rgba(h):
    h = h.lstrip("#")
    return (*(lin(int(h[i:i + 2], 16) / 255) for i in (0, 2, 4)), 1.0)


def bell(u):  # 1 in the middle of the arc, 0 at its tips
    return max(0.0, math.cos(math.pi * (u - 0.5)))


def edge(u, inset=0.0):
    s = u * 2 - 1
    a = s * TH
    r = R * (1 - 0.10 * s * s) - inset  # the tips curl back, like a swept wing
    return Vector((CX + r * math.cos(a), 0.0, r * math.sin(a)))


# --- scene -----------------------------------------------------------------------------------------------
bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
sc.render.engine = 'CYCLES'
sc.cycles.device = 'CPU'
sc.cycles.samples = SAMPLES
sc.cycles.use_denoising = False
sc.cycles.max_bounces = 0
sc.cycles.transparent_max_bounces = 48
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
co.location = ((W / 2 - AX) / 100, -10.0, (AY - H / 2) / 100)
co.rotation_euler = (math.pi / 2, 0.0, 0.0)  # looks along +y: x right, z up
sc.camera = co


def link(ob):
    sc.collection.objects.link(ob)
    return ob


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
        if isinstance(v, (int, float)):
            sock.default_value = v
        elif isinstance(v, tuple):
            sock.default_value = v
        else:
            self.t.links.new(v, sock)

    def math(self, op, a, b=0.0, clamp=False):
        n = self.new('ShaderNodeMath', operation=op, use_clamp=clamp)
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

    def smooth(self, x, a, b):  # 0 below a, 1 above b
        n = self.new('ShaderNodeMapRange', interpolation_type='SMOOTHSTEP', clamp=True)
        self.put(n.inputs[0], x)
        n.inputs[1].default_value, n.inputs[2].default_value = a, b
        n.inputs[3].default_value, n.inputs[4].default_value = 0.0, 1.0
        return n.outputs[0]

    def uv(self):
        s = self.new('ShaderNodeSeparateXYZ')
        self.put(s.inputs[0], self.new('ShaderNodeTexCoord').outputs['UV'])
        return s.outputs[0], s.outputs[1]

    def loop_noise(self, u, v, t, ku, kv, scale, detail=5.0, rough=0.55, seed=0.0, rho=1.0):
        """Noise sampled on a cylinder: it scrolls backward along v by kv periods per loop and repeats seamlessly."""
        a = self.math('MULTIPLY', self.math('SUBTRACT', self.math('MULTIPLY', v, kv), t), 2 * math.pi)
        c = self.new('ShaderNodeCombineXYZ')
        self.put(c.inputs[0], self.math('MULTIPLY', self.math('COSINE', a), rho))
        self.put(c.inputs[1], self.math('MULTIPLY', self.math('SINE', a), rho))
        self.put(c.inputs[2], self.math('ADD', self.math('MULTIPLY', u, ku), seed))
        n = self.new('ShaderNodeTexNoise', noise_dimensions='3D')
        self.put(n.inputs['Vector'], c.outputs[0])
        n.inputs['Scale'].default_value = scale
        n.inputs['Detail'].default_value = detail
        n.inputs['Roughness'].default_value = rough
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


CLOCK = []  # the Value nodes that hold the loop time (0..1)


def clock(nd):
    v = nd.new('ShaderNodeValue')
    CLOCK.append(v)
    return v.outputs[0]


def grid(name, pts, mat, y):
    """A mesh from a grid of points pts[i][j] (x, z), with u = i / (nu - 1) and v = j / (nv - 1) as its UVs."""
    nu, nv = len(pts), len(pts[0])
    verts = [(x, y, z) for row in pts for (x, z) in row]
    faces = [(i * nv + j, i * nv + j + 1, (i + 1) * nv + j + 1, (i + 1) * nv + j)
             for i in range(nu - 1) for j in range(nv - 1)]
    me = bpy.data.meshes.new(name)
    me.from_pydata(verts, [], faces)
    uvl = me.uv_layers.new(name="UVMap")
    for poly in me.polygons:
        for li in poly.loop_indices:
            i, j = divmod(me.loops[li].vertex_index, nv)
            uvl.data[li].uv = (i / (nu - 1), j / (nv - 1))
    me.materials.append(mat)
    return link(bpy.data.objects.new(name, me))


# --- the blade: white-hot at its leading edge, its colour inside, a torn inner edge ----------------------
m_blade = bpy.data.materials.new("Lame")
nd = Nodes(m_blade)
u, v = nd.uv()
t = clock(nd)
tear = nd.loop_noise(u, v, t, 12.0, 0.0, 2.0, seed=3.0, rho=0.3)
inner = nd.math('ADD', v, nd.math('MULTIPLY', nd.math('SUBTRACT', tear, 0.5), 0.6))
alpha = nd.math('SUBTRACT', 1.0, nd.smooth(inner, 0.7, 1.0))
col = nd.ramp(v, [(0.0, rgba(C_CORE)), (0.10, rgba(C_CORE)), (0.22, rgba(C_HOT)), (0.5, rgba(C_MAIN)),
                  (1.0, rgba(C_DEEP))])
glow = nd.math('ADD', nd.math('MULTIPLY', nd.math('SUBTRACT', 1.0, nd.smooth(v, 0.04, 0.3)), 5.5), 0.9)
nd.surface(col, glow, alpha)
blade = grid("lame", [[(p.x, p.z) for p in (edge(i / 80, j / 10 * THICK * bell(i / 80) ** 1.1) for j in range(11))]
                      for i in range(81)], m_blade, 0.0)

# an echo of the edge just behind it, thin and flickering: the cut seen twice
m_echo = bpy.data.materials.new("Echo")
nd = Nodes(m_echo)
u, v = nd.uv()
t = clock(nd)
fl = nd.loop_noise(v, u, t, 2.0, 2.0, 2.5, detail=2.0, seed=17.0, rho=0.6)
nd.surface(rgba(C_HOT), 3.0, nd.math('MULTIPLY', nd.smooth(fl, 0.35, 0.6), nd.math('SUBTRACT', 1.0, nd.math(
    'ABSOLUTE', nd.math('SUBTRACT', nd.math('MULTIPLY', v, 2.0), 1.0)))))
echo = grid("echo", [[(p.x, p.z) for p in (edge(0.1 + 0.8 * i / 60, THICK * 1.5 * bell(0.1 + 0.8 * i / 60) + d)
                                           for d in (-0.006, 0.006))] for i in range(61)], m_echo, -0.005)

# --- the body: energy torn into flames, streaming back from the blade -----------------------------------
m_body = bpy.data.materials.new("Corps")
nd = Nodes(m_body)
u, v = nd.uv()
t = clock(nd)
# tongues stretched along the flow: few cycles along v (rho), many along the arc (ku)
f1 = nd.loop_noise(u, v, t, 20.0, 1.0, 2.0, detail=6.0, seed=11.0, rho=0.14)
f2 = nd.loop_noise(u, v, t, 42.0, 2.0, 2.0, detail=3.0, seed=29.0, rho=0.14)
flame = nd.math('ADD', nd.math('MULTIPLY', f1, 0.7), nd.math('MULTIPLY', f2, 0.3))
burn = nd.math('ADD', 0.20, nd.math('MULTIPLY', v, 0.36))  # eaten more the farther back
alpha = nd.smooth(nd.math('SUBTRACT', flame, burn), -0.03, 0.08)
heat = nd.math('MULTIPLY', nd.math('POWER', nd.math('SUBTRACT', 1.0, v), 1.3), nd.math('ADD', 0.35, nd.math(
    'MULTIPLY', flame, 0.9)))
stops = [(0.0, rgba(C_SMOKE or C_DEEP)), (0.3, rgba(C_DEEP)), (0.55, rgba(C_MAIN)), (0.85, rgba(C_HOT)),
         (1.0, rgba(C_CORE))]
col = nd.ramp(heat, stops)
alpha = nd.math('MULTIPLY', alpha, nd.math('ADD', 0.5 * BODY, nd.math('MULTIPLY', heat, 0.5 * BODY)))  # energy, not a wall
light = nd.math('ADD', nd.math('MULTIPLY', nd.math('POWER', heat, 2.0), 2.2), 0.12 if C_SMOKE else 0.5)
if not C_SMOKE:  # no smoke: the far flames fade to nothing instead of darkening
    alpha = nd.math('MULTIPLY', alpha, nd.math('SUBTRACT', 1.0, nd.smooth(v, 0.5, 1.0)))
nd.surface(col, light, alpha)
DEPTH = 1.05 if STYLE != "p07" else 0.6
body_pts = []
for i in range(81):
    uu = i / 80
    b = bell(uu)
    root = edge(uu, THICK * b ** 1.1 * 0.8)
    d = DEPTH * (0.12 + 0.88 * b ** 0.55)
    lift = (uu - 0.5) * 0.22  # flares a little outward toward the tips
    body_pts.append([(root.x - d * j / 20, root.z + lift * d * j / 20) for j in range(21)])
body = grid("corps", body_pts, m_body, 0.05)

# --- trails from the tips and speed lines inside: thin ribbons, fading backward --------------------------
m_trail = bpy.data.materials.new("Trainee")
nd = Nodes(m_trail)
u, v = nd.uv()
t = clock(nd)
flick = nd.loop_noise(v, u, t, 3.0, 1.0, 1.8, detail=2.0, seed=5.0)  # dashes running back along the ribbon
along = nd.math('SUBTRACT', 1.0, u)
alpha = nd.math('MULTIPLY', nd.math('POWER', along, 1.6), nd.smooth(flick, 0.25, 0.6))
alpha = nd.math('MULTIPLY', alpha, nd.math('SUBTRACT', 1.0, nd.math('ABSOLUTE', nd.math('SUBTRACT', nd.math(
    'MULTIPLY', v, 2.0), 1.0))))
col = nd.ramp(along, [(0.0, rgba(C_MAIN)), (0.6, rgba(C_HOT)), (1.0, rgba(C_CORE))])
nd.surface(col, nd.math('ADD', nd.math('MULTIPLY', along, 5.0), 0.8), alpha)


def ribbon(name, start, length, width, curl, y):
    pts = []
    for i in range(31):
        k = i / 30
        cx = start.x - length * k
        cz = start.z + curl * k * k
        wdt = width * (1 - k) ** 0.7 + 0.002
        pts.append([(cx, cz - wdt), (cx, cz + wdt)])
    return grid(name, pts, m_trail, y)


ribbons = []
for s in (-1, 1):
    tip = edge(0.0 if s < 0 else 1.0)
    ribbons.append(ribbon("trace%d" % s, tip + Vector((0.02, 0, -0.02 * s)), 0.85, 0.016, 0.10 * s, -0.01))
LINES = []
for k in range(0):  # (speed lines read as scanlines: none)
    uu = 0.18 + 0.64 * rng.random()
    root = edge(uu, THICK * 0.5)
    LINES.append((root, 0.5 + 0.6 * rng.random(), 0.006 + 0.006 * rng.random(), rng.random()))
streaks = [ribbon("vitesse%d" % k, r, ln, w, 0.0, -0.02) for k, (r, ln, w, ph) in enumerate(LINES)]

# --- sparks (or embers, or glints): born on the blade, carried back, shrinking out --------------------------
m_spark = bpy.data.materials.new("Etincelle")
nd = Nodes(m_spark)
nd.surface(rgba(C_HOT if SPARKS != "ember" else "#FFB347"), 6.0, 1.0)
SPK = []
for k in range(14):
    uu = 0.08 + 0.84 * rng.random()
    root = edge(uu, THICK * rng.random())
    if SPARKS == "glint":  # a four-pointed star
        me = bpy.data.meshes.new("g%d" % k)
        a, b = 0.032, 0.006
        me.from_pydata([(-a, 0, 0), (0, 0, -b), (a, 0, 0), (0, 0, b), (-b, 0, 0), (0, 0, -a), (b, 0, 0), (0, 0, a)],
                       [], [(0, 1, 2, 3), (4, 5, 6, 7)])
    else:  # a thin comet: widest near its head, tapering to a point behind (a plain bar reads as a dash)
        me = bpy.data.meshes.new("s%d" % k)
        a, b = (rng.uniform(0.03, 0.07), 0.0045) if SPARKS == "spark" else (0.012, 0.008)
        me.from_pydata([(-a, 0, 0), (a * 0.55, 0, -b), (a, 0, 0), (a * 0.55, 0, b)], [], [(0, 1, 2, 3)])
    me.materials.append(m_spark)
    ob = link(bpy.data.objects.new("etincelle%d" % k, me))
    rise = rng.uniform(-0.25, 0.25) + (0.35 if SPARKS == "ember" else 0.0)
    SPK.append((ob, root, rng.uniform(0.6, 1.3), rise, rng.random(), rng.uniform(0.6, 1.4)))


# --- arcs crawling on the blade (lightning styles): new every frame ---------------------------------------
def jag(a, b, depth, amp):
    pts = [a, b]
    for _ in range(depth):
        new = [pts[0]]
        for p, q in zip(pts, pts[1:]):
            d = q - p
            m = (p + q) / 2
            n = Vector((-d.z, 0, d.x)).normalized() if d.length > 1e-6 else Vector((0, 0, 1))
            new += [m + n * rng.uniform(-amp, amp) * d.length, q]
        pts = new
    return pts


m_arc = bpy.data.materials.new("Arc")
nd = Nodes(m_arc)
nd.surface(rgba(C_CORE), 16.0, 1.0)
m_arc_g = bpy.data.materials.new("ArcHalo")
nd = Nodes(m_arc_g)
nd.surface(rgba(C_MAIN), 5.0, 0.75)


def curve(name, pts, radius, mat, y):
    cu = bpy.data.curves.new(name, 'CURVE')
    cu.dimensions = '3D'
    cu.bevel_depth = radius
    cu.bevel_resolution = 1
    sp = cu.splines.new('POLY')
    sp.points.add(len(pts) - 1)
    for i, p in enumerate(pts):
        sp.points[i].co = (p.x, y, p.z, 1)
    cu.materials.append(mat)
    return link(bpy.data.objects.new(name, cu))


ARC_OBS = []


def arcs(f):
    for ob in ARC_OBS:
        bpy.data.objects.remove(ob)
    ARC_OBS.clear()
    for k in range(ARCS):
        u0 = rng.uniform(0.08, 0.8)
        u1 = min(0.95, u0 + rng.uniform(0.1, 0.3))
        a = edge(u0, rng.uniform(-0.04, THICK * 0.6))
        b = edge(u1, rng.uniform(-0.04, THICK * 0.6))
        if rng.random() < 0.35:  # some leap off the blade, backward
            b = a + Vector((-rng.uniform(0.25, 0.6), 0, rng.uniform(-0.2, 0.2)))
        pts = jag(a, b, 4, 0.32)
        ARC_OBS.append(curve("arc%d_%d" % (f, k), pts, 0.0035, m_arc, -0.03))
        ARC_OBS.append(curve("arch%d_%d" % (f, k), pts, 0.011, m_arc_g, -0.025))


def state(f):
    tt = f / N
    for c in CLOCK:
        c.outputs[0].default_value = tt
    for ob, root, ln, rise, ph, sz in SPK:
        k = (tt * sz + ph) % 1.0  # its own pace, still a whole number of lives per loop when sz is rounded
        ob.location = (root.x - ln * k, 0.0 - 0.04, root.z + rise * k)
        s = math.sin(math.pi * k) * (1.0 if SPARKS != "glint" else 0.6 + 0.4 * math.sin(6 * math.pi * k))
        ob.scale = (s, s, s)
    if ARCS:
        arcs(f)


# whole lives per loop for the sparks, so the loop is seamless
SPK = [(ob, root, ln, rise, ph, float(max(1, round(sz)))) for ob, root, ln, rise, ph, sz in SPK]

os.makedirs(OUT, exist_ok=True)
FRAMES = [int(x) for x in os.environ.get("CH_FRAMES", "").split(",") if x.strip()] or range(N)
for f in FRAMES:
    state(f)
    sc.render.filepath = os.path.join(OUT, "f%02d.exr" % f)
    bpy.ops.render.render(write_still=True)
    print("IMAGE", f, flush=True)
print("FIN", OUT)
