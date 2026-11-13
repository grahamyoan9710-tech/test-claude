# Clash Horizon - Hakuen's ultimate storm, Blender v6 (yoyo's notes of 12/11): a thin, sharp bolt that crosses the
# screen on a diagonal, from a dark cloud up on the caster's side down to the ground under the target, in the manner
# of Elden Ring's red dragon lightning. It creeps down (2 frames), strikes once, flickers out and is gone before the
# next one, so the five bolts of the ultimate read one by one; the ground takes the hit (flash, arcs, sparks, shock
# ring, scorch, smoke).
# 25 frames of 900x740, the strike on frame 15, impact point (on the floor) at (680, 670).
# Two renders per frame: the smoke (cloud, smoke and glow of the impact: denoised) and the lines (bolt, branches,
# arcs, sparks, rings: not denoised, so they stay sharp). post3.py adds the glow and stacks them.
# Usage: blender -b -P eclair6.py -- <out_dir> [samples]   (env CH_FRAMES=13,15 renders only those frames)
# Writes out_dir/fumee/fNN.exr and out_dir/trait/fNN.exr: linear, premultiplied RGBA on a transparent film.
import bpy, sys, os, math, random
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
OUT = argv[0]
SAMPLES = int(argv[1]) if len(argv) > 1 else 32
W, H, N, STRIKE = 900, 740, 25, 15
AX, AY = 680, 670
TILT = math.radians(15)
CX = -(AX - W / 2) / 100
ZC = (AY - H / 2) / 100 / math.cos(TILT)
random.seed(7)


def at(u, v, y=0.0):
    """The world point drawn at pixel (u, v), at depth y (100 px per unit, the impact point is the origin)."""
    return Vector((CX + (u - W / 2) / 100, y, ZC + ((H / 2 - v) / 100 - y * math.sin(TILT)) / math.cos(TILT)))


def lin(c):
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def hexlin(h):
    h = h.lstrip("#")
    return tuple(lin(int(h[i:i + 2], 16) / 255) for i in (0, 2, 4))


# The skills' crimson, as the in-game strip (hue ~350): white-hot core, pink, crimson sheath, deep red.
WHITE, PINK, RED, DEEP = "#FFF3F5", "#FF8FA3", "#F0304A", "#9E0E26"

bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
sc.render.engine = 'CYCLES'
sc.cycles.device = 'CPU'
sc.cycles.max_bounces = 2
sc.cycles.volume_bounces = 0
sc.cycles.transparent_max_bounces = 32
try:
    sc.cycles.volume_step_rate = 1.5
except Exception:
    pass
sc.render.film_transparent = True
sc.render.resolution_x, sc.render.resolution_y, sc.render.resolution_percentage = W, H, 100
sc.view_settings.view_transform = 'Standard'
sc.view_settings.look = 'None'
ims = sc.render.image_settings
ims.file_format = 'OPEN_EXR'
ims.color_mode = 'RGBA'
ims.color_depth = '16'
ims.exr_codec = 'ZIP'
w = bpy.data.worlds.new("Noir")
sc.world = w
try:
    w.use_nodes = True
except Exception:
    pass
bg = w.node_tree.nodes.get('Background') or w.node_tree.nodes.new('ShaderNodeBackground')
bg.inputs['Color'].default_value = (0, 0, 0, 1)
bg.inputs['Strength'].default_value = 0.0

LINES, SMOKE = [], []  # the two renders of each frame


def tree(m):
    try:
        m.use_nodes = True  # deprecated (always on) in Blender 5
    except Exception:
        pass
    nt = m.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    return nt


def mat_emit(name, hexc, strength=1.0):
    """Additive light: emission plus transparency, so a line never hides what is behind it."""
    m = bpy.data.materials.new(name)
    nt = tree(m)
    e = nt.nodes.new('ShaderNodeEmission')
    e.inputs['Color'].default_value = (*hexlin(hexc), 1)
    e.inputs['Strength'].default_value = strength
    t = nt.nodes.new('ShaderNodeBsdfTransparent')
    a = nt.nodes.new('ShaderNodeAddShader')
    o = nt.nodes.new('ShaderNodeOutputMaterial')
    nt.links.new(e.outputs[0], a.inputs[0])
    nt.links.new(t.outputs[0], a.inputs[1])
    nt.links.new(a.outputs[0], o.inputs['Surface'])
    return m, e


