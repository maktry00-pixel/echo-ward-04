extends RefCounted
## Boards reinforce the existing closed collision; no extra movable obstacle.
const ALLOWED: Array[String] = ["wing_-9_-1","wing_9_-1","research_23","research_37","research_52","supply_room","diag_37","diag_52","iso_west","iso_east","iso_showers","iso_north"]
const B = preload("res://scripts/world_builder.gd")
static func visual(door: Node3D) -> Node3D:
	var root := Node3D.new()
	root.name = "BarricadeBoards"
	door.add_child(root)
	var wood = preload("res://scripts/wood_surface.gd").make()
	for i: int in range(3):
		var board := B.box(root,Vector3(door.width+.22,.17,.065),Vector3(0,.8+i*.40,door.barricade_side*.19),wood)
		board.rotation.z = .10 if i%2==0 else -.12
		for side: float in [-1,1]: B.sphere(board,.015,Vector3(side*(door.width/2-.08),0,door.barricade_side*.04),B.material("645e52"))
	return root
static func valid(data: Variant) -> bool:
	if not data is Dictionary or data.size()>ALLOWED.size(): return false
	for id: Variant in data:
		if not id is String or id not in ALLOWED or not data[id] is Dictionary: return false
		if not preload("res://scripts/field_inventory.gd")._integer(data[id].get("hp"),1,3): return false
		if not preload("res://scripts/field_inventory.gd")._integer(data[id].get("side"),-1,1) or data[id].side==0: return false
	return true
