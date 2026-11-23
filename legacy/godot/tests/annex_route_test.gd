extends SceneTree
var game: Node3D
func _initialize() -> void: call_deferred("_run")
func _run() -> void:
	game = load("res://scenes/third_person.tscn").instantiate(); game.set_meta("automated_test",true); root.add_child(game)
	for i: int in range(18): await physics_frame; await process_frame
	# Isolated segment fixture only. annex_playthrough.gd completes all previous chapters normally.
	game.mission.power_on = true; game.mission.has_badge = true; game.mission.relays.assign(["morgue","lab","theatre"])
	game.mission.has_fuel = true; game.mission.has_coolant = true; game.mission.purge_complete = true
	game.residential.restore({"mode":2,"prybar":true,"service":true,"broken":true,"core":true,"complete":true,"plate":true,"intro":true,"east":true})
	game.diagnostics.xray_solved = true; game.diagnostics.observed = true; game.diagnostics.complete = true
	game.inventory.supplies_installed = true; game.combat.acquire()
	preload("res://tests/mission_fixture.gd").finish_isolation(game); game.annex.sync()
	for i: int in range(21): game.hunters[i].set_active(false); game.hunters[i].set_physics_process(false)
	game.hero.reset_to(Vector3(41,.1,-102))
	var pilot: Node = game.admin.pilot; pilot.running = true; pilot.run_id+=1; pilot._plan()
	var timing: Dictionary = {}; var ok: bool = true
	for id: String in pilot.itinerary:
		if not id.begins_with("annex_"): continue
		if not await pilot._visit(id): ok = false; break
		pilot.release_controls(); timing[id] = snappedf(game.elapsed,.1); print("ANNEX ROUTE ",id," at ",timing[id])
		for i: int in range(5): await physics_frame; await process_frame
	var report := {"completed":ok and game.annex.complete,"timing":timing,"annex":game.annex.snapshot(),"health":game.health,"position":[game.hero.position.x,game.hero.position.y,game.hero.position.z],"note":"Isolated annex route with preceding chapters as fixture. Actual annex movement, combat, inventory and interactions; no teleportation after activation."}
	var file := FileAccess.open("res://artifacts/annex_route.json",FileAccess.WRITE); file.store_string(JSON.stringify(report,"\t")); file.close()
	print("ANNEX ROUTE COMPLETE ",JSON.stringify(report)); pilot.stop(); game.queue_free(); await process_frame; quit(0 if ok else 1)
