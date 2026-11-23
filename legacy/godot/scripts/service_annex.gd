extends Node3D
## A new mandatory exploration chapter; progress comes from travel, clues and encounters.
const B = preload("res://scripts/world_builder.gd")
const M = preload("res://scripts/environment_materials.gd")
const P = preload("res://scripts/imported_props.gd")
const Station = preload("res://scripts/isolation_station.gd")
const LOCATIONS := {"brief":Vector3(22,0,-115),"ammo_post":Vector3(41,0,-124),"aid_post":Vector3(26,0,-124),"wheel":Vector3(66,0,-119),"manual":Vector3(21,0,-145),"ammo_work":Vector3(40,0,-145),"intake":Vector3(58,0,-135),"drain":Vector3(98,0,-144),"pump":Vector3(70,0,-147),"dial_a":Vector3(23,0,-156),"dial_b":Vector3(21,0,-168),"dial_c":Vector3(40,0,-170),"boiler":Vector3(39,0,-155),"key":Vector3(61,0,-167),"aid_cold":Vector3(73,0,-171),"register":Vector3(98,0,-119),"files":Vector3(96,0,-168),"ammo_archive":Vector3(83,0,-169),"antenna":Vector3(70,0,-187),"frequency":Vector3(24,0,-185),"radio":Vector3(39,0,-185),"release":Vector3(40,0,-114)}
const LABELS := {"brief":"ДИСПЕТЧЕР / АВАРИЙНЫЙ МАРШРУТ","ammo_post":"ЗАПАС ПОСТА","aid_post":"МЕДИЦИНСКИЙ ШКАФ","wheel":"МАХОВИК / СКЛАД ЗАПЧАСТЕЙ","manual":"ЖУРНАЛ СЛЕСАРЯ","ammo_work":"ИНСТРУМЕНТАЛЬНЫЙ ШКАФ","intake":"НАСОС / ВПУСК","drain":"ПРАЧЕЧНАЯ / ОБРАТНЫЙ СЛИВ","pump":"ПУСК НАСОСА","dial_a":"КОТЁЛ / КОНТУР А","dial_b":"КОТЁЛ / КОНТУР Б","dial_c":"КОТЁЛ / КОНТУР В","boiler":"ПРОВЕРКА ДАВЛЕНИЯ","key":"ПЛОМБА АРХИВА","aid_cold":"РЕЗЕРВ МЕДСЛУЖБЫ","register":"ОПИСЬ / ШКАФ 12-Б","files":"АРХИВ / МАРШРУТ 12-Б","ammo_archive":"ЗАПАС КОНВОЯ","antenna":"РЕЗЕРВ АНТЕННЫ","frequency":"НАСТРОЙКА ЧАСТОТЫ","radio":"АВАРИЙНАЯ РАДИОСТАНЦИЯ","release":"ПОДТВЕРЖДЕНИЕ ЭВАКУАЦИИ"}
const IDS := ["brief","wheel","manual","intake","drain","pump","boiler","key","register","files","antenna","radio","release","ammo_post","ammo_work","ammo_archive","aid_post","aid_cold"]
const FREQUENCIES := ["90.6","101.2","104.8","119.1"]
const REQUIREMENTS := {"wheel":["brief"],"manual":["brief"],"intake":["wheel"],"drain":["wheel"],"pump":["intake","drain","wheel"],"boiler":["pump","manual"],"key":["boiler"],"register":["brief"],"files":["key","register","boiler"],"antenna":["files"],"radio":["antenna","files"],"release":["radio"]}
var game: Node3D
var facility: Node3D
var stations: Dictionary = {}
var gates: Dictionary = {}
var steps: Array[String] = []
var values: Array[int] = [0,0,0]
var frequency: int = 0
var complete: bool = false
var actors: Array[CharacterBody3D] = []
var awakened: Array[int] = []
var work_id: String = ""
var work: float = 0
var status: Label
var pump_wheel: Node3D
var pressure_light: OmniLight3D
var clock: float = 0
var cue_clock: float = 5
var route_distance: float = 0
var last_position: Vector3

