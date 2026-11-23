extends PanelContainer
signal changed(values:Dictionary)
signal back_requested
signal controls_requested
signal admin_requested
const UI=preload("res://scripts/ui_style.gd")
const Model=preload("res://scripts/presentation_settings.gd")
const GROUPS=["Экран","Графика","Освещение","Звук","Управление","Интерфейс"]
var values:Dictionary=Model.DEFAULTS.duplicate()
var controls:Dictionary={}
var sections:Dictionary={}
var nav_buttons:Dictionary={}
var active_section:String="Экран"
var section_title:Label
var syncing:bool=false
var controls_button:Button
var admin_button:Button
func _ready()->void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	offset_left=48;offset_right=-48;offset_top=38;offset_bottom=-38
	add_theme_stylebox_override("panel",UI.panel(.98))
	var box:=VBoxContainer.new();box.add_theme_constant_override("separation",18);add_child(box)
	var header:=HBoxContainer.new();box.add_child(header)
	var title:=UI.heading("Настройки",36);title.size_flags_horizontal=Control.SIZE_EXPAND_FILL;header.add_child(title)
	controls_button=UI.button("Управление");controls_button.pressed.connect(func()->void:controls_requested.emit());header.add_child(controls_button)
	admin_button=UI.button("Режим админа");admin_button.pressed.connect(func()->void:admin_requested.emit());header.add_child(admin_button)
	var close:=UI.button("Назад · Esc");close.pressed.connect(func()->void:back_requested.emit());header.add_child(close)
	var row:=HBoxContainer.new();row.add_theme_constant_override("separation",28);row.size_flags_vertical=Control.SIZE_EXPAND_FILL;box.add_child(row)
	var nav:=VBoxContainer.new();nav.custom_minimum_size.x=220;row.add_child(nav)
	for name:String in GROUPS:
		var button:=UI.button(name);button.custom_minimum_size.y=52
		button.pressed.connect(func()->void:select_section(name));nav.add_child(button);nav_buttons[name]=button
	var spacer:=Control.new();spacer.size_flags_vertical=Control.SIZE_EXPAND_FILL;nav.add_child(spacer)
	var reset:=UI.button("Сбросить настройки");reset.pressed.connect(_reset);nav.add_child(reset)
	var main:=VBoxContainer.new();main.size_flags_horizontal=Control.SIZE_EXPAND_FILL;row.add_child(main)
	section_title=UI.label("",17,UI.TEAL);main.add_child(section_title)
	var scroll:=ScrollContainer.new();scroll.horizontal_scroll_mode=ScrollContainer.SCROLL_MODE_DISABLED
	scroll.size_flags_vertical=Control.SIZE_EXPAND_FILL;main.add_child(scroll)
	var content:=VBoxContainer.new();content.size_flags_horizontal=Control.SIZE_EXPAND_FILL;scroll.add_child(content)
	for name:String in GROUPS:
		var section:=VBoxContainer.new();section.size_flags_horizontal=Control.SIZE_EXPAND_FILL;section.add_theme_constant_override("separation",18)
		content.add_child(section);sections[name]=section
	_check("Экран","fullscreen","Полный экран  ·  F11")
	_check("Экран","vsync","Вертикальная синхронизация")
	_pick("Экран","max_fps","Ограничение кадров",["Без ограничений","30 FPS","60 FPS","90 FPS","120 FPS","144 FPS","165 FPS"],[0,30,60,90,120,144,165])
	_note("Экран","Изображение заполняет экран при любых пропорциях. F11 переключает оконный режим.")
	_pick("Графика","texture_quality","Качество текстур",["Высокое · 4K PBR","Стандартное · 2K PBR"],["high","standard"])
	_pick("Графика","msaa","Сглаживание",["Выключено","MSAA 2×","MSAA 4×","MSAA 8×"],[0,1,2,3])
	_pick("Графика","shadow_quality","Качество теней",["Выключено","Среднее","Высокое"],["off","medium","high"])
	_check("Графика","grain","Плёночное зерно")
	_check("Графика","vignette","Затемнение краёв")
	_note("Графика","Для слабого ноутбука выбери 2K, MSAA 2× и средние тени. Изменения видны сразу.")
	_slider("Освещение","brightness","Яркость",.65,1.35,.01)
	_slider("Освещение","contrast","Контраст",.8,1.2,.01)
	_slider("Освещение","ambient","Фоновое освещение",.6,1.4,.05)
	_slider("Освещение","fog","Плотность тумана",.5,1.4,.05)
	_slider("Освещение","flashlight_strength","Яркость фонаря",.6,1.4,.05)
	_note("Освещение","Сохрани детали в тенях: тёмная стена должна отличаться от чёрного фона.")
	_slider("Звук","effects","Звуки",0,1,.01)
	_slider("Звук","music","Атмосфера и музыка погони",0,1,.01)
	_note("Звук","Наушники помогут различать шаги, дыхание и направление опасности.")
	_slider("Управление","sensitivity","Чувствительность мыши",.5,2,.05)
	_slider("Управление","fov","Угол обзора",55,85,1)
	_slider("Управление","shake","Тряска камеры",0,1,.05)
	_check("Управление","camera_motion","Покачивание камеры")
	_check("Управление","invert_y","Инверсия мыши по вертикали")
	_pick("Интерфейс","language","Язык интерфейса",["Русский","Қазақша","English"],["ru","kk","en"])
	_slider("Интерфейс","hud_opacity","Непрозрачность интерфейса",.55,1,.05)
	_check("Интерфейс","show_noise","Индикатор шума")
	_check("Интерфейс","objective_hint","Подробная подсказка задачи")
	_check("Интерфейс","crosshair","Прицел")
	_check("Интерфейс","subtitles","Субтитры аудиозаписей")
	select_section(active_section);set_values(values)
