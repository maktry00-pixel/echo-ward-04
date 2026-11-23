extends Node3D
## Explore the wing, restore power, obtain access and escape through reception.

const Facility = preload("res://scripts/facility.gd")
const HeroScene = preload("res://scenes/hero.tscn")
const EnergyCore = preload("res://scripts/energy_core.gd")
const GameHUD = preload("res://scripts/game_hud.gd")
const Hunter = preload("res://scripts/stalker.gd")
const Navigation = preload("res://scripts/hunt_navigation.gd")
const Feedback = preload("res://scripts/feedback.gd")
const SoundBank = preload("res://scripts/sound_bank.gd")
const Mission = preload("res://scripts/mission_director.gd")
const MissionObject = preload("res://scripts/mission_object.gd")
const HorrorDirector = preload("res://scripts/horror_director.gd")
const FrontEnd = preload("res://scripts/front_end.gd")
const Checkpoints = preload("res://scripts/checkpoint_store.gd")
const Combat = preload("res://scripts/survival_combat.gd")
const ResourceLayout = preload("res://scripts/resource_layout.gd")
const Acid = preload("res://scripts/acid_projectile.gd")
const INITIAL_GRACE: float = 22.0
const STORIES: Array[String] = [
	"Запись / Приёмная: Запасной предохранитель в палате 05 справа. Электрощитовая — через левую палату, в служебном крыле.",
	"Запись / Дежурный: Не беги без нужды. Они идут на шум. Закрой дверь и уйди из их поля зрения.",
	"Запись / Палата 04: В служебном крыле есть дверь обратно в приёмную. Засов открывается изнутри.",
	"Запись / Палата 05: Вспышка удерживает их лишь на несколько секунд. Батареи остались на медскладе, но там кто-то ходит.",
	"Запись / Архив: После включения питания ищи пропуск сотрудника в комнате архива справа.",
	"Запись / Сотрудник: Дверь из левого архива в коридор открывается с нашей стороны. Это короткий путь к изолятору.",
	"Запись / Карантин: Терминал слева ждёт три узла исследовательского крыла справа. После узлов спустись по лестнице за операционной. Для очистки реактора нужны хладагент из насосной и топливо из ремонта. Хранитель блокирует последний верхний узел.",
	"Запись / Последняя смена: Я оставил стул в приёмной ровно. Если он лежит — здесь прошёл кто-то ещё.",
	"Запись / Ирина: Я обещала вернуться к утру. Они велели выключить запад и оставить восток под током. За стеклом было тихо, пока мы не запустили ручной привод. Теперь я иду босиком.",
	"Запись / Мастерская: У него затянуты глаза, но он слышит шаги, привод и разбитое стекло. Ручной стопор лифта снимается у восточной стены дальней спальни. Если услышишь тяжёлые шаги — остановись. Бутылку бросай подальше от своего укрытия.",
]

var hero: CharacterBody3D
var facility: Node3D
var hud: CanvasLayer
var cores: Array[Area3D] = []
var batteries: Array[Area3D] = []
var medkits: Array[Area3D] = []
var mission: Node
var horror: Node
var front_end: CanvasLayer
var in_title_menu: bool = false
var checkpoint: Dictionary = {}
var settings: Dictionary = {"fullscreen": true, "effects": 0.8, "music": 0.7, "sensitivity": 1.0, "camera_motion": true}
var hunters: Array[CharacterBody3D] = []
var navigation := Navigation.new()
var feedback: Node3D
var sound_bank: Node
var recovered_count: int = 0
var has_won: bool = false
var has_lost: bool = false
var is_paused: bool = false
var automated: bool = false
var navigation_ready: bool = false
var navigation_refresh_pending: bool = false
var health: int = 3
var flash_charges: int = 1
var flash_cooldown: float = 0.0
var grace_remaining: float = INITIAL_GRACE
var escape_remaining: float = -1.0
var elapsed: float = 0.0
var score: int = 0
var best_score: int = 0
var combo: int = 0
var combo_remaining: float = 0.0
var heartbeat_remaining: float = 0.0
var alarm_stage: int = 1
var step_remaining: float = 0.0
var growl_remaining: float = 8.0
var combat: Node3D
var resource_layout := ResourceLayout.new()
var inventory: Node
var exploration := preload("res://scripts/exploration.gd").new()
var guardian: Node
var layout_seed: int = 1907
var show_ai: bool = false
var survival_tools: Node
var residential: Node3D
var refuge: Node3D
var pacing: Node
var diagnostics: Node3D
var upgrades: Node3D
var isolation: Node3D
var annex: Node3D
var saves: Node
var campaign_started: bool = false
var difficulty_id: String = "normal"
var presentation:CanvasLayer
var admin:Node
var run_generation:int=0
var respawn_pending:bool=false
var auto_respawn_enabled:bool=true


