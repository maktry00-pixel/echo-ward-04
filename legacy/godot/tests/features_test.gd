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
	await _frames(12)
	for enemy: CharacterBody3D in game.hunters:
		enemy.set_active(false)
		enemy.set_physics_process(false)
	game.hero.reset_to(Vector3(0,.1,11))
	game.combat.acquire()
	var steps_before: int = game.sound_bank.step_events
	Input.action_press("move_forward")
	await _frames(45)
	Input.action_release("move_forward")
	_check(game.sound_bank.step_events>steps_before and game.sound_bank.last_footstep_kind=="step_tile","Real walking triggers surface-specific footstep events")
	game.hero.reset_to(Vector3(0,.1,11))
	_check(game.inventory.entries().size()==3,"Weapon, ammo stack and bottles occupy three real slots")
	game.health = 3
	_check(game.inventory.take_supply("medkit") and game.health==3,"Healthy player can carry medkit without consuming it")
	_check(not game.inventory.use_item("medkit") and game.inventory.medkits==1,"Full health preserves carried medicine")
	game.health = 1
	_key(KEY_H)
	_check(game.health==2 and game.inventory.medkits==0,"H uses one carried medkit")
	game.flash_charges = 3
	game.inventory.take_supply("battery")
	_check(not game.inventory.use_item("battery") and game.inventory.batteries==1,"Full flash preserves battery")
	game.flash_charges = 1
	_key(KEY_T)
	_check(game.flash_charges==2 and game.inventory.batteries==0,"T uses one battery")
	for i: int in range(5): game.inventory.take_supply("medkit")
	_check(game.inventory.entries().size()==8,"Eight-slot inventory fills exactly")
	_check(not game.inventory.take_supply("battery") and game.inventory.batteries==0,"Full inventory rejects pickup atomically")
	_check(not game.inventory.can_take("badge",1,false),"Quest pickup also respects space")
	_check(not game.inventory.drop_item("weapon"),"Essential weapon cannot be discarded")
	_check(game.inventory.drop_item("medkit"),"Consumable can be left in world")
	var dropped: Node3D = get_nodes_in_group("dropped_supplies")[0]
	_check(game.inventory.medkits==4 and game.inventory.entries().size()==7,"Leaving one item frees a slot")
	await _frames(3)
	game.interact()
	await _frames(2)
	_check(game.inventory.medkits==5 and get_nodes_in_group("dropped_supplies").is_empty(),"Actual F-target interaction reclaims dropped supply")
	game.inventory.medkits = 0
	game.combat.reserve = 29
	_check(game.inventory.drop_item("ammo",5) and game.combat.reserve==24,"Partial ammo stack drops its selected count")
	dropped = get_nodes_in_group("dropped_supplies")[0]
	_check(dropped.amount==5 and dropped.collect() and game.combat.reserve==29,"Dropped partial ammunition returns exactly five rounds")
	await _frames(2)
	game.mission.has_badge = true
	_key(KEY_X)
	_check(game.is_paused and game.inventory.screen.mode=="inventory","X opens paused 3D item inspection")
	_check(game.inventory.screen.inspector.item_id=="badge","X selects actual badge model")
	var clock: float = game.elapsed
	await _frames(24)
	_check(game.elapsed==clock and not game.inventory.badge_examined,"Front of badge neither advances game nor reveals back clue")
	game.inventory.screen._input(_event(KEY_SPACE))
	await _frames(35)
	_check(game.inventory.badge_examined,"Turning real model to reverse discovers clue")
	game.inventory.screen._input(_event(KEY_ESCAPE))
	_check(not game.is_paused and game.inventory.screen.mode=="closed","Esc returns from inventory and resumes play")
	game.hero.reset_to(Vector3(0,.1,-7))
	await _frames(4)
	game.interact()
	_check(game.inventory.screen.mode=="keypad","F at locked quarantine opens keypad")
	for digit: String in ["1","1","1","ВВОД"]: game.inventory.screen.keypad_buttons[digit].pressed.emit()
	_check(game.facility.gate_by_id["quarantine_gate"].locked and game.is_paused and game.inventory.screen.keypad_label.text.begins_with("КОД НЕ ПРИНЯТ"),"Wrong code keeps door locked and shows rejection")
	for digit: String in ["4","1","7","ВВОД"]: game.inventory.screen.keypad_buttons[digit].pressed.emit()
	await _frames(35)
	_check(game.inventory.quarantine_code and game.facility.gate_by_id["quarantine_gate"].is_open and not game.is_paused,"Button-driven correct code opens physical gate and resumes game")
	game.exploration.visited.clear()
	game.exploration.update(Vector3(0,.1,11))
	_check(game.exploration.current=="reception" and game.exploration.visited.size()==1,"Map records entered room only")
	_check(game.exploration.remaining(game,"ward05")>0,"Uncollected hidden loot keeps room uncleared")
	game.exploration.update(Vector3(55,-4.9,70))
	_check(game.exploration.current=="reactor" and not game.exploration.visited.has("ward05"),"Underground room uses its correct floor and unexplored ward stays hidden")
	_check(not preload("res://scripts/exploration.gd").valid(["invented"]),"Unknown saved room is rejected")
	for sample: Array in [[Vector3(0,.1,11),"tile"],[Vector3(55,-4.9,70),"metal"],[Vector3(90,.1,20),"vinyl"]]:
		game.hero.reset_to(sample[0])
		_check(game.sound_bank.footstep(game)=="step_"+sample[1],"Physical floor selects "+sample[1]+" footsteps")
	game.sound_bank.register_glass(game.hero.position)
	_check(game.sound_bank.footstep(game)=="step_glass" and game.sound_bank.glass_visuals.size()==1,"Broken bottle leaves visible crunchy glass")
	var hashes: Dictionary = {}
	for kind: String in ["step_tile","step_metal","step_vinyl","step_glass","injured_breath","door_break"]:
		var stream: AudioStreamWAV = game.sound_bank._foley(kind,.4)
		_check(stream.data.size()==17640 and stream.data.count(0)<17000,"Nonempty generated waveform: "+kind)
		hashes[hash(stream.data)] = true
	_check(hashes.size()==6,"Surface and danger waveforms are distinct")
	var audio: Node = preload("res://scripts/sound_bank.gd").new()
	audio.listener = game.hero
	root.add_child(audio)
	_check(audio.sounds.has("injured_breath") and audio.sounds.has("heavy_step"),"Enabled bank actually loads new danger sounds")
	audio.set_tension(.4)
	var volume: float = audio.score_music.volume_db
	audio.quiet_remaining = 2
	audio.set_tension(.4)
	_check(is_equal_approx(audio.score_music.volume_db,volume-18),"Quiet interval ducks actual score by 18 dB")
	game.hero.reset_to(Vector3(55,-4.9,70))
	audio.update_world(.1,game)
	var bus: int = AudioServer.get_bus_index(audio.effects_bus)
	var reverb: AudioEffectReverb = AudioServer.get_bus_effect(bus,0)
	_check(is_equal_approx(reverb.room_size,.85),"Lower-sector footsteps use larger actual reverb room")
	game.hero.reset_to(Vector3(90,.1,20))
	audio.update_world(.1,game)
	_check(is_equal_approx(reverb.room_size,.42),"Residential room changes actual reverb")
	audio.play_spatial("claw",game.hero.position+Vector3(0,0,3))
	var spatial: AudioStreamPlayer3D = audio.get_children()[-1]
	_check(spatial.stream==audio.sounds.claw and spatial.bus==audio.effects_bus and spatial.max_distance==20,"Enemy motion plays spatial stream through effects bus")
	audio.set_suspended(true)
	await _frames(3)
	_check(spatial.stream_paused,"Pause also suspends spatial enemy movement audio")
	audio.play_cue("win")
	var victory: AudioStreamPlayer = audio.get_children()[-1]
	await _frames(3)
	_check(not victory.stream_paused,"Victory cue remains audible over paused end screen")
	audio.toggle_mute(true)
	await _frames(2)
	_check(victory.stream_paused,"Mute still applies to victory cue")
	audio.queue_free()
	await _frames(2)
	game.sound_bank.quiet_remaining = 2
	game.sound_bank.update_world(.5,game)
	_check(is_equal_approx(game.sound_bank.quiet_remaining,1.5),"Quiet interval advances in world time")
	game.restart_game()
	await _frames(4)
	for enemy: CharacterBody3D in game.hunters:
		enemy.set_active(false)
		enemy.set_physics_process(false)
	_check(game.inventory.medkits==0 and not game.inventory.badge_examined and game.sound_bank.glass_zones.is_empty(),"Restart clears carried items, clue and glass")
	var hunter: CharacterBody3D = game.hunters[0]
	hunter.position = Vector3(0,.1,10)
	hunter.set_active(true)
	hunter._update_limb_hitboxes()
	await _frames(3)
	var limb: Area3D = hunter.limb_hitboxes[0]
	var at: Vector3 = limb.global_position
	var ray := PhysicsRayQueryParameters3D.create(at+Vector3(-2,0,0),at,4)
	ray.collide_with_areas = true
	var collision: Dictionary = game.get_world_3d().direct_space_state.intersect_ray(ray)
	_check(not collision.is_empty() and collision.collider.has_method("receive_bullet"),"Actual weapon-layer ray intersects creature hit shape")
	limb.receive_bullet(game.hero.position,1,false,at)
	limb.receive_bullet(game.hero.position,1,false,at)
	_check(hunter.leg_damage[0]==2 and hunter.mobility_factor()<.6 and hunter.model.injured_legs[0],"Two leg hits cripple actual speed and gait")
	var before: int = hunter.health
	limb.receive_bullet(game.hero.position,1,false,at)
	_check(hunter.leg_damage[0]==2 and hunter.health==before-1,"Joint damage caps while subsequent hits still damage creature")
	game.mission.has_badge = true
	game.mission.power_on = true
	game.hero.reset_to(Vector3(52,.1,4))
	await _frames(2)
	_check(game.guardian.started and game.guardian.door.locked and not game.hunters[5].active,"Approaching physical guardian door starts telegraphed encounter")
	var boss: CharacterBody3D = game.hunters[5]
	var spawn: Vector3 = boss.position
	await _frames(170)
	_check(game.guardian.door.breached and game.guardian.door.body.collision_layer==0 and boss.active,"Guardian breaks real door and activates")
	_check(boss.position.distance_to(spawn)<.1,"Intro does not teleport boss to player")
	_check(not boss.receive_bullet(game.hero.position,3,true) and boss.health==12,"Closed armor blocks ordinary damage")
	boss.set_physics_process(true)
	var saw_windup: bool = false
	var saw_lunge: bool = false
	var saw_window: bool = false
	for frame: int in range(720):
		saw_windup = saw_windup or boss.windup>0
		saw_lunge = saw_lunge or boss.lunge_remaining>0
		if game.guardian.exposed_remaining>0:
			saw_window = true
			break
		await _frames(1)
	_check(saw_windup and saw_lunge and saw_window,"Live AI chases, telegraphs charge and exposes chest after recovery")
	boss.set_physics_process(false)
	_check(game.guardian.weakpoint.collision_layer==4,"Exposed weakpoint has actual weapon-only collider")
	game.combat.acquire()
	Input.action_press("aim")
	await _frames(10)
	for i: int in range(8):
		_aim(boss.aim_point())
		await _frames(3)
	var shot: Dictionary = game.combat.fire()
	Input.action_release("aim")
	_check(shot.get("hit",false) and boss.health==9,"Actual camera/muzzle shot hits exposed chest for three damage")
	boss.receive_weakspot(game.hero.position)
	_check(boss.health==6 and game.guardian.phase==2 and game.guardian.exposed_remaining==0,"Half health triggers second phase and closes first window")
	_check(not boss.receive_weakspot(game.hero.position),"Closed second-phase chest cannot be farmed")
	# Fixture advances to another real recovery, avoiding duplicate timing test.
	game.guardian.was_lunging = true
	boss.lunge_remaining = 0
	game.guardian.update(.016)
	_check(is_equal_approx(game.guardian.exposed_remaining,1.8),"Second-phase recovery is shorter")
	boss.receive_weakspot(game.hero.position)
	boss.receive_weakspot(game.hero.position)
	await _frames(2)
	_check(boss.dead and game.guardian.finished and not game.guardian.status.visible,"Defeated guardian finishes encounter and clears boss HUD")
	game.inventory.take_supply("medkit")
	game.inventory.drop_item("ammo",5)
	game.exploration.update(Vector3(52,.1,4))
	var saved: Dictionary = Saves.snapshot(game)
	_check(Saves.valid(saved),"All six features produce a valid checkpoint")
	var invalid: Dictionary = saved.duplicate(true)
	invalid.inventory.medkits = 8
	invalid.inventory.batteries = 8
	_check(not Saves.valid(invalid),"Saved combined inventory cannot exceed eight slots")
	invalid = saved.duplicate(true)
	invalid.enemies[0].legs[0] = 3
	_check(not Saves.valid(invalid),"Invalid joint state is rejected")
	var actual_layout: Dictionary = saved.layout_positions.duplicate(true)
	_check(Saves.apply(game,JSON.parse_string(JSON.stringify(saved))),"New checkpoint survives disk-format round trip")
	await _frames(3)
	_check(game.inventory.medkits==1 and get_nodes_in_group("dropped_supplies").size()==1,"Continue restores carried and left supplies")
	_check(game.hunters[0].leg_damage[0]==2 and game.hunters[5].dead and game.guardian.door.breached,"Continue preserves injuries and defeated boss passage")
	_check(game.exploration.visited.has("theatre_front") and game.resource_layout.snapshot(game)==actual_layout,"Continue preserves explored rooms and exact loot placement")
	var old: Dictionary = saved.duplicate(true)
	old.erase("isolation")
	for key: String in ["inventory","guardian","exploration","layout_positions","refuge","pacing","reactor_state","power_on","fuse","elapsed","manual_refuge","diagnostics","upgrades","difficulty"]: old.erase(key)
	old.enemies.resize(14)
	for id: String in ["diag_entry","diag_23","diag_37","diag_52","diag_cross","diag_loop","diag_office","final_passage","final_research_return"]: old.doors.erase(id)
	for key: String in ["shots","hits","kills","shotgun","loaded","shells","selected"]: old.combat.erase(key)
	for enemy: Dictionary in old.enemies:
		enemy.erase("legs")
		enemy.erase("death_style")
	_check(Saves.apply(game,old),"Older checkpoints without new fields still load")
	game.queue_free()
	await process_frame
	print("SIX FEATURES: %d checks, %d failures" % [checks,failures])
	quit(0 if failures==0 else 1)
func _aim(point: Vector3) -> void:
	var camera: Camera3D = game.hero.camera_rig.camera
	var center: Vector2 = camera.get_viewport().get_visible_rect().size/2
	var origin: Vector3 = camera.project_ray_origin(center)
	var direction: Vector3 = camera.project_ray_normal(center)
	var wanted: Vector3 = (point-origin).normalized()
	game.hero.camera_rig.apply_look(Vector2(-wrapf(atan2(-wanted.x,-wanted.z)-atan2(-direction.x,-direction.z),-PI,PI),-(asin(wanted.y)-asin(direction.y)))/game.hero.camera_rig.look_sensitivity)
func _event(code: Key) -> InputEventKey:
	var event := InputEventKey.new()
	event.physical_keycode = code
	event.pressed = true
	return event
func _key(code: Key) -> void: game._unhandled_input(_event(code))
func _frames(count: int) -> void:
	for i: int in range(count):
		await physics_frame
		await process_frame
func _check(ok: bool, message: String) -> void:
	checks += 1
	if not ok:
		failures += 1
		push_error("FAIL: "+message)

