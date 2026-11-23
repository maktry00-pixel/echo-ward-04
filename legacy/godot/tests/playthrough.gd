extends SceneTree
## Complete mission using actual movement, F interactions and live enemies.
## The bot knows the map. It does not measure human fun or first-run duration.

var game: Node3D
var frames: int = 0
var completed: Array[String] = []
var blind_observations: Array[Dictionary] = []
var refuge_resume: bool = false
var upgrade_route: bool = false
var difficulty: String = "normal"
var interface_saves:Array[String]=[]
var itinerary: Array[String] = ["map", "weapon", "ammo_reception", "fuse", "power", "service_shortcut", "badge", "archive_shortcut", "quarantine_gate", "research_gate"]

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	game = load("res://scenes/third_person.tscn").instantiate()
	# This older balance probe records its first death; shipped agent tests cover respawn.
	game.auto_respawn_enabled=false
	game.set_meta("automated_test", true)
	root.add_child(game)
	await _frames(8)
	for arg: String in OS.get_cmdline_user_args():
		if arg.begins_with("--difficulty="): difficulty = arg.trim_prefix("--difficulty=")
	if difficulty not in preload("res://scripts/difficulty_profile.gd").IDS: difficulty = "normal"
	game.front_end.game_button.pressed.emit()
	game.front_end.new_game_button.pressed.emit()
	game.front_end.difficulty_picker.item_selected.emit(preload("res://scripts/difficulty_profile.gd").IDS.find(difficulty))
	game.front_end.start_button.pressed.emit()
	await _frames(8)
	upgrade_route = OS.get_cmdline_user_args().has("--upgrade-route")
	if OS.get_cmdline_user_args().has("--interface-update") or OS.get_cmdline_user_args().has("--presentation-update"):
		game.front_end.language_picker.select(1)
		game.front_end.language_picker.item_selected.emit(1)
	if OS.get_cmdline_user_args().has("--risk-route"):
		itinerary = ["map", "weapon", "ammo_reception", "fuse", "power", "supply_room", "battery", "service_shortcut", "badge", "archive_shortcut", "medkit", "quarantine_gate", "research_gate"]
	if OS.get_cmdline_user_args().has("--refuge-route"):
		itinerary.insert(itinerary.find("badge"),"refuge_stash")
		itinerary.insert(itinerary.find("badge"),"refuge_save")
	if upgrade_route:
		itinerary.insert(itinerary.find("fuse"),"upgrade_boards_reception")
		itinerary.insert(itinerary.find("fuse"),"upgrade_record_shift")
		itinerary.insert(itinerary.find("archive_shortcut"),"upgrade_record_evidence")
	# The extended route searches existing ammunition rather than receiving test supplies.
	itinerary.insert(itinerary.find("archive_shortcut"),"ammo_archive")
	for department: String in ["morgue", "lab", "theatre"]:
		itinerary.append("ammo_" + department)
		if upgrade_route and department=="morgue": itinerary.append_array(["upgrade_shotgun","upgrade_shells_morgue","upgrade_close","upgrade_build","upgrade_remove","upgrade_open"])
		for i: int in range(3):
			itinerary.append("dial_%s_%d" % [department, i])
		itinerary.append("relay_" + department)
	itinerary.append_array(["technical_gate", "ammo_lower_entry", "fuel", "ammo_reactor", "coolant", "ammo_pump", "reactor", "res_entry", "residence_east", "residence_prybar", "residence_west", "res_west_service", "residence_east", "residence_lock", "residence_core", "ammo_residence", "residence_plate", "residence_lift", "res_return", "diag_entry", "diag_23"])
	if OS.get_cmdline_user_args().has("--investigation-route"): itinerary.append("chapter_memo")
	itinerary.append_array(["chapter_xray","diag_cross","chapter_observe"])
	if OS.get_cmdline_user_args().has("--investigation-route"): itinerary.append_array(["chapter_log","diag_office","chapter_office"])
	if OS.get_cmdline_user_args().has("--glass-route"): itinerary.append("chapter_glass")
	else: itinerary.append_array(["diag_loop","chapter_supplies"])
	itinerary.append_array(["chapter_treatment","diag_52","terminal","final_passage","diag_entry","chapter_final_reset","chapter_exit_crank","exit"])
	if upgrade_route:
		itinerary.insert(itinerary.find("reactor"),"upgrade_shells_lower")
		itinerary.insert(itinerary.find("diag_23"),"upgrade_record_ceiling")
		itinerary.insert(itinerary.find("chapter_final_reset"),"upgrade_record_farewell")
	var isolated: Array[String] = ["iso_entry","isolation_case","isolation_dark" if OS.get_cmdline_user_args().has("--dark-isolation") else "isolation_light"]
	if OS.get_cmdline_user_args().has("--dark-isolation"):
		isolated.append_array(["iso_east","isolation_coil_dark","isolation_glass","isolation_tag","isolation_dry_south","isolation_dry_north"])
	else: isolated.append_array(["iso_west","isolation_diary","isolation_coil_light","iso_showers","isolation_tag"])
	isolated.append_array(["isolation_ammo","isolation_generator","isolation_vent_a","isolation_vent_b","isolation_log"])
	if OS.get_cmdline_user_args().has("--isolation-evidence"):
		isolated.append_array(["isolation_report","isolation_diary","iso_archive","isolation_proof"])
	elif OS.get_cmdline_user_args().has("--dark-isolation"): isolated.append_array(["isolation_dry_north","isolation_dry_south"])
	isolated.append("isolation_release")
	if upgrade_route: isolated.push_front("upgrade_shells_preparation")
	for i: int in range(isolated.size()): itinerary.insert(itinerary.find("terminal"),isolated[i])
	game.admin.pilot._plan()
	for id: String in game.admin.pilot.itinerary:
		if id.begins_with("annex_"): itinerary.insert(itinerary.find("terminal"),id)
	itinerary.insert(itinerary.find("diag_entry"),"field_medkit_2")
	itinerary.insert(itinerary.find("iso_entry"),"field_medkit_1")
	for id: String in itinerary:
		var success: bool = await _visit(id)
		if not success:
			push_error("Failed mission step: " + id)
			break
		completed.append(id)
		if id=="refuge_save":
			_release()
			var saved: Dictionary = JSON.parse_string(JSON.stringify(game.checkpoint))
			var save_file := FileAccess.open("res://artifacts/refuge_route_checkpoint.json",FileAccess.WRITE)
			save_file.store_string(JSON.stringify(saved))
			save_file.close()
			game.checkpoint = saved
			refuge_resume = game.continue_checkpoint()
			await _frames(8)
			if not refuge_resume: break
		if id=="badge":
			var inspect := InputEventKey.new()
			inspect.physical_keycode = KEY_X
			inspect.pressed = true
			game._unhandled_input(inspect)
			game.inventory.screen.inspector.flip()
			await _frames(35)
			game.inventory.screen.dismiss()
		if id in ["residence_core","ammo_residence","residence_plate","residence_lift","res_return"]:
			var blind: CharacterBody3D = game.hunters[13]
			blind_observations.append({"step":id,"position":str(blind.position),"active":blind.active,"health":blind.health,"state":blind.state,"heard":str(blind.last_known)})
		print("STEP ", id, " at ", snappedf(game.elapsed, 0.1), " s; HP=", game.health)
		_release()
		await _frames(5)
		if (OS.get_cmdline_user_args().has("--interface-update") or OS.get_cmdline_user_args().has("--presentation-update")) and id in ["badge","chapter_treatment","isolation_release"]:
			if not game.saves.capture("manual_3"):
				push_error("Late chapter save failed: "+id);break
			var checkpoint_data:Dictionary=JSON.parse_string(JSON.stringify(game.saves.slots.manual_3))
			if not preload("res://scripts/checkpoint_store.gd").valid(checkpoint_data):push_error("Late save validation failed: "+id);break
			interface_saves.append(id)
	_release()
	var result: Dictionary = {"automated": true, "won": game.has_won, "lost": game.has_lost,
		"completed": completed, "records": game.recovered_count, "health": game.health,
		"power": game.mission.power_on, "badge": game.mission.has_badge,
		"optional_battery_used": game.batteries[0].is_recovered, "optional_medkit_used": game.medkits[0].is_recovered,
		"events": game.mission.events.keys(), "seconds": snappedf(game.elapsed, 0.1),
		"guardian":game.guardian.snapshot(),"guardian_health":game.hunters[5].health,"guardian_position":str(game.hunters[5].position),
		"relays": game.mission.relays, "shots": game.combat.shots_fired, "hits": game.combat.hits, "kills": game.combat.kills,
		"purge_complete": game.mission.purge_complete, "layout_seed": game.layout_seed,
		"residential_complete":game.residential.complete,"pace":"walk" if OS.get_cmdline_user_args().has("--walk-route") else "run",
		"blind_observations":blind_observations,
		"refuge_resume":refuge_resume,"refuge_storage":game.refuge.snapshot(),"pacing":game.pacing.snapshot(),
		"difficulty":game.difficulty_id,"upgrade_route":upgrade_route,"upgrades":game.upgrades.snapshot(),"shotgun":{"owned":game.combat.shotgun_owned,"loaded":game.combat.shotgun_loaded,"shells":game.combat.shells},
		"isolation":game.isolation.snapshot(),
		"interface_saves":interface_saves,"language":root.get_node("TextLocale").language,
		"medkits_found":range(game.medkits.size()).filter(func(i:int)->bool:return game.medkits[i].is_recovered),
		"diagnostics":game.diagnostics.snapshot(),"finale":{"started":game.diagnostics.finale_started,"breaker":game.diagnostics.breaker_reset,"released":game.diagnostics.exit_released},
		"note": "Bot knows the map. This does not establish human balance, fun or first-run duration."}
	print("FULL MISSION PLAYTHROUGH: ", JSON.stringify(result))
	var output_path: String = "res://artifacts/playthrough_upgrade_%s.json" % difficulty if upgrade_route else ("res://artifacts/playthrough_mode_%s.json" % difficulty if difficulty!="normal" else "res://artifacts/playthrough_glass.json" if OS.get_cmdline_user_args().has("--glass-route") else "res://artifacts/playthrough_investigation.json" if OS.get_cmdline_user_args().has("--investigation-route") else "res://artifacts/playthrough_refuge.json" if OS.get_cmdline_user_args().has("--refuge-route") else "res://artifacts/playthrough_walk.json" if OS.get_cmdline_user_args().has("--walk-route") else ("res://artifacts/playthrough_risk.json" if OS.get_cmdline_user_args().has("--risk-route") else "res://artifacts/playthrough.json"))
	output_path = "res://artifacts/playthrough_isolation_"+("dark" if OS.get_cmdline_user_args().has("--dark-isolation") else "light")+"_"+difficulty+".json"
	if OS.get_cmdline_user_args().has("--interface-update"):output_path="res://artifacts/playthrough_interface.json"
	if OS.get_cmdline_user_args().has("--presentation-update"):output_path="res://artifacts/playthrough_presentation_%s.json"%difficulty
	var file := FileAccess.open(output_path, FileAccess.WRITE)
	file.store_string(JSON.stringify(result, "\t"))
	file.close()
	var won: bool = game.has_won
	game.queue_free()
	await process_frame
	quit(0 if won else 1)

