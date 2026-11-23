extends CanvasLayer

signal restart_requested
signal resume_requested
signal menu_requested
signal checkpoint_requested
signal save_requested
const Radar = preload("res://scripts/hunt_radar.gd")
const UI=preload("res://scripts/ui_style.gd")

var objective_label: Label
var ai_label: Label
var hint_label: Label
var overlay: PanelContainer
var overlay_title: Label
var overlay_body: Label
var resume_button: Button
var health_label: Label
var abilities_label: Label
var danger_label: Label
var score_label: Label
var story_label: Label
var story_remaining: float = 0.0
var damage_flash: ColorRect
var radar: Control
var interaction_label: Label
var inventory_label: Label
var body_label: Label
var checkpoint_button: Button
var checkpoint_available: bool = false
var body_scroll: ScrollContainer
var gameplay: Control
var save_button: Button
var staging:Control
var noise_panel:VBoxContainer
var noise_label:Label
var noise_bars:Array[ColorRect]=[]
var noise_level:float=0
var health_bar:ProgressBar
var breath_bar:ProgressBar
var health_number:Label
var breath_number:Label
var quick_cards:Array[PanelContainer]=[]
var quick_counts:Array[Label]=[]
var weapon_panel:PanelContainer
var weapon_label:Label
var story_panel:PanelContainer
var work_panel:PanelContainer
var work_label:Label
var interaction_panel:PanelContainer
var context_text:Label
var context_extra:Label
var boss_panel:VBoxContainer
var preferences:Dictionary=preload("res://scripts/presentation_settings.gd").DEFAULTS.duplicate()
var subtitle_active:bool=false


