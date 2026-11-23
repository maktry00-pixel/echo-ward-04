extends SceneTree
const Grid = preload("res://scripts/hunt_navigation.gd")
const Minimax = preload("res://scripts/tactical_minimax.gd")
const CSP = preload("res://scripts/resource_csp.gd")
var game: Node3D
var checks: int = 0
var failures: int = 0
func _initialize() -> void:
	call_deferred("_run")
func _run() -> void:
	game = load("res://scenes/third_person.tscn").instantiate()
	game.set_meta("automated_test", true)
	root.add_child(game)
	await _frames(15)
	for enemy: CharacterBody3D in game.hunters:
		enemy.set_physics_process(false)
	var rig: Node3D = game.hero.camera_rig
	var sphere := SphereShape3D.new()
	sphere.radius = 0.18
	var query := PhysicsShapeQueryParameters3D.new()
	query.shape = sphere
	query.collision_mask = 1
	var collisions: int = 0
	var views: int = 0
	for place: Vector3 in [Vector3(14.2, 0.1, 3), Vector3(14.2, 0.1, 15.1), Vector3(3.6, 0.1, 2), Vector3(58.8, -4.9, 80.8),Vector3(78.2,.1,20),Vector3(103.1,.1,27),Vector3(97.05,.1,5),Vector3(60.8,.1,-12),Vector3(43.4,.1,-33),Vector3(59.1,.1,-43),Vector3(16.9,.1,-30.3),Vector3(37,.1,-49.1)]:
		game.hero.reset_to(place)
		for pitch: float in [deg_to_rad(-60), -0.12, deg_to_rad(25)]:
			for yaw: int in range(0, 360, 15):
				for aim: bool in [false, true]:
					rig.rotation = Vector3(pitch, deg_to_rad(yaw), 0)
					rig.shoulder = 0.6 if aim else 0.45
					rig.camera_distance = 1.5 if aim else 4.0
					rig.resolve_collision()
					query.transform = Transform3D(Basis.IDENTITY, rig.camera.get_camera_transform().origin)
					if not game.get_world_3d().direct_space_state.intersect_shape(query, 1).is_empty():
						collisions += 1
					views += 1
	_check(collisions == 0, "Camera sphere stays outside walls/corners/ceiling across %d views (%d overlaps)" % [views, collisions])
	_check(rig.camera.h_offset == 0 and rig.camera.v_offset == 0, "Rendered camera has no unchecked projection displacement")
	var grid := Grid.new()
	for x: int in range(7):
		for y: int in range(7):
			if not (x == 3 and y in [1, 2, 4, 5]):
				grid.walkable[Vector2i(x, y)] = true
	var table: Array[Dictionary] = []
	for i: int in range(10):
		var a := Minimax.new()
		var b := Minimax.new()
		var hunter := Vector2i(i % 3, i % 7)
		var survivor := Vector2i(4 + i % 3, (i * 3) % 7)
		var full: Dictionary = a.choose(grid, hunter, survivor, 2, false)
		var pruned: Dictionary = b.choose(grid, hunter, survivor, 2, true)
		_check(full["action"] == pruned["action"] and full["score"] == pruned["score"], "Minimax/alpha-beta same action and value scenario %d" % i)
		_check(pruned["nodes"] <= full["nodes"], "Alpha-beta expands no more nodes scenario %d" % i)
		table.append({"scenario": i, "full_nodes": full["nodes"], "pruned_nodes": pruned["nodes"], "full_usec": full["usec"], "pruned_usec": pruned["usec"], "action": str(full["action"]), "score": full["score"]})
	var solver := CSP.new()
	var impossible: Dictionary = solver.solve({"a": [Vector3.ZERO], "b": [Vector3.ZERO]}, 1)
	_check(not impossible["success"] and impossible["positions"].is_empty() and impossible["backtracks"] > 0, "CSP proves conflicting domains unsatisfiable with backtracking")
	var forced: Dictionary = solver.solve({"a": [Vector3.ZERO, Vector3(5, 0, 0)], "b": [Vector3.ZERO]}, 1)
	_check(forced["success"] and forced["positions"]["a"] == Vector3(5, 0, 0), "MRV and forward checking preserve satisfiable alternative")
	var csp_rows: Array[Dictionary] = []
	for seed_number: int in range(20):
		_check(game.resource_layout.apply(game, seed_number), "CSP resource layout is satisfiable seed %d" % seed_number)
		var result: Dictionary = game.resource_layout.last_result
		var points: Array = result.get("positions", {}).values()
		var valid: bool = points.size() == 5
		for i: int in range(points.size()):
			for j: int in range(i):
				valid = valid and points[i].distance_to(points[j]) >= 3.0
		_check(valid, "CSP assignments satisfy separation seed %d" % seed_number)
		csp_rows.append({"seed": seed_number, "assignments": result["assignments"], "backtracks": result["backtracks"], "checks": result["checks"], "usec": result["usec"]})
	var file := FileAccess.open("res://artifacts/ai_experiments.json", FileAccess.WRITE)
	var search_rows: Array[Dictionary] = []
	var cells: Array[Vector2i] = _component(game.navigation, game.navigation.nearest(game.facility.START))
	for i: int in range(20):
		var a: Vector2i = cells[(i * 157) % cells.size()]
		var b: Vector2i = cells[(i * 311 + 503) % cells.size()]
		var started: int = Time.get_ticks_usec()
		var route: Array[Vector3] = game.navigation.find_path(Vector3(a.x, 0, a.y), Vector3(b.x, 0, b.y))
		var astar_usec: int = Time.get_ticks_usec() - started
		var astar_nodes: int = game.navigation.last_expanded
		var bfs: Dictionary = _bfs(game.navigation, a, b)
		var length: int = -1 if route.is_empty() else route.size() - 1
		_check(length == bfs["length"], "A*/BFS same reachability and shortest length pair %d" % i)
		search_rows.append({"start":str(a), "goal":str(b), "length":length, "astar_nodes":astar_nodes, "bfs_nodes":bfs["nodes"], "astar_usec":astar_usec, "bfs_usec":bfs["usec"]})
	_check(game.navigation.find_path(Vector3(0, 0, 12), Vector3(0, 0, 12)).size() == 1, "A* start equals goal")
	file.store_string(JSON.stringify({"astar_bfs": search_rows, "minimax": table, "csp": csp_rows, "camera_views": views, "camera_overlaps": collisions}, "\t"))
	file.close()
	game.queue_free()
	await process_frame
	print("CAMERA AND AI: %d checks, %d failures" % [checks, failures])
	quit(0 if failures == 0 else 1)
