extends CharacterBody3D
## Sight and noise supply observations; unseen player positions are never read for navigation.

const Creature = preload("res://scripts/aberration_visual.gd")
const Minimax = preload("res://scripts/tactical_minimax.gd")
enum State { PATROL, INVESTIGATE, CHASE, SEARCH }
signal ranged_attack(origin: Vector3, observed_target: Vector3)
var target: CharacterBody3D
var navigation: RefCounted
var active: bool = false
var stunned_remaining: float = 0.0
var move_speed: float = 3.1
var route: Array[Vector3] = []
var route_index: int = 0
var repath_remaining: float = 0.0
var model: Node3D
var spawn_position: Vector3
var state: State = State.PATROL
var patrol_points: Array[Vector3] = []
var patrol_index: int = 0
var last_known: Vector3
var lost_sight: float = 0.0
var search_remaining: float = 0.0
var search_index: int = 0
var destination: Vector3
var sees_player: bool = false
var last_noise: Vector3
var state_elapsed: float = 0.0
var door_target: Node3D
var door_wait_remaining: float = 0.0
var door_retry_remaining: float = 0.0
var max_health: int = 6
var health: int = 6
var dead: bool = false
var hit_reaction: float = 0.0
var death_tween: Tween
var use_minimax: bool = false
var tactical := Minimax.new()
var stagger_guard: float = 0.0
var can_spit: bool = false
var attack_cooldown: float = 3.0
var windup: float = 0.0
var attack_point: Vector3
var lunge_remaining: float = 0.0
var lunge_heading: Vector3
var warning: OmniLight3D
var crawler: bool = false
var remembered_bed: Node3D
var bed_entry: Vector3
var head_height: float = 2.10
var alert_pursuer: bool = false
var blind_hunter: bool = false
var leg_damage: Array[int] = []
var limb_hitboxes: Array[Area3D] = []
var boss_controller: Node
var attack_mode: String = "swipe"
var melee_remaining: float = 0
var knockback_remaining: float = 0
var knockback_velocity := Vector3.ZERO
var swings: int = 0


func _ready() -> void:
	health = max_health
	collision_layer = 0
	collision_mask = 1|16
	motion_mode = CharacterBody3D.MOTION_MODE_FLOATING
	var shape := CapsuleShape3D.new()
	shape.radius = 0.4
	shape.height = 1.12 if crawler else 2.3
	var collision := CollisionShape3D.new()
	collision.shape = shape
	collision.position.y = shape.height/2
	add_child(collision)
	model = Creature.new()
	model.crawler = crawler
	model.blindfold = blind_hunter
	head_height = .85 if crawler else 2.1
	if max_health > 6:
		model.guardian = true
	add_child(model)
	_build_limb_hitboxes()
	warning = OmniLight3D.new()
	warning.position = Vector3(0, 1.8, -0.1)
	warning.omni_range = 2.0
	warning.light_energy = 0
	add_child(warning)
	spawn_position = position
	last_known = position
	visible = false
	model.set_process(false)


func configure(hero: CharacterBody3D, grid: RefCounted) -> void:
	target = hero
	navigation = grid


func set_active(value: bool) -> void:
	if dead:
		return
	active = value
	visible = value
	model.set_process(value)
	collision_layer = 4 if value else 0
	for limb: Area3D in limb_hitboxes: limb.collision_layer = 4 if value else 0


func can_see_player() -> bool:
	if blind_hunter or target == null or target.in_refuge or target.hiding_bed != null:
		return false
	var offset: Vector3 = target.global_position - global_position
	offset.y = 0
	var distance: float = offset.length()
	var sight_range: float = 11.0 if target.camera_rig.flashlight_enabled else 6.0
	if distance > sight_range:
		return false
	var forward := Vector3(-sin(model.rotation.y), 0, -cos(model.rotation.y))
	if distance > 1.6 and forward.dot(offset.normalized()) < 0.25:
		return false
	var ray := PhysicsRayQueryParameters3D.create(global_position + Vector3(0, head_height-.08, 0),
		target.global_position + Vector3(0, 1.2, 0), 1)
	return get_world_3d().direct_space_state.intersect_ray(ray).is_empty()