func _ready()->void:
	process_mode=Node.PROCESS_MODE_ALWAYS
	var root:=Control.new();root.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT);root.mouse_filter=Control.MOUSE_FILTER_IGNORE;add_child(root)
	damage_flash=ColorRect.new();damage_flash.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT);damage_flash.color=Color(.6,.07,.03,0);damage_flash.mouse_filter=Control.MOUSE_FILTER_IGNORE;root.add_child(damage_flash)
	gameplay=Control.new();gameplay.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT);gameplay.mouse_filter=Control.MOUSE_FILTER_IGNORE;root.add_child(gameplay)
	staging=Control.new();root.add_child(staging);staging.hide()
	var task:=VBoxContainer.new();task.position=Vector2(44,36);task.size=Vector2(620,180);task.add_theme_constant_override("separation",8);gameplay.add_child(task)
	task.add_child(UI.label("ЗАДАЧА",12,UI.GOLD))
	objective_label=UI.heading("",27);objective_label.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART;task.add_child(objective_label)
	hint_label=_label("",15,UI.MUTED);hint_label.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART;task.add_child(hint_label)
	inventory_label=_label("",12,UI.MUTED);staging.add_child(inventory_label)
	health_label=_label("",13,UI.PAPER);staging.add_child(health_label)
	abilities_label=_label("",13,UI.PAPER);staging.add_child(abilities_label)
	body_label=_label("",12,UI.MUTED);staging.add_child(body_label)
	score_label=_label("",12,UI.GOLD);staging.add_child(score_label)
	ai_label=_label("",12,UI.TEAL);ai_label.position=Vector2(44,225);ai_label.size=Vector2(600,95);gameplay.add_child(ai_label);ai_label.hide()
	noise_panel=VBoxContainer.new();noise_panel.set_anchors_and_offsets_preset(Control.PRESET_TOP_RIGHT);noise_panel.offset_left=-300;noise_panel.offset_right=-44;noise_panel.offset_top=38;noise_panel.offset_bottom=105;noise_panel.add_theme_constant_override("separation",11);gameplay.add_child(noise_panel)
	var noise_header:=HBoxContainer.new();noise_panel.add_child(noise_header)
	var caption:=UI.label("ШУМ",12,UI.MUTED);caption.size_flags_horizontal=Control.SIZE_EXPAND_FILL;noise_header.add_child(caption)
	noise_label=UI.label("ТИХО",12,UI.GOLD);noise_header.add_child(noise_label)
	var bars:=HBoxContainer.new();bars.add_theme_constant_override("separation",5);noise_panel.add_child(bars)
	for i:int in range(6):
		var bar:=ColorRect.new();bar.custom_minimum_size=Vector2(34,14);bar.size_flags_horizontal=Control.SIZE_EXPAND_FILL;bars.add_child(bar);noise_bars.append(bar)
	var vital:=VBoxContainer.new();vital.set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_LEFT);vital.offset_left=44;vital.offset_right=344;vital.offset_top=-150;vital.offset_bottom=-40;vital.add_theme_constant_override("separation",9);gameplay.add_child(vital)
	var health_row:=HBoxContainer.new();vital.add_child(health_row)
	var health_tag:=UI.label("ЗДОРОВЬЕ",12,UI.MUTED);health_tag.size_flags_horizontal=Control.SIZE_EXPAND_FILL;health_row.add_child(health_tag)
	health_number=UI.label("100",12);health_row.add_child(health_number)
	health_bar=_bar(UI.PAPER,3);vital.add_child(health_bar)
	var breath_row:=HBoxContainer.new();vital.add_child(breath_row)
	var breath_tag:=UI.label("ДЫХАНИЕ",12,UI.MUTED);breath_tag.size_flags_horizontal=Control.SIZE_EXPAND_FILL;breath_row.add_child(breath_tag)
	breath_number=UI.label("100",12,UI.GOLD);breath_row.add_child(breath_number)
	breath_bar=_bar(UI.GOLD,100);vital.add_child(breath_bar)
	var quick:=HBoxContainer.new();quick.set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_RIGHT);quick.offset_left=-470;quick.offset_right=-44;quick.offset_top=-132;quick.offset_bottom=-40;quick.add_theme_constant_override("separation",10);gameplay.add_child(quick)
	for item:Array in [["B","Фонарь"],["H","Аптечка"],["T","Батарея"],["I","Рюкзак"]]:
		var card:=PanelContainer.new();var style:=UI.panel(.86);style.set_content_margin_all(10);card.add_theme_stylebox_override("panel",style);card.size_flags_horizontal=Control.SIZE_EXPAND_FILL;quick.add_child(card);quick_cards.append(card)
		var stack:=VBoxContainer.new();stack.add_theme_constant_override("separation",8);card.add_child(stack)
		stack.add_child(UI.label(item[0],11,UI.MUTED))
		var name_label:=UI.label(item[1],12);name_label.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART;name_label.size_flags_vertical=Control.SIZE_EXPAND_FILL;stack.add_child(name_label)
		var count_label:=UI.label("",12,UI.GOLD);stack.add_child(count_label);quick_counts.append(count_label)
	weapon_panel=PanelContainer.new();weapon_panel.set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_RIGHT);weapon_panel.offset_left=-360;weapon_panel.offset_right=-44;weapon_panel.offset_top=-237;weapon_panel.offset_bottom=-150
	var weapon_style:=UI.panel(.8);weapon_style.set_content_margin_all(12);weapon_panel.add_theme_stylebox_override("panel",weapon_style);gameplay.add_child(weapon_panel)
	weapon_label=_label("",18,UI.PAPER);weapon_label.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART;weapon_label.horizontal_alignment=HORIZONTAL_ALIGNMENT_RIGHT;weapon_panel.add_child(weapon_label)
	story_panel=PanelContainer.new();story_panel.set_anchors_and_offsets_preset(Control.PRESET_CENTER_BOTTOM);story_panel.offset_left=-320;story_panel.offset_right=320;story_panel.offset_top=-200;story_panel.offset_bottom=-90
	var subtitle_style:=UI.panel(.72);subtitle_style.set_content_margin_all(12);subtitle_style.set_border_width_all(0);story_panel.add_theme_stylebox_override("panel",subtitle_style);gameplay.add_child(story_panel)
	story_label=_label("",16,UI.PAPER);story_label.horizontal_alignment=HORIZONTAL_ALIGNMENT_CENTER;story_label.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART;story_panel.add_child(story_label);story_panel.hide()
	danger_label=_label("",16,UI.CORAL);danger_label.set_anchors_and_offsets_preset(Control.PRESET_CENTER_TOP);danger_label.offset_left=-280;danger_label.offset_right=280;danger_label.offset_top=38;danger_label.offset_bottom=90;danger_label.horizontal_alignment=HORIZONTAL_ALIGNMENT_CENTER;gameplay.add_child(danger_label)
	interaction_panel=PanelContainer.new();interaction_panel.add_theme_stylebox_override("panel",UI.panel(.9));interaction_panel.size=Vector2(400,0);gameplay.add_child(interaction_panel)
	interaction_label=_label("",17,UI.PAPER);staging.add_child(interaction_label)
	var context_box:=VBoxContainer.new();interaction_panel.add_child(context_box)
	var context_row:=HBoxContainer.new();context_row.add_theme_constant_override("separation",13);context_box.add_child(context_row)
	context_row.add_child(UI.label("F",19,UI.GOLD))
	context_text=_label("",17,UI.PAPER);context_text.set_meta("loc_manual",true);context_text.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART;context_text.size_flags_horizontal=Control.SIZE_EXPAND_FILL;context_row.add_child(context_text);interaction_panel.hide()
	context_extra=_label("",13,UI.GOLD);context_extra.set_meta("loc_manual",true);context_extra.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART;context_box.add_child(context_extra);context_extra.hide()
	work_panel=PanelContainer.new();work_panel.set_anchors_and_offsets_preset(Control.PRESET_CENTER_BOTTOM);work_panel.offset_left=-330;work_panel.offset_right=330;work_panel.offset_top=-310;work_panel.offset_bottom=-220;work_panel.add_theme_stylebox_override("panel",UI.panel(.88));gameplay.add_child(work_panel)
	work_label=_label("",16,UI.GOLD);work_label.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART;work_label.horizontal_alignment=HORIZONTAL_ALIGNMENT_CENTER;work_panel.add_child(work_label);work_panel.hide()
	boss_panel=VBoxContainer.new();boss_panel.set_anchors_and_offsets_preset(Control.PRESET_CENTER_TOP);boss_panel.offset_left=-290;boss_panel.offset_right=290;boss_panel.offset_top=215;boss_panel.offset_bottom=295;gameplay.add_child(boss_panel)
	for control:Node in gameplay.find_children("*","Control",true,false):control.mouse_filter=Control.MOUSE_FILTER_IGNORE
	radar=Radar.new();radar.set_anchors_and_offsets_preset(Control.PRESET_TOP_RIGHT);radar.offset_left=-1030;radar.offset_right=-32;radar.offset_top=36;radar.offset_bottom=720;radar.z_index=100;radar.mouse_filter=Control.MOUSE_FILTER_IGNORE;root.add_child(radar);radar.hide()
	overlay=PanelContainer.new();overlay.set_anchors_and_offsets_preset(Control.PRESET_CENTER);overlay.offset_left=-270;overlay.offset_right=270;overlay.offset_top=-280;overlay.offset_bottom=280;overlay.add_theme_stylebox_override("panel",UI.panel());root.add_child(overlay)
	var panel_column:=VBoxContainer.new();panel_column.add_theme_constant_override("separation",16);overlay.add_child(panel_column)
	overlay_title=UI.heading("ПАУЗА",32);panel_column.add_child(overlay_title)
	overlay_body=_label("",17,UI.PAPER);_build_body_scroll(panel_column)
	resume_button=UI.button("Продолжить",true);resume_button.pressed.connect(func()->void:resume_requested.emit());panel_column.add_child(resume_button)
	var restart:=UI.button("Начать заново");restart.pressed.connect(func()->void:restart_requested.emit());panel_column.add_child(restart)
	checkpoint_button=UI.button("Загрузить контрольную точку");checkpoint_button.pressed.connect(func()->void:checkpoint_requested.emit());panel_column.add_child(checkpoint_button)
	save_button=UI.button("Сохранить игру");save_button.pressed.connect(func()->void:save_requested.emit());panel_column.add_child(save_button)
	var menu:=UI.button("Главное меню");menu.pressed.connect(func()->void:menu_requested.emit());panel_column.add_child(menu);overlay.hide()