func _label(group:String,title:String)->void:sections[group].add_child(UI.label(title,17))
func _note(group:String,text:String)->void:
	var label:=UI.label(text,15,UI.MUTED);label.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART;sections[group].add_child(label)
func _check(group:String,key:String,title:String)->void:
	var button:=CheckButton.new();button.text=title;button.custom_minimum_size.y=44
	button.add_theme_font_override("font",UI.SANS);button.add_theme_font_size_override("font_size",17)
	sections[group].add_child(button);controls[key]=button
	button.toggled.connect(func(value:bool)->void:_change(key,value))
func _pick(group:String,key:String,title:String,captions:Array,options:Array)->void:
	_label(group,title)
	var picker:=OptionButton.new();picker.custom_minimum_size.y=48
	picker.add_theme_font_override("font",UI.SANS);picker.add_theme_font_size_override("font_size",17)
	for caption:String in captions:picker.add_item(caption)
	picker.set_meta("options",options);sections[group].add_child(picker);controls[key]=picker
	picker.item_selected.connect(func(index:int)->void:_change(key,options[index]))
func _slider(group:String,key:String,title:String,low:float,high:float,step:float)->void:
	var row:=HBoxContainer.new();sections[group].add_child(row)
	var title_label:=UI.label(title,17);title_label.size_flags_horizontal=Control.SIZE_EXPAND_FILL;row.add_child(title_label)
	var amount:=UI.label("",15,UI.TEAL);amount.custom_minimum_size.x=70;amount.horizontal_alignment=HORIZONTAL_ALIGNMENT_RIGHT;row.add_child(amount)
	var slider:=HSlider.new();slider.min_value=low;slider.max_value=high;slider.step=step;slider.custom_minimum_size.y=28
	slider.set_meta("amount",amount);slider.set_meta("key",key)
	sections[group].add_child(slider);controls[key]=slider
	slider.value_changed.connect(func(value:float)->void:_amount(slider,value);_change(key,value))
func _amount(slider:HSlider,value:float)->void:
	var label:Label=slider.get_meta("amount")
	label.text="%d°"%int(value) if slider.get_meta("key")=="fov" else "%d%%"%roundi(value*100)
func _change(key:String,value:Variant)->void:
	if syncing:return
	values[key]=value;changed.emit(values.duplicate())
func set_values(data:Dictionary)->void:
	values=Model.normalize(data);syncing=true
	for key:String in controls:
		var control:Control=controls[key]
		if control is OptionButton:control.select(control.get_meta("options").find(values[key]))
		elif control is CheckButton:control.set_pressed_no_signal(values[key])
		elif control is HSlider:control.set_value_no_signal(values[key]);_amount(control,values[key])
	syncing=false
func select_section(name:String)->void:
	if not sections.has(name):return
	active_section=name;section_title.text=name.to_upper()
	for key:String in sections:
		sections[key].visible=key==name
		nav_buttons[key].add_theme_color_override("font_color",UI.TEAL if key==name else UI.PAPER)
func _reset()->void:
	set_values(Model.DEFAULTS);changed.emit(values.duplicate())
