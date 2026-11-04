extends AnimatedSprite2D
# A PixelLab effect (fx_*.png): played once (or looping), anchored on its origin, optionally lit by a 2D light and
# following a node.

var follow: Node2D = null
var follow_offset := Vector2.ZERO
var velocity := Vector2.ZERO
var life := -1.0
var light: PointLight2D = null


func setup(lib, fx_name: String, size: float, flip := false) -> void:
	sprite_frames = lib.fx_frames(fx_name)
	var m: Dictionary = lib.fxs.get(fx_name, {"ax": 0.0, "ay": 0.0, "loop": false})
	centered = false
	offset = Vector2(-m.ax, -m.ay)
	scale = Vector2(-size if flip else size, size)
	if sprite_frames.has_animation("fx"):
		play("fx")
	if not m.loop:
		animation_finished.connect(queue_free)


func add_light(color: Color, energy: float, radius: float) -> void:
	light = PointLight2D.new()
	light.texture = load_glow()
	light.color = color
	light.energy = energy
	light.texture_scale = radius / 128.0
	add_child(light)
	# The light sits at the effect's origin whatever the sprite's scale.
	light.scale = Vector2(1.0 / abs(scale.x), 1.0 / scale.y)


static var _glow: Texture2D = null


static func load_glow() -> Texture2D:
	if _glow == null:
		var g := GradientTexture2D.new()
		g.width = 256
		g.height = 256
		g.fill = GradientTexture2D.FILL_RADIAL
		g.fill_from = Vector2(0.5, 0.5)
		g.fill_to = Vector2(1.0, 0.5)
		var gr := Gradient.new()
		gr.set_color(0, Color(1, 1, 1, 1))
		gr.set_color(1, Color(1, 1, 1, 0))
		g.gradient = gr
		_glow = g
	return _glow


func _physics_process(dt: float) -> void:
	if follow:
		if not is_instance_valid(follow):
			queue_free()
			return
		global_position = follow.global_position + follow_offset
	position += velocity * dt
	if life > 0.0:
		life -= dt
		if life <= 0.0:
			queue_free()