def pair(name, core_hex):
    """A line's two materials: its bright core and its crimson sheath."""
    return mat_emit(name, core_hex), mat_emit(name + "Gaine", RED)


def link(ob):
    sc.collection.objects.link(ob)
    return ob


def math_node(nt, op, a, b=None):
    n = nt.nodes.new('ShaderNodeMath')
    n.operation = op
    for i, v in enumerate((a, b)):
        if v is None:
            continue
        if isinstance(v, (int, float)):
            n.inputs[i].default_value = v
        else:
            nt.links.new(v, n.inputs[i])
    return n.outputs[0]


def map_range(nt, v, f0, f1, t0, t1, smooth=True):
    n = nt.nodes.new('ShaderNodeMapRange')
    if smooth:
        n.interpolation_type = 'SMOOTHERSTEP'
    n.inputs['From Min'].default_value = f0
    n.inputs['From Max'].default_value = f1
    n.inputs['To Min'].default_value = t0
    n.inputs['To Max'].default_value = t1
    nt.links.new(v, n.inputs['Value'])
    return n


def jag(a, b, amps, depth=0.25, ground=False, rng=random):
    """Midpoint displacement, one amplitude per level (big kinks first, then the fine cracks). In the picture plane,
    or on the ground (ground=True: it stays on the floor, a hair above it)."""
    pts = [Vector(a), Vector(b)]
    for amp in amps:
        new = [pts[0]]
        for p, q in zip(pts, pts[1:]):
            d = q - p
            side = Vector((-d.y, d.x, 0)) if ground else Vector((-d.z, 0, d.x))
            side = side.normalized() if side.length > 1e-6 else Vector((1, 0, 0))
            m = (p + q) / 2 + side * rng.uniform(-amp, amp) * d.length
            if ground:
                m.z = max(0.006, m.z + rng.uniform(0, amp) * d.length * depth)
            else:
                m.y += rng.uniform(-amp, amp) * d.length * depth
            new += [m, q]
        pts = new
    return pts


def curve(name, pts, radius, mat, taper=False):
    cu = bpy.data.curves.new(name, 'CURVE')
    cu.dimensions = '3D'
    cu.bevel_depth = radius
    cu.bevel_resolution = 1
    sp = cu.splines.new('POLY')
    sp.points.add(len(pts) - 1)
    for i, p in enumerate(pts):
        sp.points[i].co = (p.x, p.y, p.z, 1)
        sp.points[i].radius = (1.0 - 0.8 * i / (len(pts) - 1)) if taper else random.uniform(0.8, 1.15)
    cu.materials.append(mat)
    ob = link(bpy.data.objects.new(name, cu))
    LINES.append(ob)
    return ob


def line(name, pts, r_core, mats, taper=False):
    """A crackle drawn twice: its core and, three times as wide, its crimson sheath."""
    (mc, _), (ms, _) = mats
    return [curve(name, pts, r_core, mc, taper), curve(name + "_g", pts, r_core * 2.6, ms, taper)]


def set_pts(ob, pts):
    sp = ob.data.splines[0]
    for k, p in enumerate(pts):
        sp.points[k].co = (p.x, p.y, p.z, 1)


def ring_pts(r, n=120):
    return [Vector((r * math.cos(2 * math.pi * k / n), r * math.sin(2 * math.pi * k / n), 0)) for k in range(n + 1)]


def strength(mats, core, sheath):
    mats[0][1].inputs['Strength'].default_value = core
    mats[1][1].inputs['Strength'].default_value = sheath


def show(objs, on):
    for ob in objs:
        ob.hide_render = not on


def ease(x):
    x = max(0.0, min(1.0, x))
    return x * x * (3 - 2 * x)


class Glow:
    """A light inside the smoke: the smoke shines around a point (world position, radius); strength ~ radiance."""

    def __init__(self, comb, rng, val):
        self.comb, self.rng, self.val = comb, rng, val

    def set(self, pos, s, radius=None):
        for i, c in enumerate(pos):
            self.comb.inputs[i].default_value = c
        self.val.outputs[0].default_value = s
        if radius is not None:
            self.rng.inputs['From Max'].default_value = radius


