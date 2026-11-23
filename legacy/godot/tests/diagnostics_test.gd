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
	await _frames(20)
	_freeze()
	var chapter: Node3D = game.diagnostics
	_check(game.hunters.size()==21,"Diagnostic, ceiling and isolation hunters are preplaced in the actual scene")
	_check(game.exploration.room_at(Vector3(23,0,-33))=="diag_xray" and game.exploration.ROOMS.size()==41,"Both new chapters belong to the exploration map")
	_check(game.facility.gate_by_id.diag_entry.locked,"Diagnostic wing requires employee access")
	chapter.use_station("xray")
	_check(chapter.screen.mode=="closed" and not chapter.xray_solved,"No medical access without badge")
	game.mission.power_on = true
	game.facility.apply_power()
	game.mission.has_badge = true
	game.facility.gate_by_id.diag_entry.restore_state(true,false)
	game.facility.layout_changed.emit()
	await _frames(6)
	_check(not game.navigation.find_path(Vector3(37,.1,-12),chapter.stations.xray.interaction_point()).is_empty(),"A* connects old corridor and real X-ray room")
	game.hero.reset_to(chapter.stations.xray.interaction_point())
	await _frames(5)
	_check(game.hero.is_on_floor() and game._prop_reachable(chapter.stations.xray),"X-ray stance has physical floor and unobstructed interaction")
	game.hero.camera_rig.rotation.y = -PI/2
	game.interact()
	_check(chapter.screen.mode=="xray" and game.is_paused,"Actual F interaction opens radiograph and pauses danger")
	var before: float = game.elapsed
	await _frames(10)
	_check(game.elapsed==before,"Radiograph inspection does not advance enemies or countdown")
	chapter.screen.buttons.verify.pressed.emit()
	_check(not chapter.xray_solved and chapter.screen.mode=="xray","Misaligned film cannot open observation room")
	for i: int in range(3): chapter.screen.buttons.rotate.pressed.emit()
	var click := InputEventMouseButton.new()
	click.button_index = MOUSE_BUTTON_LEFT
	click.pressed = true
	chapter.screen.board.gui_input.emit(click)
	var motion := InputEventMouseMotion.new()
	motion.relative = Vector2(-72,48)
	chapter.screen.board.gui_input.emit(motion)
	click.pressed = false
	chapter.screen.board.gui_input.emit(click)
	chapter.screen.buttons.verify.pressed.emit()
	await _frames(5)
	_check(chapter.xray_solved and not game.is_paused and chapter.screen.mode=="closed","UI rotation, dragging and verify solve the actual radiograph")
	_check(game.facility.gate_by_id.diag_cross.is_open and game.facility.gate_by_id.diag_37.is_open,"Solution changes both physical observation entrances")
	_check(game.hunters[14].active and not game.hunters[15].active,"One preplaced hunter wakes at the first reveal")
	chapter.use_station("treatment")
	_check(chapter.screen.mode=="closed" and not chapter.complete,"Treatment cannot bypass observation evidence")
	chapter.use_station("observe")
	await _frames(5)
	_check(chapter.observed and game.facility.gate_by_id.diag_loop.is_open,"Patient record opens the northern preparation route")
	_check(chapter.journal_entries().any(func(s: String) -> bool: return s.contains("охлаждение")),"Safe treatment order is retained in journal")
	var long_path: Array[Vector3] = game.navigation.find_path(Vector3(42,.1,-33),Vector3(46,.1,-33))
	_check(long_path.size()>15,"Intact glass forces a real longer route around the northern partition")
	game.hero.reset_to(Vector3(42.8,.1,-33))
	game.hero.camera_rig.rotation.y = -PI/2
	game.combat.cooldown = 0
	game.hero.stamina = 100
	await _frames(5)
	_check(game.combat.knife_attack() and chapter.partitions.observation_glass.broken,"Physical knife arc breaks glass without needing ammunition")
	await _frames(7)
	var short_path: Array[Vector3] = game.navigation.find_path(Vector3(42,.1,-33),Vector3(46,.1,-33))
	_check(short_path.size()>0 and short_path.size()<long_path.size()/2,"Breaking glass updates A* and opens a genuinely shorter passage")
	_check(game.hunters[15].active,"Noisy glass wakes the creature on the far side")
	chapter.use_station("memo")
	_check(game.facility.gate_by_id.diag_office.locked,"One clue does not unlock doctor's office")
	chapter.use_station("log")
	_check(game.facility.gate_by_id.diag_office.is_open,"Combining both clues physically opens doctor's office")
	chapter.use_station("office")
	_check(chapter.evidence.size()==3 and chapter.ending().contains("семья"),"Optional investigation changes ending text")
	_check(chapter.office_reward and game.inventory.medkits==1,"Doctor's office gives a finite usable medkit")
	var score_before: int = game.score
	chapter.use_station("office")
	_check(game.score==score_before and chapter.evidence.size()==3,"Evidence and score cannot be farmed")
	_check(game.inventory.medkits==1,"Office medkit cannot be duplicated by reading again")
	chapter.use_station("treatment")
	chapter.screen.buttons.release.pressed.emit()
	_check(not chapter.complete and chapter.screen.treatment_steps.is_empty(),"Unsafe medical command resets protocol")
	for id: String in ["cool","dose","release"]: chapter.screen.buttons[id].pressed.emit()
	_check(chapter.complete and game.facility.gate_by_id.diag_52.is_open,"Correct evidence-based sequence releases medical lock and return door")
	game.combat.reserve = 72
	chapter.use_station("supplies")
	_check(not chapter.reward_taken and game.combat.reserve==72,"Full ammunition leaves preparation reward in cupboard")
	game.combat.reserve = 8
	chapter.use_station("supplies")
	_check(chapter.reward_taken and game.combat.reserve==16,"Long route provides finite existing supply reward")
	chapter.use_station("supplies")
	_check(game.combat.reserve==16,"Preparation reward can only be claimed once")
	var snapshot: Dictionary = chapter.snapshot()
	_check(chapter.valid(snapshot),"Chapter checkpoint serializes valid evidence, glass, reward and puzzle state")
	var bad: Dictionary = snapshot.duplicate(true)
	bad.xray = false
	_check(not chapter.valid(bad),"Impossible observed-before-X-ray save is rejected")
	bad = snapshot.duplicate(true)
	bad.evidence = ["office"]
	_check(not chapter.valid(bad),"Unearned office evidence is rejected")
	var save: Dictionary = Saves.snapshot(game)
	game.inventory.spawn_supply("ammo",2,Vector3(27,.03,-32))
	save = Saves.snapshot(game)
	save.inventory.installed = true
	_check(Saves.valid(save),"Whole-game checkpoint accepts new optional chapter block")
	_check(save.inventory.drops.size()==1 and save.inventory.drops[0].at[2]<-16,"Dropped supplies in new wing are valid checkpoint positions")
	_check(Saves.apply(game,JSON.parse_string(JSON.stringify(save))),"Full JSON checkpoint loads with chapter state")
	await _frames(10)
	_freeze()
	_check(chapter.complete and chapter.evidence.size()==3 and chapter.partitions.observation_glass.broken and chapter.reward_taken,"Continue restores chapter progress without duplicating rewards")
	_check(get_nodes_in_group("dropped_supplies").size()==1,"Continue preserves actual dropped pickup in diagnostic wing")
	# Actual pistol continuation: one thin panel, then real enemy collision.
	var panel: StaticBody3D = chapter.partitions.service_sheet
	game.hero.reset_to(Vector3(54,.1,-40.8))
	game.hero.camera_rig.rotation.y = 0
	game.combat.acquire()
	game.combat.rounds = 9
	game.combat.cooldown = 0
	var hunter: CharacterBody3D = game.hunters[15]
	hunter.reset_hunter()
	hunter.position = Vector3(54,.1,-46)
	hunter.set_active(true)
	hunter.set_physics_process(false)
	Input.action_press("aim")
	await _frames(20)
	_aim(hunter.position+Vector3(0,1.25,0))
	var shot: Dictionary = game.combat.fire()
	_check(shot.get("fired",false) and shot.get("penetrated",false) and panel.broken,"Real shot penetrates explicitly thin maintenance screen")
	_check(shot.get("hit",false) and hunter.health<6,"Penetrating shot still resolves against enemy behind screen")
	Input.action_release("aim")
	# One-use electrical hazard hurts enemies and player, never grants repeated damage.
	chapter.use_station("trap")
	hunter.position = Vector3(48,.1,-34.5)
	var hp_before: int = hunter.health
	chapter.update(.02)
	_check(chapter.trap_spent and hunter.health<hp_before and hunter.stunned_remaining>=4,"Armed wet-floor trap damages and stuns actual enemy once")
	var after: int = hunter.health
	chapter.update(.02)
	_check(hunter.health==after,"Spent trap cannot repeatedly damage enemies")
	game.mission.relays.assign(["morgue","lab","theatre"])
	game.mission.has_fuel = true
	game.mission.has_coolant = true
	game.mission.purge_complete = true
	game.residential.complete = true
	game.hero.reset_to(Vector3(-6,.1,-12))
	game.mission.interact(game.mission.objects.terminal)
	_check(not game.mission.exit_started,"Terminal cannot skip the isolation chapter")
	preload("res://tests/mission_fixture.gd").finish_isolation(game)
	preload("res://tests/mission_fixture.gd").finish_annex(game)
	game.mission.interact(game.mission.objects.terminal)
	_check(game.mission.exit_started and chapter.finale_started and game.escape_remaining==180,"Full mission starts expanded 180-second finale")
	_check(game.facility.gate_by_id.quarantine_gate.locked and not game.facility.gate_by_id.quarantine_gate.is_open and game.facility.gate_by_id.final_passage.is_open,"Finale closes familiar return and physically opens emergency route")
	await _frames(8)
	var detour: Array[Vector3] = game.navigation.find_path(Vector3(0,.1,-13),chapter.stations.final_reset.interaction_point())
	_check(not detour.is_empty() and detour.any(func(p: Vector3) -> bool: return p.z<-16 and p.x>15),"A* reaches reset via real diagnostic detour around closed quarantine")
	_check(not game.save_checkpoint(true),"Final escape cannot be saved into a broken countdown checkpoint")
	game.hero.reset_to(game.facility.EXIT+Vector3(0,.1,0))
	await _frames(4)
	_check(not game.has_won,"Standing at exit cannot bypass manual mechanisms")
	game.hero.reset_to(chapter.stations.final_reset.interaction_point())
	Input.action_press("interact_hold")
	chapter.use_station("final_reset")
	chapter.update(2)
	Input.action_release("interact_hold")
	chapter.update(.1)
	_check(not chapter.breaker_reset and chapter.work_id.is_empty() and not game.hero.performing_action,"Letting go interrupts manual reset safely")
	Input.action_press("interact_hold")
	chapter.use_station("final_reset")
	chapter.update(.2)
	game._take_hit(game.hunters[0])
	_check(chapter.work_id.is_empty() and not game.hero.performing_action,"Melee interrupts manual mechanism before it completes")
	chapter.use_station("final_reset")
	chapter.update(.2)
	game.take_projectile_hit()
	_check(chapter.work_id.is_empty() and not game.hero.performing_action,"Acid hit also interrupts new manual mechanism")
	game.health = 3
	chapter.use_station("final_reset")
	chapter.update(5.1)
	_check(chapter.breaker_reset,"Holding actual work input completes manual reset")
	game.hero.reset_to(chapter.stations.exit_crank.interaction_point())
	chapter.use_station("exit_crank")
	chapter.update(5.1)
	_check(chapter.exit_released,"Exit crank requires reset and sustained work")
	Input.action_release("interact_hold")
	game.hero.reset_to(game.facility.EXIT+Vector3(0,.1,0))
	await _frames(4)
	_check(game.has_won and game.hud.overlay_body.text.contains("семья") and not game.hud.gameplay.visible,"Physical escape wins and displays earned ending without stale mission HUD")
	_check(chapter.ending_view.evidence_present and chapter.ending_view.viewport_3d.own_world_3d,"Earned evidence also changes actual 3D dawn tableau")
	game.restart_game()
	await _frames(6)
	_check(not chapter.complete and chapter.evidence.is_empty() and not chapter.finale_started and not chapter.partitions.observation_glass.broken and game.hud.gameplay.visible,"New game clears chapter, finale and damaged panels and restores gameplay HUD")
	_check(game.facility.gate_by_id.diag_office.locked and game.facility.gate_by_id.final_passage.locked,"New game restores progression gates")
	print("DIAGNOSTICS: %d checks, %d failures" % [checks,failures])
	game.queue_free()
	await process_frame
	quit(1 if failures else 0)

func _aim(at: Vector3) -> void:
	game.hero.camera_rig.camera.look_at(at)
	game.combat.aiming = true
	game.combat.update_weapon_pose()
func _freeze() -> void:
	for hunter: CharacterBody3D in game.hunters: hunter.set_active(false); hunter.set_physics_process(false)
func _frames(count: int) -> void:
	for i: int in range(count): await physics_frame; await process_frame
func _check(ok: bool,message: String) -> void:
	checks += 1
	if not ok: failures += 1; push_error("FAIL: "+message)
