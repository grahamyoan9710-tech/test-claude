# Clash Horizon - the KO blast, Blender (yoyo, 12/11: « bien énervé », « limite ça déchire l'écran »): where a
# fighter leaves the screen, the screen tears open. A jagged rift of light rips inward, the glass around it cracks
# in a web, shards and streaks fly in, a shock wave runs along the edge; then the rift closes and the cracks fade.
# White only: the game tints it in the fighter's colour. 20 frames of 700x720 at 30 fps, the screen edge at the
# bottom (anchor 350, 700), inward is up (the game turns it by quarter turns). The last frame is empty: the game
# holds it while the KO lasts.
# Usage: blender -b -P dechirure.py -- <out_dir> [samples]   (env CH_FRAMES=1,3 renders only those frames)
# Writes out_dir/fNN.exr: linear, premultiplied RGBA on a transparent film (the glow is added by post_ko.py).
import bpy, sys, os, math, random
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
OUT = argv[0]
SAMPLES = int(argv[1]) if len(argv) > 1 else 24
W, H, N = 700, 720, 20
AX, AY = 350, 700
L = 6.0  # the rift's length (600 px)
random.seed(5)

bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
sc.render.engine = 'CYCLES'
sc.cycles.device = 'CPU'
sc.cycles.samples = SAMPLES
sc.cycles.use_denoising = False
sc.cycles.max_bounces = 0
sc.cycles.transparent_max_bounces = 32
try:
    sc.cycles.filter_width = 1.0
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


def tree(m):
    try:
        m.use_nodes = True  # deprecated (always on) in Blender 5
    except Exception:
        pass
    nt = m.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    return nt


def mat_emit(name, grey=1.0):
    """Additive white light: emission plus transparency."""
    m = bpy.data.materials.new(name)
    nt = tree(m)
    e = nt.nodes.new('ShaderNodeEmission')
    e.inputs['Color'].default_value = (grey, grey, grey, 1)
    e.inputs['Strength'].default_value = 0.0
    t = nt.nodes.new('ShaderNodeBsdfTransparent')
    a = nt.nodes.new('ShaderNodeAddShader')
    o = nt.nodes.new('ShaderNodeOutputMaterial')
    nt.links.new(e.outputs[0], a.inputs[0])
    nt.links.new(t.outputs[0], a.inputs[1])
    nt.links.new(a.outputs[0], o.inputs['Surface'])
    return m, e


def link(ob):
    sc.collection.objects.link(ob)
    return ob


def jag(a, b, amps, rng=random):
    """Midpoint displacement in the picture plane (x, z), one amplitude per level."""
    pts = [Vector(a), Vector(b)]
    for amp in amps:
        new = [pts[0]]
        for p, q in zip(pts, pts[1:]):
            d = q - p
            side = Vector((-d.z, 0, d.x))
            side = side.normalized() if side.length > 1e-6 else Vector((1, 0, 0))
            new += [(p + q) / 2 + side * rng.uniform(-amp, amp) * d.length, q]
        pts = new
    return pts


def curve(name, pts, radius, mat, taper=False, parent=None):
    cu = bpy.data.curves.new(name, 'CURVE')
    cu.dimensions = '3D'
    cu.bevel_depth = radius
    cu.bevel_resolution = 1
    sp = cu.splines.new('POLY')
    sp.points.add(len(pts) - 1)
    for i, p in enumerate(pts):
        sp.points[i].co = (p.x, p.y, p.z, 1)
        sp.points[i].radius = (1.0 - 0.8 * i / (len(pts) - 1)) if taper else 1.0
    cu.materials.append(mat)
    ob = link(bpy.data.objects.new(name, cu))
    if parent:
        ob.parent = parent
    return ob


def reveal(ob, pts, u):
    """Shows the first u (0-1) of a line: the rest folds onto its last shown point."""
    m = max(1, int(round(u * (len(pts) - 1))))
    sp = ob.data.splines[0]
    for k in range(len(pts)):
        p = pts[min(k, m)]
        sp.points[k].co = (p.x, p.y, p.z, 1)


def ease(x):
    x = max(0.0, min(1.0, x))
    return x * x * (3 - 2 * x)


def table(d, f):
    return d.get(f, 0.0)


