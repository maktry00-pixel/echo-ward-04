extends CanvasLayer
## Original 3D still-life at dawn, rendered behind the final result panel.
const B = preload("res://scripts/world_builder.gd")
const P = preload("res://scripts/imported_props.gd")
var viewport_3d: SubViewport
var evidence_present: bool = false
func present(evidence_found: bool, isolation_proof: bool = false) -> void:
	evidence_present = evidence_found
	process_mode = Node.PROCESS_MODE_ALWAYS
	layer = -1
	var frame := SubViewportContainer.new()
	frame.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	frame.stretch = true
	frame.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(frame)
	viewport_3d = SubViewport.new()
	viewport_3d.size = Vector2i(1280,900)
	viewport_3d.own_world_3d = true
	viewport_3d.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	frame.add_child(viewport_3d)
	var world := Node3D.new()
	viewport_3d.add_child(world)
	var environment := WorldEnvironment.new()
	var env := Environment.new()
	env.background_mode = Environment.BG_COLOR
	env.background_color = Color("778a91")
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color = Color("b7c9cf")
	env.ambient_light_energy = .4
	environment.environment = env
	world.add_child(environment)
	var floor_mat = preload("res://scripts/environment_materials.gd").pbr("terrazzo_tiles","8e9998",.5)
	B.box(world,Vector3(12,.2,12),Vector3(0,-.1,0),floor_mat)
	P.visual(world,"metal_office_desk",Vector3(2.2,.85,1.4))
	P.detail(world,"portable_cassette_player",.34,Vector3(-.68,.88,.12),Vector3(0,-.2,0))
	P.detail(world,"small_plastic_torch",.30,Vector3(-.55,.88,-.2),Vector3(0,.6,PI/2))
	B.box(world,Vector3(.18,.008,.12),Vector3(-.55,.885,.42),B.material("778a7e"))
	var personal := Node3D.new()
	world.add_child(personal)
	personal.position = Vector3(-.6,.89,.43)
	preload("res://scripts/loot_visual.gd").build(personal,"badge","fragment")
	if evidence_found:
		B.box(world,Vector3(.65,.018,.72),Vector3(.55,.88,.13),B.material("957852"))
		for i: int in range(5):
			var page: Node3D = B.box(world,Vector3(.49,.003,.59),Vector3(.52+i*.01,.897+i*.004,.13),B.material("c5c4ac"))
			page.rotation.y = -.08+i*.025
		var label := Label3D.new()
		label.text = "ПОСЛЕДНЯЯ СМЕНА\nИРИНА / ДОКАЗАТЕЛЬСТВА\nПЕРЕДАТЬ СЕМЬЕ"
		label.font_size = 24
		label.pixel_size = .0014
		label.modulate = Color("30413e")
		label.outline_size = 0
		label.position = Vector3(.55,.92,.13)
		label.rotation.x = -PI/2
		world.add_child(label)
	var sun := DirectionalLight3D.new()
	if isolation_proof:
		B.box(world,Vector3(.38,.02,.48),Vector3(.02,.89,-.35),B.material("989d8b"))
		var label := Label3D.new()
		label.text = "ИЗОЛЯТОР 17\nКОПИЯ ПРИКАЗА\nДЛЯ СЛЕДСТВИЯ"
		label.font_size = 25
		label.pixel_size = .0011
		label.position = Vector3(.02,.91,-.35)
		label.rotation.x = -PI/2
		label.modulate = Color("293d38")
		world.add_child(label)
	sun.rotation_degrees = Vector3(-32,-35,0)
	sun.light_color = Color("f3cca0")
	sun.light_energy = .9
	sun.shadow_enabled = true
	world.add_child(sun)
	var camera := Camera3D.new()
	world.add_child(camera)
	camera.position = Vector3(.45,2.4,2.45)
	camera.look_at(Vector3(0,.75,0))
	camera.fov = 48
	camera.current = true