def smoke_volume(name, dims, loc, seed_w, glows, albedo, emit, self_emit):
    """Billowing smoke: a 4D noise pushes the edge of an ellipsoid in and out, a finer one thins the inside. It
    shines faintly by itself and around each glow (hex colour, radius). Returns the object, the noise nodes, the
    density gain node and the Glow handles."""
    bpy.ops.mesh.primitive_cube_add(size=2, location=loc)
    ob = bpy.context.active_object
    ob.name = name
    ob.scale = dims
    m = bpy.data.materials.new(name)
    nt = tree(m)
    tc = nt.nodes.new('ShaderNodeTexCoord')
    n1 = nt.nodes.new('ShaderNodeTexNoise')
    n1.noise_dimensions = '4D'
    n1.inputs['Scale'].default_value = 2.4
    n1.inputs['Detail'].default_value = 8
    n1.inputs['Roughness'].default_value = 0.6
    n1.inputs['Distortion'].default_value = 0.25
    n1.inputs['W'].default_value = seed_w
    n2 = nt.nodes.new('ShaderNodeTexNoise')
    n2.noise_dimensions = '4D'
    n2.inputs['Scale'].default_value = 6.0
    n2.inputs['Detail'].default_value = 3
    n2.inputs['W'].default_value = seed_w + 10
    iso = nt.nodes.new('ShaderNodeVectorMath')  # the noise in the smoke's own proportions: round billows, not streaks
    iso.operation = 'MULTIPLY'
    iso.inputs[1].default_value = tuple(c / max(dims) for c in dims)
    nt.links.new(tc.outputs['Object'], iso.inputs[0])
    nt.links.new(iso.outputs[0], n1.inputs['Vector'])
    nt.links.new(iso.outputs[0], n2.inputs['Vector'])
    ln = nt.nodes.new('ShaderNodeVectorMath')
    ln.operation = 'LENGTH'
    nt.links.new(tc.outputs['Object'], ln.inputs[0])
    push = math_node(nt, 'MULTIPLY', math_node(nt, 'SUBTRACT', n1.outputs['Fac'], 0.5), 1.5)
    d = math_node(nt, 'SUBTRACT', ln.outputs['Value'], push)
    shape = map_range(nt, d, 0.42, 0.72, 1.0, 0.0)
    inner = map_range(nt, n2.outputs['Fac'], 0.38, 0.62, 0.15, 1.0, smooth=False)
    gain = nt.nodes.new('ShaderNodeValue')
    gain.outputs[0].default_value = 0.0
    dens = math_node(nt, 'MULTIPLY', math_node(nt, 'MULTIPLY', shape.outputs['Result'], inner.outputs['Result']),
                     gain.outputs[0])
    vol = nt.nodes.new('ShaderNodeVolumePrincipled')
    vol.inputs['Color'].default_value = (*hexlin(albedo), 1)
    vol.inputs['Emission Color'].default_value = (*hexlin(emit), 1)
    nt.links.new(dens, vol.inputs['Density'])
    nt.links.new(math_node(nt, 'MULTIPLY', dens, self_emit), vol.inputs['Emission Strength'])
    total = vol.outputs[0]
    geo = nt.nodes.new('ShaderNodeNewGeometry')
    handles = []
    for hexc, radius in glows:
        comb = nt.nodes.new('ShaderNodeCombineXYZ')
        dist = nt.nodes.new('ShaderNodeVectorMath')
        dist.operation = 'DISTANCE'
        nt.links.new(geo.outputs['Position'], dist.inputs[0])
        nt.links.new(comb.outputs[0], dist.inputs[1])
        rng = map_range(nt, dist.outputs['Value'], 0.0, radius, 1.0, 0.0, smooth=False)
        val = nt.nodes.new('ShaderNodeValue')
        val.outputs[0].default_value = 0.0
        e = nt.nodes.new('ShaderNodeEmission')
        e.inputs['Color'].default_value = (*hexlin(hexc), 1)
        soft = math_node(nt, 'POWER', rng.outputs['Result'], 2.0)
        nt.links.new(math_node(nt, 'MULTIPLY', math_node(nt, 'MULTIPLY', soft, val.outputs[0]), dens),
                     e.inputs['Strength'])
        add = nt.nodes.new('ShaderNodeAddShader')
        nt.links.new(total, add.inputs[0])
        nt.links.new(e.outputs[0], add.inputs[1])
        total = add.outputs[0]
        handles.append(Glow(comb, rng, val))
    out = nt.nodes.new('ShaderNodeOutputMaterial')
    nt.links.new(total, out.inputs['Volume'])
    ob.data.materials.append(m)
    SMOKE.append(ob)
    return ob, (n1, n2), gain, handles


