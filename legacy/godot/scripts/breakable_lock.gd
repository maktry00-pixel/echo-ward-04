extends StaticBody3D
## An actual combat target, separate from enemy damage and kill statistics.
const Builder = preload("res://scripts/world_builder.gd")
var wing: Node3D
var broken: bool = false
func _ready() -> void:
	add_to_group("breakable_locks")
	collision_layer = 8
	collision_mask = 0
	var shape := BoxShape3D.new()
	shape.size = Vector3(.28,.28,.12)
	var collider := CollisionShape3D.new()
	collider.shape = shape
	add_child(collider)
	var iron := Builder.material("897752")
	Builder.box(self,Vector3(.16,.19,.07),Vector3.ZERO,iron)
	var hoop := TorusMesh.new()
	hoop.inner_radius = .045
	hoop.outer_radius = .065
	var visual := MeshInstance3D.new()
	visual.mesh = hoop
	visual.material_override = iron
	visual.position.y = .12
	visual.rotation.x = PI/2
	add_child(visual)

func break_lock() -> bool:
	if broken: return false
	broken = true
	collision_layer = 0
	hide()
	wing.on_lock_broken()
	return true

func reset_lock() -> void:
	broken = false
	collision_layer = 8
	show()
