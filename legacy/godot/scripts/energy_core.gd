extends Area3D

signal recovered(core: Area3D)
const Builder = preload("res://scripts/world_builder.gd")
var is_recovered: bool = false
var storage: Node3D
var visual: Node3D
var elapsed: float = 0.0
var kind: String = "fragment"
var indicator: StandardMaterial3D
var record_index: int = 0


func _ready() -> void:
	collision_layer = 0
	collision_mask = 2
	var shape := SphereShape3D.new()
	shape.radius = 0.9
	var collision := CollisionShape3D.new()
	collision.shape = shape
	collision.position.y = 0.8
	add_child(collision)
	storage = preload("res://scripts/search_storage.gd").new()
	add_child(storage)
	indicator = Builder.material("9fb8a5",0)


func _process(delta: float) -> void:
	elapsed += delta
	indicator.emission_energy_multiplier = 0.25 + sin(elapsed * 2.0) * 0.1


func try_collect(body: Node3D) -> bool:
	if is_recovered or not body.is_in_group("hero") or not storage.ready_for_loot:
		return false
	var offset: Vector3 = body.global_position - global_position
	if Vector2(offset.x, offset.z).length() > 1.8 or absf(offset.y) > 1.8:
		return false
	var game: Node = get_tree().get_first_node_in_group("game")
	if kind in ["medkit","battery"] and not game.inventory.can_take(kind): return false
	is_recovered = true
	storage.finish()
	set_deferred("monitoring", false)
	recovered.emit(self)
	return true


func prompt() -> String:
	return storage.prompt()

func legacy_prompt() -> String:
	if kind == "medkit":
		return "F — применить аптечку"
	return "F — взять батарею" if kind == "battery" else "F — прочитать запись"


func reset_core() -> void:
	is_recovered = false
	storage.reset_storage()
	show()
	set_deferred("monitoring", true)

func interaction_point() -> Vector3: return storage.interaction_point()
func interaction_rids() -> Array[RID]:
	var result: Array[RID] = []
	for node: CollisionObject3D in find_children("*","CollisionObject3D",true,false): result.append(node.get_rid())
	return result
