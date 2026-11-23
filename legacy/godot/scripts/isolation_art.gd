extends RefCounted
const B = preload("res://scripts/world_builder.gd")
const P = preload("res://scripts/imported_props.gd")
const M = preload("res://scripts/environment_materials.gd")
static func build(ch: Node3D) -> void:
	var steel := M.surface("metal","6b7774",2,.68)
	var worn := M.surface("metal","a3a791",2,.48)
	var dark := B.material("25322e")
	# A distinct wet-room floor and wall-side furniture break up the long corridors.
	B.box(ch,Vector3(13.6,.012,19.5),Vector3(37,.012,-72),M.pbr("terrazzo_tiles","829c9c",.7))
	for at: Vector3 in [Vector3(17.7,0,-72),Vector3(17.7,0,-75),Vector3(58,0,-68),Vector3(58,0,-79)]:
		var chair := Node3D.new()
		ch.add_child(chair)
		chair.position = at
		chair.rotation.y = -PI/2 if at.x<30 else PI/2
		P.visual(chair,"SchoolChair_01",Vector3(.53,.9,.57))
		P.body(chair,Vector3(.53,.84,.57),Vector3(0,.42,0))
	var post := Node3D.new()
	ch.add_child(post)
	post.position = Vector3(24,0,-54)
	P.visual(post,"sofa_03",Vector3(2.1,.9,.9))
	P.body(post,Vector3(2.1,.9,.9),Vector3(0,.45,0))
	for x: float in [32,42]:
		_pipe(ch,Vector3(x,.2,-63),Vector3(x,3.1,-63),.045,worn)
		_pipe(ch,Vector3(x,3.1,-63),Vector3(x,3.1,-80),.045,worn)
	# Furniture remains along the walls, clear of all authored interaction stances.
	for at: Vector3 in [Vector3(18,0,-69),Vector3(18,0,-78),Vector3(28,0,-96),Vector3(57,0,-97)]:
		var root := Node3D.new()
		ch.add_child(root)
		root.position = at
		P.visual(root,"painted_wooden_cabinet_02",Vector3(1.2,1.8,.58))
		P.body(root,Vector3(1.2,1.8,.58),Vector3(0,.9,0))
	for at: Vector3 in [Vector3(33,0,-69),Vector3(40,0,-81)]:
		var root := Node3D.new()
		ch.add_child(root)
		root.position = at
		P.visual(root,"metal_office_desk",Vector3(1.5,.78,.7))
		P.body(root,Vector3(1.5,.78,.7),Vector3(0,.39,0))
		P.detail(root,"desk_lamp_arm_01",.47,Vector3(.4,.79,0))
		B.box(root,Vector3(.28,.007,.34),Vector3(-.25,.80,.05),B.material("b9b19d"))
	for z: float in [-70,-77]:
		# Showers: shower heads, drains, exposed plumbing and stained dividers.
		B.box(ch,Vector3(.055,2.6,.055),Vector3(42.8,1.3,z),steel)
		B.box(ch,Vector3(.6,.045,.05),Vector3(42.5,2.55,z),steel)
		B.sphere(ch,.1,Vector3(42.2,2.51,z),steel).scale.y = .25
		B.box(ch,Vector3(.45,.012,.45),Vector3(41,.012,z),steel)
		B.box(ch,Vector3(2,1.75,.12),Vector3(42,.875,z-2.2),M.pbr("blue_plaster_weathered","728583",.5))
	for x: float in [19,27,45,57]:
		B.box(ch,Vector3(.22,.22,11),Vector3(x,3.1,-76),steel)
	for z: float in [-87,-91]:
		for x: float in [17.5,28.5]:
			B.box(ch,Vector3(.065,2.8,.065),Vector3(x,1.4,z),steel)
	# The same pursuer is visible through this cell window before release.
	B.box(ch,Vector3(5,.14,.12),Vector3(23,2.6,-84),steel)
	for x: float in [20.8,21.9,24.1,25.2]: B.box(ch,Vector3(.065,2.3,.08),Vector3(x,1.3,-84),steel)
	var bed: Node3D = preload("res://scripts/hide_bed.gd").new()
	bed.position = Vector3(26,0,-72)
	bed.metal = steel
	ch.add_child(bed)
	# Generator: physical base plus a rotating guarded flywheel.
	B.box(ch,Vector3(4.3,1.2,1.35),Vector3(37,.6,-105),steel,true)
	ch.flywheel = Node3D.new()
	ch.add_child(ch.flywheel)
	ch.flywheel.position = Vector3(37,1.45,-104.3)
	for i: int in range(6):
		var spoke := B.box(ch.flywheel,Vector3(.07,.75,.06),Vector3.ZERO,steel)
		spoke.rotation.z = i*PI/3
	B.sphere(ch.flywheel,.14,Vector3.ZERO,steel)
	# Rounded engine, cooling fins, gauges and cabling make the generator readable.
	_pipe(ch,Vector3(35.7,1.36,-105),Vector3(38.3,1.36,-105),.30,worn)
	for i: int in range(12):
		B.box(ch,Vector3(.055,.58,.09),Vector3(35.8+i*.22,.65,-104.28),dark)
	for x: float in [35.5,38.5]:
		_pipe(ch,Vector3(x,1.65,-105),Vector3(x,2.35,-105),.09,steel)
		_pipe(ch,Vector3(x,2.35,-105),Vector3(x+(.45 if x<37 else -.45),2.35,-105),.09,steel)
		B.box(ch,Vector3(.36,.22,.07),Vector3(x,1.05,-104.25),B.material("b5b292"))
		B.box(ch,Vector3(.025,.15,.018),Vector3(x,1.05,-104.20),B.material("474b3e")).rotation.z = -.45
	for i: int in range(5):
		B.box(ch,Vector3(.38,.05,.10),Vector3(35.5+i*.72,.13,-104.25),B.material("b19a58"))
	_pipe(ch,Vector3(35,.25,-104),Vector3(33,.25,-104),.045,dark)
	_pipe(ch,Vector3(33,.25,-104),Vector3(33,.25,-106.5),.045,dark)
	var work_light := OmniLight3D.new()
	work_light.position = Vector3(37,1.9,-102.6)
	work_light.omni_range = 5
	work_light.light_color = Color("d6ba8b")
	work_light.light_energy = .85
	ch.add_child(work_light)
	for x: float in [17,59]:
		B.box(ch,Vector3(.08,2.4,.15),Vector3(x,1.2,-100),steel)
		B.box(ch,Vector3(5,.12,.12),Vector3(x+(-2 if x>40 else 2),2.35,-100),steel)
	for entry: Array in [["СТАРЫЙ ИЗОЛЯТОР / 17",Vector3(52,2.8,-51)], ["ЗАПАД / СВЕТ / ДЛИННЫЙ ОБХОД",Vector3(23,2.8,-61.7)],["ВОСТОК / КАМЕРЫ / КОРОТКИЙ ПУТЬ",Vector3(52,2.8,-61.7)],["ДУШЕВЫЕ / БРАСЛЕТЫ",Vector3(37,2.8,-68)],["ГЕНЕРАТОР / ДВА СБРОСА ДАВЛЕНИЯ",Vector3(37,2.8,-102.8)]]:
		ch.facility._sign(entry[0],entry[1],0,19)
	# The warning is placed before the avoidable live-water strip.
	ch.facility._sign("ОГОЛЁННЫЙ КАБЕЛЬ / ОБОЙТИ СЛЕВА",Vector3(52,2.6,-79),0,17)
	var wet := B.material("354a4e")
	wet.roughness = .12
	B.box(ch,Vector3(8,.008,3),Vector3(52,.008,-81.5),wet)
	# Original physical trail guides the eye from the showers towards the cell.
	for i: int in range(9):
		preload("res://scripts/surface_mark.gd").create(ch,Vector3(28+i*.55,.025,-75+sin(i)*.35),Vector3.UP,.06,Color("45312b"),700+i)
	for z: float in [-68,-78,-89]:
		var frame := B.box(ch,Vector3(.035,.55,.42),Vector3(16.3,1.65,z),dark)
		B.box(frame,Vector3(.009,.46,.34),Vector3(.023,0,0),B.material("b5b1a0"))
		B.box(frame,Vector3(.012,.025,.27),Vector3(.03,.12,0),B.material("626e68"))
static func _pipe(parent: Node3D,a: Vector3,b: Vector3,radius: float,mat: Material) -> void:
	var mesh := CylinderMesh.new()
	mesh.top_radius = radius
	mesh.bottom_radius = radius
	mesh.height = a.distance_to(b)
	mesh.radial_segments = 16
	var visual := MeshInstance3D.new()
	visual.mesh = mesh
	visual.material_override = mat
	visual.position = (a+b)/2
	visual.basis = Basis(Quaternion(Vector3.UP,(b-a).normalized()))
	parent.add_child(visual)
