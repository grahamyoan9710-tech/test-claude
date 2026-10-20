extends Node2D
# A fighter of the prototype: the game's PixelLab clips on an AnimatedSprite2D, a simple state machine (run, jump,
# a 3-hit combo, two specials, hurt), a 2D light on its techniques. The real game logic (the C++ core) is not
# plugged in here: this only tests how the clips and the effects look in Godot.

const Effect := preload("res://scripts/effect.gd")
const GRAVITY := 2600.0
const RUN := 430.0
const JUMP := 1050.0

var lib
var main
var who := ""
var opponent = null
var sprite := AnimatedSprite2D.new()
var light := PointLight2D.new()
var aura = null
var facing := 1
var vel := Vector2.ZERO
var on_ground := true
var state := "idle"  # idle, run, air, attack, special, special2, hurt, down, getup
var anim := ""
var prefix := ""  # "w1_": Naruto in Kurama chakra mode
var height_px := 175.0
var input := {}
var queued := false
var hits_done := {}
var damage := 0.0
var light_target := 0.0


func setup(l, m, w: String, x: float) -> void:
	lib = l
	main = m
	who = w
	position = Vector2(x, main.GROUND_Y)
	var anims := ["idle", "run", "jump", "fall", "land", "jab", "jab2", "jab3", "nspecial", "sspecial", "hurt",
		"knockdown", "getup", "w1_idle", "w1_run", "w1_jump", "w1_fall", "w1_jab", "w1_jab2", "w1_jab3",
		"w1_nspecial", "w1_hurt"]
	sprite.sprite_frames = lib.fighter_frames(who, anims)
	sprite.centered = false
	add_child(sprite)
	sprite.animation_finished.connect(_on_anim_finished)
	sprite.frame_changed.connect(_on_frame)
	light.texture = Effect.load_glow()
	light.energy = 0.0
	light.texture_scale = 1.5
	add_child(light)
	play("idle")


func clip_name(a: String) -> String:
	return prefix + a if sprite.sprite_frames.has_animation(prefix + a) else a


func meta() -> Dictionary:
	return lib.sheets[who][sprite.animation]


func play(a: String) -> bool:
	var n := clip_name(a)
	if not sprite.sprite_frames.has_animation(n):
		return false
	anim = a
	var m: Dictionary = lib.sheets[who][n]
	var s: float = height_px / m.pxh
	sprite.offset = Vector2(-m.ax, -m.ay)
	sprite.scale = Vector2(s * facing, s)
	sprite.play(n)
	sprite.speed_scale = 1.5 if a.begins_with("jab") else 1.0
	hits_done.clear()
	return true


func busy() -> bool:
	return state in ["attack", "special", "special2", "hurt", "down", "getup"]


func _physics_process(dt: float) -> void:
	if main.hitstop > 0:
		sprite.speed_scale = 0.0
		return
	if sprite.speed_scale == 0.0:
		sprite.speed_scale = 1.5 if anim.begins_with("jab") else 1.0
	var dir := int(input.get("right", false)) - int(input.get("left", false))
	if state == "attack" and input.get("attack", false):
		queued = true
	if not busy():
		if input.get("special2", false):
			start_special2()
		elif input.get("special", false):
			start_special()
		elif input.get("attack", false):
			state = "attack"
			play("jab")
			queued = false
		elif input.get("jump", false) and on_ground:
			vel.y = -JUMP
			on_ground = false
			state = "air"
			play("jump")
		if not busy():
			if dir != 0:
				facing = dir
				vel.x = dir * RUN
			else:
				vel.x = move_toward(vel.x, 0.0, 4000.0 * dt)
				if opponent:
					facing = 1 if opponent.position.x > position.x else -1
			if on_ground:
				var want := "run" if dir != 0 else "idle"
				if anim != want:
					play(want)
					state = want
			elif vel.y > 0.0 and anim != "fall":
				play("fall")
	elif state != "special":
		vel.x = move_toward(vel.x, 0.0, (1600.0 if on_ground else 300.0) * dt)
	if not on_ground:
		vel.y += GRAVITY * dt
	position += vel * dt
	position.x = clamp(position.x, -640.0, 640.0)
	if position.y >= main.GROUND_Y:
		position.y = main.GROUND_Y
		if not on_ground:
			on_ground = true
			vel.y = 0.0
			if state == "air":
				state = "idle"
				play("idle")
			elif state == "hurt" and anim == "knockdown":
				state = "down"
				if not sprite.is_playing():  # the fall already ended in the air: up at once
					state = "getup"
					play("getup")
	sprite.scale.x = abs(sprite.scale.x) * facing
	light.energy = move_toward(light.energy, light_target, 6.0 * dt)
	light.position.x = abs(light.position.x) * facing


