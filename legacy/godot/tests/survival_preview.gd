extends SceneTree
var game: Node3D
func _initialize() -> void: call_deferred("_run")
func _run() -> void:
	root.size = Vector2i(1280,900)
	game = load("res://scenes/third_person.tscn").instantiate()
	game.set_meta("automated_test",true)
	root.add_child(game)
	await _frames(20)
	for hunter: CharacterBody3D in game.hunters: hunter.set_active(false)
	game.hero.reset_to(Vector3(-9,.1,8.7))
	game.hero.camera_rig.rotation.y = -.15
	await _frames(30)
	await _save("survival_ward")
	var bed: Node3D = get_nodes_in_group("hide_beds")[0]
	game.hero.reset_to(bed.global_position+Vector3(1.2,.1,0))
	await _frames(10)
	print("HIDE PREVIEW ",game.survival_tools.try_hide(bed))
	game.hero.camera_rig.rotation.y = -PI/2
	game.hero.camera_rig.rotation.x = -.05
	var crawler: CharacterBody3D = game.hunters[0]
	crawler.position = bed.global_position+Vector3(3,.1,-.2)
	crawler.set_active(true)
	crawler.set_physics_process(false)
	crawler.model.rotation.y = PI/2
	await _frames(25)
	await _save("under_bed")
	crawler.set_active(false)
	game.survival_tools.leave_bed()
	game.hero.reset_to(Vector3(0,.1,11))
	game.hero.model.rotation.y = PI*.8
	crawler.position = Vector3(-.6,.1,6)
	crawler.model.rotation.y = PI
	crawler.set_active(true)
	var spitter: CharacterBody3D = game.hunters[3]
	spitter.position = Vector3(1.1,.1,4)
	spitter.model.rotation.y = PI
	spitter.set_active(true)
	spitter.set_physics_process(false)
	await _frames(35)
	await _save("aberrations")
	Input.action_press("crouch")
	Input.action_press("move_forward")
	await _frames(30)
	await _save("survivor_crouch")
	Input.action_release("crouch")
	Input.action_release("move_forward")
	game.hero.reset_to(Vector3(0,.1,11))
	game.hero.model.rotation.y = PI*.8
	await _frames(25)
	# Close character inspection uses a separate physical camera; gameplay keeps shoulder view.
	var camera := Camera3D.new()
	game.get_node("World").add_child(camera)
	camera.position = Vector3(1.7,1.45,12.8)
	camera.look_at(game.hero.position+Vector3(0,1.0,0))
	camera.fov = 40
	camera.current = true
	game.hud.hide()
	game.combat.overlay.hide()
	game.hero.model.show()
	await _frames(25)
	await _save("survivor_detail")
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
