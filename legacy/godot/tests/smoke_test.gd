extends SceneTree
## Integration checks use real colliders. Teleports isolate scenarios;
## tests/playthrough.gd separately walks the complete mission with live enemies.

var game: Node3D
var failures: int = 0
var checks: int = 0

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	game = load("res://scenes/third_person.tscn").instantiate()
	game.set_meta("automated_test", true)
	root.add_child(game)
	await _frames(12)
	_check(game.hero.is_on_floor(), "Hero settles on the floor")
	_check(game.hero.model.player.has_animation("idle") and game.hero.model.player.has_animation("run"), "Skeletal animations are available")
	var start: Vector3 = game.hero.position
	Input.action_press("move_forward")
	await _frames(30)
	Input.action_release("move_forward")
	_check(game.hero.position.z < start.z - 1.0, "Physical forward movement")
	Input.action_press("move_forward")
	Input.action_press("move_right")
	await _frames(12)
	_check(is_equal_approx(Vector2(game.hero.velocity.x, game.hero.velocity.z).length(), 3.2), "Normalized diagonal movement")
	_release()
	await _at(Vector3(13.5, 0.1, 10))
	Input.action_press("move_right")
	await _frames(45)
	_release()
	_check(game.hero.position.x < 14.3 and game.hero.position.x > 14.0, "Outer wall blocks the player")
	await _at(Vector3(0, 0.1, 10))
	Input.action_press("jump")
	await _frames(8)
	Input.action_release("jump")
	_check(game.hero.position.y > 0.4, "Jump uses physics")
	await _frames(55)
	_check(game.hero.is_on_floor(), "Jump lands")
	await _at(Vector3(0, 0.1, 14))
	_check(game.hero.camera_rig.arm.get_hit_length() < 2.2, "Camera retracts at the reception wall")
	_check(not game.has_won, "Entrance cannot win before the mission is complete")
	_check(game.navigation.walkable.size() > 1000, "Navigation includes the service annex")
	var path: Array[Vector3] = game.navigation.find_path(game.facility.START, game.mission.LOCATIONS["power"])
	_check(path.size() - 1 == _bfs(game.navigation.nearest(game.facility.START), game.navigation.nearest(game.mission.LOCATIONS["power"])), "A* shortest route matches independent BFS")
	_check(not game.navigation.find_path(game.facility.START, game.mission.LOCATIONS["power"]).is_empty(), "Generator is reachable before power")
	_check(game.navigation.find_path(game.facility.START, game.mission.LOCATIONS["badge"]).is_empty(), "All archive entrances are physically gated")
	_check(game.navigation.find_path(game.facility.START, game.mission.LOCATIONS["terminal"]).is_empty(), "Quarantine cannot be bypassed")
	_key(KEY_TAB)
	_check(not game.hud.radar.visible, "Map must first be found")
	await _use("map")
	_key(KEY_TAB)
	_check(game.mission.has_map and game.hud.radar.visible, "Collected floor plan enables Tab")
	await _use("power")
	_check(not game.mission.power_on and not game.mission.objects["power"].used, "Missing fuse does not consume the generator")
	await _use("badge")
	_check(not game.mission.has_badge, "Badge cannot skip the power stage")
	var shortcut: Node3D = game.facility.gate_by_id["service_shortcut"]
	await _at(Vector3(-13, 0.1, 11))
	_key(KEY_F)
	await _frames(35)
	_check(shortcut.locked and not shortcut.is_open, "Service latch cannot open from reception")
	var before: int = game.navigation.find_path(game.facility.START, game.mission.LOCATIONS["power"]).size()
	await _at(Vector3(-17, 0.1, 11))
	_key(KEY_F)
	await _frames(40)
	_check(not shortcut.locked and shortcut.is_open, "Service shortcut opens from inside")
	var after: int = game.navigation.find_path(game.facility.START, game.mission.LOCATIONS["power"]).size()
	_check(before>0 and after>0 and after<=before, "Opening shortcut preserves or shortens A* route through wider ward aisles")
	await _use("fuse")
	_check(game.mission.has_fuse and game.hunters[0].active, "Fuse starts the first encounter")
	_check(game.mission.events.has("first_contact"), "First contact fires once")
	for hunter: CharacterBody3D in game.hunters:
		hunter.stun(500)
	await _use("power")
	await _frames(40)
	_check(game.mission.power_on and not game.mission.has_fuse, "Fuse is installed to restore power")
	_check(game.facility.power_on and game.facility.silhouette.visible, "Power event changes lights and reveals a moving silhouette")
	_check(not game.navigation.find_path(game.facility.START, game.mission.LOCATIONS["badge"]).is_empty(), "Powered doors unlock access to archive")
	_check(game.navigation.find_path(game.facility.START, game.mission.LOCATIONS["terminal"]).is_empty(), "Quarantine still requires the badge")
	await _use("terminal")
	_check(not game.mission.exit_started, "Terminal refuses access without a badge")
	await _use("badge")
	_check(game.mission.has_badge, "Badge is obtained in archive")
	await _at(Vector3(0, 0.1, -7))
	_key(KEY_F)
	await _frames(40)
	_check(game.inventory.screen.mode=="keypad", "Badge presents actual quarantine keypad")
	for digit: String in ["4","1","7","ВВОД"]: game.inventory.screen.keypad_buttons[digit].pressed.emit()
	await _frames(40)
	_check(game.facility.gate_by_id["quarantine_gate"].is_open, "Correct entered code opens quarantine")
	_check(not game.navigation.find_path(game.facility.START, game.mission.LOCATIONS["terminal"]).is_empty(), "Terminal becomes physically reachable")
	var archive: Node3D = game.facility.gate_by_id["archive_shortcut"]
	await _at(Vector3(-1, 0.1, -5))
	_key(KEY_F)
	_check(archive.locked, "Archive latch refuses the corridor side")
	await _at(Vector3(-5, 0.1, -5))
	_key(KEY_F)
	await _frames(40)
	_check(archive.is_open and not archive.locked, "Archive shortcut opens from the archive")
	var supply: Node3D = game.facility.gate_by_id["supply_room"]
	await _at(Vector3(-21, 0.1, 11))
	_key(KEY_F)
	await _frames(40)
	_check(supply.is_open, "Optional guarded supply room can be entered")
	game.flash_charges = 3
	await _at(game.batteries[0].interaction_point())
	_key(KEY_F)
	_check(not game.batteries[0].is_recovered, "Full charge leaves a battery for later")
	game.flash_charges = 0
	await preload("res://tests/test_actions.gd").search(self,game,game.batteries[0])
	_key(KEY_F)
	_check(game.flash_charges==0 and game.inventory.batteries==1, "Battery is carried rather than consumed on pickup")
	_key(KEY_T)
	_check(game.flash_charges == 1, "Carried battery restores a charge on use")
	await _at(game.medkits[0].interaction_point())
	_key(KEY_F)
	_check(not game.medkits[0].is_recovered, "Healthy player leaves the medkit")
	game.health = 1
	await preload("res://tests/test_actions.gd").search(self,game,game.medkits[0])
	_key(KEY_F)
	_check(game.health==1 and game.medkits[0].is_recovered and game.inventory.medkits==1, "Medkit pickup is stored for later")
	_key(KEY_H)
	_check(game.health == 2, "Carried medkit heals on use")
	for core: Area3D in game.cores:
		await _at(core.interaction_point())
		await preload("res://tests/test_actions.gd").search(self,game,core)
		_check(not core.is_recovered, "Records require F")
		_key(KEY_F)
	_check(game.recovered_count == game.cores.size() and not game.mission.exit_started, "Records are optional lore, not the exit switch")
	_key(KEY_J)
	_check(paused and game.hud.overlay_body.text.contains("Приёмная"), "Journal rereads collected clues")
	_key(KEY_J)
	_check(not paused, "Journal resumes the mission")
	game.flash_charges = 0
	await _use("terminal")
	_check(not game.mission.exit_started, "Old short route cannot bypass research wing")
	for department: String in game.mission.CIRCUITS:
		for i: int in range(3):
			var dial: Node3D = game.mission.objects["dial_%s_%d" % [department, i]]
			for step: int in range(game.mission.CIRCUITS[department][i]):
				game.mission.interact(dial)
		if department == "theatre":
			game.hunters[5].set_active(true)
			game.hunters[5].receive_bullet(game.hero.position, 12, true)
		game.mission.interact(game.mission.objects["relay_" + department])
	_check(game.mission.relays.size() == 3, "Three circuit puzzles unlock the final terminal")
	await _use("terminal")
	_check(not game.mission.exit_started, "Terminal additionally requires lower-sector cleanup")
	await _use("coolant")
	await _use("fuel")
	await _use("reactor")
	_check(game.mission.purge_started and not game.mission.purge_complete, "Reactor starts an active defense")
	await _frames(1000)
	await _use("purge_valve_a")
	await _frames(950)
	await _use("purge_valve_b")
	await _frames(950)
	_check(game.mission.purge_complete, "Defense completes while player stays at reactor")
	await _use("terminal")
	_check(not game.mission.exit_started,"Residential release is required after reactor")
	await _use("residence_panel")
	await _use("residence_prybar")
	await _use("residence_panel")
	await _at(Vector3(77,.1,27))
	_key(KEY_F)
	await _frames(45)
	await _use("residence_panel")
	await _at(Vector3(70,.1,5.3))
	game.hero.camera_rig.rotation.y = 0
	game.combat.knife_attack()
	await _use("residence_core")
	await _use("residence_plate")
	_check(game.residential.plate_latched,"Held manual drive releases lift interlock without a cart")
	await _use("residence_lift")
	await _use("terminal")
	_check(not game.mission.exit_started,"Diagnostic medical release is required after residence")
	game.diagnostics.solve_xray()
	game.diagnostics.use_station("observe")
	game.diagnostics.complete_treatment()
	preload("res://tests/mission_fixture.gd").finish_isolation(game)
	preload("res://tests/mission_fixture.gd").finish_annex(game)
	await _use("terminal")
	_check(game.mission.exit_started and game.escape_remaining > 170, "Terminal starts the expanded timed return journey")
	_check(game.flash_charges == 1, "Powered terminal provides one emergency flash for escape")
	game.mission.interact(game.mission.objects["terminal"])
	_check(game.flash_charges == 1, "Used terminal cannot refill flashes repeatedly")
	await _at(Vector3(-26, 0.1, 12))
	_check(not game.mission.events.has("changed_reception"), "Return event waits for reception, not the annex")
	await _at(Vector3(0, 0.1, 10))
	_check(game.facility.returned_changed and game.facility.return_prop.visible, "Returning changes the familiar reception")
	_check(game.mission.events.has("first_contact") and game.mission.events.has("observation_window") and game.mission.events.has("changed_reception"), "All three distinct story events fired")
	game.mission.fire_event("changed_reception")
	_check(game.mission.events.size() == 3, "Story events cannot duplicate")
	game.diagnostics.breaker_reset = true
	await _at(game.diagnostics.stations.exit_crank.interaction_point())
	Input.action_press("interact_hold")
	game.diagnostics.use_station("exit_crank")
	game.diagnostics.update(5.1)
	Input.action_release("interact_hold")
	await _at(game.facility.EXIT + Vector3(0, 0.1, 0))
	_check(game.has_won and paused, "Completed mission allows escape through reception")
	game.restart_game()
	await _frames(8)
	_check(not paused and not game.has_won and not game.mission.power_on and not game.mission.has_badge, "Restart clears mission state")
	_check(game.mission.events.is_empty() and not game.facility.return_prop.visible and not game.facility.silhouette.visible, "Restart clears all staged events")
	_check(not shortcut.is_open and shortcut.locked and not game.hud.radar.visible, "Restart restores gates and map")
	_check(not game.medkits[0].is_recovered and not game.batteries[0].is_recovered, "Restart restores optional resources")
	# Sight, hearing, memory and recovery from a lost chase.
	var hunter: CharacterBody3D = game.hunters[0]
	hunter.set_active(true)
	hunter.position = Vector3(0, 0.1, 5)
	hunter.model.rotation.y = 0
	hunter.patrol_points.clear()
	await _at(Vector3(0, 0.1, 9))
	_check(not hunter.can_see_player() and hunter.state == hunter.State.PATROL, "Player behind a hunter is not automatically detected")
	hunter.set_physics_process(false)
	game.step_remaining = 0
	Input.action_press("move_forward")
	await _frames(10)
	_release()
	_check(hunter.state == hunter.State.PATROL, "Actual quiet footsteps are not heard at four metres")
	game.step_remaining = 0
	Input.action_press("move_forward")
	Input.action_press("sprint")
	await _frames(10)
	_release()
	_check(hunter.state == hunter.State.INVESTIGATE, "Actual sprint footsteps trigger investigation")
	hunter.set_physics_process(true)
	var heard: Vector3 = hunter.last_known
	await _at(Vector3(12, 0.1, 12))
	_check(hunter.last_known == heard, "Unseen target movement does not update the remembered location")
	hunter.position = Vector3(-4.5, 0.1, 3)
	hunter.model.rotation.y = -PI / 2
	hunter._set_state(hunter.State.PATROL)
	await _at(Vector3(0, 0.1, 3))
	_check(not hunter.can_see_player(), "Walls block vision")
	_check(not hunter.hear_noise(game.hero.position, 5), "Walls muffle quiet noise")
	_check(hunter.hear_noise(game.hero.position, 12), "A loud sound can be heard through a wall")
	hunter.position = Vector3(0, 0.1, 5)
	hunter.model.rotation.y = 0
	await _at(Vector3(0, 0.1, 1))
	_check(hunter.state == hunter.State.CHASE, "Visible player starts a chase")
	var seen: Vector3 = hunter.last_known
	await _at(Vector3(12, 0.1, 12))
	await _frames(90)
	_check(hunter.state == hunter.State.SEARCH and hunter.last_known == seen, "Lost sight leads to searching the last observed position")
	await _frames(1000)
	_check(hunter.state == hunter.State.PATROL, "Unsuccessful search returns to patrol")
	# Physical evasion, finite resources and defeat.
	await _at(Vector3(0, 0.1, 10))
	var dash_start: Vector3 = game.hero.position
	Input.action_press("dash")
	await _frames(12)
	Input.action_release("dash")
	_check(game.hero.position.distance_to(dash_start) > 2 and game.hero.dash_cooldown > 2, "Dash moves physically and has a cooldown")
	await _at(Vector3(0, 0.1, 10))
	hunter.position = Vector3(0, 0.1, 7)
	hunter.velocity = Vector3.ZERO
	game.flash_charges = 1
	game.flash_cooldown = 0
	_check(game.use_flash() == 1 and hunter.stunned_remaining > 3, "Flash stuns a visible hunter")
	_check(game.flash_charges == 0 and game.use_flash() == 0, "Flash consumes a finite charge")
	await _at(Vector3(-4, 0.1, 3.15))
	hunter.position = Vector3(-4, 0.1, -3.15)
	hunter.stunned_remaining = 0
	game.flash_cooldown = 0
	game.flash_charges = 1
	_check(game.use_flash() == 0, "Wall blocks flash")
	await _at(Vector3(0, 0.1, 10))
	hunter.stunned_remaining = 0
	hunter.position = game.hero.position
	hunter.melee_remaining = .28
	await _frames(3)
	_check(game.health == 2 and game.hero.invulnerable_remaining > 1, "Contact damages with a protection window")
	await _frames(10)
	_check(game.health == 2, "Contact cannot remove all health in one instant")
	var death_generation:int=game.run_generation
	for hit: int in range(2):
		game.hero.invulnerable_remaining = 0
		hunter.stunned_remaining = 0
		hunter.position = game.hero.position
		hunter.melee_remaining = .28
		await _frames(2)
	await _frames(6)
	_check(not game.has_lost and not paused and game.run_generation>death_generation, "Three separated hits reload the latest checkpoint automatically")
	game.restart_game()
	await _frames(5)
	game.set_paused(true)
	var frozen: Vector3 = game.hero.position
	Input.action_press("move_forward")
	await _frames(10)
	_check(game.hero.position == frozen and game.hud.overlay.visible, "Pause freezes the world")
	_release()
	game.restart_game()
	await _frames(5)
	game.mission.exit_started = true
	game.escape_remaining = 0.01
	await _frames(8)
	_check(not game.has_lost and not paused and game.escape_remaining<0, "Missed escape deadline automatically reloads progress")
	game.restart_game()
	await _frames(5)
	await _use("fuse")
	await _use("power")
	game.restart_game()
	await _frames(40)
	_check(game.facility.gate_by_id["power_gate"].locked and not game.facility.gate_by_id["power_gate"].is_open and not game.facility.gate_by_id["power_gate"].busy, "Restart cancels a moving mission gate")
	var audio = preload("res://scripts/sound_bank.gd").new()
	root.add_child(audio)
	_check(audio.sounds.size() >= 11, "Sound bank creates environmental cues")
	audio.set_suspended(true)
	_check(audio.ambience.stream_paused, "Pause suspends ambience")
	audio.queue_free()
	await _frames(3)
	# Fixed-FPS tests can finish before the separate audio thread has mixed
	# a single buffer. Allow its stopped playback to be released.
	OS.delay_msec(100)
	_release()
	print("MISSION SMOKE TEST: %d checks, %d failures" % [checks, failures])
	game.queue_free()
	await process_frame
	quit(0 if failures == 0 else 1)

