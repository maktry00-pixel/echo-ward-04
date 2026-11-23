extends SceneTree
const Checkpoints = preload("res://scripts/checkpoint_store.gd")
var game: Node3D
var checks: int = 0
var failures: int = 0
func _initialize() -> void: call_deferred("_run")
func _run() -> void:
	game = load("res://scenes/third_person.tscn").instantiate()
	game.set_meta("automated_test",true)
	root.add_child(game)
	await _frames(15)
	var wing: Node3D = game.residential
	for hunter: CharacterBody3D in game.hunters: hunter.stun(500)
	_check(not game.navigation.find_path(Vector3(58,0,-12),Vector3(82,0,-12)).size()>0,"Residential wing is physically sealed before reactor")
	game.mission.power_on = true
	game.mission.has_badge = true
	game.mission.relays.assign(["morgue","lab","theatre"])
	game.mission.has_coolant = true
	game.mission.has_fuel = true
	game.mission.purge_complete = true
	game.mission.objects["reactor"].mark_used()
	game.mission.interact(game.mission.objects["terminal"])
	_check(not game.mission.exit_started,"Reactor alone cannot skip residential chapter")
	game.hero.reset_to(Vector3(58,.1,-12))
	game.interact()
	await _frames(45)
	_check(game.facility.gate_by_id["res_entry"].is_open,"Reactor unlocks physical residential entrance")
	_check(not game.navigation.find_path(Vector3(58,0,-12),Vector3(82,0,-12)).is_empty(),"Navigation includes new entrance and 104m bound")
	await _use("residence_panel")
	await _frames(5)
	_check(wing.mode==2 and game.facility.gate_by_id["res_west_gate"].locked and not game.facility.gate_by_id["res_east_gate"].locked,"One branch receives power at a time")
	_check(not game.navigation.find_path(Vector3(82,0,6),Vector3(99,0,28)).is_empty(),"Powered east gives actual route past cell to tool")
	_check(not wing.can_work(game.mission.objects["residence_core"]),"Tool and service route remain necessary on cold equipment")
	await _use("residence_prybar")
	_check(wing.has_prybar and game.hunters[11].active,"Tool wakes a preplaced crawler with no teleport")
	await _use("residence_panel")
	await _frames(4)
	game.hero.reset_to(Vector3(77,.1,27))
	game.interact()
	await _frames(45)
	_check(wing.service_opened and game.facility.gate_by_id["res_west_service"].is_open,"Tool opens service latch from inside")
	await _use("residence_panel")
	_check(wing.mode==2 and game.facility.gate_by_id["res_west_gate"].locked,"Switching power closes western powered route")
	await _frames(4)
	_check(not game.navigation.find_path(Vector3(82,0,27),Vector3(70,0,4)).is_empty(),"Manual rear route survives loss of branch power")
	_check(not wing.can_work(game.mission.objects["residence_core"]),"Intact combat lock still blocks extraction")
	game.hero.reset_to(Vector3(70,.1,5.3))
	await _frames(5)
	game.hero.camera_rig.rotation.y = 0
	var ammo_before: int = game.combat.rounds
	game.combat.knife_attack()
	_check(wing.lock_target.broken and game.combat.rounds==ammo_before,"Close knife strike opens physical lock without ammunition")
	Input.action_press("interact_hold")
	game.interact()
	await _frames(70)
	_check(not wing.core_extracted and game.hero.performing_action,"Extraction requires real held work")
	Input.action_release("interact_hold")
	await _frames(3)
	_check(not game.hero.performing_action and not wing.core_extracted,"Releasing extraction allows interruption")
	Input.action_press("interact_hold")
	game.interact()
	await _frames(95)
	Input.action_release("interact_hold")
	_check(wing.core_extracted,"Resuming cold repair extracts module")
	_check(game.hunters[13].active and game.facility.gate_by_id["res_cell"].is_open,"Extraction releases caged hunter before manual drive")
	await _frames(4)
	_check(not game.navigation.find_path(Vector3(101,0,5),Vector3(94,0,18)).is_empty(),"Cell opening updates A* immediately before checkpoint load or lift repair")
	game.hero.reset_to(Vector3(82,.1,-12))
	game.hunters[13].stunned_remaining = 0
	game.hunters[13].hear_noise(Vector3(94,.1,18),40)
	await _frames(450)
	_check(game.hunters[13].position.z>10 and game.hunters[13].position.x<98,"Newly released hunter actually enters ward before lift repair")
	game.hunters[13].stun(500)
	_check(not wing.can_work(game.mission.objects["residence_lift"]),"Module alone cannot bypass mechanical safety plate")
	game.hero.reset_to(Vector3(94,.1,18))
	await _frames(8)
	_check(not wing.plate_latched,"Entering the room alone does not release the interlock")
	var mid: Dictionary = JSON.parse_string(JSON.stringify(Checkpoints.snapshot(game)))
	_check(Checkpoints.valid(mid) and Checkpoints.apply(game,mid),"Mid-chapter checkpoint round trip")
	await _frames(8)
	_check(wing.mode==2 and wing.core_extracted and wing.service_opened and wing.lock_target.broken,"Save restores power, tool route and broken lock")
	_check(game.hero.position.distance_to(Vector3(82,.1,27))<.4,"Residential restore uses authored clear spine spawn")
	_check(not wing.plate_latched,"Continue does not grant unsolved plate")
	bad_record_test(mid)
	var bad: Dictionary = mid.duplicate(true)
	bad.residential.mode = 3
	_check(not Checkpoints.valid(bad),"Unbounded branch state is rejected")
	bad = mid.duplicate(true)
	bad.residential.service = false
	_check(not Checkpoints.valid(bad),"Core cannot be loaded without required service route")
	for hunter: CharacterBody3D in game.hunters: hunter.stun(500)
	wing.mode = 1
	wing._refresh_power()
	_check(not wing.can_work(game.mission.objects["residence_plate"]),"Unpowered east cannot engage manual drive")
	wing.mode = 2
	wing._refresh_power()
	var drive: Node3D = game.mission.objects["residence_plate"]
	game.hero.reset_to(drive.interaction_point())
	await _frames(5)
	Input.action_press("interact_hold")
	game.interact()
	await _frames(100)
	Input.action_release("interact_hold")
	await _frames(3)
	_check(not wing.plate_latched and not game.hero.performing_action,"Manual drive takes held work and can be interrupted")
	Input.action_press("interact_hold")
	game.interact()
	await _frames(220)
	Input.action_release("interact_hold")
	_check(wing.plate_latched and drive.used,"Actual F input releases the manual lift interlock")
	_check(get_nodes_in_group("push_carts").is_empty(),"Chapter has no movable cart dependency")
	await _use("residence_lift",true)
	_check(wing.complete and game.facility.gate_by_id["res_entry"].locked and game.facility.gate_by_id["res_return"].is_open,"Lift changes exit route instead of resetting enemies")
	_check(game.hunters[13].active and game.facility.gate_by_id["res_cell"].is_open,"Blackout retains physical roaming threat")
	await _frames(4)
	_check(not game.navigation.find_path(Vector3(94,0,26),Vector3(58,0,26)).is_empty(),"Emergency return route physically reaches operating theatre")
	var final_save: Dictionary = Checkpoints.snapshot(game)
	_check(Checkpoints.apply(game,final_save),"Completed chapter checkpoint loads")
	await _frames(5)
	_check(wing.complete and game.facility.gate_by_id["res_return"].is_open and game.facility.gate_by_id["res_entry"].locked,"Continue preserves changed return geometry")
	_check(wing.plate_latched,"Continue preserves completed mechanical latch")
	var hunter: CharacterBody3D = game.hunters[13]
	hunter.stunned_remaining = 0
	hunter.position = Vector3(94,.1,23)
	game.hero.reset_to(Vector3(94,.1,21))
	_check(hunter.blind_hunter and not hunter.can_see_player(),"Blind hunter cannot obtain player coordinates by sight")
	_check(hunter.model.blindfold,"Blind hunter has distinct covered eyes")
	_check(hunter.hear_noise(Vector3(94,.1,20),6) and hunter.last_known.distance_to(Vector3(94,.1,20))<.01,"Blind hunter follows an actual audible observation")
	hunter.position = Vector3(101,.08,5)
	game.hero.reset_to(Vector3(82,.1,-12))
	await _frames(3)
	hunter.hear_noise(Vector3(94,.1,18),40)
	await _frames(600)
	_check(hunter.position.z>10 and hunter.position.x<98,"Released hunter physically leaves cell and reaches ward by A*")
	hunter.stun(500)
	game.diagnostics.solve_xray()
	game.diagnostics.use_station("observe")
	game.diagnostics.complete_treatment()
	preload("res://tests/mission_fixture.gd").finish_isolation(game)
	preload("res://tests/mission_fixture.gd").finish_annex(game)
	game.mission.interact(game.mission.objects["terminal"])
	_check(game.mission.exit_started,"New chapter enables final timed return")
	game.restart_game()
	_check(not wing.complete and not wing.core_extracted and not wing.lock_target.broken and wing.mode==0,"Restart resets all chapter mechanisms")
	# Reusable lock also accepts actual camera-and-muzzle hitscan, not just knife.
	game.hero.reset_to(Vector3(70,.1,7))
	game.combat.acquire()
	Input.action_press("aim")
	await _frames(20)
	_aim(wing.lock_target.global_position)
	await _frames(2)
	_aim(wing.lock_target.global_position)
	var result: Dictionary = game.combat.fire()
	_check(result.get("hit",false) and wing.lock_target.broken and game.combat.kills==0,"Actual gun ray breaks lock without enemy kill credit")
	Input.action_release("aim")
	game.queue_free()
	await process_frame
	print("RESIDENTIAL: %d checks, %d failures" % [checks,failures])
	quit(0 if failures==0 else 1)
