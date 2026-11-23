extends Node3D
## Opening exposes a real drawer; three searchable compartments require attention.
const Props = preload("res://scripts/imported_props.gd")
const Builder = preload("res://scripts/world_builder.gd")
var game: Node3D
var prop: Node3D
var drawer: Node3D
var drawer_origin: Vector3
var drawer_scale: float = 1
var preview: Node3D
var opened: bool = false
var opening: float = 0
var selected: int = 0
var searched: Array[bool] = [false,false,false]
var progress: float = 0
var ready_for_loot: bool = false
var reward: int = 2
var working: bool = false
var key: String
var opening_tween: Tween
var label: Label3D
var drawer_body: StaticBody3D
var tray: Node3D
var drawer_extension: float = 0

func _ready() -> void:
	prop = get_parent()
	game = get_tree().get_first_node_in_group("game")
	key = str(prop.get("object_id")) if prop.get("object_id")!=null else str(prop.get("kind"))+str(prop.position)
	reward = 1+posmod(key.hash(),2)
	add_to_group("search_storage")
	var tall: bool = str(prop.get("object_id")) in ["badge","ammo_archive","ammo_residence"]
	var size: Vector3 = Vector3(.90,1.05,.60) if tall else Vector3(.78,.85,.60)
	Props.body(self,size,Vector3(0,size.y/2,0))
	var furniture: Node3D = Props.visual(self,"vintage_wooden_drawer_01" if tall else "painted_wooden_nightstand",size)
	for node: MeshInstance3D in furniture.find_children("*","MeshInstance3D",true,false):
		if "drawer" in str(node.name): drawer = node
	drawer_origin = drawer.position
	drawer_scale = furniture.scale.z
	drawer_body = Props.body(self,Vector3(size.x*.83,.18,.065),Vector3(0,.60,.27))
	tray = Node3D.new()
	tray.position = Vector3(0,.60,.10)
	add_child(tray)
	var wood: Material = preload("res://scripts/environment_materials.gd").surface("concrete","665b43",3,.95)
	Builder.box(tray,Vector3(.62,.018,.28),Vector3.ZERO,wood)
	for x: float in [-.10,.10]: Builder.box(tray,Vector3(.015,.09,.27),Vector3(x,.045,0),wood)
	for i: int in range(3):
		var paper := Builder.box(tray,Vector3(.11,.008,.12),Vector3((i-1)*.20,.025,0),Builder.material("897c63"))
		paper.rotation.y = .17*(i-1)
	tray.hide()
	label = Label3D.new()
	label.position = Vector3(0,1.04,.35)
	label.font_size = 18
	label.pixel_size = .0035
	label.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	add_child(label)
	label.hide()
	preview = Node3D.new()
	preview.position = Vector3((reward-1)*.20,.65,.47)
	add_child(preview)
	preload("res://scripts/loot_visual.gd").build(preview,str(prop.get("object_id")),str(prop.get("kind")))
	preview.hide()

func interaction_point() -> Vector3:
	return global_position+global_basis*Vector3(0,.1,1.25)

func interact() -> bool:
	if ready_for_loot and opened: return true
	if opening>0: return false
	if not opened:
		# Sweep the entire extension first: an animated StaticBody must never
		# appear inside the player's capsule and pin it against a bed.
		if not _drawer_clear(.37):
			game.hud.announce("Отойди на шаг от ящика, чтобы открыть его.",3)
			return false
		opened = true
		tray.show()
		preview.visible = ready_for_loot
		if game.hero.camera_rig.motion_enabled: game.hero.camera_rig.rotation.x = minf(game.hero.camera_rig.rotation.x,-.30)
		opening = .45
		opening_tween = create_tween()
		opening_tween.tween_method(_set_extension,0.0,.37,.45).set_trans(Tween.TRANS_CUBIC).set_ease(Tween.EASE_OUT)
		game.sound_bank.play_spatial("scrape",global_position,.9)
		game.emit_noise(global_position,6)
	return false

func _drawer_clear(extension: float) -> bool:
	var query := PhysicsShapeQueryParameters3D.new()
	query.shape = drawer_body.get_child(0).shape
	query.transform = drawer_body.global_transform
	query.collision_mask = 1|2|4
	query.exclude = prop.interaction_rids()
	query.margin = .035
	query.motion = global_basis*Vector3(0,0,maxf(0,extension-drawer_extension))
	var space: PhysicsDirectSpaceState3D = get_world_3d().direct_space_state
	if not space.intersect_shape(query,1).is_empty(): return false
	var safe: PackedFloat32Array = space.cast_motion(query)
	return safe.size()==2 and safe[0]>.99

