extends Node3D
## A complete indoor wing: reception, wards, archive and isolation rooms.

signal layout_changed

const Builder = preload("res://scripts/world_builder.gd")
const Props = preload("res://scripts/imported_props.gd")
const Materials = preload("res://scripts/environment_materials.gd")
const Door = preload("res://scripts/institution_door.gd")
const Creature = preload("res://scripts/horror_creature.gd")
const Research = preload("res://scripts/research_wing.gd")
var residential: Node3D
const START: Vector3 = Vector3(0, 0.1, 12)
const EXIT: Vector3 = Vector3(0, 0, 14)
const CORE_POSITIONS: Array[Vector3] = [
	Vector3(-9, 0, 11), Vector3(9, 0, 11), Vector3(-11, 0, 4), Vector3(11, 0, 4),
	Vector3(-11, 0, -5), Vector3(11, 0, -5), Vector3(-10, 0, -12), Vector3(10, 0, -12),
	Vector3(91,0,-3),Vector3(64,0,20),
]
const STRUCTURES: Array[Dictionary] = [
	{"center": Vector3(-3, 0, -14.65), "size": Vector3(0.35, 3.6, 2.7)},
	{"center": Vector3(3, 0, -14.65), "size": Vector3(0.35, 3.6, 2.7)},
	{"center": Vector3(-3, 0, -8.5), "size": Vector3(0.35, 3.6, 4.4)},
	{"center": Vector3(3, 0, -8.5), "size": Vector3(0.35, 3.6, 4.4)},
	{"center": Vector3(-3, 0, 0), "size": Vector3(0.35, 3.6, 7.4)},
	{"center": Vector3(3, 0, 0), "size": Vector3(0.35, 3.6, 7.4)},
	{"center": Vector3(-3, 0, 7.15), "size": Vector3(0.35, 3.6, 1.7)},
	{"center": Vector3(3, 0, 7.15), "size": Vector3(0.35, 3.6, 1.7)},
	{"center": Vector3(-5.35, 0, 8), "size": Vector3(4.7, 3.6, 0.35)},
	{"center": Vector3(-12.65, 0, 8), "size": Vector3(4.7, 3.6, 0.35)},
	{"center": Vector3(5.35, 0, 8), "size": Vector3(4.7, 3.6, 0.35)},
	{"center": Vector3(12.65, 0, 8), "size": Vector3(4.7, 3.6, 0.35)},
	{"center": Vector3(-5.35, 0, -1), "size": Vector3(4.7, 3.6, 0.35)},
	{"center": Vector3(-12.65, 0, -1), "size": Vector3(4.7, 3.6, 0.35)},
	{"center": Vector3(5.35, 0, -1), "size": Vector3(4.7, 3.6, 0.35)},
	{"center": Vector3(12.65, 0, -1), "size": Vector3(4.7, 3.6, 0.35)},
	{"center": Vector3(-5.35, 0, -9), "size": Vector3(4.7, 3.6, 0.35)},
	{"center": Vector3(-12.65, 0, -9), "size": Vector3(4.7, 3.6, 0.35)},
	{"center": Vector3(5.35, 0, -9), "size": Vector3(4.7, 3.6, 0.35)},
	{"center": Vector3(12.65, 0, -9), "size": Vector3(4.7, 3.6, 0.35)},
]

var doors: Array[Node3D] = []
var exit_material: StandardMaterial3D
var exit_beam: Node3D
var exit_label: Label3D
var alarm_material: StandardMaterial3D
var lights: Array[Light3D] = []
var light_phases: Array[float] = []
var light_colors: Array[Color] = []
var light_energies: Array[float] = []
var atmosphere_time: float = 0.0
var stage: int = 1
var blackout_remaining: float = 0.0
var wall_material: StandardMaterial3D
var paint_material: StandardMaterial3D
var ceiling_material: StandardMaterial3D
var metal_material: StandardMaterial3D
var power_on: bool = false
var returned_changed: bool = false
var silhouette: Node3D
var silhouette_time: float = -1.0
var return_prop: Node3D
var gate_by_id: Dictionary = {}
var map_walls: Array[Dictionary] = []
var machine_rotors: Array[Node3D] = []
var return_message: Label3D
var exit_panels: Array[Node3D] = []
var unease_location: Vector3
var unease_remaining: float = 0.0


