extends SceneTree
var game: Node3D
func _initialize() -> void: call_deferred("_run")
func _run() -> void:
	root.mode = Window.MODE_WINDOWED; root.size = Vector2i(1600,900); root.content_scale_size = Vector2i(1600,900)
	Engine.max_fps = 0; DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
	game = load("res://scenes/third_person.tscn").instantiate(); game.set_meta("automated_test",true); root.add_child(game)
	for i: int in range(20): await process_frame
	game.apply_settings(game.normalise_settings({"texture_quality":"high","shadow_quality":"high","msaa":2,"language":"ru"}),false)
	game.set_physics_process(false); game.hero.set_physics_process(false)
	for hunter: CharacterBody3D in game.hunters: hunter.set_active(true); hunter.set_physics_process(false)
	var results: Array[Dictionary] = []
	for row: Array in [["reception",Vector3(0,.1,11)],["reactor",Vector3(55,-4.9,72)],["annex_boiler",Vector3(37,.1,-154)]]:
		game.hero.reset_to(row[1]); game.hero.camera_rig.rotation = Vector3(-.12,0,0)
		var start: int = Time.get_ticks_usec()
		while Time.get_ticks_usec()-start<5000000: await process_frame
		await RenderingServer.frame_post_draw
		root.get_texture().get_image().save_png("res://artifacts/comparison_godot_"+row[0]+".png")
		var timing: Array[float] = []; var previous: int = Time.get_ticks_usec(); start = previous
		while Time.get_ticks_usec()-start<8000000:
			await process_frame
			var now: int = Time.get_ticks_usec(); timing.append((now-previous)/1000.0); previous = now
		timing.sort(); var total: float = 0
		for ms: float in timing: total += ms
		results.append({"scene":row[0],"frames":timing.size(),"mean_ms":total/timing.size(),"average_fps":1000*timing.size()/total,"median_ms":timing[timing.size()/2],"p95_ms":timing[int(timing.size()*.95)],"draw_calls":Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME)})
	var report := {"width":root.size.x,"height":root.size.y,"renderer":"Compatibility","gpu":RenderingServer.get_video_adapter_name(),"warmup_seconds_per_scene":5,"sample_seconds_per_scene":8,"vsync":false,"fps_limit":0,"texture_quality":"high","shadows":"high","msaa":root.msaa_3d,"scenarios":results,"note":"Current full Godot level, staged views with frozen AI. Not the same geometry or lighting as the small Unreal room; cannot establish engine speed superiority."}
	var file := FileAccess.open("res://artifacts/comparison_godot.json",FileAccess.WRITE); file.store_string(JSON.stringify(report,"\t")); file.close()
	print("GODOT COMPARISON COMPLETE ",JSON.stringify(report)); game.queue_free(); await process_frame; quit()
