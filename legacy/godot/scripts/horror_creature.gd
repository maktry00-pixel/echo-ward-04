extends Node3D
## Original procedural creature, with locally animated joints and a worn mask.

const Builder = preload("res://scripts/world_builder.gd")
const Materials = preload("res://scripts/environment_materials.gd")
var torso: Node3D
var head: Node3D
var arms: Array[Node3D] = []
var legs: Array[Node3D] = []
var movement_speed: float = 0.0
var clock: float = 0.0
var chasing: bool = false
var stunned: bool = false
var guardian: bool = false
var blindfold: bool = false
var attack_progress: float = 0
var attack_blend: float = 0
var impact_remaining: float = 0
var impact_axis: Vector3 = Vector3.BACK
var dying: bool = false
var death_time: float = 0
var death_side: float = 1
var corpse_meshes: Array[Node] = []
var injured_legs: Array[bool] = []
var last_hit_kind: String = ""
var limb_recoils: Dictionary = {}
var turn_amount: float = 0
var attack_mode: String = "swipe"
var death_style: String = "body"
static var sculpt_meshes: Dictionary = {}

func _ready() -> void:
	var flesh := Materials.surface("concrete", "747666", 2.4, 0.94)
	var bone := Materials.surface("concrete", "c0b9a0", 3.0, 0.9)
	var cloth := Materials.surface("concrete", "242b26", 2.0, 0.98)
	var void_mat := Builder.material("090c0a")
	flesh.albedo_color = Color("737d76")
	bone.albedo_color = Color("b1b8ac")
	cloth.albedo_color = Color("1c2320")
	if guardian:
		cloth.albedo_color = Color("302123")
		bone.albedo_color = Color("baa88c")
	for material: StandardMaterial3D in [flesh, bone, cloth]:
		material.roughness_texture = null
		material.roughness = 0.97
		material.metallic_specular = 0.12
	torso = Node3D.new()
	add_child(torso)
	_form(torso, Vector3(0.30, 0.55, 0.19), Vector3(0, 1.38, 0.05), cloth, 3)
	_form(torso, Vector3(0.23, 0.28, 0.16), Vector3(0, 1.78, -0.04), flesh, 7)
	_form(torso, Vector3(0.075, 0.22, 0.085), Vector3(0, 1.93, -0.16), flesh, 9)
	for index: int in range(6):
		var y: float = 1.66 - index * 0.11
		for sign: float in [-1.0, 1.0]:
			_strand(torso, Vector3(sign * 0.23, y + 0.035, -0.07), Vector3(sign * 0.18, y + 0.01, -0.16), 0.012, bone)
			_strand(torso, Vector3(sign * 0.18, y + 0.01, -0.16), Vector3(sign * 0.04, y - 0.04, -0.17), 0.012, bone)
	_ragged_hem(torso, cloth)
	head = Node3D.new()
	head.position = Vector3(0, 2.12, -0.20)
	torso.add_child(head)
	_form(head, Vector3(0.18, 0.29, 0.17), Vector3.ZERO, bone, 5)
	_form(head, Vector3(0.105, 0.125, 0.045), Vector3(0, -0.13, -0.16), void_mat, 1)
	for sign: float in [-1.0, 1.0]:
		_form(head, Vector3(0.066, 0.064, 0.048), Vector3(sign * 0.086, 0.045, -0.147), void_mat, 2)
		var eye := Builder.material("bd7963" if guardian else "b5b492", 0.35)
		if not blindfold: _form(head, Vector3(0.017, 0.009, 0.009), Vector3(sign * 0.086, 0.042, -0.195), eye, 0)
		_strand(head, Vector3(sign * 0.12, 0.12, -0.12), Vector3(sign * 0.15, -0.07, -0.13), 0.012, void_mat)
	_form(head, Vector3(0.035, 0.09, 0.04), Vector3(0, 0, -0.17), bone, 2)
	if blindfold:
		var bandage := Materials.surface("concrete","827764",3,.98)
		bandage.roughness_texture = null
		bandage.metallic_specular = .1
		_form(head,Vector3(.20,.075,.045),Vector3(0,.045,-.205),bandage,17)
		for sign: float in [-1.0,1.0]:
			_strand(head,Vector3(sign*.18,.05,-.16),Vector3(sign*.19,.04,.10),.024,bandage)
	for row: int in range(2):
		for index: int in range(5):
			var tooth := CylinderMesh.new()
			tooth.top_radius = 0.001
			tooth.bottom_radius = 0.012
			tooth.height = 0.047 + 0.008 * sin(index)
			tooth.radial_segments = 5
			var visual := MeshInstance3D.new()
			visual.mesh = tooth
			visual.material_override = bone
			visual.position = Vector3((index - 2) * 0.032, -0.07 if row == 0 else -0.20, -0.20)
			visual.rotation.z = PI if row == 0 else 0.0
			head.add_child(visual)
	for sign: float in [-1.0, 1.0]:
		var arm := Node3D.new()
		arm.position = Vector3(sign * 0.32, 1.77, 0)
		torso.add_child(arm)
		arms.append(arm)
		_form(arm, Vector3(0.08, 0.34, 0.07), Vector3(sign * 0.09, -0.25, 0.02), flesh, 8)
		_form(arm, Vector3(0.055, 0.34, 0.06), Vector3(sign * 0.12, -0.80, -0.02), flesh, 4)
		_form(arm, Vector3(0.09, 0.13, 0.045), Vector3(sign * 0.12, -1.11, -0.05), flesh, 6)
		for finger: int in range(4):
			var x: float = sign * 0.12 + (finger - 1.5) * 0.042
			var tip_y: float = -1.44 + absf(finger - 1.5) * 0.04
			_strand(arm, Vector3(x, -1.19, -0.055), Vector3(x, tip_y, -0.10), 0.014, bone)
			_strand(arm, Vector3(x, tip_y, -0.10), Vector3(x, tip_y - 0.04, -0.17), 0.008, bone)
		var leg := Node3D.new()
		leg.position = Vector3(sign * 0.14, 0.94, 0.04)
		add_child(leg)
		legs.append(leg)
		_form(leg, Vector3(0.12, 0.28, 0.12), Vector3(0, -0.20, 0), cloth, 4)
		_form(leg, Vector3(0.075, 0.27, 0.075), Vector3(0, -0.66, -0.03), flesh, 7)
		_form(leg, Vector3(0.10, 0.065, 0.18), Vector3(0, -0.87, -0.10), flesh, 9)

