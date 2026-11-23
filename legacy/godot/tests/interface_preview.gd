extends SceneTree
var game:Node3D
func _initialize()->void:call_deferred("_run")
func _run()->void:
	root.size=Vector2i(1280,900)
	game=load("res://scenes/third_person.tscn").instantiate()
	game.set_meta("automated_test",true)
	root.add_child(game)
	await _frames(20)
	for actor:CharacterBody3D in game.hunters:actor.set_active(false);actor.set_physics_process(false)
	game.saves.capture("manual_1")
	game.show_title_menu()
	await _shot("interface_menu_ru")
	var locale:Node=root.get_node("TextLocale")
	locale.set_language("en");await _shot("interface_menu_en")
	game.front_end.language_picker.select(1)
	game.front_end.language_picker.item_selected.emit(1)
	await _shot("interface_menu_kk")
	game.front_end.main_view.hide();game.front_end.settings_view.show()
	await _shot("interface_settings_kk")
	game.front_end.settings_view.hide();game.front_end.main_view.show()
	game.saves.open_browser("save")
	await _shot("interface_saves_kk")
	game.saves.screen.slot_buttons.manual_2.pressed.emit()
	await _shot("interface_saved_kk")
	game.saves.screen.dismiss()
	root.content_scale_size=Vector2i(1280,720)
	root.size=Vector2i(1280,720)
	await _shot("interface_menu_720")
	root.content_scale_size=Vector2i(1280,900)
	root.size=Vector2i(1280,900)
	game.continue_checkpoint()
	await _frames(10)
	game.set_paused(true)
	await _shot("interface_pause_kk")
	game.set_paused(false)
	game.hero.reset_to(Vector3(0,.1,11));game.hero.camera_rig.rotation.y=.1
	await _shot("interface_game_kk")
	game.hero.reset_to(game.saves.points[0].interaction_point());game.hero.camera_rig.rotation.y=0
	await _shot("interface_station_kk")
	game.mission.has_map=true
	for room:Dictionary in game.exploration.ROOMS:game.exploration.visited[room.id]=true
	game.hud.radar.show()
	await _shot("interface_map_kk")
	locale.set_language("ru")
	game.queue_free();await process_frame
	print("INTERFACE PREVIEW COMPLETE")
	quit()
func _shot(id:String)->void:
	await _frames(12)
	await RenderingServer.frame_post_draw
	root.get_texture().get_image().save_png("res://artifacts/"+id+".png")
func _frames(count:int)->void:
	for i:int in range(count):await process_frame
