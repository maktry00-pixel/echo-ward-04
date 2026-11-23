extends Node3D
var refuge: Node3D
var kind: String = "save"
func _ready() -> void: add_to_group("refuge_stations")
func prompt() -> String:
	return {"save":"F — журнал смены / сохранить игру","stash":"F — шкаф припасов","records":"F — прочитать записи дежурного"}[kind]
func interaction_point() -> Vector3: return global_position+global_basis*Vector3(0,.1,1.05)
func interaction_rids() -> Array[RID]: return []
func use_station() -> void:
	if refuge.contains(refuge.game.hero.position): refuge.screen.open_view(kind)
