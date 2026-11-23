extends Node
## Stealth is earned by breaking sight. A witnessed bed is remembered by that hunter.
const Bottle = preload("res://scripts/thrown_bottle.gd")
var game: Node3D
var bottles: int = 3
var throw_cooldown: float = 0
var entry_position: Vector3
var breath_remaining: float = 3
var breath_exhausted: bool = false
var action_object: Node3D
var work: Dictionary = {}
var status: Label

func configure(owner_game: Node3D) -> void:
	game = owner_game
	status = Label.new()
	status.set_anchors_and_offsets_preset(Control.PRESET_CENTER_BOTTOM)
	status.position = Vector2(-330,-250)
	status.size = Vector2(660,48)
	status.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	status.add_theme_font_size_override("font_size",16)
	status.add_theme_color_override("font_shadow_color",Color.BLACK)
	status.add_theme_constant_override("shadow_offset_x",2)
	status.add_theme_constant_override("shadow_offset_y",2)
	game.hud.staging.add_child(status)

func update(delta: float) -> void:
	throw_cooldown = maxf(0,throw_cooldown-delta)
	var hero: CharacterBody3D = game.hero
	status.text = "Ctrl — тихий шаг  ·  G — бросить бутылку  [%d]" % bottles
	if hero.in_refuge:
		status.text = "ДЕЖУРНАЯ · спокойно · F — журнал / шкаф · I — снаряжение"
		return
	if hero.hiding_bed != null:
		if not Input.is_action_pressed("sprint") and hero.stamina>=25: breath_exhausted = false
		var holding: bool = Input.is_action_pressed("sprint") and not breath_exhausted and hero.stamina > 0
		if holding: hero.stamina = maxf(0,hero.stamina-delta*18)
		else: hero.stamina = minf(100,hero.stamina+delta*8)
		if holding and hero.stamina<=0: breath_exhausted = true
		breath_remaining -= delta
		if breath_remaining <= 0:
			breath_remaining = 3
			if not holding:
				game.emit_noise(hero.global_position,2.8)
				game.sound_bank.play_cue("breath",.8)
		status.text = "ПОД КРОВАТЬЮ  ·  F — выйти  ·  Shift — задержать дыхание\nДыхание: %d%%  ·  если тебя видели, укрытие проверят" % int(hero.stamina)
		return
	if action_object != null:
		if not Input.is_action_pressed("interact_hold") or hero.global_position.distance_to(action_object.global_position)>2.1:
			cancel_action()
			return
		var id: String = action_object.object_id
		var duration: float = 5 if id=="residence_plate" else (4 if id in ["power","residence_lift"] else 2.5)
		work[id] = minf(duration,float(work.get(id,0))+delta)
		if id=="residence_plate": action_object.manual_wheel.rotation.z = -float(work[id])*TAU*.6
		var action_name: String = "освобождение стопора" if id=="residence_plate" else ("установка модуля" if id=="residence_lift" else ("снятие модуля" if id=="residence_core" else ("установка предохранителя" if id=="power" else "сброс давления" if id.begins_with("purge_valve") else "запуск реактора")))
		status.text = "УДЕРЖИВАЙ F  ·  %s  %d%%\nОтпусти, чтобы прервать работу и защищаться" % [action_name,int(work[id]/duration*100)]
		if fmod(float(work[id]),.6)<delta: game.emit_noise(action_object.global_position,9)
		if work[id]>=duration:
			var completed: Node3D = action_object
			cancel_action()
			game.mission.interact(completed)

func start_action(object: Node3D) -> void:
	if object.object_id in ["residence_core","residence_lift","residence_plate"] and not game.residential.can_work(object): return
	if object.object_id=="power" and not game.mission.has_fuse:
		game.mission.interact(object)
		return
	if object.object_id=="reactor" and (not game.mission.has_fuel or not game.mission.has_coolant or game.mission.purge_started):
		game.mission.interact(object)
		return
	if object.object_id.begins_with("purge_valve") and game.mission.purge_fault != object.object_id:
		game.hud.announce("Давление в норме. Этот вентиль пока не нужен.",3)
		return
	action_object = object
	game.hero.action_focus = object.global_position+object.global_basis*Vector3(0,1.02,.30)
	game.hero.performing_action = true
	game.hero.aiming = false

func cancel_action() -> void:
	action_object = null
	game.hero.performing_action = false

