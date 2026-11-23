extends Node3D
## A power-routing chapter: tool, interior shortcut, cold repair, changed escape.
const Builder = preload("res://scripts/world_builder.gd")
const Materials = preload("res://scripts/environment_materials.gd")
var facility: Node3D
var game: Node3D
var mode: int = 0 # 0 off, 1 west, 2 east; only one branch can be powered.
var has_prybar: bool = false
var service_opened: bool = false
var core_extracted: bool = false
var complete: bool = false
var intro_seen: bool = false
var east_seen: bool = false
var lock_target: StaticBody3D
var lights: Array[Light3D] = []
var light_modes: Array[int] = []
var escape_sign: Label3D
var prop_lid: Node3D
# Keep the save key "plate" for compatibility; it now means manual lift interlock.
var plate_latched: bool = false
var hunter_step_remaining: float = .7

func _ready() -> void:
	var tile := Materials.pbr("old_linoleum_flooring_01","a0a397",.5)
	Builder.box(facility,Vector3(44,.4,48),Vector3(82,-.2,8),tile,true)
	Builder.box(facility,Vector3(44,.25,48),Vector3(82,3.72,8),facility.ceiling_material,true)
	for x: int in range(62,104,4):
		for z: int in range(-14,32,4): Builder.box(facility,Vector3(4,.04,4),Vector3(x,-.01,z),tile)
	for pair: Array in [[Vector3(82,0,-16),Vector3(44,3.6,.4)],[Vector3(82,0,32),Vector3(44,3.6,.4)],[Vector3(104,0,8),Vector3(.4,3.6,48)],[Vector3(70,0,-8),Vector3(20,3.6,.4)],[Vector3(94.5,0,-8),Vector3(19,3.6,.4)]]: facility._wall(pair[0],pair[1])
	# Central spine with powered front doors and independent rear escape doors.
	for x: float in [79,85]:
		for pair: Vector2 in [Vector2(-1.75,12.5),Vector2(16.5,18),Vector2(30.25,3.5)]:
			facility._wall(Vector3(x,0,pair.x),Vector3(.4,3.6,pair.y))
		facility._add_door("res_west_gate" if x==79 else "res_east_gate",Vector3(x,0,6),PI/2,"res_west" if x==79 else "res_east",3)
		facility._add_door("res_west_service" if x==79 else "res_east_service",Vector3(x,0,27),PI/2,"res_service" if x==79 else "",3,x==85)
	for center: float in [70,94.5]:
		facility._wall(Vector3(center-6,0,13),Vector3(8,3.6,.4))
		facility._wall(Vector3(center+6,0,13),Vector3(8,3.6,.4))
		facility._add_door("res_inner_%d" % int(center),Vector3(center,0,13),0,"",4,true)
	facility._add_door("res_entry",Vector3(60,0,-12),PI/2,"res_entry",3.2)
	facility._add_door("res_return",Vector3(60,0,26),PI/2,"res_return",3.2)
	# The return opening leads directly from the workshop to the operating theatre.
	var wood := Materials.surface("concrete","625441",2,.98)
	var cloth := Materials.surface("concrete","716d61",2,.98)
	for at: Vector3 in [Vector3(65,0,1),Vector3(75,0,1),Vector3(90,0,20),Vector3(99,0,20)]: facility._bed(at,cloth)
	facility._bed(Vector3(90,0,7),cloth)
	for x: float in [63,67,72,76]:
		Builder.box(facility,Vector3(1.6,1.1,1.3),Vector3(x,.55,9),facility.metal_material,true)
		var drum := TorusMesh.new()
		drum.inner_radius = .28
		drum.outer_radius = .36
		var visual := MeshInstance3D.new()
		visual.mesh = drum
		visual.material_override = Builder.material("9b9d8b")
		visual.position = Vector3(x,.60,8.30)
		visual.rotation.x = PI/2
		facility.add_child(visual)
	facility._table(Vector3(66,0,23),Vector3(4,.8,1.4),wood)
	facility._table(Vector3(90,0,0),Vector3(3,.8,1.3),wood)
	for at: Vector3 in [Vector3(87,0,-4),Vector3(101,0,15)]:
		preload("res://scripts/imported_props.gd").visual(facility,"painted_wooden_cabinet_02",Vector3(1.0,1.95,.65),at)
		preload("res://scripts/imported_props.gd").body(facility,Vector3(1,1.95,.65),at+Vector3(0,.975,0))
	facility._desk_lamp(Vector3(90,.88,0),-.3)
	for i: int in range(12):
		Builder.box(facility,Vector3(.06,.20,.16),Vector3(65+i*.1,1.0,23),Builder.material("635342" if i%2==0 else "393e46"))
	# Glass-fronted observation cell: a visible threat, physically sealed until release.
	for pair: Array in [[Vector3(97.5,0,1),Vector3(.3,3.6,6)],[Vector3(101,0,-2),Vector3(6,3.6,.3)],[Vector3(98.5,0,10),Vector3(1,3.6,.3)],[Vector3(103.5,0,10),Vector3(1,3.6,.3)]]: facility._wall(pair[0],pair[1])
	# A second wall completes the left edge around its small viewing window.
	facility._wall(Vector3(97.5,0,8),Vector3(.3,3.6,4))
	var glass := Builder.material("49635b")
	glass.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	glass.albedo_color.a = .18
	Builder.box(facility,Vector3(.1,2.5,2),Vector3(97.5,1.55,5),glass,true)
	for z: float in [4,6]: Builder.box(facility,Vector3(.12,2.7,.10),Vector3(97.42,1.55,z),facility.metal_material)
	for y: float in [.25,2.85]: Builder.box(facility,Vector3(.12,.1,2.2),Vector3(97.42,y,5),facility.metal_material)
	facility._sign("НАБЛЮДЕНИЕ / НЕ БУДИТЬ",Vector3(97.30,2.95,5),-PI/2,16)
	_lamp(Vector3(101,3.2,5),0)
	# Creature exits through the visible rear gate into the eastern dormitory.
	facility._add_door("res_cell",Vector3(101,0,10),0,"res_cell",2.8)
	for at: Vector3 in [Vector3(82,0,-12),Vector3(82,0,1),Vector3(82,0,16),Vector3(82,0,29),Vector3(68,0,3),Vector3(70,0,24),Vector3(92,0,3),Vector3(94,0,25)]:
		_lamp(at+Vector3(0,3.2,0),0 if at.x==82 else (1 if at.x<79 else 2))
	facility._sign("ЖИЛОЙ КОРПУС / ПОСТ ПИТАНИЯ",Vector3(82,2.7,-15.5),0,24)
	facility._sign("ЗАПАД: ПРАЧЕЧНАЯ И МАСТЕРСКАЯ\nВОСТОК: ИНСТРУМЕНТ, СПАЛЬНИ И ЛИФТ\nМОЩНОСТИ ХВАТАЕТ НА ОДНУ ВЕТКУ",Vector3(86,2.1,-13),PI/2,17)
	facility._sign("РЕМОНТНЫЙ МОДУЛЬ\nОБЕСТОЧИТЬ ЗАПАД ПЕРЕД СНЯТИЕМ\nКРЕПЛЕНИЕ: ВЫСТРЕЛ ИЛИ НОЖ V",Vector3(69,2.2,4),0,19)
	facility._sign("МАСТЕРСКАЯ\nИНСТРУМЕНТ В ТУМБЕ ДАЛЬНЕЙ ВОСТОЧНОЙ СПАЛЬНИ\nСЛУЖЕБНЫЙ ЗАСОВ ОТКРЫВАЕТСЯ ИЗНУТРИ",Vector3(73,2.5,29),PI,16)
	facility._sign("ОНИ НЕ СПАЛИ. ОНИ ЖДАЛИ.",Vector3(87,2.1,7),PI/2,17)
	escape_sign = facility._sign("АВАРИЙНЫЙ ПРОХОД / ЗАКРЫТ",Vector3(61,2.8,26),PI/2,21)
	lock_target = preload("res://scripts/breakable_lock.gd").new()
	lock_target.wing = self
	lock_target.position = Vector3(70,1.12,4.37)
	facility.add_child(lock_target)
	prop_lid = Builder.box(facility,Vector3(.62,.04,.4),Vector3(70,1.48,4),facility.metal_material)
	facility._sign("РУЧНОЙ СТОПОР ЛИФТА\nВОСТОК ПОД ПИТАНИЕМ / МОДУЛЬ СНЯТ\nУДЕРЖИВАТЬ F — ОСВОБОДИТЬ ПРИВОД",Vector3(102.5,2.5,22),-PI/2,17)
	facility._sign("ИРИНА / НОЧНАЯ СМЕНА\nВернусь к утру. Не жди у окна.",Vector3(87,1.7,-3.5),PI/2,15)

