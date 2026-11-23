extends CharacterBody3D
## Free movement with physical collision; no snapping to logical grid cells.

const FollowCamera = preload("res://scripts/follow_camera.gd")
const CharacterVisual = preload("res://scripts/rocket_survivor.gd")
const WALK_SPEED: float = 3.2
const RUN_SPEED: float = 5.4
const ACCELERATION: float = 20.0
const GRAVITY: float = 22.0
const JUMP_SPEED: float = 7.0
const DASH_DURATION: float = 0.22
const DASH_COOLDOWN: float = 2.8

signal dash_started

var camera_rig: Node3D
var model: Node3D
var dash_remaining: float = 0.0
var dash_cooldown: float = 0.0
var dash_heading: Vector3 = Vector3.FORWARD
var invulnerable_remaining: float = 0.0
var stamina: float = 100.0
var exhausted: bool = false
var sprinting: bool = false
var recovery_delay: float = 0.0
var aiming: bool = false
var crouching: bool = false
var hiding_bed: Node3D
var body_collision: CollisionShape3D
var performing_action: bool = false
var reload_phase: float = -1
var hit_impulse: Vector3 = Vector3.ZERO
var action_focus: Vector3 = Vector3.ZERO
var inspecting: bool = false
var in_refuge: bool = false
var turn_velocity: float = 0
var free_flight:bool=false


func _ready() -> void:
	add_to_group("hero")
	collision_layer = 2
	collision_mask = 1
	floor_snap_length = 0.25
	safe_margin = .02
	var capsule := CapsuleShape3D.new()
	capsule.radius = 0.34
	capsule.height = 1.7
	var collision := CollisionShape3D.new()
	collision.shape = capsule
	collision.position.y = 0.87
	add_child(collision)
	body_collision = collision
	model = CharacterVisual.new()
	model.name = "Survivor"
	add_child(model)
	camera_rig = FollowCamera.new()
	camera_rig.name = "FollowCamera"
	add_child(camera_rig)


func _physics_process(delta: float) -> void:
	if free_flight:
		velocity=Vector3.ZERO
		model.hide()
		return
	dash_cooldown = maxf(0.0, dash_cooldown - delta)
	invulnerable_remaining = maxf(0.0, invulnerable_remaining - delta)
	if hiding_bed != null:
		velocity = Vector3.ZERO
		sprinting = false
		aiming = false
		model.hide()
		return
	crouching = Input.is_action_pressed("crouch") and not aiming
	var input: Vector2 = Input.get_vector("move_left", "move_right", "move_forward", "move_back")
	var heading: Vector3 = camera_rig.movement_direction(input)
	recovery_delay = maxf(0, recovery_delay - delta)
	if exhausted and stamina >= 30 and not Input.is_action_pressed("sprint"):
		exhausted = false
	sprinting = not crouching and not performing_action and not aiming and Input.is_action_pressed("sprint") and heading.length_squared() > 0.01 and not exhausted and stamina > 0
	if sprinting:
		stamina = maxf(0, stamina - delta * 20.0)
		recovery_delay = 1.2
		if stamina <= 0:
			exhausted = true
			sprinting = false
	elif recovery_delay <= 0:
		stamina = minf(100, stamina + delta * 24.0)
	var speed: float = RUN_SPEED if sprinting else WALK_SPEED
	if aiming:
		speed = 1.65
	if crouching: speed = 1.35
	if performing_action: speed = 0
	if Input.is_action_just_pressed("dash") and not crouching and not performing_action and dash_cooldown <= 0.0 and stamina >= 18:
		stamina -= 18
		recovery_delay = 1.2
		dash_heading = heading.normalized() if heading.length_squared() > 0.01 else camera_rig.movement_direction(Vector2.UP)
		dash_remaining = DASH_DURATION
		dash_cooldown = DASH_COOLDOWN
		dash_started.emit()
	if dash_remaining > 0.0:
		dash_remaining = maxf(0.0, dash_remaining - delta)
		velocity.x = dash_heading.x * 14.0
		velocity.z = dash_heading.z * 14.0
	else:
		velocity.x = move_toward(velocity.x, heading.x * speed, ACCELERATION * delta)
		velocity.z = move_toward(velocity.z, heading.z * speed, ACCELERATION * delta)
	velocity.x += hit_impulse.x*delta*8
	velocity.z += hit_impulse.z*delta*8
	hit_impulse = hit_impulse.move_toward(Vector3.ZERO,delta*9)
	if not is_on_floor():
		velocity.y -= GRAVITY * delta
	elif Input.is_action_just_pressed("jump") and not performing_action and not crouching:
		velocity.y = JUMP_SPEED
	else:
		velocity.y = 0.0
	move_and_slide()
	var previous_yaw: float = model.rotation.y
	if aiming:
		model.rotation.y = lerp_angle(model.rotation.y, camera_rig.rotation.y, 1.0 - exp(-16.0 * delta))
	elif heading.length_squared() > 0.01:
		var wanted_yaw: float = atan2(-heading.x,-heading.z)
		model.rotation.y = rotate_toward(model.rotation.y,wanted_yaw,(7.5 if sprinting else 5.8)*delta)
	turn_velocity = wrapf(model.rotation.y-previous_yaw,-PI,PI)/maxf(.001,delta)
	model.animate(Vector2(velocity.x, velocity.z).length(), delta)
	model.visible = not camera_rig.is_close
	if position.y < -12.0:
		reset_to(Vector3(0, 0.1, 12))


func reset_to(location: Vector3) -> void:
	hiding_bed = null
	in_refuge = false
	turn_velocity = 0
	performing_action = false
	crouching = false
	aiming = false
	reload_phase = -1
	inspecting = false
	hit_impulse = Vector3.ZERO
	set_low_collision(false)
	position = location
	velocity = Vector3.ZERO
	dash_remaining = 0.0
	dash_cooldown = 0.0
	invulnerable_remaining = 0.0
	stamina = 100.0
	exhausted = false
	sprinting = false
	recovery_delay = 0
	model.visible = true
	model.rotation = Vector3.ZERO
	model.reset_pose()
	camera_rig.reset_view()
	camera_rig.position.y = 1.55

func set_low_collision(low: bool) -> void:
	body_collision.shape.radius = 0.23 if low else 0.34
	body_collision.shape.height = 0.48 if low else 1.7
	body_collision.position.y = 0.25 if low else 0.87

func react_hit(source: Vector3) -> void:
	var away: Vector3 = global_position-source
	away.y = 0
	hit_impulse = away.normalized()*2.8
	velocity += hit_impulse
	model.hurt_remaining = .45
	dash_remaining = 0

