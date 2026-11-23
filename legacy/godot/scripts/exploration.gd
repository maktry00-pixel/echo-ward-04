extends RefCounted
## Authored room boundaries; visitation and remaining loot are separate facts.
const ROOMS: Array[Dictionary] = [
	{"id":"annex_post","name":"Южный пост","rect":Rect2(16,-128,30,20),"level":0},
	{"id":"annex_stores","name":"Запчасти","rect":Rect2(54,-128,24,20),"level":0},
	{"id":"annex_register","name":"Кладовая / опись","rect":Rect2(78,-128,26,20),"level":0},
	{"id":"annex_workshop","name":"Слесарная","rect":Rect2(16,-150,30,22),"level":0},
	{"id":"annex_pump","name":"Насосный узел","rect":Rect2(54,-150,24,22),"level":0},
	{"id":"annex_laundry","name":"Санитарная прачечная","rect":Rect2(78,-150,26,22),"level":0},
	{"id":"annex_boiler","name":"Котельная","rect":Rect2(16,-174,30,24),"level":0},
	{"id":"annex_cold","name":"Холодный склад","rect":Rect2(54,-174,24,24),"level":0},
	{"id":"annex_archive","name":"Архив маршрутов","rect":Rect2(78,-174,26,24),"level":0},
	{"id":"annex_radio","name":"Диспетчерская","rect":Rect2(16,-192,30,18),"level":0},
	{"id":"annex_antenna","name":"Аппаратная антенны","rect":Rect2(54,-192,24,18),"level":0},
	{"id":"annex_reserve","name":"Резервное хранилище","rect":Rect2(78,-192,26,18),"level":0},
	{"id":"annex_gallery","name":"Хозяйственная галерея","rect":Rect2(46,-192,8,84),"level":0},
	{"id":"iso_post","name":"Изолятор / пост","rect":Rect2(16,-62,44,12),"level":0},
	{"id":"iso_west","name":"Запад / палаты","rect":Rect2(16,-84,14,22),"level":0},
	{"id":"iso_cell","name":"Камера17","rect":Rect2(16,-94,14,10),"level":0},
	{"id":"iso_showers","name":"Душевые","rect":Rect2(30,-82,14,20),"level":0},
	{"id":"iso_records","name":"Архив17","rect":Rect2(30,-94,14,12),"level":0},
	{"id":"iso_east","name":"Восток / камеры","rect":Rect2(44,-94,16,32),"level":0},
	{"id":"iso_generator","name":"Машинный зал","rect":Rect2(16,-108,44,14),"level":0},
	{"id":"diag_office","name":"Кабинет врача","rect":Rect2(16,-50,8,8),"level":0},
	{"id":"diag_xray","name":"Рентген","rect":Rect2(16,-50,14,26),"level":0},
	{"id":"diag_observe","name":"Наблюдение","rect":Rect2(30,-50,14,26),"level":0},
	{"id":"diag_treatment","name":"Процедурная","rect":Rect2(44,-50,16,26),"level":0},
	{"id":"diag_gallery","name":"Диагностика","rect":Rect2(16,-24,44,8),"level":0},
	{"id":"evacuation","name":"Эвакуация","rect":Rect2(-2,-24,18,8),"level":0},
	{"id":"refuge","name":"Дежурная / покой","rect":Rect2(4.4,16.15,8.8,6.95),"level":0},
	{"id":"reception","name":"Приёмная","rect":Rect2(-15,8,30,8),"level":0},
	{"id":"corridor","name":"Коридор","rect":Rect2(-3,-16,6,24),"level":0},
	{"id":"ward04","name":"Палата 04","rect":Rect2(-15,-1,12,9),"level":0},
	{"id":"ward05","name":"Палата 05","rect":Rect2(3,-1,12,9),"level":0},
	{"id":"archive_w","name":"Архив","rect":Rect2(-15,-9,12,8),"level":0},
	{"id":"archive_e","name":"Пропуска","rect":Rect2(3,-9,12,8),"level":0},
	{"id":"isolation_w","name":"Терминал","rect":Rect2(-15,-16,12,7),"level":0},
	{"id":"isolation_e","name":"Изолятор","rect":Rect2(3,-16,12,7),"level":0},
	{"id":"service","name":"Щитовая","rect":Rect2(-30,-8,15,15),"level":0},
	{"id":"supply","name":"Медсклад","rect":Rect2(-30,7,7,9),"level":0},
	{"id":"annex","name":"Служебное","rect":Rect2(-23,7,8,9),"level":0},
	{"id":"research_hall","name":"Переход","rect":Rect2(15,-16,45,7),"level":0},
	{"id":"morgue_front","name":"Морг / вход","rect":Rect2(15,-9,15,16),"level":0},
	{"id":"morgue_back","name":"Морг","rect":Rect2(15,7,15,25),"level":0},
	{"id":"lab_front","name":"Лаб. / вход","rect":Rect2(30,-9,14,16),"level":0},
	{"id":"lab_back","name":"Лаборатория","rect":Rect2(30,7,14,25),"level":0},
	{"id":"theatre_front","name":"Опер. / вход","rect":Rect2(44,-9,16,16),"level":0},
	{"id":"theatre_back","name":"Операционная","rect":Rect2(44,7,16,25),"level":0},
	{"id":"stairs","name":"Лестница","rect":Rect2(50,32,4,12),"level":-2},
	{"id":"pump","name":"Насосная","rect":Rect2(36,44,10,38),"level":-1},
	{"id":"lower_hall","name":"Низ / коридор","rect":Rect2(46,44,4,38),"level":-1},
	{"id":"reactor","name":"Реактор","rect":Rect2(50,44,10,38),"level":-1},
	{"id":"res_hall","name":"Пост питания","rect":Rect2(79,-16,6,48),"level":0},
	{"id":"res_west_front","name":"Прачечная","rect":Rect2(60,-16,19,29),"level":0},
	{"id":"res_west_back","name":"Мастерская","rect":Rect2(60,13,19,19),"level":0},
	{"id":"res_east_front","name":"Дежурная","rect":Rect2(85,-16,19,29),"level":0},
	{"id":"res_east_back","name":"Спальни / лифт","rect":Rect2(85,13,19,19),"level":0}]
