extends Node
## Six manual slots and three rolling autosaves. All disk writes are recoverable.
const Saves = preload("res://scripts/checkpoint_store.gd")
const KEYS: Array[String] = ["manual_1","manual_2","manual_3","manual_4","manual_5","manual_6","auto_1","auto_2","auto_3"]
var game: Node3D
var slots: Dictionary = {}
var screen: CanvasLayer
var indicator: CanvasLayer
var file_root: String = "user://echo_saves"
var allow_test_disk: bool = false
var next_auto: int = 1
var visited: Array[String] = []
var auto_cooldown: float = 0
var points: Array[Node3D] = []
func configure(owner_game: Node3D) -> void:
	game = owner_game
	process_mode = Node.PROCESS_MODE_ALWAYS
	indicator = preload("res://scripts/save_indicator.gd").new()
	game.add_child(indicator)
	screen = preload("res://scripts/save_screen.gd").new()
	screen.manager = self
	game.add_child(screen)
	for at: Vector3 in [Vector3(58,0,-26.5),Vector3(42,0,-55),Vector3(43,0,-120),Vector3(58,0,-178)]:
		var point := preload("res://scripts/save_station.gd").new()
		point.manager = self
		point.position = at
		game.get_node("World").add_child(point)
		points.append(point)
	if not game.automated: read_slots()
func eligible() -> bool:
	return (not game.in_title_menu or game.campaign_started) and not game.has_won and not game.has_lost and not game.mission.exit_started and game.health>0 and game.hero.hiding_bed==null and not game.hero.free_flight and not game.hero.performing_action and game.hero.is_on_floor()
func capture(key: String = "manual_1") -> bool:
	if key not in KEYS or not eligible(): return false
	var data: Dictionary = Saves.snapshot(game)
	data.exact_save = true
	data.health = game.health
	data.charges = game.flash_charges
	data.resume = {"at":[game.hero.position.x,game.hero.position.y,game.hero.position.z],"yaw":game.hero.camera_rig.rotation.y,"pitch":game.hero.camera_rig.rotation.x}
	data.actors = []
	for actor: CharacterBody3D in game.hunters:
		data.actors.append({"at":[actor.position.x,actor.position.y,actor.position.z],"known":[actor.last_known.x,actor.last_known.y,actor.last_known.z],"state":int(actor.state)})
	return commit(key,data)
func record_checkpoint(data: Dictionary) -> bool:
	var key: String = "auto_%d"%next_auto
	var ok: bool = commit(key,data)
	if ok: next_auto = next_auto%3+1
	return ok
func commit(key: String, data: Dictionary) -> bool:
	if key not in KEYS or not Saves.valid(data): return false
	var copy: Dictionary = data.duplicate(true)
	copy.saved_at = Time.get_unix_time_from_system()
	copy.save_kind = "auto" if key.begins_with("auto_") else "manual"
	copy.chapter_name = game.exploration.current
	var ok: bool = true
	if not game.automated or allow_test_disk: ok = _atomic_write(key,copy)
	indicator.show_result(ok,key.begins_with("auto_"))
	if not ok: return false
	slots[key] = copy
	game.checkpoint = copy.duplicate(true)
	game.hud.checkpoint_available = true
	return true
func _atomic_write(key: String,data: Dictionary) -> bool:
	if key not in KEYS: return false
	var dir: String = ProjectSettings.globalize_path(file_root)
	var parent: String = dir
	while not DirAccess.dir_exists_absolute(parent):
		if FileAccess.file_exists(parent): return false
		var next: String = parent.get_base_dir()
		if next==parent or next.is_empty():return false
		parent=next
	if DirAccess.make_dir_recursive_absolute(dir)!=OK: return false
	var target: String = dir.path_join(key+".json")
	var temp: String = target+".tmp"
	var backup: String = target+".bak"
	var file := FileAccess.open(temp,FileAccess.WRITE)
	if file==null: return false
	file.store_string(JSON.stringify(data))
	file.flush()
	var error: Error = file.get_error()
	file.close()
	var verify:=JSON.new()
	if error!=OK or verify.parse(FileAccess.get_file_as_string(temp))!=OK or not Saves.valid(verify.data): return false
	if FileAccess.file_exists(target):
		var previous:=JSON.new()
		var previous_valid:bool=previous.parse(FileAccess.get_file_as_string(target))==OK and Saves.valid(previous.data)
		if previous_valid:
			if FileAccess.file_exists(backup) and DirAccess.remove_absolute(backup)!=OK:return false
			if DirAccess.rename_absolute(target,backup)!=OK: return false
		elif DirAccess.remove_absolute(target)!=OK:return false
	if DirAccess.rename_absolute(temp,target)!=OK:
		if FileAccess.file_exists(backup): DirAccess.rename_absolute(backup,target)
		return false
	return true
func read_slots() -> void:
	slots.clear()
	var latest: float = -1
	for key: String in KEYS:
		for suffix: String in [".json",".json.bak"]:
			var path: String = file_root.path_join(key+suffix)
			var file := FileAccess.open(path,FileAccess.READ)
			if file==null: continue
			if file.get_length()>2000000: file.close(); continue
			var json:=JSON.new()
			var error:Error=json.parse(file.get_as_text())
			file.close()
			if error!=OK:continue
			var data:Variant=json.data
			if not Saves.valid(data): continue
			slots[key] = data
			var stamp: float = float(data.get("saved_at",0))
			if stamp>=latest:
				latest = stamp
				game.checkpoint = data.duplicate(true)
				game.hud.checkpoint_available = true
				if key.begins_with("auto_"): next_auto = int(key.right(1))%3+1
			break
func load_slot(key: String) -> bool:
	if key not in KEYS or not slots.has(key): return false
	var data: Dictionary = slots[key].duplicate(true)
	if not Saves.apply(game,data): return false
	game.checkpoint = data
	screen.dismiss(false)
	return true
func latest_key() -> String:
	var newest:String=""
	var stamp:float=-INF
	for key:String in KEYS:
		var data:Dictionary=slots.get(key,{})
		if Saves.valid(data) and float(data.get("saved_at",0))>stamp:
			newest=key;stamp=float(data.get("saved_at",0))
	return newest
func can_load_latest() -> bool:
	return not latest_key().is_empty() or Saves.valid(game.checkpoint)
func load_latest() -> bool:
	var key:String=latest_key()
	if not key.is_empty():return load_slot(key)
	return game.continue_checkpoint()
func open_browser(mode: String = "load") -> void: screen.open_view(mode)
func update(delta: float) -> void:
	if game.automated or not eligible() or game.is_paused: return
	auto_cooldown = maxf(0,auto_cooldown-delta)
	var id: String = ""
	if game.isolation.complete and Rect2(35,-125,9,14).has_point(Vector2(game.hero.position.x,game.hero.position.z)): id = "annex_post"
	elif game.diagnostics.complete and Rect2(46,-59,11,7).has_point(Vector2(game.hero.position.x,game.hero.position.z)): id = "isolation_post"
	elif game.mission.has_badge and Rect2(32,-23,10,5).has_point(Vector2(game.hero.position.x,game.hero.position.z)): id = "diagnostic_gallery"
	if id.is_empty() or id in visited or auto_cooldown>0: return
	for actor: CharacterBody3D in game.hunters:
		if actor.active and not actor.dead and actor.position.distance_to(game.hero.position)<8: return
	if capture("auto_%d"%next_auto):
		next_auto = next_auto%3+1
		visited.append(id)
		auto_cooldown = 30