func hear_noise(location: Vector3, radius: float) -> bool:
	var game: Node = get_tree().get_first_node_in_group("game")
	if game!=null and game.refuge!=null and game.refuge.contains(location): return false
	if not active or stunned_remaining > 0 or state == State.CHASE:
		return false
	if blind_hunter: radius *= 2.25
	var distance: float = global_position.distance_to(location)
	var ray := PhysicsRayQueryParameters3D.create(global_position + Vector3(0, 1.2, 0), location + Vector3(0, 1.2, 0), 1)
	var blocked: bool = not get_world_3d().direct_space_state.intersect_ray(ray).is_empty()
	if distance > radius * (0.45 if blocked else 1.0):
		return false
	last_noise = location
	last_known = location
	# A nearby audible breath is evidence to inspect that bed, even without sight.
	if distance<3.0:
		for bed: Node3D in get_tree().get_nodes_in_group("hide_beds"):
			if bed.global_position.distance_to(location)<.8:
				remembered_bed = bed
				var entries: Array[Vector3] = bed.exit_points()
				bed_entry = entries[0] if global_position.distance_to(entries[0])<global_position.distance_to(entries[1]) else entries[1]
				last_known = bed_entry
	_set_state(State.INVESTIGATE)
	return true


func _physics_process(delta: float) -> void:
	_update_limb_hitboxes()
	warning.light_energy = 0.75 if windup > 0 and active else 0.0
	warning.light_color = Color("b4b36a") if can_spit else Color("ba6a4e")
	stagger_guard = maxf(0, stagger_guard - delta)
	if not active or target == null or navigation == null:
		return
	melee_remaining = maxf(0,melee_remaining-delta)
	model.attack_mode = attack_mode
	if knockback_remaining>0:
		knockback_remaining = maxf(0,knockback_remaining-delta)
		velocity = knockback_velocity
		move_and_slide()
		knockback_velocity = knockback_velocity.move_toward(Vector3.ZERO,delta*20)
		global_position.y = navigation.floor_height(global_position)+.08
		return
	model.animate(0.0 if stunned_remaining > 0.0 else velocity.length())
	model.chasing = state == State.CHASE or (alert_pursuer and state==State.INVESTIGATE)
	model.stunned = stunned_remaining > 0
	model.attack_progress = clampf(windup/.85,0,1)
	if stunned_remaining > 0:
		stunned_remaining = maxf(0, stunned_remaining - delta)
		velocity = Vector3.ZERO
		return
	if boss_controller!=null and boss_controller.exposed_remaining>0:
		velocity = Vector3.ZERO
		return
	if remembered_bed != null and target.hiding_bed == remembered_bed:
		last_known = bed_entry
		if _flat_distance(bed_entry)<1.5:
			get_tree().get_first_node_in_group("survival_tools").expose(self)
			return
	sees_player = can_see_player()
	attack_cooldown = maxf(0, attack_cooldown - delta)
	if lunge_remaining > 0:
		lunge_remaining = maxf(0, lunge_remaining - delta)
		velocity = lunge_heading * (10.0 if boss_controller!=null and boss_controller.phase==2 else 8.5) * mobility_factor()
		move_and_slide()
		return
	if windup > 0:
		windup = maxf(0, windup - delta)
		velocity = Vector3.ZERO
		if windup <= 0:
			if attack_mode=="swipe":
				melee_remaining = .28
				model.attack_progress = 1
			elif attack_mode=="feint":
				attack_cooldown = .18
			elif can_spit:
				ranged_attack.emit(global_position + Vector3(0, 1.8, 0), attack_point)
			else:
				lunge_heading = (attack_point - global_position).normalized()
				lunge_heading.y = 0
				lunge_remaining = 0.45
		return
	if melee_remaining>0:
		velocity = Vector3.ZERO
		model.attack_progress = 1-melee_remaining/.28
		return
	if boss_controller==null and attack_cooldown<=0 and (sees_player or (blind_hunter and state==State.INVESTIGATE)) and _flat_distance(last_known if blind_hunter else target.global_position)<1.8:
		var game: Node = get_tree().get_first_node_in_group("game")
		swings += 1
		attack_mode = "feint" if game.difficulty_id in ["nightmare","hardcore"] and swings%3==0 else "swipe"
		windup = (.35 if attack_mode=="feint" else .6)*float(game.difficulty_value("windup"))
		attack_cooldown = 1.8
		game.sound_bank.play_spatial("scrape",global_position,1.2)
		return
	if sees_player and attack_cooldown <= 0 and ((can_spit and _flat_distance(target.global_position) > 4) or (max_health > 6 and _flat_distance(target.global_position) > (1.25 if boss_controller!=null else 2.5) and _flat_distance(target.global_position) < 8)):
		attack_point = target.global_position + Vector3(0, 1, 0) if can_spit else target.global_position
		attack_mode = "spit" if can_spit else "lunge"
		windup = 0.85 if can_spit else (.55 if boss_controller!=null and boss_controller.phase==2 else .7)
		windup *= float(get_tree().get_first_node_in_group("game").difficulty_value("windup"))
		attack_cooldown = 5.5
		model.stunned = true
		return
	state_elapsed += delta
	if sees_player:
		last_known = target.global_position
		lost_sight = 0
		if state != State.CHASE:
			_set_state(State.CHASE)
	elif state == State.CHASE:
		lost_sight += delta
		if lost_sight > 1.25:
			_set_state(State.SEARCH)
	match state:
		State.PATROL:
			if patrol_points.is_empty():
				destination = spawn_position
			else:
				destination = patrol_points[patrol_index]
				if _flat_distance(destination) < 0.6:
					patrol_index = (patrol_index + 1) % patrol_points.size()
		State.CHASE, State.INVESTIGATE:
			destination = last_known
			if state == State.INVESTIGATE and _flat_distance(destination) < 0.7:
				_set_state(State.SEARCH)
			elif state == State.INVESTIGATE and state_elapsed > 9.0:
				_set_state(State.SEARCH)
		State.SEARCH:
			destination = last_known
			if state_elapsed > 9.0 and search_remaining <= 0:
				search_remaining = 0.01
			if search_remaining > 0 or _flat_distance(last_known) < 0.8:
				search_remaining += delta
				search_index = mini(int(search_remaining / 1.5), 3)
				var offset: Vector3 = [Vector3(1, 0, 0), Vector3(0, 0, 1), Vector3(-1, 0, 0), Vector3(0, 0, -1)][search_index]
				var cell: Vector2i = navigation.nearest(last_known + offset * 1.5)
				destination = Vector3(cell.x, global_position.y, cell.y)
				if search_remaining >= 6.0:
					_set_state(State.PATROL)
	_move_to(destination, delta)


