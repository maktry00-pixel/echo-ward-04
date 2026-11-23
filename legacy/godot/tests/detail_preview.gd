extends SceneTree
## Staged asset views followed by captures in the actual game lighting.
const Props = preload("res://scripts/imported_props.gd")
const Loot = preload("res://scripts/loot_visual.gd")
var stage: Node3D
var camera: Camera3D
func _initialize() -> void: call_deferred("_run")
func _run() -> void:
	root.size = Vector2i(1280,900)
	stage = Node3D.new()
	root.add_child(stage)
	var world := WorldEnvironment.new()
	var env := Environment.new()
	env.background_mode = Environment.BG_COLOR
	env.background_color = Color("171e1c")
	env.ambient_light_color = Color("b7c6bf")
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_energy = .45
	world.environment = env
	stage.add_child(world)
	for location: Vector3 in [Vector3(-.6,1,.4),Vector3(.7,.6,-.6)]:
		var light := OmniLight3D.new()
		light.position = location
		light.light_energy = 2
		light.omni_range = 4
		light.light_color = Color("ecd3ac") if location.x<0 else Color("c8ddd8")
		stage.add_child(light)
	camera = Camera3D.new()
	stage.add_child(camera)
	camera.current = true
	camera.fov = 36
	var pistol: Node3D = preload("res://scripts/pistol_visual.gd").new()
	stage.add_child(pistol)
	pistol.position.y = .2
	camera.position = Vector3(.43,.33,.3)
	camera.look_at(Vector3(0,.19,-.04))
	await _frames(20)
	await _save("pistol_2k_closeup")
	pistol.queue_free()
	var display := Node3D.new()
	stage.add_child(display)
	var specs: Array = [["ammo_archive",""],["fuse",""],["badge",""],["","fragment"],["","battery"],["","medkit"]]
	for i: int in range(specs.size()):
		var item := Node3D.new()
		display.add_child(item)
		item.position = Vector3((i%3-1)*.23,0,(i/3)*.26)
		Loot.build(item,specs[i][0],specs[i][1])
	Props.detail(display,"fish_knife",.28,Vector3(-.25,0,.54),Vector3(-PI/2,0,0))
	Props.detail(display,"small_plastic_torch",.20,Vector3(.19,0,.54))
	camera.position = Vector3(.5,.92,1.0)
	camera.look_at(Vector3(0,0,.25))
	await _frames(20)
	await _save("loot_2k_closeup")
	stage.queue_free()
	await process_frame
	var game: Node3D = load("res://scenes/third_person.tscn").instantiate()
	game.set_meta("automated_test",true)
	root.add_child(game)
	await _frames(20)
	for enemy: CharacterBody3D in game.hunters:
		enemy.set_active(false)
		enemy.set_physics_process(false)
	game.combat.acquire()
	game.hero.reset_to(Vector3(0,.1,11))
	Input.action_press("aim")
	await _frames(30)
	await _save("pistol_in_game")
	# Close view uses the existing room lights, with no added showcase lights.
	var inspection := Camera3D.new()
	game.add_child(inspection)
	inspection.current = true
	inspection.fov = 40
	inspection.position = game.combat.weapon.global_position+Vector3(.33,.20,.28)
	inspection.look_at(game.combat.weapon.global_position+Vector3(0,-.015,-.05))
	await _frames(12)
	await _save("pistol_room_lighting")
	Input.action_release("aim")
	game.queue_free()
	await process_frame
	print("DETAIL PREVIEWS COMPLETE")
	quit()
func _frames(count: int) -> void:
	for i: int in range(count):
		await physics_frame
		await process_frame
func _save(name: String) -> void:
	await RenderingServer.frame_post_draw
	print("DETAIL ",name," ",root.get_texture().get_image().save_png("res://artifacts/"+name+".png"))