func _bar(color:Color,maximum:float)->ProgressBar:
	var bar:=ProgressBar.new();bar.max_value=maximum;bar.value=maximum;bar.show_percentage=false;bar.custom_minimum_size.y=9
	var fill:=StyleBoxFlat.new();fill.bg_color=color;bar.add_theme_stylebox_override("fill",fill)
	var back:=StyleBoxFlat.new();back.bg_color=Color("21383c");bar.add_theme_stylebox_override("background",back);return bar
func _build_body_scroll(column: VBoxContainer) -> void:
	body_scroll = ScrollContainer.new()
	body_scroll.custom_minimum_size.y = 110
	body_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	column.add_child(body_scroll)
	overlay_body.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	overlay_body.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	body_scroll.add_child(overlay_body)


func set_body_state(stamina: float, exhausted: bool, light_on: bool, tension: float) -> void:
	breath_bar.value=stamina
	breath_number.text=str(roundi(stamina))
	body_label.text = "Дыхание: %s   ·   B  Фонарь: %s" % ["переведи дух" if exhausted else ("тяжёлое" if stamina < 30 else "ровное"), "включён" if light_on else "выключен"]
	body_label.modulate = Color("d0ac95") if tension > 0.6 or exhausted else Color.WHITE


func update_objective(count: int, total: int) -> void:
	objective_label.text = "Найденные записи   %d / %d" % [count, total]
	hint_label.text = "Аварийный выход открыт. Уходи." if count == total else "Восстанови питание аварийного выхода."


