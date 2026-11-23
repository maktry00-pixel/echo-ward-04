extends Node3D
## Poly Haven's wooden-grip variant, in metres; barrel points along game -Z.
const Props = preload("res://scripts/imported_props.gd")
const MUZZLE := Vector3(-.00056, -.019, -.174)
var slide: MeshInstance3D
var magazine: MeshInstance3D
var slide_origin: Vector3
var magazine_origin: Vector3

func _init() -> void:
	var model: Node3D = Props.scene("service_pistol")
	model.rotation.y = PI/2
	model.position.y = -.08
	add_child(model)
	for mesh: MeshInstance3D in model.find_children("*","MeshInstance3D",true,false):
		var id: String = str(mesh.name)
		if id.ends_with("_a"):
			if id=="service_pistol_slide_a": slide = mesh
		elif id=="service_pistol_magazine_loaded":
			magazine = mesh
			magazine.position.x = 0 # The source displays the magazine beside the weapon.
		else: mesh.free()
	slide_origin = slide.position
	magazine_origin = magazine.position

func pose(recoil: float, reload_phase: float) -> void:
	slide.position = slide_origin-Vector3(.022*recoil,0,0)
	magazine.position = magazine_origin
	magazine.visible = true
	if reload_phase>=0:
		# Detach, move down into the supporting hand, then insert a full magazine.
		var travel: float = sin(clampf((reload_phase-.08)/.8,0,1)*PI)
		magazine.position.y -= .13*travel
		magazine.visible = reload_phase<.36 or reload_phase>.68
