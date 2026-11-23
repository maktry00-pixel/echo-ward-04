extends Node3D
## Physical hitscan: camera chooses a point, muzzle ray checks nearby cover.

const PistolVisual = preload("res://scripts/pistol_visual.gd")
const Builder = preload("res://scripts/world_builder.gd")
const MAGAZINE: int = 9
const RELOAD_SECONDS: float = 1.8
var game: Node3D
var equipped: bool = false
var aiming: bool = false
var rounds: int = 9
var reserve: int = 12
var cooldown: float = 0.0
var reload_remaining: float = 0.0
var hit_remaining: float = 0.0
var headshot_feedback: bool = false
var shots_fired: int = 0
var hits: int = 0
var kills: int = 0
var recoil: float = 0.0
var flash_remaining: float = 0.0
var weapon: Node3D
var muzzle: Node3D
var flash: OmniLight3D
var muzzle_flame: Node3D
var pistol: Node3D
var overlay: Control
var ammo_label: Label
var shotgun: Node3D
var shotgun_owned: bool = false
var shotgun_loaded: int = 0
var shells: int = 0
var selected_weapon: String = "pistol"
var shotgun_reloading: bool = false
var pump_remaining: float = 0
var wall_blend: float = 0


func configure(owner_game: Node3D) -> void:
	game = owner_game
	process_priority = 60
	weapon = Node3D.new()
	game.hero.add_child(weapon)
	pistol = PistolVisual.new()
	weapon.add_child(pistol)
	shotgun = preload("res://scripts/shotgun_visual.gd").new()
	weapon.add_child(shotgun)
	shotgun.hide()
	muzzle = Node3D.new()
	muzzle.position = PistolVisual.MUZZLE
	weapon.add_child(muzzle)
	flash = OmniLight3D.new()
	flash.light_color = Color("ffba72")
	flash.omni_range = 5
	flash.light_energy = 0
	muzzle.add_child(flash)
	muzzle_flame = Node3D.new()
	muzzle.add_child(muzzle_flame)
	for i: int in range(3):
		var plume := CylinderMesh.new()
		plume.top_radius = .008
		plume.bottom_radius = .045 if i==0 else .022
		plume.height = .22 if i==0 else .13
		plume.radial_segments = 5
		var flame := MeshInstance3D.new()
		flame.mesh = plume
		flame.material_override = Builder.material("ffe3a1" if i==0 else "e89a4a",3)
		flame.rotation = Vector3(PI/2,0,float(i)*TAU/3)
		flame.position = Vector3(0,0,-.08)
		muzzle_flame.add_child(flame)
	muzzle_flame.hide()
	var layer := CanvasLayer.new()
	layer.layer = 3
	add_child(layer)
	overlay = Control.new()
	overlay.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	overlay.mouse_filter = Control.MOUSE_FILTER_IGNORE
	layer.add_child(overlay)
	overlay.draw.connect(_draw_reticle)
	ammo_label = game.hud.weapon_label
	weapon.hide()


