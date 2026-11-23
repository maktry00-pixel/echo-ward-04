extends SceneTree
var game:Node3D
var checks:int=0
var failures:int=0
func _initialize()->void:call_deferred("_run")
func _run()->void:
	game=load("res://scenes/third_person.tscn").instantiate();game.set_meta("automated_test",true);root.add_child(game)
	await _frames(18);_freeze()
	var admin:Node=game.admin
	game.show_title_menu();game.front_end.open_settings();game.front_end.settings_view.admin_button.pressed.emit()
	_check(admin.access_layer.visible and admin.password.secret and not admin.active,"Settings opens a masked password dialog")
	admin.password.text="1234";admin.submit_password()
	_check(not admin.active and not admin.error_label.text.is_empty(),"Incorrect password cannot activate admin flight")
	admin.password.text="0890";admin.submit_password();await _frames(4)
	_check(admin.active and not game.in_title_menu and not game.is_paused,"Correct password enters the current campaign")
	_check(root.get_camera_3d()==admin.camera and game.hero.free_flight and game.hero.collision_layer==0 and game.hero.collision_mask==0,"Independent spectator camera and collision-free proxy are active")
	_check(not game.hero.model.visible and not game.combat.weapon.visible,"Manual flight has no visible character or held weapon")
	_check(not game.saves.capture("manual_4"),"Flight cannot record an unsafe position inside walls")
	admin.camera.position=Vector3(-14,1.65,10);admin.camera.rotation=Vector3(0,PI/2,0)
	Input.mouse_mode=Input.MOUSE_MODE_CAPTURED
	Input.action_press("admin_forward");await _frames(45);Input.action_release("admin_forward")
	_check(admin.camera.position.x<-17 and game.hero.position.x<-17,"Actual flight input crosses the solid ward wall")
	var height:float=admin.camera.position.y
	Input.action_press("admin_up");await _frames(15);Input.action_release("admin_up")
	_check(admin.camera.position.y>height+1,"Flight supports vertical movement")
	var health:int=game.health;game._take_hit(game.hunters[0]);game.take_projectile_hit();game.isolation._take_shock()
	_check(game.health==health,"Invisible flying proxy is not killed by melee, acid or electricity")
	var cabinet:Node3D=game.mission.objects.fuse
	admin.camera.position=cabinet.interaction_point()+Vector3(0,1.55,0);admin.camera.rotation=Vector3(0,cabinet.rotation.y,0)
	await _frames(6)
	_check(game._interaction_target()==cabinet,"Focusing real furniture works from the flying camera")
	game.interact();await _frames(32);cabinet.storage.selected=cabinet.storage.reward
	Input.action_press("interact_hold");await _frames(78);Input.action_release("interact_hold");await _frames(2);game.interact()
	_check(game.mission.has_fuse and cabinet.used,"Manual flight opens, searches and collects a real cabinet item")
	admin.leave();await _frames(6)
	_check(not admin.active and not game.hero.free_flight and game.hero.collision_layer==2 and game.hero.collision_mask==1,"Leaving admin restores normal collision")
	_check(game.hero.is_on_floor() and game.hero.position.distance_to(game.facility.START)<.5,"Leaving flight returns to a safe standing anchor")
	_check(game.saves.KEYS.size()==9,"Six manual slots coexist with the original three autosaves")
	game.saves.file_root="res://artifacts/admin_test_saves";game.saves.allow_test_disk=true
	for slot:int in range(1,7):
		_check(game.saves.capture("manual_%d"%slot),"Manual save slot %d writes a validated file"%slot)
	game.saves.slots.clear();game.saves.read_slots()
	_check(game.saves.slots.size()==6 and game.saves.slots.has("manual_4") and game.saves.slots.has("manual_6"),"All six manual slots survive a disk reread")
	game.saves.allow_test_disk=false
	game.health=2;_check(game.saves.capture("auto_3"),"Real autosave snapshot is available for respawn")
	game.saves.slots.auto_3.saved_at=8000000000.0
	var generation:int=game.run_generation
	game.health=0;game._finish_round(false,"fixture");await _frames(8);_freeze()
	_check(not game.has_lost and not game.is_paused and game.health==2 and game.run_generation>generation,"Death immediately restores the newest save without a defeat-menu click")
	_check(game.hero.invulnerable_remaining>1,"Respawn gives a short protection window without healing the saved health")
	admin.enter();admin.pilot_button.pressed.emit();await _frames(140)
	_check(admin.pilot.running and not game.hero.free_flight and admin.camera.current,"Agent uses the normal physical hero while spectator remains independent")
	_check(game.mission.has_map or not admin.pilot.completed.is_empty(),"Runtime agent makes real early mission progress")
	game.inventory.screen.open_inventory()
	admin.pilot_button.pressed.emit();await _frames(4)
	_check(not admin.pilot.running and game.hero.free_flight and not game.is_paused and game.inventory.screen.mode=="closed" and not Input.is_action_pressed("move_forward") and not Input.is_action_pressed("aim"),"Second button press stops the agent, closes its modal screens and releases controls")
	admin.pilot_button.pressed.emit();await _frames(3);generation=game.run_generation
	game.health=0;game._finish_round(false,"agent fixture");await _frames(12);_freeze()
	_check(admin.active and admin.pilot.running and not game.hero.free_flight and not game.has_lost and game.run_generation>generation,"Agent resumes after death from the newest save, retaining spectator mode")
	admin.pilot.stop();admin.leave();await _frames(3)
	game.saves.slots.clear();game.checkpoint.clear();generation=game.run_generation
	game.health=0;game._finish_round(false,"no save fixture");await _frames(8);_freeze()
	_check(not game.has_lost and game.health==3 and game.run_generation>generation,"Without a save, death returns to the campaign start")
	for key:String in game.saves.KEYS:
		for suffix:String in [".json",".json.bak",".json.tmp"]:
			var path:String=game.saves.file_root.path_join(key+suffix)
			if FileAccess.file_exists(path):DirAccess.remove_absolute(ProjectSettings.globalize_path(path))
	game.queue_free();await process_frame
	print("ADMIN, AGENT AND RESPAWN: %d checks, %d failures"%[checks,failures]);quit(1 if failures else 0)
func _key(code:int,pressed:bool)->void:
	var key:=InputEventKey.new();key.physical_keycode=code;key.keycode=code;key.pressed=pressed;Input.parse_input_event(key)
func _freeze()->void:
	for actor:CharacterBody3D in game.hunters:actor.set_active(false);actor.set_physics_process(false)
func _frames(count:int)->void:
	for i:int in range(count):await physics_frame;await process_frame
func _check(ok:bool,message:String)->void:
	checks+=1
	if not ok:failures+=1;push_error("FAIL: "+message)
