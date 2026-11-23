extends Node
## Ordered mission state and one-shot events; optional records never gate escape.

const MissionProp = preload("res://scripts/mission_object.gd")
const LOCATIONS: Dictionary = {
	"map": Vector3(-6, 0, 12), "fuse": Vector3(11, 0, 2),
	"power": Vector3(-26, 0, -5), "badge": Vector3(12, 0, -3),
	"terminal": Vector3(-6, 0, -13),
	"weapon": Vector3(-3.8, 0, 12),
	"relay_morgue": Vector3(23, 0, 28), "relay_lab": Vector3(37, 0, 28), "relay_theatre": Vector3(52, 0, 28),
	"ammo_reception": Vector3(3.8, 0, 12), "ammo_archive": Vector3(7, 0, -3),
	"ammo_morgue": Vector3(27, 0, -5), "ammo_lab": Vector3(41, 0, -5), "ammo_theatre": Vector3(57, 0, -5),
	"coolant": Vector3(39, -5, 77), "fuel": Vector3(57, -5, 57), "reactor": Vector3(57, -5, 77),
	"ammo_lower_entry": Vector3(38, -5, 48), "ammo_pump": Vector3(44, -5, 75), "ammo_reactor": Vector3(58, -5, 73),
	"purge_valve_a": Vector3(52, -5, 75), "purge_valve_b": Vector3(58, -5, 66),
	"residence_panel": Vector3(82,0,-12), "residence_prybar": Vector3(99,0,28),
	"residence_core": Vector3(70,0,4), "residence_lift": Vector3(94,0,26),
	"ammo_residence": Vector3(88,0,28),
	"residence_plate": Vector3(102.5,0,22),
}
const CIRCUITS: Dictionary = {"morgue": [1, 3, 2], "lab": [2, 1, 3], "theatre": [3, 2, 1]}
var game: Node3D
var objects: Dictionary = {}
var has_map: bool = false
var has_fuse: bool = false
var power_on: bool = false
var has_badge: bool = false
var exit_started: bool = false
var events: Dictionary = {}
var relays: Array[String] = []
var has_coolant: bool = false
var has_fuel: bool = false
var purge_started: bool = false
var purge_complete: bool = false
var purge_remaining: float = 45.0
var purge_fault: String = ""
var serviced_valves: Array[String] = []


func configure(owner_game: Node3D, world: Node3D) -> void:
	game = owner_game
	for id: String in LOCATIONS:
		var object := MissionProp.new()
		object.object_id = id
		object.position = LOCATIONS[id]
		world.add_child(object)
		objects[id] = object
	for department: String in CIRCUITS:
		var x: float = {"morgue": 23.0, "lab": 37.0, "theatre": 52.0}[department]
		for i: int in range(3):
			var dial := MissionProp.new()
			dial.object_id = "dial_%s_%d" % [department, i]
			dial.position = Vector3(x + (1 - i) * 1.7, 0, 24)
			dial.rotation.y = PI
			world.add_child(dial)
			objects[dial.object_id] = dial
	for id: String in ["relay_morgue", "relay_lab", "relay_theatre"]:
		objects[id].rotation.y = PI
	refresh_hud()