func _ready() -> void:
	add_to_group("game")
	process_mode = Node.PROCESS_MODE_ALWAYS
	automated = get_meta("automated_test", false) or OS.get_cmdline_user_args().has("--automated-test") or DisplayServer.get_name() == "headless"
	campaign_started = automated
	_register_inputs()
	var world := Node3D.new()
	world.name = "World"
	world.process_mode = Node.PROCESS_MODE_PAUSABLE
	add_child(world)
	facility = Facility.new()
	world.add_child(facility)
	refuge = preload("res://scripts/refuge_room.gd").new()
	world.add_child(refuge)
	refuge.build(facility)
	diagnostics = preload("res://scripts/diagnostic_chapter.gd").new()
	world.add_child(diagnostics)
	diagnostics.build(facility)
	isolation = preload("res://scripts/isolation_chapter.gd").new()
	world.add_child(isolation)
	isolation.build(facility)
	annex = preload("res://scripts/service_annex.gd").new()
	world.add_child(annex)
	annex.build(facility)
	residential = facility.residential
	facility.layout_changed.connect(_refresh_navigation)
	hero = HeroScene.instantiate()
	hero.position = Facility.START
	world.add_child(hero)
	feedback = Feedback.new()
	world.add_child(feedback)
	sound_bank = SoundBank.new()
	sound_bank.enabled = not automated
	sound_bank.listener = hero
	add_child(sound_bank)
	hero.dash_started.connect(_on_dash)
	for location: Vector3 in Facility.CORE_POSITIONS:
		var core := EnergyCore.new()
		core.position = location
		core.record_index = cores.size()
		world.add_child(core)
		core.recovered.connect(_on_core_recovered)
		cores.append(core)
	for location: Vector3 in [Vector3(-27, 0, 12), Vector3(12, 0, -6)]:
		var battery := EnergyCore.new()
		battery.kind = "battery"
		battery.position = location
		world.add_child(battery)
		battery.recovered.connect(_on_battery_recovered)
		batteries.append(battery)
	for location: Vector3 in [Vector3(-25.5, 0, 12), Vector3(26, 0, 4), Vector3(56, 0, 4)]:
		var medkit := EnergyCore.new()
		medkit.kind = "medkit"
		medkit.position = location
		world.add_child(medkit)
		medkit.recovered.connect(_on_medkit_recovered)
		medkits.append(medkit)
	for location: Vector3 in [Vector3(-9, 0.1, 3), Vector3(-27, 0.1, 14)]:
		var hunter := Hunter.new()
		hunter.position = location
		hunter.crawler = hunters.size() in [0, 2, 4, 6, 8]
		world.add_child(hunter)
		hunters.append(hunter)
	hunters[0].patrol_points.assign([Vector3(-9, 0.1, 3), Vector3(-9, 0.1, 10), Vector3(0, 0.1, 5), Vector3(9, 0.1, 3)])
	hunters[1].patrol_points.assign([Vector3(-27, 0.1, 14), Vector3(-27, 0.1, 9)])
	for location: Vector3 in [Vector3(26, 0.1, 19), Vector3(39, 0.1, 18), Vector3(56, 0.1, 11), Vector3(52, 0.1, 9.4)]:
		var hunter := Hunter.new()
		hunter.position = location
		if hunters.size() == 5:
			hunter.max_health = 12
		hunter.use_minimax = hunters.size() in [3, 5]
		world.add_child(hunter)
		hunter.patrol_points.assign([location, location + Vector3(0, 0, -5)])
		hunters.append(hunter)
	hud = GameHUD.new()
	add_child(preload("res://scripts/cinematic_finish.gd").new())
	for location: Vector3 in [Vector3(31, 0.1, -12), Vector3(45, 0.1, -12), Vector3(40, -4.9, 71), Vector3(56, -4.9, 65), Vector3(48, -4.9, 77)]:
		var hunter := Hunter.new()
		hunter.position = location
		hunter.use_minimax = hunters.size() in [7, 10]
		world.add_child(hunter)
		hunter.patrol_points.assign([location, location + Vector3(0, 0, -4)])
		hunters.append(hunter)
	for location: Vector3 in [Vector3(72,.1,22),Vector3(100,.1,24),Vector3(101,.1,5)]:
		var hunter := Hunter.new()
		hunter.position = location
		hunter.crawler = hunters.size()==11
		if hunters.size()==13:
			hunter.max_health = 12
			hunter.alert_pursuer = true
			hunter.blind_hunter = true
		hunter.use_minimax = hunters.size()==13
		world.add_child(hunter)
		hunter.patrol_points.assign([location,location+Vector3(0,0,-3)])
		hunters.append(hunter)
	for index: int in [3, 9, 12]:
		hunters[index].can_spit = true
		hunters[index].ranged_attack.connect(_spawn_projectile)
	# Once released, the blind creature roams the ward instead of returning to its cage.
	hunters[13].patrol_points.assign([Vector3(94,.1,5),Vector3(94,.1,23),Vector3(82,.1,27),Vector3(82,.1,6)])
	add_child(hud)
	hud.radar.game = self
	hud.restart_requested.connect(restart_game)
	hud.resume_requested.connect(func() -> void: set_paused(false))
	mission = Mission.new()
	get_node("World").add_child(mission)
	mission.configure(self, world)
	combat = Combat.new()
	world.add_child(combat)
	combat.configure(self)
	residential.configure(self)
	horror = HorrorDirector.new()
	world.add_child(horror)
	horror.configure(self)
	survival_tools = preload("res://scripts/survival_tools.gd").new()
	survival_tools.add_to_group("survival_tools")
	world.add_child(survival_tools)
	survival_tools.configure(self)
	inventory = preload("res://scripts/field_inventory.gd").new()
	add_child(inventory)
	inventory.configure(self)
	guardian = preload("res://scripts/guardian_encounter.gd").new()
	world.add_child(guardian)
	guardian.configure(self)
	refuge.configure(self)
	pacing = preload("res://scripts/pacing_director.gd").new()
	add_child(pacing)
	pacing.configure(self)
	diagnostics.configure(self)
	upgrades = preload("res://scripts/survival_upgrade.gd").new()
	world.add_child(upgrades)
	upgrades.configure(self)
	isolation.configure(self)
	annex.configure(self)
	saves = preload("res://scripts/save_manager.gd").new()
	add_child(saves)
	saves.configure(self)
	presentation=preload("res://scripts/display_preferences.gd").new()
	add_child(presentation)
	presentation.configure(self)
	front_end = FrontEnd.new()
	add_child(front_end)
	front_end.new_game_requested.connect(_new_game)
	front_end.continue_requested.connect(func()->void:saves.load_latest())
	front_end.settings_changed.connect(func(values: Dictionary) -> void: apply_settings(values, true))
	front_end.saves_requested.connect(func()->void:saves.open_browser("load"))
	admin=preload("res://scripts/admin_mode.gd").new()
	add_child(admin);admin.configure(self)
	front_end.settings_view.admin_requested.connect(admin.open_access)
	hud.menu_requested.connect(show_title_menu)
	hud.checkpoint_requested.connect(continue_checkpoint)
	hud.save_requested.connect(func()->void:saves.open_browser("save"))
	hud.announce("Главный вход заблокирован. Найди план у стойки регистрации. F — взаимодействие.", 8.0)
	if not automated:
		_load_record()
		_load_application_data()
	apply_settings(settings,not automated)
	_capture_mouse()
	_prepare_navigation.call_deferred()
	if not automated:
		show_title_menu.call_deferred()


