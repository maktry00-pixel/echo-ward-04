extends "res://scripts/horror_creature.gd"
## Original nonhuman anatomy: low six-legged crawler, upright sac-bearing spitter.
var crawler: bool = false
var jaw: Node3D
var limbs: Array[Node3D] = []
var sacs: Array[Node3D] = []
var limb_segments: Array[Array] = []
var gait: float = 0
var hip_height: float = .72
var leg_feet: Array[Vector3] = []
var leg_elbows: Array[Vector3] = []
var death_feet: Array[Vector3] = []
var death_elbows: Array[Vector3] = []

func _ready() -> void:
	if guardian:
		super._ready()
		head.scale = Vector3(1.2, 1.2, 1.35)
		var horn := Builder.material("8b8979")
		for i: int in range(7):
			_strand(torso, Vector3(0, 1.1+i*.12, .18), Vector3(sin(i*2)*.22, 1.25+i*.15, .50), .045, horn)
		return
	var skin := ShaderMaterial.new()
	skin.shader = preload("res://shaders/organic_skin.gdshader")
	var shell: ShaderMaterial = skin.duplicate()
	shell.set_shader_parameter("tissue_color",Color("41413a"))
	shell.set_shader_parameter("wetness",.25)
	var bone := Builder.material("787365")
	bone.roughness = .95
	bone.metallic_specular = .18
	var black := Builder.material("080709")
	var wet: ShaderMaterial = skin.duplicate()
	wet.set_shader_parameter("tissue_color",Color("622e31"))
	wet.set_shader_parameter("wetness",.6)
	var glow := Builder.material("746c46", .08)
	torso = Node3D.new()
	add_child(torso)
	var height: float = .72 if crawler else 1.35
	hip_height = height
	_form(torso, Vector3(.38, .29 if crawler else .60, .47), Vector3(0, height, .12), skin, 14)
	for i: int in range(6):
		_form(torso, Vector3(.32, .055, .33), Vector3(0, height+.24-i*.045, .22+i*.09), shell, i)
		_strand(torso, Vector3(0, height+.25, i*.12), Vector3(sin(i*4)*.12, height+.40, i*.12), .025, bone)
	head = Node3D.new()
	head.position = Vector3(0, .82 if crawler else 2.02, -.37)
	torso.add_child(head)
	_form(head, Vector3(.23, .18, .28), Vector3.ZERO, shell, 3)
	_form(head, Vector3(.17, .12, .05), Vector3(0, -.055, -.25), black, 1)
	jaw = Node3D.new()
	jaw.position = Vector3(0, -.10, -.18)
	head.add_child(jaw)
	_form(jaw, Vector3(.20, .075, .20), Vector3(0, -.10, 0), wet, 8)
	for side: float in [-1, 1]:
		for i: int in range(5):
			_fang(jaw,Vector3(side*(.04+i*.028),-.06,-.15),Vector3(side*(.035+i*.025),.025+sin(i*2)*.02,-.19),.012,bone)
		_strand(head,Vector3(side*.16,.04,-.16),Vector3(side*.24,-.08,-.30),.035,skin)
		_fang(head,Vector3(side*.24,-.08,-.30),Vector3(side*.18,-.20,-.46),.028,bone)
		for i: int in range(3):
			_form(head, Vector3(.018, .012, .012), Vector3(side*(.08+i*.04), .06+i*.025, -.255+i*.035), glow, 0)
	for i: int in range(6):
		var side: float = -1 if i%2==0 else 1
		var row: int = i/2
		var limb := Node3D.new()
		limb.position = Vector3(side*.22, height, (row-1)*.32)
		torso.add_child(limb)
		limbs.append(limb)
		var elbow := Vector3(side*.52, .20 if crawler else -.35, -.12)
		var foot := Vector3(side*.64, -height+.05, -.28 if row==0 else .18)
		var upper: MeshInstance3D = _segment(limb,.065,skin)
		var lower: MeshInstance3D = _segment(limb,.035,shell)
		var joint: MeshInstance3D = Builder.sphere(limb,.085,elbow,shell)
		var claw: MeshInstance3D = _segment(limb,.023,bone)
		limb_segments.append([upper,lower,joint,claw])
		leg_feet.append(foot)
		leg_elbows.append(elbow)
		_place_leg(i,elbow,foot)
	if not crawler:
		for side: float in [-1,1]:
			var sac := Node3D.new()
			sac.position = Vector3(side*.27, 1.5, .23)
			torso.add_child(sac)
			sacs.append(sac)
			_form(sac, Vector3(.21,.35,.24), Vector3.ZERO, wet, 16)
			for j: int in range(3): _strand(sac,Vector3(side*.07,.22-j*.15,-.14),Vector3(side*.13,.12-j*.15,-.21),.015,glow)
	preload("res://scripts/creature_detail.gd").dress(self,skin)

