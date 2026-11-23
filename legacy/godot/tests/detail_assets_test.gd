extends SceneTree
## Packaging/material contracts and assembly checks for downloaded art.
const Props = preload("res://scripts/imported_props.gd")
var checks: int = 0
var failures: int = 0
func _initialize() -> void: call_deferred("_run")
func _run() -> void:
	var stage := Node3D.new()
	root.add_child(stage)
	for id: String in ["service_pistol","fish_knife","circuit_board","portable_cassette_player","small_plastic_torch","medical_tape"]:
		var model: Node3D = Props.scene(id)
		stage.add_child(model)
		var textured: bool = false
		for mesh: MeshInstance3D in model.find_children("*","MeshInstance3D",true,false):
			for i: int in range(mesh.mesh.get_surface_count()):
				var mat: Material = mesh.get_active_material(i)
				if mat is StandardMaterial3D and mat.albedo_texture!=null:
					textured = mat.albedo_texture.get_width()==2048 and mat.normal_enabled and mat.normal_texture!=null and mat.roughness_texture!=null and not mat.uv1_triplanar
					if textured: break
		_check(textured,"%s retains 2K UV colour, normals and roughness maps" % id)
		model.free()
	var gun: Node3D = preload("res://scripts/pistol_visual.gd").new()
	stage.add_child(gun)
	var bounds := AABB()
	var first: bool = true
	var meshes: Array[Node] = gun.find_children("*","MeshInstance3D",true,false)
	for mesh: MeshInstance3D in meshes:
		var box: AABB = mesh.global_transform*mesh.get_aabb()
		bounds = box if first else bounds.merge(box)
		first = false
	_check(meshes.size()==5,"Held weapon contains one complete variant and an inserted magazine; no spare gun or floating bullets")
	_check(bounds.size.z>.21 and bounds.size.z<.24 and bounds.size.x<.04,"Gun has realistic dimensions and faces game -Z without anisotropic stretching")
	_check(absf(bounds.position.z-gun.MUZZLE.z)<.002,"Muzzle ray starts at the model's actual barrel tip")
	gun.pose(0,.3)
	_check(gun.magazine.position.y<gun.magazine_origin.y-.06,"Reload separates the imported magazine from the grip")
	gun.pose(0,-1)
	_check(gun.magazine.visible and gun.magazine.position.is_equal_approx(gun.magazine_origin),"Reload cancellation restores a complete weapon")
	stage.queue_free()
	await process_frame
	print("DETAIL ASSETS: %d checks, %d failures" % [checks,failures])
	quit(0 if failures==0 else 1)
func _check(ok: bool,message: String) -> void:
	checks+=1
	if not ok:
		failures+=1
		push_error("FAIL: "+message)
