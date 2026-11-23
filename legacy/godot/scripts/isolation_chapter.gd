extends Node3D
## Authored branch chapter. Noise/sight drive a persistent pursuer, not respawns.
const B = preload("res://scripts/world_builder.gd")
const M = preload("res://scripts/environment_materials.gd")
const P = preload("res://scripts/imported_props.gd")
const Station = preload("res://scripts/isolation_station.gd")
const LOCATIONS := {"case":Vector3(57,0,-54),"light":Vector3(46,0,-57),"dark":Vector3(55,0,-57),"coil_light":Vector3(18.3,0,-81),"coil_dark":Vector3(58,0,-73),"tag":Vector3(37,0,-76),"diary":Vector3(18.3,0,-67),"report":Vector3(58,0,-87),"proof":Vector3(33,0,-89),"generator":Vector3(37,0,-103),"vent_a":Vector3(19,0,-100),"vent_b":Vector3(55,0,-100),"log":Vector3(48,0,-105),"release":Vector3(48,0,-54),"ammo":Vector3(55,0,-96)}
const CLUES := {"diary":"Дневник санитара: пациент 17 пытался вывести людей через душевые. Его подпись есть в журнале, но приказ подписан главным врачом.","report":"Отчёт охраны: приказ о закрытии камер поступил до аварии. Дневник и этот отчёт вместе позволяют открыть архив у северного перехода.","proof":"Архив 17: людей оставили внутри намеренно. Копия приказа сохранена. Служебная дверь из архива в южный холл теперь открыта."}
var game: Node3D
var facility: Node3D
var stations: Dictionary = {}
var lights: Array[Light3D] = []
var gates: Dictionary = {}
var partitions: Dictionary = {}
var actors: Array[CharacterBody3D] = []
var pursuer: CharacterBody3D
var flywheel: Node3D
var arc_light: OmniLight3D
var fitting: StaticBody3D
var status: Label
var case_read: bool = false
var circuit: String = ""
var coil: bool = false
var tag: bool = false
var evidence: Array[String] = []
var proof: bool = false
var generator_started: bool = false
var generator_time: float = 0
var serviced: Array[String] = []
var fault: String = ""
var generator_ready: bool = false
var log_taken: bool = false
var complete: bool = false
var released: bool = false
var seen: bool = false
var ammo_taken: bool = false
var work_id: String = ""
var work: float = 0
var clock: float = 0
var cue_clock: float = 0