func set_interaction(message: String,target:Node3D=null) -> void:
	interaction_label.text = message
	context_text.text=TextLocale.translate(message).trim_prefix("F — ")
	context_extra.hide()
	if target!=null:
		var storage:Node=target.get("storage")
		if storage!=null and storage.opened and not storage.ready_for_loot:
			context_extra.text=TextLocale.translate(storage.label.text);context_extra.show()
	interaction_panel.visible=not message.is_empty()
	if message.is_empty():return
	var anchor:Vector2=gameplay.size*Vector2(.5,.62)
	var camera:Camera3D=get_viewport().get_camera_3d()
	if target!=null and camera!=null:
		var at:Vector3=target.global_position+Vector3(0,1.05,0)
		if camera.is_position_behind(at):interaction_panel.hide();return
		anchor=camera.unproject_position(at)
	interaction_panel.size=Vector2(400,0)
	interaction_panel.position=Vector2(clampf(anchor.x-200,30,gameplay.size.x-430),clampf(anchor.y+30,150,gameplay.size.y-350))


func set_mission(title: String, hint: String, inventory: String, count: int, total: int) -> void:
	objective_label.text = title
	hint_label.text = hint
	inventory_label.text = inventory + "   ·   Записи %d/%d" % [count, total]


func update_hunt(health: int, charges: int, cooldown: float, score: int,
		combo: int, nearest: float, grace: float, escape: float, best: int) -> void:
	health_label.text = "Состояние: " + ["критическое", "тяжёлое ранение", "ранен", "стабильное"][clampi(health, 0, 3)]
	health_bar.value=health
	health_number.text=str(roundi(clampf(float(health)/3,0,1)*100))
	var dash: String = "ГОТОВ" if cooldown <= 0 else "%.1f с" % cooldown
	abilities_label.text = "Q  Уклонение: %s     ·     E  Вспышка: %d" % [dash.to_lower(), charges]
	score_label.text = "СЧЁТ  %d     РЕКОРД  %d" % [score, best]
	if combo > 1:
		score_label.text += "     РЕЗОНАНС ×%d" % combo
	if escape >= 0:
		danger_label.text = "ДО ЗАКРЫТИЯ ВЫХОДА: %02d с" % ceili(escape)
	elif nearest < 8:
		danger_label.text = "Слышны шаги..."
	else:
		danger_label.text = ""
	danger_label.visible=escape>=0
	danger_label.modulate = Color("ff9a83") if nearest < 5 or escape >= 0 else Color.WHITE


func announce(message: String, seconds: float = 5.0,subtitle:bool=false) -> void:
	story_label.text = message
	story_remaining = seconds
	subtitle_active=subtitle


func set_ai_debug(text: String) -> void:
	ai_label.text = text


func flash_damage() -> void:
	damage_flash.color.a = 0.35


