extends RigidBody3D
## Pushable cover delays a pursuer; it never permanently seals a required route.
const Builder = preload("res://scripts/world_builder.gd")
var facility: Node3D
var initial_position: Vector3
var game: Node3D
var sound_remaining: float = 0

func _ready() -> void:
	add_to_group("push_carts")
	mass = 8
	axis_lock_angular_x = true
	axis_lock_angular_y = true
	axis_lock_angular_z = true
	linear_damp = 3
	physics_material_override = PhysicsMaterial.new()
	physics_material_override.friction = .3
	collision_layer = 1
	collision_mask = 1|2|4
	var steel := preload("res://scripts/environment_materials.gd").surface("metal","65716a",2,.65)
	var shape := BoxShape3D.new()
	shape.size = Vector3(1.55,.95,.70)
	var collision := CollisionShape3D.new()
	collision.shape = shape
	collision.position.y = .57
	add_child(collision)
	for y: float in [.25,.96]:
		Builder.box(self,Vector3(1.55,.065,.7),Vector3(0,y,0),steel)
	for x: float in [-.69,.69]:
		for z: float in [-.27,.27]:
			Builder.box(self,Vector3(.045,.91,.045),Vector3(x,.6,z),steel)
			var wheel: MeshInstance3D = Builder.sphere(self,.10,Vector3(x,.10,z),Builder.material("202824"))
			wheel.scale.x = .4
		Builder.box(self,Vector3(.04,.04,.75),Vector3(x,1.1,0),steel)
	var tray := Builder.material("aaad9b")
	Builder.box(self,Vector3(.45,.04,.30),Vector3(.35,1.02,0),tray)
	var props = preload("res://scripts/imported_props.gd")
	props.detail(self,"medical_box",.25,Vector3(-.35,1.0,0))
	props.detail(self,"medical_tape",.055,Vector3(.34,1.045,0))
	props.detail(self,"small_plastic_torch",.23,Vector3(-.36,.287,.02),Vector3(0,.45,0))
	props.detail(self,"portable_cassette_player",.22,Vector3(.33,.287,0),Vector3(-PI/2,0,0))
	initial_position = position

func _physics_process(delta: float) -> void:
	if game==null: return
	sound_remaining -= delta
	if sound_remaining<=0 and Vector2(linear_velocity.x,linear_velocity.z).length()>.3:
		sound_remaining = .85
		game.emit_noise(global_position,6)
		game.sound_bank.play_spatial("scrape",global_position,.65)

func reset_cart() -> void:
	position = initial_position
	linear_velocity = Vector3.ZERO
	angular_velocity = Vector3.ZERO
	reset_physics_interpolation()
