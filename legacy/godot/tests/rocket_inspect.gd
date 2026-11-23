extends SceneTree
func _initialize() -> void: call_deferred("_run")
func _run() -> void:
	var body: Node3D = load("res://assets/models/rocketbox/Export/Male_Adult_04.fbx").instantiate()
	root.add_child(body)
	var rig: Skeleton3D = body.find_child("Skeleton3D",true,false)
	print("RIG ",body.get_path_to(rig)," xform ",rig.global_transform)
	for i: int in range(rig.get_bone_count()):
		print(i," ",rig.get_bone_name(i)," rest ",rig.get_bone_rest(i).origin)
	for mesh: MeshInstance3D in body.find_children("*","MeshInstance3D",true,false):
		print("MESH ",mesh.name," ",mesh.get_aabb()," ",mesh.global_transform)
		for i: int in range(mesh.mesh.get_surface_count()):
			var mat: StandardMaterial3D = mesh.mesh.surface_get_material(i)
			print("MAT ",mat.resource_name," ",mat.albedo_color," ",mat.albedo_texture)
	for n: String in ["m_idle_breathe_01.max.fbx","m_walk_neutral.max.fbx","m_run_neutral.max.fbx","m_crouch_idle.max.fbx"]:
		var source: Node3D = load("res://assets/models/rocketbox/motion/"+n).instantiate()
		root.add_child(source)
		var player: AnimationPlayer = source.find_child("AnimationPlayer",true,false)
		print("SOURCE ",n," ",player.get_animation_list())
		var clip: Animation = player.get_animation(player.get_animation_list()[0])
		print("LENGTH ",clip.length," tracks ",clip.get_track_count())
		for track: int in range(mini(clip.get_track_count(),9)):
			print(clip.track_get_path(track)," type ",clip.track_get_type(track)," first ",clip.track_get_key_value(track,0)," last ",clip.track_get_key_value(track,clip.track_get_key_count(track)-1))
		source.queue_free()
	body.queue_free()
	await process_frame
	quit()