# Naruto: the Rasengan; Sasuke: the Chidori.
func start_special() -> void:
	state = "special"
	if who == "naruto":
		play("nspecial")
		light.color = Color(0.45, 0.75, 1.0)
		light.position = Vector2(-10, -0.55 * height_px)
		light_target = 1.3
	else:
		play("sspecial")
		light.color = Color(0.6, 0.85, 1.0)
		light.position = Vector2(55, -0.5 * height_px)
		light_target = 1.4


# Naruto: Kurama chakra mode on/off; Sasuke: the great fireball.
func start_special2() -> void:
	if who == "naruto":
		prefix = "" if prefix == "w1_" else "w1_"
		main.spawn_fx("smoke_puff", position + Vector2(0, -0.5 * height_px), 1.6)
		if aura:
			aura.queue_free()
			aura = null
		if prefix != "":
			aura = Effect.new()
			aura.setup(lib, "kurama_aura", height_px / 170.0 * 1.15)
			aura.z_index = -1
			add_child(aura)
			aura.add_light(Color(1.0, 0.62, 0.2), 0.7, 190.0)
		play("idle")
		main.shake = 6.0
	else:
		state = "special2"
		play("nspecial")


func _on_frame() -> void:
	var m := meta()
	var f := sprite.frame
	if state == "special" and who == "naruto" and f >= 6:
		light.position = Vector2(70, -0.5 * height_px)
	if state == "special" and who == "sasuke":
		light.energy = light_target * randf_range(0.6, 1.2)  # the Chidori crackles
		if f >= 10 and f <= 13:
			vel.x = 1150.0 * facing
		elif f == 14:
			vel.x = 0.0
	if state == "special2" and who == "sasuke" and f == 4 and not hits_done.has(f):
		hits_done[f] = true
		main.fireball(self)
	if (state == "attack" or state == "special") and f in m.impact and not hits_done.has(f):
		hits_done[f] = true
		try_hit()


func try_hit() -> void:
	if opponent == null or opponent.state == "down":
		return
	var reach := 150.0
	var strong := anim == "jab3"
	var fx := "hit_chakra" if who == "naruto" else "hit_slash"
	if state == "special":
		reach = 170.0
		strong = true
		fx = "hit_chakra" if who == "naruto" else "hit_lightning"
	var dx: float = (opponent.position.x - position.x) * facing
	if dx < -20.0 or dx > reach or abs(opponent.position.y - position.y) > 140.0:
		return
	var at: Vector2 = opponent.position + Vector2(-facing * 25.0, -0.55 * height_px)
	main.spawn_fx(fx, at, 1.5 if strong else 1.0, facing < 0)
	opponent.get_hit(facing, 9.0 if strong else 4.0, strong)
	main.hitstop = 9 if strong else 4
	main.shake = 14.0 if strong else 4.0


func get_hit(from_dir: int, dmg: float, strong: bool) -> void:
	damage += dmg
	facing = -from_dir
	light_target = 0.0
	state = "hurt"
	if strong:
		vel = Vector2(from_dir * (520.0 + damage * 4.0), -620.0 - damage * 3.0)
		on_ground = false
		play("knockdown")
	else:
		vel.x = from_dir * 240.0
		play("hurt")


func _on_anim_finished() -> void:
	match state:
		"attack":
			if queued and anim != "jab3":
				queued = false
				if play("jab2" if anim == "jab" else "jab3"):  # Sasuke has no third hit
					return
			state = "idle"
		"special", "special2":
			light_target = 0.0
			state = "idle"
		"hurt":
			if anim == "knockdown":
				if on_ground:
					state = "getup"
					play("getup")
				return
			state = "idle"
		"down":
			state = "getup"
			play("getup")
			return
		"getup":
			state = "idle"
		_:
			return
	play("idle")
