extends Node3D
## New physical chapter, optional evidence chain and changed return route.
const B = preload("res://scripts/world_builder.gd")
const M = preload("res://scripts/environment_materials.gd")
const P = preload("res://scripts/imported_props.gd")
const Station = preload("res://scripts/chapter_station.gd")
const Partition = preload("res://scripts/breakable_partition.gd")
var game: Node3D
var facility: Node3D
var screen: CanvasLayer
var stations: Dictionary = {}
var partitions: Dictionary = {}
var xray_solved: bool = false
var observed: bool = false
var complete: bool = false
var evidence: Array[String] = []
var reward_taken: bool = false
var office_reward: bool = false
var ending_view: CanvasLayer
var finale_started: bool = false
var breaker_reset: bool = false
var exit_released: bool = false
var trap_armed: bool = false
var trap_spent: bool = false
var work_id: String = ""
var work: float = 0
var wire_light: OmniLight3D
var time: float = 0
var evidence_label: Label3D
var art_lights: Array[Light3D] = []

func build(owner_facility: Node3D) -> void:
	facility = owner_facility
	var floor_mat := M.pbr("old_linoleum_flooring_01","a1aca9",.55)
	# Corridors and treatment rooms share a real floor and a ceiling.
	B.box(self,Vector3(44,.4,34),Vector3(38,-.2,-33),floor_mat,true)
	B.box(self,Vector3(44,.23,34),Vector3(38,3.72,-33),facility.ceiling_material,true)
	B.box(self,Vector3(18,.4,8),Vector3(7,-.2,-20),floor_mat,true)
	B.box(self,Vector3(18,.23,8),Vector3(7,3.72,-20),facility.ceiling_material,true)
	_wall(Vector3(-2,0,-20),Vector3(.4,3.6,8))
	_wall(Vector3(7,0,-24),Vector3(18,3.6,.4))
	_wall(Vector3(33.1,0,-50),Vector3(34.2,3.6,.5))
	_wall(Vector3(56.9,0,-50),Vector3(6.2,3.6,.5))
	_wall(Vector3(60,0,-33),Vector3(.5,3.6,34))
	_wall(Vector3(16,0,-37),Vector3(.5,3.6,26))
	# Three legible entrances from the south gallery.
	for span: Vector2 in [Vector2(18.75,5.5),Vector2(30,10.8),Vector2(44.5,12.6),Vector2(57.25,5.5)]:
		_wall(Vector3(span.x,0,-24),Vector3(span.y,3.6,.4))
	for x: float in [23,37,52]:
		facility._add_door("diag_%d"%int(x),Vector3(x,0,-24),0,"diagnosis" if x!=23 else "",3.4,x==23)
	# X-ray to observation connection; the initial solution opens both entrances.
	_wall(Vector3(30,0,-30.2),Vector3(.4,3.6,12.4))
	_wall(Vector3(30,0,-44.8),Vector3(.4,3.6,10.4))
	facility._add_door("diag_cross",Vector3(30,0,-38),PI/2,"diagnosis",3.6)
	# A glass shortcut and a northern preparation loop offer different risks.
	_wall(Vector3(44,0,-27.7),Vector3(.4,3.6,7.4))
	_wall(Vector3(44,0,-39.2),Vector3(.4,3.6,9.0))
	_wall(Vector3(44,0,-48.8),Vector3(.4,3.6,2.4))
	_partition("observation_glass",Vector3(44,1.45,-33),Vector3(.10,2.9,3.8),"glass")
	facility._add_door("diag_loop",Vector3(44,0,-45.6),PI/2,"protocol",3.8)
	# Locked doctor's office is optional and has its own discovery chain.
	_wall(Vector3(20,0,-42),Vector3(8,3.6,.3))
	_wall(Vector3(24,0,-43),Vector3(.3,3.6,2))
	_wall(Vector3(24,0,-49),Vector3(.3,3.6,2))
	facility._add_door("diag_office",Vector3(24,0,-46),PI/2,"evidence",3.6)
	facility._add_door("diag_entry",Vector3(37,0,-16),0,"badge",3.4)
	facility._add_door("final_passage",Vector3(0,0,-16),0,"finale",3.2)
	facility._add_door("final_research_return",Vector3(15,0,-5),PI/2,"finale",3.2)
	_build_art()
	preload("res://scripts/diagnostic_art.gd").build(self)
	_station("xray",Vector3(18.5,0,-32),PI/2,"НЕГАТОСКОП / ДВЕ ЭКСПОЗИЦИИ")
	_station("observe",Vector3(32,0,-35),PI/2,"КАРТА НАБЛЮДЕНИЯ 04")
	_station("treatment",Vector3(57.8,0,-39),-PI/2,"СНЯТИЕ МЕДИЦИНСКОЙ БЛОКИРОВКИ")
	_station("memo",Vector3(18.7,0,-26.3),PI/2,"ПИСЬМО ИРИНЫ")
	_station("log",Vector3(33,0,-41.7),PI/2,"ЖУРНАЛ ПЕРЕВОДОВ")
	_station("office",Vector3(18,0,-47),PI/2,"АРХИВ ПОСЛЕДНЕЙ СМЕНЫ")
	_station("supplies",Vector3(48,0,-47.8),0,"ШКАФ ПОДГОТОВКИ / 8 ПАТРОНОВ")
	_station("trap",Vector3(48.5,0,-29),0,"АВАРИЙНЫЙ РАЗРЯД / ОДНО ИСПОЛЬЗОВАНИЕ")
	_station("final_reset",Vector3(-22,0,-5.8),0,"РУЧНОЙ СБРОС ВВОДА")
	_station("exit_crank",Vector3(0,0,14.6),PI,"АВАРИЙНАЯ РУКОЯТЬ ВЫХОДА")
	# A thin removable maintenance screen can be shot through; walls cannot.
	_partition("service_sheet",Vector3(54,1.2,-44),Vector3(2.8,2.4,.06),"sheet")
	wire_light = OmniLight3D.new()
	wire_light.position = Vector3(48,1,-34.5)
	wire_light.light_color = Color("9cd8ef")
	wire_light.omni_range = 4
	wire_light.light_energy = 0
	add_child(wire_light)

