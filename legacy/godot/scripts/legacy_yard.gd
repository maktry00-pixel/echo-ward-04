extends Node3D
## A compact 3D test location with broad routes and physical walls.

const Builder = preload("res://scripts/world_builder.gd")
const START: Vector3 = Vector3(0, 0.1, 10)
const EXIT: Vector3 = Vector3(0, 0, -13)
const STRUCTURES: Array[Dictionary] = [
	{"center": Vector3(-4, 0, 0), "size": Vector3(6, 2.8, 4)},
	{"center": Vector3(6, 0, -4), "size": Vector3(4, 2.4, 6)},
	{"center": Vector3(-10, 0, 6), "size": Vector3(3, 1.4, 3)},
	{"center": Vector3(10, 0, 6), "size": Vector3(2.5, 1.8, 2.5)},
]
const CORE_POSITIONS: Array[Vector3] = [
	Vector3(-7, 0, 10), Vector3(7, 0, 10), Vector3(-11, 0, 0),
	Vector3(11, 0, -2), Vector3(-9, 0, -9), Vector3(1, 0, -5),
	Vector3(10, 0, -11), Vector3(-1, 0, -10),
]

var exit_material: StandardMaterial3D
var exit_beam: Node3D
var exit_label: Label3D
var alarm_material: StandardMaterial3D


func _ready() -> void:
	_build_environment()
	var floor_mat := Builder.material("344650")
	var wall_mat := Builder.material("465663")
	var pale := Builder.material("9aa9ad")
	var stripe := Builder.material("ba8448")
	alarm_material = Builder.material("7aa995", 0.4)
	var cyan: StandardMaterial3D = alarm_material
	Builder.box(self, Vector3(31, 0.4, 33), Vector3(0, -0.2, 0), floor_mat, true)
	for x: float in [-15.0, 15.0]:
		Builder.box(self, Vector3(0.8, 3.5, 33), Vector3(x, 1.75, 0), wall_mat, true)
		Builder.box(self, Vector3(0.85, 0.12, 33), Vector3(x, 3.5, 0), cyan)
	for z: float in [-16.0, 16.0]:
		Builder.box(self, Vector3(30, 3.5, 0.8), Vector3(0, 1.75, z), wall_mat, true)
		Builder.box(self, Vector3(30, 0.12, 0.85), Vector3(0, 3.5, z), cyan)
	var grid := Builder.material("41545e")
	for index: int in range(-7, 8):
		Builder.box(self, Vector3(0.025, 0.012, 31), Vector3(index * 2, 0.006, 0), grid)
	for index: int in range(-7, 8):
		Builder.box(self, Vector3(29, 0.012, 0.025), Vector3(0, 0.006, index * 2), grid)
	for structure: Dictionary in STRUCTURES:
		_structure(structure["center"], structure["size"], wall_mat, pale, stripe)
	for z: float in [-9.0, -5.0, 0.0, 5.0, 10.0]:
		Builder.box(self, Vector3(0.08, 0.015, 1.2), Vector3(-0.8, 0.01, z), stripe)
		Builder.box(self, Vector3(0.08, 0.015, 1.2), Vector3(0.8, 0.01, z), stripe)
	_build_exit(pale)
	_sign("АРХИВ 01", Vector3(-4, 3.2, 0), 0.0, 48)
	_sign("ВЫХОД", Vector3(0, 4.4, -13), 0.0, 64)
	_sign("НЕ ВЕРЬ ЭХУ", Vector3(0, 3.9, -15.5), 0.0, 36)
	for location: Vector3 in [Vector3(-13, 2.7, -9), Vector3(13, 2.7, 9), Vector3(-13, 2.7, 9)]:
		Builder.sphere(self, 0.15, location, alarm_material)
		var lamp := OmniLight3D.new()
		lamp.position = location
		lamp.light_color = Color("86b5a2")
		lamp.light_energy = 0.8
		lamp.omni_range = 8
		add_child(lamp)


func set_alarm(stage: int) -> void:
	var color := Color("7aa995")
	if stage == 2:
		color = Color("c69766")
	elif stage >= 3:
		color = Color("c87064")
	alarm_material.albedo_color = color
	alarm_material.emission = color


func set_exit_ready(unlocked: bool) -> void:
	exit_material.albedo_color = Color("65e6b9") if unlocked else Color("d88845")
	exit_material.emission = exit_material.albedo_color
	exit_label.text = "ПЕЧАТЬ СНЯТА" if unlocked else "ПЕЧАТЬ ЗАКРЫТА"
	exit_beam.visible = unlocked


func _structure(location: Vector3, size: Vector3, wall: Material, cap: Material, stripe: Material) -> void:
	Builder.box(self, size, location + Vector3(0, size.y / 2.0, 0), wall, true)
	Builder.box(self, Vector3(size.x + 0.12, 0.15, size.z + 0.12),
		location + Vector3(0, size.y, 0), cap)
	Builder.box(self, Vector3(size.x + 0.02, 0.18, size.z + 0.02),
		location + Vector3(0, 0.3, 0), stripe)
	# Structural ribs add depth and break up the flat prototype boxes.
	for offset: float in [-0.35, 0.35]:
		Builder.box(self, Vector3(0.16, size.y, 0.12),
			location + Vector3(size.x * offset, size.y / 2.0, size.z / 2.0 + 0.04), cap)


func _build_exit(frame: Material) -> void:
	exit_material = Builder.material("d88845", 0.6)
	for x: float in [-2.15, 2.15]:
		Builder.box(self, Vector3(0.5, 4, 0.6), EXIT + Vector3(x, 2, 0), frame, true)
		Builder.box(self, Vector3(0.12, 3.5, 0.64), EXIT + Vector3(x, 2, 0), exit_material)
	Builder.box(self, Vector3(4.8, 0.45, 0.65), EXIT + Vector3(0, 4, 0), frame, true)
	Builder.box(self, Vector3(4, 0.08, 2.8), EXIT + Vector3(0, 0.04, 0), exit_material)
	exit_beam = Builder.box(self, Vector3(3.6, 0.08, 0.1), EXIT + Vector3(0, 3.7, 0), exit_material)
	exit_label = _sign("ПЕЧАТЬ ЗАКРЫТА", EXIT + Vector3(0, 3.2, 0.5), 0, 38)
	set_exit_ready(false)


func _sign(text: String, location: Vector3, yaw: float, size: int) -> Label3D:
	var label := Label3D.new()
	label.text = text
	label.font_size = size
	label.pixel_size = 0.01
	label.position = location
	label.rotation.y = yaw
	label.modulate = Color("bce7e7")
	label.outline_size = 8
	add_child(label)
	return label


func _build_environment() -> void:
	var world := WorldEnvironment.new()
	var environment := Environment.new()
	environment.background_mode = Environment.BG_SKY
	var sky := Sky.new()
	var sky_material := ProceduralSkyMaterial.new()
	sky_material.sky_top_color = Color("070d16")
	sky_material.sky_horizon_color = Color("26383c")
	sky_material.ground_bottom_color = Color("182332")
	sky_material.ground_horizon_color = Color("26383c")
	sky.sky_material = sky_material
	environment.sky = sky
	environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	environment.ambient_light_color = Color("bbd2df")
	environment.ambient_light_energy = 0.32
	environment.fog_enabled = true
	environment.fog_density = 0.014
	environment.fog_light_color = Color("263d3d")
	world.environment = environment
	add_child(world)
	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-52, -35, 0)
	sun.light_color = Color("a3bbca")
	sun.light_energy = 0.4
	sun.shadow_enabled = true
	sun.directional_shadow_max_distance = 70
	add_child(sun)