# --- camera: flat, orthographic, the anchor (the screen edge) at (AX, AY) ----------------------------------
cam = bpy.data.cameras.new("Camera")
cam.type = 'ORTHO'
cam.sensor_fit = 'VERTICAL'
cam.ortho_scale = H / 100
co = link(bpy.data.objects.new("Camera", cam))
co.location = ((W / 2 - AX) / 100, -20, (AY - H / 2) / 100)
co.rotation_euler = (math.pi / 2, 0, 0)
sc.camera = co

# --- the flash where the fighter left: a disc of light, white at the edge point -----------------------------
m_flash = bpy.data.materials.new("Flash")
nt = tree(m_flash)
tc = nt.nodes.new('ShaderNodeTexCoord')
ln = nt.nodes.new('ShaderNodeVectorMath')
ln.operation = 'LENGTH'
nt.links.new(tc.outputs['Object'], ln.inputs[0])
mr = nt.nodes.new('ShaderNodeMapRange')
mr.inputs['From Min'].default_value, mr.inputs['From Max'].default_value = 0.0, 1.0
mr.inputs['To Min'].default_value, mr.inputs['To Max'].default_value = 1.0, 0.0
nt.links.new(ln.outputs['Value'], mr.inputs['Value'])
pw = nt.nodes.new('ShaderNodeMath')
pw.operation = 'POWER'
pw.inputs[1].default_value = 3.0
nt.links.new(mr.outputs['Result'], pw.inputs[0])
flash_val = nt.nodes.new('ShaderNodeValue')
mul = nt.nodes.new('ShaderNodeMath')
mul.operation = 'MULTIPLY'
nt.links.new(pw.outputs[0], mul.inputs[0])
nt.links.new(flash_val.outputs[0], mul.inputs[1])
e = nt.nodes.new('ShaderNodeEmission')
nt.links.new(mul.outputs[0], e.inputs['Strength'])
t = nt.nodes.new('ShaderNodeBsdfTransparent')
a = nt.nodes.new('ShaderNodeAddShader')
o = nt.nodes.new('ShaderNodeOutputMaterial')
nt.links.new(e.outputs[0], a.inputs[0])
nt.links.new(t.outputs[0], a.inputs[1])
nt.links.new(a.outputs[0], o.inputs['Surface'])
bpy.ops.mesh.primitive_circle_add(vertices=96, radius=1.0, fill_type='TRIFAN', location=(0, 0.5, 0),
                                  rotation=(math.pi / 2, 0, 0))
flash = bpy.context.active_object
flash.data.materials.append(m_flash)

# --- the rift: a torn gap from the edge inward, dark inside, with jagged white-hot lips and a live thread --
rift = link(bpy.data.objects.new("Dechirure", None))
r_ = random.Random(9)
mid = jag((0, 0, 0), (r_.uniform(-0.25, 0.25), 0, L), [0.05, 0.07, 0.08, 0.07, 0.06, 0.05], rng=r_)
lips = ([], [])
for i, p in enumerate(mid):
    tt = i / (len(mid) - 1)
    d = mid[min(i + 1, len(mid) - 1)] - mid[max(i - 1, 0)]
    side = Vector((-d.z, 0, d.x)).normalized()
    for s, lip in zip((1, -1), lips):
        hw = 0.62 * (1 - tt) ** 0.75 * (1 + r_.uniform(-0.45, 0.35)) if i < len(mid) - 1 else 0.0
        lip.append(p + side * s * hw)
me = bpy.data.meshes.new("Dechirure")
verts = [tuple(p) for p in mid] + [tuple(p) for p in lips[0]] + [tuple(p) for p in lips[1]]
n = len(mid)
faces = []
for i in range(n - 1):
    faces.append((i, i + 1, n + i + 1, n + i))
    faces.append((i, 2 * n + i, 2 * n + i + 1, i + 1))
