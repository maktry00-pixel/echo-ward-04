extends Node3D
## A perspective orbit camera. The maze and all its geometry stay in 3D.

const DEFAULT_YAW: float = 35.0
const DEFAULT_DISTANCE: float = 44.0
const MIN_DISTANCE: float = 34.0
const MAX_DISTANCE: float = 54.0
const ELEVATION: float = 48.0

var camera: Camera3D
var target_yaw: float = DEFAULT_YAW
var current_yaw: float = DEFAULT_YAW
var target_distance: float = DEFAULT_DISTANCE
var current_distance: float = DEFAULT_DISTANCE


func _ready() -> void:
	camera = Camera3D.new()
	camera.name = "PerspectiveCamera"
	camera.projection = Camera3D.PROJECTION_PERSPECTIVE
	camera.fov = 44.0
	camera.near = 0.1
	camera.far = 150.0
	add_child(camera)
	camera.current = true
	_update_transform()


func _process(delta: float) -> void:
	var weight: float = 1.0 - exp(-9.0 * delta)
	current_yaw = lerp(current_yaw, target_yaw, weight)
	current_distance = lerp(current_distance, target_distance, weight)
	_update_transform()


func rotate_view(steps: int) -> void:
	target_yaw += steps * 90.0


func zoom_view(steps: float) -> void:
	target_distance = clampf(target_distance + steps * 2.0, MIN_DISTANCE, MAX_DISTANCE)


func reset_view() -> void:
	target_yaw = DEFAULT_YAW
	target_distance = DEFAULT_DISTANCE


func screen_direction_to_grid(screen_direction: Vector2i) -> Vector2i:
	# Snap the camera-relative controls to a grid quarter turn. Use the final
	# orbit orientation so one queued input doesn't change during the animation.
	var quarter_turn: int = posmod(roundi(target_yaw / 90.0), 4)
	var result: Vector2i = screen_direction
	for turn: int in range(quarter_turn):
		result = Vector2i(-result.y, result.x)
	return result


func _update_transform() -> void:
	var yaw: float = deg_to_rad(current_yaw)
	var elevation: float = deg_to_rad(ELEVATION)
	var radius: float = cos(elevation) * current_distance
	camera.position = Vector3(sin(yaw) * radius, sin(elevation) * current_distance,
		cos(yaw) * radius)
	# Reserve space for the HUD beneath the board, including the near corner
	# enlarged by perspective. Zoom can deliberately inspect a closer section.
	camera.look_at(global_position + Vector3(0, -4.0, 0))