func build(world_facility: Node3D) -> void:
	facility = world_facility
	var floor_mat := M.pbr("old_linoleum_flooring_01","88938c",.5)
	var floor_body: Node3D = B.box(self,Vector3(88,.4,84),Vector3(60,-.2,-150),floor_mat,true)
	for child: Node in floor_body.get_children():
		if child is MeshInstance3D: child.hide()
	for x: int in range(16,104,8):
		for z: int in range(-192,-108,8):
			var depth: float = minf(8,-108-z)
			B.box(self,Vector3(8,.025,depth),Vector3(x+4,-.01,z+depth/2),floor_mat)
	B.box(self,Vector3(88,.22,84),Vector3(60,3.72,-150),facility.ceiling_material,true)
	_wall(Vector3(16,0,-150),Vector3(.4,3.6,84))
	_wall(Vector3(104,0,-150),Vector3(.4,3.6,84))
	_wall(Vector3(60,0,-192),Vector3(88,3.6,.4))
	_wall(Vector3(82,0,-108),Vector3(44,3.6,.4))
	_gate("entry",Vector3(37,0,-108),0,"entry",4)
	# Each room has a wide doorway into a continuous central route.
	for x: float in [46,54]:
		var previous: float = -108
		for z: float in [-118,-139,-162,-183]:
			var end: float = z+2
			_wall(Vector3(x,0,(previous+end)/2),Vector3(.35,3.6,previous-end))
			var id: String = "stores" if x==54 and z==-118 else ("w" if x==46 else "e")+str(int(-z))
			_gate(id,Vector3(x,0,z),PI/2,"brief" if id=="stores" else "room",4,id!="stores")
			previous = z-2
		_wall(Vector3(x,0,(previous-192)/2),Vector3(.35,3.6,previous+192))
	for z: float in [-128,-150,-174]:
		_wall(Vector3(31,0,z),Vector3(30,3.6,.35))
		_wall(Vector3(79,0,z),Vector3(50,3.6,.35))
	# East rooms contain a second department; the laundry sluice opens into the archive.
	for pair: Vector2 in [Vector2(-118,20),Vector2(-139,22),Vector2(-162,24),Vector2(-183,18)]:
		var top: float = pair.x+pair.y/2
		var bottom: float = pair.x-pair.y/2
		_wall(Vector3(78,0,(top+pair.x+2)/2),Vector3(.35,3.6,top-pair.x-2))
		_wall(Vector3(78,0,(bottom+pair.x-2)/2),Vector3(.35,3.6,pair.x-2-bottom))
		_gate("east"+str(int(-pair.x)),Vector3(78,0,pair.x),PI/2,"archive" if pair.x==-162 else "room",4,pair.x!=-162)
	_gate("steam",Vector3(50,0,-150),0,"steam",7.5)
	for id: String in LOCATIONS: _station(id,LOCATIONS[id])
	preload("res://scripts/service_annex_art.gd").build(self)

func _wall(at: Vector3,size: Vector3) -> void: facility._wall(at,size)
func _gate(id: String,at: Vector3,yaw: float,kind: String,width: float,opened: bool = false) -> void:
	facility._add_door("annex_"+id,at,yaw,"annex_"+kind,width,opened)
	gates[id] = facility.gate_by_id["annex_"+id]