func _process(delta: float) -> void:
	if dying:
		advance_death(delta)
		return
	if guardian:
		super._process(delta)
		return
	clock += delta
	var amount: float = clampf(movement_speed/3,0,1)
	gait += movement_speed*delta*TAU/(.9 if crawler else 1.1)
	var stride: float = gait
	torso.position.y = sin(stride*2)*.025*amount
	torso.rotation.z = sin(stride)*.035*amount
	head.rotation.z = .28 if stunned else sin(clock*1.3)*.10
	head.rotation.x = -.06
	jaw.rotation.x = -.22-absf(sin(clock*(7 if chasing else 2)))*.22
	for i: int in range(limbs.size()):
		var side: float = -1 if i%2==0 else 1
		var phase: float = fposmod(stride/TAU+(.5 if i in [1,2,5] else 0),1)
		var lift: float = sin(clampf((phase-.62)/.38,0,1)*PI)*.19*amount
		var travel: float = lerpf(.23,-.23,phase/.62) if phase<.62 else lerpf(-.23,.23,(phase-.62)/.38)
		var foot := Vector3(side*(.64+clampf(turn_amount,-1,1)*side*.04),-hip_height-.045+lift-torso.position.y,( -.28 if i<2 else .18)+travel*amount*(1-clampf(turn_amount*side,-.25,.25)))
		var elbow := Vector3(side*.52,.16 if crawler else -.35,foot.z*.35)
		if i<injured_legs.size() and injured_legs[i]:
			foot = Vector3(side*.43,-hip_height-.045,.40+travel*amount*.1)
			elbow = Vector3(side*.32,-.1,foot.z*.2)
			torso.rotation.z += side*.035
		if limb_recoils.has(i):
			foot.y += sin(float(limb_recoils[i])/.32*PI)*.15
		_place_leg(i,elbow,foot)
	for sac: Node3D in sacs: sac.scale = Vector3.ONE*(1+sin(clock*3)*.035)
	torso.rotation.x = 0
	if attack_mode=="swipe":
		for i: int in range(mini(2,limbs.size())):
			var side: float = -1 if i==0 else 1
			_place_leg(i,Vector3(side*.48,.12,-.3),leg_feet[i].lerp(Vector3(side*.20,-.05,-.78),attack_blend))
		jaw.rotation.x -= attack_blend*.45
	elif attack_mode=="spit":
		for sac: Node3D in sacs: sac.scale *= 1+attack_blend*.25
		head.rotation.x -= attack_blend*.28
	apply_reaction(delta)

func _segment(parent: Node3D,radius: float,material: Material) -> MeshInstance3D:
	var mesh := CylinderMesh.new()
	mesh.top_radius = radius*.7
	mesh.bottom_radius = radius
	mesh.height = 1
	mesh.radial_segments = 12
	var visual := MeshInstance3D.new()
	visual.mesh = mesh
	visual.material_override = material
	parent.add_child(visual)
	return visual

func _align(node: MeshInstance3D,a: Vector3,b: Vector3) -> void:
	node.position = (a+b)/2
	node.scale.y = a.distance_to(b)
	node.quaternion = Quaternion(Vector3.UP,(b-a).normalized())

func _place_leg(i: int,elbow: Vector3,foot: Vector3) -> void:
	leg_feet[i] = foot
	leg_elbows[i] = elbow
	_align(limb_segments[i][0],Vector3.ZERO,elbow)
	_align(limb_segments[i][1],elbow,foot)
	limb_segments[i][2].position = elbow
	_align(limb_segments[i][3],foot,foot+Vector3((.12 if i%2==0 else -.12),-.035,-.12))

func start_death(source: Vector3) -> void:
	super.start_death(source)
	death_feet.assign(leg_feet)
	death_elbows.assign(leg_elbows)

func advance_death(delta: float) -> void:
	if guardian:
		super.advance_death(delta)
		return
	death_time = minf(1.25,death_time+delta)
	var fall: float = smoothstep(.06,.8 if death_style=="blast" else 1.05,death_time)
	rotation.x = -.12*fall if crawler else -PI/2*fall
	rotation.z = death_side*(.22 if crawler else .12)*fall
	if death_style=="blast": rotation.z += death_side*.20*fall
	if death_style=="head": head.rotation.x = -.35*fall
	position.y = 0
	torso.position.y = -.42*fall if crawler else -.18*fall
	for i: int in range(limbs.size()):
		limbs[i].rotation = Vector3.ZERO
		var side: float = -1 if i%2==0 else 1
		_place_leg(i,death_elbows[i].lerp(Vector3(side*.38,-.01,.04),fall),death_feet[i].lerp(Vector3(side*(.63 if death_style=="blast" else .25),-.16,.20 if death_style=="blast" else .02),fall))
	jaw.rotation.x = -.55*fall
	settle_corpse()
	if death_time>=1.25: set_process(false)

func reset_animation() -> void:
	super.reset_animation()
	gait = 0
	for limb: Node3D in limbs: limb.rotation = Vector3.ZERO

func _fang(parent: Node3D,a: Vector3,b: Vector3,radius: float,material: Material) -> void:
	var tooth := CylinderMesh.new()
	tooth.top_radius = 0
	tooth.bottom_radius = radius
	tooth.height = a.distance_to(b)
	tooth.radial_segments = 9
	var visual := MeshInstance3D.new()
	visual.mesh = tooth
	visual.material_override = material
	visual.position = (a+b)/2
	visual.quaternion = Quaternion(Vector3.UP,(b-a).normalized())
	parent.add_child(visual)