func knife_attack() -> bool:
	if not _can_act() or cooldown > 0 or game.hero.stamina < 20 or game.hero.dash_remaining > 0:
		return false
	game.hero.stamina -= 20
	game.hero.model.knife_remaining = .32
	game.hero.recovery_delay = 1.2
	cooldown = 0.75
	game.sound_bank.play_cue("scrape", 1.5)
	game.emit_noise(game.hero.global_position, 5)
	var blade := Node3D.new()
	game.feedback.add_child(blade)
	blade.global_position = game.hero.global_position + Vector3(0, 1.15, 0)
	blade.rotation.y = game.hero.camera_rig.rotation.y
	preload("res://scripts/imported_props.gd").detail(blade,"fish_knife",.34,Vector3(.28,0,-.40),Vector3(-PI/2,0,0))
	blade.rotation.z = -0.5
	var swing := blade.create_tween()
	swing.tween_property(blade, "rotation:z", 0.6, 0.20)
	swing.tween_callback(blade.queue_free)
	var forward: Vector3 = game.hero.camera_rig.movement_direction(Vector2.UP)
	for panel: StaticBody3D in get_tree().get_nodes_in_group("breakable_partitions"):
		if panel.broken: continue
		var point: Vector3 = panel.global_position
		var offset: Vector3 = point-game.hero.global_position-Vector3(0,1.1,0)
		var ray := PhysicsRayQueryParameters3D.create(game.hero.global_position+Vector3(0,1.1,0),point,1)
		var hit: Dictionary = get_world_3d().direct_space_state.intersect_ray(ray)
		if offset.length()<1.8 and forward.dot(offset.normalized())>.45 and hit.get("collider")==panel:
			panel.strike(game.hero.global_position)
	for lock: StaticBody3D in get_tree().get_nodes_in_group("breakable_locks"):
		var offset: Vector3 = lock.global_position-game.hero.global_position
		offset.y = 0
		var ray := PhysicsRayQueryParameters3D.create(game.hero.global_position+Vector3(0,1.1,0),lock.global_position,1)
		if not lock.broken and offset.length()<1.8 and forward.dot(offset.normalized())>.45 and get_world_3d().direct_space_state.intersect_ray(ray).is_empty():
			lock.break_lock()
	for hunter: CharacterBody3D in game.hunters:
		var offset: Vector3 = hunter.global_position - game.hero.global_position
		offset.y = 0
		if hunter.active and offset.length() <= 1.8 and forward.dot(offset.normalized()) > 0.45 and game._line_clear(game.hero.global_position, hunter.global_position):
			if not hunter.receive_bullet(game.hero.global_position,1,false): continue
			game.feedback.burst(hunter.global_position + Vector3(0, 1.3, 0), "ac9474")
			hit_remaining = 0.18
			headshot_feedback = false
			if hunter.dead:
				kills += 1
	return true


func update(delta: float) -> void:
	var old_pump: float = pump_remaining
	pump_remaining = maxf(0,pump_remaining-delta)
	if old_pump>.35 and pump_remaining<=.35:
		game.sound_bank.play_spatial("pump",game.hero.position)
		var spent := preload("res://scripts/spent_casing.gd").new()
		spent.shotgun_shell = true
		game.feedback.add_child(spent)
		spent.global_position = game.hero.position+Vector3(0,1.3,0)
		spent.linear_velocity = Basis(Vector3.UP,game.hero.camera_rig.rotation.y)*Vector3(2.2,1.1,0)
		spent.angular_velocity = Vector3(3,7,2)
	cooldown = maxf(0, cooldown - delta)
	hit_remaining = maxf(0, hit_remaining - delta)
	recoil = maxf(0, recoil - delta * 3.5)
	flash_remaining = maxf(0, flash_remaining - delta)
	flash.light_energy = 5.0 if flash_remaining > 0 else 0.0
	muzzle_flame.visible = flash_remaining>0
	game.hero.reload_phase = 1-reload_remaining/(.85 if shotgun_reloading else RELOAD_SECONDS) if reload_remaining>0 else -1
	if reload_remaining > 0:
		reload_remaining = maxf(0, reload_remaining - delta)
		if reload_remaining <= 0:
			if shotgun_reloading:
				if shells>0 and shotgun_loaded<4:
					shells -= 1
					shotgun_loaded += 1
					game.sound_bank.play_cue("shell_load")
				if shells>0 and shotgun_loaded<4: reload_remaining = .85
				else: shotgun_reloading = false
			else:
				var count: int = mini(MAGAZINE - rounds, reserve)
				rounds += count
				reserve -= count
	aiming = equipped and game.hero.hiding_bed == null and not game.hero.performing_action and Input.is_action_pressed("aim") and game.hero.dash_remaining <= 0
	game.hero.aiming = aiming
	game.hero.camera_rig.aiming = aiming
	var forward: Vector3 = game.hero.camera_rig.movement_direction(Vector2.UP)
	var from: Vector3 = game.hero.global_position+Vector3(0,1.35,0)
	var near_cover: Dictionary = get_world_3d().direct_space_state.intersect_ray(PhysicsRayQueryParameters3D.create(from,from+forward*(.95 if selected_weapon=="shotgun" else .5),1))
	wall_blend = move_toward(wall_blend,0.0 if near_cover.is_empty() else 1.0,delta*7)
	update_weapon_pose()
	weapon.visible = equipped and not game.hero.free_flight and game.hero.hiding_bed == null and not game.hero.performing_action and game.hero.model.knife_remaining<=0 and not game.in_title_menu and not game.has_won and not game.has_lost
	overlay.visible = not game.hero.free_flight and not game.in_title_menu and not game.is_paused and not game.has_won and not game.has_lost
	ammo_label.text = "СЛУЖЕБНЫЙ 9 мм\n%d / %02d   ·   R перезарядка" % [rounds, reserve] if equipped else "Оружие в приёмной\nПКМ прицел · ЛКМ выстрел"
	if reload_remaining > 0:
		ammo_label.text = "ПЕРЕЗАРЯДКА  %.1f\n%d / %02d" % [reload_remaining, rounds, reserve]
	if selected_weapon=="shotgun":
		ammo_label.text = ("ЗАРЯЖАНИЕ ПО ОДНОМУ" if shotgun_reloading else "ПОМПОВЫЙ / 12 КАЛИБР")+"\n%d / %02d · R зарядить · Z оружие" % [shotgun_loaded,shells]
	elif shotgun_owned: ammo_label.text += "\nZ — дробовик"
	overlay.queue_redraw()

