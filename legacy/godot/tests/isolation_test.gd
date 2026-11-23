extends SceneTree
const Saves = preload("res://scripts/checkpoint_store.gd")
var game: Node3D
var chapter: Node3D
var checks: int = 0
var failures: int = 0
func _initialize() -> void: call_deferred("_run")
func _run() -> void:
	game = load("res://scenes/third_person.tscn").instantiate()
	game.set_meta("automated_test",true)
	root.add_child(game)
	await _frames(18)
	chapter = game.isolation
	_freeze()
	_check(game.hunters.size()==25 and chapter.actors.size()==3,"Three isolation actors and four annex actors are preplaced; old indices remain intact")
	_check(chapter.gates.entry.locked and not chapter.complete,"Isolation begins physically locked and unfinished")
	chapter.use_station("case")
	_check(not chapter.case_read,"Chapter cannot be started before diagnostic completion")
	for id: String in chapter.stations:
		var station: Node3D = chapter.stations[id]
		game.hero.reset_to(station.interaction_point()+Vector3(0,.1,0))
		await _frames(5)
		var shape := SphereShape3D.new()
		shape.radius = .34
		var query := PhysicsShapeQueryParameters3D.new()
		query.shape = shape
		query.transform.origin = game.hero.position+Vector3(0,1,0)
		query.collision_mask = 1
		_check(game.get_world_3d().direct_space_state.intersect_shape(query).is_empty() and game._prop_reachable(station) and game.hero.is_on_floor(),"Authored station has a clear standing volume, floor and reach: "+id)
	_fixture()
	await _frames(8)
	_check(not chapter.gates.entry.locked,"Completing diagnostics releases the actual entry lock")
	await _use("case")
	_check(chapter.case_read and chapter.journal_entries().size()==1,"Real F at the guard post reads the case and records it in journal")
	await _use("light")
	_check(chapter.circuit=="light" and not chapter.gates.west.locked and chapter.gates.east.locked,"Lighting choice opens only the west entry")
	chapter.use_station("dark")
	_check(chapter.circuit=="light" and chapter.gates.east.locked,"Campaign choice cannot silently energize both branches")
	await _frames(8)
	var path: Array[Vector3] = game.navigation.find_path(Vector3(52,.1,-59),chapter.stations.coil_light.interaction_point())
	_check(not path.is_empty() and path.any(func(at:Vector3)->bool:return at.x<26 and at.z<-62),"West route requires real travel through the selected branch")
	game.hero.reset_to(chapter.stations.coil_light.interaction_point())
	await _frames(5)
	game.interact()
	_check(chapter.stations.coil_light.opened and not chapter.coil,"First F opens actual cabinet before collecting its contents")
	_check(chapter.stations.coil_light.contents.visible,"Opening the drawer reveals the actual contact block mesh")
	await _frames(24)
	game.inventory.medkits = 7
	game.interact()
	_check(not chapter.coil and chapter.stations.coil_light.contents.visible,"Full pack leaves the actual contact block inside the open drawer")
	game.inventory.medkits = 0
	game.interact()
	_check(chapter.coil,"Second F collects the contact block from the opened cabinet")
	_check(not chapter.stations.coil_light.contents.visible and game.inventory.entries().any(func(item:Dictionary)->bool:return item.id=="isolation_coil"),"Collecting hides the block and occupies a real inventory slot")
	var actor: CharacterBody3D = chapter.pursuer
	actor.set_active(true)
	actor._damage_leg(0)
	actor._damage_leg(0)
	actor.receive_bullet(game.hero.position,2,false)
	var wounded_hp: int = actor.health
	var wounded_legs: Array = actor.leg_damage.duplicate()
	await _use("tag")
	_check(chapter.tag and chapter.released and not chapter.gates.cell.locked,"Bracelet release opens the preplaced cell")
	_check(not chapter.stations.tag.contents.visible and game.inventory.entries().any(func(item:Dictionary)->bool:return item.id=="isolation_tag"),"Bracelet is removed from its drawer and carried as a protected key item")
	_check(actor==chapter.pursuer and actor.health==wounded_hp and actor.leg_damage==wounded_legs,"Release preserves the same actor, health and wounded leg")
	await _use("diary")
	_check(chapter.evidence==["diary"] and chapter.gates.archive.locked,"One clue does not unlock the evidence chain")
	await _use("report")
	_check(chapter.evidence.size()==2 and not chapter.gates.archive.locked,"Two discovered clues open the archive")
	await _use("proof")
	_check(chapter.proof and not chapter.gates.shortcut.locked and chapter.gates.shortcut.is_open,"Copying evidence opens the central return route")
	var score: int = game.score
	chapter.use_station("proof")
	_check(game.score==score,"Evidence reward cannot be farmed repeatedly")
	game.hero.reset_to(chapter.stations.generator.interaction_point())
	Input.action_press("interact_hold")
	chapter.use_station("generator")
	chapter.update(2)
	Input.action_release("interact_hold")
	chapter.update(.01)
	_check(not chapter.generator_started and chapter.work_id.is_empty() and not game.hero.performing_action,"Releasing F cancels partial generator work")
	Input.action_press("interact_hold")
	chapter.use_station("generator")
	chapter.update(2)
	game._take_hit(actor)
	_check(chapter.work_id.is_empty() and not chapter.generator_started,"Actual damage cancels work without completing the starter")
	game.health = 3
	game.hero.invulnerable_remaining = 0
	chapter.use_station("generator")
	chapter.update(6.1)
	Input.action_release("interact_hold")
	_check(chapter.generator_started and chapter.actors[1].active and chapter.actors[2].active,"Completed starter wakes existing combat actors without spawning enemies")
	_check(not game.inventory.entries().any(func(item:Dictionary)->bool:return item.id=="isolation_coil"),"Installing the contact block frees its inventory slot")
	chapter.update(7.1)
	_check(chapter.fault=="vent_a" and not chapter.generator_ready,"Generator pressure produces the first readable mechanical task")
	var held_time: float = chapter.generator_time
	chapter.update(3)
	_check(chapter.generator_time==held_time,"Fault pauses progress instead of completing in the background")
	await _work("vent_a",3.1)
	_check("vent_a" in chapter.serviced and chapter.fault!="vent_a","Holding F at the west relief actually services line A")
	chapter.update(9)
	_check(chapter.fault=="vent_b","The second stage requires the opposite relief valve")
	await _work("vent_b",3.1)
	chapter.update(10)
	_check(chapter.generator_ready and chapter.serviced.size()==2,"Both serviced lines permit the generator to stabilize")
	await _use("log")
	_check(chapter.log_taken,"An energized cabinet gives the actual patient log")
	_check(not game.inventory.entries().any(func(item:Dictionary)->bool:return item.id=="isolation_tag"),"Registered bracelet no longer blocks a mission inventory slot")
	await _use("release")
	_check(chapter.complete and chapter.ending().contains("следствию"),"Guard post completes chapter; proof changes the ending")
	await _use("ammo")
	var reserve: int = game.combat.reserve
	chapter.use_station("ammo")
	_check(chapter.ammo_taken and game.combat.reserve==reserve,"New combat supplies are finite")
	# Actual firearm hit switches off a light and drops non-blocking physical debris.
	game.hero.reset_to(Vector3(50,.1,-84.5))
	actor.set_active(false)
	for other: CharacterBody3D in chapter.actors: other.set_active(false)
	Input.action_press("aim")
	await _frames(20)
	game.hero.camera_rig.camera.look_at(chapter.fitting.position)
	game.combat.aiming = true
	game.combat.update_weapon_pose()
	game.combat.cooldown = 0
	var fired: Dictionary = game.combat.fire()
	_check(fired.get("fired",false) and chapter.fitting.broken and chapter.fitting.light.light_energy==0,"Real shot breaks the fitting and changes local lighting")
	_check(is_instance_valid(chapter.fitting.debris) and chapter.fitting.debris.collision_layer==0,"Fallen fitting is physical without becoming a new blocking trap")
	Input.action_release("aim")
	await _frames(100)
	_check(chapter.fitting.debris.position.y>.08 and chapter.fitting.debris.position.y<.6,"Loose fitting falls and settles onto the real floor")
	var saved: Dictionary = Saves.snapshot(game)
	chapter.gates.west.restore_state(false,false)
	chapter.gates.west.set_barricade(2,1)
	saved = Saves.snapshot(game)
	_check(Saves.valid(saved),"Full mission checkpoint accepts completed isolation and persistent actor state")
	_check(Saves.apply(game,JSON.parse_string(JSON.stringify(saved))),"Isolation survives an actual JSON checkpoint round trip")
	await _frames(6)
	_freeze()
	_check(chapter.complete and chapter.proof and chapter.circuit=="light" and chapter.fitting.broken,"Continue preserves branch, evidence, completion and destroyed scenery")
	_check(chapter.pursuer.health==wounded_hp and chapter.pursuer.leg_damage==wounded_legs,"Continue keeps pursuer damage rather than resurrecting it")
	_check(chapter.gates.west.barricade_hp==2 and not chapter.gates.west.is_open,"Power restoration preserves a closed barricaded branch door")
	chapter.pursuer.set_active(true)
	chapter.pursuer.receive_bullet(game.hero.position,12,true)
	var dead_save: Dictionary = JSON.parse_string(JSON.stringify(Saves.snapshot(game)))
	_check(Saves.apply(game,dead_save) and chapter.pursuer.dead and chapter.pursuer.health==0,"A killed persistent pursuer stays dead after Continue")
	await _frames(4)
	_freeze()
	var bad: Dictionary = saved.duplicate(true)
	bad.isolation.ready = false
	_check(not Saves.valid(bad),"A log without generator power is rejected")
	bad = saved.duplicate(true)
	bad.isolation.positions[0][2] = -10000
	_check(not Saves.valid(bad),"Invalid persistent actor positions are rejected")
	bad = saved.duplicate(true)
	bad.isolation.evidence = ["diary","diary"]
	_check(not Saves.valid(bad),"Duplicate evidence cannot unlock a forged return path")
	bad = saved.duplicate(true)
	bad.erase("isolation")
	_check(Saves.valid(bad) and Saves.apply(game,bad),"Earlier checkpoints without isolation data remain loadable")
	await _frames(5)
	_freeze()
	_check(not chapter.complete and chapter.circuit.is_empty(),"Legacy save leaves the new chapter available rather than inventing progress")
	# The alternative branch has distinct light, glass and avoidable electrical risk.
	chapter.use_station("case")
	chapter.use_station("dark")
	_check(chapter.circuit=="dark" and chapter.gates.west.locked and not chapter.gates.east.locked,"Dark choice opens the east route and closes the west entry")
	_check(chapter.lights[1].light_energy<.1 and chapter.lights[0].light_energy>.5,"Dark branch changes actual room lights")
	var panel: StaticBody3D = chapter.partitions.glass
	_check(panel.strike() and panel.broken,"East shower shortcut uses actual breakable glass")
	await _frames(7)
	_check(not game.navigation.find_path(Vector3(52,.1,-70),chapter.stations.tag.interaction_point()).is_empty(),"A* updates through the broken glass into showers")
	game.hero.reset_to(Vector3(52,.1,-81))
	game.health = 3
	game.hero.invulnerable_remaining = 0
	chapter.clock = 0
	chapter.update(.01)
	_check(game.health==2,"Signposted live-water strip actually hurts during its visible pulse")
	game.hero.reset_to(Vector3(46,.1,-81))
	game.hero.invulnerable_remaining = 0
	chapter.clock = 0
	chapter.update(.01)
	_check(game.health==2,"A clear dry strip allows bypassing the electrical hazard")
	game.combat.cooldown = 0
	game.combat.knife_attack()
	_check(game.hero.model.knife_remaining>0,"Melee now drives the survivor's arm animation")
	var rounded: Array[Node] = game.combat.shotgun.find_children("*","MeshInstance3D",true,false)
	_check(rounded.any(func(node:Node)->bool:return node.get_meta("bevelled",false)),"Shotgun stock and forearm use shaped bevelled geometry")
	var a: AudioStreamWAV = game.sound_bank._foley("restraint_drag",1.25)
	var b: AudioStreamWAV = game.sound_bank._foley("isolation_relay",1.25)
	_check(a.data!=b.data and a.get_length()>1,"Pursuer and machinery have distinct actual sound waveforms")
	game.restart_game()
	await _frames(4)
	_check(chapter.circuit.is_empty() and not chapter.tag and not chapter.released and not chapter.proof and not chapter.complete,"New game resets the whole chapter and its branching progress")
	_check(not chapter.fitting.broken and not chapter.partitions.glass.broken and chapter.pursuer.health==12,"New game restores scenery and original pursuer state")
	print("ISOLATION CHAPTER: %d checks, %d failures"%[checks,failures])
	game.queue_free()
	await process_frame
	quit(1 if failures else 0)