func build(owner_facility: Node3D) -> void:
	facility = owner_facility
	var floor_mat := M.pbr("old_linoleum_flooring_01","8e9a96",.55)
	var floor_body: Node3D = B.box(self,Vector3(44,.4,58),Vector3(38,-.2,-79),floor_mat,true)
	for child: Node in floor_body.get_children():
		if child is MeshInstance3D: child.hide()
	for x: int in range(16,60,8):
		for z: int in range(-108,-50,8):
			var width: float = minf(8,60-x)
			var depth: float = minf(8,-50-z)
			B.box(self,Vector3(width,.025,depth),Vector3(x+width/2,-.01,z+depth/2),floor_mat)
	B.box(self,Vector3(44,.2,58),Vector3(38,3.7,-79),facility.ceiling_material,true)
	_wall(Vector3(16,0,-79),Vector3(.4,3.6,58))
	_wall(Vector3(60,0,-79),Vector3(.4,3.6,58))
	_wall(Vector3(25.5,0,-108),Vector3(19,3.6,.4))
	_wall(Vector3(49.5,0,-108),Vector3(21,3.6,.4))
	_gate("entry",Vector3(52,0,-50),0,"entry",3.6)
	# South power hall: a long west branch, a short east branch, locked middle return.
	for span: Vector2 in [Vector2(18.625,5.25),Vector2(30,10.5),Vector2(44.5,11.5),Vector2(56.875,6.25)]: _wall(Vector3(span.x,0,-62),Vector3(span.y,3.6,.35))
	_gate("west",Vector3(23,0,-62),0,"power",3.5)
	_gate("east",Vector3(52,0,-62),0,"power",3.5)
	_gate("shortcut",Vector3(37,0,-62),0,"evidence",3.5)
	for x: float in [30,44]:
		_wall(Vector3(x,0,-67.6),Vector3(.35,3.6,11.2))
		_wall(Vector3(x,0,-81.5),Vector3(.35,3.6,9.4))
		_wall(Vector3(x,0,-91.9),Vector3(.35,3.6,4.2))
	_gate("showers",Vector3(30,0,-75),PI/2,"",3.6,true)
	_gate("archive",Vector3(30,0,-88),PI/2,"evidence",3.6)
	_gate("north",Vector3(44,0,-88),PI/2,"evidence",3.6)
	_wall(Vector3(32.6,0,-82),Vector3(5.2,3.6,.35))
	_wall(Vector3(41.4,0,-82),Vector3(5.2,3.6,.35))
	_gate("archive_inner",Vector3(37,0,-82),0,"evidence",3.6)
	_wall(Vector3(37,0,-94),Vector3(14,3.6,.35))
	var glass := preload("res://scripts/breakable_partition.gd").new()
	glass.panel_id = "isolation_glass"
	glass.position = Vector3(44,1.45,-75)
	glass.panel_size = Vector3(.10,2.9,3.6)
	glass.chapter = self
	add_child(glass)
	partitions.glass = glass
	_wall(Vector3(18.5,0,-84),Vector3(5,3.6,.35))
	_wall(Vector3(27.5,0,-84),Vector3(5,3.6,.35))
	_gate("cell",Vector3(23,0,-84),0,"release",3.6)
	# A barred leaf uses real bar colliders, allowing actual sight/shots through gaps.
	var cell_body: StaticBody3D = gates.cell.body
	for child: Node in cell_body.get_children():
		if child is MeshInstance3D: child.hide()
		if child is CollisionShape3D: child.disabled = true
	for i: int in range(9):
		var at := Vector3(-1.6+i*.4,0,0)
		B.box(cell_body,Vector3(.065,2.95,.13),at,facility.metal_material)
		var shape := BoxShape3D.new()
		shape.size = Vector3(.065,2.95,.13)
		var collider := CollisionShape3D.new()
		collider.shape = shape
		collider.position = at
		cell_body.add_child(collider)
	cell_body.set_meta("impact_surface","metal")
	for id: String in LOCATIONS: _station(id,LOCATIONS[id])
	preload("res://scripts/isolation_art.gd").build(self)
	for at: Vector3 in [Vector3(52,3.1,-55),Vector3(23,3.1,-69),Vector3(23,3.1,-79),Vector3(23,3.1,-90),Vector3(37,3.1,-74),Vector3(37,3.1,-88),Vector3(52,3.1,-73),Vector3(52,3.1,-88),Vector3(24,3.1,-99),Vector3(48,3.1,-99)]:
		var light := OmniLight3D.new()
		light.position = at
		light.omni_range = 8
		light.light_color = Color("a9c6c1") if at.x<40 else Color("bd8c70")
		light.light_energy = .55
		add_child(light)
		lights.append(light)
		B.box(self,Vector3(.65,.08,.26),at+Vector3(0,.25,0),B.material("a6b6a8",.4))
	for at: Vector3 in [Vector3(52,3.4,-55),Vector3(23,3.4,-74),Vector3(37,3.4,-100)]:
		var spot := SpotLight3D.new()
		spot.position = at
		spot.rotation_degrees.x = -90
		spot.spot_range = 9
		spot.spot_angle = 58
		spot.light_color = Color("c3c5ae")
		spot.light_energy = .9
		spot.shadow_enabled = true
		add_child(spot)
		lights.append(spot)
	arc_light = OmniLight3D.new()
	arc_light.position = Vector3(52,.3,-81)
	arc_light.light_color = Color("a0d7ff")
	arc_light.omni_range = 4
	arc_light.light_energy = 0
	add_child(arc_light)
	fitting = preload("res://scripts/reactive_fitting.gd").new()
	fitting.chapter = self
	fitting.position = Vector3(50,2.7,-88)
	add_child(fitting)

func _wall(at: Vector3,size: Vector3) -> void: facility._wall(at,size)
func _gate(id: String,at: Vector3,yaw: float,kind: String,width: float,open: bool = false) -> void:
	facility._add_door("iso_"+id,at,yaw,"iso_"+kind if not kind.is_empty() else "",width,open)
	gates[id] = facility.gate_by_id["iso_"+id]
