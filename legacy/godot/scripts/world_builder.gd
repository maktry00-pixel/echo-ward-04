extends RefCounted
## Small code-built assets for the prototype. Visuals and colliders share size.


static func material(color: String, emission: float = 0.0) -> StandardMaterial3D:
	var result := StandardMaterial3D.new()
	result.albedo_color = Color(color)
	result.roughness = 0.65
	if emission > 0.0:
		result.emission_enabled = true
		result.emission = Color(color)
		result.emission_energy_multiplier = emission
	return result


static func box(parent: Node3D, size: Vector3, location: Vector3,
		mat: Material, solid: bool = false) -> Node3D:
	var holder: Node3D
	if solid:
		var body := StaticBody3D.new()
		body.set_meta("impact_surface",mat.get_meta("surface_kind","metal" if mat is StandardMaterial3D and mat.metallic>.4 else "concrete"))
		body.collision_layer = 1
		body.collision_mask = 2
		var shape := BoxShape3D.new()
		shape.size = size
		var collision := CollisionShape3D.new()
		collision.shape = shape
		body.add_child(collision)
		holder = body
	else:
		holder = Node3D.new()
	var mesh := BoxMesh.new()
	mesh.size = size
	var visual := MeshInstance3D.new()
	visual.mesh = mesh
	visual.material_override = mat
	if not solid:
		visual.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	holder.add_child(visual)
	holder.position = location
	parent.add_child(holder)
	return holder


static func sphere(parent: Node3D, radius: float, location: Vector3,
		mat: Material) -> MeshInstance3D:
	var mesh := SphereMesh.new()
	mesh.radius = radius
	mesh.height = radius * 2.0
	mesh.radial_segments = 16
	mesh.rings = 8
	var visual := MeshInstance3D.new()
	visual.mesh = mesh
	visual.material_override = mat
	visual.position = location
	parent.add_child(visual)
	return visual
