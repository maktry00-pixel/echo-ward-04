extends RefCounted
const Solver = preload("res://scripts/resource_csp.gd")
var last_result: Dictionary = {}
var seed_value: int = 1907

func apply(game: Node3D, seed_number: int) -> bool:
	seed_value = seed_number
	var rng := RandomNumberGenerator.new()
	rng.seed = seed_number
	var domains: Dictionary = {}
	for department: String in ["morgue", "lab", "theatre"]:
		var x: float = {"morgue": 23.0, "lab": 37.0, "theatre": 52.0}[department]
		var entry := Vector3(x, 0.1, -6)
		var candidates: Array[Vector3] = []
		for side: float in [-4, 4]:
			for z: float in [-3, 3, 11, 18]:
				var point := Vector3(x + side, 0, z)
				if not game.navigation.walkable.has(Vector2i(roundi(point.x), roundi(point.z))):
					continue
				var route: Array[Vector3] = game.navigation.find_path(entry, point)
				if route.is_empty() or route.size() < 6 or route.size() > 36:
					continue
				var safe: bool = true
				for enemy: CharacterBody3D in game.hunters:
					if enemy.spawn_position.distance_to(point) < 5:
						safe = false
				if safe:
					candidates.append(point)
		# Seeded Fisher-Yates chooses reproducible domain order, not a solution.
		for i: int in range(candidates.size() - 1, 0, -1):
			var j: int = rng.randi_range(0, i)
			var tmp: Vector3 = candidates[i]
			candidates[i] = candidates[j]
			candidates[j] = tmp
		domains["ammo_" + department] = candidates
		if department != "lab":
			domains["medkit_" + department] = candidates.duplicate()
	var solver := Solver.new()
	last_result = solver.solve(domains, 3.0)
	last_result["seed"] = seed_number
	if not last_result["success"]:
		return false # Keep authored fallback positions; expose failure in metrics.
	for id: String in last_result["positions"]:
		var at: Vector3 = last_result["positions"][id]
		if id.begins_with("ammo_"):
			game.mission.objects[id].position = at
		else:
			game.medkits[1 if id == "medkit_morgue" else 2].position = at
	return true

func snapshot(game: Node3D) -> Dictionary:
	var positions: Dictionary = {}
	for id: String in ["ammo_morgue","ammo_lab","ammo_theatre","medkit_morgue","medkit_theatre"]:
		var at: Vector3 = game.mission.objects[id].position if id.begins_with("ammo_") else game.medkits[1 if id=="medkit_morgue" else 2].position
		positions[id] = [at.x,at.y,at.z]
	return positions

func restore(game: Node3D, data: Dictionary) -> void:
	for id: String in data:
		var at := Vector3(data[id][0],data[id][1],data[id][2])
		if id.begins_with("ammo_"): game.mission.objects[id].position = at
		else: game.medkits[1 if id=="medkit_morgue" else 2].position = at

static func valid_positions(data: Variant) -> bool:
	if not data is Dictionary or data.size()!=5: return false
	for id: String in ["ammo_morgue","ammo_lab","ammo_theatre","medkit_morgue","medkit_theatre"]:
		if not data.get(id) is Array or data[id].size()!=3: return false
		for number: Variant in data[id]:
			if not (number is int or number is float) or not is_finite(float(number)): return false
		var center: float = 23 if id.ends_with("morgue") else (37 if id.ends_with("lab") else 52)
		if float(data[id][0]) not in [center-4,center+4] or float(data[id][1])!=0 or float(data[id][2]) not in [-3.0,3.0,11.0,18.0]: return false
	var points: Array[Vector3] = []
	for value: Array in data.values():
		var point := Vector3(value[0],value[1],value[2])
		for previous: Vector3 in points:
			if previous.distance_to(point)<3: return false
		points.append(point)
	return true
