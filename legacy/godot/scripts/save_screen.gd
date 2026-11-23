extends CanvasLayer
const UI = preload("res://scripts/ui_style.gd")
var manager: Node
var panel: PanelContainer
var body: VBoxContainer
var message: Label
var mode: String = "closed"
var was_paused: bool = false
var was_title: bool = false
var slot_buttons: Dictionary = {}
var backdrop: ColorRect
var close_button:Button
var mode_buttons:Dictionary={}
func _ready() -> void:
	layer = 35
	process_mode = Node.PROCESS_MODE_ALWAYS
	backdrop=ColorRect.new()
	backdrop.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	backdrop.color=Color(0.01,0.02,0.025,.86)
	add_child(backdrop)
	backdrop.hide()
	panel = PanelContainer.new()
	panel.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	panel.offset_left = 140
	panel.offset_right = -140
	panel.offset_top = 75
	panel.offset_bottom = -75
	panel.add_theme_stylebox_override("panel",UI.panel())
	add_child(panel)
	var layout := VBoxContainer.new()
	panel.add_child(layout)
	var scroll := ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	layout.add_child(scroll)
	body = VBoxContainer.new()
	body.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	body.add_theme_constant_override("separation",9)
	scroll.add_child(body)
	var close := UI.button("Вернуться · Esc")
	close_button=close
	close.pressed.connect(dismiss)
	layout.add_child(close)
	panel.hide()
func open_view(kind: String) -> void:
	if mode=="closed":
		was_paused = manager.game.is_paused
		was_title = manager.game.in_title_menu
	mode = kind
	if not was_title:
		manager.game.set_paused(true)
		manager.game.hud.hide_overlay()
	panel.show()
	backdrop.show()
	_render()
func _render(focus_key:String="") -> void:
	for child: Node in body.get_children(): body.remove_child(child); child.queue_free()
	slot_buttons.clear()
	mode_buttons.clear()
	var focus_buttons:Array[Button]=[]
	body.add_child(UI.label("АРХИВ ПРОХОЖДЕНИЯ",28))
	var tabs := HBoxContainer.new()
	body.add_child(tabs)
	for kind: String in ["save","load"]:
		var tab := UI.button("Сохранить" if kind=="save" else "Загрузить",mode==kind)
		tab.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		tab.pressed.connect(func()->void:mode=kind;_render())
		tabs.add_child(tab)
		mode_buttons[kind]=tab
		focus_buttons.append(tab)
	var note := UI.label("Шесть ручных слотов. Автосохранения хранятся отдельно и не перезаписывают их.",15,UI.MUTED)
	note.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	body.add_child(note)
	body.add_child(UI.label("F5 — быстрая запись в слот 1.",14,UI.MUTED))
	for key: String in manager.KEYS:
		if mode=="save" and key.begins_with("auto_"): continue
		var data: Dictionary = manager.slots.get(key,{})
		var row := HBoxContainer.new()
		body.add_child(row)
		var text: String = ("Автосохранение %d" if key.begins_with("auto_") else "Слот %d")%int(key.right(1))
		if data.is_empty():text+=" / Пусто"
		else:
			var room_name:String="ОТДЕЛЕНИЕ 04"
			for room:Dictionary in preload("res://scripts/exploration.gd").ROOMS:
				if room.id==data.get("chapter_name",""):room_name=room.name;break
			var stamp:int=int(data.get("saved_at",0))+int(Time.get_time_zone_from_system().bias)*60
			text+=" / %02d:%02d\n%s · %s"%[int(data.get("elapsed",0))/60,int(data.get("elapsed",0))%60,room_name,Time.get_datetime_string_from_unix_time(stamp).replace("T"," ")]
		var label := UI.label(text,16)
		label.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART
		label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		row.add_child(label)
		var b := UI.button("Записать" if mode=="save" else "Загрузить")
		b.custom_minimum_size.x = 150
		b.disabled = (not manager.eligible()) if mode=="save" else data.is_empty()
		b.pressed.connect(func()->void:
			if mode=="load": manager.load_slot(key)
			else:
				var ok: bool = manager.capture(key)
				_render(key)
				message.text = "Прогресс записан. Можно продолжать." if ok else "Сохранение не выполнено. Проверь доступ к папке игры.")
		row.add_child(b)
		slot_buttons[key] = b
		if not b.disabled:focus_buttons.append(b)
	if mode=="load" and manager.slots.is_empty() and preload("res://scripts/checkpoint_store.gd").valid(manager.game.checkpoint):
		var legacy:=UI.button("Последнее сохранение")
		legacy.pressed.connect(func()->void:manager.game.continue_checkpoint();dismiss(false))
		body.add_child(legacy);focus_buttons.append(legacy)
	message = UI.label("Во время побега, прыжка, укрытия и работы с механизмом запись недоступна." if mode=="save" and not manager.eligible() else "",14,UI.MUTED)
	message.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	body.add_child(message)
	focus_buttons.append(close_button)
	for i:int in range(focus_buttons.size()):
		var b:Button=focus_buttons[i]
		var next:NodePath=focus_buttons[(i+1)%focus_buttons.size()].get_path()
		var previous:NodePath=focus_buttons[posmod(i-1,focus_buttons.size())].get_path()
		b.focus_next=next;b.focus_previous=previous
		b.focus_neighbor_top=previous;b.focus_neighbor_bottom=next
		b.focus_neighbor_left=previous;b.focus_neighbor_right=next
	var focused:Button=slot_buttons.get(focus_key)
	if focused!=null and not focused.disabled:focused.grab_focus()
	elif focus_buttons.size()>3:focus_buttons[2].grab_focus()
	else:focus_buttons[0].grab_focus()
func dismiss(resume: bool = true) -> void:
	if mode=="closed": return
	mode = "closed"
	panel.hide()
	backdrop.hide()
	if resume and not was_title: manager.game.set_paused(was_paused)
	elif resume and was_title:
		manager.game.front_end.open_game()
		manager.game.front_end.all_saves_button.grab_focus()
func _input(event: InputEvent) -> void:
	if mode=="closed": return
	if event is InputEventKey and event.pressed and event.physical_keycode not in [KEY_ESCAPE,KEY_TAB,KEY_UP,KEY_DOWN,KEY_LEFT,KEY_RIGHT,KEY_ENTER,KEY_KP_ENTER,KEY_SPACE,KEY_F11]:
		get_viewport().set_input_as_handled()
	if event is InputEventKey and event.pressed and event.physical_keycode==KEY_ESCAPE:
		dismiss()
		get_viewport().set_input_as_handled()
