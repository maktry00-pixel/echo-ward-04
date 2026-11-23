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
	for enemy: CharacterBody3D in game.hunters:
		enemy.set_active(false)
		enemy.set_physics_process(false)
	_check(get_nodes_in_group("push_carts").is_empty(),"No dynamic cart exists anywhere in the game")
	for box: Node3D in get_nodes_in_group("search_storage"):
		_check(_standing_clear(box.interaction_point()),"Standing capsule fits at search approach for "+box.key)
	var storage: Node3D = game.mission.objects["fuse"].storage
	var object: Node3D = storage.prop
	game.hero.reset_to(object.position+Vector3(0,.1,.76))
	await _frames(4)
	storage.interact()
	await _frames(32)
	_check(not storage.opened and storage.drawer_extension==0,"Drawer refuses to animate its collider into a nearby standing player")
	_check(_standing_clear(game.hero.position),"Refused opening leaves the capsule outside furniture")
	game.hero.reset_to(object.interaction_point())
	await _frames(4)
	storage.interact()
	await _frames(32)
	_check(storage.opened and storage.drawer_extension>.35,"Stepping back allows the real drawer to extend")
	var event := InputEventKey.new()
	event.physical_keycode = KEY_C
	event.pressed = true
	game._unhandled_input(event)
	await _frames(2)
	_check(not storage.opened and storage.drawer_extension==0,"Actual C input retracts the collider and releases the aisle")
	storage.searched[0] = true
	_check(storage.valid(storage.snapshot()),"Closing a searched drawer is a valid saved state")
	for bed: Node3D in get_nodes_in_group("hide_beds"):
		var clear: bool = false
		for point: Vector3 in bed.exit_points(): clear = clear or _standing_clear(point)
		_check(clear,"At least one real standing exit remains free beside bed at "+str(bed.position))
	# Reproduce the two ward reward aisles with the actual open drawers.
	for index: int in [2,3]:
		var record: Node3D = game.cores[index]
		game.hero.reset_to(record.interaction_point())
		await _frames(5)
		record.storage.interact()
		await _frames(32)
		var from: Vector3 = game.hero.position
		game.hero.camera_rig.rotation.y = 0
		Input.action_press("move_right" if index==2 else "move_left")
		await _frames(40)
		Input.action_release("move_right" if index==2 else "move_left")
		_check(game.hero.position.distance_to(from)>1.4,"Player walks out between open drawer and ward beds on side "+str(index))
		_check(_standing_clear(game.hero.position),"Walking the former trap never overlaps a furniture collider")
	var bed: Node3D = get_nodes_in_group("hide_beds")[3]
	for at: Vector3 in bed.exit_points():
		if _standing_clear(at):
			game.hero.reset_to(at)
			break
	await _frames(5)
	_check(game.survival_tools.try_hide(bed),"Rotated ward bed accepts entry from its actual open long side")
	await _frames(4)
	_check(game.survival_tools.leave_bed() and _standing_clear(game.hero.position),"Rotated bed releases the player into a clear standing aisle")
	game.queue_free()
	await process_frame
	print("FURNITURE CLEARANCE: %d checks, %d failures" % [checks,failures])
	quit(0 if failures==0 else 1)
func _standing_clear(at: Vector3) -> bool:
	var shape := CapsuleShape3D.new()
	shape.radius = .34
	shape.height = 1.7
	var query := PhysicsShapeQueryParameters3D.new()
	query.shape = shape
	query.transform = Transform3D(Basis.IDENTITY,at+Vector3(0,.87,0))
	query.collision_mask = 1
	return game.get_world_3d().direct_space_state.intersect_shape(query,1).is_empty()
func _frames(count: int) -> void:
	for i: int in range(count):
		await physics_frame
		await process_frame
func _check(ok: bool,message: String) -> void:
	checks+=1
	if not ok:
		failures+=1
		push_error("FAIL: "+message)
