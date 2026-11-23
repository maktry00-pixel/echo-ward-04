extends SceneTree
## Exports the existing original creature as articulated static OBJ parts, not a fake FBX skeleton.
const OUT := "res://unreal/EchoWard/RawAssets/Monster/"
var palette: Array[Dictionary] = []
var palette_keys: Dictionary = {}

func _initialize() -> void:
	call_deferred("export_all")

func ue(v: Vector3) -> Vector3:
	return Vector3(-v.z, v.x, v.y) * 100.0

func material_id(material: Material) -> int:
	var color := Color("545449")
	var roughness: float = .86
	if material is StandardMaterial3D:
		color = material.albedo_color
		roughness = material.roughness
	elif material is ShaderMaterial:
		var tissue: Variant = material.get_shader_parameter("tissue_color")
		if tissue is Color: color = tissue
	var key: String = color.to_html(true)
	if not palette_keys.has(key):
		palette_keys[key] = palette.size()
		palette.append({"name": "M_Creature_%d" % palette.size(), "color": [color.r, color.g, color.b], "roughness": roughness})
	return palette_keys[key]

func collect(node: Node, excluded: Array[Node], found: Array[MeshInstance3D]) -> void:
	if node in excluded: return
	if node is MeshInstance3D and node.mesh != null: found.append(node)
	for child: Node in node.get_children(): collect(child, excluded, found)

func export_part(name: String, pivot: Node3D, meshes: Array[MeshInstance3D]) -> Dictionary:
	var file := FileAccess.open(OUT + name + ".obj", FileAccess.WRITE)
	file.store_line("# Original Echo Ward creature; UE centimetres; joint-local pivot")
	file.store_line("mtllib creature.mtl")
	var count: int = 0
	var triangles: int = 0
	for item: MeshInstance3D in meshes:
		var transform: Transform3D = pivot.global_transform.affine_inverse() * item.global_transform
		var normal_transform: Basis = transform.basis.inverse().transposed()
		for surface: int in range(item.mesh.get_surface_count()):
			var arrays: Array = item.mesh.surface_get_arrays(surface)
			var positions: PackedVector3Array = arrays[Mesh.ARRAY_VERTEX]
			var normals: PackedVector3Array = arrays[Mesh.ARRAY_NORMAL]
			var uv: PackedVector2Array = arrays[Mesh.ARRAY_TEX_UV] if arrays[Mesh.ARRAY_TEX_UV] != null else PackedVector2Array()
			var indices: PackedInt32Array = arrays[Mesh.ARRAY_INDEX] if arrays[Mesh.ARRAY_INDEX] != null else PackedInt32Array()
			if indices.is_empty():
				for i: int in positions.size(): indices.append(i)
			var material: Material = item.material_override if item.material_override != null else item.mesh.surface_get_material(surface)
			file.store_line("usemtl material_%d" % material_id(material))
			var points: PackedVector3Array = []
			var directions: PackedVector3Array = []
			for i: int in positions.size():
				var p: Vector3 = ue(transform * positions[i])
				points.append(p)
				file.store_line("v %.7f %.7f %.7f" % [p.x, p.y, p.z])
			for i: int in positions.size():
				var tex: Vector2 = uv[i] if i < uv.size() else Vector2.ZERO
				file.store_line("vt %.7f %.7f" % [tex.x, 1.0-tex.y])
			for i: int in positions.size():
				var n: Vector3 = ue(normal_transform * normals[i]).normalized() if i < normals.size() else Vector3.UP
				directions.append(n)
				file.store_line("vn %.7f %.7f %.7f" % [n.x, n.y, n.z])
			for i: int in range(0, indices.size(), 3):
				var a: int = indices[i]
				var b: int = indices[i+1]
				var c: int = indices[i+2]
				if (points[b]-points[a]).cross(points[c]-points[a]).dot(directions[a]+directions[b]+directions[c]) < 0:
					var swap: int = b
					b = c
					c = swap
				a += count+1
				b += count+1
				c += count+1
				file.store_line("f %d/%d/%d %d/%d/%d %d/%d/%d" % [a,a,a,b,b,b,c,c,c])
				triangles += 1
			count += positions.size()
	file.close()
	return {"name": name, "source": "Monster/" + name + ".obj", "vertices": count, "triangles": triangles}

func export_all() -> void:
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(OUT))
	var creature := preload("res://scripts/aberration_visual.gd").new()
	creature.crawler = true
	root.add_child(creature)
	creature.set_process(false)
	await process_frame
	var exclude: Array[Node] = [creature.head]
	for limb: Node3D in creature.limbs: exclude.append(limb)
	var meshes: Array[MeshInstance3D] = []
	collect(creature, exclude, meshes)
	var parts: Array[Dictionary] = [export_part("SM_Body", creature, meshes)]
	meshes = []
	collect(creature.head, [], meshes)
	parts.append(export_part("SM_Head", creature.head, meshes))
	var head_pivot: Vector3 = ue(creature.head.global_position)
	var header: String = "#pragma once\n#include \"CoreMinimal.h\"\n// Generated from original Godot creature; centimetres.\nnamespace EWardCreature {\ninline const FVector HeadPivot(%.7f, %.7f, %.7f);\ninline const FVector LegPivots[6] = {\n" % [head_pivot.x, head_pivot.y, head_pivot.z]
	var pivots: Array = []
	for i: int in creature.limbs.size():
		var limb: Node3D = creature.limbs[i]
		meshes = []
		collect(limb, [], meshes)
		parts.append(export_part("SM_Leg%d" % i, limb, meshes))
		var p: Vector3 = ue(limb.global_position)
		pivots.append([p.x,p.y,p.z])
		header += "FVector(%.7f, %.7f, %.7f),\n" % [p.x,p.y,p.z]
	header += "};\n}\n"
	var output := FileAccess.open("res://unreal/EchoWard/Source/EchoWard/CreatureLayout.h", FileAccess.WRITE)
	output.store_string(header)
	output.close()
	output = FileAccess.open(OUT + "creature.json", FileAccess.WRITE)
	output.store_string(JSON.stringify({"parts": parts, "materials": palette, "head_pivot": [head_pivot.x,head_pivot.y,head_pivot.z], "leg_pivots": pivots, "source": "Original procedural Echo Ward creature"}, "  "))
	output.close()
	output = FileAccess.open(OUT + "creature.mtl", FileAccess.WRITE)
	for i: int in palette.size():
		var color: Array = palette[i]["color"]
		output.store_line("newmtl material_%d\nKd %.6f %.6f %.6f\nNs 20\nd 1\n" % [i, color[0], color[1], color[2]])
	output.close()
	print("UNREAL_CREATURE_EXPORT_OK parts=", parts.size())
	quit()