func _prepare_navigation() -> void:
	await get_tree().physics_frame
	await get_tree().process_frame
	navigation.build(get_world_3d().direct_space_state)
	for hunter: CharacterBody3D in hunters:
		hunter.configure(hero, navigation)
	navigation_ready = true
	resource_layout.apply(self, layout_seed)
	await get_tree().physics_frame
	await get_tree().process_frame
	navigation.build(get_world_3d().direct_space_state)
	hunters[1].set_active(true)


func _refresh_navigation() -> void:
	if not navigation_ready or navigation_refresh_pending:
		return
	navigation_refresh_pending = true
	await get_tree().physics_frame
	await get_tree().process_frame
	navigation.build(get_world_3d().direct_space_state)
	for hunter: CharacterBody3D in hunters:
		hunter.repath_remaining = 0
		hunter.route.clear()
	navigation_refresh_pending = false


func _physics_process(delta: float) -> void:
	if in_title_menu or is_paused or has_won or has_lost:
		return
	refuge.update()
	elapsed += delta
	exploration.update(hero.position)
	var focused: Node3D = _interaction_target()
	hero.inspecting = focused!=null and focused.get("storage")!=null and focused.storage.opened
	combat.update(delta)
	guardian.update(delta)
	survival_tools.update(delta)
	mission.update(delta)
	residential.update(delta)
	horror.update(delta)
	pacing.update(delta)
	diagnostics.update(delta)
	upgrades.update(delta)
	isolation.update(delta)
	annex.update(delta)
	saves.update(delta)
	sound_bank.update_world(delta,self)
	step_remaining -= delta
	if Vector2(hero.velocity.x, hero.velocity.z).length() > 0.5 and hero.is_on_floor() and step_remaining <= 0:
		sound_bank.footstep(self)
		var loud: bool = Input.is_action_pressed("sprint") and not hero.exhausted and hero.stamina > 0
		emit_noise(hero.global_position, 10.0 if loud else (0.65 if hero.crouching else 2.0))
		step_remaining = 0.32 if loud else 0.48
	growl_remaining -= delta
	if growl_remaining <= 0:
		for hunter: CharacterBody3D in hunters:
			if hunter.active:
				sound_bank.play_spatial("growl", hunter.global_position)
		growl_remaining = randf_range(6.0, 11.0)
	grace_remaining = maxf(0, grace_remaining - delta)
	flash_cooldown = maxf(0, flash_cooldown - delta)
	combo_remaining = maxf(0, combo_remaining - delta)
	if combo_remaining <= 0:
		combo = 0
	var nearest: float = INF
	for hunter: CharacterBody3D in hunters:
		hunter.move_speed = ((3.4 if hunter.max_health > 6 else 3.0) + alarm_stage * 0.2)*float(difficulty_value("speed"))
		if not hunter.active or hunter.stunned_remaining > 0:
			continue
		var distance: float = hunter.global_position.distance_to(hero.global_position)
		if not hero.in_refuge: nearest = minf(nearest,distance)
		if not hero.in_refuge and hero.hiding_bed == null and distance < 1.3 and hunter.melee_contact_ready() and hero.invulnerable_remaining <= 0 and hero.dash_remaining <= 0 and _line_clear(hero.global_position, hunter.global_position):
			_take_hit(hunter)
			if has_lost:
				return
	heartbeat_remaining -= delta
	if nearest < 6 and heartbeat_remaining <= 0:
		sound_bank.play_cue("heartbeat", 1.0 + (6.0 - nearest) * 0.06)
		heartbeat_remaining = 0.9
	if escape_remaining >= 0:
		escape_remaining = maxf(0, escape_remaining - delta)
		if escape_remaining <= 0:
			_finish_round(false, "Защитная заслонка закрылась. Ты не успел добраться до выхода.")
			return
	if mission.exit_started and diagnostics.exit_released:
		var offset: Vector3 = hero.global_position - Facility.EXIT
		if Vector2(offset.x, offset.z).length() < 1.6 and absf(offset.y) < 1.2:
			_finish_round(true)
			return
	hud.update_hunt(health, flash_charges, hero.dash_cooldown, score, combo,
		nearest, grace_remaining, escape_remaining, best_score)
	var target: Node3D = _interaction_target()
	hud.set_interaction("F — выбраться из-под кровати" if hero.hiding_bed != null else (target.prompt() if target != null else ""),target)
	if show_ai:
		var decision: Dictionary = {}
		for hunter: CharacterBody3D in hunters:
			if not hunter.tactical.last_result.is_empty():
				decision = hunter.tactical.last_result
		var layout: Dictionary = resource_layout.last_result
		hud.set_ai_debug("A*: раскрыто %d узлов\nMinimax / alpha-beta: %s\nCSP: seed %d · назначений %d · возвратов %d" % [navigation.last_expanded, ("%d узлов · %d отсечений" % [decision["nodes"], decision["prunes"]]) if not decision.is_empty() else "ожидает видимой цели", layout_seed, layout.get("assignments", 0), layout.get("backtracks", 0)])