func _at(location: Vector3) -> void:
	game.hero.reset_to(location)
	await _frames(4)

func _use(id: String) -> void:
	await _at(game.mission.objects[id].interaction_point())
	await preload("res://tests/test_actions.gd").search(self,game,game.mission.objects[id])
	_key(KEY_F)
	if id in ["power","reactor","purge_valve_a","purge_valve_b","residence_core","residence_lift","residence_plate"]:
		Input.action_press("interact_hold")
		await _frames(305 if id=="residence_plate" else 245 if id in ["power","residence_lift"] else 155)
		Input.action_release("interact_hold")
	else:
		await _frames(3)

func _key(key: int) -> void:
	var event := InputEventKey.new()
	event.physical_keycode = key
	event.pressed = true
	game._unhandled_input(event)

func _frames(count: int) -> void:
	for frame: int in range(count):
		await physics_frame
		await process_frame

func _release() -> void:
	for action: String in ["move_forward", "move_back", "move_left", "move_right", "sprint", "dash", "jump"]:
		Input.action_release(action)

func _check(condition: bool, description: String) -> void:
	checks += 1
	if not condition:
		failures += 1
		push_error("FAIL: " + description)

func _bfs(start: Vector2i, goal: Vector2i) -> int:
	var queue: Array[Vector2i] = [start]
	var distance: Dictionary = {start: 0}
	var index: int = 0
	while index < queue.size():
		var cell: Vector2i = queue[index]
		index += 1
		if cell == goal:
			return distance[cell]
		for next: Vector2i in game.navigation.neighbors(cell):
			if not distance.has(next):
				distance[next] = int(distance[cell]) + 1
				queue.append(next)
	return -1
