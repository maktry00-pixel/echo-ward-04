extends CanvasLayer
## Pause-safe inventory, mouse-driven inspection and an actual keypad input flow.
var inventory: Node
var panel: PanelContainer
var title: Label
var grid: GridContainer
var description: Label
var inspector: SubViewportContainer
var inventory_view: HBoxContainer
var keypad_view: VBoxContainer
var keypad_label: Label
var selected: String = ""
var selected_amount: int = 0
var code_input: String = ""
var mode: String = "closed"
var target_door: Node3D
var use_button: Button
var drop_button: Button
var keypad_buttons: Dictionary = {}
func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	layer = 25
	panel = PanelContainer.new()
	panel.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	panel.offset_left = 50
	panel.offset_right = -50
	panel.offset_top = 55
	panel.offset_bottom = -55
	var style := StyleBoxFlat.new()
	style.bg_color = Color("111b20")
	style.set_border_width_all(2)
	style.border_color = Color("66776f")
	style.content_margin_left = 24
	style.content_margin_right = 24
	style.content_margin_top = 20
	style.content_margin_bottom = 20
	panel.add_theme_stylebox_override("panel",style)
	add_child(panel)
	var column := VBoxContainer.new()
	column.add_theme_constant_override("separation",16)
	panel.add_child(column)
	title = Label.new()
	title.add_theme_font_size_override("font_size",26)
	column.add_child(title)
	inventory_view = HBoxContainer.new()
	inventory_view.size_flags_vertical = Control.SIZE_EXPAND_FILL
	inventory_view.add_theme_constant_override("separation",24)
	column.add_child(inventory_view)
	var left := VBoxContainer.new()
	left.custom_minimum_size.x = 350
	inventory_view.add_child(left)
	grid = GridContainer.new()
	grid.columns = 2
	left.add_child(grid)
	var tip := Label.new()
	tip.text = "Документы и план не занимают ячейки.\nБоеприпасы: 24 патрона в ячейке.\nH — аптечка · T — батарея\nПрипасы можно оставить и забрать снова."
	tip.add_theme_font_size_override("font_size",14)
	left.add_child(tip)
	var right := VBoxContainer.new()
	right.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	inventory_view.add_child(right)
	inspector = preload("res://scripts/item_inspector.gd").new()
	inspector.inventory = inventory
	inspector.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	inspector.size_flags_vertical = Control.SIZE_EXPAND_FILL
	right.add_child(inspector)
	description = Label.new()
	description.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	right.add_child(description)
	var actions := HBoxContainer.new()
	right.add_child(actions)
	_button(actions,"Повернуть / пробел",func() -> void: inspector.flip())
	use_button = _button(actions,"Применить",_use_selected)
	drop_button = _button(actions,"Оставить",_drop_selected)
	keypad_view = VBoxContainer.new()
	keypad_view.alignment = BoxContainer.ALIGNMENT_CENTER
	keypad_view.size_flags_vertical = Control.SIZE_EXPAND_FILL
	column.add_child(keypad_view)
	keypad_label = Label.new()
	keypad_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	keypad_label.add_theme_font_size_override("font_size",30)
	keypad_view.add_child(keypad_label)
	var center := CenterContainer.new()
	keypad_view.add_child(center)
	var digits := GridContainer.new()
	digits.columns = 3
	center.add_child(digits)
	for digit: String in ["1","2","3","4","5","6","7","8","9","←","0","ВВОД"]:
		var button: Button = _button(digits,digit,func() -> void: keypad_key(digit))
		button.custom_minimum_size = Vector2(100,58)
		keypad_buttons[digit] = button
	_button(column,"Вернуться в игру · Esc / I",func() -> void: dismiss())
	panel.hide()

func _button(parent: Node, text: String, callback: Callable) -> Button:
	var button := Button.new()
	button.text = text
	button.custom_minimum_size.y = 42
	button.pressed.connect(callback)
	parent.add_child(button)
	return button

func open_inventory(focus: String = "") -> void:
	if not _begin(): return
	mode = "inventory"
	inventory_view.show()
	keypad_view.hide()
	_refresh(focus)

func open_keypad(door: Node3D) -> void:
	if not _begin(): return
	mode = "keypad"
	target_door = door
	code_input = ""
	inspector.stop()
	title.text = "КАРАНТИН / КОДОВЫЙ ЗАМОК"
	inventory_view.hide()
	keypad_view.show()
	keypad_label.text = "_ _ _\nКод начальника смены записан на обороте пропуска."