func _station(id: String,at: Vector3) -> void:
	var station := Station.new()
	station.chapter = self; station.station_id = id; station.caption = LABELS[id]; station.position = at
	add_child(station); stations[id] = station
	station.label.text = LABELS[id]; station.label.font_size = 13; station.label.position = Vector3(0,1.3,.30)
	P.body(station,Vector3(.95,.8,.62),Vector3(0,.4,0))
	if id in ["wheel","manual","key","register","files","ammo_post","ammo_work","ammo_archive","aid_post","aid_cold"]:
		P.visual(station,"painted_wooden_nightstand",Vector3(.95,.8,.62))
		station.drawer = B.box(station,Vector3(.8,.20,.045),Vector3(0,.52,.34),preload("res://scripts/wood_surface.gd").make())
		station.contents = Node3D.new(); station.drawer.add_child(station.contents); station.contents.position = Vector3(0,.10,-.10)
		if id.begins_with("ammo_") or id.begins_with("aid_"):
			preload("res://scripts/loot_visual.gd").build(station.contents,id,"ammo" if id.begins_with("ammo_") else "medkit")
		elif id=="wheel":
			for i: int in range(6):
				var spoke := B.box(station.contents,Vector3(.04,.025,.36),Vector3.ZERO,facility.metal_material)
				spoke.rotation.y = i*PI/3
		else: B.box(station.contents,Vector3(.24,.008,.30),Vector3.ZERO,B.material("c6baa0"))
		station.contents.hide()
	else:
		B.box(station,Vector3(.95,.8,.62),Vector3(0,.4,0),M.surface("metal","596d68",1.5,.6))
		if id.begins_with("dial_") or id in ["intake","drain"]:
			var wheel := Node3D.new(); station.add_child(wheel); wheel.position = Vector3(0,.72,.38)
			for i: int in range(6): B.box(wheel,Vector3(.035,.37,.04),Vector3.ZERO,facility.alarm_material).rotation.z = i*PI/3
		else:
			B.box(station,Vector3(.63,.30,.035),Vector3(0,.64,.33),B.material("243f3b",.15))
		if id in ["radio","frequency"]: P.detail(station,"portable_cassette_player",.42,Vector3(0,.82,0))

func configure(owner_game: Node3D) -> void:
	game = owner_game
	for at: Vector3 in [Vector3(28,.1,-139),Vector3(28,.1,-163),Vector3(94,.1,-163),Vector3(50,.1,-172)]:
		var actor := preload("res://scripts/stalker.gd").new()
		actor.position = at; actor.crawler = actors.size() in [0,3]
		actor.blind_hunter = actors.size()==2; actor.use_minimax = actors.size()==2
		actor.max_health = 6
		game.get_node("World").add_child(actor)
		actor.patrol_points.assign([at,at+Vector3(0,0,5)])
		game.hunters.append(actor); actors.append(actor)
	actors[3].can_spit = true; actors[3].ranged_attack.connect(game._spawn_projectile)
	status = Label.new(); status.hide(); game.hud.staging.add_child(status)
	last_position = game.hero.position
	sync()

func done(id: String) -> bool:
	if id.begins_with("dial_"): return "boiler" in steps
	if id=="frequency": return frequency==2
	return id in steps
func prompt(id: String) -> String:
	if done(id): return "Маршрут подтверждён" if id=="release" else "Выполнено / "+stations[id].caption
	if stations[id].drawer!=null and not stations[id].opened: return "F — открыть шкаф · "+stations[id].caption
	if id.begins_with("dial_"): return "F — повернуть контур / "+str(values[["dial_a","dial_b","dial_c"].find(id)])+" / 0–3"
	if id=="frequency": return "F — настроить волну / "+FREQUENCIES[frequency]+" МГц"
	if id in ["wheel","manual","key","register","files","intake","drain","pump","antenna","radio"]: return "Удерживай F — "+stations[id].caption
	return "F — "+stations[id].caption

func use_station(id: String) -> void:
	if not game.isolation.complete:
		game.hud.announce("Хозяйственный корпус открывается после снятия блокировки изолятора 17.",5); return
	if done(id) and id!="frequency": return
	if id.begins_with("dial_"):
		if "pump" not in steps or "manual" not in steps: game.hud.announce("Сначала восстанови насос и прочти журнал слесаря.",5); return
		var index: int = ["dial_a","dial_b","dial_c"].find(id)
		values[index] = (values[index]+1)%4
		stations[id].label.text = LABELS[id]+" / "+str(values[index]); game.sound_bank.play_cue("reload",.5); return
	if id=="frequency":
		if "radio" in steps: return
		frequency = (frequency+1)%4
		stations[id].label.text = LABELS[id]+" / "+FREQUENCIES[frequency]; game.sound_bank.play_cue("reload",.5); return
	for prerequisite: String in REQUIREMENTS.get(id,[]):
		if prerequisite not in steps: game.hud.announce(objective()[1],6); return
	if id=="boiler":
		if values!=[2,1,3]: game.hud.announce("Давление не сбалансировано. Журнал: контур А — 2, Б — 1, В — 3.",6); game.emit_noise(stations[id].position,18); return
	if id=="radio" and frequency!=2: game.hud.announce("Нет ответа. В маршрутном деле указана дежурная волна 104.8 МГц.",6); return
	if id.begins_with("ammo_") or id.begins_with("aid_"):
		if not game.inventory.take_supply("ammo" if id.begins_with("ammo_") else "medkit",12 if id.begins_with("ammo_") else 1): return
		_finish(id); return
	if id=="wheel" and not game.inventory.can_take("annex_wheel"): return
	if id in ["brief","boiler","release"]: _finish(id); return
	if game.hero.performing_action or not work_id.is_empty(): return
	work_id = id; work = 0; game.hero.performing_action = true
	game.hero.action_focus = stations[id].position+Vector3(0,.9,.3)
	if id=="radio": _wake(3); game.emit_noise(stations[id].position,36)

