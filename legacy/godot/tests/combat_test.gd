extends SceneTree
## Real raycasts, cover, reload, resource and progression regression checks.
const Checkpoints = preload("res://scripts/checkpoint_store.gd")
var game: Node3D
var checks: int = 0
var failures: int = 0

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	game = load("res://scenes/third_person.tscn").instantiate()
	game.set_meta("automated_test", true)
	root.add_child(game)
	await _frames(12)
	_check(not game.combat.equipped and not game.combat.fire()["fired"], "No firearm before pickup")
	game.mission.interact(game.mission.objects["weapon"])
	_check(game.combat.equipped and game.mission.objects["weapon"].used, "Weapon pickup equips and consumes once")
	var hunter: CharacterBody3D = game.hunters[0]
	for enemy: CharacterBody3D in game.hunters:
		enemy.set_physics_process(false)
	game.hero.reset_to(Vector3(0, 0.1, 12))
	hunter.position = Vector3(0, 0.1, 6)
	hunter.set_active(true)
	Input.action_press("aim")
	await _frames(20)
	_check(game.hero.aiming and game.hero.camera_rig.camera.fov < 53, "Aim changes shoulder camera and player state")
	Input.action_press("move_forward")
	Input.action_press("sprint")
	await _frames(15)
	_check(not game.hero.sprinting and game.hero.velocity.length() < 1.8, "Aiming limits movement and sprint")
	Input.action_release("move_forward")
	Input.action_release("sprint")
	await _frames(6)
	_aim(hunter.global_position + Vector3(0, .40 if hunter.crawler else 1.3, 0))
	var result: Dictionary = game.combat.fire()
	_check(result["fired"] and result["hit"] and not result["headshot"] and hunter.health == 5, "Body hit resolves against physical capsule")
	_check(game.combat.rounds == 8 and hunter.stunned_remaining > 0, "Shot consumes a round and staggers")
	_check(not game.combat.fire()["fired"] and game.combat.rounds == 8, "Fire rate prevents duplicate shots")
	await _frames(25)
	_aim(hunter.aim_point())
	result = game.combat.fire()
	_check(result["headshot"] and hunter.health == 2, "Head shot has increased damage")
	await _frames(25)
	_aim(hunter.aim_point())
	result = game.combat.fire()
	_check(result["killed"] and hunter.dead and not hunter.active and hunter.collision_layer == 0, "Lethal hit removes combat and bullet collision")
	hunter.set_active(true)
	_check(not hunter.active, "Mission events cannot revive a corpse")
	_check(game.combat.reload_weapon(), "Partial magazine can reload")
	_check(not game.combat.fire()["fired"], "Reload blocks shooting")
	game.set_paused(true)
	var remaining: float = game.combat.reload_remaining
	await _frames(100)
	_check(game.combat.reload_remaining == remaining and not game.combat.overlay.visible, "Pause freezes reload and hides combat HUD")
	game.set_paused(false)
	await _frames(110)
	_check(game.combat.rounds == 9 and game.combat.reserve == 9, "Reload transfers only missing rounds from scarce reserve")
	game.combat.rounds = 0
	game.combat.reserve = 2
	game.combat.reload_weapon()
	await _frames(110)
	_check(game.combat.rounds == 2 and game.combat.reserve == 0, "Reload handles insufficient reserve")
	# Shoulder camera can see an enemy while the hand is behind a wall.
	hunter.reset_hunter()
	hunter.position = Vector3(9, 0.1, 2)
	hunter.set_active(true)
	game.hero.reset_to(Vector3(0, 0.1, 2))
	await _frames(6)
	var camera: Camera3D = game.hero.camera_rig.camera
	camera.global_position = Vector3(5, 1.5, 2)
	_aim(hunter.global_position + Vector3(0, 1.3, 0))
	game.combat.aiming = true
	game.combat.cooldown = 0
	result = game.combat.fire()
	_check(result["fired"] and not result["hit"] and hunter.health == 6, "Muzzle ray prevents firing through shoulder cover")
	# Knife is a finite-stamina fallback when no ammunition remains.
	game.hero.reset_to(Vector3(0, 0.1, 10))
	hunter.position = Vector3(0, 0.1, 8.5)
	await _frames(5)
	game.combat.cooldown = 0
	game.combat.rounds = 0
	game.combat.reserve = 0
	game.hero.stamina = 100
	_check(game.combat.knife_attack() and hunter.health == 5 and game.hero.stamina == 80, "Knife damages in arc and spends stamina")
	await _frames(1)
	_check(game.combat.flash.light_energy == 0, "Knife never triggers firearm muzzle flash")
	game.combat.cooldown = 0
	game.hero.stamina = 10
	_check(not game.combat.knife_attack(), "Exhaustion blocks knife")
	game.restart_game()
	await _frames(8)
	_check(not hunter.dead and hunter.health == 6 and game.combat.rounds == 9 and not game.combat.equipped, "Restart resets enemies and weapon")
	game.mission.has_fuse = true
	game.mission.interact(game.mission.objects["power"])
	await _frames(45)
	game.mission.interact(game.mission.objects["badge"])
	game.mission.interact(game.mission.objects["terminal"])
	_check(not game.mission.exit_started, "Badge alone cannot finish the short route")
	var gate: Node3D = game.facility.gate_by_id["research_gate"]
	gate.locked = false
	gate.toggle_door(gate.position + Vector3(-3, 0, 0))
	await _frames(45)
	for department: String in game.mission.CIRCUITS:
		var relay: Node3D = game.mission.objects["relay_" + department]
		var route: Array[Vector3] = game.navigation.find_path(Vector3(0, 0.1, -12), relay.position)
		_check(route.is_empty() if department=="theatre" else not route.is_empty(), department + " relay respects guardian door / actual colliders")
		game.mission.interact(relay)
		_check(not relay.used, "Wrong circuit cannot activate " + department)
		for i: int in range(3):
			var dial: Node3D = game.mission.objects["dial_%s_%d" % [department, i]]
			for step: int in range(game.mission.CIRCUITS[department][i]):
				game.mission.interact(dial)
		if department == "theatre":
			game.mission.interact(relay)
			_check(not relay.used, "Living guardian blocks final relay")
			game.hunters[5].set_active(true)
			game.hunters[5].receive_bullet(game.hero.position, 12, true)
		game.mission.interact(relay)
		_check(relay.used and department in game.mission.relays, "Correct circuit activates " + department)
	game.combat.equipped = true
	game.combat.rounds = 4
	game.combat.reserve = 11
	game.mission.interact(game.mission.objects["ammo_lab"])
	_check(game.combat.reserve == 19, "Ammo pickup adds eight rounds")
	game.mission.interact(game.mission.objects["ammo_lab"])
	_check(game.combat.reserve == 19, "Ammo pickup cannot duplicate")
	game.health = 1
	game.hero.reset_to(game.medkits[2].position + Vector3(0, 0.1, 0))
	await preload("res://tests/test_actions.gd").search(self,game,game.medkits[2])
	game.medkits[2].try_collect(game.hero)
	_check(game.medkits[2].is_recovered and game.health==1 and game.inventory.medkits==1, "Research medkit is carried")
	game.inventory.use_item("medkit")
	_check(game.health==2 and game.inventory.medkits==0, "Research medkit heals once on use")
	var data: Dictionary = Checkpoints.snapshot(game)
	_check(Checkpoints.valid(data), "Expanded checkpoint validates")
	var file := FileAccess.open("res://artifacts/combat_checkpoint.json", FileAccess.WRITE)
	file.store_string(JSON.stringify(data))
	file.close()
	var disk: Variant = JSON.parse_string(FileAccess.get_file_as_string("res://artifacts/combat_checkpoint.json"))
	_check(Checkpoints.apply(game, disk), "Expanded checkpoint survives JSON round trip")
	await _frames(8)
	_check(game.mission.relays.size() == 3 and game.hunters[5].dead and not game.hunters[5].active, "Continue preserves relays and dead guardian")
	_check(game.combat.rounds == 4 and game.combat.reserve == 19 and game.mission.objects["ammo_lab"].used, "Continue preserves ammo and collected boxes")
	_check(game.medkits[2].is_recovered and not game.medkits[1].is_recovered, "Continue preserves individual research medkits")
	data["relays"] = ["morgue", "morgue", "theatre"]
	_check(not Checkpoints.valid(data), "Duplicate relays cannot bypass progress")
	game.mission.interact(game.mission.objects["terminal"])
	_check(not game.mission.exit_started, "Three relays cannot bypass reactor chapter")
	game.mission.has_coolant = true
	game.mission.has_fuel = true
	game.mission.purge_complete = true
	game.mission.interact(game.mission.objects["terminal"])
	_check(not game.mission.exit_started,"Completed reactor still needs residential release")
	game.residential.complete = true
	game.diagnostics.solve_xray()
	game.diagnostics.use_station("observe")
	game.diagnostics.complete_treatment()
	preload("res://tests/mission_fixture.gd").finish_isolation(game)
	preload("res://tests/mission_fixture.gd").finish_annex(game)
	game.mission.interact(game.mission.objects["terminal"])
	_check(game.mission.exit_started, "Completed residential and diagnostic releases enable final return")
	game.restart_game()
	_check(game.mission.relays.is_empty() and game.mission.objects["dial_lab_0"].state_value == 0 and not game.mission.objects["ammo_lab"].used and not game.hunters[5].dead, "Restart resets all expansion progress")
	Input.action_release("aim")
	game.queue_free()
	await process_frame
	print("COMBAT AND EXPANSION: %d checks, %d failures" % [checks, failures])
	quit(0 if failures == 0 else 1)

func _aim(point: Vector3) -> void:
	# Center this isolated test camera; shoulder offsets are checked separately.
	game.hero.camera_rig.camera.h_offset = 0
	game.hero.camera_rig.camera.v_offset = 0
	game.hero.camera_rig.camera.look_at(point)

func _frames(count: int) -> void:
	for frame: int in range(count):
		await physics_frame
		await process_frame

func _check(condition: bool, description: String) -> void:
	checks += 1
	if not condition:
		failures += 1
		push_error("FAIL: " + description)