func _ready() -> void:
	_build_environment()
	wall_material = Materials.pbr("painted_plaster_wall", "c9c4b2", .4)
	paint_material = Materials.pbr("blue_plaster_weathered", "859eaa", .5)
	ceiling_material = Materials.pbr("painted_plaster_wall", "c4c5bc", .3)
	ceiling_material.normal_scale = .14
	metal_material = Builder.material("343e3c")
	metal_material.metallic = 0.55
	metal_material.roughness = 0.45
	alarm_material = Builder.material("cc704d", 0.7)
	var floor_mat := Materials.pbr("terrazzo_tiles", "a8b0ac", .5,.9)
	var floor_body: Node3D = Builder.box(self, Vector3(46, 0.4, 33), Vector3(-7.5, -0.2, 0), floor_mat, true)
	for child: Node in floor_body.get_children():
		if child is MeshInstance3D:
			child.hide()
	# Separate floor panels receive their local lamps correctly on OpenGL.
	for x: int in range(-30, 15, 4):
		for z: int in range(-14, 15, 4):
			Builder.box(self, Vector3(4, 0.04, 4), Vector3(x, -0.02, z), floor_mat)
	Builder.box(self, Vector3(46, 0.25, 33), Vector3(-7.5, 3.72, 0), ceiling_material, true)
	_wall(Vector3(15,0,-8.3),Vector3(.8,3.6,3.4))
	_wall(Vector3(15,0,6.3),Vector3(.8,3.6,19.4))
	_wall(Vector3(15, 0, -15.0), Vector3(0.8, 3.6, 2))
	# Two openings connect the new service annex to the original wing.
	for pair: Vector2 in [Vector2(-6, 20), Vector2(8, 3.4), Vector2(14.15, 3.7)]:
		_wall(Vector3(-15, 0, pair.x), Vector3(0.8, 3.6, pair.y))
	_wall(Vector3(-8.3,0,-16),Vector3(13.4,3.6,.8))
	_wall(Vector3(8.3,0,-16),Vector3(13.4,3.6,.8))
	for x: float in [-8.3]:
		_wall(Vector3(x, 0, 16), Vector3(13.4, 3.6, 0.8))
	_wall(Vector3(4.1,0,16),Vector3(5,3.6,.8))
	_wall(Vector3(12.1,0,16),Vector3(5.8,3.6,.8))
	Builder.box(self, Vector3(3.2, 0.6, 0.8), Vector3(0, 3.3, 16), wall_material, true)
	for part: Dictionary in STRUCTURES:
		_wall(part["center"], part["size"])
	_build_doorways()
	_build_furniture()
	_build_details()
	preload("res://scripts/opening_art.gd").build(self)
	_build_service_wing()
	Research.build(self)
	residential = preload("res://scripts/residential_wing.gd").new()
	residential.facility = self
	add_child(residential)
	_add_door("research_gate", Vector3(15, 0, -12), PI / 2, "badge", 3.2)
	_build_exit()
	for z: float in [12.0, 5.0, -3.0, -11.0]:
		_lamp(Vector3(0, 3.35, z), Color("b8c8ac"), 0.85, 6.5)
	for x: float in [-10.0, 10.0]:
		for z: float in [11.0, 4.0, -5.0, -12.0]:
			_lamp(Vector3(x, 3.35, z), Color("9eb5a7"), 0.55, 6.5)
	_sign("ПРИЁМНОЕ ОТДЕЛЕНИЕ", Vector3(-7, 2.8, 15.5), PI, 25)
	_sign("ПАЛАТА 04  ←     ПАЛАТА 05  →", Vector3(0, 3.1, 7.8), 0, 24)
	_sign("АРХИВ", Vector3(0, 3.3, -0.75), 0, 24)
	_sign("КАРАНТИН", Vector3(0, 3.3, -8.75), 0, 26)
	_sign("ИЗОЛЯТОР", Vector3(0, 3.05, -13), 0, 28)
	_sign("НЕ ОТКРЫВАЙТЕ", Vector3(-3.22, 2.25, -8), -PI / 2, 24)


func _wall(location: Vector3, size: Vector3) -> void:
	map_walls.append({"center": location, "size": size})
	Builder.box(self, size, location + Vector3(0, size.y / 2.0, 0), wall_material, true)
	Builder.box(self, Vector3(size.x + 0.035, 1.12, size.z + 0.035),
		location + Vector3(0, 0.56, 0), paint_material)
	Builder.box(self, Vector3(size.x + 0.07, 0.055, size.z + 0.07),
		location + Vector3(0, 1.14, 0), metal_material)
	Builder.box(self, Vector3(size.x + 0.07, 0.12, size.z + 0.07),
		location + Vector3(0, 0.06, 0), metal_material)


