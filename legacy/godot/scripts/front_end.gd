extends CanvasLayer
signal new_game_requested
signal continue_requested
signal settings_changed(values:Dictionary)
signal saves_requested
const UI=preload("res://scripts/ui_style.gd")
const Model=preload("res://scripts/presentation_settings.gd")
var values:Dictionary=Model.DEFAULTS.duplicate()
var root_control:Control
var menu_content:Control
var menu_scroll:ScrollContainer
var main_view:VBoxContainer
var new_game_button:Button
var load_button:Button
var continue_button:Button
var game_button:Button
var all_saves_button:Button
var start_button:Button
var main_actions:HBoxContainer
var game_view:PanelContainer
var difficulty_view:PanelContainer
var guide_return:String="main"
var maps_button:Button
var online_button:Button
var training_button:Button
var controls_button:Button
var about_button:Button
var difficulty_picker:OptionButton
var difficulty_note:Label
var settings_view:PanelContainer
var language_picker:OptionButton
var quality_picker:OptionButton
var fullscreen:CheckButton
var camera_motion:CheckButton
var effects:HSlider
var music:HSlider
var sensitivity:HSlider
var info:Control
var menu_camera:Camera3D
var story_card:PanelContainer
var page_mode:String="main"
func _ready()->void:
	layer=20;process_mode=Node.PROCESS_MODE_ALWAYS
	root_control=Control.new();root_control.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT);add_child(root_control)
	var shade:=ColorRect.new();shade.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	var material:=ShaderMaterial.new();material.shader=Shader.new()
	material.shader.code="shader_type canvas_item; void fragment(){float haze=.008*sin(UV.y*12.+TIME*.14);float a=mix(.99,.46,smoothstep(.20,.85,UV.x));COLOR=vec4(vec3(.014,.034,.036)+haze,a);}"
	shade.material=material;root_control.add_child(shade)
	menu_content=Control.new();menu_content.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT);root_control.add_child(menu_content)
	menu_scroll=ScrollContainer.new();menu_scroll.horizontal_scroll_mode=ScrollContainer.SCROLL_MODE_DISABLED;menu_content.add_child(menu_scroll)
	var column:=VBoxContainer.new();column.size_flags_horizontal=Control.SIZE_EXPAND_FILL;column.add_theme_constant_override("separation",12);menu_scroll.add_child(column)
	var logo:=Control.new();logo.custom_minimum_size.y=112;column.add_child(logo)
	for i:int in range(3,-1,-1):
		var echo:=UI.heading("EXO",84);echo.position=Vector2(i*11,0);echo.modulate=Color("173c3b") if i>0 else Color.WHITE;echo.z_index=4-i
		logo.add_child(echo)
	column.add_child(UI.label("ОТДЕЛЕНИЕ 04 · ПОСЛЕДНЯЯ СМЕНА",14,UI.TEAL))
	var gap:=Control.new();gap.custom_minimum_size.y=18;column.add_child(gap)
	main_view=VBoxContainer.new();main_view.add_theme_constant_override("separation",15);column.add_child(main_view)
	main_actions=HBoxContainer.new();main_actions.add_theme_constant_override("separation",10);main_view.add_child(main_actions)
	game_button=_menu_button("Игра",main_actions,true);game_button.pressed.connect(open_game)
	var settings_button:=_menu_button("Настройки",main_actions);settings_button.pressed.connect(open_settings)
	training_button=_menu_button("Обучение",main_actions);training_button.pressed.connect(func()->void:_show_info("training"))
	about_button=_menu_button("Об игре",main_actions);about_button.pressed.connect(func()->void:_show_info("about"))
	maps_button=_future("Карты",main_actions);online_button=_future("Онлайн",main_actions)
	var quit:=_menu_button("Выйти из игры",main_actions);quit.add_theme_color_override("font_color",UI.CORAL);quit.pressed.connect(func()->void:get_tree().quit())
	game_view=_subpage("Игра")
	var game_actions:VBoxContainer=game_view.get_child(0)
	load_button=_menu_button("Загрузить игру",game_actions);continue_button=load_button;load_button.pressed.connect(func()->void:continue_requested.emit())
	var latest_note:=UI.label("Продолжить с последнего сохранения — ручного или автоматического.",14,UI.MUTED);latest_note.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART;game_actions.add_child(latest_note)
	new_game_button=_menu_button("Новая игра",game_actions,true);new_game_button.pressed.connect(open_difficulty)
	all_saves_button=_menu_button("Все сохранения",game_actions);all_saves_button.pressed.connect(func()->void:saves_requested.emit())
	var game_back:=_menu_button("Назад · Esc",game_actions);game_back.pressed.connect(_back_to_main)
	difficulty_view=_subpage("Новая игра")
	var difficulty_actions:VBoxContainer=difficulty_view.get_child(0)
	var mode:=VBoxContainer.new();mode.add_theme_constant_override("separation",7);difficulty_actions.add_child(mode)
	mode.add_child(UI.label("СЛОЖНОСТЬ",12,UI.MUTED))
	difficulty_picker=OptionButton.new();difficulty_picker.custom_minimum_size.y=46;difficulty_picker.add_theme_font_override("font",UI.SANS);difficulty_picker.add_theme_font_size_override("font_size",19)
	difficulty_picker.add_theme_stylebox_override("normal",UI.panel(.7));mode.add_child(difficulty_picker)
	for id:String in preload("res://scripts/difficulty_profile.gd").IDS:difficulty_picker.add_item(preload("res://scripts/difficulty_profile.gd").NAMES[id])
	difficulty_note=UI.label("",13,UI.MUTED);difficulty_note.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART;difficulty_note.custom_minimum_size.y=42;mode.add_child(difficulty_note)
	difficulty_picker.item_selected.connect(func(i:int)->void:values.difficulty=preload("res://scripts/difficulty_profile.gd").IDS[i];_difficulty_note();settings_changed.emit(values.duplicate()))
	start_button=_menu_button("Начать игру",difficulty_actions,true);start_button.pressed.connect(func()->void:new_game_requested.emit())
	var difficulty_back:=_menu_button("Назад · Esc",difficulty_actions);difficulty_back.pressed.connect(open_game)
	var note:=UI.label("Главный вход заперт. В отделении остался кто-то ещё.",13,UI.MUTED);note.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART;column.add_child(note)
	story_card=PanelContainer.new();story_card.add_theme_stylebox_override("panel",UI.panel(.76));menu_content.add_child(story_card)
	var story:=VBoxContainer.new();story.add_theme_constant_override("separation",19);story_card.add_child(story)
	story.add_child(UI.label("01 / ИСТОРИЯ ВЫЖИВАНИЯ",13,UI.TEAL))
	var title:=UI.heading("Останься в тишине.",37);title.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART;story.add_child(title)
	for copy:String in ["Одна ночь. Два маршрута. Один выход.","Исследуй заброшенное отделение, раскрой судьбу пациентов и сохрани силы для последней двери."]:
		var label:=UI.label(copy,18,UI.MUTED);label.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART;story.add_child(label)
	settings_view=preload("res://scripts/settings_panel.gd").new();root_control.add_child(settings_view)
	settings_view.changed.connect(func(data:Dictionary)->void:values=data;settings_changed.emit(values.duplicate()))
	settings_view.back_requested.connect(_back_to_main)
	settings_view.controls_requested.connect(func()->void:_show_info("controls","settings"))
	controls_button=settings_view.controls_button
	language_picker=settings_view.controls.language;quality_picker=settings_view.controls.texture_quality
	fullscreen=settings_view.controls.fullscreen;camera_motion=settings_view.controls.camera_motion
	effects=settings_view.controls.effects;music=settings_view.controls.music;sensitivity=settings_view.controls.sensitivity
	set_values(values);settings_view.hide();game_view.hide();difficulty_view.hide()
	root_control.resized.connect(_layout);story_card.minimum_size_changed.connect(_layout)
	game_view.minimum_size_changed.connect(_layout);difficulty_view.minimum_size_changed.connect(_layout);_layout()
	visibility_changed.connect(_menu_visibility);hide()
