extends CanvasLayer
var panel: PanelContainer
var label: Label
var icon: Control
var remaining: float = 0
var phase: float = 0
var success: bool = true
func _ready() -> void:
	layer = 60
	process_mode = Node.PROCESS_MODE_ALWAYS
	panel = PanelContainer.new()
	panel.set_anchors_and_offsets_preset(Control.PRESET_TOP_RIGHT)
	panel.offset_left = -390
	panel.offset_right = -44
	panel.offset_top = 125
	panel.offset_bottom = 180
	panel.add_theme_stylebox_override("panel",preload("res://scripts/ui_style.gd").panel())
	panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(panel)
	var row := HBoxContainer.new()
	row.add_theme_constant_override("separation",12)
	panel.add_child(row)
	icon = Control.new()
	icon.custom_minimum_size = Vector2(25,25)
	icon.draw.connect(_draw_icon)
	row.add_child(icon)
	label = preload("res://scripts/ui_style.gd").label("",15)
	row.add_child(label)
	panel.hide()
func show_result(ok: bool, automatic: bool = false) -> void:
	success = ok
	remaining = 3.5 if ok else 5.0
	phase = 0
	label.text = ("Автосохранение завершено" if automatic else "Игра сохранена") if ok else "Не удалось сохранить игру"
	panel.modulate.a = 1
	panel.show()
func _process(delta: float) -> void:
	if remaining<=0: return
	phase += delta
	remaining = maxf(0,remaining-delta)
	panel.modulate.a = minf(1,remaining*2)
	icon.queue_redraw()
	if remaining<=0: panel.hide()
func _draw_icon() -> void:
	var color := Color("bbd5bc") if success else Color("e4a18a")
	if phase<.35:
		icon.draw_arc(Vector2(12,12),9,phase*12,phase*12+4.8,24,color,2,true)
	elif success:
		icon.draw_polyline(PackedVector2Array([Vector2(3,12),Vector2(10,19),Vector2(23,4)]),color,2.5,true)
	else:
		icon.draw_line(Vector2(5,5),Vector2(20,20),color,2.5,true)
		icon.draw_line(Vector2(20,5),Vector2(5,20),color,2.5,true)