func interact(object: Node3D) -> void:
	if object.used:
		return
	if object.object_id.begins_with("residence_"):
		game.residential.interact(object)
		return
	if object.object_id.begins_with("purge_valve"):
		if purge_fault != object.object_id: return
		serviced_valves.append(object.object_id)
		purge_fault = ""
		object.mark_used()
		game.emit_noise(object.global_position,20)
		game.sound_bank.play_cue("door")
		game.hud.announce("Давление сброшено. Очистка продолжается.",4)
		return
	if object.object_id.begins_with("dial_"):
		if not has_badge:
			return
		var department: String = object.object_id.split("_")[1]
		if department in relays:
			game.hud.announce("Схема уже зафиксирована.", 3)
			return
		object.cycle_dial()
		game.sound_bank.play_cue("reload", 0.8)
		return
	if object.object_id.begins_with("relay_"):
		if not has_badge or not power_on:
			return
		var department: String = object.object_id.trim_prefix("relay_")
		for i: int in range(3):
			if objects["dial_%s_%d" % [department, i]].state_value != CIRCUITS[department][i]:
				game.hud.announce("Схема не замкнута. Настрой три цепи по надписи на стене рядом с узлом.", 6)
				game.sound_bank.play_cue("empty")
				return
		if department == "theatre" and not game.hunters[5].dead:
			game.hud.announce("Охранный узел заблокирован. Уничтожь хранителя операционной. Вымани рывок и стреляй в открытую грудь; патроны у входа.", 7)
			return
		relays.append(department)
		if department == "morgue":
			game.hunters[6].set_active(true)
		elif department == "lab":
			game.hunters[7].set_active(true)
		object.mark_used()
		game.sound_bank.play_cue("core")
		game.emit_noise(object.global_position, 20)
		game.hud.announce("Аварийный узел включён: %d / 3. %s" % [relays.size(), "Открой лестницу за операционной и спустись в нижний сектор." if relays.size() == 3 else "Продолжай в соседнее отделение."], 7)
		game.save_checkpoint()
		return
	if object.object_id.begins_with("ammo_"):
		var supply_amount: int = int(game.difficulty_value("ammo"))
		if game.combat.reserve >= 72:
			game.hud.announce("Запас патронов полон. Оставь коробку здесь.", 3)
			return
		if not game.inventory.can_take("ammo",mini(supply_amount,72-game.combat.reserve)): return
		game.combat.reserve = mini(72, game.combat.reserve + supply_amount)
		object.mark_used()
		game.sound_bank.play_cue("reload")
		game.hud.announce("+%d патронов 9 мм." % supply_amount, 3)
		return
	match object.object_id:
		"coolant":
			if relays.size() < 3:
				return
			if not game.inventory.can_take("coolant"): return
			has_coolant = true
			game.hud.announce("Хладагент найден. Топливо в ремонтной зоне справа от нижнего коридора.", 7)
		"fuel":
			if relays.size() < 3:
				return
			if not game.inventory.can_take("fuel"): return
			has_fuel = true
			game.hud.announce("Топливо найдено. Реактор в дальней правой комнате. Нужен также хладагент из насосной слева.", 7)
		"reactor":
			if not has_coolant or not has_fuel:
				game.hud.announce("Нужны хладагент из насосной и топливо из ремонтной зоны нижнего сектора.", 6)
				return
			if not purge_started:
				purge_started = true
				game.inventory.supplies_installed = true
				for index: int in [8, 9, 10]:
					game.hunters[index].set_active(true)
				game.emit_noise(object.global_position, 55)
				game.sound_bank.play_cue("alarm")
				game.hud.announce("Очистка запущена. Следи за давлением: при аварии найди красный вентиль и удерживай F. Шум привлёк обитателей сектора.", 9)
			return
		"weapon":
			if not game.inventory.can_take("weapon"): return
			game.combat.acquire()
		"map":
			has_map = true
			game.hud.announce("План отделения найден. Tab — карта. Щиток в служебном крыле слева.", 7)
		"fuse":
			if not game.inventory.can_take("fuse"): return
			has_fuse = true
			fire_event("first_contact")
			game.hud.announce("Предохранитель найден. Отнеси его в электрощитовую. Кажется, ты здесь не один.", 7)
		"power":
			if not has_fuse:
				game.hud.announce("В щитке нет предохранителя. Запасной оставили в палате 05 справа.", 6)
				return
			power_on = true
			has_fuse = false
			game.facility.apply_power()
			game.alarm_stage = 2
			fire_event("observation_window")
			game.hud.announce("Питание восстановлено. Архив открыт. Найди пропуск сотрудника; выход управляется из карантина.", 8)
		"badge":
			if not power_on:
				game.hud.announce("Сначала нужно восстановить питание отделения.", 4)
				return
			if not game.inventory.can_take("badge"): return
			has_badge = true
			game.hud.announce("Пропуск найден. Карантин требует код начальника смены. X — осмотреть пропуск, ЛКМ — вращать. Проверь оборот.", 9)
		"terminal":
			if not power_on or not has_badge:
				game.hud.announce("Терминалу нужны питание и пропуск сотрудника.", 5)
				return
			if relays.size() < 3:
				game.hud.announce("Не хватает трёх аварийных узлов. Исследовательское крыло — через дверь справа от коридора карантина.", 7)
				return
			exit_started = true
			if not purge_complete:
				exit_started = false
				game.hud.announce("Спустись по лестнице за операционной и запусти очистку реактора.", 7)
				return
			if not game.residential.complete:
				exit_started = false
				game.hud.announce("Механическая блокировка держит выход. После очистки пройди в жилой корпус: вход в конце коридора справа, за операционной. Сними блокировку лифта.",8)
				return
			if not game.diagnostics.complete:
				exit_started = false
				game.hud.announce("Медицинская блокировка активна. Диагностика — северная дверь исследовательского перехода: рентген, наблюдательная, процедурная.",9)
				return
			if not game.isolation.complete:
				exit_started = false
				game.hud.announce("Изолятор17 держит общий выход. Северная дверь процедурной: найди браслет, запусти регистратор и сними блокировку на посту охраны.",9)
				return
			if not game.annex.complete:
				exit_started = false
				game.hud.announce("Аварийный маршрут ещё не подтверждён. Через северную дверь машинного зала изолятора пройди в хозяйственный корпус и свяжись с диспетчером.",9)
				return
			# The powered terminal provides one emergency flash for the return.
			game.flash_charges = maxi(1, game.flash_charges)
			game.escape_remaining = 180.0
			game.alarm_stage = 3
			game.facility.set_alarm(3)
			game.diagnostics.start_finale()
			game.emit_noise(object.global_position, 45)
			game.sound_bank.play_cue("alarm")
	object.mark_used()
	game.score += 100
	game.sound_bank.play_cue("core", 0.75)
	refresh_hud()
	if object.object_id in ["power", "badge"] and game.save_checkpoint():
		game.hud.story_label.text += " Контрольная точка сохранена."


