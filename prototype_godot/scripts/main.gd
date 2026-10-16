extends Node2D
# Prototype Godot of Super Clash Bros.: Naruto against Sasuke on a sunset arena, to judge Godot's glow and 2D
# lights on the game's PixelLab clips and effects (read from ../assets/sprites). Keyboard: A/D or arrows to run,
# W or space to jump, J to attack (3-hit combo), K Rasengan, L Kurama chakra mode; Sasuke is played by a small AI.
# Command line (after --): --demo plays a scripted fight; --record DIR FROM TO EVERY saves frames.

const Fighter := preload("res://scripts/fighter.gd")
const Effect := preload("res://scripts/effect.gd")
const Sheets := preload("res://scripts/sheets.gd")
const GROUND_Y := 560.0

var lib
var p1
var p2
var cam := Camera2D.new()
var hitstop := 0
var shake := 0.0
var frame := 0
var demo := false
var record_dir := ""
var record_from := 0
var record_to := 0
var record_every := 1
var ai_timer := 1.0
var ai_hold := {}
var demo_step := 0
var demo_wait := 0


func _ready() -> void:
	var args := OS.get_cmdline_user_args()
	demo = "--demo" in args
	var r := args.find("--record")
	if r >= 0 and args.size() > r + 4:
		record_dir = args[r + 1]
		record_from = int(args[r + 2])
		record_to = int(args[r + 3])
		record_every = int(args[r + 4])
	var sprites := ProjectSettings.globalize_path("res://").path_join("../assets/sprites").simplify_path()
	lib = Sheets.new(sprites)
	build_environment()
	build_stage()
	p1 = Fighter.new()
	p1.setup(lib, self, "naruto", -260.0)
	add_child(p1)
	p2 = Fighter.new()
	p2.setup(lib, self, "sasuke", 260.0)
	add_child(p2)
	p1.opponent = p2
	p2.opponent = p1
	cam.position = Vector2(0, GROUND_Y - 170)
	cam.zoom = Vector2(1.3, 1.3)
	add_child(cam)
	cam.make_current()
	build_hud()


# Glow: every bright pixel (the effects, the sun) blooms; the canvas is a little dimmed so the 2D lights show.
func build_environment() -> void:
	var env := Environment.new()
	env.background_mode = Environment.BG_CANVAS
	env.glow_enabled = true
	env.glow_intensity = 0.7
	env.glow_strength = 1.0
	env.glow_bloom = 0.0
	env.glow_hdr_threshold = 0.93
	env.glow_blend_mode = Environment.GLOW_BLEND_MODE_SCREEN
	for i in 7:
		env.set_glow_level(i, i >= 2 and i <= 4)
	var we := WorldEnvironment.new()
	we.environment = env
	add_child(we)
	var cm := CanvasModulate.new()
	cm.color = Color(0.8, 0.78, 0.86)
	add_child(cm)


func gradient(colors: Array, offsets: Array) -> Gradient:
	var g := Gradient.new()
	g.offsets = PackedFloat32Array(offsets)
	g.colors = PackedColorArray(colors)
	return g


func layer(scroll: float) -> Parallax2D:
	var p := Parallax2D.new()
	p.scroll_scale = Vector2(scroll, scroll)
	add_child(p)
	return p


func blob(parent: Node, center: Vector2, rx: float, ry: float, color: Color) -> void:
	var pts := PackedVector2Array()
	for k in 24:
		var a := TAU * k / 24.0
		pts.append(center + Vector2(cos(a) * rx, sin(a) * ry))
	var poly := Polygon2D.new()
	poly.polygon = pts
	poly.color = color
	parent.add_child(poly)


