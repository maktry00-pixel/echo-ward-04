extends SceneTree
var game: Node3D
func _initialize() -> void: call_deferred("_run")
func _run() -> void:
	root.size = Vector2i(1280,900)
	game = load("res://scenes/third_person.tscn").instantiate()
	game.set_meta("automated_test",true)
	root.add_child(game)
	await _frames(20)
	for hunter: CharacterBody3D in game.hunters:
		hunter.set_active(false)
		hunter.set_physics_process(false)
	game.combat.acquire()
	game.mission.has_badge = true
	game.inventory.take_supply("medkit")
	game.inventory.take_supply("battery")
	game.inventory.screen.open_inventory("badge")
	await _frames(10)
	await _save("inventory_front")
	game.inventory.screen.inspector.flip()
	await _frames(30)
	await _save("inventory_badge_back")
	game.inventory.screen.dismiss()
	game.inventory.screen.open_keypad(game.facility.gate_by_id["quarantine_gate"])
	game.inventory.screen.keypad_key("4")
	game.inventory.screen.keypad_key("1")
	await _frames(10)
	await _save("quarantine_keypad")
	game.inventory.screen.dismiss()
	for at: Vector3 in [Vector3(0,.1,11),Vector3(9,.1,4),Vector3(9,.1,-5),Vector3(0,.1,4),Vector3(0,.1,-12),Vector3(23,.1,-12)]: game.exploration.update(at)
	game.hero.reset_to(Vector3(9,.1,3))
	game.mission.has_map = true
	game.hud.radar.show()
	await _frames(15)
	await _save("exploration_map")
	game.hud.radar.hide()
	game.hero.reset_to(Vector3(52,.1,3.5))
	game.hero.camera_rig.rotation = Vector3(-.13,PI,0)
	game.guardian.start_encounter()
	await _frames(220)
	game.guardian.exposed_remaining = 2.8
	game.hunters[5].model.rotation.y = 0
	await _frames(4)
	await _save("guardian_exposed")
	game.queue_free()
	await process_frame
	print("SIX FEATURE PREVIEWS COMPLETE")
	quit()
func _frames(count: int) -> void:
	for i: int in range(count):
		await physics_frame
		await process_frame
func _save(name: String) -> void:
	await RenderingServer.frame_post_draw
	print(name," ",root.get_texture().get_image().save_png("res://artifacts/"+name+".png"))