func _frames(count: int) -> void:
	for i: int in range(count):
		await physics_frame
		await process_frame
func _check(condition: bool, description: String) -> void:
	checks += 1
	if not condition:
		failures += 1
		push_error("FAIL: " + description)

func _bfs(grid: RefCounted, a: Vector2i, b: Vector2i) -> Dictionary:
	var started: int = Time.get_ticks_usec()
	var frontier: Array[Vector2i] = [a]
	var distance: Dictionary = {a: 0}
	var index: int = 0
	while index < frontier.size():
		var cell: Vector2i = frontier[index]
		index += 1
		if cell == b:
			return {"length":distance[cell], "nodes":index, "usec":Time.get_ticks_usec()-started}
		for next: Vector2i in grid.neighbors(cell):
			if not distance.has(next):
				distance[next] = int(distance[cell]) + 1
				frontier.append(next)
	return {"length":-1, "nodes":index, "usec":Time.get_ticks_usec()-started}

func _component(grid: RefCounted, start: Vector2i) -> Array[Vector2i]:
	var frontier: Array[Vector2i] = [start]
	var seen: Dictionary = {start: true}
	var index: int = 0
	while index < frontier.size():
		var cell: Vector2i = frontier[index]
		index += 1
		for next: Vector2i in grid.neighbors(cell):
			if not seen.has(next):
				seen[next] = true
				frontier.append(next)
	return frontier
