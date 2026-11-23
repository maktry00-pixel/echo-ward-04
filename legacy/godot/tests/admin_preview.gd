extends SceneTree
var game:Node3D
func _initialize()->void:call_deferred("_run")
func _run()->void:
	root.size=Vector2i(1600,900)
	game=load("res://scenes/third_person.tscn").instantiate();game.set_meta("automated_test",true);root.add_child(game)
	await _frames(18);_freeze();game.show_title_menu();game.front_end.open_settings()
	await _shot("settings")
	game.front_end.settings_view.admin_button.pressed.emit();await _shot("password")
	game.admin.password.text="0890";game.admin.submit_password();await _frames(5)
	game.admin.camera.position=Vector3(0,3.4,9);game.admin.camera.look_at(Vector3(0,1,1))
	await _shot("flight")
	var cabinet:Node3D=game.mission.objects.fuse
	game.admin.camera.position=cabinet.interaction_point()+Vector3(0,1.55,0);game.admin.camera.rotation=Vector3(-.2,cabinet.rotation.y,0);await _frames(6)
	game.interact();await _frames(32);cabinet.storage.selected=cabinet.storage.reward
	Input.action_press("interact_hold");await _frames(20);await _shot("furniture");Input.action_release("interact_hold");cabinet.storage.close_drawer()
	game.admin.pilot_button.pressed.emit();await _frames(160);await _shot("agent")
	game.admin.pilot_button.pressed.emit();game.front_end.language_picker.item_selected.emit(1);await _shot("flight_kk")
	game.front_end.language_picker.item_selected.emit(2);await _shot("flight_en")
	game.front_end.language_picker.item_selected.emit(0);game.admin.leave();await _frames(6)
	game.saves.capture("manual_4");game.health=2;game.saves.capture("manual_6");game.saves.open_browser("load");await _shot("slots")
	game.saves.screen.dismiss();game.health=0;game._finish_round(false,"preview fixture");await _frames(8);await _shot("respawn")
	game.queue_free();await process_frame;print("ADMIN PREVIEW COMPLETE");quit()
func _freeze()->void:
	for actor:CharacterBody3D in game.hunters:actor.set_active(false);actor.set_physics_process(false)
func _frames(count:int)->void:
	for i:int in range(count):await physics_frame;await process_frame
func _shot(id:String)->void:
	await _frames(8);await RenderingServer.frame_post_draw
	root.get_texture().get_image().save_png("res://artifacts/admin_"+id+".png")
