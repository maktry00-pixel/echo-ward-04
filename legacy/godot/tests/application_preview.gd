extends SceneTree
## Real desktop launch: no automated metadata, no changes to the player's saves.

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	var game = load("res://scenes/third_person.tscn").instantiate()
	root.add_child(game)
	for frame: int in range(30):
		await physics_frame
		await process_frame
	await RenderingServer.frame_post_draw
	var mode: int = DisplayServer.window_get_mode()
	var expected: int = DisplayServer.WINDOW_MODE_FULLSCREEN if game.settings["fullscreen"] else DisplayServer.WINDOW_MODE_WINDOWED
	var success: bool = game.in_title_menu and paused and game.front_end.visible and game.elapsed == 0 and mode == expected
	var image: Image = root.get_texture().get_image()
	success = image.save_png("res://artifacts/application_title.png") == OK and success
	print("DESKTOP STARTUP: ", success, "; fullscreen=", mode == DisplayServer.WINDOW_MODE_FULLSCREEN, "; size=", root.size)
	game.queue_free()
	await process_frame
	OS.delay_msec(100)
	quit(0 if success else 1)