func _build_doorways() -> void:
	for x: float in [-9.0, 9.0]:
		for z: float in [-1.0, -9.0]:
			_add_door("wing_%d_%d" % [x, z], Vector3(x, 0, z), 0, "power" if z == -1 else "badge")
	_add_door("power_gate", Vector3(0, 0, -1), 0, "power", 6.0)
	_add_door("quarantine_gate", Vector3(0, 0, -9), 0, "badge", 6.0)
	_add_door("service_entry", Vector3(-15, 0, 5), PI / 2, "", 2.4, true)
	_add_door("service_shortcut", Vector3(-15, 0, 11), PI / 2, "inside")
	_add_door("archive_shortcut", Vector3(-3, 0, -5), PI / 2, "inside")
	_add_door("supply_room", Vector3(-23, 0, 11), PI / 2, "")
	# Door frames on corridor openings let rooms have readable entrances.
	for x: float in [-3.0, 3.0]:
		for z: float in [5.0, -5.0, -12.0]:
			for sign: float in [-1.0, 1.0]:
				Builder.box(self, Vector3(0.25, 3.2, 0.14), Vector3(x, 1.6, z + sign * 1.3), metal_material)
			Builder.box(self, Vector3(0.25, 0.35, 2.6), Vector3(x, 3.35, z), metal_material)


func _add_door(id: String, location: Vector3, yaw: float, lock: String, width: float = 2.4, open: bool = false) -> Node3D:
	var door := Door.new()
	door.door_id = id
	door.position = location
	door.rotation.y = yaw
	door.width = width
	door.lock_kind = lock
	door.initial_locked = not lock.is_empty()
	door.initial_open = open
	add_child(door)
	door.changed.connect(func() -> void: layout_changed.emit())
	doors.append(door)
	gate_by_id[id] = door
	return door


func apply_power() -> void:
	power_on = true
	for door: Node3D in doors:
		if door.lock_kind == "power":
			door.locked = false
			if not door.is_open:
				door.toggle_door(door.global_position + Vector3(0, 0, 3))
	silhouette_time = 0
	silhouette.show()
	brief_blackout()