func configure(owner_game: Node3D) -> void:
	game = owner_game
	_refresh_power()

func _lamp(at: Vector3, branch: int) -> void:
	Builder.box(facility,Vector3(.8,.08,.25),at,facility.metal_material)
	Builder.box(facility,Vector3(.7,.018,.18),at-Vector3(0,.05,0),Builder.material("bfccc5",.6))
	var light := SpotLight3D.new()
	light.position = at-Vector3(0,.15,0)
	light.light_color = Color("b6ac87") if branch==1 else Color("94a7a2")
	light.rotation.x = -PI/2
	light.light_energy = 1.4
	light.spot_range = 9
	light.spot_angle = 66
	light.shadow_enabled = true
	light.shadow_bias = .04
	light.shadow_normal_bias = .45
	light.distance_fade_enabled = true
	light.distance_fade_begin = 20
	light.distance_fade_length = 8
	light.distance_fade_shadow = 18
	facility.add_child(light)
	lights.append(light)
	light_modes.append(branch)

func interact(object: Node3D) -> void:
	if not game.mission.purge_complete:
		game.hud.announce("Жилой корпус обесточен до очистки реактора.",4)
		return
	match object.object_id:
		"residence_panel":
			if complete:
				game.hud.announce("Автомат выбило. Аварийный проход в мастерской ведёт в операционную.",5)
				return
			mode = 1 if mode==2 else 2
			_refresh_power()
			game.sound_bank.play_cue("door")
			game.emit_noise(object.position,18)
			game.hud.announce("Питание: %s. %s" % ["ЗАПАД" if mode==1 else "ВОСТОК","Пройди в западную мастерскую и открой служебный засов изнутри." if mode==1 else ("Инструмент в тумбе дальней восточной спальни. Пройди мимо наблюдательной клетки." if not has_prybar else "Запад обесточен. Вернись к модулю через служебный обход.")],7)
		"residence_prybar":
			if not game.inventory.can_take("residence_prybar"): return
			has_prybar = true
			object.mark_used()
			game.hunters[11].set_active(true)
			game.sound_bank.play_spatial("scrape",Vector3(70,0,23))
			game.emit_noise(object.position,15)
			game.hud.announce("Монтажный инструмент найден. Переключи щиток на запад, затем открой засов изнутри дальней западной мастерской. Там слышно движение.",8)
			game.save_checkpoint()
		"residence_core":
			if not can_work(object): return
			if not game.inventory.can_take("residence_core"): return
			core_extracted = true
			object.mark_used()
			prop_lid.rotation.x = -.8
			facility.gate_by_id["res_cell"].restore_state(true,false)
			facility.layout_changed.emit()
			game.hunters[13].set_active(true)
			game.emit_noise(object.position,70)
			game.sound_bank.play_cue("core")
			game.sound_bank.play_spatial("door",Vector3(101,0,10))
			game.hud.announce("Модуль снят. В клетке щёлкнул замок. Охотник слепой: Ctrl — тихий шаг, G — отвлечь. Ручной стопор у восточной стены дальней спальни. Его привод шумит.",10)
			game.save_checkpoint()
		"residence_plate":
			if not can_work(object): return
			plate_latched = true
			object.mark_used()
			game.emit_noise(object.position,24)
			game.sound_bank.play_spatial("door",object.position,.7)
			game.hud.announce("Ручной стопор снят. Установи модуль в лифтовой щиток. Привод шумит — не задерживайся.",6)
			game.save_checkpoint()
		"residence_lift":
			if not can_work(object): return
			complete = true
			object.mark_used()
			mode = 0
			_refresh_power()
			var entry: Node3D = facility.gate_by_id["res_entry"]
			entry.restore_state(false,true)
			facility.gate_by_id["res_return"].restore_state(true,false)
			facility.gate_by_id["res_cell"].restore_state(true,false)
			game.hunters[13].set_active(true)
			game.emit_noise(object.position,65)
			game.sound_bank.play_cue("alarm")
			escape_sign.text = "← В ОПЕРАЦИОННУЮ / АВАРИЙНЫЙ ПРОХОД"
			escape_sign.modulate = Color("e49c77")
			game.hud.announce("Блокировка выхода снята. Автомат выбило! Главный проход закрыт. Уходи через мастерскую в операционную, затем к терминалу карантина.",10)
			game.save_checkpoint()
	_refresh_status()
	game.mission.refresh_hud()

