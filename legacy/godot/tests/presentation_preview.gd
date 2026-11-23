extends SceneTree
var game:Node3D
var evidence:Dictionary={}
func _initialize()->void:call_deferred("_run")
func _run()->void:
	root.content_scale_size=Vector2i(1600,900);root.content_scale_aspect=Window.CONTENT_SCALE_ASPECT_EXPAND;root.size=Vector2i(1600,900)
	game=load("res://scenes/third_person.tscn").instantiate();game.set_meta("automated_test",true);root.add_child(game)
	await _frames(20);_freeze()
	game.show_title_menu();await _shot("presentation_menu_ru")
	game.front_end.language_picker.item_selected.emit(2);await _shot("presentation_menu_en")
	game.front_end.language_picker.item_selected.emit(1);await _shot("presentation_menu_kk")
	game.front_end.language_picker.item_selected.emit(0)
	game.front_end.open_settings()
	for name:String in ["Графика","Освещение","Интерфейс"]:
		game.front_end.settings_view.select_section(name)
		await _shot("presentation_settings_"+str(game.front_end.settings_view.GROUPS.find(name)))
	game.front_end._show_info("about");await _shot("presentation_about")
	game.front_end.info.select_page(2);await _shot("presentation_materials")
	game.front_end._show_info("training");game.front_end.info.select_page(1);await _shot("presentation_training_search")
	game.front_end.info.select_page(6);await _shot("presentation_training_save")
	game.front_end._show_info("controls");game.front_end.info.select_page(1);await _shot("presentation_controls")
	game.front_end._back_to_main()
	root.size=Vector2i(1920,820);await _shot("presentation_ultrawide")
	evidence.ultrawide={"window":str(root.size),"viewport":str(root.get_visible_rect().size),"aspect_mode":root.content_scale_aspect}
	root.size=Vector2i(1200,900);await _shot("presentation_4by3")
	root.size=Vector2i(1600,900)
	game._new_game();await _frames(12);_freeze();game.hero.reset_to(Vector3(0,.1,11));await _shot("presentation_hud")
	game.combat.acquire();game.hero.reset_to(Vector3(0,.1,9));game.hero.camera_rig.rotation.y=0
	game.hunters[0].position=Vector3(0,.1,3);game.hunters[0].set_active(true);Input.action_press("aim");game.combat.update(.1)
	await _shot("presentation_combat");Input.action_release("aim")
	_freeze()
	var cabinet:Node3D=game.mission.objects.fuse
	game.hero.reset_to(cabinet.interaction_point());game.hero.camera_rig.rotation=Vector3(-.25,cabinet.rotation.y,0);game.hero.invulnerable_remaining=0
	await _frames(6);await _shot("presentation_prompt")
	game.interact();await _frames(32);cabinet.storage.selected=cabinet.storage.reward
	Input.action_press("interact_hold");await _frames(18);await _shot("presentation_search");Input.action_release("interact_hold")
	cabinet.storage.close_drawer()
	game.front_end.language_picker.item_selected.emit(1);await _shot("presentation_hud_kk")
	game.front_end.language_picker.item_selected.emit(2);await _shot("presentation_hud_en")
	game.front_end.language_picker.item_selected.emit(0)
	game.show_title_menu()
	DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_FULLSCREEN)
	await _frames(20);await _shot("presentation_fullscreen")
	evidence.fullscreen={"window":str(DisplayServer.window_get_size()),"screen":str(DisplayServer.screen_get_size(DisplayServer.window_get_current_screen())),"mode":DisplayServer.window_get_mode(),"aspect_mode":root.content_scale_aspect}
	DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_WINDOWED)
	var file:=FileAccess.open("res://artifacts/presentation_display.json",FileAccess.WRITE);file.store_string(JSON.stringify(evidence,"\t"));file.close()
	game.queue_free();await process_frame;print("PRESENTATION PREVIEW COMPLETE");quit()
func _freeze()->void:
	for actor:CharacterBody3D in game.hunters:actor.set_active(false);actor.set_physics_process(false)
func _shot(id:String)->void:
	await _frames(12);await RenderingServer.frame_post_draw
	root.get_texture().get_image().save_png("res://artifacts/"+id+".png")
func _frames(count:int)->void:
	for i:int in range(count):await physics_frame;await process_frame
