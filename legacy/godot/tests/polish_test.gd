extends SceneTree
const Saves = preload("res://scripts/checkpoint_store.gd")
var game: Node3D
var checks: int = 0
var failures: int = 0
func _initialize() -> void: call_deferred("_run")
func _run() -> void:
	game = load("res://scenes/third_person.tscn").instantiate()
	game.set_meta("automated_test",true)
	root.add_child(game)
	await _frames(16)
	_freeze_hunters()
	_check(game.facility.get_node_or_null("OpeningArt")!=null,"Authored first-zone dressing is part of actual scene")
	_check(game.refuge.threshold.collision_layer==16 and game.refuge.threshold.collision_mask==4,"Refuge threshold blocks hunter collision only")
	_check(not game.navigation.find_path(Vector3(8,.1,12),game.refuge.SPAWN).is_empty(),"A* finds real passage from reception to refuge")
	game.hero.reset_to(Vector3(8,.1,12))
	game.hero.camera_rig.rotation.y = PI
	Input.action_press("move_forward")
	await _frames(145)
	Input.action_release("move_forward")
	await _frames(8)
	_check(game.hero.position.z>18 and game.hero.in_refuge and game.refuge.inside,"Real WASD walking enters safe room through physical doorway")
	_check(game.refuge.discovered and game.exploration.visited.has("refuge"),"Entering refuge discovers its map zone")
	var before: int = game.health
	game._take_hit(game.hunters[0])
	game.take_projectile_hit()
	_check(game.health==before,"Refuge prevents both melee and acid damage")
	game.combat.acquire()
	Input.action_press("aim")
	await _frames(3)
	_check(not game.combat.fire().get("fired",false),"Refuge cannot be used as a shooting bunker")
	_check(not game.survival_tools.throw_bottle() and game.use_flash()==0,"Peaceful zone prevents offensive bottle and flash")
	Input.action_release("aim")
	var enemy: CharacterBody3D = game.hunters[0]
	enemy.position = Vector3(8,.1,14)
	enemy.set_active(true)
	_check(not enemy.can_see_player() and not enemy.hear_noise(game.hero.position,50),"Refuge suppresses new sight/hearing observations")
	for i: int in range(100):
		enemy.velocity = Vector3(0,0,5)
		enemy.move_and_slide()
		await _frames(1)
	_check(enemy.position.z<15.9,"Actual enemy move_and_slide cannot cross open refuge threshold")
	enemy.set_physics_process(true)
	enemy.hear_noise(Vector3(8,.1,15),12)
	await _frames(420)
	_check(enemy.position.z<15.9 and not enemy.sees_player,"Live AI searches outside refuge instead of pursuing hidden player through threshold")
	enemy.set_active(false)
	enemy.set_physics_process(false)
	game.hero.reset_to(game.refuge.stations.stash.interaction_point())
	await _frames(5)
	var look: Vector3 = game.refuge.stations.stash.position-game.hero.position
	game.hero.camera_rig.rotation.y = atan2(-look.x,-look.z)
	game.combat.reserve = 29
	game.inventory.medkits = 1
	game.inventory.batteries = 1
	game.survival_tools.bottles = 2
	game.interact()
	_check(game.refuge.screen.mode=="stash" and game.is_paused,"Actual F target opens stash interface")
	game.refuge.screen.transfer_buttons.ammo_deposit.pressed.emit()
	_check(game.combat.reserve==5 and game.refuge.stash.ammo==24,"Stash button deposits existing stack, no new ammo")
	game.refuge.screen.transfer_buttons.ammo_take.pressed.emit()
	_check(game.combat.reserve==29 and game.refuge.stash.ammo==0,"Stash button retrieves exact stack")
	game.refuge.screen.transfer_buttons.medkit_deposit.pressed.emit()
	_check(game.inventory.medkits==0 and game.refuge.stash.medkit==1,"Medicine can be stored for return visit")
	game.inventory.medkits = 3
	var stored: int = game.refuge.stash.medkit
	game.refuge.screen.transfer_buttons.medkit_take.pressed.emit()
	_check(game.refuge.stash.medkit==stored and game.inventory.medkits==3 and not game.refuge.screen.message.text.is_empty(),"Full eight-slot pack rejects button withdrawal without losing item and explains why")
	game.refuge.stash.ammo = 24
	_check(game.refuge.transfer("ammo",false) and game.combat.reserve==48 and game.refuge.stash.ammo==5,"Full pack can top up an existing partial ammo cell")
	game.inventory.medkits = 2
	game.combat.reserve = 70
	game.refuge.stash.ammo = 10
	_check(game.refuge.transfer("ammo",false) and game.combat.reserve==72 and game.refuge.stash.ammo==8,"Withdrawal respects 72-round limit without losing remainder")
	game.combat.reserve = 29
	game.refuge.stash.ammo = 0
	game.inventory.medkits = 0
	game.health = 1
	game.flash_charges = 0
	game.combat.shots_fired = 4
	game.combat.hits = 3
	game.combat.kills = 1
	var recorded_time: float = game.elapsed
	game.refuge.screen.save_button.pressed.emit()
	_check(game.checkpoint.get("manual_refuge",false) and game.checkpoint.stage=="start" and Saves.valid(game.checkpoint),"Journal saves valid progress before power restoration")
	var data: Dictionary = JSON.parse_string(JSON.stringify(game.checkpoint))
	game.refuge.screen.dismiss()
	_check(Saves.apply(game,data),"Manual safe-room checkpoint round trips through JSON")
	await _frames(5)
	_freeze_hunters()
	_check(game.hero.position.distance_to(game.refuge.SPAWN)<.3 and game.hero.in_refuge,"Continue spawns in authored refuge interior")
	_check(game.elapsed>=recorded_time and game.elapsed<recorded_time+1,"Continue preserves elapsed mission time")
	_check(game.combat.shots_fired==4 and game.combat.hits==3 and game.combat.kills==1,"Continue preserves combat counters")
	_check(not game.mission.power_on and not game.mission.objects.power.used and not game.mission.has_badge,"Early save cannot grant power or badge")
	_check(game.health==1 and game.flash_charges==0,"Manual save neither heals nor recharges")
	_check(game.refuge.stash.medkit==1 and game.inventory.medkits==0 and game.combat.reserve==29,"Continue preserves separate carried and stored supplies")
	var bad: Dictionary = data.duplicate(true)
	bad.refuge.stash.ammo = -1
	_check(not Saves.valid(bad),"Invalid stash count is rejected")
	bad = data.duplicate(true)
	bad.pacing.variants.ward = 4
	_check(not Saves.valid(bad),"Invalid narrative variant is rejected")
	bad = data.duplicate(true)
	bad.manual_refuge = false
	_check(not Saves.valid(bad),"Unpowered stage requires actual manual-refuge save marker")
	game.refuge.screen.open_view("records")
	_check(game.refuge.screen.mode=="records" and game.is_paused,"Refuge has readable in-world shift notes")
	game.refuge.screen._input(_key(KEY_ESCAPE))
	_check(not game.is_paused and game.refuge.screen.mode=="closed","Esc closes refuge notes and resumes")
	# Running reactor state must survive a voluntary trip back to the refuge.
	game.mission.power_on = true
	game.mission.has_badge = true
	game.mission.relays.assign(["morgue","lab","theatre"])
	game.mission.has_fuel = true
	game.mission.has_coolant = true
	game.mission.purge_started = true
	game.mission.purge_remaining = 15
	game.mission.purge_fault = "purge_valve_b"
	game.mission.serviced_valves.assign(["purge_valve_a"])
	game.inventory.supplies_installed = true
	_check(game.save_checkpoint(true) and Saves.valid(game.checkpoint),"Mid-reactor manual save validates")
	_check(Saves.apply(game,JSON.parse_string(JSON.stringify(game.checkpoint))),"Mid-reactor checkpoint loads")
	await _frames(4)
	_freeze_hunters()
	_check(game.mission.purge_started and game.mission.purge_remaining==15 and game.mission.purge_fault=="purge_valve_b" and game.mission.serviced_valves==["purge_valve_a"],"Continue preserves reactor timer, fault and serviced valve")
	game.mission.exit_started = true
	_check(not game.save_checkpoint(true),"Final escape cannot be paused indefinitely through manual saves")
	game.restart_game()
	await _frames(5)
	_freeze_hunters()
	_check(game.refuge.stash.medkit==0 and not game.refuge.discovered,"New game clears refuge storage and discovery")
	var variants: Dictionary = {}
	for seed_number: int in range(20):
		game.pacing.reset_pacing(seed_number)
		variants[str(game.pacing.variants)] = true
	_check(variants.size()>5,"Different run seeds yield multiple authored scene combinations")
	game.pacing.reset_pacing(1907)
	var expected: Dictionary = game.pacing.variants.duplicate()
	game.pacing.reset_pacing(1907)
	_check(expected==game.pacing.variants,"Same seed gives reproducible staging for tests")
	game.hero.reset_to(Vector3(9,.1,4))
	game.mission.has_fuse = true
	var hunter_count: int = game.hunters.size()
	await _frames(195)
	_check(game.pacing.seen.has("ward") and game.pacing.state=="cue","Exploration actually triggers authored ward beat")
	await _frames(100)
	_check(game.pacing.fired and not game.pacing.ambient_allowed(),"Quiet lead-in performs variant and suppresses overlapping ambience")
	await _frames(190)
	_check(game.pacing.state=="recovery" and game.pacing.beat.is_empty(),"Scene ends in a real quiet recovery interval")
	_check(game.hunters.size()==hunter_count and game.health==3,"Narrative scene does not spawn hunters or inflict arbitrary damage")
	var event_count: int = game.pacing.log.size()
	await _frames(850)
	_check(game.pacing.log.size()==event_count,"Completed one-shot scene cannot spam on the same visit")
	var saved_pacing: Dictionary = game.pacing.snapshot()
	game.pacing.restore(JSON.parse_string(JSON.stringify(saved_pacing)))
	_check(game.pacing.seen.has("ward") and game.pacing.variants==expected,"Scene choice and played state survive JSON restore")
	game.hunters[0].sees_player = true
	game.hunters[0].set_active(true)
	game.pacing.update(.016)
	_check(game.pacing.state=="threat","Observed chase takes precedence over staged cues")
	game.hunters[0].sees_player = false
	game.hunters[0].set_active(false)
	game.pacing.update(.016)
	_check(game.pacing.state=="recovery" and game.pacing.recovery>9,"Ending chase starts a cooldown before new scares")
	# Actual locomotion and IK, not a duplicate of the selection formula.
	game.hero.reset_to(Vector3(0,.1,12))
	await _frames(10)
	var locks: int = game.hero.model.foot_plant.lock_events
	Input.action_press("move_forward")
	await _frames(75)
	Input.action_release("move_forward")
	_check(game.hero.model.foot_plant.lock_events>locks,"Live locomotion plants feet against real floor")
	_check(is_finite(game.hero.model.playback_speed) and game.hero.model.playback_speed>0,"Animation rate remains finite under actual movement")
	game.combat.acquire()
	game.hero.reset_to(Vector3(0,.1,11))
	Input.action_press("aim")
	Input.action_press("move_right")
	await _frames(25)
	Input.action_release("move_right")
	_check(absf(game.hero.model.legs_yaw)>.35,"Aimed strafe turns lower body while upper body aims")
	await _frames(15)
	var hand: Vector3 = game.hero.model.hand_world
	var expected_grip: Vector3 = game.combat.weapon.global_transform*Vector3(.03,-.075,.085)
	_check(hand.distance_to(expected_grip)<.06,"Right wrist follows actual pitched pistol grip")
	Input.action_release("aim")
	game.hero.reset_to(Vector3(0,.1,11))
	game.hero.model.rotation.y = PI
	Input.action_press("move_forward")
	await _frames(2)
	_check(absf(game.hero.turn_velocity)<=5.81,"Exploration turns respect bounded angular speed")
	Input.action_release("move_forward")
	game.feedback.reset_effects()
	await _frames(2)
	game.feedback.impact(Vector3(0,1.1,11),Vector3.BACK,true,Vector3.FORWARD)
	_check(game.feedback.blood_marks>0 and not game.feedback.impact_marks.is_empty(),"Flesh hit splashes onto actual floor geometry")
	for mark: MeshInstance3D in game.feedback.impact_marks:
		_check(mark.get_meta("blood",false) and mark.position.y<.02,"Persistent blood mark lies on sampled surface")
	game.feedback.impact(Vector3(3.18,1.4,1),Vector3.LEFT,false,Vector3.RIGHT,"metal")
	_check(game.feedback.last_surface=="metal","Metal impact keeps distinct feedback material")
	for i: int in range(60): game.feedback.impact(Vector3(3.18,1.4,1),Vector3.LEFT,false,Vector3.RIGHT,"wood")
	_check(game.feedback.impact_marks.size()==48,"Persistent hit geometry has bounded count")
	var audio: Node = preload("res://scripts/sound_bank.gd").new()
	audio.listener = game.hero
	root.add_child(audio)
	_check(audio.refuge_music!=null and audio.refuge_music.stream.loop_mode==AudioStreamWAV.LOOP_FORWARD,"Actual calm refuge music layer is loaded and looping")
	game.hero.in_refuge = true
	audio.update_world(2,game)
	_check(audio.refuge_mix==1 and audio.refuge_music.volume_db>-30 and audio.score_music.volume_db<-40,"Refuge crossfade replaces horror score with calm layer")
	audio.set_suspended(true)
	_check(audio.refuge_music.stream_paused,"Pause includes safe-room music")
	audio.set_levels(.5,0)
	_check(audio.refuge_music.volume_db<-70,"Music slider also controls refuge layer")
	var hashes: Dictionary = {}
	for id: String in ["flesh_hit","wood_hit","stone_hit","metal_hit"]: hashes[hash(audio.sounds[id].data)] = true
	_check(hashes.size()==4,"Four impact materials have distinct actual waveforms")
	audio.queue_free()
	await _frames(6)
	OS.delay_msec(100)
	game.queue_free()
	await process_frame
	print("POLISH AND REFUGE: %d checks, %d failures" % [checks,failures])
	quit(0 if failures==0 else 1)
func _freeze_hunters() -> void:
	for enemy: CharacterBody3D in game.hunters:
		enemy.set_active(false)
		enemy.set_physics_process(false)
func _key(code: Key) -> InputEventKey:
	var event := InputEventKey.new()
	event.physical_keycode = code
	event.pressed = true
	return event
func _frames(count: int) -> void:
	for i: int in range(count):
		await physics_frame
		await process_frame
func _check(ok: bool, message: String) -> void:
	checks += 1
	if not ok:
		failures += 1
		push_error("FAIL: "+message)
