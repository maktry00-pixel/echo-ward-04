extends Node3D
## Optional equipment, dismantlable barricades, recordings and a preplaced ambush.
const B = preload("res://scripts/world_builder.gd")
const P = preload("res://scripts/imported_props.gd")
const Station = preload("res://scripts/upgrade_station.gd")
const Barricade = preload("res://scripts/door_barricade.gd")
const LOCATIONS := {"shotgun":Vector3(19,0,3),"shells_morgue":Vector3(28,0,4),"shells_lower":Vector3(53,-5,72),"shells_preparation":Vector3(50,0,-48.1),"boards_reception":Vector3(13,0,10),"boards_supply":Vector3(-28,0,8.5),"record_shift":Vector3(-12,0,10.5),"record_evidence":Vector3(5,0,-7.5),"record_ceiling":Vector3(20,0,-20),"record_farewell":Vector3(-18,0,-4)}
const RECORDS := {
	"shift":"Ночная смена. Четыре кровати пусты, но выписку никто не подписал. Доски плотника остались у приёмной. Закрой дверь, прежде чем её укреплять.",
	"evidence":"Врач стёр Ирину из списка. Остались её письмо и журнал переводов. Сопоставь их — и получишь доступ в кабинет. Пожалуйста, вынеси документы.",
	"ceiling":"Не иди на скрежет. Оно ждёт над галереей. Если сыплется пыль — тебя заметили. Отойди, прежде чем оно отпустит потолок.",
	"farewell":"Если главная заслонка упадёт, иди северной эвакуацией. Резервный ввод — в служебном крыле. Скажи моей семье: я осталась помогать."}
var game: Node3D
var stations: Dictionary = {}
var used: Array[String] = []
var records: Array[String] = []
var replay_index: int = 0
var ceiling: CharacterBody3D
var work_door: Node3D
var work_time: float = 0
var work_remove: bool = false
var changed: Node3D
var change_seen: bool = false
var status: Label

func configure(owner_game: Node3D) -> void:
	game = owner_game
	for id: String in LOCATIONS: _build_station(id,LOCATIONS[id])
	ceiling = preload("res://scripts/ceiling_hunter.gd").new()
	ceiling.position = Vector3(29,2.4,-20)
	game.get_node("World").add_child(ceiling)
	ceiling.patrol_points.assign([Vector3(29,.1,-20),Vector3(37,.1,-20),Vector3(23,.1,-28)])
	game.hunters.append(ceiling)
	_build_story()
	status = Label.new()
	status.set_anchors_and_offsets_preset(Control.PRESET_CENTER_BOTTOM)
	status.position = Vector2(-400,-290)
	status.size = Vector2(800,38)
	status.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	status.add_theme_font_size_override("font_size",16)
	status.add_theme_color_override("font_shadow_color",Color.BLACK)
	status.add_theme_constant_override("shadow_offset_x",2)
	game.hud.staging.add_child(status)

func _build_station(id: String,at: Vector3) -> void:
	var station := Station.new()
	station.upgrades = self
	station.station_id = id
	station.position = at
	add_child(station)
	stations[id] = station
	P.body(station,Vector3(.95,.75,.6),Vector3(0,.375,0))
	P.visual(station,"painted_wooden_nightstand",Vector3(.95,.75,.6))
	var item := Node3D.new()
	station.add_child(item)
	item.position.y = .77
	station.marker = item
	if id=="shotgun":
		var gun := preload("res://scripts/shotgun_visual.gd").new()
		item.add_child(gun)
		gun.rotation = Vector3(0,PI/2,PI/2)
		gun.position.x = -.12
	elif id.begins_with("record_"):
		P.detail(item,"portable_cassette_player",.3)
		B.box(item,Vector3(.19,.008,.11),Vector3(.23,.01,.06),B.material("c0b8a1"))
	else: preload("res://scripts/loot_visual.gd").build(item,id,"shells" if id.begins_with("shells_") else "boards")

