extends SubViewportContainer
## A real 3D inspection viewport. Drag rotates the item; the clue is on its back.
var pivot: Node3D
var content: Node3D
var camera: Camera3D
var item_id: String
var inventory: Node
var backside_time: float = 0
var viewport_3d: SubViewport
func _ready() -> void:
	custom_minimum_size = Vector2(400,360)
	stretch = true
	mouse_filter = Control.MOUSE_FILTER_STOP
	viewport_3d = SubViewport.new()
	viewport_3d.size = Vector2i(540,480)
	viewport_3d.own_world_3d = true
	viewport_3d.render_target_update_mode = SubViewport.UPDATE_DISABLED
	add_child(viewport_3d)
	var scene := Node3D.new()
	viewport_3d.add_child(scene)
	pivot = Node3D.new()
	scene.add_child(pivot)
	camera = Camera3D.new()
	scene.add_child(camera)
	camera.position = Vector3(0,.06,.65)
	camera.look_at(Vector3.ZERO)
	camera.fov = 38
	var world := WorldEnvironment.new()
	var env := Environment.new()
	env.background_mode = Environment.BG_COLOR
	env.background_color = Color("151d20")
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color = Color("bcc8c9")
	env.ambient_light_energy = .6
	world.environment = env
	scene.add_child(world)
	for point: Vector3 in [Vector3(-.5,.7,.4),Vector3(.5,.2,-.2)]:
		var light := OmniLight3D.new()
		light.position = point
		light.light_energy = 1.4
		light.omni_range = 3
		light.light_color = Color("ead2af") if point.x<0 else Color("a6c5d0")
		scene.add_child(light)
	gui_input.connect(_drag)

func show_item(id: String) -> void:
	item_id = id
	backside_time = 0
	if content!=null: content.free()
	content = Node3D.new()
	pivot.add_child(content)
	var loot = preload("res://scripts/loot_visual.gd")
	if id=="weapon": content.add_child(preload("res://scripts/pistol_visual.gd").new())
	elif id=="shotgun": content.add_child(preload("res://scripts/shotgun_visual.gd").new())
	elif id in ["shells","boards"]: loot.build(content,id,id)
	else: loot.build(content,"ammo_inspection" if id=="ammo" else id,id if id in ["battery","medkit"] else "fragment")
	if id=="badge":
		var clue := Label3D.new()
		clue.text = "КАРАНТИН\n4  ·  1  ·  7\nКОД НАЧАЛЬНИКА СМЕНЫ"
		clue.font_size = 24
		clue.pixel_size = .00030
		clue.modulate = Color("26372d")
		clue.rotation = Vector3(PI/2,0,PI)
		clue.outline_size = 0
		clue.position = Vector3(0,-.001,0)
		content.add_child(clue)
	var bounds := AABB()
	var first: bool = true
	for mesh: MeshInstance3D in content.find_children("*","MeshInstance3D",true,false):
		var box: AABB = content.global_transform.affine_inverse()*mesh.global_transform*mesh.get_aabb()
		bounds = box if first else bounds.merge(box)
		first = false
	content.position = -bounds.get_center()
	pivot.scale = Vector3.ONE*(.30/maxf(.01,maxf(bounds.size.x,maxf(bounds.size.y,bounds.size.z))))
	pivot.rotation = Vector3(PI/2,0,0) if id=="badge" else Vector3(-.3,.35,0)
	viewport_3d.render_target_update_mode = SubViewport.UPDATE_ALWAYS

func flip() -> void: pivot.rotation.y += PI
func stop() -> void: viewport_3d.render_target_update_mode = SubViewport.UPDATE_DISABLED
func _drag(event: InputEvent) -> void:
	if event is InputEventMouseMotion and event.button_mask&MOUSE_BUTTON_MASK_LEFT:
		pivot.rotation.y += event.relative.x*.012
		pivot.rotation.x += event.relative.y*.012
	if event is InputEventMouseButton and event.pressed:
		if event.button_index==MOUSE_BUTTON_WHEEL_UP: camera.position *= .9
		elif event.button_index==MOUSE_BUTTON_WHEEL_DOWN: camera.position *= 1.1
		camera.position = camera.position.normalized()*clampf(camera.position.length(),.35,1.1)
func _process(delta: float) -> void:
	if not is_visible_in_tree() or item_id!="badge" or inventory.badge_examined: return
	var back: Vector3 = pivot.global_basis*Vector3.DOWN
	if back.normalized().dot((camera.global_position-pivot.global_position).normalized())>.7:
		backside_time += delta
		if backside_time>.35:
			inventory.badge_examined = true
			inventory.game.hud.announce("На обороте пропуска: КАРАНТИН — 4 · 1 · 7.",6)
	else: backside_time = 0