func _visit(id: String) -> bool:
	# Keep the older balance probe on the same real annex controller as the shipped agent.
	if id.begins_with("annex_"):
		var pilot: Node = game.admin.pilot
		pilot.running = true; pilot.run_id+=1
		var ok: bool = await pilot._visit(id)
		pilot.release_controls(); pilot.running = false
		return ok
	var goal: Vector3
	var object: Node3D
	if id.begins_with("field_medkit_"):
		if game.health>=3: return true
		object = game.medkits[int(id.trim_prefix("field_medkit_"))]
		goal = object.interaction_point()
	elif id.begins_with("isolation_dry_"):
		goal = Vector3(46,.1,-78 if id.ends_with("south") else -85.5)
	elif id=="isolation_glass":
		object = game.isolation.partitions.glass
		goal = Vector3(45.2,.1,-75)
	elif id.begins_with("isolation_"):
		object = game.isolation.stations[id.trim_prefix("isolation_")]
		goal = object.interaction_point()
	elif id in ["upgrade_close","upgrade_build","upgrade_remove","upgrade_open"]:
		object = game.facility.gate_by_id.research_23
		goal = object.position+Vector3(0,0,2.1)
	elif id.begins_with("upgrade_"):
		object = game.upgrades.stations[id.trim_prefix("upgrade_")]
		goal = object.interaction_point()
	elif id=="chapter_glass":
		object = game.diagnostics.partitions.observation_glass
		goal = Vector3(42.8,.1,-33)
	elif id.begins_with("chapter_"):
		object = game.diagnostics.stations[id.trim_prefix("chapter_")]
		goal = object.interaction_point()
	elif id.begins_with("refuge_"):
		object = game.refuge.stations[id.trim_prefix("refuge_")]
		goal = object.interaction_point()
	elif id == "exit":
		goal = game.facility.EXIT
	elif id in ["residence_west","residence_east"]:
		object = game.mission.objects["residence_panel"]
		goal = object.position
	elif id=="residence_lock":
		object = game.residential.lock_target
		goal = Vector3(object.position.x,.1,object.position.z+1.0)
	elif id in ["battery", "medkit"]:
		object = game.batteries[0] if id == "battery" else game.medkits[0]
		if (id == "battery" and game.flash_charges >= 3) or (id == "medkit" and game.health >= 3):
			return true
		goal = object.position
	elif game.mission.objects.has(id):
		object = game.mission.objects[id]
		goal = object.position
	else:
		object = game.facility.gate_by_id[id]
		goal = object.position + (Vector3(0, 0, 2) if id == "quarantine_gate" else (Vector3(2, 0, 0) if id == "supply_room" else Vector3(-2, 0, 0)))
		if id == "technical_gate":
			goal = object.position + Vector3(0, 0, -2)
		if id in ["diag_entry","diag_23","diag_37","diag_52"]: goal = object.position+Vector3(0,0,2)
		if id.begins_with("iso_"): goal = object.position+object.basis.z*2
		if id=="diag_office": goal = object.position+Vector3(2,0,0)
	if object!=null and id!="residence_lock" and object.has_method("interaction_point"): goal = object.interaction_point()
	var budget: int = 7200 if id in ["reactor","terminal"] else 3600
	var path: Array[Vector3] = []
	var index: int = 0
	var repath: int = 0
	while budget > 0 and frames < 100000 and not game.has_lost:
		if id.begins_with("isolation_dry_") and game.hero.position.distance_to(goal)<.4: return true
		if id=="isolation_glass" and object.broken: return true
		if id.begins_with("isolation_") and id!="isolation_glass" and not id.begins_with("isolation_dry_") and game.isolation.done(id.trim_prefix("isolation_")): return true
		if id.begins_with("upgrade_"):
			if id=="upgrade_close" and not object.is_open and not object.busy: return true
			if id=="upgrade_open" and object.is_open and not object.busy: return true
			if id=="upgrade_build" and object.barricade_hp>0: return true
			if id=="upgrade_remove" and object.barricade_hp==0 and game.upgrades.work_door==null: return true
			if id.begins_with("upgrade_record_") and id.trim_prefix("upgrade_record_") in game.upgrades.records: return true
		if id.begins_with("chapter_"):
			var chapter: Node3D = game.diagnostics
			if id=="chapter_xray" and chapter.xray_solved or id=="chapter_observe" and chapter.observed or id=="chapter_treatment" and chapter.complete or id=="chapter_supplies" and chapter.reward_taken or id=="chapter_final_reset" and chapter.breaker_reset or id=="chapter_exit_crank" and chapter.exit_released or id.trim_prefix("chapter_") in chapter.evidence or id=="chapter_glass" and object.broken: return true
			if chapter.screen.mode!="closed":
				_release()
				if chapter.screen.mode=="xray":
					for i: int in range(3): chapter.screen.buttons.rotate.pressed.emit()
					var click := InputEventMouseButton.new()
					click.button_index = MOUSE_BUTTON_LEFT
					click.pressed = true
					chapter.screen.board.gui_input.emit(click)
					var drag := InputEventMouseMotion.new()
					drag.relative = -chapter.screen.film_offset
					chapter.screen.board.gui_input.emit(drag)
					click.pressed = false
					chapter.screen.board.gui_input.emit(click)
					chapter.screen.buttons.verify.pressed.emit()
				else:
					for step: String in ["cool","dose","release"]: chapter.screen.buttons[step].pressed.emit()
				await _frames(5)
				continue
		if id.begins_with("refuge_") and game.refuge.screen.mode!="closed":
			_release()
			if id=="refuge_stash": game.refuge.screen.transfer_buttons.bottle_deposit.pressed.emit()
			else: game.refuge.screen.save_button.pressed.emit()
			game.refuge.screen.dismiss()
			return true
		if game.inventory.screen.mode=="keypad":
			_release()
			for digit: String in ["4","1","7","ВВОД"]: game.inventory.screen.keypad_buttons[digit].pressed.emit()
			await _frames(35)
		if game.health<3 and game.inventory.medkits>0: game.inventory.use_item("medkit")
		if object!=null and game.inventory.entries().size()>=8 and object.get("used")==false:
			if game.inventory.boards>0: game.inventory.drop_item("boards")
			elif game.survival_tools.bottles>0: game.inventory.drop_item("bottle")
			elif id.begins_with("ammo_"): return true # No expendable inventory slot left.
			elif game.combat.reserve>24: game.inventory.drop_item("ammo")
		if id in ["residence_west","residence_east"] and game.residential.mode==(1 if id=="residence_west" else 2): return true
		if id=="residence_lock" and object.broken: return true
		if id=="reactor" and not game.mission.purge_fault.is_empty():
			if not await _visit(game.mission.purge_fault): return false
			repath = 0
		if game.has_won:
			return id == "exit"
		if object != null:
			if id.begins_with("dial_"):
				var parts: PackedStringArray = id.split("_")
				if object.state_value == game.mission.CIRCUITS[parts[1]][int(parts[2])]:
					return true
			if id not in ["upgrade_close","upgrade_build","upgrade_remove","upgrade_open"] and (object.get("used") == true or object.get("is_recovered") == true or (object.get("is_open") == true and not object.busy)):
				return true
		var flat: float = Vector2(goal.x - game.hero.position.x, goal.z - game.hero.position.z).length()
		var fighting: bool = _fight()
		if fighting:
			repath = 0
		# Reach the authored safe stance precisely; stopping 35cm early can stand
		# inside a drawer's extension sweep, which correctly asks the player to back up.
		elif object != null and flat < (.12 if object.get("storage")!=null else (.35 if object.has_method("interaction_point") else .5)):
			_release()
			var look: Vector3 = object.global_position-game.hero.global_position
			game.hero.camera_rig.rotation.y = atan2(-look.x,-look.z)
			if id in ["power","reactor","purge_valve_a","purge_valve_b","residence_core","residence_lift","residence_plate","chapter_final_reset","chapter_exit_crank"] or object.get("storage")!=null: Input.action_press("interact_hold")
			if id in ["chapter_glass","isolation_glass"]: game.combat.knife_attack()
			if id in ["isolation_generator","isolation_vent_a","isolation_vent_b"]: Input.action_press("interact_hold")
			if id=="residence_lock":
				game.hero.camera_rig.rotation.y = 0
				game.combat.knife_attack()
			if id in ["upgrade_build","upgrade_remove"]:
				Input.action_press("barricade")
				if game.upgrades.work_door==null:
					var key := InputEventKey.new()
					key.physical_keycode = KEY_K
					key.pressed = true
					game._unhandled_input(key)
			elif frames % 8 == 0:
				var key := InputEventKey.new()
				key.physical_keycode = KEY_F
				key.pressed = true
				game._unhandled_input(key)
		else:
			if repath <= 0:
				path = game.navigation.find_clear_path(game.hero.position, goal)
				index = 1 if path.size() > 1 else 0
				repath = 60
			while index < path.size() and Vector2(path[index].x - game.hero.position.x, path[index].z - game.hero.position.z).length() < 0.25:
				index += 1
			var destination: Vector3 = path[index] if index < path.size() else goal
			var offset: Vector3 = destination - game.hero.position
			offset.y = 0
			_steer(offset.normalized())
			if game.hero.stamina < 20 or game.hero.exhausted or OS.get_cmdline_user_args().has("--walk-route"):
				Input.action_release("sprint")
			else:
				Input.action_press("sprint")
			Input.action_release("dash")
			var nearest: float = INF
			for hunter: CharacterBody3D in game.hunters:
				if hunter.active and hunter.stunned_remaining <= 0 and game._line_clear(game.hero.position, hunter.position):
					nearest = minf(nearest, hunter.position.distance_to(game.hero.position))
			if nearest < 3.5 and game.flash_charges > 0 and game.flash_cooldown <= 0:
				game.use_flash()
			elif nearest < 2.8 and game.hero.dash_cooldown <= 0:
				Input.action_press("dash")
		await _frames(1)
		frames += 1
		budget -= 1
		repath -= 1
	for i: int in range(game.hero.get_slide_collision_count()):
		var contact: KinematicCollision3D = game.hero.get_slide_collision(i)
		print("CONTACT ",contact.get_collider().get_path()," ",contact.get_normal())
	print("COMBAT STATE ",game.combat.rounds,"/",game.combat.reserve)
	print("STUCK: ", id, " position=", game.hero.position, " goal=", goal, " remaining path=", path)
	return false

