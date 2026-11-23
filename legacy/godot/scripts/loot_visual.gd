extends RefCounted
## Actual silhouettes for each reward. Source UV textures stay intact.
const Props = preload("res://scripts/imported_props.gd")
const Builder = preload("res://scripts/world_builder.gd")

static func build(parent: Node3D, id: String, kind: String) -> void:
	if kind=="shells":
		for i: int in range(3): preload("res://scripts/shotgun_visual.gd").shell_visual(parent,Vector3((i-1)*.029,.012,0))
	elif kind=="boards":
		for i: int in range(2): Builder.box(parent,Vector3(.48,.028,.085),Vector3(0,.025+i*.03,0),preload("res://scripts/wood_surface.gd").make()).rotation.y = i*.12
	elif id.begins_with("ammo_"):
		Props.detail(parent,"service_pistol",.11,Vector3(-.035,0,0),Vector3(PI/2,0,.18),"service_pistol_magazine_loaded")
		for i: int in range(6):
			Props.detail(parent,"service_pistol",.029,Vector3(.025+(i%2)*.014,.002,(i/2-1)*.035),Vector3(PI/2,0,0),"service_pistol_bullet")
	elif id=="residence_prybar":
		Props.detail(parent,"crowbar_01",.29,Vector3.ZERO,Vector3(PI/2,0,0))
	elif id in ["coolant","fuel"]:
		Props.detail(parent,"plastic_jerrycan",.18)
	elif id in ["fuse","residence_core"]:
		Props.detail(parent,"circuit_board",.16)
		# Ceramic cartridge and conducting end caps on a real textured circuit board.
		var fuse := CylinderMesh.new()
		fuse.top_radius = .012
		fuse.bottom_radius = .012
		fuse.height = .072
		var ceramic := MeshInstance3D.new()
		ceramic.mesh = fuse
		ceramic.material_override = Builder.material("d2c8a9")
		ceramic.rotation.z = PI/2
		ceramic.position = Vector3(0,.04,0)
		parent.add_child(ceramic)
		for x: float in [-.039,.039]:
			Builder.box(parent,Vector3(.018,.028,.028),Vector3(x,.04,0),Builder.material("8e998f"))
	elif id=="bottle":
		var glass: StandardMaterial3D = Builder.material("54736c")
		glass.roughness = .2
		var mesh := CylinderMesh.new()
		mesh.top_radius = .035
		mesh.bottom_radius = .045
		mesh.height = .21
		mesh.radial_segments = 20
		var bottle := MeshInstance3D.new()
		bottle.mesh = mesh
		bottle.position.y = .105
		bottle.material_override = glass
		parent.add_child(bottle)
		Builder.box(parent,Vector3(.025,.065,.025),Vector3(0,.24,0),glass)
	elif id=="badge":
		card(parent,"res://assets/textures/props/staff_badge.svg",Vector3(.105,.003,.15))
	elif kind=="battery":
		var body: MeshInstance3D = card(parent,"res://assets/textures/props/battery_label.svg",Vector3(.055,.025,.085))
		body.material_override.roughness = .52
		for x: float in [-.014,.014]:
			var terminal := CylinderMesh.new()
			terminal.top_radius = .005 if x<0 else .007
			terminal.bottom_radius = terminal.top_radius
			terminal.height = .007
			var contact := MeshInstance3D.new()
			contact.mesh = terminal
			contact.material_override = Builder.material("a4ada5")
			contact.position = Vector3(x,.014,-.045)
			contact.rotation.x = PI/2
			parent.add_child(contact)
	elif kind=="medkit":
		Props.detail(parent,"medical_box",.16,Vector3(-.012,0,0))
		Props.detail(parent,"medical_tape",.042,Vector3(.055,.025,.025))
	else:
		Props.detail(parent,"portable_cassette_player",.14,Vector3.ZERO,Vector3(-PI/2,0,0),"portable_cassette_player_casette")

static func card(parent: Node3D, texture: String, size: Vector3) -> MeshInstance3D:
	var mat := StandardMaterial3D.new()
	mat.albedo_color = Color("9e988a")
	mat.roughness = .83
	var body := MeshInstance3D.new()
	var box := BoxMesh.new()
	box.size = size
	body.mesh = box
	body.material_override = mat
	body.position.y = size.y/2
	parent.add_child(body)
	var face := MeshInstance3D.new()
	var plane := PlaneMesh.new()
	plane.size = Vector2(size.x,size.z)
	face.mesh = plane
	var printed := StandardMaterial3D.new()
	printed.albedo_texture = load(texture)
	printed.roughness = .72
	printed.texture_filter = BaseMaterial3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC
	face.material_override = printed
	face.position.y = size.y+.0003
	parent.add_child(face)
	return body