func try_hide(bed: Node3D) -> bool:
	var hero: CharacterBody3D = game.hero
	if hero.hiding_bed != null or hero.dash_remaining>0: return false
	var entries: Array[Vector3] = bed.exit_points()
	entries.sort_custom(func(a: Vector3,b: Vector3) -> bool: return a.distance_squared_to(hero.global_position)<b.distance_squared_to(hero.global_position))
	if entries[0].distance_to(hero.global_position)>1.2:
		game.hud.announce("Подойди к длинной стороне кровати.",3)
		return false
	# Sweep from the accessible side; never teleport through a wall or cabinet.
	var low := CapsuleShape3D.new()
	low.radius = .23
	low.height = .48
	var query := PhysicsShapeQueryParameters3D.new()
	query.shape = low
	query.collision_mask = 1
	query.transform = Transform3D(Basis.IDENTITY,hero.global_position+Vector3(0,.25,0))
	query.motion = bed.global_position+Vector3(0,.05,0)-hero.global_position
	var safe: PackedFloat32Array = game.get_world_3d().direct_space_state.cast_motion(query)
	if safe.size()<2 or safe[0]<.98:
		game.hud.announce("Путь под кровать перекрыт.",3)
		return false
	for hunter: CharacterBody3D in game.hunters:
		if hunter.active and hunter.stunned_remaining<=0 and hunter.can_see_player():
			hunter.remembered_bed = bed
			hunter.bed_entry = entries[0]
			hunter.last_known = entries[0]
			hunter._set_state(hunter.State.INVESTIGATE)
	entry_position = entries[0]
	cancel_action()
	hero.set_low_collision(true)
	hero.hiding_bed = bed
	hero.global_position = bed.global_position+Vector3(0,.05,0)
	hero.velocity = Vector3.ZERO
	hero.aiming = false
	hero.camera_rig.flashlight_enabled = false
	hero.camera_rig.position.y = .27
	breath_remaining = 3
	breath_exhausted = hero.stamina<18
	game.sound_bank.play_cue("step",.7)
	return true

func leave_bed() -> bool:
	var hero: CharacterBody3D = game.hero
	if hero.hiding_bed == null: return false
	var candidates: Array[Vector3] = [entry_position]
	candidates.append_array(hero.hiding_bed.exit_points())
	for at: Vector3 in candidates:
		var standing := CapsuleShape3D.new()
		standing.radius = .34
		standing.height = 1.7
		var query := PhysicsShapeQueryParameters3D.new()
		query.shape = standing
		query.collision_mask = 1|4
		query.transform = Transform3D(Basis.IDENTITY,at+Vector3(0,.87,0))
		if not game.get_world_3d().direct_space_state.intersect_shape(query).is_empty(): continue
		hero.hiding_bed = null
		hero.set_low_collision(false)
		hero.global_position = at
		hero.model.show()
		hero.camera_rig.position.y = 1.55
		return true
	game.hud.announce("Выход занят. Подожди или выбери другую сторону.",3)
	return false

func expose(hunter: CharacterBody3D) -> void:
	if game.hero.hiding_bed == null: return
	# The witnessed entry supplies this knowledge, not a global hidden-player lookup.
	game.hud.announce("Он видел, куда ты спрятался! Выбирайся!",4)
	leave_bed()
	if game.hero.invulnerable_remaining<=0: game._take_hit(hunter)
	hunter.remembered_bed = null

func throw_bottle() -> bool:
	if game.hero.in_refuge or bottles<=0 or throw_cooldown>0 or game.hero.hiding_bed!=null or game.hero.performing_action or game.is_paused or game.has_won or game.has_lost: return false
	var camera: Camera3D = game.hero.camera_rig.camera
	var start: Vector3 = game.hero.global_position+Vector3(0,1.25,0)
	var direction: Vector3 = (-camera.global_basis.z+Vector3.UP*.18).normalized()
	var bottle := Bottle.new()
	bottle.game = game
	bottle.velocity = direction*10
	game.get_node("World").add_child(bottle)
	bottle.global_position = start
	bottles -= 1
	throw_cooldown = 1.2
	return true

func reset_tools() -> void:
	cancel_action()
	if status != null: status.show()
	bottles = 3
	throw_cooldown = 0
	work.clear()
	for bottle: Node in get_tree().get_nodes_in_group("thrown_bottles"): bottle.queue_free()
