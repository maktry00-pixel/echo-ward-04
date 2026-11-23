extends SceneTree

var game: Node3D
var success: bool = true

func _initialize() -> void:
	call_deferred("_capture")

func _capture() -> void:
	root.size = Vector2i(1280, 900)
	game = load("res://scenes/third_person.tscn").instantiate()
	game.set_meta("automated_test", true)
	root.add_child(game)
	await _frames(16)
	game.show_title_menu()
	await _frames(4)
	await _save("title_menu")
	game.front_end.main_view.hide()
	game.front_end.settings_view.show()
	await _frames(4)
	await _save("settings")
	game._new_game()
	await _frames(8)
	await _save("ward_corridor")
	game.hero.reset_to(Vector3(0, 0.1, 10))
	game.hunters[0].position = Vector3(1.1, 0.1, 6.5)
	game.hunters[0].set_active(true)
	game.hunters[0].stun(100)
	game.hunters[0].model.rotation.y = PI
	await _frames(8)
	await _save("horror_creature")
	game.restart_game()
	await _frames(8)
	game.mission.interact(game.mission.objects["map"])
	game.hud.radar.show()
	await _frames(4)
	await _save("mission_map")
	game.hud.radar.hide()
	game.hero.reset_to(Vector3(-26, 0.1, -3))
	game.mission.interact(game.mission.objects["fuse"])
	game.mission.interact(game.mission.objects["power"])
	await _frames(120)
	await _save("service_power")
	game.hero.reset_to(Vector3(-21, 0.1, 11))
	game.hero.camera_rig.rotation.y = PI / 2
	await _frames(10)
	await _save("supply_room")
	game.mission.interact(game.mission.objects["badge"])
	for department: String in game.mission.CIRCUITS:
		for i: int in range(3):
			var dial: Node3D = game.mission.objects["dial_%s_%d" % [department, i]]
			for step: int in range(game.mission.CIRCUITS[department][i]):
				game.mission.interact(dial)
		if department == "theatre":
			game.hunters[5].set_active(true)
			game.hunters[5].receive_bullet(game.hero.position, 12, true)
		game.mission.interact(game.mission.objects["relay_" + department])
	game.mission.interact(game.mission.objects["terminal"])
	game.hero.reset_to(Vector3(0, 0.1, 10))
	game.hero.camera_rig.rotation.y = PI + 0.25
	game.hero.model.rotation.y = PI
	await _frames(120)
	await _save("changed_reception")
	game.set_paused(true)
	game.hud.show_journal(game.STORIES)
	await _frames(4)
	await _save("journal")
	game.queue_free()
	await process_frame
	quit(0 if success else 1)

func _frames(count: int) -> void:
	for frame: int in range(count):
		await physics_frame
		await process_frame

func _save(name: String) -> void:
	await RenderingServer.frame_post_draw
	var result: Error = root.get_texture().get_image().save_png("res://artifacts/" + name + ".png")
	success = success and result == OK
	print("PREVIEW ", name, ": ", result == OK)
