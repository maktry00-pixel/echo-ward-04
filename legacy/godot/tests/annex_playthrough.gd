extends SceneTree
var game:Node3D
func _initialize()->void:call_deferred("_run")
func _run()->void:
	game=load("res://scenes/third_person.tscn").instantiate();game.set_meta("automated_test",true);root.add_child(game)
	for i:int in range(18):await physics_frame;await process_frame
	game._new_game()
	for i:int in range(8):await physics_frame;await process_frame
	game.admin.password.text="0890";game.admin.submit_password();game.admin.pilot_button.pressed.emit()
	var frames:int=0
	var count:int=0
	var force_death:bool=OS.get_cmdline_user_args().has("--respawn-at-badge")
	var forced:bool=false
	while frames<180000 and not game.has_won and game.admin.pilot.running:
		await physics_frame;await process_frame;frames+=1
		if game.admin.pilot.completed.size()!=count:
			count=game.admin.pilot.completed.size()
			if count>0:print("AGENT STEP ",game.admin.pilot.completed[-1]," at ",snappedf(game.elapsed,.1)," s; HP=",game.health)
		if force_death and not forced and "badge" in game.admin.pilot.completed:
			forced=true;game.health=0;game._finish_round(false,"agent respawn integration fixture")
	for i:int in range(30):await physics_frame;await process_frame
	var report:Dictionary={"won":game.has_won,"seconds":snappedf(game.elapsed,.1),"frames":frames,"status":game.admin.pilot.status,"steps":game.admin.pilot.completed,"health":game.health,"shots":game.combat.shots_fired,"diagnostics":game.diagnostics.complete,"isolation":game.isolation.complete,"records":game.upgrades.records,"auto_slots":game.saves.slots.keys(),"note":"Actual shipped runtime agent, normal physics and live enemies; no supply gifts or teleportation after activation."}
	report.forced_death=forced
	report.annex=game.annex.snapshot()
	report.location=[game.hero.position.x,game.hero.position.y,game.hero.position.z]
	var output:String="annex_agent_resume" if force_death else "annex_agent_playthrough"
	var file:=FileAccess.open("res://artifacts/"+output+".json",FileAccess.WRITE);file.store_string(JSON.stringify(report,"\t"));file.close()
	print("RUNTIME AGENT COMPLETE ",JSON.stringify(report))
	var won:bool=game.has_won;game.admin.pilot.stop();game.queue_free();await process_frame;quit(0 if won else 1)