func animate(speed: float) -> void:
	movement_speed = speed

func _ragged_hem(parent: Node3D, material: Material) -> void:
	var surface := SurfaceTool.new()
	surface.begin(Mesh.PRIMITIVE_TRIANGLES)
	surface.set_smooth_group(0)
	for index: int in range(32):
		var points: Array[Vector3] = []
		for pair: Vector2i in [Vector2i(index, 0), Vector2i(index + 1, 0), Vector2i(index + 1, 1), Vector2i(index, 1)]:
			var angle: float = TAU * pair.x / 32.0
			var y: float = 1.13 if pair.y == 0 else 0.63 + sin(angle * 7) * 0.065
			var radius: float = 0.25 if pair.y == 0 else 0.29
			points.append(Vector3(cos(angle) * radius, y, sin(angle) * radius * 0.64 + 0.05))
		for vertex: int in [0, 1, 2, 0, 2, 3]:
			surface.add_vertex(points[vertex])
	surface.index()
	surface.generate_normals()
	var visual := MeshInstance3D.new()
	visual.mesh = surface.commit()
	visual.material_override = material
	parent.add_child(visual)

func _process(delta: float) -> void:
	if dying:
		advance_death(delta)
		return
	if get_parent() is CharacterBody3D: position.y = -.08
	clock += delta
	var stride: float = clock * (4.8 if chasing else 3.2)
	var moving: float = clampf(movement_speed / 3.0, 0, 1)
	torso.position.y = absf(sin(stride)) * 0.025 * moving
	torso.scale.x = (1.2 if guardian else 1.0) * (1.0 + sin(clock * 1.7) * 0.022)
	head.rotation.x = -0.10 + sin(clock * 1.9) * 0.04
	head.rotation.y = sin(clock * 0.9) * (0.07 if chasing else 0.24)
	head.rotation.z = 0.3 if stunned else sin(clock * 1.3) * 0.05
	torso.rotation.x = 0
	torso.rotation.z = 0
	for index: int in range(2):
		var sign: float = -1.0 if index == 0 else 1.0
		arms[index].rotation.x = sin(stride + index * PI + 0.5) * moving * 0.19
		arms[index].rotation.z = sign * (0.05 + sin(clock * 2.3 + index) * 0.025)
		legs[index].rotation.x = sin(stride + index * PI) * moving * 0.36
		if index<injured_legs.size() and injured_legs[index]:
			legs[index].rotation.x = .32+sin(stride)*moving*.07
			torso.rotation.z = sign*.1
	apply_reaction(delta)

func react_to_hit(source: Vector3, headshot: bool) -> void:
	last_hit_kind = "head" if headshot else "body"
	impact_axis = global_basis.inverse()*(global_position-source).normalized()
	impact_remaining = .45 if headshot else .28

