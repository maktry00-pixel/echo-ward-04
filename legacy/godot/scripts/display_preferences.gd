extends CanvasLayer
var game:Node3D
var material:ShaderMaterial
var environment:Environment
var lights:Array[WeakRef]=[]
func configure(owner_game:Node3D)->void:
	game=owner_game;layer=-10;process_mode=Node.PROCESS_MODE_ALWAYS
	var screen:=ColorRect.new();screen.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	screen.mouse_filter=Control.MOUSE_FILTER_IGNORE
	material=ShaderMaterial.new();material.shader=preload("res://shaders/display_tuning.gdshader")
	screen.material=material;add_child(screen)
	for node:Node in game.find_children("*","WorldEnvironment",true,false):environment=node.environment;break
	for light:Node in game.find_children("*","Light3D",true,false):
		light.set_meta("authored_shadow",light.shadow_enabled);lights.append(weakref(light))
func apply(values:Dictionary)->void:
	var window:Window=get_tree().root
	window.content_scale_aspect=Window.CONTENT_SCALE_ASPECT_EXPAND
	window.msaa_3d=int(values.msaa) as Viewport.MSAA
	window.positional_shadow_atlas_size=2048 if values.shadow_quality=="medium" else 4096
	for key:String in ["brightness","contrast","grain","vignette"]:material.set_shader_parameter(key,values[key])
	if environment!=null:
		environment.ambient_light_energy=.16*float(values.ambient)
		environment.fog_density=.009*float(values.fog)
	for ref:WeakRef in lights:
		var light:Light3D=ref.get_ref()
		if light!=null:light.shadow_enabled=light.get_meta("authored_shadow",false) and values.shadow_quality!="off"
	game.hero.camera_rig.base_fov=values.fov
	game.hero.camera_rig.shake_scale=values.shake
	game.hero.camera_rig.invert_y=values.invert_y
	game.hero.camera_rig.flashlight.light_energy=4.0*float(values.flashlight_strength)
	game.hud.apply_preferences(values)
	if not game.automated:
		Engine.max_fps=int(values.max_fps)
		DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_ENABLED if values.vsync else DisplayServer.VSYNC_DISABLED)
