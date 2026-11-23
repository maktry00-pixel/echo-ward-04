extends Node
## Pacing uses actual pursuit states. Ambient cues are non-damaging and stop in a chase.

var game: Node3D
var intensity: float = 0.0
var cue_remaining: float = 27.0
var sting_remaining: float = 0.0
var breath_remaining: float = 0.0
var echo_remaining: float = -1.0
var echo_source: Vector3
var rng := RandomNumberGenerator.new()
var ambient_count: int = 0

func configure(owner_game: Node3D) -> void:
	game = owner_game
	rng.seed = 1907

func update(delta: float) -> void:
	var danger: float = 0.0
	for hunter: CharacterBody3D in game.hunters:
		if not hunter.active or hunter.stunned_remaining > 0:
			continue
		var distance: float = hunter.position.distance_to(game.hero.position)
		if hunter.sees_player:
			danger = maxf(danger, clampf(1.15 - distance / 18.0, 0.4, 1.0))
		elif hunter.state == hunter.State.SEARCH and distance < 8:
			danger = maxf(danger, 0.28)
		elif hunter.blind_hunter and hunter.state==hunter.State.INVESTIGATE and distance<10:
			danger = maxf(danger,clampf(.8-distance*.05,.3,.7))
		_check_door(hunter, delta)
	if game.hero.in_refuge: danger = 0
	intensity = move_toward(intensity,danger,delta*(1.0 if danger>intensity or game.hero.in_refuge else .13))
	game.hero.camera_rig.tension = intensity
	game.sound_bank.set_tension(intensity)
	game.hud.set_body_state(game.hero.stamina, game.hero.exhausted, game.hero.camera_rig.flashlight_enabled, intensity)
	sting_remaining = maxf(0, sting_remaining - delta)
	if danger > 0.7 and sting_remaining <= 0:
		game.sound_bank.play_cue("sting")
		sting_remaining = 18
	breath_remaining -= delta
	if (game.hero.stamina < 25 or game.health<3) and breath_remaining <= 0:
		game.sound_bank.play_cue("injured_breath" if game.health<3 else "breath",.9 if game.health==1 else 1.0)
		breath_remaining = 2.1 if game.hero.stamina<25 or game.health==1 else 4.5
	if danger > 0.5:
		cue_remaining = maxf(cue_remaining, 12)
	else:
		cue_remaining -= delta
	if echo_remaining >= 0:
		echo_remaining -= delta
		if echo_remaining < 0 and intensity < 0.4:
			game.sound_bank.play_spatial("step", echo_source, 0.7)
	if not game.hero.in_refuge and (game.pacing==null or game.pacing.ambient_allowed()) and not (game.guardian.started and game.guardian.intro_time<2.6) and cue_remaining <= 0 and intensity < 0.3 and not game.mission.exit_started and (game.mission.has_fuse or game.mission.power_on):
		var candidates: Array[Vector3] = []
		for source: Vector3 in [Vector3(21,0,-36),Vector3(38,0,-46),Vector3(57,0,-33)]:
			var distance: float = game.hero.position.distance_to(source)
			if distance>4 and distance<13: candidates.append(source)
		for source: Vector3 in [Vector3(-11, 0, 4), Vector3(11, 0, 4), Vector3(-11, 0, -5), Vector3(11, 0, -5), Vector3(-22, 0, 3), Vector3(-18, 0, 12), Vector3(0, 0, 5), Vector3(18, 0, 18), Vector3(42, 0, 19), Vector3(57, 0, 27), Vector3(40, -5, 61), Vector3(55, -5, 73),Vector3(68,0,9),Vector3(75,0,24),Vector3(87,0,3),Vector3(99,0,20),Vector3(82,0,16)]:
			var distance: float = game.hero.position.distance_to(source)
			if distance > 4 and distance < 13:
				candidates.append(source)
		if not candidates.is_empty():
			var source: Vector3 = candidates[rng.randi_range(0, candidates.size() - 1)]
			var cue: String = ["knock", "scrape", "whisper","creak"][ambient_count % 4]
			game.sound_bank.quiet_remaining = 1.5
			game.sound_bank.play_spatial(cue, source)
			game.facility.unease_location = source
			game.facility.unease_remaining = 3.0
			echo_source = source
			echo_remaining = 0.7
			ambient_count += 1
		cue_remaining = rng.randf_range(17, 29)

func _check_door(hunter: CharacterBody3D, delta: float) -> void:
	hunter.door_retry_remaining = maxf(0, hunter.door_retry_remaining - delta)
	if hunter.state == hunter.State.PATROL or hunter.door_retry_remaining > 0:
		hunter.door_target = null
		hunter.door_wait_remaining = 0
		return
	var target: Node3D
	for door: Node3D in game.facility.doors:
		if door.door_id=="refuge_door" or door.locked or door.is_open or door.busy or hunter.position.distance_to(door.position) > 2.0:
			continue
		var from: Vector3 = door.to_local(hunter.position)
		var to: Vector3 = door.to_local(hunter.last_known)
		if from.z * to.z < -0.3 and absf(from.x) < door.width / 2 + 0.5:
			target = door
			break
	if target == null:
		hunter.door_target = null
		hunter.door_wait_remaining = 0
		return
	if hunter.door_target != target:
		hunter.door_target = target
		hunter.door_wait_remaining = 2.4
		game.sound_bank.play_spatial("knock", target.position, 0.7)
	else:
		hunter.door_wait_remaining -= delta
		if hunter.door_wait_remaining <= 0:
			if target.barricade_hp>0:
				target.damage_barricade()
				game.sound_bank.play_spatial("wood_hit",target.position)
				game.emit_noise(target.position,15,hunter)
			else:
				target.toggle_door(hunter.position)
				game.sound_bank.play_spatial("door", target.position, 0.75)
			hunter.door_target = null
			hunter.door_retry_remaining = 1.0 if target.barricade_hp>0 else 4.0

func reset_director() -> void:
	intensity = 0
	cue_remaining = 27
	sting_remaining = 0
	breath_remaining = 0
	echo_remaining = -1
	ambient_count = 0
	rng.seed = 1907
	game.sound_bank.set_tension(0)