func _build_story() -> void:
	# Belongings remain on furniture, never become collision traps in walkways.
	for bed: Node3D in get_tree().get_nodes_in_group("hide_beds"):
		if bed.global_position.z>8 or absf(bed.global_position.x)>14: continue
		var note := Node3D.new()
		bed.add_child(note)
		note.position = Vector3(.15,.66,-.38)
		B.box(note,Vector3(.22,.008,.29),Vector3.ZERO,B.material("b5ae98"))
		for i: int in range(5): B.box(note,Vector3(.13,.002,.005),Vector3(0,.006,-.09+i*.026),B.material("737e73"))
	changed = Node3D.new()
	add_child(changed)
	changed.hide()
	var mark = preload("res://scripts/surface_mark.gd")
	for i: int in range(9): mark.create(changed,Vector3(-1.7+i*.12,.007,8.6+i*.13),Vector3.UP,.035,Color("442b2a"),260+i)
	P.detail(changed,"portable_cassette_player",.26,Vector3(-1.3,.03,9.5),Vector3(PI/2,.2,0))
	B.box(changed,Vector3(.34,.006,.23),Vector3(-1.6,.023,9.3),B.material("b8aa8b"))

func prompt(id: String) -> String:
	if id.begins_with("record_"): return "F — прослушать запись · русские субтитры · L повтор найденных"
	if id in used: return "Припасы забраны"
	if id=="shotgun": return "F — взять помповый дробовик · 1 ячейка · Z смена оружия"
	return "F — взять патроны 12 калибра" if id.begins_with("shells_") else "F — взять доску · K у закрытой двери — баррикада"

func use_station(id: String) -> void:
	if id.begins_with("record_"):
		var record: String = id.trim_prefix("record_")
		if record not in records: records.append(record)
		play_record(record)
		return
	if id in used: return
	if id=="shotgun":
		if not game.combat.equipped: game.hud.announce("Сначала возьми пистолет в приёмной.",4); return
		if not game.inventory.can_take("shotgun"): return
		game.combat.shotgun_owned = true
		game.combat.shotgun_loaded = 2
		game.hud.announce("Дробовик: Z — смена оружия, R — по одному патрону. Мощный вблизи; патроны редкие.",8)
	else:
		var kind: String = "shells" if id.begins_with("shells_") else "boards"
		var count: int = int(game.difficulty_value("shells")) if kind=="shells" else 1
		if not game.inventory.take_supply(kind,count): return
		game.hud.announce("+%d · %s" % [count,"12 калибр" if kind=="shells" else "доска. Закрой дверь F, затем удерживай K"],5)
	used.append(id)
	stations[id].used = true
	stations[id].marker.hide()
	game.sound_bank.play_cue("reload")

func play_record(id: String) -> void:
	var seconds: float = game.sound_bank.play_recording(id)
	game.hud.announce("ЗАПИСЬ / "+RECORDS[id],maxf(12,seconds+1),true)

func replay_record() -> void:
	if records.is_empty(): game.hud.announce("Записей ещё нет. Ищи кассетные проигрыватели.",4); return
	play_record(records[replay_index%records.size()])
	replay_index += 1

func start_barricade() -> bool:
	if game.is_paused or game.has_won or game.has_lost or game.hero.in_refuge or game.hero.hiding_bed!=null or game.hero.performing_action: return false
	var chosen: Node3D
	var nearest: float = 2.3
	for door: Node3D in game.facility.doors:
		if door.door_id not in Barricade.ALLOWED or not game._prop_reachable(door): continue
		var distance: float = game.hero.position.distance_to(door.position)
		if distance<nearest: chosen = door; nearest = distance
	if chosen==null: game.hud.announce("Подойди к обычной двери. Аварийные и сюжетные шлюзы укреплять нельзя.",4); return false
	if chosen.is_open or chosen.busy or chosen.locked: game.hud.announce("Сначала закрой незапертую дверь клавишей F.",4); return false
	if chosen.barricade_hp<=0 and game.inventory.boards<=0: game.hud.announce("Нужна доска: у правой стены приёмной или на медскладе.",5); return false
	work_door = chosen
	work_remove = chosen.barricade_hp>0
	work_time = 0
	game.hero.performing_action = true
	game.hero.action_focus = chosen.position+Vector3(0,1.2,0)
	return true

