extends RefCounted
## Finite adversarial approximation: hunter maximizes, survivor minimizes.
## One ply moves one agent by one grid edge (or waits). No unseen observations.
var nodes: int = 0
var prunes: int = 0
var last_result: Dictionary = {}

func choose(grid: RefCounted, hunter: Vector2i, survivor: Vector2i, pairs: int = 2, pruning: bool = true) -> Dictionary:
	nodes = 0
	prunes = 0
	var started: int = Time.get_ticks_usec()
	var action: Vector2i = hunter
	var score: float = -INF
	var alpha: float = -INF
	for candidate: Vector2i in _moves(grid, hunter):
		var value: float = _search(grid, candidate, survivor, maxi(1, pairs) * 2 - 1, false, alpha, INF, pruning)
		if value > score:
			score = value
			action = candidate
		alpha = maxf(alpha, score)
	last_result = {"action": action, "score": score, "nodes": nodes, "prunes": prunes, "usec": Time.get_ticks_usec() - started}
	return last_result

func _moves(grid: RefCounted, cell: Vector2i) -> Array[Vector2i]:
	var result: Array[Vector2i] = grid.neighbors(cell)
	result.append(cell)
	return result

func _search(grid: RefCounted, hunter: Vector2i, survivor: Vector2i, depth: int, maximizing: bool, alpha_value: float, beta_value: float, pruning: bool) -> float:
	nodes += 1
	if hunter == survivor:
		return 100000.0 + depth
	if depth <= 0:
		var distance: int = absi(hunter.x - survivor.x) + absi(hunter.y - survivor.y)
		return -distance * 10.0 - grid.neighbors(survivor).size() * 2.0
	var alpha: float = alpha_value
	var beta: float = beta_value
	var best: float = -INF if maximizing else INF
	for candidate: Vector2i in _moves(grid, hunter if maximizing else survivor):
		var value: float = _search(grid, candidate if maximizing else hunter, survivor if maximizing else candidate, depth - 1, not maximizing, alpha, beta, pruning)
		if maximizing:
			best = maxf(best, value)
			alpha = maxf(alpha, best)
		else:
			best = minf(best, value)
			beta = minf(beta, best)
		if pruning and beta <= alpha:
			prunes += 1
			break
	return best
