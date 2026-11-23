extends RefCounted
## A* implemented explicitly for the course. The grid is sampled from the
## actual world colliders, while the player still moves freely in 3D.

var walkable: Dictionary = {}
var last_expanded: int = 0
var ignored_movable_rids: Array[RID] = []
var movable_bodies: Array[WeakRef] = []

func find_clear_path(origin: Vector3, destination: Vector3) -> Array[Vector3]:
	# Furniture is static; gates rebuild this physical grid when their state changes.
	return find_path(origin,destination)


func build(space: PhysicsDirectSpaceState3D) -> void:
	walkable.clear()
	var shape := SphereShape3D.new()
	shape.radius = 0.52
	var query := PhysicsShapeQueryParameters3D.new()
	query.shape = shape
	query.collision_mask = 1
	query.exclude = ignored_movable_rids
	for z: int in range(-191, 82):
		for x: int in range(-29, 104):
			if z<-108 and x<17: continue
			if z >= 32 and (x < 36 or (z < 44 and (x < 51 or x > 53))):
				continue
			var y: float = floor_height(Vector3(x, 0, z))
			query.transform = Transform3D(Basis.IDENTITY, Vector3(x, y + 1.0, z))
			var floor_ray := PhysicsRayQueryParameters3D.create(Vector3(x, y + 0.3, z), Vector3(x, y - 0.6, z), 1)
			floor_ray.exclude = ignored_movable_rids
			if space.intersect_shape(query, 1).is_empty() and not space.intersect_ray(floor_ray).is_empty():
				walkable[Vector2i(x, z)] = true


func nearest(location: Vector3) -> Vector2i:
	var rounded := Vector2i(roundi(location.x), roundi(location.z))
	if walkable.has(rounded):
		return rounded
	var result: Vector2i = rounded
	var distance: float = INF
	for cell: Vector2i in walkable:
		var candidate: float = Vector2(cell.x - location.x, cell.y - location.z).length_squared()
		if candidate < distance:
			distance = candidate
			result = cell
	return result


func neighbors(cell: Vector2i) -> Array[Vector2i]:
	var result: Array[Vector2i] = []
	for step: Vector2i in [Vector2i.UP, Vector2i.RIGHT, Vector2i.DOWN, Vector2i.LEFT]:
		if walkable.has(cell + step):
			result.append(cell + step)
	return result


func find_path(origin: Vector3, destination: Vector3) -> Array[Vector3]:
	last_expanded = 0
	var result: Array[Vector3] = []
	if walkable.is_empty():
		return result
	var start: Vector2i = nearest(origin)
	var goal: Vector2i = nearest(destination)
	var frontier: Array[Vector2i] = [start]
	var cost: Dictionary = {start: 0}
	var estimate: Dictionary = {start: _heuristic(start, goal)}
	var previous: Dictionary = {}
	var closed: Dictionary = {}
	while not frontier.is_empty():
		var best_index: int = 0
		for index: int in range(1, frontier.size()):
			if estimate[frontier[index]] < estimate[frontier[best_index]]:
				best_index = index
		var current: Vector2i = frontier[best_index]
		frontier.remove_at(best_index)
		last_expanded += 1
		if current == goal:
			var cursor: Vector2i = goal
			result.push_front(Vector3(cursor.x, floor_height(Vector3(cursor.x, 0, cursor.y)) + 0.1, cursor.y))
			while previous.has(cursor):
				cursor = previous[cursor]
				result.push_front(Vector3(cursor.x, floor_height(Vector3(cursor.x, 0, cursor.y)) + 0.1, cursor.y))
			return result
		closed[current] = true
		for next: Vector2i in neighbors(current):
			if closed.has(next):
				continue
			var next_cost: int = int(cost[current]) + 1
			if next_cost < int(cost.get(next, 1000000)):
				previous[next] = current
				cost[next] = next_cost
				estimate[next] = next_cost + _heuristic(next, goal)
				if not frontier.has(next):
					frontier.append(next)
	return result


func _heuristic(a: Vector2i, b: Vector2i) -> int:
	return absi(a.x - b.x) + absi(a.y - b.y)


func floor_height(point: Vector3) -> float:
	if point.z > 32:
		return -5.0 * clampf((point.z - 32) / 12.0, 0, 1)
	return 0.0
