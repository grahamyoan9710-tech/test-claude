extends RefCounted
# The game's sprite sheets, read straight from assets/sprites (sheets.txt and the PNG strips PixelLab clips were
# imported into): nothing is copied or converted.

var root := ""
var sheets := {}  # fighter -> clip -> {frames, fw, fh, ax, ay, pxh, loop, impact: [], fps}
var fxs := {}     # effect -> {frames, fw, fh, ax, ay, fps, loop, hit}
var _textures := {}


func _init(sprites_dir: String) -> void:
	root = sprites_dir
	var f := FileAccess.open(root.path_join("sheets.txt"), FileAccess.READ)
	if f == null:
		push_error("sheets.txt not found in " + root)
		return
	while not f.eof_reached():
		var p := f.get_line().split(" ", false)
		if p.size() >= 11 and p[0] == "sheet":
			var impact := []
			for v in p[10].split(","):
				if int(v) >= 0:
					impact.append(int(v))
			if not sheets.has(p[1]):
				sheets[p[1]] = {}
			sheets[p[1]][p[2]] = {"frames": int(p[3]), "fw": float(p[4]), "fh": float(p[5]), "ax": float(p[6]),
				"ay": float(p[7]), "pxh": float(p[8]), "loop": p[9] == "1", "impact": impact,
				"fps": float(p[11]) if p.size() > 11 else 15.0}
		elif p.size() >= 9 and p[0] == "fx":
			fxs[p[1]] = {"frames": int(p[2]), "fw": float(p[3]), "fh": float(p[4]), "ax": float(p[5]),
				"ay": float(p[6]), "fps": float(p[7]), "loop": p[8] == "1", "hit": float(p[9]) if p.size() > 9 else 0.0}


func texture(file: String) -> Texture2D:
	if not _textures.has(file):
		var img := Image.load_from_file(root.path_join(file))
		_textures[file] = ImageTexture.create_from_image(img) if img else null
	return _textures[file]


func _add(sf: SpriteFrames, anim: String, file: String, m: Dictionary) -> void:
	var tex := texture(file)
	if tex == null:
		return
	sf.add_animation(anim)
	sf.set_animation_loop(anim, m.loop)
	sf.set_animation_speed(anim, m.fps)
	for k in m.frames:
		var at := AtlasTexture.new()
		at.atlas = tex
		at.region = Rect2(k * m.fw, 0, m.fw, m.fh)
		sf.add_frame(anim, at)


# The clips of a fighter that exist among `anims`.
func fighter_frames(who: String, anims: Array) -> SpriteFrames:
	var sf := SpriteFrames.new()
	for a in anims:
		if sheets.get(who, {}).has(a):
			_add(sf, a, "%s_%s.png" % [who, a], sheets[who][a])
	return sf


func fx_frames(fx_name: String) -> SpriteFrames:
	var sf := SpriteFrames.new()
	if fxs.has(fx_name):
		_add(sf, "fx", "fx_%s.png" % fx_name, fxs[fx_name])
	return sf
