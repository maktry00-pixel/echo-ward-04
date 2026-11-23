extends RefCounted
## Flat, irregular geometry for the Compatibility renderer; no screen-space decals.
static func create(parent: Node3D, point: Vector3, normal: Vector3, radius: float, color: Color, seed_number: int = 0) -> MeshInstance3D:
	var mesh := SurfaceTool.new()
	mesh.begin(Mesh.PRIMITIVE_TRIANGLES)
	var rng := RandomNumberGenerator.new()
	rng.seed = seed_number
	var points: Array[Vector3] = []
	for i: int in range(16):
		var angle: float = TAU*i/16
		points.append(Vector3(cos(angle),sin(angle),0)*radius*rng.randf_range(.65,1.12))
	for i: int in range(16):
		for vertex: Vector3 in [Vector3.ZERO,points[i],points[(i+1)%16]]:
			mesh.set_normal(Vector3.BACK)
			mesh.add_vertex(vertex)
	var visual := MeshInstance3D.new()
	visual.mesh = mesh.commit()
	var material := StandardMaterial3D.new()
	material.albedo_color = color
	material.roughness = .88
	material.metallic_specular = .15
	material.cull_mode = BaseMaterial3D.CULL_DISABLED
	visual.material_override = material
	visual.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	parent.add_child(visual)
	visual.global_position = point+normal*.006
	visual.quaternion = Quaternion(Vector3.BACK,normal.normalized())
	return visual
