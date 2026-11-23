extends RefCounted
## Room-specific dressing with colliders kept away from puzzle and door stances.
const B = preload("res://scripts/world_builder.gd")
const P = preload("res://scripts/imported_props.gd")
const M = preload("res://scripts/environment_materials.gd")
static func build(chapter: Node3D) -> void:
	var f: Node3D = chapter.facility
	var steel := M.surface("metal","8a9792",1.3,.42)
	var dark := B.material("293b3d")
	var cloth := M.pbr("stretch_poplin","acb2ad",3)
	cloth.albedo_texture = null
	var paper := B.material("c2c2ae")
	# Acoustic grid, vents and a continuous wall protection band give human scale.
	for x: float in [18,22,26,32,36,40,46,50,54,58]:
		B.box(chapter,Vector3(.045,.025,24),Vector3(x,3.57,-37),dark)
	for z: float in [-26,-30,-34,-38,-42,-46,-49]:
		B.box(chapter,Vector3(43,.025,.045),Vector3(38,3.57,z),dark)
	for x: float in [19,33,47]:
		B.box(chapter,Vector3(1.25,.02,.8),Vector3(x,3.54,-29),steel)
		for i: int in range(8): B.box(chapter,Vector3(1.1,.025,.023),Vector3(x,3.51,-29.3+i*.085),dark)
	# Scanned desk banks and lamps are pushed against the west wall.
	for z: float in [-28,-38.7]:
		var at := Vector3(17.3,0,z)
		P.visual(chapter,"metal_office_desk",Vector3(1.5,.82,1.1),at)
		P.body(chapter,Vector3(1.5,.82,1.1),at+Vector3(0,.41,0),"metal")
		P.detail(chapter,"desk_lamp_arm_01",.6,at+Vector3(0,.85,-.25),Vector3(0,.5,0))
		P.detail(chapter,"medical_box",.3,at+Vector3(.3,.85,.2))
		for i: int in range(4): B.box(chapter,Vector3(.2,.005,.28),at+Vector3(-.2+i*.025,.85+i*.009,.12),paper).rotation.y = .3
		_light(chapter,at+Vector3(0,1.3,-.2),Color("ddbd8b"),.52,3.7)
	# X-ray head housing has separate shutters, lens ring, rail and cable.
	for x: float in [21.8,22.5,23.2]: B.box(chapter,Vector3(.025,.045,.52),Vector3(x,2.79,-36),steel)
	_cylinder(chapter,.29,.055,Vector3(23.6,1.89,-36),steel)
	_cylinder(chapter,.21,.06,Vector3(23.6,1.85,-36),dark)
	for z: float in [-36.2,-35.8]: B.box(chapter,Vector3(.8,.055,.035),Vector3(23.6,1.82,z),steel)
	for i: int in range(12):
		var at := Vector3(21.35+sin(i*.25)*.08,2.65-i*.12,-36.3)
		B.sphere(chapter,.035,at,dark)
	# Observation: monitor stand, bed-side chart and obscuring but traversable curtains.
	var monitor := Vector3(41.6,1.5,-37.8)
	P.body(chapter,Vector3(.7,1.0,.7),Vector3(41.6,.5,-37.8),"metal")
	B.box(chapter,Vector3(.08,1,.08),Vector3(41.6,1.35,-37.8),steel)
	B.box(chapter,Vector3(.72,.52,.12),monitor,dark)
	B.box(chapter,Vector3(.62,.42,.018),monitor+Vector3(0,0,.075),B.material("365c52",.45))
	for i: int in range(12):
		var y: float = .035*sin(i*.9)+(.13 if i%6==3 else 0)
		B.box(chapter,Vector3(.045,.012,.01),monitor+Vector3(-.27+i*.045,y,.09),B.material("98c4a0",.4))
	f._sign("04 / 39.2°C",monitor+Vector3(0,-.15,.09),0,13).pixel_size = .0025
	for at: Vector3 in [Vector3(36.5,0,-35),Vector3(50.5,0,-40)]:
		f._pipe(at+Vector3(-1,2.8,0),at+Vector3(1,2.8,0),.025,steel)
		for i: int in range(18):
			var strip: Node3D = B.box(chapter,Vector3(.09,2,.035),at+Vector3(-.86+i*.075,1.75,.045*sin(i*1.5)),cloth)
			strip.rotation.y = sin(i*1.5)*.32
	# Treatment worktop, basin, instrument tray and a physically fixed wheelchair.
	var at := Vector3(58.7,0,-35.8)
	P.face(P.visual(chapter,"metal_office_desk",Vector3(1.1,.85,2.7),at),at,0)
	P.body(chapter,Vector3(1.1,.85,2.7),at+Vector3(0,.425,0),"metal")
	for z: float in [-36.6,-35]:
		_cylinder(chapter,.21,.07,Vector3(58.7,.91,z),steel)
		B.box(chapter,Vector3(.035,.24,.035),Vector3(59,.99,z),steel)
		B.box(chapter,Vector3(.22,.035,.035),Vector3(58.92,1.12,z),steel)
	P.detail(chapter,"fish_knife",.24,Vector3(58.4,.89,-35.7),Vector3(0,0,PI/2))
	P.detail(chapter,"medical_tape",.14,Vector3(58.4,.89,-36))
	P.detail(chapter,"wheelchair_01",1.15,Vector3(46,.02,-48),Vector3(0,.6,0))
	P.body(chapter,Vector3(.7,1.1,.9),Vector3(46,.55,-48),"metal")
	_light(chapter,Vector3(56.7,2.0,-36),Color("d4bea0"),.38,5)
	_light(chapter,Vector3(22,2.2,-34),Color("8aa9bd"),.28,6)
	# Readable wall signs have backing plates and avoid oversized floating type.
	var warning := Vector3(59.65,2,-32.8)
	B.box(chapter,Vector3(.08,.65,2.6),warning, B.material("c4b985"))
	var sign: Label3D = f._sign("ОПАСНО: РАЗРЯД\nОБХОДИТЕ МОКРЫЙ ПОЛ",warning-Vector3(.055,0,0),-PI/2,20)
	sign.pixel_size = .0034
	sign.outline_size = 0
	sign.modulate = Color("263b3b")
	# Scuffs and a short actual blood trail remain flush with floor/wall surfaces.
	var mark = preload("res://scripts/surface_mark.gd")
	for i: int in range(12):
		mark.create(chapter,Vector3(39+sin(i)*.15,.006,-35-i*.4),Vector3.UP,.035+(i%3)*.012,Color("3e2727"),160+i)
	for i: int in range(5): mark.create(chapter,Vector3(43.77,1.15+i*.07,-39.9+i*.04),Vector3.LEFT,.035,Color("50342f"),185+i)

static func _light(parent: Node3D,at: Vector3,color: Color,energy: float,radius: float) -> void:
	var light := OmniLight3D.new()
	light.position = at
	light.light_color = color
	light.light_energy = energy
	light.omni_range = radius
	light.distance_fade_enabled = true
	light.distance_fade_begin = 15
	light.distance_fade_length = 5
	parent.add_child(light)

static func _cylinder(parent: Node3D,radius: float,height: float,at: Vector3,mat: Material) -> void:
	var node := MeshInstance3D.new()
	var mesh := CylinderMesh.new()
	mesh.top_radius = radius
	mesh.bottom_radius = radius
	mesh.height = height
	mesh.radial_segments = 24
	node.mesh = mesh
	node.material_override = mat
	node.position = at
	parent.add_child(node)