func can_work(object: Node3D) -> bool:
	var message: String = ""
	if not game.mission.purge_complete: message = "Сначала очисти реактор."
	elif object.object_id=="residence_core":
		if mode==1: message = "Опасное напряжение. Переключи питание на восток, затем вернись через служебный обход."
		elif not has_prybar or not service_opened: message = "Нужен инструмент из мастерской и открытый служебный обход."
		elif not lock_target.broken: message = "Сначала разбей крепление выстрелом или ударом ножа V."
	elif object.object_id in ["residence_lift","residence_plate"]:
		if not core_extracted: message = "Нужен модуль из обесточенной прачечной. Инструкция на его щитке."
		elif mode!=2: message = "Лифту нужно питание восточной ветки. Переключатель у входа в корпус."
		elif object.object_id=="residence_lift" and not plate_latched: message = "Освободи ручной стопор у восточной стены дальней спальни."
	if not message.is_empty(): game.hud.announce(message,6)
	return message.is_empty()

func interact_door(door: Node3D) -> void:
	if door.door_id=="res_entry":
		if not game.mission.purge_complete or complete:
			game.hud.announce("После очистки реактора откроется доступ." if not complete else "Проход отрезан. Аварийный выход в мастерской!",5)
			return
	elif door.door_id=="res_return" and not complete:
		game.hud.announce("Аварийная створка управляется из жилого корпуса.",4)
		return
	elif door.door_id=="res_cell":
		game.hud.announce("Наблюдательная клетка. Замок не отвечает.",3)
		return
	elif door.lock_kind in ["res_west","res_east"] and door.locked:
		game.hud.announce("Ветка обесточена. Щиток у входа в жилой корпус. Есть служебный обход с дальней стороны.",5)
		return
	elif door.lock_kind=="res_service" and door.locked:
		if not has_prybar or game.hero.position.x>=79:
			game.hud.announce("Засов изнутри. Инструмент на столе в мастерской западной ветки.",5)
			return
		service_opened = true
		game.emit_noise(door.position,18)
		game.hud.announce("Служебный обход открыт. Теперь можно обесточить запад и вернуться к модулю.",6)
	door.locked = false
	if door.toggle_door(game.hero.position):
		game.sound_bank.play_spatial("door",door.position)
		if door.door_id=="res_west_service": game.save_checkpoint()

