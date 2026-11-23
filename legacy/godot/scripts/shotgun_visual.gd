extends Node3D
## Original fictional pump shotgun. Metres, muzzle along local -Z.
const B = preload("res://scripts/world_builder.gd")
const M = preload("res://scripts/environment_materials.gd")
const MUZZLE := Vector3(0,.015,-.76)
var pump: Node3D
var bolt: Node3D
var shell: Node3D
func _ready() -> void:
	var steel := M.surface("metal","424b49",4,.39)
	var dark := B.material("172021")
	var wood: ShaderMaterial = preload("res://scripts/wood_surface.gd").make(Color("9b7048"),Vector3(7,9,.22))
	_barrel(self,.026,.61,Vector3(0,.015,-.44),steel)
	_barrel(self,.021,.46,Vector3(0,-.048,-.36),steel)
	_barrel(self,.017,.006,MUZZLE-Vector3(0,0,.003),dark)
	B.box(self,Vector3(.075,.10,.23),Vector3(0,-.025,-.065),steel)
	B.box(self,Vector3(.004,.042,.115),Vector3(.039,-.005,-.05),dark)
	bolt = B.box(self,Vector3(.006,.029,.069),Vector3(.043,-.005,-.035),steel)
	B.box(self,Vector3(.013,.04,.02),Vector3(0,.061,-.70),steel)
	B.box(self,Vector3(.075,.01,.04),Vector3(0,.035,.015),steel)
	var grip := _rounded(self,Vector3(.057,.12,.085),Vector3(0,-.12,.06),wood)
	grip.rotation.x = -.35
	var stock := _rounded(self,Vector3(.068,.10,.32),Vector3(0,-.12,.27),wood,.55)
	stock.rotation.x = .14
	_rounded(self,Vector3(.075,.14,.025),Vector3(0,-.14,.435),dark)
	for x: float in [-.027,.027]: B.box(self,Vector3(.008,.008,.085),Vector3(x,-.15,.025),steel)
	B.box(self,Vector3(.055,.009,.015),Vector3(0,-.15,-.015),steel)
	pump = Node3D.new()
	add_child(pump)
	var forearm := _rounded(pump,Vector3(.085,.09,.20),Vector3(0,-.04,-.36),wood)
	for i: int in range(8): B.box(forearm,Vector3(.09,.006,.007),Vector3(0,-.045,-.08+i*.022),dark)
	shell = shell_visual(self,Vector3(-.05,-.12,-.05))
	shell.hide()
static func _rounded(parent: Node3D,size: Vector3,at: Vector3,mat: Material,taper: float = 1) -> MeshInstance3D:
	var surface := SurfaceTool.new()
	surface.begin(Mesh.PRIMITIVE_TRIANGLES)
	var rings: Array[PackedVector3Array] = []
	for end: int in range(2):
		var scale_xy: float = taper if end==0 else 1.0
		var half: Vector2 = Vector2(size.x,size.y)*.5*scale_xy
		var bevel: float = minf(half.x,half.y)*.3
		var ring := PackedVector3Array()
		for p: Vector2 in [Vector2(-half.x+bevel,-half.y),Vector2(half.x-bevel,-half.y),Vector2(half.x,-half.y+bevel),Vector2(half.x,half.y-bevel),Vector2(half.x-bevel,half.y),Vector2(-half.x+bevel,half.y),Vector2(-half.x,half.y-bevel),Vector2(-half.x,-half.y+bevel)]: ring.append(Vector3(p.x,p.y,size.z*(end-.5)))
		rings.append(ring)
	for i: int in range(8):
		var j: int = (i+1)%8
		for point: Vector3 in [rings[0][i],rings[1][i],rings[1][j],rings[0][i],rings[1][j],rings[0][j]]: surface.add_vertex(point)
		for end: int in range(2):
			var points: Array[Vector3] = [Vector3(0,0,size.z*(end-.5)),rings[end][i],rings[end][j]]
			if end==1: points.reverse()
			for point: Vector3 in points: surface.add_vertex(point)
	surface.generate_normals()
	var mesh := MeshInstance3D.new()
	mesh.mesh = surface.commit()
	mesh.position = at
	mesh.material_override = mat
	mesh.set_meta("bevelled",true)
	parent.add_child(mesh)
	return mesh
func pose(recoil: float,reload_phase: float,pump_phase: float = 0) -> void:
	position.z = recoil*.075
	pump.position.z = sin(clampf(pump_phase,0,1)*PI)*.085
	bolt.position.z = -.035+pump.position.z
	shell.visible = reload_phase>=0 and reload_phase<.85
	shell.position = Vector3(-.055,-.18+sin(clampf(reload_phase,0,1)*PI)*.08,-.04)
static func _barrel(parent: Node3D,radius: float,length: float,at: Vector3,mat: Material) -> void:
	var visual := MeshInstance3D.new()
	var mesh := CylinderMesh.new()
	mesh.top_radius = radius
	mesh.bottom_radius = radius
	mesh.height = length
	mesh.radial_segments = 24
	visual.mesh = mesh
	visual.material_override = mat
	visual.position = at
	visual.rotation.x = PI/2
	parent.add_child(visual)
static func shell_visual(parent: Node3D,at: Vector3) -> Node3D:
	var root := Node3D.new()
	parent.add_child(root)
	root.position = at
	_barrel(root,.011,.058,Vector3.ZERO,B.material("8a3530"))
	_barrel(root,.012,.013,Vector3(0,0,.029),B.material("a68c54"))
	return root
