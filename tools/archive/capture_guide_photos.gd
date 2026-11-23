extends SceneTree
var game:Node3D
var camera:Camera3D
func _initialize()->void:call_deferred("_run")
func _run()->void:
	root.content_scale_size=Vector2i(1280,720)
	root.size=Vector2i(1280,720)
	game=load("res://scenes/third_person.tscn").instantiate()
	game.set_meta("automated_test",true)
	root.add_child(game)
	await _frames(20)
	for actor:CharacterBody3D in game.hunters:actor.set_active(false);actor.set_physics_process(false)
	game.set_physics_process(false)
	game.hud.hide();game.combat.overlay.hide();game.survival_tools.status.hide();game.upgrades.status.hide();game.isolation.status.hide()
	game.hero.reset_to(Vector3(0,.1,11))
	game.hero.set_physics_process(false)
	camera=Camera3D.new();game.add_child(camera);camera.current=true;camera.fov=62
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://assets/ui/guide"))
	await _shot("exploration",Vector3(2.5,1.8,14),Vector3(0,1.1,4))
	game.combat.acquire();Input.action_press("aim");game.hero.set_physics_process(true)
	game.combat.update(.1)
	game.combat.overlay.hide()
	var actor:CharacterBody3D=game.hunters[0]
	actor.position=Vector3(0,.1,4);actor.set_active(true)
	await _shot("combat",Vector3(1.5,1.75,12),Vector3(-.1,1.1,4))
	Input.action_release("aim");actor.set_active(false)
	game.hero.set_physics_process(false);game.hero.camera_rig.set_process(false);game.hero.aiming=false
	game.hero.model.hide();game.combat.weapon.hide()
	await _shot("hiding",Vector3(4.5,1.4,.5),Vector3(10,.7,6.3))
	var storage:Node3D=game.mission.objects.fuse.storage
	game.hero.reset_to(storage.interaction_point());game.hero.model.hide()
	storage.interact();await _frames(32);storage.set_process(false);storage.set_physics_process(false);storage.label.hide()
	var at:Vector3=storage.prop.position
	await _shot("search",at+Vector3(1.1,1.45,1.6),at+Vector3(0,.55,.2))
	await _shot("saving",Vector3(8,1.65,19.6),Vector3(6.3,.9,22))
	await _shot("medicine",Vector3(28,2.2,-27),Vector3(21,1.1,-34.5))
	await _shot("generator",Vector3(42,1.9,-96.5),Vector3(37,1.3,-104))
	await _shot("ward",Vector3(22,1.75,-66),Vector3(25,1,-77))
	game.queue_free();await process_frame
	print("GUIDE PHOTOS COMPLETE: eight original in-game photographs")
	quit()
func _shot(id:String,at:Vector3,target:Vector3)->void:
	camera.position=at;camera.look_at(target)
	await _frames(12);await RenderingServer.frame_post_draw
	root.get_texture().get_image().save_jpg("res://assets/ui/guide/"+id+".jpg",.92)
func _frames(count:int)->void:
	for i:int in range(count):await process_frame
