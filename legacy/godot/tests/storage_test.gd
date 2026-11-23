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
	await _frames(16)
	for enemy: CharacterBody3D in game.hunters: enemy.stun(500)
	var object: Node3D = game.mission.objects["fuse"]
	var storage: Node3D = object.storage
	game.hero.reset_to(object.interaction_point())
	await _frames(5)
	_check(game._interaction_target()==object,"Furniture itself does not occlude its own interaction")
	_check(not storage.ready_for_loot and not game.mission.has_fuse,"Closed drawer hides objective")
	game.interact()
	await _frames(30)
	_check(storage.opened and storage.drawer.position.z>storage.drawer_origin.z+.1,"Actual imported drawer slides out")
	_check(not game.mission.has_fuse,"Opening alone never grants item")
	storage.selected = 0
	Input.action_press("interact_hold")
	await _frames(30)
	Input.action_release("interact_hold")
	await _frames(3)
	_check(not game.hero.performing_action and not storage.searched[0],"Interrupted search releases movement without granting contents")
	Input.action_press("interact_hold")
	await _frames(55)
	Input.action_release("interact_hold")
	await _frames(2)
	_check(storage.searched[0] and not storage.ready_for_loot,"Empty compartment supplies no objective")
	storage.selected = storage.reward
	Input.action_press("interact_hold")
	await _frames(78)
	Input.action_release("interact_hold")
	await _frames(2)
	_check(storage.ready_for_loot and storage.preview.visible,"Searching correct compartment physically reveals contents")
	game.interact()
	_check(game.mission.has_fuse and object.used,"Real F interaction collects revealed objective")
	_check(object.visible and not storage.preview.visible,"Collected loot leaves cabinet and collision in place")
	var ids: Array[RID] = object.interaction_rids()
	_check(not ids.is_empty(),"Collectible furniture has real world collider")
	var query := PhysicsRayQueryParameters3D.create(object.position+Vector3(0,.5,2),object.position+Vector3(0,.5,-2),1)
	_check(not game.get_world_3d().direct_space_state.intersect_ray(query).is_empty(),"Bullets and world rays hit solid cabinet")
	game.hero.reset_to(game.mission.objects["power"].interaction_point())
	game.hero.camera_rig.rotation.y = 0
	Input.action_press("move_forward")
	await _frames(70)
	Input.action_release("move_forward")
	_check(game.hero.position.z>game.mission.objects["power"].position.z+.45,"Actual movement cannot pass through electrical panel")
	_check(get_nodes_in_group("push_carts").is_empty(),"No movable carts can jam the furniture route")
	var progress: Dictionary = Checkpoints.snapshot(game)
	progress.storage[storage.key].searched = [true,1,true]
	progress.storage[storage.key].opened = false
	_check(not Checkpoints.valid(progress),"Save rejects non-boolean search flags")
	game.mission.power_on = true
	game.mission.objects["power"].mark_used()
	progress = JSON.parse_string(JSON.stringify(Checkpoints.snapshot(game)))
	_check(Checkpoints.valid(progress) and Checkpoints.apply(game,progress),"Furniture search state round-trips through real checkpoint format")
	_check(not storage.opened and storage.searched[storage.reward] and game.mission.objects["fuse"].used,"Continue preserves solved compartment and used objective")
	game.restart_game()
	_check(not storage.opened and not storage.ready_for_loot and not storage.searched.has(true),"New game resets drawer pose, search and hidden loot")
	game.queue_free()
	await process_frame
	print("STORAGE AND PHYSICS: %d checks, %d failures" % [checks,failures])
	quit(0 if failures==0 else 1)
func _frames(count: int) -> void:
	for i: int in range(count):
		await physics_frame
		await process_frame
func _check(ok: bool,message: String) -> void:
	checks+=1
	if not ok:
		failures+=1
		push_error("FAIL: "+message)