func _finish(id: String) -> void:
	if id in steps: return
	steps.append(id); stations[id].used = true
	if stations[id].contents!=null: stations[id].contents.hide()
	match id:
		"brief": game.hud.announce("Связь снаружи оборвана. Склад запчастей открыт: найди маховик насоса. Журнал слесаря — в западной мастерской.",10)
		"wheel": _wake(0); game.hud.announce("Маховик найден. Перекрой впуск в насосной и обратный слив в прачечной. Шаги слышны из мастерской.",8)
		"manual": game.hud.announce("Журнал слесаря: после запуска насоса выставить А — 2, Б — 1, В — 3. Запись сохранена в J.",9)
		"intake","drain": game.sound_bank.play_spatial("isolation_pipe",stations[id].position)
		"pump": _wake(1); game.hud.announce("Пар рассеялся. Северная галерея открыта; котельная за западной дверью. Маховик установлен в насос.",8)
		"boiler": game.hud.announce("Температура выровнена. Забери пломбу допуска в холодном складе. Опись шкафов осталась в юго-восточной кладовой.",9)
		"key": game.hud.announce("Архивный допуск снят с пломбы. Для поиска нужного дела нужна опись из юго-восточной кладовой.",7)
		"register": game.hud.announce("Опись: маршрут санитарного выхода хранится в шкафу 12-Б северо-восточного архива. Дежурная волна указана внутри дела.",9)
		"files": _wake(2); game.hud.announce("Маршрут найден. Волна 104.8 МГц. Включи резерв антенны в дальней северной аппаратной, затем передай сигнал из диспетчерской напротив.",10)
		"antenna": game.hud.announce("Антенна получила питание. Диспетчерская — через северный коридор на запад. Частота 104.8, затем удерживай F у передатчика.",8)
		"radio": game.hud.announce("Ответ снаружи: «Маршрут принят. Подтвердите эвакуацию на южном посту». Прямой обратный проход открыт.",9)
		"release": complete = true; game.score += 500; game.hud.announce("Хозяйственный корпус пройден. Аварийный маршрут подтверждён. Вернись к терминалу карантина и начни финальную эвакуацию.",10)
	game.sound_bank.play_cue("core",.65); sync(); game.mission.refresh_hud()
	if id in ["brief","boiler","files","release"]: game.save_checkpoint()

func _wake(index: int) -> void:
	if index in awakened: return
	awakened.append(index)
	if not actors[index].dead: actors[index].set_active(true)

func sync(keep_doors: bool = false) -> void:
	var entry_allowed: bool = game!=null and game.isolation.complete
	for id: String in ["entry","stores","steam","east162","w183"]:
		var allowed: bool = entry_allowed if id=="entry" else "brief" in steps if id=="stores" else "pump" in steps if id=="steam" else "key" in steps if id=="east162" else "files" in steps
		gates[id].restore_state((gates[id].is_open if keep_doors else true) and allowed,not allowed)
	if pressure_light!=null: pressure_light.light_color = Color("90b7ad") if "pump" in steps else Color("d86e3b")
	facility.layout_changed.emit()

func interact_door(door: Node3D) -> bool:
	if not door.door_id.begins_with("annex_"): return false
	if not door.locked: return false
	game.hud.announce("Хозяйственный корпус: "+objective()[1],7); return true