func _layout()->void:
	var size:Vector2=root_control.size
	var left:float=maxf(50,size.x*.055)
	var width:float=size.x-left*2
	menu_scroll.position=Vector2(left,45);menu_scroll.size=Vector2(width,size.y-75)
	for page:PanelContainer in [game_view,difficulty_view]:
		page.position=Vector2(left,70);page.size=Vector2(550,minf(600,size.y-120))
	var card_width:float=minf(620,size.x*.40)
	var card_height:float=maxf(295,story_card.get_combined_minimum_size().y)
	story_card.position=Vector2(size.x-left-card_width,size.y-card_height-50)
	story_card.size=Vector2(card_width,card_height)
func _menu_button(caption:String,parent:Node,primary:bool=false)->Button:
	var b:=UI.button(caption,primary);b.custom_minimum_size.y=64;b.size_flags_horizontal=Control.SIZE_EXPAND_FILL;b.add_theme_font_size_override("font_size",16);parent.add_child(b);return b
func _subpage(title:String)->PanelContainer:
	var page:=PanelContainer.new();page.add_theme_stylebox_override("panel",UI.panel(.96));root_control.add_child(page)
	var content:=VBoxContainer.new();content.add_theme_constant_override("separation",18);page.add_child(content)
	content.add_child(UI.heading(title,36))
	return page
