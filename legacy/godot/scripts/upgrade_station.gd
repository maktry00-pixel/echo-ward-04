extends Node3D
var upgrades: Node3D
var station_id: String
var caption: String
var used: bool = false
var storage: Node3D
var marker: Node3D
func _ready() -> void: add_to_group("upgrade_stations")
func prompt() -> String: return upgrades.prompt(station_id)
func use_station() -> void: upgrades.use_station(station_id)
func interaction_point() -> Vector3: return to_global(Vector3(0,.1,1.05))
func interaction_rids() -> Array[RID]:
	var result: Array[RID] = []
	for body: CollisionObject3D in find_children("*","CollisionObject3D",true,false): result.append(body.get_rid())
	return result
