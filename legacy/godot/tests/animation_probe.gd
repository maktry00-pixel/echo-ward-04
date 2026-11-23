extends SceneTree
func _initialize() -> void: call_deferred("_run")
func _run() -> void:
	var body: Node3D = load("res://assets/models/survivor/survivor.gltf").instantiate()
	var motion: Node3D = load("res://assets/models/survivor/locomotion.glb").instantiate()
	root.add_child(body)
	root.add_child(motion)
	var a: Skeleton3D = body.find_child("Skeleton3D", true, false)
	var b: Skeleton3D = motion.find_child("Skeleton3D", true, false)
	for bone: String in ["pelvis", "spine_01", "upperarm_r", "lowerarm_r", "hand_r", "thigh_r", "calf_r", "foot_r"]:
		print(bone, " BODY ", a.get_bone_rest(a.find_bone(bone)), " SOURCE ", b.get_bone_rest(b.find_bone(bone)))
	var player: AnimationPlayer = motion.find_child("AnimationPlayer", true, false)
	var clip: Animation = player.get_animation("Walk")
	for track: int in range(clip.get_track_count()):
		if clip.track_get_type(track) == Animation.TYPE_POSITION_3D:
			print("POSITION ", clip.track_get_path(track), " 0=", clip.track_get_key_value(track, 0), " last=", clip.track_get_key_value(track, clip.track_get_key_count(track)-1))
	body.queue_free()
	motion.queue_free()
	await process_frame
	quit()