func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo and event.physical_keycode == KEY_F11:
		var values: Dictionary = settings.duplicate()
		values["fullscreen"] = not values["fullscreen"]
		apply_settings(values, true)
		return
	if in_title_menu:
		return
	if diagnostics.screen.mode!="closed":
		diagnostics.screen._unhandled_input(event)
		return
	if event is InputEventKey and event.pressed and not event.echo:
		match event.physical_keycode:
			KEY_F5:
				saves.capture("manual_1")
			KEY_F3:
				show_ai = not show_ai
				hud.ai_label.visible = show_ai
			KEY_ESCAPE:
				if not has_won and not has_lost:
					set_paused(not is_paused)
			KEY_R:
				combat.reload_weapon()
			KEY_Z:
				if not is_paused: combat.cycle_weapon()
			KEY_K:
				upgrades.start_barricade()
			KEY_L:
				if not is_paused and not has_won and not has_lost: upgrades.replay_record()
			KEY_I:
				inventory.screen.open_inventory()
			KEY_X:
				inventory.screen.open_inventory("badge" if mission.has_badge else "weapon")
			KEY_H:
				if not is_paused: inventory.use_item("medkit")
			KEY_T:
				if not is_paused: inventory.use_item("battery")
			KEY_F6:
				restart_game()
			KEY_V:
				combat.knife_attack()
			KEY_G:
				survival_tools.throw_bottle()
			KEY_B:
				if hero.hiding_bed == null and not is_paused and not has_won and not has_lost:
					hero.camera_rig.flashlight_enabled = not hero.camera_rig.flashlight_enabled
			KEY_E:
				use_flash()
			KEY_F:
				interact()
			KEY_C:
				if not is_paused and not has_won and not has_lost:
					for box: Node3D in get_tree().get_nodes_in_group("search_storage"):
						if box.opened and box.global_position.distance_to(hero.global_position)<2.2 and _prop_reachable(box.prop): box.close_drawer()
			KEY_TAB:
				if mission.has_map:
					hud.radar.visible = not hud.radar.visible
				else:
					hud.announce("Сначала возьми план у стойки регистрации в приёмной.", 4)
			KEY_J:
				if not has_won and not has_lost:
					if is_paused:
						set_paused(false)
					else:
						set_paused(true)
						var entries: Array[String] = []
						for core: Area3D in cores:
							if core.is_recovered:
								entries.append(STORIES[core.record_index])
						entries.append_array(diagnostics.journal_entries())
						entries.append_array(upgrades.journal_entries())
						entries.append_array(isolation.journal_entries())
						entries.append_array(annex.journal_entries())
						hud.show_journal(entries)
			KEY_M:
				sound_bank.toggle_mute(is_paused or has_won or has_lost)
				hud.announce("Звук выключен" if sound_bank.muted else "Звук включён", 2.0)
	elif event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		if not is_paused and not has_won and not has_lost:
			_capture_mouse()
			combat.fire()


func use_flash() -> int:
	if hero.in_refuge or hero.hiding_bed != null or hero.performing_action or is_paused or has_won or has_lost or flash_cooldown > 0:
		return 0
	if flash_charges <= 0:
		hud.announce("Нет зарядов вспышки. Батареи есть на медскладе и в архиве.", 3.0)
		return 0
	flash_charges -= 1
	flash_cooldown = 0.7
	feedback.pulse(hero.position, 6.5)
	sound_bank.play_cue("flash")
	emit_noise(hero.global_position, 14)
	var affected: int = 0
	for hunter: CharacterBody3D in hunters:
		if not hunter.active or hunter.blind_hunter or hunter.global_position.distance_to(hero.global_position) > 6.5:
			continue
		var ray := PhysicsRayQueryParameters3D.create(hero.global_position + Vector3(0, 1, 0),
			hunter.global_position + Vector3(0, 1, 0), 1)
		if get_world_3d().direct_space_state.intersect_ray(ray).is_empty():
			hunter.stun(3.5)
			affected += 1
	hud.announce("Свет удерживает их. Беги!" if affected > 0 else "Вспышка рассеялась в тумане.", 2.5)
	return affected