func apply_reaction(delta: float) -> void:
	for index: int in limb_recoils:
		limb_recoils[index] = maxf(0,float(limb_recoils[index])-delta)
		if index<legs.size(): legs[index].rotation.x += sin(float(limb_recoils[index])/.32*PI)*.3
	impact_remaining = maxf(0,impact_remaining-delta)
	attack_blend = move_toward(attack_blend,attack_progress,delta*5)
	var hit: float = sin(clampf(impact_remaining/.45,0,1)*PI)
	torso.rotation.x = -attack_blend*.26+hit*impact_axis.z*.25
	torso.rotation.z += hit*impact_axis.x*-.30
	head.rotation.x += hit*(.42 if last_hit_kind=="head" else .14)-attack_blend*.18
	if last_hit_kind=="head": head.rotation.z += hit*.20*signf(impact_axis.x+.001)

func start_death(source: Vector3) -> void:
	dying = true
	death_time = 0
	death_side = -1 if (global_basis.inverse()*(global_position-source)).x<0 else 1
	corpse_meshes = find_children("*","MeshInstance3D",true,false)
	set_process(true)

func advance_death(delta: float) -> void:
	death_time = minf(1.25,death_time+delta)
	var fall: float = smoothstep(.15,1.1,death_time)
	rotation.x = -PI/2*fall
	rotation.z = death_side*sin(fall*PI)*.28
	position.y = .16*fall
	torso.rotation.x = sin(fall*PI)*.45
	head.rotation.z = death_side*.32*fall
	if death_style=="head": head.rotation.x = -.45*fall
	if death_style=="blast": rotation.z += death_side*.25*fall
	for i: int in range(legs.size()): legs[i].rotation.x = (-.7 if i==0 else .42)*fall
	for i: int in range(arms.size()):
		arms[i].rotation.x = -.5*fall
		arms[i].rotation.z = (.40 if i==0 else -.28)*fall
	settle_corpse()
	if death_time>=1.25: set_process(false)

func settle_corpse() -> void:
	var lowest: float = INF
	for mesh: MeshInstance3D in corpse_meshes:
		var bounds: AABB = mesh.global_transform*mesh.get_aabb()
		lowest = minf(lowest,bounds.position.y)
	var floor_y: float = get_parent().global_position.y-.06
	if is_finite(lowest): global_position.y += floor_y-lowest

func restore_death(source: Vector3) -> void:
	start_death(source)
	advance_death(1.25)

func reset_animation() -> void:
	death_style = "body"
	dying = false
	death_time = 0
	impact_remaining = 0
	last_hit_kind = ""
	limb_recoils.clear()
	attack_blend = 0
	attack_progress = 0
	rotation.x = 0
	rotation.z = 0
	position.y = 0
	torso.rotation = Vector3.ZERO
	for node: Node3D in arms+legs: node.rotation = Vector3.ZERO

func _strand(parent: Node3D, a: Vector3, b: Vector3, radius: float, mat: Material) -> void:
	var mesh := CylinderMesh.new()
	mesh.top_radius = radius * 0.7
	mesh.bottom_radius = radius
	mesh.height = a.distance_to(b)
	mesh.radial_segments = 7
	var node := MeshInstance3D.new()
	node.mesh = mesh
	node.material_override = mat
	node.position = (a + b) / 2.0
	node.quaternion = Quaternion(Vector3.UP, (b - a).normalized())
	parent.add_child(node)

func _form(parent: Node3D, radius: Vector3, at: Vector3, mat: Material, seed: int) -> void:
	var cache_key: String = str(radius)+"/"+str(seed)
	if sculpt_meshes.has(cache_key):
		var cached := MeshInstance3D.new()
		cached.mesh = sculpt_meshes[cache_key]
		cached.material_override = mat
		cached.position = at
		parent.add_child(cached)
		return
	var surface := SurfaceTool.new()
	surface.begin(Mesh.PRIMITIVE_TRIANGLES)
	surface.set_smooth_group(0)
	for ring: int in range(18):
		for segment: int in range(28):
			var p: Array[Vector3] = []
			for pair: Vector2i in [Vector2i(ring, segment), Vector2i(ring + 1, segment), Vector2i(ring + 1, segment + 1), Vector2i(ring, segment + 1)]:
				var phi: float = PI * pair.x / 18.0
				var theta: float = TAU * pair.y / 28.0
				var normal := Vector3(sin(phi) * cos(theta), cos(phi), sin(phi) * sin(theta))
				var rough: float = 1.0 + 0.035 * sin(normal.x * 13 + seed) * cos(normal.y * 17 + normal.z * 11)
				p.append(normal * radius * rough)
			for index: int in [0, 2, 1, 0, 3, 2]:
				surface.add_vertex(p[index])
	surface.index()
	surface.generate_normals()
	var node := MeshInstance3D.new()
	node.mesh = surface.commit()
	sculpt_meshes[cache_key] = node.mesh
	node.material_override = mat
	node.position = at
	parent.add_child(node)

func react_to_limb(index: int) -> void:
	last_hit_kind = "leg"
	limb_recoils[index] = .32