func update(delta: float = 0.0) -> void:
	if purge_started and not purge_complete:
		if purge_fault.is_empty() and game.hero.hiding_bed == null and game.hero.global_position.distance_to(objects["reactor"].global_position) <= 12:
			purge_remaining = maxf(0, purge_remaining - delta)
		for event: Array in [[30.0,"purge_valve_a"],[15.0,"purge_valve_b"]]:
			if purge_remaining<=event[0] and event[1] not in serviced_valves and purge_fault.is_empty():
				purge_fault = event[1]
				purge_remaining = maxf(purge_remaining,float(event[0]))
				objects[purge_fault].set_fault(true)
				game.sound_bank.play_cue("alarm")
				game.emit_noise(objects[purge_fault].global_position,24)
				game.hud.announce("Аварийное давление! %s. Подойди и удерживай F, чтобы открыть вентиль." % ("Вентиль А слева от реактора" if purge_fault.ends_with("a") else "Вентиль Б в ремонтной зоне, у правой стены"),8)
		if purge_remaining <= 0 and purge_fault.is_empty() and serviced_valves.size()==2:
			purge_complete = true
			objects["reactor"].mark_used()
			game.save_checkpoint()
			game.hud.announce("Очистка завершена. Нужен сброс механической блокировки. Поднимись и пройди в жилой корпус: дальняя правая дверь исследовательского коридора.", 9)
	if has_badge and game.hero.position.x > 15 and game.hero.position.x < 60 and game.hero.position.z > 9 and game.hero.position.z < 32 and game.hero.position.y > -1:
		var department: String = "morgue" if game.hero.position.x < 30 else ("lab" if game.hero.position.x < 44 else "theatre")
		var id: String = "encounter_" + department
		if not events.has(id):
			events[id] = true
			var index: int = {"morgue": 2, "lab": 3, "theatre": 4}[department]
			game.hunters[index].set_active(true)
			if department == "theatre":
				game.hunters[5].set_active(true)
			game.emit_noise(game.hero.position, 22)
			game.sound_bank.play_cue("sting")
	if exit_started and game.hero.position.z > 8 and absf(game.hero.position.x) < 14.5 and not events.has("changed_reception"):
		fire_event("changed_reception")
	refresh_hud()


func fire_event(id: String) -> void:
	if events.has(id):
		return
	events[id] = true
	match id:
		"first_contact":
			game.facility.brief_blackout()
			game.hunters[0].set_active(true)
			game.emit_noise(objects["fuse"].global_position, 18)
			game.sound_bank.play_cue("door", 0.65)
		"observation_window":
			game.hunters[1].set_active(true)
			game.emit_noise(objects["power"].global_position, 25)
			game.sound_bank.play_spatial("growl", Vector3(-26, 0, -8.7))
		"changed_reception":
			game.facility.play_return_event()
			game.sound_bank.play_spatial("door", Vector3(5.5, 0, 12))
			game.hud.announce("В приёмной кто-то был. Выход рядом — не задерживайся.", 5)


