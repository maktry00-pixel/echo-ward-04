extends Node3D
var manager: Node
func _ready() -> void:
	add_to_group("save_stations")
	var props = preload("res://scripts/imported_props.gd")
	props.body(self,Vector3(.85,.76,.55),Vector3(0,.38,0))
	props.visual(self,"painted_wooden_nightstand",Vector3(.85,.76,.55))
	props.detail(self,"portable_cassette_player",.30,Vector3(0,.78,0))
	var label := Label3D.new()
	label.text = "ЖУРНАЛ СМЕНЫ / СОХРАНЕНИЕ"
	label.position = Vector3(0,1.1,0)
	label.pixel_size = .003
	label.font_size = 17
	add_child(label)
func prompt() -> String: return "F — записать прогресс"
func interaction_point() -> Vector3: return position+Vector3(0,.1,1.1)
func interaction_rids() -> Array[RID]:
	var result: Array[RID] = []
	for body: CollisionObject3D in find_children("*","CollisionObject3D",true,false): result.append(body.get_rid())
	return result
func use_station() -> void: manager.open_browser("save")