def light_ball(name, loc, scale, hexc):
    """Pure light in a volume, no smoke: bright in the middle, nothing at the edge. Returns object, strength node."""
    bpy.ops.mesh.primitive_uv_sphere_add(radius=1.0, segments=32, ring_count=16, location=loc)
    ob = bpy.context.active_object
    ob.name = name
    ob.scale = scale
    m = bpy.data.materials.new(name)
    nt = tree(m)
    tc = nt.nodes.new('ShaderNodeTexCoord')
    ln = nt.nodes.new('ShaderNodeVectorMath')
    ln.operation = 'LENGTH'
    nt.links.new(tc.outputs['Object'], ln.inputs[0])
    fall = math_node(nt, 'POWER', map_range(nt, ln.outputs['Value'], 0.0, 1.0, 1.0, 0.0).outputs['Result'], 2.0)
    val = nt.nodes.new('ShaderNodeValue')
    e = nt.nodes.new('ShaderNodeEmission')
    e.inputs['Color'].default_value = (*hexlin(hexc), 1)
    nt.links.new(math_node(nt, 'MULTIPLY', fall, val.outputs[0]), e.inputs['Strength'])
    out = nt.nodes.new('ShaderNodeOutputMaterial')
    nt.links.new(e.outputs[0], out.inputs['Volume'])
    ob.data.materials.append(m)
    SMOKE.append(ob)
    return ob, val


def scorch_mat(name):
    """The scorched ground: a hot heart graded to deep red (object coords), under a darker veil that grows as it
    cools. Returns the material, its emission and the veil's map range."""
    m = bpy.data.materials.new(name)
    nt = tree(m)
    tc = nt.nodes.new('ShaderNodeTexCoord')
    ln = nt.nodes.new('ShaderNodeVectorMath')
    ln.operation = 'LENGTH'
    rp = nt.nodes.new('ShaderNodeValToRGB')
    els = rp.color_ramp.elements
    for k, (pos, hexc, wgt) in enumerate([(0.0, WHITE, 1.0), (1.0, DEEP, 0.0), (0.1, RED, 1.0), (0.45, DEEP, 0.3)]):
        el = els[k] if k < 2 else els.new(pos)
        el.position = pos
        el.color = (*[c * wgt for c in hexlin(hexc)], 1)
    e = nt.nodes.new('ShaderNodeEmission')
    t = nt.nodes.new('ShaderNodeBsdfTransparent')
    a = nt.nodes.new('ShaderNodeAddShader')
    o = nt.nodes.new('ShaderNodeOutputMaterial')
    nt.links.new(tc.outputs['Object'], ln.inputs[0])
    nt.links.new(ln.outputs['Value'], rp.inputs['Fac'])
    nt.links.new(rp.outputs['Color'], e.inputs['Color'])
    nt.links.new(e.outputs[0], a.inputs[0])
    nt.links.new(t.outputs[0], a.inputs[1])
    vr = map_range(nt, ln.outputs['Value'], 0.15, 1.0, 0.0, 0.0, smooth=False)
    dark = nt.nodes.new('ShaderNodeEmission')
    dark.inputs['Color'].default_value = (*hexlin("#2A0509"), 1)
    dark.inputs['Strength'].default_value = 0.5
    mx = nt.nodes.new('ShaderNodeMixShader')
    nt.links.new(vr.outputs['Result'], mx.inputs[0])
    nt.links.new(a.outputs[0], mx.inputs[1])
    ad = nt.nodes.new('ShaderNodeAddShader')
    nt.links.new(dark.outputs[0], ad.inputs[0])
    nt.links.new(e.outputs[0], ad.inputs[1])
    nt.links.new(ad.outputs[0], mx.inputs[2])
    nt.links.new(mx.outputs[0], o.inputs['Surface'])
    return m, e, vr


# --- camera: orthographic, 15 degrees above the ground, impact point at (AX, AY) -------------------------
cam = bpy.data.cameras.new("Camera")
cam.type = 'ORTHO'
cam.sensor_fit = 'VERTICAL'
cam.ortho_scale = H / 100
co = link(bpy.data.objects.new("Camera", cam))
look = Vector((CX, 0, ZC))
co.location = look + Vector((0, -20 * math.cos(TILT), 20 * math.sin(TILT)))
co.rotation_euler = (look - co.location).to_track_quat('-Z', 'Y').to_euler()
sc.camera = co