func configure(owner_game: Node3D) -> void:
	game = owner_game
	screen = preload("res://scripts/diagnostic_screen.gd").new()
	game.add_child(screen)
	screen.configure(self)
	for at: Vector3 in [Vector3(39,.1,-38),Vector3(53,.1,-35),Vector3(7,.1,-20)]:
		var hunter: CharacterBody3D = preload("res://scripts/stalker.gd").new()
		hunter.position = at
		hunter.crawler = game.hunters.size()!=15
		game.get_node("World").add_child(hunter)
		hunter.patrol_points.assign([at,at+Vector3(0,0,3)])
		game.hunters.append(hunter)
	game.hunters[15].can_spit = true
	game.hunters[15].ranged_attack.connect(game._spawn_projectile)

func _wall(at: Vector3, size: Vector3) -> void: facility._wall(at,size)

func _station(id: String, at: Vector3, yaw: float, caption: String) -> void:
	var station := Station.new()
	station.chapter = self
	station.station_id = id
	station.caption = caption
	station.position = at
	station.rotation.y = yaw
	add_child(station)
	stations[id] = station
	if id in ["xray","observe","treatment","final_reset","trap"]:
		var steel := M.surface("metal","52645f",1.5,.55)
		B.box(station,Vector3(.85,1.25,.4),Vector3(0,.625,0),steel,true)
		B.box(station,Vector3(.68,.32,.025),Vector3(0,1.03,.22),B.material("536f65",.35))
		for i: int in range(3): B.sphere(station,.026,Vector3((i-1)*.18,.67,.23),B.material(["739f9b","c5ae75","80a28a"][i],.3))
		station.label.text = {"xray":"РЕНТГЕН / 04","observe":"КАРТА / 04","treatment":"ПРОТОКОЛ / 04","final_reset":"РЕЗЕРВНЫЙ ВВОД","trap":"РАЗРЯД / 1"}[id]
		station.label.position = Vector3(0,1.03,.245)
	elif id=="exit_crank":
		B.box(station,Vector3(.6,.4,.25),Vector3(0,1.05,0),M.surface("metal","5e6253",1))
		B.box(station,Vector3(.08,.5,.08),Vector3(.2,.85,.2),M.surface("metal","ada084",1))
	else:
		P.body(station,Vector3(.9,.76,.6),Vector3(0,.38,0))
		P.visual(station,"painted_wooden_nightstand",Vector3(.9,.76,.6))
		P.detail(station,"portable_cassette_player",.27,Vector3(0,.78,.02))
		station.label.text = {"memo":"ПИСЬМО ИРИНЫ","log":"ЖУРНАЛ ПЕРЕВОДОВ","office":"ЛИЧНОЕ ДЕЛО","supplies":"ПРИПАСЫ"}.get(id,caption)
		station.label.position = Vector3(0,.85,.30)
	station.label.pixel_size = .0023
	station.label.font_size = 18
	station.label.outline_size = 0