func _interaction_target() -> Node3D:
	if hero.hiding_bed!=null: return hero.hiding_bed
	var candidates: Array[Node3D] = []
	for station: Node3D in get_tree().get_nodes_in_group("save_stations"):
		if hero.position.distance_to(station.position)<1.9: candidates.append(station)
	for station: Node3D in get_tree().get_nodes_in_group("upgrade_stations"):
		if (not station.used or station.station_id.begins_with("record_")) and hero.global_position.distance_to(station.global_position)<2: candidates.append(station)
	for station: Node3D in get_tree().get_nodes_in_group("chapter_stations"):
		if hero.global_position.distance_to(station.global_position)<2: candidates.append(station)
	for station: Node3D in get_tree().get_nodes_in_group("refuge_stations"):
		if hero.global_position.distance_to(station.global_position)<2: candidates.append(station)
	for bed: Node3D in get_tree().get_nodes_in_group("hide_beds"):
		if hero.global_position.distance_to(bed.global_position)<1.75: candidates.append(bed)
	for core: Area3D in cores+batteries+medkits:
		if not core.is_recovered and hero.global_position.distance_to(core.global_position)<1.8: candidates.append(core)
	for supply: Node3D in get_tree().get_nodes_in_group("dropped_supplies"):
		if hero.global_position.distance_to(supply.global_position)<1.8: candidates.append(supply)
	for object: Node3D in mission.objects.values():
		if not object.used and hero.global_position.distance_to(object.global_position)<2: candidates.append(object)
	for door: Node3D in facility.doors:
		if not door.busy and not door.breached and hero.global_position.distance_to(door.global_position)<2.3: candidates.append(door)
	var target: Node3D
	var best: float = INF
	var forward: Vector3 = hero.camera_rig.movement_direction(Vector2.UP)
	for object: Node3D in candidates:
		if not _prop_reachable(object): continue
		var offset: Vector3 = object.global_position-hero.global_position
		offset.y = 0
		var score: float = offset.length()+(1-forward.dot(offset.normalized()))*.9
		if score<best:
			best = score
			target = object
	return target


func interact() -> void:
	if is_paused or has_won or has_lost:
		return
	var target: Node3D = _interaction_target()
	if target!=null and target.is_in_group("save_stations"):
		target.use_station()
		return
	if target!=null and target.is_in_group("upgrade_stations"):
		target.use_station()
		return
	if target!=null and target.is_in_group("chapter_stations"):
		target.use_station()
		return
	if hero.hiding_bed != null:
		survival_tools.leave_bed()
		return
	if target!=null and target.is_in_group("refuge_stations"):
		target.use_station()
		return
	if target != null and target.is_in_group("hide_beds"):
		if hero.free_flight:return
		survival_tools.try_hide(target)
		return
	if target!=null and target.is_in_group("dropped_supplies"):
		target.collect()
		return
	if target is MissionObject:
		if target.storage!=null and not target.storage.interact(): return
		if target.object_id in ["power", "reactor", "purge_valve_a", "purge_valve_b", "residence_core", "residence_lift", "residence_plate"]:
			survival_tools.start_action(target)
		else:
			mission.interact(target)
	elif target is Area3D:
		if target.storage!=null and not target.storage.interact(): return
		if target.kind in ["medkit","battery"] and not inventory.can_take(target.kind):
			return
		target.try_collect(hero)
	elif target != null:
		if upgrades.interact_door(target): return
		if diagnostics.interact_door(target): return
		if isolation.interact_door(target): return
		if annex.interact_door(target): return
		if target.door_id.begins_with("res_"):
			residential.interact_door(target)
			return
		if target.locked:
			if target.door_id=="quarantine_gate" and mission.has_badge and not inventory.quarantine_code:
				inventory.screen.open_keypad(target)
				return
			match target.lock_kind:
				"guardian":
					if mission.has_badge: guardian.start_encounter()
					hud.announce("Дверь заклинило. С другой стороны слышны удары.",4)
					return
				"containment":
					if mission.relays.size() < 3:
						hud.announce("Лестница нижнего сектора заблокирована. Включи все три аварийных узла.", 5)
						return
				"power":
					hud.announce("Электрозамок обесточен. Нужен щиток в служебном крыле.", 5)
					return
				"badge":
					if not mission.has_badge:
						hud.announce("Карантин. Требуется пропуск сотрудника из архива.", 5)
						return
				"inside":
					if target.to_local(hero.global_position).z >= -0.2:
						hud.announce("Засов с другой стороны. Найди обходной путь.", 4)
						return
			target.locked = false
			hud.announce("Короткий путь открыт." if target.lock_kind == "inside" else "Доступ разрешён.", 3)
		if target.toggle_door(hero.global_position):
			sound_bank.play_spatial("door", target.global_position)
			emit_noise(target.global_position, 11)
		else:
			hud.announce("Отойди от створки, чтобы закрыть дверь.", 2.0)


