extends Node3D
## MIT Rocketbox male with native Biped animations; preserve lengths, remove root travel.
const ACTOR = preload("res://assets/models/rocketbox/Export/Male_Adult_04.fbx")
const MOTIONS: Dictionary = {"idle":"m_idle_breathe_01.max.fbx","walk":"m_walk_neutral.max.fbx","run":"m_run_neutral.max.fbx","crouch_idle":"m_crouch_idle.max.fbx"}
var player: AnimationPlayer
var current_clip: String = ""
var skeleton: Skeleton3D
var hand_world: Vector3
var crouch_offset: float = 0
var hurt_remaining: float = 0
var shot_remaining: float = 0
var dying: bool = false
var knife_remaining: float = 0
var death_time: float = 0
var death_bones: Dictionary = {}
var action_clock: float = 0
var foot_plant := preload("res://scripts/foot_plant.gd").new()
var playback_speed: float = 1
var legs_yaw: float = 0
var finger_rest: Dictionary = {}

func _ready() -> void:
	var body: Node3D = ACTOR.instantiate()
	body.rotation.y = PI
	add_child(body)
	skeleton = body.find_child("Skeleton3D",true,false)
	for mesh: MeshInstance3D in body.find_children("*","MeshInstance3D",true,false):
		for i: int in range(mesh.mesh.get_surface_count()):
			var material: StandardMaterial3D = mesh.mesh.surface_get_material(i).duplicate()
			material.metallic = 0
			material.roughness = .90 if "body" in material.resource_name else .82
			material.metallic_specular = .23
			if "opacity" in material.resource_name:
				material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA_SCISSOR
				material.alpha_scissor_threshold = .4
				material.cull_mode = BaseMaterial3D.CULL_DISABLED
			mesh.set_surface_override_material(i,material)
	for i: int in range(skeleton.get_bone_count()):
		if "Finger" in skeleton.get_bone_name(i): finger_rest[i] = skeleton.get_bone_rest(i).basis.get_rotation_quaternion()
	player = AnimationPlayer.new()
	body.add_child(player)
	player.callback_mode_process = AnimationMixer.ANIMATION_CALLBACK_MODE_PROCESS_MANUAL
	var library := AnimationLibrary.new()
	for alias: String in MOTIONS:
		var source: Node3D = load("res://assets/models/rocketbox/motion/"+MOTIONS[alias]).instantiate()
		var source_player: AnimationPlayer = source.find_child("AnimationPlayer",true,false)
		var source_rig: Skeleton3D = source.find_child("Skeleton3D",true,false)
		var clip: Animation = source_player.get_animation(source_player.get_animation_list()[0]).duplicate(true)
		clip.loop_mode = Animation.LOOP_LINEAR
		for track: int in range(clip.get_track_count()-1,-1,-1):
			var path: NodePath = clip.track_get_path(track)
			var bone: String = path.get_subname(0) if path.get_subname_count()>0 else ""
			var dst: int = skeleton.find_bone(bone)
			var src: int = source_rig.find_bone(bone)
			if dst<0 or src<0 or clip.track_get_type(track)==Animation.TYPE_SCALE_3D:
				clip.remove_track(track)
				continue
			var a: Transform3D = skeleton.get_bone_rest(dst)
			var b: Transform3D = source_rig.get_bone_rest(src)
			for key: int in range(clip.track_get_key_count(track)):
				if clip.track_get_type(track)==Animation.TYPE_ROTATION_3D:
					var q: Quaternion = clip.track_get_key_value(track,key)
					# Rocketbox clips share the same authored Biped axes. Imported rest
					# is the first animation frame, so rest correction would cancel its pose.
					clip.track_set_key_value(track,key,q.normalized())
				elif clip.track_get_type(track)==Animation.TYPE_POSITION_3D:
					var p: Vector3 = clip.track_get_key_value(track,key)
					# Keep the destination's limb lengths and only pelvis vertical motion.
					clip.track_set_key_value(track,key,Vector3(a.origin.x,p.y,a.origin.z) if bone=="Bip01" else a.origin)
			clip.track_set_path(track,NodePath(str(body.get_path_to(skeleton))+":"+bone))
		library.add_animation(alias,clip)
		source.free()
	library.add_animation("sprint",library.get_animation("run").duplicate())
	library.add_animation("crouch",library.get_animation("walk").duplicate())
	player.add_animation_library("",library)
	animate(0)