func _process(_delta: float) -> void:
	if game != null: update_weapon_pose()

func update_weapon_pose() -> void:
	var yaw: float = game.hero.camera_rig.rotation.y
	var basis_yaw := Basis(Vector3.UP, yaw)
	weapon.position = Vector3(0, 1.35, 0) + basis_yaw * Vector3(0.24, 0, -0.36+recoil*.035)
	weapon.rotation = Vector3(game.hero.camera_rig.rotation.x + recoil * 0.15, yaw, 0)
	if selected_weapon=="shotgun" and aiming: weapon.position.y += .13
	if not aiming:
		weapon.rotation.x -= 0.8
		weapon.rotation.y = game.hero.model.rotation.y
		weapon.global_position = game.hero.model.hand_world + Vector3(0, 0.08, 0)
	if reload_remaining > 0:
		weapon.rotation.z = sin(reload_remaining * 4) * 0.35
		weapon.global_position = game.hero.model.hand_world+Vector3(0,.06,0)
		weapon.rotation.x = -.25
	else:
		weapon.rotation.z = 0
	if selected_weapon=="shotgun" and reload_remaining>0:
		weapon.position = Vector3(0,1.14,0)+basis_yaw*Vector3(.20,0,-.22)
		weapon.rotation = Vector3(-.25,yaw,.18)
	weapon.rotation.x += wall_blend*1.1
	pistol.pose(recoil,1-reload_remaining/RELOAD_SECONDS if reload_remaining>0 else -1)
	shotgun.pose(recoil,1-reload_remaining/.85 if shotgun_reloading else -1,pump_remaining/.65)
	pistol.visible = selected_weapon=="pistol"
	shotgun.visible = selected_weapon=="shotgun"
	muzzle.position = preload("res://scripts/shotgun_visual.gd").MUZZLE if selected_weapon=="shotgun" else PistolVisual.MUZZLE


func acquire() -> void:
	equipped = true
	game.hud.announce("Служебный пистолет. ПКМ — прицел, ЛКМ — выстрел, R — перезарядить. Голова уязвима. Выстрелы слышны далеко.", 9)