func emit_noise(location: Vector3, radius: float, source: CharacterBody3D = null) -> void:
	if source==null and hero!=null and location.distance_to(hero.global_position)<3:hud.note_noise(radius)
	for hunter: CharacterBody3D in hunters:
		if hunter==source: continue
		hunter.hear_noise(location, radius)


func _line_clear(origin: Vector3, destination: Vector3) -> bool:
	var ray := PhysicsRayQueryParameters3D.create(origin + Vector3(0, 1.2, 0), destination + Vector3(0, 1.2, 0), 1)
	return get_world_3d().direct_space_state.intersect_ray(ray).is_empty()


func _take_hit(hunter: CharacterBody3D) -> void:
	if hero.free_flight:return
	if refuge.contains(hero.position): return
	survival_tools.cancel_action()
	diagnostics.cancel_work()
	isolation.cancel_work()
	annex.cancel_work()
	upgrades.cancel_barricade()
	hero.react_hit(hunter.global_position)
	health -= 1
	hero.invulnerable_remaining = float(difficulty_value("invulnerability"))
	hero.camera_rig.shake_remaining = 0.45
	hunter.stun(0.8)
	feedback.burst(hero.position + Vector3(0, 1, 0), "dca184")
	hud.flash_damage()
	sound_bank.play_cue("hurt")
	combo = 0
	combo_remaining = 0
	if health <= 0:
		_finish_round(false, "Охотники услышали твои шаги.")
	else:
		hud.announce("Тебя задели! Рывок Q или вспышка E помогут оторваться.", 3)


func _spawn_projectile(origin: Vector3, point: Vector3) -> void:
	var projectile := Acid.new()
	projectile.game = self
	projectile.position = origin
	projectile.heading = (point - origin).normalized()
	feedback.add_child(projectile)
	sound_bank.play_spatial("whisper", origin - Vector3(0, 1.2, 0), 1.5)


func take_projectile_hit() -> void:
	if hero.free_flight:return
	if refuge.contains(hero.position): return
	survival_tools.cancel_action()
	diagnostics.cancel_work()
	isolation.cancel_work()
	annex.cancel_work()
	upgrades.cancel_barricade()
	hero.react_hit(hero.global_position+hero.camera_rig.movement_direction(Vector2.UP))
	health -= 1
	hero.invulnerable_remaining = float(difficulty_value("invulnerability"))
	hero.camera_rig.shake_remaining = 0.4
	hud.flash_damage()
	sound_bank.play_cue("hurt")
	if health <= 0:
		_finish_round(false, "Кислота прожгла защиту. Используй укрытия и уходи в сторону.")
	else:
		hud.announce("Ожог! Прячься за оборудованием или уходи в сторону во время замаха.", 4)


func _on_dash() -> void:
	feedback.burst(hero.position + Vector3(0, 0.7, 0), "d2dccc")
	sound_bank.play_cue("dash")
	emit_noise(hero.global_position, 12)


func _on_core_recovered(core: Area3D) -> void:
	if has_won or has_lost:
		return
	recovered_count += 1
	combo = mini(combo + 1, 4) if combo_remaining > 0 else 1
	combo_remaining = 7.0
	score += 100 * combo
	feedback.burst(core.position + Vector3(0, 1, 0), "a5e1c7")
	sound_bank.play_cue("core", 0.8 + combo * 0.1)
	hud.announce(STORIES[core.record_index], 9)
	mission.refresh_hud()


func _on_battery_recovered(battery: Area3D) -> void:
	inventory.take_supply("battery")
	score += 50
	feedback.burst(battery.position + Vector3(0, 1, 0), "e6cf9e")
	sound_bank.play_cue("core", 0.75)
	hud.announce("Батарея в инвентаре. T — зарядить вспышку, когда понадобится.", 4)


func _on_medkit_recovered(_medkit: Area3D) -> void:
	inventory.take_supply("medkit")
	score += 50
	hud.announce("Аптечка в инвентаре. H — перевязать рану, I — снаряжение.", 4)
	sound_bank.play_cue("core", 0.7)


func _finish_round(won: bool, reason: String = "") -> void:
	if respawn_pending:return
	if not won and auto_respawn_enabled:
		respawn_pending=true;has_lost=true
		survival_tools.cancel_action();diagnostics.cancel_work();isolation.cancel_work();annex.cancel_work();upgrades.cancel_barricade()
		get_tree().paused=true
		if admin!=null:admin.pilot.release_controls()
		_respawn_latest.call_deferred()
		return
	upgrades.cancel_barricade()
	upgrades.status.hide()
	isolation.cancel_work()
	annex.cancel_work()
	isolation.status.hide()
	diagnostics.screen.dismiss(false)
	inventory.screen.dismiss(false)
	refuge.screen.dismiss(false)
	guardian.status.hide()
	if not won:
		hero.model.play_death()
		combat.drop_weapon()
	has_won = won
	has_lost = not won
	combat.overlay.hide()
	survival_tools.status.hide()
	if won:
		score += health * 200 + maxi(0, roundi(escape_remaining)) * 5
	best_score = maxi(best_score, score)
	if not automated:
		_save_record()
	get_tree().paused = true
	hero.camera_rig.input_enabled = false
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	sound_bank.set_suspended(true)
	sound_bank.play_cue("win" if won else "lose")
	if won:
		diagnostics.show_ending()
		hud.show_overlay(true)
		hud.body_scroll.custom_minimum_size.y = 210
		hud.overlay_body.text = diagnostics.ending()+"\n\nАктивное время: %02d:%02d\nСчёт: %d   Рекорд: %d" % [int(elapsed)/60,int(elapsed)%60,score,best_score]
	else:
		hud.show_defeat(reason, score)