func animate(speed: float, delta: float = 0.0) -> void:
	if player==null: return
	if dying: return
	hurt_remaining = maxf(0,hurt_remaining-delta)
	action_clock += delta
	shot_remaining = maxf(0,shot_remaining-delta)
	knife_remaining = maxf(0,knife_remaining-delta)
	var hero: CharacterBody3D = get_parent()
	var running: bool = speed>(2.05 if current_clip=="run" else 2.35)
	var moving: bool = speed>(.08 if current_clip in ["walk","run","crouch"] else .18)
	var clip: String = "run" if running else ("walk" if moving else "idle")
	if hero.crouching: clip = "crouch" if speed>.2 else "crouch_idle"
	if clip!=current_clip:
		player.play(clip,.20)
		current_clip = clip
	var desired_rate: float = clampf(speed/(3.075 if clip=="run" else 1.402),.06,2.2) if moving else 1
	if hero.aiming and hero.velocity.dot(-global_basis.z)<-.15: desired_rate *= -1
	playback_speed = move_toward(playback_speed,desired_rate,delta*7)
	player.speed_scale = playback_speed
	skeleton.clear_bones_global_pose_override()
	player.advance(delta)
	crouch_offset = move_toward(crouch_offset,.25 if clip=="crouch" else 0,delta*1.25)
	if crouch_offset>0:
		var feet: Array[Transform3D] = []
		for side: String in ["R","L"]: feet.append(skeleton.global_transform*skeleton.get_bone_global_pose(skeleton.find_bone("Bip01 "+side+" Foot")))
		var root_bone: int = skeleton.find_bone("Bip01")
		skeleton.set_bone_pose_position(root_bone,skeleton.get_bone_pose_position(root_bone)-Vector3(0,crouch_offset,0))
		for i: int in range(2):
			var prefix: String = "Bip01 "+("R" if i==0 else "L")+" "
			var forward: Vector3 = -global_basis.z
			_pose_chain(skeleton.find_bone(prefix+"Thigh"),skeleton.find_bone(prefix+"Calf"),skeleton.find_bone(prefix+"Foot"),feet[i].origin,forward)
			skeleton.set_bone_global_pose_override(skeleton.find_bone(prefix+"Foot"),skeleton.global_transform.affine_inverse()*feet[i],1,true)
	_directional_pose(hero,delta)
	foot_plant.apply(self,delta)
	_offset_bone("Bip01 Spine",Vector3(0,0,clampf(-hero.turn_velocity*.018,-.12,.12)))
	var game: Node = get_tree().get_first_node_in_group("game")
	if game!=null and game.health<3: _offset_bone("Bip01 Spine1",Vector3(.075*(3-game.health),0,.025*sin(action_clock*5)))
	if hurt_remaining>0: _offset_bone("Bip01 Spine1",Vector3(.22,0,.12)*sin(hurt_remaining/.45*PI))
	if shot_remaining>0: _offset_bone("Bip01 Spine2",Vector3(-shot_remaining*.3,0,0))
	update_pose()