func _build_service_wing() -> void:
	_wall(Vector3(-30, 0, 4), Vector3(0.8, 3.6, 24))
	_wall(Vector3(-22.5, 0, 16), Vector3(15, 3.6, 0.8))
	# Observation window is solid, with an unreachable silhouette behind it.
	_wall(Vector3(-28.85, 0, -8), Vector3(2.3, 3.6, 0.4))
	_wall(Vector3(-19.85, 0, -8), Vector3(9.7, 3.6, 0.4))
	Builder.box(self, Vector3(3.2, 0.9, 0.4), Vector3(-26.1, 0.45, -8), paint_material, true)
	Builder.box(self, Vector3(3.2, 0.5, 0.4), Vector3(-26.1, 3.35, -8), wall_material, true)
	var glass := Builder.material("78958c")
	glass.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	glass.albedo_color.a = 0.22
	Builder.box(self, Vector3(3.2, 2.2, 0.08), Vector3(-26.1, 2.0, -8), glass, true)
	for x: float in [-27.6, -26.1, -24.6]:
		Builder.box(self, Vector3(0.035, 2.2, 0.15), Vector3(x, 2, -7.9), metal_material)
	Builder.box(self, Vector3(5, 3.6, 0.3), Vector3(-26.1, 1.8, -9.5), paint_material)
	silhouette = Creature.new()
	silhouette.position = Vector3(-27.5, 0.1, -8.7)
	add_child(silhouette)
	silhouette.hide()
	var backlight := OmniLight3D.new()
	backlight.position = Vector3(-26, 2.7, -8.8)
	backlight.light_energy = 0.7
	backlight.light_color = Color("bac4b2")
	backlight.omni_range = 4
	add_child(backlight)
	# Supply room is optional, guarded and separated from the generator route.
	_wall(Vector3(-23, 0, 8.15), Vector3(0.35, 3.6, 3.1))
	_wall(Vector3(-23, 0, 14.15), Vector3(0.35, 3.6, 3.7))
	_wall(Vector3(-26.5, 0, 6.6), Vector3(7, 3.6, 0.35))
	for x: float in [-28.5, -18.5]:
		_lamp(Vector3(x, 3.35, 12), Color("b8ad80"), 0.6, 7)
	_lamp(Vector3(-23, 3.35, 3), Color("abb99d"), 0.6, 8)
	_lamp(Vector3(-25, 3.35, -5), Color("abb99d"), 0.6, 6)
	for x: float in [-21.5, -18.0]:
		Builder.box(self, Vector3(1.6, 2.4, 1.0), Vector3(x, 1.2, -6.8), metal_material, true)
		for y: float in [0.7, 1.3, 1.9]:
			Builder.box(self, Vector3(1.35, 0.045, 0.04), Vector3(x, y, -6.27), alarm_material)
		var rotor := Node3D.new()
		rotor.position = Vector3(x, 1.4, -6.23)
		add_child(rotor)
		Builder.box(rotor, Vector3(0.75, 0.06, 0.05), Vector3.ZERO, metal_material)
		Builder.box(rotor, Vector3(0.06, 0.75, 0.05), Vector3.ZERO, metal_material)
		machine_rotors.append(rotor)
	_sign("СЛУЖЕБНОЕ КРЫЛО", Vector3(-15.6, 2.75, 5), PI / 2, 23)
	_sign("МЕДСКЛАД — ПОСТОРОННИМ ВХОД ЗАПРЕЩЁН", Vector3(-22.75, 2.6, 11), PI / 2, 17)
	_sign("ЭЛЕКТРОЩИТОВАЯ  ↑", Vector3(-22, 2.8, 0), 0, 22)
	_sign("ЗАСОВ С ДРУГОЙ СТОРОНЫ", Vector3(-14.65, 2.65, 11), PI / 2, 15)
	return_prop = Node3D.new()
	return_prop.position = Vector3(5.5, 0, 12)
	add_child(return_prop)
	Builder.box(return_prop, Vector3(0.75, 0.12, 0.65), Vector3(0, 0.45, 0), paint_material)
	Builder.box(return_prop, Vector3(0.75, 0.7, 0.12), Vector3(0, 0.8, 0.28), paint_material)
	return_prop.hide()
	return_message = _sign("ТЫ УЖЕ БЫЛ ЗДЕСЬ", Vector3(4.5, 1.7, 15.5), PI, 24)
	return_message.modulate = Color("935e4c")
	return_message.hide()


func play_return_event() -> void:
	returned_changed = true
	return_prop.show()
	return_prop.rotation.z = PI / 2.2
	return_prop.position.y = 0.3
	return_message.show()
	brief_blackout()


func _build_furniture() -> void:
	var wood := Materials.surface("concrete", "675443", 0.8)
	var cloth := Materials.surface("concrete", "888b78", 0.9)
	# Reception counter, file cabinets and waiting seats sit off the main route.
	preload("res://scripts/opening_art.gd").counter(self)
	for x: float in [6,10.5]:
		Props.detail(self,"sofa_03",2.6,Vector3(x,0,14.35),Vector3(0,PI,0))
		Props.body(self,Vector3(2.6,.65,.83),Vector3(x,.325,14.35))
	_chair(Vector3(-13,0,13.9),metal_material,cloth)
	_desk_lamp(Vector3(-10.5,.94,14),.5)
	for x: float in [-11.5, -7.0, 7.0, 11.5]:
		# Long sides face the room. Keep the wall gap closed and the drawer aisle wide.
		_bed(Vector3(x, 0, 6.9), cloth, PI/2)
	for side: float in [-1,1]:
		_window(Vector3(side*14.48,2.3,4.4),side)
		_desk_lamp(Vector3(side*11,.87,4),side*.6)
	for x: float in [-13.6,-12.3,-11.0,-9.7,-8.4]:
		Props.body(self,Vector3(1.1,2.6,.7),Vector3(x,1.3,-7.8),"metal")
		Props.visual(self,"steel_frame_shelves_01",Vector3(1.1,2.6,.7),Vector3(x,0,-7.8))
	_table(Vector3(8, 0, -6.7), Vector3(3.6, 0.9, 1.5), metal_material)
	_table(Vector3(-11.5, 0, -13.9), Vector3(3.5, 0.8, 1.2), wood)
	_bed(Vector3(12.4, 0, -14), cloth)
	# Pipework, screens and documents provide scale and environmental detail.
	for x: float in [-2.45, 2.45]:
		_pipe(Vector3(x, 3.3, -15), Vector3(x, 3.3, 15), 0.06, metal_material)
	for z: float in [9.0, 1.0, -7.0]:
		_pipe(Vector3(-14, 3.3, z), Vector3(14, 3.3, z), 0.04, metal_material)
	var paper := Builder.material("b9b09a")
	for index: int in range(24):
		var x: float = sin(index * 7.31) * 12.8
		var z: float = cos(index * 4.71) * 14
		var sheet: Node3D = Builder.box(self, Vector3(0.23, 0.008, 0.34), Vector3(x, 0.012, z), paper)
		sheet.rotation.y = index * 0.71
	var dark_stain := Builder.material("2e2420")
	dark_stain.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	dark_stain.albedo_color.a = 0.65
	for location: Vector3 in [Vector3(-1, 0.014, 7), Vector3(10, 0.014, 1.2), Vector3(-8, 0.014, -10)]:
		var stain: MeshInstance3D = Builder.sphere(self, 0.8, location, dark_stain)
		stain.scale = Vector3(1.8, 0.004, 0.8)
		stain.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF


