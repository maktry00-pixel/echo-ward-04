extends RefCounted
## Authored extension: three large departments off a connected service corridor.

const Builder = preload("res://scripts/world_builder.gd")
const Materials = preload("res://scripts/environment_materials.gd")
const Technical = preload("res://scripts/technical_sector.gd")

static func build(facility: Node3D) -> void:
	var floor_mat := Materials.pbr("terrazzo_tiles", "a2b1b0", .5,.9)
	Builder.box(facility, Vector3(45, 0.4, 48), Vector3(37.5, -0.2, 8), floor_mat, true)
	Builder.box(facility, Vector3(45, 0.25, 48), Vector3(37.5, 3.72, 8), facility.ceiling_material, true)
	for x: int in range(18, 60, 4):
		for z: int in range(-14, 32, 4):
			Builder.box(facility, Vector3(4, 0.04, 4), Vector3(x, -0.01, z), floor_mat)
	for pair: Array in [[Vector3(60, 0, 8), Vector3(0.8, 3.6, 48)], [Vector3(25.15, 0, -16), Vector3(20.3, 3.6, 0.8)], [Vector3(49.35,0,-16),Vector3(21.3,3.6,.8)], [Vector3(32.6, 0, 32), Vector3(35.2, 3.6, 0.8)], [Vector3(57.3, 0, 32), Vector3(5.4, 3.6, 0.8)], [Vector3(15, 0, 24), Vector3(0.8, 3.6, 16)]]:
		if pair[0].x==60: continue
		facility._wall(pair[0], pair[1])
	for pair: Vector2 in [Vector2(-14.8,2.4),Vector2(7,34.8),Vector2(29.8,4.4)]:
		facility._wall(Vector3(60,0,pair.x),Vector3(.8,3.6,pair.y))
	facility._add_door("technical_gate", Vector3(52, 0, 32), 0, "containment", 3.2)
	Technical.build(facility)
	for x: float in [30, 44]:
		facility._wall(Vector3(x, 0, 11.5), Vector3(0.4, 3.6, 41))
	# A broad corridor links the three departments; every room has a loop
	# around its central island, so combat never requires a narrow dead end.
	for pair: Vector2 in [Vector2(18.25, 6.5), Vector2(30, 11), Vector2(44.5, 12), Vector2(56.75, 6.5)]:
		facility._wall(Vector3(pair.x, 0, -9), Vector3(pair.y, 3.6, 0.35))
	for x: float in [23, 37, 52]:
		facility._add_door("research_%d" % int(x), Vector3(x, 0, -9), 0, "", 3.0, true)
		facility._wall(Vector3(x - 4.1, 0, 7), Vector3(5, 3.6, 0.35))
		facility._wall(Vector3(x + 4.1, 0, 7), Vector3(5, 3.6, 0.35))
		facility._add_door("sterile_%d" % int(x), Vector3(x, 0, 7), 0, "", 3.2, true)
		for z: float in [-12, -3, 12, 26]:
			var light_color: Color = {23.0: Color("8ba1b0"), 37.0: Color("92ad93"), 52.0: Color("bcad99")}[x]
			facility._lamp(Vector3(x, 3.35, z), light_color, 0.6, 8)
		for z: float in [-4, 5, 16, 28]:
			facility._pipe(Vector3(x - 5.8, 3.25, z), Vector3(x + 5.8, 3.25, z), 0.065, facility.metal_material)
		facility._pipe(Vector3(x - 5.6, 3.25, -8), Vector3(x - 5.6, 3.25, 30), 0.09, facility.metal_material)
		# Dirty glazing, skirting and directional floor strips establish scale.
		for z: float in range(-7, 30, 3):
			Builder.box(facility, Vector3(0.08, 0.009, 1.5), Vector3(x - 1.9, 0.03, z), Builder.material("887959"))
		facility._sign("АВАРИЙНЫЙ УЗЕЛ  /  %02d" % int(x), Vector3(x, 2.9, 29.6), PI, 24)
	facility._sign("ИССЛЕДОВАТЕЛЬСКОЕ КРЫЛО  →", Vector3(16.4, 2.8, -14.7), 0, 24)
	facility._sign("01  МОРГ", Vector3(23, 2.8, -8.7), 0, 28)
	facility._sign("02  ЛАБОРАТОРИЯ", Vector3(37, 2.8, -8.7), 0, 25)
	facility._sign("03  ОПЕРАЦИОННАЯ", Vector3(52, 2.8, -8.7), 0, 25)
	# Morgue lockers and autopsy tables.
	var steel := Materials.surface("metal", "7c8a85", 1.5, 0.48)
	for z: float in [-5, -1, 11, 15, 19, 27]:
		if z==-5: continue # Keep the new emergency exit physically clear.
		Builder.box(facility, Vector3(0.7, 2.8, 2.8), Vector3(16.1, 1.4, z), steel, true)
		for y: float in [0.5, 1.4, 2.3]:
			Builder.box(facility, Vector3(0.08, 0.06, 0.5), Vector3(16.52, y, z), facility.metal_material)
	for z: float in [0, 15]:
		facility._table(Vector3(23, 0, z), Vector3(2.2, 0.85, 3.4), steel)
		var cover := Materials.surface("concrete", "8e9382", 2, 0.95)
		var sheet: MeshInstance3D = Builder.sphere(facility, 1, Vector3(23, 1.1, z), cover)
		sheet.scale = Vector3(0.55, 0.20, 1.25)
	# Laboratory has luminous containment cylinders and a central island.
	for z: float in [-4, 12, 20, 27]:
		for x: float in [32, 42]:
			_tank(facility, Vector3(x, 0, z))
	facility._table(Vector3(37, 0, 0), Vector3(3.4, 0.9, 3.4), steel)
	facility._table(Vector3(37, 0, 16), Vector3(3.4, 0.9, 3.4), steel)
	for x: float in [35.9, 38.1]:
		_monitor(facility, Vector3(x, 1.05, 16), "72  /  CONTAINMENT")
	# Operating theatre: tables, suspended lights, monitoring equipment.
	for z: float in [0, 15]:
		facility._bed(Vector3(52, 0, z), Materials.surface("concrete", "68766d", 1.5))
		facility._pipe(Vector3(52, 3.5, z), Vector3(52, 2.7, z), 0.06, steel)
		Builder.sphere(facility, 0.45, Vector3(52, 2.7, z), facility.metal_material).scale.y = 0.15
		_monitor(facility, Vector3(57.8, 1.2, z), "PULSE  /  --")
		Builder.box(facility, Vector3(1, 1.1, 0.65), Vector3(57.8, 0.55, z), steel, true)
	facility._sign("МОРГ / ИНСТРУКЦИЯ\nВсе цепи различны: I, II, III.\nПервая — минимальная.\nВторая больше третьей.", Vector3(28.8, 2.1, 26), -PI / 2, 19)
	facility._sign("ЛАБОРАТОРИЯ / ИНСТРУКЦИЯ\nВсе цепи различны: I, II, III.\nТретья = первая + вторая.\nПервая больше второй.", Vector3(43.7, 2.1, 26), -PI / 2, 19)
	facility._sign("ОПЕРАЦИОННАЯ / ИНСТРУКЦИЯ\nВсе цепи различны: I, II, III.\nКаждая следующая меньше\nпредыдущей на единицу.", Vector3(59.5, 2.1, 26), -PI / 2, 19)
	var stain := Builder.material("382c27")
	for i: int in range(36):
		var x: float = 17 + fmod(i * 7.31, 41)
		var z: float = -7 + fmod(i * 5.7, 36)
		var debris := Builder.box(facility, Vector3(0.2, 0.01, 0.35), Vector3(x, 0.03, z), stain)
		debris.rotation.y = i * 0.9