# --- a soft key light from the upper left: the cloud gets lit tops and dark undersides ----------------------
sun = bpy.data.lights.new("Soleil", 'SUN')
sun.energy = 3.0
sun.angle = math.radians(12)
sun.color = hexlin("#FFD6DC")
so = link(bpy.data.objects.new("Soleil", sun))
so.rotation_euler = Vector((0.55, 0.45, -1.0)).to_track_quat('-Z', 'Y').to_euler()

# --- the cloud, up on the caster's side: dark smoke, two faint red hearts, a glow per crackle, the strike's flare
CLOUD_C = at(250, 140, 0.2)
TOP = at(268, 192)  # where the bolt leaves the cloud
cloud, cloud_noise, cloud_gain, (heart1, heart2, crack_glow, flare) = smoke_volume(
    "Nuage", (2.3, 0.9, 1.0), CLOUD_C, 0.0, [(RED, 0.5), (RED, 0.5), (PINK, 0.5), (RED, 1.2)],
    albedo="#A0606A", emit=DEEP, self_emit=0.02)
hearts = [heart1, heart2]
heart_path = [[CLOUD_C + Vector((random.uniform(-1.2, 1.2), random.uniform(-0.3, 0.1), random.uniform(-0.3, 0.2)))
               for _ in range(5)] for _ in hearts]

# --- the bolt: one main channel of sharp kinks, a second strand along it, a few short branches, and the stroke
# spreading in the cloud. All of it stays close to the diagonal: centred, not scattered. ---------------------
M_MAIN, M_STRAND, M_BR, M_CL = pair("Coeur", WHITE), pair("Brin", PINK), pair("Branche", WHITE), pair("Ciel", PINK)
M_LEAD, M_CRK, M_ARC = pair("Traceur", PINK), pair("Crepitement", PINK), pair("Arc", WHITE)
random.seed(11)
MAIN = jag(TOP, (0, 0, 0), [0.10, 0.13, 0.14, 0.13, 0.12, 0.10], 0.2)
core, sheath = line("eclair", MAIN, 0.014, M_MAIN)


def weave(path, amp, waves, seed):
    """A strand twisting around a channel: it crosses it a few times, pinned to it at both ends."""
    r = random.Random(seed)
    ph = r.uniform(0, 2 * math.pi)
    out = []
    for i, p in enumerate(path):
        t = i / (len(path) - 1)
        d = path[min(i + 1, len(path) - 1)] - path[max(i - 1, 0)]
        side = Vector((-d.z, 0, d.x)).normalized()
        out.append(p + side * amp * (math.sin(t * math.pi * waves + ph) * math.sin(t * math.pi) + r.uniform(-0.3, 0.3)))
    return out


strand = line("brin", weave(MAIN, 0.1, 5, 13), 0.006, M_STRAND)


def branch_paths(path, count, lo, hi, reach, seed, spread=(18.0, 50.0)):
    r = random.Random(seed)
    out = []
    for _ in range(count):
        i = r.randint(int(len(path) * lo), int(len(path) * hi))
        s = path[i]
        d = (path[min(i + 6, len(path) - 1)] - path[max(i - 6, 0)]).normalized()
        a = math.radians(r.uniform(*spread)) * r.choice((-1, 1))
        dd = Vector((d.x * math.cos(a) - d.z * math.sin(a), 0, d.x * math.sin(a) + d.z * math.cos(a)))
        out.append(jag(s, s + dd * r.uniform(*reach), [0.16, 0.15, 0.13, 0.12], 0.2, rng=r))
    return out


branch_objs = []
for k, bp in enumerate(branch_paths(MAIN, 5, 0.1, 0.85, (0.35, 1.1), 5)):
    branch_objs += line("branche", bp, 0.008, M_BR, taper=True)
    if k % 2 == 0:  # a twig on every other branch
        for tp in branch_paths(bp, 1, 0.3, 0.6, (0.15, 0.4), 50 + k):
            branch_objs += line("rameau", tp, 0.006, M_BR, taper=True)
cloud_objs = []
r_ = random.Random(19)
for s in (-1, -1, 1):
    e = TOP + Vector((s * r_.uniform(0.5, 1.1), r_.uniform(-0.2, 0.2), r_.uniform(0.25, 0.6)))
    cloud_objs += line("ciel", jag(TOP, e, [0.2, 0.18, 0.15, 0.12], rng=r_), 0.007, M_CL, taper=True)

