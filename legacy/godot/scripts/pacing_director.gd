extends Node
## Authored beats choose a seeded variant. They never spawn or teleport a hunter.
var game: Node3D
var state: String = "explore"
var recovery: float = 0
var clear_time: float = 0
var beat: String = ""
var beat_time: float = 0
var fired: bool = false
var seen: Dictionary = {}
var variants: Dictionary = {}
var log: Array[Dictionary] = []
var anchors: Dictionary = {}
var motion: Node3D
var motion_origin: Vector3
var seed_number: int = 1907
var receiver: Node3D
var needle: Node3D
var signal_light: OmniLight3D

func configure(owner_game: Node3D) -> void:
	game = owner_game
	var world: Node3D = game.get_node("World")
	for item: Array in [["ward",Vector3(-5.9,1.2,3.4)],["archive",Vector3(8,1,-6.4)],["lab",Vector3(37,1.5,5.8)]]:
		var point := Node3D.new()
		point.position = item[1]
		world.add_child(point)
		anchors[item[0]] = point
	# A hanging chart reacts physically in view; it never blocks movement.
	motion = preload("res://scripts/world_builder.gd").box(anchors.ward,Vector3(.035,.48,.36),Vector3.ZERO,preload("res://scripts/world_builder.gd").material("9eab9e"))
	motion_origin = motion.rotation
	var builder = preload("res://scripts/world_builder.gd")
	var dark: Material = builder.material("353a35")
	builder.box(anchors.archive,Vector3(.30,.06,.22),Vector3.ZERO,dark)
	receiver = Node3D.new()
	anchors.archive.add_child(receiver)
	builder.box(receiver,Vector3(.31,.045,.06),Vector3(0,.065,0),dark)
	for x: float in [-.12,.12]: builder.sphere(receiver,.045,Vector3(x,.05,0),dark)
	signal_light = OmniLight3D.new()
	signal_light.position = Vector3(0,.15,0)
	signal_light.light_color = Color("cda370")
	signal_light.omni_range = .7
	signal_light.light_energy = 0
	anchors.archive.add_child(signal_light)
	builder.box(anchors.lab,Vector3(.42,.48,.06),Vector3.ZERO,dark)
	var dial: MeshInstance3D = builder.sphere(anchors.lab,.16,Vector3(0,.03,-.05),builder.material("b4b6a0"))
	dial.scale.z = .12
	needle = builder.box(anchors.lab,Vector3(.012,.13,.02),Vector3(0,.05,-.075),dark)
	reset_pacing(game.layout_seed)

func reset_pacing(value: int) -> void:
	seed_number = value
	var rng := RandomNumberGenerator.new()
	rng.seed = value+4107
	variants.clear()
	for id: String in ["ward","archive","lab"]: variants[id] = rng.randi_range(0,2)
	seen.clear()
	log.clear()
	state = "explore"
	recovery = 0
	clear_time = 0
	beat = ""
	beat_time = 0
	fired = false
	if motion!=null: motion.rotation = motion_origin
	if receiver!=null: receiver.rotation = Vector3.ZERO
	if signal_light!=null: signal_light.light_energy = 0
	if needle!=null: needle.rotation.z = 0

func ambient_allowed() -> bool: return state=="explore" and recovery<=0 and beat.is_empty()

