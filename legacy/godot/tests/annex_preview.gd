extends SceneTree
var game: Node3D
var camera: Camera3D
func _initialize() -> void: call_deferred("_run")
func _run() -> void:
	root.size = Vector2i(1280,800)
	game = load("res://scenes/third_person.tscn").instantiate(); game.set_meta("automated_test",true); root.add_child(game)
	await _frames(18)
	game._new_game()
	for actor: CharacterBody3D in game.hunters: actor.set_active(false); actor.set_physics_process(false)
	game.set_physics_process(false); game.hero.set_physics_process(false)
	game.hero.position = Vector3(50,.1,-120)
	game.hud.hide(); game.combat.overlay.hide(); game.upgrades.status.hide(); game.survival_tools.status.hide()
	camera = Camera3D.new(); game.add_child(camera); camera.current = true; camera.fov = 65
	await _shot("annex_gallery",Vector3(50,1.8,-111),Vector3(50,1.2,-144))
	await _shot("annex_pump",Vector3(57,1.9,-129.5),Vector3(65,1.1,-140))
	await _shot("annex_boiler",Vector3(43,1.9,-151.5),Vector3(30,1.2,-162))
	await _shot("annex_archive",Vector3(80,1.8,-153),Vector3(96,1.3,-168))
	await _shot("annex_radio",Vector3(43,1.8,-178),Vector3(31,1.2,-189))
	game.mission.power_on = true; game.mission.has_badge = true; game.mission.purge_complete = true
	game.residential.complete = true; game.diagnostics.complete = true; game.isolation.complete = true
	game.annex.steps.assign(["brief","wheel","manual","intake","drain","pump"]); game.annex.sync()
	game.hero.reset_to(Vector3(37,.1,-154)); game.hero.camera_rig.rotation = Vector3(-.1,0,0)
	camera.current = false; game.hero.camera_rig.camera.current = true; game.hero.camera_rig.resolve_collision()
	game.hud.show(); game.mission.refresh_hud(); game.exploration.update(game.hero.position)
	await _frames(12); await RenderingServer.frame_post_draw
	root.get_texture().get_image().save_png("res://artifacts/annex_gameplay.png")
	for room: Dictionary in game.exploration.ROOMS: game.exploration.visited[room.id] = true
	game.hud.radar.show(); await _frames(12); await RenderingServer.frame_post_draw
	root.get_texture().get_image().save_png("res://artifacts/annex_map.png")
	game.hud.hide(); game.front_end.show(); game.front_end._show_info("training"); game.front_end.info.select_page(7)
	await _frames(12); await RenderingServer.frame_post_draw
	root.get_texture().get_image().save_png("res://artifacts/annex_guide.png")
	game.queue_free(); await process_frame; print("ANNEX PREVIEW COMPLETE"); quit()
func _shot(id: String,at: Vector3,target: Vector3) -> void:
	camera.position = at; camera.look_at(target); await _frames(12); await RenderingServer.frame_post_draw
	root.get_texture().get_image().save_png("res://artifacts/"+id+".png")
func _frames(count: int) -> void:
	for i: int in range(count): await physics_frame; await process_frame