func _station(id: String,at: Vector3) -> void:
	var station := Station.new()
	station.chapter = self
	station.station_id = id
	station.position = at
	add_child(station)
	stations[id] = station
	var labels := {"case":"ПОСТ / ПРОПАВШИЙ ПАЦИЕНТ 17","light":"ЗАПАД / СВЕТ / ДЛИННЫЙ ПУТЬ","dark":"ВОСТОК / КАМЕРЫ / КОРОТКИЙ ПУТЬ","coil_light":"КОНТАКТНЫЙ БЛОК / ЗАПАД","coil_dark":"КОНТАКТНЫЙ БЛОК / ВОСТОК","tag":"ДУШЕВЫЕ / ЛИЧНЫЕ ВЕЩИ","diary":"ДНЕВНИК САНИТАРА","report":"ОТЧЁТ ОХРАНЫ","proof":"ЗАПЕЧАТАННЫЙ АРХИВ","generator":"СТАРТЕР ГЕНЕРАТОРА","vent_a":"СБРОС А","vent_b":"СБРОС Б","log":"МАГНИТНЫЙ РЕГИСТРАТОР","release":"РАЗБЛОКИРОВКА ИЗОЛЯТОРА","ammo":"ЗАПАС ПОСТА"}
	station.caption = labels[id]
	station.label.text = labels[id]
	station.label.position = Vector3(0,1.25,.32)
	station.label.font_size = 14
	P.body(station,Vector3(.9,.78,.6),Vector3(0,.39,0))
	if id in ["tag","coil_light","coil_dark","diary","report","proof","log","ammo"]:
		P.visual(station,"painted_wooden_nightstand",Vector3(.9,.78,.6))
		station.drawer = B.box(station,Vector3(.76,.22,.04),Vector3(0,.51,.32),preload("res://scripts/wood_surface.gd").make())
		B.box(station,Vector3(.19,.008,.25),Vector3(.1,.79,.04),B.material("c4bba2"))
		if id=="tag":
			var band := B.box(station.drawer,Vector3(.13,.012,.06),Vector3(-.14,.08,-.10),B.material("a6b6bd"))
			station.contents = band
			var label := Label3D.new()
			label.text = "17"
			label.font_size = 40
			label.pixel_size = .001
			label.rotation.x = -PI/2
			label.position.y = .01
			band.add_child(label)
		elif id.begins_with("coil_"):
			station.contents = P.detail(station.drawer,"circuit_board",.23,Vector3(0,.08,-.10))
		elif id=="log":
			station.contents = P.detail(station.drawer,"portable_cassette_player",.24,Vector3(0,.08,-.10))
		elif id=="ammo":
			station.contents = Node3D.new()
			station.drawer.add_child(station.contents)
			station.contents.position = Vector3(0,.08,-.10)
			preload("res://scripts/loot_visual.gd").build(station.contents,"ammo_isolation","ammo")
		if station.contents!=null: station.contents.hide()
	else:
		B.box(station,Vector3(.85,.78,.6),Vector3(0,.39,0),M.surface("metal","667a73",1.5,.6))
		for i: int in range(3): B.sphere(station,.035,Vector3((i-1)*.20,.62,.32),B.material("bcb283",.3))

func configure(owner_game: Node3D) -> void:
	game = owner_game
	pursuer = preload("res://scripts/isolation_pursuer.gd").new()
	pursuer.chapter = self
	pursuer.position = Vector3(23,.1,-90)
	game.get_node("World").add_child(pursuer)
	pursuer.patrol_points.assign([Vector3(23,.1,-90),Vector3(23,.1,-70),Vector3(37,.1,-98),Vector3(52,.1,-70)])
	game.hunters.append(pursuer)
	actors.append(pursuer)
	for at: Vector3 in [Vector3(52,.1,-90),Vector3(40,.1,-99)]:
		var actor := preload("res://scripts/stalker.gd").new()
		actor.crawler = actors.size()==1
		actor.position = at
		game.get_node("World").add_child(actor)
		actor.patrol_points.assign([at,at+Vector3(0,0,4)])
		game.hunters.append(actor)
		actors.append(actor)
	actors[2].can_spit = true
	actors[2].ranged_attack.connect(game._spawn_projectile)
	status = Label.new()
	status.set_anchors_and_offsets_preset(Control.PRESET_CENTER_BOTTOM)
	status.position = Vector2(-450,-325)
	status.size = Vector2(900,42)
	status.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	status.add_theme_font_size_override("font_size",17)
	status.add_theme_color_override("font_shadow_color",Color.BLACK)
	status.add_theme_constant_override("shadow_offset_x",2)
	status.add_theme_constant_override("shadow_offset_y",2)
	game.hud.staging.add_child(status)
	_apply_power()

