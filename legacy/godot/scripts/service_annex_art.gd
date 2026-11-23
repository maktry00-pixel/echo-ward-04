extends RefCounted
const B = preload("res://scripts/world_builder.gd")
const P = preload("res://scripts/imported_props.gd")
const M = preload("res://scripts/environment_materials.gd")
static func furniture(ch: Node3D,id: String,size: Vector3,at: Vector3,yaw: float = 0) -> Node3D:
	var root := Node3D.new(); ch.add_child(root); root.position = at; root.rotation.y = yaw
	P.visual(root,id,size); P.body(root,size,Vector3(0,size.y/2,0))
	return root
static func build(ch: Node3D) -> void:
	var steel := M.surface("metal","5e716d",1.5,.65)
	var dark := B.material("253834")
	var warning := B.material("bc784b",.12)
	var paper := B.material("b3aa91")
	for row: Array in [[-118,"ЮЖНЫЙ ПОСТ / СКЛАД →"],[-139,"← МАСТЕРСКАЯ / НАСОСНАЯ →"],[-162,"← КОТЕЛЬНАЯ / ХОЛОДНЫЙ СКЛАД →"],[-183,"← ДИСПЕТЧЕРСКАЯ / АНТЕННА →"]]:
		ch.facility._sign(row[1],Vector3(50,2.9,row[0]+3),0,20)
		for x: float in [25,41,63,73,88,99]:
			var light := OmniLight3D.new(); ch.add_child(light)
			light.position = Vector3(x,3.1,row[0]); light.omni_range = 10
			light.light_color = Color("b7b89a") if x<46 else Color("92b5b5")
			light.light_energy = .48
			B.box(ch,Vector3(.70,.08,.24),light.position+Vector3(0,.30,0),B.material("aab5a3",.4))
		var hall_light := SpotLight3D.new(); ch.add_child(hall_light)
		hall_light.position = Vector3(50,3.3,row[0]); hall_light.rotation.x = -PI/2
		hall_light.spot_range = 11; hall_light.spot_angle = 64; hall_light.light_energy = 1.05
		hall_light.light_color = Color("aac8c1"); hall_light.shadow_enabled = true
	# Real furniture collision stays at the perimeter, leaving door stances and corridor clear.
	furniture(ch,"metal_office_desk",Vector3(2.2,.8,.9),Vector3(28,0,-110.5))
	furniture(ch,"sofa_03",Vector3(2.3,.9,.9),Vector3(18.3,0,-123),PI/2)
	furniture(ch,"SchoolChair_01",Vector3(.58,.9,.6),Vector3(29,0,-112.5),PI)
	furniture(ch,"painted_wooden_cabinet_02",Vector3(1.2,1.8,.6),Vector3(44,0,-123),-PI/2)
	for at: Vector3 in [Vector3(57,0,-111),Vector3(74,0,-125),Vector3(82,0,-111),Vector3(100,0,-112),Vector3(18,0,-131),Vector3(44,0,-131),Vector3(99,0,-151),Vector3(81,0,-172),Vector3(102,0,-171)]:
		furniture(ch,"steel_frame_shelves_01",Vector3(1.5,2,.6),at)
		for i: int in range(4):
			B.box(ch,Vector3(.45,.22,.36),at+Vector3((i%2-.5)*.6,.5+floori(i/2)*.55,0),paper)
	for at: Vector3 in [Vector3(28,0,-131),Vector3(40,0,-131)]:
		var desk: Node3D = furniture(ch,"metal_office_desk",Vector3(2,.8,.85),at)
		P.detail(desk,"desk_lamp_arm_01",.55,Vector3(.65,.82,0))
		P.detail(desk,"circuit_board",.3,Vector3(-.5,.82,0))
	# Pump machinery: feet, motor, flanges and a visible turning impeller.
	B.box(ch,Vector3(4,.8,2),Vector3(65,.4,-139),steel,true)
	_pipe(ch,Vector3(63.4,1.1,-139),Vector3(66.5,1.1,-139),.48,steel)
	for x: float in [63.5,66.5]:
		_pipe(ch,Vector3(x,1.1,-139),Vector3(x,2.7,-139),.12,steel)
		_pipe(ch,Vector3(x,2.7,-139),Vector3(x,2.7,-147),.12,steel)
	ch.pump_wheel = Node3D.new(); ch.add_child(ch.pump_wheel); ch.pump_wheel.position = Vector3(65,1.2,-137.9)
	for i: int in range(8): B.box(ch.pump_wheel,Vector3(.035,.8,.05),Vector3.ZERO,warning).rotation.z = i*PI/4
	B.sphere(ch.pump_wheel,.16,Vector3.ZERO,steel)
	B.box(ch,Vector3(23,.012,20),Vector3(66,.012,-139),M.pbr("terrazzo_tiles","667e7e",.6))
	# Laundry has heavy washers, open drains and a clear route to the return valve.
	for x: float in [82,89,99]:
		B.box(ch,Vector3(2,1.5,1.8),Vector3(x,.75,-131),M.surface("metal","92988b",2,.6),true)
		B.sphere(ch,.58,Vector3(x,.8,-129.98),dark).scale.z = .15
		B.sphere(ch,.42,Vector3(x,.8,-129.88),steel).scale.z = .12
		B.box(ch,Vector3(.6,.012,.6),Vector3(x,.015,-134),steel)
	_pipe(ch,Vector3(101,.4,-130),Vector3(101,.4,-147),.09,steel)
	# Two boilers stand away from the three hand controls and their safe working positions.
	for at: Vector3 in [Vector3(30,0,-157),Vector3(32,0,-168)]:
		# The plinth fits inside the cylinder's navigation clearance; no low lip catches feet.
		B.box(ch,Vector3(3,.4,3),at+Vector3(0,.2,0),steel,true)
		_pipe(ch,at+Vector3(0,.6,0),at+Vector3(0,2.9,0),1.3,steel)
		P.body(ch,Vector3(2.6,2.6,2.6),at+Vector3(0,1.5,0),"metal")
		for z: float in [-1,1]: _pipe(ch,at+Vector3(-1.2,2.8,z),at+Vector3(1.2,2.8,z),.12,warning)
		_pipe(ch,at+Vector3(0,3,0),Vector3(48,3.1,at.z),.14,steel)
	for z: float in [-131,-141,-158,-170,-182]:
		_pipe(ch,Vector3(47,3.1,z),Vector3(53,3.1,z),.12,steel)
	ch.pressure_light = OmniLight3D.new(); ch.add_child(ch.pressure_light)
	ch.pressure_light.position = Vector3(50,2.4,-149); ch.pressure_light.omni_range = 5
	ch.pressure_light.light_color = Color("d86e3b"); ch.pressure_light.light_energy = .9
	ch.facility._sign("ГОРЯЧИЙ ПАР / ВОССТАНОВИТЬ НАСОС",Vector3(50,2.9,-146),0,18)
	# Cold store and archive: identifiable cupboards, file bundles and ward stretchers.
	for x: float in [57,65,73]:
		furniture(ch,"painted_wooden_cabinet_02",Vector3(1.7,2,.75),Vector3(x,0,-151))
	for x: float in [82,88,101]:
		var rack: Node3D = furniture(ch,"steel_frame_shelves_01",Vector3(1.8,2.1,.65),Vector3(x,0,-156))
		for i: int in range(8): B.box(rack,Vector3(.12,.30,.35),Vector3(-.7+i*.19,1.2,0),B.material("7b7d61" if i%2 else "9a8d70"))
	for x: float in [84,91,101]:
		B.box(ch,Vector3(2.2,.15,.6),Vector3(x,1.1,-172),steel,true)
		for i: int in range(4): B.box(ch,Vector3(.35,.15,.4),Vector3(x-.7+i*.45,1.25,-172),paper)
	# Radio control and reserve antenna equipment occupy separate rooms.
	var control: Node3D = furniture(ch,"metal_office_desk",Vector3(3,.8,1),Vector3(32,0,-190))
	P.detail(control,"portable_cassette_player",.5,Vector3(.2,.83,0))
	P.detail(control,"desk_lamp_arm_01",.65,Vector3(-1,.83,0))
	furniture(ch,"SchoolChair_01",Vector3(.6,.95,.6),Vector3(32,0,-188),PI)
	for x: float in [58,62,74]:
		B.box(ch,Vector3(1.6,2.6,.75),Vector3(x,1.3,-190),steel,true)
		for i: int in range(8): B.box(ch,Vector3(1.3,.035,.04),Vector3(x,.4+i*.28,-189.59),dark)
	for at: Vector3 in [Vector3(32,0,-122),Vector3(88,0,-145),Vector3(92,0,-188)]:
		var bed := preload("res://scripts/hide_bed.gd").new(); bed.position = at; bed.metal = steel; ch.add_child(bed)
	for z: float in [-135,-166,-186]:
		furniture(ch,"SchoolChair_01",Vector3(.6,.9,.6),Vector3(52.8,0,z),-PI/2)
	ch.facility._sign("КОНТУРЫ / А → 2 / Б → 1 / В → 3",Vector3(18,2.5,-165),PI/2,17)
	ch.facility._sign("АРХИВ / ШКАФ 12-Б",Vector3(96,2.5,-171),0,19)
	ch.facility._sign("ДЕЖУРНАЯ ВОЛНА / 104.8 МГц",Vector3(32,2.6,-191.7),0,18)
	for at: Vector3 in [Vector3(18,.03,-121),Vector3(42,.03,-143),Vector3(87,.03,-168)]:
		for i: int in range(5): B.box(ch,Vector3(.16,.006,.22),at+Vector3(i*.18,0,sin(i)*.2),paper).rotation.y = i*.7
	_work_lighting(ch)
	# Wall-mounted conduits, skirting and inspection panels stay outside walkable space.
	for z: float in [-119,-139,-162,-183]:
		for x: float in [16.3,103.7]:
			for y: float in [2.5,2.72]: _pipe(ch,Vector3(x,y,z-6),Vector3(x,y,z+6),.05,steel)
			B.box(ch,Vector3(.08,.7,.6),Vector3(x,1.85,z),steel)
	for z: float in [-126.9,-149,-172.9,-191]:
		for x: float in [22,36,60,70,86,98]:
			B.box(ch,Vector3(1.8,.015,.11),Vector3(x,.02,z),warning)
	for row: Array in [[Vector3(65,0,-139),4.6,3.2],[Vector3(30,0,-157),4.0,4.0],[Vector3(32,0,-168),4.0,4.0]]:
		for side: float in [-1,1]:
			B.box(ch,Vector3(row[1],.009,.09),row[0]+Vector3(0,.026,side*row[2]/2),warning)
			B.box(ch,Vector3(.09,.009,row[2]),row[0]+Vector3(side*row[1]/2,.026,0),warning)
	for z: float in [-113,-133,-155,-177,-189]:
		B.box(ch,Vector3(8,.12,.25),Vector3(50,3.49,z),steel)