func _bed(location: Vector3, cloth: Material, yaw: float = 0) -> void:
	var bed := preload("res://scripts/hide_bed.gd").new()
	bed.position = location
	bed.rotation.y = yaw
	bed.cloth = cloth
	bed.metal = metal_material
	add_child(bed)


func _table(location: Vector3, size: Vector3, _material: Material) -> void:
	# Modular desks keep credible proportions instead of stretching one desk to 5m.
	var columns: int = maxi(1,ceili(size.x/1.9))
	var rows: int = maxi(1,ceili(size.z/1.5))
	var cell := Vector3(size.x/columns,size.y+.05,size.z/rows)
	for ix: int in range(columns):
		for iz: int in range(rows):
			var at: Vector3 = location+Vector3((ix-(columns-1)*.5)*cell.x,0,(iz-(rows-1)*.5)*cell.z)
			Props.visual(self,"metal_office_desk",cell,at)
			Props.body(self,Vector3(cell.x,.10,cell.z),at+Vector3(0,size.y,0),"metal")
			for x: float in [-cell.x/2+.12,cell.x/2-.12]:
				for z: float in [-cell.z/2+.12,cell.z/2-.12]:
					Props.body(self,Vector3(.12,size.y,.12),at+Vector3(x,size.y/2,z),"metal")

func _chair(location: Vector3, _frame: Material, _cloth: Material) -> void:
	Props.detail(self,"SchoolChair_01",.95,location)
	Props.body(self,Vector3(.52,.46,.52),location+Vector3(0,.23,0))
	Props.body(self,Vector3(.52,.45,.09),location+Vector3(0,.68,.21))

func _desk_lamp(at: Vector3, yaw: float) -> void:
	Props.detail(self,"desk_lamp_arm_01",.62,at,Vector3(0,yaw,0))
	var light := OmniLight3D.new()
	light.position = at+Vector3(0,.42,0)
	light.light_color = Color("ffd19a")
	light.light_energy = .55
	light.omni_range = 3.8
	light.shadow_enabled = false # Soft local fill; ceiling fixtures provide the room shadows.
	light.distance_fade_enabled = true
	light.distance_fade_begin = 15
	light.distance_fade_length = 6
	add_child(light)

func _window(at: Vector3, side: float) -> void:
	var glass := Builder.material("274454",.16)
	glass.roughness = .35
	Builder.box(self,Vector3(.035,1.8,2.65),at,glass)
	for z: float in [-1.4,1.4]:
		Builder.box(self,Vector3(.18,2.0,.11),at+Vector3(-side*.05,0,z),metal_material)
	for y: float in [-.94,.94]:
		Builder.box(self,Vector3(.22,.10,2.85),at+Vector3(-side*.05,y,0),metal_material)
	var blind := Builder.material("75827e")
	for i: int in range(10):
		var slat: Node3D = Builder.box(self,Vector3(.19,.035,2.65),at+Vector3(-side*.23,-.78+i*.175,0),blind)
		slat.rotation.z = side*.22
		(slat.get_child(0) as MeshInstance3D).cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_ON
	var moon := SpotLight3D.new()
	add_child(moon)
	moon.look_at_from_position(at+Vector3(-side*.10,.35,0),at+Vector3(-side*6,-2.3,2.7))
	moon.light_color = Color("8eb8d0")
	moon.light_energy = 1.5
	moon.spot_range = 12
	moon.spot_angle = 42
	moon.shadow_enabled = true
	moon.shadow_bias = .035
	moon.shadow_normal_bias = .25
	moon.distance_fade_enabled = true
	moon.distance_fade_begin = 20
	moon.distance_fade_length = 8


