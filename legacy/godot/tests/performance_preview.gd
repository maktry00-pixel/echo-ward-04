extends SceneTree
var game: Node3D
func _initialize() -> void: call_deferred("_run")
func _run() -> void:
	root.size = Vector2i(1280,900)
	var presentation:bool=OS.get_cmdline_user_args().has("--presentation-update")
	if presentation:
		root.size=Vector2i(1600,900)
		root.content_scale_size=Vector2i(1600,900)
		root.content_scale_aspect=Window.CONTENT_SCALE_ASPECT_EXPAND
	DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
	game = load("res://scenes/third_person.tscn").instantiate()
	game.set_meta("automated_test",true)
	root.add_child(game)
	for i: int in range(30): await process_frame
	if OS.get_cmdline_user_args().has("--interface-update") or presentation:
		var settings:Dictionary=game.settings.duplicate()
		settings.language="kk"
		settings.texture_quality="high"
		game.apply_settings(settings)
	for hunter: CharacterBody3D in game.hunters:
		hunter.set_active(true)
		hunter.set_physics_process(false)
	game.set_physics_process(false)
	var scenarios: Array[Vector3] = [Vector3(0,.1,11),Vector3(40,.1,11),Vector3(55,-4.9,72),Vector3(94,.1,18),Vector3(8,.1,20),Vector3(40,.1,-34)]
	if OS.get_cmdline_user_args().has("--isolation"):
		game.diagnostics.complete = true
		game.isolation.circuit = "light"
		game.isolation._apply_power()
		scenarios.append_array([Vector3(23,.1,-75),Vector3(40,.1,-99)])
	var results: Array[Dictionary] = []
	for at: Vector3 in scenarios:
		game.hero.reset_to(at)
		for i: int in range(120): await process_frame
		var timings: Array[float] = []
		var previous: int = Time.get_ticks_usec()
		for i: int in range(240):
			await process_frame
			var now: int = Time.get_ticks_usec()
			timings.append((now-previous)/1000.0)
			previous = now
		timings.sort()
		results.append({"position":str(at),"median_ms":timings[120],"p95_ms":timings[228],"draw_calls":Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME)})
	var report := {"resolution":"1280x900","vsync":false,"samples_per_scene":240,"renderer":"Compatibility","gpu":RenderingServer.get_video_adapter_name(),"scenarios":results,"note":"Staged uncapped GPU preview, AI frozen; not a minimum-spec benchmark or a human playtest."}
	var output: String = "performance_survival_upgrade.json" if OS.get_cmdline_user_args().has("--survival-upgrade") else "performance_diagnostics.json"
	if OS.get_cmdline_user_args().has("--isolation"): output = "performance_isolation.json"
	if OS.get_cmdline_user_args().has("--interface-update"):
		output="performance_interface.json"
		report.language="kk"
		report.texture_quality="high"
	if presentation:
		output="performance_presentation.json"
		report.resolution="1600x900"
		report.language="kk"
		report.texture_quality="high"
		report.msaa=root.msaa_3d
		report.shadows=game.settings.shadow_quality
	var file := FileAccess.open("res://artifacts/"+output,FileAccess.WRITE)
	file.store_string(JSON.stringify(report,"\t"))
	print("FRAME PACING ",JSON.stringify(report))
	game.queue_free()
	await process_frame
	quit()