func update(delta: float) -> void:
	if game.hero.in_refuge:
		state = "refuge"
		clear_time = 0
		return
	var danger: bool = false
	for hunter: CharacterBody3D in game.hunters:
		if hunter.active and hunter.stunned_remaining<=0 and hunter.sees_player: danger = true
	if danger:
		state = "threat"
		clear_time = 0
		if not beat.is_empty(): _finish_beat()
		return
	clear_time += delta
	if state=="threat": recovery = 10
	if not beat.is_empty():
		state = "cue"
		beat_time += delta
		if beat_time>=1.4 and not fired:
			fired = true
			_perform(beat,int(variants[beat]))
		if beat=="ward" and fired:
			motion.rotation.z = sin((beat_time-1.4)*7)*.15*exp(-(beat_time-1.4)*1.2)
		if beat=="archive" and fired:
			receiver.rotation.z = sin(beat_time*30)*.07
			signal_light.light_energy = .6 if fmod(beat_time,.4)<.2 else 0.0
		if beat=="lab" and fired: needle.rotation.z = sin(beat_time*14)*.45
		if beat_time>=4.5: _finish_beat()
		return
	recovery = maxf(0,recovery-delta)
	state = "recovery" if recovery>0 else "explore"
	if recovery>0 or clear_time<3 or game.mission.exit_started or (game.guardian.started and not game.guardian.finished): return
	var at: Vector3 = game.hero.position
	if not seen.has("ward") and game.mission.has_fuse and absf(at.x)<14 and at.z>0 and at.z<8: _begin("ward")
	elif not seen.has("archive") and game.mission.power_on and not game.mission.has_badge and at.x>3 and at.x<14 and at.z>-8 and at.z<-1: _begin("archive")
	elif not seen.has("lab") and "morgue" in game.mission.relays and at.x>30 and at.x<44 and at.z>-8 and at.z<7: _begin("lab")

func _begin(id: String) -> void:
	seen[id] = true
	beat = id
	beat_time = 0
	fired = false
	state = "cue"
	game.sound_bank.quiet_remaining = 1.4
	log.append({"beat":id,"variant":int(variants[id]),"at":game.elapsed})

func _perform(id: String, variant: int) -> void:
	var source: Vector3 = anchors[id].global_position
	var sounds: Dictionary = {"ward":["creak","metal_hit","scrape"],"archive":["phone","radio","knock"],"lab":["vent","door_hit","glass_tinkle"]}
	game.sound_bank.play_spatial(sounds[id][variant],source,.9)
	game.facility.unease_location = source
	game.facility.unease_remaining = 2.5
	game.emit_noise(source,5)
	if id=="ward":
		motion.rotation.z = .14
		game.hud.announce(["Бирка койки качнулась. За ширмой кто-то есть?","Под кроватью звякнул металл. В коридоре снова тихо.","Из пустой палаты донёсся скрежет. Не задерживайся у ширмы."][variant],4)
	elif id=="archive":
		game.hud.announce(["На старом посту звонит телефон. В комнате никого нет.","Радио в архиве: «Начальник… пропуск… на обороте…» — связь оборвалась.","Три удара с той стороны стены. Затем тишина."][variant],5)
	else:
		game.hud.announce(["Из вентиляции вырвался воздух. Дальше — операционная.","За стерильной дверью раздался тяжёлый удар.","В одном из резервуаров что-то коснулось стекла изнутри."][variant],4)

func _finish_beat() -> void:
	beat = ""
	beat_time = 0
	recovery = 12
	clear_time = 0
	motion.rotation = motion_origin
	receiver.rotation = Vector3.ZERO
	signal_light.light_energy = 0
	needle.rotation.z = 0
	state = "recovery"

func snapshot() -> Dictionary: return {"seed":seed_number,"seen":seen.keys(),"variants":variants.duplicate()}
func restore(data: Dictionary) -> void:
	reset_pacing(int(data.get("seed",game.layout_seed)))
	for id: String in data.get("seen",[]): seen[id] = true
	for id: String in data.get("variants",{}): variants[id] = int(data.variants[id])
	recovery = 8
static func valid(data: Variant) -> bool:
	if not data is Dictionary or not preload("res://scripts/field_inventory.gd")._integer(data.get("seed"),0,1000000): return false
	if not data.get("seen") is Array or not data.get("variants") is Dictionary or data.variants.size()!=3: return false
	var unique: Array = []
	for id: Variant in data.seen:
		if id not in ["ward","archive","lab"] or id in unique: return false
		unique.append(id)
	for id: String in ["ward","archive","lab"]:
		if not preload("res://scripts/field_inventory.gd")._integer(data.variants.get(id),0,2): return false
	return true
