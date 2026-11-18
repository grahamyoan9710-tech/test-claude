# Clash Horizon - the KO blast, Blender, v2 (yoyo, 18/11: the torn screen "ne va pas avec le jeu"; feel of Smash
# Bros and Brawlhalla, everything original): where a fighter leaves the screen, a jet of light bursts inward from the
# edge, made of tapered streaks (a white-hot core, the fighter's colour around it), speed lines racing ahead, a few
# broken shock arcs running along the edge, an uneven star of a flash and sparks flung in; it thins and is gone.
# No disc, ring or cone: streaks are tapered drops, the shock is broken arcs.
# White only: the game tints it in the fighter's colour. 20 frames of 700x720 at 30 fps, the screen edge at the
# bottom (anchor 350, 700), inward is up (the game turns it by quarter turns). The last frame is empty: the game
# holds it while the KO lasts.
# Usage: blender -b -P ejection.py -- <out_dir> [samples]   (env CH_FRAMES=1,3 renders only those frames)
# Writes out_dir/fNN.exr: linear, premultiplied RGBA on a transparent film (the glow is added by post_ko.py).
import bpy, sys, os, math, random
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
OUT = argv[0]
SAMPLES = int(argv[1]) if len(argv) > 1 else 24
W, H, N = 700, 720, 20
AX, AY = 350, 700
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


def ease(x):
    x = max(0.0, min(1.0, x))
    return x * x * (3 - 2 * x)


# --- camera: flat, orthographic, the anchor (the screen edge) at (AX, AY) ----------------------------------
cam = bpy.data.cameras.new("Camera")
cam.type = 'ORTHO'
cam.sensor_fit = 'VERTICAL'
cam.ortho_scale = H / 100
co = link(bpy.data.objects.new("Camera", cam))
co.location = ((W / 2 - AX) / 100, -20, (AY - H / 2) / 100)
co.rotation_euler = (math.pi / 2, 0, 0)
sc.camera = co

# --- the jet: tapered streaks from the edge inward, a narrow white core inside a wider fan -------------------


def drop(name, L, w, mat):
    """A tapered streak along +z, L long: round at its base, widest near it, sharp at its tip."""
    n = 24
    prof = [w * (min(1.0, s / 0.12) ** 0.5) * ((1 - s) / 0.88 if s > 0.12 else 1.0) ** 1.3
            for s in (i / n for i in range(n + 1))]
    verts = [(-hw, 0, L * i / n) for i, hw in enumerate(prof)] + [(hw, 0, L * i / n) for i, hw in enumerate(prof)]
    faces = [(i, i + 1, n + 2 + i, n + 1 + i) for i in range(n)]
    me = bpy.data.meshes.new(name)
    me.from_pydata(verts, [], faces)
    me.materials.append(mat)
    return link(bpy.data.objects.new(name, me))


r_ = random.Random(7)
m_core, e_core = mat_emit("Coeur", 1.0)
m_jet, e_jet = mat_emit("Jet", 0.8)
jets = []  # (object, angle, length, delay)
for i in range(7):  # the core: long, narrow, nearly straight in
    th = math.radians(r_.uniform(-3.5, 3.5))
    L, w = r_.uniform(4.2, 5.4), r_.uniform(0.07, 0.12)
    jets.append((drop("coeur", L, w, m_core), th, L, r_.randint(0, 1), 0.0))
for i in range(16):  # the fan around it, the fighter's colour once tinted
    th = math.radians(r_.choice((-1, 1)) * r_.uniform(2.0, 15.0))
    L, w = r_.uniform(2.2, 4.4) * (1 - abs(th) * 1.2), r_.uniform(0.1, 0.22)
    jets.append((drop("jet", L, w, m_jet), th, L, r_.randint(0, 2), r_.uniform(-0.15, 0.15)))

# --- speed lines racing ahead of it ----------------------------------------------------------------------
m_line, e_line = mat_emit("Vitesse", 0.95)
lines = []
for i in range(12):
    x = r_.choice((-1, 1)) * r_.uniform(0.25, 1.9)
    L = r_.uniform(0.9, 2.0)
    ob = curve("vitesse", [Vector((0, 0, 0)), Vector((0, 0, L))], r_.uniform(0.006, 0.011), m_line, taper=True)
    lines.append((ob, x, r_.uniform(0.3, 1.6), r_.uniform(9, 15), r_.randint(0, 2)))

# --- the shock: a few broken arcs running out along the edge ----------------------------------------------
m_arc, e_arc = mat_emit("Choc", 1.0)
arcs = []
for side in (-1, 1):
    for j in range(2):
        a0 = r_.uniform(8, 25) if j == 0 else r_.uniform(40, 60)
        a1 = a0 + r_.uniform(22, 38)
        arcs.append((side, math.radians(a0), math.radians(a1), j, r_.uniform(0.9, 1.1)))
arc_obs = [curve("choc", [Vector((0, 0, 0))] * 13, 0.02, m_arc) for _ in arcs]