func _set_extension(value: float) -> void:
	if value>drawer_extension and not _drawer_clear(value): return
	drawer_extension = value
	drawer.position.z = drawer_origin.z+value/drawer_scale
	drawer_body.position.z = .27+value
	tray.position.z = .10+value
	preview.position.z = .10+value

func close_drawer() -> void:
	_stop_work()
	if opening_tween!=null and opening_tween.is_valid(): opening_tween.kill()
	opened = false
	opening = 0
	_set_extension(0)
	tray.hide()
	preview.hide()
	label.hide()

func _physics_process(delta: float) -> void:
	opening = maxf(0,opening-delta)
	if game==null or game.hero==null or game.has_lost or game.has_won: return
	if game.hero.global_position.distance_to(prop.global_position)>2.2:
		label.hide()
		_stop_work()
		if opened: close_drawer()
		return
	var focused: bool = game._interaction_target()==prop
	# The unified contextual HUD renders this status beside the drawer.
	label.visible = false
	if not focused or not opened or opening>0 or ready_for_loot or game.hero.invulnerable_remaining>0:
		_stop_work()
		return
	for i: int in range(3):
		if Input.is_physical_key_pressed(KEY_1+i) and selected!=i:
			selected = i
			progress = 0
	label.text = "ОТДЕЛЕНИЕ %d / 3  ·  %s  ·  C — закрыть" % [selected+1,"ПУСТО" if searched[selected] else str(int(progress/1.2*100))+"%"]
	if not Input.is_action_pressed("interact_hold") or searched[selected] or game.combat.reload_remaining>0:
		_stop_work()
		return
	working = true
	game.hero.performing_action = true
	game.hero.action_focus = prop.global_position+prop.global_basis*Vector3(0,.69,.55)
	progress += delta
	if fmod(progress,.5)<delta: game.emit_noise(global_position,4)
	if progress>=1.2:
		searched[selected] = true
		progress = 0
		_stop_work()
		if selected==reward:
			ready_for_loot = true
			preview.show()
			game.sound_bank.play_cue("core",.75)
			game.hud.announce("В глубине ящика что-то есть. F — забрать.",3)
		else:
			game.sound_bank.play_spatial("scrape",global_position,1.15)
			game.hud.announce("Только пыль и старые бумаги. Проверь другое отделение: 1 / 2 / 3.",3)
			selected = (selected+1)%3

func _stop_work() -> void:
	if working:
		game.hero.performing_action = false
		working = false

func prompt() -> String:
	if prop.get("used")==true or prop.get("is_recovered")==true: return "F / C — закрыть пустой ящик"
	if ready_for_loot and opened: return "F — забрать найденное  ·  C — закрыть"
	if not opened: return "F — открыть старую тумбу"
	return "1 / 2 / 3 — отделение  ·  удерживай F — осмотреть  ·  C — закрыть"

func finish() -> void:
	_stop_work()
	preview.hide()
	label.hide()
	close_drawer()

func reset_storage() -> void:
	_stop_work()
	if opening_tween!=null and opening_tween.is_valid(): opening_tween.kill()
	opened = false
	opening = 0
	selected = 0
	searched.assign([false,false,false])
	progress = 0
	ready_for_loot = false
	drawer.position = drawer_origin
	drawer_extension = 0
	drawer_body.position.z = .27
	tray.position.z = .10
	tray.hide()
	preview.hide()
	label.hide()

func snapshot() -> Dictionary:
	return {"opened":opened,"searched":searched.duplicate()}

func restore(data: Dictionary) -> void:
	opened = data.get("opened",false)
	searched.assign(data.get("searched",[false,false,false]))
	ready_for_loot = searched[reward]
	_set_extension(.37 if opened else 0)
	tray.visible = opened
	preview.visible = opened and ready_for_loot and not (prop.get("used")==true or prop.get("is_recovered")==true)
	for i: int in range(3):
		if not searched[i]:
			selected = i
			break

static func valid(data: Variant) -> bool:
	if not data is Dictionary or not data.get("opened") is bool or not data.get("searched") is Array or data["searched"].size()!=3: return false
	for value: Variant in data["searched"]:
		if not value is bool: return false
	return true
