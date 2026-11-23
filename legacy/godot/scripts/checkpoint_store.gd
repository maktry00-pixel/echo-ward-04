extends RefCounted
## Bounded progress data; exact manual locations are checked against real collision.

static func snapshot(game: Node3D) -> Dictionary:
	var storage: Dictionary = {}
	for box: Node3D in game.get_tree().get_nodes_in_group("search_storage"): storage[box.key] = box.snapshot()
	var doors: Dictionary = {}
	for door: Node3D in game.facility.doors:
		doors[door.door_id] = {"open": door.is_open, "locked": door.locked}
	var records: Array[int] = []
	for core: Area3D in game.cores:
		if core.is_recovered:
			records.append(core.record_index)
	var batteries: Array[int] = []
	for index: int in range(game.batteries.size()):
		if game.batteries[index].is_recovered:
			batteries.append(index)
	var used: Array[String] = []
	var dials: Dictionary = {}
	var enemies: Array[Dictionary] = []
	var used_medkits: Array[int] = []
	for index: int in range(game.medkits.size()):
		if game.medkits[index].is_recovered:
			used_medkits.append(index)
	for id: String in game.mission.objects:
		if game.mission.objects[id].used:
			used.append(id)
		if id.begins_with("dial_"):
			dials[id] = game.mission.objects[id].state_value
	for hunter: CharacterBody3D in game.hunters:
		enemies.append({"health": hunter.health, "active": hunter.active,"legs":hunter.leg_damage.duplicate(),"death_style":hunter.model.death_style})
	return {"version": 1, "active": true, "stage": "badge" if game.mission.has_badge else ("power" if game.mission.power_on else "start"),
		"difficulty":game.difficulty_id,"upgrades":game.upgrades.snapshot(),
		"power_on":game.mission.power_on,"fuse":game.mission.has_fuse,"elapsed":game.elapsed,
		"refuge":game.refuge.snapshot(),
		"pacing":game.pacing.snapshot(),"events":game.mission.events.keys(),
		"diagnostics":game.diagnostics.snapshot(),
		"isolation":game.isolation.snapshot(),
		"annex":game.annex.snapshot(),
		"reactor_state":{"started":game.mission.purge_started,"remaining":game.mission.purge_remaining,"fault":game.mission.purge_fault,"serviced":game.mission.serviced_valves.duplicate()},
		"inventory":game.inventory.snapshot(),
		"layout_positions":game.resource_layout.snapshot(game),
		"exploration":game.exploration.snapshot(),
		"guardian":game.guardian.snapshot(),
		"storage":storage,
		"residential": game.residential.snapshot(),
		"bottles": game.survival_tools.bottles,
		"layout_seed": game.layout_seed, "coolant": game.mission.has_coolant, "fuel": game.mission.has_fuel, "purge_complete": game.mission.purge_complete,
		"relays": game.mission.relays.duplicate(), "used": used, "dials": dials, "enemies": enemies, "medkits": used_medkits,
		"combat": {"equipped":game.combat.equipped,"rounds":game.combat.rounds,"reserve":game.combat.reserve,"shots":game.combat.shots_fired,"hits":game.combat.hits,"kills":game.combat.kills,"shotgun":game.combat.shotgun_owned,"loaded":game.combat.shotgun_loaded,"shells":game.combat.shells,"selected":game.combat.selected_weapon},
		"map": game.mission.has_map, "records": records, "batteries": batteries,
		"medkit": game.medkits[0].is_recovered, "doors": doors,
		"health": maxi(2, game.health), "charges": maxi(1, game.flash_charges), "score": game.score}