func _partition(id: String, at: Vector3, size: Vector3, kind: String) -> void:
	var panel := Partition.new()
	panel.panel_id = id
	panel.panel_size = size
	panel.kind = kind
	panel.chapter = self
	panel.position = at
	add_child(panel)
	panel.broken_changed.connect(func() -> void: facility.layout_changed.emit())
	partitions[id] = panel

func _build_art() -> void:
	var steel := M.surface("metal","788682",1,.48)
	var paper := B.material("b6baa9")
	for at: Vector3 in [Vector3(23,3.3,-20),Vector3(37,3.3,-20),Vector3(52,3.3,-20),Vector3(24,3.3,-35),Vector3(38,3.3,-35),Vector3(52,3.3,-38),Vector3(37,3.3,-46)]:
		facility._lamp(at,Color("8fa8b8") if at.x<30 else Color("abb59d"),.65,7)
		B.box(self,Vector3(1.25,.07,.2),at,steel)
		B.box(self,Vector3(1.1,.025,.12),at-Vector3(0,.045,0),B.material("c0cfc5",.7))
	# Normal-mapped scanned furniture, fixed against walls with broad aisles.
	for at: Vector3 in [Vector3(28.5,0,-27),Vector3(41.9,0,-27),Vector3(58.6,0,-47.5)]:
		P.body(self,Vector3(1.4,2.05,.6),at+Vector3(0,1.025,0))
		P.face(P.visual(self,"painted_wooden_cabinet_02",Vector3(1.4,2.05,.6),at),at,PI)
	for at: Vector3 in [Vector3(25,0,-34),Vector3(38,0,-33),Vector3(52,0,-38)]:
		facility._bed(at,M.pbr("stretch_poplin","c0c2b1",2))
		B.box(self,Vector3(.85,.014,.45),at+Vector3(0,.84,-.55),paper)
		P.detail(self,"medical_tape",.16,at+Vector3(.25,.87,-.5))
	# X-ray scanner arm, not a loose glowing cube.
	B.box(self,Vector3(.55,2.6,.55),Vector3(21,1.3,-36),steel,true)
	B.box(self,Vector3(3.2,.22,.5),Vector3(22.4,2.65,-36),steel)
	B.box(self,Vector3(.85,.65,.8),Vector3(23.6,2.25,-36),steel)
	B.sphere(self,.19,Vector3(23.6,1.9,-36),B.material("1f2929")).scale.y = .3
	B.box(self,Vector3(.12,1.35,2.4),Vector3(16.35,1.95,-34),steel)
	B.box(self,Vector3(.025,1.15,2.2),Vector3(16.43,1.95,-34),B.material("849d9b",.25))
	for i: int in range(8):
		var rib := B.box(self,Vector3(.03,.035,.65-absf(i-3.5)*.05),Vector3(16.46,1.5+i*.12,-34),paper)
		rib.rotation.x = .12
	# Blinds cast a structured pool of cold light without closing a doorway.
	for i: int in range(13): B.box(self,Vector3(3.4,.045,.08),Vector3(36,1.05+i*.13,-49.67),steel)
	var fill := OmniLight3D.new()
	fill.position = Vector3(36,2.2,-48.5)
	fill.light_color = Color("7b9bb5")
	fill.light_energy = .8
	fill.omni_range = 8
	add_child(fill)
	art_lights.append(fill)
	for x: float in [18,28,32,42,46,58]:
		facility._pipe(Vector3(x,1,-25),Vector3(x,1,-41),.045,steel)
	for z: float in [-28,-40,-46]:
		P.detail(self,"medical_box",.4,Vector3(56,.04,z))
		B.box(self,Vector3(.45,.012,.32),Vector3(57,.025,z+.7),paper).rotation.y = z*.3
	for pair: Array in [["ДИАГНОСТИКА / 04",Vector3(37,2.8,-16.5)],["01  РЕНТГЕН",Vector3(23,2.8,-23.7)],["02  НАБЛЮДЕНИЕ",Vector3(37,2.8,-23.7)],["03  ПРОЦЕДУРНАЯ",Vector3(52,2.8,-23.7)]]:
		facility._sign(pair[0],pair[1],0,23)
	facility._sign("ПОДГОТОВКА  ↑\nСТЕКЛО: КОРОТКИЙ ПУТЬ →",Vector3(43.7,2.3,-38),-PI/2,18)
	facility._sign("АВАРИЙНЫЙ ВЫХОД В АРХИВ",Vector3(15.5,2.8,-5),PI/2,17)
	var wet := M.pbr("old_linoleum_flooring_01","4e6868",.55,.16)
	B.box(self,Vector3(3.5,.012,3),Vector3(48.25,.012,-34.5),wet)
	for i: int in range(10):
		B.box(self,Vector3(.05,.02,.18),Vector3(46.3+i*.4,.025,-32.8),B.material("bfa96a")).rotation.y = .5
	evidence_label = facility._sign("СМЕНА НЕ ЗАКРЫТА",Vector3(17,2.3,-49.65),PI,19)

