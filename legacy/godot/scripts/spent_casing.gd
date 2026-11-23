extends RigidBody3D
var remaining: float = 3.5
var shotgun_shell: bool = false
func _ready() -> void:
	add_to_group("spent_casings")
	mass = .012
	collision_layer = 0
	collision_mask = 1
	continuous_cd = true
	var shape := SphereShape3D.new()
	shape.radius = .018
	var collision := CollisionShape3D.new()
	collision.shape = shape
	add_child(collision)
	var mesh := CylinderMesh.new()
	mesh.top_radius = .012
	mesh.bottom_radius = .012
	mesh.height = .035
	mesh.radial_segments = 7
	var visual := MeshInstance3D.new()
	visual.mesh = mesh
	visual.material_override = preload("res://scripts/world_builder.gd").material("a58b50")
	if shotgun_shell: preload("res://scripts/shotgun_visual.gd").shell_visual(self,Vector3.ZERO)
	else: add_child(visual)
	if shotgun_shell: visual.free()
	physics_material_override = PhysicsMaterial.new()
	physics_material_override.bounce = .35

func _physics_process(delta: float) -> void:
	remaining -= delta
	if remaining<=0: queue_free()
