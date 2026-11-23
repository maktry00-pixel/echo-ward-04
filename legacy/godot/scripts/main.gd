extends Node3D
## Milestone 01: playable movement and collection. Ghost AI comes next.

const MazeData = preload("res://scripts/maze.gd")
const PlayerActor = preload("res://scripts/player.gd")
const OrbitCamera = preload("res://scripts/camera_rig.gd")

var maze := MazeData.new()
var player: Node3D
var pellets: Dictionary = {}
var pellet_mesh: SphereMesh
var pellet_material: StandardMaterial3D
var pellet_root: Node3D
var score: int = 0
var total_pellets: int = 0
var is_paused: bool = false
var has_won: bool = false
var score_label: Label
var remaining_label: Label
var status_label: Label
var camera_rig: Node3D


func _ready() -> void:
	_build_environment()
	_build_maze()
	_build_player()
	_build_ui()
	_update_ui()


func _unhandled_key_input(event: InputEvent) -> void:
	if not event is InputEventKey or not event.pressed or event.echo:
		return
	match event.physical_keycode:
		KEY_Q:
			camera_rig.rotate_view(-1)
		KEY_E:
			camera_rig.rotate_view(1)
		KEY_HOME:
			camera_rig.reset_view()
		KEY_R:
			restart_game()
		KEY_ESCAPE, KEY_P:
			if not has_won:
				is_paused = not is_paused
				player.movement_enabled = not is_paused
				_update_ui()
		KEY_W, KEY_UP:
			_request_turn(Vector2i.UP)
		KEY_D, KEY_RIGHT:
			_request_turn(Vector2i.RIGHT)
		KEY_S, KEY_DOWN:
			_request_turn(Vector2i.DOWN)
		KEY_A, KEY_LEFT:
			_request_turn(Vector2i.LEFT)


func _request_turn(step: Vector2i) -> void:
	if not is_paused and not has_won:
		player.queue_turn(camera_rig.screen_direction_to_grid(step))


func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed:
		if event.button_index == MOUSE_BUTTON_WHEEL_UP:
			camera_rig.zoom_view(-event.factor)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			camera_rig.zoom_view(event.factor)


func _build_environment() -> void:
	var world := WorldEnvironment.new()
	var environment := Environment.new()
	environment.background_mode = Environment.BG_COLOR
	environment.background_color = Color("070e20")
	environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	environment.ambient_light_color = Color("b5cdf5")
	environment.ambient_light_energy = 0.4
	world.environment = environment
	add_child(world)
	var sunlight := DirectionalLight3D.new()
	sunlight.rotation_degrees = Vector3(-48, -35, 0)
	sunlight.light_color = Color("d9e9ff")
	sunlight.light_energy = 1.1
	sunlight.shadow_enabled = true
	add_child(sunlight)
	var center := Vector3((maze.width() - 1) * MazeData.CELL_SIZE / 2.0, 0,
		(maze.height() - 1) * MazeData.CELL_SIZE / 2.0)
	camera_rig = OrbitCamera.new()
	camera_rig.name = "CameraRig"
	camera_rig.position = center
	add_child(camera_rig)
	var board_size: float = maze.width() * MazeData.CELL_SIZE + 0.8
	# A raised board, luminous trim and a ground plane make its depth visible.
	_make_box(Vector3(board_size + 0.35, 0.7, board_size + 0.35),
		center + Vector3(0, -0.65, 0), _material(Color("0b142b")))
	var trim := _material(Color("2c87af"))
	trim.emission_enabled = true
	trim.emission = Color("1d698e")
	trim.emission_energy_multiplier = 0.45
	_make_box(Vector3(board_size + 0.12, 0.08, board_size + 0.12),
		center + Vector3(0, -0.28, 0), trim)
	var floor_material := _material(Color("101f39"))
	_make_box(Vector3(board_size, 0.2, board_size), center + Vector3(0, -0.14, 0), floor_material)
	_make_box(Vector3(200, 0.2, 200), center + Vector3(0, -1.25, 0),
		_material(Color("070e1b")))


func _build_maze() -> void:
	var wall_material := _material(Color("2353a0"))
	wall_material.metallic = 0.1
	wall_material.roughness = 0.55
	var wall_cap := _material(Color("3987d6"))
	pellet_mesh = SphereMesh.new()
	pellet_mesh.radius = 0.095
	pellet_mesh.height = 0.19
	pellet_mesh.radial_segments = 12
	pellet_mesh.rings = 6
	pellet_material = _material(Color("ffe5a3"))
	pellet_material.emission_enabled = true
	pellet_material.emission = Color("ffd17c")
	pellet_material.emission_energy_multiplier = 0.7
	pellet_root = Node3D.new()
	pellet_root.name = "Pellets"
	add_child(pellet_root)
	for row: int in range(maze.height()):
		for column: int in range(maze.width()):
			var cell := Vector2i(column, row)
			if not maze.is_walkable(cell):
				_make_box(Vector3(1.19, 0.82, 1.19), maze.world_position(cell, 0.41), wall_material)
				_make_box(Vector3(1.19, 0.06, 1.19), maze.world_position(cell, 0.85), wall_cap)
	_spawn_pellets()
	total_pellets = pellets.size()