func prompt(id: String) -> String:
	if stations[id].drawer!=null and not stations[id].opened: return "F — открыть шкаф · "+stations[id].caption
	if id in ["light","dark"]: return "F — выбрать "+("освещённый длинный запад" if id=="light" else "короткий тёмный восток")+" · другая ветка останется закрытой"
	if id in ["generator","vent_a","vent_b"]: return "Удерживать F — "+stations[id].caption
	return "F — "+stations[id].caption

func use_station(id: String) -> void:
	if not game.diagnostics.complete: game.hud.announce("Сначала закончи медицинскую диагностику.",4); return
	match id:
		"case":
			case_read = true
			game.hud.announce("Пациент 17 исчез в душевых. Найди его браслет и выгрузи журнал из северного регистратора. Сначала выбери линию питания: запад со светом или восток через камеры.",13)
		"light","dark":
			if not case_read: game.hud.announce("Сначала прочитай дело на посту охраны.",4); return
			if not circuit.is_empty(): game.hud.announce("Линия уже выбрана: "+("запад" if circuit=="light" else "восток")+". Резерва на вторую нет.",4); return
			circuit = id
			_apply_power()
			game.sound_bank.play_spatial("isolation_relay",stations[id].position)
			game.hud.announce("Линия включена. Контактный блок — в шкафу выбранной ветки. Браслет ищи в душевых между коридорами. На востоке оголённый кабель: обходи воду слева.",10)
			game.save_checkpoint()
		"coil_light","coil_dark":
			if circuit!=("light" if id=="coil_light" else "dark"): game.hud.announce("Этот шкаф остался за отключённой линией.",4); return
			if coil: return
			if not game.inventory.can_take("isolation_coil"): return
			coil = true
			game.hud.announce("Контактный блок найден. Для регистратора нужен браслет пациента из душевых.",6)
		"tag":
			if circuit.is_empty(): return
			if tag: return
			if not game.inventory.can_take("isolation_tag"): return
			tag = true
			_release_pursuer()
			game.hud.announce("Браслет 17. За стеной сорван засов. Это то же существо из камеры — ранения остаются. Северный генератор питает регистратор.",10)
			game.save_checkpoint()
		"diary","report":
			if id not in evidence: evidence.append(id)
			game.hud.announce(CLUES[id],12)
			if evidence.size()==2:
				for gate: String in ["archive","archive_inner","north"]: _unlock(gate)
		"proof":
			if evidence.size()!=2: game.hud.announce("Нужны дневник санитара и отчёт охраны.",5); return
			if proof: return
			proof = true
			_unlock("shortcut")
			game.hud.announce(CLUES.proof,12)
			game.score += 500
			game.save_checkpoint()
		"generator":
			if not coil or not tag: game.hud.announce("Нужны контактный блок выбранной ветки и браслет из душевых.",5); return
			if generator_started: return
			_start_work(id)
		"vent_a","vent_b":
			if fault!=id: game.hud.announce("Этот сброс сейчас не требуется.",3); return
			_start_work(id)
		"log":
			if not generator_ready or not tag: game.hud.announce("Регистратор обесточен. Запусти генератор и сбрось давление на обеих линиях.",6); return
			if log_taken: return
			log_taken = true
			game.hud.announce("Журнал пациента 17 выгружен. Вернись на южный пост и сними блокировку изолятора.",8)
		"release":
			if not log_taken: game.hud.announce("Блокировка ждёт журнал регистратора из северного машинного зала.",5); return
			if complete: return
			complete = true
			game.score += 350
			game.hud.announce("Изолятор разблокирован. Северная дверь машинного зала открывает хозяйственный корпус: нужно подтвердить аварийный маршрут снаружи. "+("Приказ сохранён: правда выйдет вместе с тобой." if proof else "В архиве ещё могут оставаться доказательства."),10)
			game.save_checkpoint()
		"ammo":
			if ammo_taken: return
			if not game.inventory.take_supply("ammo",int(game.difficulty_value("ammo"))): return
			ammo_taken = true
	stations[id].used = id not in ["generator","vent_a","vent_b"] or generator_ready
	if stations[id].contents!=null and done(id): stations[id].contents.hide()
	game.mission.refresh_hud()

