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
	await _frames(20)
	var tools: Node = game.survival_tools
	var hero: CharacterBody3D = game.hero
	for enemy: CharacterBody3D in game.hunters: enemy.set_active(false)
	_check(get_nodes_in_group("hide_beds").size()==16,"Sixteen actual beds across wards, theatre, residence, diagnostics and isolation")
	_check(get_nodes_in_group("push_carts").is_empty(),"Movable carts removed from all rooms")
	_check(game.navigation.movable_bodies.is_empty(),"No obsolete cart navigation exclusions")
	var bed: Node3D = get_nodes_in_group("hide_beds")[4]
	hero.reset_to(bed.global_position+Vector3(-1.2,.1,0))
	await _frames(5)
	_check(tools.try_hide(bed),"Reachable bedside permits hiding")
	await _frames(15)
	_check(hero.hiding_bed==bed and is_equal_approx(hero.body_collision.shape.height,.48),"Hiding uses reduced real capsule")
	var query := PhysicsShapeQueryParameters3D.new()
	query.shape = hero.body_collision.shape
	query.transform = hero.body_collision.global_transform
	query.collision_mask = 1
	_check(game.get_world_3d().direct_space_state.intersect_shape(query).is_empty(),"Prone capsule fits under real bed without overlap")
	_check(not hero.model.visible and hero.camera_rig.position.y<.4,"Camera moves to floor and hides body")
	var hunter: CharacterBody3D = game.hunters[0]
	hunter.position = bed.global_position+Vector3(-2.2,.1,0)
	hunter.model.rotation.y = -PI/2
	hunter.set_active(true)
	hunter.set_physics_process(false)
	_check(not hunter.can_see_player(),"Unwitnessed hidden player is not visible")
	game.combat.acquire()
	_check(not tools.throw_bottle() and not game.combat.reload_weapon() and game.use_flash()==0,"Hiding cannot be used as a firing or gadget bunker")
	var stamina_before: float = hero.stamina
	Input.action_press("sprint")
	await _frames(190)
	Input.action_release("sprint")
	_check(hero.stamina<stamina_before-45 and hunter.remembered_bed==null,"Holding breath costs stamina and avoids audible clue")
	hero.stamina = 1
	Input.action_press("sprint")
	await _frames(30)
	_check(tools.breath_exhausted,"Empty stamina cannot be exploited to hold breath on alternating frames")
	Input.action_release("sprint")
	await _frames(185)
	_check(hunter.remembered_bed==bed,"Nearby audible breathing supplies evidence to inspect bed")
	hunter.remembered_bed = null
	_check(tools.leave_bed() and hero.hiding_bed==null,"F exit restores standing collider in clear space")
	await _frames(8)
	hunter.stunned_remaining = 0
	_check(hunter.can_see_player(),"Hunter sees exposed player at side of bed")
	_check(tools.try_hide(bed) and hunter.remembered_bed==bed,"Witnessed entry is remembered only by observer")
	var health_before: int = game.health
	hunter.position = hunter.bed_entry+Vector3(0,0,-2.0)
	hunter.set_physics_process(true)
	for i: int in range(300):
		if hero.hiding_bed == null: break
		await _frames(1)
	hunter.set_physics_process(false)
	_check(game.health==health_before-1,"Actual AI walks to witnessed bed and attacks instead of forgetting player")
	_check(hero.hiding_bed==null,"Checked bed forces player back into the room")
	hunter.set_active(false)
	hero.reset_to(Vector3(0,.1,11))
	Input.action_press("crouch")
	Input.action_press("move_forward")
	await _frames(40)
	_check(hero.crouching and not hero.sprinting and Vector2(hero.velocity.x,hero.velocity.z).length()<1.4,"Quiet movement is slower and uses crouch animation")
	_check(hero.model.current_clip=="crouch","Authored crouch locomotion is active")
	Input.action_release("crouch")
	Input.action_release("move_forward")
	await _frames(15)
	hunter.position = Vector3(0,.1,3)
	hunter.model.rotation.y = PI
	hunter._set_state(hunter.State.PATROL)
	hunter.stunned_remaining = 0
	hunter.set_active(true)
	_check(tools.throw_bottle() and tools.bottles==2,"Throw consumes limited bottle")
	_check(not tools.throw_bottle(),"Cooldown prevents duplicate throws")
	await _frames(180)
	_check(get_nodes_in_group("thrown_bottles").is_empty() and hunter.state==hunter.State.INVESTIGATE,"Ballistic impact shatters and attracts an actual hunter")
	hunter.set_active(false)
	game.mission.has_fuse = true
	hero.reset_to(game.mission.objects["power"].position+Vector3(0,.1,.8))
	await _frames(5)
	Input.action_press("interact_hold")
	game.interact()
	await _frames(90)
	_check(not game.mission.power_on and hero.performing_action,"A single press cannot install fuse")
	Input.action_release("interact_hold")
	await _frames(5)
	_check(not hero.performing_action and tools.work.get("power",0)>1,"Interrupted repair retains partial work and releases movement")
	var work_before: float = tools.work["power"]
	await _frames(30)
	_check(is_equal_approx(tools.work["power"],work_before),"Released key does not progress repair")
	Input.action_press("interact_hold")
	game.interact()
	await _frames(160)
	Input.action_release("interact_hold")
	_check(game.mission.power_on and not hero.performing_action,"Holding resumes and completes powered progression")
	var snapshot: Dictionary = Checkpoints.snapshot(game)
	_check(snapshot["bottles"]==2 and Checkpoints.valid(snapshot),"Checkpoint preserves remaining bottles")
	var invalid: Dictionary = snapshot.duplicate(true)
	invalid["bottles"] = 99
	_check(not Checkpoints.valid(invalid),"Invalid bottle counts are rejected")
	_check(Checkpoints.apply(game,snapshot) and tools.bottles==2 and hero.hiding_bed==null,"Continue restores supplies at safe nonhidden spawn")
	await _frames(20)
	var hand_rest_world: Vector3 = hero.model.hand_world-hero.global_position
	_check(hand_rest_world.y<1.15,"Native idle lowers hands instead of leaving adult hero in A-pose")
	# Rig regression: every animated position track is relative to its destination rest.
	var rig: Skeleton3D = hero.model.skeleton
	var foot: int = rig.find_bone("Bip01 R Foot")
	var calf: int = rig.find_bone("Bip01 R Calf")
	var rest_length: float = rig.get_bone_rest(foot).origin.length()
	hero.reset_to(Vector3(0,.1,12))
	Input.action_press("move_forward")
	await _frames(30)
	var pose_length: float = rig.get_bone_global_pose(foot).origin.distance_to(rig.get_bone_global_pose(calf).origin)
	_check(absf(pose_length-rest_length)<.005,"Retargeting preserves destination limb lengths")
	Input.action_release("move_forward")
	for i: int in range(120): hero.model.animate(3.2,1.0/60)
	var pelvis: Vector3 = rig.get_bone_pose_position(rig.find_bone("Bip01"))
	_check(absf(pelvis.x)<.001 and absf(pelvis.z)<.001,"Repeated locomotion does not translate visual root away from physical body")
	await _frames(15)
	game.combat.update_weapon_pose()
	_check(game.combat.weapon.global_position.distance_to(hero.model.hand_world+Vector3(0,.08,0))<.001,"Weapon uses current sampled hand position")
	_check(game.hunters[0].crawler and game.hunters[0].head_height<1 and game.hunters[3].model.sacs.size()==2,"Enemy anatomy and hit regions differ from human hero")
	_check(game.hunters[0].is_head_hit(game.hunters[0].aim_point()) and not game.hunters[0].is_head_hit(game.hunters[0].position+Vector3(0,.3,0)),"Crawler head/body distinction matches low anatomy")
	game.restart_game()
	_check(tools.bottles==3 and tools.work.is_empty() and hero.hiding_bed==null,"New run resets tools and interrupted work")
	game.queue_free()
	await process_frame
	print("STEALTH: %d checks, %d failures" % [checks,failures])
	quit(0 if failures==0 else 1)
func _frames(count: int) -> void:
	for i: int in range(count):
		await physics_frame
		await process_frame
func _check(ok: bool, message: String) -> void:
	checks += 1
	if not ok:
		failures += 1
		push_error("FAIL: "+message)