func reload_weapon() -> bool:
	if selected_weapon=="shotgun":
		if not _can_act() or not shotgun_owned or reload_remaining>0 or shotgun_loaded>=4 or shells<=0: return false
		shotgun_reloading = true
		reload_remaining = .85
		game.sound_bank.play_cue("shell_load")
		return true
	if not _can_act() or not equipped or reload_remaining > 0 or rounds >= MAGAZINE or reserve <= 0:
		return false
	reload_remaining = RELOAD_SECONDS
	game.sound_bank.play_cue("reload")
	return true


func _can_act() -> bool:
	return not game.hero.free_flight and not game.hero.in_refuge and game.hero.hiding_bed == null and not game.hero.performing_action and not game.in_title_menu and not game.is_paused and not game.has_won and not game.has_lost


func fire() -> Dictionary:
	if selected_weapon=="shotgun": return _fire_shotgun()
	if not _can_act() or not equipped or not aiming or cooldown > 0 or reload_remaining > 0 or game.hero.dash_remaining > 0:
		return {"fired": false}
	if rounds <= 0:
		cooldown = 0.25
		game.sound_bank.play_cue("empty")
		game.hud.announce("Магазин пуст. R — перезарядить.", 2)
		return {"fired": false}
	rounds -= 1
	shots_fired += 1
	cooldown = 0.38
	recoil = 1.0
	game.hero.model.shot_remaining = .16
	flash_remaining = 0.07
	var casing := preload("res://scripts/spent_casing.gd").new()
	game.feedback.add_child(casing)
	casing.global_position = weapon.global_position+Vector3(0,.03,0)
	casing.linear_velocity = Basis(Vector3.UP,weapon.rotation.y)*Vector3(2.4,1.5,.3)
	casing.angular_velocity = Vector3(9,4,7)
	game.hero.camera_rig.shake_remaining = 0.16
	game.sound_bank.play_spatial("gunshot", game.hero.global_position)
	game.emit_noise(game.hero.global_position, 38)
	var camera: Camera3D = game.hero.camera_rig.camera
	var center: Vector2 = camera.get_viewport().get_visible_rect().size / 2.0
	var origin: Vector3 = camera.project_ray_origin(center)
	var direction: Vector3 = camera.project_ray_normal(center)
	direction = _assisted_direction(origin,direction)
	var space: PhysicsDirectSpaceState3D = get_world_3d().direct_space_state
	var camera_query := PhysicsRayQueryParameters3D.create(origin, origin + direction * 60, 13)
	camera_query.collide_with_areas = true
	var camera_hit: Dictionary = space.intersect_ray(camera_query)
	var target: Vector3 = camera_hit.get("position", origin + direction * 60)
	# A second ray from the hand prevents shooting through a wall beside the shoulder.
	var muzzle_origin: Vector3 = game.hero.global_position + Vector3(0, 1.35, 0)
	var hand_cover: Dictionary = space.intersect_ray(PhysicsRayQueryParameters3D.create(muzzle_origin,muzzle.global_position,1))
	var bullet_query := PhysicsRayQueryParameters3D.create(muzzle.global_position, target + direction * .05, 13)
	bullet_query.collide_with_areas = true
	var hit: Dictionary = hand_cover if not hand_cover.is_empty() else space.intersect_ray(bullet_query)
	var bullet_direction: Vector3 = (target-muzzle.global_position).normalized()
	var penetrated: bool = false
	if not hit.is_empty() and hit.collider.has_method("strike") and hit.collider.is_in_group("breakable_partitions"):
		var panel: StaticBody3D = hit.collider
		panel.strike(game.hero.global_position)
		game.feedback.impact(hit.position,hit.get("normal",Vector3.UP),false,direction,"metal" if panel.kind=="sheet" else "concrete")
		# Exactly one authored thin panel; the continuation still stops at solid cover.
		var continuation := PhysicsRayQueryParameters3D.create(hit.position+bullet_direction*.12,hit.position+bullet_direction*60,13)
		continuation.exclude = [panel.get_rid()]
		continuation.collide_with_areas = true
		hit = space.intersect_ray(continuation)
		penetrated = true
		if hit.is_empty(): target += bullet_direction*25
	var endpoint: Vector3 = hit.get("position", target)
	_tracer(muzzle.global_position, endpoint)
	var result := {"fired": true, "hit": false, "headshot": false, "killed": false}
	result["impact"] = endpoint
	result["penetrated"] = penetrated
	if not hit.is_empty():
		var collider: Object = hit["collider"]
		if collider.has_method("break_lock"):
			result["hit"] = collider.break_lock()
			hit_remaining = .18
			headshot_feedback = false
		elif collider.has_method("receive_bullet"):
			var headshot: bool = collider.is_head_hit(endpoint)
			if collider.receive_bullet(game.hero.global_position, 3 if headshot else 1, headshot,endpoint):
				hits += 1
				hit_remaining = 0.18
				headshot_feedback = headshot
				result["hit"] = true
				result["headshot"] = headshot
				result["killed"] = collider.dead
				if collider.dead:
					kills += 1
				game.feedback.impact(endpoint,hit.get("normal",Vector3.UP),true,direction)
				game.sound_bank.play_spatial("flesh_hit",endpoint)
			else:
				game.feedback.impact(endpoint,hit.get("normal",Vector3.UP),false,direction,"metal")
		else:
			var surface: String = str(collider.get_meta("impact_surface","concrete"))
			game.feedback.impact(endpoint,hit.get("normal",Vector3.UP),false,direction,surface)
			game.sound_bank.play_spatial("metal_hit" if surface=="metal" else ("wood_hit" if surface=="wood" else "stone_hit"),endpoint)
	if game.hero.camera_rig.motion_enabled: game.hero.camera_rig.rotation.x = minf(deg_to_rad(25),game.hero.camera_rig.rotation.x+.014)
	return result