func _use(id: String, held: bool = false) -> void:
	game.hero.reset_to(game.mission.objects[id].position+Vector3(0,.1,1.2))
	await _frames(5)
	await preload("res://tests/test_actions.gd").search(self,game,game.mission.objects[id])
	if held: Input.action_press("interact_hold")
	game.interact()
	await _frames(250 if held else 4)
	Input.action_release("interact_hold")
func bad_record_test(data: Dictionary) -> void:
	var sample: Dictionary = data.duplicate(true)
	sample.records = [8,9]
	_check(Checkpoints.valid(sample),"Both residential story records are valid checkpoint data")
	sample.records = [10]
	_check(not Checkpoints.valid(sample),"Out-of-range story record rejected")
	sample.records = []
	sample.batteries = [2]
	_check(not Checkpoints.valid(sample),"Battery bound is independent from story record bound")
func _frames(count: int) -> void:
	for i: int in range(count):
		await physics_frame
		await process_frame
func _aim(point: Vector3) -> void:
	var camera: Camera3D = game.hero.camera_rig.camera
	var center: Vector2 = camera.get_viewport().get_visible_rect().size/2
	var origin: Vector3 = camera.project_ray_origin(center)
	var direction: Vector3 = camera.project_ray_normal(center)
	var wanted: Vector3 = (point-origin).normalized()
	var yaw_error: float = atan2(-wanted.x,-wanted.z)-atan2(-direction.x,-direction.z)
	var pitch_error: float = asin(wanted.y)-asin(direction.y)
	game.hero.camera_rig.apply_look(Vector2(-wrapf(yaw_error,-PI,PI),-pitch_error)/game.hero.camera_rig.look_sensitivity)
func _check(ok: bool,message: String) -> void:
	checks += 1
	if not ok:
		failures += 1
		push_error("FAIL: "+message)
