extends Node3D

const Builder = preload("res://scripts/world_builder.gd")
var object_id: String = ""
var used: bool = false
var storage: Node3D
var content_visual: Node3D
var indicator: StandardMaterial3D
var status_label: Label3D
var state_value: int = 0
var manual_wheel: Node3D
const NAMES: Dictionary = {"map":"ПЛАН ОТДЕЛЕНИЯ", "fuse":"ПРЕДОХРАНИТЕЛЬ", "power":"АВАРИЙНЫЙ ЩИТОК", "badge":"ПРОПУСК", "terminal":"УПРАВЛЕНИЕ ВЫХОДОМ", "weapon":"СЛУЖЕБНЫЙ ПИСТОЛЕТ"}


func _ready() -> void:
	var metal := Builder.material("394740")
	metal.metallic = .6
	metal.set_meta("surface_kind","metal")
	var paper := Builder.material("bdb59b")
	indicator = Builder.material("d9ad64", 0.35)
	if is_panel():
		Builder.box(self, Vector3(0.85, 1.65, 0.45), Vector3(0, 0.83, 0), metal, true)
		Builder.box(self, Vector3(0.58, 0.36, 0.03), Vector3(0, 1.24, 0.25), indicator)
		for x: float in [-0.25, 0.0, 0.25]:
			Builder.box(self, Vector3(0.06, 0.18, 0.05), Vector3(x, 0.73, 0.26), paper)
		if object_id.begins_with("purge_valve") or object_id=="residence_plate":
			var wheel := TorusMesh.new()
			wheel.inner_radius = .20
			wheel.outer_radius = .25
			var rim := MeshInstance3D.new()
			rim.mesh = wheel
			rim.material_override = indicator
			rim.position = Vector3(0,.9,-.31)
			rim.rotation.x = PI/2
			add_child(rim)
			if object_id=="residence_plate": manual_wheel = rim
			for angle: float in [0,PI/2]:
				var spoke := Builder.box(self,Vector3(.46,.035,.035),Vector3(0,.9,-.31),paper)
				spoke.rotation.z = angle
	elif object_id in ["weapon","map"]:
		preload("res://scripts/imported_props.gd").body(self,Vector3(.85,.74,.6),Vector3(0,.37,0))
		preload("res://scripts/imported_props.gd").visual(self,"painted_wooden_nightstand",Vector3(.85,.74,.6))
		if object_id=="weapon":
			var gun: Node3D = preload("res://scripts/pistol_visual.gd").new()
			gun.rotation.z = PI/2
			gun.position = Vector3(-.08,.762,0)
			add_child(gun)
			content_visual = gun
		else: content_visual = Builder.box(self,Vector3(.63,.015,.44),Vector3(0,.77,0),paper)
	else:
		storage = preload("res://scripts/search_storage.gd").new()
		add_child(storage)
	status_label = Label3D.new()
	status_label.position = Vector3(0, 1.9 if object_id in ["power", "terminal"] else 1.16, 0)
	status_label.font_size = 20
	status_label.pixel_size = 0.008
	status_label.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	if is_panel():
		status_label.position = Vector3(0, 1.55, 0.28)
		status_label.font_size = 16
		status_label.pixel_size = 0.0045
		status_label.billboard = BaseMaterial3D.BILLBOARD_DISABLED
	status_label.modulate = Color("c2bba3")
	status_label.text = display_name()
	add_child(status_label)
	if storage!=null or object_id in ["weapon","map"]: status_label.hide()
	var light := OmniLight3D.new()
	light.position.y = 1.2
	light.light_color = Color("e7c78d")
	light.light_energy = .25 if storage==null else 0
	light.omni_range = 2.3
	add_child(light)


func mark_used() -> void:
	used = true
	if is_panel():
		indicator.albedo_color = Color("89c5a1")
		indicator.emission = indicator.albedo_color
		status_label.text = "ВЫХОД ОТКРЫТ" if object_id == "terminal" else "УЗЕЛ ВКЛЮЧЁН"
		if object_id == "reactor":
			status_label.text = "ОЧИСТКА ЗАВЕРШЕНА"
		if object_id == "residence_plate": status_label.text = "РУЧНОЙ СТОПОР СНЯТ"
		if object_id.begins_with("purge_valve"):
			status_label.text = "ДАВЛЕНИЕ В НОРМЕ"
	elif storage!=null:
		storage.finish()
	elif content_visual!=null:
		content_visual.hide()
		status_label.hide()
	else:
		hide()