func prompt(id: String) -> String:
	match id:
		"xray": return "F — изучить рентгеновские экспозиции" if not xray_solved else "Снимок восстановлен · пациент 04"
		"observe": return "F — прочесть карту пациента / протокол"
		"treatment": return "F — открыть медицинский пульт" if not complete else "Блокировка снята"
		"trap": return "F — подать напряжение на мокрый участок" if not trap_armed and not trap_spent else "Разряд использован" if trap_spent else "Под напряжением · обойди мокрый пол"
		"final_reset": return "Удерживай F — сбросить аварийный ввод" if finale_started and not breaker_reset else "Аварийный ввод · резерв готов"
		"exit_crank": return "Удерживай F — открыть тяжёлую заслонку" if breaker_reset else "Ручная заслонка · нужен резервный ввод"
		"supplies": return "F — забрать 8 патронов из шкафа" if not reward_taken else "Шкаф пуст"
	return "F — прочесть / исследовать"

func use_station(id: String) -> void:
	if id=="xray":
		if not game.mission.has_badge: game.hud.announce("Нужен допуск сотрудника.",3); return
		if not xray_solved: screen.open_screen("xray")
		else: game.hud.announce("Пациент 04: сначала охлаждение, затем седативное, после — разблокировка.",7)
	elif id=="observe":
		if not xray_solved: game.hud.announce("Карта закрыта. Восстанови экспозиции в рентгене.",4); return
		observed = true
		_open("diag_loop")
		game.hunters[15].set_active(true)
		game.hud.announce("Пациент 04: гипертермия → охлаждение; затем седативное; только потом снять фиксаторы. К процедурной: северный обход с припасами или стекло справа. Запись добавлена в J.",12)
	elif id=="treatment":
		if not observed: game.hud.announce("Сначала прочти карту в наблюдательной. Нельзя снимать фиксаторы вслепую.",5); return
		if not complete: screen.open_screen("treatment")
	elif id in ["memo","log","office"]:
		if id not in evidence:
			evidence.append(id)
			if id=="office": game.score += 450
		if id=="office" and not office_reward: office_reward = game.inventory.take_supply("medkit")
		game.hud.announce({"memo":"Ирина: «Мою смену стёрли. В журнале переводов осталась строка — ищи в наблюдательной». Расследование: J.","log":"Журнал: Ирина не покидала отделение. Приказ подписан врачом; доступ к его кабинету восстановлен после сверки письма и журнала.","office":"Врач знал о побеге пациента. Ирина вывела людей через служебный вход. Ты забираешь доказательства для её семьи. Финальная запись изменена."}[id],11)
		if "memo" in evidence and "log" in evidence: _open("diag_office")
		evidence_label.text = "ДОКАЗАТЕЛЬСТВА СОХРАНЕНЫ" if "office" in evidence else "СМЕНА НЕ ЗАКРЫТА"
	elif id=="supplies":
		if not reward_taken and game.inventory.take_supply("ammo",8): reward_taken = true; game.sound_bank.play_cue("reload")
	elif id=="trap":
		if not trap_armed and not trap_spent:
			trap_armed = true
			game.emit_noise(stations.trap.position,20)
			game.hud.announce("Мокрый участок под током. Примани противника бутылкой; сам обойди воду.",7)
	elif id in ["final_reset","exit_crank"]:
		if not finale_started or (id=="exit_crank" and not breaker_reset):
			game.hud.announce("Ввод будет доступен после команды терминала карантина.",4); return
		if (id=="final_reset" and breaker_reset) or exit_released: return
		if work_id==id: return
		work_id = id
		work = 0
		game.hero.performing_action = true
		game.hero.action_focus = stations[id].position+Vector3(0,1,0)

