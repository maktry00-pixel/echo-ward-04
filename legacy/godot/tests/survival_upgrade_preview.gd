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
	for hunter: CharacterBody3D in game.hunters: hunter.set_active(false); hunter.set_physics_process(false)
	game.show_title_menu()
	game.front_end.difficulty_picker.item_selected.emit(2)
	await _frames(5)
	await _save("upgrade_difficulty")
	game.front_end.hide()
	game.in_title_menu = false
	game.set_paused(false)
	game.hud.show()
	game.hero.reset_to(Vector3(0,.1,11))
	game.combat.acquire()
	game.combat.shotgun_owned = true
	game.combat.shotgun_loaded = 3
	game.combat.shells = 4
	game.combat.select_weapon("shotgun")
	Input.action_press("aim")
	await _frames(20)
	camera = Camera3D.new()
	game.add_child(camera)
	camera.current = true
	camera.fov = 48
	game.hud.hide()
	game.combat.overlay.hide()
	game.survival_tools.status.hide()
	game.upgrades.status.hide()
	await _shot("upgrade_shotgun",Vector3(.95,1.80,9.5),Vector3(.15,1.30,10.5))
	game.combat.shotgun_loaded = 1
	game.combat.reload_weapon()
	await _frames(24)
	await _shot("upgrade_reload",Vector3(.85,1.6,9.8),Vector3(.05,1.15,10.6))
	Input.action_release("aim")
	game.combat.reload_remaining = 0
	game.combat.shotgun_reloading = false
	var crawler: CharacterBody3D = game.hunters[0]
	crawler.position = Vector3(23,.08,-28)
	crawler.model.rotation.y = .5
	crawler.set_active(true)
	await _shot("upgrade_creature",Vector3(20,1.25,-31.2),Vector3(23,.65,-28))
	crawler.set_active(false)
	game.upgrades.ceiling.reset_hunter()
	game.upgrades.ceiling.set_active(true)
	await _shot("upgrade_ceiling",Vector3(31,1.65,-17.8),Vector3(29,2.75,-20))
	var door: Node3D = game.facility.gate_by_id.research_37
	door.restore_state(false,false)
	door.set_barricade(3,1)
	await _shot("upgrade_barricade",Vector3(39,1.7,-5.8),Vector3(37,1.5,-9))
	game.hero.reset_to(game.refuge.SPAWN)
	await _frames(4)
	game.hud.show()
	game.refuge.stash.shells = 6
	game.refuge.stash.boards = 2
	game.refuge.screen.open_view("stash")
	await _frames(6)
	await _save("upgrade_stash")
	game.refuge.screen.dismiss()
	game.queue_free()
	await process_frame
	print("SURVIVAL UPGRADE PREVIEW COMPLETE")
	quit()
func _shot(id: String,at: Vector3,target: Vector3) -> void:
	camera.position = at
	camera.look_at(target)
	await _frames(12)
	await _save(id)
func _save(id: String) -> void:
	await RenderingServer.frame_post_draw
	root.get_texture().get_image().save_png("res://artifacts/"+id+".png")
func _frames(count: int) -> void:
	for i: int in range(count): await physics_frame; await process_frame