func _begin() -> bool:
	var game: Node3D = inventory.game
	if game.has_won or game.has_lost or game.in_title_menu or (game.is_paused and mode=="closed"): return false
	if mode=="closed": game.set_paused(true)
	game.hud.overlay.hide()
	panel.show()
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	return true

func _refresh(focus: String = "") -> void:
	for child: Node in grid.get_children(): child.free()
	var items: Array[Dictionary] = inventory.entries()
	title.text = "СНАРЯЖЕНИЕ   %d / 8" % items.size()
	for i: int in range(8):
		var item: Dictionary = items[i] if i<items.size() else {}
		var text: String = "%02d · ПУСТО" % (i+1) if item.is_empty() else "%02d · %s\n× %d" % [i+1,item.name,item.count]
		var button: Button = _button(grid,text,func() -> void: _select(item.id,int(item.count))) if not item.is_empty() else _button(grid,text,func() -> void: pass)
		button.set_meta("item_id",item.get("id",""))
		button.toggle_mode = true
		button.custom_minimum_size = Vector2(170,74)
		button.disabled = item.is_empty()
	var id: String = focus if not focus.is_empty() else selected
	if not items.any(func(item: Dictionary) -> bool: return item.id==id): id = items[0].id if not items.is_empty() else ""
	_select(id)

func _select(id: String, amount: int = 0) -> void:
	selected = id
	for button: Button in grid.get_children(): button.button_pressed = button.get_meta("item_id","")==id
	selected_amount = amount
	if id.is_empty():
		inspector.stop()
		description.text = "Инвентарь пуст."
		return
	inspector.show_item(id)
	description.text = "Удерживай ЛКМ — вращать · колесо — приблизить."
	if id=="badge": description.text += "\nПропуск принадлежал начальнику смены. Проверь обе стороны."
	elif id=="medkit": description.text += "\nМожно сохранить до ранения. H — применить в игре."
	elif id=="battery": description.text += "\nВосстанавливает один заряд вспышки. T — применить в игре."
	elif id in ["ammo","shells","boards","bottle"]: description.text += "\nМожно оставить у ног и забрать позже."
	else: description.text += "\nОружие и ключевые инструменты остаются при герое."
	use_button.disabled = id not in ["medkit","battery","weapon","shotgun"]
	drop_button.disabled = id not in ["medkit","battery","ammo","bottle","shells","boards"]
	if id=="shotgun": description.text += "\nZ — сменить оружие. R — заряжать по одному; выстрел прерывает заряжание."
	if id=="boards": description.text += "\nЗакрой обычную дверь. Удерживай K — укрепить или разобрать."

func _use_selected() -> void:
	inventory.use_item(selected)
	_refresh()
func _drop_selected() -> void:
	inventory.drop_item(selected,selected_amount)
	_refresh()

func keypad_key(key: String) -> bool:
	if mode!="keypad": return false
	if key=="←": code_input = code_input.left(maxi(0,code_input.length()-1))
	elif key=="ВВОД":
		if code_input=="417" and inventory.game.mission.has_badge:
			inventory.quarantine_code = true
			target_door.locked = false
			target_door.toggle_door(inventory.game.hero.position)
			inventory.game.sound_bank.play_cue("door")
			dismiss()
			inventory.game.hud.announce("Код принят. Карантин открыт.",5)
			return true
		code_input = ""
		keypad_label.text = "КОД НЕ ПРИНЯТ\nОсмотри оборот пропуска: I → пропуск → повернуть."
		return false
		return false
	elif key.is_valid_int() and code_input.length()<3: code_input += key
	keypad_label.text = (code_input+"___").left(3)+"\nВвод — подтвердить · ← — стереть"
	return false

func dismiss(resume: bool = true) -> void:
	mode = "closed"
	panel.hide()
	inspector.stop()
	if resume and inventory.game.is_paused: inventory.game.set_paused(false)

func _input(event: InputEvent) -> void:
	if mode=="closed" or not event is InputEventKey or not event.pressed or event.echo: return
	if event.physical_keycode in [KEY_ESCAPE,KEY_I]: dismiss()
	elif mode=="inventory" and event.physical_keycode==KEY_SPACE: inspector.flip()
	elif mode=="keypad":
		if event.physical_keycode==KEY_ENTER or event.physical_keycode==KEY_KP_ENTER: keypad_key("ВВОД")
		elif event.physical_keycode==KEY_BACKSPACE: keypad_key("←")
		elif event.physical_keycode>=KEY_0 and event.physical_keycode<=KEY_9: keypad_key(str(event.physical_keycode-KEY_0))
	get_viewport().set_input_as_handled()