me.from_pydata(verts, [], faces)
m_fill = bpy.data.materials.new("Faille")  # the gap: nearly black and nearly opaque (the tint cannot light it)
nt = tree(m_fill)
dark = nt.nodes.new('ShaderNodeEmission')
dark.inputs['Color'].default_value = (0.004, 0.003, 0.005, 1)
tr = nt.nodes.new('ShaderNodeBsdfTransparent')
gap = nt.nodes.new('ShaderNodeMixShader')
nt.links.new(tr.outputs[0], gap.inputs[1])
nt.links.new(dark.outputs[0], gap.inputs[2])
o = nt.nodes.new('ShaderNodeOutputMaterial')
nt.links.new(gap.outputs[0], o.inputs['Surface'])
me.materials.append(m_fill)
fill = link(bpy.data.objects.new("Faille", me))
fill.parent = rift
m_lip, e_lip = mat_emit("Levres")
lip_objs = [curve("levre", lip, 0.011, m_lip, parent=rift) for lip in lips]
m_core, e_core = mat_emit("Filament")
core = curve("filament", jag(mid[0], mid[-8], [0.08, 0.08, 0.07], rng=r_), 0.007, m_core, taper=True, parent=rift)

# --- the glass cracks around it: rays from the edge point, a twig or two, rings linking them (a web) ----------
m_crack, e_crack = mat_emit("Fissure", 0.92)
rays = []
r_ = random.Random(21)
angles = [12, 27, 41, 56, 69, 111, 124, 139, 153, 168]
for ang in angles:
    a = math.radians(ang + r_.uniform(-4, 4))
    reach = min(r_.uniform(2.4, 4.3), 3.2 / max(0.2, abs(math.cos(a))), 6.6)
    end = Vector((reach * math.cos(a), 0, reach * math.sin(a)))
    path = jag((0, 0, 0), end, [0.05, 0.07, 0.06, 0.05], rng=r_)
    rays.append((a, reach, path, curve("fissure", path, 0.0085, m_crack)))
twigs = []
for a, reach, path, _ in rays:
    for _ in range(r_.randint(1, 2)):
        i = r_.randint(int(len(path) * 0.3), int(len(path) * 0.75))
        s = path[i]
        b = a + math.radians(r_.uniform(25, 45)) * r_.choice((-1, 1))
        e_ = s + Vector((math.cos(b), 0, math.sin(b))) * r_.uniform(0.4, 1.1)
        if abs(e_.x) < 3.3 and e_.z > 0.05:
            tp = jag(s, e_, [0.08, 0.07, 0.06], rng=r_)
            twigs.append((i / (len(path) - 1), tp, curve("brindille", tp, 0.006, m_crack, taper=True)))
links = []
for j in range(len(rays) - 1):
    (a0, r0, p0, _), (a1, r1, p1, _) = rays[j], rays[j + 1]
    if (a0 < math.pi / 2) != (a1 < math.pi / 2):
        continue  # not across the rift
    for frac in (0.32, 0.62):
        rr = frac * min(r0, r1)
        if rr < 0.7:
            continue
        A = Vector((rr * math.cos(a0), 0, rr * math.sin(a0)))
        B = Vector((rr * math.cos(a1), 0, rr * math.sin(a1)))
        lp = jag(A, B, [0.06, 0.06], rng=r_)
        links.append((frac, lp, curve("anneau", lp, 0.006, m_crack)))

# --- the shock wave along the edge, shards and streaks thrown inward -------------------------------------


def half_ring(r, k=72):
    return [Vector((r * math.cos(math.pi * i / k), 0, r * math.sin(math.pi * i / k))) for i in range(k + 1)]


wave_mats = [mat_emit("Onde"), mat_emit("Onde2")]
waves = [curve("onde", half_ring(1.0), 0.04, wave_mats[0][0]), curve("onde2", half_ring(1.0), 0.02, wave_mats[1][0])]
m_shard, e_shard = mat_emit("Eclat")
shards = []
for _ in range(26):
    s = r_.uniform(0.07, 0.2)
    me = bpy.data.meshes.new("eclat")
    pts = [(s * math.cos(q), 0, s * math.sin(q) * r_.uniform(0.4, 1.0))
           for q in (r_.uniform(0, 2), r_.uniform(2.1, 4.1), r_.uniform(4.2, 6.2))]
    me.from_pydata(pts, [], [(0, 1, 2)])
    me.materials.append(m_shard)
    ob = link(bpy.data.objects.new("eclat", me))
    p0 = Vector((r_.uniform(-0.5, 0.5), 0, r_.uniform(0.2, 1.4)))
    v = Vector((r_.choice((-1, 1)) * r_.uniform(1.0, 3.6), 0, r_.uniform(2.5, 7.5)))
    shards.append((ob, p0, v, r_.uniform(0, 6.3), r_.uniform(-14, 14)))