func _process(delta: float) -> void:
	story_panel.visible=not story_label.text.is_empty() and (not subtitle_active or preferences.subtitles)
	var game:Node=get_tree().get_first_node_in_group("game")
	if game!=null and game.inventory!=null:
		quick_counts[0].text="●" if game.hero.camera_rig.flashlight_enabled else "—"
		quick_counts[1].text="×%d"%game.inventory.medkits
		quick_counts[2].text="×%d"%game.inventory.batteries
		quick_counts[3].text="%d / 8"%game.inventory.entries().size()
		weapon_panel.visible=game.combat.equipped and not game.in_title_menu
		var work:String=""
		if game.hero.hiding_bed!=null or game.survival_tools.action_object!=null or not game.diagnostics.work_id.is_empty():work=game.survival_tools.status.text
		elif game.upgrades.work_door!=null:work=game.upgrades.status.text
		elif not game.isolation.work_id.is_empty() or not game.isolation.fault.is_empty():work=game.isolation.status.text
		elif game.annex!=null and not game.annex.work_id.is_empty():work=game.annex.status.text
		work_label.text=work;work_panel.visible=not work.is_empty()
		if work_panel.visible:interaction_panel.hide()
	noise_level=maxf(0,noise_level-delta*.6)
	var lit:int=ceili(noise_level*6)
	for i:int in range(noise_bars.size()):noise_bars[i].color=UI.CORAL if i<lit and lit>=5 else UI.GOLD if i<lit else Color("203a3e")
	noise_label.text="ГРОМКО" if lit>=5 else "СЛЫШНО" if lit>=3 else "ТИХО"
	if get_tree().paused:
		return
	damage_flash.color.a = maxf(0, damage_flash.color.a - delta * 0.8)
	story_remaining = maxf(0, story_remaining - delta)
	if story_remaining <= 0:
		story_label.text = ""

func note_noise(radius:float)->void:noise_level=maxf(noise_level,clampf(radius/14,0,1))
func apply_preferences(data:Dictionary)->void:
	preferences=data.duplicate()
	gameplay.modulate.a=data.hud_opacity
	noise_panel.visible=data.show_noise
	hint_label.visible=data.objective_hint


func show_overlay(won: bool) -> void:
	if won: gameplay.hide(); radar.hide()
	overlay.offset_left = -250
	overlay.offset_right = 250
	overlay.offset_top = -260
	overlay.offset_bottom = 260
	body_scroll.custom_minimum_size.y = 110
	checkpoint_button.visible = checkpoint_available and not won
	overlay_body.add_theme_font_size_override("font_size", 17)
	overlay.show()
	overlay_title.text = "ТЫ ВЫБРАЛСЯ" if won else "ПАУЗА"
	overlay_title.modulate = Color.WHITE
	overlay_body.text = "Аварийная блокировка снята. Ты покинул отделение." if won else "Мышь освобождена. Можно перевести дух."
	resume_button.visible = not won
	save_button.visible = not won
	if not won:
		resume_button.grab_focus()


func show_journal(entries: Array[String]) -> void:
	overlay.offset_left = -410
	overlay.offset_right = 410
	overlay.offset_top = -360
	overlay.offset_bottom = 360
	body_scroll.custom_minimum_size.y = 350
	checkpoint_button.hide()
	save_button.hide()
	overlay_title.text = "НАЙДЕННЫЕ ЗАПИСИ"
	overlay_body.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	overlay_body.add_theme_font_size_override("font_size", 15)
	overlay_body.text = "Записи ещё не найдены. Ищи кассеты на тумбах и нажимай F." if entries.is_empty() else "\n\n".join(entries)
	resume_button.show()
	resume_button.grab_focus()


func show_defeat(reason: String, score: int) -> void:
	overlay.offset_left = -250
	overlay.offset_right = 250
	overlay.offset_top = -260
	overlay.offset_bottom = 260
	body_scroll.custom_minimum_size.y = 110
	checkpoint_button.visible = checkpoint_available
	save_button.hide()
	overlay_body.add_theme_font_size_override("font_size", 17)
	overlay.show()
	overlay_title.text = "ТЕБЯ НАШЛИ"
	overlay_title.modulate = Color("f3a08b")
	overlay_body.text = "%s\nСчёт: %d. Попробуй другой маршрут." % [reason, score]
	overlay_body.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	resume_button.hide()


func reset_feedback() -> void:
	gameplay.show()
	damage_flash.color.a = 0
	story_label.text = ""
	story_remaining = 0
	interaction_label.text = ""
	radar.hide()


func hide_overlay() -> void:
	overlay.hide()


func _label(text: String, size: int, color: Color) -> Label:
	var result := UI.label(text,size,color)
	result.add_theme_color_override("font_shadow_color", Color(0.02, 0.04, 0.07, 0.9))
	result.add_theme_constant_override("shadow_offset_x", 2)
	result.add_theme_constant_override("shadow_offset_y", 2)
	result.mouse_filter = Control.MOUSE_FILTER_IGNORE
	return result