func solve_xray() -> void:
	xray_solved = true
	_open("diag_37")
	_open("diag_cross")
	game.hunters[14].set_active(true)
	game.sound_bank.play_cue("core")
	game.emit_noise(stations.xray.position,18)
	game.hud.announce("Снимок восстановлен: пациент 04. Открыта наблюдательная. За стеной послышалось движение.",8)

func complete_treatment() -> void:
	if not observed: return
	complete = true
	_open("diag_52")
	game.sound_bank.play_cue("door")
	game.hud.announce("Медицинская блокировка снята. Вернись к терминалу карантина. Обратный выход процедурной открыт.",8)
	game.save_checkpoint()

func start_finale() -> void:
	finale_started = true
	for id: String in ["quarantine_gate","wing_-9_-9","wing_9_-9"]:
		game.facility.gate_by_id[id].restore_state(false,true)
	_open("final_passage")
	_open("diag_entry")
	_open("final_research_return")
	game.hunters[16].set_active(true)
	game.escape_remaining = float(game.difficulty_value("escape"))
	game.facility.set_exit_ready(false)
	game.hud.announce("Карантинная заслонка упала! Северный аварийный проход открыт. Обойди через диагностику, сбрось ввод в щитовой, затем открой рукоять у главного входа. 3 минуты.",12)
	game.facility.layout_changed.emit()

func on_partition_broken(id: String) -> void:
	if id=="observation_glass" and game!=null:
		game.hunters[15].set_active(true)
		game.hud.announce("Стекло разбито. Короткий проход открыт — шум услышали по обе стороны.",5)

func _open(id: String) -> void:
	facility.gate_by_id[id].restore_state(true,false)
	facility.layout_changed.emit()

func interact_door(door: Node3D) -> bool:
	if door.door_id in ["quarantine_gate","wing_-9_-9","wing_9_-9"] and finale_started:
		game.hud.announce("Заслонка заклинила. Северный аварийный проход за изолятором → диагностика.",6); return true
	if door.lock_kind not in ["diagnosis","protocol","evidence","finale"]: return false
	if not door.locked: return false
	game.hud.announce({"diagnosis":"Сначала восстанови снимок в рентгене. Процедурная открывается с другой стороны.","protocol":"Ознакомься с картой пациента в наблюдательной.","evidence":"Кабинет врача: сопоставь письмо Ирины и журнал переводов.","finale":"Только аварийная эвакуация. Управляется терминалом карантина."}[door.lock_kind],6)
	return true

func update(delta: float) -> void:
	time += delta
	wire_light.light_energy = (1.0+sin(time*43)*.4) if trap_armed and not trap_spent else 0
	if trap_armed and not trap_spent:
		var wet := Rect2(46.5,-36,3.5,3)
		if wet.has_point(Vector2(game.hero.position.x,game.hero.position.z)):
			trap_spent = true
			game._take_hit(game.hunters[15])
		for hunter: CharacterBody3D in game.hunters:
			if hunter.active and wet.has_point(Vector2(hunter.position.x,hunter.position.z)):
				trap_spent = true
				hunter.receive_bullet(stations.trap.position,3,false,hunter.aim_point())
				hunter.stunned_remaining = maxf(hunter.stunned_remaining,4)
		if trap_spent: game.sound_bank.play_spatial("metal_hit",wire_light.global_position)
	if not work_id.is_empty():
		if not Input.is_action_pressed("interact_hold") or game.hero.position.distance_to(stations[work_id].position)>2.1:
			cancel_work(); return
		work += delta
		game.survival_tools.status.text = "УДЕРЖИВАЙ F · %s · %d%%" % ["аварийный ввод" if work_id=="final_reset" else "рукоять заслонки",mini(100,int(work/5*100))]
		if fmod(work,.6)<delta: game.emit_noise(stations[work_id].position,12)
		if work>=5:
			if work_id=="final_reset":
				breaker_reset = true
				game.hud.announce("Резервный ввод включён. Вернись к главному входу и удерживай F у рукояти заслонки.",8)
			else:
				exit_released = true
				game.facility.set_exit_ready(true)
			cancel_work()

func cancel_work() -> void:
	work_id = ""
	work = 0
	game.hero.performing_action = false

func objective() -> Array[String]:
	if finale_started:
		return ["Аварийная эвакуация / " + ("открой главный выход" if breaker_reset else "сбрось ввод в щитовой"),"Северный проход за терминалом → диагностика → исследовательский переход → служебное крыло." if not breaker_reset else "Главный вход в приёмной. Удерживай F у аварийной рукояти."]
	return ["Диагностика / медицинская блокировка","Северная дверь исследовательского перехода. " + ("Восстанови две экспозиции в рентгене." if not xray_solved else "Прочти карту пациента в наблюдательной." if not observed else "Пройди в процедурную через стекло или северную подготовку; введи безопасный протокол.")]