func _respawn_latest()->void:
	if not respawn_pending:return
	var loaded:bool=saves.load_latest()
	if not loaded:restart_game()
	hero.invulnerable_remaining=maxf(hero.invulnerable_remaining,2.0)
	hud.announce("Последнее сохранение загружено." if loaded else "Сохранений пока нет. Возвращение к началу игры.",4)
	respawn_pending=false


func set_paused(value: bool) -> void:
	if has_won or has_lost:
		return
	is_paused = value
	survival_tools.status.visible = not value
	get_tree().paused = value
	hero.camera_rig.input_enabled = not value and (admin==null or not admin.active)
	sound_bank.set_suspended(value)
	if value:
		annex.cancel_work()
		combat.overlay.hide()
		Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
		hud.show_overlay(false)
		hud.overlay_body.text += "\nРежим: "+preload("res://scripts/difficulty_profile.gd").NAMES[difficulty_id]
	else:
		hud.hide_overlay()
		_capture_mouse()


func restart_game() -> void:
	run_generation+=1
	respawn_pending=false
	hero.free_flight=false
	campaign_started = true
	if saves!=null: saves.screen.dismiss(false)
	if refuge!=null: refuge.reset_room()
	if inventory!=null: inventory.reset_inventory()
	exploration.visited.clear()
	exploration.current = ""
	in_title_menu = false
	front_end.hide()
	hud.show()
	get_tree().paused = false
	is_paused = false
	has_won = false
	has_lost = false
	recovered_count = 0
	health = 3
	flash_charges = 1
	flash_cooldown = 0
	grace_remaining = INITIAL_GRACE
	escape_remaining = -1
	elapsed = 0
	score = 0
	combo = 0
	combo_remaining = 0
	heartbeat_remaining = 0
	alarm_stage = 1
	step_remaining = 0
	growl_remaining = 8
	hero.reset_to(Facility.START)
	combat.reset_combat()
	survival_tools.reset_tools()
	hero.camera_rig.input_enabled = true
	for core: Area3D in cores + batteries + medkits:
		core.reset_core()
	for hunter: CharacterBody3D in hunters:
		hunter.reset_hunter()
	feedback.reset_effects()
	facility.set_alarm(1)
	facility.reset_doors()
	facility.set_exit_ready(false)
	mission.reset_mission()
	residential.reset_chapter()
	diagnostics.reset_chapter()
	upgrades.reset_upgrade()
	isolation.reset_chapter()
	annex.reset_chapter()
	guardian.reset_encounter()
	horror.reset_director()
	if pacing!=null: pacing.reset_pacing(layout_seed)
	sound_bank.reset_foley()
	hunters[1].set_active(true)
	hud.hide_overlay()
	hud.reset_feedback()
	hud.announce("Главный вход заперт.\nВ отделении остался кто-то ещё.", 5)
	sound_bank.set_suspended(false)
	_capture_mouse()
	if admin!=null:admin.restore_after_reset.call_deferred()


func _capture_mouse() -> void:
	if not automated and not in_title_menu:
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED


func _exit_tree() -> void:
	get_tree().paused = false
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE


func _notification(what: int) -> void:
	if what == NOTIFICATION_APPLICATION_FOCUS_OUT and not automated and not in_title_menu:
		if is_instance_valid(hero) and is_instance_valid(hud) and not has_won and not has_lost:
			set_paused(true)


func _register_inputs() -> void:
	var bindings: Dictionary = {
		"move_left": [KEY_A, KEY_LEFT], "move_right": [KEY_D, KEY_RIGHT],
		"move_forward": [KEY_W, KEY_UP], "move_back": [KEY_S, KEY_DOWN],
		"sprint": [KEY_SHIFT], "jump": [KEY_SPACE], "dash": [KEY_Q], "crouch": [KEY_CTRL], "interact_hold": [KEY_F],
		"barricade": [KEY_K],
	}
	for action: String in bindings:
		if not InputMap.has_action(action):
			InputMap.add_action(action)
		for key: int in bindings[action]:
			var event := InputEventKey.new()
			event.physical_keycode = key
			if not InputMap.action_has_event(action, event):
				InputMap.action_add_event(action, event)
	if not InputMap.has_action("aim"):
		InputMap.add_action("aim")
		var mouse := InputEventMouseButton.new()
		mouse.button_index = MOUSE_BUTTON_RIGHT
		InputMap.action_add_event("aim", mouse)


func _load_record() -> void:
	if not FileAccess.file_exists("user://echo_record.json"):
		return
	var data = JSON.parse_string(FileAccess.get_file_as_string("user://echo_record.json"))
	if data is Dictionary:
		best_score = maxi(0, int(data.get("best", 0)))


func _save_record() -> void:
	var file := FileAccess.open("user://echo_record.json", FileAccess.WRITE)
	if file != null:
		file.store_string(JSON.stringify({"best": best_score}))


