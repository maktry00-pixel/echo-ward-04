extends SceneTree
func _initialize() -> void:
	call_deferred("_run")
func _run() -> void:
	var source: Node3D = load("res://assets/models/survivor/locomotion.glb").instantiate()
	root.add_child(source)
	var player: AnimationPlayer = source.find_child("AnimationPlayer", true, false)
	print("CLIPS ", player.get_animation_list())
	var body: Node3D = load("res://assets/models/survivor/survivor.gltf").instantiate()
	root.add_child(body)
	print("SKELETONS ", body.find_children("*", "Skeleton3D", true, false))
	print("PATH ", body.get_path_to(body.find_child("Skeleton3D", true, false)))
	source.queue_free()
	body.queue_free()
	await process_frame
	quit()