func _start_work(id: String) -> void:
	if game.hero.performing_action or not work_id.is_empty(): return
	work_id = id
	work = 0
	game.hero.performing_action = true
	game.hero.action_focus = stations[id].position+Vector3(0,.9,.3)
func cancel_work() -> void:
	if not work_id.is_empty(): game.hero.performing_action = false
	work_id = ""
	work = 0
	if status!=null: status.text = ""
func _unlock(id: String) -> void:
	gates[id].restore_state(true,false)
	facility.layout_changed.emit()
func _apply_power(keep_doors: bool = false) -> void:
	var entry_allowed: bool = game!=null and game.diagnostics.complete
	gates.entry.restore_state(gates.entry.is_open and entry_allowed,not entry_allowed)
	for id: String in ["west","east"]:
		var powered: bool = circuit==("light" if id=="west" else "dark")
		gates[id].restore_state((gates[id].is_open if keep_doors else true) and powered,not powered)
	for light: Light3D in lights:
		light.light_energy = .65 if (circuit=="light" and light.position.x<44) or light.position.z>-62 or light.position.z<-94 else .055
	if evidence.size()==2:
		for gate: String in ["archive","archive_inner","north"]: gates[gate].restore_state(gates[gate].is_open if keep_doors else true,false)
	if proof: gates.shortcut.restore_state(gates.shortcut.is_open if keep_doors else true,false)
	if released: gates.cell.restore_state(gates.cell.is_open if keep_doors else true,false)
	facility.layout_changed.emit()
func _release_pursuer() -> void:
	if released: return
	released = true
	_unlock("cell")
	if not pursuer.dead: pursuer.set_active(true)
	game.emit_noise(stations.tag.position,45)
	game.sound_bank.play_spatial("door_break",gates.cell.position)
func on_partition_broken(_id: String) -> void:
	facility.layout_changed.emit()
func interact_door(door: Node3D) -> bool:
	if not door.door_id.begins_with("iso_"): return false
	if door.door_id=="iso_entry" and game.diagnostics.complete and door.locked: door.restore_state(true,false); facility.layout_changed.emit(); return true
	if not door.locked: return false
	game.hud.announce({"iso_entry":"Вход после завершения диагностики.","iso_west":"Запад открывается выбором света на посту охраны.","iso_east":"Восток открывается выбором камер на посту охраны.","iso_archive":"Сопоставь дневник санитара и отчёт охраны.","iso_shortcut":"Открывается после копирования приказа в архиве.","iso_cell":"Засов держит камеру. За ним кто-то дышит."}.get(door.door_id,"Дверь заблокирована."),6)
	return true

