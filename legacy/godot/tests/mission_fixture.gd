extends RefCounted
## Only for isolated legacy unit suites; full-play bots complete the real chapter.
static func finish_isolation(game: Node3D) -> void:
	var data: Dictionary = game.isolation.snapshot()
	data.merge({"case":true,"circuit":"light","coil":true,"tag":true,"released":true,"seen":true,"started":true,"time":25.0,"serviced":["vent_a","vent_b"],"ready":true,"log":true,"complete":true},true)
	game.isolation.restore(data)

static func finish_annex(game: Node3D) -> void:
	var data: Dictionary = game.annex.snapshot()
	data.steps = ["brief","wheel","manual","intake","drain","pump","boiler","key","register","files","antenna","radio","release"]
	data["values"] = [2,1,3]; data.frequency = 2; data.complete = true
	game.annex.restore(data)
