extends Node3D
## CC0 Quaternius base, derived outfit, and matching humanoid animations.
const BODY = preload("res://assets/models/survivor/survivor.gltf")
const MOTION = preload("res://assets/models/survivor/locomotion.glb")
var infected: bool = false
var player: AnimationPlayer
var current_clip: String = ""
var skeleton: Skeleton3D
var hand_world: Vector3

func _ready() -> void:
	var body: Node3D = BODY.instantiate()
	body.rotation.y = PI
	add_child(body)
	skeleton = body.find_child("Skeleton3D", true, false)
	process_priority = 50
	for mesh: MeshInstance3D in body.find_children("*", "MeshInstance3D", true, false):
		for i: int in range(mesh.mesh.get_surface_count()):
			var mat: Material = mesh.mesh.surface_get_material(i)
			if mat is StandardMaterial3D and mat.resource_name in ["Jacket", "Trousers", "Boots"]:
				var fabric: StandardMaterial3D = mat.duplicate()
				fabric.roughness = 0.95
				fabric.metallic = 0
				fabric.normal_enabled = false
				fabric.normal_texture = load("res://assets/textures/concrete/Concrete035_1K-JPG_NormalGL.jpg")
				fabric.normal_scale = 0.12
				fabric.uv1_triplanar = true
				fabric.uv1_scale = Vector3.ONE * 10
				mesh.set_surface_override_material(i, fabric)
	if not infected:
		_add_clothing_details(body)
	player = AnimationPlayer.new()
	body.add_child(player)
	player.callback_mode_process = AnimationMixer.ANIMATION_CALLBACK_MODE_PROCESS_MANUAL
	var source: Node3D = MOTION.instantiate()
	var source_player: AnimationPlayer = source.find_child("AnimationPlayer", true, false)
	var source_rig: Skeleton3D = source.find_child("Skeleton3D", true, false)
	var library := AnimationLibrary.new()
	var aliases: Dictionary = {"Idle":"idle", "Jog_Fwd":"run", "Walk":"walk", "Sprint":"sprint", "Pistol_Aim_Neutral":"aim", "Pistol_Reload":"reload", "Crouch_Fwd":"crouch", "Crouch_Idle":"crouch_idle"}
	for name: String in source_player.get_animation_list():
		for ending: String in aliases:
			if name.get_file() == ending:
				var clip: Animation = source_player.get_animation(name).duplicate(true)
				clip.loop_mode = Animation.LOOP_LINEAR
				for track: int in range(clip.get_track_count() - 1, -1, -1):
					var path: NodePath = clip.track_get_path(track)
					if path.get_subname_count() == 0:
						clip.remove_track(track)
						continue
					var bone: String = path.get_subname(0)
					if skeleton.find_bone(bone) < 0 or source_rig.find_bone(bone) < 0 or clip.track_get_type(track) == Animation.TYPE_SCALE_3D:
						clip.remove_track(track)
						continue
					var a: Transform3D = skeleton.get_bone_rest(skeleton.find_bone(bone))
					var b: Transform3D = source_rig.get_bone_rest(source_rig.find_bone(bone))
					for key: int in range(clip.track_get_key_count(track)):
						if clip.track_get_type(track) == Animation.TYPE_ROTATION_3D:
							var q: Quaternion = clip.track_get_key_value(track, key)
							clip.track_set_key_value(track, key, (a.basis.get_rotation_quaternion() * b.basis.get_rotation_quaternion().inverse() * q).normalized())
						elif clip.track_get_type(track) == Animation.TYPE_POSITION_3D:
							var p: Vector3 = clip.track_get_key_value(track, key)
							clip.track_set_key_value(track, key, a.origin + p - b.origin)
					clip.track_set_path(track, NodePath(str(body.get_path_to(skeleton)) + ":" + bone))
				library.add_animation(aliases[ending], clip)
	source.free()
	player.add_animation_library("", library)
	var aim_walk: Animation = library.get_animation("walk").duplicate(true)
	var aim: Animation = library.get_animation("aim")
	for track: int in range(aim_walk.get_track_count()):
		var bone: String = aim_walk.track_get_path(track).get_subname(0)
		if "arm" in bone or "hand" in bone or "clavicle" in bone:
			var match_track: int = aim.find_track(aim_walk.track_get_path(track),aim_walk.track_get_type(track))
			if match_track >= 0:
				for key: int in range(aim_walk.track_get_key_count(track)):
					aim_walk.track_set_key_value(track,key,aim.track_get_key_value(match_track,0))
	library.add_animation("aim_walk",aim_walk)
	animate(0)

