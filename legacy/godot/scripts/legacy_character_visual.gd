extends Node3D
## Kenney's CC0 skinned character and matching skeleton animations.

const BODY = preload("res://assets/models/characters/character.fbx")
const IDLE = preload("res://assets/models/characters/idle.fbx")
const RUN = preload("res://assets/models/characters/run.fbx")
var infected: bool = false
var player: AnimationPlayer
var current_clip: String = ""
var skeleton: Skeleton3D
var hand_world: Vector3


func _ready() -> void:
	var body: Node3D = BODY.instantiate()
	body.scale = Vector3(0.62, 0.66, 0.57) if infected else Vector3.ONE * 0.51
	body.rotation.y = PI
	add_child(body)
	skeleton = body.find_child("Skeleton3D", true, false)
	process_priority = 50
	var skin := StandardMaterial3D.new()
	skin.albedo_texture = load("res://assets/models/characters/infected.png" if infected else "res://assets/models/characters/survivor.png")
	skin.roughness = 0.9
	if infected:
		skin.albedo_color = Color("7c9187")
	for node: Node in body.find_children("*", "MeshInstance3D", true, false):
		node.material_override = skin
	player = AnimationPlayer.new()
	body.add_child(player)
	var library := AnimationLibrary.new()
	for pair: Array in [[IDLE, "Idle", "idle"], [RUN, "Run", "run"]]:
		var source: Node = pair[0].instantiate()
		var source_player: AnimationPlayer = source.find_child("AnimationPlayer", true, false)
		for name: String in source_player.get_animation_list():
			if name.ends_with(pair[1]):
				var clip: Animation = source_player.get_animation(name).duplicate(true)
				clip.loop_mode = Animation.LOOP_LINEAR
				# Less oversized heads and a taller, gaunt silhouette for this setting.
				for track: int in range(clip.get_track_count() - 1, -1, -1):
					if clip.track_get_type(track) == Animation.TYPE_SCALE_3D and str(clip.track_get_path(track)).ends_with(":Head"):
						clip.remove_track(track)
				var head_track: int = clip.add_track(Animation.TYPE_SCALE_3D)
				clip.track_set_path(head_track, NodePath("Root/Skeleton3D:Head"))
				clip.track_insert_key(head_track, 0.0, Vector3.ONE * (0.6 if infected else 0.66))
				library.add_animation(pair[2], clip)
		source.free()
	player.add_animation_library("", library)
	animate(0.0)


func animate(speed: float) -> void:
	if player == null:
		return
	var clip: String = "run" if speed > 0.2 else "idle"
	if clip != current_clip:
		player.play(clip, 0.18)
		current_clip = clip
	player.speed_scale = clampf(speed / 4.5, 0.5, 1.35) if clip == "run" else 0.8


func _process(_delta: float) -> void:
	if skeleton == null or infected:
		return
	skeleton.clear_bones_global_pose_override()
	var hero: CharacterBody3D = get_parent()
	if hero.aiming:
		var yaw := Basis(Vector3.UP, hero.camera_rig.rotation.y)
		var grip: Vector3 = hero.global_position + Vector3(0, 1.23, 0) + yaw * Vector3(0.24, 0, -0.36)
		_pose_arm("Right", grip)
		_pose_arm("Left", grip + yaw * Vector3(-0.07, -0.02, -0.06))
	var hand: int = skeleton.find_bone("RightHand")
	if hand >= 0:
		hand_world = (skeleton.global_transform * skeleton.get_bone_global_pose(hand)).origin


func _pose_arm(side: String, target: Vector3) -> void:
	var upper: int = skeleton.find_bone(side + "Arm")
	var lower: int = skeleton.find_bone(side + "ForeArm")
	var hand: int = skeleton.find_bone(side + "Hand")
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
