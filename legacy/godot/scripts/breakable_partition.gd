extends StaticBody3D
## Only these explicitly authored thin panels can be penetrated by a bullet.
signal broken_changed
const B = preload("res://scripts/world_builder.gd")
var panel_id: String = ""
var panel_size := Vector3(.1,2.8,3.4)
var kind: String = "glass"
var broken: bool = false
var visual: Node3D
var collision: CollisionShape3D
var chapter: Node3D

func _ready() -> void:
	collision_layer = 1
	collision_mask = 2|4
	set_meta("impact_surface","metal" if kind=="sheet" else "glass")
	var shape := BoxShape3D.new()
	shape.size = panel_size
	collision = CollisionShape3D.new()
	collision.shape = shape
	add_child(collision)
	var mat := B.material("899e9d" if kind=="glass" else "67746d")
	if kind=="glass":
		mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
		mat.albedo_color.a = .28
		mat.roughness = .18
	else: mat.roughness = .75
	visual = B.box(self,panel_size,Vector3.ZERO,mat)
	add_to_group("breakable_partitions")

func strike(_origin: Vector3 = Vector3.ZERO) -> bool:
	if broken: return false
	set_broken(true)
	if chapter!=null:
		chapter.game.sound_bank.play_spatial("glass" if kind=="glass" else "metal_hit",global_position)
		chapter.game.emit_noise(global_position,30)
		chapter.on_partition_broken(panel_id)
	return true

func set_broken(value: bool) -> void:
	broken = value
	collision_layer = 0 if value else 1
	visual.visible = not value
	for node: Node in get_children():
		if node.is_in_group("partition_fragments"): node.free()
	if value:
		for i: int in range(12):
			var shard := B.box(self,Vector3(.12+(i%3)*.04,.015,.16),Vector3(((i%3)-1)*.45,-global_position.y+.035,((i/3)-1.5)*.5),B.material("647773"))
			shard.rotation.y = i*1.7
			shard.add_to_group("partition_fragments")
	broken_changed.emit()
