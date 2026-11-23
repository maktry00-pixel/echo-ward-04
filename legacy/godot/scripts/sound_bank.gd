extends Node
## Licensed score plus original synthesized foley and pursuit layers.

const RATE: int = 22050
var enabled: bool = true
var muted: bool = false
var sounds: Dictionary = {}
var ambience: AudioStreamPlayer
var pursuit: AudioStreamPlayer
var listener: Node3D
var suspended: bool = false
var effects_level: float = 0.8
var music_level: float = 0.7
var tension: float = 0.0
var score_music: AudioStreamPlayer
var effects_bus: StringName
var quiet_remaining: float = 0
var refuge_mix: float = 0
var refuge_music: AudioStreamPlayer
var glass_zones: Array[Vector3] = []
var glass_visuals: Array[Node3D] = []
var last_footstep_kind: String = "step_tile"
var step_events: int = 0
var enemy_steps: Dictionary = {}
var enemy_positions: Dictionary = {}
var current_room: String = ""
var voice_player: AudioStreamPlayer


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	if not enabled:
		return
	effects_bus = StringName("WardFX_"+str(get_instance_id()))
	AudioServer.add_bus()
	var bus: int = AudioServer.bus_count-1
	AudioServer.set_bus_name(bus,effects_bus)
	var reverb := AudioEffectReverb.new()
	reverb.room_size = .72
	reverb.damping = .65
	reverb.predelay_msec = 38
	reverb.wet = .18
	reverb.dry = .95
	AudioServer.add_bus_effect(bus,reverb)
	sounds["core"] = _foley("click", 0.18)
	sounds["step"] = _foley("step", 0.2)
	for kind: String in ["step_tile","step_metal","step_vinyl","step_glass","claw","heavy_step","drag","bone_hit","metal_hit","door_hit","door_break","creak","wood_hit","stone_hit","flesh_hit","phone","radio","vent","glass_tinkle"]:
		sounds[kind] = _foley(kind,.9 if kind=="door_break" else .38)
	sounds["injured_breath"] = _foley("injured_breath",1.7)
	sounds["door"] = _foley("door", 0.8)
	sounds["growl"] = _foley("growl", 1.4)
	sounds["knock"] = _foley("knock", 0.7)
	sounds["scrape"] = _foley("scrape", 1.2)
	sounds["whisper"] = _foley("whisper", 1.8)
	sounds["breath"] = _foley("breath", 1.6)
	sounds["sting"] = _foley("sting", 0.8)
	sounds["gunshot"] = _foley("gunshot", 0.65)
	for kind: String in ["shotgun","pump","shell_load","ceiling_scrape"]: sounds[kind] = _foley(kind,.8)
	for kind: String in ["isolation_relay","isolation_pipe","restraint_drag"]: sounds[kind] = _foley(kind,1.25)
	sounds["impact"] = _foley("impact", 0.22)
	sounds["reload"] = _foley("reload", 0.7)
	sounds["empty"] = _foley("click", 0.1)
	sounds["dash"] = _tone([180.0, 130.0, 85.0], 0.18)
	sounds["flash"] = _tone([900.0, 500.0, 250.0, 120.0], 0.5)
	sounds["hurt"] = _tone([95.0, 72.0, 55.0], 0.4)
	sounds["heartbeat"] = _tone([62.0, 0.0, 58.0], 0.3)
	sounds["alarm"] = _tone([220.0, 165.0, 220.0, 110.0], 0.9)
	sounds["win"] = _tone([330.0, 440.0, 550.0, 660.0], 1.2)
	sounds["lose"] = _tone([165.0, 110.0, 82.5, 55.0], 1.1)
	ambience = AudioStreamPlayer.new()
	var drone := _foley("room", 12.0)
	drone.loop_mode = AudioStreamWAV.LOOP_FORWARD
	drone.loop_end = drone.data.size() / 2
	ambience.stream = drone
	ambience.volume_db = -28
	add_child(ambience)
	ambience.play()
	pursuit = AudioStreamPlayer.new()
	var pulse := _foley("tension", 12.0)
	pulse.loop_mode = AudioStreamWAV.LOOP_FORWARD
	pulse.loop_end = pulse.data.size() / 2
	pursuit.stream = pulse
	add_child(pursuit)
	pursuit.play()
	set_levels(effects_level, music_level)
	score_music = AudioStreamPlayer.new()
	var track: AudioStreamOggVorbis = load("res://assets/audio/lost.ogg")
	track.loop = true
	score_music.stream = track
	add_child(score_music)
	score_music.play()
	refuge_music = AudioStreamPlayer.new()
	var safe_pad: AudioStreamWAV = _refuge_pad()
	refuge_music.stream = safe_pad
	refuge_music.volume_db = -80
	add_child(refuge_music)
	refuge_music.play()
	set_levels(effects_level, music_level)