func _tracer(a: Vector3, b: Vector3) -> void:
	var distance: float = a.distance_to(b)
	if distance < 0.01:
		return
	var holder: Node3D = Builder.box(game.feedback, Vector3(0.012, 0.012, distance), (a + b) / 2, Builder.material("f2cc8b", 1.5))
	holder.look_at(b)
	var tween := holder.create_tween()
	tween.tween_interval(0.045)
	tween.tween_callback(holder.queue_free)


func _draw_reticle() -> void:
	if not aiming or not game.settings.get("crosshair",true):
		return
	var center: Vector2 = overlay.size / 2
	var gap: float = 6 + recoil * 9 + minf(game.hero.velocity.length(), 4) * 1.4
	var color := Color("eadfc7")
	for axis: Vector2 in [Vector2.RIGHT, Vector2.LEFT, Vector2.UP, Vector2.DOWN]:
		overlay.draw_line(center + axis * gap, center + axis * (gap + 7), Color(0, 0, 0, 0.8), 4)
		overlay.draw_line(center + axis * gap, center + axis * (gap + 7), color, 1.5)
	if hit_remaining > 0:
		var hit_color := Color("e5a76a") if headshot_feedback else Color.WHITE
		for axis: Vector2 in [Vector2(1, 1), Vector2(-1, 1), Vector2(1, -1), Vector2(-1, -1)]:
			overlay.draw_line(center + axis * 5, center + axis * 10, hit_color, 2)


func reset_combat() -> void:
	shotgun_owned = false
	shotgun_loaded = 0
	shells = 0
	selected_weapon = "pistol"
	shotgun_reloading = false
	pump_remaining = 0
	wall_blend = 0
	equipped = false
	aiming = false
	rounds = MAGAZINE
	reserve = 12
	reload_remaining = 0
	cooldown = 0
	shots_fired = 0
	hits = 0
	kills = 0
	hit_remaining = 0
	recoil = 0
	flash_remaining = 0
	flash.light_energy = 0
	muzzle_flame.hide()
	game.hero.reload_phase = -1
	game.hero.aiming = false
	game.hero.camera_rig.aiming = false
	weapon.hide()