func _frames(count: int) -> void:
	for frame: int in range(count):
		await physics_frame
		await process_frame

func _release() -> void:
	for action: String in ["move_forward", "move_back", "move_left", "move_right", "sprint", "dash", "aim", "interact_hold","barricade"]:
		Input.action_release(action)

func _steer(direction: Vector3) -> void:
	direction = Basis(Vector3.UP, -game.hero.camera_rig.rotation.y) * direction
	for action: String in ["move_forward", "move_back", "move_left", "move_right"]:
		Input.action_release(action)
	Input.action_press("move_forward" if direction.z < 0 else "move_back", absf(direction.z))
	Input.action_press("move_left" if direction.x < 0 else "move_right", absf(direction.x))


func _fight() -> bool:
	# Empty weapons require reaching real supplies; never kite forever without ammo.
	if game.combat.rounds+game.combat.reserve+game.combat.shotgun_loaded+game.combat.shells<=0:
		Input.action_release("aim")
		return false
	var target: CharacterBody3D
	var nearest: float = 10.0
	for hunter: CharacterBody3D in game.hunters:
		var distance: float = hunter.position.distance_to(game.hero.position)
		var ray := PhysicsRayQueryParameters3D.create(game.hero.position+Vector3(0,1.35,0),hunter.aim_point(),1)
		if hunter.active and distance < nearest and game.get_world_3d().direct_space_state.intersect_ray(ray).is_empty():
			target = hunter
			nearest = distance
	if target == null or not game.combat.equipped:
		Input.action_release("aim")
		return false
	if nearest<3.5 and target.stunned_remaining<=0 and game.flash_charges>0 and game.flash_cooldown<=0:
		game.use_flash()
	if upgrade_route and game.combat.shotgun_owned:
		if game.combat.shotgun_reloading:
			_steer((game.hero.position-target.position).normalized() if nearest<5 else Vector3.ZERO)
			return true
		if nearest>6 and game.combat.shotgun_loaded<2 and game.combat.shells>0:
			if game.combat.selected_weapon!="shotgun": game.combat.select_weapon("shotgun")
			game.combat.reload_weapon()
			return true
		var wanted_weapon: String = "shotgun" if nearest<5.5 and game.combat.shotgun_loaded>0 and target.boss_controller==null else "pistol"
		if game.combat.selected_weapon!=wanted_weapon: game.combat.select_weapon(wanted_weapon)
	Input.action_press("aim")
	Input.action_release("sprint")
	Input.action_release("dash")
	var camera: Camera3D = game.hero.camera_rig.camera
	Input.action_release("interact_hold")
	game.survival_tools.cancel_action()
	game.isolation.cancel_work()
	var point: Vector3 = target.aim_point()
	var center: Vector2 = camera.get_viewport().get_visible_rect().size / 2
	var origin: Vector3 = camera.project_ray_origin(center)
	var direction: Vector3 = camera.project_ray_normal(center)
	var wanted: Vector3 = (point - origin).normalized()
	var yaw_error: float = atan2(-wanted.x, -wanted.z) - atan2(-direction.x, -direction.z)
	var pitch_error: float = asin(wanted.y) - asin(direction.y)
	game.hero.camera_rig.apply_look(Vector2(-wrapf(yaw_error, -PI, PI), -pitch_error) / game.hero.camera_rig.look_sensitivity)
	var retreat: Vector3 = (game.hero.position - target.position).normalized() if nearest < 4 else Vector3.ZERO
	if target.boss_controller!=null and target.boss_controller.started and target.boss_controller.exposed_remaining<=0:
		var away: Vector3 = (game.hero.position-target.position).normalized()
		away.y = 0
		var dodge: Vector3 = Vector3(away.z,0,-away.x)
		if (game.hero.position.x>52 and dodge.x>0) or (game.hero.position.x<52 and dodge.x<0): dodge = -dodge
		if target.windup>0 or target.lunge_remaining>0:
			_steer(dodge)
			Input.action_release("aim")
			Input.action_press("sprint")
			if game.hero.dash_cooldown<=0: Input.action_press("dash")
		else: _steer(away if nearest<5.0 else Vector3.ZERO)
		if game.combat.rounds<3 and game.combat.reserve>0: game.combat.reload_weapon()
		return true
	_steer(retreat)
	if game.combat.selected_weapon=="pistol" and game.combat.rounds <= 0:
		if game.combat.reserve > 0:
			game.combat.reload_weapon()
		elif nearest < 1.8:
			game.combat.knife_attack()
	elif game.combat.cooldown <= 0 and absf(yaw_error) < 0.02 and absf(pitch_error) < 0.02:
		var click := InputEventMouseButton.new()
		click.button_index = MOUSE_BUTTON_LEFT
		click.pressed = true
		game._unhandled_input(click)
	return true
