extends Node3D
## Ballistic throw with swept collision; only the impact supplies an AI observation.
const Builder = preload("res://scripts/world_builder.gd")
var game: Node3D
var velocity: Vector3
var lifetime: float = 3.0
var broken: bool = false

func _ready() -> void:
	add_to_group("thrown_bottles")
	var glass := Builder.material("54736c")
	glass.roughness = .25
	var mesh := CylinderMesh.new()
	mesh.top_radius = .035
	mesh.bottom_radius = .045
	mesh.height = .21
	mesh.radial_segments = 10
	var visual := MeshInstance3D.new()
	visual.mesh = mesh
	visual.material_override = glass
	add_child(visual)
	Builder.box(self,Vector3(.02,.05,.02),Vector3(0,.12,0),glass)

func _physics_process(delta: float) -> void:
	if broken: return
	lifetime -= delta
	velocity.y -= 12*delta
	var next: Vector3 = global_position+velocity*delta
	var ray := PhysicsRayQueryParameters3D.create(global_position,next,1|4)
	var hit: Dictionary = get_world_3d().direct_space_state.intersect_ray(ray)
	if not hit.is_empty():
		global_position = hit.position
		shatter()
	elif lifetime <= 0:
		shatter()
	else:
		global_position = next
		rotate_x(delta*8)

func shatter() -> void:
	if broken: return
	broken = true
	game.emit_noise(global_position,22)
	game.sound_bank.play_spatial("impact",global_position,1.5)
	game.sound_bank.register_glass(Vector3(global_position.x,game.navigation.floor_height(global_position),global_position.z))
	game.feedback.pulse(global_position,.7)
	queue_free()
