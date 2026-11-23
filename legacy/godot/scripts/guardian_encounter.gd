extends Node
## Preplaced guardian, a physical breached door, telegraphed charges and two phases.
var game: Node3D
var boss: CharacterBody3D
var door: Node3D
var started: bool = false
var finished: bool = false
var phase: int = 1
var intro_time: float = 0
var exposed_remaining: float = 0
var was_lunging: bool = false
var armor_message: float = 0
var weakpoint: Area3D
var weak_material: StandardMaterial3D
var status: Label
func configure(owner_game: Node3D) -> void:
	game = owner_game
	boss = game.hunters[5]
	boss.boss_controller = self
	door = game.facility.gate_by_id["sterile_52"]
	door.lock_kind = "guardian"
	weakpoint = preload("res://scripts/limb_hitbox.gd").new()
	weakpoint.hunter = boss
	weakpoint.weakpoint = true
	weakpoint.monitoring = false
	weakpoint.collision_layer = 0
	weakpoint.collision_mask = 0
	weakpoint.position = Vector3(0,1.47,-.34)
	var shape := CollisionShape3D.new()
	var sphere := SphereShape3D.new()
	sphere.radius = .24
	shape.shape = sphere
	weakpoint.add_child(shape)
	boss.model.torso.add_child(weakpoint)
	weak_material = preload("res://scripts/world_builder.gd").material("443c34")
	preload("res://scripts/world_builder.gd").sphere(weakpoint,.17,Vector3.ZERO,weak_material)
	var layer := CanvasLayer.new()
	layer.layer = 4
	add_child(layer)
	status = Label.new()
	status.set_anchors_and_offsets_preset(Control.PRESET_TOP_WIDE)
	status.offset_top = 175
	status.offset_bottom = 235
	status.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	status.add_theme_font_size_override("font_size",20)
	status.add_theme_color_override("font_shadow_color",Color.BLACK)
	status.add_theme_constant_override("shadow_offset_x",2)
	status.add_theme_constant_override("shadow_offset_y",2)
	status.add_theme_font_size_override("font_size",16)
	game.hud.boss_panel.add_child(status)
	reset_encounter()

func update(delta: float) -> void:
	armor_message = maxf(0,armor_message-delta)
	if boss.dead:
		if not finished:
			finished = true
			weakpoint.collision_layer = 0
			if not door.breached: door.breach()
			game.hud.announce("Хранитель повержен. Охранный узел операционной доступен.",6)
		status.hide()
		return
	if not started:
		var at: Vector3 = game.hero.position
		if game.mission.has_badge and at.y>-1 and at.x>44.4 and at.x<59.6 and at.z>-5 and at.z<30:
			start_encounter()
		return
	if intro_time<2.6:
		var previous: float = intro_time
		intro_time += delta
		for moment: float in [.35,1.1,1.95]:
			if previous<moment and intro_time>=moment:
				game.sound_bank.play_spatial("door_hit",door.position,.75)
				game.hero.camera_rig.shake_remaining = .15
		if intro_time>=2.6:
			door.breach()
			game.emit_noise(door.position,35)
			game.sound_bank.play_spatial("door_break",door.position)
			game.sound_bank.quiet_remaining = 0
			boss.set_active(true)
			boss.attack_cooldown = 0
			boss.hear_noise(door.position,40)
		return
	exposed_remaining = maxf(0,exposed_remaining-delta)
	if was_lunging and boss.lunge_remaining<=0:
		exposed_remaining = 2.8 if phase==1 else 1.8
		game.sound_bank.play_spatial("scrape",boss.position,.6)
		game.hud.announce("Он потерял равновесие! Оранжевая ткань на груди открыта.",3)
	was_lunging = boss.lunge_remaining>0
	weakpoint.collision_layer = 4 if exposed_remaining>0 and boss.active else 0
	weak_material.albedo_color = Color("a35034") if exposed_remaining>0 else Color("443c34")
	weak_material.emission_enabled = exposed_remaining>0
	weak_material.emission = Color("db7542")
	weak_material.emission_energy_multiplier = .15
	status.visible = boss.active and game.hero.position.distance_to(boss.position)<19
	status.text = "ХРАНИТЕЛЬ · %d / 12 · ФАЗА %d\n%s" % [boss.health,phase,"ГРУДЬ ОТКРЫТА — СТРЕЛЯЙ" if exposed_remaining>0 else ("РЫВОК — УХОДИ В СТОРОНУ" if boss.windup>0 or boss.lunge_remaining>0 else "БРОНЯ ЗАКРЫТА · ВЫМАНИ РЫВОК / ПОВРЕДИ НОГИ")]

func start_encounter() -> void:
	if started or boss.dead: return
	started = true
	intro_time = 0
	boss.set_active(false)
	boss.show()
	boss.model.set_process(true)
	boss.model.animate(0)
	game.sound_bank.quiet_remaining = 2.6
	game.sound_bank.play_spatial("growl",boss.position,.65)
	game.hud.announce("За дверью тяжёлое дыхание. Отойди от проёма. После рывка броня хранителя раскрывается.",7)

func allow_damage() -> bool:
	if not started or exposed_remaining>0: return true
	if armor_message<=0:
		armor_message = 2
		game.sound_bank.play_spatial("metal_hit",boss.position)
		game.hud.announce("Броня держит. Уйди от рывка и стреляй в открытую грудь. Ноги уязвимы к повреждению.",4)
	return false

func damage_received() -> void:
	if started and phase==1 and boss.health<=6 and boss.health>0:
		phase = 2
		exposed_remaining = 0
		boss.stun(.7)
		boss.attack_cooldown = .9
		game.sound_bank.play_spatial("growl",boss.position,.85)
		game.hud.announce("Он сбросил часть брони. Рывки быстрее, окно уязвимости короче.",5)

func reset_encounter() -> void:
	started = false
	finished = false
	phase = 1
	intro_time = 0
	exposed_remaining = 0
	was_lunging = false
	weakpoint.collision_layer = 0
	status.hide()
	door.restore_state(false,true)

func snapshot() -> Dictionary:
	return {"started":started or boss.dead,"finished":finished or boss.dead,"phase":phase}
func restore(data: Dictionary) -> void:
	started = data.get("started",false) or boss.active or boss.dead
	finished = boss.dead
	phase = int(data.get("phase",1))
	intro_time = 3 if started else 0
	if started:
		door.breach()
		exposed_remaining = 1.8 if not finished else 0
	else: door.restore_state(false,true)
static func valid(data: Variant) -> bool:
	return data is Dictionary and data.get("started") is bool and data.get("finished") is bool and preload("res://scripts/field_inventory.gd")._integer(data.get("phase"),1,2) and (not data.finished or data.started)