func set_tension(value: float) -> void:
	tension = clampf(value, 0, 1)
	if pursuit != null:
		pursuit.volume_db = -55 + tension * 36 + linear_to_db(maxf(0.001, music_level))-(18 if quiet_remaining>0 else 0)-refuge_mix*30
	if score_music != null:
		score_music.volume_db = -15 - tension * 5 + linear_to_db(maxf(0.001, music_level))-(18 if quiet_remaining>0 else 0)-refuge_mix*30
	if ambience!=null: ambience.volume_db = -30+linear_to_db(maxf(.001,music_level))-(12 if quiet_remaining>0 else 0)-refuge_mix*5

	if refuge_music!=null: refuge_music.volume_db = -80+refuge_mix*58+linear_to_db(maxf(.001,music_level))

func set_levels(effects: float, music: float) -> void:
	effects_level = clampf(effects, 0, 1)
	music_level = clampf(music, 0, 1)
	if ambience != null:
		ambience.volume_db = -30 + linear_to_db(maxf(0.001, music_level))
	set_tension(tension)
	for player: Node in get_children():
		if player.has_meta("effect_db"):
			player.volume_db = float(player.get_meta("effect_db")) + linear_to_db(maxf(0.001, effects_level))


func play_spatial(kind: String, location: Vector3, pitch: float = 1.0) -> void:
	if not enabled or muted or suspended or not sounds.has(kind):
		return
	var player := AudioStreamPlayer3D.new()
	player.bus = effects_bus
	player.stream = sounds[kind]
	player.pitch_scale = pitch
	player.unit_size = 4.0
	player.max_distance = 20.0
	player.volume_db = -14 if kind.begins_with("step") or kind=="claw" else -8
	if listener != null:
		var ray := PhysicsRayQueryParameters3D.create(location + Vector3(0, 1.2, 0), listener.global_position + Vector3(0, 1.2, 0), 1)
		if not listener.get_world_3d().direct_space_state.intersect_ray(ray).is_empty():
			player.attenuation_filter_cutoff_hz = 900
			player.volume_db -= 5
		if listener.in_refuge:
			player.volume_db -= 8
			player.attenuation_filter_cutoff_hz = 700
	player.set_meta("effect_db", player.volume_db)
	player.volume_db += linear_to_db(maxf(0.001, effects_level))
	add_child(player)
	player.global_position = location + Vector3(0, 0.2, 0)
	player.finished.connect(player.queue_free)
	player.play()

func footstep(game: Node3D) -> String:
	var surface: String = game.exploration.surface_at(game.hero.position)
	for at: Vector3 in glass_zones:
		if at.distance_to(game.hero.position)<.95: surface = "glass"
	last_footstep_kind = "step_"+surface
	step_events += 1
	play_spatial(last_footstep_kind,game.hero.global_position,randf_range(.93,1.06))
	if surface=="glass": game.emit_noise(game.hero.global_position,7)
	return last_footstep_kind

func register_glass(at: Vector3) -> void:
	if glass_zones.size()>=16:
		glass_zones.pop_front()
		glass_visuals.pop_front().queue_free()
	glass_zones.append(at)
	var shards := Node3D.new()
	add_child(shards)
	shards.global_position = at+Vector3(0,.008,0)
	glass_visuals.append(shards)
	var material: StandardMaterial3D = preload("res://scripts/world_builder.gd").material("679084")
	material.metallic = .25
	material.roughness = .16
	for i: int in range(13):
		var shard: Node3D = preload("res://scripts/world_builder.gd").box(shards,Vector3(.05+(.01*(i%3)),.008,.025),Vector3(sin(i*2.4)*.6,0,cos(i*1.7)*.6),material)
		shard.rotation.y = i*1.9

