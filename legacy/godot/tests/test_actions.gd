extends RefCounted
## Fixtures may place a character; the opening/search itself uses real F input.
static func search(tree: SceneTree,game: Node3D,object: Node3D) -> void:
	if object.get("storage")==null or (object.storage.ready_for_loot and object.storage.opened): return
	game.hero.reset_to(object.interaction_point())
	var look: Vector3 = object.global_position-game.hero.global_position
	game.hero.camera_rig.rotation.y = atan2(-look.x,-look.z)
	for i: int in range(4):
		await tree.physics_frame
		await tree.process_frame
	game.interact()
	Input.action_press("interact_hold")
	for i: int in range(280):
		if object.storage.ready_for_loot: break
		await tree.physics_frame
		await tree.process_frame
	Input.action_release("interact_hold")
	await tree.physics_frame
	await tree.process_frame
