extends SceneTree
var game: Node3D
func _initialize() -> void:
	call_deferred("_run")
func _run() -> void:
	root.size = Vector2i(1280, 900)
	game = load("res://scenes/third_person.tscn").instantiate()
	game.set_meta("automated_test", true)
	root.add_child(game)
	await _frames(12)
	game.mission.has_fuse = true
	game.mission.interact(game.mission.objects["power"])
	game.mission.interact(game.mission.objects["badge"])
	game.combat.acquire()
	game.hero.reset_to(Vector3(40, 0.1, 11))
	game.hero.camera_rig.rotation.y = PI - 0.2
	game.hero.model.rotation.y = PI
	await _frames(120)
	await _save("research_laboratory")
	game.hero.reset_to(Vector3(52, 0.1, 16.5))
	game.hero.camera_rig.rotation.y = PI
	game.hero.camera_rig.rotation.x = 0.06
	game.hero.model.rotation.y = PI
	for hunter: CharacterBody3D in game.hunters:
		hunter.stun(100)
	game.hunters[5].model.rotation.y = 0
	game.hunters[5].position = Vector3(52, 0.1, 22)
	Input.action_press("aim")
	await _frames(45)
	await _save("combat_aim")
	game.combat.fire()
	await _frames(2)
	await _save("combat_fire")
	Input.action_release("aim")
	game.hero.reset_to(Vector3(23, 0.1, 20))
	game.hero.camera_rig.rotation.y = PI
	game.hero.model.rotation.y = PI
	await _frames(15)
	await _save("circuit_puzzle")
	game.hud.radar.show()
	await _frames(3)
	await _save("expanded_map")
	game.hud.radar.hide()
	game.hero.reset_to(Vector3(55, -4.9, 72))
	game.hero.camera_rig.rotation.y = PI - 0.25
	game.hero.model.rotation.y = PI - 0.25
	await _frames(15)
	await _save("lower_sector")
	game.hero.reset_to(Vector3(0, 0.1, 10))
	game.hunters[0].position = Vector3(0.3, 0.1, 6)
	game.hunters[0].set_active(true)
	game.hunters[0].stun(100)
	game.hunters[0].model.rotation.y = PI
	await _frames(12)
	await _save("patient_encounter")
	game.queue_free()
	await process_frame
	quit()
func _frames(count: int) -> void:
	for i: int in range(count):
		await physics_frame
		await process_frame
func _save(name: String) -> void:
	await RenderingServer.frame_post_draw
	print("PREVIEW ", name, ": ", root.get_texture().get_image().save_png("res://artifacts/" + name + ".png"))