func update(delta: float) -> void:
	clock += delta
	status.visible = not game.is_paused and not game.has_lost and not game.has_won and not game.in_title_menu
	status.text = ""
	if game.diagnostics.complete and gates.entry.locked: gates.entry.restore_state(false,false); facility.layout_changed.emit()
	if game.hero.position.z<-62 and not seen:
		seen = true
		if not pursuer.dead: pursuer.set_active(true)
		game.sound_bank.play_spatial("restraint_drag",pursuer.position)
		game.hud.announce("За решёткой — силуэт с ремнями на груди. Он смотрит на тебя.",6)
	var live: bool = circuit=="dark" and fmod(clock,3.4)<.85 and not generator_ready
	arc_light.light_energy = 1.9 if live else 0
	if live and Rect2(48,-83,8,3).has_point(Vector2(game.hero.position.x,game.hero.position.z)) and game.hero.invulnerable_remaining<=0:
		_take_shock()
	if not work_id.is_empty():
		if not Input.is_action_pressed("interact_hold") or game.hero.position.distance_to(stations[work_id].position)>2.1:
			cancel_work()
		else:
			work += delta
			var duration: float = 6 if work_id=="generator" else 3
			status.text = "УДЕРЖИВАЙ F / "+stations[work_id].caption+" / %d%%"%mini(100,int(work/duration*100))
			if fmod(work,.7)<delta: game.emit_noise(stations[work_id].position,26); game.sound_bank.play_spatial("isolation_relay",stations[work_id].position,.6)
			if work>=duration:
				if work_id=="generator":
					generator_started = true
					actors[1].set_active(true)
					actors[2].set_active(true)
					game.hud.announce("Генератор запущен. Следи за давлением: сбросы А и Б на противоположных стенах. Здесь можно отступать и использовать мебель как укрытие.",9)
				else: serviced.append(work_id); fault = ""
				cancel_work()
	if generator_started and not generator_ready:
		flywheel.rotation.z += delta*7
		if fault.is_empty(): generator_time = minf(25,generator_time+delta)
		if generator_time>=7 and "vent_a" not in serviced: fault = "vent_a"
		elif generator_time>=16 and "vent_b" not in serviced: fault = "vent_b"
		if not fault.is_empty(): status.text = "ДАВЛЕНИЕ / "+("СБРОС А: ЗАПАДНАЯ СТЕНА" if fault=="vent_a" else "СБРОС Б: ВОСТОЧНАЯ СТЕНА")+" / удерживать F"
		if generator_time>=25:
			generator_ready = true
			game.hud.announce("Регистратор получил питание. Открой его шкаф у северной стены.",7)
			game.save_checkpoint()
	if generator_ready: flywheel.rotation.z += delta*2
	if game.hero.position.z<-62:
		cue_clock -= delta
		if cue_clock<=0:
			game.sound_bank.play_spatial("isolation_pipe",Vector3(37,2.8,-83),.6)
			cue_clock = 18

func _take_shock() -> void:
	if game.hero.free_flight:return
	cancel_work()
	game.survival_tools.cancel_action()
	game.upgrades.cancel_barricade()
	game.hero.react_hit(arc_light.global_position)
	game.health -= 1
	game.hero.invulnerable_remaining = float(game.difficulty_value("invulnerability"))
	game.hero.camera_rig.shake_remaining = .4
	game.hud.flash_damage()
	game.sound_bank.play_spatial("isolation_relay",arc_light.global_position)
	if game.health<=0: game._finish_round(false,"Оголённый кабель в воде. Следи за вспышками и обходи опасный участок по сухой полосе слева.")
	else: game.hud.announce("Разряд! Обойди воду по сухой полосе слева.",5)

func goal_id() -> String:
	if not case_read: return "case"
	if circuit.is_empty(): return "light"
	if not coil: return "coil_light" if circuit=="light" else "coil_dark"
	if not tag: return "tag"
	if not generator_started: return "generator"
	if not fault.is_empty(): return fault
	if not generator_ready: return "generator"
	if not log_taken: return "log"
	return "release"
func done(id: String) -> bool:
	return {"case":case_read,"light":circuit=="light","dark":circuit=="dark","coil_light":coil,"coil_dark":coil,"tag":tag,"diary":"diary" in evidence,"report":"report" in evidence,"proof":proof,"generator":generator_started,"vent_a":"vent_a" in serviced,"vent_b":"vent_b" in serviced,"log":log_taken,"release":complete,"ammo":ammo_taken}.get(id,false)
func objective() -> Array[String]:
	var hints := {"case":"Вход — северная дверь процедурной. Прочти дело пациента на посту.","light":"Выбери линию: светлый длинный запад или короткий тёмный восток.","coil_light":"Шкаф контактного блока в западном коридоре. F открывает, повторное F забирает.","coil_dark":"Шкаф контактного блока у восточной стены. Шум стекла привлекает существо.","tag":"Браслет в шкафу душевых между коридорами. Восточный вход закрыт разбиваемым стеклом.","generator":"Северный машинный зал. Удерживай F у стартера; сбрасывай давление А и Б.","vent_a":"Северный машинный зал: западная стена, удерживай F3с.","vent_b":"Северный машинный зал: восточная стена, удерживай F3с.","log":"Шкаф регистратора у северной стены справа от генератора.","release":"Вернись на южный пост охраны. Доказательства открывают центральный обратный проход."}
	return ["Старый изолятор / пациент 17",hints[goal_id()]]
