extends SceneTree
const Saves = preload("res://scripts/checkpoint_store.gd")
var game: Node3D
var chapter: Node3D
var checks: int = 0
var failures: int = 0
func _initialize() -> void: call_deferred("_run")
func _run() -> void:
	game = load("res://scenes/third_person.tscn").instantiate(); game.set_meta("automated_test",true); root.add_child(game)
	await _frames(18); chapter = game.annex; _freeze()
	_check(chapter.actors.size()==4 and game.hunters.size()==25,"Four preplaced annex actors preserve existing encounters")
	_check(chapter.gates.entry.locked and chapter.gates.steam.locked,"Annex and northern rooms begin physically locked")
	chapter.use_station("brief")
	_check(chapter.steps.is_empty(),"Old isolation must be completed before entering annex progression")
	for id: String in chapter.stations:
		var station: Node3D = chapter.stations[id]
		game.hero.reset_to(station.interaction_point()+Vector3(0,.1,0)); await _frames(5)
		var shape := SphereShape3D.new(); shape.radius = .34
		var query := PhysicsShapeQueryParameters3D.new(); query.shape = shape; query.transform.origin = game.hero.position+Vector3(0,1,0); query.collision_mask = 1
		_check(game.get_world_3d().direct_space_state.intersect_shape(query).is_empty() and game._prop_reachable(station) and game.hero.is_on_floor(),"Safe stance, floor and real reach: "+id)
	_fixture(); chapter.sync(); await _frames(5)
	_check(not game.navigation.find_path(Vector3(41,.1,-102),chapter.stations.brief.interaction_point()).is_empty(),"Actual north doorway connects old isolation to new post")
	chapter.use_station("pump"); chapter.use_station("release")
	_check(not chapter.complete and not chapter.done("pump"),"Pump and finale cannot skip prerequisites")
	await _use("brief")
	_check(chapter.done("brief") and not chapter.gates.stores.locked,"Reading route sheet opens actual spare-parts gate")
	var all_ids: Array = chapter.actors.map(func(actor:Node)->int:return actor.get_instance_id())
	await _open("wheel")
	game.inventory.medkits = 7
	chapter.use_station("wheel")
	_check(not chapter.done("wheel") and chapter.work_id.is_empty(),"Full inventory leaves handwheel in physical drawer")
	game.inventory.medkits = 0
	chapter.use_station("wheel"); Input.action_press("interact_hold"); chapter.update(1); Input.action_release("interact_hold"); chapter.update(.1)
	_check(not chapter.done("wheel") and chapter.work_id.is_empty() and not game.hero.performing_action,"Releasing F cancels unfinished search")
	await _work("wheel",3)
	_check(chapter.done("wheel") and game.inventory.entries().any(func(item:Dictionary)->bool:return item.id=="annex_wheel"),"Real search collects handwheel into protected quest inventory slot")
	_check(chapter.awakened==[0] and chapter.actors[0].active,"Search awakens the existing workshop crawler once")
	_check(Saves.valid(JSON.parse_string(JSON.stringify(Saves.snapshot(game)))),"Checkpoint validates while the handwheel still occupies a quest slot")
	_check(not game.inventory.drop_item("annex_wheel"),"Essential wheel cannot be discarded or strand progression")
	_check(game.saves.capture("manual_6"),"Manual slot 6 captures exact location in expanded map")
	_check(Saves.valid(JSON.parse_string(JSON.stringify(game.saves.slots.manual_6))),"Exact annex save survives JSON with all 25 enemy positions")
	game.inventory.drop_item("bottle")
	_check(Saves.valid(JSON.parse_string(JSON.stringify(Saves.snapshot(game)))),"Dropped supplies in the far north remain valid save data")
	await _work("manual",3); await _work("intake",3); await _work("drain",3)
	await _work("pump",4.1)
	_check(chapter.done("pump") and not chapter.gates.steam.locked,"Installing wheel opens actual steam barrier")
	_check(not game.inventory.entries().any(func(item:Dictionary)->bool:return item.id=="annex_wheel"),"Installed handwheel frees inventory slot")
	chapter.use_station("boiler")
	_check(not chapter.done("boiler"),"Incorrect pressure values cannot pass boiler puzzle")
	for pair: Array in [["dial_a",2],["dial_b",1],["dial_c",3]]:
		for i: int in range(pair[1]): chapter.use_station(pair[0])
	await _use("boiler")
	_check(chapter.done("boiler") and chapter.values==[2,1,3],"Three independent controls solve pressure from journal clue")
	await _work("key",3); await _work("register",3); await _work("files",5.1)
	_check(chapter.done("files") and not chapter.gates.w183.locked,"Actual archive search releases control-room return door")
	await _open("antenna"); Input.action_press("interact_hold"); chapter.use_station("antenna"); chapter.update(1)
	game.set_paused(true)
	_check(chapter.work_id.is_empty() and not game.hero.performing_action,"Opening pause cancels annex work and releases the character")
	Input.action_release("interact_hold"); game.set_paused(false)
	await _work("antenna",4.1); chapter.use_station("radio")
	_check(not chapter.done("radio") and chapter.work_id.is_empty(),"Transmitter rejects wrong frequency even with antenna powered")
	chapter.use_station("frequency"); chapter.use_station("frequency")
	game.hero.reset_to(chapter.stations.radio.interaction_point()); chapter.use_station("radio"); Input.action_press("interact_hold"); chapter.update(2)
	game.hero.position += Vector3(4,0,0); chapter.update(.1); Input.action_release("interact_hold")
	_check(not chapter.done("radio") and chapter.work_id.is_empty(),"Leaving transmitter cancels transmission and keeps encounter active")
	await _work("radio",6.1); await _use("release")
	_check(chapter.complete and chapter.awakened==[0,1,2,3],"Complete physical quest chain awakens four authored encounters")
	var score: int = game.score; chapter.use_station("release")
	_check(game.score==score and chapter.actors.map(func(actor:Node)->int:return actor.get_instance_id())==all_ids,"Repeated interactions neither duplicate rewards nor spawn enemies")
	_check(chapter.journal_entries().size()==5,"All important clues and return route remain in journal")
	var route: Array[String] = ["brief","wheel","manual","intake","drain","pump","dial_a","dial_b","dial_c","boiler","key","register","files","antenna","frequency","radio","release"]
	var capsule := CapsuleShape3D.new(); capsule.radius = .34; capsule.height = 1.7
	var probe := PhysicsShapeQueryParameters3D.new(); probe.shape = capsule; probe.collision_mask = 1
	var blocked: Array[String] = []
	for i: int in range(1,route.size()):
		var path: Array[Vector3] = game.navigation.find_path(chapter.stations[route[i-1]].interaction_point(),chapter.stations[route[i]].interaction_point())
		if path.is_empty(): blocked.append(route[i]+" / no path")
		for at: Vector3 in path:
			probe.transform.origin = at+Vector3(0,.87,0)
			if not game.get_world_3d().direct_space_state.intersect_shape(probe,1).is_empty(): blocked.append(route[i]+" / "+str(at))
	_check(blocked.is_empty(),"All required northern A* routes clear the full player capsule, including boiler plinths: "+str(blocked.slice(0,5)))
	var saved: Dictionary = JSON.parse_string(JSON.stringify(Saves.snapshot(game)))
	_check(Saves.valid(saved),"Full completed annex checkpoint validates after JSON serialization")
	_check(Saves.apply(game,saved),"Full checkpoint restores expanded chapter")
	await _frames(6); _freeze()
	_check(chapter.complete and chapter.values==[2,1,3] and chapter.frequency==2,"Save load preserves puzzle controls, radio and finale")
	_check(game.hero.position.z < -108,"Checkpoint returns to annex instead of older southern sector")
	var bad: Dictionary = saved.duplicate(true); bad.annex.steps.erase("wheel")
	_check(not Saves.valid(bad),"Save without prerequisite handwheel is rejected")
	bad = saved.duplicate(true); bad.annex.frequency = 1
	_check(not Saves.valid(bad),"Completed transmission with wrong tuning is rejected")
	bad = saved.duplicate(true); bad.annex.positions[0][2] = -9999
	_check(not Saves.valid(bad),"Out-of-world annex enemy position is rejected")
	bad = saved.duplicate(true); bad.annex["values"] = [9,1,3]
	_check(not Saves.valid(bad),"Invalid pressure control state is rejected")
	bad = saved.duplicate(true); bad.erase("annex"); bad.enemies.resize(21)
	_check(Saves.valid(bad) and Saves.apply(game,bad),"Older 21-enemy save without annex remains loadable")
	await _frames(5); _freeze()
	_check(not chapter.complete and chapter.steps.is_empty(),"Legacy load starts the annex without fabricated progress")
	game.restart_game(); await _frames(5)
	_check(chapter.steps.is_empty() and chapter.values==[0,0,0] and chapter.gates.entry.locked,"New game resets puzzles, drawers and story gate")
	var file := FileAccess.open("res://artifacts/annex_test.json",FileAccess.WRITE); file.store_string(JSON.stringify({"checks":checks,"failures":failures},"\t")); file.close()
	print("ANNEX CHAPTER: %d checks, %d failures"%[checks,failures]); game.queue_free(); await process_frame; quit(1 if failures else 0)
