extends SceneTree
var game: Node3D
func _initialize() -> void: call_deferred("_run")
func _run() -> void:
	root.size = Vector2i(1280,900)
	game = load("res://scenes/third_person.tscn").instantiate()
	game.set_meta("automated_test",true)
	root.add_child(game)
	await _frames(16)
	for enemy: CharacterBody3D in game.hunters: enemy.stun(500)
	game.hero.reset_to(Vector3(-10,.1,-5.3))
	await _frames(20)
	await _save("imported_shelves")
	var item: Node3D = game.mission.objects["fuse"]
	game.hero.reset_to(item.interaction_point())
	await _frames(15)
	game.interact()
	await _frames(32)
	await _save("search_drawer")
	Input.action_press("interact_hold")
	await _frames(35)
	await _save("search_hands")
	Input.action_release("interact_hold")
	game.hero.reset_to(Vector3(12,.1,-1.7))
	await _frames(20)
	game.interact()
	await _frames(32)
	await _save("archive_cabinet")
	game.hero.reset_to(Vector3(52,.1,3.5))
	await _frames(20)
	await _save("imported_bed")
	game.hero.reset_to(Vector3(0,.1,9))
	var enemy: CharacterBody3D = game.hunters[0]
	enemy.position = Vector3(0,.08,4)
	enemy.set_active(true)
	enemy.set_physics_process(false)
	enemy.model.animate(2.8)
	enemy.model.chasing = true
	await _frames(17)
	await _save("creature_gait_a")
	await _frames(11)
	await _save("creature_gait_b")
	enemy.receive_bullet(game.hero.position,100,true)
	await _frames(30)
	await _save("creature_fall")
	await _frames(55)
	await _save("creature_corpse")
	game.combat.acquire()
	Input.action_press("aim")
	game.combat.rounds = 5
	game.combat.reload_weapon()
	await _frames(44)
	await _save("reload_hands")
	Input.action_release("aim")
	await _frames(90)
	game._finish_round(false,"Постановочный кадр")
	game.hud.overlay.hide()
	await _frames(80)
	await _save("survivor_fall")
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