func update(delta: float) -> void:
	clock += delta
	if gates.entry.locked and game.isolation.complete: sync()
	if game.hero.position.z<-108:
		if last_position.z<-108 and last_position.distance_to(game.hero.position)<3: route_distance += last_position.distance_to(game.hero.position)
		cue_clock -= delta
		if cue_clock<=0:
			game.sound_bank.play_spatial("isolation_pipe",Vector3(50,2.8,-160),.6); cue_clock = 15
	last_position = game.hero.position
	if pump_wheel!=null: pump_wheel.rotation.z += delta*(3 if "pump" in steps else .15)
	if pressure_light!=null: pressure_light.light_energy = .7 if "pump" in steps else .8+sin(clock*4)*.25
	status.text = ""
	if work_id.is_empty(): return
	if not Input.is_action_pressed("interact_hold") or game.hero.position.distance_to(stations[work_id].position)>2.1:
		cancel_work(); return
	work += delta
	var duration: float = 6 if work_id=="radio" else 5 if work_id=="files" else 4 if work_id in ["pump","antenna"] else 2.5
	status.text = "УДЕРЖИВАЙ F / "+stations[work_id].caption+" / %d%%"%mini(100,int(work/duration*100))
	if work_id in ["pump","antenna","radio"] and fmod(work,.8)<delta: game.emit_noise(stations[work_id].position,24)
	if work>=duration:
		var id: String = work_id
		cancel_work(); _finish(id)

func cancel_work() -> void:
	if not work_id.is_empty() and game!=null: game.hero.performing_action = false
	work_id = ""; work = 0
	if status!=null: status.text = ""
func goal_id() -> String:
	for id: String in ["brief","wheel","manual","intake","drain","pump"]:
		if id not in steps: return id
	if "boiler" not in steps:
		for i: int in range(3):
			if values[i]!=[2,1,3][i]: return ["dial_a","dial_b","dial_c"][i]
		return "boiler"
	for id: String in ["key","register","files","antenna"]:
		if id not in steps: return id
	if frequency!=2 and "radio" not in steps: return "frequency"
	return "radio" if "radio" not in steps else "release"
func objective() -> Array[String]:
	var hints := {"brief":"Северная дверь машинного зала изолятора 17. Прочти маршрутный лист на южном посту.","wheel":"Восточная дверь южного поста → склад запчастей. Открой шкаф и удерживай F.","manual":"Западная мастерская, второй ряд комнат. Журнал в шкафу у дальней стены.","intake":"Насосная: второй ряд справа от центральной галереи. Удерживай F у впуска.","drain":"Из насосной пройди на восток в прачечную. Удерживай F у обратного слива.","pump":"Вернись к стартеру насоса у северной стены насосной. Удерживай F.","dial_a":"Котельная: третий ряд слева. Контур А должен показывать 2.","dial_b":"Котельная: контур Б должен показывать 1.","dial_c":"Котельная: контур В должен показывать 3.","boiler":"Все три контура выставлены. Подтверди баланс у котлового пульта.","key":"Холодный склад: третий ряд справа. Обыщи шкаф с пломбой архива.","register":"Опись шкафов: юго-восточная кладовая, за складом запчастей. Вернись по галерее.","files":"Из холодного склада пройди на восток в архив. Шкаф 12-Б у северной стены.","antenna":"Северная аппаратная: последний ряд справа. Включи резерв антенны.","frequency":"Диспетчерская: последний ряд слева. Выстави 104.8 МГц на приёмнике.","radio":"Удерживай F у передатчика в диспетчерской. Шум привлечёт охранника галереи.","release":"Вернись по центральной галерее на южный пост и подтверди маршрут эвакуации."}
	return ["Хозяйственный корпус / связь снаружи",hints[goal_id()]]
func journal_entries() -> Array[String]:
	var result: Array[String] = []
	if "brief" in steps: result.append("Хозяйственный корпус: склад → мастерская → впуск насосной и слив прачечной → насос → котельная → холодный склад → архив → антенна → диспетчерская → южный пост.")
	if "manual" in steps: result.append("Журнал слесаря: контуры котла А — 2, Б — 1, В — 3. Пуск только после восстановления насоса.")
	if "register" in steps: result.append("Архивная опись: шкаф 12-Б. Допуск хранится под пломбой в холодном складе.")
	if "files" in steps: result.append("Маршрут 12-Б: санитарный выход ведёт наружу. Дежурная волна 104.8 МГц; резерв антенны в северной аппаратной.")
	if "radio" in steps: result.append("Диспетчер ответил снаружи. Вернуться на южный пост, подтвердить маршрут и запустить эвакуацию у терминала карантина.")
	return result

