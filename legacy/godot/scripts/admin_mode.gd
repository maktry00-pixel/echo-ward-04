extends Node
## Local developer spectator. Flying does not write progress or alter saved resources.
const UI=preload("res://scripts/ui_style.gd")
var game:Node3D
var pilot:Node
var active:bool=false
var camera:Camera3D
var light:SpotLight3D
var safe_anchor:Vector3
var controls:CanvasLayer
var panel:PanelContainer
var pilot_button:Button
var status:Label
var follow:CheckButton
var access_layer:CanvasLayer
var access:PanelContainer
var backdrop:ColorRect
var password:LineEdit
var error_label:Label
func configure(owner_game:Node3D)->void:
	game=owner_game;process_mode=Node.PROCESS_MODE_ALWAYS;process_physics_priority=-100
	var bindings:Dictionary={"admin_left":KEY_A,"admin_right":KEY_D,"admin_forward":KEY_W,"admin_back":KEY_S,"admin_up":KEY_SPACE,"admin_down":KEY_CTRL,"admin_fast":KEY_SHIFT}
	for action:String in bindings:
		if not InputMap.has_action(action):InputMap.add_action(action)
		var key:=InputEventKey.new();key.physical_keycode=bindings[action]
		if not InputMap.action_has_event(action,key):InputMap.action_add_event(action,key)
	pilot=preload("res://scripts/game_autopilot.gd").new();add_child(pilot);pilot.configure(game);pilot.state_changed.connect(_pilot_changed)
	camera=Camera3D.new();camera.near=.04;camera.far=200;camera.fov=70;add_child(camera)
	light=SpotLight3D.new();light.light_color=Color("eee1bc");light.light_energy=4;light.spot_range=20;light.spot_angle=40;camera.add_child(light);light.hide()
	controls=CanvasLayer.new();controls.layer=25;add_child(controls)
	panel=PanelContainer.new();panel.set_anchors_and_offsets_preset(Control.PRESET_CENTER_TOP);panel.offset_left=-405;panel.offset_right=405;panel.offset_top=20;panel.add_theme_stylebox_override("panel",UI.panel(.94));controls.add_child(panel)
	var body:=VBoxContainer.new();body.add_theme_constant_override("separation",8);panel.add_child(body)
	var row:=HBoxContainer.new();row.add_theme_constant_override("separation",12);body.add_child(row)
	status=UI.label("АДМИН · свободный полёт",15,UI.TEAL);status.size_flags_horizontal=Control.SIZE_EXPAND_FILL;status.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART;row.add_child(status)
	pilot_button=UI.button("Запустить ИИ-агента · F9",true);pilot_button.pressed.connect(toggle_pilot);row.add_child(pilot_button)
	var leave_button:=UI.button("Выйти · F10");leave_button.pressed.connect(leave);row.add_child(leave_button)
	follow=CheckButton.new();follow.text="Следовать за агентом";follow.button_pressed=true;follow.add_theme_font_override("font",UI.SANS);body.add_child(follow)
	var hint:=UI.label("WASD — полёт · Space / Ctrl — вверх / вниз · Shift — быстрее · F — действие · F1 — курсор",12,UI.MUTED);hint.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART;body.add_child(hint)
	controls.hide()
	access_layer=CanvasLayer.new();access_layer.layer=65;add_child(access_layer)
	backdrop=ColorRect.new();backdrop.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT);backdrop.color=Color(0,0,0,.75);access_layer.add_child(backdrop)
	access=PanelContainer.new();access.set_anchors_and_offsets_preset(Control.PRESET_CENTER);access.offset_left=-260;access.offset_right=260;access.offset_top=-190;access.offset_bottom=190;access.add_theme_stylebox_override("panel",UI.panel());access_layer.add_child(access)
	var form:=VBoxContainer.new();form.add_theme_constant_override("separation",16);access.add_child(form)
	form.add_child(UI.heading("Режим админа",30));form.add_child(UI.label("Введите пароль",16,UI.MUTED))
	password=LineEdit.new();password.secret=true;password.max_length=16;password.custom_minimum_size.y=48;password.add_theme_font_override("font",UI.SANS);form.add_child(password)
	password.text_submitted.connect(func(_text:String)->void:submit_password())
	error_label=UI.label("",15,UI.CORAL);form.add_child(error_label)
	var unlock:=UI.button("Войти в режим полёта",true);unlock.pressed.connect(submit_password);form.add_child(unlock)
	var cancel:=UI.button("Отмена · Esc");cancel.pressed.connect(close_access);form.add_child(cancel);access_layer.hide()
func open_access()->void:
	if active:leave();return
	password.clear();error_label.text="";access_layer.show();password.grab_focus();Input.mouse_mode=Input.MOUSE_MODE_VISIBLE
func close_access()->void:
	access_layer.hide();password.clear();game.front_end.settings_view.admin_button.grab_focus()
func submit_password()->void:
	if password.text!="0890":error_label.text="Неверный пароль";password.select_all();password.grab_focus();return
	access_layer.hide();password.clear();enter()
func enter()->void:
	if active:return
	if not game.campaign_started:
		if not game.saves.load_latest():game._new_game()
	elif game.has_lost or game.has_won:
		if not game.saves.load_latest():game.restart_game()
	game.in_title_menu=false;game.front_end.hide();game.hud.show();game.set_paused(false)
	safe_anchor=game.hero.global_position
	camera.global_transform=game.hero.camera_rig.camera.global_transform
	active=true;camera.current=true;light.show();controls.show();_manual_body();_pilot_changed()
	var task:Control=game.hud.objective_label.get_parent();task.position.y=195;task.size.x=360
	Input.mouse_mode=Input.MOUSE_MODE_CAPTURED