func show_title_menu() -> void:
	if admin!=null and admin.active:admin.leave()
	in_title_menu = true
	is_paused = true
	get_tree().paused = true
	hero.camera_rig.input_enabled = false
	hero.model.hide()
	if combat != null:
		combat.weapon.hide()
		combat.overlay.hide()
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	hud.hide_overlay()
	hud.hide()
	front_end.show_menu(Checkpoints.valid(checkpoint))
	# The title screen has the room tone while its world is paused.
	sound_bank.set_suspended(false)
	sound_bank.set_tension(0)


func _new_game() -> void:
	saves.visited.clear()
	difficulty_id = settings.get("difficulty","normal")
	layout_seed = 1907 if automated else randi_range(1, 1000000)
	checkpoint.clear()
	hud.checkpoint_available = false
	if not automated:
		var file := FileAccess.open("user://echo_checkpoint.json", FileAccess.WRITE)
		if file != null:
			file.store_string(JSON.stringify({"version": 1, "active": false}))
	restart_game()
	if navigation_ready:
		resource_layout.apply(self, layout_seed)


func save_checkpoint(manual: bool = false) -> bool:
	if mission.exit_started or (not manual and not mission.power_on) or (manual and not refuge.contains(hero.position)):
		return false
	var data:Dictionary = Checkpoints.snapshot(self)
	if manual:
		data["manual_refuge"] = true
		data["health"] = health
		data["charges"] = flash_charges
	return saves.commit("manual_1",data) if manual else saves.record_checkpoint(data)


func continue_checkpoint() -> bool:
	if not Checkpoints.valid(checkpoint):
		return false
	return Checkpoints.apply(self, checkpoint.duplicate(true))


func _load_application_data() -> void:
	if checkpoint.is_empty() and FileAccess.file_exists("user://echo_checkpoint.json"):
		var parser:=JSON.new()
		parser.parse(FileAccess.get_file_as_string("user://echo_checkpoint.json"))
		var data:Variant=parser.data
		if Checkpoints.valid(data):
			checkpoint = data
			hud.checkpoint_available = true
	if FileAccess.file_exists("user://echo_settings.json"):
		var parser:=JSON.new()
		parser.parse(FileAccess.get_file_as_string("user://echo_settings.json"))
		var values:Variant=parser.data
		if values is Dictionary:
			settings = normalise_settings(values)
			if values.get("presentation_revision",0)!=2:settings.fullscreen=true


func normalise_settings(values: Dictionary) -> Dictionary:
	return preload("res://scripts/presentation_settings.gd").normalize(values)

func apply_settings(values: Dictionary, persist: bool = false) -> void:
	settings = normalise_settings(values)
	TextLocale.set_language(settings.language)
	preload("res://scripts/environment_materials.gd").set_quality(settings.texture_quality)
	front_end.set_values(settings)
	presentation.apply(settings)
	hero.camera_rig.look_sensitivity = 0.003 * settings["sensitivity"]
	hero.camera_rig.motion_enabled = settings["camera_motion"]
	sound_bank.set_levels(settings["effects"], settings["music"])
	if not automated:
		DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_FULLSCREEN if settings["fullscreen"] else DisplayServer.WINDOW_MODE_WINDOWED)
		if persist:
			var file := FileAccess.open("user://echo_settings.json", FileAccess.WRITE)
			if file != null:
				file.store_string(JSON.stringify(settings))

func _prop_reachable(object: Node3D) -> bool:
	var query := PhysicsRayQueryParameters3D.create(hero.global_position+Vector3(0,1.2,0),object.global_position+Vector3(0,.8,0),1)
	if object.has_method("interaction_rids"): query.exclude = object.interaction_rids()
	else:
		var excluded: Array[RID] = []
		for child: CollisionObject3D in object.find_children("*","CollisionObject3D",true,false): excluded.append(child.get_rid())
		query.exclude = excluded
	return get_world_3d().direct_space_state.intersect_ray(query).is_empty()

func difficulty_value(key: String) -> Variant:
	return preload("res://scripts/difficulty_profile.gd").value(difficulty_id,key)

func _restore_exact_position(data: Dictionary) -> void:
	await get_tree().physics_frame
	var at := Vector3(data.at[0],data.at[1],data.at[2])
	var shape := CapsuleShape3D.new()
	shape.radius = .34
	shape.height = 1.7
	var query := PhysicsShapeQueryParameters3D.new()
	query.shape = shape
	query.transform.origin = at+Vector3(0,.9,0)
	query.collision_mask = 1
	var excluded: Array[RID] = []
	for door: Node3D in facility.doors:
		if door.is_open or door.breached: excluded.append(door.body.get_rid())
	query.exclude = excluded
	var space: PhysicsDirectSpaceState3D = get_world_3d().direct_space_state
	var floor_query := PhysicsRayQueryParameters3D.create(at+Vector3(0,.35,0),at-Vector3(0,.65,0),1)
	if space.intersect_shape(query,1).is_empty() and not space.intersect_ray(floor_query).is_empty():
		var protection:float=hero.invulnerable_remaining
		hero.reset_to(at)
		hero.invulnerable_remaining=protection
		hero.camera_rig.rotation.y = float(data.yaw)
		hero.camera_rig.rotation.x = float(data.pitch)
	else: hud.announce("Место записи недоступно. Загружена безопасная контрольная точка.",5)