func _refresh_power() -> void:
	for i: int in range(lights.size()):
		var powered: bool = light_modes[i]==0 or light_modes[i]==mode
		lights[i].light_energy = 1.4 if powered and not complete else .2
		lights[i].light_color = Color("ba7759") if complete else (Color("b6ac87") if light_modes[i]==1 else Color("94a7a2"))
	for pair: Array in [["res_west_gate",1],["res_east_gate",2]]:
		var door: Node3D = facility.gate_by_id[pair[0]]
		door.restore_state(mode==pair[1],mode!=pair[1])
	if game!=null:
		game.mission.objects["residence_panel"].state_value = mode
		game.mission.objects["residence_panel"].status_label.text = "ПИТАНИЕ: "+["ОТКЛЮЧЕНО","ЗАПАД","ВОСТОК"][mode]
		_refresh_status()
	facility.layout_changed.emit()

func _refresh_status() -> void:
	var core: Node3D = game.mission.objects["residence_core"]
	core.status_label.text = "МОДУЛЬ СНЯТ" if core_extracted else ("ОПАСНОЕ НАПРЯЖЕНИЕ" if mode==1 else ("ОБЕСТОЧЕНО / РАЗБИТЬ КРЕПЛЕНИЕ" if not lock_target.broken else "ОБЕСТОЧЕНО / УДЕРЖИВАТЬ F"))
	core.status_label.modulate = Color("d0a57c") if mode==1 else Color("a0b8aa")
	var lift: Node3D = game.mission.objects["residence_lift"]
	lift.status_label.text = "АВАРИЙНЫЙ ВЫХОД ОТКРЫТ" if complete else ("НЕТ МОДУЛЯ" if not core_extracted else ("НЕТ ПИТАНИЯ" if mode!=2 else ("СТОПОР НЕ ЗАФИКСИРОВАН" if not plate_latched else "УДЕРЖИВАТЬ F / РЕМОНТ")))