def arc_pts(side, a0, a1, r):
    pts = []
    for i in range(13):
        a = a0 + (a1 - a0) * i / 12
        rr = r
        pts.append(Vector((side * rr * math.cos(a), 0, rr * math.sin(a) * 0.55)))
    return pts


# --- the flash: an uneven star of short rays at the edge point --------------------------------------------
m_flash, e_flash = mat_emit("Eclair", 1.0)
star = []
for i in range(20):
    a = math.pi * i / 10 + r_.uniform(-0.08, 0.08)
    rr = (r_.uniform(0.55, 1.25) if i % 2 == 0 else 0.12)
    star.append((rr * math.cos(a), 0, abs(rr * math.sin(a)) * (1 if math.sin(a) >= 0 else 0.25)))
me = bpy.data.meshes.new("eclair")
me.from_pydata([(0, 0, 0)] + star, [], [(0, i + 1, (i + 1) % 20 + 1) for i in range(20)])
me.materials.append(m_flash)
flash = link(bpy.data.objects.new("eclair", me))

# --- sparks flung inward ---------------------------------------------------------------------------------
m_spark, e_spark = mat_emit("Etincelle", 1.0)
sparks = []
for _ in range(30):
    s = r_.uniform(0.03, 0.07)
    me = bpy.data.meshes.new("etincelle")
    me.from_pydata([(0, 0, -s * 2.5), (s * 0.45, 0, 0), (0, 0, s * 2.5), (-s * 0.45, 0, 0)], [], [(0, 1, 2, 3)])
    me.materials.append(m_spark)
    ob = link(bpy.data.objects.new("etincelle", me))
    a = math.radians(r_.uniform(-40, 40))
    v = r_.uniform(4.0, 11.0)
    sparks.append((ob, Vector((math.sin(a) * v, 0, math.cos(a) * v)), a, r_.randint(0, 3)))


def state(f):
    tt = f / 30
    # the jet bursts in (f0-3), holds, then thins and lifts off the edge (f5-15)
    e_core.inputs['Strength'].default_value = 9.0 * (1 - ease((f - 5) / 10))
    e_jet.inputs['Strength'].default_value = 5.0 * (1 - ease((f - 4) / 10))
    for ob, th, L, d, sway in jets:
        j = f - d
        ob.hide_render = j < 0 or f > 15
        if j < 0:
            continue
        grow = (0.35, 0.75, 0.95)[j] if j < 3 else 1.0 + 0.02 * (j - 3)
        thin = 1.0 if j < 4 else max(0.05, 1 - ease((j - 4) / 10))
        lift = 0.0 if j < 4 else 0.07 * (j - 4) ** 1.2
        ob.scale = (thin * (1.25 if j == 0 else 1.0), 1, grow)
        ob.rotation_euler = (0, th + sway * tt, 0)
        ob.location = (math.sin(th) * lift, 0, math.cos(th) * lift)
    # speed lines race in, a frame or two each
    e_line.inputs['Strength'].default_value = 6.0
    for ob, x, z0, sp, d in lines:
        j = f - d
        ob.hide_render = not (0 <= j < 6)
        ob.location = (x, 0, z0 + sp * j / 30 * 3)
        ob.scale = (1, 1, 1 - j / 7)
    # the shock arcs run out along the edge and thin
    for (side, a0, a1, j, k), ob in zip(arcs, arc_obs):
        g = f - j
        ob.hide_render = not (0 <= g <= 8)
        if 0 <= g <= 8:
            sp = ob.data.splines[0]
            for i, p in enumerate(arc_pts(side, a0, a1, (0.5 + 0.34 * g) * k)):
                sp.points[i].co = (p.x, p.y, p.z, 1)
            ob.data.bevel_depth = 0.014 * (1 - 0.7 * g / 8)
    e_arc.inputs['Strength'].default_value = 5.0 * (1 - f / 10) ** 1.2 if f < 10 else 0.0
    # the flash: three frames
    flash.hide_render = f > 2
    e_flash.inputs['Strength'].default_value = (8.0, 4.0, 1.5)[f] if f < 3 else 0.0
    flash.scale = ((1.0, 1.35, 1.6)[f] if f < 3 else 1.0,) * 3
    # sparks fly in and slow down
    e_spark.inputs['Strength'].default_value = 6.0 * (1 - ease((f - 6) / 9))
    for ob, v, a, d in sparks:
        j = f - d
        ob.hide_render = j < 0 or f > 15
        s = j / 30
        ob.location = v * s * (1 - 0.9 * s)
        ob.rotation_euler = (0, a, 0)


os.makedirs(OUT, exist_ok=True)
FRAMES = [int(x) for x in os.environ.get("CH_FRAMES", "").split(",") if x.strip()] or range(N)
for f in FRAMES:
    state(f)
    sc.frame_set(f + 1)
    sc.render.filepath = os.path.join(OUT, "f%02d.exr" % f)
    bpy.ops.render.render(write_still=True)
    print("IMAGE", f, flush=True)
print("FIN", OUT)
