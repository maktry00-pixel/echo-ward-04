extends Node3D
## A world interaction with an authored stance, shared by the diagnostic chapter.
var chapter: Node3D
var station_id: String
var caption: String
var used: bool = false
var stance := Vector3(0,0,1.05)
var label: Label3D

func _ready() -> void:
	add_to_group("chapter_stations")
	label = Label3D.new()
	label.position = Vector3(0,1.55,.12)
	label.font_size = 17
	label.pixel_size = .004
	label.text = caption
	label.modulate = Color("b8c5bb")
	add_child(label)

func prompt() -> String: return chapter.prompt(station_id)
func use_station() -> void: chapter.use_station(station_id)
func interaction_point() -> Vector3: return to_global(stance)
func interaction_rids() -> Array[RID]:
	var result: Array[RID] = []
	for body: CollisionObject3D in find_children("*","CollisionObject3D",true,false): result.append(body.get_rid())
	return result