func update(delta: float) -> void:
	status.visible = not game.is_paused and not game.has_won and not game.has_lost and not game.in_title_menu
	status.text = ""
	if game.mission.purge_complete and not change_seen:
		change_seen = true
		changed.show()
	if work_door==null: return
	if not Input.is_action_pressed("barricade") or game.hero.position.distance_to(work_door.position)>2.3 or work_door.is_open or work_door.busy or work_door.locked:
		cancel_barricade(); return
	work_time += delta
	var duration: float = 1.5 if work_remove else 2.2
	status.text = "УДЕРЖИВАЙ K · %s · %d%%" % ["разобрать баррикаду" if work_remove else "укрепить дверь",mini(100,int(work_time/duration*100))]
	if fmod(work_time,.65)<delta: game.sound_bank.play_spatial("wood_hit",work_door.position); game.emit_noise(work_door.position,10)
	if work_time<duration: return
	if work_remove:
		var refund: bool = signf(work_door.to_local(game.hero.position).z)==work_door.barricade_side
		work_door.set_barricade(0,1)
		if refund and not game.inventory.take_supply("boards",1): game.inventory.spawn_supply("boards",1,game.hero.position)
		game.hud.announce("Проход освобождён. "+("Доска возвращена или оставлена у ног." if refund else "Доска сломана с обратной стороны."),4)
	else:
		if game.inventory.boards<=0: cancel_barricade(); return
		game.inventory.boards -= 1
		work_door.set_barricade(3,int(signf(work_door.to_local(game.hero.position).z)))
		game.hud.announce("Три крепления. Враг будет ломать их по очереди. K — разобрать для выхода.",5)
	cancel_barricade()

func cancel_barricade() -> void:
	if work_door!=null: game.hero.performing_action = false
	work_door = null
	work_time = 0
	if status!=null: status.text = ""

func interact_door(door: Node3D) -> bool:
	if door.barricade_hp<=0: return false
	game.hud.announce("Дверь укреплена. Удерживай K, чтобы разобрать или выбить доски.",4)
	return true

func journal_entries() -> Array[String]:
	var result: Array[String] = []
	for id: String in records: result.append("Аудиозапись / "+id+": "+RECORDS[id])
	return result

func snapshot() -> Dictionary:
	var barriers: Dictionary = {}
	for door: Node3D in game.facility.doors:
		if door.barricade_hp>0: barriers[door.door_id] = {"hp":door.barricade_hp,"side":door.barricade_side}
	return {"used":used.duplicate(),"records":records.duplicate(),"barriers":barriers,"ceiling":ceiling.perch_state,"changed":change_seen}

static func valid(data: Variant) -> bool:
	if not data is Dictionary or not data.get("changed") is bool or data.get("ceiling") not in ["perched","warning","falling","grounded"]: return false
	if not Barricade.valid(data.get("barriers")): return false
	for key: String in ["used","records"]:
		if not data.get(key) is Array or data[key].size()>LOCATIONS.size(): return false
		var seen: Array = []
		for id: Variant in data[key]:
			if not id is String or id in seen or (not LOCATIONS.has(id) if key=="used" else not RECORDS.has(id)): return false
			seen.append(id)
	return true

func restore(data: Dictionary) -> void:
	if data.is_empty(): return
	used.assign(data.used)
	records.assign(data.records)
	for id: String in used: stations[id].used = true; stations[id].marker.hide()
	for id: String in data.barriers:
		var door: Node3D = game.facility.gate_by_id[id]
		if not door.locked:
			door.restore_state(false,false)
			door.set_barricade(int(data.barriers[id].hp),int(data.barriers[id].side))
	ceiling.restore_perch(data.ceiling)
	change_seen = data.changed
	changed.visible = change_seen
	game.facility.layout_changed.emit()

func reset_upgrade() -> void:
	cancel_barricade()
	used.clear()
	records.clear()
	replay_index = 0
	change_seen = false
	changed.hide()
	for station: Node3D in stations.values(): station.used = false; station.marker.show()