var visited: Dictionary = {}
var current: String = ""
func room_at(at: Vector3) -> String:
	for room: Dictionary in ROOMS:
		if room.level!=-2 and int(room.level)!=(-1 if at.y<-1 else 0): continue
		if room.rect.has_point(Vector2(at.x,at.z)): return room.id
	return ""
func update(at: Vector3) -> void:
	current = room_at(at)
	if not current.is_empty(): visited[current] = true
func surface_at(at: Vector3) -> String:
	if at.y<-1: return "metal"
	return "vinyl" if at.x>60 else "tile"
func remaining(game: Node3D, room_id: String) -> int:
	var count: int = 0
	if game.annex!=null:
		for id: String in ["brief","wheel","manual","key","register","files","ammo_post","ammo_work","ammo_archive","aid_post","aid_cold"]:
			if room_at(game.annex.stations[id].position)==room_id and not game.annex.done(id): count += 1
	if game.isolation!=null:
		for id: String in ["diary","report","proof","ammo"]:
			if room_at(game.isolation.stations[id].position)!=room_id: continue
			if id in ["diary","report"]: count += 0 if id in game.isolation.evidence else 1
			elif id=="proof": count += 0 if game.isolation.proof else 1
			else: count += 0 if game.isolation.ammo_taken else 1
	if game.upgrades!=null:
		for id: String in game.upgrades.stations:
			if room_at(game.upgrades.stations[id].position)!=room_id: continue
			if id.begins_with("record_"):
				if id.trim_prefix("record_") not in game.upgrades.records: count += 1
			elif id not in game.upgrades.used: count += 1
	if game.diagnostics!=null:
		for id: String in ["memo","log","office","supplies"]:
			if room_at(game.diagnostics.stations[id].global_position)!=room_id: continue
			if id=="supplies": count += 0 if game.diagnostics.reward_taken else 1
			else: count += 0 if id in game.diagnostics.evidence else 1
			if id=="office" and id in game.diagnostics.evidence and not game.diagnostics.office_reward: count += 1
	for object: Node3D in game.mission.objects.values():
		if (object.storage!=null or object.object_id in ["map","weapon"]) and not object.used and room_at(object.position)==room_id: count+=1
	for item: Area3D in game.cores+game.batteries+game.medkits:
		if not item.is_recovered and room_at(item.position)==room_id: count+=1
	for item: Node3D in game.get_tree().get_nodes_in_group("dropped_supplies"):
		if room_at(item.position)==room_id: count+=1
	return count
func seen_door(door: Node3D) -> bool:
	for offset: float in [-1,1]:
		if visited.has(room_at(door.position+door.basis.z*offset)): return true
	return false
func snapshot() -> Array: return visited.keys()
func restore(data: Array) -> void:
	visited.clear()
	for id: String in data: visited[id] = true
static func valid(data: Variant) -> bool:
	if not data is Array or data.size()>ROOMS.size(): return false
	var seen: Dictionary = {}
	for id: Variant in data:
		if not id is String or seen.has(id): return false
		var exists: bool = false
		for room: Dictionary in ROOMS: exists = exists or room.id==id
		if not exists: return false
		seen[id] = true
	return true