func _fixture() -> void:
	game.mission.power_on = true
	game.mission.has_badge = true
	game.mission.relays.assign(["morgue","lab","theatre"])
	game.mission.has_fuel = true
	game.mission.has_coolant = true
	game.mission.purge_complete = true
	game.residential.restore({"mode":2,"prybar":true,"service":true,"broken":true,"core":true,"complete":true,"plate":true,"intro":true,"east":true})
	game.diagnostics.xray_solved = true
	game.diagnostics.observed = true
	game.diagnostics.complete = true
	game.combat.acquire()
	game.inventory.supplies_installed = true
func _use(id: String) -> void:
	game.hero.reset_to(chapter.stations[id].interaction_point()+Vector3(0,.1,0))
	game.hero.camera_rig.rotation.y = 0
	await _frames(5)
	game.interact()
	await _frames(24)
	if chapter.stations[id].drawer!=null: game.interact(); await _frames(2)
func _work(id: String,duration: float) -> void:
	game.hero.reset_to(chapter.stations[id].interaction_point())
	Input.action_press("interact_hold")
	chapter.use_station(id)
	chapter.update(duration)
	Input.action_release("interact_hold")
func _freeze() -> void:
	for hunter: CharacterBody3D in game.hunters: hunter.set_active(false); hunter.set_physics_process(false)
func _frames(count: int) -> void:
	for i: int in range(count): await physics_frame; await process_frame
func _check(ok: bool,message: String) -> void:
	checks+=1
	if not ok: failures+=1; push_error("FAIL: "+message)