func journal_entries() -> Array[String]:
	var result: Array[String] = []
	if case_read: result.append("Изолятор: пациент17 исчез в душевых. Выбранная линия открывает один путь. Контактный блок питает северный регистратор; браслет подтверждает личность.")
	for id: String in evidence: result.append(CLUES[id])
	if proof: result.append(CLUES.proof)
	if log_taken: result.append("Журнал17: пациент пытался остановить закрытие камер. Данные вынесены из регистратора.")
	return result
func ending() -> String:
	return "\n\nПациент 17: "+("приказ о намеренном закрытии камер и журнал переданы следствию." if proof else "журнал вынесен, но приказ о закрытии камер остался в архиве.") if complete else ""

func snapshot() -> Dictionary:
	var positions: Array = []
	for actor: CharacterBody3D in actors: positions.append([actor.position.x,actor.position.y,actor.position.z])
	return {"case":case_read,"circuit":circuit,"coil":coil,"tag":tag,"evidence":evidence.duplicate(),"proof":proof,"started":generator_started,"time":generator_time,"serviced":serviced.duplicate(),"fault":fault,"ready":generator_ready,"log":log_taken,"complete":complete,"released":released,"seen":seen,"ammo":ammo_taken,"glass":partitions.glass.broken,"fitting":fitting.broken,"positions":positions}
static func valid(data: Variant) -> bool:
	if not data is Dictionary: return false
	for key: String in ["case","coil","tag","proof","started","ready","log","complete","released","seen","ammo","glass","fitting"]:
		if not data.get(key) is bool: return false
	if data.get("circuit") not in ["","light","dark"] or data.get("fault") not in ["","vent_a","vent_b"]: return false
	if not (data.get("time") is int or data.get("time") is float) or not is_finite(float(data.time)) or data.time<0 or data.time>26: return false
	for pair: Array in [["evidence",["diary","report"]],["serviced",["vent_a","vent_b"]]]:
		if not data.get(pair[0]) is Array or data[pair[0]].size()>2: return false
		var seen_ids: Array = []
		for id: Variant in data[pair[0]]:
			if id not in pair[1] or id in seen_ids: return false
			seen_ids.append(id)
	if data.circuit!="" and not data.case: return false
	if (data.coil or data.tag) and data.circuit=="": return false
	if data.proof and data.evidence.size()!=2: return false
	if data.released!=data.tag: return false
	if data.started and (not data.coil or not data.tag): return false
	if data.ready and (not data.started or data.serviced.size()!=2 or data.time<25): return false
	if data.log and not data.ready: return false
	if data.complete and not data.log: return false
	if not data.get("positions") is Array or data.positions.size()!=3: return false
	for at: Variant in data.positions:
		if not at is Array or at.size()!=3: return false
		for value: Variant in at:
			if not (value is int or value is float) or not is_finite(float(value)): return false
		if at[0]<-30 or at[0]>104 or at[1]<-5.2 or at[1]>4 or at[2]<-192 or at[2]>82: return false
	return true
func restore(data: Dictionary) -> void:
	if data.is_empty(): return
	case_read = data.case
	circuit = data.circuit
	coil = data.coil
	tag = data.tag
	evidence.assign(data.evidence)
	proof = data.proof
	generator_started = data.started
	generator_time = float(data.time)
	serviced.assign(data.serviced)
	fault = data.fault
	generator_ready = data.ready
	log_taken = data.log
	complete = data.complete
	released = data.released
	seen = data.seen
	ammo_taken = data.ammo
	partitions.glass.set_broken(data.glass)
	fitting.set_broken(data.fitting)
	for i: int in range(3): actors[i].position = Vector3(data.positions[i][0],data.positions[i][1],data.positions[i][2])
	_apply_power(true)
func reset_chapter() -> void:
	cancel_work()
	case_read = false
	circuit = ""
	coil = false
	tag = false
	evidence.clear()
	proof = false
	generator_started = false
	generator_time = 0
	serviced.clear()
	fault = ""
	generator_ready = false
	log_taken = false
	complete = false
	released = false
	seen = false
	ammo_taken = false
	partitions.glass.set_broken(false)
	fitting.set_broken(false)
	for station: Node3D in stations.values(): station.reset_station()
	for id: String in ["archive","archive_inner","north","shortcut","cell"]: gates[id].restore_state(false,true)
	_apply_power()
