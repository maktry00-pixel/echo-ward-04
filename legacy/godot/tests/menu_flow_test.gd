extends SceneTree
const Saves=preload("res://scripts/checkpoint_store.gd")
var game:Node3D
var menu:CanvasLayer
var checks:int=0
var failures:int=0
var preview:bool=false
func _initialize()->void:call_deferred("_run")
func _run()->void:
	preview=OS.get_cmdline_user_args().has("--preview")
	if preview:root.size=Vector2i(1600,900)
	game=load("res://scenes/third_person.tscn").instantiate();game.set_meta("automated_test",true);root.add_child(game)
	await _frames(16);_freeze();menu=game.front_end
	game.saves.slots.clear();game.checkpoint.clear();game.show_title_menu();await _frames(2)
	_check(menu.main_actions.get_child_count()==7,"Main menu has exactly seven peer actions")
	_check(menu.game_button.is_visible_in_tree() and not menu.difficulty_picker.is_visible_in_tree(),"Main screen exposes Game and hides difficulty")
	await _shot("main_ru")
	menu.game_button.pressed.emit();await _frames(2)
	_check(menu.page_mode=="game" and menu.new_game_button.is_visible_in_tree() and menu.load_button.is_visible_in_tree() and menu.all_saves_button.is_visible_in_tree(),"Game opens New game / Load game / All saves")
	_check(menu.game_view.get_global_rect().end.y<=root.get_visible_rect().size.y,"Nested game panel stays fully inside the screen after text wrapping")
	_check(menu.load_button.disabled and not menu.all_saves_button.disabled,"No phantom latest load; empty save list remains accessible")
	await _shot("game_empty")
	menu.all_saves_button.pressed.emit();await _frames(2)
	_check(game.saves.screen.mode=="load" and game.saves.screen.slot_buttons.size()==9,"All saves opens six manual and three auto slots")
	var escape:=InputEventKey.new();escape.physical_keycode=KEY_ESCAPE;escape.pressed=true
	menu._input(escape)
	_check(menu.page_mode=="game","Menu does not consume archive Esc or discard its parent screen")
	game.saves.screen.mode_buttons.save.pressed.emit();game.saves.screen.slot_buttons.manual_1.pressed.emit()
	game.saves.screen._input(escape);await _frames(2)
	_check(game.saves.screen.mode=="closed" and menu.page_mode=="game" and game.is_paused,"Archive closes back into Game without unpausing the title screen")
	_check(not menu.load_button.disabled and game.saves.slots.has("manual_1"),"First save made inside the archive immediately enables Load game on return")
	game.saves.slots.clear();game.checkpoint.clear();menu.open_game()
	var before:float=game.elapsed
	menu.new_game_button.pressed.emit();await _frames(2)
	_check(menu.page_mode=="difficulty" and menu.difficulty_picker.is_visible_in_tree() and game.in_title_menu and game.elapsed==before,"New game reveals difficulty without starting simulation")
	menu.difficulty_picker.item_selected.emit(2)
	_check(game.difficulty_id=="normal" and game.settings.difficulty=="nightmare","Pending difficulty does not alter the current campaign")
	await _shot("difficulty")
	menu._input(escape)
	_check(menu.page_mode=="game" and not menu.difficulty_picker.is_visible_in_tree(),"Esc from difficulty returns one level to Game")
	menu.new_game_button.pressed.emit();menu.start_button.pressed.emit();await _frames(6);_freeze()
	_check(not game.in_title_menu and game.difficulty_id=="nightmare","Start confirms the chosen profile and begins a real campaign")
	game.health=3;_check(game.saves.capture("manual_1"),"First manual fixture is a real validated save")
	game.saves.slots.manual_1.saved_at=100.0
	game.health=1;_check(game.saves.capture("auto_2"),"Autosave fixture uses the same real snapshot path")
	game.saves.slots.auto_2.saved_at=200.0
	game.health=2;_check(game.saves.capture("manual_3"),"Second manual slot remains separate")
	game.saves.slots.manual_3.saved_at=300.0
	_check(game.saves.latest_key()=="manual_3","Latest save is selected by timestamp across manual and auto slots")
	game.show_title_menu();menu.game_button.pressed.emit();menu.new_game_button.pressed.emit();menu.difficulty_picker.item_selected.emit(0);menu._input(escape)
	menu.load_button.pressed.emit();await _frames(6);_freeze()
	_check(not game.in_title_menu and game.health==2 and game.difficulty_id=="nightmare" and game.saves.screen.mode=="closed","Load game directly resumes newest save, preserving its own difficulty")
	game.saves.slots.auto_2.saved_at=400.0
	_check(game.saves.load_slot("manual_1"),"An older manual save can still be selected independently")
	await _frames(6);_freeze()
	game.show_title_menu();menu.game_button.pressed.emit();menu.load_button.pressed.emit();await _frames(6);_freeze()
	_check(game.health==1 and game.saves.latest_key()=="auto_2","Loading an older slot does not replace the actual newest autosave")
	var count:int=game.saves.slots.size()
	game.show_title_menu();menu.game_button.pressed.emit();menu.all_saves_button.pressed.emit();await _frames(2)
	await _shot("saves")
	game.saves.screen.dismiss();menu._input(escape)
	_check(menu.page_mode=="main" and game.saves.slots.size()==count,"Returning through menus preserves every slot")
	menu.open_settings();menu.settings_view.select_section("Управление");menu.controls_button.pressed.emit();await _frames(2)
	_check(menu.page_mode=="controls" and menu.info.visible and not menu.settings_view.visible,"Settings contains the illustrated controls guide")
	await _shot("controls")
	menu._input(escape)
	_check(menu.page_mode=="settings" and menu.settings_view.active_section=="Управление","Controls guide returns to its previous Settings section")
	menu._input(escape)
	_check(menu.page_mode=="main","Settings Esc returns to main menu")
	for language:Array in [["en",2,"Game","All saves"],["kk",1,"Ойын","Барлық сақтаулар"]]:
		menu.language_picker.item_selected.emit(language[1]);await _frames(3)
		_check(menu.game_button.text==language[2] and menu.all_saves_button.text==language[3],"Nested menu labels translate in place: "+language[0])
		await _shot("main_"+language[0])
		menu.game_button.pressed.emit();await _shot("game_"+language[0]);menu._input(escape)
	menu.language_picker.item_selected.emit(0);await _frames(3)
	if preview:
		root.size=Vector2i(1200,900);await _shot("main_4by3")
		root.size=Vector2i(1920,820);await _shot("main_ultrawide")
	game.saves.slots.clear();game.checkpoint.clear();game.show_title_menu();menu.game_button.pressed.emit()
	_check(menu.load_button.disabled and not game.saves.load_latest(),"No valid progress never starts a phantom campaign")
	# The old single-file format first saved after restoring power, not at spawn.
	game.mission.power_on=true;game.facility.apply_power();game.mission.objects.power.mark_used()
	var legacy:Dictionary=Saves.snapshot(game)
	_check(Saves.valid(legacy),"Legacy fallback fixture is structurally valid")
	game.checkpoint=legacy;game.show_title_menu();menu.game_button.pressed.emit();menu.load_button.pressed.emit();await _frames(6);_freeze()
	_check(not game.in_title_menu,"Legacy single-checkpoint files remain loadable without slots")
	game.queue_free();await process_frame
	print("MENU FLOW: %d checks, %d failures"%[checks,failures]);quit(1 if failures else 0)
func _freeze()->void:
	for actor:CharacterBody3D in game.hunters:actor.set_active(false);actor.set_physics_process(false)
func _frames(count:int)->void:
	for i:int in range(count):await physics_frame;await process_frame
func _shot(id:String)->void:
	if not preview:return
	await _frames(10);await RenderingServer.frame_post_draw
	root.get_texture().get_image().save_png("res://artifacts/menu_flow_"+id+".png")
func _check(ok:bool,message:String)->void:
	checks+=1
	if not ok:failures+=1;push_error("FAIL: "+message)