func refresh_hud() -> void:
	var title: String
	var hint: String
	if exit_started:
		var objective: Array[String] = game.diagnostics.objective()
		title = objective[0]
		hint = objective[1]
	elif purge_complete and not game.residential.complete:
		var goal: String = game.residential.goal_id()
		title = "Жилой корпус / сброс блокировки выхода"
		hint = {"residence_panel":"Вход: дальний правый конец исследовательского коридора. Щиток у входа в корпус.","residence_prybar":"Питание востока включено. Обыщи тумбу дальней восточной спальни, за наблюдательной клеткой.","res_west_service":"Открой служебный засов изнутри мастерской, затем переключи питание на восток.","residence_core":"Запад обесточен. Через служебный обход вернись в прачечную; V или выстрел по креплению, затем удерживай F.","residence_plate":"Ручной стопор у восточной стены дальней спальни. Удерживай F; привод шумит.","residence_lift":"Стопор зафиксирован. Удерживай F у лифтового щитка в дальней восточной спальне."}[goal]
		if goal=="residence_panel":
			if not game.residential.has_prybar: hint = "У входного щитка включи ВОСТОК. Инструмент спрятан в тумбе дальней восточной спальни."
			elif not game.residential.service_opened: hint = "Инструмент найден. Включи ЗАПАД и пройди в дальнюю мастерскую к ручному засову."
			else: hint = "Включи ВОСТОК, чтобы обесточить запад. К модулю возвращайся через ручной обход."

	elif purge_complete and not game.diagnostics.complete:
		var objective: Array[String] = game.diagnostics.objective()
		title = objective[0]
		hint = objective[1]
	elif purge_complete and not game.isolation.complete:
		var objective: Array[String] = game.isolation.objective()
		title = objective[0]
		hint = objective[1]
	elif purge_complete and not game.annex.complete:
		var objective: Array[String] = game.annex.objective()
		title = objective[0]
		hint = objective[1]
	elif purge_complete:
		title = "Вернись к терминалу карантина"
		hint = "Аварийный проход в западной мастерской → операционная → терминал в изоляторе слева."
	elif purge_started:
		title = "Очистка: %02d с" % ceili(purge_remaining)
		hint = "Защищай пост. Дальше 12 метров от реактора очистка приостанавливается."
		if not purge_fault.is_empty():
			title = "АВАРИЙНОЕ ДАВЛЕНИЕ / ВЕНТИЛЬ " + ("А" if purge_fault.ends_with("a") else "Б")
			hint = "А: слева от реактора. Б: правая стена ремонтной зоны. Удерживай F у красного вентиля."
	elif relays.size() == 3:
		title = "Запусти очистку нижнего сектора"
		hint = "Лестница за операционной. Хладагент в насосной слева, топливо в ремонте справа."
	elif has_badge and not game.inventory.quarantine_code and not game.facility.gate_by_id["quarantine_gate"].is_open:
		title = "Открой кодовый замок карантина"
		hint = "X — осмотреть пропуск. Поверни его: код на обороте. F у двери карантина — клавиатура замка."
	elif has_badge and relays.size() < 3:
		title = "Включи аварийные узлы: %d / 3" % relays.size()
		hint = "Карантин → дверь справа → морг, лаборатория, операционная. Схемы на стенах у узлов."
	elif has_badge:
		title = "Запусти аварийный выход"
		hint = "Приложи пропуск к двери карантина. Терминал в изоляторе слева."
	elif power_on:
		title = "Найди пропуск сотрудника"
		hint = "Архив справа, за обесточенными прежде дверями."
	elif has_fuse:
		title = "Восстанови питание отделения"
		hint = "Электрощитовая в служебном крыле слева от палат."
	else:
		title = "Вооружись и найди запасной предохранитель"
		hint = "Пистолет слева от стойки приёмной. Предохранитель — в палате 05 справа."
	var inventory: String = "План: %s   ·   Предохранитель: %s   ·   Пропуск: %s" % [
		"есть" if has_map else "нет", "есть" if has_fuse else ("установлен" if power_on else "нет"), "есть" if has_badge else "нет"]
	game.hud.set_mission(title, hint, inventory, game.recovered_count, game.cores.size())


func reset_mission() -> void:
	has_coolant = false
	has_fuel = false
	purge_started = false
	purge_complete = false
	purge_remaining = 45
	purge_fault = ""
	serviced_valves.clear()
	relays.clear()
	has_map = false
	has_fuse = false
	power_on = false
	has_badge = false
	exit_started = false
	events.clear()
	for object: Node3D in objects.values():
		object.reset_object()
	refresh_hud()