func leave()->void:
	if not active:return
	active=false;pilot.stop();_normal_body()
	game.hero.reset_to(_safe_return(safe_anchor));game.hero.camera_rig.input_enabled=not game.is_paused
	camera.current=false;game.hero.camera_rig.camera.current=true;light.hide();controls.hide()
	var task:Control=game.hud.objective_label.get_parent();task.position.y=36;task.size.x=620;game.hud.apply_preferences(game.settings)
	if not game.is_paused:game._capture_mouse()
func _normal_body()->void:
	game.hero.free_flight=false;game.hero.collision_layer=2;game.hero.collision_mask=1
func _manual_body()->void:
	game.annex.cancel_work()
	game.hero.free_flight=true;game.hero.collision_layer=0;game.hero.collision_mask=0
	game.hero.invulnerable_remaining=0
	game.hero.hiding_bed=null;game.hero.model.hide();game.combat.weapon.hide();game.combat.overlay.hide();game.hero.camera_rig.input_enabled=false
func toggle_pilot()->void:
	if not active:return
	if pilot.running:pilot.stop()
	else:
		_normal_body();game.hero.reset_to(_safe_return(safe_anchor));game.hero.camera_rig.input_enabled=false;pilot.start()
	if not game.automated:Input.mouse_mode=Input.MOUSE_MODE_CAPTURED
func _pilot_changed()->void:
	pilot_button.text="Остановить ИИ-агента · F9" if pilot.running else "Запустить ИИ-агента · F9"
	status.text="АДМИН · свободный полёт" if pilot.status=="ИИ-агент остановлен" else pilot.status
	if active and not pilot.running:
		safe_anchor=game.hero.global_position;_manual_body()
func restore_after_reset()->void:
	if not active:return
	game.hero.camera_rig.input_enabled=false;camera.current=true
	if pilot.running:_normal_body()
	else:safe_anchor=game.hero.global_position;_manual_body()
func _physics_process(delta:float)->void:
	if not active or game.is_paused or game.has_won or game.has_lost:return
	var direction:=Vector3.ZERO
	if Input.mouse_mode==Input.MOUSE_MODE_CAPTURED:
		direction=Vector3(Input.get_action_strength("admin_right")-Input.get_action_strength("admin_left"),Input.get_action_strength("admin_up")-Input.get_action_strength("admin_down"),Input.get_action_strength("admin_back")-Input.get_action_strength("admin_forward"))
	if direction.length_squared()>0:
		if pilot.running:follow.button_pressed=false
		var motion:Vector3=camera.global_basis*Vector3(direction.x,0,direction.z)+Vector3.UP*direction.y
		camera.global_position+=motion.normalized()*delta*(18 if Input.is_action_pressed("admin_fast") else 6)
	if pilot.running:
		if follow.button_pressed:camera.global_transform=game.hero.camera_rig.camera.global_transform
	else:
		game.hero.global_position=camera.global_position-Vector3(0,1.55,0)
		game.hero.camera_rig.rotation=Vector3(camera.rotation.x,camera.rotation.y,0)
		game.hero.model.hide();game.combat.weapon.hide()
	light.visible=game.hero.camera_rig.flashlight_enabled
func _input(event:InputEvent)->void:
	if access_layer.visible:
		if event is InputEventKey and event.pressed and event.physical_keycode==KEY_ESCAPE:close_access();get_viewport().set_input_as_handled()
		return
	if not active:return
	if event is InputEventMouseMotion and Input.mouse_mode==Input.MOUSE_MODE_CAPTURED and not game.is_paused:
		if not pilot.running or not follow.button_pressed:
			camera.rotation.y-=event.screen_relative.x*game.hero.camera_rig.look_sensitivity
			camera.rotation.x=clampf(camera.rotation.x-event.screen_relative.y*game.hero.camera_rig.look_sensitivity,deg_to_rad(-89),deg_to_rad(89))
	if event is InputEventKey and event.pressed and not event.echo:
		match event.physical_keycode:
			KEY_F1:Input.mouse_mode=Input.MOUSE_MODE_VISIBLE if Input.mouse_mode==Input.MOUSE_MODE_CAPTURED else Input.MOUSE_MODE_CAPTURED
			KEY_F9:toggle_pilot()
			KEY_F10:leave()
			_:return
		get_viewport().set_input_as_handled()
func _safe_return(at:Vector3)->Vector3:
	var space:=game.get_world_3d().direct_space_state
	var shape:=CapsuleShape3D.new();shape.radius=.34;shape.height=1.7
	var query:=PhysicsShapeQueryParameters3D.new();query.shape=shape;query.collision_mask=1|4;query.exclude=[game.hero.get_rid()];query.transform=Transform3D(Basis.IDENTITY,at+Vector3(0,.87,0))
	var floor_hit:Dictionary=space.intersect_ray(PhysicsRayQueryParameters3D.create(at+Vector3(0,.3,0),at-Vector3(0,.5,0),1))
	if space.intersect_shape(query).is_empty() and not floor_hit.is_empty():return at
	return game.facility.START
