extends SceneTree
var game: Node3D
var checks: int = 0
var failures: int = 0
func _initialize() -> void: call_deferred("_run")
func _run() -> void:
	game = load("res://scenes/third_person.tscn").instantiate()
	game.set_meta("automated_test",true)
	root.add_child(game)
	await _frames(16)
	for enemy: CharacterBody3D in game.hunters:
		enemy.set_active(false)
		enemy.set_physics_process(false)
	game.hero.reset_to(Vector3(0,.1,11))
	game.combat.acquire()
	var enemy: CharacterBody3D = game.hunters[0]
	enemy.position = Vector3(0,.08,4)
	enemy.set_active(true)
	enemy.model.animate(2.4)
	var foot: Vector3 = enemy.model.leg_feet[0]
	await _frames(12)
	_check(enemy.model.leg_feet[0].distance_to(foot)>.05,"Jointed foot travels relative to body during locomotion")
	var low: float = INF
	var high: float = -INF
	for frame: int in range(24):
		for point: Vector3 in enemy.model.leg_feet:
			low = minf(low,point.y)
			high = maxf(high,point.y)
		await _frames(1)
	_check(high-low>.05,"Tripod gait alternates planted and lifted feet")
	Input.action_press("aim")
	await _frames(10)
	_aim(enemy.aim_point())
	await _frames(2)
	_aim(enemy.aim_point())
	var before: int = game.combat.rounds
	var pitch: float = game.hero.camera_rig.rotation.x
	var shot: Dictionary = game.combat.fire()
	_check(shot.get("hit",false) and shot.get("headshot",false) and game.combat.rounds==before-1,"Actual camera/muzzle shot hits low creature head")
	_check(get_nodes_in_group("spent_casings").size()==1,"Live shot ejects physical casing")
	_check(game.combat.flash_remaining>0 and game.hero.camera_rig.rotation.x>pitch,"Shot produces muzzle burst and camera recoil after ray evaluation")
	_check(enemy.model.impact_remaining>0 and enemy.health==3,"Nonlethal hit supplies body reaction and damage")
	await _frames(27)
	_aim(enemy.aim_point())
	await _frames(2)
	_aim(enemy.aim_point())
	shot = game.combat.fire()
	_check(shot.get("killed",false) and enemy.model.dying and not enemy.active,"Lethal shot starts jointed death instead of disabling animation")
	await _frames(20)
	_check(enemy.model.death_time>0 and enemy.model.death_time<1.25,"Death is a progressing collapse")
	await _frames(80)
	var lowest: float = INF
	for mesh: MeshInstance3D in enemy.model.corpse_meshes:
		var bounds: AABB = mesh.global_transform*mesh.get_aabb()
		lowest = minf(lowest,bounds.position.y)
	_check(absf(lowest-.02)<.04,"Settled corpse geometry rests on physical floor height")
	_check(not enemy.model.is_processing(),"Settled corpse stops animation processing")
	game.combat.reload_weapon()
	await _frames(18)
	var rig: Skeleton3D = game.hero.model.skeleton
	var left: int = rig.find_bone("Bip01 L Hand")
	var hand: Vector3 = (rig.global_transform*rig.get_bone_global_pose(left)).origin
	await _frames(23)
	_check(hand.distance_to((rig.global_transform*rig.get_bone_global_pose(left)).origin)>.05,"Reload moves left hand between pouch and grip")
	await _frames(75)
	Input.action_release("aim")
	var hit_at: Vector3 = game.hero.position
	game._take_hit(game.hunters[1])
	var reaction: bool = game.health==2 and game.hero.model.hurt_remaining>0 and game.hero.hit_impulse.length()>1
	await _frames(12)
	_check(reaction and game.hero.position.distance_to(hit_at)>.15,"Player damage triggers pose and actual physical knockback")
	var blind: CharacterBody3D = game.hunters[13]
	blind.position = game.hero.position+Vector3(0,0,-3)
	blind.set_active(true)
	game.flash_charges = 1
	_check(game.use_flash()==0 and blind.stunned_remaining==0,"Blind hunter cannot be stopped by visual flash")
	_check(blind.last_known.distance_to(game.hero.position)<.1,"Flash noise remains a real hearing observation")
	game.combat.acquire()
	# The animation fixture explicitly retains the underlying defeat presentation.
	game.auto_respawn_enabled=false
	game._finish_round(false,"fixture")
	var gun: RigidBody3D = get_nodes_in_group("dropped_weapons")[0]
	var gun_height: float = gun.global_position.y
	await _frames(90)
	_check(game.hero.model.dying and game.hero.model.rotation.x<-.9,"Player collapse advances while defeat pauses gameplay")
	_check(not game.combat.weapon.visible and gun.global_position.y<gun_height-.2,"Defeat drops physical gun instead of leaving held weapon floating")
	var gun_lowest: float = INF
	for mesh: MeshInstance3D in gun.find_children("*","MeshInstance3D",true,false):
		gun_lowest = minf(gun_lowest,(mesh.global_transform*mesh.get_aabb()).position.y)
	_check(gun.settled and absf(gun_lowest-.003)<.015,"Detailed dropped weapon rests on floor rather than floating above its old collider")
	game.restart_game()
	await _frames(3)
	_check(not game.hero.model.dying and absf(game.hero.model.rotation.x)<.001 and get_nodes_in_group("dropped_weapons").is_empty(),"Restart clears collapse, dropped weapon and presentation state")
	game.auto_respawn_enabled=true
	game.queue_free()
	await process_frame
	print("ACTION AND ANIMATION: %d checks, %d failures" % [checks,failures])
	quit(0 if failures==0 else 1)
func _aim(point: Vector3) -> void:
	var camera: Camera3D = game.hero.camera_rig.camera
	var center: Vector2 = camera.get_viewport().get_visible_rect().size/2
	var origin: Vector3 = camera.project_ray_origin(center)
	var direction: Vector3 = camera.project_ray_normal(center)
	var wanted: Vector3 = (point-origin).normalized()
	game.hero.camera_rig.apply_look(Vector2(-wrapf(atan2(-wanted.x,-wanted.z)-atan2(-direction.x,-direction.z),-PI,PI),-(asin(wanted.y)-asin(direction.y)))/game.hero.camera_rig.look_sensitivity)
func _frames(count: int) -> void:
	for i: int in range(count):
		await physics_frame
		await process_frame
func _check(ok: bool,message: String) -> void:
	checks+=1
	if not ok:
		failures+=1
		push_error("FAIL: "+message)
