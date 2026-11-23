extends SceneTree

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	var game = load("res://scenes/third_person.tscn").instantiate()
	game.set_meta("automated_test", true)
	root.add_child(game)
	for frame: int in range(8):
		await physics_frame
		await process_frame
	print("LEVEL LOADED: ", game.navigation.walkable.size(), " cells, ", game.facility.doors.size(), " doors")
	game.queue_free()
	await process_frame
	quit()
