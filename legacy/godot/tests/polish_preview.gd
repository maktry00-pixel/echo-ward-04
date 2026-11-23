extends SceneTree
var game: Node3D
var inspection: Camera3D
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
	game.hero.reset_to(Vector3(9,.1,3.7))
	game.hero.camera_rig.rotation = Vector3(-.20,PI,0)
	game.hero.model.rotation.y = PI
	await _frames(20)
	await _save("polish_ward_gameplay")
	inspection = Camera3D.new()
	game.add_child(inspection)
	inspection.current = true
	inspection.fov = 62
	game.hud.hide()
	game.combat.overlay.hide()
	game.survival_tools.status.hide()
	await _shot("polish_ward",Vector3(4.5,2.5,.5),Vector3(10,.9,6.3))
	await _shot("polish_reception",Vector3(-3,2.1,10.3),Vector3(-10.0,1.0,14.0))
	await _shot("polish_refuge",Vector3(8.2,2.0,17.4),Vector3(8.5,1.0,22.3))
	game.hero.reset_to(Vector3(0,.1,11))
	game.combat.acquire()
	game.hero.camera_rig.rotation.y = 0
	Input.action_press("aim")
	await _frames(20)
	inspection.fov = 43
	inspection.position = Vector3(.8,1.65,9.9)
	inspection.look_at(Vector3(.10,1.3,10.7))
	await _frames(8)
	await _save("polish_grip")
	Input.action_release("aim")
	inspection.fov = 55
	game.hero.reset_to(Vector3(0,.1,12))
	Input.action_press("move_forward")
	for i: int in range(3):
		await _frames(16)
		inspection.position = game.hero.position+Vector3(1.6,1.5,-1.7)
		inspection.look_at(game.hero.position+Vector3(0,.85,0))
		await _save("polish_stride_"+str(i))
	Input.action_release("move_forward")
	game.hero.reset_to(game.refuge.SPAWN)
	await _frames(4)
	game.hud.show()
	game.inventory.take_supply("medkit")
	game.inventory.take_supply("battery")
	game.refuge.stash.ammo = 24
	game.refuge.screen.open_view("stash")
	await _frames(8)
	await _save("polish_stash")
	game.refuge.screen.dismiss()
	game.refuge.screen.open_view("save")
	await _frames(6)
	await _save("polish_journal")
	game.refuge.screen.dismiss()
	game.queue_free()
	await process_frame
	print("POLISH PREVIEWS COMPLETE")
	quit()
func _shot(name: String, at: Vector3, target: Vector3) -> void:
	inspection.position = at
	inspection.look_at(target)
	await _frames(15)
	await _save(name)
func _frames(count: int) -> void:
	for i: int in range(count):
		await physics_frame
		await process_frame
func _save(name: String) -> void:
	await RenderingServer.frame_post_draw
	print(name," ",root.get_texture().get_image().save_png("res://artifacts/"+name+".png"))
