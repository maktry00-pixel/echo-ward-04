extends Area3D
var inventory: Node
var kind: String = "medkit"
var amount: int = 1
func _ready() -> void:
	add_to_group("dropped_supplies")
	collision_layer = 0
	collision_mask = 0
	var visual := Node3D.new()
	add_child(visual)
	preload("res://scripts/loot_visual.gd").build(visual,"ammo_dropped" if kind=="ammo" else "",kind)
	if kind=="bottle":
		for child: Node in visual.get_children(): child.free()
		var bottle := CylinderMesh.new()
		bottle.top_radius = .025
		bottle.bottom_radius = .05
		bottle.height = .22
		var mesh := MeshInstance3D.new()
		mesh.mesh = bottle
		mesh.material_override = preload("res://scripts/world_builder.gd").material("3b5146")
		mesh.position.y = .11
		visual.add_child(mesh)
	var label := Label3D.new()
	label.text = "F · "+{"medkit":"АПТЕЧКА","battery":"БАТАРЕЯ","ammo":"9 мм","bottle":"БУТЫЛКИ","shells":"12 КАЛИБР","boards":"ДОСКИ"}[kind]+" ×"+str(amount)
	label.position.y = .36
	label.pixel_size = .002
	label.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	add_child(label)
func prompt() -> String: return "F — вернуть оставленные припасы в инвентарь"
func interaction_point() -> Vector3: return global_position+Vector3(0,.1,.75)
func interaction_rids() -> Array[RID]: return []
func collect() -> bool:
	if inventory.game.hero.global_position.distance_to(global_position)>1.8: return false
	if not inventory.take_supply(kind,amount): return false
	remove_from_group("dropped_supplies")
	queue_free()
	return true
