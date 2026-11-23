extends SceneTree

const Checkpoints = preload("res://scripts/checkpoint_store.gd")
const Creature = preload("res://scripts/horror_creature.gd")
const Aberration = preload("res://scripts/aberration_visual.gd")
var game: Node3D
var checks: int = 0
var failures: int = 0

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	game = load("res://scenes/third_person.tscn").instantiate()
	game.set_meta("automated_test", true)
	root.add_child(game)
	await _frames(10)
	_check(game.hunters[0].model is Aberration and game.hunters[5].model is Creature, "Nonhuman aberrations and guardian use their own anatomy")
	_check(game.hunters[0].model.limbs.size()==6 and game.hunters[0].crawler and game.hunters[5].model.guardian, "Crawler has six animated limbs and guardian has separate tall silhouette")
	# The actual front-end signals pause and resume the existing world.
	game.show_title_menu()
	var elapsed: float = game.elapsed
	Input.action_press("move_forward")
	await _frames(30)
	Input.action_release("move_forward")
	_check(paused and game.in_title_menu and game.front_end.visible and not game.hud.visible, "Title screen replaces gameplay HUD")
	_check(game.elapsed == elapsed and not game.hero.camera_rig.input_enabled, "Title screen freezes gameplay and camera input")
	_check(game.front_end.continue_button.disabled, "No phantom continue without a checkpoint")
	game.front_end.new_game_requested.emit()
	await _frames(4)
	_check(not paused and not game.in_title_menu and game.hud.visible and game.hero.model.visible, "New-game button resumes the world")
	var original_mode: bool = game.settings["fullscreen"]
	_key(KEY_F11)
	_check(game.settings["fullscreen"] != original_mode, "F11 changes the fullscreen preference")
	var bounded: Dictionary = game.normalise_settings({"effects": 9, "music": -5, "sensitivity": 100, "fullscreen": "bad"})
	_check(bounded["effects"] == 1 and bounded["music"] == 0 and bounded["sensitivity"] == 2 and bounded["fullscreen"] == true, "Settings reject invalid types and clamp numeric values")
	game.apply_settings({"effects": 0.5, "music": 0.4, "sensitivity": 1.5, "camera_motion": false, "fullscreen": false})
	_check(not game.hero.camera_rig.motion_enabled and is_equal_approx(game.hero.camera_rig.look_sensitivity, 0.0045), "Settings control camera motion and sensitivity")
	# Exhaustion must matter even if Shift stays held.
	game.hero.reset_to(Vector3(0, 0.1, 12))
	Input.action_press("move_forward")
	Input.action_press("sprint")
	await _frames(360)
	_check(game.hero.exhausted and game.hero.stamina < 10 and not game.hero.sprinting, "Sustained running exhausts the player")
	Input.action_release("sprint")
	Input.action_release("move_forward")
	await _frames(200)
	_check(not game.hero.exhausted and game.hero.stamina > 50, "Walking or resting restores stamina")
	game.hero.stamina = 5
	game.hero.dash_cooldown = 0
	Input.action_press("dash")
	await _frames(2)
	Input.action_release("dash")
	_check(game.hero.dash_remaining == 0, "Empty stamina blocks repeated evasion")
	# Light reveals a distant player; walls remain authoritative.
	var hunter: CharacterBody3D = game.hunters[0]
	hunter.set_physics_process(false)
	hunter.set_active(true)
	hunter.position = Vector3(0, 0.1, 12)
	hunter.model.rotation.y = 0
	game.hero.reset_to(Vector3(0, 0.1, 4))
	await _frames(3)
	_check(hunter.can_see_player(), "A lit player is visible at eight metres")
	_key(KEY_B)
	_check(not game.hero.camera_rig.flashlight_enabled and not hunter.can_see_player(), "Turning off the light reduces distant detection")
	game.set_paused(true)
	_key(KEY_B)
	_check(not game.hero.camera_rig.flashlight_enabled, "Pause blocks light changes")
	game.set_paused(false)
	game.restart_game()
	_check(game.hero.camera_rig.flashlight_enabled and game.hero.stamina == 100, "Restart restores light and stamina")
	# Checkpoint bytes round-trip through a real file without touching user data.
	game.facility.gate_by_id["service_shortcut"].restore_state(true, false)
	await _use("map")
	await _use("fuse")
	game.health = 1
	game.flash_charges = 0
	await _use("power")
	_check(Checkpoints.valid(game.checkpoint) and game.checkpoint["stage"] == "power", "Power restoration creates a checkpoint")
	var file := FileAccess.open("res://artifacts/checkpoint_roundtrip.json", FileAccess.WRITE)
	file.store_string(JSON.stringify(game.checkpoint))
	file.close()
	var data: Variant = JSON.parse_string(FileAccess.get_file_as_string("res://artifacts/checkpoint_roundtrip.json"))
	_check(Checkpoints.valid(data), "Serialized checkpoint validates after JSON decoding")
	_check(not Checkpoints.valid({"version": 1, "active": true, "stage": "cheat"}) and not Checkpoints.valid([]), "Malformed checkpoint cannot advance the mission")
	var bad: Dictionary = data.duplicate(true)
	bad["health"] = "bad"
	_check(not Checkpoints.valid(bad), "Non-numeric save data is rejected")
	game.checkpoint = data
	game.show_title_menu()
	_check(not game.front_end.continue_button.disabled, "Saved checkpoint enables Continue")
	game.front_end.continue_requested.emit()
	await _frames(40)
	_check(not paused and not game.in_title_menu and game.mission.power_on and not game.mission.has_badge, "Continue restores the correct mission stage")
	_check(game.health >= 2 and game.flash_charges >= 1 and game.hero.stamina == 100, "Checkpoint avoids an exhausted death loop")
	_check(game.hero.position.distance_to(Vector3(-20, 0.1, 0)) < 0.3, "Load uses an authored safe location")
	_check(game.facility.gate_by_id["service_shortcut"].is_open and not game.facility.gate_by_id["service_shortcut"].locked, "Unlocked shortcuts survive loading")
	_check(game.mission.events.size() == 2 and not game.facility.silhouette.visible and not game.mission.exit_started, "Loading does not replay scares or start the escape timer")
	await _use("badge")
	_check(game.checkpoint["stage"] == "badge", "Badge updates the checkpoint")
	game.continue_checkpoint()
	await _frames(40)
	_check(game.mission.has_badge and game.mission.objects["badge"].used, "Badge progress survives loading")
	# A witnessed escape through an unlocked door gets a warning and delay.
	game.hero.reset_to(Vector3(13, 0.1, 11))
	var door: Node3D = game.facility.gate_by_id["wing_-9_-1"]
	door.restore_state(false, false)
	game.facility.layout_changed.emit()
	hunter.set_physics_process(false)
	hunter.set_active(true)
	hunter.stunned_remaining = 0
	hunter.position = Vector3(-9, 0.1, 0.7)
	hunter.last_known = Vector3(-9, 0.1, -4)
	hunter._set_state(hunter.State.SEARCH)
	await _frames(3)
	_check(hunter.door_target == door and not door.is_open, "Hunter warns at the observed door before opening it")
	await _frames(190)
	_check(door.is_open and door.body.collision_layer == 0, "Hunter can open an unlocked observed doorway after a delay")
	door.restore_state(false, true)
	hunter.door_retry_remaining = 0
	await _frames(190)
	_check(not door.is_open, "Hunter cannot bypass a progression lock")
	door.restore_state(false, false)
	hunter.last_known = Vector3(-9, 0.1, 2)
	hunter.door_retry_remaining = 0
	await _frames(190)
	_check(not door.is_open and hunter.door_target == null, "Hunter does not open doors without evidence across them")
	hunter.set_physics_process(true)
	game.restart_game()
	await _frames(4)
	game.mission.has_fuse = true
	game.horror.cue_remaining = 0
	await _frames(2)
	_check(game.horror.ambient_count == 1 and game.health == 3, "Quiet-room cue is non-damaging")
	game.set_paused(true)
	var cue_time: float = game.horror.cue_remaining
	await _frames(120)
	_check(game.horror.cue_remaining == cue_time, "Pause freezes ambient-event timers")
	game.restart_game()
	_check(game.horror.ambient_count == 0 and game.facility.unease_remaining == 0, "Restart clears pending atmosphere events")
	# Real sound objects exercise playback pause and mixing controls.
	var audio = preload("res://scripts/sound_bank.gd").new()
	root.add_child(audio)
	_check(audio.sounds.size() >= 16 and audio.pursuit != null, "Sound bank has room tone, pursuit and environmental cues")
	audio.set_tension(1)
	var high_volume: float = audio.pursuit.volume_db
	audio.set_tension(0)
	_check(audio.pursuit.volume_db < high_volume - 30, "Pursuit layer fades in and out with danger")
	audio.set_suspended(true)
	_check(audio.ambience.stream_paused and audio.pursuit.stream_paused, "Pause suspends both audio layers")
	_check(audio.score_music.stream_paused and audio.score_music.stream is AudioStreamOggVorbis, "Licensed horror score is loaded and paused")
	audio.toggle_mute(true)
	audio.set_suspended(false)
	_check(audio.ambience.stream_paused, "Muted ambience remains muted when resuming")
	audio.toggle_mute(false)
	_check(not audio.ambience.stream_paused, "Unmute restores ambience")
	audio.queue_free()
	await _frames(3)
	OS.delay_msec(100)
	game.queue_free()
	await process_frame
	print("HORROR AND APP TEST: %d checks, %d failures" % [checks, failures])
	quit(0 if failures == 0 else 1)

func _use(id: String) -> void:
	game.hero.reset_to(game.mission.objects[id].interaction_point())
	await _frames(4)
	await preload("res://tests/test_actions.gd").search(self,game,game.mission.objects[id])
	_key(KEY_F)
	if id=="power":
		Input.action_press("interact_hold")
		await _frames(245)
		Input.action_release("interact_hold")
	else:
		await _frames(3)

func _key(key: int) -> void:
	var event := InputEventKey.new()
	event.physical_keycode = key
	event.pressed = true
	game._unhandled_input(event)

func _frames(count: int) -> void:
	for frame: int in range(count):
		await physics_frame
		await process_frame

func _check(condition: bool, description: String) -> void:
	checks += 1
	if not condition:
		failures += 1
		push_error("FAIL: " + description)
