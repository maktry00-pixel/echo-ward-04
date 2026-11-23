extends Node3D
## A real raised bed collider leaves physical clearance for the prone capsule.
const Builder = preload("res://scripts/world_builder.gd")
var cloth: Material
var metal: Material

func _ready() -> void:
	add_to_group("hide_beds")
	var props = preload("res://scripts/imported_props.gd")
	var textiles = preload("res://scripts/textile_mesh.gd")
	cloth = preload("res://scripts/environment_materials.gd").pbr("stretch_poplin","b9b9a8",3.5)
	# Bleached hospital linen: retain the scanned weave/roughness, omit its lime dye.
	cloth.albedo_texture = null
	cloth.normal_scale = .22
	props.visual(self,"old_bed_frame",Vector3(1.5,1.4,2.3))
	Builder.box(self,Vector3(1.5,.10,2.3),Vector3(0,.63,0),metal,true)
	textiles.cushion(self,Vector3(1.4,.20,2.18),Vector3(0,.79,0),cloth,.085)
	textiles.cushion(self,Vector3(.92,.16,.43),Vector3(0,.965,-.73),cloth,.07)
	var sheets: Material = preload("res://scripts/environment_materials.gd").pbr("stretch_poplin","8e9ca1",3.5)
	sheets.albedo_texture = null
	sheets.normal_scale = .22
	textiles.blanket(self,sheets)
	for side: float in [-1,1]:
		for z: float in [-1,1]:
			props.body(self,Vector3(.065,.58,.065),Vector3(side*.64,.29,z),"metal")
	for z: float in [-1.07,1.07]:
		props.body(self,Vector3(1.45,.05,.05),Vector3(0,1.36,z),"metal")
		for side: float in [-.68,.68]: props.body(self,Vector3(.06,1.4,.06),Vector3(side,.70,z),"metal")

func prompt() -> String:
	return "F — спрятаться под кроватью"

func exit_points() -> Array[Vector3]:
	return [to_global(Vector3(-1.2,.05,0)),to_global(Vector3(1.2,.05,0))]