static func _work_lighting(ch: Node3D) -> void:
	for row: Array in [[Vector3(64,2.8,-136),Vector3(65,1,-139),"bac9bc"],[Vector3(28,2.8,-154),Vector3(30,1,-157),"d9ad75"],[Vector3(35,2.8,-165),Vector3(32,1,-168),"d9ad75"],[Vector3(94,2.8,-165),Vector3(96,.5,-168),"a0c8c2"],[Vector3(32,2.8,-187),Vector3(32,.7,-190),"c8c2a3"]]:
		var light := SpotLight3D.new(); ch.add_child(light); light.position = row[0]; light.look_at(row[1])
		light.spot_range = 11; light.spot_angle = 61; light.light_energy = 2; light.light_color = Color(row[2])
		light.shadow_enabled = true
static func _pipe(parent: Node3D,a: Vector3,b: Vector3,radius: float,material: Material) -> void:
	var mesh := CylinderMesh.new(); mesh.top_radius = radius; mesh.bottom_radius = radius; mesh.height = a.distance_to(b); mesh.radial_segments = 12
	var visual := MeshInstance3D.new(); visual.mesh = mesh; visual.material_override = material
	visual.position = (a+b)/2; visual.basis = Basis(Quaternion(Vector3.UP,(b-a).normalized())); parent.add_child(visual)
