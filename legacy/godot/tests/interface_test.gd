extends SceneTree
const Saves=preload("res://scripts/checkpoint_store.gd")
const Materials=preload("res://scripts/environment_materials.gd")
var game:Node3D
var checks:int=0
var failures:int=0
func _initialize()->void:call_deferred("_run")
func _run()->void:
	game=load("res://scenes/third_person.tscn").instantiate()
	game.set_meta("automated_test",true)
	root.add_child(game)
	await _frames(15)
	_freeze()
	var manager:Node=game.saves
	manager.file_root="res://artifacts/interface_test_saves"
	manager.allow_test_disk=true
	_clean()
	_check(manager.eligible(),"Grounded living campaign can be saved before power restoration")
	game.health=1;game.flash_charges=0
	game.combat.acquire();game.combat.rounds=4;game.combat.reserve=12
	game.hunters[0].position=Vector3(-8,.1,6)
	game.hunters[0].state=2
	game.hunters[0].last_known=Vector3(0,.1,10)
	var saved_at:Vector3=game.hero.position
	var key:=InputEventKey.new();key.physical_keycode=KEY_F5;key.pressed=true
	game._unhandled_input(key)
	_check(manager.slots.has("manual_1") and FileAccess.file_exists(manager.file_root+"/manual_1.json"),"Actual F5 writes a validated manual slot to disk")
	var data:Dictionary=manager.slots.get("manual_1",{})
	_check(Saves.valid(data) and data.get("exact_save",false),"Pre-power save round trips through strict checkpoint validation")
	_check(manager.indicator.panel.visible and manager.indicator.success,"Successful write displays the save icon")
	game.hero.reset_to(Vector3(4,.1,10));game.health=3;game.combat.reserve=0
	_check(manager.load_slot("manual_1"),"Manual slot loads through actual checkpoint system")
	_check(not game.hunters[0].active,"Exact save preserves inactive enemies without waking them on load")
	await _frames(5);_freeze()
	_check(game.hero.position.distance_to(saved_at)<.25,"Manual load restores original safe position")
	_check(game.health==1 and game.flash_charges==0 and game.combat.rounds==4 and game.combat.reserve==12,"Save preserves health, flash and ammunition without healing or gifts")
	_check(game.hunters[0].position.distance_to(Vector3(-8,.1,6))<.25 and game.hunters[0].state==2,"Manual load restores enemy location and pursuit state")
	_check(game.facility.gate_by_id.power_gate.locked and game.mission.events.is_empty(),"Loading before power cannot unlock the gate or skip unseen story encounters")
	game.mission.fire_event("first_contact")
	_check(game.hunters[0].active and game.mission.events.has("first_contact"),"Unseen first-contact encounter still activates normally after manual load")
	_freeze()
	var bad:Dictionary=data.duplicate(true);bad.resume.at=[999,0,0]
	_check(not Saves.valid(bad),"Out-of-bounds saved position rejected")
	bad=data.duplicate(true);bad.resume.pitch="bad"
	_check(not Saves.valid(bad),"Invalid camera data rejected")
	bad=data.duplicate(true);bad.enemies=13
	_check(not Saves.valid(bad),"Malformed enemy data rejected without crashing")
	bad=data.duplicate(true);bad.actors[0].known=[0,NAN,0]
	_check(not Saves.valid(bad),"Nonfinite AI location rejected")
	bad=data.duplicate(true);bad.resume.at=[-15,0,10]
	_check(Saves.apply(game,bad),"A structurally valid save may be checked against changed scenery")
	await _frames(5);_freeze()
	_check(game.hero.position.distance_to(Vector3(-15,0,10))>1 and _standing_clear(game.hero.position),"Blocked saved location falls back to a collision-free authored checkpoint")
	game.hero.reset_to(saved_at);await _frames(4)
	game.hero.performing_action=true
	_check(not manager.capture("manual_2"),"Cannot save midway through a mechanism")
	game.hero.performing_action=false
	game.mission.exit_started=true
	_check(not manager.capture("manual_2"),"Final timed escape cannot be bypassed with saves")
	game.mission.exit_started=false
	_check(not manager.capture("../outside"),"Slot names cannot traverse save directory")
	manager.open_browser("save")
	_check(game.is_paused and manager.screen.mode=="save" and not game.hud.overlay.visible,"Save browser pauses simulation and owns the interface")
	manager.screen.slot_buttons.manual_2.pressed.emit()
	_check(manager.slots.has("manual_2"),"Save screen button writes second manual slot")
	manager.screen.dismiss()
	_check(not game.is_paused,"Closing browser returns to gameplay")
	game.set_paused(true);game.hud.save_button.pressed.emit()
	_check(manager.screen.mode=="save","Pause menu Save opens real slot selection")
	manager.screen.dismiss()
	_check(game.is_paused and game.hud.overlay.visible,"Closing save browser restores the previous pause menu")
	game.set_paused(false)
	var preserved:Dictionary=manager.slots.manual_1.duplicate(true)
	for i:int in range(4):
		game.mission.power_on=true
		_check(game.save_checkpoint(),"Milestone autosave writes rotating slot %d"%i)
	_check(manager.slots.size()==5 and manager.slots.manual_1==preserved,"Autosaves rotate through three slots without touching manual progress")
	game.mission.power_on=false
	_check(manager.capture("manual_1"),"Second manual write creates a backup")
	var file:=FileAccess.open(manager.file_root+"/manual_1.json",FileAccess.WRITE);file.store_string("{broken");file.close()
	manager.slots.clear();manager.read_slots()
	_check(manager.slots.has("manual_1") and Saves.valid(manager.slots.manual_1),"Restart reads recoverable backup after corrupt primary file")
	var backup_text:String=FileAccess.get_file_as_string(manager.file_root+"/manual_1.json.bak")
	_check(manager.capture("manual_1") and FileAccess.get_file_as_string(manager.file_root+"/manual_1.json.bak")==backup_text,"Saving after recovery preserves valid backup instead of replacing it with corrupt primary")
	var good_root:String=manager.file_root
	manager.file_root="res://project.godot/invalid"
	var previous_checkpoint:Dictionary=game.checkpoint.duplicate(true)
	_check(not manager.capture("manual_3") and not manager.slots.has("manual_3") and not manager.indicator.success,"Failed disk write shows failure and creates no phantom slot")
	_check(game.checkpoint==previous_checkpoint,"Failed write preserves the previously usable Continue checkpoint")
	manager.file_root=good_root
	manager.allow_test_disk=false
	game.hero.reset_to(Vector3(36,.1,-20));game.mission.has_badge=true;game.mission.power_on=true
	await _frames(4);_freeze()
	var saved_auto:int=manager.next_auto
	game.automated=false;manager.allow_test_disk=true
	manager.update(.016)
	game.automated=true
	_check("diagnostic_gallery" in manager.visited and manager.next_auto!=saved_auto,"Safe-zone entry triggers autosave once")
	var auto_after:int=manager.next_auto
	game.automated=false;manager.update(31);game.automated=true
	_check(manager.next_auto==auto_after,"Remaining in an autosave zone does not spam new slots")
	for station:Node3D in manager.points:
		_check(_standing_clear(station.interaction_point()),"New save point has a collision-free standing approach "+str(station.position))
		game.hero.reset_to(station.interaction_point());game.hero.camera_rig.rotation.y=0
		await _frames(5)
		game.interact()
		_check(manager.screen.mode=="save","Actual F interaction opens save point "+str(station.position))
		manager.screen.dismiss()
	game.show_title_menu();game.front_end.game_button.pressed.emit();game.front_end.all_saves_button.pressed.emit()
	manager.screen.mode_buttons.save.pressed.emit()
	_check(manager.screen.mode=="save","All saves browser offers manual saving for the active campaign")
	manager.screen.dismiss()
	_check(game.front_end.maps_button.disabled and game.front_end.online_button.disabled,"Maps and Online are explicitly unavailable prototypes")
	var locale:Node=root.get_node("TextLocale")
	var missing:Array[String]=[]
	var letters:=RegEx.new();letters.compile("[А-Яа-яЁё]")
	locale.set_language("en")
	for source:String in locale.entries:
		var formatted:String=source
		var values:Array=[]
		var spec:=RegEx.new();spec.compile("%[0-9.]*[dfs]")
		for match:RegExMatch in spec.search_all(source):values.append("ровное" if match.get_string().ends_with("s") else 7)
		if not values.is_empty():formatted=source%values
		var translated:String=locale.translate(formatted)
		if letters.search(translated)!=null:missing.append(formatted+" => "+translated)
	_check(missing.is_empty(),"All catalog entries and formatted messages translate without Russian leftovers: "+str(missing.slice(0,5)))
	await _frames(10)
	_check(game.front_end.new_game_button.text=="New game" and game.front_end.maps_button.text=="Maps","Existing menu labels update to English in place")
	locale.set_language("kk");await _frames(10)
	_check(game.front_end.new_game_button.text=="Жаңа ойын" and game.front_end.language_picker.get_item_text(1)=="Қазақша","Kazakh interface updates and language names retain native spelling")
	_check(locale.translate("УДЕРЖИВАЙ F · установка модуля · 75%") == "F ҰСТАП ТҰР · модульді орнату · 75%","Kazakh mechanism text preserves percentage and translated dynamic action")
	locale.set_language("ru");await _frames(10)
	_check(game.front_end.new_game_button.text=="Новая игра","Language can return to Russian without losing original strings")
	var normalized:Dictionary=game.normalise_settings({"language":"kk","texture_quality":"standard"})
	_check(normalized.language=="kk" and normalized.texture_quality=="standard","Language and texture settings persist through settings schema")
	_check(game.normalise_settings({"language":13,"texture_quality":"bogus"}).language=="ru","Invalid language defaults safely")
	game.front_end.language_picker.item_selected.emit(2)
	_check(game.settings.language=="en","Actual language option signal updates application settings")
	game.front_end.language_picker.item_selected.emit(0)
	var material:StandardMaterial3D=Materials.pbr("painted_plaster_wall")
	_check(material.albedo_texture.get_width()==4096 and material.normal_texture.get_width()==4096 and material.roughness_texture.get_width()==4096,"High quality uses genuine 4K color, normal and roughness maps")
	_check(material.albedo_texture.get_image().has_mipmaps(),"4K maps include mipmaps to control distant shimmering")
	Materials.set_quality("standard")
	_check(material.albedo_texture.get_width()==2048,"Quality option switches existing materials to real 2K maps")
	Materials.set_quality("high")
	_check(material.albedo_texture.get_width()==4096,"High quality can be restored without restarting")
	_clean()
	game.queue_free();await process_frame
	print("SAVE AND INTERFACE: %d checks, %d failures"%[checks,failures])
	quit(1 if failures else 0)
func _clean()->void:
	for key:String in game.saves.KEYS:
		for suffix:String in [".json",".json.tmp",".json.bak"]:
			var path:String=game.saves.file_root.path_join(key+suffix)
			if FileAccess.file_exists(path):DirAccess.remove_absolute(ProjectSettings.globalize_path(path))
func _standing_clear(at:Vector3)->bool:
	var shape:=CapsuleShape3D.new();shape.radius=.34;shape.height=1.7
	var query:=PhysicsShapeQueryParameters3D.new();query.shape=shape;query.transform.origin=at+Vector3(0,.9,0);query.collision_mask=1
	return game.get_world_3d().direct_space_state.intersect_shape(query,1).is_empty()
func _freeze()->void:
	for actor:CharacterBody3D in game.hunters:actor.set_active(false);actor.set_physics_process(false)
func _frames(count:int)->void:
	for i:int in range(count):await physics_frame;await process_frame
func _check(ok:bool,message:String)->void:
	checks+=1
	if not ok:failures+=1;push_error("FAIL: "+message)
