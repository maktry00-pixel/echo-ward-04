extends RefCounted
## CC0 Poly Haven geometry; explicit simplified collision stays aligned to the visual.
static var scenes: Dictionary = {}
const CLOSE_UP: Array[String] = ["painted_wooden_nightstand", "vintage_wooden_drawer_01", "medical_box", "crowbar_01", "service_pistol", "fish_knife", "circuit_board", "portable_cassette_player", "small_plastic_torch", "medical_tape", "sofa_03", "SchoolChair_01", "painted_wooden_cabinet_02", "desk_lamp_arm_01"]

static func scene(id: String) -> Node3D:
	if not scenes.has(id):
		var resolution: String = "2k" if id in CLOSE_UP else "1k"
		scenes[id] = load("res://assets/models/polyhaven/%s/%s_%s.gltf" % [id,id,resolution])
	var root: Node3D = scenes[id].instantiate()
	for mesh: MeshInstance3D in root.find_children("*","MeshInstance3D",true,false):
		for i: int in range(mesh.mesh.get_surface_count()):
			var material: Material = mesh.get_active_material(i)
			if material is BaseMaterial3D:
				material.texture_filter = BaseMaterial3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC
	return root

static func visual(parent: Node3D, id: String, size: Vector3, at: Vector3 = Vector3.ZERO) -> Node3D:
	var root: Node3D = scene(id)
	parent.add_child(root)
	var bounds := AABB()
	var first: bool = true
	for mesh: MeshInstance3D in root.find_children("*","MeshInstance3D",true,false):
		var box: AABB = root.global_transform.affine_inverse()*mesh.global_transform*mesh.get_aabb()
		bounds = box if first else bounds.merge(box)
		first = false
		for i: int in range(mesh.mesh.get_surface_count()):
			var source: Material = mesh.get_active_material(i)
			if source is StandardMaterial3D:
				var mat: StandardMaterial3D = source.duplicate()
				mat.metallic_specular = .3
				mesh.set_surface_override_material(i,mat)
	var factor: Vector3 = size/bounds.size.max(Vector3.ONE*.001)
	root.scale = factor
	root.position = at-Vector3(bounds.get_center().x,bounds.position.y,bounds.get_center().z)*factor
	return root

## Small objects keep their original proportions and UV/PBR materials.
## Optional mesh selection removes the author's display-layout spare parts.
static func detail(parent: Node3D, id: String, longest: float, at: Vector3 = Vector3.ZERO, angles: Vector3 = Vector3.ZERO, selected_mesh: String = "") -> Node3D:
	var root: Node3D = scene(id)
	parent.add_child(root)
	root.rotation = angles
	var bounds := AABB()
	var first: bool = true
	for mesh: MeshInstance3D in root.find_children("*","MeshInstance3D",true,false):
		if not selected_mesh.is_empty() and str(mesh.name)!=selected_mesh:
			mesh.free()
			continue
		var box: AABB = parent.global_transform.affine_inverse()*mesh.global_transform*mesh.get_aabb()
		bounds = box if first else bounds.merge(box)
		first = false
	var factor: float = longest / maxf(bounds.size.x,maxf(bounds.size.y,bounds.size.z))
	root.scale = Vector3.ONE*factor
	root.position = at-Vector3(bounds.get_center().x,bounds.position.y,bounds.get_center().z)*factor
	return root

static func body(parent: Node3D,size: Vector3,at: Vector3,surface_kind: String = "wood") -> StaticBody3D:
	var result := StaticBody3D.new()
	result.set_meta("impact_surface",surface_kind)
	result.collision_layer = 1
	result.collision_mask = 2|4
	result.position = at
	var shape := BoxShape3D.new()
	shape.size = size
	var collision := CollisionShape3D.new()
	collision.shape = shape
	result.add_child(collision)
	parent.add_child(result)
	result.add_to_group("solid_furniture")
	return result

## Turn a normalized visual around its authored placement, preserving its centre.
static func face(root: Node3D, at: Vector3, yaw: float) -> Node3D:
	root.position = at+Basis(Vector3.UP,yaw)*(root.position-at)
	root.rotation.y = yaw
	return root
