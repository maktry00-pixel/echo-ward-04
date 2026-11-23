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
	await _save("ward_gameplay")
	inspection = Camera3D.new()
	game.add_child(inspection)
	inspection.current = true
	inspection.fov = 62
	game.hud.hide()
	game.combat.overlay.hide()
	game.combat.set_process(false)
	game.combat.ammo_label.hide()
	game.survival_tools.status.hide()
	await _shot("ward_room",Vector3(4.5,2.7,.5),Vector3(10,.8,6.3))
	await _shot("reception_room",Vector3(1.5,2.2,9),Vector3(8,.8,14.3))
	await _shot("reception_desk",Vector3(-7.4,2.1,10),Vector3(-10.5,.9,14))
	await _shot("ward_corridor",Vector3(.4,1.7,12),Vector3(0,1.4,-1))
	await _shot("residential_room",Vector3(88,2.3,23),Vector3(95,.9,29))
	game.queue_free()
	await process_frame
	print("ROOM PREVIEWS COMPLETE")
	quit()
func _shot(name: String, at: Vector3, target: Vector3) -> void:
	game.hero.reset_to(at-Vector3(0,1.8,0))
	game.hero.camera_rig.flashlight_enabled = false
	inspection.position = at
	inspection.look_at(target)
	await _frames(20)
	await _save(name)
func _frames(count: int) -> void:
	for i: int in range(count):
		await physics_frame
		await process_frame
func _save(name: String) -> void:
	await RenderingServer.frame_post_draw
	print("ROOM ",name," ",root.get_texture().get_image().save_png("res://artifacts/"+name+".png"))