func journal_entries() -> Array[String]:
	var result: Array[String] = []
	if xray_solved: result.append("Диагностика: снимок пациента 04 восстановлен. Наблюдательная открыта.")
	if observed: result.append("Карта 04: гипертермия — охлаждение; возбуждение — седативное; затем разблокировка. Путь в процедурную: северный обход с припасами или разбиваемое стекло справа.")
	if "memo" in evidence: result.append("Ирина: смену стёрли. Сопоставить её письмо с журналом переводов в наблюдательной.")
	if "log" in evidence: result.append("Журнал: Ирина осталась в отделении. Письмо + журнал открывают кабинет врача на северо-западе рентгена.")
	if "office" in evidence: result.append("Доказательства врача найдены. Ирина вывела пациентов через служебный вход. Документы будут переданы семье.")
	return result

func ending() -> String:
	var text: String = "Ты выбрался с доказательствами. Утром семья Ирины узнает, что она осталась, чтобы вывести пациентов." if "office" in evidence else "Снаружи рассвет. В кармане остался пропуск Ирины. Что случилось с её сменой, пока неизвестно."
	return text+game.isolation.ending()

func snapshot() -> Dictionary:
	var broken: Array[String] = []
	for id: String in partitions:
		if partitions[id].broken: broken.append(id)
	return {"xray":xray_solved,"observed":observed,"complete":complete,"evidence":evidence.duplicate(),"reward":reward_taken,"office_reward":office_reward,"broken":broken,"trap_armed":trap_armed,"trap_spent":trap_spent}

static func valid(data: Variant) -> bool:
	if not data is Dictionary: return false
	for key: String in ["xray","observed","complete","reward","trap_armed","trap_spent"]:
		if not data.get(key) is bool: return false
	if data.observed and not data.xray or data.complete and not data.observed: return false
	if data.trap_spent and not data.trap_armed: return false
	for key: String in ["evidence","broken"]:
		if not data.get(key) is Array or data[key].size()>3: return false
		var seen: Array = []
		for id: Variant in data[key]:
			if id in seen or id not in (["memo","log","office"] if key=="evidence" else ["observation_glass","service_sheet"]): return false
			seen.append(id)
	if "office" in data.evidence and ("memo" not in data.evidence or "log" not in data.evidence): return false
	if data.has("office_reward") and (not data.office_reward is bool or data.office_reward and "office" not in data.evidence): return false
	return true

func restore(data: Dictionary) -> void:
	if data.is_empty(): return
	xray_solved = data.xray
	observed = data.observed
	complete = data.complete
	evidence.assign(data.evidence)
	reward_taken = data.reward
	office_reward = data.get("office_reward",false)
	trap_armed = data.trap_armed
	trap_spent = data.trap_spent
	for id: String in partitions: partitions[id].set_broken(id in data.broken)
	for id: String in ["diag_37","diag_cross","diag_loop","diag_52","diag_office","final_passage","final_research_return"]:
		facility.gate_by_id[id].restore_state(false,true)
	if xray_solved: _open("diag_37"); _open("diag_cross")
	if observed: _open("diag_loop")
	if complete: _open("diag_52")
	if "memo" in evidence and "log" in evidence: _open("diag_office")
	evidence_label.text = "ДОКАЗАТЕЛЬСТВА СОХРАНЕНЫ" if "office" in evidence else "СМЕНА НЕ ЗАКРЫТА"

func reset_chapter() -> void:
	if ending_view!=null:
		ending_view.queue_free()
		ending_view = null
	screen.dismiss(false)
	xray_solved = false
	observed = false
	complete = false
	evidence.clear()
	reward_taken = false
	office_reward = false
	finale_started = false
	breaker_reset = false
	exit_released = false
	trap_armed = false
	trap_spent = false
	cancel_work()
	screen.film_offset = Vector2(72,-48)
	screen.film_turn = 1
	screen.treatment_steps.clear()
	for panel: Node3D in partitions.values(): panel.set_broken(false)
	evidence_label.text = "СМЕНА НЕ ЗАКРЫТА"

func show_ending() -> void:
	ending_view = preload("res://scripts/ending_tableau.gd").new()
	game.add_child(ending_view)
	ending_view.present("office" in evidence,game.isolation.proof)
