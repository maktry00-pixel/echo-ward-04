extends CanvasLayer
## A draggable radiograph overlay and an evidence-based treatment console.
var chapter: Node3D
var mode: String = "closed"
var root: Control
var board: Control
var message: Label
var title: Label
var film_offset := Vector2(72,-48)
var film_turn: int = 1
var dragging: bool = false
var buttons: Dictionary = {}
var treatment_steps: Array[String] = []

func configure(owner_chapter: Node3D) -> void:
	chapter = owner_chapter
	process_mode = Node.PROCESS_MODE_ALWAYS
	layer = 28
	root = Control.new()
	root.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(root)
	var shade := ColorRect.new()
	shade.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	shade.color = Color(.015,.025,.03,.96)
	root.add_child(shade)
	var panel := VBoxContainer.new()
	panel.set_anchors_and_offsets_preset(Control.PRESET_CENTER)
	panel.position = Vector2(-370,-330)
	panel.size = Vector2(740,660)
	panel.add_theme_constant_override("separation",12)
	root.add_child(panel)
	title = Label.new()
	title.add_theme_font_size_override("font_size",25)
	panel.add_child(title)
	board = Control.new()
	board.custom_minimum_size = Vector2(740,390)
	board.draw.connect(_draw_board)
	board.gui_input.connect(_board_input)
	panel.add_child(board)
	message = Label.new()
	message.custom_minimum_size.y = 64
	message.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	panel.add_child(message)
	var row := HBoxContainer.new()
	panel.add_child(row)
	for id: String in ["rotate","verify","cool","dose","release","close"]:
		var button := Button.new()
		button.text = {"rotate":"Повернуть ↻","verify":"Совместить","cool":"Охлаждение","dose":"Седативное","release":"Разблокировка","close":"Закрыть / Esc"}[id]
		button.custom_minimum_size.y = 42
		button.pressed.connect(func() -> void: activate(id))
		row.add_child(button)
		buttons[id] = button
	root.hide()

func open_screen(next_mode: String) -> void:
	mode = next_mode
	chapter.game.set_paused(true)
	chapter.game.hud.hide_overlay()
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	root.show()
	dragging = false
	title.text = "РЕНТГЕН · совместить две экспозиции" if mode=="xray" else "ПРОЦЕДУРНАЯ · протокол пациента 04"
	message.text = "Перетащи голубую плёнку мышью. Поверни её так, чтобы три контрольные метки совпали с белыми. Стрелки — точная подстройка; R — поворот." if mode=="xray" else "В карте наблюдения указан безопасный порядок. Ошибочная команда сбрасывает последовательность; препарат не расходуется."
	for id: String in buttons:
		buttons[id].visible = id in (["rotate","verify","close"] if mode=="xray" else ["cool","dose","release","close"])
	board.queue_redraw()

func dismiss(resume: bool = true) -> void:
	if mode=="closed": return
	mode = "closed"
	dragging = false
	root.hide()
	if resume: chapter.game.set_paused(false)

func activate(id: String) -> void:
	if id=="close": dismiss(); return
	if mode=="xray":
		if id=="rotate": film_turn = (film_turn+1)%4
		if id=="verify":
			if film_turn==0 and film_offset.length()<=9:
				chapter.solve_xray()
				dismiss()
			else: message.text = "Экспозиции расходятся. Сопоставь все три метки, а не только центр снимка."
	elif mode=="treatment" and id in ["cool","dose","release"]:
		var wanted: Array[String] = ["cool","dose","release"]
		if id!=wanted[treatment_steps.size()]:
			treatment_steps.clear()
			message.text = "Порядок небезопасен. Протокол сброшен. Проверь карту наблюдения."
		else:
			treatment_steps.append(id)
			message.text = "Протокол: %d / 3" % treatment_steps.size()
			if treatment_steps.size()==3:
				chapter.complete_treatment()
				dismiss()
	board.queue_redraw()

func _unhandled_input(event: InputEvent) -> void:
	if mode=="closed": return
	if event is InputEventKey and event.pressed:
		if event.physical_keycode==KEY_ESCAPE: dismiss()
		elif mode=="xray":
			match event.physical_keycode:
				KEY_R: activate("rotate")
				KEY_LEFT: film_offset.x -= 3
				KEY_RIGHT: film_offset.x += 3
				KEY_UP: film_offset.y -= 3
				KEY_DOWN: film_offset.y += 3
				KEY_ENTER: activate("verify")
			board.queue_redraw()
		get_viewport().set_input_as_handled()

func _board_input(event: InputEvent) -> void:
	if mode!="xray": return
	if event is InputEventMouseButton and event.button_index==MOUSE_BUTTON_LEFT: dragging = event.pressed
	if event is InputEventMouseMotion and dragging:
		film_offset = (film_offset+event.relative).clamp(Vector2(-200,-130),Vector2(200,130))
		board.queue_redraw()

func _draw_board() -> void:
	board.draw_rect(Rect2(0,0,740,390),Color("101f26"))
	if mode=="treatment":
		for i: int in range(3):
			board.draw_circle(Vector2(180+i*190,180),40,Color("80bcad") if treatment_steps.size()>i else Color("334743"))
		return
	var center := Vector2(370,195)
	# Original procedural radiograph: ribs, vertebrae and three registration marks.
	for i: int in range(9):
		var y: float = -118+i*27
		board.draw_line(center+Vector2(-7,y),center+Vector2(7,y),Color("b9cbca"),8,true)
		for side: float in [-1,1]:
			var curve := PackedVector2Array()
			for n: int in range(12):
				var t: float = float(n)/11
				curve.append(center+Vector2(side*(15+sin(t*PI*.8)*(78-i*3)),y+t*30))
			board.draw_polyline(curve,Color(.65,.77,.77,.45),5,true)
	var marks: Array[Vector2] = [Vector2(-110,-115),Vector2(105,-85),Vector2(65,128)]
	for p: Vector2 in marks:
		board.draw_arc(center+p,10,0,TAU,24,Color("efdfbe"),2,true)
		var q: Vector2 = center+film_offset+p.rotated(film_turn*PI/2)
		board.draw_line(q-Vector2(8,0),q+Vector2(8,0),Color("5ac3da"),2,true)
		board.draw_line(q-Vector2(0,8),q+Vector2(0,8),Color("5ac3da"),2,true)
	board.draw_line(center+film_offset+Vector2(-35,-40).rotated(film_turn*PI/2),center+film_offset+Vector2(38,70).rotated(film_turn*PI/2),Color(.2,.8,.9,.65),6,true)
