extends StaticBody3D
## Shooting the hanging fitting changes its light and drops non-blocking debris.
const B = preload("res://scripts/world_builder.gd")
var chapter: Node3D
var broken: bool = false
var lamp: Node3D
var light: OmniLight3D
var debris: RigidBody3D
func _ready() -> void:
	collision_layer = 8
	collision_mask = 0
	set_meta("impact_surface","metal")
	var shape := BoxShape3D.new()
	shape.size = Vector3(.9,.24,.35)
	var collision := CollisionShape3D.new()
	collision.shape = shape
	add_child(collision)
	lamp = B.box(self,shape.size,Vector3.ZERO,B.material("8f9c8a",.5))
	light = OmniLight3D.new()
	light.light_color = Color("8aa6ab")
	light.light_energy = .8
	light.omni_range = 6
	add_child(light)
func break_lock() -> bool:
	if broken: return false
	set_broken(true)
	var game: Node3D = chapter.game
	debris = RigidBody3D.new()
	debris.mass = .6
	debris.collision_layer = 0
	debris.collision_mask = 1
	debris.continuous_cd = true
	var collision := CollisionShape3D.new()
	var shape := BoxShape3D.new()
	shape.size = Vector3(.9,.24,.35)
	collision.shape = shape
	debris.add_child(collision)
	B.box(debris,shape.size,Vector3.ZERO,B.material("65736c"))
	game.feedback.add_child(debris)
	debris.global_position = global_position
	debris.angular_velocity = Vector3(1,0,2)
	game.feedback.impact(global_position,Vector3.DOWN,false,Vector3.DOWN,"metal")
	game.sound_bank.play_spatial("glass_tinkle",global_position)
	game.emit_noise(global_position,22)
	return true
func set_broken(value: bool) -> void:
	broken = value
	collision_layer = 0 if value else 8
	lamp.visible = not value
	light.light_energy = 0 if value else .8
	if not value and is_instance_valid(debris): debris.queue_free()
