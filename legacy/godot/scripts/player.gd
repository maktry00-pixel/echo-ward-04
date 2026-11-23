extends Node3D
## Smooth motion between cell centers. A requested turn waits until legal.

signal cell_entered(cell: Vector2i)

const MazeData = preload("res://scripts/maze.gd")
const MOVE_SPEED: float = 4.8

var maze: RefCounted
var cell: Vector2i
var target_cell: Vector2i
var direction: Vector2i = Vector2i.ZERO
var queued_direction: Vector2i = Vector2i.ZERO
var movement_enabled: bool = true


func configure(map_data: RefCounted, start: Vector2i) -> void:
	maze = map_data
	cell = start
	target_cell = start
	position = maze.world_position(start)
	rotation.y = -PI / 2.0
	_build_model()


func queue_turn(next_direction: Vector2i) -> void:
	queued_direction = next_direction


func _physics_process(delta: float) -> void:
	if not movement_enabled or maze == null:
		return
	if cell == target_cell:
		if queued_direction != Vector2i.ZERO and maze.is_walkable(cell + queued_direction):
			direction = queued_direction
		if direction == Vector2i.ZERO or not maze.is_walkable(cell + direction):
			return
		target_cell = cell + direction
		rotation.y = atan2(-float(direction.x), -float(direction.y))
	var destination: Vector3 = maze.world_position(target_cell)
	position = position.move_toward(destination, MOVE_SPEED * delta)
	if position.distance_squared_to(destination) < 0.000001:
		position = destination
		cell = target_cell
		cell_entered.emit(cell)


func reset_to_start() -> void:
	cell = MazeData.START
	target_cell = cell
	direction = Vector2i.ZERO
	queued_direction = Vector2i.ZERO
	position = maze.world_position(cell)
	rotation = Vector3(0, -PI / 2.0, 0)
	movement_enabled = true


func _build_model() -> void:
	var yellow := StandardMaterial3D.new()
	yellow.albedo_color = Color("ffd13b")
	yellow.roughness = 0.3
	yellow.emission_enabled = true
	yellow.emission = Color("ffb700")
	yellow.emission_energy_multiplier = 0.15
	var sphere := SphereMesh.new()
	sphere.radius = 0.43
	sphere.height = 0.86
	var body := MeshInstance3D.new()
	body.mesh = sphere
	body.material_override = yellow
	add_child(body)
	var black := StandardMaterial3D.new()
	black.albedo_color = Color("12152b")
	for eye_x: float in [-0.15, 0.15]:
		var eye_mesh := SphereMesh.new()
		eye_mesh.radius = 0.075
		eye_mesh.height = 0.15
		var eye := MeshInstance3D.new()
		eye.mesh = eye_mesh
		eye.material_override = black
		eye.position = Vector3(eye_x, 0.17, -0.35)
		add_child(eye)