func update_pose() -> void:
	var hero: CharacterBody3D = get_parent()
	if hero.aiming:
		var yaw := Basis(Vector3.UP,hero.camera_rig.rotation.y)
		var pitch := Basis(Vector3.RIGHT,hero.camera_rig.rotation.x)
		var grip: Vector3 = hero.global_position+Vector3(0,1.35,0)+yaw*Vector3(.24,0,-.36+(get_tree().get_first_node_in_group("game").combat.recoil*.035 if get_tree().get_first_node_in_group("game").combat!=null else 0))+yaw*pitch*Vector3(.03,-.075,.085)
		var combat: Node = get_tree().get_first_node_in_group("game").combat
		var points: Array[Vector3] = combat.grip_points()
		if combat.selected_weapon=="shotgun" or combat.wall_blend>.01: grip = points[0]
		_pose_arm("Right",grip)
		_pose_arm("Left",points[1] if combat.selected_weapon=="shotgun" or combat.wall_blend>.01 else grip+yaw*pitch*Vector3(-.065,-.015,.005))
	if hero.reload_phase>=0:
		var yaw := Basis(Vector3.UP,hero.camera_rig.rotation.y)
		var grip: Vector3 = hero.global_position+Vector3(0,1.1,0)+yaw*Vector3(.20,0,-.22)
		var phase: float = hero.reload_phase
		var left: Vector3 = grip+yaw*Vector3(-.05,-.12,0)
		if phase<.4: left = left.lerp(hero.global_position+Vector3(0,.87,0)+yaw*Vector3(-.18,0,.02),sin(phase/.4*PI/2))
		elif phase<.7: left = (hero.global_position+Vector3(0,.87,0)+yaw*Vector3(-.18,0,.02)).lerp(left,smoothstep(.4,.7,phase))
		else: left += yaw*Vector3(-.04,sin((phase-.7)/.3*PI)*.12,-.04)
		_pose_arm("Right",grip)
		_pose_arm("Left",left)
		var combat: Node = get_tree().get_first_node_in_group("game").combat
		if combat.selected_weapon=="shotgun":
			_pose_arm("Right",combat.grip_points()[0])
			_pose_arm("Left",combat.weapon.global_transform*Vector3(-.055,-.18+sin(phase*PI)*.08,-.04))
		else:
			_offset_bone("Bip01 Spine2",Vector3(.03,0,sin(action_clock*5)*minf(hero.velocity.length(),3)*.012))
	if hero.performing_action:
		_offset_bone("Bip01 Spine1",Vector3(.10,0,sin(action_clock*4)*.025))
		var sway := Vector3(sin(action_clock*4)*.025,sin(action_clock*6)*.035,0)
		_pose_arm("Right",hero.action_focus+Vector3(.10,0,0)+sway)
		_pose_arm("Left",hero.action_focus+Vector3(-.10,0,0)-sway)
	if knife_remaining>0:
		var yaw := Basis(Vector3.UP,hero.camera_rig.rotation.y)
		var phase: float = 1-knife_remaining/.32
		_pose_arm("Right",hero.global_position+Vector3(0,1.18+sin(phase*PI)*.12,0)+yaw*Vector3(.27-.40*phase,0,-.42))
		_pose_arm("Left",hero.global_position+Vector3(0,1.22,0)+yaw*Vector3(-.20,0,-.12))
	var game: Node = get_tree().get_first_node_in_group("game")
	if game!=null and game.combat!=null and game.combat.equipped:
		for index: int in finger_rest:
			var name: String = skeleton.get_bone_name(index)
			var curl: float = .85 if not "Finger0" in name else .35
			if " R Finger1" in name: curl = .35 if shot_remaining<=0 else .8
			skeleton.set_bone_pose_rotation(index,finger_rest[index]*Quaternion(Vector3.BACK if " R " in name else Vector3.FORWARD,curl))
		if hero.aiming and hero.reload_phase<0:
			for side: String in ["R","L"]:
				var prefix: String = "Bip01 "+side+" Finger0"
				var gun: Transform3D = game.combat.weapon.global_transform
				var thumb: Vector3 = gun*Vector3(.005 if side=="R" else -.025,-.045,.005)
				if game.combat.selected_weapon=="shotgun": thumb = gun*Vector3(.02 if side=="R" else -.025,-.06 if side=="R" else -.035,.02 if side=="R" else -.33+game.combat.shotgun.pump.position.z)
				_pose_chain(skeleton.find_bone(prefix),skeleton.find_bone(prefix+"1"),skeleton.find_bone(prefix+"2"),thumb,gun.basis.y)
	hand_world = (skeleton.global_transform*skeleton.get_bone_global_pose(skeleton.find_bone("Bip01 R Hand"))).origin

func _pose_arm(side: String, target: Vector3) -> void:
	var prefix: String = "Bip01 "+("R" if side=="Right" else "L")+" "
	var upper: int = skeleton.find_bone(prefix+"UpperArm")
	var lower: int = skeleton.find_bone(prefix+"Forearm")
	var hand: int = skeleton.find_bone(prefix+"Hand")
	_pose_chain(upper,lower,hand,target,Vector3.DOWN)
	var hero: CharacterBody3D = get_parent()
	if hero.aiming or hero.reload_phase>=0:
		var pose: Transform3D = skeleton.global_transform*skeleton.get_bone_global_pose(hand)
		var forward: Vector3 = Basis(Vector3.UP,hero.camera_rig.rotation.y)*Basis(Vector3.RIGHT,hero.camera_rig.rotation.x)*Vector3.FORWARD
		var up: Vector3 = Vector3.UP.slide(forward).normalized()
		pose.origin = target
		pose.basis = (Basis(forward,up,forward.cross(up))*Basis(Vector3.RIGHT,-PI/2 if side=="Right" else PI/2)).orthonormalized()
		skeleton.set_bone_global_pose_override(hand,skeleton.global_transform.affine_inverse()*pose,1,true)