func update(delta: float) -> void:
	if not game.mission.purge_complete: return
	if core_extracted:
		hunter_step_remaining -= delta
		var hunter: CharacterBody3D = game.hunters[13]
		if hunter.active and hunter.stunned_remaining<=0 and hunter.velocity.length()>.4 and hunter_step_remaining<=0 and hunter.position.distance_to(game.hero.position)<20:
			game.sound_bank.play_spatial("step",hunter.position,.55)
			hunter_step_remaining = .65
	if game.hero.position.x>63 and not intro_seen:
		intro_seen = true
		game.hunters[13].show()
		game.hunters[13].model.rotation.y = PI/2
		game.hunters[13].model.set_process(true)
		game.sound_bank.play_spatial("knock",Vector3(100,0,5),.7)
		game.hud.announce("Жилой корпус. За восточным стеклом кто-то стоит. Питания хватает только на одну ветку: схема у входа.",8)
	if core_extracted and game.hero.position.x>86 and game.hero.position.z>12 and not east_seen:
		east_seen = true
		game.hunters[12].set_active(true)
		game.emit_noise(game.hero.position,18)
		game.sound_bank.play_spatial("growl",Vector3(99,0,24),.9)

func goal_id() -> String:
	if not has_prybar: return "residence_panel" if mode!=2 else "residence_prybar"
	if not service_opened: return "residence_panel" if mode!=1 else "res_west_service"
	if not core_extracted: return "residence_panel" if mode==1 else "residence_core"
	return "residence_panel" if mode!=2 else ("residence_plate" if not plate_latched else "residence_lift")

func snapshot() -> Dictionary:
	return {"mode":mode,"prybar":has_prybar,"service":service_opened,"broken":lock_target.broken,"core":core_extracted,"complete":complete,"intro":intro_seen,"east":east_seen,"plate":plate_latched}

static func valid(data: Variant) -> bool:
	if not data is Dictionary: return false
	var value: Variant = data.get("mode")
	if not (value is int or value is float) or not is_finite(float(value)) or float(value)!=floor(float(value)) or float(value)<0 or float(value)>2: return false
	for key: String in ["prybar","service","broken","core","complete","intro","east"]:
		if not data.get(key) is bool: return false
	if data["service"] and not data["prybar"]: return false
	if data["core"] and (not data["service"] or not data["broken"]): return false
	if data["complete"] and not data["core"]: return false
	if data.has("plate") and not data["plate"] is bool: return false
	if data["complete"] and not data.get("plate",true): return false
	return true

func restore(data: Dictionary) -> void:
	mode = int(data.get("mode",0))
	has_prybar = data.get("prybar",false)
	service_opened = data.get("service",false)
	core_extracted = data.get("core",false)
	complete = data.get("complete",false)
	intro_seen = data.get("intro",false)
	east_seen = data.get("east",false)
	plate_latched = data.get("plate",complete)
	if data.get("broken",false):
		lock_target.broken = true
		lock_target.collision_layer = 0
		lock_target.hide()
	for pair: Array in [["residence_prybar",has_prybar],["residence_core",core_extracted],["residence_lift",complete],["residence_plate",plate_latched]]:
		if pair[1]: game.mission.objects[pair[0]].mark_used()
	_refresh_power()
	facility.gate_by_id["res_entry"].restore_state(not complete and game.mission.purge_complete,complete or not game.mission.purge_complete)
	facility.gate_by_id["res_west_service"].restore_state(service_opened,not service_opened)
	facility.gate_by_id["res_return"].restore_state(complete,not complete)
	facility.gate_by_id["res_cell"].restore_state(core_extracted,not core_extracted)
	if complete: escape_sign.text = "← В ОПЕРАЦИОННУЮ / АВАРИЙНЫЙ ПРОХОД"
	elif intro_seen:
		game.hunters[13].show()
		game.hunters[13].model.set_process(true)

func reset_chapter() -> void:
	mode = 0
	has_prybar = false
	service_opened = false
	core_extracted = false
	complete = false
	intro_seen = false
	east_seen = false
	plate_latched = false
	hunter_step_remaining = .7
	lock_target.reset_lock()
	prop_lid.rotation.x = 0
	escape_sign.text = "АВАРИЙНЫЙ ПРОХОД / ЗАКРЫТ"
	escape_sign.modulate = Color("c2bba3")
	_refresh_power()

func on_lock_broken() -> void:
	_refresh_status()
	game.sound_bank.play_spatial("impact",lock_target.position)
	game.emit_noise(lock_target.position,28)
	game.feedback.burst(lock_target.position,"b7a379")
	game.hud.announce("Крепление разбито. Снимать модуль можно только при отключённом западе.",5)
