extends SceneTree
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
	await _frames(15)
	for enemy: CharacterBody3D in game.hunters:
		enemy.stun(500)
	_check(game.hunters.size() == 25 and game.hunters[3].can_spit and game.hunters[9].can_spit and game.hunters[12].can_spit, "Roster includes ranged, residential, diagnostic, ceiling, isolation and annex encounters")
	game.hero.reset_to(Vector3(52, 0.1, 30))
	_key(KEY_F)
	_check(game.facility.gate_by_id["technical_gate"].locked, "Technical door cannot bypass three relays")
	game.mission.relays.assign(["morgue", "lab", "theatre"])
	_key(KEY_F)
	await _frames(50)
	_check(game.facility.gate_by_id["technical_gate"].is_open, "Three relays allow technical gate interaction")
	Input.action_press("move_back")
	await _frames(330)
	Input.action_release("move_back")
	await _frames(6)
	_check(game.hero.position.z > 45 and game.hero.position.y < -4.8 and game.hero.is_on_floor(), "Actual movement descends ramp to lower sector")
	Input.action_press("move_forward")
	await _frames(390)
	Input.action_release("move_forward")
	await _frames(6)
	_check(game.hero.position.z < 32 and game.hero.position.y > -0.1 and game.hero.is_on_floor(), "Actual movement climbs ramp without jumping: " + str(game.hero.position))
	game.mission.has_badge = true
	game.mission.power_on = true
	game.mission.interact(game.mission.objects["reactor"])
	_check(not game.mission.purge_started, "Reactor needs both supplies")
	game.mission.interact(game.mission.objects["coolant"])
	game.mission.interact(game.mission.objects["fuel"])
	game.hero.reset_to(Vector3(55, -4.9, 76))
	game.mission.interact(game.mission.objects["reactor"])
	await _frames(60)
	_check(game.mission.purge_started and game.mission.purge_remaining < 45 and game.hunters[8].active, "Purge advances and wakes preplaced lower enemies")
	game.hero.reset_to(Vector3(52, -4.9, 48))
	var before: float = game.mission.purge_remaining
	await _frames(60)
	_check(game.mission.purge_remaining == before, "Leaving reactor radius pauses progress")
	game.hero.reset_to(Vector3(55, -4.9, 76))
	game.set_paused(true)
	await _frames(60)
	_check(game.mission.purge_remaining == before, "Application pause freezes purge")
	game.set_paused(false)
	await _frames(1000)
	_check(game.mission.purge_fault == "purge_valve_a", "First pressure fault interrupts cleanup")
	game.mission.interact(game.mission.objects["purge_valve_a"])
	await _frames(950)
	_check(game.mission.purge_fault == "purge_valve_b", "Second fault requires a different location")
	game.mission.interact(game.mission.objects["purge_valve_b"])
	await _frames(950)
	_check(game.mission.purge_complete and game.mission.objects["reactor"].used, "Surviving defense completes reactor")
	var data: Dictionary = JSON.parse_string(JSON.stringify(Checkpoints.snapshot(game)))
	_check(Checkpoints.valid(data) and Checkpoints.apply(game, data), "Lower-sector checkpoint round trip loads")
	await _frames(8)
	_check(game.mission.purge_complete and game.hero.position.y < -4.8 and game.mission.has_fuel and game.mission.has_coolant and game.mission.serviced_valves.size()==2 and game.mission.objects["purge_valve_b"].used, "Continue restores reactor completion, both serviced valves and authored lower spawn")
	_check(game.layout_seed == 1907 and game.resource_layout.last_result["success"], "Continue reproduces CSP layout seed")
	game.restart_game()
	_check(not game.mission.purge_started and not game.mission.has_fuel and game.facility.gate_by_id["technical_gate"].locked, "Restart resets lower chapter and its gate")
	for enemy: CharacterBody3D in game.hunters:
		enemy.stun(500)
	game.hero.reset_to(Vector3(0, 0.1, 2))
	game._spawn_projectile(Vector3(8, 1.1, 2), Vector3(0, 1.1, 2))
	await _frames(70)
	_check(game.health == 3, "Acid projectile cannot cross solid wall")
	game.hero.reset_to(Vector3(0, 0.1, 10))
	game._spawn_projectile(Vector3(0, 1.1, 6), Vector3(0, 1.1, 10))
	await _frames(40)
	_check(game.health == 2 and game.hero.invulnerable_remaining > 0, "Visible projectile hits real hero collider once")
	var enemy: CharacterBody3D = game.hunters[0]
	enemy.reset_hunter()
	enemy.set_active(true)
	enemy.receive_bullet(game.hero.position, 1, false)
	enemy.stunned_remaining = 0
	enemy.receive_bullet(game.hero.position, 1, false)
	_check(enemy.health == 4 and enemy.stunned_remaining == 0, "Repeated body hits cannot maintain permanent stun")
	enemy = game.hunters[3]
	enemy.set_active(true)
	enemy.windup = 0.5
	enemy.receive_bullet(game.hero.position, 3, true)
	_check(enemy.windup == 0, "Accurate head hit interrupts ranged windup")
	_key(KEY_F3)
	await _frames(2)
	_check(game.hud.ai_label.visible and game.hud.ai_label.text.contains("Minimax") and game.hud.ai_label.text.contains("CSP"), "F3 displays actual academic algorithm diagnostics")
	game.queue_free()
	await process_frame
	print("LOWER SECTOR: %d checks, %d failures" % [checks, failures])
	quit(0 if failures == 0 else 1)
func _frames(count: int) -> void:
	for i: int in range(count):
		await physics_frame
		await process_frame
func _key(code: int) -> void:
	var event := InputEventKey.new()
	event.physical_keycode = code
	event.pressed = true
	game._unhandled_input(event)
func _check(condition: bool, description: String) -> void:
	checks += 1
	if not condition:
		failures += 1
		push_error("FAIL: " + description)
