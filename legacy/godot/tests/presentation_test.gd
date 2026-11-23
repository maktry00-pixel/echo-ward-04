extends SceneTree
const Prefs=preload("res://scripts/presentation_settings.gd")
const Saves=preload("res://scripts/checkpoint_store.gd")
var game:Node3D
var checks:int=0
var failures:int=0
func _initialize()->void:call_deferred("_run")
func _run()->void:
	game=load("res://scenes/third_person.tscn").instantiate();game.set_meta("automated_test",true);root.add_child(game)
	await _frames(16);_freeze()
	var menu:CanvasLayer=game.front_end
	game.show_title_menu();await _frames(3)
	_check(root.content_scale_aspect==Window.CONTENT_SCALE_ASPECT_EXPAND,"Display expands to screen proportions without letterboxing")
	_check(menu.new_game_button.text=="Новая игра" and menu.load_button.text=="Загрузить игру","Main actions use requested New game / Load game labels")
	_check(menu.maps_button.get_parent()==menu.game_button.get_parent() and menu.online_button.get_parent()==menu.about_button.get_parent(),"Future modes share the main horizontal action row")
	_check(menu.maps_button.disabled and menu.online_button.disabled and menu.maps_button.get_child(0).text=="В РАЗРАБОТКЕ","Future mode badges remain explicit and disabled")
	_check(menu.difficulty_picker.item_count==4,"Four real difficulty profiles are offered")
	menu.game_button.pressed.emit();menu.new_game_button.pressed.emit();menu.difficulty_picker.item_selected.emit(3);menu.start_button.pressed.emit();await _frames(4);_freeze()
	_check(game.difficulty_id=="hardcore" and game.difficulty_value("ammo")==4 and game.difficulty_value("escape")==150.0,"Hardcore changes supply balance, attacks and final escape time")
	_check(game.saves.capture("manual_1") and Saves.valid(game.saves.slots.manual_1),"Hardcore campaign remains compatible with manual saving")
	game.show_title_menu();menu.open_settings()
	_check(menu.settings_view.sections.size()==6 and menu.settings_view.controls.size()>=24,"Settings are grouped into six working categories")
	var settings:PanelContainer=menu.settings_view
	settings.select_section("Освещение");settings.controls.brightness.value=1.2;settings.controls.fog.value=.6;settings.controls.ambient.value=1.3
	_check(is_equal_approx(game.presentation.material.get_shader_parameter("brightness"),1.2),"Brightness slider drives actual scene postprocessing")
	_check(is_equal_approx(game.presentation.environment.fog_density,.009*.6) and is_equal_approx(game.presentation.environment.ambient_light_energy,.16*1.3),"Fog and ambient sliders drive the real WorldEnvironment")
	settings.controls.flashlight_strength.value=.7
	_check(is_equal_approx(game.hero.camera_rig.flashlight.light_energy,2.8),"Flashlight strength changes actual light energy")
	settings.controls.msaa.item_selected.emit(1)
	_check(root.msaa_3d==Viewport.MSAA_2X,"Antialiasing picker controls the actual viewport")
	settings.controls.shadow_quality.item_selected.emit(0)
	var disabled:bool=true
	for ref:WeakRef in game.presentation.lights:
		var light:Light3D=ref.get_ref()
		if light!=null:disabled=disabled and not light.shadow_enabled
	_check(disabled,"Shadow Off disables authored realtime shadows")
	settings.controls.shadow_quality.item_selected.emit(1)
	_check(root.positional_shadow_atlas_size==2048,"Medium shadow quality changes the atlas resolution")
	settings.controls.fov.value=76;settings.controls.shake.value=.25;settings.controls.invert_y.button_pressed=true
	_check(game.hero.camera_rig.base_fov==76 and game.hero.camera_rig.shake_scale==.25,"FOV and shake sliders reach the live camera")
	var before:float=game.hero.camera_rig.rotation.x;game.hero.camera_rig.apply_look(Vector2(0,5))
	_check(game.hero.camera_rig.rotation.x>before,"Invert Y reverses actual mouse pitch movement")
	settings.controls.show_noise.button_pressed=false;settings.controls.objective_hint.button_pressed=false
	_check(not game.hud.noise_panel.visible and not game.hud.hint_label.visible,"Interface switches hide the intended HUD elements")
	settings.controls.grain.button_pressed=false
	_check(game.presentation.material.get_shader_parameter("grain")==false,"Film grain switch controls shader output")
	var stored:Dictionary=JSON.parse_string(JSON.stringify(game.settings))
	var restored:Dictionary=game.normalise_settings(stored)
	var same:bool=restored.size()==game.settings.size()
	for key:String in game.settings:
		same=same and (is_equal_approx(float(restored[key]),float(game.settings[key])) if game.settings[key] is float else restored[key]==game.settings[key])
	_check(same,"All presentation settings round trip through the persisted JSON schema")
	var bad:Dictionary=game.normalise_settings({"msaa":999,"fog":NAN,"language":17,"shadow_quality":"unknown","fov":1000})
	_check(bad.msaa==2 and bad.fog==1.0 and bad.language=="ru" and bad.shadow_quality=="high" and bad.fov==85,"Malformed settings are rejected or bounded safely")
	game.apply_settings(Prefs.DEFAULTS)
	for kind:String in ["training","controls","about"]:
		menu._show_info(kind)
		_check(menu.info.visible and not menu.menu_content.visible,"Illustrated page owns the screen: "+kind)
		for index:int in range(menu.info.pages.size()):
			menu.info.select_page(index);await _frames(1)
			var picture:TextureRect=menu.info.body.get_child(0)
			_check(picture.texture!=null and picture.texture.get_width()>=1280,"Actual in-game photograph loads for "+kind+" page "+str(index))
		var event:=InputEventKey.new();event.physical_keycode=KEY_ESCAPE;event.pressed=true;menu._input(event)
		_check(menu.menu_content.visible and menu.page_mode=="main","Esc closes page and restores main menu: "+kind)
	menu.language_picker.item_selected.emit(1);menu._show_info("training");await _frames(2)
	_check(menu.info.nav_buttons[0].text=="Алғашқы қадамдар","Tutorial navigates with Kazakh text")
	menu.language_picker.item_selected.emit(2);await _frames(2)
	_check(menu.info.nav_buttons[0].text=="First steps","Existing tutorial updates to English without reopening")
	menu._back_to_main();game.apply_settings(Prefs.DEFAULTS);game._new_game();await _frames(8);_freeze()
	game.health=1;game.hero.set_physics_process(false);game.hero.stamina=55;await _frames(3)
	_check(game.hud.health_bar.value==1 and game.hud.breath_bar.value==55,"HUD bars show real health and breath")
	_check(game.survival_tools.status.get_parent()==game.hud.staging and not game.hud.staging.visible,"Old standalone telemetry cannot appear over the new layout")
	_check(game.combat.ammo_label==game.hud.weapon_label,"Ammunition is owned by the unified weapon card")
	game.hud.noise_level=0;game.emit_noise(game.hero.position,14);await _frames(2)
	_check(game.hud.noise_level>.8 and game.hud.noise_label.text=="ГРОМКО","Noise meter responds to a real local noise event")
	game.hud.noise_level=0;game.emit_noise(game.hero.position+Vector3(20,0,0),14)
	_check(game.hud.noise_level==0,"Remote noises do not masquerade as player noise")
	game.hud.set_interaction("");_check(not game.hud.interaction_panel.visible,"Context prompt disappears when no action is available")
	game.hud.set_interaction("F — открыть дверь",game.facility.gate_by_id.power_gate)
	_check(game.hud.interaction_panel.visible and game.hud.interaction_label.text.contains("F"),"Available object shows its contextual F action")
	var cabinet:Node3D=game.mission.objects.fuse
	game.hero.reset_to(cabinet.interaction_point());game.hero.invulnerable_remaining=0
	await _frames(5);game.interact();await _frames(32)
	_check(cabinet.storage.opened and not cabinet.storage.label.visible and game.hud.context_extra.visible,"Open cabinet search details belong to the contextual panel, without floating world text")
	cabinet.storage.selected=cabinet.storage.reward
	Input.action_press("interact_hold");await _frames(20);Input.action_release("interact_hold")
	_check(cabinet.storage.progress>0 and game.hud.context_extra.text.contains("%"),"Held F advances real search progress inside the object prompt")
	cabinet.storage.close_drawer();await _frames(2)
	_check(not game.hud.context_extra.visible,"Closing the cabinet removes compartment details")
	var data:Dictionary=game.settings.duplicate();data.subtitles=false;game.apply_settings(data)
	game.hud.announce("ЗАПИСЬ / test",5,true);await _frames(2)
	_check(not game.hud.story_panel.visible,"Subtitles option suppresses recording captions")
	game.hud.announce("test objective",5);await _frames(2)
	_check(game.hud.story_panel.visible,"Subtitles option does not suppress gameplay announcements")
	game.queue_free();await process_frame
	print("PRESENTATION AND HUD: %d checks, %d failures"%[checks,failures]);quit(1 if failures else 0)
func _freeze()->void:
	for actor:CharacterBody3D in game.hunters:actor.set_active(false);actor.set_physics_process(false)
func _frames(count:int)->void:
	for i:int in range(count):await physics_frame;await process_frame
func _check(ok:bool,message:String)->void:
	checks+=1
	if not ok:failures+=1;push_error("FAIL: "+message)