func _pose_chain(upper: int,lower: int,hand: int,target: Vector3,bend_direction: Vector3) -> void:
	var a: Transform3D = skeleton.global_transform*skeleton.get_bone_global_pose(upper)
	var b: Transform3D = skeleton.global_transform*skeleton.get_bone_global_pose(lower)
	var c: Transform3D = skeleton.global_transform*skeleton.get_bone_global_pose(hand)
	var la: float = a.origin.distance_to(b.origin)
	var lb: float = b.origin.distance_to(c.origin)
	var axis: Vector3 = (target-a.origin).normalized()
	var distance: float = clampf(a.origin.distance_to(target),.03,la+lb-.005)
	var along: float = (la*la-lb*lb+distance*distance)/(2*distance)
	var bend: Vector3 = (bend_direction-axis*bend_direction.dot(axis)).normalized()
	if bend.length_squared()<.1: bend = Vector3.RIGHT
	var elbow: Vector3 = a.origin+axis*along+bend*sqrt(maxf(0,la*la-along*along))
	a.basis = Basis(Quaternion((b.origin-a.origin).normalized(),(elbow-a.origin).normalized()))*a.basis
	b.basis = Basis(Quaternion((c.origin-b.origin).normalized(),(a.origin+axis*distance-elbow).normalized()))*b.basis
	b.origin = elbow
	skeleton.set_bone_global_pose_override(upper,skeleton.global_transform.affine_inverse()*a,1,true)
	skeleton.set_bone_global_pose_override(lower,skeleton.global_transform.affine_inverse()*b,1,true)

func reset_pose() -> void:
	dying = false
	death_time = 0
	hurt_remaining = 0
	shot_remaining = 0
	knife_remaining = 0
	position.y = 0
	rotation.x = 0
	rotation.z = 0
	process_mode = Node.PROCESS_MODE_INHERIT
	crouch_offset = 0
	current_clip = ""
	playback_speed = 1
	legs_yaw = 0
	foot_plant.reset()
	player.stop()
	skeleton.clear_bones_global_pose_override()
	animate(0,0)

func _offset_bone(name: String, angles: Vector3) -> void:
	var index: int = skeleton.find_bone(name)
	if index>=0: skeleton.set_bone_pose_rotation(index,skeleton.get_bone_pose_rotation(index)*Quaternion.from_euler(angles))

func play_death() -> void:
	if dying: return
	dying = true
	death_time = 0
	process_mode = Node.PROCESS_MODE_ALWAYS
	skeleton.clear_bones_global_pose_override()
	death_bones.clear()
	for name: String in ["Bip01 Spine","Bip01 Spine1","Bip01 Head","Bip01 R Thigh","Bip01 L Thigh","Bip01 R Calf","Bip01 L Calf","Bip01 R UpperArm","Bip01 L UpperArm"]:
		var index: int = skeleton.find_bone(name)
		if index>=0: death_bones[name] = skeleton.get_bone_pose_rotation(index)
	show()

func _process(delta: float) -> void:
	if not dying or death_time>=1.4: return
	death_time = minf(1.4,death_time+delta)
	var collapse: float = smoothstep(0,.55,death_time)
	var fall: float = smoothstep(.3,1.3,death_time)
	rotation.x = -PI/2*fall
	rotation.z = .12*sin(fall*PI)
	position.y = .20*fall
	for name: String in death_bones:
		var angle := Vector3(.2,0,0)
		if "Thigh" in name: angle.x = -.55 if " R " in name else -.25
		if "Calf" in name: angle.x = .70
		if "UpperArm" in name: angle = Vector3(-.45,0,.3 if " R " in name else -.2)
		skeleton.set_bone_pose_rotation(skeleton.find_bone(name),death_bones[name]*Quaternion.from_euler(angle*collapse))

func _directional_pose(hero: CharacterBody3D, delta: float) -> void:
	var local_velocity: Vector3 = global_basis.inverse()*hero.velocity
	var angle: float = atan2(-local_velocity.x,-local_velocity.z)
	if absf(angle)>PI/2: angle = wrapf(angle+PI,-PI,PI)
	var wanted: float = clampf(angle,-1.05,1.05) if hero.aiming and local_velocity.length()>.2 else 0.0
	legs_yaw = lerp_angle(legs_yaw,wanted,1-exp(-10*delta))
	if absf(legs_yaw)<.005: return
	var pelvis: int = skeleton.find_bone("Bip01")
	var spine: int = skeleton.find_bone("Bip01 Spine")
	var a: Transform3D = skeleton.global_transform*skeleton.get_bone_global_pose(pelvis)
	var b: Transform3D = skeleton.global_transform*skeleton.get_bone_global_pose(spine)
	a.basis = Basis(Vector3.UP,legs_yaw)*a.basis
	skeleton.set_bone_global_pose_override(pelvis,skeleton.global_transform.affine_inverse()*a,1,true)
	skeleton.set_bone_global_pose_override(spine,skeleton.global_transform.affine_inverse()*b,1,true)
