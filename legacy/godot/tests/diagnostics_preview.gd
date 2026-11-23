extends SceneTree
var game: Node3D
var camera: Camera3D
func _initialize() -> void: call_deferred("_run")
func _run() -> void:
	root.size = Vector2i(1280,900)
	game = load("res://scenes/third_person.tscn").instantiate()
	game.set_meta("automated_test",true)
	root.add_child(game)
	await _frames(20)
	for hunter: CharacterBody3D in game.hunters: hunter.set_active(false); hunter.set_physics_process(false)
	camera = Camera3D.new()
	game.add_child(camera)
	camera.current = true
	camera.fov = 64
	game.hud.hide()
	game.combat.overlay.hide()
	game.survival_tools.status.hide()
	await _shot("diagnostics_xray",Vector3(28,2.2,-27),Vector3(21,1.1,-34.5))
	await _shot("diagnostics_observation",Vector3(33,2,-26.5),Vector3(40,1.2,-37))
	await _shot("diagnostics_treatment",Vector3(46,2,-30),Vector3(54,1,-39))
	game.diagnostics.screen.open_screen("xray")
	await _frames(10)
	await _save("diagnostics_puzzle")
	game.diagnostics.screen.dismiss()
	game.diagnostics.xray_solved = true
	game.diagnostics.observed = true
	game.diagnostics.screen.open_screen("treatment")
	await _frames(5)
	await _save("diagnostics_protocol")
	game.diagnostics.screen.dismiss()
	game.diagnostics.evidence.assign(["memo","log","office"])
	game.hud.show()
	game._finish_round(true)
	await _frames(15)
	await _save("diagnostics_ending")
	game.queue_free()
	await process_frame
	print("DIAGNOSTIC PREVIEW COMPLETE")
	quit()
func _shot(id: String,at: Vector3,target: Vector3) -> void:
	camera.position = at
	camera.look_at(target)
	await _frames(20)
	await _save(id)
func _save(id: String) -> void:
	await RenderingServer.frame_post_draw
	root.get_texture().get_image().save_png("res://artifacts/"+id+".png")
func _frames(count: int) -> void:
	for i: int in range(count): await physics_frame; await process_frame
