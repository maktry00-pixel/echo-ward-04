extends SceneTree

func _initialize() -> void:
	call_deferred("_inspect")

func _inspect() -> void:
	for path: String in ["res://assets/models/characters/character.fbx", "res://assets/models/characters/idle.fbx", "res://assets/models/characters/run.fbx"]:
		var model: Node3D = load(path).instantiate()
		root.add_child(model)
		print("MODEL ", path)
		for node: Node in model.find_children("*", "", true, false):
			if node is Skeleton3D:
				for index: int in range(node.get_bone_count()):
					print("BONE ", index, " ", node.get_bone_name(index))
			if node is Node3D:
				print(model.get_path_to(node), " local=", node.transform)
			if node is MeshInstance3D:
				print("MESH ", model.get_path_to(node), " bounds=", node.get_aabb(), " transform=", node.global_transform)
			if node is AnimationPlayer:
				print("PLAYER ", model.get_path_to(node), " root=", node.root_node, " animations=", node.get_animation_list())
				for clip: String in node.get_animation_list():
					var anim: Animation = node.get_animation(clip)
					print("CLIP ",clip," len=", anim.length," track=",anim.track_get_path(0))
		model.free()
	quit()