func snapshot() -> Dictionary:
	var positions: Array = []
	for actor: CharacterBody3D in actors: positions.append([actor.position.x,actor.position.y,actor.position.z])
	var opened: Array[String] = []
	for id: String in stations:
		if stations[id].opened: opened.append(id)
	return {"steps":steps.duplicate(),"values":values.duplicate(),"frequency":frequency,"complete":complete,"awakened":awakened.duplicate(),"opened":opened,"positions":positions,"distance":route_distance}
static func valid(data: Variant) -> bool:
	if not data is Dictionary or not data.get("steps") is Array or data.steps.size()>IDS.size(): return false
	var seen: Array = []
	for id: Variant in data.steps:
		if id not in IDS or id in seen: return false
		seen.append(id)
	for id: String in REQUIREMENTS:
		if id not in seen: continue
		for prerequisite: String in REQUIREMENTS[id]:
			if prerequisite not in seen: return false
	if not data.get("values") is Array or data["values"].size()!=3: return false
	for value: Variant in data["values"]:
		if not preload("res://scripts/field_inventory.gd")._integer(value,0,3): return false
	if not preload("res://scripts/field_inventory.gd")._integer(data.get("frequency"),0,3): return false
	if "boiler" in seen:
		for i: int in range(3):
			if int(data["values"][i])!=[2,1,3][i]: return false
	if "radio" in seen and data.frequency!=2: return false
	if not data.get("complete") is bool or data.complete!=("release" in seen): return false
	if not data.get("awakened") is Array or data.awakened.size()>4: return false
	var unique: Array = []
	for index: Variant in data.awakened:
		if not preload("res://scripts/field_inventory.gd")._integer(index,0,3) or index in unique: return false
		unique.append(index)
	if not data.get("opened") is Array or data.opened.size()>LOCATIONS.size(): return false
	unique.clear()
	for id: Variant in data.opened:
		if id not in LOCATIONS or id in unique: return false
		unique.append(id)
	if not data.get("positions") is Array or data.positions.size()!=4: return false
	for at: Variant in data.positions:
		if not at is Array or at.size()!=3: return false
		for n: Variant in at:
			if not (n is int or n is float) or not is_finite(float(n)): return false
		if at[0]<-30 or at[0]>104 or at[1]<-5.2 or at[1]>4 or at[2]<-192 or at[2]>82: return false
	var distance: Variant = data.get("distance")
	return (distance is int or distance is float) and is_finite(float(distance)) and distance>=0 and distance<1000000
func restore(data: Dictionary) -> void:
	if data.is_empty(): sync(); return
	steps.assign(data.steps); values.assign(data["values"]); frequency = int(data.frequency); complete = data.complete
	awakened.assign(data.awakened); route_distance = float(data.distance)
	for i: int in range(4): actors[i].position = Vector3(data.positions[i][0],data.positions[i][1],data.positions[i][2])
	for id: String in stations:
		var station: Node3D = stations[id]
		station.used = done(id); station.opened = id in data.opened
		if station.drawer!=null: station.drawer.position.z = .60 if station.opened else .34
		if station.contents!=null: station.contents.visible = station.opened and not done(id)
	for i: int in range(3): stations[["dial_a","dial_b","dial_c"][i]].label.text = LABELS[["dial_a","dial_b","dial_c"][i]]+" / "+str(values[i])
	stations.frequency.label.text = LABELS.frequency+" / "+FREQUENCIES[frequency]
	sync(true)
func reset_chapter() -> void:
	cancel_work(); steps.clear(); values.assign([0,0,0]); frequency = 0; complete = false; awakened.clear(); route_distance = 0
	for station: Node3D in stations.values(): station.reset_station(); station.label.text = LABELS[station.station_id]
	sync()
