extends RefCounted
## Authored first-zone dressing. All new large props stay outside the tested aisles.
const B = preload("res://scripts/world_builder.gd")
const P = preload("res://scripts/imported_props.gd")
const M = preload("res://scripts/environment_materials.gd")
static func counter(facility: Node3D) -> void:
	var wood: StandardMaterial3D = _wood()
	B.box(facility,Vector3(5.4,.79,.67),Vector3(-10.5,.395,14.13),wood,true)
	B.box(facility,Vector3(5.65,.09,1.03),Vector3(-10.5,.865,14),wood,true)
	B.box(facility,Vector3(5.4,.075,.045),Vector3(-10.5,.09,13.77),facility.metal_material)
	for x: float in [-12.1,-10.5,-8.9]:
		B.box(facility,Vector3(.025,.7,.03),Vector3(x,.41,13.78),facility.metal_material)
	for x: float in [-12.0,-9.0]:
		P.face(P.visual(facility,"painted_wooden_cabinet_02",Vector3(2.4,1.5,.45),Vector3(x,0,15.28)),Vector3(x,0,15.28),PI)
		P.body(facility,Vector3(2.4,1.5,.45),Vector3(x,.75,15.28))
	P.detail(facility,"portable_cassette_player",.30,Vector3(-11.8,.92,14))
	P.detail(facility,"medical_box",.30,Vector3(-8.6,.92,14.1))
	for i: int in range(5):
		B.box(facility,Vector3(.24,.008,.32),Vector3(-10.5+i*.045,.922+i*.008,13.83),B.material("b7ad96"))
	facility._sign("РЕГИСТРАЦИЯ  /  04",Vector3(-10.5,.57,13.775),PI,19)

static func build(facility: Node3D) -> Node3D:
	var root := Node3D.new()
	root.name = "OpeningArt"
	facility.add_child(root)
	# Directory and the stopped clock form a readable landmark beside the entry.
	var frame: Material = facility.metal_material
	B.box(root,Vector3(2.6,1.15,.06),Vector3(12.5,2.1,15.5),frame)
	B.box(root,Vector3(2.45,1.0,.02),Vector3(12.5,2.1,15.455),B.material("c3bba7"))
	var board: Label3D = facility._sign("ОТДЕЛЕНИЕ 04\nПАЛАТЫ — ПРЯМО\nСЛУЖЕБНОЕ КРЫЛО — НАЛЕВО\nДЕЖУРНАЯ — РЯДОМ СО ВХОДОМ",Vector3(12.5,2.1,15.43),PI,24)
	board.pixel_size = .0045
	board.outline_size = 0
	board.modulate = Color("273a3b")
	var clock: MeshInstance3D = B.sphere(root,.29,Vector3(-4.8,2.8,15.5),B.material("b8baac"))
	clock.scale.z = .10
	B.box(root,Vector3(.015,.18,.02),Vector3(-4.8,2.83,15.46),frame)
	B.box(root,Vector3(.13,.015,.02),Vector3(-4.74,2.8,15.45),frame)
	# A continuous skirting line and acoustic ceiling strips replace a bare box.
	for z: float in [9,11,13,15]:
		B.box(root,Vector3(29,.018,.055),Vector3(0,3.57,z),B.material("6f7976"))
	for x: float in [-13,-5,5,13]:
		B.box(root,Vector3(.055,.018,7.5),Vector3(x,3.56,12),B.material("6f7976"))
	for side: float in [-1,1]:
		var paint := M.pbr("painted_plaster_wall","9ba6a0" if side<0 else "91a3ad",.45)
		B.box(root,Vector3(.018,1.28,7.1),Vector3(side*14.58,1.85,3.4),paint)
		var line: Node3D = B.box(root,Vector3(.025,.065,6.5),Vector3(side*14.55,1.18,3.4),frame)
		line.name = "WardHandrail"
		for i: int in range(4):
			B.box(root,Vector3(.12,.045,.045),Vector3(side*14.5,1.18,1+i*1.5),frame)
		# Bed charts and belongings stay on the bed, away from drawer sweeps.
		B.box(root,Vector3(.27,.013,.34),Vector3(side*7,.66,5.95),B.material("a79e85"))
		P.detail(root,"medical_tape",.075,Vector3(side*7+.20,.68,6.0))
		P.detail(root,"small_plastic_torch",.23,Vector3(side*11.5,.64,6.6),Vector3(0,.45,PI/2))
		for j: int in range(3):
			var y: float = .65+j*.003
			B.box(root,Vector3(.18,.007,.23),Vector3(side*11.5+.16,y,6.2),B.material("b7b1a0"))
		facility._sign("04 / НАБЛЮДЕНИЕ" if side<0 else "05 / НЕ ПЕРЕМЕЩАТЬ",Vector3(side*12.3,2.35,7.77),PI,15)
	# Blood is an authored trail, not a glowing floor marker.
	var mark = preload("res://scripts/surface_mark.gd")
	for i: int in range(8):
		mark.create(root,Vector3(8.7+sin(i)*.25,.003,5.2-i*.34),Vector3.UP,.04+(i%3)*.018,Color("3c2726"),i+90)
	mark.create(root,Vector3(3.21,1.25,2.2),Vector3.RIGHT,.10,Color("4e2d27"),12)
	for i: int in range(4): mark.create(root,Vector3(3.21,1.37+i*.022,2.10+i*.065),Vector3.RIGHT,.023,Color("4e2d27"),i+31)
	# Broad bounce keeps faces/furniture readable; ceiling fixtures retain shadows.
	for at: Vector3 in [Vector3(-8,2.2,12),Vector3(8,2.2,12),Vector3(-9,2.25,3.8),Vector3(9,2.25,3.8)]:
		var light := OmniLight3D.new()
		light.position = at
		light.light_color = Color("c0cbd0") if at.z<8 else Color("dec6a5")
		light.light_energy = .23 if at.z<8 else .30
		light.omni_range = 6.5
		light.distance_fade_enabled = true
		light.distance_fade_begin = 15
		light.distance_fade_length = 6
		root.add_child(light)
	return root

static func _wood() -> StandardMaterial3D:
	var sample: Node3D = P.scene("painted_wooden_cabinet_02")
	var material: StandardMaterial3D = sample.find_children("*","MeshInstance3D",true,false)[0].get_active_material(0).duplicate()
	material.uv1_triplanar = true
	material.uv1_world_triplanar = true
	material.uv1_scale = Vector3.ONE*.6
	material.roughness = .84
	material.metallic = 0
	material.set_meta("surface_kind","wood")
	sample.free()
	return material