# --- the leader: the same channel creeping down (half, then nearly all of it), feelers at its tip; and the
# streamer rising from the ground to meet it ------------------------------------------------------------------
leaders = {}
for k, u in ((-2, 0.5), (-1, 0.93)):
    pre = MAIN[:max(2, int(len(MAIN) * u))]
    objs = line("traceur", pre, 0.005, M_LEAD)
    for fp in branch_paths(pre, 3 if k == -2 else 4, 0.55, 0.98, (0.12, 0.34), 70 + k, (25.0, 60.0)):
        objs += line("antenne", fp, 0.004, M_LEAD, taper=True)
    leaders[k] = objs
up = MAIN[-12].normalized()
leaders[-1] += line("remontee", jag((0, 0, 0.01), up * 0.32, [0.2, 0.18, 0.15], rng=random.Random(3)), 0.004, M_LEAD,
                    taper=True)

# --- small crackles inside the cloud while it charges --------------------------------------------------------
crackles = []
r_ = random.Random(23)
for f in (5, 9, 12):
    a = CLOUD_C + Vector((r_.uniform(-1.3, 0.6), -0.45, r_.uniform(-0.35, 0.15)))
    b = a + Vector((r_.uniform(0.5, 0.9), r_.uniform(-0.1, 0.1), r_.uniform(-0.3, 0.2)))
    crackles.append((f, line("crepitement", jag(a, b, [0.25, 0.2, 0.18, 0.14], rng=r_), 0.005, M_CRK, taper=True),
                     (a + b) / 2))

# --- the warning on the ground, until the strike: two rings closing in and static rising inside them -----------
m_ring, e_ring = mat_emit("Anneau", RED)
m_ring2, e_ring2 = mat_emit("AnneauLarge", RED)
ring = curve("anneau", ring_pts(0.6), 0.012, m_ring)
ring2 = curve("anneau_large", ring_pts(0.75), 0.008, m_ring2)
m_stat, e_stat = mat_emit("Statique", PINK, 2.5)
static = [curve("statique", [Vector((0, 0, 0)), Vector((0, 0, 0.1))], 0.004, m_stat) for _ in range(8)]

# --- the ground takes the hit: a white-hot point, arcs crawling on the floor, a shock ring, sparks, embers -------
m_hot, e_hot = mat_emit("PointChaud", WHITE)
bpy.ops.mesh.primitive_uv_sphere_add(radius=0.055, segments=16, ring_count=8, location=(0, 0, 0.03))
hot = bpy.context.active_object
hot.scale = (1.4, 1.0, 0.7)
hot.data.materials.append(m_hot)
LINES.append(hot)
arc_sets = []
for j, (n, rlo, rhi, seed) in enumerate(((5, 0.6, 1.4, 41), (4, 0.4, 1.0, 42), (2, 0.3, 0.7, 43))):
    r_ = random.Random(seed)
    objs = []
    for i in range(n):
        th = r_.uniform(-0.6, 0.6) + (0.0 if i % 2 else math.pi)
        R = r_.uniform(rlo, rhi)
        end = Vector((R * math.cos(th), R * math.sin(th) * 0.9, 0.006))
        objs += line("arc", jag((0, 0, 0.01), end, [0.25, 0.22, 0.2, 0.16, 0.12], 0.3, ground=True, rng=r_),
                     0.006 if j == 0 else 0.005, M_ARC, taper=True)
    arc_sets.append(objs)
m_wave, e_wave = mat_emit("Onde", PINK)
wave = curve("onde", ring_pts(0.4), 0.014, m_wave)
m_spark, e_spark = mat_emit("Etincelle", PINK)
sparks = []
for _ in range(34):
    ob = curve("etincelle", [Vector((0, 0, 0)), Vector((0, 0, 0.01))], random.uniform(0.005, 0.01), m_spark)
    sparks.append((ob, Vector((random.uniform(-2.3, 2.3), random.uniform(-0.9, 0.9), random.uniform(1.0, 3.6)))))
m_ember, e_ember = mat_emit("Braise", PINK)
embers = []
for _ in range(14):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1, radius=1.0)
    s = bpy.context.active_object
    s.data.materials.append(m_ember)
    LINES.append(s)
    p0 = Vector((random.uniform(-0.8, 0.8), random.uniform(-0.4, 0.4), random.uniform(0.0, 0.25)))
    embers.append((s, p0, Vector((random.uniform(-0.25, 0.25), 0, random.uniform(0.3, 0.7))),
                   random.uniform(0.01, 0.018), random.randint(1, 3)))

