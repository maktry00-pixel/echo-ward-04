extends "res://scripts/stalker.gd"
## Preplaced ceiling ambush: line of sight/heard evidence -> warning -> physical fall.
var perch_state: String = "perched"
var tell_remaining: float = 0
var fall_speed: float = 0
var dust: Node3D
func _ready() -> void:
	crawler = true
	super._ready()
	_set_perch()
func _set_perch() -> void:
	perch_state = "perched"
	position.y = 2.4
	model.position.y = .9
	model.rotation.z = PI
	model.rotation.y = PI
	head_height = .12
	fall_speed = 0
	tell_remaining = 0
func _physics_process(delta: float) -> void:
	if target==null or navigation==null: return
	_update_limb_hitboxes()
	var game: Node = get_tree().get_first_node_in_group("game")
	if perch_state=="perched":
		if not game.mission.has_badge: return
		if not active and target.position.z<-16 and global_position.distance_to(target.global_position)<15: set_active(true)
		if not active: return
		model.animate(.08)
		if can_see_player() or state==State.INVESTIGATE:
			perch_state = "warning"
			tell_remaining = 1.5*float(game.difficulty_value("windup"))
			if can_see_player(): last_known = target.global_position
			game.sound_bank.play_spatial("ceiling_scrape",global_position)
			game.hud.announce("С потолка сыплется пыль. Над головой скребут когти…",4)
			_dust(game)
		return
	if perch_state=="warning":
		tell_remaining -= delta
		model.rotation.x = sin(tell_remaining*18)*.09
		if tell_remaining<=0: perch_state = "falling"
		return
	if perch_state=="falling" or (dead and position.y>.1):
		fall_speed += delta*16
		velocity = Vector3(0,-fall_speed,0)
		move_and_slide()
		model.rotation.z = move_toward(model.rotation.z,0,delta*7)
		model.position.y = move_toward(model.position.y,0,delta*2.6)
		model.rotation.x = 0
		if position.y<=.1 or is_on_floor():
			position.y = .08
			model.position.y = 0
			model.rotation.z = 0
			head_height = .85
			perch_state = "grounded"
			velocity = Vector3.ZERO
			if not dead: _set_state(State.INVESTIGATE)
			game.sound_bank.play_spatial("bone_hit",position)
			game.emit_noise(position,15,self)
		return
	if perch_state=="grounded": super._physics_process(delta)
func _dust(game: Node3D) -> void:
	for i: int in range(8):
		var flake: Node3D = preload("res://scripts/world_builder.gd").box(game.feedback,Vector3.ONE*.025,global_position+Vector3(sin(i)*.35,.3,cos(i)*.35),preload("res://scripts/world_builder.gd").material("8c8877"))
		var tween := flake.create_tween()
		tween.tween_property(flake,"position:y",.04,1.2+i*.07)
		tween.tween_callback(flake.queue_free)
func receive_bullet(point: Vector3,damage: int,headshot: bool,impact: Vector3 = Vector3.INF) -> bool:
	var accepted: bool = super.receive_bullet(point,damage,headshot,impact)
	if accepted and perch_state!="grounded": perch_state = "falling"
	return accepted
func aim_point() -> Vector3:
	return model.head.global_position if perch_state!="grounded" else super.aim_point()
func reset_hunter() -> void:
	super.reset_hunter()
	_set_perch()
func restore_perch(value: String) -> void:
	if value=="perched" and not dead: _set_perch()
	else:
		perch_state = "grounded"
		position.y = .08
		model.position.y = 0
		model.rotation.z = 0
		head_height = .85
		if dead: model.restore_death(position+Vector3.FORWARD)