func _spawn_pellets() -> void:
	for row: int in range(maze.height()):
		for column: int in range(maze.width()):
			var cell := Vector2i(column, row)
			if not maze.is_walkable(cell) or cell == MazeData.START:
				continue
			var pellet := MeshInstance3D.new()
			pellet.mesh = pellet_mesh
			pellet.material_override = pellet_material
			pellet.position = maze.world_position(cell, 0.22)
			pellet_root.add_child(pellet)
			pellets[cell] = pellet


func _build_player() -> void:
	player = PlayerActor.new()
	player.name = "Player"
	add_child(player)
	player.configure(maze, MazeData.START)
	player.cell_entered.connect(_on_cell_entered)


func _on_cell_entered(cell: Vector2i) -> void:
	if not pellets.has(cell):
		return
	var pellet: MeshInstance3D = pellets[cell]
	pellet.hide()
	pellet.queue_free()
	pellets.erase(cell)
	score += 10
	if pellets.is_empty():
		has_won = true
		player.movement_enabled = false
	_update_ui()


func restart_game() -> void:
	for pellet: MeshInstance3D in pellets.values():
		pellet.hide()
		pellet.queue_free()
	pellets.clear()
	_spawn_pellets()
	score = 0
	is_paused = false
	has_won = false
	player.reset_to_start()
	_update_ui()


func _build_ui() -> void:
	var canvas := CanvasLayer.new()
	add_child(canvas)
	var margin := MarginContainer.new()
	margin.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	margin.add_theme_constant_override("margin_left", 32)
	margin.add_theme_constant_override("margin_right", 32)
	margin.add_theme_constant_override("margin_top", 24)
	margin.add_theme_constant_override("margin_bottom", 24)
	margin.mouse_filter = Control.MOUSE_FILTER_IGNORE
	canvas.add_child(margin)
	var column := VBoxContainer.new()
	column.add_theme_constant_override("separation", 6)
	column.mouse_filter = Control.MOUSE_FILTER_IGNORE
	margin.add_child(column)
	var header := HBoxContainer.new()
	column.add_child(header)
	var title := _label("PACMAN / 3D", 28, Color("ffda55"))
	title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	header.add_child(title)
	score_label = _label("", 24, Color("f1f5ff"))
	header.add_child(score_label)
	column.add_child(_label("PROTOTYPE 02     /     PERSPECTIVE 3D", 13, Color("8ba8d3")))
	var spacer := Control.new()
	spacer.size_flags_vertical = Control.SIZE_EXPAND_FILL
	spacer.mouse_filter = Control.MOUSE_FILTER_IGNORE
	column.add_child(spacer)
	status_label = _label("", 25, Color("ffda55"))
	status_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	column.add_child(status_label)
	remaining_label = _label("", 17, Color("c5d7f5"))
	remaining_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	column.add_child(remaining_label)
	var controls := _label("WASD / ARROWS   Move     |     ESC / P   Pause     |     R   Restart", 15, Color("8ba8d3"))
	controls.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	column.add_child(controls)
	var camera_controls := _label("Q / E   Orbit camera     |     SCROLL   Zoom     |     HOME   Reset view", 14, Color("8ba8d3"))
	camera_controls.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	column.add_child(camera_controls)


func _update_ui() -> void:
	score_label.text = "SCORE  %04d" % score
	remaining_label.text = "%d / %d pellets collected" % [total_pellets - pellets.size(), total_pellets]
	if has_won:
		status_label.text = "MAZE CLEARED!   Press R to play again"
	elif is_paused:
		status_label.text = "PAUSED   Press ESC to continue"
	else:
		status_label.text = "Collect every pellet"


func _make_box(size: Vector3, location: Vector3, material: StandardMaterial3D) -> void:
	var mesh := BoxMesh.new()
	mesh.size = size
	var instance := MeshInstance3D.new()
	instance.mesh = mesh
	instance.material_override = material
	instance.position = location
	add_child(instance)


func _material(color: Color) -> StandardMaterial3D:
	var result := StandardMaterial3D.new()
	result.albedo_color = color
	result.roughness = 0.7
	return result


func _label(text: String, size: int, color: Color) -> Label:
	var result := Label.new()
	result.text = text
	result.add_theme_font_size_override("font_size", size)
	result.add_theme_color_override("font_color", color)
	result.mouse_filter = Control.MOUSE_FILTER_IGNORE
	return result