# --- soft light and smoke of the impact (the denoised render): the flash, the scorch, the rising smoke ---------
flash, flash_s = light_ball("Eclat", (0, 0, 0.06), (0.9, 0.5, 0.38), PINK)
bpy.ops.mesh.primitive_circle_add(vertices=96, radius=1.0, fill_type='TRIFAN', location=(0, 0, 0.002))
disc = bpy.context.active_object
m_disc, e_disc, veil = scorch_mat("Brulure")
disc.data.materials.append(m_disc)
disc.scale = (1.1, 1.1, 1)
SMOKE.append(disc)
puff, puff_noise, puff_gain, (puff_glow,) = smoke_volume("Fumee", (0.45, 0.38, 0.32), (0, 0, 0.3), 3.0, [(RED, 0.9)],
                                                         albedo="#5A3236", emit=DEEP, self_emit=0.03)


def lerp_path(path, u):
    u = max(0.0, min(1.0, u)) * (len(path) - 1)
    i = min(int(u), len(path) - 2)
    return path[i].lerp(path[i + 1], u - i)


def state(f):
    k = f - STRIKE
    # the cloud gathers (f0-8), boils, is lit from inside by the strike, then thins out and is gone at the end
    for i, nz in enumerate(cloud_noise):
        nz.inputs['W'].default_value = f * 0.06 + 10 * i
    grow = ease(f / 8)
    cloud_gain.outputs[0].default_value = 11.0 * grow * (1 - ease((f - 17) / 7))
    s = 0.85 + 0.15 * grow + 0.012 * max(k, 0)
    cloud.scale = (2.3 * s, 0.9 * s, 1.0 * s)
    charge = ease(f / 14)
    for i, g in enumerate(hearts):
        flick = 0.7 + 0.3 * math.sin(f * 2.3 + i * 1.9)
        g.set(lerp_path(heart_path[i], (f / N + i * 0.37) % 1.0), (0.1 + 0.2 * charge) * flick if k < 0 else 0.0)
    on = [c for c in crackles if c[0] == f]
    for cf, objs, _ in crackles:
        show(objs, cf == f)
    strength(M_CRK, 2.2, 0.3)
    crack_glow.set(on[0][2] if on else (0, 0, 0), 0.9 if on else 0.0)
    flare.set(TOP + Vector((0, -0.1, 0.35)), {0: 1.0, 1: 0.4, 2: 0.12}.get(k, 0.0))
    # the warning on the ground
    u = f / (STRIKE - 1)
    show([ring, ring2], k < 0)
    if k < 0:
        r = 0.62 - 0.12 * u * u
        set_pts(ring, ring_pts(r))
        set_pts(ring2, ring_pts(r * 1.25))
        e_ring.inputs['Strength'].default_value = (0.9 + 1.8 * u) * ease(f / 3)
        e_ring2.inputs['Strength'].default_value = (0.25 + 0.4 * u) * ease(f / 3)
    rs = random.Random(1000 + f)
    for ob in static:
        live = k < 0 and f >= 3 and rs.random() < 0.6
        ob.hide_render = not live
        if live:
            a = rs.uniform(0, 2 * math.pi)
            rr = rs.uniform(0.15, 0.9) * (0.62 - 0.12 * u * u)
            base = Vector((rr * math.cos(a), rr * math.sin(a), rs.uniform(0.0, 0.2)))
            set_pts(ob, [base, base + Vector((0, 0, rs.uniform(0.05, 0.14)))])
    # the leader creeps down, then the stroke: one strike, a flicker, a red trace, gone
    for kk, objs in leaders.items():
        show(objs, k == kk)
    strength(M_LEAD, {-2: 1.6, -1: 2.4}.get(k, 0.0), {-2: 0.22, -1: 0.32}.get(k, 0.0))
    show([core], k in (0, 1))
    show([sheath], k in (0, 1, 2))
    core.data.bevel_depth = {0: 0.014, 1: 0.008}.get(k, 0.008)
    sheath.data.bevel_depth = {0: 0.036, 1: 0.02, 2: 0.012}.get(k, 0.012)
    strength(M_MAIN, {0: 9.0, 1: 1.0}.get(k, 0.0), {0: 1.2, 1: 0.3, 2: 0.12}.get(k, 0.0))
    show(strand, k == 0)
    strength(M_STRAND, 2.5, 0.35)
    show(branch_objs, k in (0, 1))
    strength(M_BR, {0: 4.5, 1: 0.6}.get(k, 0.0), {0: 0.55, 1: 0.08}.get(k, 0.0))
    show(cloud_objs, k in (0, 1))
    strength(M_CL, {0: 3.0, 1: 0.7}.get(k, 0.0), {0: 0.4, 1: 0.1}.get(k, 0.0))
    # the ground: white-hot point, arcs (a new set each frame), flash, shock ring, sparks, embers, scorch, smoke
    hot.hide_render = k not in (0, 1)
    e_hot.inputs['Strength'].default_value = {0: 14.0, 1: 5.0}.get(k, 0.0)
    for j, objs in enumerate(arc_sets):
        show(objs, k == j)
    strength(M_ARC, {0: 6.0, 1: 3.5, 2: 1.8}.get(k, 0.0), {0: 0.7, 1: 0.45, 2: 0.25}.get(k, 0.0))
    fs = {0: 9.0, 1: 3.5, 2: 1.2, 3: 0.4, 4: 0.12}.get(k, 0.0)
    flash.hide_render = fs == 0
    flash_s.outputs[0].default_value = fs
    j = k - 1
    wave.hide_render = not (0 <= j <= 5)
    if 0 <= j <= 5:
        set_pts(wave, ring_pts(0.35 + 0.26 * j))
        e_wave.inputs['Strength'].default_value = 2.2 * (1 - j / 6) ** 1.5
    ts = k / 15
    e_spark.inputs['Strength'].default_value = max(0.0, 5 * (1 - ts / 0.62))
    for ob, v in sparks:
        p = v * ts + Vector((0, 0, -0.5 * 7.0 * ts * ts))
        vel = v + Vector((0, 0, -7.0 * ts))
        alive = 0 <= k and p.z >= 0 and ts < 0.6
        ob.hide_render = not alive
        if alive:
            set_pts(ob, [p, p - vel * 0.03])
    for s_, p0, v, r, k0 in embers:
        jj = k - k0
        s_.hide_render = jj < 0
        if jj >= 0:
            s_.location = p0 + v * (jj / 8) + Vector((0.05 * math.sin(jj * 0.9 + p0.x * 5), 0, 0))
            s_.scale = (r * max(0.3, 1 - jj / 14),) * 3
    e_ember.inputs['Strength'].default_value = 3.0 * (1 - ease((k - 4) / 5))
    disc.hide_render = k < 0
    e_disc.inputs['Strength'].default_value = 0 if k < 0 else (1.7 * math.exp(-k / 3.0) + 0.35) * (1 - ease((k - 5) / 4))
    veil.inputs['To Min'].default_value = 0 if k < 1 else min(0.5, 0.14 * k) * (1 - ease((k - 5) / 4))
    disc.scale = (1.1 * (1 + 0.04 * max(k, 0)),) * 2 + (1,)
    puff.hide_render = k < 2
    if k >= 2:
        jj = k - 2
        puff.location = (0, 0.05, 0.3 + 0.08 * jj)
        puff.scale = (0.45 + 0.06 * jj, 0.38 + 0.03 * jj, 0.32 + 0.04 * jj)
        for i, nz in enumerate(puff_noise):
            nz.inputs['W'].default_value = 3.0 + jj * 0.12 + 10 * i
        puff_gain.outputs[0].default_value = 12.0 * min(1.0, jj / 2 + 0.4) * (1 - ease((k - 4) / 5))
        puff_glow.set((0, -0.1, 0.0), 1.2 * math.exp(-jj / 3.0) + 0.15)


def render(path, objs_off, samples, denoise, filt):
    keep = [(ob, ob.hide_render) for ob in objs_off]
    for ob, _ in keep:
        ob.hide_render = True
    sc.cycles.samples = samples
    sc.cycles.use_denoising = denoise
    try:
        sc.cycles.filter_width = filt
    except Exception:
        pass
    sc.render.filepath = path
    bpy.ops.render.render(write_still=True)
    for ob, h in keep:
        ob.hide_render = h


for sub in ("fumee", "trait"):
    os.makedirs(os.path.join(OUT, sub), exist_ok=True)
FRAMES = [int(x) for x in os.environ.get("CH_FRAMES", "").split(",") if x.strip()] or range(N)  # test a few
for f in FRAMES:
    state(f)
    sc.frame_set(f + 1)
    render(os.path.join(OUT, "fumee", "f%02d.exr" % f), LINES, SAMPLES, True, 1.5)
    render(os.path.join(OUT, "trait", "f%02d.exr" % f), SMOKE, 24, False, 1.0)
    print("IMAGE", f, flush=True)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "eclair6.blend"))
print("FIN", OUT)