static func valid(data: Variant) -> bool:
	if not data is Dictionary:
		return false
	if data.has("saved_at") and (not _number(data.saved_at) or data.saved_at<0 or data.saved_at>9000000000): return false
	if data.has("save_kind") and data.save_kind not in ["auto","manual"]: return false
	if data.has("chapter_name") and (not data.chapter_name is String or data.chapter_name.length()>100): return false
	if data.has("events"):
		if not data.events is Array or data.events.size()>6:return false
		var seen_events:Array=[]
		for id:Variant in data.events:
			if id not in ["first_contact","observation_window","encounter_morgue","encounter_lab","encounter_theatre","changed_reception"] or id in seen_events:return false
			seen_events.append(id)
	if data.has("exact_save"):
		if data.exact_save!=true or not data.get("resume") is Dictionary or not _position(data.resume.get("at")): return false
		for key: String in ["yaw","pitch"]:
			if not _number(data.resume.get(key)) or absf(float(data.resume[key]))>100000: return false
		if absf(float(data.resume.pitch))>1.6: return false
		if not data.get("actors") is Array or not data.get("enemies") is Array or data.actors.size()>32 or data.actors.size()!=data.enemies.size(): return false
		for actor: Variant in data.actors:
			if not actor is Dictionary or not _position(actor.get("at")) or not _position(actor.get("known")): return false
			if not preload("res://scripts/field_inventory.gd")._integer(actor.get("state"),0,3): return false
	if data.get("version") != 1 or data.get("active") != true or data.get("stage") not in ["start", "power", "badge"]:
		return false
	if data.has("difficulty") and (not data.difficulty is String or data.difficulty not in preload("res://scripts/difficulty_profile.gd").IDS): return false
	if data.has("upgrades") and not preload("res://scripts/survival_upgrade.gd").valid(data.upgrades): return false
	if data.has("elapsed"):
		var time_value: Variant = data.elapsed
		if not (time_value is int or time_value is float) or not is_finite(float(time_value)) or time_value<0 or time_value>1000000: return false
	if data.has("manual_refuge") and not data.manual_refuge is bool: return false
	if data.stage=="start" and ((data.get("manual_refuge",false)!=true and data.get("exact_save",false)!=true) or data.get("power_on",false)!=false): return false
	if data.has("power_on") and (not data.power_on is bool or (data.stage!="start" and not data.power_on)): return false
	if data.has("fuse") and not data.fuse is bool: return false
	if data.has("pacing") and not preload("res://scripts/pacing_director.gd").valid(data.pacing): return false
	if data.has("diagnostics") and not preload("res://scripts/diagnostic_chapter.gd").valid(data.diagnostics): return false
	if data.has("isolation"):
		if not preload("res://scripts/isolation_chapter.gd").valid(data.isolation): return false
		if data.isolation.case and not data.get("diagnostics",{}).get("complete",false): return false
	if data.has("refuge") and not preload("res://scripts/refuge_room.gd").valid(data.refuge): return false
	if data.has("annex"):
		if not preload("res://scripts/service_annex.gd").valid(data.annex): return false
		if not data.annex.steps.is_empty() and not data.get("isolation",{}).get("complete",false): return false
	if data.has("reactor_state"):
		var reactor: Variant = data.reactor_state
		if not reactor is Dictionary or not reactor.get("started") is bool or reactor.get("fault") not in ["","purge_valve_a","purge_valve_b"] or not reactor.get("serviced") is Array: return false
		var remaining: Variant = reactor.get("remaining")
		if not (remaining is int or remaining is float) or not is_finite(float(remaining)) or remaining<0 or remaining>45: return false
		var valves: Array = []
		for id: Variant in reactor.serviced:
			if id not in ["purge_valve_a","purge_valve_b"] or id in valves: return false
			valves.append(id)
		if reactor.fault in valves: return false
		if reactor.started and (data.get("relays",[]).size()!=3 or not data.get("fuel",false) or not data.get("coolant",false)): return false
	if data.has("layout_positions") and not preload("res://scripts/resource_layout.gd").valid_positions(data.layout_positions): return false
	if data.has("inventory") and not preload("res://scripts/field_inventory.gd").valid(data.inventory): return false
	if data.has("exploration") and not preload("res://scripts/exploration.gd").valid(data.exploration): return false
	if data.has("guardian") and not preload("res://scripts/guardian_encounter.gd").valid(data.guardian): return false
	if not data.get("records") is Array or not data.get("batteries") is Array or not data.get("doors") is Dictionary:
		return false
	if data.has("upgrades"):
		if ("shotgun" in data.upgrades.used)!=data.get("combat",{}).get("shotgun",false): return false
		for id: String in data.upgrades.barriers:
			var gate: Variant = data.doors.get(id)
			if not gate is Dictionary or gate.get("open")!=false or gate.get("locked")!=false: return false
	if data.has("combat"):
		var weapon: Variant = data["combat"]
		if not weapon is Dictionary or not weapon.get("equipped") is bool:
			return false
		if weapon.has("shotgun") and not weapon.shotgun is bool: return false
		if weapon.has("loaded") and not preload("res://scripts/field_inventory.gd")._integer(weapon.loaded,0,4): return false
		if weapon.has("shells") and not preload("res://scripts/field_inventory.gd")._integer(weapon.shells,0,18): return false
		if weapon.has("selected") and weapon.selected not in ["pistol","shotgun"]: return false
		if not weapon.get("shotgun",false) and (weapon.get("loaded",0)>0 or weapon.get("selected","pistol")=="shotgun"): return false
		if weapon.get("shotgun",false) and not weapon.equipped: return false
		for counter: String in ["shots","hits","kills"]:
			if weapon.has(counter) and not preload("res://scripts/field_inventory.gd")._integer(weapon[counter],0,1000000): return false
		for key: String in ["rounds", "reserve"]:
			var number: Variant = weapon.get(key)
			if not (number is int or number is float) or not is_finite(float(number)) or float(number) < 0 or float(number) > (9 if key == "rounds" else 72):
				return false
	if data.has("residential"):
		if not preload("res://scripts/residential_wing.gd").valid(data["residential"]): return false
		if (data["residential"]["prybar"] or data["residential"]["core"] or data["residential"]["complete"]) and data.get("purge_complete",false)!=true: return false
	if data.has("storage"):
		if not data["storage"] is Dictionary or data["storage"].size()>64: return false
		for key: Variant in data["storage"]:
			if not key is String or key.length()>120 or not preload("res://scripts/search_storage.gd").valid(data["storage"][key]): return false
	if data.has("inventory"):
		var inv: Dictionary = data.inventory
		var slots: int = int(inv.medkits)+int(inv.batteries)
		slots += (1 if data.get("combat",{}).get("shotgun",false) else 0)+ceili(float(data.get("combat",{}).get("shells",0))/8)+(1 if inv.get("boards",0)>0 else 0)
		slots += 1 if data.get("combat",{}).get("equipped",true) else 0
		slots += ceili(float(data.get("combat",{}).get("reserve",27))/24)
		slots += 1 if data.get("bottles",3)>0 else 0
		slots += 1 if data.stage=="badge" else 0
		slots += 1 if data.get("fuse",false) and data.stage=="start" else 0
		if not inv.get("installed",false) and not data.get("purge_complete",false):
			slots += (1 if data.get("fuel",false) else 0)+(1 if data.get("coolant",false) else 0)
		var residential: Dictionary = data.get("residential",{})
		if not residential.get("complete",false):
			slots += (1 if residential.get("prybar",false) else 0)+(1 if residential.get("core",false) else 0)
		var isolation: Dictionary = data.get("isolation",{})
		slots += (1 if isolation.get("coil",false) and not isolation.get("started",false) else 0)+(1 if isolation.get("tag",false) and not isolation.get("log",false) else 0)
		var annex_steps: Array = data.get("annex",{}).get("steps",[])
		slots += 1 if "wheel" in annex_steps and "pump" not in annex_steps else 0
		if slots>8: return false
	if data.has("layout_seed"):
		var seed_number: Variant = data["layout_seed"]
		if not (seed_number is int or seed_number is float) or not is_finite(float(seed_number)) or float(seed_number) < 0 or float(seed_number) > 1000000:
			return false
	if data.has("bottles"):
		var count: Variant = data["bottles"]
		if not (count is int or count is float) or not is_finite(float(count)) or float(count)!=floor(float(count)) or float(count)<0 or float(count)>3: return false
	for key: String in ["coolant", "fuel", "purge_complete"]:
		if data.has(key) and not data[key] is bool:
			return false
	for key: String in ["relays", "used", "enemies", "medkits"]:
		if data.has(key) and not data[key] is Array:
			return false
	if data.has("dials") and not data["dials"] is Dictionary:
		return false
	for index: Variant in data.get("medkits", []):
		if not (index is int or index is float) or not is_finite(float(index)) or float(index) != floor(float(index)) or float(index) < 0 or float(index) > 2:
			return false
	var unique: Dictionary = {}
	for id: Variant in data.get("relays", []):
		if not id is String or id not in ["morgue", "lab", "theatre"] or unique.has(id) or data["stage"] != "badge":
			return false
		unique[id] = true
	for enemy: Variant in data.get("enemies", []):
		if not enemy is Dictionary or not enemy.get("active") is bool:
			return false
		if enemy.has("death_style") and enemy.death_style not in ["body","head","blast"]: return false
		var hp: Variant = enemy.get("health")
		if not (hp is int or hp is float) or not is_finite(float(hp)) or float(hp) < 0 or float(hp) > 12:
			return false
		if enemy.has("legs"):
			if not enemy.legs is Array or enemy.legs.size() not in [2,6]: return false
			for value: Variant in enemy.legs:
				if not preload("res://scripts/field_inventory.gd")._integer(value,0,2): return false
	for key: String in ["health", "charges", "score"]:
		var value: Variant = data.get(key)
		if not (value is int or value is float):
			return false
		if not is_finite(float(value)) or float(value) < 0 or float(value) > 1000000:
			return false
	for key: String in ["records","batteries"]:
		for value: Variant in data[key]:
			if not (value is int or value is float): return false
			var maximum: int = 9 if key=="records" else 1
			if not is_finite(float(value)) or float(value)!=floor(float(value)) or float(value)<0 or float(value)>maximum: return false
	return true

