extends "res://scripts/character_visual.gd"
## Human-derived infected silhouette; guardian retains its distinct creature form.
var chasing: bool = false
var stunned: bool = false
var patient_clock: float = 0.0
func _ready() -> void:
	infected = true
	super._ready()
	get_child(0).scale = Vector3(0.88, 1.18, 0.88)
	for mesh: MeshInstance3D in find_children("*", "MeshInstance3D", true, false):
		for i: int in range(mesh.mesh.get_surface_count()):
			var source: Material = mesh.get_active_material(i)
			if source is StandardMaterial3D:
				var mat: StandardMaterial3D = source.duplicate()
				mat.albedo_color *= Color("849486")
				mat.roughness = 0.97
				mat.metallic = 0
				mesh.set_surface_override_material(i, mat)
func animate(speed: float, _delta: float = 0.0) -> void:
	if player == null:
		return
	var clip: String = "run" if speed > 2.2 else ("walk" if speed > 0.2 else "idle")
	if clip != current_clip:
		player.play(clip, 0.2)
		current_clip = clip
	player.speed_scale = 0.85 if clip == "run" else 0.65
func _process(delta: float) -> void:
	patient_clock += delta
	if skeleton != null:
		skeleton.clear_bones_global_pose_override()
