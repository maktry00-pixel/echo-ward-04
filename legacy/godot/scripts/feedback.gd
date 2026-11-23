extends Node3D

const Builder = preload("res://scripts/world_builder.gd")
var impact_marks: Array[Node3D] = []

func reset_effects() -> void:
	impact_marks.clear()
	blood_marks = 0
	last_surface = ""
	for node: Node in get_children(): node.queue_free()

var last_surface: String = ""
var blood_marks: int = 0
func impact(point: Vector3, normal: Vector3, organic: bool, direction: Vector3, surface: String = "concrete") -> void:
	last_surface = "flesh" if organic else surface
	for i: int in range(7 if organic else 6):
		var spark: bool = not organic and surface=="metal"
		var mat := Builder.material("632d26" if organic else ("e9be78" if spark else ("8c7760" if surface=="wood" else "9a9a8e")),1.2 if spark else 0.0)
		mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
		var chip: MeshInstance3D = Builder.sphere(self,.016 if organic else .009,point,mat)
		chip.scale = Vector3(.7,.7,2.0) if organic or spark else Vector3.ONE*1.5
		var spread := Vector3(sin(i*2.3),.35+cos(i*1.7)*.4,cos(i*2.3))
		var destination: Vector3 = point+(normal*.25+spread*.30+direction*.22)
		var tween := chip.create_tween().set_parallel(true)
		tween.tween_property(chip,"position",destination,.26).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
		tween.tween_property(mat,"albedo_color:a",0,.4)
		tween.chain().tween_callback(chip.queue_free)
	if organic:
		# Spray only marks actual scenery behind/below the hit, never an empty plane.
		var space: PhysicsDirectSpaceState3D = get_world_3d().direct_space_state
		var back: Dictionary = space.intersect_ray(PhysicsRayQueryParameters3D.create(point+direction*.08,point+direction*2.8,1))
		if not back.is_empty(): _mark(back.position,back.normal,.13,Color("4c2522"),true)
		var floor_hit: Dictionary = space.intersect_ray(PhysicsRayQueryParameters3D.create(point+direction*.25,point+direction*.25-Vector3.UP*3.0,1))
		if not floor_hit.is_empty(): _mark(floor_hit.position,floor_hit.normal,.12,Color("49221f"),true)
		return
	_mark(point,normal,.046,Color("262a25"),false)
	if surface!="metal":
		for i: int in range(3):
			var tangent: Vector3 = normal.cross(Vector3.UP if absf(normal.y)<.9 else Vector3.RIGHT).normalized()
			_mark(point+tangent*(i-1)*.028,normal,.012,Color("817565"),false)

func _mark(at: Vector3, normal: Vector3, radius: float, color: Color, blood: bool) -> void:
	var mark: MeshInstance3D = preload("res://scripts/surface_mark.gd").create(self,at,normal,radius,color,impact_marks.size()*47+19)
	mark.set_meta("blood",blood)
	impact_marks.append(mark)
	if blood: blood_marks+=1
	if impact_marks.size()>48: impact_marks.pop_front().queue_free()


func pulse(origin: Vector3, radius: float, color: String = "c5e5ed") -> void:
	var mesh := TorusMesh.new()
	mesh.inner_radius = 0.92
	mesh.outer_radius = 1.0
	var visual := MeshInstance3D.new()
	visual.mesh = mesh
	var mat := Builder.material(color, 0.9)
	mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	visual.material_override = mat
	visual.position = origin + Vector3(0, 0.3, 0)
	visual.scale = Vector3.ONE * 0.15
	add_child(visual)
	var tween := create_tween().set_parallel(true)
	tween.tween_property(visual, "scale", Vector3(radius, 0.2, radius), 0.55).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	tween.tween_property(mat, "albedo_color:a", 0.0, 0.55)
	tween.chain().tween_callback(visual.queue_free)


func burst(origin: Vector3, color: String) -> void:
	for index: int in range(8):
		var mat := Builder.material(color, 0.6)
		mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
		var visual: MeshInstance3D = Builder.sphere(self, 0.06, origin, mat)
		var angle: float = TAU * index / 8.0
		var endpoint: Vector3 = origin + Vector3(cos(angle), 0.3 + (index % 3) * 0.3, sin(angle))
		var tween := create_tween().set_parallel(true)
		tween.tween_property(visual, "position", endpoint, 0.5).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
		tween.tween_property(mat, "albedo_color:a", 0.0, 0.5)
		tween.chain().tween_callback(visual.queue_free)
