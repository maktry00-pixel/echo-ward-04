extends Node3D
## Shoulder camera; mild breathing and movement remain separate from collision.

const MOUSE_SENSITIVITY: float = 0.003
const DEFAULT_PITCH: float = -0.12
const SHOULDER_OFFSET: float = 0.45

var arm: SpringArm3D
var camera: Camera3D
var input_enabled: bool = true
var shake_remaining: float = 0.0
var flashlight: SpotLight3D
var flashlight_enabled: bool = true
var tension: float = 0.0
var motion_enabled: bool = true
var look_sensitivity: float = MOUSE_SENSITIVITY
var breath_phase: float = 0.0
var aiming: bool = false
var exploration_distance: float = 3.2
var is_close: bool = false
var collision_radius: float = 0.23
var shoulder: float = SHOULDER_OFFSET
var camera_distance: float = 3.2
var base_fov:float=60
var shake_scale:float=1
var invert_y:bool=false


func _ready() -> void:
	position = Vector3(0, 1.55, 0)
	rotation.x = DEFAULT_PITCH
	arm = SpringArm3D.new()
	arm.name = "SpringArm"
	arm.spring_length = 3.2
	arm.margin = 0.18
	arm.collision_mask = 1
	var probe := SphereShape3D.new()
	probe.radius = collision_radius
	arm.shape = probe
	if get_parent() is CollisionObject3D:
		arm.add_excluded_object(get_parent().get_rid())
	add_child(arm)
	camera = Camera3D.new()
	camera.name = "Camera"
	camera.fov = 60.0
	camera.h_offset = 0
	camera.near = 0.08
	camera.far = 160.0
	# The rendered camera has no untested projection offset. Sweep its entire
	# shoulder-to-pivot segment, including a sphere around the near plane.
	add_child(camera)
	camera.current = true
	flashlight = SpotLight3D.new()
	# Put the source in front of the helmet rather than inside its geometry.
	flashlight.position = Vector3(0.1, 0.3, -0.4)
	flashlight.light_color = Color("eee1bc")
	flashlight.light_energy = 4.0
	flashlight.spot_range = 18.0
	flashlight.spot_angle = 36.0
	flashlight.shadow_enabled = true
	flashlight.shadow_bias = 0.3
	flashlight.shadow_normal_bias = 1.0
	add_child(flashlight)


func _process(delta: float) -> void:
	var hero: CharacterBody3D = get_parent()
	if hero.free_flight:
		hero.model.hide()
		flashlight.visible=flashlight_enabled
		return
	var hidden: bool = hero.hiding_bed != null
	var inspecting: bool = hero.inspecting and not aiming
	position.y = lerpf(position.y, .27 if hidden else (1.15 if inspecting else (1.05 if hero.crouching else 1.55)), 1-exp(-12*delta))
	var fov: float = base_fov*.8 if aiming else (base_fov+12 if hero.dash_remaining > 0 else base_fov + tension * 2.0)
	arm.spring_length = lerpf(arm.spring_length, 1.5 if aiming else (1.85 if inspecting else exploration_distance), 1.0 - exp(-10.0 * delta))
	camera_distance = arm.spring_length
	if hidden: camera_distance = .35
	camera.fov = lerpf(camera.fov, fov, 1.0 - exp(-10.0 * delta))
	shake_remaining = maxf(0.0, shake_remaining - delta)
	breath_phase += delta * (8.0 if hero.sprinting else 4.0)
	var moving: float = minf(Vector2(hero.velocity.x, hero.velocity.z).length() / 5.4, 1.0)
	shoulder = 0.60 if aiming else (.9 if inspecting else SHOULDER_OFFSET)
	if hidden: shoulder = 0
	var bob: float = 0.0
	if motion_enabled:
		shoulder += sin(breath_phase * 7.0) * shake_remaining * 0.10*shake_scale
		bob = sin(breath_phase) * moving * 0.016 + sin(breath_phase * 5.0) * shake_remaining * 0.06*shake_scale
	resolve_collision(bob)
	flashlight.visible = flashlight_enabled


func resolve_collision(bob: float = 0.0) -> void:
	if camera == null or not is_inside_tree():
		return
	var desired: Vector3 = global_basis * Vector3(shoulder, bob, camera_distance)
	var viewport_size: Vector2 = camera.get_viewport().get_visible_rect().size
	var aspect: float = viewport_size.x / maxf(1, viewport_size.y)
	var near_half_height: float = camera.near * tan(deg_to_rad(camera.fov * 0.5))
	collision_radius = maxf(0.23, near_half_height * sqrt(1 + aspect * aspect) + 0.04)
	var shape := SphereShape3D.new()
	shape.radius = collision_radius
	var query := PhysicsShapeQueryParameters3D.new()
	query.shape = shape
	query.collision_mask = 1
	query.transform = Transform3D(Basis.IDENTITY, global_position)
	query.motion = desired
	query.margin = 0.035
	var safe: PackedFloat32Array = get_world_3d().direct_space_state.cast_motion(query)
	var fraction: float = safe[0] if safe.size() == 2 else 0.0
	camera.global_position = global_position + desired * maxf(0, fraction - 0.015)
	camera.h_offset = 0
	camera.v_offset = 0
	is_close = camera.global_position.distance_to(global_position) < 0.8
	get_parent().model.visible = not is_close and get_parent().hiding_bed == null


func _unhandled_input(event: InputEvent) -> void:
	if not input_enabled:
		return
	if event is InputEventMouseMotion and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
		apply_look(event.screen_relative)
	elif event is InputEventMouseButton and event.pressed:
		if event.button_index == MOUSE_BUTTON_WHEEL_UP:
			exploration_distance = clampf(exploration_distance - 0.3, 2.2, 4.0)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			exploration_distance = clampf(exploration_distance + 0.3, 2.2, 4.0)


func apply_look(mouse_delta: Vector2) -> void:
	rotation.y -= mouse_delta.x * look_sensitivity
	rotation.y = wrapf(rotation.y, -PI, PI)
	rotation.x = clampf(rotation.x - mouse_delta.y * look_sensitivity*(-1 if invert_y else 1),
		deg_to_rad(-60.0), deg_to_rad(25.0))


func movement_direction(input: Vector2) -> Vector3:
	return Basis(Vector3.UP, rotation.y) * Vector3(input.x, 0, input.y)


func reset_view() -> void:
	aiming = false
	exploration_distance = 3.2
	rotation = Vector3(DEFAULT_PITCH, 0, 0)
	arm.spring_length = 3.2
	shake_remaining = 0
	flashlight_enabled = true
	tension = 0
	breath_phase = 0
	camera.h_offset = 0
	camera.v_offset = 0
	camera.fov = base_fov
	camera.rotation = Vector3.ZERO
	shoulder = SHOULDER_OFFSET
	camera_distance = 3.2
	resolve_collision()
