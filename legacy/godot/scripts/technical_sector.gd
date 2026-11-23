extends RefCounted
const Builder = preload("res://scripts/world_builder.gd")
const Materials = preload("res://scripts/environment_materials.gd")

static func build(f: Node3D) -> void:
	var steel := Materials.surface("metal", "586661", 1.1)
	# Sloped collision supports walking in both directions; thin step treads
	# are visual only, preventing the capsule from catching on each riser.
	var ramp: Node3D = Builder.box(f, Vector3(4.0, 0.3, 13), Vector3(52, -2.65, 38), steel, true)
	ramp.rotation.x = atan2(5, 12)
	for i: int in range(24):
		Builder.box(f, Vector3(4, 0.04, 0.48), Vector3(52, -float(i) * 5 / 24, 32 + float(i) * 12 / 24), steel)
	for x: float in [49.65, 54.35]:
		var wall: Node3D = Builder.box(f, Vector3(0.4, 4, 14), Vector3(x, -0.75, 38), f.wall_material, true)
		wall.rotation.x = atan2(5, 12)
		f._pipe(Vector3(x, 1.15, 32), Vector3(x, -3.85, 44), 0.055, steel)
	var floor_mat := Materials.surface("concrete", "737c71", 0.6)
	Builder.box(f, Vector3(24, 0.4, 38), Vector3(48, -5.2, 63), floor_mat, true)
	for x: int in range(38, 60, 4):
		for z: int in range(46, 82, 4):
			Builder.box(f, Vector3(4, 0.04, 4), Vector3(x, -5.01, z), floor_mat)
	Builder.box(f, Vector3(24, 0.25, 38), Vector3(48, -1.28, 63), f.wall_material, true)
	# Outer walls and three departments. Gaps form a loop around machinery.
	for pair: Array in [[Vector3(36, -5, 63), Vector3(0.4, 3.6, 38)], [Vector3(60, -5, 63), Vector3(0.4, 3.6, 38)], [Vector3(48, -5, 82), Vector3(24, 3.6, 0.4)], [Vector3(43, -5, 44), Vector3(14, 3.6, 0.4)], [Vector3(57.1, -5, 44), Vector3(5.8, 3.6, 0.4)], [Vector3(46, -5, 64), Vector3(0.35, 3.6, 12)], [Vector3(50, -5, 64), Vector3(0.35, 3.6, 12)]]:
		f._wall(pair[0], pair[1])
	for x: float in [40, 56]:
		for z: float in [48, 58, 68, 78]:
			f._lamp(Vector3(x, -1.65, z), Color("9aa6a8") if x == 40 else Color("c79a75"), 0.55, 8)
		f._pipe(Vector3(x, -1.85, 46), Vector3(x, -1.85, 80), 0.13, steel)
	for z: float in [54, 62, 70]:
		for x: float in [39, 42]:
			Builder.box(f, Vector3(1.5, 2.6, 1), Vector3(x, -3.7, z), steel, true)
			for y: float in [-4.5, -3.6, -2.7]:
				Builder.box(f, Vector3(1.25, 0.05, 0.05), Vector3(x, y, z - 0.55), Builder.material("807550"))
	for z: float in [60, 69]:
		Builder.box(f, Vector3(3, 1.1, 2.2), Vector3(55.5, -4.45, z), steel, true)
		for i: int in range(5):
			f._pipe(Vector3(54.4 + i * 0.5, -3.8, z - 0.8), Vector3(54.4 + i * 0.5, -2.4, z - 0.8), 0.09, steel)
	f._sign("НИЖНИЙ СЕКТОР / ОЧИСТКА", Vector3(52, -2.15, 47), 0, 26)
	f._sign("НАСОСНАЯ  ←      РЕАКТОР  →", Vector3(48, -2.15, 71), 0, 22)
	f._sign("ХЛАДАГЕНТ", Vector3(37, -2.5, 78), PI / 2, 20)
	f._sign("НЕ ПОКИДАТЬ ПОСТ ВО ВРЕМЯ ОЧИСТКИ", Vector3(59.5, -2.2, 77), -PI / 2, 18)
