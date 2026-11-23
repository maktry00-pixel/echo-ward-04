extends RefCounted
## Rounded cushions and a hanging sheet, with actual surface normals and folds.
static func cushion(parent: Node3D, size: Vector3, at: Vector3, material: Material, radius: float) -> void:
	var surface := SurfaceTool.new()
	surface.begin(Mesh.PRIMITIVE_TRIANGLES)
	var half: Vector3 = size/2
	var inner: Vector3 = half-Vector3.ONE*radius
	for normal: Vector3 in [Vector3.UP,Vector3.DOWN,Vector3.LEFT,Vector3.RIGHT,Vector3.FORWARD,Vector3.BACK]:
		var u: Vector3 = Vector3.RIGHT if absf(normal.x)<.5 else Vector3.FORWARD
		var v: Vector3 = u.cross(normal)
		for i: int in range(8):
			for j: int in range(8):
				for offset: Vector2i in [Vector2i(0,0),Vector2i(1,0),Vector2i(1,1),Vector2i(0,0),Vector2i(1,1),Vector2i(0,1)]:
					var uv := Vector2(i+offset.x,j+offset.y)/8
					var point: Vector3 = (normal+u*(uv.x*2-1)+v*(uv.y*2-1))*half
					var core: Vector3 = point.clamp(-inner,inner)
					var direction: Vector3 = (point-core).normalized()
					surface.set_normal(direction)
					surface.set_uv(uv)
					surface.add_vertex(core+direction*radius)
	surface.generate_tangents()
	var mesh := MeshInstance3D.new()
	mesh.mesh = surface.commit()
	mesh.material_override = material
	mesh.position = at
	parent.add_child(mesh)

static func blanket(parent: Node3D, material: Material) -> void:
	var surface := SurfaceTool.new()
	surface.begin(Mesh.PRIMITIVE_TRIANGLES)
	for i: int in range(20):
		for j: int in range(24):
			for offset: Vector2i in [Vector2i(0,0),Vector2i(1,0),Vector2i(1,1),Vector2i(0,0),Vector2i(1,1),Vector2i(0,1)]:
				var x: float = (float(i+offset.x)/20-.5)*1.48
				var z: float = float(j+offset.y)/24*1.55-.43
				var dx: float = .008*12*cos(x*12+z*4)-signf(x)*4.0*maxf(0,absf(x)-.57)
				var dz: float = .008*4*cos(x*12+z*4)+.009*9*cos(z*9)
				var y: float = .91+.008*sin(x*12+z*4)+.009*sin(z*9)-2.0*pow(maxf(0,absf(x)-.57),2)
				surface.set_normal(Vector3(-dx,1,-dz).normalized())
				surface.set_uv(Vector2(x,z))
				surface.add_vertex(Vector3(x,y,z))
	surface.generate_tangents()
	var mesh := MeshInstance3D.new()
	mesh.mesh = surface.commit()
	mesh.material_override = material
	parent.add_child(mesh)

static func curtain(parent: Node3D, at: Vector3, material: Material) -> void:
	var surface := SurfaceTool.new()
	surface.begin(Mesh.PRIMITIVE_TRIANGLES)
	for i: int in range(36):
		for j: int in range(12):
			for offset: Vector2i in [Vector2i(0,0),Vector2i(1,0),Vector2i(1,1),Vector2i(0,0),Vector2i(1,1),Vector2i(0,1)]:
				var z: float = float(i+offset.x)/36*2.65
				var height: float = float(j+offset.y)/12
				var phase: float = z*TAU*3.4
				var x: float = sin(phase)*.065
				var y: float = .33+height*1.84+sin(z*9)*.018*(1-height)
				surface.set_normal(Vector3(1,0,-cos(phase)*.065*TAU*3.4).normalized())
				surface.set_uv(Vector2(z,height*1.84))
				surface.add_vertex(Vector3(x,y,z))
	surface.generate_tangents()
	var mesh := MeshInstance3D.new()
	mesh.mesh = surface.commit()
	mesh.position = at
	mesh.material_override = material
	parent.add_child(mesh)