func _future(caption:String,parent:Node)->Button:
	var b:=_menu_button(caption,parent);b.disabled=true;b.tooltip_text="Этот режим находится в разработке."
	b.custom_minimum_size.x=146
	var badge:=UI.label("В РАЗРАБОТКЕ",9,UI.CORAL);badge.set_anchors_and_offsets_preset(Control.PRESET_TOP_RIGHT)
	badge.offset_left=-112;badge.offset_right=-8;badge.offset_top=3;badge.mouse_filter=Control.MOUSE_FILTER_IGNORE;b.add_child(badge);return b
func _difficulty_note()->void:difficulty_note.text=preload("res://scripts/difficulty_profile.gd").description(values.difficulty)
func show_menu(has_checkpoint:bool)->void:
	load_button.disabled=not has_checkpoint
	var game:Node=get_tree().get_first_node_in_group("game")
	if menu_camera==null:
		menu_camera=Camera3D.new();game.add_child(menu_camera);menu_camera.position=Vector3(28,2.2,-27);menu_camera.look_at(Vector3(21,1.1,-34.5));menu_camera.fov=64
	load_button.disabled=not game.saves.can_load_latest()
	_back_to_main();show();_menu_visibility();game_button.grab_focus()
func _menu_visibility()->void:
	if menu_camera==null:return
	menu_camera.visible=visible;menu_camera.current=visible
	if not visible:
		var game:Node=get_tree().get_first_node_in_group("game")
		if game!=null and game.hero!=null:game.hero.camera_rig.camera.current=true
func set_values(data:Dictionary)->void:
	values=Model.normalize(data);settings_view.set_values(values)
	difficulty_picker.select(preload("res://scripts/difficulty_profile.gd").IDS.find(values.difficulty));_difficulty_note()
func open_settings()->void:
	_hide_pages();page_mode="settings";settings_view.show();settings_view.nav_buttons[settings_view.active_section].grab_focus()
func open_game()->void:
	_hide_pages();page_mode="game";game_view.show()
	var game:Node=get_tree().get_first_node_in_group("game")
	load_button.disabled=not game.saves.can_load_latest()
	new_game_button.grab_focus()
func open_difficulty()->void:
	_hide_pages();page_mode="difficulty";difficulty_view.show();difficulty_picker.grab_focus()
func _hide_pages()->void:
	_close_info();menu_content.hide();settings_view.hide();game_view.hide();difficulty_view.hide()
func _show_info(id:String,return_to:String="main")->void:
	_hide_pages();guide_return=return_to;page_mode=id
	info=preload("res://scripts/field_guide.gd").new();info.kind=id;root_control.add_child(info);info.back_requested.connect(_back_from_guide)
func _back_from_guide()->void:
	if guide_return=="settings":open_settings();controls_button.grab_focus()
	else:_back_to_main()
func _close_info()->void:
	if is_instance_valid(info):info.hide();info.queue_free();info=null
func _back_to_main()->void:
	_hide_pages();page_mode="main";menu_content.show()
	if visible:game_button.grab_focus()
func _input(event:InputEvent)->void:
	if visible and event is InputEventKey and event.pressed and event.physical_keycode==KEY_ESCAPE and page_mode!="main":
		var game:Node=get_tree().get_first_node_in_group("game")
		if game!=null and game.saves.screen.mode!="closed":return
		if page_mode=="difficulty":open_game()
		elif page_mode in ["controls","training","about"]:_back_from_guide()
		else:_back_to_main()
		get_viewport().set_input_as_handled()
