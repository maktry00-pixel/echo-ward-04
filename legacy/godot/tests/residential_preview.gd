extends SceneTree
var game: Node3D
func _initialize() -> void: call_deferred("_run")
func _run() -> void:
	root.size = Vector2i(1280,900)
	game = load("res://scenes/third_person.tscn").instantiate()
	game.set_meta("automated_test",true)
	root.add_child(game)
	await _frames(15)
	game.mission.power_on = true
	game.mission.has_badge = true
	game.mission.relays.assign(["morgue","lab","theatre"])
	game.mission.purge_complete = true
	game.residential.interact(game.mission.objects["residence_panel"])
	for hunter: CharacterBody3D in game.hunters: hunter.stun(500)
	game.hero.reset_to(Vector3(82,.1,-7))
	game.hero.camera_rig.rotation.y = PI
	game.hero.model.rotation.y = PI
	await _frames(30)
	await _save("residential_spine")
	game.residential.mode = 2
	game.residential._refresh_power()
	game.hero.reset_to(Vector3(94,.1,5))
	game.hero.camera_rig.rotation.y = -PI/2
	game.hero.model.rotation.y = -PI/2
	game.residential.update(0)
	await _frames(25)
	await _save("residential_cell")
	game.hero.reset_to(Vector3(70,.1,7))
	game.hero.camera_rig.rotation.y = 0
	await _frames(20)
	await _save("residential_laundry")
	game.residential.has_prybar = true
	game.residential.service_opened = true
	game.residential.core_extracted = true
	game.residential.plate_latched = true
	game.residential.interact(game.mission.objects["residence_lift"])
	game.hero.reset_to(Vector3(77,.1,27))
	game.hero.camera_rig.rotation.y = PI/2
	game.hero.model.rotation.y = PI/2
	await _frames(25)
	await _save("residential_return")
	game.queue_free()
	await process_frame
	quit()
func _frames(count: int) -> void:
	for i: int in range(count):
		await physics_frame
		await process_frame
func _save(name: String) -> void:
	await RenderingServer.frame_post_draw
	print("PREVIEW ",name," ",root.get_texture().get_image().save_png("res://artifacts/"+name+".png"))
