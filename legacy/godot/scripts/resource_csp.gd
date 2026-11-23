extends RefCounted
## Actual MRV + forward-checking backtracking over finite authored domains.
var assignments: int = 0
var backtracks: int = 0
var checks: int = 0
var minimum_separation: float = 3.0

func solve(domains: Dictionary, separation: float = 3.0) -> Dictionary:
	assignments = 0
	backtracks = 0
	checks = 0
	minimum_separation = separation
	var started: int = Time.get_ticks_usec()
	var result: Dictionary = {}
	var success: bool = _search(domains, result)
	return {"success": success, "positions": result if success else {}, "assignments": assignments, "backtracks": backtracks, "checks": checks, "usec": Time.get_ticks_usec() - started}

func _legal(point: Vector3, assigned: Dictionary) -> bool:
	for other: Vector3 in assigned.values():
		checks += 1
		if point.distance_to(other) < minimum_separation:
			return false
	return true

func _search(domains: Dictionary, assigned: Dictionary) -> bool:
	if assigned.size() == domains.size():
		return true
	var selected: String = ""
	var remaining: Array[Vector3] = []
	var count: int = 2147483647
	for id: String in domains:
		if assigned.has(id):
			continue
		var legal: Array[Vector3] = []
		for point: Vector3 in domains[id]:
			if _legal(point, assigned):
				legal.append(point)
		if legal.is_empty():
			return false
		if legal.size() < count:
			count = legal.size()
			selected = id
			remaining = legal
	for point: Vector3 in remaining:
		assignments += 1
		assigned[selected] = point
		if _search(domains, assigned):
			return true
		assigned.erase(selected)
		backtracks += 1
	return false
