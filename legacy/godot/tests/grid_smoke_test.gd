extends SceneTree
## Run with: Godot --headless --path . --script res://tests/smoke_test.gd
## Drives the actual scene and actor through walls, turns, pause and victory.

var failures: int = 0


func _initialize() -> void:
	call_deferred("_run")


func _run() -> void:
	var game = load("res://scenes/main.tscn").instantiate()
	root.add_child(game)
	await process_frame
	game.player.set_physics_process(false)
	game.camera_rig.set_process(false)
	_check(game.camera_rig.camera.projection == Camera3D.PROJECTION_PERSPECTIVE,
		"Game uses a perspective 3D camera")
	var first_camera_position: Vector3 = game.camera_rig.camera.global_position
	_press(game, KEY_E)
	game.camera_rig._process(1.0)
	_check(game.camera_rig.camera.global_position.distance_to(first_camera_position) > 10.0,
		"Orbit changes the camera position around the actual level")
	_press(game, KEY_W)
	game.player._physics_process(0.3)
	_check(game.player.cell == Vector2i(2, 1), "Keyboard directions follow the rotated camera")
	_press(game, KEY_HOME)
	game.camera_rig._process(1.0)
	var wheel := InputEventMouseButton.new()
	wheel.button_index = MOUSE_BUTTON_WHEEL_UP
	wheel.pressed = true
	wheel.factor = 1.0
	var first_distance: float = game.camera_rig.target_distance
	game._unhandled_input(wheel)
	_check(game.camera_rig.target_distance < first_distance, "Mouse wheel zooms in")
	_press(game, KEY_HOME)
	_press(game, KEY_R)
	_check(game.total_pellets == 102, "Initial pellet count")
	_check(game.score == 0, "Initial score")
	_check(game.maze.neighbors(Vector2i(1, 1)).size() == 2, "Start has two legal exits")
	_check(not game.maze.is_walkable(Vector2i(-1, 1)), "Map boundary is blocked")
	game.player.queue_turn(Vector2i.UP)
	for frame: int in range(10):
		game.player._physics_process(0.1)
	_check(game.player.cell == Vector2i(1, 1), "Wall blocks movement")

	game.player.queue_turn(Vector2i.RIGHT)
	game.player._physics_process(0.1)
	game.player.queue_turn(Vector2i.DOWN)
	for frame: int in range(100):
		game.player._physics_process(0.05)
	_check(game.player.cell == Vector2i(5, 3), "Buffered turn waits until a legal junction")
	_check(game.score > 0, "Actual movement collects pellets")

	_press(game, KEY_P)
	var paused_position: Vector3 = game.player.position
	var paused_score: int = game.score
	game.player._physics_process(1.0)
	_check(game.is_paused and game.player.position == paused_position, "Pause freezes motion")
	_check(game.score == paused_score, "Pause preserves score")
	_press(game, KEY_R)
	_check(not game.is_paused and game.player.cell == Vector2i(1, 1), "Restart exits pause")
	_check(game.score == 0 and game.pellets.size() == 102, "Restart restores pellets and score")

	var steps: int = 0
	while not game.pellets.is_empty() and steps < 5000:
		var target: Vector2i = game.pellets.keys()[0]
		var path: Array[Vector2i] = _path(game.maze, game.player.cell, target)
		_check(not path.is_empty(), "Remaining pellet is reachable")
		if path.is_empty():
			break
		for next_cell: Vector2i in path:
			game.player.queue_turn(next_cell - game.player.cell)
			for frame: int in range(20):
				game.player._physics_process(0.05)
				if game.player.cell == next_cell:
					break
			_check(game.player.cell == next_cell, "Player arrives at requested cell")
			steps += 1
	_check(game.has_won and game.pellets.is_empty(), "Collecting every pellet wins")
	_check(game.score == 1020, "Every pellet scores exactly once")
	_check(not game.player.movement_enabled, "Victory stops movement")
	for repeat: int in range(10):
		_press(game, KEY_R)
		_check(game.score == 0 and game.pellets.size() == 102 and not game.has_won,
			"Repeated restart resets victory and pellets")
		await process_frame
	print("SMOKE TEST: %d failures; all 102 pellets traversed in %d steps." % [failures, steps])
	game.queue_free()
	await process_frame
	quit(0 if failures == 0 else 1)


func _path(maze: RefCounted, start: Vector2i, target: Vector2i) -> Array[Vector2i]:
	var frontier: Array[Vector2i] = [start]
	var previous: Dictionary = {start: start}
	var index: int = 0
	while index < frontier.size() and not previous.has(target):
		var current: Vector2i = frontier[index]
		index += 1
		for next_cell: Vector2i in maze.neighbors(current):
			if not previous.has(next_cell):
				previous[next_cell] = current
				frontier.append(next_cell)
	var result: Array[Vector2i] = []
	if not previous.has(target):
		return result
	var cursor: Vector2i = target
	while cursor != start:
		result.push_front(cursor)
		cursor = previous[cursor]
	return result


func _press(game: Node, key: int) -> void:
	var event := InputEventKey.new()
	event.physical_keycode = key
	event.pressed = true
	game._unhandled_key_input(event)


func _check(condition: bool, description: String) -> void:
	if not condition:
		failures += 1
		push_error("FAIL: " + description)