func reset_object() -> void:
	if manual_wheel!=null: manual_wheel.rotation.z = 0
	state_value = 0
	used = false
	if storage!=null: storage.reset_storage()
	if content_visual!=null:
		content_visual.show()
		status_label.visible = object_id not in ["weapon","map"]
	show()
	indicator.albedo_color = Color("d9ad64")
	indicator.emission = indicator.albedo_color
	status_label.text = display_name()


func prompt() -> String:
	if storage!=null: return storage.prompt()
	if object_id.begins_with("residence_"):
		return {"residence_panel":"F — переключить питание ЗАПАД / ВОСТОК","residence_prybar":"F — взять монтажный инструмент","residence_core":"V / выстрел — крепление  ·  затем удерживай F — снять модуль","residence_lift":"Удерживай F — установить модуль и снять блокировку","residence_plate":"Удерживай F — освободить ручной стопор"}[object_id]
	if object_id.begins_with("purge_valve"): return "Удерживай F — открыть аварийный вентиль"
	if object_id == "power": return "Удерживай F — установить предохранитель  ·  работа шумит"
	if object_id == "reactor": return "Удерживай F — запустить очистку"
	if object_id in ["coolant", "fuel", "reactor"]:
		return {"coolant": "F — взять хладагент", "fuel": "F — взять топливо", "reactor": "F — запустить очистку"}[object_id]
	if object_id.begins_with("dial_"):
		return "F — переключить цепь %s  [%s]" % [str(int(object_id.right(1)) + 1), ["0", "I", "II", "III"][state_value]]
	if object_id.begins_with("relay_"):
		return "F — проверить схему и включить узел"
	if object_id.begins_with("ammo_"):
		return "F — патроны 9 мм  (+8)"
	if object_id == "weapon":
		return "F — взять служебный пистолет"
	return {"map":"F — взять план отделения", "fuse":"F — взять предохранитель", "power":"F — восстановить питание", "badge":"F — взять пропуск", "terminal":"F — запустить аварийное открытие"}.get(object_id, "")


func is_panel() -> bool:
	return object_id in ["power", "terminal", "reactor","residence_panel","residence_core","residence_lift","residence_plate"] or object_id.begins_with("purge_valve") or object_id.begins_with("relay_") or object_id.begins_with("dial_")


func display_name() -> String:
	if object_id.begins_with("residence_"):
		return {"residence_panel":"ПИТАНИЕ: ОТКЛЮЧЕНО","residence_prybar":"МОНТАЖНЫЙ ИНСТРУМЕНТ","residence_core":"МОДУЛЬ / ОПАСНОЕ НАПРЯЖЕНИЕ","residence_lift":"СБРОС БЛОКИРОВКИ ЛИФТА","residence_plate":"РУЧНОЙ ПРИВОД СТОПОРА"}[object_id]
	if object_id.begins_with("purge_valve"): return "ВЕНТИЛЬ " + ("А" if object_id.ends_with("a") else "Б")
	if object_id in ["coolant", "fuel", "reactor"]:
		return {"coolant":"ХЛАДАГЕНТ", "fuel":"ТОПЛИВО", "reactor":"УПРАВЛЕНИЕ ОЧИСТКОЙ"}[object_id]
	if object_id.begins_with("dial_"):
		return "ЦЕПЬ %d  /  %s" % [int(object_id.right(1)) + 1, ["0", "I", "II", "III"][state_value]]
	if object_id.begins_with("relay_"):
		return "АВАРИЙНЫЙ УЗЕЛ"
	if object_id.begins_with("ammo_"):
		return "9 мм  /  8 ПАТРОНОВ"
	return NAMES.get(object_id, "")


func cycle_dial() -> void:
	state_value = (state_value + 1) % 4
	status_label.text = display_name()
	indicator.albedo_color = Color("8cafa0") if state_value > 0 else Color("d9ad64")

func set_fault(value: bool) -> void:
	indicator.albedo_color = Color("d8523a") if value else Color("89c5a1")
	indicator.emission = indicator.albedo_color
	status_label.text = "СБРОСЬ ДАВЛЕНИЕ / " + display_name() if value else display_name()

func interaction_point() -> Vector3:
	return storage.interaction_point() if storage!=null else global_position+global_basis*Vector3(0,.1,1.05)

func interaction_rids() -> Array[RID]:
	var result: Array[RID] = []
	for node: CollisionObject3D in find_children("*","CollisionObject3D",true,false): result.append(node.get_rid())
	return result