func _move_to(goal: Vector3, delta: float) -> void:
	goal = _refuge_approach(goal)
	if door_target != null and door_wait_remaining > 0:
		velocity = Vector3.ZERO
		return
	repath_remaining -= delta
	if repath_remaining <= 0:
		var planned_goal: Vector3 = goal
		if use_minimax and state == State.CHASE and sees_player and _flat_distance(last_known) < 12:
			var decision: Dictionary = tactical.choose(navigation, navigation.nearest(global_position), navigation.nearest(last_known), 2, true)
			var cell: Vector2i = decision["action"]
			planned_goal = Vector3(cell.x, global_position.y, cell.y)
		planned_goal = _refuge_approach(planned_goal)
		route = navigation.find_clear_path(global_position, planned_goal)
		route_index = 1 if route.size() > 1 and _flat_distance(route[0]) < 0.8 else 0
		repath_remaining = 0.45 if state == State.CHASE else 0.9
	while route_index < route.size() and _flat_distance(route[route_index]) < 0.3:
		route_index += 1
	var point: Vector3 = global_position
	if route_index < route.size():
		point = route[route_index]
	elif _flat_distance(goal) < 0.8:
		point = goal
	var heading: Vector3 = point - global_position
	heading.y = 0
	if heading.length() > 0.15:
		heading = heading.normalized()
		var speed: float = move_speed if state == State.CHASE or (alert_pursuer and state==State.INVESTIGATE) else move_speed * 0.48
		speed *= mobility_factor()*(1.18 if boss_controller!=null and boss_controller.phase==2 else 1.0)
		velocity = velocity.move_toward(heading*speed,delta*13)
		var old_yaw: float = model.rotation.y
		model.rotation.y = rotate_toward(old_yaw,atan2(-heading.x,-heading.z),delta*(5.4 if crawler else 3.8))
		model.turn_amount = wrapf(model.rotation.y-old_yaw,-PI,PI)/maxf(delta,.001)*.15
	else:
		velocity = Vector3.ZERO
	move_and_slide()
	if navigation != null:
		global_position.y = navigation.floor_height(global_position) + 0.08


