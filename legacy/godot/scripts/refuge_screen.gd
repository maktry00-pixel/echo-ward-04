extends CanvasLayer
var refuge: Node3D
var panel: PanelContainer
var body: VBoxContainer
var footer: VBoxContainer
var message: Label
var mode: String = "closed"
var transfer_buttons: Dictionary = {}
var save_button: Button
func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	layer = 26
	panel = PanelContainer.new()
	panel.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	panel.offset_left = 140
	panel.offset_right = -140
	panel.offset_top = 130
	panel.offset_bottom = -130
	var style := StyleBoxFlat.new()
	style.bg_color = Color("1d2423")
	style.border_color = Color("a58c65")
	style.set_border_width_all(2)
	style.content_margin_left = 28
	style.content_margin_right = 28
	style.content_margin_top = 24
	style.content_margin_bottom = 24
	panel.add_theme_stylebox_override("panel",style)
	add_child(panel)
	body = VBoxContainer.new()
	var layout := VBoxContainer.new()
	panel.add_child(layout)
	var scroll := ScrollContainer.new()
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	layout.add_child(scroll)
	body.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	body.add_theme_constant_override("separation",14)
	scroll.add_child(body)
	footer = VBoxContainer.new()
	footer.add_theme_constant_override("separation",8)
	layout.add_child(footer)
	panel.hide()
func _button(text: String, callback: Callable, fixed: bool = false) -> Button:
	var button := Button.new()
	button.text = text
	button.custom_minimum_size.y = 44
	button.pressed.connect(callback)
	(footer if fixed else body).add_child(button)
	return button
func _label(text: String, size: int = 18) -> Label:
	var label := Label.new()
	label.text = text
	label.add_theme_font_size_override("font_size",size)
	label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	body.add_child(label)
	return label
func open_view(kind: String) -> void:
	var game: Node3D = refuge.game
	if not refuge.contains(game.hero.position) or game.has_won or game.has_lost or (game.is_paused and mode=="closed"): return
	mode = kind
	game.set_paused(true)
	game.hud.overlay.hide()
	panel.show()
	_render()
func _render() -> void:
	for child: Node in footer.get_children(): footer.remove_child(child); child.queue_free()
	for child: Node in body.get_children():
		body.remove_child(child)
		child.queue_free()
	transfer_buttons.clear()
	_label("ДЕЖУРНАЯ / "+{"save":"ЖУРНАЛ СМЕНЫ","stash":"ШКАФ ПРИПАСОВ","records":"ЗАПИСИ ДЕЖУРНОГО"}[mode],26)
	if mode=="stash":
		_label("С собой: %d / 8 ячеек. Шкаф сохраняется вместе с игрой.\nОружие и ключевые предметы остаются при герое." % refuge.game.inventory.entries().size(),16)
		for kind: String in ["ammo","medkit","battery","bottle","shells","boards"]:
			var names := {"ammo":"Патроны","medkit":"Аптечки","battery":"Батареи","bottle":"Бутылки","shells":"12 калибр","boards":"Доски"}
			_label("%s: с собой %d  /  в шкафу %d" % [names[kind],refuge._carried(kind),int(refuge.stash[kind])],18)
			var row := HBoxContainer.new()
			body.add_child(row)
			for deposit: bool in [true,false]:
				var button := Button.new()
				button.text = ("Положить" if deposit else "Взять")+(" до 24" if kind=="ammo" else "")
				button.custom_minimum_size = Vector2(180,36)
				button.disabled = refuge._carried(kind)<=0 if deposit else int(refuge.stash[kind])<=0
				button.pressed.connect(func() -> void:
					var success: bool = refuge.transfer(kind,deposit)
					_render()
					if not success: message.text = "Нет свободного места или запас этого предмета уже полон.")
				row.add_child(button)
				transfer_buttons[kind+("_deposit" if deposit else "_take")] = button
	elif mode=="records":
		_label("17 октября. Нас осталось двое. Мы укрепили порог дежурной: сюда они не заходят. Дверь можно оставить открытой.\n\nЗапасной предохранитель — в правой палате. В архиве лежит пропуск начальника смены; он всегда писал важное на обороте.\n\nНе трать всё на убийства. Повреждённая нога даст время уйти. Перед рывком Хранитель замирает — беги в сторону.\n\nЕсли вернёшься, оставь сообщение в журнале. Я буду ждать у окна.",18)
		_label("Найденные кассеты можно перечитать клавишей J после выхода из этого окна.",15)
	else:
		_label("Записать текущий прогресс и содержимое шкафа. Продолжение начнётся здесь, в дежурной.\nСохранение не лечит и не добавляет патронов. Во время финального побега запись недоступна.",19)
	message = _label("",16)
	message.size_flags_vertical = Control.SIZE_EXPAND_FILL
	save_button = _button("Сохранить игру в журнал",_save,true)
	save_button.disabled = refuge.game.mission.exit_started
	_button("Вернуться · Esc",func() -> void: dismiss(),true)
func _save() -> void:
	var success: bool = refuge.game.save_checkpoint(true)
	message.text = "Запись сохранена. Снаряжение и шкаф включены." if success else "Сейчас сохранить не удалось."
func dismiss(resume: bool = true) -> void:
	mode = "closed"
	panel.hide()
	if resume and refuge.game.is_paused: refuge.game.set_paused(false)
func _input(event: InputEvent) -> void:
	if mode=="closed" or not event is InputEventKey or not event.pressed: return
	if event.physical_keycode==KEY_ESCAPE: dismiss()
	get_viewport().set_input_as_handled()