func update_world(delta: float, game: Node3D) -> void:
	quiet_remaining = maxf(0,quiet_remaining-delta)
	refuge_mix = move_toward(refuge_mix,1.0 if game.hero.in_refuge else 0.0,delta*.65)
	set_tension(tension)
	current_room = game.exploration.room_at(game.hero.position)
	if enabled and not effects_bus.is_empty():
		var bus: int = AudioServer.get_bus_index(effects_bus)
		if bus>0:
			var reverb: AudioEffectReverb = AudioServer.get_bus_effect(bus,0)
			reverb.room_size = .85 if game.hero.position.y<-1 else (.42 if game.hero.position.x>60 else .65)
	for enemy: CharacterBody3D in game.hunters:
		var id: int = enemy.get_instance_id()
		var previous: Vector3 = enemy_positions.get(id,enemy.position)
		var travel: float = Vector2(enemy.position.x-previous.x,enemy.position.z-previous.z).length()
		enemy_positions[id] = enemy.position
		if not enemy.active or travel>1.5 or game.hero.position.distance_to(enemy.position)>18: continue
		enemy_steps[id] = float(enemy_steps.get(id,0))+travel
		if enemy_steps[id]>.85:
			enemy_steps[id] = 0.0
			play_spatial("heavy_step" if enemy.max_health>6 else "claw",enemy.position,.9)
			if enemy.mobility_factor()<1: play_spatial("drag",enemy.position)

func reset_foley() -> void:
	if voice_player!=null: voice_player.stop()
	quiet_remaining = 0
	refuge_mix = 0
	glass_zones.clear()
	for shards: Node3D in glass_visuals: shards.queue_free()
	glass_visuals.clear()
	enemy_positions.clear()
	enemy_steps.clear()


func play_cue(kind: String, pitch: float = 1.0) -> void:
	if not enabled or muted or not sounds.has(kind):
		return
	if suspended and kind not in ["win", "lose"]:
		return
	var player := AudioStreamPlayer.new()
	player.stream = sounds[kind]
	player.set_meta("ignore_suspend",kind in ["win","lose"])
	player.pitch_scale = pitch
	player.volume_db = -21 if kind in ["heartbeat", "sting", "breath"] else -13
	player.set_meta("effect_db", player.volume_db)
	player.volume_db += linear_to_db(maxf(0.001, effects_level))
	add_child(player)
	player.finished.connect(player.queue_free)
	player.play()


func _process(_delta: float) -> void:
	# Spatial play() is committed by the audio server after the current frame.
	# Keep a newly queued voice paused even when inventory opened in that frame.
	for player: Node in get_children():
		if player is AudioStreamPlayer or player is AudioStreamPlayer3D:
			var paused: bool = (suspended and not player.get_meta("ignore_suspend",false)) or muted
			if player.stream_paused!=paused: player.stream_paused = paused

func set_suspended(value: bool) -> void:
	suspended = value
	for player: Node in get_children():
		if player is AudioStreamPlayer or player is AudioStreamPlayer3D:
			player.stream_paused = (value and not player.get_meta("ignore_suspend",false)) or muted


func toggle_mute(paused: bool) -> void:
	muted = not muted
	set_suspended(paused)


func _exit_tree() -> void:
	# Stop the audio-server playbacks before their scene nodes disappear.
	for child: Node in get_children():
		if child is AudioStreamPlayer or child is AudioStreamPlayer3D:
			child.stop()
			child.stream = null
	sounds.clear()
	if not effects_bus.is_empty():
		var bus: int = AudioServer.get_bus_index(effects_bus)
		if bus>0: AudioServer.remove_bus(bus)


