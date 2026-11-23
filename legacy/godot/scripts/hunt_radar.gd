extends Control

var game: Node3D

func _process(_delta: float) -> void:
	if visible:
		queue_redraw()

func _point(location: Vector3) -> Vector2:
	return Vector2(20 + (location.x + 30.0) / 134.0 * (size.x-40), 45 + (location.z + 192.0) / 274.0 * (size.y-115))

func _room(center: Vector3, size: Vector3, color: Color) -> void:
	var a: Vector2 = _point(center - size / 2.0)
	var b: Vector2 = _point(center + size / 2.0)
	draw_rect(Rect2(a, b - a), color)

func _draw() -> void:
	var background := StyleBoxFlat.new()
	background.bg_color = Color(0.025, 0.04, 0.035, 0.99)
	background.set_corner_radius_all(3)
	draw_style_box(background, Rect2(Vector2.ZERO,size))
	var font: Font = get_theme_default_font()
	draw_string(font, Vector2(12, 20), TextLocale.translate("ПЛАН ОТДЕЛЕНИЯ"), HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color("c1c5ad"))
	if not is_instance_valid(game) or not is_instance_valid(game.hero):
		return
	var rooms: Array = game.exploration.ROOMS.duplicate()
	rooms.sort_custom(func(a: Dictionary,b: Dictionary) -> bool: return a.rect.get_area()>b.rect.get_area())
	for room: Dictionary in rooms:
		var rect: Rect2 = room.rect
		var known: bool = game.exploration.visited.has(room.id)
		var left: int = game.exploration.remaining(game,room.id) if known else 0
		var color := Color("422e25") if left>0 else Color("254b43")
		_room(Vector3(rect.get_center().x,0,rect.get_center().y),Vector3(rect.size.x,0,rect.size.y),color if known else Color("111b20"))
		var title: String = room.name if known else "?"
		if rect.size.x>6:
			var origin: Vector2 = _point(Vector3(rect.position.x,0,rect.position.y))
			var extent: Vector2 = _point(Vector3(rect.end.x,0,rect.end.y))-origin
			draw_rect(Rect2(origin,extent),Color("8fa19a") if room.id==game.exploration.current else Color("34423e"),false,1)
			draw_string(font,origin+Vector2(5,17),TextLocale.translate(title),HORIZONTAL_ALIGNMENT_LEFT,extent.x-8,12,Color("d0c5a7") if known else Color("566568"))
			if known and extent.y>39:
				draw_string(font,origin+Vector2(5,33),TextLocale.translate("Находок: "+str(left) if left>0 else "Обыскана"),HORIZONTAL_ALIGNMENT_LEFT,extent.x-8,11,Color("b6ac90"))
	for wall: Dictionary in game.facility.map_walls:
		if game.exploration.visited.has(game.exploration.room_at(wall.center)): _room(wall.center,wall.size,Color("899082"))
	for door: Node3D in game.facility.doors:
		if not game.exploration.seen_door(door): continue
		var axis: Vector3 = door.basis.x.normalized() * door.width / 2
		var color := Color("b1735c") if door.locked else (Color("7aa98b") if door.is_open else Color("baaa79"))
		draw_line(_point(door.position - axis), _point(door.position + axis), color, 2)
	var id: String = "fuse"
	if game.mission.exit_started:
		id = "exit"
	elif game.mission.relays.size() == 3 and not game.mission.purge_complete:
		id = "reactor" if game.mission.has_coolant and game.mission.has_fuel else ("fuel" if game.mission.has_coolant else "coolant")
	elif game.mission.purge_complete and not game.residential.complete:
		id = game.residential.goal_id()
	elif game.mission.has_badge and not game.inventory.quarantine_code and not game.facility.gate_by_id["quarantine_gate"].is_open:
		id = "quarantine_gate"
	elif game.mission.has_badge:
		id = "terminal"
		for department: String in ["morgue", "lab", "theatre"]:
			if department not in game.mission.relays:
				id = "relay_" + department
				break
	elif game.mission.power_on:
		id = "badge"
	elif game.mission.has_fuse:
		id = "power"
	var goal: Vector3 = game.facility.EXIT if id == "exit" else (game.facility.gate_by_id[id].position if game.facility.gate_by_id.has(id) else game.mission.LOCATIONS[id])
	if game.mission.purge_complete and game.residential.complete and not game.diagnostics.complete:
		goal = game.diagnostics.stations["xray" if not game.diagnostics.xray_solved else "observe" if not game.diagnostics.observed else "treatment"].position
	if game.diagnostics.complete and not game.isolation.complete:
		goal = game.isolation.stations[game.isolation.goal_id()].position
	if game.isolation.complete and not game.annex.complete:
		goal = game.annex.stations[game.annex.goal_id()].position
	if game.diagnostics.finale_started:
		goal = game.diagnostics.stations["exit_crank" if game.diagnostics.breaker_reset else "final_reset"].position
	for box: Node3D in game.get_tree().get_nodes_in_group("search_storage"):
		if box.ready_for_loot and box.opened and game.exploration.visited.has(game.exploration.room_at(box.global_position)):
			draw_circle(_point(box.global_position),3,Color("e3aa61"))
	for supply: Node3D in game.get_tree().get_nodes_in_group("dropped_supplies"):
		draw_circle(_point(supply.global_position),3,Color("82becd"))
	var save_locations:Array[Vector3]=[game.refuge.SPAWN]
	for station:Node3D in game.get_tree().get_nodes_in_group("save_stations"):save_locations.append(station.position)
	for at:Vector3 in save_locations:
		if not game.exploration.visited.has(game.exploration.room_at(at)):continue
		var point:Vector2=_point(at)
		draw_rect(Rect2(point-Vector2(7,7),Vector2(14,14)),Color("c4a875"))
		draw_string(font,point+Vector2(-4,5),"S",HORIZONTAL_ALIGNMENT_LEFT,-1,12,Color("101b1d"))
	draw_circle(_point(goal), 4.0, Color("e7c986"))
	# The plan is not an X-ray tracker: only an enemy currently seeing the
	# player is displayed, while a lost pursuer disappears from the map.
	for hunter: CharacterBody3D in game.hunters:
		if hunter.active and hunter.sees_player:
			draw_circle(_point(hunter.position), 3.5, Color("d17f68"))
	var position_2d: Vector2 = _point(game.hero.position)
	var yaw: float = game.hero.camera_rig.rotation.y
	var forward := Vector2(-sin(yaw), -cos(yaw))
	var side := Vector2(-forward.y, forward.x)
	draw_colored_polygon(PackedVector2Array([position_2d + forward * 6,
		position_2d - forward * 4 + side * 4, position_2d - forward * 4 - side * 4]), Color("eeead7"))
	draw_string(font, Vector2(16,size.y-55), TextLocale.translate("Тёплая комната: остались находки · зелёная: обыскана · ?: не посещена"), HORIZONTAL_ALIGNMENT_LEFT, size.x-32, 12, Color("b1b7a4"))
	draw_string(font, Vector2(16,size.y-34), TextLocale.translate("Жёлтый: цель · красная дверь: замок · голубой: оставленные припасы")+" · "+TextLocale.translate("S — точка сохранения"), HORIZONTAL_ALIGNMENT_LEFT, size.x-32, 11, Color("b1b7a4"))
	draw_string(font, Vector2(16,size.y-13), TextLocale.translate("Tab — убрать план · I — снаряжение · H — аптечка · T — батарея"), HORIZONTAL_ALIGNMENT_LEFT, size.x-32, 12, Color("8b9d8d"))