static func _monitor(parent: Node3D, at: Vector3, text: String) -> void:
	Builder.box(parent, Vector3(0.9, 0.65, 0.13), at, Builder.material("192521"))
	Builder.box(parent, Vector3(0.76, 0.5, 0.02), at + Vector3(0, 0, -0.08), Builder.material("344f44", 0.35))
	var label: Label3D = parent._sign(text, at + Vector3(0, 0, -0.10), PI, 14)
	label.pixel_size = 0.003


static func _tank(parent: Node3D, at: Vector3) -> void:
	Builder.box(parent, Vector3(1.1, 0.25, 1.1), at + Vector3(0, 0.125, 0), parent.metal_material, true)
	Builder.box(parent, Vector3(1.1, 0.25, 1.1), at + Vector3(0, 2.85, 0), parent.metal_material)
	var mesh := CylinderMesh.new()
	mesh.top_radius = 0.43
	mesh.bottom_radius = 0.43
	mesh.height = 2.5
	var visual := MeshInstance3D.new()
	visual.mesh = mesh
	visual.position = at + Vector3(0, 1.55, 0)
	var glass := Builder.material("344f43", 0.16)
	glass.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	glass.albedo_color.a = 0.40
	visual.material_override = glass
	parent.add_child(visual)
	Builder.sphere(parent, 0.28, at + Vector3(0, 1.6, 0), Builder.material("666c53")).scale.y = 1.8
	for x: float in [-0.48, 0.48]:
		parent._pipe(at + Vector3(x, 0.2, 0), at + Vector3(x, 2.9, 0), 0.04, parent.metal_material)