func select_weapon(id: String) -> bool:
	if id not in ["pistol","shotgun"] or not equipped or (id=="shotgun" and not shotgun_owned): return false
	if game.has_won or game.has_lost or game.hero.performing_action: return false
	reload_remaining = 0
	shotgun_reloading = false
	selected_weapon = id
	cooldown = maxf(cooldown,.18)
	update_weapon_pose()
	return true

func cycle_weapon() -> bool: return select_weapon("pistol" if selected_weapon=="shotgun" else "shotgun")

func _assisted_direction(origin: Vector3,direction: Vector3) -> Vector3:
	var cone: float = float(game.difficulty_value("assist"))
	if cone<=0: return direction
	var best: float = cone
	var wanted: Vector3 = direction
	for hunter: CharacterBody3D in game.hunters:
		if not hunter.active or hunter.dead: continue
		var point: Vector3 = hunter.aim_point()
		if origin.distance_to(point)>18: continue
		var ray := PhysicsRayQueryParameters3D.create(origin,point,1)
		if not get_world_3d().direct_space_state.intersect_ray(ray).is_empty(): continue
		var candidate: Vector3 = (point-origin).normalized()
		var angle: float = direction.angle_to(candidate)
		if angle<best: best = angle; wanted = candidate
	return direction.slerp(wanted,.55).normalized()

func _fire_shotgun() -> Dictionary:
	if not _can_act() or not shotgun_owned or not aiming or cooldown>0 or game.hero.dash_remaining>0: return {"fired":false}
	if shotgun_reloading:
		shotgun_reloading = false
		reload_remaining = 0
		cooldown = .12
		return {"fired":false,"interrupted_reload":true}
	if shotgun_loaded<=0:
		cooldown = .3
		game.sound_bank.play_cue("empty")
		game.hud.announce("Дробовик пуст. R — заряжать по одному патрону.",3)
		return {"fired":false}
	shotgun_loaded -= 1
	shots_fired += 1
	cooldown = .95
	pump_remaining = .65
	recoil = 1.5
	game.hero.model.shot_remaining = .30
	flash_remaining = .09
	game.hero.camera_rig.shake_remaining = .30
	game.sound_bank.play_spatial("shotgun",game.hero.position)
	game.emit_noise(game.hero.position,48)
	var camera: Camera3D = game.hero.camera_rig.camera
	var center: Vector2 = camera.get_viewport().get_visible_rect().size/2
	var origin: Vector3 = camera.project_ray_origin(center)
	var forward: Vector3 = _assisted_direction(origin,camera.project_ray_normal(center))
	var victims: Dictionary = {}
	var result := {"fired":true,"hit":false,"headshot":false,"killed":false,"pellets":8}
	for i: int in range(8):
		var spread: Vector2 = Vector2.ZERO if i==0 else Vector2(cos(i*2.4),sin(i*2.4))*(.025 if i<4 else .05)
		var direction: Vector3 = (forward+camera.global_basis.x*spread.x+camera.global_basis.y*spread.y).normalized()
		var impact: Dictionary = _pellet_ray(origin,direction)
		var at: Vector3 = impact.get("position",origin+direction*20)
		_tracer(muzzle.global_position,at)
		if impact.is_empty(): continue
		var collider: Object = impact.collider
		if collider.has_method("receive_bullet"):
			var enemy: Object = collider.get("hunter") if collider is Area3D else collider
			var key: int = enemy.get_instance_id()
			if not victims.has(key): victims[key] = {"collider":collider,"enemy":enemy,"at":at,"normal":impact.normal,"count":0,"head":collider.is_head_hit(at)}
			victims[key].count += 1
			if collider is Area3D and collider.get("weakpoint")==true: victims[key].collider = collider
		elif collider.has_method("break_lock"):
			collider.break_lock()
		else: game.feedback.impact(at,impact.normal,false,direction,str(collider.get_meta("impact_surface","concrete")))
	for victim: Dictionary in victims.values():
		var damage: int = mini(int(victim.count),6 if game.hero.position.distance_to(victim.at)<7 else 2)
		if victim.collider.receive_bullet(game.hero.position,damage,victim.head,victim.at):
			hits += 1
			result.hit = true
			result.headshot = result.headshot or victim.head
			hit_remaining = .22
			headshot_feedback = victim.head
			game.feedback.impact(victim.at,victim.normal,true,forward)
			game.sound_bank.play_spatial("flesh_hit",victim.at,.8)
			if victim.enemy.dead:
				kills += 1
				result.killed = true
			else: victim.enemy.apply_knockback(game.hero.position,4.5)
	if game.hero.camera_rig.motion_enabled: game.hero.camera_rig.rotation.x = minf(deg_to_rad(25),game.hero.camera_rig.rotation.x+.045)
	return result