func animate(speed: float, delta: float = 0.0) -> void:
	if player == null:
		return
	var hero: CharacterBody3D = get_parent()
	var clip: String = "sprint" if speed > 4.2 else ("run" if speed > 2.4 else ("walk" if speed > 0.2 else "idle"))
	if hero.crouching:
		clip = "crouch" if speed > 0.2 else "crouch_idle"
	if hero.aiming:
		clip = "aim" if speed < 0.2 else "aim_walk"
	if not player.has_animation(clip):
		clip = "idle"
	if clip != current_clip:
		player.play(clip, 0.16)
		current_clip = clip
	var stride: float = 5.4 if clip == "sprint" else (3.2 if clip == "run" else 1.6)
	player.speed_scale = clampf(speed / stride, 0.55, 1.3) if speed > 0.2 else 1.0
	skeleton.clear_bones_global_pose_override()
	player.advance(delta)
	update_pose()

func update_pose() -> void:
	if skeleton == null or infected:
		return
	var hero: CharacterBody3D = get_parent()
	if hero.aiming:
		var yaw := Basis(Vector3.UP, hero.camera_rig.rotation.y)
		var grip: Vector3 = hero.global_position + Vector3(0, 1.23, 0) + yaw * Vector3(0.24, 0, -0.36)
		_pose_arm("Right", grip)
		_pose_arm("Left", grip + yaw * Vector3(-0.07, -0.02, -0.06))
	var hand: int = skeleton.find_bone("hand_r")
	if hand >= 0:
		hand_world = (skeleton.global_transform * skeleton.get_bone_global_pose(hand)).origin

func _add_clothing_details(body: Node3D) -> void:
	var builder = preload("res://scripts/world_builder.gd")
	var fabric := builder.material("3b4740")
	var leather := builder.material("292c28")
	for attachment_data: Array in [["spine_03",Vector3(0,1.28,.16)],["pelvis",Vector3(0,.97,0)],["Head",Vector3.ZERO]]:
		var attachment := BoneAttachment3D.new()
		attachment.bone_name = attachment_data[0]
		skeleton.add_child(attachment)
		var rest_inverse: Transform3D = skeleton.get_bone_global_rest(skeleton.find_bone(attachment.bone_name)).affine_inverse()
		# Convert design-space positions through the imported armature to bone space.
		var details := Node3D.new()
		attachment.add_child(details)
		details.transform = rest_inverse * skeleton.global_transform.affine_inverse() * body.global_transform
		if attachment.bone_name == "spine_03":
			for side: float in [-1,1]:
				builder.box(details,Vector3(.09,.12,.035),Vector3(side*.13,1.28,.16),fabric)
				builder.box(details,Vector3(.10,.025,.04),Vector3(side*.13,1.34,.16),leather)
			builder.box(details,Vector3(.014,.32,.012),Vector3(0,1.3,.163),leather)
		elif attachment.bone_name == "pelvis":
			builder.box(details,Vector3(.37,.045,.30),Vector3(0,.97,0),leather)
		else:
			var hair := builder.material("29251f")
			hair.roughness = 1
			var crown: MeshInstance3D = builder.sphere(details,.12,Vector3(0,1.763,-.007),hair)
			crown.scale = Vector3(.79,.42,.92)
			for side: float in [-1,1]:
				var temple: MeshInstance3D = builder.sphere(details,.065,Vector3(side*.077,1.728,-.025),hair)
				temple.scale = Vector3(.30,.70,1.05)


func _pose_arm(side: String, target: Vector3) -> void:
	var upper: int = skeleton.find_bone("upperarm_" + ("r" if side == "Right" else "l"))
	var lower: int = skeleton.find_bone("lowerarm_" + ("r" if side == "Right" else "l"))
	var hand: int = skeleton.find_bone("hand_" + ("r" if side == "Right" else "l"))
	if upper < 0 or lower < 0 or hand < 0:
		return
	var a: Transform3D = skeleton.global_transform * skeleton.get_bone_global_pose(upper)
	var b: Transform3D = skeleton.global_transform * skeleton.get_bone_global_pose(lower)
	var c: Transform3D = skeleton.global_transform * skeleton.get_bone_global_pose(hand)
	var length_a: float = a.origin.distance_to(b.origin)
	var length_b: float = b.origin.distance_to(c.origin)
	var axis: Vector3 = (target - a.origin).normalized()
	var distance: float = clampf(a.origin.distance_to(target), 0.03, length_a + length_b - 0.005)
	var along: float = (length_a * length_a - length_b * length_b + distance * distance) / (2 * distance)
	var bend: Vector3 = (Vector3.DOWN - axis * Vector3.DOWN.dot(axis)).normalized()
	if bend.length_squared() < 0.1:
		bend = Vector3.RIGHT
	var elbow: Vector3 = a.origin + axis * along + bend * sqrt(maxf(0, length_a * length_a - along * along))
	var end: Vector3 = a.origin + axis * distance
	a.basis = Basis(Quaternion((b.origin - a.origin).normalized(), (elbow - a.origin).normalized())) * a.basis
	b.basis = Basis(Quaternion((c.origin - b.origin).normalized(), (end - elbow).normalized())) * b.basis
	b.origin = elbow
	skeleton.set_bone_global_pose_override(upper, skeleton.global_transform.affine_inverse() * a, 1, true)
	skeleton.set_bone_global_pose_override(lower, skeleton.global_transform.affine_inverse() * b, 1, true)
