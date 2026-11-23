extends SceneTree
var game: Node3D
var camera: Camera3D
func _initialize() -> void: call_deferred("_run")
func _run() -> void:
	root.size = Vector2i(1280,900)
	game = load("res://scenes/third_person.tscn").instantiate()
	game.set_meta("automated_test",true)
	root.add_child(game)
	await _frames(18)
	for actor: CharacterBody3D in game.hunters: actor.set_active(false); actor.set_physics_process(false)
	game.set_physics_process(false)
	game.diagnostics.complete = true
	game.isolation.case_read = true
	game.isolation.circuit = "light"
	game.isolation._apply_power()
	game.hero.reset_to(Vector3(52,.1,-55))
	game.hero.set_physics_process(false)
	game.hud.hide()
	game.combat.overlay.hide()
	game.upgrades.status.hide()
	game.survival_tools.status.hide()
	game.isolation.status.hide()
	camera = Camera3D.new()
	game.add_child(camera)
	camera.current = true
	camera.fov = 65
	await _shot("isolation_post",Vector3(49,2,-51.6),Vector3(52,1.15,-59))
	await _shot("isolation_ward",Vector3(22,1.75,-66),Vector3(25,1,-77))
	await _shot("isolation_showers",Vector3(34,1.8,-70),Vector3(40,1.1,-77))
	game.isolation.stations.coil_light.use_station()
	await _frames(24)
	await _shot("isolation_drawer",Vector3(19.2,1.25,-79.1),Vector3(18.3,.65,-80.5))
	game.isolation.pursuer.set_active(true)
	await _shot("isolation_pursuer",Vector3(22,1.9,-85),Vector3(23,1.1,-90))
	await _shot("isolation_generator",Vector3(42,1.9,-96.5),Vector3(37,1.3,-104))
	game.isolation.circuit = "dark"
	game.isolation._apply_power()
	await _shot("isolation_dark",Vector3(50,1.8,-69),Vector3(52,1,-82))
	game.hero.reset_to(Vector3(0,.1,11))
	game.hero.set_physics_process(true)
	game.combat.acquire()
	game.combat.shotgun_owned = true
	game.combat.select_weapon("shotgun")
	Input.action_press("aim")
	await _frames(20)
	game.combat.update(.016)
	await _shot("isolation_shotgun",Vector3(.95,1.8,9.5),Vector3(.15,1.3,10.5))
	Input.action_release("aim")
	game.combat.overlay.hide()
	game.isolation.proof = true
	game.diagnostics.evidence.assign(["memo","log","office"])
	game.diagnostics.show_ending()
	await _shot("isolation_ending",Vector3(.95,1.8,9.5),Vector3(.15,1.3,10.5))
	game.queue_free()
	await process_frame
	print("ISOLATION PREVIEW COMPLETE")
	quit()
func _shot(id: String,at: Vector3,target: Vector3) -> void:
	camera.position = at
	camera.look_at(target)
	await _frames(12)
	await RenderingServer.frame_post_draw
	root.get_texture().get_image().save_png("res://artifacts/"+id+".png")
func _frames(count: int) -> void:
	for i: int in range(count): await physics_frame; await process_frame