func _flat_distance(point: Vector3) -> float:
	return Vector2(point.x - global_position.x, point.z - global_position.z).length()


func _set_state(value: State) -> void:
	state = value
	state_elapsed = 0
	repath_remaining = 0
	route.clear()
	search_remaining = 0
	search_index = 0


func stun(seconds: float) -> void:
	stunned_remaining = maxf(stunned_remaining, seconds)
	velocity = Vector3.ZERO


func reset_hunter() -> void:
	melee_remaining = 0
	knockback_remaining = 0
	knockback_velocity = Vector3.ZERO
	swings = 0
	attack_mode = "swipe"
	for i: int in range(leg_damage.size()): leg_damage[i] = 0
	_refresh_injury()
	remembered_bed = null
	warning.light_energy = 0
	attack_cooldown = 3
	windup = 0
	lunge_remaining = 0
	stagger_guard = 0
	tactical.last_result.clear()
	if death_tween != null and death_tween.is_valid():
		death_tween.kill()
	dead = false
	health = max_health
	collision_layer = 4
	model.position = Vector3.ZERO
	position = spawn_position
	velocity = Vector3.ZERO
	stunned_remaining = 0
	last_known = spawn_position
	last_noise = spawn_position
	lost_sight = 0
	door_target = null
	door_wait_remaining = 0
	door_retry_remaining = 0
	sees_player = false
	patrol_index = 0
	model.rotation = Vector3.ZERO
	model.reset_animation()
	_set_state(State.PATROL)
	set_active(false)

func aim_point() -> Vector3:
	if boss_controller!=null and boss_controller.exposed_remaining>0: return boss_controller.weakpoint.global_position
	return global_position+Vector3(0,head_height,0)

func is_head_hit(at: Vector3) -> bool:
	return absf(at.y-global_position.y-head_height)<(.23 if crawler else .28)


func receive_bullet(point: Vector3, damage: int, headshot: bool, impact: Vector3 = Vector3.INF) -> bool:
	if dead or not active:
		return false
	if boss_controller!=null and not boss_controller.allow_damage(): return false
	if impact.is_finite() and not headshot and impact.y-global_position.y<(.48 if crawler else .75):
		_damage_leg(_nearest_leg(impact))
	health = maxi(0, health - damage)
	if boss_controller!=null: boss_controller.damage_received()
	model.react_to_hit(point,headshot)
	if headshot:
		windup = 0
		lunge_remaining = 0
		melee_remaining = 0
	if stagger_guard <= 0:
		stun((0.50 if headshot else 0.12) if max_health > 6 else (0.65 if headshot else 0.30))
		stagger_guard = 1.1 if max_health > 6 else 0.8
	if health <= 0:
		model.death_style = "blast" if damage>=4 else "head" if headshot else "body"
		dead = true
		active = false
		collision_layer = 0
		for limb: Area3D in limb_hitboxes: limb.collision_layer = 0
		velocity = Vector3.ZERO
		model.animate(0)
		model.chasing = false
		model.start_death(point)
	else:
		last_known = point
		_set_state(State.INVESTIGATE)
	return true