func _pipe(a: Vector3, b: Vector3, radius: float, material: Material) -> void:
	var mesh := CylinderMesh.new()
	mesh.top_radius = radius
	mesh.bottom_radius = radius
	mesh.height = a.distance_to(b)
	mesh.radial_segments = 10
	var visual := MeshInstance3D.new()
	visual.mesh = mesh
	visual.material_override = material
	visual.position = (a + b) / 2
	visual.quaternion = Quaternion(Vector3.UP, (b - a).normalized())
	add_child(visual)


func _lamp(location: Vector3, color: Color, energy: float, radius: float) -> void:
	Builder.box(self, Vector3(1.0, 0.12, 0.28), location, metal_material)
	var glow := Builder.material("c4cfb4", 1.0)
	Builder.box(self, Vector3(0.86, 0.025, 0.2), location + Vector3(0, -0.075, 0), glow)
	var light := SpotLight3D.new()
	light.position = location + Vector3(0, -0.2, 0)
	light.light_color = color
	light.light_energy = energy*2.4
	light.rotation.x = -PI/2
	light.spot_range = radius
	light.spot_angle = 66
	light.spot_attenuation = .8
	light.shadow_enabled = true
	light.shadow_bias = .04
	light.shadow_normal_bias = .45
	light.distance_fade_enabled = true
	light.distance_fade_begin = 20
	light.distance_fade_length = 8
	light.distance_fade_shadow = 18
	add_child(light)
	lights.append(light)
	light_phases.append(float(lights.size()) * 1.7)
	light_colors.append(color)
	light_energies.append(energy*2.4)


func _process(delta: float) -> void:
	atmosphere_time += delta
	blackout_remaining = maxf(0.0, blackout_remaining - delta)
	unease_remaining = maxf(0, unease_remaining - delta)
	if power_on:
		for rotor: Node3D in machine_rotors:
			rotor.rotate_z(delta * 4.0)
	if silhouette_time >= 0:
		silhouette_time += delta
		silhouette.position.x = -27.5 + silhouette_time * 0.8
		silhouette.rotation.y = -PI / 2
		silhouette.animate(2.0)
		if silhouette_time > 3.4:
			silhouette.hide()
			silhouette_time = -1
	for index: int in range(lights.size()):
		var phase: float = atmosphere_time + light_phases[index]
		var flicker: float = 0.2 if fmod(phase, 13.0) < 0.15 else 1.0
		lights[index].light_energy = 0.0 if blackout_remaining > 0 else light_energies[index] * (1.0 if stage < 3 else 0.62) * flicker
		lights[index].light_color = Color("d28b6f") if stage >= 3 else light_colors[index]
		if power_on and stage < 3:
			lights[index].light_energy *= 1.45
		if unease_remaining > 0 and lights[index].position.distance_to(unease_location) < 6.5:
			lights[index].light_energy *= 0.45 + 0.1 * sin(atmosphere_time * 2)


func brief_blackout() -> void:
	blackout_remaining = 1.8