func build_stage() -> void:
	# The sky, fixed to the screen.
	var sky_layer := CanvasLayer.new()
	sky_layer.layer = -10
	add_child(sky_layer)
	var sky := TextureRect.new()
	var gt := GradientTexture2D.new()
	gt.width = 8
	gt.height = 256
	gt.fill_from = Vector2(0, 0)
	gt.fill_to = Vector2(0, 1)
	gt.gradient = gradient([Color(0.32, 0.24, 0.5), Color(0.62, 0.36, 0.55), Color(0.98, 0.6, 0.42), Color(1.0, 0.8, 0.55)],
		[0.0, 0.45, 0.8, 1.0])
	sky.texture = gt
	sky.stretch_mode = TextureRect.STRETCH_SCALE
	sky.size = Vector2(1280, 720)
	sky_layer.add_child(sky)
	# The sun, far away: it blooms.
	var far := layer(0.05)
	var sun := Sprite2D.new()
	var st := GradientTexture2D.new()
	st.width = 512
	st.height = 512
	st.fill = GradientTexture2D.FILL_RADIAL
	st.fill_from = Vector2(0.5, 0.5)
	st.fill_to = Vector2(1.0, 0.5)
	st.gradient = gradient([Color(1, 1, 0.92, 1), Color(1, 0.95, 0.7, 1), Color(1, 0.75, 0.45, 0.35), Color(1, 0.6, 0.4, 0)],
		[0.0, 0.32, 0.36, 1.0])
	sun.texture = st
	sun.position = Vector2(0, GROUND_Y - 150)
	far.add_child(sun)
	# Clouds and mountains in layers that scroll slower than the arena.
	var clouds := layer(0.12)
	for c in [[-420, 150, 170, 26], [380, 110, 220, 30], [60, 60, 140, 18], [-60, 220, 260, 20]]:
		blob(clouds, Vector2(c[0], c[1]), c[2], c[3], Color(1.0, 0.82, 0.86, 0.7))
	var hills_far := layer(0.3)
	var pts := PackedVector2Array([Vector2(-1400, 760)])
	for k in 29:
		pts.append(Vector2(-1400 + k * 100, 470 - (60 if k % 2 == 0 else 0) - 50 * sin(k * 0.9)))
	pts.append(Vector2(1400, 760))
	var hf := Polygon2D.new()
	hf.polygon = pts
	hf.color = Color(0.55, 0.36, 0.52)
	hills_far.add_child(hf)
	var hills := layer(0.55)
	pts = PackedVector2Array([Vector2(-1400, 800)])
	for k in 15:
		pts.append(Vector2(-1400 + k * 200, 540 - 70 * abs(sin(k * 1.3))))
	pts.append(Vector2(1400, 800))
	var hn := Polygon2D.new()
	hn.polygon = pts
	hn.color = Color(0.68, 0.42, 0.48)
	hills.add_child(hn)
	# The arena: a stone platform, its grass edge.
	var ground := Polygon2D.new()
	ground.polygon = PackedVector2Array([Vector2(-700, GROUND_Y), Vector2(700, GROUND_Y), Vector2(640, GROUND_Y + 150),
		Vector2(-640, GROUND_Y + 150)])
	ground.color = Color(0.5, 0.42, 0.4)
	add_child(ground)
	var top := Polygon2D.new()
	top.polygon = PackedVector2Array([Vector2(-700, GROUND_Y - 4), Vector2(700, GROUND_Y - 4), Vector2(700, GROUND_Y + 14),
		Vector2(-700, GROUND_Y + 14)])
	top.color = Color(0.4, 0.58, 0.32)
	add_child(top)
	for k in 14:
		var stone := Polygon2D.new()
		var x0 := -640.0 + k * 92.0
		stone.polygon = PackedVector2Array([Vector2(x0, GROUND_Y + 40), Vector2(x0 + 70, GROUND_Y + 40),
			Vector2(x0 + 70, GROUND_Y + 46), Vector2(x0, GROUND_Y + 46)])
		stone.color = Color(0.42, 0.35, 0.34)
		add_child(stone)


func build_hud() -> void:
	var hud := CanvasLayer.new()
	hud.layer = 10
	add_child(hud)
	var l := Label.new()
	l.text = "Prototype Godot 4 - Naruto contre Sasuke\nA/D : courir   W : sauter   J : combo   K : Rasengan   L : mode Kurama"
	l.position = Vector2(16, 12)
	l.add_theme_color_override("font_color", Color(1, 1, 1))
	l.add_theme_color_override("font_outline_color", Color(0.1, 0.08, 0.15))
	l.add_theme_constant_override("outline_size", 6)
	hud.add_child(l)


func spawn_fx(fx_name: String, at: Vector2, size: float, flip := false):
	var e := Effect.new()
	e.setup(lib, fx_name, size, flip)
	e.position = at
	e.z_index = 5
	add_child(e)
	return e


# Sasuke's great fireball: a PixelLab projectile that lights everything on its way.
func fireball(caster) -> void:
	var e = spawn_fx("great_fireball", caster.position + Vector2(caster.facing * 90.0, -0.55 * caster.height_px), 1.25,
		caster.facing < 0)
	e.velocity = Vector2(caster.facing * 620.0, 0)
	e.life = 2.4
	e.add_light(Color(1.0, 0.55, 0.2), 1.3, 240.0)
	e.set_meta("owner", caster)