func apply_knockback(source: Vector3,force: float) -> void:
	if dead or not active: return
	knockback_velocity = (global_position-source).normalized()*force
	knockback_velocity.y = 0
	knockback_remaining = .24
	windup = 0
	lunge_remaining = 0
	melee_remaining = 0
	stun(.7)

func melee_contact_ready() -> bool:
	return lunge_remaining>0 or (boss_controller!=null and windup<=0) or melee_remaining>0

func mobility_factor() -> float:
	var injured: int = leg_damage.count(2)
	return 1.0 if injured==0 else (.58 if injured==1 else .35)

func receive_limbshot(source: Vector3, index: int) -> bool:
	if not active or dead: return false
	_damage_leg(index)
	if boss_controller!=null and not boss_controller.allow_damage():
		model.react_to_hit(source,false)
		model.react_to_limb(index)
		return true # Armour blocks health damage; joints can still be crippled.
	var accepted: bool = receive_bullet(source,1,false)
	model.react_to_limb(index)
	return accepted

func receive_weakspot(source: Vector3) -> bool:
	if boss_controller==null or boss_controller.exposed_remaining<=0: return false
	return receive_bullet(source,3,false)

func _damage_leg(index: int) -> void:
	if index<0 or index>=leg_damage.size(): return
	leg_damage[index] = mini(2,leg_damage[index]+1)
	_refresh_injury()
	if leg_damage[index]==2:
		var game: Node = get_tree().get_first_node_in_group("game")
		if game!=null:
			game.hud.announce("Конечность повреждена. Он движется медленнее — можно оторваться.",3)
			game.sound_bank.play_spatial("bone_hit",global_position,.9)

func _refresh_injury() -> void:
	if model==null: return
	model.injured_legs.clear()
	for damage: int in leg_damage: model.injured_legs.append(damage>=2)

func _nearest_leg(at: Vector3) -> int:
	var nearest: int = 0
	var distance: float = INF
	for i: int in range(limb_hitboxes.size()):
		var current: float = at.distance_squared_to(limb_hitboxes[i].global_position)
		if current<distance:
			distance = current
			nearest = i
	return nearest

func _build_limb_hitboxes() -> void:
	var count: int = model.legs.size() if model.guardian else model.limbs.size()
	for i: int in range(count):
		leg_damage.append(0)
		var area: Area3D = preload("res://scripts/limb_hitbox.gd").new()
		area.hunter = self
		area.index = i
		area.collision_layer = 0
		area.collision_mask = 0
		area.monitoring = false
		var capsule := CapsuleShape3D.new()
		capsule.radius = .10
		capsule.height = .5
		var shape := CollisionShape3D.new()
		shape.shape = capsule
		area.add_child(shape)
		(model.legs[i] if model.guardian else model.limbs[i]).add_child(area)
		limb_hitboxes.append(area)
	_update_limb_hitboxes()

func _update_limb_hitboxes() -> void:
	for i: int in range(limb_hitboxes.size()):
		var a: Vector3 = Vector3(0,-.4,0) if model.guardian else model.leg_elbows[i]
		var b: Vector3 = Vector3(0,-.82,-.03) if model.guardian else model.leg_feet[i]
		var area: Area3D = limb_hitboxes[i]
		area.position = (a+b)/2
		area.quaternion = Quaternion(Vector3.UP,(b-a).normalized())
		area.get_child(0).shape.height = maxf(.21,a.distance_to(b))

func _refuge_approach(point: Vector3) -> Vector3:
	var game: Node = get_tree().get_first_node_in_group("game")
	if game==null or game.refuge==null: return point
	if game.refuge.contains(point) or (point.y>-.6 and absf(point.x-8)<1.5 and point.z>15.45 and point.z<16.4):
		return Vector3(8,.1,14.8)
	return point