func _foley(kind: String, duration: float) -> AudioStreamWAV:
	var data := PackedByteArray()
	var samples: int = int(RATE * duration)
	data.resize(samples * 2)
	var rng := RandomNumberGenerator.new()
	rng.seed = 7813
	var smooth_noise: float = 0.0
	for index: int in range(samples):
		var time: float = float(index) / RATE
		var fraction: float = time / duration
		var noise: float = rng.randf_range(-1, 1)
		smooth_noise = lerpf(smooth_noise, noise, 0.06)
		var wave: float = 0.0
		match kind:
			"shotgun": wave = (noise*.9*exp(-time*23)+sin(TAU*43*time)*.85*exp(-time*11)+smooth_noise*1.4*exp(-time*6))
			"pump": wave = (noise*.23+sin(TAU*350*time)*.12)*exp(-fmod(time,.31)*32)*(1-fraction)
			"shell_load": wave = (noise*.23+sin(TAU*920*time)*.18)*exp(-time*47)
			"ceiling_scrape": wave = (smooth_noise*2+sin(TAU*(900+200*sin(time*21))*time)*.15)*sin(PI*fraction)
			"isolation_relay": wave = (sin(TAU*110*time)*.45+noise*.22)*exp(-fmod(time,.27)*39)*(1-fraction)
			"isolation_pipe": wave = (sin(TAU*73*time)*.34+sin(TAU*417*time)*.17+smooth_noise*.7)*exp(-time*4)
			"restraint_drag": wave = (smooth_noise*2.2+sin(TAU*333*time)*.10)*(sin(time*26)*.3+.7)*sin(PI*fraction)
			"wood_hit": wave = (sin(TAU*135*time)*.5+smooth_noise*1.2)*exp(-time*23)
			"stone_hit": wave = (noise*.55+smooth_noise)*exp(-time*29)
			"flesh_hit": wave = (smooth_noise*3+sin(TAU*78*time)*.4)*exp(-time*21)
			"phone": wave = (sin(TAU*840*time)+sin(TAU*1060*time))*.17*(1 if fmod(time,.15)<.10 else 0)*sin(PI*fraction)
			"radio": wave = (smooth_noise*1.9+sin(TAU*230*time)*.16)*sin(PI*fraction)*(sin(time*39)*.3+.7)
			"vent": wave = smooth_noise*2.8*sin(PI*fraction)
			"glass_tinkle": wave = (sin(TAU*2700*time)+sin(TAU*3491*time))*.25*exp(-time*12)
			"step_tile": wave = (sin(TAU*115*time)*.55+noise*.28)*exp(-time*32)
			"step_metal", "metal_hit": wave = (sin(TAU*380*time)*.38+sin(TAU*823*time)*.25+noise*.2)*exp(-time*17)
			"step_vinyl": wave = (sin(TAU*68*time)*.55+smooth_noise*.8)*exp(-time*36)
			"step_glass": wave = (noise*.5+sin(TAU*2700*time)*.15)*exp(-time*18)*(1+.3*sin(time*137))
			"claw": wave = (smooth_noise*1.4+sin(TAU*420*time)*.12)*exp(-time*21)
			"heavy_step": wave = (sin(TAU*43*time)*.7+smooth_noise)*exp(-time*13)
			"drag": wave = smooth_noise*2.3*pow(sin(PI*fraction),2)
			"bone_hit": wave = (noise*.6+sin(TAU*180*time)*.3)*exp(-time*26)
			"door_hit": wave = (sin(TAU*54*time)*.65+smooth_noise*1.4)*exp(-time*12)
			"door_break": wave = (noise*.55+smooth_noise*1.8+sin(TAU*39*time)*.5)*exp(-time*5)
			"creak": wave = (sin(TAU*(270+45*sin(time*19))*time)*.3+smooth_noise)*sin(PI*fraction)
			"injured_breath": wave = (smooth_noise*2.5+sin(TAU*62*time)*.14)*pow(sin(PI*fraction),2)*(1+.4*sin(time*31))
			"gunshot":
				wave = noise * exp(-time * 32) * 0.9 + sin(TAU * 63 * time) * exp(-time * 17) * 0.7 + smooth_noise * exp(-time * 7) * 0.6
			"impact":
				wave = (noise * 0.4 + sin(TAU * 140 * time) * 0.5) * exp(-time * 24)
			"reload":
				var click_time: float = fmod(time, 0.23)
				wave = (noise * 0.3 + sin(TAU * 720 * time) * 0.15) * exp(-click_time * 55)
			"step", "click":
				wave = (noise * 0.15 + sin(TAU * 95.0 * time) * 0.6) * exp(-time * 35.0)
			"door":
				wave = (smooth_noise * 1.8 + sin(TAU * (75.0 + 12.0 * sin(time * 17)) * time) * 0.2) * sin(PI * fraction)
			"growl":
				wave = (sin(TAU * 47 * time) * sin(TAU * 3 * time) * 0.5 + smooth_noise) * sin(PI * fraction)
			"knock":
				var hit: float = fmod(time, 0.3)
				wave = (sin(TAU * 72 * time) * 0.65 + smooth_noise) * exp(-hit * 22) * (1.0 - fraction)
			"scrape":
				wave = (smooth_noise * 2 + sin(TAU * 735 * time) * 0.07) * sin(PI * fraction) * (0.7 + sin(time * 31) * 0.25)
			"whisper", "breath":
				wave = smooth_noise * 2.5 * pow(sin(PI * fraction), 2) * (0.6 + 0.3 * sin(time * 12))
			"sting":
				wave = (sin(TAU * 87 * time) + sin(TAU * 123 * time) * 0.5 + smooth_noise) * sin(PI * fraction) * 0.45
			"tension":
				wave = (sin(TAU * 43 * time) * 0.3 + sin(TAU * 57 * time) * 0.15 + smooth_noise * 0.5) * (0.4 + 0.6 * pow(maxf(0, sin(TAU * 1.5 * time)), 5))
			"room":
				wave = 0.18 * sin(TAU * 41 * time) + 0.08 * sin(TAU * 61 * time) + smooth_noise * 0.3
		data.encode_s16(index * 2, int(clampf(wave, -1, 1) * 16000))
	var stream := AudioStreamWAV.new()
	stream.format = AudioStreamWAV.FORMAT_16_BITS
	stream.mix_rate = RATE
	stream.data = data
	return stream