func _fixture() -> void:
	game.mission.power_on = true; game.mission.has_badge = true; game.mission.relays.assign(["morgue","lab","theatre"])
	game.mission.has_fuel = true; game.mission.has_coolant = true; game.mission.purge_complete = true
	game.residential.restore({"mode":2,"prybar":true,"service":true,"broken":true,"core":true,"complete":true,"plate":true,"intro":true,"east":true})
	game.diagnostics.xray_solved = true; game.diagnostics.observed = true; game.diagnostics.complete = true
	game.inventory.supplies_installed = true
	preload("res://tests/mission_fixture.gd").finish_isolation(game)
func _freeze() -> void:
	for hunter: CharacterBody3D in game.hunters: hunter.set_active(false); hunter.set_physics_process(false)
func _open(id: String) -> void:
	game.hero.reset_to(chapter.stations[id].interaction_point()+Vector3(0,.1,0)); game.hero.camera_rig.rotation.y = 0; await _frames(5)
	if chapter.stations[id].drawer!=null and not chapter.stations[id].opened: game.interact(); await _frames(24)
func _use(id: String) -> void:
	await _open(id); game.interact(); await _frames(2)
func _work(id: String,duration: float) -> void:
	await _open(id); Input.action_press("interact_hold"); game.interact(); chapter.update(duration); Input.action_release("interact_hold")
func _frames(count: int) -> void:
	for i: int in range(count): await physics_frame; await process_frame
func _check(ok: bool,message: String) -> void:
	checks+=1
	if not ok: failures+=1; push_error("FAIL: "+message)
