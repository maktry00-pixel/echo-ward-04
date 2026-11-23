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
	await _frames(18)
	_freeze()
	var upgrade: Node3D = game.upgrades
	var combat: Node3D = game.combat
	# Every authored pickup needs a clear standing volume and an unobstructed reach.
	for id: String in upgrade.stations:
		var station: Node3D = upgrade.stations[id]
		game.hero.reset_to(station.interaction_point())
		await _frames(4)
		var shape := SphereShape3D.new()
		shape.radius = .34
		var query := PhysicsShapeQueryParameters3D.new()
		query.shape = shape
		query.transform.origin = game.hero.position+Vector3(0,1,0)
		query.collision_mask = 1
		var clear: bool = game.get_world_3d().direct_space_state.intersect_shape(query).is_empty()
		_check(clear and game._prop_reachable(station) and game.hero.is_on_floor(),"Pickup standing space, floor and actual reach are clear: "+id)
	_check(game.difficulty_id=="normal" and game.front_end.difficulty_picker.item_count==4,"Four campaign difficulties are present; normal remains default")
	game.front_end.difficulty_picker.item_selected.emit(0)
	_check(game.settings.difficulty=="story" and game.difficulty_id=="normal","Menu choice changes next campaign, not active balance")
	game._new_game()
	await _frames(6)
	_freeze()
	_check(game.difficulty_id=="story" and float(game.difficulty_value("speed"))<1,"Starting campaign applies selected profile")
	_check(game.normalise_settings({"difficulty":17}).difficulty=="normal","Invalid difficulty type is sanitized without engine errors")
	game.mission.interact(game.mission.objects.weapon)
	game.mission.interact(game.mission.objects.ammo_reception)
	_check(combat.reserve==24,"Story profile grants twelve rounds instead of eight from ammo cache")
	game.health = 1
	game.inventory.take_supply("medkit")
	game.inventory.use_item("medkit")
	_check(game.health==3,"Story profile heals two points per actual medkit")
	game.settings.difficulty = "normal"
	game._new_game()
	await _frames(5)
	_freeze()
	game.mission.interact(game.mission.objects.weapon)
	game.inventory.medkits = 5
	upgrade.use_station("shotgun")
	_check(not combat.shotgun_owned and "shotgun" not in upgrade.used,"Full eight-slot inventory leaves shotgun in world")
	game.inventory.medkits = 0
	game.hero.reset_to(upgrade.stations.shotgun.interaction_point())
	game.hero.camera_rig.rotation.y = 0
	await _frames(5)
	game.interact()
	_check(combat.shotgun_owned and combat.shotgun_loaded==2 and upgrade.stations.shotgun.used,"Actual F pickup grants finite shotgun with two loaded shells")
	upgrade.use_station("shotgun")
	_check(combat.shotgun_loaded==2,"Weapon pickup cannot refill loaded ammunition repeatedly")
	upgrade.use_station("shells_morgue")
	_check(combat.shells==3 and game.inventory.entries().any(func(item: Dictionary)->bool: return item.id=="shells"),"Shotgun ammunition occupies its own inventory stack")
	_check(combat.select_weapon("shotgun") and combat.selected_weapon=="shotgun","Shotgun can be equipped separately from pistol")
	var enemy: CharacterBody3D = game.hunters[1]
	game.hero.reset_to(Vector3(0,.1,12))
	enemy.reset_hunter()
	enemy.position = Vector3(0,.1,8)
	enemy.set_active(true)
	enemy.set_physics_process(false)
	Input.action_press("aim")
	await _frames(20)
	_aim(enemy.position+Vector3(0,1.3,0))
	combat.cooldown = 0
	var shot: Dictionary = combat.fire()
	_check(shot.get("fired",false) and shot.get("pellets")==8 and shot.get("killed",false),"Eight real pellets kill an exposed ordinary enemy at close range")
	_check(combat.shotgun_loaded==1 and combat.rounds==9 and combat.shots_fired==1,"One blast consumes one shell and one shot counter, leaving pistol rounds intact")
	_check(not combat.fire().get("fired",false),"Shotgun pump recovery enforces slower rate of fire")
	_check(enemy.model.death_style=="blast","Close blast selects a different procedural collapse")
	var heavy: CharacterBody3D = game.hunters[13]
	heavy.reset_hunter()
	heavy.position = Vector3(0,.1,8)
	heavy.set_active(true)
	heavy.set_physics_process(false)
	await _frames(2)
	combat.cooldown = 0
	_aim(heavy.position+Vector3(0,1.3,0))
	combat.fire()
	var kick_at: Vector3 = heavy.position
	heavy.set_physics_process(true)
	await _frames(8)
	_check(not heavy.dead and heavy.position.distance_to(kick_at)>.15,"Surviving creature is physically pushed back by close shotgun blast")
	heavy.set_active(false)
	heavy.set_physics_process(false)
	combat.cooldown = 0
	combat.shotgun_loaded = 0
	combat.shells = 3
	_check(combat.reload_weapon(),"Empty shotgun starts shell-by-shell reload")
	await _frames(54)
	_check(combat.shotgun_loaded==1 and combat.shells==2 and combat.shotgun_reloading,"A completed insertion transfers one shell, not an entire magazine")
	_check(get_nodes_in_group("spent_casings").any(func(casing:Node)->bool:return casing.shotgun_shell),"Pump cycle ejects a distinct physical shotgun casing")
	combat.cooldown = 0
	shot = combat.fire()
	_check(shot.get("interrupted_reload",false) and not combat.shotgun_reloading and combat.shotgun_loaded==1,"Trigger interrupts reload while preserving only inserted shells")
	await _frames(10)
	combat.reload_weapon()
	await _frames(110)
	_check(combat.shotgun_loaded==3 and combat.shells==0,"Reload completes only available loose shells")
	_check(combat.cycle_weapon() and combat.selected_weapon=="pistol","Z cycle returns to original pistol")
	combat.select_weapon("shotgun")
	combat.cooldown = 0
	enemy.reset_hunter()
	enemy.position = Vector3(0,.1,6)
	enemy.set_active(true)
	enemy.set_physics_process(false)
	var wall: Node3D = preload("res://scripts/world_builder.gd").box(game.get_node("World"),Vector3(2.5,2.5,.35),Vector3(0,1.25,9),preload("res://scripts/world_builder.gd").material("777777"),true)
	await _frames(5)
	_aim(enemy.position+Vector3(0,1.3,0))
	shot = combat.fire()
	_check(shot.get("fired",false) and not shot.get("hit",false) and enemy.health==6,"Pellets stop at real wall instead of passing through camera cover")
	wall.queue_free()
	Input.action_release("aim")
	_freeze()
	game.hero.reset_to(Vector3(14.0,.1,2))
	game.hero.camera_rig.rotation.y = -PI/2
	Input.action_press("aim")
	await _frames(14)
	_check(combat.wall_blend>.8,"Long weapon visibly raises when approaching wall")
	Input.action_release("aim")
	game.hero.reset_to(Vector3(0,.1,12))
	await _frames(12)
	_check(combat.wall_blend<.1,"Weapon returns smoothly when clear of wall")
	combat.shells = 10
	_check(game.inventory.entries().filter(func(item:Dictionary)->bool: return item.id=="shells").size()==2,"Loose shells use stacks of eight")
	_check(game.inventory.drop_item("shells",2) and combat.shells==8,"Only selected loose shell quantity is dropped")
	var drop: Area3D = get_nodes_in_group("dropped_supplies")[0]
	_check(drop.collect() and combat.shells==10,"Dropped shell stack can be reclaimed atomically")
	await _frames(2)
	# Barricades use the existing closed door and can always be removed.
	combat.shells = 0
	game.inventory.boards = 1
	var door: Node3D = game.facility.gate_by_id.research_23
	door.restore_state(false,false)
	game.hero.reset_to(Vector3(23,.1,-7.2))
	await _frames(5)
	Input.action_press("barricade")
	_check(upgrade.start_barricade() and game.hero.performing_action,"K starts manual work at closed ordinary door")
	await _frames(30)
	Input.action_release("barricade")
	await _frames(2)
	_check(door.barricade_hp==0 and game.inventory.boards==1 and not game.hero.performing_action,"Interrupted construction consumes no board")
	Input.action_press("barricade")
	upgrade.start_barricade()
	await _frames(140)
	Input.action_release("barricade")
	_check(door.barricade_hp==3 and game.inventory.boards==0 and not door.toggle_door(game.hero.position),"Completed barricade costs one board and prevents ordinary opening")
	_check(door.body.collision_layer==1 and door.barricade_mesh!=null,"Barricade has visible boards and preserves physical door collision")
	Input.action_press("barricade")
	upgrade.start_barricade()
	await _frames(95)
	Input.action_release("barricade")
	_check(door.barricade_hp==0 and game.inventory.boards==1,"Player dismantles own barricade and recovers one board")
	Input.action_press("barricade")
	upgrade.start_barricade()
	await _frames(140)
	Input.action_release("barricade")
	enemy.reset_hunter()
	enemy.position = Vector3(23,.1,-10.6)
	enemy.set_active(true)
	enemy.set_physics_process(true)
	enemy.last_known = game.hero.position
	enemy._set_state(enemy.State.INVESTIGATE)
	await _frames(145)
	_check(not door.is_open and enemy.position.z<-9,"Reinforced door physically delays live pursuing enemy")
	var budget: int = 1500
	while not door.is_open and budget>0: await _frames(1); budget-=1
	_check(door.is_open and door.barricade_hp==0,"Live hunter breaks all three fasteners before opening door")
	enemy.set_active(false)
	enemy.set_physics_process(false)
	game.hero.reset_to(game.refuge.SPAWN)
	await _frames(4)
	game.inventory.boards = 1
	combat.shells = 5
	_check(game.refuge.transfer("boards",true) and game.refuge.transfer("shells",true),"Refuge stores actual boards and shells")
	_check(game.inventory.boards==0 and combat.shells==0 and game.refuge.stash.shells==5,"New stash transfers do not generate resources")
	_check(game.refuge.transfer("boards",false) and game.refuge.transfer("shells",false),"New supplies can be taken back with normal capacity limits")
	# Save difficulty, weapon, stock, clues, stash and barricade in one real JSON file shape.
	door.restore_state(false,false)
	door.set_barricade(2,1)
	upgrade.use_station("record_shift")
	game.difficulty_id = "nightmare"
	var data: Dictionary = Saves.snapshot(game)
	data.manual_refuge = true
	_check(Saves.valid(data),"Expanded campaign save accepts actual equipment and barricade state")
	_check(Saves.apply(game,JSON.parse_string(JSON.stringify(data))),"New equipment survives full JSON checkpoint load")
	await _frames(7)
	_freeze()
	_check(game.difficulty_id=="nightmare" and combat.shotgun_owned and combat.selected_weapon=="shotgun" and combat.shells==5,"Continue preserves active difficulty, selected firearm and shells")
	_check(door.barricade_hp==2 and "shift" in upgrade.records and game.inventory.boards==1,"Continue preserves barricades, recordings and carried boards")
	var bad: Dictionary = data.duplicate(true)
	bad.combat.loaded = 99
	_check(not Saves.valid(bad),"Invalid loaded shell count is rejected")
	bad = data.duplicate(true)
	bad.difficulty = "impossible"
	_check(not Saves.valid(bad),"Unknown campaign mode is rejected")
	bad = data.duplicate(true)
	bad.upgrades.barriers = {"quarantine_gate":{"hp":3,"side":1}}
	_check(not Saves.valid(bad),"Save cannot barricade mission-critical gates")
	game.difficulty_id = "normal"
	# The ambush has an observable warning, a continuous fall and ground navigation.
	var ceiling: CharacterBody3D = upgrade.ceiling
	ceiling.reset_hunter()
	ceiling.set_physics_process(true)
	game.mission.has_badge = true
	game.hero.reset_to(Vector3(35,.1,-18))
	await _frames(8)
	_check(ceiling.perch_state=="warning" and ceiling.position.y>2.3,"Preplaced ceiling creature gives warning before falling")
	_check(game.hud.story_label.text.contains("пыль"),"Ceiling ambush communicates dust and sound cue to player")
	var fall_budget: int = 220
	var drift: float = 0
	while ceiling.perch_state!="grounded" and fall_budget>0:
		drift = maxf(drift,absf(ceiling.position.x-29))
		await _frames(1)
		fall_budget-=1
	_check(ceiling.perch_state=="grounded" and ceiling.position.y<.2 and drift<.1,"Creature physically falls from authored perch without horizontal teleport")
	var landed: Vector3 = ceiling.position
	await _frames(25)
	_check(ceiling.position.distance_to(landed)>.1,"Landed creature continues pursuit with actual ground A*")
	ceiling.reset_hunter()
	ceiling.set_active(true)
	ceiling.receive_bullet(game.hero.position,6,true)
	await _frames(110)
	_check(ceiling.dead and ceiling.position.y<.2,"Creature shot off ceiling leaves a grounded corpse")
	_check(game.hunters[0].model.has_meta("anatomy_detail") and game.hunters[0].model.limbs.size()==6,"Ordinary creatures have additional original anatomy and articulated legs")
	# Real melee timing rather than unavoidable touch damage.
	_freeze()
	game.hero.reset_to(Vector3(0,.1,10))
	game.health = 3
	enemy.reset_hunter()
	enemy.position = Vector3(0,.1,9)
	enemy.model.rotation.y = PI
	enemy.attack_cooldown = 0
	enemy.set_active(true)
	enemy.set_physics_process(true)
	await _frames(6)
	_check(enemy.windup>0 and game.health==3,"Close enemy visibly prepares melee without immediate touch damage")
	await _frames(36)
	_check(game.health==2 and game.hero.invulnerable_remaining>0,"Prepared swing creates a bounded damaging window")
	_freeze()
	for id: String in upgrade.RECORDS:
		var stream: AudioStreamWAV = AudioStreamWAV.load_from_file("res://assets/audio/stories/"+id+".wav")
		_check(stream!=null and stream.get_length()>8 and stream.data.size()>100000,"Actual original spoken recording exists: "+id)
	upgrade.use_station("record_ceiling")
	_check(upgrade.records.has("ceiling") and upgrade.journal_entries().any(func(text: String)->bool:return text.contains("пыль")),"Found recording can be read again in journal")
	upgrade.replay_record()
	_check(not game.hud.story_label.text.is_empty(),"L replays a discovered recording with subtitles")
	game.mission.purge_complete = true
	upgrade.update(.01)
	_check(upgrade.change_seen and upgrade.changed.visible,"Reactor completion changes familiar reception with authored story evidence")
	combat.selected_weapon = "shotgun"
	combat.drop_weapon()
	var fallen: Node3D = get_nodes_in_group("dropped_weapons").back()
	_check(fallen.get_child(1).get_script()==preload("res://scripts/shotgun_visual.gd"),"Defeat drops the actually equipped shotgun mesh")
	game.restart_game()
	await _frames(5)
	_check(not combat.shotgun_owned and combat.shells==0 and game.inventory.boards==0 and upgrade.records.is_empty(),"New game clears weapon, new supplies and recordings")
	_check(door.barricade_hp==0 and ceiling.perch_state=="perched" and not upgrade.changed.visible,"New game resets barricades, ceiling ambush and changed environment")
	print("SURVIVAL UPGRADE: %d checks, %d failures" % [checks,failures])
	game.queue_free()
	await process_frame
	quit(1 if failures else 0)
func _freeze() -> void:
	for hunter: CharacterBody3D in game.hunters: hunter.set_active(false); hunter.set_physics_process(false)
func _aim(at: Vector3) -> void:
	game.hero.camera_rig.camera.look_at(at)
	game.combat.aiming = true
	game.combat.update_weapon_pose()
func _frames(count: int) -> void:
	for i: int in range(count): await physics_frame; await process_frame
func _check(ok: bool,message: String) -> void:
	checks+=1
	if not ok: failures+=1; push_error("FAIL: "+message)