func _build_details() -> void:
	# Curtains, wall boards and abandoned equipment distinguish the rooms.
	var fabric: StandardMaterial3D = Materials.pbr("stretch_poplin","718b8c",3.5)
	fabric.albedo_texture = null
	fabric.normal_scale = .22
	fabric.cull_mode = BaseMaterial3D.CULL_DISABLED
	for x: float in [-5.8, 5.8]:
		_pipe(Vector3(x, 0, 2.2), Vector3(x, 2.25, 2.2), 0.035, metal_material)
		_pipe(Vector3(x, 2.25, 2.2), Vector3(x, 2.25, 5.0), 0.035, metal_material)
		preload("res://scripts/textile_mesh.gd").curtain(self,Vector3(x,0,2.3),fabric)
	var paper := Builder.material("bfb69c")
	for side: float in [-1.0, 1.0]:
		Builder.box(self, Vector3(0.05, 1.0, 1.5), Vector3(side * 2.77, 1.95, 0.7), metal_material)
		for index: int in range(5):
			Builder.box(self, Vector3(0.02, 0.28, 0.22),
				Vector3(side * 2.73, 1.8 + (index % 2) * 0.32, 0.15 + index * 0.25), paper)
	# A wheeled IV stand and a covered examination table in quarantine.
	_pipe(Vector3(7, 0.08, -13), Vector3(7, 2.2, -13), 0.025, metal_material)
	_pipe(Vector3(6.7, 2.2, -13), Vector3(7.3, 2.2, -13), 0.025, metal_material)
	Builder.box(self, Vector3(0.22, 0.4, 0.09), Vector3(7.25, 1.92, -13), fabric)
	for angle: float in [0.0, TAU / 3, TAU * 2 / 3]:
		_pipe(Vector3(7, 0.08, -13), Vector3(7 + cos(angle) * 0.35, 0.08, -13 + sin(angle) * 0.35), 0.025, metal_material)
	_sign("04", Vector3(-2.78, 2.5, 3.4), PI / 2, 34)
	_sign("05", Vector3(2.78, 2.5, 3.4), -PI / 2, 34)


func set_alarm(value: int) -> void:
	stage = value
	if stage == 1:
		blackout_remaining = 0.0
	alarm_material.albedo_color = Color("e99467") if stage >= 3 else Color("a6ba99")
	alarm_material.emission = alarm_material.albedo_color


func reset_doors() -> void:
	unease_remaining = 0
	power_on = false
	returned_changed = false
	silhouette_time = -1
	silhouette.hide()
	return_prop.hide()
	return_message.hide()
	for rotor: Node3D in machine_rotors:
		rotor.rotation = Vector3.ZERO
	for door: Node3D in doors:
		door.reset_door()
	layout_changed.emit()


func set_exit_ready(unlocked: bool) -> void:
	exit_material.albedo_color = Color("9fc994") if unlocked else Color("a77855")
	exit_material.emission = exit_material.albedo_color
	exit_label.text = "ВЫХОД ОТКРЫТ" if unlocked else "АВАРИЙНАЯ БЛОКИРОВКА"
	exit_beam.visible = unlocked
	for index: int in range(exit_panels.size()):
		exit_panels[index].position.x = (-1.0 if index == 0 else 1.0) * (2.5 if unlocked else 0.8)


func _build_exit() -> void:
	exit_material = Builder.material("a77855", 0.6)
	for x: float in [-1.75, 1.75]:
		Builder.box(self, Vector3(0.2, 3, 0.25), EXIT + Vector3(x, 1.5, 0), metal_material)
	Builder.box(self, Vector3(3.7, 0.2, 0.25), EXIT + Vector3(0, 3, 0), metal_material)
	Builder.box(self, Vector3(0.4, 0.4, 0.05), EXIT + Vector3(1.95, 1.2, 0.1), exit_material)
	exit_beam = Builder.box(self, Vector3(3.2, 0.035, 0.4), EXIT + Vector3(0, 0.03, 0), exit_material)
	exit_label = _sign("АВАРИЙНАЯ БЛОКИРОВКА", EXIT + Vector3(0, 2.65, 0.1), PI, 18)
	for x: float in [-0.8, 0.8]:
		var panel: Node3D = Builder.box(self, Vector3(1.58, 2.8, 0.13), Vector3(x, 1.4, 15.35), metal_material, true)
		exit_panels.append(panel)
	set_exit_ready(false)


func _sign(text: String, location: Vector3, yaw: float, size: int) -> Label3D:
	var label := Label3D.new()
	label.text = text
	label.font_size = size
	label.pixel_size = 0.01
	label.position = location
	label.rotation.y = yaw
	label.modulate = Color("c1c5a9")
	label.outline_size = 3
	label.set_draw_flag(Label3D.FLAG_DOUBLE_SIDED, false)
	add_child(label)
	return label


func _build_environment() -> void:
	var world := WorldEnvironment.new()
	var environment := Environment.new()
	environment.background_mode = Environment.BG_COLOR
	environment.background_color = Color("050909")
	environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	environment.ambient_light_color = Color("9eaebc")
	environment.ambient_light_energy = 0.16
	environment.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	environment.fog_enabled = true
	environment.fog_density = 0.009
	environment.fog_light_color = Color("151e1b")
	world.environment = environment
	add_child(world)