func _tone(notes: Array[float], duration: float) -> AudioStreamWAV:
	var data := PackedByteArray()
	var samples: int = int(RATE * duration)
	data.resize(samples * 2)
	var note_duration: float = duration / notes.size()
	for index: int in range(samples):
		var time: float = float(index) / RATE
		var note: int = mini(int(time / note_duration), notes.size() - 1)
		var local_time: float = fmod(time, note_duration)
		var envelope: float = minf(local_time / 0.012, 1.0) * pow(1.0 - local_time / note_duration, 1.4)
		var wave: float = sin(TAU * notes[note] * time) + 0.15 * sin(TAU * notes[note] * 2.0 * time)
		data.encode_s16(index * 2, int(wave * envelope * 11000.0))
	var stream := AudioStreamWAV.new()
	stream.format = AudioStreamWAV.FORMAT_16_BITS
	stream.mix_rate = RATE
	stream.data = data
	return stream

func play_recording(id: String) -> float:
	if id not in ["shift","evidence","ceiling","farewell"]: return 0
	var stream: AudioStreamWAV = AudioStreamWAV.load_from_file("res://assets/audio/stories/"+id+".wav")
	if stream==null: return 0
	if not enabled or muted or suspended: return stream.get_length()
	if voice_player==null:
		voice_player = AudioStreamPlayer.new()
		voice_player.set_meta("effect_db",-7.0)
		add_child(voice_player)
	voice_player.stop()
	voice_player.stream = stream
	voice_player.volume_db = -7+linear_to_db(maxf(.001,effects_level))
	voice_player.play()
	play_cue("radio",.9)
	return stream.get_length()

func _refuge_pad() -> AudioStreamWAV:
	var bytes := PackedByteArray()
	var length: int = RATE*12
	bytes.resize(length*2)
	for i: int in range(length):
		var time: float = float(i)/RATE
		var envelope: float = pow(sin(PI*time/12),2)
		var signal_value: float = (sin(TAU*130.8128*time)+sin(TAU*164.8138*time)*.55+sin(TAU*195.9977*time)*.35)*envelope*.16
		bytes.encode_s16(i*2,int(signal_value*16000))
	var stream := AudioStreamWAV.new()
	stream.format = AudioStreamWAV.FORMAT_16_BITS
	stream.mix_rate = RATE
	stream.data = bytes
	stream.loop_mode = AudioStreamWAV.LOOP_FORWARD
	stream.loop_end = length
	return stream
