extends SceneTree
## Render the real game once and save a preview; requires a graphics display.


func _initialize() -> void:
	call_deferred("_capture")


func _capture() -> void:
	root.size = Vector2i(1280, 900)
	var game = load("res://scenes/main.tscn").instantiate()
	root.add_child(game)
	game.camera_rig.set_process(false)
	for frame: int in range(4):
		await process_frame
	await RenderingServer.frame_post_draw
	DirAccess.make_dir_recursive_absolute("res://artifacts")
	var image: Image = root.get_texture().get_image()
	var result: Error = image.save_png("res://artifacts/prototype_3d.png")
	print("PREVIEW SAVED: ", result == OK)
	game.camera_rig.rotate_view(1)
	game.camera_rig._process(1.0)
	for frame: int in range(3):
		await process_frame
	await RenderingServer.frame_post_draw
	image = root.get_texture().get_image()
	var rotated_result: Error = image.save_png("res://artifacts/prototype_3d_rotated.png")
	print("ROTATED PREVIEW SAVED: ", rotated_result == OK)
	quit(0 if result == OK and rotated_result == OK else 1)