func _physics_process(_dt: float) -> void:
	frame += 1
	if hitstop > 0:
		hitstop -= 1
	if demo:
		demo_inputs()
	else:
		p1.input = {"left": Input.is_physical_key_pressed(KEY_A) or Input.is_physical_key_pressed(KEY_LEFT),
			"right": Input.is_physical_key_pressed(KEY_D) or Input.is_physical_key_pressed(KEY_RIGHT),
			"jump": Input.is_physical_key_pressed(KEY_W) or Input.is_physical_key_pressed(KEY_SPACE),
			"attack": just("j"), "special": just("k"), "special2": just("l")}
		ai_inputs()
	# Fireballs against the fighters.
	for e in get_children():
		if e.has_meta("owner") and is_instance_valid(e):
			var target = p2 if e.get_meta("owner") == p1 else p1
			var c: Vector2 = target.position + Vector2(0, -0.5 * target.height_px)
			if e.position.distance_to(c) < 80.0 and target.state != "down":
				spawn_fx("explosion_fire", e.position, 1.3)
				target.get_hit(1 if e.velocity.x > 0 else -1, 10.0, true)
				hitstop = 8
				shake = 16.0
				e.queue_free()
	# The camera frames both fighters and shakes on big hits.
	var mid: Vector2 = (p1.position + p2.position) * 0.5
	cam.position = cam.position.lerp(Vector2(mid.x * 0.7, GROUND_Y - 170), 0.08)
	var span: float = abs(p1.position.x - p2.position.x)
	var z: float = clamp(1500.0 / (span + 700.0), 1.2, 1.6)
	cam.zoom = cam.zoom.lerp(Vector2(z, z), 0.05)
	cam.offset = Vector2(randf_range(-shake, shake), randf_range(-shake, shake))
	shake = move_toward(shake, 0.0, 1.2)
	if record_dir != "" and frame >= record_from and frame <= record_to and (frame - record_from) % record_every == 0:
		var img := get_viewport().get_texture().get_image()
		img.save_png(record_dir.path_join("f%05d.png" % frame))
	if record_dir != "" and frame > record_to:
		get_tree().quit()


var _was := {}


func just(key: String) -> bool:
	var code: int = {"j": KEY_J, "k": KEY_K, "l": KEY_L}[key]
	var down := Input.is_physical_key_pressed(code)
	var j: bool = down and not _was.get(key, false)
	_was[key] = down
	return j


# Sasuke's small AI: closes in and combos, throws a fireball from afar, a Chidori from mid range.
func ai_inputs() -> void:
	ai_timer -= 1.0 / 60.0
	var dx: float = p1.position.x - p2.position.x
	var inp := {}
	if ai_timer <= 0.0:
		ai_timer = randf_range(0.5, 1.2)
		var d: float = abs(dx)
		var roll := randf()
		ai_hold = {}
		if d > 420.0 and roll < 0.4:
			inp["special2"] = true
		elif d > 200.0 and d < 420.0 and roll < 0.3:
			inp["special"] = true
		elif d < 160.0:
			inp["attack"] = true
		else:
			ai_hold = {"left": dx < 0, "right": dx > 0}
	if abs(dx) < 140.0:
		ai_hold = {}
	inp.merge(ai_hold)
	if p2.state == "attack" and randf() < 0.08:
		inp["attack"] = true
	p2.input = inp


# The scripted fight of --demo: each step waits, then gives inputs until its condition holds.
func demo_inputs() -> void:
	p1.input = {}
	p2.input = {}
	var steps := [
		[50, "p1", "approach"], [0, "p1", "attack"], [16, "p1", "attack"], [16, "p1", "attack"],
		[110, "p2", "special2"], [100, "p1", "special2"], [60, "p1", "approach"], [0, "p1", "special"],
		[120, "p2", "special"], [150, "p1", "special2"], [40, "p2", "approach"], [0, "p2", "attack"],
		[16, "p2", "attack"], [80, "p1", "end"]]
	if demo_step >= steps.size():
		return
	var s: Array = steps[demo_step]
	if demo_wait < s[0]:
		demo_wait += 1
		return
	var who = p1 if s[1] == "p1" else p2
	var other = p2 if who == p1 else p1
	if s[2] == "approach":
		if abs(other.position.x - who.position.x) > 140.0 or who.busy():
			if not who.busy():
				who.input = {"right": other.position.x > who.position.x, "left": other.position.x < who.position.x}
			return
	elif s[2] != "end":
		if who.busy() and s[2] != "attack":
			return
		who.input = {s[2]: true}
	demo_step += 1
	demo_wait = 0