func _pellet_ray(origin: Vector3,direction: Vector3) -> Dictionary:
	var space: PhysicsDirectSpaceState3D = get_world_3d().direct_space_state
	var camera_query := PhysicsRayQueryParameters3D.create(origin,origin+direction*20,13)
	camera_query.collide_with_areas = true
	var camera_hit: Dictionary = space.intersect_ray(camera_query)
	var point: Vector3 = camera_hit.get("position",origin+direction*20)
	var hand: Vector3 = game.hero.position+Vector3(0,1.35,0)
	var cover: Dictionary = space.intersect_ray(PhysicsRayQueryParameters3D.create(hand,muzzle.global_position,1))
	var query := PhysicsRayQueryParameters3D.create(muzzle.global_position,point+direction*.05,13)
	query.collide_with_areas = true
	var hit: Dictionary = cover if not cover.is_empty() else space.intersect_ray(query)
	if not hit.is_empty() and hit.collider.is_in_group("breakable_partitions"):
		var panel: StaticBody3D = hit.collider
		panel.strike(game.hero.position)
		var continuation := PhysicsRayQueryParameters3D.create(hit.position+direction*.12,hit.position+direction*20,13)
		continuation.exclude = [panel.get_rid()]
		continuation.collide_with_areas = true
		hit = space.intersect_ray(continuation)
	return hit

func grip_points() -> Array[Vector3]:
	if selected_weapon=="shotgun": return [weapon.global_transform*Vector3(0,-.10,.06),weapon.global_transform*Vector3(0,-.07,-.36+shotgun.pump.position.z)]
	var right: Vector3 = weapon.global_transform*Vector3(.03,-.075,.085)
	return [right,right+weapon.global_basis*Vector3(-.065,-.015,.005)]

func drop_weapon() -> void:
	weapon.hide()
	if not equipped: return
	var dropped: RigidBody3D = preload("res://scripts/fallen_weapon.gd").new()
	dropped.add_to_group("dropped_weapons")
	dropped.process_mode = Node.PROCESS_MODE_ALWAYS
	dropped.mass = 3.1 if selected_weapon=="shotgun" else .8
	dropped.collision_layer = 0
	dropped.collision_mask = 1
	dropped.continuous_cd = true
	var shape := BoxShape3D.new()
	shape.size = Vector3(.09,.20,1.16) if selected_weapon=="shotgun" else Vector3(.033,.13,.222)
	var collision := CollisionShape3D.new()
	collision.shape = shape
	collision.position = Vector3(0,-.025,-.18) if selected_weapon=="shotgun" else Vector3(0,-.066,-.063)
	dropped.add_child(collision)
	var visual: Node3D = preload("res://scripts/shotgun_visual.gd").new() if selected_weapon=="shotgun" else PistolVisual.new()
	dropped.add_child(visual)
	game.feedback.add_child(dropped)
	dropped.global_transform = weapon.global_transform
	dropped.linear_velocity = Vector3(.5,.5,-.4)
	dropped.angular_velocity = Vector3(2,3,1)