static func apply(game: Node3D, data: Dictionary) -> bool:
	if not valid(data):
		return false
	game.difficulty_id = data.get("difficulty","normal")
	game.restart_game()
	game.survival_tools.bottles = int(data.get("bottles",3))
	game.mission.power_on = data.get("power_on",true)
	game.mission.has_fuse = data.get("fuse",false) and not game.mission.power_on
	game.mission.has_badge = data["stage"] == "badge"
	game.mission.has_map = data.get("map", false) == true
	game.layout_seed = int(data.get("layout_seed", 1907))
	game.resource_layout.apply(game, game.layout_seed)
	if data.has("layout_positions"): game.resource_layout.restore(game,data.layout_positions)
	game.mission.has_coolant = data.get("coolant", false) == true
	game.mission.has_fuel = data.get("fuel", false) == true
	game.mission.purge_complete = data.get("purge_complete", false) == true and data.get("relays", []).size() == 3 and game.mission.has_coolant and game.mission.has_fuel
	game.mission.purge_started = game.mission.purge_complete
	if game.mission.has_coolant:
		game.mission.objects["coolant"].mark_used()
	if game.mission.has_fuel:
		game.mission.objects["fuel"].mark_used()
	if game.mission.purge_complete:
		game.mission.objects["reactor"].mark_used()
		game.mission.purge_remaining = 0
		game.mission.serviced_valves.assign(["purge_valve_a","purge_valve_b"])
		for id: String in game.mission.serviced_valves: game.mission.objects[id].mark_used()
	game.mission.events = {"first_contact": true, "observation_window": true}
	if data.has("events"):
		game.mission.events.clear()
		for id:String in data.events:game.mission.events[id]=true
	if game.mission.has_fuse or game.mission.power_on: game.mission.objects["fuse"].mark_used()
	if game.mission.power_on: game.mission.objects["power"].mark_used()
	if game.mission.has_badge:
		game.mission.objects["badge"].mark_used()
	if game.mission.has_map:
		game.mission.objects["map"].mark_used()
	if game.mission.power_on: game.facility.apply_power()
	game.combat.equipped = data.get("combat", {}).get("equipped", true)
	game.combat.shotgun_owned = data.get("combat",{}).get("shotgun",false)
	game.combat.shotgun_loaded = int(data.get("combat",{}).get("loaded",0))
	game.combat.shells = int(data.get("combat",{}).get("shells",0))
	game.combat.selected_weapon = data.get("combat",{}).get("selected","pistol")
	game.combat.rounds = int(data.get("combat", {}).get("rounds", 9))
	game.combat.reserve = int(data.get("combat", {}).get("reserve", 27))
	game.combat.shots_fired = int(data.get("combat",{}).get("shots",0))
	game.combat.hits = int(data.get("combat",{}).get("hits",0))
	game.combat.kills = int(data.get("combat",{}).get("kills",0))
	game.elapsed = float(data.get("elapsed",0))
	if game.combat.equipped:
		game.mission.objects["weapon"].mark_used()
	for id: Variant in data.get("used", []):
		if id is String and game.mission.objects.has(id) and (id.begins_with("ammo_") or id == "weapon"):
			game.mission.objects[id].mark_used()
	for id: Variant in data.get("dials", {}):
		var value: Variant = data["dials"][id]
		if id is String and id.begins_with("dial_") and game.mission.objects.has(id) and (value is int or value is float) and is_finite(float(value)):
			game.mission.objects[id].state_value = clampi(int(value), 0, 3)
			game.mission.objects[id].status_label.text = game.mission.objects[id].display_name()
	for id: String in data.get("relays", []):
		game.mission.relays.append(id)
		game.mission.objects["relay_" + id].mark_used()
	for index: int in range(mini(data.get("enemies", []).size(), game.hunters.size())):
		var enemy: Dictionary = data["enemies"][index]
		var hunter: CharacterBody3D = game.hunters[index]
		hunter.health = clampi(int(enemy["health"]), 0, hunter.max_health)
		hunter.model.death_style = enemy.get("death_style","body")
		if enemy.has("legs") and enemy.legs.size()==hunter.leg_damage.size():
			hunter.leg_damage.assign(enemy.legs)
			hunter._refresh_injury()
		if hunter.health == 0:
			hunter.dead = true
			hunter.active = false
			hunter.collision_layer = 0
			hunter.model.restore_death(hunter.position+Vector3.FORWARD)
			hunter.show()
		else:
			hunter.set_active(enemy["active"])
	game.facility.silhouette.hide()
	game.facility.silhouette_time = -1
	game.facility.blackout_remaining = 0
	for door: Node3D in game.facility.doors:
		var saved: Variant = data["doors"].get(door.door_id)
		if saved is Dictionary and saved.get("open") is bool and saved.get("locked") is bool:
			# Authored progression always wins over untrusted door values.
			var locked: bool = saved["locked"]
			if door.lock_kind == "power": locked = not game.mission.power_on
			if door.lock_kind == "badge" and not game.mission.has_badge: locked = true
			if door.lock_kind == "containment" and data.get("relays", []).size() < 3: locked = true
			door.restore_state(saved["open"] and not locked, locked)
	for core: Area3D in game.cores:
		if core.record_index in data["records"]:
			core.is_recovered = true
			core.storage.finish()
			game.recovered_count += 1
	for index: int in range(game.batteries.size()):
		if index in data["batteries"]:
			game.batteries[index].is_recovered = true
			game.batteries[index].storage.finish()
	if data.get("medkit", false) == true:
		game.medkits[0].is_recovered = true
		game.medkits[0].storage.finish()
	for index: Variant in data.get("medkits", []):
		game.medkits[int(index)].is_recovered = true
		game.medkits[int(index)].storage.finish()
	game.health = clampi(int(data.get("health",2)),1 if data.get("manual_refuge",false) or data.get("exact_save",false) else 2,3)
	game.flash_charges = clampi(int(data.get("charges",1)),0 if data.get("manual_refuge",false) or data.get("exact_save",false) else 1,3)
	game.score = clampi(int(data.get("score", 0)), 0, 1000000)
	game.alarm_stage = 2 if game.mission.power_on else 1
	game.hunters[0].set_active(true)
	game.hero.reset_to(Vector3(10, 0.1, -3) if game.mission.has_badge else Vector3(-20, 0.1, 0))
	if not game.mission.relays.is_empty():
		var x: float = {"morgue": 23.0, "lab": 37.0, "theatre": 52.0}[game.mission.relays.back()]
		game.hero.reset_to(Vector3(x + 4, 0.1, 23))
	if game.mission.purge_complete:
		game.hero.reset_to(Vector3(53, -4.9, 77))
	game.residential.restore(data.get("residential",{}))
	game.inventory.restore(data.get("inventory",{}))
	game.exploration.restore(data.get("exploration",[]))
	game.guardian.restore(data.get("guardian",{}))
	for box: Node3D in game.get_tree().get_nodes_in_group("search_storage"):
		if data.get("storage",{}).has(box.key): box.restore(data["storage"][box.key])
	if game.residential.has_prybar:
		game.hero.reset_to(Vector3(82,.1,27) if game.residential.service_opened else Vector3(99,.1,30))
	game.facility.layout_changed.emit()
	game.refuge.restore(data.get("refuge",{}))
	game.pacing.restore(data.get("pacing",{}))
	game.diagnostics.restore(data.get("diagnostics",{}))
	game.upgrades.restore(data.get("upgrades",{}))
	game.isolation.restore(data.get("isolation",{}))
	game.annex.restore(data.get("annex",{}))
	if game.diagnostics.complete: game.hero.reset_to(Vector3(56,.1,-41))
	if data.get("isolation",{}).get("case",false): game.hero.reset_to(Vector3(52,.1,-55))
	if not data.get("annex",{}).get("steps",[]).is_empty(): game.hero.reset_to(Vector3(41,.1,-119))
	if data.has("reactor_state") and data.reactor_state.started and not game.mission.purge_complete:
		game.mission.purge_started = true
		game.mission.purge_remaining = float(data.reactor_state.remaining)
		game.mission.purge_fault = data.reactor_state.fault
		game.mission.serviced_valves.assign(data.reactor_state.serviced)
		for id: String in game.mission.serviced_valves: game.mission.objects[id].mark_used()
		if not game.mission.purge_fault.is_empty(): game.mission.objects[game.mission.purge_fault].set_fault(true)
	if data.get("manual_refuge",false):
		game.hero.reset_to(preload("res://scripts/refuge_room.gd").SPAWN)
		game.hero.camera_rig.rotation.y = PI
		game.refuge.update()
	game.mission.refresh_hud()
	if data.get("exact_save",false):
		for i: int in range(mini(game.hunters.size(),data.actors.size())):
			var actor: CharacterBody3D = game.hunters[i]
			var saved: Dictionary = data.actors[i]
			actor.position = Vector3(saved.at[0],saved.at[1],saved.at[2])
			actor.last_known = Vector3(saved.known[0],saved.known[1],saved.known[2])
			actor.state = int(saved.state)
			actor.set_active(data.enemies[i].active)
			actor.route.clear()
			actor.repath_remaining = 0
		game.call_deferred("_restore_exact_position",data.resume)
	game.hud.announce("Контрольная точка загружена. Переведи дух и продолжай.", 5)
	return true

static func _number(value: Variant) -> bool:
	return (value is int or value is float) and is_finite(float(value))
static func _position(value: Variant) -> bool:
	if not value is Array or value.size()!=3: return false
	for n: Variant in value:
		if not _number(n): return false
	return value[0]>=-30 and value[0]<=104 and value[1]>=-5.2 and value[1]<=4 and value[2]>=-192 and value[2]<=82