m_streak, e_streak = mat_emit("Trainee")
streaks = []
for _ in range(12):
    x = r_.choice((-1, 1)) * r_.uniform(0.15, 1.5)
    ln_ = r_.uniform(0.8, 2.2)
    ob = curve("trainee", [Vector((x, 0, 0)), Vector((x, 0, ln_))], r_.uniform(0.004, 0.008), m_streak, taper=True)
    streaks.append((ob, x, ln_, r_.uniform(0.0, 1.5), r_.uniform(8, 14)))


def state(f):
    # flash: blinding at once, gone in a few frames
    flash_val.outputs[0].default_value = table({0: 3.5, 1: 1.5, 2: 0.6, 3: 0.2, 4: 0.06}, f)
    flash.scale = (1.4 + 0.4 * f,) * 3
    flash.hide_render = f > 4
    # the rift rips in (f0-2), quivers open (f3-6), then closes (f7-14)
    sz = table({0: 0.35, 1: 0.8}, f) or 1.0
    sx = table({0: 0.6, 1: 0.95, 2: 1.12, 3: 1.0, 4: 1.06, 5: 0.97, 6: 0.94}, f) or 0.9 * (1 - ease((f - 6) / 8))
    rift.scale = (max(sx, 0.02), 1, sz)
    on = f <= 14
    fill.hide_render = not on or sx < 0.05
    show = 1 - ease((f - 8) / 6)
    gap.inputs['Fac'].default_value = 0.9 * (1 if f < 7 else show)
    e_lip.inputs['Strength'].default_value = 9.0 * (1 if f < 7 else show)
    e_core.inputs['Strength'].default_value = 14.0 * (1 if f < 7 else show) * (0.55 + 0.45 * abs(math.sin(f * 1.9)))
    for ob in lip_objs + [core]:
        ob.hide_render = not on
    # the cracks shoot out from the edge (f1-3), hold, then fade (f8-16)
    grow = table({0: 0.0, 1: 0.45, 2: 0.85}, f) if f < 3 else 1.0
    for _, _, path, ob in rays:
        ob.hide_render = grow <= 0 or f > 16
        if grow > 0:
            reveal(ob, path, grow)
    for u0, tp, ob in twigs:
        ob.hide_render = grow < u0 + 0.1 or f > 16
    for frac, lp, ob in links:
        ob.hide_render = grow < frac + 0.25 or f > 16
    e_crack.inputs['Strength'].default_value = 5.0 * (1 - ease((f - 8) / 8))
    # the shock wave runs out along the edge (it fades before the sides of the picture)
    for ob, (_, ew), k0, th in ((waves[0], wave_mats[0], 0, 0.04), (waves[1], wave_mats[1], 2, 0.02)):
        j = f - k0
        ob.hide_render = not (0 <= j <= 9)
        if 0 <= j <= 9:
            sp = ob.data.splines[0]
            for i, p in enumerate(half_ring(0.5 + 0.31 * j)):
                sp.points[i].co = (p.x, p.y, p.z, 1)
            ob.data.bevel_depth = th * (1 - 0.6 * j / 9)
            ew.inputs['Strength'].default_value = 4.0 * (1 - j / 10) ** 1.5
    # shards and streaks thrown inward
    tt = f / 30
    e_shard.inputs['Strength'].default_value = 3.0 * (1 - ease((f - 6) / 9))
    for ob, p0, v, a0, wv in shards:
        ob.hide_render = f < 1 or f > 15
        ob.location = p0 + v * tt * (1 - 0.6 * tt)
        ob.rotation_euler = (0, a0 + wv * tt, 0)
    e_streak.inputs['Strength'].default_value = table({0: 5.0, 1: 5.0, 2: 4.0, 3: 2.5, 4: 1.2, 5: 0.5}, f)
    for ob, x, ln_, z0, sp_ in streaks:
        ob.hide_render = f > 5
        ob.location = (0, 0, z0 + sp_ * tt)


os.makedirs(OUT, exist_ok=True)
FRAMES = [int(x) for x in os.environ.get("CH_FRAMES", "").split(",") if x.strip()] or range(N)
for f in FRAMES:
    state(f)
    sc.frame_set(f + 1)
    sc.render.filepath = os.path.join(OUT, "f%02d.exr" % f)
    bpy.ops.render.render(write_still=True)
    print("IMAGE", f, flush=True)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "dechirure.blend"))
print("FIN", OUT)
